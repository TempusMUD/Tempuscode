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


static char editbuf[MAX_STRING_LENGTH * 2];
extern struct descriptor_data *descriptor_list;

// Muha. Call it Tine

/* Sets up text editor params and echo's passed in message.
*/
void 
start_text_editor(struct descriptor_data *d, char **dest, bool sendmessage=true, int max=MAX_STRING_LENGTH) {
    /*  Editor Command
        Note: Add info for logall
    */
    // There MUST be a destination!
    if(!dest) {
        mudlog("ERROR: NULL destination pointer passed into start_text_editor!!",
                BRF, LVL_IMMORT, TRUE);
        send_to_char("This command seems to be broken. Bug this.\r\n",d->character);
        REMOVE_BIT(PLR_FLAGS(d->character), PLR_WRITING | PLR_OLC | PLR_MAILING);
        return;
    }
    if(d->text_editor) {
        mudlog("ERROR: Text editor object not null in start_text_editor.",
                BRF,LVL_IMMORT,TRUE);
        REMOVE_BIT(PLR_FLAGS(d->character), PLR_WRITING | PLR_OLC | PLR_MAILING);
        return;
    }
    if(*dest && (strlen(*dest) > (unsigned int)max)) {
        send_to_char("ERROR: Buffer too large for editor.\r\n",d->character);
        REMOVE_BIT(PLR_FLAGS(d->character), PLR_WRITING | PLR_OLC | PLR_MAILING);
        return;
    }
        
    d->text_editor = new CTextEditor(d, dest, max, sendmessage);
}

void CTextEditor::Process( char *inStr ) {
    // 2 special chars, @ and &

    delete_doubledollar(inStr);

    if(*inStr == '&') {// Commands
        ProcessCommand(inStr);
        return;
    } else if (*inStr == '@') {// Finish up
        SaveText(inStr);
        desc->text_editor = NULL;
        delete this;
        return;
    } else { // Dump the text in
        Append(inStr);
    }
    desc->editor_cur_lnum = theText.size() + 1;
}
void CTextEditor::List( unsigned int startline=1 ) {
    list<string>::iterator itr;
    int i;
    int num_lines;
    strcpy(editbuf,"\r\n");
    
    itr = theText.begin();
    
    // Allow a param, cut off when the buffer hits LARGE_BUF_SIZE
    if(startline > 1) {
        for(i = startline ;i > 1 && itr != theText.end();i--)
            itr++;
    }
    
    for(i = startline;itr != theText.end();i++, itr++) {
        sprintf(buf, "%-2d%s%s]%s ",i ,
            CCBLD(desc->character,C_CMP),
            CCBLU(desc->character,C_NRM),
            CCNRM(desc->character,C_NRM));
        strcat(editbuf,buf);
        strcat(editbuf,itr->c_str());
        strcat(editbuf,"\r\n");
    }
    SendMessage(editbuf);
}

void CTextEditor::SaveText( char *inStr) {
    list<string>::iterator itr;
    int length = 0;

    // If we were editing rather than creating, wax what was there.
    if(*target) {
        free(*target);
        *target = NULL;
    }
    
    length = curSize + ( theText.size() * 2 );
    *target = new char[length + 3];
    strcpy(*target,"");

    for(itr = theText.begin();itr != theText.end();itr++) {
        strcat(*target,itr->c_str());
        strcat(*target,"\r\n");
    }
    // If they're in the game
    if(desc->connected == CON_PLAYING) {
        // Saving a file
        if ((desc->editor_file != NULL)) {
            SaveFile();
        }
        // Sending Mail
        else if (PLR_FLAGGED(desc->character, PLR_MAILING)) {
            ExportMail();
            free(target);
        }
    } 
    // WTF is this?
    if (desc->mail_to && desc->mail_to->recpt_idnum >= BOARD_MAGIC) {
        Board_save_board(desc->mail_to->recpt_idnum - BOARD_MAGIC);
        desc->mail_to = desc->mail_to->next;
    }
    // If editing thier description.
    if (desc->connected == CON_EXDESC) {
        SEND_TO_Q("\033[H\033[J", desc);
        show_menu(desc);
        desc->connected = CON_MENU;
    }
    // Remove the "using the editor" bits.
    if (desc->connected == CON_PLAYING && 
        desc->character && 
        !IS_NPC(desc->character)) {
        REMOVE_BIT(PLR_FLAGS(desc->character), PLR_WRITING | PLR_OLC | PLR_MAILING);
    }
}

void CTextEditor::ExportMail( void ) {

    char   *cc_list = NULL;
    int    stored_mail=0;
    struct descriptor_data *r_d;
    struct mail_recipient_data *mail_rcpt = NULL;

    if(desc->mail_to->next) {
        cc_list = new char[MAX_INPUT_LENGTH * 3 + 7];
        strcpy(cc_list,"  CC: ");
        for(mail_rcpt = desc->mail_to; mail_rcpt; mail_rcpt = mail_rcpt->next){
            strcat(cc_list, get_name_by_id(mail_rcpt->recpt_idnum));
            if (mail_rcpt->next)
                strcat(cc_list, ", ");
            else
                strcat(cc_list, "\r\n");
        }
    }
    mail_rcpt = desc->mail_to;
    while (mail_rcpt) {
        if((stored_mail = store_mail(mail_rcpt->recpt_idnum,GET_IDNUM(desc->character),*target, cc_list))) {
            for (r_d = descriptor_list; r_d; r_d = r_d->next) {
                if (!r_d->connected && r_d->character && r_d->character != desc->character &&
                GET_IDNUM(r_d->character) == desc->mail_to->recpt_idnum &&
                !PLR_FLAGGED(r_d->character,PLR_WRITING|PLR_MAILING|PLR_OLC)) {
                    send_to_char("A strange voice in your head says, 'You have new mail.'\r\n", r_d->character);
                }
            }
        }
        mail_rcpt = mail_rcpt->next;
        free(desc->mail_to);
        desc->mail_to = mail_rcpt;
    }
    if(cc_list)
        delete cc_list;
    if(stored_mail)
        SendMessage("Message sent!\r\n");
}

void CTextEditor::SaveFile( void ) {

    char   filebuf[512], filename[64];
    int    file_to_write;
    int    backup_file,nread;

    sprintf(filename,"%s", desc->editor_file);
    if ((file_to_write = open(filename,O_RDWR, 0666)) == -1) {
        sprintf(buf, "Could not open file %s, buffer not saved!\r\n", filename);
        mudlog(buf, NRM, LVL_AMBASSADOR, TRUE);
    } else {
        sprintf(filename,"%s.bak",desc->editor_file);
        if ((backup_file = open(filename, O_RDWR|O_CREAT|O_TRUNC, 0666)) == -1) {
            sprintf(buf, "Could not open file %s, buffer not saved!\r\n", filename);
            mudlog(buf, NRM, LVL_AMBASSADOR, TRUE);
            close(file_to_write);
        } else {
            while ((nread = read(file_to_write, filebuf, sizeof(filebuf))) > 0) {
                if (write(backup_file, filebuf, nread) != nread) {
                    send_to_char("Could not save backup file!!\r\n", desc->character);
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

bool CTextEditor::Full ( char *inStr=NULL) {
    if( ( strlen(inStr) + curSize) + 
        ( (theText.size() + 1) * 2 ) > maxSize) {
        return true;
    }
    return false;
}

void CTextEditor::Append(char *inStr) {
    if(Full(inStr)) {
        SendMessage("Error: The buffer is full.\r\n");
        return;
    }
    theText.push_back(inStr);
    Wrap();
    UpdateSize();
}
bool CTextEditor::Insert(unsigned int line, char *inStr) {
    string text;
    list<string>::iterator s;
    unsigned int i = 1;

    if(*inStr) {
        inStr++;
    }
    text = inStr;
    if(line > theText.size()) {
        SendMessage("You can't insert before a line that doesn't exist.\r\n");
        return false;
    }
    if((text.length() + curSize) +
         ((theText.size() + 1) * 2 ) > maxSize) {
        SendMessage("Error: The buffer is full.\r\n");
        return false;
    }

    // Find it again (the one after it actually)
    for(s = theText.begin();i < line;i++,s++);

    // Insert the new text
    theText.insert(s,text);
    Wrap();
    UpdateSize();
    
    return true;
}

bool CTextEditor::ReplaceLine(unsigned int line, char *inStr) {
    string text;
    list<string>::iterator s;
    unsigned int i = 1;

    if(*inStr && *inStr == ' ')
        inStr++;
    text = inStr;

    if(line > theText.size()) {
        SendMessage("Your going to have to write the line first dummy.\r\n");
        return false;
    }
    // Find the line
    for(i = 1,s = theText.begin();i < line;i++,s++);

    // Make sure we can fit the new stuff in
    if((text.length() + curSize - s->length()) +
        ( (theText.size() + 1) * 2 ) > maxSize) {
        SendMessage("Error: The buffer is full.\r\n");
        return false;
    }
    // Erase it
    theText.erase(s);

    // Find it again (the one after it actually)
    for(i = 1,s = theText.begin();i < line;i++,s++);
    // Insert the new text
    theText.insert(s,text);

    Wrap();
    UpdateSize();
    return true;
}
bool CTextEditor::FindReplace(char *args) {
    // MAKE SURE YOU CHECK maxSize before inserting!
	list<string>::iterator line;
	string findit;
	string replaceit;
    int replaced = 0;
	char temp[MAX_INPUT_LENGTH];
	char *r,*w;

	for(r = args;*r == ' ';r++);

    if(!*r) {
        SendMessage("The format for find/replace is &f [search string] [replace string] \r\nYou must actually include the brackets\r\n");
        return false;
    }
// Find "findit"

	if(*r != '[') {
        SendMessage("Mismatched brackets.\r\n");
        return false;
    }
	// Advance past [
    r++;
    if(*r == ']') {
        SendMessage("You can't search for nothing...\r\n");
        return false;
    }

	for(w = temp;*r && *r != ']';r++)
		*w++ = *r;
	// terminate w;
	*w = '\0';

	if(*r != ']') {
        SendMessage("Mismatched brackets.\r\n");
        return false;
    }
	// Advance past ]
	r++;
	findit = temp;

// Find "replaceit"
	for(;*r == ' ';r++);
	if(*r != '[') {
        SendMessage("Mismatched brackets.\r\n");
        return false;
    }
	// Advance past [
	r++;
	for(w = temp;*r && *r != ']';r++)
		*w++ = *r;
	// terminate w;
	*w = '\0';

	if(*r != ']') {
        SendMessage("Mismatched brackets.\r\n");
        return false;
    }
	replaceit = temp;
	
	// Find "findit" in theText a line at a time and replace each instance.
    unsigned int pos;
    bool overflow=false;
	for(line = theText.begin();!overflow && line != theText.end();line++) {
        pos=0;
		while(pos < line->length()) {
            pos = line->find(findit,pos);
            if(pos < line->length()) {
                if((curSize - findit.length() + replaceit.length()) +
                  ((theText.size() + 1) * 2 ) > maxSize) {
                }
                *line = line->replace(pos,findit.length(),replaceit);
                replaced++;
                pos += replaceit.length();
            }
            if(replaced >= 100) {
                SendMessage("Replacement limit of 100 reached.\r\n");
                overflow = true;
                break;
            }
		}
	}
	if(replaced > 0) {
        sprintf(buf,"%d occurances of [%s] replaced with [%s].\r\n",
            replaced,findit.c_str(),replaceit.c_str());
        SendMessage(buf);
    } else {
        SendMessage("Search string not found.\r\n");
    }
    Wrap();
    UpdateSize();
    return true;
}
bool CTextEditor::Wrap( void ) {
    list<string>::iterator line;
    string::iterator s;
    string tempstr;
    int linebreak;

    for(line = theText.begin();line != theText.end();line++) {
        linebreak = 76;
        tempstr = "";
        // If its less than 80 chars, it dont need ta be wrapped.
        if(line->length() <= 76)
            continue;

        s = line->begin();

        // Find the first space <= 80th char.
        for(s += 75;s != line->begin() && *s != ' ';s--)
            linebreak--;

        if(linebreak == 1) { // Linebreak is at 80
            s = line->begin();
            s += 75;
        }
        tempstr.append(s,line->end());
        if(linebreak > 1)  // If its a pos other than 1, its a space.
            tempstr.erase(tempstr.begin());
        line->erase(s,line->end());
        line++;
        theText.insert(line,1,tempstr);
        line = theText.begin();
    }
    return true;
}
bool CTextEditor::Remove(unsigned int line) {
    list<string>::iterator s;
    unsigned int i;
    if(line > theText.size()) {
        SendMessage("Someone already deleted that line boss.\r\n");
        return false;
    }
    for(i = 1,s = theText.begin();i < line ;s++,i++);

    theText.erase(s);
    sprintf(buf,"Line %d deleted.\r\n",line);
    SendMessage(buf);

    Wrap();
    UpdateSize();
    return true;
}
bool CTextEditor::Clear( void ) {

    theText.erase(theText.begin(),theText.end());

    desc->editor_cur_lnum = theText.size() + 1;
    UpdateSize();
    return true;
}
void CTextEditor::ImportText( void ) {
    char *b,*s;// s is the cursor, b is the beginning of the current line
    string text;

    strcpy(editbuf, *target);
    s = editbuf;
    while (s && *s) {
        for(b = s; *s && *s != '\r';s++);

        *s = '\0';
        s += 2;
        text = b;
        theText.push_back(text);
    }
    Wrap();
}

void CTextEditor::UndoChanges( char *inStr ) {
    if(!*target) {
        SendMessage("There's no original to undo to.\r\n");
        return;
    }
    Clear();
    ImportText();
    UpdateSize();
    desc->editor_cur_lnum = theText.size() + 1;
    SendMessage("Original buffer restored.\r\n");
    return;
}

// Constructor
// Params: Users descriptor, The final destination of the text, 
//      the max size of the text.
CTextEditor::CTextEditor(struct descriptor_data *d, 
                         char **dest, 
                         int max, 
                         bool startup) :theText() {
    // Internal pointer to the descriptor
    desc = d;
    // Internal pointer to the destination
    target = dest;

    // The maximum size of the buffer.
    maxSize = max;

    if(*target) {
        ImportText();
    }
    desc->editor_cur_lnum = theText.size() + 1;
    UpdateSize();
    if(startup) {
        SendStartupMessage();
        List();
    }
}

void CTextEditor::SendMessage(const char *message) {
    send_to_char(message,desc->character);
}

void CTextEditor::SendStartupMessage( void ) {
    struct char_data *ch;
    ch = desc->character;

    sprintf(buf,"%s%s    *",CCBLD(ch,C_CMP),CCCYN(ch,C_NRM));
    sprintf(buf,"%s%s TEDII ",buf,CCYEL(ch,C_NRM));
    sprintf(buf,"%s%s] ",buf,CCBLU(ch,C_NRM));
    sprintf(buf,"%s%sTerminate with @ on a new line. &H for help",buf,CCNRM(ch,C_NRM));
    sprintf(buf,"%s%s%s                 *\r\n",buf,CCBLD(ch,C_CMP),CCCYN(ch,C_NRM));
    sprintf(buf,"%s    %s",buf,CCBLD(ch,C_CMP));
    for (int x=0;x < 7;x++) {
        sprintf(buf,"%s%s%d%s---------",buf,CCCYN(ch,C_NRM),x,CCBLU(ch,C_NRM));
    }
    sprintf(buf,"%s%s7%s\r\n",buf,CCCYN(ch,C_NRM),CCNRM(ch,C_NRM));
    SEND_TO_Q(buf,desc);
}

void CTextEditor::UpdateSize( void ) {
    list<string>::iterator s;
    
    curSize = 0;

    for(s = theText.begin();s != theText.end();s++) {
        curSize += s->length();
    }
    desc->editor_cur_lnum = theText.size() + 1;
}
void CTextEditor::ProcessHelp(char *inStr) {
    struct char_data *ch = desc->character;
    char command[MAX_INPUT_LENGTH];
    if(!*inStr) {
        sprintf(buf,"%s%s     *",CCBLD(ch,C_CMP),CCCYN(ch,C_NRM));
        sprintf(buf,"%s%s-----------------------",buf,CCBLU(ch,C_NRM));
        sprintf(buf,"%s%s H E L P ",buf,CCYEL(ch,C_NRM));
        sprintf(buf,"%s%s-----------------------",buf,CCBLU(ch,C_NRM));
        sprintf(buf,"%s%s* \r\n%s",buf,CCCYN(ch,C_NRM),CCNRM(ch,C_NRM));
        sprintf(buf,"%s%s%s            ",buf,CCBLD(ch,C_CMP),CCYEL(ch,C_NRM));
        sprintf(buf,"%sF%s - %sFind & Replace   ",buf,CCYEL(ch,C_NRM),CCNRM(ch,C_NRM));
        sprintf(buf,"%s    %s%s",buf,CCBLD(ch,C_CMP),CCYEL(ch,C_NRM));
        sprintf(buf,"%sH%s - %sHelp         \r\n",buf,CCYEL(ch,C_NRM),CCNRM(ch,C_NRM));
        sprintf(buf,"%s%s%s            ",buf,CCBLD(ch,C_CMP),CCYEL(ch,C_NRM));
        sprintf(buf,"%sS%s - %sSave and Exit    ",buf,CCYEL(ch,C_NRM),CCNRM(ch,C_NRM));
        sprintf(buf,"%s    %s%s",buf,CCBLD(ch,C_CMP),CCYEL(ch,C_NRM));
        sprintf(buf,"%sQ%s - %sQuit (Cancel)\r\n",buf,CCYEL(ch,C_NRM),CCNRM(ch,C_NRM));
        sprintf(buf,"%s%s%s            ",buf,CCBLD(ch,C_CMP),CCYEL(ch,C_NRM));
        sprintf(buf,"%sL%s - %sReplace Line     ",buf,CCYEL(ch,C_NRM),CCNRM(ch,C_NRM));
        sprintf(buf,"%s    %s%s",buf,CCBLD(ch,C_CMP),CCYEL(ch,C_NRM));
        sprintf(buf,"%sD%s - %sDelete Line  \r\n",buf,CCYEL(ch,C_NRM),CCNRM(ch,C_NRM));
        sprintf(buf,"%s%s%s            ",buf,CCBLD(ch,C_CMP),CCYEL(ch,C_NRM));
        sprintf(buf,"%sI%s - %sInsert Line      ",buf,CCYEL(ch,C_NRM),CCNRM(ch,C_NRM));
        sprintf(buf,"%s    %s%s",buf,CCBLD(ch,C_CMP),CCYEL(ch,C_NRM));
        sprintf(buf,"%sR%s - %sRefresh Screen\r\n",buf,CCYEL(ch,C_NRM),CCNRM(ch,C_NRM));
        sprintf(buf,"%s%s%s            ",buf,CCBLD(ch,C_CMP),CCYEL(ch,C_NRM));
        sprintf(buf,"%sC%s - %sClear Buffer     ",buf,CCYEL(ch,C_NRM),CCNRM(ch,C_NRM));
        sprintf(buf,"%s    %s%s",buf,CCBLD(ch,C_CMP),CCYEL(ch,C_NRM));
        sprintf(buf,"%sU%s - %sUndo Changes  \r\n",buf,CCYEL(ch,C_NRM),CCNRM(ch,C_NRM));
        sprintf(buf,"%s%s%s     *",buf,CCBLD(ch,C_CMP),CCCYN(ch,C_NRM));
        sprintf(buf,"%s%s-------------------------------------------------------",buf,CCBLU(ch,C_NRM));
        sprintf(buf,"%s%s*%s\r\n",buf,CCCYN(ch,C_NRM),CCNRM(ch,C_NRM));
        SendMessage(buf);
    } else {
        inStr++;
        inStr = one_argument(inStr,command);
        *command = tolower(*command);
        switch(*command) {
            case 'f':
                sprintf(buf,"%sFind & Replace: '&f' \r\n",buf);
                sprintf(buf,"%s&f [red] [yellow]\r\n",buf);
                sprintf(buf,"%sReplaces all occurances of \"red\" with \"yellow\".\r\n",buf);
                sprintf(buf,"%se.g. 'That is a red dog.' would become 'That is a yellow dog.'\r\n",buf);
                sprintf(buf,"%sAlso, 'Fred is here.' would become 'Fyellow is here.'\r\n",buf);
                sprintf(buf,"%sWhen replacing words, remember to include the spaces     to either side\r\n",buf);
                sprintf(buf,"%sin the replacement to keep from replacing partial words my mistake.\r\n",buf);
                sprintf(buf,"%s(i.e. use '[ red ] [ yellow ]' instead of '[red] [yellow]'.\r\n",buf);
                break;
            case 'r':
                sprintf(buf,"%sRefresh Screen: &r\r\n",buf);
                sprintf(buf,"%sPrints out the entire text buffer.\r\n",buf);
                sprintf(buf,"%s(The message/post/description)\r\n",buf);
                sprintf(buf,"%sA single period '.' without an '&' can be used as well.\r\n",buf);
                break;
            case 'l':
                sprintf(buf,"%sReplace Line: &l\r\n",buf);
                sprintf(buf,"%s&l <line #> <replacement text>\r\n",buf);
                sprintf(buf,"%sReplaces line <line #> with <replacement text>.",buf);
                sprintf(buf,"%sSpaces are saved in the replacement text to save indentation.\r\n",buf);
                break;
            case 'd':
                sprintf(buf,"%sDelete Line: &d\r\n",buf);
                sprintf(buf,"%s&d <line #>\r\n",buf);
                sprintf(buf,"%sDeletes line <line #>\r\n",buf);
                break;
            case 'i':
                sprintf(buf,"%sInsert Line: &i\r\n",buf);
                sprintf(buf,"%s&i <line #> <insert text>\r\n",buf);
                sprintf(buf,"%sInserts <insert text> before line <line #>.\r\n",buf);
                sprintf(buf,"%sSpaces are saved in the replacement text to save indentation\r\n",buf);
                sprintf(buf,"%sA note to TinTin users:\r\n",buf);
                sprintf(buf,"%s    Tintin removes all spaces before a command.\r\n",buf);
                sprintf(buf,"%s    Use '&i <current line #> <spaces> <text>' to indent a line.\r\n",buf);
                break;
            case 'u':
                sprintf(buf,"%sUndo Changes: &u\r\n",buf);
                sprintf(buf,"%s&u yes\r\n",buf);
                sprintf(buf,"%sUndoes all changes since last save.\r\n",buf);
                sprintf(buf,"%s(Changes do not save until you exit the editor)\r\n",buf);
                break;
            case 'c':
                sprintf(buf,"%sClear Buffer: &c\r\n",buf);
                sprintf(buf,"%sDeletes ALL text in the current buffer.\r\n",buf);
                break;
            default:
                sprintf(buf,"Sorry. There is no help on that.\r\n");
        }
        SendMessage(buf);
    }
}
bool CTextEditor::ProcessCommand(char *inStr) {
    int line;
    char command[MAX_INPUT_LENGTH];
    inStr++;
    if(!*inStr) {
        SendMessage("Invalid Command\r\n");
    }
    inStr = one_argument(inStr,command);
    *command = tolower(*command);
    switch(*command) {
        case 'h': // Help
            ProcessHelp(inStr);
            return true;
            break;
        case 'f':   // Find/Replace
            return FindReplace(inStr);
            break;
        case 's':   // Save and Exit
            SaveText(inStr);
            desc->text_editor=NULL;
            delete this;
            return true;
            break;
        case 'l':   // Replace Line
            inStr = one_argument(inStr,command);
            if(!isdigit(*command)) {
                SendMessage("Format for Replace Line is: &l <line #> <text>\r\n");
                return false;
            }
            line = atoi(command);
            if(line < 0) {
                SendMessage("Format for Replace Line is: &l <line #> <text>\r\n");
                return false;
            }
            return ReplaceLine((unsigned int)line,inStr);
            break;
        case 'i':   // Insert Line
            inStr = one_argument(inStr,command);
            if(!isdigit(*command)) {
                SendMessage("Format for insert command is: &insert <line #> <text>\r\n");
                return false;
            }
            line = atoi(command);
            if(line < 0) {
                SendMessage("Format for insert command is: &insert <line #><text>\r\n");
                return false;
            }
            return Insert((unsigned int)line,inStr);
            break;  
        case 'c':   // Clear Buffer
            Clear();
            SendMessage("Cleared.\r\n");
            return true;
            break;
        case 'q':   // Quit without saving
            SendMessage("Not Implemented.\r\n");
            return true;
            break;
        case 'd':   // Delete Line
            inStr = one_argument(inStr,command);
            if(!isdigit(*command)) {
                SendMessage("Format for delete command is: &d <line #>\r\n");
                return false;
            }
            line = atoi(command);
            if(line < 0) {
                SendMessage("Format for delete command is: &d <line #>\r\n");
                return false;
            }
            return Remove((unsigned int)line);
            break;
        case 'r':   // Refresh Screen
            List();
            break;
        case 'u':   // Undo Changes
            UndoChanges(inStr);
            break;
    }

    return false;
}

