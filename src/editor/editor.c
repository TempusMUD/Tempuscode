//
// File: editor.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//
//

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
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
#include "account.h"
#include "screen.h"
#include "tmpstr.h"
#include "accstr.h"
#include "prog.h"
#include "help.h"
#include "editor.h"
#include "strutil.h"

extern struct descriptor_data *descriptor_list;
extern struct help_collection *Help;

int
pin(int val, int min, int max)
{
    if (val < min) {
        return min;
    }
    if (val > max) {
        return max;
    }
    return val;
}

bool
already_being_edited(struct creature *ch, char *buffer)
{
    struct descriptor_data *d = NULL;

    for (d = descriptor_list; d; d = d->next) {
        if (d->text_editor
            && d->text_editor->is_editing
            && d->text_editor->is_editing(d->text_editor, buffer)) {
            send_to_char(ch, "%s is already editing that buffer.\r\n",
                         d->creature ? PERS(d->creature, ch) : "BOGUSMAN");
            return true;
        }
    }
    return false;
}


void
editor_import(struct editor *editor, const char *text)
{
    gchar **strv;
    gint strv_end;

    g_list_free(editor->original);
    editor->original = NULL;
    g_list_free(editor->lines);
    editor->lines = NULL;

    strv = g_strsplit(text, "\n", 0);
    strv_end = g_strv_length(strv) - 1;
    for (int i = 0; i < strv_end; i++) {
        g_strchomp(strv[i]);
        editor->original = g_list_prepend(editor->original, g_string_new(strv[i]));
        editor->lines = g_list_prepend(editor->lines, g_string_new(strv[i]));
    }
    g_strfreev(strv);

    editor->original = g_list_reverse(editor->original);
    editor->lines = g_list_reverse(editor->lines);
}

void
editor_emit(struct editor *editor, const char *text)
{
    d_printf(editor->desc, "%s", text);
}

void
emit_editor_startup(struct editor *editor)
{
    int i;

    d_printf(editor->desc,
             "&C     * &YTED///  &b]&n Save and exit with @ on a new line. "
             "&&H for help             &C*\r\n");
    d_printf(editor->desc, "     ");
    for (i = 0; i < 7; i++) {
        d_printf(editor->desc, "&C%d&B---------", i);
    }
    d_printf(editor->desc, "&C7&n\r\n");
    editor->displaybuffer(editor, 1, 0);
}

guint
editor_line_count(struct editor *editor)
{
    return g_list_length(editor->lines);
}

void
editor_send_prompt(struct editor *editor)
{
    d_printf(editor->desc, "%3d&b]&n ", editor_line_count(editor) + 1);
}

int
editor_buffer_size(struct editor *editor)
{
    int len = 0;

    for (GList *it = editor->lines; it; it = it->next) {
        GString *str = it->data;

        len += str->len + 2;
    }

    return len;
}

void
editor_finish(struct editor *editor, bool save)
{
    struct descriptor_data *desc = editor->desc;

    if (save) {
        int length = editor_buffer_size(editor);
        size_t buf_size = length + 1;
        char *text = (char *)malloc(buf_size);
        char *write_pt = text;

        *write_pt = '\0';

        for (GList *it = editor->lines; it; it = it->next) {
            GString *line = it->data;

            strcpy_s(write_pt, buf_size, line->str);
            write_pt += line->len;
            buf_size -= line->len;
            strcpy_s(write_pt, buf_size, "\r\n");
            write_pt += 2;
            buf_size -= 2;
        }

        // Call the finalizer
        editor->finalize(editor, text);

        free(text);
    } else {
        editor->cancel(editor);
    }

    if (IS_PLAYING(desc) && desc->creature && !IS_NPC(desc->creature)) {
        // Remove "using the editor bits"
        REMOVE_BIT(PLR_FLAGS(desc->creature),
                   PLR_WRITING | PLR_OLC | PLR_MAILING);
    }
    // Free the editor
    desc->text_editor = NULL;
    g_list_foreach(editor->original, (GFunc) g_string_free,
                   GINT_TO_POINTER(true));
    g_list_free(editor->original);
    g_list_foreach(editor->lines, (GFunc) g_string_free,
                   GINT_TO_POINTER(true));
    g_list_free(editor->lines);
    free(editor);
}

void
editor_finalize(struct editor *editor __attribute__((unused)), const char *text __attribute__((unused)))
{
    // Do nothing
}

void
editor_cancel(struct editor *editor __attribute__((unused)))
{
    // Do nothing
}

bool
editor_is_full(struct editor *editor, char *line)
{
    return (strlen(line) + editor_buffer_size(editor) > editor->max_size);
}

void
editor_append(struct editor *editor, char *line)
{
    if (editor_is_full(editor, line)) {
        editor_emit(editor, "Error: The buffer is full.\r\n");
        return;
    }
    if (PLR_FLAGGED(editor->desc->creature, PLR_OLC)) {
        g_strdelimit(line, "~", '?');
    }
    editor->lines = g_list_append(editor->lines, g_string_new(line));
}

void
editor_handle_input(struct editor *editor, char *input)
{
    // 2 special chars, @ and &
    char inbuf[MAX_INPUT_LENGTH + 1];
    char *args;

    strncpy(inbuf, input, MAX_INPUT_LENGTH);

    if (*inbuf == '&') {        // Commands
        if (!inbuf[1]) {
            editor_emit(editor,
                        "&& is the command character. Type &h for help.\r\n");
            return;
        }
        args = inbuf + 2;
        while (*args && isspace(*args)) {
            args++;
        }
        if (!editor->do_command(editor, inbuf[1], args)) {
            editor_emit(editor, "Invalid Command. Type &&h for help.\r\n");
        }
    } else if (*inbuf == '@') { // Finish up
        editor_finish(editor, true);
    } else {                    // Dump the text in
        editor_append(editor, inbuf);
    }
}

void
editor_display(struct editor *editor, unsigned int start_line, unsigned int display_lines)
{
    unsigned int linenum, end_line;
    unsigned int line_count;
    GList *itr;

    acc_string_clear();

    line_count = editor_line_count(editor);

    // Ensure that the start line is valid
    if (start_line > line_count) {
        // Display nothing if the start line is more than the number of lines
        return;
    }
    start_line = MAX(1, start_line);

    // Calculate last line number we're going to show
    end_line = line_count + 1;
    if (display_lines > 0 && start_line + display_lines < end_line) {
        end_line = start_line + display_lines;
    }

    // Set the iterator to the beginning line
    itr = g_list_nth(editor->lines, start_line - 1);

    // Display the lines from the beginning line to the end, making
    // sure we don't overflow the LARGE_BUF desc buffer
    for (linenum = start_line; linenum < end_line;
         linenum++, itr = g_list_next(itr)) {
        acc_sprintf("%3d%s%s]%s %s\r\n", linenum, CCBLD(editor->desc->creature,
                                                        C_CMP), CCBLU(editor->desc->creature, C_NRM),
                    CCNRM(editor->desc->creature, C_NRM),
                    ((GString *) itr->data)->str);
        if (acc_get_length() > (LARGE_BUFSIZE - 1024)) {
            break;
        }
    }

    acc_strcat("\r\n", NULL);
    if (acc_get_length() > (LARGE_BUFSIZE - 1024)) {
        acc_strcat
            ("Output buffer limit reached. Use \"&&r <line number>\" to specify starting line.\r\n",
            NULL);
    }

    editor_emit(editor, acc_get_string());
}

bool
editor_insert(struct editor *editor, unsigned int lineno, char *line)
{
    GList *s;

    if (lineno < 1) {
        lineno = 1;
    }
    if (*line) {
        line++;
    }

    if (lineno > editor_line_count(editor)) {
        editor_emit(editor,
                    "You can't insert before a line that doesn't exist.\r\n");
        return false;
    }
    if (editor_is_full(editor, line)) {
        editor_emit(editor, "Error: The buffer is full.\r\n");
        return false;
    }


    s = g_list_nth(editor->lines, lineno - 1);
    editor->lines = g_list_insert_before(editor->lines, s, g_string_new(line));

    return true;
}

bool
editor_replace_line(struct editor *editor, unsigned int lineno, char *line)
{
    GList *s;

    if (*line && *line == ' ') {
        line++;
    }

    if (lineno < 1 || lineno > editor_line_count(editor)) {
        editor_emit(editor, "There's no line to replace there.\r\n");
        return false;
    }
    // Find the line
    s = g_list_nth(editor->lines, lineno - 1);

    // Make sure we can fit the new stuff in
    if (editor_is_full(editor, line)) {
        editor_emit(editor, "Error: The buffer is full.\r\n");
        return false;
    }
    g_string_assign(s->data, line);

    return true;
}

bool
editor_movelines(struct editor *editor,
                 unsigned int start_line, unsigned int end_line, unsigned int dest_line)
{
    GList *dest, *begin, *end, *next;
    GList *cur;

    if (start_line < 1 || start_line > editor_line_count(editor)) {
        if (start_line == end_line) {
            editor_emit(editor, "Line %d is an invalid line.\r\n");
        } else {
            editor_emit(editor, "Starting line %d is an invalid line.\r\n");
        }
        return false;
    }
    if (end_line < 1 || end_line > editor_line_count(editor)) {
        editor_emit(editor, "Ending line %d is an invalid line.\r\n");
        return false;
    }
    // Nothing needs to be done if destination is inside the block
    if (dest_line >= start_line && dest_line <= end_line) {
        return true;
    }

    if (dest_line < 1) {
        dest_line = 1;
    }
    if (dest_line >= editor_line_count(editor)) {
        dest_line = editor_line_count(editor) + 1;
    }

    begin = g_list_nth(editor->lines, start_line);
    end = g_list_nth(editor->lines, end_line + 1);
    dest = g_list_nth(editor->lines, dest_line);

    for (cur = begin; cur != end; cur = next) {
        next = cur->next;
        editor->lines = g_list_insert_before(editor->lines, dest, cur->data);
        editor->lines = g_list_delete_link(editor->lines, cur);
    }

    if (start_line == end_line) {
        editor_emit(editor, tmp_sprintf("Moved line %d above line %d.\r\n",
                                        start_line, dest_line));
    } else {
        editor_emit(editor,
                    tmp_sprintf("Moved lines %d-%d to the line above line %d.\r\n",
                                start_line, end_line, dest_line));
    }

    return true;
}

bool
editor_find(struct editor *editor, char *args)
{
    int i = 0;

    acc_string_clear();
    for (GList *it = editor->lines; it; it = it->next) {
        GString *str = it->data;

        i++;
        if (strcasestr(str->str, args)) {
            acc_sprintf("%3d%s%s]%s %s\r\n", i,
                        CCBLD(editor->desc->creature, C_CMP),
                        CCBLU(editor->desc->creature, C_NRM),
                        CCNRM(editor->desc->creature, C_NRM), str->str);
        }
    }
    acc_strcat("\r\n", NULL);
    editor_emit(editor, acc_get_string());

    return true;
}

bool
editor_substitute(struct editor *editor, char *args)
{
    const char *usage =
        "There are two formats for substitute:\r\n  &&s [search pattern] [replacement]\r\n  &&s /search pattern/replacement/\r\nIn the first form, you can use (), [], <>, or {}.\r\n";
    // Iterator to the current line in theText
    GList *line;
    // The string containing the search and replacement pattern
    GString *pattern, *replacement;
    // Number of replacements made
    int replaced = 0;
    // read pointer and write pointer.
    char *start, end_char;
    bool balanced;
    int size_delta;
    guint buffer_size;

    while (isspace(*args)) {
        args++;
    }

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
        end_char = ')';
        break;
    case '[':
        end_char = ']';
        break;
    case '{':
        end_char = '}';
        break;
    case '<':
        end_char = '>';
        break;
    default:
        end_char = *args;
        balanced = false;
        break;
    }
    args++;

    start = args;
    while (*args && *args != end_char) {
        args++;
    }
    if (!*args) {
        editor_emit(editor, usage);
        return false;
    }

    pattern = g_string_new_len(start, args - start);

    // If the pattern was specified with a balanced delimiter, then
    // the replacement must be, too
    if (balanced) {
        while (*args
               && *args != '(' && *args != '[' && *args != '{' && *args != '<') {
            args++;
        }
        if (!*args) {
            editor_emit(editor,
                        "If the search pattern uses a balanced delimiter, the replacement must use a balanced\r\ndelimiter as well.\r\n");
            g_string_free(pattern, true);
            return false;
        }

        switch (*args) {
        case '(':
            end_char = ')';
            break;
        case '[':
            end_char = ']';
            break;
        case '{':
            end_char = '}';
            break;
        case '<':
            end_char = '>';
            break;
        default:
            end_char = *args;
            break;
        }
    }

    args++;
    start = args;
    while (*args && *args != end_char) {
        args++;
    }

    replacement = g_string_new_len(start, args - start);

    size_delta = replacement->len - pattern->len;
    buffer_size = editor_buffer_size(editor);

    // Find pattern in theText a line at a time and replace each instance.
    char *pos;                  // Current position in the line

    for (line = editor->lines;
         line && buffer_size + size_delta < editor->max_size;
         line = line->next) {
        pos = strstr(((GString *) line->data)->str, pattern->str);
        while (pos && buffer_size + size_delta < editor->max_size) {
            g_string_erase((GString *) line->data,
                           pos - ((GString *) line->data)->str, pattern->len);
            g_string_insert_len((GString *) line->data,
                                pos - ((GString *) line->data)->str,
                                replacement->str, replacement->len);
            replaced++;
            buffer_size += size_delta;
            pos = strstr(pos + replacement->len, pattern->str);
        }
    }
    if (buffer_size + size_delta > editor->max_size) {
        editor_emit(editor, "Error: The buffer is full.\r\n");
    }
    if (replaced > 0) {
        editor_emit(editor,
                    tmp_sprintf("Replaced %d occurrence%s of '%s' with '%s'.\r\n",
                                replaced, (replaced == 1) ? "" : "s", pattern->str, replacement->str));
    } else {
        editor_emit(editor, "Search string not found.\r\n");
    }
    g_string_free(pattern, true);
    return true;
}

bool
parse_optional_range(const char *arg, int *start, int *finish)
{
    char *endp = NULL;

    skip_spaces_const(&arg);
    *start = strtol(arg, &endp, 10);
    skip_spaces(&endp);
    if (endp == arg || (*endp != '\0' && *endp != '-') || *start < 1) {
        return false;
    }

    if (*endp == '\0') {
        // Single number
        *finish = *start;
        return true;
    }
    // range with a dash
    arg = endp + 1;
    skip_spaces_const(&arg);
    *finish = strtol(arg, &endp, 10);
    skip_spaces(&endp);
    if (endp == arg || *endp != '\0' || *finish < 1) {
        return false;
    }

    if (*start > *finish) {
        int tmp = *finish;

        *finish = *start;
        *start = tmp;
    }
    return true;
}

bool
editor_wrap(struct editor *editor, char *args)
{
    const char *usage = "Usage: &&w [<start line #>][-<end line #>]\r\n";
    GList *line_it, *start_line, *finish_line;
    int start_lineno, end_lineno;

    start_lineno = 0;
    end_lineno = g_list_length(editor->lines);

    if (*args) {
        if (!parse_optional_range(args, &start_lineno, &end_lineno)) {
            editor_emit(editor, usage);
            return false;
        }
        start_lineno = pin(start_lineno, 1, editor_line_count(editor)) - 1;
        end_lineno = pin(end_lineno, 1, editor_line_count(editor));
    }

    start_line = g_list_nth(editor->lines, start_lineno);
    finish_line = g_list_nth(editor->lines, end_lineno);

    GString *old_text = g_string_new("");
    bool empty_line_found = false;

    // Build flat string with paragraph breaks
    for (line_it = start_line;
         line_it != finish_line;
         line_it = line_it->next) {
        GString *line = line_it->data;
        char *start = line->str;

        skip_spaces(&start);
        if (!*start) {
            empty_line_found = true;
            continue;
        }

        if (isspace(line->str[0]) || empty_line_found) {
            g_string_append(old_text, "\n");
            skip_spaces(&start);
        }
        g_string_append(old_text, start);
        g_string_append(old_text, " ");

        empty_line_found = false;
    }

    // Format flat string
    int starting_indent = (isspace(((GString *)start_line->data)->str[0])) ? 3 : 0;
    char *new_text = tmp_format(old_text->str, 72, starting_indent, 3, 0);

    // Delete old text from buffer
    while (start_line != finish_line) {
        line_it = start_line->next;
        g_string_free(start_line->data, true);
        editor->lines = g_list_delete_link(editor->lines, start_line);
        start_line = line_it;
    }

    // Insert new text into old position
    gchar **strv;
    gint strv_end;

    strv = g_strsplit(new_text, "\n", 0);
    strv_end = g_strv_length(strv) - 1;
    for (int i = 0; i < strv_end; i++) {
        editor->lines = g_list_insert_before(editor->lines, start_line, g_string_new(strv[i]));
    }

    editor_display(editor, start_lineno + 1, g_strv_length(strv));

    g_strfreev(strv);

    return true;
}

bool
editor_remove(struct editor *editor,
              unsigned int start_line, unsigned int finish_line)
{
    GList *start, *finish, *next;

    if (start_line < 1 || start_line > editor_line_count(editor)) {
        editor_emit(editor, "Someone already deleted that line boss.\r\n");
        return false;
    }
    if (finish_line < 1 || finish_line > editor_line_count(editor)) {
        editor_emit(editor, "Someone already deleted that line boss.\r\n");
        return false;
    }

    start = g_list_nth(editor->lines, start_line - 1);
    finish = g_list_nth(editor->lines, finish_line);
    while (start != finish) {
        next = start->next;
        g_string_free((GString *) start->data, true);
        editor->lines = g_list_delete_link(editor->lines, start);
        start = next;
    }

    if (start_line == finish_line) {
        editor_emit(editor, tmp_sprintf("Line %d deleted.\r\n", start_line));
    } else {
        editor_emit(editor, tmp_sprintf("Lines %d-%d deleted.\r\n",
                                        start_line, finish_line));
    }

    return true;
}

bool
editor_clear(struct editor *editor)
{
    g_list_foreach(editor->lines, (GFunc) g_string_free,
                   GINT_TO_POINTER(true));
    g_list_free(editor->lines);
    editor->lines = NULL;

    return true;
}

void
editor_undo(struct editor *editor)
{
    if (editor->original) {
        editor_clear(editor);
        GList *line;

        for (line = editor->original; line; line = line->next) {
            GString *new_line = g_string_new_len(((GString *) line->data)->str,
                                                 ((GString *) line->data)->len);
            editor->lines = g_list_prepend(editor->lines, new_line);
        }

        editor->lines = g_list_reverse(editor->lines);

        editor_emit(editor, "Original buffer restored.\r\n");
    } else {
        editor_emit(editor, "There's no original to undo to.\r\n");
    }
}

void
editor_sendmodalhelp(struct editor *editor)
{
    // default offers the clear buffer and undo changes options
    d_printf(editor->desc,
             "            &YC - &nClear Buffer         &YU - &nUndo Changes  \r\n");
}

void
editor_help(struct editor *editor, char *line)
{
    char *command;

    acc_string_clear();

    if (!*line) {
        d_printf(editor->desc,
                 "     &C*&B-----------------------&Y H E L P &B-----------------------&C*\r\n"
                 "            &YR - &nRefresh Screen       &YH - &nHelp         \r\n"
                 "            &YE - &nSave and Exit        &YQ - &nQuit (Cancel)\r\n"
                 "            &YL - &nReplace Line         &YD - &nDelete Line  \r\n"
                 "            &YI - &nInsert Line          &YM - &nMove Line(s)\r\n"
                 "            &YF - &nFind                 &YS - &nSubstitute\r\n");
        editor->sendmodalhelp(editor);
        d_printf(editor->desc,
                 "     &C*&B-------------------------------------------------------&C*&n\r\n");
    } else {
        struct help_item *help_item;

        command = tmp_getword(&line);
        *command = tolower(*command);
        help_item = help_collection_find_items(help,
                                               tmp_sprintf("tedii-%c", *command), false, 0, false);
        if (help_item) {
            help_item_load_text(help_item);
            d_printf(editor->desc, "&cTEDII Command '%c'&n\r\n", *command);
            d_printf(editor->desc, "%s", help_item->text);
        } else {
            d_printf(editor->desc,
                     "Sorry.  There is no help on that.\r\n");
        }
    }
}

bool
editor_do_command(struct editor *editor, char cmd, char *args)
{
    int line, start_line, end_line, dest_line;
    char command[MAX_INPUT_LENGTH];

    switch (tolower(cmd)) {
    case 'h':                  // Help
        editor_help(editor, args);
        break;
    case 'e':                  // Save and Exit
        editor_finish(editor, true);
        break;
    case 'q':                  // Quit without saving
        editor_finish(editor, false);
        break;
    case 's':                  // Substitute
        editor_substitute(editor, args);
        break;
    case 'f':                  // Find
        editor_find(editor, args);
        break;
    case 'c':                  // Clear Buffer
        editor_clear(editor);
        editor_emit(editor, "Cleared.\r\n");
        break;
    case 'l':                  // Replace Line
        args = one_argument(args, command);
        if (!isdigit(*command)) {
            editor_emit(editor,
                        "Format for Replace Line is: &&l <line #> <text>\r\n");
            break;
        }
        line = atoi(command);
        if (line < 1) {
            editor_emit(editor,
                        "Format for Replace Line is: &&l <line #> <text>\r\n");
            break;
        }
        editor_replace_line(editor, line, args);
        break;
    case 'i':                  // Insert Line
        args = one_argument(args, command);
        if (!isdigit(*command)) {
            editor_emit(editor,
                        "Format for insert command is: &&i <line #> <text>\r\n");
            break;
        }
        line = atoi(command);
        if (line < 1) {
            editor_emit(editor,
                        "Format for insert command is: &&i <line #><text>\r\n");
            break;
        }
        editor_insert(editor, line, args);
        break;
    case 'd':                  // Delete Line
        args = one_argument(args, command);
        if (!parse_optional_range(command, &start_line, &end_line)) {
            editor_emit(editor,
                        "Format for delete command is: &&d <line #>\r\n");
            break;
        }
        editor_remove(editor, start_line, end_line);
        break;
    case 'm': {
        char *arg;

        arg = tmp_getword(&args);
        if (!*arg) {
            editor_emit(editor,
                        "Format for move command is: &&m (<start line>-<end line>|<linenum>) <destination>\r\n");
            break;
        }

        if (!parse_optional_range(arg, &start_line, &end_line)) {
            editor_emit(editor,
                        "Format for move command is: &&m (<start line>-<end line>|<linenum>) <destination>\r\n");
            break;
        }

        arg = tmp_getword(&args);
        if (!*arg || !is_number(arg)) {
            editor_emit(editor,
                        "Format for move command is: &&m (<start line>-<end line>|<line #>) <destination>\r\n");
            break;
        }
        dest_line = atoi(arg);

        if (dest_line >= start_line && dest_line <= end_line + 1) {
            editor_emit(editor,
                        "The line range you specified is already at the destination.\r\n");
            break;
        }

        editor_movelines(editor, start_line, end_line, dest_line);
        break;
    }
    case 'r':                  // Refresh Screen
        if (!*args) {
            editor->displaybuffer(editor, 1, 0);
        } else if (is_number(args) && (line = atoi(args)) > 0) {
            int line_count = editor->desc->account->term_height;

            editor->displaybuffer(editor, MAX(1, line - line_count / 2), line_count);
        } else {
            editor_emit(editor,
                        "Format for refresh command is: &&r [<line #>]\r\nOmit line number to display the whole buffer.\r\n");
        }
        break;
    case 'w':
        editor_wrap(editor, args);
        break;
    default:
        return false;
    }

    return true;
}

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
