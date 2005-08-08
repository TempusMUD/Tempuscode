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
    prog_token *children;   // sublist of prog_tokens
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
    char *msg;
    va_list args;

    va_start(args, str);
    msg = tmp_vsprintf(str, args);
    va_end(args);

    switch (compiler->owner_type) {
    case PROG_TYPE_MOBILE:
        place = tmp_sprintf("mobile %d", GET_MOB_VNUM((Creature *)compiler->owner));
        break;
    case PROG_TYPE_ROOM:
        place = tmp_sprintf("room %d", ((room_data *)compiler->owner)->number);
        break;
    default:
        place = "an unknown location";
    }
    if (compiler->ch)
        send_to_char(compiler->ch, "Prog error in %s, line %d: %s\r\n",
                     place,
                     linenum,
                     msg);
    else
        slog("Prog error in %s, line %d: %s", place, linenum, msg);

}

prog_token *
prog_create_token(prog_token_kind kind, int linenum, const char *value)
{
    prog_token *new_token;

    CREATE(new_token, prog_token, 1);
    if (!new_token)
        return NULL;

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
        if (cur_token->children)
            prog_free_tokens(cur_token->children);
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
                if (!new_token)
                    return false;
                
                if (prev_token)
                    prev_token->next = new_token;
                else
                    compiler->token_list = new_token;
                prev_token = new_token;
            }

            // if we have an argument to the command, tokenize it
            if (*line) {
                new_token = prog_create_token(PROG_TOKEN_STR, linenum, line);
                if (!new_token)
                    return false;

                if (prev_token)
                    prev_token->next = new_token;
                else
                    compiler->token_list = new_token;
                prev_token = new_token;
            }

            // The end-of-line is a language token in progs
            new_token = prog_create_token(PROG_TOKEN_EOL, linenum, line);
            if (!new_token)
                return false;
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

unsigned char *
prog_compile_prog(Creature *ch,
                  char *prog_text,
                  void *owner,
                  prog_evt_type owner_type)
{
    prog_compiler_state state;
    prog_command *cmd;
    unsigned char *obj;
    char *dataseg = NULL, *data_pt;
    unsigned short *codeseg = NULL, *code_pt;
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
    dataseg = new char[65536];
    data_pt = dataseg;
    *data_pt++ = '\0';

    data_len = 0;
    codeseg = new unsigned short[32768];
    code_pt = codeseg;
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

    while (state.cur_token) {
        // Ignore blank statements
        switch (state.cur_token->kind) {
        case PROG_TOKEN_EOL:
            break;
        case PROG_TOKEN_SYM:
            // Find the prog command
            cmd = prog_cmds + 1;
            while (cmd->str && strcasecmp(cmd->str, state.cur_token->sym))
                cmd++;
            if (!cmd->str) {
                prog_report_compile_err(&state,
                                        state.cur_token->linenum,
                                        "Unknown command '%s'",
                                        state.cur_token->sym);
                goto error;
            }
        
            // Emit prog command code
            *code_pt++ = (cmd - prog_cmds);

            // Advance to the next token
            state.cur_token = state.cur_token->next;

            if (state.cur_token && state.cur_token->kind == PROG_TOKEN_STR) {
                *code_pt++ = data_pt - dataseg;
                strcpy(data_pt, state.cur_token->str);
                data_pt += strlen(data_pt) + 1;
              
                state.cur_token = state.cur_token->next;
            } else {
                *code_pt++ = 0;
            }
            break;
        case PROG_TOKEN_STR:
            if (owner_type != PROG_TYPE_MOBILE) {
                prog_report_compile_err(&state, state.cur_token->linenum,
                                        "Non-mobs cannot perform in-game commands.");
                goto error;
            }

            // It's an in-game command with a mob, so emit the *do command
            *code_pt++ = PROG_CMD_DO;

            if (state.cur_token && state.cur_token->kind == PROG_TOKEN_STR) {
                *code_pt++ = data_pt - dataseg;
                strcpy(data_pt, state.cur_token->str);
                data_pt += strlen(data_pt) + 1;
                state.cur_token = state.cur_token->next;
            } else {
                *code_pt++ = 0;
            }
            break;
        default:
            errlog("Can't happen");
        }
        
        if (state.cur_token) {
            if (state.cur_token->kind != PROG_TOKEN_EOL) {
                prog_report_compile_err(&state, state.cur_token->linenum,
                                        "Expected end of line");
                goto error;
            }
            state.cur_token = state.cur_token->next;
        }
    }

    // Add an endofprog command to the end of the code to make sure it doesn't
    // wander into the data segment
    *code_pt++ = PROG_CMD_ENDOFPROG;
    *code_pt++ = 0;

    // We don't need the tokens anymore
    prog_free_tokens(state.token_list);

    code_len = (code_pt - codeseg) * sizeof(short);
    data_len = data_pt - dataseg;
    obj = new unsigned char[code_len + data_len];

    // Since the code comes first, we have to change all the argument
    // offsets to take into account the length of the code.
    for (code_pt = codeseg + 1;code_pt - codeseg < code_len;code_pt += 2)
        *code_pt += code_len;
    memcpy(obj, codeseg, code_len);

    // Now copy the data segment
    memcpy(obj + code_len, dataseg, data_len);

    delete [] codeseg;
    delete [] dataseg;

    // We can potentially use a LOT of temporary string space in our
    // compilation, so we gc it here.
    tmp_gc_strings();

    return obj;
error:
    prog_free_tokens(state.token_list);
    delete [] codeseg;
    delete [] dataseg;

    return NULL;
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

