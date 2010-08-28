#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include "structs.h"
#include "creature.h"
#include "utils.h"
#include "help.h"
#include "interpreter.h"
#include "db.h"
#include "comm.h"
#include "screen.h"
#include "handler.h"
#include "tmpstr.h"
#include "security.h"
#include "accstr.h"

// The global HelpCollection object.
// Allocated in comm.cc
struct help_collection *help = NULL;

static char gHelpbuf[MAX_STRING_LENGTH];
static char linebuf[MAX_STRING_LENGTH];
extern const char *class_names[];

char *one_word(char *argument, char *first_arg);
static const struct hcollect_command {
	const char *keyword;
	const char *usage;
	int level;
} hc_cmds[] = {
	{
	"approve", "<topic #>", LVL_DEMI}, {
	"create", "", LVL_DEMI}, {
	"edit", "<topic #>", LVL_IMMORT}, {
	"info", "", LVL_DEMI}, {
	"list", "[range <start>[end]]", LVL_IMMORT}, {
	"save", "", LVL_IMMORT}, {
	"set", "<param><value>", LVL_IMMORT}, {
	"stat", "[<topic #>]", LVL_IMMORT}, {
	"sync", "", LVL_GRGOD}, {
	"search", "<keyword>", LVL_IMMORT}, {
	"unapprove", "<topic #>", LVL_DEMI}, {
	"immhelp", "<keyword>", LVL_IMMORT}, {
	"olchelp", "<keyword>", LVL_IMMORT}, {
	"qchelp", "<keyword>", LVL_IMMORT}, {
	NULL, NULL, 0}				// list terminator
};

static const struct group_command {
	const char *keyword;
    const char *usage;
	int level;
} grp_cmds[] = {
	{
	"adduser", "<username> <groupnames>", LVL_GOD}, {
	"create", "", LVL_GOD}, {
	"list", "", LVL_IMMORT}, {
	"members", "<groupname>", LVL_IMMORT}, {
	"remuser", "<username> <groupnames>", LVL_GOD}, {
	NULL, NULL, 0}				// list terminator
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
	"bard",
	"monk",
	"mercenary",
	"helpeditors",
	"helpgods",
	"immhelp",
	"qcontrol",
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
	"BARD",
	"MONK",
	"MERC",
	"HEDT",
	"HGOD",
	"IMM",
	"QCTR",
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

struct help_collection *
make_help_collection(void)
{
    struct help_collection *col;

    CREATE(col, struct help_collection, 1);

    return col;
}

void
free_help_collection(struct help_collection *col)
{
    g_list_foreach(col->items, free_helpitem, 0);
    g_list_free(col->items);

    slog("Help system ended.");
}

void
help_collection_push(struct help_collection *col, struct help_item *n)
{
    col->items = g_list_append(col->items, n);
}

// Show all the items
void
help_collection_list(struct help_collection *col,
                     struct creature *ch,
                     char *args)
{
	struct help_item *cur;
	int start = 0, end = col->top_id;
	int space_left = sizeof(gHelpbuf) - 480;

	skip_spaces(&args);
	args = one_argument(args, linebuf);
	if (linebuf[0] && !strncmp("range", linebuf, strlen(linebuf))) {
		args = one_argument(args, linebuf);
		if (isdigit(linebuf[0]))
			start = atoi(linebuf);
		args = one_argument(args, linebuf);
		if (isdigit(linebuf[0]))
			end = atoi(linebuf);
	}
	sprintf(gHelpbuf, "Help Topics (%d,%d):\r\n", start, end);
	space_left -= strlen(gHelpbuf);
	for (cur = col->items; cur; cur = cur->next) {
		if (cur->idnum > end)
			break;
		if (cur->idnum < start)
			continue;
		helpitem_show(cur, ch, linebuf, 1);
		strcat(gHelpbuf, linebuf);
		space_left -= strlen(linebuf);
		if (space_left <= 0) {
			sprintf(linebuf,
				"Maximum buffer size reached at item # %d. \r\nUse \"range\" param for higher numbered items.\r\n",
				cur->idnum);
			strcat(gHelpbuf, linebuf);
			break;
		}
	}
	page_string(ch->desc, gHelpbuf);
}

// Create an item. (calls Edit)
bool
help_collection_create_item(struct help_collection *col, struct creature *ch)
{
	struct help_item *n;

    n = make_helpitem();
	n->idnum = ++col->top_id;
	help_collection_push(col, n);
	send_to_char(ch, "Item #%d created.\r\n", n->idnum);
	helpitem_save(n);
	help_collection_save_index();
    helpitem_edit(n, ch);
	SET_BIT(n->flags, HFLAG_MODIFIED);
	slog("%s has help topic # %d.", GET_NAME(ch), n->idnum);
	return true;
}

// Begin editing an item
bool
help_collection_edit_item(struct help_collection *col, struct creature *ch, int idnum)
{
	// See if you can edit it before you do....
	struct help_item *cur;

	if(! Security_isMember( ch, "Help" ) ) {
		send_to_char(ch, "You cannot edit help files.\r\n");
	}
    cur = col->items;
	while (cur && cur->idnum != idnum)
        cur = cur->next;
	if( cur != NULL ) {
		helpitem_edit(cur, ch);
		return true;
	}
	send_to_char(ch, "No such item.\r\n");
	return false;
}

// Clear an item
bool
help_collection_clear_item(struct help_collection *col, struct creature *ch)
{
	if (!GET_OLC_HELP(ch)) {
		send_to_char(ch, "You must be editing an item to clear it.\r\n");
		return false;
	}
	helpitem_clear(GET_OLC_HELP(ch));
	return true;
}

// Save and Item
bool
help_collection_save_item(struct help_collection *col, struct creature *ch)
{
	if (!GET_OLC_HELP(ch)) {
		send_to_char(ch, "You must be editing an item to save it.\r\n");
		return false;
	}
	helpitem_save(GET_OLC_HELP(ch));
	return true;
}

// Find an Item in the index
// This should take an optional "mode" argument to specify groups the
//  returned topic can be part of. e.g. (FindItems(argument,FIND_MODE_OLC))
struct help_item *
help_collection_find_items(struct help_collection *col,
                           char *args,
                           bool find_no_approve,
                           int thegroup,
                           bool searchmode)
{
	struct help_item *cur = NULL;
	struct help_item *list = NULL;
	char stack[256];
	char *b;					// beginning of stack
	char bit[256];				// the current bit of the stack we're lookin through
	int length;
	for (b = args; *b; b++)
		*b = tolower(*b);
	length = strlen(args);
	for (cur = col->items; cur; cur = cur->next) {
		if (IS_SET(cur->flags, HFLAG_UNAPPROVED) && !find_no_approve)
			continue;
		if (thegroup && !helpitem_is_in_group(cur, thegroup))
			continue;
		strcpy(stack, cur->keys);
		b = stack;
		while (*b) {
			b = one_word(b, bit);
			if (!strncmp(bit, args, length)) {
				if (searchmode) {
					cur->next_show = list;
					list = cur;
					break;
				} else {
					return cur;
				}
			}
		}
	}
	return list;
}

// Calls FindItems
// Mode is how to show the item.
// Type: 0==normal help, 1==immhelp, 2==olchelp
void
help_collection_get_topic(struct help_collection *col,
                          struct creature *ch,
                          char *args,
                          int mode,
                          bool show_no_app,
                          int thegroup,
                          bool searchmode)
{

	struct help_item *cur = NULL;

	gHelpbuf[0] = '\0';
	linebuf[0] = '\0';
	int space_left = sizeof(gHelpbuf) - 480;
	if (!args || !*args) {
		send_to_char(ch, "You must enter search criteria.\r\n");
		return;
	}
	cur = help_collection_find_items(col, args, show_no_app, thegroup, searchmode);
	if (cur) {
        FILE *outf;

        outf = fopen("log/help.log", "a");
        if (outf)  {
            fprintf(outf, "%s :: %s [%5d] %s\n",
                    tmp_ctime(time(NULL)),
                    GET_NAME(ch),
                    (ch->in_room) ? ch->in_room->number:0,
                    args);
            fclose(outf);
        }

		send_to_char(ch, "No items were found matching your search criteria.\r\n");
		return;
	}
	// Normal plain old help. One item at a time.
	if (searchmode == false) {
		helpitem_show(cur, ch, gHelpbuf, mode);
	} else {					// Searching for multiple items.
		space_left -= strlen(gHelpbuf);
		for (; cur; cur = cur->next_show) {
			helpitem_show(cur, ch, linebuf, 1);
			strcat(gHelpbuf, linebuf);
			space_left -= strlen(linebuf);
			if (space_left <= 0) {
				sprintf(linebuf, "Maximum buffer size reached at item # %d.",
					cur->idnum);
				strcat(gHelpbuf, linebuf);
				break;
			}
		}
	}
	page_string(ch->desc, gHelpbuf);
	return;
}

// Save everything.
bool
help_collection_SaveAll(struct help_collection *col, struct creature *ch)
{
	struct help_item *cur;
	SaveIndex();
	for (cur = col->items; cur; cur = cur->next) {
		if (IS_SET(cur->flags, HFLAG_MODIFIED))
			helpitem_save(cur);
	}
	send_to_char(ch, "Saved.\r\n");
	slog("%s has saved the help system.", GET_NAME(ch));
	return true;
}

// Save the index
bool
help_collection_SaveIndex(struct help_collection *col)
{
	FILE *outf;
    char *fname;
	struct help_item *cur = NULL;
	int num_items = 0;

	fname = tmp_sprintf("%s/%s", HELP_DIRECTORY, "index");

    outf = fopen(fname, "w");
    if (outf) {
        fprintf(outf, "%d\n", col->top_id);
        for (cur = col->items; cur; cur = cur->next) {
            fprintf(outf, "%d %u %d %u %ld\n%s\n%s\n",
                    cur->idnum,
                    cur->groups,
                    cur->counter,
                    cur->flags,
                    cur->owner,
                    cur->name,
                    cur->keys);
            num_items++;
        }
        fclose(outf);
    } else {
        errlog("Cannot open help index.");
		return false;
    }

	if (num_items == 0) {
		errlog("No records saved to help file index.");
		return false;
	}
	slog("Help: %d items saved to help file index\r\n", num_items);
	return true;
}

// Load the items from the index file
bool
help_collection_load_index(struct help_collection *col)
{
    FILE *inf;
	struct help_item *n;
	int num_items = 0;
    char line[1024];
    int i;

    inf = fopen(tmp_sprintf("%s/%s", HELP_DIRECTORY, "index"), "r");
    if (!inf) {
        errlog("Cannot open help index.");
        return false;
    }

    fscanf(inf, "%d\n", &col->top_id);
    for (i = 0; i < col->top_id; i++) {
        CREATE(n, struct help_item, 1);
        fscanf(inf, "%d %d %d %d %ld\n",
               &n->idnum,
               &n->groups,
               &n->counter,
               &n->flags,
               &n->owner);
        fgets(line, sizeof(line), inf);
        n->name = strdup(line);
        fgets(line, sizeof(line), inf);
        n->keys = strdup(line);
        REMOVE_BIT(n->flags, HFLAG_MODIFIED);
        help_collection_push(col, n);
        num_items++;
    }
    fclose(inf);
    if (num_items == 0) {
        slog("WARNING: No records read from help file index.");
        return false;
    }

    slog("%d items read from help file index", num_items);
    return true;
}

// Funnels outside commands into struct help_item functions
// (that should be protected or something... shrug.)
bool
help_collection_set(struct help_collection *col, struct creature *ch, char *argument)
{
	char arg1[256];

	if (!GET_OLC_HELP(ch)) {
		send_to_char(ch, "You have to be editing an item to set it.\r\n");
		return false;
	}
	if (!argument || !*argument) {
		send_to_char(ch,
			"hcollect set <groups[+/-]|flags[+/-]|name|keywords|description> [args]\r\n");
		return false;
	}
	argument = one_argument(argument, arg1);
	if (!strncmp(arg1, "groups", strlen(arg1))) {
		helpitem_setgroups(GET_OLC_HELP(ch), argument);
		return true;
	} else if (!strncmp(arg1, "flags", strlen(arg1))) {
        helpitem_setflags(GET_OLC_HELP(ch), argument);
		return true;
	} else if (!strncmp(arg1, "name", strlen(arg1))) {
        free(GET_OLC_HELP(ch)->name);
		GET_OLC_HELP(ch)->name = strdup(argument);
		return true;
	} else if (!strncmp(arg1, "keywords", strlen(arg1))) {
        free(GET_OLC_HELP(ch)->keys);
		GET_OLC_HELP(ch)->keys = strdup(argument);
		return true;
	} else if (!strncmp(arg1, "description", strlen(arg1))) {
        helpitem_edittext(GET_OLC_HELP(ch));
		return true;
	}
	return false;
}

// Trims off the text of all the help items that people have looked at.
// (each text is approx 65k. We only want the ones in ram that we need.)
void
help_collection_sync(struct help_collection *col)
{
	struct help_item *n;

	for (n = col->items; n; n = n->next) {
		if (n->text && !IS_SET(n->flags, HFLAG_MODIFIED) && !n->editor) {
			free(n->text);
			n->text = NULL;
		}
	}
}

// Approve an item
void
help_collection_ApproveItem(struct help_collection *col, struct creature *ch, char *argument)
{
	char arg1[256];
	int idnum = 0;
	struct help_item *cur;
	skip_spaces(&argument);
	argument = one_argument(argument, arg1);
	if (isdigit(arg1[0]))
		idnum = atoi(arg1);
	if (idnum > col->top_id || idnum < 1) {
		send_to_char(ch, "Approve which item?\r\n");
		return;
	}
    cur = col->items;
	while (cur && cur->idnum != idnum)
        cur = cur->next;
	if (cur) {
		REMOVE_BIT(cur->flags, HFLAG_UNAPPROVED);
		SET_BIT(cur->flags, HFLAG_MODIFIED);
		send_to_char(ch, "Item #%d approved.\r\n", cur->idnum);
	} else {
		send_to_char(ch, "Unable to find item.\r\n");
	}
	return;
}

// Unapprove an item
void
help_collection_UnApproveItem(struct help_collection *col, struct creature *ch, char *argument)
{
	char arg1[256];
	int idnum = 0;
	struct help_item *cur;
	skip_spaces(&argument);
	argument = one_argument(argument, arg1);
	if (isdigit(arg1[0]))
		idnum = atoi(arg1);
	if (idnum > col->top_id || idnum < 1) {
		send_to_char(ch, "UnApprove which item?\r\n");
		return;
	}
    cur = col->items;
	while (cur && cur->idnum != idnum)
        cur = cur->next;
	if (cur) {
		SET_BIT(cur->flags, HFLAG_MODIFIED);
		SET_BIT(cur->flags, HFLAG_UNAPPROVED);
		send_to_char(ch, "Item #%d unapproved.\r\n", cur->idnum);
	} else {
		send_to_char(ch, "Unable to find item.\r\n");
	}
	return;
}

// Give some stat info on the Help System
void
help_collection_Show(struct help_collection *col, struct creature *ch)
{
	int num_items = 0;
	int num_modified = 0;
	int num_unapproved = 0;
	int num_editing = 0;
	int num_no_group = 0;
	struct help_item *cur;

	for (cur = col->items; cur; cur = cur->next) {
		num_items++;
		if (cur->flags != 0) {
			if (IS_SET(cur->flags, HFLAG_UNAPPROVED))
				num_unapproved++;
			if (IS_SET(cur->flags, HFLAG_MODIFIED))
				num_modified++;
		}
		if (cur->editor)
			num_editing++;
		if (cur->groups == 0)
			num_no_group++;
	}
	send_to_char(ch, "%sTopics [%s%d%s] %sUnapproved [%s%d%s] %sModified [%s%d%s] "
		"%sEditing [%s%d%s] %sGroupless [%s%d%s]%s\r\n",
		CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), num_items, CCCYN(ch, C_NRM),
		CCYEL(ch, C_NRM), CCNRM(ch, C_NRM), num_unapproved, CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), num_modified, CCGRN(ch, C_NRM),
		CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), num_editing, CCCYN(ch, C_NRM),
		CCRED(ch, C_NRM), CCNRM(ch, C_NRM), num_no_group, CCRED(ch, C_NRM),
		CCNRM(ch, C_NRM));
}

// Blah blah print out the hcollect commands.
void
do_hcollect_cmds(struct creature *ch)
{
    int i;

	strcpy(gHelpbuf, "hcollect commands:\r\n");
	for (i = 0; hc_cmds[i].keyword; i++) {
		if (GET_LEVEL(ch) < hc_cmds[i].level)
			continue;
		sprintf(gHelpbuf, "%s  %-15s %s\r\n", gHelpbuf,
			hc_cmds[i].keyword, hc_cmds[i].usage);
	}
	page_string(ch->desc, gHelpbuf);
}

struct help_item *
help_collection_find_item_by_id(struct help_collection *col, int id)
{
	struct help_item *cur;
    cur = col->items;
	while (cur && cur->idnum != id)
        cur = cur->next;
	return cur;
}

// The "immhelp" command
ACMD(do_immhelp)
{
	struct help_item *cur = NULL;
	skip_spaces(&argument);

	// Take care of all the special cases.
	// Default help file
	if (!argument || !*argument) {
		cur = help_collection_find_item_by_id(help, 699);
	}
	// If we have a special case, do it, otherwise try to get it normally.
	if (cur) {
		helpitem_show(cur, ch, gHelpbuf, 2);
		page_string(ch->desc, gHelpbuf);
	} else {
		help_collection_get_topic(help, ch, argument, 2, false, HGROUP_IMMHELP, false);
	}
}

// The standard "help" command
ACMD(do_hcollect_help)
{
	struct help_item *cur = NULL;
	skip_spaces(&argument);

	// Take care of all the special cases.
	if (subcmd == SCMD_CITIES) {
		cur = help_collection_find_item_by_id(help, 62);
	} else if (subcmd == SCMD_MODRIAN) {
        cur = help_collection_find_item_by_id(help, 196);
	} else if (subcmd == SCMD_SKILLS) {
		send_to_char(ch, "Type 'Help %s' to see the skills available to your char_class.\r\n",
                     class_names[(int)GET_CLASS(ch)]);
	} else if (subcmd == SCMD_POLICIES) {
        cur = help_collection_find_item_by_id(help, 667);
	} else if (subcmd == SCMD_HANDBOOK) {
        cur = help_collection_find_item_by_id(help, 999);
		// Default help file
	} else if (!argument || !*argument) {
        cur = help_collection_find_item_by_id(help, 666);
	}
	// If we have a special case, do it, otherwise try to get it normally.
	if (cur) {
		helpitem_show(cur, ch, gHelpbuf, 2);
		page_string(ch->desc, gHelpbuf);
	} else {
        help_collection_get_topic(help, ch, argument, 2, false, 0, false);
	}
}

// 'qcontrol help'
void
do_qcontrol_help( struct creature *ch, char *argument )
{
	struct help_item *cur = NULL;
	skip_spaces(&argument);

	// Take care of all the special cases.
	if (!argument || !*argument) {
		cur = help_collection_find_item_by_id(help, 900);
	}
	// If we have a special case, do it, otherwise try to get it normally.
	if (cur) {
		helpitem_show(cur, ch, gHelpbuf, 2);
		page_string(ch->desc, gHelpbuf);
	} else {
        help_collection_get_topic(help, ch, argument, 2, false, HGROUP_QCONTROL, false);
	}
}

// The "olchelp" command
ACMD(do_olchelp)
{
	struct help_item *cur = NULL;
	skip_spaces(&argument);

	// Take care of all the special cases.
	if (!argument || !*argument) {
        cur = help_collection_find_item_by_id(help, 700);
	}
	// If we have a special case, do it, otherwise try to get it normally.
	if (cur) {
        helpitem_show(cur, ch, gHelpbuf, 2);
		page_string(ch->desc, gHelpbuf);
	} else {
        help_collection_get_topic(help, ch, argument, 2, false, HGROUP_OLC, false);
	}
}

// Main command parser for hcollect.
ACMD(do_help_collection_command)
{
	int com;
	int id;
	struct help_item *cur;
	argument = one_argument(argument, linebuf);
	skip_spaces(&argument);
	if (!*linebuf) {
		do_hcollect_cmds(ch);
		return;
	}
	for (com = 0;; com++) {
		if (!hc_cmds[com].keyword) {
			send_to_char(ch, "Unknown hcollect command, '%s'.\r\n", linebuf);
			return;
		}
		if (is_abbrev(linebuf, hc_cmds[com].keyword))
			break;
	}
	if (hc_cmds[com].level > GET_LEVEL(ch)) {
		send_to_char(ch, "You are not godly enough to do this!\r\n");
		return;
	}
	switch (com) {
	case 0:					// Approve
		help_collection_approveitem(help, ch, argument);
		break;
	case 1:					// Create
		help_collection_createitem(help, ch);
		break;
	case 2:					// Edit
		argument = one_argument(argument, linebuf);
		if (isdigit(*linebuf)) {
			id = atoi(linebuf);
			help_collection_edit_item(help, ch, id);
		} else if (!strncmp("exit", linebuf, strlen(linebuf))) {
			if (!GET_OLC_HELP(ch)) {
				send_to_char(ch, "You're not editing a help topic.\r\n");
			} else {
				GET_OLC_HELP(ch)->editor = NULL;
				GET_OLC_HELP(ch) = NULL;
				send_to_char(ch, "Help topic editor exited.\r\n");
			}
		} else {
			send_to_char(ch, "hcollect edit <#|exit>\r\n");
		}
		break;
	case 3:					// Info
		help_collection_show(help, ch);
		break;
	case 4:					// List
        help_collection_list(help, ch, argument);
		break;
	case 5:					// Save
        help_collection_saveall(help, ch);
		break;
	case 6:					// Set
        help_collection_set(help, ch, argument);
		break;
	case 7:					// Stat
		argument = one_argument(argument, linebuf);
		if (*linebuf && isdigit(linebuf[0])) {
			id = atoi(linebuf);
			if (id < 0 || id > help->top_id) {
				send_to_char(ch, "There is no such item #.\r\n");
				break;
			}
            cur = help->items;
			while (cur && cur->idnum != id)
                cur = cur->next;
			if (cur) {
                helpitem_show(cur, ch, gHelpbuf, 3);
				page_string(ch->desc, gHelpbuf);
				break;
			} else {
				send_to_char(ch, "There is no item: %d.\r\n", id);
				break;
			}
		}
		if (GET_OLC_HELP(ch)) {
            helpitem_show(GET_OLC_HELP(ch), ch, gHelpbuf, 3);
			page_string(ch->desc, gHelpbuf);
		} else {
			send_to_char(ch, "Stat what item?\r\n");
		}
		break;
	case 8:					// Sync
        help_collection_sync(help);
		send_to_char(ch, "Okay.\r\n");
		break;
	case 9:
        // Search (mode==3 is "stat" rather than "show") show_no_app "true"
		// searchmode=true is find all items matching.
        help_collection_get_topic(help, ch, argument, 3, true, -1, true);
		break;
	case 10:					// UnApprove
        help_collection_unapprove_item(help, ch, argument);
		break;
	case 11:					// Immhelp
        help_collection_get_topic(help, ch, argument, 2, false, HGROUP_IMMHELP, false);
		break;
	case 12:					// olchelp
        help_collection_get_topic(help, ch, argument, 2, false, HGROUP_OLC, false);
		break;
	case 13: 				   // QControl Help
        help_collection_get_topic(help, ch, argument, 2, false, HGROUP_QCONTROL, false);
		break;
	default:
		do_hcollect_cmds(ch);
		break;
	}
}
