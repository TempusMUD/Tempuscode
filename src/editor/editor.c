//
// File: editor.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//
//

#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include <fcntl.h>
// Tempus Includes
#include "screen.h"
#include "desc_data.h"
#include "comm.h"
#include "db.h"
#include "utils.h"
#include "login.h"
#include "interpreter.h"
#include "boards.h"
#include "mail.h"
#include "editor.h"
#include "tmpstr.h"
#include "accstr.h"
#include "help.h"
#include "comm.h"

extern struct descriptor_data *descriptor_list;
extern HelpCollection *Help;

struct editor *
make_editor(struct descriptor_data *d, int max)
{
    struct editor *editor;

    CREATE(editor, struct editor, 1);
    editor->desc = d;
    editor->max_size = max;
    editor->do_command = editor_do_command;
    editor->finalize = editor_finalize;
    editor->cancel = editor_cancel;
    editor->displaybuffer = editor_display;
    editor->sendmodalhelp = editor_sendmodalhelp;

    return editor;
}

void
editor_import(struct editor *editor, char *text)
{
    gchar **strv, **strp;

    g_list_free(editor->original);
    g_list_free(editor->lines);

    strv = g_strsplit(text, "\n", 0);
    for (strp = strv;*strp;strp++) {
        editor->original = g_list_prepend(editor->original, g_strdup(*strp));
        editor->lines = g_list_prepend(editor->lines, g_strdup(*strp));
    }
    g_freestrv(strv);

    g_list_reverse(editor->original);
    g_list_reverse(editor->lines);
}

void
editor_emit(struct editor *editor, char *text)
{
    send_to_desc(editor->desc, "%s", text);
}

void
emit_editor_startup(struct editor *editor)
{
    send_to_desc(editor->desc,
                 "&C     * &YTEDII &b]&n Save and exit with @ on a new line. "
                 "&&H for help             &C*\r\n");
    send_to_desc(editor->desc, "     ");
	for (int i = 0; i < 7; i++)
        send_to_desc(editor->desc, "&C%d&B---------", i);
    send_to_desc(editor->desc, "&C7&n\r\n");
}

void
editor_send_prompt(struct editor *editor)
{
    send_to_desc(desc, "%3zd&b]&n ", editor_line_count(editor) + 1);
}

int
editor_buffer_size(struct editor *editor)
{
    int len;

    void count_string_len(char *str, gpointer ignore) {
        len += strlen(str) + 1;
    }
    g_list_foreach(editor->lines, count_string_len, 0);

    return len;
}

void
editor_finish(struct editor *editor, bool save)
{
    struct descriptor_data *desc = editor->desc;

    if (save) {
        int length;
        char *text, *write_pt;

        length = editor_buffer_size(editor);
        text = (char *)malloc(length);
        strcpy(text, "");
        write_pt = text;
        void concat_line(char *line, gpointer ignore) {
            strcpy(write_pt, line);
            write_pt += strlen(line);
        }
        g_list_foreach(editor->lines, concat_line, 0);

        // Call the finalizer
        editor->finalize(text);

        free(text);
    } else {
        editor->cancel(text);
    }

	if (IS_PLAYING(desc) && desc->creature && !IS_NPC(desc->creature)) {
        // Remove "using the editor bits"
        REMOVE_BIT(PLR_FLAGS(desc->creature),
                   PLR_WRITING | PLR_OLC | PLR_MAILING);
        REMOVE_BIT(PRF2_FLAGS(desc->creature), PRF2_NOWRAP);
	}

    // Free the editor
	desc->text_editor = NULL;
    g_list_foreach(editor->original, free, 0);
    g_list_free(editor->original);
    g_list_foreach(editor->lines, free, 0);
    g_list_free(editor->lines);
	free(editor);
}

void
editor_finalize(struct editor *editor, char *text)
{
    // Do nothing
}

void
editor_cancel(struct editor *editor)
{
    // Do nothing
}

void
editor_handle_input(struct editor *editor, char *input)
{
	// 2 special chars, @ and &
	char inbuf[MAX_INPUT_LENGTH + 1];
    char *args;

	strncpy(inbuf, input, MAX_INPUT_LENGTH);

	if (*inbuf == '&') {		// Commands
        if (!inbuf[1]) {
            SendMessage("& is the command character. Type &h for help.\r\n");
            return;
        }
        args = inbuf + 2;
        while (*args && isspace(*args))
            args++;
        if (!editor->do_command(inbuf[1], args))
            editor_emit(editor, "Invalid Command. Type &h for help.\r\n");
	} else if (*inbuf == '@') {	// Finish up
        editor_finish(true);
	} else {					// Dump the text in
		editor_append(inbuf);
	}
}

void
editor_display(struct editor *editor, int start_line, int line_count)
{
    unsigned int linenum, end_line;
    GList *itr;

    acc_string_clear();

    // Calculate last line number we're going to show
    end_line = editor_line_count(editor) + 1;
    if (line_count >= 0 && start_line + line_count < end_line)
        end_line = start_line + line_count;

    // Set the iterator to the beginning line
    itr = g_list_nth(start_line - 1);

    // Display the lines from the beginning line to the end, making
    // sure we don't overflow the LARGE_BUF desc buffer
	for (linenum = start_line;linenum < end_line;linenum++, itr = g_list_next(itr)) {
		acc_sprintf("%3d%s%s]%s %s\r\n", linenum,
                    CCBLD(desc->creature, C_CMP),
                    CCBLU(desc->creature, C_NRM),
                    CCNRM(desc->creature, C_NRM),
                    itr->data);
		if (acc_get_length() > (LARGE_BUFSIZE - 1024))
			break;
	}

    acc_strcat("\r\n", NULL);
	if (acc_get_length() > (LARGE_BUFSIZE - 1024))
        acc_strcat("Output buffer limit reached. Use \"&r <line number>\" to specify starting line.\r\n", NULL);

	editor_emit(acc_get_string());
}

bool
editor_is_full(struct editor *editor, char *line)
{
    return (strlen(line) + editor_buffer_size(editor) > editor->max_size)
}

gint
editor_line_count(struct editor *editor)
{
    return editor_line_count(editor);
}

void
editor_append(struct editor *editor, char *line)
{
    if (editor_is_full(editor, line)) {
        editor_emit("Error: The buffer is full.\r\n");
        return;
    }
    if (PLR_FLAGGED(desc->creature, PLR_OLC))
        g_strdelimit(line, "~", '?');
    g_list_append(editor->lines, line);
}

bool
editor_insert(struct editor *editor, int lineno, char *line)
{
	GList *s;

    if (lineno < 1)
        lineno = 1;
	if (*line)
		line++;

	if (line > editor_line_count(editor)) {
		editor_emit("You can't insert before a line that doesn't exist.\r\n");
		return false;
	}
	if (editor_is_full(editor, line)) {
		editor_emit(editor, "Error: The buffer is full.\r\n");
		return false;
	}


    s = g_list_nth(editor->lines, lineno - 1);
    g_list_insert_before(s, line);

	return true;
}

bool
editor_replace_line(struct editor *editor, unsigned int lineno, char *line)
{
    GList *s;

	if (*line && *line == ' ')
		line++;

	if (line < 1 || line > editor_line_count(editor)) {
		editor_emit(editor, "There's no line to replace there.\r\n");
		return false;
	}
	// Find the line
    s = g_list_nth(editor->lines, lineno - 1);
    advance(s, line - 1);

	// Make sure we can fit the new stuff in
    if (editor_is_full(line)) {
		editor_emit(editor, "Error: The buffer is full.\r\n");
		return false;
	}
    free(s->data);
    s->data = line

	return true;
}

bool
editor_movelines(unsigned int start_line,
                 unsigned int end_line,
                 unsigned int dest_line)
{
    GList *dest, *begin, *end, *next;


    if (start_line < 1 || start_line > editor_line_count(editor)) {
        if (start_line == end_line)
            editor_emit(editor, "Line %d is an invalid line.\r\n");
        else
            editor_emit(editor, "Starting line %d is an invalid line.\r\n");
        return false;
    }
    if (end_line < 1 || end_line > editor_line_count(editor)) {
        editor_emit(editor, "Ending line %d is an invalid line.\r\n");
        return false;
    }

    // Nothing needs to be done if destination is inside the block
    if (dest_line >= start_line && dest_line <= end_line)
        return true;

    if (dest_line < 1)
        dest_line = 1;
    if (dest_line >= editor_line_count(editor))
        dest_line = editor_line_count(editor) + 1;

    begin = g_list_nth(editor->lines, start_line);
    end = g_list_nth(editor->lines, end_line + 1);
    dest = g_list_nth(editor->lines, dest_line);

    for (GList *cur = begin;cur != end;cur = next;) {
        next = cur->next;
        editor->lines = g_list_insert_before(editor->lines, dest, cur->data);
        editor->lines = g_list_delete_link(editor->lines, cur);
    }

    if (start_line == end_line) {
        editor_emit(editor, tmp_sprintf("Moved line %d above line %d.\r\n",
                                start_line, dest_line));
    } else {
        editor_emit(editor, tmp_sprintf("Moved lines %d-%d to the line above line %d.\r\n",
                                start_line, end_line, dest_line));
    }

    return true;
}

bool
editor_find(struct editor *editor, char *args)
{
    void print_if_match(char *str, gpointer ignore) {
        if (strcasestr(str, args))
            acc_sprintf("%3d%s%s]%s %s\r\n", i,
                    CCBLD(desc->creature, C_CMP),
                    CCBLU(desc->creature, C_NRM),
                    CCNRM(desc->creature, C_NRM),
                    str);
    }

    acc_string_clear();
    g_list_foreach(editor->lines, print_if_match, 0);
    acc_strcat("\r\n", NULL);
    editor_emit(editor, acc_get_string());

    return true;
}

bool
editor_substitute(struct editor *editor, char *args)
{
    const char *usage = "There are two formats for substitute:\r\n  &s [search pattern] [replacement]\r\n  &s /search pattern/replacement/\r\nIn the first form, you can use (), [], <>, or {}.\r\n";
	// Iterator to the current line in theText
	GList *line;
	// The string containing the search pattern
	char *pattern;
    int pattern_len;
	// String containing the replace pattern
	char *replacement;
    int replacement_len;
	// Number of replacements made
	int replaced = 0;
	// read pointer and write pointer.
	char *temp, end_char;
    bool balanced;
    int size_delta;

    while (isspace(*args))
        args++;

	if (!*args) {
		editor_emit(editor, usage);
		return false;
	}

    // First non-space character is the search delimiter.  If the
    // delimiter is something used with some kind of brace, it
    // terminates in the opposite brace
    balanced = true;
    switch (*args) {
    case '(':
        end_char = ')'; break;
    case '[':
        end_char = ']'; break;
    case '{':
        end_char = '}'; break;
    case '<':
        end_char = '>'; break;
    default:
        end_char = *args;
        balanced = false;
        break;
    }
    args++;

    pattern = args;
    while (*args && *args != end_char)
        args++;
    if (!*args) {
        editor_emit(editor, usage);
        return false;
    }

    pattern_len = args - temp;

    // If the pattern was specified with a balanced delimiter, then
    // the replacement must be, too
    if (balanced) {
        while (*args
               && *args != '('
               && *args != '['
               && *args != '{'
               && *args != '<')
            args++;
        if (!*args) {
            editor_emit(editor, "If the search pattern uses a balanced delimiter, the replacement must use a balanced\r\ndelimiter as well.\r\n");
            return false;
        }

        switch (*args) {
        case '(':
            end_char = ')'; break;
        case '[':
            end_char = ']'; break;
        case '{':
            end_char = '}'; break;
        case '<':
            end_char = '>'; break;
            break;
        default:
            end_char = *args; break;
        }
        args++;
    }

    replacement = temp;

    while (*args && *args != end_char)
        args++;

    replacement_len = args - temp;

    size_delta = strlen(replacement) - strlen(pattern);

	// Find pattern in theText a line at a time and replace each instance.
	unsigned int pos;			// Current position in the line
	bool overflow = false;		// Have we overflowed the buffer?

    for (line = editor->lines;line;line = line->next) {
        pos = strstr(line->data, pattern);
        while (pos) {
            if (pattern_len < replacement_len) {
                // increase in string length
                memmove();
            } else if (pattern_len > replacement_len) {
                // reduction of string length
            } else {
                // simple copy if equal
                strcpy(pos, replacement);
            }
            replaced++;
            pos = strstr(pos + replacement_len, pattern);
        }
	for (line = theText.begin(); !overflow && line != theText.end(); line++) {
        pos = line->find(pattern, 0);
		while (pos < line->length()) {
            // Handle both buffer size and replacement overflows
            if (curSize + size_delta > maxSize || replaced >= 100) {
                overflow = true;
                break;
            }
            *line = line->replace(pos, pattern.length(), replacement);
            replaced++;
            curSize += size_delta;
            pos = line->find(pattern, pos + replacement.length());
		}
	}
    if (curSize + size_delta > maxSize)
        editor_emit(editor, "Error: The buffer is full.\r\n");
    if (replaced >= 100)
        editor_emit(editor, "Replacement limit of 100 reached.\r\n");
    if (replaced > 0) {
        editor_emit(editor, tmp_sprintf(
                        "Replaced %d occurrence%s of '%s' with '%s'.\r\n",
                        replaced,
                        (replaced == 1) ? "":"s",
                        pattern.c_str(),
                        replacement.c_str()));
    } else {
        editor_emit(editor, "Search string not found.\r\n");
    }

    if (wrap)
	    Wrap();
	UpdateSize();
	return true;
}

bool
CEditor_Substitute(char *args)
{
    const char *usage = "There are two formats for substitute:\r\n  &s [search pattern] [replacement]\r\n  &s /search pattern/replacement/\r\nIn the first form, you can use (), [], <>, or {}.\r\n";
	// Iterator to the current line in theText
	list <string>_iterator line;
	// The string containing the search pattern
	string pattern;
	// String containing the replace pattern
	string replacement;
	// Number of replacements made
	int replaced = 0;
	// read pointer and write pointer.
	char *temp, end_char;
    bool balanced;
    int size_delta;

    while (isspace(*args))
        args++;

	if (!*args) {
		editor_emit(editor, usage);
		return false;
	}

    // First non-space character is the search delimiter.  If the
    // delimiter is something used with some kind of brace, it
    // terminates in the opposite brace
    balanced = true;
    switch (*args) {
    case '(':
        end_char = ')'; break;
    case '[':
        end_char = ']'; break;
    case '{':
        end_char = '}'; break;
    case '<':
        end_char = '>'; break;
    default:
        end_char = *args;
        balanced = false;
        break;
    }
    args++;

    temp = args;
    while (*args && *args != end_char)
        args++;
    if (!*args) {
        editor_emit(editor, usage);
        return false;
    }

    *args++ = '\0';
    pattern = temp;

    // If the pattern was specified with a balanced delimiter, then
    // the replacement must be, too
    if (balanced) {
        while (*args
               && *args != '('
               && *args != '['
               && *args != '{'
               && *args != '<')
            args++;
        if (!*args) {
            editor_emit(editor, "If the search pattern uses a balanced delimiter, the replacement must use a balanced\r\ndelimiter as well.\r\n");
            return false;
        }

        switch (*args) {
        case '(':
            end_char = ')'; break;
        case '[':
            end_char = ']'; break;
        case '{':
            end_char = '}'; break;
        case '<':
            end_char = '>'; break;
            break;
        default:
            end_char = *args; break;
        }
        args++;
    }

    temp = args;
    while (*args && *args != end_char)
        args++;

    *args = '\0';
    replacement = temp;

    size_delta = replacement.length() - pattern.length();

	// Find pattern in theText a line at a time and replace each instance.
	unsigned int pos;			// Current position in the line
	bool overflow = false;		// Have we overflowed the buffer?

	for (line = theText.begin(); !overflow && line != theText.end(); line++) {
        pos = line->find(pattern, 0);
		while (pos < line->length()) {
            // Handle both buffer size and replacement overflows
            if (curSize + size_delta > maxSize || replaced >= 100) {
                overflow = true;
                break;
            }
            *line = line->replace(pos, pattern.length(), replacement);
            replaced++;
            curSize += size_delta;
            pos = line->find(pattern, pos + replacement.length());
		}
	}
    if (curSize + size_delta > maxSize)
        editor_emit(editor, "Error: The buffer is full.\r\n");
    if (replaced >= 100)
        editor_emit(editor, "Replacement limit of 100 reached.\r\n");
    if (replaced > 0) {
        editor_emit(editor, tmp_sprintf(
                        "Replaced %d occurrence%s of '%s' with '%s'.\r\n",
                        replaced,
                        (replaced == 1) ? "":"s",
                        pattern.c_str(),
                        replacement.c_str()));
    } else {
        editor_emit(editor, "Search string not found.\r\n");
    }

    if (wrap)
	    Wrap();
	UpdateSize();
	return true;
}

bool
CEditor_Wrap(void)
{
	list <string>_iterator line;
	string_iterator s;
	string tempstr;
	int linebreak;

	if (PRF2_FLAGGED(desc->creature, PRF2_NOWRAP)) {
		return false;
	}
	for (line = theText.begin(); line != theText.end(); line++) {
		linebreak = 76;
		tempstr = "";
		// If its less than 77 chars, it don't need ta be wrapped.
		if (line->length() <= 76)
			continue;

		s = line->begin();

		// Find the first space <= 76th char.
		for (s += 75; s != line->begin() && *s != ' '; s--)
			linebreak--;

		if (linebreak == 1) {	// Linebreak is at 76
			s = line->begin();
			s += 75;
		}
		tempstr.append(s, line->end());
		if (linebreak > 1)		// If its a pos other than 1, its a space.
			tempstr.erase(tempstr.begin());
		line->erase(s, line->end());
		line++;
		theText.insert(line, 1, tempstr);
		line = theText.begin();
	}
	return true;
}

bool
CEditor_Remove(unsigned int start_line, unsigned int finish_line)
{
	list <string>_iterator start, finish;

	if (start_line < 1 || start_line > editor_line_count(editor)) {
		editor_emit(editor, "Someone already deleted that line boss.\r\n");
		return false;
	}
	if (finish_line < 1 || finish_line > editor_line_count(editor)) {
		editor_emit(editor, "Someone already deleted that line boss.\r\n");
		return false;
	}

    start = theText.begin();
    advance(start, start_line - 1);
    finish = theText.begin();
    advance(finish, finish_line);
	theText.erase(start, finish);

    if (start_line == finish_line)
        editor_emit(editor, tmp_sprintf("Line %d deleted.\r\n", start_line));
    else
        editor_emit(editor, tmp_sprintf("Lines %d-%d deleted.\r\n",
                                start_line,
                                finish_line));

    if (wrap)
	    Wrap();
	UpdateSize();
	return true;
}

bool
CEditor_Clear(void)
{

	theText.erase(theText.begin(), theText.end());

	UpdateSize();
	return true;
}

void
CEditor_ImportText(const char *str)
{
    char *line;

    origText.clear();
    if (str) {
        while ((line = tmp_getline(&str)) != NULL)
            origText.push_back(string(line));
    }

    theText = origText;

    UpdateSize();
    if (wrap)
	    Wrap();
}

void
CEditor_UndoChanges(void)
{
	if (origText.size()) {
        Clear();
        theText = origText;
        UpdateSize();
        editor_emit(editor, "Original buffer restored.\r\n");
	} else {
		editor_emit(editor, "There's no original to undo to.\r\n");
    }
}

void
CEditor_editor_emit(editor, const char *message)
{
	if (!desc || !desc->creature) {
		errlog("TEDII Attempting to SendMessage with null desc or desc->creature\r\n");
		return;
	}

	// If the original message is too long, make a new one thats small
	if (strlen(message) >= LARGE_BUFSIZE) {
        char *temp = NULL;

		slog("SendMessage Truncating message. NAME(%s) Length(%zd)",
             GET_NAME(desc->creature),
             strlen(message));
        temp = new char[LARGE_BUFSIZE];
		strncpy(temp, message, LARGE_BUFSIZE - 2);
		send_to_char(desc->creature, "%s", temp);
        delete [] temp;
	} else {
        // If the original message is small enough, just let it through.
		send_to_char(desc->creature, "%s", message);
	}
}

static inline int
text_length(list <string> &theText)
{
	int length = 0;
	list <string>_iterator s;
	for (s = theText.begin(); s != theText.end(); s++) {
		length += s->length();
	}
	return length;
}

void
CEditor_UpdateSize(void)
{
	int linesRemoved = 0;

	// update the current size
	curSize = text_length(theText);

	// Buffer overflow state.
	// This is probably happening in response to word wrap
	while (curSize > maxSize) {
		theText.pop_back();
		curSize = text_length(theText);
		linesRemoved++;
	}

	// Warn the player if the buffer was truncated.
	if (linesRemoved > 0) {
		editor_emit(editor, tmp_sprintf("Error: Buffer limit exceeded.  %d %s removed.\r\n",
                                linesRemoved,
                                linesRemoved == 1 ? "line" : "lines"));
		slog("TEDINF: UpdateSize removed %d lines from buffer. Name(%s) Size(%d) Max(%d)",
             linesRemoved, GET_NAME(desc->creature), curSize, maxSize);
	}
	// Obvious buffer flow state. This should never happen, but if it does, say something.
	if (curSize > maxSize) {
		slog("TEDERR: UpdateSize updated to > maxSize. Name(%s) Size(%d) Max(%d)",
			GET_NAME(desc->creature), curSize, maxSize);
	}
}

void
CEditor_SendModalHelp(void)
{
    // default offers the clear buffer and undo changes options
    send_to_desc(desc, "            &YC - &nClear Buffer         &YU - &nUndo Changes  \r\n");
}

void
CEditor_ProcessHelp(char *inStr)
{
	char command[MAX_INPUT_LENGTH];

    acc_string_clear();

	if (!*inStr) {
        send_to_desc(desc,
                     "     &C*&B-----------------------&Y H E L P &B-----------------------&C*\r\n"
                     "            &YR - &nRefresh Screen       &YH - &nHelp         \r\n"
                     "            &YE - &nSave and Exit        &YQ - &nQuit (Cancel)\r\n"
                     "            &YL - &nReplace Line         &YD - &nDelete Line  \r\n"
                     "            &YI - &nInsert Line          &YM - &nMove Line(s)\r\n"
                     "            &YF - &nFind                 &YS - &nSubstitute\r\n");
        this->SendModalHelp();
        send_to_desc(desc,
                     "     &C*&B-------------------------------------------------------&C*&n\r\n");
	} else {
        HelpItem *help_item;

		inStr = one_argument(inStr, command);
		*command = tolower(*command);
        help_item = _Help->FindItems(tmp_sprintf("tedii-%c", *command), false, 0, false);
        if (help_item) {
            help_item->LoadText();
            send_to_desc(desc, "&cTEDII Command '%c'&n\r\n", *command);
            SEND_TO_Q(help_item->text, desc);
        } else {
            send_to_desc(desc, "Sorry.  There is no help on that.\r\n");
        }
    }
}

bool
parse_optional_range(const char *arg, int &start, int &finish)
{
    const char *dash = strchr(arg, '-');

    if (dash) {
        char *str;

        // Parse range
        str = tmp_substr(arg, 0, dash - arg - 1);
        if (!isnumber(str))
            return false;
        start = atoi(str);

        str = tmp_substr(arg, dash - arg + 1);
        if (!isnumber(str))
            return false;
        finish = atoi(str);

        if (start > finish)
            swap(start, finish);

        return true;
    }

    // Ensure single arg is numeric
    if (!isnumber(arg))
        return false;

    // Single number
    start = finish = atoi(arg);
    return true;
}

bool
CEditor_PerformCommand(char cmd, char *args)
{
	int line, start_line, end_line, dest_line;
	char command[MAX_INPUT_LENGTH];

	switch (tolower(cmd)) {
	case 'h':					// Help
		ProcessHelp(args);
        break;
	case 'e':					// Save and Exit
        Finish(true);
		break;
	case 'q':					// Quit without saving
        Finish(false);
		break;
	case 's':					// Substitute
        Substitute(args);
		break;
    case 'f':                   // Find
        Find(args);
        break;
	case 'c':					// Clear Buffer
		Clear();
		editor_emit(editor, "Cleared.\r\n");
		break;
	case 'l':					// Replace Line
		args = one_argument(args, command);
		if (!isdigit(*command)) {
			editor_emit(editor, "Format for Replace Line is: &l <line #> <text>\r\n");
			break;
		}
		line = atoi(command);
		if (line < 1) {
			editor_emit(editor, "Format for Replace Line is: &l <line #> <text>\r\n");
			break;
		}
        ReplaceLine(line, args);
		break;
	case 'i':					// Insert Line
		args = one_argument(args, command);
		if (!isdigit(*command)) {
			SendMessage
				("Format for insert command is: &i <line #> <text>\r\n");
			break;
		}
		line = atoi(command);
		if (line < 1) {
			editor_emit(editor, "Format for insert command is: &i <line #><text>\r\n");
			break;
		}
        Insert(line, args);
		break;
	case 'd':					// Delete Line
		args = one_argument(args, command);
        if (!parse_optional_range(command, start_line, end_line)) {
			editor_emit(editor, "Format for delete command is: &d <line #>\r\n");
			break;
		}
        Remove(start_line, end_line);
		break;
    case 'm': {
        char *arg;

        arg = tmp_getword(&args);
        if (!*arg) {
            editor_emit(editor, "Format for move command is: &m (<start line>-<end line>|<linenum>) <destination>\r\n");
            break;
        }

        if (!parse_optional_range(arg, start_line, end_line)) {
            editor_emit(editor, "Format for move command is: &m (<start line>-<end line>|<linenum>) <destination>\r\n");
            break;
        }

        arg = tmp_getword(&args);
        if (!*arg || !isnumber(arg)) {
            editor_emit(editor, "Format for move command is: &m (<start line>-<end line>|<line #>) <destination>\r\n");
            break;
        }
        dest_line = atoi(arg);

        if (dest_line >= start_line && dest_line <= end_line + 1) {
            editor_emit(editor, "The line range you specified is already at the destination.\r\n");
            break;
        }

        MoveLines(start_line, end_line, dest_line);
        break;
    }
	case 'r':					// Refresh Screen
        if (!*args) {
            editor_display(editor, 0, -1);
        } else if (isnumber(args) && (line = atoi(args)) > 0) {
            int line_count = desc->account->get_term_height();

            editor_display(editor, MAX(1, line - line_count / 2), line_count);
		} else {
            editor_emit(editor, "Format for refresh command is: &r [<line #>]\r\nOmit line number to display the whole buffer.\r\n");
        }
		break;
	default:
		return false;
	}

	return true;
}
