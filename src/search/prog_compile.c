#include <string.h>

#ifdef HAS_CONFIG_H
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <libpq-fe.h>
#include <glib.h>

#include "interpreter.h"
#include "utils.h"
#include "constants.h"
#include "comm.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "race.h"
#include "creature.h"
#include "db.h"
#include "tmpstr.h"
#include "spells.h"
#include "prog.h"

enum prog_token_kind {
    PROG_TOKEN_EOL,
    PROG_TOKEN_SYM,
    PROG_TOKEN_STR,
    PROG_TOKEN_HANDLER,
};

struct prog_token {
    enum prog_token_kind kind;  // kind of prog_token
    struct prog_token *next;    // next prog_token in list
    int linenum;                // line number this token occurs in
    // prog_token data
    union {
        char *str;
        char *sym;
    };
};

struct prog_code_block {
    struct prog_code_block *next;
    unsigned short code_seg[MAX_STRING_LENGTH];
    unsigned short *code_pt;
    char data_seg[MAX_STRING_LENGTH];
    char *data_pt;
};

struct prog_compiler_state {
    struct creature *ch;        // Player doing the compiling
    char *prog_text;            // Text to be compiled
    void *owner;                // Owner of the prog
    enum prog_evt_type owner_type;  // Owner type of the prog
    struct prog_token *token_list;  // The prog converted to a list of tokens
    struct prog_token *token_tail;  // End of list of tokens, for appending
    struct prog_token *cur_token;   // The token under inspection
    struct prog_code_block *code;   // The current code block being compiled
    struct prog_code_block *handlers[PROG_PHASE_COUNT][PROG_EVT_COUNT]; // entry table for event handlers
    int error;                  // true if a fatal error has occurred
};

const char *prog_phase_strs[] = { "before", "handle", "after", "\n" };

const char *prog_event_strs[] = {
    "command", "idle", "fight", "give", "chat", "enter", "leave", "load",
    "tick", "spell", "combat", "death", "dying", "\n"
};

// Function prototypes
void prog_compile_error(struct prog_compiler_state *compiler,
    int linenum, const char *str, ...)
    __attribute__ ((format(printf, 3, 4)));
void prog_compile_warning(struct prog_compiler_state *compiler,
    int linenum, const char *str, ...)
    __attribute__ ((format(printf, 3, 4)));

char *
prog_get_text(void *owner, enum prog_evt_type owner_type)
{
    switch (owner_type) {
    case PROG_TYPE_OBJECT:
        break;
    case PROG_TYPE_MOBILE:
        if (owner) {
            return GET_NPC_PROG(((struct creature *)owner));
        } else {
            errlog("Mobile Prog with no owner - Can't happen at %s:%d",
                __FILE__, __LINE__);
            return NULL;
        }
    case PROG_TYPE_ROOM:
        return ((struct room_data *)owner)->prog;
    default:
        errlog("Can't happen at %s:%d", __FILE__, __LINE__);
    }
    return NULL;
}

void
prog_compile_message(struct prog_compiler_state *compiler,
    int level, int linenum, const char *str, va_list args)
{
    const char *severity = "";
    const char *place = NULL;
    const char *linestr = "";
    char *msg;

    msg = tmp_vsprintf(str, args);

    if (linenum)
        linestr = tmp_sprintf(", line %d", linenum);

    switch (compiler->owner_type) {
    case PROG_TYPE_MOBILE:
        place = tmp_sprintf("mobile %d%s",
            GET_NPC_VNUM(((struct creature *)compiler->owner)), linestr);
        break;
    case PROG_TYPE_ROOM:
        place = tmp_sprintf("room %d%s",
            ((struct room_data *)compiler->owner)->number, linestr);
        break;
    default:
        place = "an unknown location";
    }

    switch (level) {
    case 0:
        severity = "notice";
        break;
    case 1:
        severity = "warning";
        break;
    case 2:
        severity = "error";
        break;
    }

    if (compiler->ch)
        send_to_char(compiler->ch, "Prog %s in %s: %s\r\n",
            severity, place, msg);
    else
        slog("Prog %s in %s: %s", severity, place, msg);
}

void
prog_compile_error(struct prog_compiler_state *compiler,
    int linenum, const char *str, ...)
{
    va_list args;

    va_start(args, str);
    prog_compile_message(compiler, 2, linenum, str, args);
    va_end(args);

    compiler->error = 1;
}

void
prog_compile_warning(struct prog_compiler_state *compiler,
    int linenum, const char *str, ...)
{
    va_list args;

    va_start(args, str);
    prog_compile_message(compiler, 1, linenum, str, args);
    va_end(args);
}

bool
prog_push_new_token(struct prog_compiler_state *compiler,
    enum prog_token_kind kind, int linenum, const char *value)
{
    struct prog_token *new_token;

    CREATE(new_token, struct prog_token, 1);
    if (!new_token) {
        prog_compile_error(compiler,
            compiler->cur_token->linenum,
            "Out of memory while creating symbol");
        return false;
    }

    new_token->kind = kind;
    new_token->linenum = linenum;
    switch (kind) {
    case PROG_TOKEN_SYM:
        new_token->sym = tmp_strdup(value);
        break;
    case PROG_TOKEN_STR:
        new_token->str = tmp_strdup(value);
        break;
    case PROG_TOKEN_HANDLER:
        new_token->sym = tmp_strdup(value);
        break;
    case PROG_TOKEN_EOL:
        // EOL has no data
        break;
    default:
        errlog("Can't happen");
        prog_compile_error(compiler,
            compiler->cur_token->linenum, "Internal Compiler Error #194");
        free(new_token);
        return false;
    }

    if (compiler->token_tail)
        compiler->token_tail = compiler->token_tail->next = new_token;
    else
        compiler->token_list = compiler->token_tail = new_token;

    return true;
}

void
prog_free_compiler(struct prog_compiler_state *compiler)
{
    struct prog_token *next_token, *cur_token;
    struct prog_code_block *cur_code, *next_code;
    int phase_idx, event_idx;

    for (cur_token = compiler->token_list; cur_token; cur_token = next_token) {
        next_token = cur_token->next;
        free(cur_token);
    }

    for (phase_idx = 0; phase_idx < PROG_PHASE_COUNT; phase_idx++)
        for (event_idx = 0; event_idx < PROG_EVT_COUNT; event_idx++)
            for (cur_code = compiler->handlers[phase_idx][event_idx];
                cur_code; cur_code = next_code) {
                next_code = cur_code->next;
                free(cur_code);
            }
}

//
// prog_lexify
//
// Given the prog text, return a list of prog_tokens.  Returns NULL
// on error.
bool
prog_lexify(struct prog_compiler_state *compiler)
{
    const char *cmd_str;
    char *line_start, *line_end, *line;
    int linenum;

    line_start = line_end = compiler->prog_text;
    linenum = 1;
    compiler->token_list = compiler->token_tail = NULL;
    line = tmp_strdup("");

    // Skip initial spaces and blank lines in prog
    while (isspace(*line_start))
        line_start++;
    line_end = line_start;

    while (line_start && *line_start) {
        // Find the end of the line
        while (*line_end && *line_end != '\n' && *line_end != '\r')
            line_end++;

        // Grab a temporary version of the line
        line =
            tmp_strcat(line, tmp_substr(line_start, 0,
                line_end - line_start - 1), NULL);
        // If the line ends with a backslash, do nothing and the next
        // line will be appended.  If it doesn't, we can process the
        // line now
        if (line_end != line_start && *(line_end - 1) == '\\') {
            line[strlen(line) - 1] = '\0';
        } else if (line[0] == '\0' || line[0] == '-') {
            // comment or blank line - ignore this line
            line = tmp_strdup("");
        } else {
            // We have an actual line, so we need to lexify it.  Currently,
            // the lexical format is just a symbol followed by a string.
            // We'll improve things once we start parsing arguments
            bool ok = true;
            if (*line == '*') {
                cmd_str = tmp_getword(&line) + 1;
                if (!strcasecmp(cmd_str, "before")
                    || !strcasecmp(cmd_str, "handle")
                    || !strcasecmp(cmd_str, "after"))
                    ok = prog_push_new_token(compiler,
                        PROG_TOKEN_HANDLER, linenum, cmd_str);
                else
                    ok = prog_push_new_token(compiler,
                        PROG_TOKEN_SYM, linenum, cmd_str);
                if (!ok)
                    return false;
            }
            // if we have an argument to the command, tokenize it
            if (*line)
                if (!prog_push_new_token(compiler, PROG_TOKEN_STR, linenum,
                        line))
                    return false;

            // The end-of-line is a language token in progs
            if (!prog_push_new_token(compiler, PROG_TOKEN_EOL, linenum, line))
                return false;

            // Set line to empty string so it won't be concatenated again
            line = tmp_strdup("");
        }

        while (isspace(*line_end)) {
            if (*line_end == '\n')
                linenum++;
            line_end++;
        }
        line_start = line_end;
    }

    compiler->cur_token = compiler->token_list;

    return true;
}

void
prog_compiler_emit(struct prog_compiler_state *compiler, int instr, void *data,
    size_t data_len)
{
    *compiler->code->code_pt++ = instr;
    if (data_len) {
        *compiler->code->code_pt++ =
            compiler->code->data_pt - compiler->code->data_seg;
        memcpy(compiler->code->data_pt, data, data_len);
        compiler->code->data_pt += data_len;
    } else {
        *compiler->code->code_pt++ = 0;
    }
}

void
prog_compile_statement(struct prog_compiler_state *compiler)
{
    struct prog_command *cmd;

    switch (compiler->cur_token->kind) {
    case PROG_TOKEN_EOL:
        // Ignore blank statements
        break;
    case PROG_TOKEN_SYM:
        // Find the prog command
        cmd = prog_cmds + 1;
        while (cmd->str && strcasecmp(cmd->str, compiler->cur_token->sym))
            cmd++;
        if (!cmd->str) {
            prog_compile_error(compiler,
                compiler->cur_token->linenum,
                "Unknown command '%s'", compiler->cur_token->sym);
            return;
        }
        // Advance to the next token
        compiler->cur_token = compiler->cur_token->next;

        if (compiler->cur_token && compiler->cur_token->kind == PROG_TOKEN_STR) {
            prog_compiler_emit(compiler,
                cmd - prog_cmds,
                compiler->cur_token->str,
                strlen(compiler->cur_token->str) + 1);
            compiler->cur_token = compiler->cur_token->next;
        } else {
            prog_compiler_emit(compiler, cmd - prog_cmds, NULL, 0);
        }
        break;
    case PROG_TOKEN_STR:
        if (compiler->owner_type != PROG_TYPE_MOBILE) {
            prog_compile_error(compiler, compiler->cur_token->linenum,
                "Non-mobs cannot perform in-game commands.");
            return;
        }
        // It's an in-game command with a mob, so emit the *do command
        prog_compiler_emit(compiler,
            PROG_CMD_DO,
            compiler->cur_token->str, strlen(compiler->cur_token->str) + 1);
        compiler->cur_token = compiler->cur_token->next;
        break;
    default:
        errlog("Can't happen");
    }

    if (compiler->cur_token) {
        if (compiler->cur_token->kind != PROG_TOKEN_EOL) {
            prog_compile_error(compiler, compiler->cur_token->linenum,
                "Expected end of line");
            return;
        }
        compiler->cur_token = compiler->cur_token->next;
    }
}

void
prog_compile_handler(struct prog_compiler_state *compiler)
{
    struct prog_token *cmd_token;
    int phase, event, cmd;
    char *arg, *args;

    // Make sure the code starts with a handler
    if (compiler->cur_token->kind != PROG_TOKEN_HANDLER) {
        prog_compile_error(compiler, compiler->cur_token->linenum,
            "Command without handler");
        return;
    }
    // Set up new code block
    CREATE(compiler->code, struct prog_code_block, 1);
    compiler->code->next = NULL;
    compiler->code->code_pt = compiler->code->code_seg;
    compiler->code->data_pt = compiler->code->data_seg;

    // A data point of 0 should return a null string
    *compiler->code->data_pt++ = '\0';

    // Retrieve the handler phase
    cmd_token = compiler->cur_token;
    phase = search_block(cmd_token->sym, prog_phase_strs, true);
    switch (phase) {
    case PROG_EVT_BEGIN:
        prog_compiler_emit(compiler, PROG_CMD_BEFORE, NULL, 0);
        break;
    case PROG_EVT_HANDLE:
        prog_compiler_emit(compiler, PROG_CMD_HANDLE, NULL, 0);
        break;
    case PROG_EVT_AFTER:
        prog_compiler_emit(compiler, PROG_CMD_AFTER, NULL, 0);
        break;
    default:
        slog("Can't happen at %s:%d", __FILE__, __LINE__);
    }

    // Advance to the next token, which should be a STR
    compiler->cur_token = cmd_token->next;
    if (!compiler->cur_token || compiler->cur_token->kind != PROG_TOKEN_STR) {
        prog_compile_error(compiler, cmd_token->linenum,
            "Expected parameter to *%s", cmd_token->sym);
        goto err;
    }
    // Retrieve the event type
    args = compiler->cur_token->str;
    arg = tmp_getword(&args);
    event = search_block(arg, prog_event_strs, true);
    if (event < 0) {
        prog_compile_error(compiler, compiler->cur_token->linenum,
            "Invalid parameter '%s' to *%s", arg, cmd_token->sym);
        goto err;
    }
    // Add code to handle any other conditions attached to the handler
    switch (event) {
    case PROG_EVT_COMMAND:
        arg = tmp_getword(&args);
        if (!*arg) {
            prog_compile_error(compiler, compiler->cur_token->linenum,
                "No command specified for *%s command", cmd_token->sym);
            goto err;
        }
        prog_compiler_emit(compiler, PROG_CMD_CLRCOND, NULL, 0);
        while (*arg) {
            cmd = find_command(arg);
            if (cmd < 0) {
                prog_compile_error(compiler, compiler->cur_token->linenum,
                    "'%s' is not a valid command", arg);
                goto err;
            }
            prog_compiler_emit(compiler, PROG_CMD_CMPCMD, &cmd, sizeof(int));

            arg = tmp_getword(&args);
        }

        prog_compiler_emit(compiler, PROG_CMD_CONDNEXTHANDLER, NULL, 0);
        break;
    case PROG_EVT_SPELL:
        arg = tmp_getword(&args);
        if (!*arg) {
            prog_compile_error(compiler, compiler->cur_token->linenum,
                "No spell numbers specified for *%s spell", cmd_token->sym);
            goto err;
        }
        prog_compiler_emit(compiler, PROG_CMD_CLRCOND, NULL, 0);
        while (*arg) {
            if (!is_number(arg)) {
                prog_compile_error(compiler, compiler->cur_token->linenum,
                    "Spell number expected, got '%s'", arg);
                goto err;
            }
            cmd = atoi(arg);
            if (cmd < 0 || cmd > max_spell_num || spells[cmd][0] == '!') {
                prog_compile_error(compiler, compiler->cur_token->linenum,
                    "%s is not a valid spell number", arg);
                goto err;
            }

            prog_compiler_emit(compiler, PROG_CMD_CMPCMD, &cmd, sizeof(int));

            arg = tmp_getword(&args);
        }

        prog_compiler_emit(compiler, PROG_CMD_CONDNEXTHANDLER, NULL, 0);
        break;
    case PROG_EVT_GIVE:
        arg = tmp_getword(&args);
        if (*arg) {
            if (!is_number(arg)) {
                prog_compile_error(compiler, compiler->cur_token->linenum,
                    "Object vnum expected, got '%s'", arg);
                goto err;
            }
            cmd = atoi(arg);
            if (cmd < 0) {
                prog_compile_error(compiler, compiler->cur_token->linenum,
                    "%s is not a valid object vnum", arg);
                goto err;
            }
            if (!real_object_proto(cmd))
                prog_compile_warning(compiler, compiler->cur_token->linenum,
                    "No object %s exists.", arg);

            prog_compiler_emit(compiler, PROG_CMD_CLRCOND, NULL, 0);
            prog_compiler_emit(compiler, PROG_CMD_CMPOBJVNUM, &cmd,
                sizeof(int));
            prog_compiler_emit(compiler, PROG_CMD_CONDNEXTHANDLER, NULL, 0);

        }
        break;
    default:
        break;
    }

    // Ensure there's another token to get to
    if (!compiler->cur_token->next) {
        prog_compile_error(compiler, compiler->cur_token->linenum,
            "End of line expected.");
        goto err;
    }
    // Advance token and ensure that it's an end of line
    compiler->cur_token = compiler->cur_token->next;

    if (compiler->cur_token->kind != PROG_TOKEN_EOL) {
        prog_compile_error(compiler, compiler->cur_token->linenum,
            "End of line expected.");
        goto err;
    }
    // Compile statements until an error or the next event handler
    compiler->cur_token = compiler->cur_token->next;
    while (compiler->cur_token
        && compiler->cur_token->kind != PROG_TOKEN_HANDLER && !compiler->error)
        prog_compile_statement(compiler);

    // If an entry in the event handler table already exists, append
    // the new code to the end
    if (compiler->handlers[phase][event]) {
        struct prog_code_block *prev_block;

        prev_block = compiler->handlers[phase][event];
        while (prev_block->next)
            prev_block = prev_block->next;
        prev_block->next = compiler->code;
    } else {
        // otherwise, just add an entry to the table
        compiler->handlers[phase][event] = compiler->code;
    }

    compiler->code = NULL;
    return;

  err:
    free(compiler->code);
    compiler->code = NULL;
}

void
prog_display_obj(struct creature *ch, unsigned char *exec)
{
    int cmd, arg_addr, read_pt;
    int cmd_count;
    int phase_idx, event_idx;

    cmd_count = 0;
    while (prog_cmds[cmd_count].str)
        cmd_count++;

    for (phase_idx = 0; phase_idx < PROG_PHASE_COUNT; phase_idx++)
        for (event_idx = 0; event_idx < PROG_EVT_COUNT; event_idx++) {
            read_pt =
                *((short *)exec + phase_idx * PROG_EVT_COUNT + event_idx);
            if (read_pt) {
                send_to_char(ch, "-- %s %s -------------------------\r\n",
                    tmp_toupper(prog_phase_strs[phase_idx]),
                    tmp_toupper(prog_event_strs[event_idx]));
                while (read_pt >= 0 && *((short *)&exec[read_pt])) {
                    // Get the command and the arg address
                    cmd = *((short *)(exec + read_pt));
                    arg_addr = *((short *)(exec + read_pt) + 1);
                    // Set the execution point to the next command by default
                    read_pt += sizeof(short) * 2;
                    if (cmd < 0 || cmd >= cmd_count)
                        send_to_char(ch, "<INVALID CMD #%d>\r\n", cmd);
                    else
                        send_to_char(ch, "%-9s %s\n",
                            tmp_toupper(prog_cmds[cmd].str),
                            (char *)(exec + arg_addr));
                }
            }
        }
}

unsigned char *
prog_map_to_block(struct prog_compiler_state *compiler)
{
    struct prog_code_block *cur_code;
    unsigned char *block;
    unsigned short *code_pt;
    int code_len, data_len, block_len;
    int phase_idx, event_idx;
    int code_offset, data_offset;

    // Find the amount of space we'll need for the final block
    block_len = PROG_PHASE_COUNT * PROG_EVT_COUNT * sizeof(short);
    code_len = 0;
    data_len = 0;

    // Add all code and data segments from all event handlers
    for (phase_idx = 0; phase_idx < PROG_PHASE_COUNT; phase_idx++)
        for (event_idx = 0; event_idx < PROG_EVT_COUNT; event_idx++) {
            if (compiler->handlers[phase_idx][event_idx]) {
                for (cur_code = compiler->handlers[phase_idx][event_idx];
                    cur_code; cur_code = cur_code->next) {
                    code_len +=
                        (cur_code->code_pt -
                        cur_code->code_seg) * sizeof(unsigned short);
                    data_len += (cur_code->data_pt - cur_code->data_seg);
                }
                code_len += 2;  // add size of halt command
            }
        }

    // Add length for the end of prog marker
    code_len += 2;
    block_len += code_len * sizeof(unsigned short) + data_len;

    // Allocate result block and clear it
    CREATE(block, unsigned char, block_len);

    // Assign initial offsets
    code_offset = PROG_PHASE_COUNT * PROG_EVT_COUNT * sizeof(short);
    data_offset = code_offset + code_len * sizeof(unsigned short);

    // Iterate through event handling table
    for (phase_idx = 0; phase_idx < PROG_PHASE_COUNT; phase_idx++)
        for (event_idx = 0; event_idx < PROG_EVT_COUNT; event_idx++) {
            // Go to next event handler if no code attached to it
            if (!compiler->handlers[phase_idx][event_idx])
                continue;
            // Assign current code offset to event table entry
            *((short *)block + phase_idx * PROG_EVT_COUNT + event_idx) =
                code_offset;

            // Concatenate and relocate code and data segments
            for (cur_code = compiler->handlers[phase_idx][event_idx];
                cur_code; cur_code = cur_code->next) {
                // Copy the data segment
                memcpy(&block[data_offset], cur_code->data_seg,
                    cur_code->data_pt - cur_code->data_seg);
                // Backpatch the data addresses
                for (code_pt = cur_code->code_seg + 1;
                    code_pt < cur_code->code_pt; code_pt += 2)
                    *code_pt += data_offset;
                // Copy the code segment
                memcpy(&block[code_offset], cur_code->code_seg,
                    (cur_code->code_pt - cur_code->code_seg) * sizeof(short));

                // Update code and data offsets
                code_offset +=
                    (cur_code->code_pt - cur_code->code_seg) * sizeof(short);
                data_offset += (cur_code->data_pt - cur_code->data_seg);
            }

            // Concatenate halt code for end of event handler
            *((short *)&block[code_offset]) = PROG_CMD_ENDOFPROG;
            *((short *)&block[code_offset + sizeof(short)]) = 0;
            code_offset += 2 * sizeof(short);
        }

    // Concatenate end-of-prog marker to code.
    *((short *)&block[code_offset]) = PROG_CMD_ENDOFPROG;
    *((short *)&block[code_offset + sizeof(short)]) = 0;

    return block;
}

unsigned char *
prog_compile_prog(struct creature *ch,
    char *prog_text, void *owner, enum prog_evt_type owner_type)
{
    struct prog_compiler_state state;
    unsigned char *obj = NULL;

    if (!prog_text || !*prog_text)
        return NULL;

    memset(&state, 0, sizeof(state));

    state.ch = ch;
    state.prog_text = prog_text;
    state.owner = owner;
    state.owner_type = owner_type;
    state.code = NULL;

    // Get a list of tokens from the prog
    if (!prog_lexify(&state))
        goto error;

    // We can have a prog that doesn't do anything... in those cases,
    // we should just clear the prog
    if (!state.token_list)
        return NULL;

    // Run through tokens until no more tokens are left or a fatal
    // compiler error occurs.
    state.error = 0;
    while (state.cur_token && !state.error)
        prog_compile_handler(&state);

    if (!state.error) {
        // No error occurred - map entry table, object code, and data to
        // a single memory block...
        obj = prog_map_to_block(&state);
    } else {
        // An error occurred - set the result to NULL
        obj = NULL;
    }

  error:

    // Free compiler structures
    prog_free_compiler(&state);

    // We can potentially use a LOT of temporary string space in our
    // compilation, so we gc it here.
    tmp_gc_strings();

    return obj;
}

void
prog_compile(struct creature *ch, void *owner, enum prog_evt_type owner_type)
{
    char *prog;
    unsigned char *obj;

    // Get the prog
    prog = prog_get_text(owner, owner_type);

    // Compile the prog, if one exists.
    obj = (prog) ? prog_compile_prog(ch, prog, owner, owner_type) : NULL;

    // Set the object code of the owner
    switch (owner_type) {
    case PROG_TYPE_MOBILE:
        free(((struct creature *)owner)->mob_specials.shared->progobj);
        ((struct creature *)owner)->mob_specials.shared->progobj = obj;
        break;
    case PROG_TYPE_ROOM:
        free(((struct room_data *)owner)->progobj);
        ((struct room_data *)owner)->progobj = obj;
        break;
    case PROG_TYPE_OBJECT:
        break;
    default:
        errlog("Can't happen at %s:%u", __FILE__, __LINE__);
    }

    // Kill all the progs the owner has to avoid invalid code
    destroy_attached_progs(owner);
}
