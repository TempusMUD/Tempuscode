#include <fstream.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include "structs.h"
#include "char_data.h"
#include "utils.h"
#include "defs.h"
#include "help.h"
#include "interpreter.h"
#include "db.h"
#include "comm.h"
#include "screen.h"
#include "handler.h"

// Where under lib do we toss our data?
const char *Help_Directory = "text/help_data/";
// The global HelpCollection object.
// Allocated in comm.cc
HelpCollection *Help = NULL;
// Since one_word isnt int he header file...
static char gHelpbuf[MAX_HELP_TEXT_LENGTH];
char *one_word(char *argument, char *first_arg);
static const struct hcollect_command {
    char *keyword;
    char *usage;
    int level;
} hc_cmds[] = {
    { "approve", "<topic #>",              LVL_GOD },
    { "create",  "",                       LVL_GOD },
    { "edit",    "<topic #>",              LVL_IMMORT },
    { "group",   "(subcommand)",           LVL_IMMORT },
    { "info",    "",                       LVL_GOD },
    { "list",    "[range <start>[end]]",   LVL_IMMORT },
    { "save",    "",                       LVL_IMMORT },
    { "set",     "<param><value>",         LVL_IMMORT },
    { "stat",    "[<topic #>]",            LVL_IMMORT },
    { "sync",    "",                       LVL_GRGOD  },
    { "search",  "<keyword>",              LVL_IMMORT },
    { NULL, NULL, 0 }       // list terminator
};
static const struct group_command {
    char *keyword;
    char *usage;
    int level;
} grp_cmds[] = {
    { "adduser", "<username> <groupname>",     LVL_GRGOD },
    { "create",  "",                           LVL_GRGOD },
    { "list",    "",                           LVL_IMMORT },
    { "members", "<groupname>",                LVL_IMMORT },
    { "remuser", "<username> <groupname>",     LVL_GRGOD },
    { NULL, NULL, 0 }       // list terminator
};
const char *help_group_names[] = {
    "olc",
    "misc",
    "newbie",
    "skill",
    "spell",
    "class",
    "city",
    "player",
    "mage",
    "cleric",
    "thief",
    "warrior",
    "barb",
    "psionic",
    "physic",
    "cyborg",
    "knight",
    "ranger",
    "hood",
    "monk",
    "mercenary",
    "helpeditors",
    "\n"
};
const char *help_group_bits[] = {
    "OLC",
    "MISC",
    "NEWB",
    "SKIL",
    "SPEL",
    "CLAS",
    "CITY",
    "PLR",
    "MAGE",
    "CLE",
    "THI",
    "WAR",
    "BARB",
    "PSI",
    "PHY",
    "CYB",
    "KNIG",
    "RANG",
    "HOOD",
    "MONK",
    "MERC",
    "HEDT",
    "\n"
};
static const char *help_bit_descs[] = {
    "!APP",
    "IMM+",
    "MOD",
    "\n"
};
static const char *help_bits[] = {
    "unapproved",
    "immortal",
    "modified",
    "\n"
};

// Sets the flags bitvector for things like !approved and
// IMM+ and changed and um... other flaggy stuff.
void HelpItem::SetFlags( char *argument ) {
    int state, cur_flags = 0, tmp_flags = 0, flag = 0, old_flags = 0;
    char arg1[MAX_INPUT_LENGTH];
    argument = one_argument(argument,arg1);
    skip_spaces(&argument);
    if (!*argument) {
        send_to_char("Invalid flags. \r\n",editor);
        return;
    }

    if (*arg1 == '+') {
        state = 1;
    } else if (*arg1 == '-') {
        state = 2;
    } else {
        send_to_char("Invalid flags.\r\n",editor);
        return;
    }

    argument = one_argument(argument, arg1);
    old_flags = cur_flags = flags;
    while (*arg1) {
        if ((flag = search_block(arg1, help_bits,FALSE)) == -1) {
            sprintf(buf, "Invalid flag %s, skipping...\r\n", arg1);
            send_to_char(buf, editor);
        } else {
            tmp_flags = tmp_flags | (1 << flag);
        }
        argument = one_argument(argument, arg1);
    }
    REMOVE_BIT(tmp_flags,HFLAG_UNAPPROVED);
    if (state == 1) {
        cur_flags = cur_flags | tmp_flags;
    } else {
        tmp_flags = cur_flags & tmp_flags;
        cur_flags = cur_flags ^ tmp_flags;
    }

    flags = cur_flags;
    tmp_flags = old_flags ^ cur_flags;
    sprintbit(tmp_flags, help_bits, buf2);

    if (tmp_flags == 0) {
        sprintf(buf, "Flags for help item %d not altered.\r\n", idnum);
        send_to_char(buf, editor);
    } else {
        sprintf(buf, "[%s] flags %s for help item %d.\r\n", buf2,
            state == 1 ? "added" : "removed", idnum);
        send_to_char(buf, editor);
    }
}
// Crank up the text editor and lets hit it.
void HelpItem::EditText( void ) {
    SET_BIT(PLR_FLAGS(editor), PLR_OLC);

    if (text) {
        send_to_char("Use TED to modify the description.\r\n", editor);
        editor->desc->editor_mode = 1;
        editor->desc->editor_cur_lnum = get_line_count(text);
    } else {
        send_to_char(TED_MESSAGE, editor);
    }

    editor->desc->str = &text;
    editor->desc->max_str = MAX_HELP_TEXT_LENGTH;

    act("$n begins to edit a help file.\r\n",TRUE,editor,0,0,TO_ROOM);
}   
// Sets the groups bitvector much like the
// flags in quests.
void HelpItem::SetGroups(char *argument) {
    int state, cur_groups = 0, tmp_groups = 0, flag = 0, old_groups = 0;
    char arg1[MAX_INPUT_LENGTH];
    argument = one_argument(argument,arg1);
    skip_spaces(&argument);
    if (!*argument) {
        send_to_char("Invalid group. \r\n",editor);
        return;
    }

    if (*arg1 == '+') {
        state = 1;
    } else if (*arg1 == '-') {
        state = 2;
    } else {
        send_to_char("Invalid Groups.\r\n",editor);
        return;
    }

    argument = one_argument(argument, arg1);
    old_groups = cur_groups = groups;
    while (*arg1) {
        if ((flag = search_block(arg1, help_group_names,FALSE)) == -1) {
            sprintf(buf, "Invalid group: %s, skipping...\r\n", arg1);
            send_to_char(buf, editor);
        } else {
            tmp_groups = tmp_groups | (1 << flag);
        }
        argument = one_argument(argument, arg1);
    }
    if (state == 1) {
        cur_groups = cur_groups | tmp_groups;
    } else {
        tmp_groups = cur_groups & tmp_groups;
        cur_groups = cur_groups ^ tmp_groups;
    }
    groups = cur_groups;
    tmp_groups = old_groups ^ cur_groups;
    sprintbit(tmp_groups, help_group_bits, buf2);
    if (tmp_groups == 0) {
        sprintf(buf, "Groups for help item %d not altered.\r\n", idnum);
        send_to_char(buf, editor);
    } else {
        sprintf(buf, "[%s] groups %s for help item %d.\r\n", buf2,
            state == 1 ? "added" : "removed", idnum);
        send_to_char(buf, editor);
    }
    REMOVE_BIT(groups,HGROUP_HELP_EDIT);
    SET_BIT(flags,HFLAG_MODIFIED);
}
// Don't call me Roger.
void HelpItem::SetName(char *argument) {
    skip_spaces(&argument);
    name = new char[strlen(argument) + 1];
    strcpy(name, argument);
    SET_BIT(flags,HFLAG_MODIFIED);
    if(editor)
        send_to_char("Name set!\r\n",editor);
}

// Set the...um. keywords and stuff.
void HelpItem::SetKeyWords(char *argument) {
    skip_spaces(&argument);
    keys = new char[strlen(argument) + 1];
    strcpy(keys, argument);
    SET_BIT(flags,HFLAG_MODIFIED);
    if(editor)
        send_to_char("Keywords set!\r\n",editor);
}
// Help Item
HelpItem::HelpItem(){
    idnum = 0;
    next = NULL;
    next_show = NULL;
    editor = NULL;
    text = NULL;
    name = NULL;
    keys = NULL;
    Clear();
}

HelpItem::~HelpItem(){
    delete text;
    delete keys;
    delete name;
}

// Clear out the item.
bool HelpItem::Clear( void ) {
    counter = 0;
    flags = (HFLAG_UNAPPROVED | HFLAG_MODIFIED);
    groups = 0;

    if(text) {
        delete text;
        text = NULL;
    }
    if(name) {
        delete name;
        name = NULL;
    }
    if(keys) {
        delete keys;
        keys = NULL;
    }
    name = new char[32];
    strcpy(name,"A New Help Entry");
    keys = new char[32];
    strcpy(keys,"new help entry");
    return true;
}

// Begin editing an item
// much like olc oedit.
// Sets yer currently editable item to this one.
bool HelpItem::Edit( char_data *ch ) {
    if(editor) {
        if(editor != ch)
            send_to_char("Someone is already editing that item!\r\n",ch);
        else
            send_to_char("You're already editing that item!\r\n",ch);
        return false;
    }
    if(GET_OLC_HELP(ch))
        GET_OLC_HELP(ch)->editor = NULL;
    GET_OLC_HELP(ch) = this;
    editor = ch;
    sprintf(buf,"You are now editing help item #%d.\r\n",idnum);
    send_to_char(buf,ch);
    SET_BIT(flags,HFLAG_MODIFIED);
    return true;
}

// Push on the the tail of the items list.
// Make sure the set items and bottom equal
// to the first element you push in.
inline HelpItem *HelpItem::Push( HelpItem *tail) {
    if(tail)
        tail->next = this;
    return this;
}
// Push into the "next to show" list.
inline HelpItem *HelpItem::PushShow( HelpItem *list ) {
    next_show = list;
    return this;
}
// Returns the next HelpItem
inline HelpItem *HelpItem::Next( void ){
    return next;
} 
// Returns the next HelpItem in the list to be shown
inline HelpItem *HelpItem::NextShow( void ){
    return next_show;
} 
// Wrapper for the next_show pointer
inline void HelpItem::BlankShow( void ){
    next_show = NULL;
}

// Saves the current HelpItem. Does NOT alter the index.
// You have to save that too.
// (SaveAll does everything)
bool HelpItem::Save(){
    char fname[256];
    ofstream file;
    sprintf(fname,"%s/%04d.topic",Help_Directory,idnum);
    // If we dont have the text to edit, get from the file.
    if(!text) 
        LoadText();
    remove(fname);
    file.open(fname,ios::out | ios::trunc);
    if(!file && editor){
        send_to_char("Error, could not open help file for write.\r\n",editor);
        return false;
    }
    file.seekp(0);
    file << idnum << " " 
         << (text ? strlen(text) + 1 : 0) 
         << endl << name << endl;
    if(text)
        file.write(text,strlen(text));
    file.close();
    REMOVE_BIT(flags,HFLAG_MODIFIED);
    return true;
}

// Loads in the text for a particular help entry.
bool HelpItem::LoadText() {
    char fname[256];
    int di;

    sprintf(fname,"%s/%04d.topic",Help_Directory,idnum);
    help_file.open(fname,ios::in | ios::nocreate);
    if(!help_file) {
        sprintf(buf,"Unable to open help file (%s).",fname);
        return false;
    }
    help_file.seekp(0);
    if(!text)
        text = new char[MAX_HELP_TEXT_LENGTH];
    strcpy(text,"");

    help_file >> di >> di;
    help_file.getline(fname,256,'\n'); // eat the \r\n at the end of the #s
    help_file.getline(fname,256,'\n'); // then burn up the name since we dont really need it.
    if(di > 0)
        help_file.read(text,di);
    help_file.close();
    return true;
}

// Show the entry. 
// buffer is output buffer.
void HelpItem::Show( char_data *ch, char *buffer,int mode=0 ){
    char bitbuf[256];
    char groupbuf[256];
    switch ( mode ) {
        case 0: // 0 == One Line Listing.
            sprintf(buffer,"Name: %s\r\n    (%s)\r\n",name,keys);
            break;
        case 1: // 1 == One Line Stat
            sprintbit(flags, help_bit_descs, bitbuf);
            sprintbit(groups, help_group_bits, groupbuf);
            sprintf(buffer,"%s%3d. %s%-20s %sGroups: %s%-20s %sFlags:%s %s "
                    "\r\n        %sKeywords: [ %s%s%s ]\r\n",
                    CCCYN(ch,C_NRM),idnum, CCYEL(ch,C_NRM),
                    name, CCCYN(ch,C_NRM),CCNRM(ch,C_NRM),
                    groupbuf, CCCYN(ch,C_NRM),
                    CCNRM(ch,C_NRM),bitbuf, CCCYN(ch,C_NRM),
                    CCNRM(ch,C_NRM),keys,CCCYN(ch,C_NRM));
            break;
        case 2: // 2 == Entire Entry  
            if(!text)
                LoadText();
            sprintf(buffer,"    %s%s%s\r\n%s\r\n",
                CCCYN(ch,C_NRM),name,CCNRM(ch,C_NRM),text);
            counter++;
            break;
        case 3: // 3 == Entire Entry Stat
            if(!text)
                LoadText();
            sprintbit(flags, help_bit_descs, bitbuf);
            sprintbit(groups, help_group_bits, groupbuf);
            sprintf(buffer,
            "%s%3d. %s%-20s %sGroups: %s%-20s %sFlags:%s %s \r\n        %s"
                    "Keywords: [ %s%s%s ]\r\n%s%s\r\n",
                    CCCYN(ch,C_NRM),idnum, CCYEL(ch,C_NRM),
                    name, CCCYN(ch,C_NRM),CCNRM(ch,C_NRM),
                    groupbuf, CCCYN(ch,C_NRM),
                    CCNRM(ch,C_NRM),bitbuf, CCCYN(ch,C_NRM),
                    CCNRM(ch,C_NRM),keys,CCCYN(ch,C_NRM),
                    CCNRM(ch,C_NRM),text);
            break;
        default:
            break;
    }
}

// Help Collection
HelpCollection::HelpCollection() {
    need_save = false;
    items = NULL;
    bottom = NULL;
    top_id = 0;
}
HelpCollection::~HelpCollection() {
    HelpItem *i;
    while(items) {
        i = items;
        items = items->Next();
        delete i;
    }
    slog("Help system ended.");
}
int HelpCollection::GetTop( void ) {
    return top_id;
}
// Calls FindItems 
void HelpCollection::GetTopic(char_data *ch, char *args,int mode=2,bool show_no_app=false) {

    HelpItem *cur = NULL;
    gHelpbuf[0] = '\0';
    if(!args || !*args) {
        send_to_char("You must enter search criteria.\r\n",ch);
        return;
    }
    cur = FindItems( args, show_no_app );
    if(!cur) {
        send_to_char("No items were found matching your search criteria.\r\n",ch);
        return;
    }
    cur->Show( ch, gHelpbuf, mode );
    page_string(ch->desc,gHelpbuf,0);
    return;
}
// Show all the items
void HelpCollection::List( char_data *ch, char *args ) {
    HelpItem *cur;
    char sbuf[MAX_HELP_TEXT_LENGTH];
    int start = 0,end = top_id;
    skip_spaces(&args);
    args = one_argument(args,sbuf);
    if(sbuf[0] && !strncmp("range",sbuf,strlen(sbuf))) {
        args = one_argument(args,sbuf);
        if(isdigit(sbuf[0]))
            start = atoi(sbuf);
        args = one_argument(args,sbuf);
        if(isdigit(sbuf[0]))
            end = atoi(sbuf);
    }
    sprintf(gHelpbuf,"Help Topics (%d,%d):\r\n",start,end);
    for(cur = items;cur;cur = cur->Next()) {
        if(cur->idnum > end)
            break;
        if(cur->idnum < start)
            continue;
        cur->Show(ch,sbuf,1);
        strcat(gHelpbuf,sbuf);
    }
    page_string(ch->desc, gHelpbuf, 0);
}
// Create an item. (calls Edit)
bool HelpCollection::CreateItem( char_data *ch ) {
    HelpItem *n;

    n = new HelpItem();
    n->idnum = ++top_id;
    bottom = n->Push(bottom);
    if(!items)
        items = bottom;
    sprintf(buf,"Item #%d created.\r\n",n->idnum);
    send_to_char(buf,ch);
    n->Save();
    SaveIndex(ch);
    n->Edit(ch);
    SET_BIT(n->flags,HFLAG_MODIFIED);
    sprintf(buf,"%s has help topic # %d.",GET_NAME(ch),n->idnum);
    slog(buf);
    return true;
}
// Begin editing an item
bool HelpCollection::EditItem( char_data *ch, int idnum ) {
    // See if you can edit it before you do....
    HelpItem *cur;
    for(cur = items;cur && cur->idnum != idnum;cur = cur->Next());
    if(cur && Groups.CanEdit(ch,cur)) {
        cur->Edit(ch);
        return true;
    }
    send_to_char("No such item.\r\n",ch);
    return false;
}
// Clear an item
bool HelpCollection::ClearItem( char_data *ch ) {
    if( !ch->player_specials->olc_help_item ) {
        send_to_char("You must be editing an item to clear it.\r\n",ch);
        return false;
    }
    GET_OLC_HELP(ch)->Clear();
    return true;
}
// Save and Item
bool HelpCollection::SaveItem( char_data *ch ) {
    if(!GET_OLC_HELP(ch)) {
        send_to_char("You must be editing an item to save it.\r\n",ch);
        return false;
    }
    GET_OLC_HELP(ch)->Clear();
    return true;
}
// Find an Item in the index 
// This should take an optional "mode" argument to specify groups the
//  returned topic can be part of. e.g. (FindItems(argument,FIND_MODE_OLC))
HelpItem *HelpCollection::FindItems( char *args, bool find_no_approve=false ) {
    HelpItem *cur = NULL;
    char stack[256];
    char *b;// beginning of stack
    char bit[256];// the current bit of the stack we're lookin through
    int length;
    for(b = args;*b;b++)
        *b = tolower(*b);
    length = strlen(args);
    for(cur = items; cur; cur = cur->Next()) {
        if(IS_SET(cur->flags,HFLAG_UNAPPROVED) && !find_no_approve)
            continue;
        strcpy(stack,cur->keys);
        b = stack;
        while(*b) {
            b = one_word(b,bit);
            if(!strncmp(bit,args,length)) {
                return cur;
            }
        }
    }
    return cur;
}
// Save everything.
bool HelpCollection::SaveAll( char_data *ch ) {
    HelpItem *cur;
    SaveIndex(ch);
    Groups.Save();
    for(cur = items;cur;cur = cur->Next()) {
        if(IS_SET(cur->flags,HFLAG_MODIFIED))
            cur->Save();
    }
    Sync();
    send_to_char("Saved.\r\n",ch);
    sprintf(buf,"%s has saved the help system.",GET_NAME(ch));
    slog(buf);
    return true;
}
// Save the index
bool HelpCollection::SaveIndex( char_data *ch ) {
    char fname[128];
    HelpItem *cur=NULL;
    int num_items = 0;
    sprintf(fname,"%s/%s",Help_Directory,"index");

    index_file.open(fname,ios::out | ios::trunc);
    index_file.seekp(0);
    if(!index_file) {
        slog("SYSERR: Cannot open help index.");
        return false;
    }
    index_file << top_id << endl;
    for(cur = items;cur;cur = cur->Next()) {
        index_file << cur->idnum << " " << cur->groups << " " 
                   << cur->counter << " " << cur->flags << " " 
                   << cur->owner << endl;
        index_file << cur->name << endl << cur->keys << endl;
        num_items++;
    }
    index_file.close();
    if(num_items == 0){
        slog("SYSERR: No records saved to help file index.");
        return false;
    }
    sprintf(buf,"Help: %d items saved to help file index\r\n",num_items);
    return true;
}
// Load the items from the index file
bool HelpCollection::LoadIndex() {
    char fname[256],*s;
    HelpItem *theItem;
    int num_items = 0;
    s = fname;
    sprintf(fname,"%s/%s",Help_Directory,"index");

    index_file.open(fname,ios::in | ios::nocreate);
    if(!index_file){
        slog("SYSERR: Cannot open help index.");
        return false;
    }
    index_file.seekp(0);
    index_file >> top_id;
    for(int i = 1;i <= top_id; i++) {
            theItem = new HelpItem;
            index_file  >> theItem->idnum >> theItem->groups 
                        >>  theItem->counter >> theItem->flags  
                        >> theItem->owner;

            index_file.getline(fname,100,'\n');
            index_file.getline(fname,100,'\n');
            theItem->SetName(s);
                
            s = fname;
            index_file.getline(fname,100,'\n');
            theItem->SetKeyWords(s);

            bottom = theItem->Push(bottom);
            if(!items)
                items = bottom;
            num_items++;
            REMOVE_BIT(theItem->flags,HFLAG_MODIFIED);
    }
    index_file.close();
    if(num_items == 0){
        slog("SYSERR: No records read from help file index.");
        return false;
    }
    sprintf(buf,"%d items read from help file index",num_items);
    slog(buf);
    return true;
}
// Funnels outside commands into HelpItem functions
// (that should be protected or something... shrug.)
bool HelpCollection::Set(char_data *ch, char *argument) {
    char arg1[256];
    if(!GET_OLC_HELP(ch)) {
        send_to_char("You have to be editing an item to set it.\r\n",ch);
        return false;
    }
    if(!argument || !*argument) {
        send_to_char("hcollect set <groups[+/-]|flags[+/-]|name|keywords|description> [args]\r\n",ch);
        return false;
    }
    argument = one_argument(argument,arg1);
    if(!strncmp(arg1,"groups",strlen(arg1))) {
        GET_OLC_HELP(ch)->SetGroups(argument);
        return true;
    } else if(!strncmp(arg1,"flags",strlen(arg1))) {
        GET_OLC_HELP(ch)->SetFlags(argument);
        return true;
    } else if(!strncmp(arg1,"name",strlen(arg1))) {
        GET_OLC_HELP(ch)->SetName(argument);
        return true;
    } else if(!strncmp(arg1,"keywords",strlen(arg1))) {
        GET_OLC_HELP(ch)->SetKeyWords(argument);
        return true;
    } else if(!strncmp(arg1,"description",strlen(arg1))) {
        GET_OLC_HELP(ch)->EditText();
        return true;
    }
    return false; 
}

// Trims off the text of all the help items that people have looked at.
// (each text is approx 65k. We only want the ones in ram that we need.)
void HelpCollection::Sync( void ) {
    HelpItem *n; 
    for (n = items; n; n = n->Next()) {
        if( n->text && !IS_SET(n->flags, HFLAG_MODIFIED) && !n->editor ) {
            delete n->text;
            n->text = NULL;
        }
    }
}
// Approve an item
void HelpCollection::ApproveItem( char_data *ch, char *argument) {
        char arg1[256];
        int idnum = 0;
        HelpItem *cur;
        skip_spaces(&argument);
        argument = one_argument(argument,arg1);
        if(isdigit(arg1[0])) 
            idnum = atoi(arg1);
        if(idnum > top_id || idnum < 1) {
            send_to_char("Approve which item?\r\n",ch);
            return;
        }
        for(cur = items;cur && cur->idnum != idnum;cur = cur->Next());
        if(cur) {
            REMOVE_BIT(cur->flags,HFLAG_UNAPPROVED);
            sprintf(buf,"Item #%d approved.\r\n",cur->idnum);
            send_to_char(buf,ch);
        } else {
            send_to_char("Unable to find item.\r\n",ch);
        }
    return;
}
// Give some stat info on the Help System
void HelpCollection::Show( char_data *ch) {
    int num_items = 0;
    int num_modified = 0;
    int num_unapproved = 0;
    int num_editing = 0;
    int num_no_group = 0;
    HelpItem *cur;
    
    for(cur = items;cur;cur = cur->Next()) {
        num_items++;
        if(cur->flags != 0) {
            if(IS_SET(cur->flags,HFLAG_UNAPPROVED))
                num_unapproved++;
            if(IS_SET(cur->flags,HFLAG_MODIFIED))
                num_modified++;
        }
        if(cur->editor)
            num_editing++;
        if(cur->groups == 0)
            num_no_group++;
    }
    sprintf(buf,"%sTopics [%s%d%s] %sUnapproved [%s%d%s] %sModified [%s%d%s] "
                "%sEditing [%s%d%s] %sGroupless [%s%d%s]%s\r\n",
        CCCYN(ch,C_NRM), CCNRM(ch,C_NRM),num_items,CCCYN(ch,C_NRM),
        CCYEL(ch,C_NRM),CCNRM(ch,C_NRM),num_unapproved,CCYEL(ch,C_NRM),
        CCGRN(ch,C_NRM),CCNRM(ch,C_NRM),num_modified,CCGRN(ch,C_NRM), 
        CCCYN(ch,C_NRM),CCNRM(ch,C_NRM),num_editing,CCCYN(ch,C_NRM),
        CCRED(ch,C_NRM),CCNRM(ch,C_NRM),num_no_group,CCRED(ch,C_NRM),
        CCNRM(ch,C_NRM));
    send_to_char(buf,ch);
    Groups.Show(ch);
}
// Blah blah print out the hcollect commands.
void do_hcollect_cmds(CHAR *ch) {
    strcpy(gHelpbuf, "hcollect commands:\r\n");
    for(int i = 0;hc_cmds[i].keyword;i++) {
        if(GET_LEVEL(ch) < hc_cmds[i].level)
            continue;
        sprintf(gHelpbuf, "%s  %-15s %s\r\n", gHelpbuf, 
            hc_cmds[i].keyword, hc_cmds[i].usage);
    }
    page_string(ch->desc, gHelpbuf, 1);
}
// blah blah usage blah
void do_hcollect_usage(CHAR *ch, int com) {
    if (com < 0) {
        do_hcollect_cmds(ch);
    } else {
        sprintf(buf, "Usage: hcollect %s %s\r\n",
            hc_cmds[com].keyword, hc_cmds[com].usage);
        send_to_char(buf, ch);
    }
}
// group command listing
void do_group_cmds(CHAR *ch) {
    strcpy(gHelpbuf, "hcollect group commands:\r\n");
    for(int i = 0;grp_cmds[i].keyword;i++)  {
        if(GET_LEVEL(ch) < grp_cmds[i].level)
            continue;
        sprintf(gHelpbuf, "%s  %-15s %s\r\n", gHelpbuf, 
            grp_cmds[i].keyword, grp_cmds[i].usage);
    }
    page_string(ch->desc, gHelpbuf, 1);
}

void do_group_usage(CHAR *ch, int com) {
    if (com < 0) {
        do_group_cmds(ch);
    } else {
        sprintf(buf, "Usage: hcollect group %s %s\r\n",
            grp_cmds[com].keyword, grp_cmds[com].usage);
        send_to_char(buf, ch);
    }
}
// Group command parser. 
// Yeah yeah. Prolly doesn't need it but here it is.
static void do_group_command(char_data *ch, char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    int com;
    argument = one_argument(argument, arg1);
    skip_spaces(&argument);
    if (!*arg1) {
        do_group_cmds(ch);
        return;
    }
    for (com = 0;;com++) {
        if (!grp_cmds[com].keyword) {
            sprintf(buf, "Unknown group command, '%s'.\r\n", arg1);
            send_to_char(buf, ch);
            return;
        }
        if (is_abbrev(arg1, grp_cmds[com].keyword))
            break;
    }
    if(grp_cmds[com].level > GET_LEVEL(ch)) {
        send_to_char("You are not godly enough to do this!\r\n",ch);
        return;
    }
    switch(com) {
        case 0: // adduser
            Help->Groups.AddUser(ch,argument);
            break;
        case 1: // create
            //Help->Groups.Create(ch, argument);
            send_to_char("Unimplimented.\r\n",ch);
            break;
        case 2: // list
            Help->Groups.Show(ch);
            break;
        case 3: // member listing "members"
            Help->Groups.Members(ch,argument);
            break;
        case 4: // remuser
            Help->Groups.RemoveUser(ch,argument);
            break;
        default:
            do_group_cmds(ch);
    }
}
// Main command parser for hcollect.
ACMD(do_help_collection_command) {
    char arg1[MAX_INPUT_LENGTH];
    int com;
    int id;
    HelpItem *cur;
    argument = one_argument(argument, arg1);
    skip_spaces(&argument);
    if (!*arg1) {
        do_hcollect_cmds(ch);
        return;
    }
    for (com = 0;;com++) {
        if (!hc_cmds[com].keyword) {
            sprintf(buf, "Unknown hcollect command, '%s'.\r\n", arg1);
            send_to_char(buf, ch);
            return;
        }
        if (is_abbrev(arg1, hc_cmds[com].keyword))
            break;
    }
    if(hc_cmds[com].level > GET_LEVEL(ch)) {
        send_to_char("You are not godly enough to do this!\r\n",ch);
        return;
    }
    switch(com) {
        case 0: // Approve
            Help->ApproveItem( ch, argument );
            break;
        case 1: // Create
            Help->CreateItem(ch);
            break;
        case 2: // Edit
            argument = one_argument(argument,arg1);
            if(isdigit(*arg1)) {
                id = atoi(arg1);
                Help->EditItem(ch,id);
            } else if(!strncmp("exit",arg1,strlen(arg1))) {
                if(!GET_OLC_HELP(ch)) {
                    send_to_char("You're not editing a help topic.\r\n",ch);
                } else {
                    GET_OLC_HELP(ch)->editor = NULL;
                    GET_OLC_HELP(ch) = NULL;
                    send_to_char("Help topic editor exited.\r\n",ch);
                }
            } else {
                send_to_char("hcollect edit <#|exit>\r\n",ch);
            }
            break;
        case 3: // Group
            do_group_command( ch, argument);
            break;
        case 4: // Info
            Help->Show(ch);
            break;
        case 5: // List
            Help->List( ch, argument );
            break;
        case 6: // Save
            Help->SaveAll( ch );
            break;
        case 7: // Set
            Help->Set( ch, argument );
            break;
        case 8: // Stat
            char outbuf[MAX_HELP_TEXT_LENGTH];
            argument = one_argument(argument,arg1);
            if(*arg1 && isdigit(arg1[0])) {
                id = atoi(arg1);
                if(id < 0 || id > Help->GetTop()) {
                    send_to_char("There is no such item #.\r\n",ch);
                    break;
                }
                for(cur = Help->items; cur && cur->idnum != id; cur = cur->Next());
                if(cur) {
                    cur->Show(ch,outbuf,3);
                    send_to_char(outbuf,ch);
                    break;
                } else {
                    sprintf(buf,"There is no item: %d.\r\n",id);
                    send_to_char(buf,ch);
                    break;
                }
            }
            if(GET_OLC_HELP(ch)) {
                GET_OLC_HELP(ch)->Show(ch,outbuf,3);
                send_to_char(outbuf,ch);
            } else {
                send_to_char("Stat what item?\r\n",ch);
            }
            break;
        case 9: // Sync
            Help->Sync();
            send_to_char("Okay.\r\n",ch);
            break;
        case 10: // Search (mode==3 is "stat" rather than "show") show_no_app "true"
            Help->GetTopic(ch,argument,3,true);
            break;
        default:
            do_hcollect_cmds(ch);
            break;
    }
}

