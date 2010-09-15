#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include "structs.h"
#include "creature.h"
#include "utils.h"
#include "help.h"
#include "interpreter.h"
#include "db.h"
#include "editor.h"
#include "comm.h"
#include "screen.h"
#include "handler.h"

extern struct help_collection *Help;
extern char gHelpbuf[];
char *one_word(char *argument, char *first_arg);
extern const char *help_group_names[];
extern const char *help_group_bits[];
extern const char *help_bit_descs[];
extern const char *help_bits[];

// Sets the flags bitvector for things like !approved and
// IMM+ and changed and um... other flaggy stuff.
void
help_item_setflags(struct help_item *item, char *argument)
{
	int state, cur_flags = 0, tmp_flags = 0, flag = 0, old_flags = 0;
	char arg1[MAX_INPUT_LENGTH];
	argument = one_argument(argument, arg1);
	skip_spaces(&argument);
	if (!*argument) {
		send_to_char(item->editor, "Invalid flags. \r\n");
		return;
	}

	if (*arg1 == '+') {
		state = 1;
	} else if (*arg1 == '-') {
		state = 2;
	} else {
		send_to_char(item->editor, "Invalid flags.\r\n");
		return;
	}

	argument = one_argument(argument, arg1);
	old_flags = cur_flags = item->flags;
	while (*arg1) {
		if ((flag = search_block(arg1, help_bits, false)) == -1) {
			send_to_char(item->editor, "Invalid flag %s, skipping...\r\n", arg1);
		} else {
			tmp_flags = tmp_flags | (1 << flag);
		}
		argument = one_argument(argument, arg1);
	}
	REMOVE_BIT(tmp_flags, HFLAG_UNAPPROVED);
	if (state == 1) {
		item->flags |= tmp_flags;
	} else {
        item->flags &= ~tmp_flags;
	}

	tmp_flags = old_flags ^ item->flags;

	if (tmp_flags == 0) {
		send_to_char(item->editor, "Flags for help item %d not altered.\r\n",
                     item->idnum);
	} else {
		SET_BIT(item->flags, HFLAG_MODIFIED);
		send_to_char(item->editor, "[%s] flags %s for help item %d.\r\n", tmp_printbits(tmp_flags, help_bits),
			state == 1 ? "added" : "removed", item->idnum);
	}
}

// Loads in the text for a particular help entry.
bool
help_item_load_text(struct help_item *item)
{
    FILE *inf;
	char *fname;
	int idnum, textlen;
    char buf[512];

	if (item->text)
		return true;

	fname = tmp_sprintf("%s/%04d.topic", HELP_DIRECTORY, item->idnum);

	if (access(fname, F_OK) < 0)  {
        // no file found. Likely just a new entry
		return true;
	}

    inf = fopen(fname, "r");
    if (inf) {
        fscanf(inf, "%d %d\n", &idnum, &textlen);
        if (textlen > MAX_HELP_TEXT_LENGTH - 1)
            textlen = MAX_HELP_TEXT_LENGTH - 1;
	    fgets(buf, sizeof(buf), inf);
        CREATE(item->text, char, textlen + 1);
        fread(item->text, 1, textlen, inf);

        return true;
    }

    errlog("Unable to open help file to load text (%s): %s",
           fname, strerror(errno));
    return false;
}

// Crank up the text item->editor and lets hit it.
void
help_item_edittext(struct help_item *item)
{

	help_item_load_text(item);
	SET_BIT(item->flags, HFLAG_MODIFIED);
	start_editing_text(item->editor->desc, &item->text, MAX_HELP_TEXT_LENGTH);
	SET_BIT(PLR_FLAGS(item->editor), PLR_OLC);

	act("$n begins to edit a help file.\r\n", true, item->editor, 0, 0, TO_ROOM);
}

// Sets the groups bitvector much like the
// flags in quests.
void
help_item_setgroups(struct help_item *item, char *argument)
{
	int state, cur_groups = 0, tmp_groups = 0, flag = 0, old_groups = 0;
	char arg1[MAX_INPUT_LENGTH];
	argument = one_argument(argument, arg1);
	skip_spaces(&argument);
	if (!*argument) {
		send_to_char(item->editor, "Invalid group. \r\n");
		return;
	}

	if (*arg1 == '+') {
		state = 1;
	} else if (*arg1 == '-') {
		state = 2;
	} else {
		send_to_char(item->editor, "Invalid Groups.\r\n");
		return;
	}

	argument = one_argument(argument, arg1);
	old_groups = cur_groups = item->groups;
	while (*arg1) {
		if ((flag = search_block(arg1, help_group_names, false)) == -1) {
			send_to_char(item->editor, "Invalid group: %s, skipping...\r\n", arg1);
		} else {
			tmp_groups = tmp_groups | (1 << flag);
		}
		argument = one_argument(argument, arg1);
	}
	if (state == 1) {
		item->groups = cur_groups | tmp_groups;
	} else {
        item->groups = cur_groups & ~tmp_groups;
	}
	tmp_groups = old_groups ^ cur_groups;

	if (tmp_groups == 0) {
		send_to_char(item->editor, "Groups for help item %d not altered.\r\n",
                     item->idnum);
	} else {
		send_to_char(item->editor, "[%s] groups %s for help item %d.\r\n",
                     tmp_printbits(tmp_groups, help_group_bits),
                     state == 1 ? "added" : "removed", item->idnum);
	}
	REMOVE_BIT(item->groups, HGROUP_HELP_EDIT);
	SET_BIT(item->flags, HFLAG_MODIFIED);
}

bool
help_item_in_group(struct help_item *item, int thegroup)
{
	if (IS_SET(item->groups, thegroup))
		return true;
	return false;
}

// Don't call me Roger.
void
help_item_setname(struct help_item *item, char *argument)
{
	skip_spaces(&argument);
    free(item->name);
    item->name = strdup(argument);
	SET_BIT(item->flags, HFLAG_MODIFIED);
	if (item->editor)
		send_to_char(item->editor, "Name set!\r\n");
}

// Set the...um. keywords and stuff.
void
help_item_setkeywords(struct help_item *item, char *argument)
{
	skip_spaces(&argument);
	free(item->keys);
    item->keys = strdup(argument);
	SET_BIT(item->flags, HFLAG_MODIFIED);
	if (item->editor)
		send_to_char(item->editor, "Keywords set!\r\n");
}

void
help_item_clear(struct help_item *item)
{
	item->counter = 0;
	item->flags = (HFLAG_UNAPPROVED | HFLAG_MODIFIED);
	item->groups = 0;

    free(item->name);
    free(item->keys);
    free(item->text);

	item->name = strdup("A New Help Entry");
	item->keys = strdup("new help entry");
    item->text = NULL;
}

struct help_item *
make_help_item(void)
{
    struct help_item *item;

    CREATE(item, struct help_item, 1);
	help_item_clear(item);

    return item;
}

void
free_help_item(struct help_item *item)
{
    free(item->name);
    free(item->keys);
    free(item->text);
}

// Begin editing an item much like olc oedit.
// Sets yer currently editable item to this one.
bool
help_item_edit(struct help_item *item, struct creature *ch)
{
	if (item->editor) {
		if (item->editor != ch) {
			send_to_char(ch, "%s is already editing that item. Tough!\r\n",
				GET_NAME(item->editor));
		} else {
			send_to_char(ch,
				"I don't see how editing it _again_ will help any.\r\n");
		}
		return false;
	}
	if (GET_OLC_HELP(ch))
		GET_OLC_HELP(ch)->editor = NULL;
	GET_OLC_HELP(ch) = item;
	item->editor = ch;
	send_to_char(ch, "You are now editing help item #%d.\r\n", item->idnum);
	SET_BIT(item->flags, HFLAG_MODIFIED);
	return true;
}

// Saves the current Help_Item. Does NOT alter the index.
// You have to save that too.
// (SaveAll does everything)
bool
help_item_save(struct help_item *item)
{
	char *fname;
    FILE *outf;

	fname = tmp_sprintf("%s/%04d.topic", HELP_DIRECTORY, item->idnum);
	// If we don't have the text to edit, get from the file.
	if (!item->text)
		help_item_load_text(item);

    outf = fopen(fname, "w");
    if (outf) {
        fprintf(outf, "%d %zd\n%s\n%s",
                item->idnum,
                (item->text) ? strlen(item->text):0,
                item->name,
                (item->text && *item->text) ? item->text:"");
        fclose(outf);
        REMOVE_BIT(item->flags, HFLAG_MODIFIED);
        return true;
    }

    if (item->editor)
		send_to_char(item->editor, "Error, could not open help file for write.\r\n");

    return false;
}


// Show the entry.
// buffer is output buffer.
void
help_item_show(struct help_item *item, struct creature *ch, char *buffer, int mode)
{
	char bitbuf[256];
	char groupbuf[256];
	switch (mode) {
	case 0:					// 0 == One Line Listing.
		sprintf(buffer, "Name: %s\r\n    (%s)\r\n", item->name, item->keys);
		strcpy(buffer, "");
		break;
	case 1:					// 1 == One Line Stat
		sprintbit(item->flags, help_bit_descs, bitbuf);
		sprintbit(item->groups, help_group_bits, groupbuf);
		strcpy(buffer, "");
		sprintf(buffer, "%s%3d. %s%-25s %sGroups: %s%-20s %sFlags:%s %s\r\n",
                CCCYN(ch, C_NRM), item->idnum, CCYEL(ch, C_NRM),
                item->name, CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                groupbuf, CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), bitbuf);
		break;
	case 2:					// 2 == Entire Entry
		if (!item->text)
			help_item_load_text(item);
		strcpy(buffer, "");
		sprintf(buffer, "\r\n%s%s%s\r\n%s\r\n",
			CCCYN(ch, C_NRM), item->name, CCNRM(ch, C_NRM), item->text);
		item->counter++;
		break;
	case 3:					// 3 == Entire Entry Stat
		if (!item->text)
			help_item_load_text(item);
		sprintbit(item->flags, help_bit_descs, bitbuf);
		sprintbit(item->groups, help_group_bits, groupbuf);
		strcpy(buffer, "");
		sprintf(buffer,
			"\r\n%s%d. %s%-25s %sGroups: %s%-20s %sFlags:%s %s \r\n        %s"
			"Keywords: [ %s%s%s ]\r\n%s%s\r\n",
			CCCYN(ch, C_NRM), item->idnum, CCYEL(ch, C_NRM),
			item->name, CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
			groupbuf, CCCYN(ch, C_NRM),
			CCNRM(ch, C_NRM), bitbuf, CCCYN(ch, C_NRM),
			CCNRM(ch, C_NRM), item->keys, CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), item->text);
		break;
	default:
		break;
	}
}
