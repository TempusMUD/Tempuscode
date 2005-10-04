 #include <string.h>

#include "constants.h"
#include "comm.h"
#include "creature.h"
#include "prog.h"
#include "utils.h"
#include "spells.h"
#include "db.h"

enum prog_token_kind {
    PROG_TOKEN_EOL,
    PROG_TOKEN_SYM,
    PROG_TOKEN_STR,
    PROG_TOKEN_HANDLER,
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

struct prog_code_block {
    prog_code_block *next;
    unsigned short code_seg[8196];
    unsigned short *code_pt;
    char data_seg[8196];
    char *data_pt;
};

struct prog_compiler_state {
    Creature *ch;               // Player doing the compiling
    char *prog_text;            // Text to be compiled
    void *owner;                // Owner of the prog
    prog_evt_type owner_type;   // Owner type of the prog
    prog_token *token_list;     // The prog converted to a list of tokens
    prog_token *cur_token;      // The token under inspection
    prog_code_block *code;      // The current code block being compiled
    prog_code_block *handlers[PROG_PHASE_COUNT][PROG_EVT_COUNT]; // entry table for event handlers
    int error;                  // true if a fatal error has occurred
};

const char *prog_phase_strs[] = { "before", "handle", "after", "\n" };
const char *prog_event_strs[] = {
    "command", "idle", "fight", "give", "enter", "leave", "load", "tick",
    "spell", "combat", "death", "\n"
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
prog_compile_message(prog_compiler_state *compiler,
                    int level,
                    int linenum,
                    const char *str,
                    va_list args)
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

    switch (level) {
    case 0:
        severity = "notice"; break;
    case 1:
        severity = "warning"; break;
    case 2:
        severity = "error"; break;
    }

    if (compiler->ch)
        send_to_char(compiler->ch, "Prog %s in %s: %s\r\n",
                     severity, place, msg);
    else
        slog("Prog %s in %s: %s", severity, place, msg);
}

void
prog_compile_error(prog_compiler_state *compiler,
                   int linenum,
                   const char *str,
                   ...)
{
    va_list args;

    va_start(args, str);
    prog_compile_message(compiler, 2, linenum, str, args);
    va_end(args);

    compiler->error = 1;
}

void
prog_compile_warning(prog_compiler_state *compiler,
                     int linenum,
                     const char *str,
                     ...)
{
    va_list args;

    va_start(args, str);
    prog_compile_message(compiler, 1, linenum, str, args);
    va_end(args);
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
    case PROG_TOKEN_HANDLER:
        new_token->sym = strdup(value); break;
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
prog_free_compiler(prog_compiler_state *compiler)
{
    prog_token *next_token, *cur_token;
    prog_code_block *cur_code, *next_code;
    int phase_idx, event_idx;

    for (cur_token = compiler->token_list;cur_token;cur_token = next_token) {
        next_token = cur_token->next;
        free(cur_token->str);
        delete cur_token;
    }

    for (phase_idx = 0;phase_idx < PROG_PHASE_COUNT;phase_idx++)
        for (event_idx = 0;event_idx < PROG_EVT_COUNT;event_idx++)
            for (cur_code = compiler->handlers[phase_idx][event_idx];
                 cur_code;
                 cur_code = next_code) {
                next_code = cur_code->next;
                delete cur_code;
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
            
            if (*line == '*') {
                cmd_str = tmp_getword(&line) + 1;
                if (!strcasecmp(cmd_str, "before")
                    || !strcasecmp(cmd_str, "handle")
                    || !strcasecmp(cmd_str, "after"))
                    new_token = prog_create_token(PROG_TOKEN_HANDLER,
                                                  linenum,
                                                  cmd_str);
                else
                    new_token = prog_create_token(PROG_TOKEN_SYM,
                                                  linenum,
                                                  cmd_str);
                if (!new_token) {
                    prog_compile_error(compiler,
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
                    prog_compile_error(compiler,
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
                    prog_compile_error(compiler,
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
prog_compiler_emit(prog_compiler_state *compiler, int instr, void *data, size_t data_len)
{
	*compiler->code->code_pt++ = instr;
	if (data_len) {
		*compiler->code->code_pt++ = compiler->code->data_pt - compiler->code->data_seg;
		memcpy(compiler->code->data_pt, data, data_len);
		compiler->code->data_pt += data_len;
	} else {
		*compiler->code->code_pt++ = 0;
	}
}

void
prog_compile_statement(prog_compiler_state *compiler)
{
    prog_command *cmd;
	int instr;

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
                                    "Unknown command '%s'",
                                    compiler->cur_token->sym);
            return;
        }
        
        // Emit prog command code
		instr = cmd - prog_cmds;

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
			compiler->cur_token->str,
			strlen(compiler->cur_token->str) + 1);
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
prog_compile_handler(prog_compiler_state *compiler)
{
    prog_token *cmd_token;
    int phase, event, cmd;
    char *arg, *args;

    // Make sure the code starts with a handler
    if (compiler->cur_token->kind != PROG_TOKEN_HANDLER) {
        prog_compile_error(compiler, compiler->cur_token->linenum,
                           "Command without handler");
        return;
    }

    // Set up new code block
    compiler->code = new prog_code_block;
    memset(compiler->code, 0, sizeof(prog_code_block));
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
		prog_compiler_emit(compiler, PROG_CMD_BEFORE, NULL, 0); break;
	case PROG_EVT_HANDLE:
		prog_compiler_emit(compiler, PROG_CMD_HANDLE, NULL, 0); break;
	case PROG_EVT_AFTER:
		prog_compiler_emit(compiler, PROG_CMD_AFTER, NULL, 0); break;
	default:
        slog("Can't happen at %s:%d", __FILE__, __LINE__);
	}

    // Advance to the next token, which should be a STR
    compiler->cur_token = cmd_token->next;
    if (!compiler->cur_token || compiler->cur_token->kind != PROG_TOKEN_STR) {
            prog_compile_error(compiler, cmd_token->linenum,
                                    "Expected parameter to *%s",
                                    cmd_token->sym);
            return;
    }

    // Retrieve the event type
    args = compiler->cur_token->str;
	event = search_block(tmp_getword(&args), prog_event_strs, true);
	if (event < 0) {
        prog_compile_error(compiler, compiler->cur_token->linenum,
                                "Invalid parameter '%s' to *%s",
                                arg,
                                cmd_token->sym);
        return;
    }

    // Add code to handle any other conditions attached to the handler
    switch (event) {
    case PROG_EVT_COMMAND:
        arg = tmp_getword(&args);
        if (!*arg) {
            prog_compile_error(compiler, compiler->cur_token->linenum,
                                    "No command specified for *%s command",
                                    arg,
                                    cmd_token->sym);
            return;
        }
		prog_compiler_emit(compiler, PROG_CMD_CLRCOND, NULL, 0);
        while (*arg) {
            cmd = find_command(arg);
            if (cmd < 0) {
                prog_compile_error(compiler, compiler->cur_token->linenum,
                                        "'%s' is not a valid command", arg);
                return;
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
                                    "No spell numbers specified for *%s spell",
                                    arg,
                                    cmd_token->sym);
            return;
        }
		prog_compiler_emit(compiler, PROG_CMD_CLRCOND, NULL, 0);
        while (*arg) {
            if (!is_number(arg)) {
                prog_compile_error(compiler, compiler->cur_token->linenum,
                                        "Spell number expected, got '%s'",
                                        arg);
                return;
            }
            cmd = atoi(arg);
            if (cmd < 0 || cmd > max_spell_num || spells[cmd][0] == '!') {
                prog_compile_error(compiler, compiler->cur_token->linenum,
                                        "%s is not a valid spell number",
                                        arg);
                return;
                
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
                                        "Object vnum expected, got '%s'",
                                        arg);
                return;
            }
            cmd = atoi(arg);
            if (cmd < 0) {
                prog_compile_error(compiler, compiler->cur_token->linenum,
                                        "%s is not a valid object vnum",
                                        arg);
                return;
            }
            if (!real_object_proto(cmd))
                prog_compile_warning(compiler, compiler->cur_token->linenum,
                                     "No object %s exists.",
                                     arg);

			prog_compiler_emit(compiler, PROG_CMD_CLRCOND, NULL, 0);
			prog_compiler_emit(compiler, PROG_CMD_CMPOBJVNUM, &cmd, sizeof(int));
			prog_compiler_emit(compiler, PROG_CMD_CONDNEXTHANDLER, NULL, 0);

        }
        break;
    default:
        break;
    }

    // Advance token and ensure that it's an end of line
    compiler->cur_token = compiler->cur_token->next;
    if (!compiler->cur_token || compiler->cur_token->kind != PROG_TOKEN_EOL) {
        prog_compile_error(compiler, compiler->cur_token->linenum,
                             "End of line expected.",
                             arg);
        return;
    }

    // Compile statements until an error or the next event handler
    compiler->cur_token = compiler->cur_token->next;
    while (compiler->cur_token
           && compiler->cur_token->kind != PROG_TOKEN_HANDLER
           && !compiler->error)
        prog_compile_statement(compiler);

    // If an entry in the event handler table already exists, append
    // the new code to the end
    if (compiler->handlers[phase][event]) {
        prog_code_block *prev_block;

        prev_block = compiler->handlers[phase][event];
        while (prev_block->next)
            prev_block = prev_block->next;
        prev_block->next = compiler->code;
    } else {
        // otherwise, just add an entry to the table
        compiler->handlers[phase][event] = compiler->code;
    }

    compiler->code = NULL;
}

void
prog_display_obj(Creature *ch, unsigned char *exec)
{
    int cmd, arg_addr, read_pt;
    int cmd_count;
	int phase_idx, event_idx;

    cmd_count = 0;
    while (prog_cmds[cmd_count].str)
        cmd_count++;

	for (phase_idx = 0;phase_idx < PROG_PHASE_COUNT;phase_idx++)
		for (event_idx = 0;event_idx < PROG_EVT_COUNT;event_idx++) {
			read_pt = *((short *)exec + phase_idx * PROG_EVT_COUNT + event_idx);
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
prog_map_to_block(prog_compiler_state *compiler)
{
    prog_code_block *cur_code;
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
        for (event_idx = 0; event_idx < PROG_EVT_COUNT;event_idx++) {
			if (compiler->handlers[phase_idx][event_idx]) {
				for (cur_code = compiler->handlers[phase_idx][event_idx];
					 cur_code;
					 cur_code = cur_code->next) {
					code_len += (cur_code->code_pt - cur_code->code_seg) * sizeof(unsigned short);
					data_len += (cur_code->data_pt - cur_code->data_seg);
				}
				code_len += 2;      // add size of halt command
			}
        }

    // Add length for the end of prog marker
    code_len += 2;
    block_len += code_len * sizeof(unsigned short) + data_len;

    // Allocate result block and clear it
    block = new unsigned char[block_len];
    memset(block, 0, block_len);

    // Assign initial offsets
    code_offset = PROG_PHASE_COUNT * PROG_EVT_COUNT * sizeof(short);
    data_offset = code_offset + code_len * sizeof(unsigned short);

    // Iterate through event handling table
    for (phase_idx = 0; phase_idx < PROG_PHASE_COUNT; phase_idx++)
        for (event_idx = 0; event_idx < PROG_EVT_COUNT;event_idx++) {
            // Go to next event handler if no code attached to it
            if (!compiler->handlers[phase_idx][event_idx])
                continue;
            // Assign current code offset to event table entry
            *((short *)block + phase_idx * PROG_EVT_COUNT + event_idx) = code_offset;

            // Concatenate and relocate code and data segments
            for (cur_code = compiler->handlers[phase_idx][event_idx];
                 cur_code;
                 cur_code = cur_code->next) {
                // Copy the data segment
                memcpy(&block[data_offset], cur_code->data_seg, cur_code->data_pt - cur_code->data_seg);
                // Backpatch the data addresses
                for (code_pt = cur_code->code_seg + 1;
                     code_pt < cur_code->code_pt;
                     code_pt += 2)
                    *code_pt += data_offset;
                // Copy the code segment
                memcpy(&block[code_offset], cur_code->code_seg, (cur_code->code_pt - cur_code->code_seg) * sizeof(short));

                // Update code and data offsets
                code_offset += (cur_code->code_pt - cur_code->code_seg) * sizeof(short);
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
prog_compile_prog(Creature *ch,
                  char *prog_text,
                  void *owner,
                  prog_evt_type owner_type)
{
    prog_compiler_state state;
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

