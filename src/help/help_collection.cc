#include <fstream>
using namespace std;
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
static char gHelpbuf[MAX_STRING_LENGTH * 2];
static char linebuf[MAX_STRING_LENGTH];
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
const char *help_bit_descs[] = {
    "!APP",
    "IMM+",
    "MOD",
    "\n"
};
const char *help_bits[] = {
    "unapproved",
    "immortal",
    "modified",
    "\n"
};

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
void HelpCollection::Push(HelpItem *n) {
    if(bottom) {
        bottom->SetNext(n);
		bottom = n;
    } else {
        bottom = n;
        items = n;
    }
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
    int start = 0,end = top_id;
    int space_left = sizeof(gHelpbuf) - 480;
    skip_spaces(&args);
    args = one_argument(args,linebuf);
    if(linebuf[0] && !strncmp("range",linebuf,strlen(linebuf))) {
        args = one_argument(args,linebuf);
        if(isdigit(linebuf[0]))
            start = atoi(linebuf);
        args = one_argument(args,linebuf);
        if(isdigit(linebuf[0]))
            end = atoi(linebuf);
    }
    sprintf(gHelpbuf,"Help Topics (%d,%d):\r\n",start,end);
    space_left -= strlen(gHelpbuf);
    for(cur = items;cur;cur = cur->Next()) {
        if(cur->idnum > end)
            break;
        if(cur->idnum < start)
            continue;
        cur->Show(ch,linebuf,1);
        strcat(gHelpbuf,linebuf);
        space_left -= strlen(linebuf);
        if(space_left <= 0) {
            sprintf(linebuf,"Maximum buffer size reached at item # %d. \r\nUse \"range\" param for higher numbered items.\r\n",cur->idnum);
            strcat(gHelpbuf,linebuf);
            break;
        }
    }
    page_string(ch->desc, gHelpbuf, 0);
}
// Create an item. (calls Edit)
bool HelpCollection::CreateItem( char_data *ch ) {
    HelpItem *n;

    n = new HelpItem;
    n->idnum = ++top_id;
    Push(n);
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
    if( !GET_OLC_HELP(ch) ) {
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
    GET_OLC_HELP(ch)->Save();
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
    HelpItem *n;
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
            n = new HelpItem;
            index_file  >> n->idnum >> n->groups 
                        >>  n->counter >> n->flags  
                        >> n->owner;

            index_file.getline(fname,100,'\n');
            index_file.getline(fname,100,'\n');
            n->SetName(s);
                
            s = fname;
            index_file.getline(fname,100,'\n');
            n->SetKeyWords(s);
            REMOVE_BIT(n->flags,HFLAG_MODIFIED);
            Push(n);
            num_items++;
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
    int com;
    argument = one_argument(argument, linebuf);
    skip_spaces(&argument);
    if (!*linebuf) {
        do_group_cmds(ch);
        return;
    }
    for (com = 0;;com++) {
        if (!grp_cmds[com].keyword) {
            sprintf(buf, "Unknown group command, '%s'.\r\n", linebuf);
            send_to_char(buf, ch);
            return;
        }
        if (is_abbrev(linebuf, grp_cmds[com].keyword))
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
    int com;
    int id;
    HelpItem *cur;
    argument = one_argument(argument, linebuf);
    skip_spaces(&argument);
    if (!*linebuf) {
        do_hcollect_cmds(ch);
        return;
    }
    for (com = 0;;com++) {
        if (!hc_cmds[com].keyword) {
            sprintf(buf, "Unknown hcollect command, '%s'.\r\n", linebuf);
            send_to_char(buf, ch);
            return;
        }
        if (is_abbrev(linebuf, hc_cmds[com].keyword))
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
            argument = one_argument(argument,linebuf);
            if(isdigit(*linebuf)) {
                id = atoi(linebuf);
                Help->EditItem(ch,id);
            } else if(!strncmp("exit",linebuf,strlen(linebuf))) {
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
            argument = one_argument(argument,linebuf);
            if(*linebuf && isdigit(linebuf[0])) {
                id = atoi(linebuf);
                if(id < 0 || id > Help->GetTop()) {
                    send_to_char("There is no such item #.\r\n",ch);
                    break;
                }
                for(cur = Help->items; cur && cur->idnum != id; cur = cur->Next());
                if(cur) {
                    cur->Show(ch,linebuf,3);
                    send_to_char(linebuf,ch);
                    break;
                } else {
                    sprintf(buf,"There is no item: %d.\r\n",id);
                    send_to_char(buf,ch);
                    break;
                }
            }
            if(GET_OLC_HELP(ch)) {
                GET_OLC_HELP(ch)->Show(ch,linebuf,3);
                send_to_char(linebuf,ch);
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

