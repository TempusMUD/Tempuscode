//
// File: editor.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//
//

#include <string>
#include <list>
using namespace std;
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
#include "player_table.h"

extern struct descriptor_data *descriptor_list;
extern HelpCollection *Help;

/* Sets up text editor params and echo's passed in message.
*/


// Constructor
// Params: Users descriptor, The final destination of the text, 
//      the max size of the text.
CEditor::CEditor(struct descriptor_data *d, int max)
    :theText()
{
	desc = d;
    curSize = 0;
	maxSize = max;
}

void
CEditor::SendStartupMessage(void)
{
    send_to_desc(desc, "&C    * &YTEDII &b]&n Save and exit with @ on a new line. &&H for help             &C*\r\n");
    send_to_desc(desc, "    ");
	for (int i = 0; i < 7; i++)
        send_to_desc(desc, "&C%d&B---------", i);
    send_to_desc(desc, "&C7&n\r\n");
}

void
CEditor::SendPrompt(void)
{
    send_to_desc(desc, "%2d&b]&n ", theText.size() + 1);
}

void
CEditor::Finish(bool save)
{

    if (save) {
        list <string>::iterator itr;
        int length;
        char *text;

        length = curSize + (theText.size() * 2);
        text = (char *)malloc(sizeof(char) * length + 3);
        strcpy(text, "");
        for (itr = theText.begin(); itr != theText.end(); itr++) {
            strcat(text, itr->c_str());
            strcat(text, "\r\n");
        }

        // Call the finalizer
        this->Finalize(text);
        
        free(text);
    }

	if (IS_PLAYING(desc) && desc->creature && !IS_NPC(desc->creature)) {
        // Remove "using the editor bits"
        REMOVE_BIT(PLR_FLAGS(desc->creature),
                   PLR_WRITING | PLR_OLC | PLR_MAILING);
        REMOVE_BIT(PRF2_FLAGS(desc->creature), PRF2_NOWRAP);
	}

    // Free the editor
	desc->text_editor = NULL;
	delete this;
}

void
CEditor::Process(char *inStr)
{
	// 2 special chars, @ and &
	char inbuf[MAX_INPUT_LENGTH + 1];
    char *args;
	strncpy(inbuf, inStr, MAX_INPUT_LENGTH);

	delete_doubledollar(inbuf);

	if (*inbuf == '&') {		// Commands
        args = inbuf + ((inbuf[2]) ? 3:2);
        if (*args)
            args++;
        if (!this->PerformCommand(inbuf[1], inbuf + 3))
            SendMessage("Invalid Command. Type &h for help.\r\n");
	} else if (*inbuf == '@') {	// Finish up
        Finish(true);
	} else {					// Dump the text in
		Append(inbuf);
	}
}

void
CEditor::DisplayBuffer(unsigned int startline)
{
	list <string>::iterator itr;
	unsigned int i;

    acc_string_clear();

	itr = theText.begin();

    for (i = 1;i < startline && itr != theText.end();i++)
         itr++;

	for (i = startline; itr != theText.end(); i++, itr++) {
		acc_sprintf("%2d%s%s]%s %s\r\n", i,
                    CCBLD(desc->creature, C_CMP),
                    CCBLU(desc->creature, C_NRM),
                    CCNRM(desc->creature, C_NRM),
                    itr->c_str());
		// Overflowing the LARGE_BUF desc buffer.
		if (acc_get_length() > 10240) {
			break;
		}
	}
	SendMessage(acc_get_string());
	if (acc_get_length() > 10240)
		SendMessage("Output buffer limit reached. Use \"&r <line number>\" to specify starting line.\r\n");
}

bool
CEditor::Full(char *inStr)
{
	if ((strlen(inStr) + curSize) + ((theText.size() + 1) * 2) > maxSize) {
		return true;
	}
	return false;
}

void
CEditor::Append(char *inStr)
{

	if (Full(inStr)) {
		SendMessage("Error: The buffer is full.\r\n");
		return;
	}
	// All tildes must die
	if (PLR_FLAGGED(desc->creature, PLR_OLC)) {
		char *readPt, *writePt;

		readPt = writePt = inStr;
		while (*readPt) {
			if (*readPt != '~')
				*writePt++ = *readPt;
			readPt++;
		}
		*writePt = '\0';
	}
	theText.push_back(inStr);
	Wrap();
	UpdateSize();
}

bool
CEditor::Insert(unsigned int line, char *inStr)
{
	string text;
	list <string>::iterator s;
	unsigned int i = 1;

	if (*inStr) {
		inStr++;
	}
	text = inStr;
	if (line > theText.size()) {
		SendMessage("You can't insert before a line that doesn't exist.\r\n");
		return false;
	}
	if ((text.length() + curSize) + ((theText.size() + 1) * 2) > maxSize) {
		SendMessage("Error: The buffer is full.\r\n");
		return false;
	}
	// Find it again (the one after it actually)
	for (s = theText.begin(); i < line; i++, s++);

	// Insert the new text
	theText.insert(s, text);
	Wrap();
	UpdateSize();

	return true;
}

bool
CEditor::ReplaceLine(unsigned int line, char *inStr)
{
	string text;
	list <string>::iterator s;
	unsigned int i = 1;

	if (*inStr && *inStr == ' ')
		inStr++;
	text = inStr;

	if (line > theText.size()) {
		SendMessage("There's no line to replace there.\r\n");
		return false;
	}
	// Find the line
	for (i = 1, s = theText.begin(); i < line; i++, s++);

	// Make sure we can fit the new stuff in
	if ((text.length() + curSize - s->length()) +
		((theText.size() + 1) * 2) > maxSize) {
		SendMessage("Error: The buffer is full.\r\n");
		return false;
	}
	// Erase it
	theText.erase(s);

	// Find it again (the one after it actually)
	for (i = 1, s = theText.begin(); i < line; i++, s++);
	// Insert the new text
	theText.insert(s, text);

	Wrap();
	UpdateSize();
	return true;
}

bool
CEditor::FindReplace(char *args)
{
	// Iterator to the current line in theText
	list <string>::iterator line;
	// The string containing the search pattern
	string findit;
	// String containing the replace pattern
	string replaceit;
	// Number of replacements made
	int replaced = 0;
	// Temporary string.
	char temp[MAX_INPUT_LENGTH];
	// read pointer and write pointer.
	char *r, *w;

	for (r = args; *r == ' '; r++);

	if (!*r) {
		SendMessage
			("The format for find/replace is &f [search string] [replace string] \r\nYou must actually include the brackets\r\n");
		return false;
	}
// Find "findit"

	if (*r != '[') {
		SendMessage("Mismatched brackets.\r\n");
		return false;
	}
	// Advance past [
	r++;
	if (*r == ']') {
		SendMessage("You can't search for nothing...\r\n");
		return false;
	}

	for (w = temp; *r && *r != ']'; r++)
		*w++ = *r;
	// terminate w;
	*w = '\0';

	if (*r != ']') {
		SendMessage("Mismatched brackets.\r\n");
		return false;
	}
	// Advance past ]
	r++;
	findit = temp;

// Find "replaceit"
	for (; *r == ' '; r++);
	if (*r != '[') {
		SendMessage("Mismatched brackets.\r\n");
		return false;
	}
	// Advance past [
	r++;
	for (w = temp; *r && *r != ']'; r++)
		*w++ = *r;
	// terminate w;
	*w = '\0';

	if (*r != ']') {
		SendMessage("Mismatched brackets.\r\n");
		return false;
	}
	replaceit = temp;

	// Find "findit" in theText a line at a time and replace each instance.
	unsigned int pos;			// Current position in the line
	bool overflow = false;		// Have we overflowed the buffer?
	for (line = theText.begin(); !overflow && line != theText.end(); line++) {
		pos = 0;
		while (pos < line->length()) {
			pos = line->find(findit, pos);
			if (pos < line->length()) {
				if ((curSize - findit.length() + replaceit.length()) +
					((theText.size() + 1) * 2)
					> maxSize) {
					SendMessage("Error: The buffer is full.\r\n");
					overflow = true;
					break;
				}
				*line = line->replace(pos, findit.length(), replaceit);
				replaced++;
				pos += replaceit.length();
			}
			if (replaced >= 100) {
				SendMessage("Replacement limit of 100 reached.\r\n");
				overflow = true;
				break;
			}
		}
	}
	if (replaced > 0 && !overflow) {
		SendMessage(tmp_sprintf("%d occurrences of [%s] replaced with [%s].\r\n",
                                replaced, findit.c_str(), replaceit.c_str()));
	} else if (!overflow) {
		SendMessage("Search string not found.\r\n");
	}
	Wrap();
	UpdateSize();
	return true;
}

bool
CEditor::Wrap(void)
{
	list <string>::iterator line;
	string::iterator s;
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
CEditor::Remove(unsigned int line)
{
	list <string>::iterator s;
	unsigned int i;
	if (line > theText.size()) {
		SendMessage("Someone already deleted that line boss.\r\n");
		return false;
	}
	for (i = 1, s = theText.begin(); i < line; s++, i++);

	theText.erase(s);
	SendMessage(tmp_sprintf("Line %d deleted.\r\n", line));

	Wrap();
	UpdateSize();
	return true;
}

bool
CEditor::Clear(void)
{

	theText.erase(theText.begin(), theText.end());

	UpdateSize();
	return true;
}

void
CEditor::ImportText(char *str)
{
    char *line;

    origText.clear();
    if (str) {
        while ((line = tmp_getline(&str)) != NULL)
            origText.push_back(string(line));
    }

    theText = origText;

    UpdateSize();
	Wrap();
}

void
CEditor::UndoChanges(void)
{
	if (origText.size()) {
        Clear();
        theText = origText;
        UpdateSize();
        SendMessage("Original buffer restored.\r\n");
	} else {
		SendMessage("There's no original to undo to.\r\n");
    }
}

void
CEditor::SendMessage(const char *message)
{
	if (!desc || !desc->creature) {
		errlog("TEDII Attempting to SendMessage with null desc or desc->creature\r\n");
		return;
	}

	// If the original message is too long, make a new one thats small
	if (strlen(message) >= LARGE_BUFSIZE) {
        char *temp = NULL;

		slog("SendMessage Truncating message. NAME(%s) Length(%d)",
             GET_NAME(desc->creature),
             strlen(message));
        temp = new char[LARGE_BUFSIZE];
		strncpy(temp, message, LARGE_BUFSIZE - 2);
		send_to_char(desc->creature, "%s", temp);
        delete temp;
	} else {
        // If the original message is small enough, just let it through.
		send_to_char(desc->creature, "%s", message);
	}
}

static inline int
text_length(list <string> &theText)
{
	int length = 0;
	list <string>::iterator s;
	for (s = theText.begin(); s != theText.end(); s++) {
		length += s->length();
	}
	return length;
}

void
CEditor::UpdateSize(void)
{
	int linesRemoved = 0;

	// update the current size
	curSize = text_length(theText);

	// Buffer overflow state.
	// This is probably happening in response to word wrap
	while ((curSize + (theText.size() * 2) + 2) > maxSize) {
		theText.pop_back();
		curSize = text_length(theText);
		linesRemoved++;
	}

	// Warn the player if the buffer was truncated.
	if (linesRemoved > 0) {
		SendMessage(tmp_sprintf("Error: Buffer limit exceeded.  %d %s removed.\r\n",
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
CEditor::ProcessHelp(char *inStr)
{
	struct Creature *ch = desc->creature;
	char command[MAX_INPUT_LENGTH];

    acc_string_clear();

	if (!*inStr) {
        send_to_desc(desc,
                     "     &C*&B-----------------------&Y H E L P &B-----------------------&C*\r\n"
                     "            &YF - &nFind && Replace       &YH - &nHelp         \r\n"
                     "            &YS - &nSave and Exit        &YQ - &nQuit (Cancel)\r\n"
                     "            &YL - &nReplace Line         &YD - &nDelete Line  \r\n"
                     "            &YI - &nInsert Line          &YR - &nRefresh Screen\r\n");
        if (PLR_FLAGGED(ch, PLR_MAILING)) {
            // TODO: this should use virtual dispatch for extensibility.
            send_to_desc(desc,
                         "            &YC - &nClear Buffer         &YA - &nAdd Recipient\r\n"
                         "            &YT - &nList Recipients      &YE - &nRemove Recipient\r\n");
        } else {
            send_to_desc(desc, "            &YC - &nClear Buffer         &YU - &nUndo Changes  \r\n");
        }
        send_to_desc(desc,
                     "     &C*&B-------------------------------------------------------&C*&n\r\n");
	} else {
        HelpItem *help_item;

		inStr = one_argument(inStr, command);
		*command = tolower(*command);
        help_item = ::Help->FindItems(tmp_sprintf("tedii-%c", *command), false, 0, false);
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
CEditor::PerformCommand(char cmd, char *args)
{
	int line;
	char command[MAX_INPUT_LENGTH];

	switch (tolower(cmd)) {
	case 'h':					// Help
		ProcessHelp(args);
        break;
	case 'f':					// Find/Replace
        FindReplace(args);
		break;
	case 's':					// Save and Exit
        Finish(true);
		break;
	case 'l':					// Replace Line
		args = one_argument(args, command);
		if (!isdigit(*command)) {
			SendMessage("Format for Replace Line is: &l <line #> <text>\r\n");
			break;
		}
		line = atoi(command);
		if (line < 1) {
			SendMessage("Format for Replace Line is: &l <line #> <text>\r\n");
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
			SendMessage("Format for insert command is: &i <line #><text>\r\n");
			break;
		}
        Insert(line, args);
		break;
	case 'c':					// Clear Buffer
		Clear();
		SendMessage("Cleared.\r\n");
		break;
	case 'q':					// Quit without saving
        Finish(false);
		break;
	case 'd':					// Delete Line
		args = one_argument(args, command);
		if (!isdigit(*command)) {
			SendMessage("Format for delete command is: &d <line #>\r\n");
			break;
		}
		line = atoi(command);
		if (line < 1) {
			SendMessage("Format for delete command is: &d <line #>\r\n");
			break;
		}
        Remove(line);
		break;
	case 'r':					// Refresh Screen
		args = one_argument(args, command);
		if (!isdigit(*command)) {
			DisplayBuffer();
			break;
		}
		line = atoi(command);
		if (line < 1) {
			SendMessage
				("Format for refresh command is: &r <starting line #>\r\n");
			break;
		}
		DisplayBuffer((unsigned int)line);
		break;
	default:
		return false;
	}

	return true;
}
