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
#include "iscript.h"
#include "tmpstr.h"
#include "player_table.h"

static char small_editbuf[MAX_INPUT_LENGTH];
static char editbuf[MAX_STRING_LENGTH * 2];
static char tedii_out_buf[MAX_STRING_LENGTH];
extern struct descriptor_data *descriptor_list;

void voting_add_poll(void);

/* Sets up text editor params and echo's passed in message.
*/
void
start_text_editor(struct descriptor_data *d, char **dest, bool sendmessage, int max)
{
	/*  Editor Command
	   Note: Add info for logall
	 */
	// There MUST be a destination!
	if (!dest) {
		mudlog(LVL_IMMORT, BRF, true,
			"SYSERR: NULL destination pointer passed into start_text_editor!!");
		send_to_char(d->creature, "This command seems to be broken. Bug this.\r\n");
		REMOVE_BIT(PLR_FLAGS(d->creature),
			PLR_WRITING | PLR_OLC | PLR_MAILING);
		return;
	}
	if (d->text_editor) {
		mudlog(LVL_IMMORT, BRF, true,
			"SYSERR: Text editor object not null in start_text_editor.");
		REMOVE_BIT(PLR_FLAGS(d->creature),
			PLR_WRITING | PLR_OLC | PLR_MAILING);
		return;
	}
	if (*dest && (strlen(*dest) > (unsigned int)max)) {
		send_to_char(d->creature, "ERROR: Buffer too large for editor.\r\n");
		REMOVE_BIT(PLR_FLAGS(d->creature),
			PLR_WRITING | PLR_OLC | PLR_MAILING);
		return;
	}

	d->text_editor = new CTextEditor(d, dest, max, sendmessage);
}

void
start_script_editor(struct descriptor_data *d, list <string> dest,
	bool isscript)
{
	if (&dest == NULL) {
		mudlog(LVL_IMMORT, BRF, true,
			"SYSERR: NULL destination pointer passed into start_text_editor!!");
		send_to_char(d->creature, "This command seems to be broken. Bug this.\r\n");
		REMOVE_BIT(PLR_FLAGS(d->creature),
			PLR_WRITING | PLR_OLC | PLR_MAILING);
		return;
	}
	if (d->text_editor) {
		mudlog(LVL_IMMORT, BRF, true,
			"SYSERR: Text editor object not null in start_text_editor.");
		REMOVE_BIT(PLR_FLAGS(d->creature),
			PLR_WRITING | PLR_OLC | PLR_MAILING);
		return;
	}

	d->text_editor = new CTextEditor(d, dest, isscript);
}

void
CTextEditor::Process(char *inStr)
{
	// 2 special chars, @ and &
	char inbuf[MAX_INPUT_LENGTH + 1];
	strncpy(inbuf, inStr, MAX_INPUT_LENGTH);

	delete_doubledollar(inbuf);

	if (*inbuf == '&') {		// Commands
		ProcessCommand(inbuf);
		return;
	} else if (*inbuf == '@') {	// Finish up
		REMOVE_BIT(PRF2_FLAGS(desc->creature), PRF2_NOWRAP);
		SaveText(inbuf);
		desc->text_editor = NULL;
		delete this;
		return;
	} else {					// Dump the text in
		Append(inbuf);
	}
	desc->editor_cur_lnum = theText.size() + 1;
}

void
CTextEditor::List(unsigned int startline)
{
	list <string>::iterator itr;
	int i;
	int num_lines = 0;
	strcpy(editbuf, "\r\n");

	itr = theText.begin();

	// Allow a param, cut off when the buffer hits LARGE_BUF_SIZE
	if (startline > 1) {
		for (i = startline; i > 1 && itr != theText.end(); i--)
			itr++;
	}

	for (i = startline; itr != theText.end(); i++, itr++, num_lines++) {
		sprintf(tedii_out_buf, "%-2d%s%s]%s ", i,
			CCBLD(desc->creature, C_CMP),
			CCBLU(desc->creature, C_NRM), CCNRM(desc->creature, C_NRM));
		strcat(editbuf, tedii_out_buf);
		strcat(editbuf, itr->c_str());
		strcat(editbuf, "\r\n");
		// Overflowing the LARGE_BUF desc buffer.
		if (strlen(editbuf) > 10240) {
			break;
		}
	}
	SendMessage(editbuf);
	if (strlen(editbuf) > 10240) {
		sprintf(editbuf,
			"Output buffer limit reached. Use \"&r <line number>\" to specify starting line.\r\n");
		SendMessage(editbuf);
	}
}

void
CTextEditor::SaveText(char *inStr)
{
	struct mail_recipient_data *next_mail;
	list <string>::iterator itr;
	int length = 0;

	// If we were editing rather than creating, wax what was there.
	if (target) {
		if (*target) {
			free(*target);
			*target = NULL;
		}
	}

	length = curSize + (theText.size() * 2);
	if (target) {
		*target = (char*) malloc(sizeof(char) * length + 3);
		//*target = new char[length + 3];
		strcpy(*target, "");
		for (itr = theText.begin(); itr != theText.end(); itr++) {
			strcat(*target, itr->c_str());
			strcat(*target, "\r\n");
		}
	}
	// If they're in the game
	if (IS_PLAYING(desc)) {
		// Saving a file
		if ((desc->editor_file != NULL)) {
			SaveFile();
			free(*target);
			free(target);
		}
		// Sending Mail
		else if (PLR_FLAGGED(desc->creature, PLR_MAILING)) {
			ExportMail();
			free(*target);
			free(target);
		}
		// Creating a script handler
		else if ((PLR_FLAGGED(desc->creature, PLR_OLC)) && scripting) {
			desc->creature->player_specials->olc_handler->getTheLines() =
				theText;
		}
	}
	// Save the board if we were writing to a board
	if (desc->mail_to && desc->mail_to->recpt_idnum >= BOARD_MAGIC) {
		Board_save_board(desc->mail_to->recpt_idnum - BOARD_MAGIC);
		next_mail = desc->mail_to->next;
		free(desc->mail_to);
		desc->mail_to = next_mail;
	}
	// Add the poll if we were adding to a poll
	if (desc->mail_to && desc->mail_to->recpt_idnum == VOTING_MAGIC) {
		voting_add_poll();
		next_mail = desc->mail_to->next;
		free(desc->mail_to);
		desc->mail_to = next_mail;
	}
	// If editing thier description.
	if (STATE(desc) == CXN_EDIT_DESC) {
		send_to_desc(desc, "\033[H\033[J");
		set_desc_state(CXN_MENU, desc);
		return;
	}
	// Remove the "using the editor" bits.
	if (IS_PLAYING(desc) && desc->creature && !IS_NPC(desc->creature)) {
		tedii_out_buf[0] = '\0';
		// Decide what to say to the room since they're done
		if (PLR_FLAGGED(desc->creature, PLR_WRITING)) {
			sprintf(tedii_out_buf,
				"%s finishes writing.", GET_NAME(desc->creature));
		} else if (PLR_FLAGGED(desc->creature, PLR_OLC)) {
			sprintf(tedii_out_buf,
				"%s nods with satisfaction as $e saves $s work.",
				GET_NAME(desc->creature));
		} else if ((PLR_FLAGGED(desc->creature, PLR_OLC)) && scripting) {
			sprintf(tedii_out_buf,
				"%s nods with satisfaction as $e saves $s work.",
				GET_NAME(desc->creature));
		} else if (PLR_FLAGGED(desc->creature, PLR_MAILING)
			&& GET_LEVEL(desc->creature) >= LVL_AMBASSADOR) {
			sprintf(tedii_out_buf,
				"%s postmarks and dispatches $s mail.",
				GET_NAME(desc->creature));
		}
		// Let the room know that they're done
		if (tedii_out_buf[0] != '\0') {
			act(tedii_out_buf, TRUE, desc->creature, 0, 0, TO_NOTVICT);
		}
		REMOVE_BIT(PLR_FLAGS(desc->creature),
			PLR_WRITING | PLR_OLC | PLR_MAILING);
	}
}
//#alias {sms %1 %2} {#loop {1,%%1} {ms %0.%%2}}
void
CTextEditor::ExportMail(void)
{

	int stored_mail = 0;
	struct descriptor_data *r_d;
	struct mail_recipient_data *mail_rcpt = NULL;
    list<string> cc_list;

	// If they're trying to send a blank message
	if (!*target || !strlen(*target)) {
		SendMessage("Why would you send a blank message?\r\n");
        return;
	}

    for (mail_rcpt = desc->mail_to; mail_rcpt; mail_rcpt = mail_rcpt->next) {
        cc_list.push_back(playerIndex.getName(mail_rcpt->recpt_idnum));
	}
    cc_list.sort();
    cc_list.unique();

 	mail_rcpt = desc->mail_to;
	while (mail_rcpt) {
		desc->mail_to = desc->mail_to->next;
		free(mail_rcpt);
		mail_rcpt = desc->mail_to;
	}
   
//    stored_mail = store_mail(mail_to_id, GET_IDNUM(desc->creature), *target, cc_list);

    list<string>::iterator si;
    for (si = cc_list.begin(); si != cc_list.end(); si++) {
        long id = playerIndex.getID(si->c_str());
        stored_mail = store_mail(id, GET_IDNUM(desc->creature), *target,  cc_list);
        if (stored_mail == 1) {
            for (r_d = descriptor_list; r_d; r_d = r_d->next) {
                if (IS_PLAYING(r_d) && r_d->creature &&
                    (r_d->creature != desc->creature) &&
                    (GET_IDNUM(r_d->creature) == id) &&
                    (!PLR_FLAGGED(r_d->creature, PLR_WRITING | PLR_MAILING | PLR_OLC))) {
                    send_to_char(r_d->creature, "A strange voice in your head says, "
                                                "'You have new mail.'\r\n");
                }
            }
        }
    }

	if (stored_mail) {
		SendMessage("Message sent!\r\n");
    }
    else {
        SendMessage("Your message was not received by one or more recipients.\r\n"
                    "Please try again later!\r\n");
        slog("SYSERR: store_mail() has returned <= 0");
    }
}

void
CTextEditor::SaveFile(void)
{

	char filebuf[512], filename[64];
	int file_to_write;
	int backup_file, nread;

	sprintf(filename, "%s", desc->editor_file);
	if ((file_to_write = open(filename, O_RDWR, 0666)) == -1) {
		mudlog(LVL_AMBASSADOR, NRM, true,
			"Could not open file %s, buffer not saved!\r\n", filename);
	} else {
		sprintf(filename, "%s.bak", desc->editor_file);
		if ((backup_file =
				open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666)) == -1) {
			mudlog(LVL_AMBASSADOR, NRM, true,
				"Could not open file %s, buffer not saved!\r\n", filename);
			close(file_to_write);
		} else {
			while ((nread = read(file_to_write, filebuf, sizeof(filebuf))) > 0) {
				if (write(backup_file, filebuf, nread) != nread) {
					send_to_char(desc->creature, "Could not save backup file!!\r\n");
					break;
				}
			}
			close(backup_file);
			lseek(file_to_write, 0, 0);

			write(file_to_write, *target, strlen(*target));
			close(file_to_write);
		}
		free(desc->editor_file);
		desc->editor_file = NULL;
	}
}

bool
CTextEditor::Full(char *inStr)
{
	if ((strlen(inStr) + curSize) + ((theText.size() + 1) * 2) > maxSize) {
		return true;
	}
	return false;
}

void
CTextEditor::Append(char *inStr)
{

	if (Full(inStr)) {
		SendMessage("Error: The buffer is full.\r\n");
		return;
	}
	// All tildes must die
	if ((PLR_FLAGGED(desc->creature, PLR_OLC)) && scripting) {
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
CTextEditor::Insert(unsigned int line, char *inStr)
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
CTextEditor::ReplaceLine(unsigned int line, char *inStr)
{
	string text;
	list <string>::iterator s;
	unsigned int i = 1;

	if (*inStr && *inStr == ' ')
		inStr++;
	text = inStr;

	if (line > theText.size()) {
		SendMessage("Your going to have to write the line first dummy.\r\n");
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
CTextEditor::FindReplace(char *args)
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
		sprintf(tedii_out_buf, "%d occurances of [%s] replaced with [%s].\r\n",
			replaced, findit.c_str(), replaceit.c_str());
		SendMessage(tedii_out_buf);
	} else if (!overflow) {
		SendMessage("Search string not found.\r\n");
	}
	Wrap();
	UpdateSize();
	return true;
}

bool
CTextEditor::Wrap(void)
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
		// If its less than 77 chars, it dont need ta be wrapped.
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
CTextEditor::Remove(unsigned int line)
{
	list <string>::iterator s;
	unsigned int i;
	if (line > theText.size()) {
		SendMessage("Someone already deleted that line boss.\r\n");
		return false;
	}
	for (i = 1, s = theText.begin(); i < line; s++, i++);

	theText.erase(s);
	sprintf(tedii_out_buf, "Line %d deleted.\r\n", line);
	SendMessage(tedii_out_buf);

	Wrap();
	UpdateSize();
	return true;
}

bool
CTextEditor::Clear(void)
{

	theText.erase(theText.begin(), theText.end());

	desc->editor_cur_lnum = theText.size() + 1;
	UpdateSize();
	return true;
}

void
CTextEditor::ImportText(void)
{
	char *b, *s;				// s is the cursor, b is the beginning of the current line
	string text;
	strncpy(editbuf, *target, maxSize);
	s = editbuf;
	while (s && *s) {
		for (b = s; *s && *s != '\r'; s++);

		*s = '\0';
		s += 2;
		text = b;
		theText.push_back(text);
	}
	Wrap();
}

void
CTextEditor::UndoChanges(char *inStr)
{
	if (!*target && (origText.size() == 0)) {
		SendMessage("There's no original to undo to.\r\n");
		return;
	}
	Clear();
	if (origText.size() == 0)
		ImportText();
	else
		theText = origText;
	UpdateSize();
	desc->editor_cur_lnum = theText.size() + 1;
	SendMessage("Original buffer restored.\r\n");
	return;
}

// Constructor
// Params: Users descriptor, The final destination of the text, 
//      the max size of the text.
CTextEditor::CTextEditor(struct descriptor_data * d, char **dest, int max, bool startup):theText
	()
{
	// Internal pointer to the descriptor
	desc = d;
	// Internal pointer to the destination
	target = dest;

	scripting = false;
	// The maximum size of the buffer.
	maxSize = max;

	if (*target) {
		ImportText();
	}
	desc->editor_cur_lnum = theText.size() + 1;
	UpdateSize();
	if (startup) {
		SendStartupMessage();
		List();
	}
}

CTextEditor::CTextEditor(struct descriptor_data *d,
	list <string> dest, bool isscript):
theText()
{
	desc = d;

	//make DAMN sure target is null, crashes the mud otherwise
	target = NULL;

	origText = dest;

	scripting = isscript;

	maxSize = 1024;

	if (origText.size() > 0)
		theText = dest;

	desc->editor_cur_lnum = theText.size() + 1;
	UpdateSize();
	SendStartupMessage();
	List();
}

void
CTextEditor::SendMessage(const char *message)
{
	char *output = NULL;
	if (desc == NULL || desc->creature == NULL) {
		slog("SYSERR: TEDII Attempting to SendMessage with null desc or desc->creature\r\n");
		return;
	}
	// If the original message is too long, make a new one thats small
	if (strlen(message) >= LARGE_BUFSIZE) {
		sprintf(small_editbuf,
			"TEDERR: SendMessage Truncating message. NAME(%s) Length(%d)",
			GET_NAME(desc->creature), strlen(message));
		slog(small_editbuf);
		output = (char*) malloc( sizeof(char) * LARGE_BUFSIZE );
		//output = new char[LARGE_BUFSIZE];
		strncpy(output, message, LARGE_BUFSIZE - 2);
		send_to_char(desc->creature, output);
		free(output);
	} else {					// If the original message is small enough, just let it through.
		send_to_char(desc->creature, message);
	}
}

void
CTextEditor::SendStartupMessage(void)
{
	struct Creature *ch;
	ch = desc->creature;

	sprintf(tedii_out_buf, "%s%s    *", CCBLD(ch, C_CMP), CCCYN(ch, C_NRM));
	sprintf(tedii_out_buf, "%s%s TEDII ", tedii_out_buf, CCYEL(ch, C_NRM));
	sprintf(tedii_out_buf, "%s%s] ", tedii_out_buf, CCBLU(ch, C_NRM));
	sprintf(tedii_out_buf, "%s%sTerminate with @ on a new line. &H for help",
		tedii_out_buf, CCNRM(ch, C_NRM));
	sprintf(tedii_out_buf, "%s%s%s                 *\r\n", tedii_out_buf,
		CCBLD(ch, C_CMP), CCCYN(ch, C_NRM));
	sprintf(tedii_out_buf, "%s    %s", tedii_out_buf, CCBLD(ch, C_CMP));
	for (int x = 0; x < 7; x++) {
		sprintf(tedii_out_buf, "%s%s%d%s---------", tedii_out_buf, CCCYN(ch,
				C_NRM), x, CCBLU(ch, C_NRM));
	}
	sprintf(tedii_out_buf, "%s%s7%s", tedii_out_buf, CCCYN(ch, C_NRM),
		CCNRM(ch, C_NRM));
	SendMessage(tedii_out_buf);
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
CTextEditor::UpdateSize(void)
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

	// Current line number for prompt.
	desc->editor_cur_lnum = theText.size() + 1;
	// Warn the player if the buffer was truncated.
	if (linesRemoved > 0) {
		sprintf(tedii_out_buf,
			"Error: Buffer limit exceeded.  %d %s removed.\r\n",
			linesRemoved, linesRemoved == 1 ? "line" : "lines");
		SendMessage(tedii_out_buf);
		sprintf(tedii_out_buf,
			"TEDINF: UpdateSize removed %d lines from buffer. Name(%s) Size(%d) Max(%d)",
			linesRemoved, GET_NAME(desc->creature), curSize, maxSize);
		slog(tedii_out_buf);
	}
	// Obvious buffer flow state. This should never happen, but if it does, say something.
	if (curSize > maxSize) {
		sprintf(tedii_out_buf,
			"TEDERR: UpdateSize updated to > maxSize. Name(%s) Size(%d) Max(%d)",
			GET_NAME(desc->creature), curSize, maxSize);
		slog(tedii_out_buf);
	}
}
void
CTextEditor::ProcessHelp(char *inStr)
{
	struct Creature *ch = desc->creature;
	char command[MAX_INPUT_LENGTH];
	if (!*inStr) {
		sprintf(tedii_out_buf, "%s%s     *", CCBLD(ch, C_CMP), CCCYN(ch,
				C_NRM));
		sprintf(tedii_out_buf, "%s%s-----------------------", tedii_out_buf,
			CCBLU(ch, C_NRM));
		sprintf(tedii_out_buf, "%s%s H E L P ", tedii_out_buf, CCYEL(ch,
				C_NRM));
		sprintf(tedii_out_buf, "%s%s-----------------------", tedii_out_buf,
			CCBLU(ch, C_NRM));
		sprintf(tedii_out_buf, "%s%s* \r\n%s", tedii_out_buf, CCCYN(ch, C_NRM),
			CCNRM(ch, C_NRM));
		sprintf(tedii_out_buf, "%s%s%s            ", tedii_out_buf, CCBLD(ch,
				C_CMP), CCYEL(ch, C_NRM));
		sprintf(tedii_out_buf, "%sF%s - %sFind & Replace   ", tedii_out_buf,
			CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
		sprintf(tedii_out_buf, "%s    %s%s", tedii_out_buf, CCBLD(ch, C_CMP),
			CCYEL(ch, C_NRM));
		sprintf(tedii_out_buf, "%sH%s - %sHelp         \r\n", tedii_out_buf,
			CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
		sprintf(tedii_out_buf, "%s%s%s            ", tedii_out_buf, CCBLD(ch,
				C_CMP), CCYEL(ch, C_NRM));
		sprintf(tedii_out_buf, "%sS%s - %sSave and Exit    ", tedii_out_buf,
			CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
		sprintf(tedii_out_buf, "%s    %s%s", tedii_out_buf, CCBLD(ch, C_CMP),
			CCYEL(ch, C_NRM));
		sprintf(tedii_out_buf, "%sQ%s - %sQuit (Cancel)\r\n", tedii_out_buf,
			CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
		sprintf(tedii_out_buf, "%s%s%s            ", tedii_out_buf, CCBLD(ch,
				C_CMP), CCYEL(ch, C_NRM));
		sprintf(tedii_out_buf, "%sL%s - %sReplace Line     ", tedii_out_buf,
			CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
		sprintf(tedii_out_buf, "%s    %s%s", tedii_out_buf, CCBLD(ch, C_CMP),
			CCYEL(ch, C_NRM));
		sprintf(tedii_out_buf, "%sD%s - %sDelete Line  \r\n", tedii_out_buf,
			CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
		sprintf(tedii_out_buf, "%s%s%s            ", tedii_out_buf, CCBLD(ch,
				C_CMP), CCYEL(ch, C_NRM));
		sprintf(tedii_out_buf, "%sI%s - %sInsert Line      ", tedii_out_buf,
			CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
		sprintf(tedii_out_buf, "%s    %s%s", tedii_out_buf, CCBLD(ch, C_CMP),
			CCYEL(ch, C_NRM));
		sprintf(tedii_out_buf, "%sR%s - %sRefresh Screen\r\n", tedii_out_buf,
			CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
		sprintf(tedii_out_buf, "%s%s%s            ", tedii_out_buf, CCBLD(ch,
				C_CMP), CCYEL(ch, C_NRM));
		sprintf(tedii_out_buf, "%sC%s - %sClear Buffer     ", tedii_out_buf,
			CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
		if (!PLR_FLAGGED(ch, PLR_MAILING)) {	// Can't undo if yer mailin.
			sprintf(tedii_out_buf, "%s    %s%s", tedii_out_buf, CCBLD(ch,
					C_CMP), CCYEL(ch, C_NRM));
			sprintf(tedii_out_buf, "%sU%s - %sUndo Changes  \r\n",
				tedii_out_buf, CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
		} else {
			sprintf(tedii_out_buf, "%s    %s%s", tedii_out_buf, CCBLD(ch,
					C_CMP), CCYEL(ch, C_NRM));
			sprintf(tedii_out_buf, "%sA%s - %sAdd Recipient \r\n",
				tedii_out_buf, CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
			sprintf(tedii_out_buf, "%s%s%s            ", tedii_out_buf,
				CCBLD(ch, C_CMP), CCYEL(ch, C_NRM));
			sprintf(tedii_out_buf, "%sT%s - %sList Recipients", tedii_out_buf,
				CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
			sprintf(tedii_out_buf, "%s      %s%s", tedii_out_buf, CCBLD(ch,
					C_CMP), CCYEL(ch, C_NRM));
			sprintf(tedii_out_buf, "%sE%s - %sRemove Recipient\r\n",
				tedii_out_buf, CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
		}
		sprintf(tedii_out_buf, "%s%s%s     *", tedii_out_buf, CCBLD(ch, C_CMP),
			CCCYN(ch, C_NRM));


		sprintf(tedii_out_buf,
			"%s%s-------------------------------------------------------",
			tedii_out_buf, CCBLU(ch, C_NRM));
		sprintf(tedii_out_buf, "%s%s*%s\r\n", tedii_out_buf, CCCYN(ch, C_NRM),
			CCNRM(ch, C_NRM));
		SendMessage(tedii_out_buf);
	} else {
		inStr++;
		inStr = one_argument(inStr, command);
		*command = tolower(*command);
		switch (*command) {
		case 'f':
			sprintf(tedii_out_buf, "%sFind & Replace: '&f' \r\n",
				tedii_out_buf);
			sprintf(tedii_out_buf, "%s&f [red] [yellow]\r\n", tedii_out_buf);
			sprintf(tedii_out_buf,
				"%sReplaces all occurances of \"red\" with \"yellow\".\r\n",
				tedii_out_buf);
			sprintf(tedii_out_buf,
				"%se.g. 'That is a red dog.' would become 'That is a yellow dog.'\r\n",
				tedii_out_buf);
			sprintf(tedii_out_buf,
				"%sAlso, 'Fred is here.' would become 'Fyellow is here.'\r\n",
				tedii_out_buf);
			sprintf(tedii_out_buf,
				"%sWhen replacing words, remember to include the spaces     to either side\r\n",
				tedii_out_buf);
			sprintf(tedii_out_buf,
				"%sin the replacement to keep from replacing partial words my mistake.\r\n",
				tedii_out_buf);
			sprintf(tedii_out_buf,
				"%s(i.e. use '[ red ] [ yellow ]' instead of '[red] [yellow]'.\r\n",
				tedii_out_buf);
			break;
		case 'r':
			sprintf(tedii_out_buf, "%sRefresh Screen: &r\r\n", tedii_out_buf);
			sprintf(tedii_out_buf,
				"%sPrints out the entire text tedii_out_buffer.\r\n",
				tedii_out_buf);
			sprintf(tedii_out_buf, "%s(The message/post/description)\r\n",
				tedii_out_buf);
			sprintf(tedii_out_buf,
				"%sA single period '.' without an '&' can be used as well.\r\n",
				tedii_out_buf);
			break;
		case 'l':
			sprintf(tedii_out_buf, "%sReplace Line: &l\r\n", tedii_out_buf);
			sprintf(tedii_out_buf, "%s&l <line #> <replacement text>\r\n",
				tedii_out_buf);
			sprintf(tedii_out_buf,
				"%sReplaces line <line #> with <replacement text>.",
				tedii_out_buf);
			sprintf(tedii_out_buf,
				"%sSpaces are saved in the replacement text to save indentation.\r\n",
				tedii_out_buf);
			break;
		case 'd':
			sprintf(tedii_out_buf, "%sDelete Line: &d\r\n", tedii_out_buf);
			sprintf(tedii_out_buf, "%s&d <line #>\r\n", tedii_out_buf);
			sprintf(tedii_out_buf, "%sDeletes line <line #>\r\n",
				tedii_out_buf);
			break;
		case 'i':
			sprintf(tedii_out_buf, "%sInsert Line: &i\r\n", tedii_out_buf);
			sprintf(tedii_out_buf, "%s&i <line #> <insert text>\r\n",
				tedii_out_buf);
			sprintf(tedii_out_buf,
				"%sInserts <insert text> before line <line #>.\r\n",
				tedii_out_buf);
			sprintf(tedii_out_buf,
				"%sSpaces are saved in the replacement text to save indentation\r\n",
				tedii_out_buf);
			sprintf(tedii_out_buf, "%sA note to TinTin users:\r\n",
				tedii_out_buf);
			sprintf(tedii_out_buf,
				"%s    Tintin removes all spaces before a command.\r\n",
				tedii_out_buf);
			sprintf(tedii_out_buf,
				"%s    Use '&i <current line #> <spaces> <text>' to indent a line.\r\n",
				tedii_out_buf);
			break;
		case 'u':
			sprintf(tedii_out_buf, "%sUndo Changes: &u\r\n", tedii_out_buf);
			sprintf(tedii_out_buf, "%s&u yes\r\n", tedii_out_buf);
			sprintf(tedii_out_buf, "%sUndoes all changes since last save.\r\n",
				tedii_out_buf);
			sprintf(tedii_out_buf,
				"%s(Changes do not save until you exit the editor)\r\n",
				tedii_out_buf);
			break;
		case 'c':
			sprintf(tedii_out_buf, "%sClear Buffer: &c\r\n", tedii_out_buf);
			sprintf(tedii_out_buf,
				"%sDeletes ALL text in the current tedii_out_buffer.\r\n",
				tedii_out_buf);
			break;
		default:
			sprintf(tedii_out_buf, "Sorry. There is no help on that.\r\n");
		}
		SendMessage(tedii_out_buf);
	}
}
bool
CTextEditor::ProcessCommand(char *inStr)
{
	int line;
	char command[MAX_INPUT_LENGTH];
	inStr++;
	if (!*inStr) {
		SendMessage("Invalid Command\r\n");
	}
	inStr = one_argument(inStr, command);
	*command = tolower(*command);
	switch (*command) {
	case 'h':					// Help
		ProcessHelp(inStr);
		return true;
		break;
	case 'f':					// Find/Replace
		return FindReplace(inStr);
		break;
	case 's':					// Save and Exit
		REMOVE_BIT(PRF2_FLAGS(desc->creature), PRF2_NOWRAP);
		SaveText(inStr);
		desc->text_editor = NULL;
		delete this;
		return true;
		break;
	case 'l':					// Replace Line
		inStr = one_argument(inStr, command);
		if (!isdigit(*command)) {
			SendMessage("Format for Replace Line is: &l <line #> <text>\r\n");
			return false;
		}
		line = atoi(command);
		if (line < 1) {
			SendMessage("Format for Replace Line is: &l <line #> <text>\r\n");
			return false;
		}
		return ReplaceLine((unsigned int)line, inStr);
		break;
	case 'i':					// Insert Line
		inStr = one_argument(inStr, command);
		if (!isdigit(*command)) {
			SendMessage
				("Format for insert command is: &i <line #> <text>\r\n");
			return false;
		}
		line = atoi(command);
		if (line < 1) {
			SendMessage("Format for insert command is: &i <line #><text>\r\n");
			return false;
		}
		return Insert((unsigned int)line, inStr);
		break;
	case 'c':					// Clear Buffer
		Clear();
		SendMessage("Cleared.\r\n");
		return true;
		break;
	case 'q':					// Quit without saving
		SendMessage("Not Implemented.\r\n");
		return true;
		break;
	case 'd':					// Delete Line
		inStr = one_argument(inStr, command);
		if (!isdigit(*command)) {
			SendMessage("Format for delete command is: &d <line #>\r\n");
			return false;
		}
		line = atoi(command);
		if (line < 1) {
			SendMessage("Format for delete command is: &d <line #>\r\n");
			return false;
		}
		return Remove((unsigned int)line);
		break;
	case 'r':					// Refresh Screen
		inStr = one_argument(inStr, command);
		if (!isdigit(*command)) {
			List();
			return true;
		}
		line = atoi(command);
		if (line < 1) {
			SendMessage
				("Format for refresh command is: &r <starting line #>\r\n");
			return false;
		}
		List((unsigned int)line);
		break;
	case 'u':					// Undo Changes
		UndoChanges(inStr);
		return true;
	case 't':
		/*if(GET_LEVEL(desc->creature) < 72) {
		   SendMessage ("This command removed until further notice.\r\n");
		   return false;
		   } */
		if (PLR_FLAGGED(desc->creature, PLR_MAILING)) {
			ListRecipients();
			return true;
		}
	case 'a':
		/*if(GET_LEVEL(desc->creature) < 72) {
		   SendMessage ("This command removed until further notice.\r\n");
		   return false;
		   } */
		if (PLR_FLAGGED(desc->creature, PLR_MAILING)) {
			inStr = one_argument(inStr, command);
			AddRecipient(command);
			return true;
		}
	case 'e':
		/*if(GET_LEVEL(desc->creature) < 72) {
		   SendMessage ("This command removed until further notice.\r\n");
		   return false;
		   } */
		if (PLR_FLAGGED(desc->creature, PLR_MAILING)) {
			inStr = one_argument(inStr, command);
			RemRecipient(command);
			return true;
		}
	default:
		SendMessage("Invalid Command. Type &h for help.\r\n");
		return false;
	}

	return false;
}

void
CTextEditor::ListRecipients(void)
{
	char *cc_list = NULL;
	struct mail_recipient_data *mail_rcpt = NULL;
	int cc_len = 0;

	for (mail_rcpt = desc->mail_to; mail_rcpt; mail_rcpt = mail_rcpt->next)
		cc_len++;

	cc_list = (char*) malloc( sizeof(char) * ((cc_len * MAX_NAME_LENGTH) + 32) );
	//cc_list = new char[(cc_len * MAX_NAME_LENGTH) + 32];

	sprintf(cc_list, "%sTo%s:%s ",
		CCYEL(desc->creature, C_NRM),
		CCBLU(desc->creature, C_NRM), CCCYN(desc->creature, C_NRM));
	for (mail_rcpt = desc->mail_to; mail_rcpt;) {
		strcat(cc_list, tmp_capitalize(playerIndex.getName(mail_rcpt->recpt_idnum)));
		if (mail_rcpt->next) {
			strcat(cc_list, ", ");
			mail_rcpt = mail_rcpt->next;
		} else {
			strcat(cc_list, "\r\n");
			break;
		}
	}

	sprintf(cc_list, "%s%s", cc_list, CCNRM(desc->creature, C_NRM));
	SendMessage(cc_list);

	if (cc_list) {
		free(cc_list);
	}

}

void
CTextEditor::AddRecipient(char *name)
{
	long new_id_num = 0;
	struct mail_recipient_data *cur = NULL;
	struct mail_recipient_data *new_rcpt = NULL;
	const char *money_desc;
	int money, cost;

	new_id_num = playerIndex.getID(name);
	if (!new_id_num) {
		SendMessage("Cannot find anyone by that name.\r\n");
		return;
	}

	new_rcpt = (mail_recipient_data *)malloc(sizeof(mail_recipient_data));
	new_rcpt->recpt_idnum = new_id_num;
	new_rcpt->next = NULL;

	// Now find the end of the current list and add the new cur
	cur = desc->mail_to;
	while (cur && cur->recpt_idnum != new_id_num && cur->next)
		cur = cur->next;

	if (cur->recpt_idnum == new_id_num) {
		sprintf(tedii_out_buf,
			"%s is already on the recipient list.\r\n",
			tmp_capitalize(playerIndex.getName(new_id_num)));
		SendMessage(tedii_out_buf);
		free(new_rcpt);
		return;
	}

	if (GET_LEVEL(desc->creature) < LVL_AMBASSADOR) {
		//mailing the grimp, charge em out the ass
		if (desc->creature->in_room->zone->time_frame == TIME_ELECTRO) {
			money_desc = "creds";
			money = GET_CASH(desc->creature);
		} else {
			money_desc = "gold";
			money = GET_GOLD(desc->creature);
		}

		if (new_id_num == 1)
			cost = 1000000;
		else
			cost = STAMP_PRICE;

		if (money < cost) {
			sprintf(tedii_out_buf,
				"You don't have the %d %s necessary to add %s.\r\n",
				cost, money_desc, 
				tmp_capitalize(playerIndex.getName(new_id_num)));
			free(new_rcpt);
			SendMessage(tedii_out_buf);
			return;
		} else {
			sprintf(tedii_out_buf,
				"%s added to recipient list.  You have been charged %d %s.\r\n",
				tmp_capitalize(playerIndex.getName(new_id_num)), cost,
				money_desc);
			if (desc->creature->in_room->zone->time_frame == TIME_ELECTRO)
				GET_CASH(desc->creature) -= cost;
			else
				GET_GOLD(desc->creature) -= cost;
		}
	} else {
		sprintf(tedii_out_buf, "%s added to recipient list.\r\n",
			tmp_capitalize(playerIndex.getName(new_id_num)));
	}
	cur->next = new_rcpt;
	SendMessage(tedii_out_buf);
	ListRecipients();
}

void
CTextEditor::RemRecipient(char *name)
{
	int removed_idnum = -1;
	struct mail_recipient_data *cur = NULL;
	struct mail_recipient_data *prev = NULL;
	char buf[MAX_INPUT_LENGTH];

	removed_idnum = playerIndex.getID(name);

	if (removed_idnum) {
		SendMessage("Cannot find anyone by that name.\r\n");
		return;
	}
	// First case...the mail only has one recipient
	if (!desc->mail_to->next) {
		SendMessage("You cannot remove the last recipient of the letter.\r\n");
		return;
	}
	// Second case... Its the first one.
	if (desc->mail_to->recpt_idnum == removed_idnum) {
		cur = desc->mail_to;
		desc->mail_to = desc->mail_to->next;
		free(cur);
		if (desc->creature->in_room->zone->time_frame == TIME_ELECTRO) {
			if (GET_LEVEL(desc->creature) < LVL_AMBASSADOR) {

				if (removed_idnum == 1) {	//fireball :P
					sprintf(buf,
						"%s removed from recipient list.  %d credits have been refunded.\r\n",
						tmp_capitalize(playerIndex.getName(removed_idnum)), 1000000);
					GET_CASH(desc->creature) += 1000000;	//credit mailer for removed recipient 
				} else {
					sprintf(buf,
						"%s removed from recipient list.  %d credits have been refunded.\r\n",
						tmp_capitalize(playerIndex.getName(removed_idnum)), STAMP_PRICE);
					GET_CASH(desc->creature) += STAMP_PRICE;	//credit mailer for removed recipient 
				}
			} else {
				sprintf(buf, "%s removed from recipient list.\r\n",
					tmp_capitalize(playerIndex.getName(removed_idnum)));
			}
		} else {				//not in the future, refund gold
			if (GET_LEVEL(desc->creature) < LVL_AMBASSADOR) {
				if (removed_idnum == 1) {	//fireball :P
					sprintf(buf,
						"%s removed from recipient list.  %d gold has been refunded.\r\n",
						tmp_capitalize(playerIndex.getName(removed_idnum)), 1000000);
					GET_GOLD(desc->creature) += 1000000;	//credit mailer for removed recipient 
				} else {
					sprintf(buf,
						"%s removed from recipient list.  %d gold has been refunded.\r\n",
						tmp_capitalize(playerIndex.getName(removed_idnum)), STAMP_PRICE);
					GET_GOLD(desc->creature) += STAMP_PRICE;	//credit mailer for removed recipient 
				}
			} else {
				sprintf(buf, "%s removed from recipient list.\r\n",
					tmp_capitalize(playerIndex.getName(removed_idnum)));
			}
		}
		SendMessage(buf);
		return;
	}
	// Last case... Somewhere past the first recipient.
	cur = desc->mail_to;
	// Find the recipient in question
	while (cur && cur->recpt_idnum != removed_idnum) {
		prev = cur;
		cur = cur->next;
	}
	// If the name isn't in the recipient list
	if (!cur) {
		SendMessage
			("You can't remove someone who's not on the list genious.\r\n");
		return;
	}
	// Link around the recipient to be removed.
	prev->next = cur->next;
	free(cur);
	if (desc->creature->in_room->zone->time_frame == TIME_ELECTRO) {
		if (GET_LEVEL(desc->creature) < LVL_AMBASSADOR) {
			if (removed_idnum == 1) {	//fireball :P
				sprintf(buf,
					"%s removed from recipient list.  %d credits have been refunded.\r\n",
					tmp_capitalize(playerIndex.getName(removed_idnum)), 1000000);
				GET_CASH(desc->creature) += 1000000;	//credit mailer for removed recipient 
			} else {
				sprintf(buf,
					"%s removed from recipient list.  %d credits have been refunded.\r\n",
					tmp_capitalize(playerIndex.getName(removed_idnum)), STAMP_PRICE);
				GET_CASH(desc->creature) += STAMP_PRICE;	//credit mailer for removed recipient 
			}
		} else {
			sprintf(buf, "%s removed from recipient list.\r\n",
				tmp_capitalize(playerIndex.getName(removed_idnum)));
		}
	} else {					//not in future, refund gold
		if (GET_LEVEL(desc->creature) < LVL_AMBASSADOR) {
			if (removed_idnum == 1) {	//fireball :P
				sprintf(buf,
					"%s removed from recipient list.  %d gold has been refunded.\r\n",
					tmp_capitalize(playerIndex.getName(removed_idnum)), 1000000);
				GET_GOLD(desc->creature) += 1000000;	//credit mailer for removed recipient 
			} else {
				sprintf(buf,
					"%s removed from recipient list.  %d gold has been refunded.\r\n",
					tmp_capitalize(playerIndex.getName(removed_idnum)), STAMP_PRICE);
				GET_GOLD(desc->creature) += STAMP_PRICE;	//credit mailer for removed recipient 
			}
		} else {
			sprintf(buf, "%s removed from recipient list.\r\n",
				tmp_capitalize(playerIndex.getName(removed_idnum)));
		}
	}
	SendMessage(buf);

	return;
}
