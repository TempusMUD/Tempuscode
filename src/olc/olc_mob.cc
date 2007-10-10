//
// File: olc_mob.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//


#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include "structs.h"
#include "creature_list.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "char_class.h"
#include "handler.h"
#include "db.h"
#include "editor.h"
#include "security.h"
#include "olc.h"
#include "screen.h"
#include "flow_room.h"
#include "specs.h"
#include "language.h"
#include "mobile_map.h"

extern struct zone_data *zone_table;
extern struct descriptor_data *descriptor_list;
extern struct player_special_data dummy_mob;
extern int top_of_zone_table;
extern int olc_lock;
extern int top_of_mobt;
extern int *obj_index;
extern int *mob_index;
extern int *shp_index;
extern const char *language_names[];

long asciiflag_conv(char *buf);

char *one_argument_no_lower(char *argument, char *first_arg);
int search_block_no_lower(char *arg, char **list, bool exact);
int fill_word_no_lower(char *argument);
void num2str(char *str, int num);
void set_physical_attribs(struct Creature *ch);
void do_stat_character(struct Creature *ch, struct Creature *k);
struct extra_descr_data *locate_exdesc(char *word,
	struct extra_descr_data *list, int exact = 0);
void set_move_buffer(struct Creature *ch);
char *find_exdesc(char *word, struct extra_descr_data *list, int find_exact = 0);

// locals
static char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

const char *olc_mset_keys[] = {
	"alias",
	"name",
	"ldesc",
	"desc",
	"flags",
	"flags2",		   /** 5 **/
	"aff",
	"aff2",
	"aff3",
	"align",
	"str",			  /** 10 **/
	"int",
	"wis",
	"dex",
	"con",
	"cha",			  /** 15 **/
	"level",
	"hitp_mod",
	"hitd_num",
	"hitd_size",
	"mana",		   /** 20 **/
	"move",
	"baredam",
	"baredsize",
	"gold",
	"exp",		 /** 25 **/
	"attacktype",
	"position",
	"sex",
	"remort_class",
	"cash",		   /** 30 **/
	"hitroll",
	"damroll",
	"ac",
	"class",
	"race",			/** 35 **/
	"dpos",
	"stradd",
	"height",
	"weight",
	"reply",		 /** 40 **/
	"special",
	"morale",
	"str_app",
	"move_buf",
	"lair",			/** 45 **/
	"leader",
	"specparam",
    "generation",
	"loadparam",
    "knownlanguage",	/** 50 **/
    "curlanguage",
	"prog",
	"\n"
};

struct Creature *
do_create_mob(struct Creature *ch, int vnum)
{
	struct Creature *mob = NULL, *new_mob = NULL;
	struct zone_data *zone = NULL;
	int j;

	if ((mob = real_mobile_proto(vnum))) {
		send_to_char(ch, "ERROR: Mobile already exists.\r\n");
		return NULL;
	}

	for (zone = zone_table; zone; zone = zone->next)
		if (vnum >= zone->number * 100 && vnum <= zone->top)
			break;

	if (!zone) {
		send_to_char(ch, "ERROR: A zone must be defined for the mobile first.\r\n");
		return NULL;
	}

	if (!CAN_EDIT_ZONE(ch, zone)) {
		send_to_char(ch, "Try creating mobiles in your own zone, luser.\r\n");
		mudlog(GET_INVIS_LVL(ch), BRF, true,
			"OLC: %s failed attempt to CREATE mob %d.",
			GET_NAME(ch), vnum);
		return NULL;
	}

	if (!OLC_EDIT_OK(ch, zone, ZONE_MOBS_APPROVED)) {
		send_to_char(ch, "Mobile OLC is not approved for this zone.\r\n");
		return NULL;
	}
	MobileMap::iterator mit = mobilePrototypes.begin();
	MobileMap::iterator nit;
	for (; mit != mobilePrototypes.end(); ++mit) {
		mob = mit->second;
		nit = mit;
		++nit;
		if (vnum > mob->mob_specials.shared->vnum &&
			(nit != mobilePrototypes.end() ||
				vnum < (nit->second)->mob_specials.shared->vnum))
			break;

	}
	new_mob = new Creature(false);

	CREATE(new_mob->mob_specials.shared, struct mob_shared_data, 1);
	new_mob->mob_specials.shared->vnum = vnum;
	new_mob->mob_specials.shared->number = 0;
	new_mob->mob_specials.shared->func = NULL;
	new_mob->mob_specials.shared->proto = new_mob;

	new_mob->player_specials = &dummy_mob;

	/***** String data *** */
	new_mob->player.name = str_dup("Fresh Blank Mobile");
	new_mob->player.short_descr = str_dup("A Fresh Blank Mobile");
	new_mob->player.long_descr =
		str_dup("A Fresh Blank Mobile is here waiting to be violated.");
	new_mob->player.description = NULL;
	new_mob->player.title = NULL;

	/* *** Numeric data *** */
	MOB_FLAGS(new_mob) = 0;
	MOB2_FLAGS(new_mob) = 0;
	SET_BIT(MOB_FLAGS(new_mob), MOB_ISNPC);
	AFF_FLAGS(new_mob) = 0;
	AFF2_FLAGS(new_mob) = 0;
	AFF3_FLAGS(new_mob) = 0;
	GET_ALIGNMENT(new_mob) = 0;
    GET_REAL_GEN(new_mob) = 0;
    GET_TONGUE(new_mob) = TONGUE_COMMON;

	new_mob->real_abils.str = 11;
	new_mob->real_abils.intel = 11;
	new_mob->real_abils.wis = 11;
	new_mob->real_abils.dex = 11;
	new_mob->real_abils.con = 11;
	new_mob->real_abils.cha = 11;

	GET_LEVEL(new_mob) = 1;
	new_mob->points.hitroll = 0;
	new_mob->points.armor = 0;

	/* max hit = 0 is a flag that H, M, V is xdy+z */
	new_mob->points.max_hit = 0;
	new_mob->points.hit = 10;
	new_mob->points.mana = 10;
	new_mob->points.move = 50;

	new_mob->points.max_mana = 10;
	new_mob->points.max_move = 50;

	new_mob->mob_specials.shared->damnodice = 5;
	new_mob->mob_specials.shared->damsizedice = 2;
	new_mob->points.damroll = 0;

	new_mob->player.char_class = CLASS_NORMAL;
	new_mob->player.race = RACE_MOBILE;
    
	GET_GOLD(new_mob) = 0;
	GET_EXP(new_mob) = 100;
	GET_MORALE(new_mob) = 100;
	GET_MOB_LAIR(new_mob) = -1;
	GET_MOB_LEADER(new_mob) = -1;

	new_mob->mob_specials.shared->attack_type = 0;

	new_mob->char_specials.position = POS_STANDING;
	new_mob->mob_specials.shared->default_pos = POS_STANDING;
	new_mob->player.sex = SEX_NEUTRAL;

	new_mob->player.weight = 200;
	new_mob->player.height = 198;

	new_mob->player.remort_char_class = -1;
	set_physical_attribs(new_mob);

	for (j = 0; j < 3; j++)
		GET_COND(new_mob, j) = -1;

	for (j = 0; j < 5; j++)
		GET_SAVE(new_mob, j) = 0;

	new_mob->aff_abils = new_mob->real_abils;

	for (j = 0; j < NUM_WEARS; j++)
		new_mob->equipment[j] = NULL;

	new_mob->desc = NULL;
	new_mob->in_room = NULL;

	mobilePrototypes.add(new_mob);
	top_of_mobt++;

	return (new_mob);
}


void
do_mob_medit(struct Creature *ch, char *argument)
{
	struct Creature *mobile = NULL, *tmp_mob = NULL;
	struct zone_data *zone = NULL;
	struct descriptor_data *d = NULL;
	int j;

	mobile = GET_OLC_MOB(ch);

	if (!*argument) {
		if (!mobile)
			send_to_char(ch, "You are not currently editing a mobile.\r\n");
		else {
			send_to_char(ch, "Current olc mobile: [%5d] %s\r\n",
				mobile->mob_specials.shared->vnum, mobile->player.short_descr);
		}
		return;
	}
	if (!is_number(argument)) {
		if (is_abbrev(argument, "exit")) {
			send_to_char(ch, "Exiting mobile editor.\r\n");
			GET_OLC_MOB(ch) = NULL;
			return;
		}
		send_to_char(ch, "The argument must be a number.\r\n");
		return;
	} else {
		j = atoi(argument);
		if ((tmp_mob = real_mobile_proto(j)) == NULL)
			send_to_char(ch, "There is no such mobile.\r\n");
		else {
			for (zone = zone_table; zone; zone = zone->next)
				if (j <= zone->top)
					break;
			if (!zone) {
				send_to_char(ch, "That mobile does not belong to any zone!!\r\n");
				errlog("mobile not in any zone.");
				return;
			}

			if (!CAN_EDIT_ZONE(ch, zone)) {
				send_to_char(ch, 
					"You do not have permission to edit those mobiles.\r\n");
				return;
			}

			if (!OLC_EDIT_OK(ch, zone, ZONE_MOBS_APPROVED)) {
				send_to_char(ch, "Mobile OLC is not approved for this zone.\r\n");
				return;
			}

			for (d = descriptor_list; d; d = d->next) {
				if (d->creature && GET_OLC_MOB(d->creature) == tmp_mob) {
					act("$N is already editing that mobile.", FALSE, ch, 0,
						d->creature, TO_CHAR);
					return;
				}
			}

			GET_OLC_MOB(ch) = tmp_mob;
			send_to_char(ch, "Now editing mobile [%d] %s%s%s\r\n",
				tmp_mob->mob_specials.shared->vnum,
				CCGRN(ch, C_NRM), tmp_mob->player.short_descr,
				CCNRM(ch, C_NRM));
		}
	}
}

void
do_mob_mstat(struct Creature *ch)
{
	struct Creature *mob = NULL;

	mob = GET_OLC_MOB(ch);

	if (!mob)
		send_to_char(ch, "You are not currently editing a mobile.\r\n");
	else
		do_stat_character(ch, mob);
}

#define mob_p GET_OLC_MOB(ch)
#define OLC_REPLY_USAGE "olc mset reply  <create | remove | edit | addkey>" \
"<keywords> [new keywords]\r\n"

void
do_mob_mset(struct Creature *ch, char *argument)
{
	struct Creature *tmp_mob;
	struct zone_data *zone;
	int i, mset_command, tmp_flags, flag, cur_flags, state;

	if (!mob_p) {
		send_to_char(ch, "You are not currently editing a mobile.\r\n");
		return;
	}

	if (!*argument) {
		strcpy(buf, "Valid mset commands:\r\n");
		strcat(buf, CCYEL(ch, C_NRM));
		i = 0;
		while (*olc_mset_keys[i] != '\n') {
			strcat(buf, olc_mset_keys[i]);
			strcat(buf, "\r\n");
			i++;
		}
		strcat(buf, CCNRM(ch, C_NRM));
		page_string(ch->desc, buf);
		return;
	}

	half_chop(argument, arg1, arg2);
	skip_spaces(&argument);

	if ((mset_command = search_block(arg1, olc_mset_keys, FALSE)) < 0) {
		send_to_char(ch, "Invalid mset command '%s'.\r\n", arg1);
		return;
	}
	// Check for desc and specparam setting, both of which use tedii
	if (mset_command != 3 && mset_command != 47 
	&& mset_command != 49 && mset_command != 52 && !*arg2) {
		send_to_char(ch, "Set %s to what??\r\n", olc_mset_keys[mset_command]);
		return;
	}
#ifdef DMALLOC
	dmalloc_verify(0);
#endif

	switch (mset_command) {
	case 0:{				 /** Alias **/
			if (strchr(arg2, '~')) {
				send_to_char(ch, "Tildes don't belong in aliases.\r\n");
			} else {
				if (mob_p->player.name)
					free(mob_p->player.name);
				mob_p->player.name = strdup(arg2);
				UPDATE_MOBLIST(mob_p, tmp_mob,->player.name);
				send_to_char(ch, "Mobile aliases set.\r\n");
			}
			break;
		}
	case 1:{				   /** Name **/
			if (mob_p->player.short_descr)
				free(mob_p->player.short_descr);
			mob_p->player.short_descr = strdup(arg2);
			UPDATE_MOBLIST_NAMES(mob_p, tmp_mob,->player.short_descr);
			send_to_char(ch, "Mobile name set.\r\n");
			break;
		}
	case 2:{				   /** ldesc **/
			if (mob_p->player.long_descr)
				free(mob_p->player.long_descr);
			if (arg2[0] == '~')
				mob_p->player.long_descr = NULL;
			else {
				strcpy(buf, arg2);
				mob_p->player.long_descr = strdup(buf);
			}
			send_to_char(ch, "Mobile long description set.\r\n");
			UPDATE_MOBLIST(mob_p, tmp_mob,->player.long_descr);
			break;
		}
	case 3:{				   /** desc **/
			if (mob_p->player.description == NULL) {
				act("$n starts to write a mobile description.", TRUE, ch, 0, 0,
					TO_ROOM);
			} else {
				act("$n begins to edit a mobile description.", TRUE, ch, 0, 0,
					TO_ROOM);
			}
			start_editing_text(ch->desc, &mob_p->player.description);
			SET_BIT(PLR_FLAGS(ch), PLR_OLC);
			CreatureList::iterator cit = characterList.begin();
			for (; cit != characterList.end(); ++cit) {
				if (GET_MOB_VNUM((*cit)) == GET_MOB_VNUM(mob_p)) {
					(*cit)->player.description = NULL;
					break;
				}
			}
			break;
		}
	case 4:{				  /** flags **/
			tmp_flags = 0;
			argument = one_argument(arg2, arg1);

			for (zone = zone_table; zone; zone = zone->next)
				if (mob_p->mob_specials.shared->vnum >= zone->number * 100 &&
					mob_p->mob_specials.shared->vnum <= zone->top)
					break;

			if (!zone) {
				errlog(" Error!  mobile not in zone.");
				send_to_char(ch, "ERROR\r\n");
				return;
			}

			if (*arg1 == '+')
				state = 1;
			else if (*arg1 == '-')
				state = 2;
			else {
				send_to_char(ch, 
					"Usage: olc mset flags [+/-] [FLAG, FLAG, ...]\r\n");
				return;
			}

			argument = one_argument(argument, arg1);

			cur_flags = MOB_FLAGS(mob_p);

			while (*arg1) {
				if ((flag = search_block(arg1, action_bits_desc, FALSE)) == -1) {
					send_to_char(ch, "Invalid flag %s, skipping...\r\n", arg1);
				} else if ((1 << flag) == MOB_SPEC && !GET_MOB_SPEC(mob_p) &&
					state == 1)
					send_to_char(ch, 
						"Can't set SPEC bit until special is assigned.\r\n");
				else
					tmp_flags = tmp_flags | (1 << flag);

				argument = one_argument(argument, arg1);
			}

			if (state == 1)
				cur_flags = cur_flags | tmp_flags;
			else {
				tmp_flags = cur_flags & tmp_flags;
				cur_flags = cur_flags ^ tmp_flags;
			}

			MOB_FLAGS(mob_p) = cur_flags;

			if (tmp_flags == 0 && cur_flags == 0) {
				SET_BIT(zone->flags, ZONE_MOBS_MODIFIED);
				SET_BIT(MOB_FLAGS(mob_p), MOB_ISNPC);
				send_to_char(ch, "Mobile flags set to: ISNPC\r\n");
			} else if (tmp_flags == 0)
				send_to_char(ch, "Mobile flags not altered.\r\n");
			else {
				SET_BIT(zone->flags, ZONE_MOBS_MODIFIED);
				SET_BIT(MOB_FLAGS(mob_p), MOB_ISNPC);
				send_to_char(ch, "Mobile flags set.\r\n");
			}
			break;
		}
	case 5:{					/** flags2 **/
			tmp_flags = 0;
			argument = one_argument(arg2, arg1);

			for (zone = zone_table; zone; zone = zone->next)
				if (mob_p->mob_specials.shared->vnum >= zone->number * 100 &&
					mob_p->mob_specials.shared->vnum <= zone->top)
					break;

			if (*arg1 == '+')
				state = 1;
			else if (*arg1 == '-')
				state = 2;
			else {
				send_to_char(ch, 
					"Usage: olc mset flags2 [+/-] [FLAG, FLAG, ...]\r\n");
				return;
			}

			argument = one_argument(argument, arg1);

			cur_flags = MOB2_FLAGS(mob_p);

			while (*arg1) {
				if ((flag =
						search_block(arg1, action2_bits_desc, FALSE)) == -1) {
					send_to_char(ch, "Invalid flag %s, skipping...\r\n", arg1);
				} else
					tmp_flags = tmp_flags | (1 << flag);

				argument = one_argument(argument, arg1);
			}

			if (state == 1)
				cur_flags = cur_flags | tmp_flags;
			else {
				tmp_flags = cur_flags & tmp_flags;
				cur_flags = cur_flags ^ tmp_flags;
			}

			MOB2_FLAGS(mob_p) = cur_flags;

			if (tmp_flags == 0 && cur_flags == 0) {
				SET_BIT(zone->flags, ZONE_MOBS_MODIFIED);
				send_to_char(ch, "Mobile flags2 set to: none\r\n");
			} else if (tmp_flags == 0)
				send_to_char(ch, "Mobile flags2 not altered.\r\n");
			else {
				SET_BIT(zone->flags, ZONE_MOBS_MODIFIED);
				send_to_char(ch, "Mobile flags2 set.\r\n");
				REMOVE_BIT(MOB2_FLAGS(mob_p), MOB2_RENAMED);
			}
			break;
		}
	case 6:{					  /** aff **/
			tmp_flags = 0;
			argument = one_argument(arg2, arg1);

			for (zone = zone_table; zone; zone = zone->next)
				if (mob_p->mob_specials.shared->vnum >= zone->number * 100 &&
					mob_p->mob_specials.shared->vnum <= zone->top)
					break;

			if (*arg1 == '+')
				state = 1;
			else if (*arg1 == '-')
				state = 2;
			else {
				send_to_char(ch, "Usage: olc mset aff [+/-] [FLAG, FLAG, ...]\r\n");
				return;
			}

			argument = one_argument(argument, arg1);

			cur_flags = AFF_FLAGS(mob_p);

			while (*arg1) {
				if ((flag =
						search_block(arg1, affected_bits_desc, FALSE)) == -1) {
					send_to_char(ch, "Invalid flag %s, skipping...\r\n", arg1);
				} else
					tmp_flags = tmp_flags | (1 << flag);

				argument = one_argument(argument, arg1);
			}

			if (state == 1)
				cur_flags = cur_flags | tmp_flags;
			else {
				tmp_flags = cur_flags & tmp_flags;
				cur_flags = cur_flags ^ tmp_flags;
			}

			AFF_FLAGS(mob_p) = cur_flags;

			if (tmp_flags == 0 && cur_flags == 0) {
				SET_BIT(zone->flags, ZONE_MOBS_MODIFIED);
				send_to_char(ch, "Mobile affected flags set to: none\r\n");
			} else if (tmp_flags == 0)
				send_to_char(ch, "Mobile affected flags not altered.\r\n");
			else {
				SET_BIT(zone->flags, ZONE_MOBS_MODIFIED);
				send_to_char(ch, "Mobile affected flags set.\r\n");
			}
			break;
		}
	case 7:{					  /** aff2 **/
			tmp_flags = 0;
			argument = one_argument(arg2, arg1);

			for (zone = zone_table; zone; zone = zone->next)
				if (mob_p->mob_specials.shared->vnum >= zone->number * 100 &&
					mob_p->mob_specials.shared->vnum <= zone->top)
					break;

			if (*arg1 == '+')
				state = 1;
			else if (*arg1 == '-')
				state = 2;
			else {
				send_to_char(ch, 
					"Usage: olc mset aff2 [+/-] [FLAG, FLAG, ...]\r\n");
				return;
			}

			argument = one_argument(argument, arg1);

			cur_flags = AFF2_FLAGS(mob_p);

			while (*arg1) {
				if ((flag =
						search_block(arg1, affected2_bits_desc,
							FALSE)) == -1) {
					send_to_char(ch, "Invalid flag %s, skipping...\r\n", arg1);
				} else
					tmp_flags = tmp_flags | (1 << flag);

				argument = one_argument(argument, arg1);
			}

			if (state == 1)
				cur_flags = cur_flags | tmp_flags;
			else {
				tmp_flags = cur_flags & tmp_flags;
				cur_flags = cur_flags ^ tmp_flags;
			}

			AFF2_FLAGS(mob_p) = cur_flags;

			if (tmp_flags == 0 && cur_flags == 0) {
				SET_BIT(zone->flags, ZONE_MOBS_MODIFIED);
				send_to_char(ch, "Mobile affected2 flags set to: none\r\n");
			} else if (tmp_flags == 0)
				send_to_char(ch, "Mobile affected2 flags not altered.\r\n");
			else {
				SET_BIT(zone->flags, ZONE_MOBS_MODIFIED);
				send_to_char(ch, "Mobile affected2 flags set.\r\n");
			}
			break;
		}
	case 8:{					  /** aff3 **/
			tmp_flags = 0;
			argument = one_argument(arg2, arg1);

			for (zone = zone_table; zone; zone = zone->next)
				if (mob_p->mob_specials.shared->vnum >= zone->number * 100 &&
					mob_p->mob_specials.shared->vnum <= zone->top)
					break;

			if (*arg1 == '+')
				state = 1;
			else if (*arg1 == '-')
				state = 2;
			else {
				send_to_char(ch, 
					"Usage: olc mset aff3 [+/-] [FLAG, FLAG, ...]\r\n");
				return;
			}

			argument = one_argument(argument, arg1);

			cur_flags = AFF3_FLAGS(mob_p);

			while (*arg1) {
				if ((flag =
						search_block(arg1, affected3_bits_desc,
							FALSE)) == -1) {
					send_to_char(ch, "Invalid flag %s, skipping...\r\n", arg1);
				} else
					tmp_flags = tmp_flags | (1 << flag);

				argument = one_argument(argument, arg1);
			}

			if (state == 1)
				cur_flags = cur_flags | tmp_flags;
			else {
				tmp_flags = cur_flags & tmp_flags;
				cur_flags = cur_flags ^ tmp_flags;
			}

			AFF3_FLAGS(mob_p) = cur_flags;

			if (tmp_flags == 0 && cur_flags == 0) {
				SET_BIT(zone->flags, ZONE_MOBS_MODIFIED);
				send_to_char(ch, "Mobile affected3 flags set to: none\r\n");
			} else if (tmp_flags == 0)
				send_to_char(ch, "Mobile affected3 flags not altered.\r\n");
			else {
				SET_BIT(zone->flags, ZONE_MOBS_MODIFIED);
				send_to_char(ch, "Mobile affected3 flags set.\r\n");
			}
			break;
		}
	case 9:{				 /** align **/
			i = atoi(arg2);
			if (i < -1000 || i > 1000)
				send_to_char(ch, "Alignment must be between -1000 and 1000.\r\n");
			else {
				GET_ALIGNMENT(mob_p) = i;
				send_to_char(ch, "Mobile alignment set.\r\n");
			}
			break;
		}
	case 10:{				 /** str **/
			i = atoi(arg2);
			if (i < 3 || i > 25)
				send_to_char(ch, "Strength must be between 3 and 25.\r\n");
			else {
				mob_p->aff_abils.str = mob_p->real_abils.str = i;
				send_to_char(ch, "Mobile strength set.\r\n");
			}
			break;
		}
	case 11:{				 /** intel **/
			i = atoi(arg2);
			if (i < 3 || i > 25)
				send_to_char(ch, "Intelligence must be between 3 and 25.\r\n");
			else {
				mob_p->aff_abils.intel = mob_p->real_abils.intel = i;
				send_to_char(ch, "Mobile intelligence set.\r\n");
			}
			break;
		}
	case 12:{				/** wis **/
			i = atoi(arg2);
			if (i < 3 || i > 25)
				send_to_char(ch, "Wisdom must be between 3 and 25.\r\n");
			else {
				mob_p->aff_abils.wis = mob_p->real_abils.wis = i;
				send_to_char(ch, "Mobile wisdom set.\r\n");
			}
			break;
		}
	case 13:{				/** dex **/
			i = atoi(arg2);
			if (i < 3 || i > 25)
				send_to_char(ch, "Dexterity must be between 3 and 25.\r\n");
			else {
				mob_p->aff_abils.dex = mob_p->real_abils.dex = i;
				send_to_char(ch, "Mobile dexterity set.\r\n");
			}
			break;
		}
	case 14:{				 /** con **/
			i = atoi(arg2);
			if (i < 3 || i > 25)
				send_to_char(ch, "Constitution must be between 3 and 25.\r\n");
			else {
				mob_p->aff_abils.con = mob_p->real_abils.con = i;
				send_to_char(ch, "Mobile constitution set.\r\n");
			}
			break;
		}
	case 15:{				/** cha **/
			i = atoi(arg2);
			if (i < 3 || i > 25)
				send_to_char(ch, "Charisma must be between 3 and 25.\r\n");
			else {
				mob_p->aff_abils.cha = mob_p->real_abils.cha = i;
				send_to_char(ch, "Mobile charisma set.\r\n");
			}
			break;
		}
	case 16:{			   /** level **/
			i = atoi(arg2);
			if (i < 1 || i > 50)
				send_to_char(ch, "Level must be between 1 and 50.\r\n");
			else {
				GET_LEVEL(mob_p) = i;
				send_to_char(ch, "Mobile level set.\r\n");
                recalculate_based_on_level( mob_p );
			}
			break;
		}
	case 17:{			   /** hitp_mod **/
			i = atoi(arg2);
			if (i < 0 || i > 32767)
				send_to_char(ch, 
					"Hit point modifier must be between 1 and 32767.\r\n");
			else {
				GET_MOVE(mob_p) = i;
				send_to_char(ch, "Mobile hit point mod set.\r\n");
			}
			break;
		}
	case 18:{				/** hitd_num **/
			i = atoi(arg2);
			if (i < 0 || i > 200)
				send_to_char(ch, 
					"Hit point dice number must be between 1 and 200.\r\n");
			else {
				GET_HIT(mob_p) = i;
				send_to_char(ch, "Mobile hit point dice number set.\r\n");
			}
			break;
		}
	case 19:{			   /** hitd_size **/
			i = atoi(arg2);
			if (i < 0 || i > 200)
				send_to_char(ch, 
					"Hit point dice size must be between 1 and 200.\r\n");
			else {
				GET_MANA(mob_p) = i;
				send_to_char(ch, "Mobile hit point dice size set.\r\n");
			}
			break;

		}
	case 20:{			   /** mana **/
			i = atoi(arg2);
			if (i < 0 || i > 32767)
				send_to_char(ch, "Mana must be bewteen 1 and 32767.\r\n");
			else {
				GET_MAX_MANA(mob_p) = i;
				send_to_char(ch, "Mobile mana set.\r\n");
			}
			break;
		}
	case 21:{			   /** move **/
			i = atoi(arg2);
			if (i < 0 || i > 32767)
				send_to_char(ch, "Movement must be between 1 and 32767.\r\n");
			else {
				GET_MAX_MOVE(mob_p) = i;
				send_to_char(ch, "Mobile movement set.\r\n");
			}
			break;
		}
	case 22:{			   /** baredam **/
			i = atoi(arg2);
			if (i < 1 || i > 125)
				send_to_char(ch, "Bare hand damage must be between 1 and 125.\r\n");
			else {
				mob_p->mob_specials.shared->damnodice = i;
				send_to_char(ch, "Mobile bare handed damage set.\r\n");
			}
			break;
		}
	case 23:{			   /** baredsize **/
			i = atoi(arg2);
			if (i < 1 || i > 125)
				send_to_char(ch, 
					"Bare handed damage dice size must be between 1 and 125.\r\n");
			else {
				mob_p->mob_specials.shared->damsizedice = i;
				send_to_char(ch, "Mobile damage dice size set.\r\n");
			}
			break;
		}
	case 24:{			   /** gold **/
			i = atoi(arg2);
			if (i < 0 || i > 10000000)
				send_to_char(ch, "Gold must be between 0 and 10,000,000.\r\n");
			else {
				GET_GOLD(mob_p) = i;
				send_to_char(ch, "Mobile gold set.\r\n");
			}
			break;
		}
	case 25:{			   /** exp **/
			i = atoi(arg2);
			if (i < 0 || i > 200000000)
				send_to_char(ch, 
					"Experience must be between 0 and 200,000,000.\r\n");
			else {
				GET_EXP(mob_p) = i;
				send_to_char(ch, "Mobile experience set.\r\n");
			}
			break;
		}
	case 26:{			   /** attack **/
			if ((i = search_block(arg2, attack_type, FALSE)) < 0) {
				send_to_char(ch, "Invalid attack type, '%s'.\r\n", arg2);
			} else {
				mob_p->mob_specials.shared->attack_type = i;
				send_to_char(ch, "Mobile attack type set.\r\n");
			}
			break;
		}
	case 27:{			  /** position **/
			if ((i = search_block(arg2, position_types, FALSE)) < 0) {
				send_to_char(ch, "Invalid position, '%s'.\r\n", arg2);
			} else {
				mob_p->setPosition(i);
				send_to_char(ch, "Mobile position set.\r\n");
			}
			break;
		}
	case 28:{			   /** sex **/
			if ((i = search_block(arg2, genders, FALSE)) < 0) {
				send_to_char(ch, "Invalid gender, '%s'.\r\n", arg2);
			} else {
				mob_p->player.sex = i;
				send_to_char(ch, "Mobile gender set.\r\n");
			}
			break;
		}
	case 29:{			   /** remortchar_class **/
			if (!strncmp(arg2, "none", 4))
				i = -1;
			else if ((i = search_block(arg2, class_names, FALSE)) < 0) {
				send_to_char(ch, "Invalid char_class type, '%s'.\r\n", arg2);
				break;
			}
			mob_p->player.remort_char_class = i;
			send_to_char(ch, "Mobile Remort Class set.\r\n");
			break;
		}
	case 30:{			   /** cash **/
			i = atoi(arg2);
			if (i < 0 || i > 1000000)
				send_to_char(ch, "Cash must be between 0 and 1,000,000.\r\n");
			else {
				GET_CASH(mob_p) = i;
				send_to_char(ch, "Mobile cash credits set.\r\n");
			}
			break;
		}
	case 31:{			   /** hitroll **/
			i = atoi(arg2);
			if (i < -125 || i > 125)
				send_to_char(ch, "Hitroll must be between -125 and 125.\r\n");
			else {
				GET_HITROLL(mob_p) = i;
				send_to_char(ch, "Mobile hitroll set.\r\n");
			}
			break;
		}
	case 32:{			   /** damroll **/
			i = atoi(arg2);
			if (i < -125 || i > 125)
				send_to_char(ch, "Damroll must be between -125 and 125.\r\n");
			else {
				GET_DAMROLL(mob_p) = i;
				send_to_char(ch, "Mobile damroll set.\r\n");
			}
			break;
		}
	case 33:{			   /** ac **/
			i = atoi(arg2);
			if (i < -500 || i > 100)
				send_to_char(ch, "Armor Class must be between -500 and 100.\r\n");
			else {
				GET_AC(mob_p) = i;
				send_to_char(ch, "Mobile Armor Class set.\r\n");
			}
			break;
		}
	case 34:{			   /** char_class **/
			if ((i = search_block(arg2, class_names, FALSE)) < 0) {
				send_to_char(ch, "Invalid char_class type, '%s'.\r\n", arg2);
			} else {
				GET_CLASS(mob_p) = i;
				set_physical_attribs(mob_p);
				send_to_char(ch, "Mobile Class set.\r\n");
			}
			break;
		}
	case 35:{			   /** race **/
			if ((i = search_block(arg2, player_race, FALSE)) < 0) {
				send_to_char(ch, "Invalid race, '%s'.\r\n", arg2);
			} else {
				GET_RACE(mob_p) = i;
				set_physical_attribs(mob_p);
                set_move_buffer(mob_p);
				send_to_char(ch, "Mobile Race set.\r\n");
                
                SET_TONGUE(mob_p, TONGUE_COMMON, 0);
                set_initial_tongue(mob_p);
			}
			break;
		}
	case 36:{			   /** dpos **/
			if ((i = search_block(arg2, position_types, FALSE)) < 0) {
				send_to_char(ch, "Invalid default position, '%s'.\r\n", arg2);
			} else {
				GET_DEFAULT_POS(mob_p) = i;
				send_to_char(ch, "Mobile default position set.\r\n");
			}
			break;
		}
	case 37:{			   /** stradd **/
			i = atoi(arg2);
			if (i < 0 || i > 100)
				send_to_char(ch, "StrAdd must be between 0 and 100.\r\n");
			else {
				GET_ADD(mob_p) = i;
				send_to_char(ch, "Mobile StrAdd set.\r\n");
			}
			break;
		}
	case 38:{			   /** height **/
			i = atoi(arg2);
			if (i < 1 || i > 10000)
				send_to_char(ch, "Height must be between 1 and 10000.\r\n");
			else {
				GET_HEIGHT(mob_p) = i;
				send_to_char(ch, "Mobile height set.\r\n");
			}
			break;
		}
	case 39:{			   /** weight **/
			i = atoi(arg2);
			if (i < 1 || i > 50000)
				send_to_char(ch, "Weight must be between 1 and 50000.\r\n");
			else {
				GET_WEIGHT(mob_p) = i;
				send_to_char(ch, "Mobile weight set.\r\n");
			}
			break;
		}
    case 40:
        send_to_char(ch, "Removed.  Use a prog instead.\r\n");
        break;
	case 41:{
			if (!*arg2 || (i = find_spec_index_arg(arg2)) < 0)
				send_to_char(ch, "That is not a valid special.\r\n"
					"Type show special mob to view a list.\r\n");
			else if (!IS_SET(spec_list[i].flags, SPEC_MOB))
				send_to_char(ch, "This special is not for mobiles.\r\n");
			else if (IS_SET(spec_list[i].flags, SPEC_RES) && !OLCIMP(ch))
				send_to_char(ch, "This special is reserved.\r\n");
			else {

				mob_p->mob_specials.shared->func = spec_list[i].func;
				if (MOB_SHARED(mob_p)->func_param)
					free(MOB_SHARED(mob_p)->func_param);
				MOB_SHARED(mob_p)->func_param = NULL;
				do_specassign_save(ch, SPEC_MOB);
				send_to_char(ch, "Mobile special set.\r\n");
			}
			break;

		}
	case 42:{			  /*** morale ***/
			i = atoi(arg2);
			if (i < 0 || i > 125)
				send_to_char(ch, "Morale must be between 1 and 125.\r\n");
			else {
				mob_p->mob_specials.shared->morale = i;
				send_to_char(ch, "Mobile morale set.\r\n");
			}
			break;

		}
	case 43:{				 /** str_app **/
			i = atoi(arg2);
			if (mob_p->real_abils.str != 18) {
				send_to_char(ch, "The mob must have an 18 strength.\r\n");
				return;
			}
			if (i < 0 || i > 100)
				send_to_char(ch, "Strength apply must be between 0 and 100.\r\n");
			else {
				mob_p->aff_abils.str_add = mob_p->real_abils.str_add = i;
				send_to_char(ch, "Mobile strength apply set.\r\n");
			}
			break;

		}
	case 44:{					/** move_buf **/
			if (MOB_SHARED(mob_p)->move_buf)
				free(MOB_SHARED(mob_p)->move_buf);
			MOB_SHARED(mob_p)->move_buf = strdup(arg2);
			send_to_char(ch, "Mobile move_buf set.\r\n");
			break;

		}
	case 45:{		  /** lair **/
			i = atoi(arg2);
			mob_p->mob_specials.shared->lair = i;
			send_to_char(ch, "Mobile lair set.\r\n");
			break;

		}
	case 46:{		  /** leader **/
			i = atoi(arg2);
			mob_p->mob_specials.shared->leader = i;
			send_to_char(ch, "Mobile leader set.\r\n");
			break;

		}
	case 47:
		// Make sure they have a mob special
		if (!GET_MOB_SPEC(mob_p)) {
			send_to_char(ch, "You should set a special first!\r\n");
			break;
		}

		// Check to see that they can set the spec param
		i = find_spec_index_ptr(GET_MOB_SPEC(mob_p));
		if (IS_SET(spec_list[i].flags, SPEC_RES) && !OLCIMP(ch)) {
			send_to_char(ch, "This special is reserved.\r\n");
			break;
		}

		// It's ok.  Let em set it.
		start_editing_text(ch->desc, &MOB_SHARED(mob_p)->func_param);
		SET_BIT(PLR_FLAGS(ch), PLR_OLC);
		act("$n begins to write a mobile spec param.", TRUE, ch, 0, 0,
			TO_ROOM);
		break;
    case 48:{ // generation
			i = atoi(arg2);
			if (i < 0 || i > 10 ) {
				send_to_char(ch, "Generation must be between 1 and 10.\r\n");
			} else {
                GET_REAL_GEN(mob_p) = i;
				send_to_char(ch, "Mobile generation set.\r\n");
                recalculate_based_on_level( mob_p );
			}
			break;

		}
	case 49:
		start_editing_text(ch->desc, &MOB_SHARED(mob_p)->load_param);
		SET_BIT(PLR_FLAGS(ch), PLR_OLC);
		act("$n begins to write a mobile load param.", TRUE, ch, 0, 0, TO_ROOM);
		break;
    case 50: { // Known language
        char *arguments = tmp_strdup(arg2);
        char *myarg;

        myarg = tmp_getword(&arguments);

        if (*myarg == '+')
            state = 1;
        else if (*myarg == '-')
            state = 2;
        else {
            send_to_char(ch, 
                "Usage: olc mset knownlanguage [+/-] [LANG, LANG, ...]\r\n");
            return;
        }

        myarg = tmp_getword(&arguments);

        for (;*myarg;myarg = tmp_getword(&arguments)) {
            int idx = find_tongue_idx_by_name(myarg);
            if (idx == TONGUE_NONE) {
                send_to_char(ch, "Invalid language %s, skipping...\r\n", arg1);
            } else {
                SET_TONGUE(mob_p, idx, (state == 1) ? 100:0);
            }
        }

        send_to_char(ch, "Mobile languages set.\r\n");
       break;
    }
    case 51: {// Current language
       int idx = TONGUE_NONE;

       if (!*arg2) {
           send_to_char(ch, "Usage: olc mset curlanguage <language>");
           return;
       }
       
       char *argument = tmp_strdup(arg2);
       idx = find_tongue_idx_by_name(argument);
       if (idx == TONGUE_NONE) {
           send_to_char(ch, "That is not a language!\r\n");
           return;
       }

       GET_TONGUE(mob_p) = idx;
       send_to_char(ch, "Mobile default language set.\r\n");
       break;
	   }
	case 52:
		start_editing_prog(ch->desc, mob_p, PROG_TYPE_MOBILE);
		SET_BIT(PLR_FLAGS(ch), PLR_OLC);
		act("$n begins to write a mobile prog.", TRUE, ch, 0, 0, TO_ROOM);
		break;
	default:{
			break;
		}
	}

	if (mset_command != 25)		/*mob exp */
		GET_EXP(mob_p) = mobile_experience(mob_p);

	for (zone = zone_table; zone; zone = zone->next)
		if (mob_p->mob_specials.shared->vnum >= zone->number * 100 &&
			mob_p->mob_specials.shared->vnum <= zone->top)
			break;

	if (!ZONE_FLAGGED(zone, ZONE_FULLCONTROL) && !OLCIMP(ch) )
		SET_BIT(MOB2_FLAGS(mob_p), MOB2_UNAPPROVED);

}



int
write_mob_index(struct Creature *ch, struct zone_data *zone)
{
	int done = 0, i, j, found = 0, count = 0, *new_index;
	char fname[64];
	FILE *index;

	for (i = 0; mob_index[i] != -1; i++) {
		count++;
		if (mob_index[i] == zone->number) {
			found = 1;
			break;
		}
	}

	if (found == 1)
		return (1);

	CREATE(new_index, int, count + 2);

	for (i = 0, j = 0;; i++) {
		if (mob_index[i] == -1) {
			if (done == 0) {
				new_index[j] = zone->number;
				new_index[j + 1] = -1;
			} else
				new_index[j] = -1;
			break;
		}
		if (mob_index[i] > zone->number && done != 1) {
			new_index[j] = zone->number;
			j++;
			new_index[j] = mob_index[i];
			done = 1;
		} else
			new_index[j] = mob_index[i];
		j++;
	}

	free(mob_index);

#ifdef DMALLOC
	dmalloc_verify(0);
#endif

	mob_index = new_index;

	sprintf(fname, "world/mob/index");
	if (!(index = fopen(fname, "w"))) {
		send_to_char(ch, "Could not open index file, mobile save aborted.\r\n");
		return (0);
	}

	for (i = 0; mob_index[i] != -1; i++)
		fprintf(index, "%d.mob\n", mob_index[i]);

	fprintf(index, "$\n");

	send_to_char(ch, "Mobile index file re-written.\r\n");

	fclose(index);

	return (1);
}

bool
save_mobs(struct Creature *ch, struct zone_data *zone)
{
	int espec_mob = 0;
	unsigned int i, tmp;
	room_num low = 0;
	room_num high = 0;
	char fname[64];
	char sbuf1[64], sbuf2[64], sbuf3[64], sbuf4[64], sbuf5[64];
	struct Creature *mob;
	FILE *file;
	FILE *realfile;

	if (!zone) {
		errlog("save_mob() called with NULL zone");
		return false;
	}

	sprintf(fname, "world/mob/%d.mob", zone->number);
	if ((access(fname, F_OK) >= 0) && (access(fname, W_OK) < 0)) {
		mudlog(0, BRF, true,
			"OLC: ERROR - Main mobile file for zone %d is read-only.",
			ch->in_room->zone->number);
	}
	sprintf(fname, "world/mob/olc/%d.mob", zone->number);
	if (!(file = fopen(fname, "w"))) {
		slog("OLC: ERROR while saving mobile file - %s", strerror(errno));
		return false;
	}

	if ((write_mob_index(ch, zone)) != 1) {
		fclose(file);
		return false;
	}

	low = zone->number * 100;
	high = zone->top;

	MobileMap::iterator mit = mobilePrototypes.begin();
	for (; mit != mobilePrototypes.end(); ++mit) {
		mob = mit->second;
		if (mob->mob_specials.shared->vnum < low)
			continue;
		if (mob->mob_specials.shared->vnum > high)
			break;

		if (mob->mob_specials.shared->attack_type != 0 ||
			GET_STR(mob) != 11 ||
			GET_ADD(mob) != 0 ||
			GET_INT(mob) != 11 ||
			GET_WIS(mob) != 11 ||
			GET_DEX(mob) != 11 ||
			GET_CON(mob) != 11 ||
			GET_CHA(mob) != 11 ||
			GET_MAX_MANA(mob) != 10 ||
			GET_MAX_MOVE(mob) != 50 ||
			GET_WEIGHT(mob) != 200 ||
			GET_HEIGHT(mob) != 198 ||
			GET_MORALE(mob) != 100 ||
			GET_MOB_LAIR(mob) >= 0 ||
			GET_MOB_LEADER(mob) >= 0 ||
			GET_REMORT_CLASS(mob) != -1 ||
			GET_CASH(mob) != 0 || MOB_SHARED(mob)->move_buf)
			espec_mob = 1;

		fprintf(file, "#%d\n", mob->mob_specials.shared->vnum);

		if (mob->player.name)
			fprintf(file, "%s", mob->player.name);
		fprintf(file, "~\n");

		if (mob->player.short_descr)
			fprintf(file, "%s", mob->player.short_descr);
		fprintf(file, "~\n");

		if (mob->player.long_descr) {
			tmp = strlen(mob->player.long_descr);
			for (i = 0; i < tmp; i++)
				if (mob->player.long_descr[i] != '\r' &&
					mob->player.long_descr[i] != '~')
					fputc(mob->player.long_descr[i], file);
		}
		fprintf(file, "\n~\n");

		if (mob->player.description) {
			tmp = strlen(mob->player.description);
			for (i = 0; i < tmp; i++)
				if (mob->player.description[i] != '\r' &&
					mob->player.description[i] != '~')
					fputc(mob->player.description[i], file);
		}
		fprintf(file, "~\n");

		REMOVE_BIT(MOB_FLAGS(mob), MOB_WIMPY);
		num2str(sbuf1, MOB_FLAGS(mob));
		num2str(sbuf2, MOB2_FLAGS(mob));
		num2str(sbuf3, AFF_FLAGS(mob));
		num2str(sbuf4, AFF2_FLAGS(mob));
		num2str(sbuf5, AFF3_FLAGS(mob));

		fprintf(file, "%s %s %s %s %s %d ", sbuf1, sbuf2, sbuf3,
			sbuf4, sbuf5, GET_ALIGNMENT(mob));

		if (espec_mob == 1)
			fprintf(file, "E\n");
		else
			fprintf(file, "S\n");

		fprintf(file, "%d %d %d %dd%d+%d %dd%d+%d\n",
			GET_LEVEL(mob),
			(20 - mob->points.hitroll),
			mob->points.armor / 10,
			mob->points.hit,
			mob->points.mana,
			mob->points.move,
			mob->mob_specials.shared->damnodice,
			mob->mob_specials.shared->damsizedice, mob->points.damroll);

		fprintf(file, "%d %d %d %d\n", GET_GOLD(mob),
			GET_EXP(mob), GET_RACE(mob), GET_CLASS(mob));

		fprintf(file, "%d %d %d %d\n", mob->getPosition(),
			GET_DEFAULT_POS(mob),
			GET_SEX(mob), mob->mob_specials.shared->attack_type);

		if (espec_mob == 1) {
			if (GET_STR(mob) != 11)
				fprintf(file, "Str: %d\n", GET_STR(mob));
			if (GET_ADD(mob) != 0)
				fprintf(file, "StrAdd: %d\n", GET_ADD(mob));
			if (GET_INT(mob) != 11)
				fprintf(file, "Int: %d\n", GET_INT(mob));
			if (GET_WIS(mob) != 11)
				fprintf(file, "Wis: %d\n", GET_WIS(mob));
			if (GET_DEX(mob) != 11)
				fprintf(file, "Dex: %d\n", GET_DEX(mob));
			if (GET_CON(mob) != 11)
				fprintf(file, "Con: %d\n", GET_CON(mob));
			if (GET_CHA(mob) != 11)
				fprintf(file, "Cha: %d\n", GET_CHA(mob));
			if (GET_MAX_MANA(mob) != 10)
				fprintf(file, "MaxMana: %d\n", GET_MAX_MANA(mob));
			if (GET_MAX_MOVE(mob) != 50)
				fprintf(file, "MaxMove: %d\n", GET_MAX_MOVE(mob));
			if (GET_WEIGHT(mob) != 200)
				fprintf(file, "Weight: %d\n", GET_WEIGHT(mob));
			if (GET_HEIGHT(mob) != 198)
				fprintf(file, "Height: %d\n", GET_HEIGHT(mob));
            if (GET_TONGUE(mob) != 0)
                fprintf(file, "CurTongue: %d\n", GET_TONGUE(mob));
            map<int, Tongue>::iterator it = tongues.begin();
            for (;it != tongues.end();++it)
                if (CHECK_TONGUE(mob, it->first))
                    fprintf(file, "KnownTongue: %d\n", it->first);

			if (GET_CASH(mob) != 0)
				fprintf(file, "Cash: %d\n", GET_CASH(mob));
			if (GET_MORALE(mob) != 100)
				fprintf(file, "Morale: %d\n", GET_MORALE(mob));
			if (GET_MOB_LAIR(mob) >= 0)
				fprintf(file, "Lair: %d\n", GET_MOB_LAIR(mob));
			if (GET_MOB_LEADER(mob) >= 0)
				fprintf(file, "Leader: %d\n", GET_MOB_LEADER(mob));
			if (GET_REMORT_CLASS(mob) != -1)
				fprintf(file, "RemortClass: %d\n", GET_REMORT_CLASS(mob));
			if (MOB_SHARED(mob)->move_buf)
				fprintf(file, "Move_buf: %s\n", MOB_SHARED(mob)->move_buf);
			if (MOB_SHARED(mob)->func_param) {
				char *str;

				str = tmp_gsub(MOB_SHARED(mob)->func_param, "\r", "");
				str = tmp_gsub(str, "~", "!");
				fprintf(file, "SpecParam:\n%s~\n", str);
			}
			if (MOB_SHARED(mob)->load_param) {
				char *str;
				str = tmp_gsub(MOB_SHARED(mob)->load_param, "\r", "");
				str = tmp_gsub(str, "~", "!");
				fprintf(file, "LoadParam:\n%s~\n", str);
			}
			if (MOB_SHARED(mob)->prog) {
				char *str;
				str = tmp_gsub(MOB_SHARED(mob)->prog, "\r", "");
				str = tmp_gsub(str, "~", "!");
				fprintf(file, "Prog:\n%s~\n", str);
			}
            if( GET_REMORT_GEN(mob) > 0 && GET_REMORT_GEN(mob) <= 10 ) {
                fprintf(file, "Generation: %d\n", GET_REMORT_GEN(mob) );
            }

			fprintf(file, "E\n");
		}
	}

	fprintf(file, "$\n");

	slog("OLC: %s msaved %d.", GET_NAME(ch), zone->number);

	sprintf(fname, "world/mob/%d.mob", zone->number);
	realfile = fopen(fname, "w");
	if (realfile) {
		fclose(file);
		sprintf(fname, "world/mob/olc/%d.mob", zone->number);
		if (!(file = fopen(fname, "r"))) {
			errlog("Failure to reopen olc mob file.");
			send_to_char(ch, 
				"OLC Error: Failure to duplicate mob file in main dir."
				"\r\n");
			fclose(realfile);
			return false;
		}
		do {
			tmp = fread(buf, 1, 512, file);
			if (fwrite(buf, 1, tmp, realfile) != tmp) {
				errlog("Failure to duplicate olc mob file in the main wld dir.");
				send_to_char(ch, 
					"OLC Error: Failure to duplicate mob file in main dir."
					"\r\n");
				fclose(realfile);
				fclose(file);
				return false;
			}
		} while (tmp == 512);

		fclose(realfile);
	}
	fclose(file);

	do_specassign_save(ch, SPEC_MOB);
	REMOVE_BIT(zone->flags, ZONE_MOBS_MODIFIED);
	return true;
}


int
do_destroy_mobile(struct Creature *ch, int vnum)
{

	struct zone_data *zone = NULL;
	struct Creature *mob = NULL;
	struct descriptor_data *d = NULL;
	struct memory_rec_struct *mem_r = NULL;

	if (!(mob = real_mobile_proto(vnum))) {
		send_to_char(ch, "ERROR: That mobile does not exist.\r\n");
		return 1;
	}

	for (zone = zone_table; zone; zone = zone->next)
		if (vnum < zone->top)
			break;

	if (!zone) {
		send_to_char(ch, "That mobile does not belong to any zone!!\r\n");
		errlog("mobile not in any zone.");
		return 1;
	}

	if (!CAN_EDIT_ZONE(ch, zone)) {
		send_to_char(ch, "Oh, no you don't!!!\r\n");
		mudlog(GET_INVIS_LVL(ch), BRF, true,
			"OLC: %s failed attempt to DESTROY mobile %d.",
			GET_NAME(ch), GET_MOB_VNUM(mob));
		return 1;
	}
	CreatureList::iterator cit = characterList.begin();
	for (; cit != characterList.end(); ++cit) {
		if (GET_MOB_VNUM((*cit)) == GET_MOB_VNUM(mob))
			(*cit)->purge(false);
	}
	mobilePrototypes.remove(mob);

	for (d = descriptor_list; d; d = d->next) {
		if (d->creature && GET_OLC_MOB(d->creature) == mob) {
			GET_OLC_MOB(d->creature) = NULL;
			send_to_char(d->creature, "The mobile you were editing has been destroyed!\r\n");
			break;
		}
	}

	if (mob->player.name) {
		free(mob->player.name);
	}
	if (mob->player.title) {
		free(mob->player.title);
	}
	if (mob->player.short_descr) {
		free(mob->player.short_descr);
	}
	if (mob->player.long_descr) {
		free(mob->player.long_descr);
	}
	if (mob->player.description) {
		free(mob->player.description);
	}

	mob->mob_specials.shared->proto = NULL;
	if (mob->mob_specials.shared->move_buf) {
		free(mob->mob_specials.shared->move_buf);
	}
	if (mob->mob_specials.func_data) {
		free(mob->mob_specials.func_data);
		mob->mob_specials.func_data = NULL;
	}
	while ((mem_r = mob->mob_specials.memory)) {
		mob->mob_specials.memory = mem_r->next;
		free(mem_r);
	}

	if (mob->mob_specials.shared) {
		free(mob->mob_specials.shared);
	}
	top_of_mobt--;
	delete mob;
	return 0;
}

int
mobile_experience(struct Creature *mob, FILE *outfile)
{
	int exp = 0, tmp = 0;
	exp = 13 * ((GET_LEVEL(mob) * GET_LEVEL(mob) + 24) / 8);
	tmp = exp_scale[(int)MIN(GET_LEVEL(mob) + 1, LVL_GRIMP)];
	tmp += (tmp * GET_LEVEL(mob)) / 100;
	exp = tmp / MAX(1, exp);
	exp = MAX(10, exp);

	if (GET_MAX_HIT(mob))
		exp += (exp * GET_MAX_HIT(mob)) /
			MAX(1, GET_LEVEL(mob) * GET_LEVEL(mob) * 50);
	else
		exp +=
			(exp * (((GET_HIT(mob) * (GET_MANA(mob) + 1)) / 2) +
				GET_MOVE(mob))) / MAX(1, GET_LEVEL(mob) * GET_LEVEL(mob) * 50);

	exp += (exp * mob->mob_specials.shared->damnodice *
		(mob->mob_specials.shared->damsizedice + 1)) / 120;

    // Class/points modifiers
	if (IS_MAGE(mob) || IS_CLERIC(mob) || IS_LICH(mob))
		exp += (exp * GET_MAX_MANA(mob)) / 32000;
	if (IS_KNIGHT(mob) || IS_BARB(mob) || IS_RANGER(mob) || IS_WARRIOR(mob))
		exp += (exp * GET_MAX_MOVE(mob)) / 15000;
	if (IS_THIEF(mob))
		exp += (exp * GET_LEVEL(mob)) / 150;

    // Class/race modifiers
	if (GET_CLASS(mob) == CLASS_ARCH || IS_DRAGON(mob))
		exp = (int)(exp * 1.7);
	else if (IS_DEVIL(mob) || IS_DEMON(mob) || IS_SLAAD(mob) || IS_CELESTIAL(mob))
		exp = (int)(exp * 1.1);
	else if (NON_CORPOREAL_MOB(mob))
		exp = (int)(exp * 1.1);
	else if (IS_GIANT(mob))
		exp = (int)(exp * 1.1);
	else if (IS_TROLL(mob))
		exp = (int)(exp * 1.3);

	exp += (exp * (GET_STR(mob) - 11)) / 20;
	exp += (exp * (GET_DEX(mob) - 11)) / 23;
	exp += (exp * (GET_INT(mob) - 11)) / 27;
	exp += (exp * (GET_WIS(mob) - 11)) / 29;

	exp += (exp * (GET_DAMROLL(mob))) / 200;
	exp += (exp * (GET_HITROLL(mob))) / 300;

	exp -= (exp * (MAX(-200, GET_AC(mob)) - 100)) / 200;
	exp += (exp * GET_MORALE(mob)) / 300;

    float factor = 1.5;

    // Damaging shield modifiers
	if (AFF3_FLAGGED(mob, AFF3_DOUBLE_DAMAGE))
        factor += 0.1;
	if (AFF2_FLAGGED(mob, AFF2_BLADE_BARRIER))
		factor += 0.1;
	if (AFF3_FLAGGED(mob, AFF3_PRISMATIC_SPHERE))
		factor += 0.1;
	if (AFF2_FLAGGED(mob, AFF2_ENERGY_FIELD))
		factor += 0.1;

    // Attack speed modifiers
	if (AFF2_FLAGGED(mob, AFF2_HASTE))
		factor += 0.2;
	if (AFF_FLAGGED(mob, AFF_ADRENALINE))
		factor += 0.1;
	if (AFF_FLAGGED(mob, AFF_CONFIDENCE))
		factor += 0.1;
	if (AFF2_FLAGGED(mob, AFF2_SLOW))
        factor -= 0.1;

    // Protection modifiers
	if (MOB_FLAGGED(mob, MOB_NOBASH))
		factor += 0.3;
	if (AFF_FLAGGED(mob, AFF_SANCTUARY | AFF_NOPAIN))
		factor += 0.2;
	if (AFF2_FLAGGED(mob, AFF2_FIRE_SHIELD))
		factor += 0.1;
	if (AFF_FLAGGED(mob, AFF_INVISIBLE))
		factor += 0.1;
	if (AFF2_FLAGGED(mob, AFF2_TRANSPARENT))
		factor += 0.1;
	if (AFF_FLAGGED(mob, AFF_BLUR))
		factor += 0.1;
    if (AFF2_FLAGGED(mob, AFF2_EVADE))
        factor += 0.1;

    // Thac0 modifiers
	if (AFF2_FLAGGED(mob, AFF2_TRUE_SEEING))
		factor += 0.1;

    // Regen modifiers
	if (AFF_FLAGGED(mob, AFF_REGEN) || IS_TROLL(mob))
		factor += 0.2;

    exp = (int)(exp * factor);
	exp -= exp % 10;
	exp = MAX(0, exp);
	if( outfile != NULL ) {
		int oldmobile_experience(struct Creature *mob);

		char name[21];
		strncpy( name, GET_NAME(mob), 20 );
		name[20] = '\0';
		int oldexp = oldmobile_experience(mob);

		int percent = (int) ( ((float)exp)/((float)oldexp) * 100.0 );
		if( oldexp <= 0 )
			percent = 9999;
		
		fprintf(outfile, "%d %6d \'%20s\' L%2d G%2d %10d %10d [%5d] (%2.2f)\r\n",
                exp,
                GET_MOB_VNUM(mob), name,
                GET_LEVEL(mob), GET_REMORT_GEN(mob),
                oldexp, exp, percent, factor);
	}
	return exp;
}

int
oldmobile_experience(struct Creature *mob)
{
	int exp = 0, tmp = 0;

	exp = 13 * ((GET_LEVEL(mob) * GET_LEVEL(mob) + 24) / 8);
	tmp = exp_scale[(int)MIN(GET_LEVEL(mob) + 1, LVL_GRIMP)];
	tmp += (tmp * GET_LEVEL(mob)) / 100;
	exp = tmp / MAX(1, exp);
	exp = MAX(10, exp);

	if (GET_MAX_HIT(mob))
		exp += (exp * GET_MAX_HIT(mob)) /
			MAX(1, GET_LEVEL(mob) * GET_LEVEL(mob) * 50);
	else
		exp +=
			(exp * (((GET_HIT(mob) * (GET_MANA(mob) + 1)) / 2) +
				GET_MOVE(mob))) / MAX(1, GET_LEVEL(mob) * GET_LEVEL(mob) * 50);

	exp += (exp * mob->mob_specials.shared->damnodice *
		(mob->mob_specials.shared->damsizedice + 1)) / 120;

	if (IS_MAGE(mob) || IS_CLERIC(mob) || IS_LICH(mob))
		exp += (exp * GET_MAX_MANA(mob)) / 32000;
	if (IS_KNIGHT(mob) || IS_BARB(mob) || IS_RANGER(mob) || IS_WARRIOR(mob))
		exp += (exp * GET_MAX_MOVE(mob)) / 15000;
	if (IS_THIEF(mob))
		exp += (exp * GET_LEVEL(mob)) / 150;
	if (GET_CLASS(mob) == CLASS_ARCH || IS_DRAGON(mob))
		exp = (int)(exp * 1.7);
	else if (IS_DEVIL(mob) || IS_DEMON(mob) || IS_SLAAD(mob) || IS_CELESTIAL(mob))
		exp = (int)(exp * 1.5);
	else if (NON_CORPOREAL_MOB(mob))
		exp = (int)(exp * 1.3);
	else if (IS_GIANT(mob))
		exp = (int)(exp * 1.2);
	else if (IS_TROLL(mob))
		exp *= 2;

	exp += (exp * (GET_STR(mob) - 11)) / 20;
	exp += (exp * (GET_DEX(mob) - 11)) / 23;
	exp += (exp * (GET_INT(mob) - 11)) / 27;
	exp += (exp * (GET_WIS(mob) - 11)) / 29;

	exp += (exp * (GET_DAMROLL(mob))) / 200;
	exp += (exp * (GET_HITROLL(mob))) / 300;

	exp -= (exp * (MAX(-200, GET_AC(mob)) - 100)) / 200;
	exp += (exp * GET_MORALE(mob)) / 300;

	if (AFF2_FLAGGED(mob, AFF2_SLOW)) {
		exp = (int)(exp * 0.7);
	}

	const int MAXAFF = 3;
	int affs = 0;
	
	if (affs < MAXAFF && AFF3_FLAGGED(mob, AFF3_DOUBLE_DAMAGE)) {
		exp = (int)(exp * 1.3);
		++affs;
	}
	if (affs < MAXAFF && AFF2_FLAGGED(mob, AFF2_BLADE_BARRIER)) {
		exp = (int)(exp * 1.2);
		++affs;
	}
	if (affs < MAXAFF && AFF3_FLAGGED(mob, AFF3_PRISMATIC_SPHERE)) {
		exp = (int)(exp * 1.2);
		++affs;
	}
	if (affs < MAXAFF && AFF2_FLAGGED(mob, AFF2_HASTE)) {
		exp = (int)(exp * 1.2);
		++affs;
	}
	if (MOB_FLAGGED(mob, MOB_NOBASH)) {
		exp = (int)(exp * 1.2);
		++affs;
	}
	if (affs < MAXAFF && AFF2_FLAGGED(mob, AFF2_TRUE_SEEING)) {
		exp = (int)(exp * 1.2);
		++affs;
	}
	if (affs < MAXAFF && AFF_FLAGGED(mob, AFF_REGEN) || IS_TROLL(mob)) {
		exp = (int)(exp * 1.2);
		++affs;
	}
	if (affs < MAXAFF && AFF_FLAGGED(mob, AFF_SANCTUARY | AFF_NOPAIN)) {
		exp = (int)(exp * 1.2);
		++affs;
	}
	if (affs < MAXAFF && AFF2_FLAGGED(mob, AFF2_FIRE_SHIELD)) {
		exp = (int)(exp * 1.15);
		++affs;
	}
	if (affs < MAXAFF && AFF_FLAGGED(mob, AFF_INVISIBLE)) {
		exp = (int)(exp * 1.15);
		++affs;
	}
	if (affs < MAXAFF && AFF2_FLAGGED(mob, AFF2_ENERGY_FIELD)) {
		exp = (int)(exp * 1.15);
		++affs;
	}
	if (affs < MAXAFF && AFF2_FLAGGED(mob, AFF2_TRANSPARENT)) {
		exp = (int)(exp * 1.1);
		++affs;
	}
	if (affs < MAXAFF && AFF_FLAGGED(mob, AFF_BLUR)) {
		exp = (int)(exp * 1.1);
		++affs;
	}
	if (affs < MAXAFF && AFF_FLAGGED(mob, AFF_SENSE_LIFE)) {
		exp = (int)(exp * 1.1);
		++affs;
	}
	if (affs < MAXAFF && AFF_FLAGGED(mob, AFF_ADRENALINE)) {
		exp = (int)(exp * 1.1);
		++affs;
	}
	if (affs < MAXAFF && AFF_FLAGGED(mob, AFF_CONFIDENCE)) {
		exp = (int)(exp * 1.1);
		++affs;
	}
	
	exp = (int)(exp * 1.5); // arbitrary exp bonus. :)
	
	exp = (exp / 10) * 10;
	exp = MAX(0, exp);

	return exp;
}

int
do_clear_olc_mob(struct Creature *ch)
{
	struct Creature *targ = GET_OLC_MOB(ch);

	targ->mob_specials.shared->number = 0;
	targ->mob_specials.shared->func = NULL;
	targ->mob_specials.shared->proto = targ;

#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	targ->player.name = str_dup("Fresh Blank Mobile");
	targ->player.short_descr = str_dup("a Fresh Blank Mobile");
	targ->player.long_descr =
		str_dup("A Fresh Blank Mobile is here waiting to be violated.");
	targ->player.description = NULL;
	targ->player.title = NULL;

#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	MOB_FLAGS(targ) = 0;
	MOB2_FLAGS(targ) = 0;
	SET_BIT(MOB_FLAGS(targ), MOB_ISNPC);
	SET_BIT(MOB2_FLAGS(targ), MOB2_UNAPPROVED);
	AFF_FLAGS(targ) = 0;
	AFF2_FLAGS(targ) = 0;
	AFF3_FLAGS(targ) = 0;
	GET_ALIGNMENT(targ) = 0;
    GET_REAL_GEN(targ) = 0;

	targ->real_abils.str = 11;
	targ->real_abils.intel = 11;
	targ->real_abils.wis = 11;
	targ->real_abils.dex = 11;
	targ->real_abils.con = 11;
	targ->real_abils.cha = 11;

	GET_LEVEL(targ) = 1;
	targ->points.hitroll = 0;
	targ->points.armor = 0;

	targ->points.hit = 10;
	targ->points.mana = 10;
	targ->points.move = 50;

	targ->points.max_hit = 10;
	targ->points.max_mana = 10;
	targ->points.max_move = 50;

	targ->mob_specials.shared->damnodice = 5;
	targ->mob_specials.shared->damsizedice = 2;
	targ->points.damroll = 0;

	targ->player.char_class = CLASS_NORMAL;
	targ->player.race = RACE_MOBILE;

	GET_GOLD(targ) = 0;
	GET_EXP(targ) = 100;
	GET_MORALE(targ) = 100;
	GET_MOB_LAIR(targ) = -1;
	GET_MOB_LEADER(targ) = -1;

	targ->mob_specials.shared->attack_type = 0;

	targ->char_specials.position = POS_STANDING;
	targ->mob_specials.shared->default_pos = POS_STANDING;
	targ->player.sex = SEX_NEUTRAL;

	targ->player.weight = 130;
	targ->player.height = 166;

	targ->player.remort_char_class = -1;

	targ->aff_abils.str = 11;
	targ->aff_abils.str_add = 0;
	targ->aff_abils.intel = 11;
	targ->aff_abils.wis = 11;
	targ->aff_abils.dex = 11;
	targ->aff_abils.con = 11;
	targ->aff_abils.cha = 11;

	targ->real_abils.str = 11;
	targ->real_abils.str_add = 0;
	targ->real_abils.intel = 11;
	targ->real_abils.wis = 11;
	targ->real_abils.dex = 11;
	targ->real_abils.con = 11;
	targ->real_abils.cha = 11;

	return 0;

}

int
olc_mimic_mob(struct Creature *ch,
	struct Creature *orig, struct Creature *targ, int mode)
{

	if (mode) {					/* (mode) => mimicing prototype... else real mob */
		CreatureList::iterator cit = characterList.begin();
		for (; cit != characterList.end(); ++cit) {
			if (IS_NPC((*cit)) && GET_MOB_VNUM((*cit)) == GET_MOB_VNUM(targ))
				(*cit)->purge(false);
		}
	}

	targ->mob_specials.shared->func = orig->mob_specials.shared->func;
	do_specassign_save(ch, SPEC_MOB);

	if (targ->player.name)
		free(targ->player.name);
	targ->player.name = strdup(orig->player.name);

	if (targ->player.short_descr)
		free(targ->player.short_descr);
	targ->player.short_descr = strdup(orig->player.short_descr);

	if (targ->player.long_descr)
		free(targ->player.long_descr);
	if (orig->player.long_descr)
		targ->player.long_descr = strdup(orig->player.long_descr);
	else
		targ->player.long_descr = NULL;

	if (targ->player.description)
		free(targ->player.description);
	if (orig->player.description)
		targ->player.description = strdup(orig->player.description);
	else
		targ->player.description = NULL;
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	MOB_FLAGS(targ) = MOB_FLAGS(orig);
	MOB2_FLAGS(targ) = MOB2_FLAGS(orig);
	AFF_FLAGS(targ) = AFF_FLAGS(orig);
	AFF2_FLAGS(targ) = AFF2_FLAGS(orig);
	AFF3_FLAGS(targ) = AFF3_FLAGS(orig);
	GET_ALIGNMENT(targ) = GET_ALIGNMENT(orig);
    GET_REAL_GEN(targ) = GET_REMORT_GEN(orig);


	GET_LEVEL(targ) = GET_LEVEL(orig);
	targ->points.hitroll = orig->points.hitroll;
	targ->points.armor = orig->points.armor;

	/* max hit = 0 is a flag that H, M, V is xdy+z */
	targ->points.max_hit = orig->points.max_hit;
	targ->points.hit = orig->points.hit;
	targ->points.mana = orig->points.mana;
	targ->points.move = orig->points.move;

	targ->points.max_mana = orig->points.max_mana;
	targ->points.max_move = orig->points.max_move;

	targ->mob_specials.shared->damnodice =
		orig->mob_specials.shared->damnodice;
	targ->mob_specials.shared->damsizedice =
		orig->mob_specials.shared->damsizedice;
	targ->points.damroll = orig->points.damroll;

	targ->player.char_class = orig->player.char_class;
	targ->player.race = orig->player.race;

	GET_GOLD(targ) = GET_GOLD(orig);
	GET_EXP(targ) = GET_EXP(orig);

	targ->mob_specials.shared->attack_type =
		orig->mob_specials.shared->attack_type;

	targ->char_specials.position = orig->char_specials.position;
	targ->mob_specials.shared->default_pos =
		orig->mob_specials.shared->default_pos;
	targ->player.sex = orig->player.sex;

	targ->player.weight = orig->player.weight;
	targ->player.height = orig->player.height;

	targ->player.remort_char_class = orig->player.remort_char_class;

	targ->aff_abils.str = orig->aff_abils.str;
	targ->aff_abils.str_add = orig->aff_abils.str_add;
	targ->aff_abils.intel = orig->aff_abils.intel;
	targ->aff_abils.wis = orig->aff_abils.wis;
	targ->aff_abils.dex = orig->aff_abils.dex;
	targ->aff_abils.con = orig->aff_abils.con;
	targ->aff_abils.cha = orig->aff_abils.cha;

	targ->real_abils.str = orig->real_abils.str;
	targ->real_abils.str_add = orig->real_abils.str_add;
	targ->real_abils.intel = orig->real_abils.intel;
	targ->real_abils.wis = orig->real_abils.wis;
	targ->real_abils.dex = orig->real_abils.dex;
	targ->real_abils.con = orig->real_abils.con;
	targ->real_abils.cha = orig->real_abils.cha;

	return 0;

}

void set_move_buffer(struct Creature *ch)
{
    if (GET_RACE(ch) == RACE_GRIFFIN) {
        if (MOB_SHARED(ch)->move_buf)
            free(MOB_SHARED(ch)->move_buf);
        MOB_SHARED(ch)->move_buf = strdup("treads"); 
    }
    if (GET_RACE(ch) == RACE_ROTARIAN) {
        if (MOB_SHARED(ch)->move_buf)
            free(MOB_SHARED(ch)->move_buf);
        MOB_SHARED(ch)->move_buf = strdup("lumbers"); 
    }
}
