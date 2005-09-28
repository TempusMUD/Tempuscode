#include <string.h>

#include "constants.h"
#include "comm.h"
#include "creature.h"
#include "prog.h"
#include "utils.h"

enum prog_token_kind {
    PROG_TOKEN_EOL,
    PROG_TOKEN_SYM,
    PROG_TOKEN_STR,
};

struct prog_token {
    prog_token_kind kind;   // kind of prog_token
    prog_token *next;       // next prog_token in list
    int linenum;            // line number this token occurs in
    // prog_token data
    union {
        char *str;
        char *sym;
    };
};

struct prog_compiler_state {
    Creature *ch;
    char *prog_text;
    void *owner;
    prog_evt_type owner_type;
    prog_token *token_list;
    prog_token *cur_token;
    unsigned short *codeseg;
    unsigned short *code_pt;
    char *dataseg;
    char *data_pt;
    int error;
};


char *
prog_get_text(void *owner, prog_evt_type owner_type)
{
	switch (owner_type) {
	case PROG_TYPE_OBJECT:
		break;
	case PROG_TYPE_MOBILE:
		if ((Creature *)owner) {
			return GET_MOB_PROG(((Creature *)owner));
		} else {
			errlog("Mobile Prog with no owner - Can't happen at %s:%d",
				__FILE__, __LINE__);
			return NULL;
		}
	case PROG_TYPE_ROOM:
		return ((room_data *)owner)->prog;
    default:
        errlog("Can't happen at %s:%d", __FILE__, __LINE__);
	}
	return NULL;
}


void
prog_report_compile_err(prog_compiler_state *compiler,
                        int linenum,
                        const char *str,
                        ...)
{
    const char *place = NULL;
    const char *linestr = "";
    char *msg;
    va_list args;

    va_start(args, str);
    msg = tmp_vsprintf(str, args);
    va_end(args);

    if (linenum)
        linestr = tmp_sprintf(", line %d", linenum);

    switch (compiler->owner_type) {
    case PROG_TYPE_MOBILE:
        place = tmp_sprintf("mobile %d%s",
                            GET_MOB_VNUM((Creature *)compiler->owner),
                            linestr);
        break;
    case PROG_TYPE_ROOM:
        place = tmp_sprintf("room %d%s",
                            ((room_data *)compiler->owner)->number,
                            linestr);
        break;
    default:
        place = "an unknown location";
    }
    if (compiler->ch)
        send_to_char(compiler->ch, "Prog error in %s: %s\r\n", place, msg);
    else
        slog("Prog error in %s: %s", place, msg);

    compiler->error = 1;
}

prog_token *
prog_create_token(prog_token_kind kind, int linenum, const char *value)
{
    prog_token *new_token;

    new_token = new prog_token;
    if (!new_token)
        return NULL;
    memset(new_token, 0, sizeof(prog_token));

    new_token->kind = kind;
    new_token->linenum = linenum;
    switch (kind) {
    case PROG_TOKEN_SYM:
        new_token->sym = strdup(value); break;
    case PROG_TOKEN_STR:
        new_token->str = strdup(value); break;
    case PROG_TOKEN_EOL:
        // EOL has no data
        break;
    default:
        errlog("Can't happen");
        return NULL;
    }

    return new_token;
}

void
prog_free_tokens(prog_token *token_list)
{
    prog_token *next_token, *cur_token;

    for (cur_token = token_list;cur_token;cur_token = next_token) {
        next_token = cur_token->next;
        if (cur_token->kind == PROG_TOKEN_SYM ||
            cur_token->kind == PROG_TOKEN_STR)
            free(cur_token->str);
        free(cur_token);
    }
}

//
// prog_lexify
//
// Given the prog text, return a list of prog_tokens.  Returns NULL
// on error.
bool
prog_lexify(prog_compiler_state *compiler)
{
    const char *cmd_str;
    char *line_start, *line_end, *line;
    int linenum;
    prog_token *prev_token, *new_token;

    line_start = line_end = compiler->prog_text;
    linenum = 1;
    compiler->token_list = prev_token = NULL;
    line = "";

    // Skip initial spaces and blank lines in prog
    while (isspace(*line_start))
        line_start++;
    line_end = line_start;

    while (line_start && *line_start) {
        // Find the end of the line
        while (*line_end && *line_end != '\n' && *line_end != '\r')
            line_end++;

        // Grab a temporary version of the line
        line = tmp_strcat(line, tmp_substr(line_start, 0, line_end - line_start - 1));
        // If the line ends with a backslash, do nothing and the next
        // line will be appended.  If it doesn't, we can process the
        // line now
        if (line_end != line_start && *(line_end - 1) == '\\') {
            line[strlen(line) - 1] = '\0';
        } else if (line[0] == '\0' || line[0] == '-') {
            // comment or blank line - ignore this line
            line = "";
        } else {
            // We have an actual line, so we need to lexify it.  Currently,
            // the lexical format is just a symbol followed by a string.
            // We'll improve things once we start parsing arguments
            
            // First, get the command symbol
                                          
            if (*line == '*') {
                cmd_str = tmp_getword(&line) + 1;
                new_token = prog_create_token(PROG_TOKEN_SYM, linenum, cmd_str);
                if (!new_token) {
                    prog_report_compile_err(compiler,
                                            compiler->cur_token->linenum,
                                            "Out of memory",
                                            compiler->cur_token->sym);
                    return false;
                }
                if (prev_token)
                    prev_token->next = new_token;
                else
                    compiler->token_list = new_token;
                prev_token = new_token;
            }

            // if we have an argument to the command, tokenize it
            if (*line) {
                new_token = prog_create_token(PROG_TOKEN_STR, linenum, line);
                if (!new_token) {
                    prog_report_compile_err(compiler,
                                            compiler->cur_token->linenum,
                                            "Out of memory",
                                            compiler->cur_token->sym);
                    return false;
                }

                if (prev_token)
                    prev_token->next = new_token;
                else
                    compiler->token_list = new_token;
                prev_token = new_token;
            }

            // The end-of-line is a language token in progs
            new_token = prog_create_token(PROG_TOKEN_EOL, linenum, line);
            if (!new_token) {
                    prog_report_compile_err(compiler,
                                            compiler->cur_token->linenum,
                                            "Out of memory",
                                            compiler->cur_token->sym);
                return false;
            }
            prev_token->next = new_token;
            prev_token = new_token;

            // Set line to empty string so it won't be concatenated again
            line = "";
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
prog_compile_statement(prog_compiler_state *compiler)
{
    prog_command *cmd;

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
            prog_report_compile_err(compiler,
                                    compiler->cur_token->linenum,
                                    "Unknown command '%s'",
                                    compiler->cur_token->sym);
            return;
        }
        
        // Emit prog command code
        *compiler->code_pt++ = (cmd - prog_cmds);

        // Advance to the next token
        compiler->cur_token = compiler->cur_token->next;

        if (compiler->cur_token && compiler->cur_token->kind == PROG_TOKEN_STR) {
            *compiler->code_pt++ = compiler->data_pt - compiler->dataseg;
            strcpy(compiler->data_pt, compiler->cur_token->str);
            compiler->data_pt += strlen(compiler->data_pt) + 1;
              
            compiler->cur_token = compiler->cur_token->next;
        } else {
            *compiler->code_pt++ = 0;
        }
        break;
    case PROG_TOKEN_STR:
        if (compiler->owner_type != PROG_TYPE_MOBILE) {
            prog_report_compile_err(compiler, compiler->cur_token->linenum,
                                    "Non-mobs cannot perform in-game commands.");
            return;
        }

        // It's an in-game command with a mob, so emit the *do command
        *compiler->code_pt++ = PROG_CMD_DO;

        *compiler->code_pt++ = compiler->data_pt - compiler->dataseg;
        strcpy(compiler->data_pt, compiler->cur_token->str);
        compiler->data_pt += strlen(compiler->data_pt) + 1;
        compiler->cur_token = compiler->cur_token->next;
        break;
    default:
        errlog("Can't happen");
    }
        
    if (compiler->cur_token) {
        if (compiler->cur_token->kind != PROG_TOKEN_EOL) {
            prog_report_compile_err(compiler, compiler->cur_token->linenum,
                                    "Expected end of line");
            return;
        }
        compiler->cur_token = compiler->cur_token->next;
    }
}

unsigned char *
prog_compile_prog(Creature *ch,
                  char *prog_text,
                  void *owner,
                  prog_evt_type owner_type)
{
    prog_compiler_state state;
    unsigned char *obj = NULL;
    unsigned short *code_pt;
    int data_len, code_len;

    if (!prog_text || !*prog_text)
        return NULL;

    state.ch = ch;
    state.prog_text = prog_text;
    state.owner = owner;
    state.owner_type = owner_type;

    // Get a list of tokens from the prog
    if (!prog_lexify(&state))
        goto error;

    // We can have a prog that doesn't do anything... in those cases,
    // we should just clear the prog
    if (!state.token_list)
        return NULL;

    // Initialize object and data segments
    // FIXME: allocates a large buffer initially.  We should be automatically
    // adjusting this as needed
    state.dataseg = new char[65536];
    if (!state.dataseg) {
        prog_report_compile_err(&state, 0, "Out of memory");
        goto error;
    }
    state.data_pt = state.dataseg;
    *state.data_pt++ = '\0';

    data_len = 0;
    state.codeseg = new unsigned short[32768];
    // FIXME: allocates a large buffer initially.  We should be automatically
    // adjusting this as needed
    if (!state.codeseg) {
        prog_report_compile_err(&state, 0, "Out of memory");
        goto error;
    }
    state.code_pt = state.codeseg;
    code_len = 0;

    // Make sure the code starts with a handler
    if (state.cur_token->kind != PROG_TOKEN_SYM ||
        (strcasecmp(state.cur_token->sym, "before") &&
         strcasecmp(state.cur_token->sym, "handle") &&
         strcasecmp(state.cur_token->sym, "after"))) {
        prog_report_compile_err(&state, state.cur_token->linenum,
                                "Command without handler");
        goto error;
    }

    // Run through tokens until no more tokens are left or a fatal
    // compiler error occurs.
    state.error = 0;
    while (state.cur_token && !state.error)
        prog_compile_statement(&state);

    if (!state.error) {
        // No error occurred - map object code and data to a single
        // memory block...

        // Add an endofprog command to the end of the code to make
        // sure it doesn't wander into the data segment
        *state.code_pt++ = PROG_CMD_ENDOFPROG;
        *state.code_pt++ = 0;

        code_len = (state.code_pt - state.codeseg) * sizeof(short);
        data_len = state.data_pt - state.dataseg;
        obj = new unsigned char[code_len + data_len];
        if (!obj) {
            prog_report_compile_err(&state, 0, "Out of memory");

            goto error;
        }
        // Since the code comes first, we have to change all the argument
        // offsets to take into account the length of the code.
        for (code_pt = state.codeseg + 1;
             code_pt - state.codeseg < code_len;
             code_pt += 2)
            *code_pt += code_len;
        memcpy(obj, state.codeseg, code_len);

        // Now copy the data segment
        memcpy(obj + code_len, state.dataseg, data_len);
    } else {
        // An error occurred - set the result to NULL
        obj = NULL;
    }

error:

    // Free compiler structures
    prog_free_tokens(state.token_list);
    delete [] state.codeseg;
    delete [] state.dataseg;

    // We can potentially use a LOT of temporary string space in our
    // compilation, so we gc it here.
    tmp_gc_strings();

    return obj;
}

void
prog_display_obj(Creature *ch, unsigned char *exec)
{
    int cmd, arg_addr, read_pt;
    int cmd_count;

    cmd_count = 0;
    while (prog_cmds[cmd_count].str)
        cmd_count++;

    read_pt = 0;
    while (read_pt >= 0 && *((short *)&exec[read_pt])) {
        // Get the command and the arg address
        cmd = *((short *)(exec + read_pt));
        arg_addr = *((short *)(exec + read_pt + sizeof(short)));
        // Set the execution point to the next command by default
        read_pt += sizeof(short) * 2;
        if (cmd < 0 || cmd >= cmd_count)
            send_to_char(ch, "<INVALID CMD>\r\n");
        else
            send_to_char(ch, "%-9s %s\n",
                         tmp_toupper(prog_cmds[cmd].str),
                         (char *)(exec + arg_addr));
    }
}

void
prog_compile(Creature *ch, void *owner, prog_evt_type owner_type)
{
    char *prog;
    unsigned char *obj;

    // Get the prog
    prog = prog_get_text(owner, owner_type);

    // Compile the prog, if one exists.
    obj = (prog) ? prog_compile_prog(ch, prog, owner, owner_type):NULL;
    
    // Set the object code of the owner
    switch (owner_type) {
    case PROG_TYPE_MOBILE:
        delete [] ((Creature *)owner)->mob_specials.shared->progobj;
        ((Creature *)owner)->mob_specials.shared->progobj = obj;
        break;
    case PROG_TYPE_ROOM:
        delete [] ((room_data *)owner)->progobj;
        ((room_data *)owner)->progobj = obj;
        break;
    case PROG_TYPE_OBJECT:
        break;
    default:
        errlog("Can't happen at %s:%u", __FILE__, __LINE__);
    }

    // Kill all the progs the owner has to avoid invalid code
    destroy_attached_progs(owner);
}

