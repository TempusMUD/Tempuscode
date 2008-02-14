/* ************************************************************************
*   File: act.wizard.c                                  Part of CircleMUD *
*  Usage: Player-level god commands and other goodies                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: act.wizard.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <fstream>
#include <list>
#include <vector>
#include <algorithm>
#include <string.h>
#include <execinfo.h>
using namespace std;

#include "structs.h"
#include "utils.h"
#include "creature_list.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "house.h"
#include "screen.h"
#include "char_class.h"
#include "vehicle.h"
#include "security.h"
#include "olc.h"
#include "materials.h"
#include "clan.h"
#include "specs.h"
#include "flow_room.h"
#include "smokes.h"
#include "paths.h"
#include "login.h"
#include "bomb.h"
#include "guns.h"
#include "fight.h"
#include "defs.h"
#include "tokenizer.h"
#include "tmpstr.h"
#include "accstr.h"
#include "interpreter.h"
#include "utils.h"
#include "player_table.h"
#include "quest.h"
#include "ban.h"
#include "boards.h"
#include "language.h"
#include "prog.h"
#include "mobile_map.h"
#include "object_map.h"
#include "house.h"
#include "editor.h"

/*   external vars  */
extern struct obj_data *object_list;
extern struct descriptor_data *descriptor_list;
extern struct Creature *mob_proto;
//extern struct obj_data *obj_proto;
extern struct zone_data *zone_table;
extern int top_of_zone_table;
extern int restrict;
extern int top_of_world;
extern int log_cmds;
extern int olc_lock;
extern int lunar_day;
extern int quest_status;
extern struct Creature *combat_list;    /* head of list of fighting chars */
extern int shutdown_count;
extern int shutdown_mode;
extern int mini_mud;
extern int current_mob_idnum;
extern struct last_command_data last_cmd[NUM_SAVE_CMDS];
extern const char *language_names[];
extern const char *instrument_types[];
extern const char *zone_pk_flags[];


char *how_good(int percent);
extern char *prac_types[];
int spell_sort_info[MAX_SPELLS + 1];
int skill_sort_info[MAX_SKILLS - MAX_SPELLS + 1];
int has_mail(long idnum);
int prototype_obj_value(struct obj_data *obj);
int choose_material(struct obj_data *obj);
int set_maxdamage(struct obj_data *obj);

long asciiflag_conv(char *buf);
void build_player_table(void);
void show_social_messages(struct Creature *ch, char *arg);
void autosave_zones(int SAVE_TYPE);
void perform_oset(struct Creature *ch, struct obj_data *obj_p,
    char *argument, byte subcmd);
void do_show_objects(struct Creature *ch, char *value, char *arg);
void do_show_mobiles(struct Creature *ch, char *value, char *arg);
void show_searches(struct Creature *ch, char *value, char *arg);
void show_rooms(struct Creature *ch, char *value, char *arg);
void do_zone_cmdlist(struct Creature *ch, struct zone_data *zone, char *arg);
const char *stristr(const char *haystack, const char *needle);

int parse_char_class(char *arg);
void retire_trails(void);
float prac_gain(struct Creature *ch, int mode);
int skill_gain(struct Creature *ch, int mode);
void list_obj_to_char(struct obj_data *list, struct Creature *ch, int mode,
    bool show);
void save_quests(); // quests.cc - saves quest data
void save_all_players();
int do_freeze_char(char *argument, Creature *vict, Creature *ch);
void verify_tempus_integrity(Creature *ch);
void do_stat_obj_tmp_affs(struct Creature *ch, struct obj_data *obj);

ACMD(do_equipment);

static const char *logtypes[] = {
    "off", "brief", "normal", "complete", "\n"
};


void
show_char_class_skills(struct Creature *ch, int con, int immort, int bits)
{
    int lvl, skl;
    bool found;
    char *tmp;

	acc_string_clear();

    acc_strcat("The ",
        class_names[con],
        " class can learn the following ",
        IS_SET(bits, SPELL_BIT) ? "spells" :
            IS_SET(bits, TRIG_BIT) ? "triggers" :
            IS_SET(bits, ZEN_BIT) ? "zens" :
            IS_SET(bits, ALTER_BIT) ? "alterations" : 
            IS_SET(bits, SONG_BIT) ? "songs" : "skills",
        ":\r\n", NULL);

    if (GET_LEVEL(ch) < LVL_IMMORT)
        acc_strcat("Lvl    Skill\r\n", NULL);
    else
        acc_strcat("Lvl  Number   Skill           Mana: Max  Min  Chn  Flags\r\n", NULL);

    for (lvl = 1; lvl < LVL_AMBASSADOR; lvl++) {
        found = false;
        for (skl = 1; skl < MAX_SKILLS; skl++) {
            /* pre-prune the list */
            if (GET_LEVEL(ch) < LVL_IMMORT) {
                if (!bits && skl < MAX_SPELLS)
                    continue;
                if (bits && skl >= MAX_SPELLS)
                    break;
            }
            if (spell_info[skl].min_level[con] == lvl &&
                (immort || GET_REMORT_GEN(ch) >= spell_info[skl].gen[con])) {

                if (!found)
                    acc_sprintf(" %-2d", lvl);
                else
                    acc_strcat("   ", NULL);

                if (GET_LEVEL(ch) < LVL_IMMORT)
                    acc_strcat("    ", NULL);
                else
                    acc_sprintf(" - %3d. ", skl);

                if (spell_info[skl].gen[con]) {
                    // This is wasteful, but it looks a lot better to have
                    // the gen after the spell.  The trick is that we want it
                    // to be yellow, but printf doesn't recognize the existence
                    // of escape codes for purposes of padding.
                    int len;

                    tmp = tmp_sprintf("%s (gen %d)", spell_to_str(skl),
                        spell_info[skl].gen[con]);
                    len = strlen(tmp);
                    if (len > 33)
                        len = 33;
                    acc_sprintf("%s%s %s(gen %d)%s%s",
                        CCGRN(ch, C_NRM), spell_to_str(skl), CCYEL(ch, C_NRM),
                        spell_info[skl].gen[con], CCNRM(ch, C_NRM),
                        tmp_pad(' ', 33 - len));
                } else {
                    acc_sprintf("%s%-33s%s",
                        CCGRN(ch, C_NRM), spell_to_str(skl), CCNRM(ch, C_NRM));
                }

                if (GET_LEVEL(ch) >= LVL_IMMORT) {
                    sprintbit(spell_info[skl].routines, spell_bits, buf2);
                    acc_sprintf("%3d  %3d  %2d   %s%s%s",
                        spell_info[skl].mana_max,
                        spell_info[skl].mana_min, spell_info[skl].mana_change,
                        CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
                }
                
                acc_strcat("\r\n", NULL);
                found = true;
            }
        }
    }
    page_string(ch->desc, acc_get_string());
}

void
list_residents_to_char(struct Creature *ch, int town)
{
    struct descriptor_data *d;
    byte found = 0;

	acc_string_clear();
    if (town < 0) {
        acc_strcat("       HOMETOWNS\r\n", NULL);
        for (d = descriptor_list; d; d = d->next) {
            if (!d->creature || !can_see_creature(ch, d->creature))
                continue;
            acc_sprintf("%s%-20s%s -- %s%-30s%s\r\n",
                GET_LEVEL(d->creature) >= LVL_AMBASSADOR ? CCYEL(ch,
                    C_NRM) : "", GET_NAME(d->creature),
                GET_LEVEL(d->creature) >= LVL_AMBASSADOR ? CCNRM(ch,
                    C_NRM) : "", CCCYN(ch, C_NRM),
                home_towns[(int)GET_HOME(d->creature)], CCNRM(ch, C_NRM));
        }
    } else {
        acc_sprintf("On-line residents of %s.\r\n", home_towns[town]);
        for (d = descriptor_list; d; d = d->next) {
            if (!d->creature || !can_see_creature(ch, d->creature))
                continue;
            if (GET_HOME(d->creature) == town) {
                acc_sprintf("%s%-20s%s\r\n",
                    GET_LEVEL(d->creature) >= LVL_AMBASSADOR ? CCYEL(ch,
                        C_NRM) : "", GET_NAME(d->creature),
                    GET_LEVEL(d->creature) >= LVL_AMBASSADOR ? CCNRM(ch,
                        C_NRM) : "");
                found = 1;
            }
        }
        if (!found)
            acc_strcat("None online.\r\n", NULL);
    }
    page_string(ch->desc, acc_get_string());
    return;
}


ACMD(do_echo)
{
    char *mort_see;
    char *imm_see;

    skip_spaces(&argument);

    if (!*argument) {
        send_to_char(ch, "Yes.. but what?\r\n");
        return;
    }

    if (subcmd == SCMD_EMOTE) {
        mort_see = tmp_sprintf("$n%s%s", (*argument == '\'') ? "":" ",
                               act_escape(argument));

		act(mort_see, FALSE, ch, 0, 0, TO_CHAR);
        act(mort_see, FALSE, ch, 0, 0, TO_ROOM);
    } else {
        mort_see = tmp_strdup(argument);
        imm_see = tmp_sprintf("[$n] %s", argument);

        CreatureList::iterator it = ch->in_room->people.begin();
        for (; it != ch->in_room->people.end(); ++it) {
            if (GET_LEVEL((*it)) > GET_LEVEL(ch))
                act(imm_see, FALSE, ch, 0, (*it), TO_VICT);
            else
                act(mort_see, FALSE, ch, 0, (*it), TO_VICT);
        }
    }
}


ACMD(do_send)
{
    struct Creature *vict;

    half_chop(argument, arg, buf);

    if (!*arg) {
        send_to_char(ch, "Send what to who?\r\n");
        return;
    }
    if (!(vict = get_char_vis(ch, arg))) {
        send_to_char(ch, NOPERSON);
        return;
    }
    send_to_char(vict, "%s\r\n", buf);
	send_to_char(ch, "You send '%s' to %s.\r\n", buf, GET_NAME(vict));
	slog("(GC) %s send %s %s", GET_NAME(ch), GET_NAME(vict), buf);
}



/* take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93 */
struct room_data *
find_target_room(struct Creature *ch, char *rawroomstr)
{
    int tmp;
    struct room_data *location;
    struct Creature *target_mob;
    struct obj_data *target_obj;
    char roomstr[MAX_INPUT_LENGTH];

    one_argument(rawroomstr, roomstr);

    if (!*roomstr) {
        send_to_char(ch, "You must supply a room number or name.\r\n");
        return NULL;
    }
    if (isdigit(*roomstr) && !strchr(roomstr, '.')) {
        tmp = atoi(roomstr);
        if (!(location = real_room(tmp))) {
            send_to_char(ch, "No room exists with that number.\r\n");
            return NULL;
        }
	} else if (is_abbrev(roomstr, "previous")) {
		int bottom_room = ch->in_room->zone->number * 100;
		location = NULL;
		for (tmp = ch->in_room->number - 1;tmp >= bottom_room && !location;tmp--)
			location = real_room(tmp);
		if (!location) {
			send_to_char(ch, "No previous room exists in this zone!\r\n");
			return NULL;
		}
	} else if (is_abbrev(roomstr, "next")) {
		int top_room = ch->in_room->zone->top;

		location = NULL;
		for (tmp = ch->in_room->number + 1;tmp <= top_room && !location;tmp++)
			location = real_room(tmp);
		if (!location) {
			send_to_char(ch, "No next room exists in this zone!\r\n");
			return NULL;
		}
    } else if ((target_mob = get_char_vis(ch, roomstr))) {
        if (GET_LEVEL(ch) < LVL_SPIRIT && GET_MOB_SPEC(target_mob) == fate) {
            send_to_char(ch, "%s's magic repulses you.\r\n", GET_NAME(target_mob));
            return NULL;
        }
        location = target_mob->in_room;
    } else if ((target_obj = get_obj_vis(ch, roomstr))) {
        if (target_obj->in_room != NULL) {
            location = target_obj->in_room;
        } else {
            send_to_char(ch, "That object is not available.\r\n");
            return NULL;
        }
    } else {
        send_to_char(ch, "No such creature or object around.\r\n");
        return NULL;
    }

    /* a location has been found -- if you're < GRGOD, check restrictions. */
    if (!Security::isMember(ch, "WizardFull")) {
        if (location->zone->number == 12 && GET_LEVEL(ch) < LVL_AMBASSADOR) {
            send_to_char(ch, "You can't go there.\r\n");
            return NULL;
        }
        if (ROOM_FLAGGED(location, ROOM_GODROOM)) {
            send_to_char(ch, "You are not godly enough to use that room!\r\n");
            return NULL;
        }
        if (ROOM_FLAGGED(location, ROOM_HOUSE) &&
            !Housing.canEnter(ch, location->number)) {
            send_to_char(ch, "That's private property -- no trespassing!\r\n");
            return NULL;
        }
    }
    return location;
}



ACMD(do_at)
{
    char command[MAX_INPUT_LENGTH];
    struct room_data *location = NULL, *original_loc = NULL;

    half_chop(argument, buf, command);
    if (!*buf) {
        send_to_char(ch, "You must supply a room number or a name.\r\n");
        return;
    }

    if (!*command) {
        send_to_char(ch, "What do you want to do there?\r\n");
        return;
    }

    if (!(location = find_target_room(ch, buf)))
        return;

    /* a location has been found. */
    original_loc = ch->in_room;
    char_from_room(ch,false);
    char_to_room(ch, location,false);
    command_interpreter(ch, command);

    /* check if the char is still there */
    if (ch->in_room == location) {
        char_from_room(ch,false);
        char_to_room(ch, original_loc,false);
    }
}

ACMD(do_distance)
{
    struct room_data *location = NULL, *tmp = ch->in_room;
    int steps = -1;

    if ((location = find_target_room(ch, argument)) == NULL)
        return;

    steps = find_distance(tmp, location);

    if (steps < 0)
        send_to_char(ch, "There is no valid path to room %d.\r\n",
            location->number);
    else
        send_to_char(ch, "Room %d is %d steps away.\r\n", location->number, steps);

}

void
perform_goto(Creature *ch, room_data *room, bool allow_follow)
{
    room_data *was_in = NULL;
    char *msg;

    if (!Housing.canEnter(ch, room->number) ||
        !clan_house_can_enter(ch, room) ||
        (GET_LEVEL(ch) < LVL_SPIRIT && ROOM_FLAGGED(room, ROOM_DEATH))) {
        send_to_char(ch, "You cannot enter there.\r\n");
        return;
    }

    was_in = ch->in_room;

    if (POOFOUT(ch)) {
        if (strstr(POOFOUT(ch), "$$n")) {
            msg = tmp_gsub(POOFOUT(ch), "$$n", "$n");
        }
        else {
            msg = tmp_sprintf("$n %s", POOFOUT(ch));
        }
    }
    else
        msg = "$n disappears in a puff of smoke.";

    act(msg, TRUE, ch, 0, 0, TO_ROOM);
    char_from_room(ch,false);
    char_to_room(ch, room,false);
    if (room->isOpenAir())
        ch->setPosition(POS_FLYING);

    if (POOFIN(ch)) {
        if (strstr(POOFIN(ch), "$$n")) {
            msg = tmp_gsub(POOFIN(ch), "$$n", "$n");
        }
        else {
            msg = tmp_sprintf("$n %s", POOFIN(ch));
        }
    }
    else
        msg = "$n appears with an ear-splitting bang.";


    act(msg, TRUE, ch, 0, 0, TO_ROOM);
    look_at_room(ch, ch->in_room, 0);

    if (allow_follow && ch->followers) {
        struct follow_type *k, *next;

        for (k = ch->followers; k; k = next) {
            next = k->next;
            if (was_in == k->follower->in_room &&
                    GET_LEVEL(k->follower) >= LVL_AMBASSADOR &&
                    !PLR_FLAGGED(k->follower, PLR_OLC | PLR_WRITING | PLR_MAILING) &&
                    can_see_creature(k->follower, ch))
                perform_goto(k->follower, room, true);
        }
    }

    if (ROOM_FLAGGED(ch->in_room, ROOM_DEATH) && GET_LEVEL(ch) < LVL_IMPL) {
        slog("(GC) %s goto deathtrap [%d] %s.", GET_NAME(ch),
            ch->in_room->number, ch->in_room->name);
    }
}


ACMD(do_goto)
{
    struct room_data *location = NULL;

    if PLR_FLAGGED
        (ch, PLR_AFK)
            REMOVE_BIT(PLR_FLAGS(ch), PLR_AFK);

    if ((location = find_target_room(ch, argument)) == NULL)
        return;

    perform_goto(ch, location, true);
}



ACMD(do_trans)
{
    struct descriptor_data *i;
    struct room_data *was_in;
    struct Creature *victim;
	char *name_str;

	if (GET_LEVEL(ch) < LVL_IMMORT
			|| !(Security::isMember(ch, "WizardBasic")
				|| Security::isMember(ch, "Questor")
                || Security::isMember(ch, "AdminBasic"))) {
		send_to_char(ch, "Sorry, but you can't do that here!\r\n");
		return;
	}

    name_str = tmp_getword(&argument);

    if (!*name_str) {
        send_to_char(ch, "Whom do you wish to transfer?\r\n");
		return;
    }

	if (!str_cmp("all", name_str)) {
		// Transfer all (below ch's level)
        if (GET_LEVEL(ch) < LVL_GRGOD) {
            send_to_char(ch, "I think not.\r\n");
            return;
        }

        for (i = descriptor_list; i; i = i->next)
            if (STATE(i) == CXN_PLAYING && i->creature && i->creature != ch) {
                victim = i->creature;
                if (GET_LEVEL(victim) >= GET_LEVEL(ch))
                    continue;
                act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0,
                    TO_ROOM);

                was_in = victim->in_room;

                char_from_room(victim,false);
                char_to_room(victim, ch->in_room,false);

                act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0,
                    TO_ROOM);
                act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
                look_at_room(victim, victim->in_room, 0);

            }
        send_to_char(ch, OK);
		return;
    }

	while (*name_str) {
		victim = get_char_vis(ch, name_str);

		if (!victim) {
			send_to_char(ch, "You can't detect any '%s'\r\n", name_str);
		} else if (victim == ch) {
			send_to_char(ch, "Sure, sure.  Try to transport yourself.\r\n");
		} else if ((GET_LEVEL(ch) < GET_LEVEL(victim)) && !IS_NPC(victim)) {
			send_to_char(ch, "%s is far too powerful for you to transport.\r\n",
				GET_NAME(victim));
		} else if (ch->in_room->isOpenAir() && victim->getPosition() != POS_FLYING) {
			send_to_char(ch, "You are in midair and %s isn't flying.\r\n",
				GET_NAME(victim));
		} else {
			act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0,
				TO_ROOM);
			was_in = victim->in_room;

			char_from_room(victim,false);
			char_to_room(victim, ch->in_room,false);
			act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0,
				TO_ROOM);
			act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
			look_at_room(victim, victim->in_room, 0);

			if (victim->followers) {
				struct follow_type *k, *next;

				for (k = victim->followers; k; k = next) {
					next = k->next;
					if (was_in == k->follower->in_room &&
							GET_LEVEL(k->follower) >= LVL_AMBASSADOR &&
							!PLR_FLAGGED(k->follower, PLR_OLC | PLR_WRITING | PLR_MAILING) &&
							can_see_creature(k->follower, victim))
						perform_goto(k->follower, ch->in_room, true);
				}
			}

			slog("(GC) %s has transferred %s to %s.", GET_NAME(ch),
				GET_NAME(victim), ch->in_room->name);
		}
		name_str = tmp_getword(&argument);
	}
}



ACMD(do_teleport)
{
    struct Creature *victim;
    struct room_data *target;

    two_arguments(argument, buf, buf2);

    if (!*buf)
        send_to_char(ch, "Whom do you wish to teleport?\r\n");
    else if (!(victim = get_char_vis(ch, buf))) {
        send_to_char(ch, NOPERSON);
    } else if (victim == ch)
        send_to_char(ch, "Use 'goto' to teleport yourself.\r\n");
    else if (GET_LEVEL(victim) >= GET_LEVEL(ch))
        send_to_char(ch, "Maybe you shouldn't do that.\r\n");
    else if (!*buf2)
        send_to_char(ch, "Where do you wish to send this person?\r\n");
    else if ((target = find_target_room(ch, buf2)) != NULL) {
        send_to_char(ch, OK);
        act("$n disappears in a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
        char_from_room(victim,false);
        char_to_room(victim, target,false);
        act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
        act("$n has teleported you!", FALSE, ch, 0, victim, TO_VICT);
        look_at_room(victim, victim->in_room, 0);

        slog("(GC) %s has teleported %s to [%d] %s.", GET_NAME(ch),
            GET_NAME(victim), victim->in_room->number, victim->in_room->name);
    }
}



ACMD(do_vnum)
{
	const char *mode;

    mode = tmp_getword(&argument);
    if (!*mode || !*argument ||
		(!is_abbrev(mode, "mob") && !is_abbrev(mode, "obj"))) {
        send_to_char(ch, "Usage: vnum { obj | mob } <name>\r\n");
        return;
    }
    if (is_abbrev(mode, "mob"))
        if (!vnum_mobile(argument, ch))
            send_to_char(ch, "No mobiles by that name.\r\n");

    if (is_abbrev(mode, "obj"))
        if (!vnum_object(argument, ch))
            send_to_char(ch, "No objects by that name.\r\n");
}

#define CHARADD(sum,var) if (var) {sum += strlen(var) +1;}
void
do_stat_memory(struct Creature *ch)
{
    int sum = 0, total = 0;
    int i = 0, j = 0;
    struct alias_data *al;
    struct extra_descr_data *tmpdesc;
    struct special_search_data *tmpsearch;
    struct affected_type *af;
    struct descriptor_data *desc = NULL;
    struct follow_type *fol;
    struct Creature *chars, *mob;
    struct zone_data *zone;
    struct room_data *rm;

    send_to_char(ch, "Current memory usage:\r\n");

    sum = top_of_world * sizeof(struct room_data);
    for (zone = zone_table; zone; zone = zone->next) {
        for (rm = zone->world; rm; rm = rm->next) {

            CHARADD(sum, rm->name);
            CHARADD(sum, rm->description);
            CHARADD(sum, rm->sounds);

            tmpdesc = rm->ex_description;
            while (tmpdesc) {
                sum += sizeof(struct extra_descr_data);
                CHARADD(sum, tmpdesc->keyword);
                CHARADD(sum, tmpdesc->description);
                tmpdesc = tmpdesc->next;
            }

            for (j = 0; j < NUM_OF_DIRS; j++) {
                if (rm->dir_option[j]) {
                    sum += sizeof(struct room_direction_data);
                    CHARADD(sum, rm->dir_option[j]->general_description);
                    CHARADD(sum, rm->dir_option[j]->keyword);
                }
            }

            tmpsearch = rm->search;
            while (tmpsearch) {
                sum += sizeof(struct special_search_data);
                tmpsearch = tmpsearch->next;
            }
        }
    }
    total = sum;
    send_to_char(ch, "%s  world structs: %9d  (%d)\r\n", buf, sum, i);

    sum = mobilePrototypes.size() * (sizeof(struct Creature));

    MobileMap::iterator mit = mobilePrototypes.begin();
    for (; mit != mobilePrototypes.end(); ++mit) {
        mob = mit->second;
        CHARADD(sum, mob->player.name);
        CHARADD(sum, mob->player.short_descr);
        CHARADD(sum, mob->player.long_descr);
        CHARADD(sum, mob->player.description);
        CHARADD(sum, mob->player.title);

        if (mob->player_specials && i == 0) {
            sum += sizeof(struct player_special_data);
            CHARADD(sum, mob->player_specials->poofin);
            CHARADD(sum, mob->player_specials->poofout);
            al = mob->player_specials->aliases;
            while (al) {
                sum += sizeof(struct alias_data);
                CHARADD(sum, al->alias);
                CHARADD(sum, al->replacement);
                al = al->next;
            }
        }
        af = mob->affected;
        while (af) {
            sum += sizeof(struct affected_type);
            af = af->next;
        }
        if (desc) {
            sum += sizeof(struct descriptor_data);
        }
        fol = mob->followers;
        while (fol) {
            sum += sizeof(struct follow_type);
            fol = fol->next;
        }
    }
    total += sum;
    send_to_char(ch, "%s     mob protos: %9d  (%d)\r\n", buf, sum, i);

    sum = 0;
    i = 0;
    CreatureList::iterator cit = characterList.begin();
    for (; cit != characterList.end(); ++cit) {
        chars = *cit;
        if (!IS_NPC(chars))
            continue;
        i++;
        sum += sizeof(struct Creature);

        af = chars->affected;
        while (af) {
            sum += sizeof(struct affected_type);
            af = af->next;
        }
        if (desc) {
            sum += sizeof(struct descriptor_data);
        }
        fol = chars->followers;
        while (fol) {
            sum += sizeof(struct follow_type);
            fol = fol->next;
        }
    }
    total += sum;
    send_to_char(ch, "%s        mobiles: %9d  (%d)\r\n", buf, sum, i);

    sum = 0;
    i = 0;
    cit = characterList.begin();
    for (; cit != characterList.end(); ++cit) {
        chars = *cit;
        if (IS_NPC(chars))
            continue;
        i++;
        sum += sizeof(struct Creature);

        CHARADD(sum, chars->player.name);
        CHARADD(sum, chars->player.short_descr);
        CHARADD(sum, chars->player.long_descr);
        CHARADD(sum, chars->player.description);
        CHARADD(sum, chars->player.title);

        if (chars->player_specials) {
            sum += sizeof(struct player_special_data);
            CHARADD(sum, chars->player_specials->poofin);
            CHARADD(sum, chars->player_specials->poofout);
            al = chars->player_specials->aliases;
            while (al) {
                sum += sizeof(struct alias_data);
                CHARADD(sum, al->alias);
                CHARADD(sum, al->replacement);
                al = al->next;
            }
        }
        af = chars->affected;
        while (af) {
            sum += sizeof(struct affected_type);
            af = af->next;
        }
        if (desc) {
            sum += sizeof(struct descriptor_data);
        }
        fol = chars->followers;
        while (fol) {
            sum += sizeof(struct follow_type);
            fol = fol->next;
        }
    }
    total += sum;

    send_to_char(ch, "%s        players: %9d  (%d)\r\n", buf, sum, i);
    send_to_char(ch, "%s               ___________\r\n", buf);
    send_to_char(ch, "%s         total   %9d\r\n", buf, total);
}

#undef CHARADD

void
do_stat_zone(struct Creature *ch, struct zone_data *zone)
{
    struct obj_data *obj;
    struct descriptor_data *plr;
    struct room_data *rm = NULL;
    struct special_search_data *srch = NULL;
    int numm = 0, numo = 0, nump = 0, numr = 0, nums = 0, av_lev = 0,
        numm_proto = 0, numo_proto = 0, av_lev_proto = 0, numur = 0;

    send_to_char(ch, "Zone #%s%d: %s%s%s\r\n",
                 CCYEL(ch, C_NRM), zone->number,
                 CCCYN(ch, C_NRM), zone->name, CCNRM(ch, C_NRM));
    send_to_char(ch, "Authored by: %s\r\n", (zone->author) ? zone->author:"<none>");
    send_to_char(ch,
        "Rooms: [%d-%d]  Respawn pt: [%d]  Reset Mode: %s\r\n",
        zone->number * 100, zone->top,
        zone->respawn_pt, reset_mode[zone->reset_mode]);

    send_to_char(ch, "TimeFrame: [%s]  Plane: [%s]   ",
        time_frames[zone->time_frame], planes[zone->plane]);

    CreatureList::iterator cit = characterList.begin();
    for (; cit != characterList.end(); ++cit)
        if (IS_NPC((*cit)) && (*cit)->in_room && (*cit)->in_room->zone == zone) {
            numm++;
            av_lev += GET_LEVEL((*cit));
        }

    if (numm)
        av_lev /= numm;
    MobileMap::iterator mit = mobilePrototypes.begin();
    Creature *mob;
    for (; mit != mobilePrototypes.end(); ++mit) {
        mob = mit->second;
        if (GET_MOB_VNUM(mob) >= zone->number * 100 &&
            GET_MOB_VNUM(mob) <= zone->top && IS_NPC(mob)) {
            numm_proto++;
            av_lev_proto += GET_LEVEL(mob);
        }
	}

    if (numm_proto)
        av_lev_proto /= numm_proto;


    send_to_char(ch, "Owner: %s  ", (playerIndex.getName(zone->owner_idnum) ?
            playerIndex.getName(zone->owner_idnum) : "<none>"));
    send_to_char(ch, "Co-Owner: %s\r\n", (playerIndex.getName(zone->co_owner_idnum) ?
            playerIndex.getName(zone->co_owner_idnum) : "<none>"));
    send_to_char(ch, "Hours: [%3d]  Years: [%3d]  Idle:[%3d]  Lifespan: [%d]  Age: [%d]\r\n",
        zone->hour_mod, zone->year_mod, zone->idle_time,
		zone->lifespan, zone->age);

    send_to_char(ch,
        "Sun: [%s (%d)] Sky: [%s (%d)] Moon: [%s (%d)] "
        "Pres: [%3d] Chng: [%3d]\r\n",
        sun_types[(int)zone->weather->sunlight],
        zone->weather->sunlight, sky_types[(int)zone->weather->sky],
        zone->weather->sky,
        moon_sky_types[(int)zone->weather->moonlight],
        zone->weather->moonlight,
        zone->weather->pressure, zone->weather->change);

    sprintbit(zone->flags, zone_flags, buf2);
    send_to_char(ch, "Flags: %s%s%s%s\r\n", CCGRN(ch, C_NRM), buf2, 
                 zone_pk_flags[zone->getPKStyle()], CCNRM(ch, C_NRM));
	
	if (zone->min_lvl)
		send_to_char(ch, "Target lvl/gen: [% 2d/% 2d - % 2d/% 2d]\r\n",
			zone->min_lvl, zone->min_gen, zone->max_lvl, zone->max_gen);

	if (zone->public_desc)
		send_to_char(ch, "Public Description:\r\n%s", zone->public_desc);
	if (zone->private_desc)
		send_to_char(ch, "Private Description:\r\n%s", zone->private_desc);

    for (obj = object_list; obj; obj = obj->next)
        if (obj->in_room && obj->in_room->zone == zone)
            numo++;

    ObjectMap::iterator oi = objectPrototypes.begin();
    for (; oi != objectPrototypes.end(); ++oi) {
        obj = oi->second;
        if (GET_OBJ_VNUM(obj) >= zone->number * 100 &&
            GET_OBJ_VNUM(obj) <= zone->top)
            numo_proto++;
    }

    for (plr = descriptor_list; plr; plr = plr->next)
        if (plr->creature && plr->creature->in_room &&
            plr->creature->in_room->zone == zone)
            nump++;

    for (rm = zone->world, numr = 0, nums = 0; rm; numr++, rm = rm->next) {
        if (!rm->description)
            numur++;
        for (srch = rm->search; srch; nums++, srch = srch->next);
    }

    send_to_char(ch, "\r\nZone Stats :-\r\n"
        "  mobs in zone : %-3d, %-3d protos;   objs in zone  : %-3d, %-3d protos;\r\n"
        "  players in zone: (%3d) %3d   rooms in zone: %-3d   undescripted rooms: %-3d\r\n"
        "  search in zone: %d\r\n  usage count: %d\r\n"
        "  Avg. Level [%s%d%s]real, [%s%d%s] proto\r\n\r\n",
        numm, numm_proto, numo, numo_proto,
        zone->num_players, nump, numr, numur, nums, zone->enter_count,
        CCGRN(ch, C_NRM), av_lev,
        CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), av_lev_proto, CCNRM(ch, C_NRM));
}

void
do_stat_trails(struct Creature *ch)
{

    struct room_trail_data *trail = NULL;
    int i;
    time_t mytime, timediff;

	if (!ch->in_room->trail) {
		send_to_char(ch, "No trails exist within this room.\r\n");
		return;
	}
    mytime = time(0);
	acc_string_clear();
    for (i = 0, trail = ch->in_room->trail; trail; trail = trail->next) {
        timediff = mytime - trail->time;
        sprintbit(trail->flags, trail_flags, buf2);
        acc_sprintf(" [%2d] -- Name: '%s', (%s), Idnum: [%5d]\r\n"
            "         Time Passed: %ld minutes, %ld seconds.\r\n"
            "         From dir: %s, To dir: %s, Track: [%2d]\r\n"
            "         Flags: %s\r\n",
            ++i, trail->name,
            get_char_in_world_by_idnum(trail->idnum) ? "in world" : "gone",
            trail->idnum, timediff / 60, timediff % 60,
            (trail->from_dir >= 0) ? dirs[(int)trail->from_dir] : "NONE",
            (trail->to_dir >= 0) ? dirs[(int)trail->to_dir] : "NONE",
            trail->track, buf2);
    }

    page_string(ch->desc, acc_get_string());
}

void
acc_format_prog(Creature *ch, char *prog)
{
    const char *line_color = NULL;

    acc_sprintf("Prog:\r\n");

    int line_num = 1;
    for (char *line = tmp_getline(&prog);
         line;
         line = tmp_getline(&prog), line_num++) {

        // Line number looks like TEDII
        acc_sprintf("%s%3d%s] ",
                    CCYEL(ch, C_NRM),
                    line_num,
                    CCBLU(ch, C_NRM));
        char *c = line;
        skip_spaces(&c);
        if (!line_color) {
            // We don't know the line color of this line
            if (*c == '-') {
                // comment
                line_color = CCBLU(ch, C_NRM);
            } else if (*c == '*') {
                // prog command
                c++;
                char *cmd = tmp_getword(&c);
                if (!strcasecmp(cmd, "before")
                    || !strcasecmp(cmd, "handle")
                    || !strcasecmp(cmd, "after"))
                    line_color = CCCYN(ch, C_NRM);
                else if (!strcasecmp(cmd, "require")
                         || !strcasecmp(cmd, "unless")
                         || !strcasecmp(cmd, "randomly")
                         || !strcasecmp(cmd, "or"))
                    line_color = CCMAG(ch, C_NRM);
                else
                    line_color = CCYEL(ch, C_NRM);
            } else if (*c) {
                // mob command
                line_color = CCNRM(ch, C_NRM);
            } else {
                // blank line
                line_color = "";
            }
        }

        // Append the line
        acc_sprintf("%s%s\r\n", line_color, line);

        // If not a continued line, zero out the line color
        if (line[strlen(line) - 1] != '\\')
            line_color = NULL;
    }
}

void
do_stat_room(struct Creature *ch, char *roomstr)
{
    int tmp;
    struct extra_descr_data *desc;
    struct room_data *rm = ch->in_room;
    struct room_affect_data *aff = NULL;
    struct special_search_data *cur_search = NULL;
    int i, found = 0;
    struct obj_data *j = 0;
    struct Creature *k = 0;

    if (roomstr && *roomstr) {
        if (isdigit(*roomstr) && !strchr(roomstr, '.')) {
            tmp = atoi(roomstr);
            if (!(rm = real_room(tmp))) {
                send_to_char(ch, "No room exists with that number.\r\n");
                return;
            }
        } else {
            send_to_char(ch, "No room exists with that number.\r\n");
            return;
        }
    }

    acc_string_clear();
    acc_sprintf("Room name: %s%s%s\r\n", CCCYN(ch, C_NRM), rm->name,
                CCNRM(ch, C_NRM));

    acc_sprintf("Zone: [%s%3d%s], VNum: [%s%5d%s], Type: %s, Lighting: [%d], Max: [%d]\r\n",
                CCYEL(ch, C_NRM),
                rm->zone->number,
                CCNRM(ch, C_NRM),
                CCGRN(ch, C_NRM),
                rm->number,
                CCNRM(ch, C_NRM),
                strlist_aref(rm->sector_type, sector_types),
                rm->light, rm->max_occupancy);

    acc_sprintf("SpecProc: %s, Flags: %s\r\n",
        (rm->func == NULL) ? "None" :
        (i = find_spec_index_ptr(rm->func)) < 0 ? "Exists" :
            spec_list[i].tag, tmp_printbits(rm->room_flags, room_bits));

    if (FLOW_SPEED(rm)) {
        acc_sprintf("Flow (Direction: %s, Speed: %d, Type: %s (%d)).\r\n",
            dirs[(int)FLOW_DIR(rm)], FLOW_SPEED(rm),
            flow_types[(int)FLOW_TYPE(rm)], (int)FLOW_TYPE(rm));
    }

    acc_sprintf("Description:\r\n%s",
                (rm->description) ? rm->description:"  None.\r\n");

    if (rm->sounds) {
        acc_sprintf("%sSound:%s\r\n%s", CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
            rm->sounds);
    }

    if (rm->ex_description) {
        acc_sprintf("Extra descs:%s", CCCYN(ch, C_NRM));
        for (desc = rm->ex_description; desc; desc = desc->next)
            acc_sprintf(" %s%s", desc->keyword, (desc->next) ? ";":"");
        acc_strcat(CCNRM(ch, C_NRM), "\r\n", NULL);
    }

    if (rm->affects) {
        acc_sprintf("Room affects:\r\n");
        for (aff = rm->affects; aff; aff = aff->next) {
            if (aff->type == RM_AFF_FLAGS) {
                acc_sprintf("  Room flags: %s%s%s, duration[%d]\r\n",
                            CCCYN(ch, C_NRM),
                            tmp_printbits(aff->flags, room_bits),
                            CCNRM(ch, C_NRM),
                            aff->duration);
            } else if (aff->type < NUM_DIRS) {
                acc_sprintf("  Door flags exit [%s], %s, duration[%d]\r\n",
                            dirs[(int)aff->type],
                            tmp_printbits(aff->flags, exit_bits),
                            aff->duration);
            } else
                acc_sprintf("  ERROR: Type %d.\r\n", aff->type);

            acc_sprintf("  Desc: %s\r\n",
                        aff->description ? aff->description : "None.");
        }
    }

    acc_sprintf("Chars present:%s", CCYEL(ch, C_NRM));

    CreatureList::iterator it = rm->people.begin();
    CreatureList::iterator nit = it;
    for (found = 0; it != rm->people.end(); ++it) {
        ++nit;
        k = *it;
        if (!can_see_creature(ch, k))
            continue;
        acc_sprintf("%s %s(%s)", found++ ? "," : "", GET_NAME(k),
                    (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")));
    }
    acc_strcat(CCNRM(ch, C_NRM), "\r\n", NULL);

    if (rm->contents) {
        acc_sprintf("Contents:%s", CCGRN(ch, C_NRM));
        for (found = 0, j = rm->contents; j; j = j->next_content) {
            if (!can_see_object(ch, j))
                continue;
            acc_sprintf("%s %s", found++ ? "," : "", j->name);
        }
        acc_strcat(CCNRM(ch, C_NRM), NULL);
    }

    if (rm->search) {
        acc_sprintf("SEARCHES:\r\n");
        for (cur_search = rm->search;
             cur_search;
             cur_search = cur_search->next)
            acc_format_search_data(ch, rm, cur_search);
    }

    for (i = 0; i < NUM_OF_DIRS; i++) {
        if (ABS_EXIT(rm, i)) {
            acc_sprintf("Exit %s%-5s%s:  To: [%s%s%s], Key: [%5d], Keywrd: %s, Type: %s\r\n",
                        CCCYN(ch, C_NRM),
                        dirs[i],
                        CCNRM(ch, C_NRM),
                        CCCYN(ch, C_NRM),
                        ((ABS_EXIT(rm, i)->to_room) ? tmp_sprintf("%5d", ABS_EXIT(rm, i)->to_room->number):"NONE"),
                        CCNRM(ch, C_NRM),
                        ABS_EXIT(rm, i)->key,
                        ABS_EXIT(rm, i)->keyword ? ABS_EXIT(rm, i)->keyword : "None",
                        tmp_printbits(ABS_EXIT(rm, i)->exit_info, exit_bits));
            if (ABS_EXIT(rm, i)->general_description) {
                acc_strcat(ABS_EXIT(rm, i)->general_description, NULL);
            } else {
                acc_strcat("  No exit description.\r\n", NULL);
            }
        }
    }
    if (GET_ROOM_STATE(rm) && GET_ROOM_STATE(rm)->var_list) {
		prog_var *cur_var;
		acc_strcat("Prog state variables:\r\n", NULL);
		for (cur_var = GET_ROOM_STATE(rm)->var_list;cur_var;cur_var = cur_var->next) {
            acc_sprintf("     %s = '%s'\r\n", cur_var->key, cur_var->value);
        }
    }
	if (rm->prog) {
        acc_format_prog(ch, rm->prog);
	}

    page_string(ch->desc, acc_get_string());
}

void
do_stat_object(struct Creature *ch, struct obj_data *j)
{
    int i, found;
    struct extra_descr_data *desc;
    extern struct attack_hit_type attack_hit_text[];
    extern const char *egun_types[];
    struct room_data *rm = NULL;

    if (IS_OBJ_TYPE(j, ITEM_NOTE) && isname("letter", j->aliases)) {
        if (j->carried_by && GET_LEVEL(j->carried_by) > GET_LEVEL(ch)) {
            act("$n just tried to stat your mail.",
                FALSE, ch, 0, j->carried_by, TO_VICT);
            send_to_char(ch, "You're pretty brave, bucko.\r\n");
            return;
        }
        if (GET_LEVEL(ch) < LVL_GOD) {
            send_to_char(ch, "You can't stat mail.\r\n");
            return;
        }
    }

    acc_string_clear();
    acc_sprintf("Name: '%s%s%s', Aliases: %s\r\n", CCGRN(ch, C_NRM),
        ((j->name) ? j->name : "<None>"),
        CCNRM(ch, C_NRM), j->aliases);

    acc_sprintf("VNum: [%s%5d%s], Exist: [%3d/%3d], Type: %s, SpecProc: %s\r\n",
                CCGRN(ch, C_NRM),
                GET_OBJ_VNUM(j),
                CCNRM(ch, C_NRM),
                j->shared->number,
                j->shared->house_count,
                strlist_aref(GET_OBJ_TYPE(j), item_types),
                (j->shared->func) ?
                ((i = find_spec_index_ptr(j->shared->func)) < 0 ? "Exists" :
                 spec_list[i].tag) : "None");
    acc_sprintf("L-Des: %s%s%s\r\n",
                CCGRN(ch, C_NRM),
                ((j->line_desc) ? j->line_desc : "None"),
                CCNRM(ch, C_NRM));

    if (j->action_desc) {
        acc_sprintf("Action desc: %s\r\n", j->action_desc);
    }

    if (j->ex_description) {
        acc_sprintf("Extra descs:%s", CCCYN(ch, C_NRM));
        for (desc = j->ex_description; desc; desc = desc->next) {
            acc_strcat(" ", desc->keyword, ";", NULL);
        }
        acc_strcat(CCNRM(ch, C_NRM), "\r\n", NULL);
    }

    if (!j->line_desc) {
        acc_sprintf("**This object currently has no description**\r\n");
    }

	if (j->creation_time) {
		switch (j->creation_method) {
		case CREATED_ZONE:
			acc_sprintf("Created by zone #%ld on %s\r\n",
				j->creator, tmp_ctime(j->creation_time));
			break;
		case CREATED_MOB:
			acc_sprintf("Loaded onto mob #%ld on %s\r\n",
				j->creator, tmp_ctime(j->creation_time));
		case CREATED_SEARCH:
			acc_sprintf("Created by search in room #%ld on %s\r\n",
				j->creator, tmp_ctime(j->creation_time));
		case CREATED_IMM:
			acc_sprintf("Loaded by %s on %s\r\n",
				playerIndex.getName(j->creator),
				tmp_ctime(j->creation_time));
			break;
		case CREATED_PROG:
			acc_sprintf("Created by prog (mob or room #%ld) on %s\r\n",
				j->creator,
				tmp_ctime(j->creation_time));
			break;
		case CREATED_PLAYER:
			acc_sprintf("Created by player %s on %s\r\n",
				playerIndex.getName(j->creator),
				tmp_ctime(j->creation_time));
			break;
		default:
			acc_sprintf("Created on %s\r\n", tmp_ctime(j->creation_time));
			break;
		}
	}

	if (j->unique_id)
		acc_sprintf("Unique object id: %ld\r\n", j->unique_id);

	if( j->shared->owner_id != 0 ) {
		if( playerIndex.exists(j->shared->owner_id) ) {
			acc_sprintf("Oedit Owned By: %s[%ld]\r\n",
							playerIndex.getName(j->shared->owner_id),
							j->shared->owner_id );
		} else {
			acc_sprintf("Oedit Owned By: NOONE[%ld]\r\n",
							j->shared->owner_id );
		}
					
	}
    acc_sprintf("Can be worn on: ");
    acc_strcat(tmp_printbits(j->obj_flags.wear_flags, wear_bits), "\r\n", NULL);
    if (j->obj_flags.bitvector[0] ||
        j->obj_flags.bitvector[1] ||
        j->obj_flags.bitvector[2]) {
        acc_strcat("Set char bits : ", NULL);
        if (j->obj_flags.bitvector[0])
            acc_strcat(tmp_printbits(j->obj_flags.bitvector[0],
                                      affected_bits),
                       "  ", NULL);
        if (j->obj_flags.bitvector[1])
            acc_strcat(tmp_printbits(j->obj_flags.bitvector[1],
                                      affected2_bits),
                       "  ", NULL);
        if (j->obj_flags.bitvector[2])
            acc_strcat(tmp_printbits(j->obj_flags.bitvector[2],
                                     affected3_bits), NULL);
        acc_strcat("\r\n", NULL);
    }
    acc_sprintf("Extra flags : %s\r\n",
                tmp_printbits(GET_OBJ_EXTRA(j), extra_bits));
    acc_sprintf("Extra2 flags: %s\r\n",
                tmp_printbits(GET_OBJ_EXTRA2(j), extra2_bits));
    acc_sprintf("Extra3 flags: %s\r\n",
                tmp_printbits(GET_OBJ_EXTRA3(j), extra3_bits));

    acc_sprintf("Weight: %d, Cost: %d (%d), Rent: %d, Timer: %d\r\n",
        j->getWeight(), GET_OBJ_COST(j),
        prototype_obj_value(j), GET_OBJ_RENT(j), GET_OBJ_TIMER(j));

    if ((rm = where_obj(j))) {
        acc_sprintf("Absolute location: %s (%d)\r\n", rm->name, rm->number);

        acc_sprintf("In room: %s%s%s, In obj: %s%s%s, Carry: %s%s%s, Worn: %s%s%s%s, Aux: %s%s%s\r\n",
                    CCCYN(ch, C_NRM),
                    (j->in_room) ? tmp_sprintf("%d", j->in_room->number):"N",
                    CCNRM(ch, C_NRM),
                    CCGRN(ch, C_NRM),
                    (j->in_obj) ? j->in_obj->name : "N",
                    CCNRM(ch, C_NRM),
                    CCGRN(ch, C_NRM),
                    (j->carried_by) ? GET_NAME(j->carried_by):"N",
                    CCNRM(ch, C_NRM),
                    CCGRN(ch, C_NRM),
                    (j->worn_by) ? GET_NAME(j->worn_by):"N",
                    (!j->worn_by || j == GET_EQ(j->worn_by, j->worn_on)) ? 
                    "" : " (impl)",
                    CCNRM(ch, C_NRM),
                    CCGRN(ch, C_NRM),
                    (j->aux_obj) ? j->aux_obj->name:"N",
                    CCNRM(ch, C_NRM));
    }
    acc_sprintf("Material: [%s%s%s (%d)], Maxdamage: [%d (%d)], Damage: [%d]\r\n",
                CCYEL(ch, C_NRM),
                material_names[GET_OBJ_MATERIAL(j)],
                CCNRM(ch, C_NRM),
                GET_OBJ_MATERIAL(j),
                GET_OBJ_MAX_DAM(j),
                set_maxdamage(j),
                GET_OBJ_DAM(j));

    switch (GET_OBJ_TYPE(j)) {
    case ITEM_LIGHT:
        acc_sprintf("Color: [%d], Type: [%d], Hours: [%d], Value3: [%d]\r\n",
                    GET_OBJ_VAL(j, 0),
                    GET_OBJ_VAL(j, 1),
                    GET_OBJ_VAL(j, 2),
                    GET_OBJ_VAL(j, 3));
        break;
    case ITEM_INSTRUMENT:
        acc_sprintf("Instrument Type: [%d] (%s)\r\n",
                    GET_OBJ_VAL(j, 0),
                    GET_OBJ_VAL(j, 0) < 2 ? instrument_types[GET_OBJ_VAL(j, 0)] : "UNDEFINED");
        break;
    case ITEM_SCROLL:
    case ITEM_POTION:
    case ITEM_PILL:
    case ITEM_SYRINGE:
	case ITEM_BOOK:
        acc_sprintf("Level: %d, Spells: %s(%d), %s(%d), %s(%d)\r\n",
                    GET_OBJ_VAL(j, 0),
                    (GET_OBJ_VAL(j, 1) > 0) ? spell_to_str((int)GET_OBJ_VAL(j, 1)) : "None",
                    GET_OBJ_VAL(j, 1),
                    (GET_OBJ_VAL(j, 2) > 0) ? spell_to_str((int)GET_OBJ_VAL(j, 2)) : "None",
                    GET_OBJ_VAL(j, 2),
                    (GET_OBJ_VAL(j, 3) > 0) ? spell_to_str((int)GET_OBJ_VAL(j, 3)) : "None",
                    GET_OBJ_VAL(j, 3));
        break;
    case ITEM_WAND:
    case ITEM_STAFF:
        acc_sprintf("Level: %d, Max Charge: %d, Current Charge: %d, Spell: %s\r\n",
                    GET_OBJ_VAL(j, 0),
                    GET_OBJ_VAL(j, 1),
                    GET_OBJ_VAL(j, 2),
                    spell_to_str((int)GET_OBJ_VAL(j, 3)));
        break;
    case ITEM_WEAPON:
        acc_sprintf("Spell: %s (%d), Todam: %dd%d (av %d), Damage Type: %s (%d)\r\n",
            ((GET_OBJ_VAL(j, 0) > 0
                    && GET_OBJ_VAL(j,
                        0) < TOP_NPC_SPELL) ? spell_to_str((int)GET_OBJ_VAL(j,
                        0)) : "NONE"), GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1),
            GET_OBJ_VAL(j, 2), (GET_OBJ_VAL(j, 1) * (GET_OBJ_VAL(j,
                        2) + 1)) / 2, (GET_OBJ_VAL(j, 3) >= 0
                && GET_OBJ_VAL(j,
                    3) < 19) ? attack_hit_text[(int)GET_OBJ_VAL(j,
                    3)].plural : "bunk", GET_OBJ_VAL(j, 3));
        break;
    case ITEM_CAMERA:
        acc_sprintf("Targ room: %d\r\n", GET_OBJ_VAL(j, 0));
		break;
    case ITEM_MISSILE:
        acc_sprintf("Tohit: %d, Todam: %d, Type: %d\r\n",
                    GET_OBJ_VAL(j, 0),
                    GET_OBJ_VAL(j, 1),
                    GET_OBJ_VAL(j, 3));
        break;
    case ITEM_ARMOR:
        acc_sprintf("AC-apply: [%d]\r\n", GET_OBJ_VAL(j, 0));
        break;
    case ITEM_TRAP:
        acc_sprintf("Spell: %d, - Hitpoints: %d\r\n",
                    GET_OBJ_VAL(j, 0),
                    GET_OBJ_VAL(j, 1));
        break;
    case ITEM_CONTAINER:
        acc_sprintf("Max-contains: %d, Locktype: %d, Key/Owner: %d, Corpse: %s, Killer: %d\r\n",
                    GET_OBJ_VAL(j, 0),
                    GET_OBJ_VAL(j, 1),
                    GET_OBJ_VAL(j, 2),
                    GET_OBJ_VAL(j, 3) ? "Yes" : "No",
                    CORPSE_KILLER(j));
        break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
        acc_sprintf("Max-contains: %d, Contains: %d, Liquid: %s(%d), Poisoned: %s (%d)\r\n",
                    GET_OBJ_VAL(j, 0),
                    GET_OBJ_VAL(j, 1),
                    strlist_aref(GET_OBJ_VAL(j, 2), drinks),
                    GET_OBJ_VAL(j, 2),
                    GET_OBJ_VAL(j, 3) ? "Yes" : "No",
                    GET_OBJ_VAL(j, 3));
        break;
    case ITEM_KEY:
        acc_sprintf("Keytype: %d, Rentable: %s, Car Number: %d\r\n",
                    GET_OBJ_VAL(j, 0),
                    YESNO(GET_OBJ_VAL(j, 1)),
                    GET_OBJ_VAL(j, 2));
        break;
    case ITEM_FOOD:
        acc_sprintf("Makes full: %d, Spell Lvl: %d, Spell : %s(%d), Poisoned: %d\r\n",
                    GET_OBJ_VAL(j, 0),
                    GET_OBJ_VAL(j, 1),
                    spell_to_str((int)GET_OBJ_VAL(j, 2)),
                    GET_OBJ_VAL(j, 2),
                    GET_OBJ_VAL(j, 3));
        break;
    case ITEM_HOLY_SYMB:
        // FIXME: buffer overflow possibility
        acc_sprintf("Alignment: %s(%d), Class: %s(%d), Min Level: %d, Max Level: %d \r\n",
                    alignments[GET_OBJ_VAL(j, 0)],
                    GET_OBJ_VAL(j, 0),
                    char_class_abbrevs[(int)GET_OBJ_VAL(j, 1)],
                    GET_OBJ_VAL(j, 1),
                    GET_OBJ_VAL(j, 2),
                    GET_OBJ_VAL(j, 3));
        break;
    case ITEM_BATTERY:
        acc_sprintf("Max_Energy: %d, Cur_Energy: %d, Rate: %d, Cost/Unit: %d\r\n",
                    GET_OBJ_VAL(j, 0),
                    GET_OBJ_VAL(j, 1),
                    GET_OBJ_VAL(j, 2),
                    GET_OBJ_VAL(j, 3));
        break;
    case ITEM_VEHICLE:
        acc_sprintf("Room/Key Number: %d, Door State: %d, Type: %s, v3: %d\r\n",
                    GET_OBJ_VAL(j, 0),
                    GET_OBJ_VAL(j, 1),
                    tmp_printbits(GET_OBJ_VAL(j, 2), vehicle_types),
                    GET_OBJ_VAL(j, 3));
        break;
    case ITEM_ENGINE:
        acc_sprintf("Max_Energy: %d, Cur_Energy: %d, Run_State: %s, Consume_rate: %d\r\n",
                    GET_OBJ_VAL(j, 0),
                    GET_OBJ_VAL(j, 1),
                    tmp_printbits(GET_OBJ_VAL(j, 2), engine_flags),
                    GET_OBJ_VAL(j, 3));
        break;
    case ITEM_ENERGY_GUN:
        // FIXME: possible buffer overflow
        acc_sprintf("Drain Rate: %d, Todam: %dd%d (av %d), Damage Type: %s (%d)\r\n",
                    GET_OBJ_VAL(j, 0),
                    GET_OBJ_VAL(j, 1),
                    GET_OBJ_VAL(j, 2), 
                    (GET_OBJ_VAL(j, 1) * (GET_OBJ_VAL(j,2) + 1)) / 2, 
                    (GET_OBJ_VAL(j, 3) >= 0 && GET_OBJ_VAL(j,3) < EGUN_TOP) ? 
                    egun_types[(int)GET_OBJ_VAL(j,3)] : "unknown", 
                    GET_OBJ_VAL(j, 3));
            break;
    case ITEM_BOMB:
        // FIXME: possible buffer overflow
        acc_sprintf("Values: %s:[%s(%d)] %s:[%d] %s:[%d] %s:[%d]\r\n",
                    item_value_types[(int)GET_OBJ_TYPE(j)][0],
                    bomb_types[(int)GET_OBJ_VAL(j, 0)], GET_OBJ_VAL(j, 0),
                    item_value_types[(int)GET_OBJ_TYPE(j)][1], GET_OBJ_VAL(j, 1),
                    item_value_types[(int)GET_OBJ_TYPE(j)][2], GET_OBJ_VAL(j, 2),
                    item_value_types[(int)GET_OBJ_TYPE(j)][3], GET_OBJ_VAL(j, 3));
        break;
    case ITEM_FUSE:
        acc_sprintf("Values: %s:[%s(%d)] %s:[%d] %s:[%d] %s:[%d]\r\n",
                    item_value_types[(int)GET_OBJ_TYPE(j)][0],
                    fuse_types[(int)GET_OBJ_VAL(j, 0)], GET_OBJ_VAL(j, 0),
                    item_value_types[(int)GET_OBJ_TYPE(j)][1], GET_OBJ_VAL(j, 1),
                    item_value_types[(int)GET_OBJ_TYPE(j)][2], GET_OBJ_VAL(j, 2),
                    item_value_types[(int)GET_OBJ_TYPE(j)][3], GET_OBJ_VAL(j, 3));
        break;

    case ITEM_TOBACCO:
        acc_sprintf("Values: %s:[%s(%d)] %s:[%d] %s:[%d] %s:[%d]\r\n",
                    item_value_types[(int)GET_OBJ_TYPE(j)][0],
                    smoke_types[(int)MIN(GET_OBJ_VAL(j, 0), NUM_SMOKES - 1)],
                    GET_OBJ_VAL(j, 0),
                    item_value_types[(int)GET_OBJ_TYPE(j)][1], GET_OBJ_VAL(j, 1),
                    item_value_types[(int)GET_OBJ_TYPE(j)][2], GET_OBJ_VAL(j, 2),
                    item_value_types[(int)GET_OBJ_TYPE(j)][3], GET_OBJ_VAL(j, 3));
        break;

    case ITEM_GUN:
    case ITEM_BULLET:
    case ITEM_CLIP:
        acc_sprintf("Values 0-3: %s:[%d] %s:[%d] %s:[%d] %s:[%s (%d)]\r\n",
                    item_value_types[(int)GET_OBJ_TYPE(j)][0], GET_OBJ_VAL(j, 0),
                    item_value_types[(int)GET_OBJ_TYPE(j)][1], GET_OBJ_VAL(j, 1),
                    item_value_types[(int)GET_OBJ_TYPE(j)][2], GET_OBJ_VAL(j, 2),
                    item_value_types[(int)GET_OBJ_TYPE(j)][3],
                    (GET_OBJ_VAL(j, 3) >= 0 && GET_OBJ_VAL(j, 3) < NUM_GUN_TYPES) ?
                    gun_types[GET_OBJ_VAL(j, 3)] : "ERROR", GET_OBJ_VAL(j, 3));
        break;
    case ITEM_INTERFACE:
        acc_sprintf("Values 0-3: %s:[%s (%d)] %s:[%d] %s:[%d] %s:[%d]\r\n",
            item_value_types[(int)GET_OBJ_TYPE(j)][0],
            GET_OBJ_VAL(j, 0) >= 0 && GET_OBJ_VAL(j, 0) < NUM_INTERFACES ?
            interface_types[GET_OBJ_VAL(j, 0)] : "Error",
            GET_OBJ_VAL(j, 0),
            item_value_types[(int)GET_OBJ_TYPE(j)][1], GET_OBJ_VAL(j, 1),
            item_value_types[(int)GET_OBJ_TYPE(j)][2], GET_OBJ_VAL(j, 2),
            item_value_types[(int)GET_OBJ_TYPE(j)][3], GET_OBJ_VAL(j, 3));
        break;
    case ITEM_MICROCHIP:
        acc_sprintf("Values 0-3: %s:[%s (%d)] %s:[%d] %s:[%d] %s:[%d]\r\n",
            item_value_types[(int)GET_OBJ_TYPE(j)][0],
            GET_OBJ_VAL(j, 0) >= 0 && GET_OBJ_VAL(j, 0) < NUM_CHIPS ?
            microchip_types[GET_OBJ_VAL(j, 0)] : "Error",
            GET_OBJ_VAL(j, 0),
            item_value_types[(int)GET_OBJ_TYPE(j)][1], GET_OBJ_VAL(j, 1),
            item_value_types[(int)GET_OBJ_TYPE(j)][2], GET_OBJ_VAL(j, 2),
            item_value_types[(int)GET_OBJ_TYPE(j)][3], GET_OBJ_VAL(j, 3));
        break;
    default:
        acc_sprintf("Values 0-3: %s:[%d] %s:[%d] %s:[%d] %s:[%d]\r\n",
            item_value_types[(int)GET_OBJ_TYPE(j)][0], GET_OBJ_VAL(j, 0),
            item_value_types[(int)GET_OBJ_TYPE(j)][1], GET_OBJ_VAL(j, 1),
            item_value_types[(int)GET_OBJ_TYPE(j)][2], GET_OBJ_VAL(j, 2),
            item_value_types[(int)GET_OBJ_TYPE(j)][3], GET_OBJ_VAL(j, 3));
        break;
    }

	if (j->shared->proto == j) {
        char *param = GET_OBJ_PARAM(j);
        if (param && strlen(param) > 0 )
            acc_sprintf("Spec_param: \r\n%s\r\n", GET_OBJ_PARAM(j));
    }

    found = 0;
    acc_sprintf("Affections:");
    for (i = 0; i < MAX_OBJ_AFFECT; i++)
        if (j->affected[i].modifier) {
            acc_sprintf("%s %+d to %s",
                        found++ ? "," : "",
                        j->affected[i].modifier,
                        strlist_aref(j->affected[i].location, apply_types));
        }
    if (!found)
        acc_sprintf(" None");

    acc_sprintf("\r\n");

    if(j->shared->proto == j) {
        // All the rest of the stats are only meaningful if the object
        // is in the game.
        page_string(ch->desc, acc_get_string());
        return;
    }

    if (j->contains) {
        acc_sprintf("Contents:\r\n");
        list_obj_to_char(j->contains, ch, SHOW_OBJ_CONTENT, TRUE);

    }

    if (OBJ_SOILAGE(j))
        acc_sprintf("Soilage: %s\r\n",
                    tmp_printbits(OBJ_SOILAGE(j), soilage_bits));

    if (GET_OBJ_SIGIL_IDNUM(j)) {
        acc_sprintf("Warding Sigil: %s (%d), level %d.\r\n",
            playerIndex.getName(GET_OBJ_SIGIL_IDNUM(j)), GET_OBJ_SIGIL_IDNUM(j),
            GET_OBJ_SIGIL_LEVEL(j));
    }

    do_stat_obj_tmp_affs(ch, j);

    page_string(ch->desc, acc_get_string());
}

void
do_stat_obj_tmp_affs(struct Creature *ch, struct obj_data *obj)
{
    char *stat_prefix;
    
    if (!obj->tmp_affects)
        return;

    for (tmp_obj_affect *aff = obj->tmp_affects;aff; aff = aff->next) {
        stat_prefix = tmp_sprintf("AFF: (%3dhr) [%3hhd] %s%-20s%s", 
                                  aff->duration, aff->level, CCCYN(ch, C_NRM),
                                  spell_to_str(aff->type), CCNRM(ch, C_NRM));

        for (int i = 0; i < 4; i++)
            if (aff->val_mod[i] != 0)
                acc_sprintf("%s %s by %d\r\n",
                            stat_prefix, 
                            item_value_types[(int)GET_OBJ_TYPE(obj)][i],
                            aff->val_mod[i]);

        if (aff->type_mod)
            acc_sprintf("%s type = %s\r\n", stat_prefix, 
                        strlist_aref(GET_OBJ_TYPE(obj), item_type_descs));

        if (aff->worn_mod)
            acc_sprintf("%s worn + %s\r\n",
                        stat_prefix,
                        tmp_printbits(aff->worn_mod, wear_bits));

        if (aff->extra_mod) {
            const char **bit_descs[3] = {
                extra_bits, extra2_bits, extra3_bits
            };
            acc_sprintf("%s extra + %s\r\n",
                        stat_prefix,
                        tmp_printbits(aff->extra_mod,
                                      bit_descs[(int)aff->extra_index - 1]));
        }

        if (aff->weight_mod)
            acc_sprintf("%s weight by %d\r\n", stat_prefix, aff->weight_mod);

        for (int i = 0; i < MAX_OBJ_AFFECT; i++)
            if (aff->affect_loc[i])
                acc_sprintf("%s %+d to %s\r\n",
                             stat_prefix, 
                             aff->affect_mod[i],
                             strlist_aref(aff->affect_loc[i], apply_types));

        if (aff->dam_mod)
            acc_sprintf("%s damage by %d\r\n", stat_prefix, aff->dam_mod);

        if (aff->maxdam_mod)
            acc_sprintf("%s maxdam by %d\r\n", stat_prefix, aff->maxdam_mod);
    }
}

void
do_stat_character_kills(Creature *ch, Creature *k)
{
    if (!IS_PC(k)) {
        send_to_char(ch, "Recent kills by a mob are not recorded.\r\n");
    } else if (GET_RECENT_KILLS(k).empty()) {
        send_to_char(ch, "This player has not killed anything yet.\r\n");
    } else {
        acc_string_clear();
        acc_sprintf("Recently killed by %s:\r\n", GET_NAME(k));
        std::list<KillRecord>::iterator kill = GET_RECENT_KILLS(k).begin();
        for (;kill != GET_RECENT_KILLS(k).end();++kill) {
            Creature *killed = real_mobile_proto(kill->_vnum);
            acc_sprintf("%s%3d. %-30s %17d%s\r\n",
                        CCGRN(ch, C_NRM),
                        kill->_vnum,
                        (killed) ? GET_NAME(killed):"<unknown>",
                        kill->_times,
                        CCNRM(ch, C_NRM));
        }
        page_string(ch->desc, acc_get_string());
    }
}

void
do_stat_character_prog(Creature *ch, Creature *k)
{
    if (IS_PC(k)) {
        send_to_char(ch, "Players don't have progs!\r\n");
    } else if (!GET_MOB_PROG(k)) {
        send_to_char(ch, "Mobile %d does not have a prog.\r\n",
                     GET_MOB_VNUM(k));
    } else {
        acc_string_clear();
        acc_format_prog(ch, GET_MOB_PROG(k));
        page_string(ch->desc, acc_get_string());
    }
}

void
do_stat_character(Creature *ch, Creature *k, char *options)
{
    int i, num, num2, found = 0, rexp;
	char *line_buf;
    struct follow_type *fol;
    struct affected_type *aff;
    extern struct attack_hit_type attack_hit_text[];

	if (IS_PC(k) && !Security::isMember(ch, Security::ADMINBASIC)) {
        send_to_char(ch, "You can't stat this player.\r\n");
        return;
	}

	if (GET_MOB_SPEC(k) == fate
			&& !Security::isMember(ch, Security::WIZARDBASIC)) {
        send_to_char(ch, "You can't stat this mob.\r\n");
        return;
	}
		
    for (char *opt_str = tmp_getword(&options);*opt_str;opt_str = tmp_getword(&options)) {
        if (is_abbrev(opt_str, "kills")) {
            do_stat_character_kills(ch, k);
            return;
        } else if (is_abbrev(opt_str, "prog")) {
            do_stat_character_prog(ch, k);
            return;
        }
    }

	acc_string_clear();
    switch (GET_SEX(k)) {
    case SEX_NEUTRAL:
        acc_strcat("NEUTER", NULL); break;
    case SEX_MALE:
        acc_strcat("MALE", NULL); break;
    case SEX_FEMALE:
        acc_strcat("FEMALE", NULL); break;
    default:
        acc_strcat("ILLEGAL-SEX!!", NULL); break;
    }

    acc_sprintf(" %s '%s%s%s'  IDNum: [%5ld], AccountNum: [%5ld], In room %s[%s%5d%s]%s\n",
        (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")),
        CCYEL(ch, C_NRM), GET_NAME(k), CCNRM(ch, C_NRM),
        IS_NPC(k) ? MOB_IDNUM(k) : GET_IDNUM(k),
        IS_NPC(k) ? -1 : k->getAccountID(),
        CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), k->in_room ?
        k->in_room->number : -1, CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));

    if (!IS_MOB(k) && GET_LEVEL(k) >= LVL_AMBASSADOR)
        acc_sprintf("OlcObj: [%d], OlcMob: [%d]\r\n",
            (GET_OLC_OBJ(k) ? GET_OLC_OBJ(k)->shared->vnum : (-1)),
            (GET_OLC_MOB(k) ?
                GET_OLC_MOB(k)->mob_specials.shared->vnum : (-1)));
    else
        acc_strcat("\r\n", NULL);

    if (IS_MOB(k)) {
        acc_sprintf(
            "Alias: %s, VNum: %s[%s%5d%s]%s, Exist: [%3d], SVNum: %s[%s%5d%s]%s\r\n",
            k->player.name, CCGRN(ch, C_NRM), CCYEL(ch, C_NRM),
            GET_MOB_VNUM(k), CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
            k->mob_specials.shared->number, CCGRN(ch, C_NRM), CCYEL(ch, C_NRM),
            GET_SCRIPT_VNUM(k), CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
        acc_sprintf("L-Des: %s%s%s\r\n", CCYEL(ch, C_NRM),
            (k->player.long_descr ? k->player.long_descr : "<None>"),
            CCNRM(ch, C_NRM));
    } else {
        acc_sprintf("Title: %s\r\n",
            (k->player.title ? k->player.title : "<None>"));
    }

/* Race, Class */
    acc_sprintf("Race: %s, Class: %s%s/%s Gen: %d\r\n",
		strlist_aref(k->player.race, player_race),
		strlist_aref(k->player.char_class, class_names),
		(IS_CYBORG(k) ?
			tmp_sprintf("(%s)",
				strlist_aref(GET_OLD_CLASS(k), borg_subchar_class_names)) :
			""),
		(IS_REMORT(k) ?
			strlist_aref(GET_REMORT_CLASS(k), class_names) :
			"None"),
		GET_REAL_GEN(k));

    if (!IS_NPC(k)) {
        acc_sprintf("Lev: [%s%2d%s], XP: [%s%7d%s/%s%d%s], Align: [%4d]\r\n",
            CCYEL(ch, C_NRM), GET_LEVEL(k), CCNRM(ch, C_NRM),
            CCYEL(ch, C_NRM), GET_EXP(k), CCNRM(ch, C_NRM),
            CCCYN(ch, C_NRM), exp_scale[GET_LEVEL(k) + 1] - GET_EXP(k),
            CCNRM(ch, C_NRM), GET_ALIGNMENT(k));
    } else {
        rexp = mobile_experience(k);
        acc_sprintf(
            "Lev: [%s%2d%s], XP: [%s%7d%s/%s%d%s] %s(%s%3d p%s)%s, Align: [%4d]\r\n",
            CCYEL(ch, C_NRM), GET_LEVEL(k), CCNRM(ch, C_NRM), CCYEL(ch, C_NRM),
            GET_EXP(k), CCNRM(ch, C_NRM), CCCYN(ch, C_NRM), rexp, CCNRM(ch,
                C_NRM), CCRED(ch, C_NRM), CCNRM(ch, C_NRM),
            GET_EXP(k) > 10000000 ? (((unsigned int)GET_EXP(k)) / MAX(1,
                    rexp / 100)) : (((unsigned int)(GET_EXP(k) * 100)) / MAX(1,
                    rexp)), CCRED(ch, C_NRM), CCNRM(ch, C_NRM),
            GET_ALIGNMENT(k));
    }

    acc_sprintf("Height:  %d centimeters , Weight: %d pounds.\r\n",
        GET_HEIGHT(k), GET_WEIGHT(k));

    if (!IS_NPC(k)) {
        strcpy(buf1, (char *)asctime(localtime(&(k->player.time.birth))));
        strcpy(buf2, (char *)asctime(localtime(&(k->player.time.logon))));
        buf1[10] = buf2[10] = '\0';

        acc_sprintf("Created: [%s], Last Logon: [%s], Played [%ldh %ldm], Age [%d]\r\n", 
                buf1, buf2, 
                k->player.time.played / 3600, 
                ((k->player.time.played / 3600) % 60), 
                GET_AGE(k) );

        acc_sprintf("Homeroom:[%d], Loadroom: [%d], Clan: %s%s%s\r\n",
            GET_HOMEROOM(k), k->player_specials->saved.home_room,
            CCCYN(ch, C_NRM),
            real_clan(GET_CLAN(k)) ? real_clan(GET_CLAN(k))->name : "NONE",
            CCNRM(ch, C_NRM));

        acc_sprintf(
            "Life: [%d], Thac0: [%d], Reputation: [%4d]",
            GET_LIFE_POINTS(k), (int)MIN(THACO(GET_CLASS(k), GET_LEVEL(k)), 
                     THACO(GET_REMORT_CLASS(k), GET_LEVEL(k))),
			GET_REPUTATION(k));

		if (IS_IMMORT(k))
			acc_sprintf(", Qpoints: [%d/%d]", GET_IMMORT_QP(k),
				GET_QUEST_ALLOWANCE(k));

        acc_sprintf(
            "\r\n%sMobKills:%s [%4d], %sPKills:%s [%4d], %sDeaths:%s [%4d]\r\n",
            CCYEL(ch, C_NRM), CCNRM(ch, C_NRM), GET_MOBKILLS(k), CCRED(ch,
                C_NRM), CCNRM(ch, C_NRM), GET_PKILLS(k), CCGRN(ch, C_NRM),
            CCNRM(ch, C_NRM), GET_PC_DEATHS(k));
    }
    acc_sprintf("Str: [%s%d/%d%s]  Int: [%s%d%s]  Wis: [%s%d%s]  "
        "Dex: [%s%d%s]  Con: [%s%d%s]  Cha: [%s%d%s]\r\n",
        CCCYN(ch, C_NRM), GET_STR(k), GET_ADD(k), CCNRM(ch, C_NRM),
        CCCYN(ch, C_NRM), GET_INT(k), CCNRM(ch, C_NRM),
        CCCYN(ch, C_NRM), GET_WIS(k), CCNRM(ch, C_NRM),
        CCCYN(ch, C_NRM), GET_DEX(k), CCNRM(ch, C_NRM),
        CCCYN(ch, C_NRM), GET_CON(k), CCNRM(ch, C_NRM),
        CCCYN(ch, C_NRM), GET_CHA(k), CCNRM(ch, C_NRM));
    if (k->in_room || !IS_NPC(k)) {    // Real Mob/Char
        acc_sprintf(
            "Hit p.:[%s%d/%d+%d%s]  Mana p.:[%s%d/%d+%d%s]  Move p.:[%s%d/%d+%d%s]\r\n",
            CCGRN(ch, C_NRM), GET_HIT(k), GET_MAX_HIT(k), hit_gain(k),
            CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), GET_MANA(k), GET_MAX_MANA(k),
            mana_gain(k), CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), GET_MOVE(k),
            GET_MAX_MOVE(k), move_gain(k), CCNRM(ch, C_NRM));
    } else {                    // Virtual Mob
        acc_sprintf(
            "Hit p.:[%s%dd%d+%d (%d)%s]  Mana p.:[%s%d%s]  Move p.:[%s%d%s]\r\n",
            CCGRN(ch, C_NRM), GET_HIT(k), GET_MANA(k), GET_MOVE(k),
            (GET_HIT(k) * (GET_MANA(k) + 1) / 2) + GET_MOVE(k), CCNRM(ch,
                C_NRM), CCGRN(ch, C_NRM), GET_MAX_MANA(k), CCNRM(ch, C_NRM),
            CCGRN(ch, C_NRM), GET_MAX_MOVE(k), CCNRM(ch, C_NRM));
    }

    acc_sprintf(
        "AC: [%s%d/10%s], Hitroll: [%s%2d%s], Damroll: [%s%2d%s], Speed: [%s%2d%s], Damage Reduction: [%s%2d%s]\r\n",
        CCYEL(ch, C_NRM), GET_AC(k), CCNRM(ch, C_NRM), CCYEL(ch, C_NRM),
        k->points.hitroll, CCNRM(ch, C_NRM), CCYEL(ch, C_NRM),
        k->points.damroll, CCNRM(ch, C_NRM), CCYEL(ch, C_NRM), k->getSpeed(),
        CCNRM(ch, C_NRM), CCYEL(ch, C_NRM), (int)(k->getDamReduction() * 100),
        CCNRM(ch, C_NRM));

    if (!IS_NPC(k) || k->in_room) {
        acc_sprintf(
            "Pr:[%s%2d%s],Rd:[%s%2d%s],Pt:[%s%2d%s],Br:[%s%2d%s],Sp:[%s%2d%s],Ch:[%s%2d%s],Ps:[%s%2d%s],Ph:[%s%2d%s]\r\n",
            CCYEL(ch, C_NRM), GET_SAVE(k, 0), CCNRM(ch, C_NRM),
            CCYEL(ch, C_NRM), GET_SAVE(k, 1), CCNRM(ch, C_NRM),
            CCYEL(ch, C_NRM), GET_SAVE(k, 2), CCNRM(ch, C_NRM),
            CCYEL(ch, C_NRM), GET_SAVE(k, 3), CCNRM(ch, C_NRM),
            CCYEL(ch, C_NRM), GET_SAVE(k, 4), CCNRM(ch, C_NRM),
            CCYEL(ch, C_NRM), GET_SAVE(k, 5), CCNRM(ch, C_NRM),
            CCYEL(ch, C_NRM), GET_SAVE(k, 6), CCNRM(ch, C_NRM),
            CCYEL(ch, C_NRM), GET_SAVE(k, 7), CCNRM(ch, C_NRM));
    }
    if (IS_NPC(k))
        acc_sprintf("Gold:[%8d], Cash:[%8d], (Total: %d)\r\n",
            GET_GOLD(k), GET_CASH(k), GET_GOLD(k) + GET_CASH(k));
    else
        acc_sprintf(
            "Au:[%8d], Bank:[%9lld], Cash:[%8d], Enet:[%9lld], (Total: %lld)\r\n",
            GET_GOLD(k), GET_PAST_BANK(k), GET_CASH(k), GET_FUTURE_BANK(k),
            GET_GOLD(k) + GET_PAST_BANK(k) + GET_FUTURE_BANK(k) + GET_CASH(k));

    if (IS_NPC(k)) {
        acc_sprintf("Pos: %s, Dpos: %s, Attack: %s",
            position_types[k->getPosition()],
            position_types[(int)k->mob_specials.shared->default_pos],
            attack_hit_text[k->mob_specials.shared->attack_type].singular);
        if (k->in_room)
            acc_sprintf(", %sFT%s: %s, %sHNT%s: %s, Timer: %d",
                CCRED(ch, C_NRM), CCNRM(ch, C_NRM),
                (k->isFighting() ? GET_NAME(k->findRandomCombat()) : "N"),
                CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
                k->isHunting() ? PERS(k->isHunting(), ch) : "N",
                k->char_specials.timer);
		acc_strcat("\r\n", NULL);
    } else if (k->in_room) {
        acc_sprintf("Pos: %s, %sFT%s: %s, %sHNT%s: %s",
            position_types[k->getPosition()],
            CCRED(ch, C_NRM), CCNRM(ch, C_NRM),
            (k->isFighting() ? GET_NAME(k->findRandomCombat()) : "N"),
            CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
            k->isHunting() ? PERS(k->isHunting(), ch) : "N");
    }
    if (k->desc)
        acc_sprintf(", Connected: %s, Idle [%d]\r\n",
			strlist_aref((int)k->desc->input_mode, desc_modes),
			k->char_specials.timer);
    else
        acc_strcat("\r\n", NULL);

    if (k->getPosition() == POS_MOUNTED && k->isMounted())
		acc_sprintf("Mount: %s\r\n", GET_NAME(k->isMounted()));

    if (IS_NPC(k)) {
        sprintbit(MOB_FLAGS(k), action_bits, buf);
        acc_sprintf("NPC flags: %s%s%s\r\n", CCCYN(ch, C_NRM), buf,
			CCNRM(ch, C_NRM));
        sprintbit(MOB2_FLAGS(k), action2_bits, buf);
        acc_sprintf("NPC flags(2): %s%s%s\r\n", CCCYN(ch, C_NRM), buf,
            CCNRM(ch, C_NRM));
    } else {
        if (GET_LEVEL(ch) >= LVL_CREATOR)
            sprintbit(PLR_FLAGS(k), player_bits, buf);
        else
            sprintbit(PLR_FLAGS(k) & ~PLR_LOG, player_bits, buf);
        acc_sprintf("PLR: %s%s%s\r\n", CCCYN(ch, C_NRM), buf,
			CCNRM(ch, C_NRM));
        sprintbit(PLR2_FLAGS(k), player2_bits, buf);
        acc_sprintf("PLR2: %s%s%s\r\n", CCCYN(ch, C_NRM), buf,
			CCNRM(ch, C_NRM));
        sprintbit(PRF_FLAGS(k), preference_bits, buf);
        acc_sprintf("PRF: %s%s%s\r\n", CCGRN(ch, C_NRM), buf,
			CCNRM(ch, C_NRM));
        sprintbit(PRF2_FLAGS(k), preference2_bits, buf);
        acc_sprintf("PRF2: %s%s%s\r\n", CCGRN(ch, C_NRM), buf,
			CCNRM(ch, C_NRM));
        if (PLR_FLAGGED(k, PLR_FROZEN)) {
            acc_sprintf("%sFrozen by: %s", CCCYN(ch, C_NRM),
                        playerIndex.getName(k->player_specials->freezer_id));
            if (k->player_specials->thaw_time > 0)
                acc_sprintf(", will auto-thaw at %s%s\r\n",
                            tmp_ctime(k->player_specials->thaw_time),
                            CCNRM(ch, C_NRM));
            else
                acc_sprintf("%s\r\n", CCNRM(ch, C_NRM));
        }
    }

    if (IS_MOB(k)) {
        acc_sprintf(
            "Mob Spec: %s, NPC Dam: %dd%d, Morale: %d, Lair: %d, Ldr: %d\r\n",
            (k->mob_specials.shared->func ? ((i =
                        find_spec_index_ptr(k->mob_specials.shared->func)) <
                    0 ? "Exists" : spec_list[i].tag) : "None"),
            k->mob_specials.shared->damnodice,
            k->mob_specials.shared->damsizedice,
            k->mob_specials.shared->morale, k->mob_specials.shared->lair,
            k->mob_specials.shared->leader);

        if (MOB_SHARED(k)->move_buf)
            acc_sprintf("Move_buf: %s\r\n",
                MOB_SHARED(k)->move_buf);
		if (k->mob_specials.shared->proto == k) {
            char *param = GET_MOB_PARAM(k);

			if (param && strlen(param) > 0 )
				acc_sprintf("Spec_param: \r\n%s\r\n", GET_MOB_PARAM(k));
            param = GET_LOAD_PARAM(k);
			if (param && strlen(param) > 0 )
				acc_sprintf("Load_param: \r\n%s\r\n", GET_LOAD_PARAM(k));
		}
    }

    for (i = 0, num = 0, num2 = 0; i < NUM_WEARS; i++) {
        if (GET_EQ(k, i))
            num++;
        if (GET_IMPLANT(k, i))
            num2++;
    }

    found = FALSE;
    if (k->in_room) {
        acc_sprintf(
            "Encum : (%d inv + %d eq) = (%d tot)/%d, Number: %d/%d inv, %d eq, %d imp\r\n",
            IS_CARRYING_W(k), IS_WEARING_W(k),
            (IS_CARRYING_W(k) + IS_WEARING_W(k)), CAN_CARRY_W(k),
            IS_CARRYING_N(k), (int)CAN_CARRY_N(k), num, num2);
        if (k->getBreathCount() || GET_FALL_COUNT(k)) {
            acc_sprintf("Breath_count: %d, Fall_count: %d",
                k->getBreathCount(), GET_FALL_COUNT(k));
            found = TRUE;
        }
    }
    if (!IS_MOB(k)) {
        acc_sprintf("%sHunger: %d, Thirst: %d, Drunk: %d\r\n",
            found ? ", " : "",
            GET_COND(k, FULL), GET_COND(k, THIRST), GET_COND(k, DRUNK));
    } else if (found)
		acc_strcat("\r\n", NULL);

    if (!IS_NPC(k) && GET_QUEST(k)) {
        char* name = "None";
        Quest *quest = quest_by_vnum( GET_QUEST(k) );

        if( quest != NULL && quest->isPlaying(GET_IDNUM(k)) )
            name = quest->name;
        acc_sprintf("Quest [%d]: \'%s\'\r\n", GET_QUEST(k), name );
    }

    if (k->in_room && (k->master || k->followers)) {
        acc_sprintf("Master is: %s, Followers are:",
            ((k->master) ? GET_NAME(k->master) : "<none>"));

		line_buf = "";
        for (fol = k->followers; fol; fol = fol->next) {
			line_buf = tmp_sprintf("%s %s",
				(*line_buf) ? "," : "",
				PERS(fol->follower, ch));
            if (strlen(line_buf) >= 62) {
				acc_strcat(line_buf, "\r\n");
				line_buf = "";
            }
        }

		acc_strcat("\r\n", NULL);
    }
    /* Showing the bitvector */
	if (AFF_FLAGS(k)) {
		sprintbit(AFF_FLAGS(k), affected_bits, buf);
		acc_sprintf("AFF: %s%s%s\r\n", CCYEL(ch, C_NRM), buf,
			CCNRM(ch, C_NRM));
	}
    if (AFF2_FLAGS(k)) {
        sprintbit(AFF2_FLAGS(k), affected2_bits, buf);
        acc_sprintf("AFF2: %s%s%s\r\n", CCYEL(ch, C_NRM), buf,
			CCNRM(ch, C_NRM));
    }
    if (AFF3_FLAGS(k)) {
        sprintbit(AFF3_FLAGS(k), affected3_bits, buf);
        acc_sprintf("AFF3: %s%s%s\r\n", CCYEL(ch, C_NRM), buf,
			CCNRM(ch, C_NRM));
    }
    if (k->getPosition() == POS_SITTING && IS_AFFECTED_2(k, AFF2_MEDITATE))
        acc_sprintf("Meditation Timer: [%d]\r\n", MEDITATE_TIMER(k));

    if (IS_CYBORG(k)) {
        acc_sprintf("Broken component: [%s (%d)], Dam Count: %d/%d.\r\n",
			get_component_name(GET_BROKE(k), GET_OLD_CLASS(k)),
            GET_BROKE(k), GET_TOT_DAM(k), max_component_dam(k));

        if (AFF3_FLAGGED(k, AFF3_SELF_DESTRUCT))
            acc_sprintf("Self-destruct Timer: [%d]\r\n", MEDITATE_TIMER(k));
    }

	if (GET_MOB_STATE(k) && GET_MOB_STATE(k)->var_list) {
		prog_var *cur_var;
		acc_strcat("Prog state variables:\r\n", NULL);
		for (cur_var = GET_MOB_STATE(k)->var_list;cur_var;cur_var = cur_var->next)
			acc_sprintf("     %s = '%s'\r\n", cur_var->key, cur_var->value);
	}
    acc_sprintf("Currently speaking: %s%s%s\r\n", CCCYN(ch, C_NRM),
                tongue_name(GET_TONGUE(k)), CCNRM(ch, C_NRM));

    if (GET_LANG_HEARD(k).size()) {
        acc_sprintf("Recently heard: %s", CCCYN(ch, C_NRM));
        std::list<int>::iterator lang = GET_LANG_HEARD(k).begin();
        bool first = true;
        for (;lang != GET_LANG_HEARD(k).end();++lang) {
            if (first)
                first = false;
            else
                acc_strcat(CCNRM(ch, C_NRM), ", ", CCCYN(ch, C_NRM), NULL);

            acc_strcat(tongue_name(*lang), NULL);
        }
        acc_sprintf("%s\r\n", CCNRM(ch, C_NRM));
    }
    acc_sprintf("Known Languages:\r\n");
    map<int, Tongue>::iterator it = tongues.begin();
    for (;it != tongues.end();++it) {
        if (CHECK_TONGUE(k, it->first)) {
            acc_sprintf("%s%3d. %-30s %s%-17s%s%s\r\n",
                        CCCYN(ch, C_NRM),
                        it->first,
                        it->second._name,
                        CCBLD(ch, C_SPR),
                        fluency_desc(k, it->first),
                        tmp_sprintf("%s[%3d]%s", CCYEL(ch, C_NRM),
                                    CHECK_TONGUE(k, it->first),
                                    CCNRM(ch, C_NRM)),
                        CCNRM(ch, C_SPR));
        }
    }
    
    if (IS_PC(k)) {
        if (!GET_GRIEVANCES(k).empty()) {
            acc_sprintf("Grievances:\r\n");
            std::list<Grievance>::iterator grievance = GET_GRIEVANCES(k).begin();
            for (;grievance != GET_GRIEVANCES(k).end();++grievance) {
                acc_sprintf("%s%3d. %s got %d rep for %s at %s%s\r\n",
                            CCGRN(ch, C_NRM),
                            grievance->_player_id,
                            playerIndex.getName(grievance->_player_id),
                            grievance->_rep,
                            Grievance::kind_descs[grievance->_grievance],
                            tmp_ctime(grievance->_time),
                            CCNRM(ch, C_NRM));
            }
        }
    }

    /* Routine to show what spells a char is affected by */
    if (k->affected) {
        for (aff = k->affected; aff; aff = aff->next) {
            acc_sprintf("SPL: (%3d%s) [%2d] %s(%ld) %s%-24s%s ", aff->duration + 1,
                aff->is_instant ? "sec" : "hr", aff->level,
                CCYEL(ch, C_NRM), aff->owner, 
                CCCYN(ch, C_NRM), spell_to_str(aff->type), CCNRM(ch, C_NRM));
            if (aff->modifier) {
                acc_sprintf("%+d to %s", aff->modifier,
                    apply_types[(int)aff->location]);
				if (aff->bitvector)
                    acc_strcat(", ", NULL);
			}

            if (aff->bitvector) {
                if (aff->aff_index == 3)
                    sprintbit(aff->bitvector, affected3_bits, buf);
                else if (aff->aff_index == 2)
                    sprintbit(aff->bitvector, affected2_bits, buf);
                else
                    sprintbit(aff->bitvector, affected_bits, buf);
				acc_strcat("sets ", buf, NULL);
            }
			acc_strcat("\r\n", NULL);
        }
    }
    page_string(ch->desc, acc_get_string());
}


ACMD(do_stat)
{
    struct Creature *victim = 0;
    struct obj_data *object = 0;
    struct zone_data *zone = NULL;
    int tmp, found;
    char *arg1 = tmp_getword(&argument);
    char *arg2 = tmp_getword(&argument);

    if (!*arg1) {
        send_to_char(ch, "Stats on who or what?\r\n");
        return;
    } else if (is_abbrev(arg1, "room")) {
        do_stat_room(ch, arg2);
    } else if (!strncmp(arg1, "trails", 6)) {
        do_stat_trails(ch);
    } else if (is_abbrev(arg1, "zone")) {
        if (!*arg2)
            do_stat_zone(ch, ch->in_room->zone);
        else {
            if (is_number(arg2)) {
                tmp = atoi(arg2);
                for (found = 0, zone = zone_table; zone && found != 1;
                    zone = zone->next)
                    if (zone->number <= tmp && zone->top > tmp*100) {
                        do_stat_zone(ch, zone);
                        found = 1;
                    }
                if (found != 1)
                    send_to_char(ch, "No zone exists with that number.\r\n");
            } else
                send_to_char(ch, "Invalid zone number.\r\n");
        }
    } else if (is_abbrev(arg1, "mob")) {
        if (!*arg2)
            send_to_char(ch, "Stats on which mobile?\r\n");
        else {
            if ((victim = get_char_vis(ch, arg2)) &&
                (IS_NPC(victim) || GET_LEVEL(ch) >= LVL_DEMI))
                do_stat_character(ch, victim, argument);
            else
                send_to_char(ch, "No such mobile around.\r\n");
        }
    } else if (is_abbrev(arg1, "player")) {
        if (!*arg2) {
            send_to_char(ch, "Stats on which player?\r\n");
        } else {
            if ((victim = get_player_vis(ch, arg2, 0)))
                do_stat_character(ch, victim, argument);
            else
                send_to_char(ch, "No such player around.\r\n");
        }
    } else if (is_abbrev(arg1, "file")) {
        if (GET_LEVEL(ch) < LVL_TIMEGOD && !Security::isMember(ch, "AdminFull")
            && !Security::isMember(ch, "WizardFull")) {
            send_to_char(ch, "You cannot peer into the playerfile.\r\n");
            return;
        }
        if (!*arg2) {
            send_to_char(ch, "Stats on which player?\r\n");
        } else {
			victim = new Creature(true);
			if (!playerIndex.exists(arg2)) {
                send_to_char(ch, "There is no such player.\r\n");
            } else {
				if (victim->loadFromXML(playerIndex.getID(arg2)))
					do_stat_character(ch, victim, argument);
				else
					send_to_char(ch, "Error loading character '%s'\r\n",arg2);
			}
            delete victim;
        }
    } else if (is_abbrev(arg1, "object")) {
        if (!*arg2)
            send_to_char(ch, "Stats on which object?\r\n");
        else {
            if ((object = get_obj_vis(ch, arg2)))
                do_stat_object(ch, object);
            else
                send_to_char(ch, "No such object around.\r\n");
        }
    } else {
        if ((object = get_object_in_equip_vis(ch, arg1, ch->equipment, &tmp)))
            do_stat_object(ch, object);
        else if ((object = get_obj_in_list_vis(ch, arg1, ch->carrying)))
            do_stat_object(ch, object);
        else if ((victim = get_char_room_vis(ch, arg1)))
            do_stat_character(ch, victim, argument);
        else if ((object =
                get_obj_in_list_vis(ch, arg1, ch->in_room->contents)))
            do_stat_object(ch, object);
        else if ((victim = get_char_vis(ch, arg1)))
            do_stat_character(ch, victim, argument);
        else if ((object = get_obj_vis(ch, arg1)))
            do_stat_object(ch, object);
        else
            send_to_char(ch, "Nothing around by that name.\r\n");
    }
}


ACMD(do_shutdown)
{
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int count = 0;

    extern int circle_shutdown, circle_reboot, mini_mud, shutdown_idnum;

    if (subcmd != SCMD_SHUTDOWN) {
        send_to_char(ch, "If you want to shut something down, say so!\r\n");
        return;
    }
    two_arguments(argument, arg, arg2);

    if (*arg2) {
        if (shutdown_count >= 0) {
            send_to_char(ch, "Shutdown process is already running.\r\n"
                "Use Shutdown abort first.\r\n");
            return;
        }
        if ((count = atoi(arg2)) < 0) {
            send_to_char(ch, "Real funny shutdown time.\r\n");
            return;
        }
    }

    if (!*arg) {
        if (!mini_mud) {
            send_to_char(ch, "Please specify a shutdown mode.\r\n");
            return;
        }
        slog("(GC) Shutdown by %s.", GET_NAME(ch));
		save_all_players();
		Housing.collectRent();
		Housing.save();
        save_quests();
        autosave_zones(ZONE_RESETSAVE);
        send_to_all("\r\n"
            "As you stand amazed, you see the sun swell and fill the sky,\r\n"
            "and you are overtaken by an intense heat as the particles\r\n"
            "of your body are mixed with the rest of the cosmos by the\r\n"
            "power of a blinding supernova explosion.\r\n\r\n"
            "Shutting down.\r\n"
            "Please visit our website at http://tempusmud.com\r\n");
        circle_shutdown = 1;
    } else if (!str_cmp(arg, "abort")) {
        if (shutdown_count < 0)
            send_to_char(ch, "Shutdown process is not currently running.\r\n");
        else {
            shutdown_count = -1;
            shutdown_idnum = -1;
            shutdown_mode = SHUTDOWN_NONE;
            slog("(GC) Shutdown ABORT called by %s.", GET_NAME(ch));
            send_to_all(":: Tempus REBOOT sequence aborted ::\r\n");
        }
        return;

    } else if (!str_cmp(arg, "reboot")) {
        if (!count) {
            touch("../.fastboot");
            slog("(GC) Reboot by %s.", GET_NAME(ch));
            save_all_players();
			Housing.collectRent();
			Housing.save();
            autosave_zones(ZONE_RESETSAVE);
            save_quests();
            send_to_all("\r\n"
                "You stagger under the force of a sudden gale, and cringe in terror\r\n"
                "as the sky darkens before the gathering stormclouds.  Lightning\r\n"
                "crashes to the ground all around you, and you see the hand of the\r\n"
                "Universal Power rush across the land, destroying all and purging\r\n"
                "the cosmos, only to begin rebuilding once again.\r\n\r\n"
                "Rebooting... come back in five minutes.\r\n"
                "Please visit our website at http://tempusmud.com\r\n");
            circle_shutdown = circle_reboot = 1;
        } else {
            slog("(GC) Reboot in [%d] seconds by %s.", count,
                GET_NAME(ch));
            sprintf(buf, "\007\007:: Tempus REBOOT in %d seconds ::\r\n",
                count);
            send_to_all(buf);
            shutdown_idnum = GET_IDNUM(ch);
            shutdown_count = count;
            shutdown_mode = SHUTDOWN_REBOOT;
        }
    } else if (!str_cmp(arg, "die")) {
        slog("(GC) Shutdown by %s.", GET_NAME(ch));
		save_all_players();
		Housing.collectRent();
		Housing.save();
        autosave_zones(ZONE_RESETSAVE);
        save_quests();
        send_to_all
            ("As you stand amazed, you see the sun swell and fill the sky,\r\n"
            "and you are overtaken by an intense heat as the particles\r\n"
            "of your body are mixed with the rest of the cosmos by the\r\n"
            "power of a blinding supernova explosion.\r\n\r\n"
            "Shutting down for maintenance.\r\n"
            "Please visit our website at http://tempusmud.com\r\n");

        touch("../.killscript");
        circle_shutdown = 1;
    } else if (!str_cmp(arg, "pause")) {
        slog("(GC) Shutdown by %s.", GET_NAME(ch));
		save_all_players();
		Housing.collectRent();
		Housing.save();
        autosave_zones(ZONE_RESETSAVE);
        save_quests();
        send_to_all
            ("As you stand amazed, you see the sun swell and fill the sky,\r\n"
            "and you are overtaken by an intense heat as the particles\r\n"
            "of your body are mixed with the rest of the cosmos by the\r\n"
            "power of a blinding supernova explosion.\r\n\r\n"
            "Shutting down for maintenance.\r\n"
            "Please visit our website at http://tempusmud.com\r\n");

        touch("../pause");
        circle_shutdown = 1;
    } else
        send_to_char(ch, "Unknown shutdown option.\r\n");
}


void
stop_snooping(struct Creature *ch)
{
    if (!ch->desc->snooping)
        send_to_char(ch, "You aren't snooping anyone.\r\n");
    else {
        send_to_char(ch, "You stop snooping.\r\n");
        vector<descriptor_data *>::iterator vi = ch->desc->snooping->snoop_by.begin();
        for (; vi != ch->desc->snooping->snoop_by.end(); ++vi) {
            if (*vi == ch->desc) {
                ch->desc->snooping->snoop_by.erase(vi);
                break;
            }
        }
        ch->desc->snooping = NULL;
    }
}


ACMD(do_snoop)
{
    struct Creature *victim, *tch;

    if (!ch->desc)
        return;

    one_argument(argument, arg);

    if (!*arg)
        stop_snooping(ch);
    else if (!(victim = get_char_vis(ch, arg)))
        send_to_char(ch, "No such person around.\r\n");
    else if (!victim->desc)
        send_to_char(ch, "There's no link.. nothing to snoop.\r\n");
    else if (victim == ch)
        stop_snooping(ch);
    else if (PRF_FLAGGED(victim, PRF_NOSNOOP) && GET_LEVEL(ch) < LVL_ENTITY)
        send_to_char(ch, "The gods say I don't think so!\r\n");
    else if (victim->desc->snooping == ch->desc)
        send_to_char(ch, "Don't be stupid.\r\n");
    else if (ROOM_FLAGGED(victim->in_room, ROOM_GODROOM)
        && !Security::isMember(ch, "WizardFull")) {
        send_to_char(ch, "You cannot snoop into that place.\r\n");
    } else {
        if (victim->desc->snoop_by.size()) {
            for (unsigned x = 0; x < victim->desc->snoop_by.size(); x++) {
                if (victim->desc->snoop_by[x] == ch->desc) {
                    act("You're already snooping $M.", FALSE, ch, 0, victim, TO_CHAR);
                    return;
                }
            }
        }
        if (victim->desc->original)
            tch = victim->desc->original;
        else
            tch = victim;

        if (GET_LEVEL(tch) >= GET_LEVEL(ch)) {
            send_to_char(ch, "You can't.\r\n");
            return;
        }
        send_to_char(ch, OK);

        if (ch->desc->snooping) {
            descriptor_data *tdesc = ch->desc->snooping;
            vector<descriptor_data *>::iterator vi = tdesc->snoop_by.begin();
            for (; vi != tdesc->snoop_by.end(); ++vi) {
                if (*vi == ch->desc) {
                    tdesc->snoop_by.erase(vi);
                    break;
                }
            }
        }

        slog("(GC) %s has begun to snoop %s.", 
			 GET_NAME(ch), GET_NAME(victim) );

        ch->desc->snooping = victim->desc;
        victim->desc->snoop_by.push_back(ch->desc);
    }
}



ACMD(do_switch)
{
    struct Creature *victim;

    one_argument(argument, arg);

    if (ch->desc->original)
        send_to_char(ch, "You're already switched.\r\n");
    else if (!*arg)
        send_to_char(ch, "Switch with who?\r\n");

    else if (!(victim = get_char_vis(ch, arg)))
        send_to_char(ch, "No such character.\r\n");
    else if (ch == victim)
        send_to_char(ch, "Hee hee... we are jolly funny today, eh?\r\n");
    else if (victim->desc)
        send_to_char(ch, "You can't do that, the body is already in use!\r\n");
    else if ((GET_LEVEL(ch) < LVL_IMPL) && !IS_NPC(victim))
        send_to_char(ch, "You aren't holy enough to use a mortal's body.\r\n");
    else if (GET_LEVEL(ch) <= GET_LEVEL(victim) && GET_IDNUM(ch) != 1)
        send_to_char(ch, "Nope.\r\n");
    else {
        send_to_char(ch, OK);

        ch->desc->creature = victim;
        ch->desc->original = ch;

        victim->desc = ch->desc;
        ch->desc = NULL;

        slog("(%s) %s has %sswitched into %s at %d.",
            subcmd == SCMD_QSWITCH ? "QC" : "GC", GET_NAME(ch),
            subcmd == SCMD_QSWITCH ? "q" : "",
            GET_NAME(victim), victim->in_room->number);
    }
}

ACMD(do_rswitch)
{
    struct Creature *orig, *victim;
    char arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg, arg2);

    if (!*arg)
        send_to_char(ch, "Switch with who?\r\n");
    else if (!*arg2)
        send_to_char(ch, "Switch them with who?\r\n");
    else if (!(orig = get_char_vis(ch, arg)))
        send_to_char(ch, "What player do you want to switch around?\r\n");
    else if (!orig->desc)
        send_to_char(ch, "No link there, sorry.\r\n");
    else if (orig->desc->original)
        send_to_char(ch, "They're already switched.\r\n");
    else if (!(victim = get_char_vis(ch, arg2)))
        send_to_char(ch, "No such character.\r\n");
    else if (orig == victim || ch == victim)
        send_to_char(ch, "Hee hee... we are jolly funny today, eh?\r\n");
    else if (victim->desc)
        send_to_char(ch, "You can't do that, the body is already in use!\r\n");
    else if ((GET_LEVEL(ch) < LVL_IMPL) && !IS_NPC(victim))
        send_to_char(ch, "You can't use a mortal's body like that!\r\n");
    else if ((GET_LEVEL(ch) < LVL_IMPL) && GET_LEVEL(orig) < LVL_AMBASSADOR)
        send_to_char(ch, "You cannot allow that mortal to do that.\r\n");
    else if (GET_LEVEL(ch) < GET_LEVEL(orig))
        send_to_char(ch, "Maybe you should just ask them.\r\n");
    else {
        send_to_char(ch, OK);
        send_to_char(orig, "The world seems to lurch and shift.\r\n");
        orig->desc->creature = victim;
        orig->desc->original = orig;

        victim->desc = orig->desc;
        orig->desc = NULL;

        slog("(GC) %s has rswitched %s into %s at %d.", GET_NAME(ch),
            GET_NAME(orig), GET_NAME(victim), victim->in_room->number);

    }
}

// This NASTY macro written by Nothing, spank me next time you see me if you want... :P
#define GET_START_ROOM(ch) (GET_HOME(ch) == HOME_MODRIAN ? r_mortal_start_room :\
                            GET_HOME(ch) == HOME_ELECTRO ? r_electro_start_room :\
                            GET_HOME(ch) == HOME_NEW_THALOS ? r_new_thalos_start_room :\
                            GET_HOME(ch) == HOME_KROMGUARD ? r_kromguard_start_room :\
                            GET_HOME(ch) == HOME_ELVEN_VILLAGE ? r_elven_start_room :\
                            GET_HOME(ch) == HOME_ISTAN ? r_istan_start_room :\
                            GET_HOME(ch) == HOME_MONK ? r_monk_start_room :\
                            GET_HOME(ch) == HOME_ARENA ? r_arena_start_room :\
                            GET_HOME(ch) == HOME_SKULLPORT ? r_skullport_start_room :\
                            GET_HOME(ch) == HOME_SOLACE_COVE ? r_solace_start_room :\
                            GET_HOME(ch) == HOME_MAVERNAL ? r_mavernal_start_room :\
                            GET_HOME(ch) == HOME_DWARVEN_CAVERNS ? r_dwarven_caverns_start_room :\
                            GET_HOME(ch) == HOME_HUMAN_SQUARE ? r_human_square_start_room :\
                            GET_HOME(ch) == HOME_SKULLPORT ? r_skullport_start_room :\
                            GET_HOME(ch) == HOME_DROW_ISLE ? r_drow_isle_start_room :\
                            GET_HOME(ch) == HOME_ASTRAL_MANSE ? r_astral_manse_start_room :\
                            GET_HOME(ch) == HOME_NEWBIE_SCHOOL ? r_newbie_school_start_room :\
                            r_mortal_start_room)
ACMD(do_return)
{
    struct Creature *orig = NULL;
    bool cloud_found = false;

    if (ch->desc && (orig = ch->desc->original)) {
        // Return from being switched or gasified
        if (IS_MOB(ch) && GET_MOB_VNUM(ch) == 1518) {
            cloud_found = true;
            if (subcmd == SCMD_RETURN) {
                send_to_char(ch, "Use recorporealize.\r\n");
                return;
            }
        }
        send_to_char(ch, "You return to your original body.\r\n");

        /* JE 2/22/95 */
        /* if someone switched into your original body, disconnect them */
        if (ch->desc->original->desc)
            set_desc_state(CXN_DISCONNECT, ch->desc->original->desc);

        ch->desc->creature = ch->desc->original;
        ch->desc->original = NULL;

        ch->desc->creature->desc = ch->desc;
        ch->desc = NULL;

        if (cloud_found) {
            char_from_room(orig,false);
            char_to_room(orig, ch->in_room,false);
            act("$n materializes from a cloud of gas.",
                FALSE, orig, 0, 0, TO_ROOM);
            if (subcmd != SCMD_NOEXTRACT)
                ch->purge(true);
        }
    } else if (!IS_NPC(ch) && !IS_REMORT(ch) && (GET_LEVEL(ch) < LVL_IMMORT)) {
        // Return to newbie start room
        if (ch->isFighting()) {
            send_to_char(ch, "No way!  You're fighting for your life!\r\n");
        } else if (GET_LEVEL(ch) <= LVL_CAN_RETURN) {
            act("A whirling globe of multi-colored light appears and whisks you away!", FALSE, ch, NULL, NULL, TO_CHAR);
            act("A whirling globe of multi-colored light appears and whisks $n away!", FALSE, ch, NULL, NULL, TO_ROOM);
            char_from_room(ch,false);
            char_to_room(ch, GET_START_ROOM(ch),false);
            look_at_room(ch, ch->in_room, 0);
            act("A whirling globe of multi-colored light appears and deposits $n on the floor!", FALSE, ch, NULL, NULL, TO_ROOM);
        } else
            send_to_char(ch, "There is no need to return.\r\n");
    } else {
        send_to_char(ch, "There is no need to return.\r\n");
    }
}

#undef GET_START_ROOM

ACMD(do_mload)
{
    struct Creature *mob;
    int number;

    one_argument(argument, buf);

    if (!*buf || !isdigit(*buf)) {
        send_to_char(ch, "Usage: mload <mobile vnum number>\r\n");
        return;
    }
    if ((number = atoi(buf)) < 0) {
        send_to_char(ch, "A NEGATIVE number??\r\n");
        return;
    }
    if (!real_mobile_proto(number)) {
        send_to_char(ch, "There is no mobile thang with that number.\r\n");
        return;
    }
    mob = read_mobile(number);
    char_to_room(mob, ch->in_room,false);

    act("$n makes a quaint, magical gesture with one hand.", TRUE, ch,
        0, 0, TO_ROOM);
    act("$n has created $N!", FALSE, ch, 0, mob, TO_ROOM);
    act("You create $N.", FALSE, ch, 0, mob, TO_CHAR);

    slog("(GC) %s mloaded %s at %d.", GET_NAME(ch), GET_NAME(mob),
        ch->in_room->number);

	if (GET_MOB_PROG(mob))
		trigger_prog_load(mob);
}

ACMD(do_oload)
{
    struct obj_data *obj = NULL;
    int number, quantity;
    char *temp, *temp2;

    temp = tmp_getword(&argument);
    temp2 = tmp_getword(&argument);
    
    if (!*temp || !isdigit(*temp)) {
        send_to_char(ch, "Usage: oload [quantity] <object vnum>\r\n");
        return;
    }
    
    if(!*temp2 || !isdigit(*temp2)) {
        number = atoi(temp);
        quantity = 1;
    }
    else {
        quantity = atoi(temp);
        number = atoi(temp2);
    }

    if (number < 0) {
        send_to_char(ch, "A NEGATIVE number??\r\n");
        return;
    }
    if (!real_object_proto(number)) {
        send_to_char(ch, "There is no object with that number.\r\n");
        return;
    }
    if(quantity < 0) {
        send_to_char(ch, "How do you expect to create negative objects?\r\n");
        return;
    }
    if(quantity == 0) {
        send_to_char(ch, "POOF!  Congratulations!  You've created nothing!\r\n");
        return;
    }
    if(quantity > 100) {
        send_to_char(ch, "You can't possibly need THAT many!\r\n");
        return;
    }

    for(int i = 0; i < quantity; i++) {
        obj = read_object(number);
		obj->creation_method = CREATED_IMM;
		obj->creator = GET_IDNUM(ch);
        obj_to_room(obj, ch->in_room);
    }
    act("$n makes a strange magical gesture.", TRUE, ch, 0, 0, TO_ROOM);
    if(quantity == 1) {
        act("$n has created $p!", FALSE, ch, obj, 0, TO_ROOM);
        act("You create $p.", FALSE, ch, obj, 0, TO_CHAR);
        slog("(GC) %s loaded %s at %d.", GET_NAME(ch),
            obj->name, ch->in_room->number);
    }
    else {
        act(tmp_sprintf("%s has created %s! (x%d)", GET_NAME(ch), obj->name, 
            quantity), FALSE, ch, obj, 0, TO_ROOM);
        act(tmp_sprintf("You create %s. (x%d)", obj->name, quantity), FALSE,
            ch, obj, 0, TO_CHAR);
        slog("(GC) %s loaded %s at %d. (x%d)", GET_NAME(ch), obj->name, 
            ch->in_room->number, quantity);
    }
}

ACMD(do_pload)
{
    struct Creature *vict = NULL;
    struct obj_data *obj = NULL;
    int number, quantity;
    char *temp, *temp2;

    temp = tmp_getword(&argument);
    temp2 = tmp_getword(&argument);
    
    if (!*temp || !isdigit(*temp)) {
        send_to_char(ch, "Usage: pload [quantity] <object vnum> [target char]\r\n");
        return;
    }
    if(!*temp2) {
        number = atoi(temp);
        quantity = 1;
    }
    //else, :)  there are two or three arguments
    else {
        //check to see if there's a quantity argument
        if(isdigit(*temp2)) {
            quantity = atoi(temp);
            number = atoi(temp2);
            //get the vict if there is one

            temp = tmp_getword(&argument);
            if(*temp) {
                if(!(vict = get_char_vis(ch, temp))) {
                    send_to_char(ch, NOPERSON);
                    return;
                }
            }
        }
        //second arg must be vict
        else {
            number = atoi(temp);
            quantity = 1;
            //find vict.  If no visible corresponding char, return
            if(!(vict = get_char_vis(ch, temp2))) {
                send_to_char(ch, NOPERSON);
                return;
            }
        }
    }
    
    if (number < 0) {
        send_to_char(ch, "A NEGATIVE number??\r\n");
        return;
    }
    if (quantity == 0) {
        send_to_char(ch, "POOF!  Congratulations!  You've created nothing!\r\n");
        return;
    }
    if (quantity < 0) {
        send_to_char(ch, "How do you expect to create negative objects?\r\n");
        return;
    }
    //put a cap on how many someone can load
    if (quantity > 100) {
        send_to_char(ch, "You can't possibly need THAT many!\r\n");
        return;
    }
    
    /*//no visible characters corresponding to vict.
    if (!vict) {
        send_to_char(ch, NOPERSON);
        return;
    }
    */
    if (!real_object_proto(number)) {
        send_to_char(ch, "There is no object with that number.\r\n");
        return;
    }
    if (vict) {
        for(int i=0; i < quantity; i++) {
            obj = read_object(number);
			obj->creation_method = CREATED_IMM;
			obj->creator = GET_IDNUM(ch);
            obj_to_char(obj, vict);
        }
        
        
        act("$n does something suspicious and alters reality.", TRUE, ch, 0, 0,
            TO_ROOM);
        
        //no quantity list if there's only one object.
        if(quantity == 1) {
            act("You load $p onto $N.", TRUE, ch, obj, vict, TO_CHAR);
            act("$N causes $p to appear in your hands.", TRUE, vict, obj, ch,
                TO_CHAR);
        }
        
        else {
            act(tmp_sprintf("You load %s onto %s. (x%d)", obj->name, 
                    GET_NAME(vict), quantity), FALSE, ch, obj, vict, TO_CHAR);
            act(tmp_sprintf("%s causes %s to appear in your hands. (x%d)", 
                    GET_NAME(ch), obj->name, quantity), FALSE, ch, obj, 
                    vict, TO_VICT);
        }
        
        
        
    } else {
        act("$n does something suspicious and alters reality.", TRUE, ch, 0, 0,
            TO_ROOM);
        for(int i=0; i < quantity; i++) {
            obj = read_object(number);
			obj->creation_method = CREATED_IMM;
			obj->creator = GET_IDNUM(ch);
            obj_to_char(obj, ch);
        }
        
        if(quantity == 1) {
            act("You create $p.", FALSE, ch, obj, 0, TO_CHAR);
        } 
        else {
            act(tmp_sprintf("You create %s. (x%d)", obj->name, 
                quantity), FALSE, ch, obj, 0, TO_CHAR);
        }
    }
    if(quantity == 1) {
        slog("(GC) %s ploaded %s on %s.", GET_NAME(ch), obj->name,
            vict ? GET_NAME(vict) : GET_NAME(ch));
    }
    else {
        slog("(GC) %s ploaded %s on %s. (x%d)", GET_NAME(ch), 
            obj->name, vict ? GET_NAME(vict) : GET_NAME(ch), 
            quantity);
    }
}


ACMD(do_vstat)
{
    struct Creature *mob = NULL;
    struct obj_data *obj = NULL;
    bool mob_stat = false;
    char *str;
    int number, tmp;

    str = tmp_getword(&argument);

    if (!*str) {
        send_to_char(ch, "Usage: vstat { { obj | mob } <number> | <alias> }\r\n");
        return;
    } else if ((mob_stat = is_abbrev(str, "mobile")) ||
               is_abbrev(str, "object")) {
        str = tmp_getword(&argument);
        if (!is_number(str)) {
            send_to_char(ch, "You must specify a vnum when not using an alias.\r\n");
            return;
        }
        number = atoi(str);
    } else if ((obj = get_object_in_equip_vis(ch, str, ch->equipment, &tmp)))
        number = GET_OBJ_VNUM(obj);
    else if ((obj = get_obj_in_list_vis(ch, str, ch->carrying)))
        number = GET_OBJ_VNUM(obj);
    else if ((mob = get_char_room_vis(ch, str))) {
        number = GET_MOB_VNUM(mob);
        mob_stat = true;
    } else if ((obj = get_obj_in_list_vis(ch, str, ch->in_room->contents)))
        number = GET_OBJ_VNUM(obj);
    else if ((mob = get_char_vis(ch, str))) {
        number = GET_MOB_VNUM(mob);
        mob_stat = true;
    } else if ((obj = get_obj_vis(ch, str)))
        number = GET_OBJ_VNUM(obj);
    else {
        send_to_char(ch, "Nothing around by that name.\r\n");
        return;
    }

    if (number < 0) {
        send_to_char(ch, "A NEGATIVE number??\r\n");
        return;
    }

    if (mob_stat) {
        mob = real_mobile_proto(number);
        if (mob) {
            do_stat_character(ch, mob, argument);
        } else {
            send_to_char(ch, "There is no mobile with that vnum.\r\n");
            return;
        }
    } else {
        obj = real_object_proto(number);
        if (obj) {
            do_stat_object(ch, obj);
        } else {
            send_to_char(ch, "There is no object with that vnum.\r\n");
            return;
        }
    }
}

ACMD(do_rstat)
{
    struct Creature *mob;
    int number;

    two_arguments(argument, buf, buf2);

    if (!*buf || !*buf2 || !isdigit(*buf2)) {
        send_to_char(ch, "Usage: rstat { obj | mob } <id number>\r\n");
        return;
    }
    if ((number = atoi(buf2)) < 0) {
        send_to_char(ch, "A NEGATIVE number??\r\n");
        return;
    }
    if (is_abbrev(buf, "mob")) {
        if (!(mob = get_char_in_world_by_idnum(-number)))
            send_to_char(ch, "There is no monster with that id number.\r\n");
        else
            do_stat_character(ch, mob, "");
    } else if (is_abbrev(buf, "obj")) {
        send_to_char(ch, "Not enabled.\r\n");
    } else
        send_to_char(ch, "That'll have to be either 'obj' or 'mob'.\r\n");
}




/* clean a room of all mobiles and objects */
ACMD(do_purge)
{
    struct Creature *vict;
    struct obj_data *obj, *next_o;
    struct room_trail_data *trail;
    int i = 0;

    one_argument(argument, buf);

    if (*buf) {                    /* argument supplied. destroy single object
                                   * or char */

        if (!strncmp(buf, "trails", 6)) {
            while ((trail = ch->in_room->trail)) {
                i++;
                if (trail->name)
                    free(trail->name);
                if (trail->aliases)
                    free(trail->aliases);
                ch->in_room->trail = trail->next;
                free(trail);
            }
            send_to_char(ch, "%d trails purged from room.\r\n", i);
            return;
        }

        if ((vict = get_char_room_vis(ch, buf))) {
            if (!IS_NPC(vict) && (GET_LEVEL(ch) <= GET_LEVEL(vict))) {
                send_to_char(ch, "Fuuuuuuuuu!\r\n");
                return;
            }
            act("$n disintegrates $N.", FALSE, ch, 0, vict, TO_NOTVICT);

            if (!IS_NPC(vict)) {
                mudlog(LVL_POWER, BRF, true,
                    "(GC) %s has purged %s at %d.",
                    GET_NAME(ch), GET_NAME(vict), vict->in_room->number);
            }
            vict->purge(false);
        } else if ((obj = get_obj_in_list_vis(ch, buf, ch->in_room->contents))) {
            act("$n destroys $p.", FALSE, ch, obj, 0, TO_ROOM);
            slog("(GC) %s purged %s at %d.", GET_NAME(ch),
                obj->name, ch->in_room->number);
            extract_obj(obj);

        } else {
            send_to_char(ch, "Nothing here by that name.\r\n");
            return;
        }

        send_to_char(ch, OK);
    } else {                    /* no argument. clean out the room */
        act("$n gestures... You are surrounded by scorching flames!",
            FALSE, ch, 0, 0, TO_ROOM);
        send_to_room("The world seems a little cleaner.\r\n", ch->in_room);

        CreatureList::iterator it = ch->in_room->people.begin();
        for (; it != ch->in_room->people.end(); ++it) {
            if (IS_NPC((*it))) {
                (*it)->purge(true);
            }
        }

        for (obj = ch->in_room->contents; obj; obj = next_o) {
            next_o = obj->next_content;
            extract_obj(obj);
        }

        while (ch->in_room->affects)
            affect_from_room(ch->in_room, ch->in_room->affects);
        slog("(GC) %s has purged room %s.", GET_NAME(ch),
            ch->in_room->name);

    }
}



ACMD(do_advance)
{
    struct Creature *victim;
    char *name = arg, *level = buf2;
    int newlevel;
    void do_start(struct Creature *ch, int mode);

    two_arguments(argument, name, level);

    if (*name) {
        if (!(victim = get_char_vis(ch, name))) {
            send_to_char(ch, "That player is not here.\r\n");
            return;
        }
    } else {
        send_to_char(ch, "Advance who?\r\n");
        return;
    }
    if (GET_LEVEL(ch) <= GET_LEVEL(victim)) {
        send_to_char(ch, "Maybe that's not such a great idea.\r\n");
        return;
    }
    if (IS_NPC(victim)) {
        send_to_char(ch, "NO!  Not on NPC's.\r\n");
        return;
    }
    if (!*level || (newlevel = atoi(level)) <= 0) {
        send_to_char(ch, "That's not a level!\r\n");
        return;
    }
    if (newlevel > LVL_GRIMP) {
        send_to_char(ch, "%d is the highest possible level.\r\n", LVL_GRIMP);
        return;
    }
    if (newlevel > GET_LEVEL(ch)) {
        send_to_char(ch, "Yeah, right.\r\n");
        return;
    }
    if (newlevel < GET_LEVEL(victim)) {
        do_start(victim, FALSE);
        GET_LEVEL(victim) = newlevel;
    } else {
        act("$n makes some strange gestures.\r\n"
            "A strange feeling comes upon you,\r\n"
            "Like a giant hand, light comes down\r\n"
            "from above, grabbing your body, that\r\n"
            "begins to pulse with colored lights\r\n"
            "from inside.\r\n\r\n"
            "Your head seems to be filled with demons\r\n"
            "from another plane as your body dissolves\r\n"
            "to the elements of time and space itself.\r\n"
            "Suddenly a silent explosion of light\r\n"
            "snaps you back to reality.\r\n\r\n"
            "You feel slightly different.", FALSE, ch, 0, victim, TO_VICT);
    }

    send_to_char(ch, OK);

    mudlog(MAX(GET_INVIS_LVL(ch), GET_INVIS_LVL(victim)), NRM, true,
        "(GC) %s has advanced %s to level %d (from %d)",
        GET_NAME(ch), GET_NAME(victim), 
        newlevel, GET_LEVEL(victim) );
    gain_exp_regardless(victim, exp_scale[newlevel] - GET_EXP(victim));
    victim->saveToXML();
}

ACMD(do_restore)
{
    struct Creature *vict;

    one_argument(argument, buf);
    if (!*buf)
        send_to_char(ch, "Whom do you wish to restore?\r\n");
    else if (!(vict = get_char_vis(ch, buf))) {
        send_to_char(ch, NOPERSON);
    } else {
		vict->restore();
        send_to_char(ch, OK);
        act("You have been fully healed by $N!", FALSE, vict, 0, ch, TO_CHAR);
        mudlog(GET_LEVEL(ch), CMP, true,
            "%s has been restored by %s.", GET_NAME(vict), GET_NAME(ch));
    }
}

void
perform_vis(struct Creature *ch)
{

    int level = GET_INVIS_LVL(ch);

    if (!GET_INVIS_LVL(ch) && !IS_AFFECTED(ch, AFF_HIDE | AFF_INVISIBLE)) {
        send_to_char(ch, "You are already fully visible.\r\n");
        return;
    }

    GET_INVIS_LVL(ch) = 0;
    CreatureList::iterator it = ch->in_room->people.begin();
    for (; it != ch->in_room->people.end(); ++it) {
        if ((*it) == ch || !can_see_creature((*it), ch))
            continue;
        if (GET_LEVEL(*it) < level)
			act("you suddenly realize that $n is standing beside you.",
				false, ch, 0, (*it), TO_VICT);
    }

    send_to_char(ch, "You are now fully visible.\r\n");
}


void
perform_invis(struct Creature *ch, int level)
{
    int old_level;
    
    if (IS_NPC(ch))
        return;

    // We set the invis level to 0 here because of a logic problem with
    // can_see_creature().  If we keep the old level, people won't be able to
    // see people appear, and if we set the new level here, people won't be
    // able to see them disappear.  Setting the invis level to 0 ensures
    // that we can still take invisibility/transparency into account.
    old_level = GET_INVIS_LVL(ch);
    GET_INVIS_LVL(ch) = 0;

    CreatureList::iterator it = ch->in_room->people.begin();
    for (; it != ch->in_room->people.end(); ++it) {
        if ((*it) == ch || !can_see_creature((*it), ch))
            continue;

        if (GET_LEVEL(*it) < old_level && GET_LEVEL(*it) >= level) {
            if (GET_LEVEL(ch) >= LVL_AMBASSADOR)
                act("you suddenly realize that $n is standing beside you.",
                    false, ch, 0, (*it), TO_VICT);
            else if (GET_REMORT_GEN(*it) <= GET_REMORT_GEN(ch))
                act("$n suddenly appears from the thin air beside you.",
                    false, ch, 0, (*it), TO_VICT);
        }

        if (GET_LEVEL(*it) >= old_level && GET_LEVEL(*it) < level ) {
            if (GET_LEVEL(ch) >= LVL_AMBASSADOR)
                act("You blink and suddenly realize that $n is gone.",
                    false, ch, 0, (*it), TO_VICT);
            else if (GET_REMORT_GEN(*it) <= GET_REMORT_GEN(ch))
                act("$n suddenly vanishes from your reality.",
                    FALSE, ch, 0, (*it), TO_VICT);
        }
    }

    GET_INVIS_LVL(ch) = level;
    send_to_char(ch, "Your invisibility level is %d.\r\n", level);
}


ACMD(do_invis)
{
    char *arg;
    int level;

    if (IS_NPC(ch)) {
        send_to_char(ch, "You can't do that!\r\n");
        return;
    }

    if (GET_LEVEL(ch) < LVL_AMBASSADOR && !IS_REMORT(ch)) {
        send_unknown_cmd(ch);
        return;
    }

    arg = tmp_getword(&argument);
    if (!*arg) {
        if (GET_INVIS_LVL(ch) > 0)
            perform_vis(ch);
        else
            perform_invis(ch, MIN(GET_LEVEL(ch), 70));
    } else {
        level = MIN(atoi(arg), 70);
        if (level > GET_LEVEL(ch)) {
            send_to_char(ch, "You can't go invisible above your own level.\r\n");
            return;
        }

        if (level < 1)
            perform_vis(ch);
        else
            perform_invis(ch, level);
    }
}


ACMD(do_gecho)
{
    struct descriptor_data *pt;

    skip_spaces(&argument);

    if (!*argument)
        send_to_char(ch, "That must be a mistake...\r\n");
    else {
        for (pt = descriptor_list; pt; pt = pt->next) {
            if (pt->input_mode == CXN_PLAYING &&
				pt->creature && pt->creature != ch &&
                !PRF2_FLAGGED(pt->creature, PRF2_NOGECHO) &&
                !PLR_FLAGGED(pt->creature, PLR_OLC | PLR_WRITING)) {
                if (GET_LEVEL(pt->creature) >= 50 &&
                    GET_LEVEL(pt->creature) > GET_LEVEL(ch))
                    send_to_char(pt->creature, "[%s-g] %s\r\n", GET_NAME(ch), argument);
                else
                    send_to_char(pt->creature, "%s\r\n", argument);
            }
        }
		send_to_char(ch, "%s\r\n", argument);
    }
}

ACMD(do_zecho)
{
    struct descriptor_data *pt;
    struct zone_data *here;
    // charmed mobs and players < LVL_GOD cant use this
    if ((!IS_NPC(ch) && !Security::isMember(ch, "WizardBasic"))
        || (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM))) {
        send_to_char(ch, "You probably shouldn't be using this.\r\n");
    }
    if (ch->in_room && ch->in_room->zone) {
        here = ch->in_room->zone;
    } else {
        return;
    }

    skip_spaces(&argument);

    if (!*argument)
        send_to_char(ch, "That must be a mistake...\r\n");
    else {
        for (pt = descriptor_list; pt; pt = pt->next) {
            if (pt->input_mode == CXN_PLAYING && 
				pt->creature && pt->creature != ch &&
                pt->creature->in_room && pt->creature->in_room->zone == here
                && !PRF2_FLAGGED(pt->creature, PRF2_NOGECHO)
                && !PLR_FLAGGED(pt->creature, PLR_OLC | PLR_WRITING)) {
                if (GET_LEVEL(pt->creature) > GET_LEVEL(ch))
                    send_to_char(pt->creature, "[%s-zone] %s\r\n", GET_NAME(ch), argument);
                else
                    send_to_char(pt->creature, "%s\r\n", argument);
            }
        }
		send_to_char(ch, "%s", argument);
    }
}

ACMD(do_oecho)
{

    skip_spaces(&argument);

    if (!*argument)
        send_to_char(ch, "That must be a mistake...\r\n");
    else {
        send_to_char(ch, "%s\r\n", argument);
		send_to_outdoor(argument, 1);
    }
}

ACMD(do_poofset)
{
    char **msg;

    switch (subcmd) {
    case SCMD_POOFIN:
        msg = &(POOFIN(ch));
        break;
    case SCMD_POOFOUT:
        msg = &(POOFOUT(ch));
        break;
    default:
        return;
    }

    skip_spaces(&argument);

    if (*msg) {
        free(*msg);
    }

    if (!*argument)
        *msg = NULL;
    else {
        *msg = str_dup(argument);
    }
    send_to_char(ch, OK);
}



ACMD(do_dc)
{
    struct descriptor_data *d;
    int num_to_dc;

    one_argument(argument, arg);
    if (!(num_to_dc = atoi(arg))) {
        send_to_char(ch, 
            "Usage: DC <connection number> (type USERS for a list)\r\n");
        return;
    }
    for (d = descriptor_list; d && d->desc_num != num_to_dc; d = d->next);

    if (!d) {
        send_to_char(ch, "No such connection.\r\n");
        return;
    }
    if (d->creature && GET_LEVEL(d->creature) >= GET_LEVEL(ch)) {
        send_to_char(ch, "Umm.. maybe that's not such a good idea...\r\n");
        return;
    }
    set_desc_state(CXN_DISCONNECT, d);
    send_to_char(ch, "Connection #%d closed.\r\n", num_to_dc);
    slog("(GC) Connection closed by %s.", GET_NAME(ch));
}

ACMD(do_wizcut)
{
    int level;
    struct descriptor_data *d;

    one_argument(argument, arg);

    if (*arg) {
        level = atoi(arg);

        if (level <= 0 || level >= LVL_IMMORT) {
            send_to_char(ch, "You can only use wizcut on mortals.\r\n");
            return;
        }

        for (d = descriptor_list; d; d = d->next)
            if (d->creature && GET_LEVEL(d->creature) <= level)
                set_desc_state(CXN_DISCONNECT, d);

        send_to_char(ch,
                "All players level %d and below have been disconnected.\r\n",
                level );
        slog("(GC) %s wizcut everyone level %d and below.\r\n",
            GET_NAME(ch), 
            level);
    }
}

ACMD(do_wizlock)
{
    int value;
    char *when;

    one_argument(argument, arg);
    if (*arg) {
        value = atoi(arg);
        if (value < 0 || value > GET_LEVEL(ch)) {
            send_to_char(ch, "Invalid wizlock value.\r\n");
            return;
        }
        restrict = value;
        when = "now";
    } else
        when = "currently";

    switch (restrict) {
    case 0:
        send_to_char(ch, "The game is %s completely open.\r\n", when);
        break;
    case 1:
        send_to_char(ch, "The game is %s closed to new players.\r\n", when);
        break;
    default:
        send_to_char(ch, "Only level %d and above may enter the game %s.\r\n",
            restrict, when);
        break;
    }
    mudlog(MAX(LVL_AMBASSADOR, GET_INVIS_LVL(ch)), CMP, false,
        "(GC) %s has set wizlock to %d.", GET_NAME(ch), restrict);
}


ACMD(do_date)
{
    char *tmstr;
    time_t mytime;
    int d, h, m;
    extern time_t boot_time;

    if (subcmd == SCMD_DATE)
        mytime = time(0);
    else
        mytime = boot_time;

    tmstr = (char *)asctime(localtime(&mytime));
    *(tmstr + strlen(tmstr) - 1) = '\0';

    if (subcmd == SCMD_DATE)
        send_to_char(ch, "Current machine time: %s\r\n", tmstr);
    else {
        mytime = time(0) - boot_time;
        d = mytime / 86400;
        h = (mytime / 3600) % 24;
        m = (mytime / 60) % 60;

        send_to_char(ch, "Up since %s: %d day%s, %d:%02d\r\n", tmstr, d,
            ((d == 1) ? "" : "s"), h, m);
    }

}



ACMD(do_last)
{
	int pid;
	Creature *vict;

    one_argument(argument, arg);
    if (!*arg) {
        send_to_char(ch, "For whom do you wish to search?\r\n");
        return;
    }
	pid = playerIndex.getID(arg);
    if (pid <= 0) {
        send_to_char(ch, "There is no such player.\r\n");
        return;
    }
	vict = new Creature(true);
	if (!vict->loadFromXML(pid)) {
		send_to_char(ch, "There was an error.\r\n");
		slog("Unable to load character for 'LAST'.");
		delete vict;
		return;
	}
    if ((GET_LEVEL(vict) > GET_LEVEL(ch)) && (GET_LEVEL(ch) < LVL_GRIMP)) {
        send_to_char(ch, "You are not sufficiently godly for that!\r\n");
		delete vict;
        return;
    }

    send_to_char(ch, "[%5ld] [%2d %s] %-12s : %-20s",
        GET_IDNUM(vict), GET_LEVEL(vict),
        char_class_abbrevs[GET_CLASS(vict)], GET_NAME(vict),
        ctime(&(vict->player.time.logon)) );
    if (GET_LEVEL(ch) >= GET_LEVEL(vict) && has_mail(GET_IDNUM(vict)))
        send_to_char(ch, "Player has unread mail.\r\n");
	delete vict;
}


ACMD(do_force)
{
    struct descriptor_data *i, *next_desc;
    struct Creature *vict;
    CreatureList::iterator cit;
    char to_force[MAX_INPUT_LENGTH + 2];

    half_chop(argument, arg, to_force);

    sprintf(buf1, "$n has forced you to '%s'.", to_force);

    if (!*arg || !*to_force) {
        send_to_char(ch, "Whom do you wish to force do what?\r\n");
        return;
    }

    if (GET_LEVEL(ch) >= LVL_GRGOD) {
        // Check for high-level special arguments
        if (!strcasecmp("all", arg) &&
            Security::isMember(ch, "Coder")) {
            send_to_char(ch, OK);
            mudlog(GET_LEVEL(ch), NRM, true,
                   "(GC) %s forced all to %s", GET_NAME(ch), to_force);
            for (cit = characterList.begin();
                 cit != characterList.end();
                 ++cit) {
                if (ch != (*cit) && GET_LEVEL(ch) > GET_LEVEL(*cit)) {
                    act(buf1, TRUE, ch, NULL, *cit, TO_VICT);
                    command_interpreter(*cit, to_force);
                }
            }
            return;
        }

        if (!strcasecmp("players", arg)) {
            send_to_char(ch, OK);
            mudlog(GET_LEVEL(ch), NRM, true,
                   "(GC) %s forced players to %s", GET_NAME(ch), to_force);

            for (i = descriptor_list; i; i = next_desc) {
                next_desc = i->next;

                if (STATE(i) || !(vict = i->creature) ||
                    GET_LEVEL(vict) >= GET_LEVEL(ch))
                    continue;
                act(buf1, TRUE, ch, NULL, vict, TO_VICT);
                command_interpreter(vict, to_force);
            }
            return;
        }
        
        if (!strcasecmp("room", arg)) {
            send_to_char(ch, OK);
            mudlog(GET_LEVEL(ch), NRM, true, "(GC) %s forced room %d to %s",
                   GET_NAME(ch), ch->in_room->number, to_force);
            CreatureList::iterator it = ch->in_room->people.begin();
            for (; it != ch->in_room->people.end(); ++it) {
                if (GET_LEVEL((*it)) >= GET_LEVEL(ch) ||
                    (!IS_NPC((*it)) && GET_LEVEL(ch) < LVL_GRGOD))
                    continue;
                act(buf1, TRUE, ch, NULL, (*it), TO_VICT);
                command_interpreter((*it), to_force);
            }
            return;
        }
    }

    // Normal individual creature forcing
    if (!(vict = get_char_vis(ch, arg))) {
        send_to_char(ch, NOPERSON);
    } else if (GET_LEVEL(ch) <= GET_LEVEL(vict))
        send_to_char(ch, "No, no, no!\r\n");
    else if (!IS_NPC(vict) && !Security::isMember(ch, "WizardFull"))
        send_to_char(ch, "You cannot force players to do things.\r\n");
    else {
        send_to_char(ch, OK);
        act(buf1, TRUE, ch, NULL, vict, TO_VICT);
        mudlog(GET_LEVEL(ch), NRM, true, "(GC) %s forced %s to %s",
               GET_NAME(ch), GET_NAME(vict), to_force);
        command_interpreter(vict, to_force);
    }
}



ACMD(do_wiznet)
{
    struct descriptor_data *d;
    char emote = FALSE;
    char any = FALSE;
    int level = LVL_AMBASSADOR;
    char *subcmd_str = NULL;
    char *subcmd_desc = NULL;

    skip_spaces(&argument);

    if (!*argument) {
        if (subcmd == SCMD_IMMCHAT)
            send_to_char(ch, 
                "Usage: imm <text> | #<level> <text> | *<emotetext> |\r\n "
                "       imm @<level> *<emotetext> | imm @\r\n");
        else
            send_to_char(ch, 
                "Usage: wiz <text> | #<level> <text> | *<emotetext> |\r\n "
                "       wiz @<level> *<emotetext> | wiz @\r\n");
        return;
    }

    if (subcmd == SCMD_WIZNET)
        level = LVL_IMMORT;

    switch (*argument) {
    case '*':
        emote = TRUE;
    case '#':
        one_argument(argument + 1, buf1);
        if (is_number(buf1)) {
            half_chop(argument + 1, buf1, argument);
            level = MAX(atoi(buf1), LVL_AMBASSADOR);
            if (level > GET_LEVEL(ch)) {
                send_to_char(ch, "You can't %s above your own level.\r\n",
                    subcmd == SCMD_IMMCHAT ? "immchat" : "wiznet");
                return;
            }
        } else if (emote)
            argument++;
        break;
    case '@':
        for (d = descriptor_list; d; d = d->next) {
            if (STATE(d) == CXN_PLAYING && GET_LEVEL(d->creature) >=
                (subcmd == SCMD_IMMCHAT ? LVL_AMBASSADOR : LVL_DEMI) &&
                ((subcmd == SCMD_IMMCHAT &&
                        !PRF2_FLAGGED(d->creature, PRF2_NOIMMCHAT)) ||
                    (subcmd == SCMD_WIZNET &&
                        Security::isMember(d->creature, "WizardBasic") &&
						!PRF_FLAGGED(d->creature, PRF_NOWIZ))) &&
                (can_see_creature(ch, d->creature) || GET_LEVEL(ch) == LVL_GRIMP)) {
                if (!any) {
                    sprintf(buf1, "Gods online:\r\n");
                    any = TRUE;
                }
                sprintf(buf1, "%s  %s", buf1, GET_NAME(d->creature));
                if (PLR_FLAGGED(d->creature, PLR_WRITING))
                    sprintf(buf1, "%s (Writing)\r\n", buf1);
                else if (PLR_FLAGGED(d->creature, PLR_MAILING))
                    sprintf(buf1, "%s (Writing mail)\r\n", buf1);
                else if (PLR_FLAGGED(d->creature, PLR_OLC))
                    sprintf(buf1, "%s (Creating)\r\n", buf1);
                else
                    sprintf(buf1, "%s\r\n", buf1);

            }
        }

        if (!any)                /* hacked bug fix */
            strcpy(buf1, "");

        any = FALSE;
        for (d = descriptor_list; d; d = d->next) {
            if (STATE(d) == CXN_PLAYING
                && GET_LEVEL(d->creature) >= LVL_AMBASSADOR
                && ((subcmd == SCMD_IMMCHAT
                        && PRF2_FLAGGED(d->creature, PRF2_NOIMMCHAT))
                    || (subcmd == SCMD_WIZNET
                        && Security::isMember(d->creature, "WizardBasic")
                        && PRF_FLAGGED(d->creature, PRF_NOWIZ)))
                && can_see_creature(ch, d->creature)) {
                if (!any) {
                    sprintf(buf1, "%sGods offline:\r\n", buf1);
                    any = TRUE;
                }
                send_to_char(ch, "%s  %s\r\n", buf1, GET_NAME(d->creature));
            }
        }
        return;
    case '\\':
        ++argument;
        break;
    default:
        break;
    }
    if ((subcmd == SCMD_WIZNET && PRF_FLAGGED(ch, PRF_NOWIZ)) ||
        (subcmd == SCMD_IMMCHAT && PRF2_FLAGGED(ch, PRF2_NOIMMCHAT))) {
        send_to_char(ch, "You are offline!\r\n");
        return;
    }
    skip_spaces(&argument);

    if (!*argument) {
        send_to_char(ch, "Don't bother the gods like that!\r\n");
        return;
    }

    switch (subcmd) {
    case SCMD_IMMCHAT:
        subcmd_str = "imm";
        subcmd_desc = " imms";
        break;
    case SCMD_WIZNET:
        subcmd_str = "wiz";
        subcmd_desc = "";
        break;
    }

    if ((subcmd == SCMD_IMMCHAT && level > LVL_AMBASSADOR) ||
        (subcmd == SCMD_WIZNET && level > LVL_IMMORT)) {
        sprintf(buf1, "%s%s: <%d> %s\r\n", GET_NAME(ch),
            subcmd_desc, level, argument);
        sprintf(buf2, "Someone%s: <%d> %s\r\n", subcmd_desc, level, argument);
    } else {
        if (emote) {
            sprintf(buf1, "<%s> %s%s%s\r\n",
                subcmd_str, GET_NAME(ch),
				(*argument != '\'') ? " ":"", argument);
            sprintf(buf2, "<%s> Someone%s%s\r\n", subcmd_str,
				(*argument != '\'') ? " ":"", argument);
        } else {
            sprintf(buf1, "%s%s: %s\r\n", GET_NAME(ch), subcmd_desc, argument);
            sprintf(buf2, "Someone%s: %s\r\n", subcmd_desc, argument);
        }
    }

    for (d = descriptor_list; d; d = d->next) {
        if ((STATE(d) == CXN_PLAYING) && (GET_LEVEL(d->creature) >= level) &&
            (subcmd != SCMD_WIZNET
                || Security::isMember(d->creature, "WizardBasic"))
            && (subcmd != SCMD_WIZNET || !PRF_FLAGGED(d->creature, PRF_NOWIZ))
            && (subcmd != SCMD_IMMCHAT
                || !PRF2_FLAGGED(d->creature, PRF2_NOIMMCHAT))
            && (!PLR_FLAGGED(d->creature,
				PLR_WRITING | PLR_MAILING | PLR_OLC))) {

            if (subcmd == SCMD_IMMCHAT) {
                send_to_char(d->creature, CCYEL(d->creature, C_SPR));
            } else {
                send_to_char(d->creature, CCCYN(d->creature, C_SPR));
            }
            if (can_see_creature(d->creature, ch))
                send_to_char(d->creature, "%s", buf1);
            else
                send_to_char(d->creature, "%s", buf2);

            send_to_char(d->creature, CCNRM(d->creature, C_SPR));
        }
    }
}



ACMD(do_zreset)
{
    void reset_zone(struct zone_data *zone);
    struct zone_data *zone;

    int j;

    one_argument(argument, arg);
    if (!*arg) {
        send_to_char(ch, "You must specify a zone.\r\n");
        return;
    }
    if (*arg == '*') {
        for (zone = zone_table; zone; zone = zone->next)
            reset_zone(zone);

        send_to_char(ch, "Reset world.\r\n");
        mudlog(MAX(LVL_GRGOD, GET_INVIS_LVL(ch)), NRM, true,
            "(GC) %s reset entire world.", GET_NAME(ch));
        return;
    } else if (*arg == '.')
        zone = ch->in_room->zone;
    else {
        j = atoi(arg);
        for (zone = zone_table; zone; zone = zone->next)
            if (zone->number == j)
                break;
    }
    if (zone) {
        reset_zone(zone);
        send_to_char(ch, "Reset zone %d : %s.\r\n", zone->number, zone->name);
		act("$n waves $s hand.", false, ch, 0, 0, TO_ROOM);
		send_to_zone("You feel a strangely refreshing breeze.\r\n", zone, 0);
        slog("(GC) %s %sreset zone %d (%s)", GET_NAME(ch),
            subcmd == SCMD_OLC ? "olc-" : "", zone->number, zone->name);

    } else
        send_to_char(ch, "Invalid zone number.\r\n");
}


/*
 *  General fn for wizcommands of the sort: cmd <player>
 */

ACMD(do_wizutil)
{
    struct Creature *vict;
    long result;
    char *msg;
    void roll_real_abils(struct Creature *ch);
    char *arg = tmp_getword(&argument);
    bool loaded = false;

    // Make sure they entered something useful
    if (!*arg) {
        send_to_char(ch, "Yes, but for whom?!?\r\n");
        return;
    }

    // Make sure they specified a valid player name
    if (!playerIndex.exists(arg)) {
        send_to_char(ch, "There is no such player.\r\n");
        return;
    }
            
    // Get the player or load it from file
    vict = get_char_vis(ch, arg);
    if (!vict) {
        vict = new Creature(true);
        if (!vict->loadFromXML(playerIndex.getID(arg))) {
            send_to_char(ch, "That player could not be loaded.\r\n");
            delete vict;
            return;
        }
        loaded = true;
    }

    if (IS_NPC(vict))
        send_to_char(ch, "You can't do that to a mob!\r\n");
    else if (GET_LEVEL(vict) > GET_LEVEL(ch))
        send_to_char(ch, "Hmmm...you'd better not.\r\n");
    else {
        switch (subcmd) {
        case SCMD_REROLL:
            roll_real_abils(vict);
            send_to_char(vict, "Your stats have been rerolled.\r\n");
            slog("(GC) %s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
            send_to_char(ch,
                "New stats: Str %d/%d, Int %d, Wis %d, Dex %d, Con %d, Cha %d\r\n",
                GET_STR(vict), GET_ADD(vict), GET_INT(vict), GET_WIS(vict),
                GET_DEX(vict), GET_CON(vict), GET_CHA(vict));
            break;
        case SCMD_NOTITLE: {
            result = PLR_TOG_CHK(vict, PLR_NOTITLE);
            msg = tmp_sprintf("Notitle %s for %s by %s.", 
                                    ONOFF(result),
                                    GET_NAME(vict), 
                                    GET_NAME(ch));
            send_to_char(ch,"%s\r\n",msg);
            mudlog(MAX(LVL_GOD, GET_INVIS_LVL(ch)), NRM, true, "(GC) %s", msg);
            break;
        }
        case SCMD_NOPOST: {
            result = PLR_TOG_CHK(vict, PLR_NOPOST);
            msg = tmp_sprintf("Nopost %s for %s by %s.", 
                                    ONOFF(result),
                                    GET_NAME(vict), 
                                    GET_NAME(ch));
            send_to_char(ch,"%s\r\n",msg);
            mudlog(MAX(LVL_GOD, GET_INVIS_LVL(ch)), NRM, true, "(GC) %s", msg);
            break;
        }
        case SCMD_SQUELCH: {
            result = PLR_TOG_CHK(vict, PLR_NOSHOUT);
            msg = tmp_sprintf("Squelch %s for %s by %s.", 
                                    ONOFF(result),
                                    GET_NAME(vict), 
                                    GET_NAME(ch) );
            send_to_char(ch,"%s\r\n",msg);
            mudlog(MAX(LVL_GOD, GET_INVIS_LVL(ch)), NRM, true, "(GC) %s", msg);
            break;
        }
        case SCMD_FREEZE:
			do_freeze_char(argument, vict, ch);
            break;
            
        case SCMD_THAW:
            if (!PLR_FLAGGED(vict, PLR_FROZEN)) {
                send_to_char(ch, 
                    "Sorry, your victim is not morbidly encased in ice at the moment.\r\n");
                break;
            }
            if (GET_FREEZE_LEV(vict) > GET_LEVEL(ch)) {
                sprintf(buf,
                    "Sorry, a level %d God froze %s... you can't unfreeze %s.\r\n",
                    GET_FREEZE_LEV(vict), GET_NAME(vict), HMHR(vict));
                send_to_char(ch, "%s", buf);
                break;
            }
            mudlog(MAX(LVL_POWER, GET_INVIS_LVL(ch)), BRF, true,
                "(GC) %s un-frozen by %s.", GET_NAME(vict),
                GET_NAME(ch));
            REMOVE_BIT(PLR_FLAGS(vict), PLR_FROZEN);
            send_to_char(vict, 
                "A fireball suddenly explodes in front of you, melting the ice!\r\nYou feel thawed.\r\n");
            send_to_char(ch, "Thawed.\r\n");
            if (vict->in_room)
                act("A sudden fireball conjured from nowhere thaws $n!", FALSE,
                    vict, 0, 0, TO_ROOM);
            break;
        case SCMD_UNAFFECT:
            if (vict->affected) {
                send_to_char(vict, "There is a brief flash of light!\r\n"
                    "You feel slightly different.\r\n");
                send_to_char(ch, "All spells removed.\r\n");
                while (vict->affected)
                    affect_remove(vict, vict->affected);
            } else {
                send_to_char(ch, "Your victim does not have any affections!\r\n");
                break;
            }
            break;
        default:
            errlog("Unknown subcmd passed to do_wizutil (act.wizard.c)");
            break;
        }
        vict->saveToXML();
    }

    if (loaded)
        delete vict;
}


/* single zone printing fn used by "show zone" so it's not repeated in the
   code 3 times ... -je, 4/6/93 */

void
print_zone_to_buf(struct Creature *ch, char *bufptr, struct zone_data *zone)
{
    sprintf(bufptr,
        "%s%s%s%3d%s %s%-30.30s%s Age:%s%3d%s;Reset:%s%3d%s(%s%1d%s);Top: %s%5d%s;Own:%s%4d%s%s%s\r\n",
        bufptr, CCBLD(ch, C_CMP), CCYEL(ch, C_NRM), zone->number, CCNRM(ch,
            C_CMP), CCCYN(ch, C_NRM), zone->name, CCNRM(ch, C_NRM), CCGRN(ch,
            C_NRM), zone->age, CCNRM(ch, C_NRM), CCGRN(ch, C_NRM),
        zone->lifespan, CCNRM(ch, C_NRM), CCRED(ch, C_NRM), zone->reset_mode,
        CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), zone->top, CCNRM(ch, C_NRM),
        CCYEL(ch, C_NRM), zone->owner_idnum, CCRED(ch, C_NRM), (olc_lock
            || IS_SET(zone->flags, ZONE_LOCKED)) ? "L" : "", CCNRM(ch, C_NRM));
}


#define PRAC_TYPE        3        /* should it say 'spell' or 'skill'? */

void
list_skills_to_char(struct Creature *ch, struct Creature *vict)
{
    extern struct spell_info_type spell_info[];
    char buf3[MAX_STRING_LENGTH];
    int i, sortpos;

    if (prac_params[PRAC_TYPE][(int)GET_CLASS(vict)] != 1 ||
        (CHECK_REMORT_CLASS(vict) >= 0 &&
            prac_params[PRAC_TYPE][(int)GET_REMORT_CLASS(vict)] != 1)) {
        sprintf(buf, "%s%s%s%s knows of the following %ss:%s\r\n", buf,
            CCYEL(ch, C_CMP), PERS(vict, ch), CCBLD(ch, C_SPR),
            SPLSKL(vict), CCNRM(ch, C_SPR));

        strcpy(buf2, buf);

        for (sortpos = 1; sortpos < MAX_SPELLS; sortpos++) {
            i = spell_sort_info[sortpos];
            if (strlen(buf2) >= MAX_STRING_LENGTH - 32) {
                strcat(buf2, "**OVERFLOW**\r\n");
                break;
            }
            sprintf(buf3, "%s[%3d]", CCYEL(ch, C_NRM), GET_SKILL(vict, i));
            if (CHECK_SKILL(vict, i) ||
                (GET_LEVEL(vict) >=
                    spell_info[i].min_level[(int)GET_CLASS(vict)]
                    || (CHECK_REMORT_CLASS(vict) >= 0
                        && GET_LEVEL(vict) >=
                        spell_info[i].
                        min_level[(int)GET_REMORT_CLASS(vict)]))) {
                sprintf(buf, "%s%3d.%-20s %s%-17s%s %s(%3d mana)%s\r\n",
                    CCGRN(ch, C_NRM), i, spell_to_str(i), CCBLD(ch, C_SPR),
                    how_good(GET_SKILL(vict, i)),
                    GET_LEVEL(ch) > LVL_ETERNAL ? buf3 : "", CCRED(ch, C_SPR),
                    mag_manacost(vict, i), CCNRM(ch, C_SPR));
                strcat(buf2, buf);
            }
        }
        sprintf(buf3, "\r\n%s%s knows of the following skills:%s\r\n",
            CCYEL(ch, C_CMP), PERS(vict, ch), CCNRM(ch, C_SPR));
        strcat(buf2, buf3);
    } else {
        sprintf(buf, "%s%s%s knows of the following skills:%s\r\n",
            buf, CCYEL(ch, C_CMP), PERS(vict, ch), CCNRM(ch, C_SPR));
        strcpy(buf2, buf);
    }

    for (sortpos = 1; sortpos < MAX_SKILLS - MAX_SPELLS; sortpos++) {
        i = skill_sort_info[sortpos];
        if (strlen(buf2) >= MAX_STRING_LENGTH - 32) {
            strcat(buf2, "**OVERFLOW**\r\n");
            break;
        }
        sprintf(buf3, "%s[%3d]%s",
            CCYEL(ch, C_NRM), GET_SKILL(vict, i), CCNRM(ch, NRM));
        if (CHECK_SKILL(vict, i) ||
            (GET_LEVEL(vict) >= spell_info[i].min_level[(int)GET_CLASS(vict)]
                || (CHECK_REMORT_CLASS(vict) >= 0
                    && GET_LEVEL(vict) >=
                    spell_info[i].min_level[(int)GET_REMORT_CLASS(vict)]))) {
            sprintf(buf, "%s%3d.%-20s %s%-17s%s%s\r\n", CCGRN(ch, C_NRM), i,
                spell_to_str(i), CCBLD(ch, C_SPR), how_good(GET_SKILL(vict, i)),
                GET_LEVEL(ch) > LVL_ETERNAL ? buf3 : "", CCNRM(ch, C_SPR));
            strcat(buf2, buf);
        }
    }
    page_string(ch->desc, buf2);
}

void
do_show_stats(struct Creature *ch)
{
    int i = 0, j = 0, k = 0, con = 0, tr_count = 0, srch_count = 0;
    short int num_active_zones = 0;
    struct Creature *vict;
    struct obj_data *obj;
    struct room_data *room;
    struct room_trail_data *trail;
    struct special_search_data *srch;
    struct zone_data *zone;
    extern int buf_switches, buf_largecount, buf_overflows;
    CreatureList::iterator cit = characterList.begin();

    for (; cit != characterList.end(); ++cit) {
        vict = *cit;
        if (IS_NPC(vict))
            j++;
        else if (can_see_creature(ch, vict)) {
            i++;
            if (vict->desc)
                con++;
        }
    }
    for (obj = object_list; obj; obj = obj->next)
        k++;

    for (tr_count = 0, zone = zone_table; zone; zone = zone->next) {
        if (zone->idle_time < ZONE_IDLE_TIME)
            num_active_zones++;

        for (room = zone->world; room; room = room->next) {
            for (trail = room->trail; trail; ++tr_count, trail = trail->next);
            for (srch = room->search; srch; ++srch_count, srch = srch->next);
        }
    }

    send_to_char(ch, "Current statistics of Tempus:\r\n");
    send_to_char(ch, "  %5d players in game  %5d connected\r\n", i, con);
    send_to_char(ch, "  %5zd accounts cached  %5zd characters\r\n",
                 Account::cache_size(), playerIndex.size());
    send_to_char(ch, "  %5d mobiles          %5zd prototypes (%d id'd)\r\n",
                 j, mobilePrototypes.size(), current_mob_idnum);
    send_to_char(ch, "  %5d objects          %5zd prototypes\r\n",
                 k, objectPrototypes.size());
    send_to_char(ch, "  %5d rooms            %5d zones (%d active)\r\n",
        top_of_world + 1, top_of_zone_table, num_active_zones);
    send_to_char(ch, "  %5d searches\r\n", srch_count);
    send_to_char(ch, "  %5d large bufs\r\n", buf_largecount);
    send_to_char(ch, "  %5d buf switches     %5d overflows\r\n",
        buf_switches, buf_overflows);
    send_to_char(ch, "  %5zd tmpstr space     %5zu accstr space\r\n",
        tmp_max_used, acc_str_space);
#ifdef MEMTRACK
    send_to_char(ch, "  %5ld trail count      %ldMB total memory\r\n",
		tr_count, dbg_memory_used() / (1024 * 1024));
#else
    send_to_char(ch, "  %5d trail count\r\n", tr_count);
#endif
    send_to_char(ch, "  %5u running progs (%u total, %u free)\r\n",
        prog_count(false), prog_count(true), free_prog_count());
    send_to_char(ch, "  %5zu fighting creatures\r\n",
		combatList.size());
    send_to_char(ch, "  Lunar day: %2d, phase: %s (%d)\r\n",
        lunar_day, lunar_phases[get_lunar_phase(lunar_day)],
		get_lunar_phase(lunar_day));

    if (GET_LEVEL(ch) >= LVL_LOGALL && log_cmds) {
        send_to_char(ch, "  Logging all commands :: file is %s.\r\n",
            1 ? "OPEN" : "NULL");
    }
    if (shutdown_count >= 0) {
        send_to_char(ch,
            "  Shutdown sequence ACTIVE.  Shutdown %s in [%d] seconds.\r\n",
            shutdown_mode == SHUTDOWN_DIE ? "DIE" :
            shutdown_mode == SHUTDOWN_PAUSE ? "PAUSE" : "REBOOT",
            shutdown_count);
    }
}

void
show_wizcommands(Creature *ch)
{
    if (IS_PC(ch))
        Security::sendAvailableCommands(ch, GET_IDNUM(ch));
    else
        send_to_char(ch, "You are a mobile. Deal with it.\r\n");
}


void
show_account(Creature *ch, char *value)
{
    char created_buf[30];
    char last_buf[30];
    int idnum = 0;
    Account *account = NULL;
	time_t last, creation;

    if (!*value) {
        send_to_char(ch, "A name or id would help.\r\n");
        return;
    }

	if (*value == '.') {
		value++;
		if (is_number(value)) {
			idnum = atoi(value);
			if (!playerIndex.exists(idnum)) {
				send_to_char(ch, "There is no such player: %s\r\n", value);
				return;
			}
			idnum = playerIndex.getAccountID(atoi(value));
		} else {
			if (!playerIndex.exists(value)) {
				send_to_char(ch, "There is no such player: %s\r\n", value);
				return;
			}
			idnum = playerIndex.getAccountID(value);
		}

		account = Account::retrieve(idnum);
	} else {
		if (is_number(value)) {
			idnum = atoi(value);
			account = Account::retrieve(idnum);
		} else {
			account = Account::retrieve(value);
		}
	}

    if( account == NULL ) {
        send_to_char(ch, "There is no such account: '%s'\r\n", value);
        return;
    }

    send_to_desc(ch->desc, "&y  Account: &n%s [%d]", account->get_name(), 
		  account->get_idnum());
	if (account->get_email_addr() && *account->get_email_addr())
		send_to_desc(ch->desc, " &c<%s>&n",
			account->get_email_addr());
	House *h = Housing.findHouseByOwner( account->get_idnum() );
    if (h)
        send_to_desc(ch->desc, " &y House: &n%d", h->getID());
	if (account->is_banned())
		send_to_desc(ch->desc, " &y(BANNED)&n");
	else if (account->is_quest_banned())
		send_to_desc(ch->desc, " &y(QBANNED)&n");
    send_to_desc(ch->desc, "\r\n\r\n");
    
    
    
    last = account->get_login_time();
    creation = account->get_creation_time();

    strftime(created_buf, 29, "%a %b %d, %Y %H:%M:%S",
		localtime(&creation));
    strftime(last_buf, 29, "%a %b %d, %Y %H:%M:%S",
		localtime(&last));
    send_to_desc(ch->desc, "&y  Started: &n%s   &yLast login: &n%s\r\n", created_buf, last_buf);
	if( Security::isMember(ch, "AdminFull") ) {
		send_to_desc(ch->desc, "&y  Created: &n%-15s   &yLast: &n%-15s       &yReputation: &n%d\r\n", 
					 account->get_creation_addr(),
					 account->get_login_addr(), 
					 account->get_reputation());
	}
	send_to_desc(ch->desc, "&y  Past bank: &n%-12lld    &yFuture Bank: &n%-12lld",
		account->get_past_bank(), account->get_future_bank());
	if (Security::isMember(ch, "Questor")) {
		send_to_desc(ch->desc, "   &yQuest Points: &n%d\r\n",
			account->get_quest_points());
	} else {
		send_to_desc(ch->desc, "\r\n");
	}
	send_to_desc(ch->desc, "&b ----------------------------------------------------------------------------&n\r\n");

	show_account_chars(ch->desc, account, true, false);
}

void
show_player(Creature *ch, char *value)
{
    Creature *vict;
    char birth[80];
    char last_login[80];
    char remort_desc[80];
    long idnum = 0;

    if (!*value) {
        send_to_char(ch, "A name would help.\r\n");
        return;
    }

    /* added functionality for show player by idnum */
    if (is_number(value) && playerIndex.getName(atoi(value))) {
        strcpy(value, playerIndex.getName(atoi(value)));
    }
    if (!playerIndex.exists(value)) {
        send_to_char(ch, "There is no such player.\r\n");
        return;
    }

    idnum = playerIndex.getID(value);
	vict = new Creature(true);
	vict->loadFromXML(idnum);
    vict->account = Account::retrieve(playerIndex.getAccountID(idnum));


    if (GET_REAL_GEN(vict) <= 0) {
        strcpy(remort_desc, "");
    } else {
        sprintf(remort_desc, "/%s",
            char_class_abbrevs[(int)GET_REMORT_CLASS(vict)]);
    }
    sprintf(buf, "Player: [%ld] %-12s Act[%ld] (%s) [%2d %s %s%s]  Gen: %d", 
            GET_IDNUM(vict), GET_NAME(vict),
            playerIndex.getAccountID(GET_IDNUM(vict)),
            genders[(int)GET_SEX(vict)], GET_LEVEL(vict), 
            player_race[(int)GET_RACE(vict)], char_class_abbrevs[GET_CLASS(vict)], 
            remort_desc, GET_REAL_GEN(vict));
    sprintf(buf, "%s  Rent: Unknown%s\r\n", buf, CCNRM(ch, C_NRM));
    sprintf(buf,
            "%sAu: %-8d  Cr: %-8d  Past: %-8lld  Fut: %-8lld\r\n",
            buf, GET_GOLD(vict), GET_CASH(vict), 
                 GET_PAST_BANK(vict), GET_FUTURE_BANK(vict) );
    // Trim and fit the date to show year but not seconds.
    strcpy(birth, ctime(&vict->player.time.birth));
	memmove(birth + 16, birth + 19, strlen(birth + 19) + 1);
    if (GET_LEVEL(vict) > GET_LEVEL(ch)) {
        strcpy(last_login, "Unknown");
    } else {
        // Trim and fit the date to show year but not seconds.
        strcpy(last_login, ctime(&vict->player.time.logon));
		memmove(birth + 16, birth + 19, strlen(birth + 19) + 1);
    }
    sprintf(buf,
        "%sStarted: %-22.21s Last: %-22.21s Played: %3dh %2dm\r\n",
        buf, birth, last_login, (int)(vict->player.time.played / 3600),
        (int)(vict->player.time.played / 60 % 60));
    
    if (IS_SET(vict->char_specials.saved.act, PLR_FROZEN))
        sprintf(buf, "%s%s%s is FROZEN!%s\r\n", buf, CCCYN(ch, C_NRM),
            GET_NAME(vict), CCNRM(ch, C_NRM));

    if (IS_SET(vict->player_specials->saved.plr2_bits, PLR2_BURIED))
        sprintf(buf, "%s%s%s is BURIED!%s\r\n", buf, CCGRN(ch, C_NRM),
            GET_NAME(vict), CCNRM(ch, C_NRM));

    if (IS_SET(vict->char_specials.saved.act, PLR_DELETED))
        sprintf(buf, "%s%s%s is DELETED!%s\r\n", buf, CCRED(ch, C_NRM),
            GET_NAME(vict), CCNRM(ch, C_NRM));
    send_to_char(ch, "%s", buf);

	delete vict;
}

void
show_multi(Creature *ch, char *arg)
{
	send_to_char(ch, "show multi has been removed.\r\n");
}

void
show_zoneusage(Creature *ch, char *value)
{
    int i;
    struct zone_data *zone;

    if (!*value)
        i = -1;
    else if (is_abbrev(value, "active"))
        i = -2;
    else
        i = atoi(value);
    sprintf(buf, "ZONE USAGE RECORD:\r\n");
    for (zone = zone_table; zone; zone = zone->next) {
        if ((i >= 0 && i != zone->number) || (i == -2 && !zone->num_players))
            continue;
        sprintf(buf,
            "%s[%s%3d%s] %s%-30s%s %s[%s%6d%s]%s acc, [%3d] idle  Owner: %s%s%s\r\n",
            buf, CCYEL(ch, C_NRM), zone->number, CCNRM(ch, C_NRM), CCCYN(ch,
                C_NRM), zone->name, CCNRM(ch, C_NRM), CCGRN(ch, C_NRM),
            CCNRM(ch, C_NRM), zone->enter_count, CCGRN(ch, C_NRM), CCNRM(ch,
                C_NRM), zone->idle_time, CCYEL(ch, C_NRM),
            playerIndex.getName(zone->owner_idnum), CCNRM(ch, C_NRM));
    }

    page_string(ch->desc, buf);
}

bool 
topZoneComparison(const struct zone_data* a, const struct zone_data* b)
{
   return a->enter_count < b->enter_count;
}

void
show_topzones(Creature *ch, char *value)
{
    int i, num_zones = 0;
    struct zone_data *zone; 
    vector<zone_data*> zone_list;
    
    char *temp = NULL;
    char *lower = tmp_tolower(value);
    int lessThan=INT_MAX, greaterThan=-1; 
    bool reverse = false;
    
    temp = strstr(lower, "limit ");
    if (temp && strlen(temp) > 6) {
        num_zones = atoi(temp+6);
    }
    temp = NULL;
    if (!num_zones) {
        if (ch && ch->desc && ch->desc->account)
            num_zones = ch->desc->account->get_term_height()-3;
        else
            num_zones = 50;
    }
    
    temp = strstr(lower, ">");
    if (temp && strlen(temp) > 1) {
        greaterThan = atoi(temp+1);
    }
    temp = NULL;
    temp = strstr(lower, "<");
    if (temp && strlen(temp) > 1) {
        lessThan = atoi(temp+1);
    }
    temp = NULL;
    
    temp = strstr(lower, "reverse");
    if (temp) {
        reverse = true;
    }
    temp = NULL;
    
    
    for (i=0, zone=zone_table; i < top_of_zone_table; zone = zone->next, i++) {
        if (zone->enter_count > greaterThan && zone->enter_count < lessThan
                && zone->number < 700) {
            zone_list.push_back(zone);
        }
    }
    
    sort(zone_list.begin(), zone_list.end(), topZoneComparison);
    
    num_zones = MIN((int)zone_list.size(), num_zones);

    sprintf(buf, "Usage Options: [limit #] [>#] [<#] [reverse]\r\nTOP %d zones:\r\n", num_zones);

    
    if (reverse) {
        for (i = 0; i < num_zones; i++)
            sprintf(buf,
        "%s%2d.[%s%3d%s] %s%-30s%s %s[%s%6d%s]%s accesses.  Owner: %s%s%s\r\n",
        buf, i+1, CCYEL(ch, C_NRM), zone_list[i]->number, CCNRM(ch, C_NRM),
        CCCYN(ch, C_NRM), zone_list[i]->name, CCNRM(ch, C_NRM), CCGRN(ch,
        C_NRM), CCNRM(ch, C_NRM), zone_list[i]->enter_count, CCGRN(ch,
        C_NRM), CCNRM(ch, C_NRM), CCYEL(ch, C_NRM),
        playerIndex.getName(zone_list[i]->owner_idnum), CCNRM(ch, C_NRM));
    } else {
        for (i = zone_list.size()-1; i >= (int)zone_list.size()-num_zones; i--)
            sprintf(buf,
        "%s%2zd.[%s%3d%s] %s%-30s%s %s[%s%6d%s]%s accesses.  Owner: %s%s%s\r\n",
        buf, zone_list.size()-i, CCYEL(ch, C_NRM), zone_list[i]->number, CCNRM(ch, C_NRM),
        CCCYN(ch, C_NRM), zone_list[i]->name, CCNRM(ch, C_NRM), CCGRN(ch,
        C_NRM), CCNRM(ch, C_NRM), zone_list[i]->enter_count, CCGRN(ch,
        C_NRM), CCNRM(ch, C_NRM), CCYEL(ch, C_NRM),
        playerIndex.getName(zone_list[i]->owner_idnum), CCNRM(ch, C_NRM));
    }
        
    page_string(ch->desc, buf);

}

void
show_mobkills(Creature *ch, char *value, char *arg)
{
    Creature *mob = NULL;
    float ratio;
    float thresh;
    int i = 0;

    if (!*value) {
        send_to_char(ch, "Usage: show mobkills <ratio>\r\n"
            "<ratio> should be between 0.00 and 1.00\r\n");
        return;
    }

    thresh = atof(value);

    sprintf(buf, "Mobiles with mobkills ratio >= %f:\r\n", thresh);
    strcat(buf,
        " ---- -Vnum-- -Name------------------------- -Kills- -Loaded- -Ratio-\r\n");
    MobileMap::iterator mit = mobilePrototypes.begin();
    for (; mit != mobilePrototypes.end(); ++mit) {
        mob = mit->second;
        if (!mob->mob_specials.shared->loaded)
            continue;
        ratio = (float)((float)mob->mob_specials.shared->kills /
            (float)mob->mob_specials.shared->loaded);
        if (ratio >= thresh) {
            sprintf(buf, "%s %3d. [%5d]  %-29s %6d %8d    %.2f\r\n",
                buf, ++i, GET_MOB_VNUM(mob), GET_NAME(mob),
                mob->mob_specials.shared->kills,
                mob->mob_specials.shared->loaded, ratio);
            if (strlen(buf) > MAX_STRING_LENGTH - 128) {
                strcat(buf, "**OVERFLOW**\r\n");
                break;
            }
        }
    }
    page_string(ch->desc, buf);
}

const char *show_room_flags[] = {
    "rflags",
    "searches",
    "title",
    "rexdescs",
    "noindent",
    "sounds",
    "occupancy",
    "noexits",
    "mobcount",
    "mobload",
    "deathcount",
    "sector",
    "desc_length",
    "orphan_words",
    "period_space",
    "exits",
    "\n"
};

const char *show_room_modes[] = {
	"zone",
	"time",
	"plane",
	"world",
	"\n"
};

void
show_room_append(Creature *ch, room_data *room, int mode, const char *extra)
{
	if (!extra)
		extra = "";

	if (mode != 0)
		acc_sprintf("%s%s%3d %s%s%-30s %s%5d %s%-30s %s%s\r\n",
			CCBLD(ch, C_CMP), CCYEL(ch, C_NRM), room->zone->number,
			CCNRM(ch, C_NRM), CCCYN(ch, C_NRM), room->zone->name,
			CCYEL(ch, C_NRM), room->number,
			CCCYN(ch, C_NRM), room->name, 
			CCNRM(ch, C_NRM), extra);
	else
		acc_sprintf("%s#%d %s%-40s %s%s\r\n",
			CCYEL(ch, C_NRM), room->number,
			CCCYN(ch, C_NRM), room->name, 
			CCNRM(ch, C_NRM), extra);
}

// returns 0 if none found, 1 if found, -1 if error
int
show_rooms_in_zone(Creature *ch, zone_data *zone, int pos, int mode, char *args)
{
	special_search_data *srch = NULL; 
	list<string> str_list;
	list<string>::iterator str_it;
	list<string> mob_names;
	CreatureList::iterator cit;
	room_data *room;
	bool match, gt = false, lt = false;
	int found = 0, num, flags = 0, tmp_flag = 0;
	char *arg;

	arg = tmp_getword(&args);

	switch (pos) {
    case 0:  //rflags
        if (!*arg) {
            send_to_char(ch, "Usage: show rooms <world | zone> rflags [flag flag ...]\n");
            return -1;
        }

        while (*arg) {
            tmp_flag = search_block(arg, roomflag_names, 0);
            if (tmp_flag == -1)
                send_to_char(ch, "Invalid flag %s, skipping...\n", arg);
            else
                flags |= (1 << tmp_flag);
            arg = tmp_getword(&args);
        }

        for (room = zone->world; room; room = room->next)
            if ((room->room_flags & flags) == flags) {
                show_room_append(ch, room, mode, NULL);
                found = 1;
            }
        break;

    case 1:  //searches
        while (*arg) {
            tmp_flag = search_block(arg, search_bits, 0);
            if (tmp_flag == -1)
                send_to_char(ch, "Invalid flag %s, skipping...\n", arg);
            else
                flags |= (1 << tmp_flag);
            arg = tmp_getword(&args);
        }

			
        for (room = zone->world; room; room = room->next)
            for (srch = room->search; srch != NULL; srch = srch->next)
                if ((srch->flags & flags) == flags) {
                    show_room_append(ch, room, mode,
                                     tmp_sprintf("[%-6s]",
                                                 search_cmd_short[(int)srch->command]));
                    found = 1;
                }
        break;

    case 2: //title
        if (!*arg) {
            send_to_char(ch, "Usage: show rooms <world | zone> title [keyword keyword ...]\n");
            return -1;
        }

        while (*arg) {
            str_list.push_back(arg);
            arg = tmp_getword(&args);
        }

        for (room = zone->world; room; room = room->next) {
            match = true;
            for (str_it = str_list.begin(); str_it != str_list.end(); str_it++)
                if (room->name && !isname(str_it->c_str(), room->name))
                    match = false;
				
            if (match) {
                show_room_append(ch, room, mode, NULL);
                found = 1;
            }
        }
        break;
    case 3: //rexdescs 
        for (room = zone->world; room; room = room->next)
            if (room->ex_description != NULL) {
                show_room_append(ch, room, mode, NULL);
                found = 1;
            }
        break;
    case 4: //noindent
        for (room = zone->world; room; room = room->next) {
            if (!room->description)
                continue;

            if (strncmp(room->description, "   ", 3) != 0) {
                show_room_append(ch, room, mode, NULL);
                found = 1;
            }
        }
        break;
    case 5: //sounds
        for (room = zone->world; room; room = room->next)
            if (room->sounds != NULL) {
                show_room_append(ch, room, mode, NULL);
                found = 1;
            }
        break;
    case 6: //occupancy
        if (!*arg) {
            send_to_char(ch, "Usage: show rooms <world | zone> occupancy < < | >  # > \n");
            return -1;
        }
			
        if (*arg == '<')
            lt = true;
        else if (*arg == '>')
            gt = true;
        else {
            send_to_char(ch, "Usage: show rooms <world | zone> occupancy < < | >  # > \n");
            return -1;
        }
			
        arg = tmp_getword(&args);
        if (!*arg || !isnumber(arg)) {
            send_to_char(ch, "Usage: show rooms <world | zone> occupancy < < | >  # > \n");
            return -1;
        }
        num = atoi(arg);
			
			
        for (room = zone->world; room; room = room->next)
            if ((gt && (room->max_occupancy > num))
                || (lt && (room->max_occupancy < num))) {
                show_room_append(ch, room, mode,
                                 tmp_sprintf("[%2d]", room->max_occupancy));
                found = 1;
            }
        break;
    case 7: //noexits
        for (room = zone->world; room; room = room->next) {
            if (!room->countExits()) {
                show_room_append(ch, room, mode, NULL);
                found = 1;
            }
        }
        break;
    case 8: //mobcount
        if (mode == 3) {
            // No listing all mobs in world
            send_to_char(ch, "No way!  Are you losing it?\n");
            return -1;
        }

        if (!*arg || !isnumber(arg)) {
            send_to_char(ch, "Usage: show rooms <world | zone> mobcount [mobcount]\n");
            return -1;
        }

        num = atoi(arg);
        for (room = zone->world; room; room = room->next) {
            mob_names.clear();

            for (cit = room->people.begin(); cit != room->people.end(); cit++) {
                if (!IS_NPC(*cit))
                    continue;
                mob_names.push_front((*cit)->player.short_descr);
            }

            if (mob_names.size() >= (unsigned int)num) {
                show_room_append(ch, room, mode,
                                 tmp_sprintf("[%2zd]", mob_names.size()));
                found = 1;
                for (str_it = mob_names.begin(); str_it != mob_names.end(); str_it++)
                    acc_sprintf("\t%s%s%s\r\n", CCYEL(ch, C_NRM),
								str_it->c_str(), CCNRM(ch, C_NRM));
            }
        }
        break;
    case 9: //mobload
        send_to_char(ch, "disabled.\n");
        break ;
    case 10: //deathcount
        send_to_char(ch, "disabled.\n");
        break;
    case 11: //sector
        if (!*arg) {
            send_to_char(ch, "Usage: show rooms <world | zone> sector <sector_type>\n");
            return -1;
        }

        num = search_block(arg, sector_types, 0);
        if (num < 0) {
            send_to_char(ch, "No such sector type.  Type olc h rsect.\r\n");
            return -1;
        }

        for (room = zone->world; room; room = room->next)
            if (room->sector_type == num) {
                show_room_append(ch, room, mode, NULL);
                found = 1;
            }
        break;
    case 12: //desc_length
        if (!*arg) {
            send_to_char(ch, "Usage: show rooms <world | zone> desc_length < < | >  # > \n");
            return -1;
        }
			
        if (*arg == '<')
            lt = true;
        else if (*arg == '>')
            gt = true;
        else {
            send_to_char(ch, "Usage: show rooms <world | zone> desc_length < < | >  # > \n");
            return -1;
        }
			
        arg = tmp_getword(&args);
        if (!*arg || !isnumber(arg)) {
            send_to_char(ch, "Usage: show rooms <world | zone> desc_length < < | >  # > \n");
            return -1;
        }
        num = atoi(arg);
			
        for (room = zone->world; room; room = room->next)
            if (room->description
                && ((gt && ((unsigned)num <= strlen(room->description)))
                    || (lt && ((unsigned)num >= strlen(room->description))))) {
                show_room_append(ch, room, mode, 
                                 tmp_sprintf("[%zd]", strlen(room->description)));
                found = 1;
            }
        break;
    case 13: //orphan_words
        for (room = zone->world; room;room = room->next) {
            if (!room->description)
                continue;

            char *desc = room->description;
            // find the last line of the desc
            arg = desc + strlen(desc) - 3;
            // skip trailing space
            while (arg > desc && *arg == ' ')
                arg--;
            // search for spaces while decrementing to the beginning of the line
            while (arg > desc && *arg != '\n' && *arg != ' ')
                arg--;
            // we don't worry about one-line descs
            if (arg == desc)
                continue;
            // check for a space - if one doesn't exist, we have an orphaned word.
            if (*arg != ' ') {
                show_room_append(ch, room, mode, NULL);
                found = 1;
            }
        }
        break;
    case 14: //period_space
        for (room = zone->world; room; room = room->next) {
            if (!room->description)
                continue;
            arg = room->description;
            match = false;
            while (*arg && !match) {
                // Find sentence terminating punctuation
                arg = strpbrk(arg, ".?!");
                if (!arg)
                    break;
                // Skip past multiple punctuation
                arg += strspn(arg, ".?!");
                // Count spaces
                int count = strspn(arg, " ");
                // Ensure that either there were two spaces after the punctuation, or
                // any spaces were trailing space
                arg += count;
                if (*arg && !isspace(*arg) && count != 2)
                    match = true;
            }
            if (match) {
                show_room_append(ch, room, mode, NULL);
                found = 1;
            }
        }
        break;
    case 15: // exits
        for (room = zone->world; room; room = room->next) {
            for (int dir = 0;dir < NUM_DIRS;dir++) {
                room_direction_data *exit = ABS_EXIT(room, dir);
                if (exit) {
                    if (IS_SET(exit->exit_info, EX_CLOSED | EX_LOCKED | EX_HARD_PICK | EX_PICKPROOF | EX_HEAVY_DOOR | EX_TECH | EX_REINFORCED | EX_SPECIAL) &&
                        !IS_SET(exit->exit_info, EX_ISDOOR)) {
                        show_room_append(ch, room, mode,
                                         tmp_sprintf("doorflags on non-door for %s exit", dirs[dir]));
                        found = 1;
                    }
                    if (IS_SET(exit->exit_info, EX_CLOSED | EX_LOCKED) && !exit->keyword) {
                        show_room_append(ch, room, mode,
                                         tmp_sprintf("no keyword for %s exit", dirs[dir]));
                        found = 1;
                    }
                    if (exit->key && exit->key != -1 && !objectPrototypes.find(exit->key)) {
                        show_room_append(ch, room, mode,
                                         tmp_sprintf("non-existant key for %s exit", dirs[dir]));
                        found = 1;
                    }
                    if (!exit->general_description) {
                        show_room_append(ch, room, mode,
                                         tmp_sprintf("no description for %s exit", dirs[dir]));
                        found = 1;
                    }
                }
            }
        }
        break;
    default:
        errlog("Can't happen at %s:%d", __FILE__, __LINE__);
        send_to_char(ch, "Uh, oh... Bad juju...\r\n");
		break;
	}
	return found;
}

void
show_rooms(Creature *ch, char *value, char *args)
{
    const char *usage = "Usage: show rooms <zone | time | plane | world> <flag> [options]";
    int show_mode;
	int idx, pos;
	int found = 0;
	char *arg;

	acc_string_clear();

	show_mode = search_block(value, show_room_modes, 0);
    if (show_mode < 0) {
        send_to_char(ch, "%s\n", usage);
        return;
    }
    
	arg = tmp_getword(&args);
    if (arg && ((pos = search_block(arg, show_room_flags, 0)) >= 0)) {
		switch (show_mode) {
		case 0:
			found = show_rooms_in_zone(ch, ch->in_room->zone, pos, show_mode, args);
			if (found < 0)
				return;
			break;
		case 1:
            for (zone_data *zone = zone_table; zone; zone = zone->next)
				if (ch->in_room->zone->time_frame == zone->time_frame) {
					found |= show_rooms_in_zone(ch, zone, pos, show_mode, args);
					if (found < 0)
						return;
				}
			break;
		case 2:
            for (zone_data *zone = zone_table; zone; zone = zone->next)
				if (ch->in_room->zone->plane == zone->plane) {
					found |= show_rooms_in_zone(ch, zone, pos, show_mode, args);
					if (found < 0)
						return;
				}
			break;
		case 3:
            for (zone_data *zone = zone_table; zone; zone = zone->next) {
				found |= show_rooms_in_zone(ch, zone, pos, show_mode, args);
				if (found < 0)
					return;
			}
			break;
		default:
			slog("Can't happen at %s:%d", __FILE__, __LINE__);
		}

		if (found)
			page_string(ch->desc, acc_get_string());
		else
			send_to_char(ch, "No matching rooms.\r\n");
    } else {
        string usage_string;
        
        usage_string += "Valid flags are:\n";

		for (idx = 0;*show_room_flags[idx] != '\n';idx++) {
            usage_string += show_room_flags[idx];
            usage_string += '\r';
            usage_string += '\n';
        }
        send_to_char(ch, "%s", usage_string.c_str());
    }
}

#define MLEVELS_USAGE "Usage: show mlevels <real | proto> [remort] [expand]\r\n"

void
show_mlevels(Creature *ch, char *value, char *arg)
{

    int remort = 0;
    int expand = 0;
    int count[50];
    int i, j;
    int max = 0;
    int to = 0;
    Creature *mob = NULL;
    char arg1[MAX_INPUT_LENGTH];
    if (!*value) {
        send_to_char(ch, MLEVELS_USAGE);
        return;
    }

    skip_spaces(&arg);
    if (*arg) {
        while (1) {
            arg = one_argument(arg, arg1);
            if (!*arg1)
                break;
            if (!strcmp(arg1, "remort"))
                remort = 1;
            else if (!strcmp(arg1, "expand"))
                expand = 1;
            else {
                send_to_char(ch, "Unknown option: '%s'\r\n%s", arg1,
                    MLEVELS_USAGE);
            }
        }
    }

    for (i = 0; i < 50; i++)
        count[i] = 0;

    sprintf(buf, "Mlevels for %s", remort ? "remort " : "mortal ");
    // scan the existing mobs
    if (!strcmp(value, "real")) {
        strcat(buf, "real mobiles:\r\n");
        MobileMap::iterator mit = mobilePrototypes.begin();
        for (; mit != mobilePrototypes.end(); ++mit) {
            mob = mit->second;
            if (IS_NPC(mob) && GET_LEVEL(mob) < 50 &&
                ((remort && IS_REMORT(mob)) || (!remort && !IS_REMORT(mob)))) {
                if (expand)
                    count[(int)GET_LEVEL(mob)]++;
                else
                    count[(int)(GET_LEVEL(mob) / 5)]++;
            }
        }
    } else if (!strcmp(value, "proto")) {
        strcat(buf, "mobile protos:\r\n");
        MobileMap::iterator mit = mobilePrototypes.begin();
        for (; mit != mobilePrototypes.end(); ++mit) {
            mob = mit->second;
            if (GET_LEVEL(mob) < 50 &&
                ((remort && IS_REMORT(mob)) || (!remort && !IS_REMORT(mob)))) {
                if (expand)
                    count[(int)GET_LEVEL(mob)]++;
                else
                    count[(int)(GET_LEVEL(mob) / 5)]++;
            }
        }
    } else {
        send_to_char(ch, "First argument must be either 'real' or 'proto'\r\n");
        send_to_char(ch, MLEVELS_USAGE);
        return;
    }

    if (expand)
        to = 50;
    else
        to = 10;

    // get max to scale the display
    for (i = 0; i < to; i++) {
        if (count[i] > max)
            max = count[i];
    }

    if (!max) {
        send_to_char(ch, "There are no mobiles?\r\n");
        return;
    }
    // print the long, expanded list
    if (expand) {

        for (i = 0; i < 50; i++) {
            to = (count[i] * 60) / max;

            *buf2 = '\0';
            for (j = 0; j < to; j++)
                strcat(buf2, "+");

            sprintf(buf, "%s%2d] %-60s %5d\r\n", buf, i, buf2, count[i]);
        }
    }
    // print the smaller list
    else {

        for (i = 0; i < 10; i++) {

            to = (count[i] * 60) / max;

            *buf2 = '\0';
            for (j = 0; j < to; j++)
                strcat(buf2, "+");

            sprintf(buf, "%s%2d-%2d] %-60s %5d\r\n", buf, i * 5,
                (i + 1) * 5 - 1, buf2, count[i]);
        }
    }

    page_string(ch->desc, buf);
}

#define SFC_OBJ 1
#define SFC_MOB 2
#define SFC_ROOM 3

struct show_struct fields[] = {
    {"nothing", 0, ""},            // 0 
    {"zones", LVL_AMBASSADOR, ""},    // 1 
    {"player", LVL_IMMORT, "AdminBasic"},
    {"rent", LVL_IMMORT, "AdminFull"},
    {"stats", LVL_AMBASSADOR, ""},
    {"errors", LVL_GRIMP, "Coder"},    // 5
    {"death", LVL_IMMORT, "WizardBasic"},
    {"godrooms", LVL_CREATOR, "WizardFull"},
    {"shops", LVL_IMMORT, ""},
    {"bugs", LVL_IMMORT, "WizardBasic"},
    {"typos", LVL_IMMORT, ""},    // 10 
    {"ideas", LVL_IMMORT, ""},
    {"skills", LVL_IMMORT, "AdminBasic"},
    {"spells", LVL_GRIMP, ""},
    {"residents", LVL_AMBASSADOR, ""},    // 14
    {"aliases", LVL_IMMORT, "AdminBasic"},
    {"memory use", LVL_IMMORT, "Coder"},
    {"social", LVL_DEMI, ""},    // 17 
    {"quest", LVL_IMMORT, ""},
    {"questguide", LVL_IMMORT, ""},    // 19 
    {"zoneusage", LVL_IMMORT, ""},    // 20
    {"population", LVL_AMBASSADOR, ""},
    {"topzones", LVL_AMBASSADOR, ""},
    {"nomaterial", LVL_IMMORT, ""},
    {"objects", LVL_IMMORT, ""},
    {"broken", LVL_IMMORT, ""},    // 25
    {"quiz", LVL_GRIMP, ""},
    {"clans", LVL_AMBASSADOR, ""},
    {"specials", LVL_IMMORT, ""},
    {"implants", LVL_IMMORT, ""},
    {"paths", LVL_IMMORT, ""},    // 30
    {"pathobjs", LVL_IMMORT, ""},
    {"searches", LVL_IMMORT, "OLCApproval"},
    {"str_app", LVL_IMMORT, ""},
    {"timezones", LVL_IMMORT, ""},
    {"class", LVL_IMMORT, ""},    // 35 
    {"unapproved", LVL_IMMORT, ""},
    {"elevators", LVL_IMMORT, ""},
    {"free_create", LVL_IMMORT, ""},
    {"exp_percent", LVL_IMMORT, "OLCApproval"},
    {"demand_items", LVL_IMMORT, "OLCApproval"},    // 40 
    {"descriptor", LVL_CREATOR, ""},
    {"fighting", LVL_IMMORT, "WizardBasic"},
    {"quad", LVL_IMMORT, ""},
    {"mobiles", LVL_IMMORT, ""},
    {"hunting", LVL_IMMORT, "WizardBasic"},    // 45 
    {"last_cmd", LVL_LUCIFER, ""},
    {"duperooms", LVL_IMMORT, "OLCWorldWrite"},
    {"specialization", LVL_IMMORT, ""},
    {"p_index", LVL_IMMORT, ""},
    {"zexits", LVL_IMMORT, ""},    // 50
    {"mlevels", LVL_IMMORT, ""},
    {"plevels", LVL_IMMORT, ""},
    {"mobkills", LVL_IMMORT, "WizardBasic"},
    {"wizcommands", LVL_IMMORT, ""},
    {"timewarps", LVL_IMMORT, ""},    // 55
    {"zonecommands", LVL_IMMORT, ""},
    {"multi", LVL_IMMORT, "AdminBasic"},
    {"nohelps", LVL_IMMORT, "Help"},
    {"account", LVL_IMMORT, "AdminBasic"},
    {"rooms", LVL_IMMORT, "OLC"}, // 60
	{"boards", LVL_IMMORT, "OLC"},
    {"\n", 0, ""}
};

ACMD(do_show)
{
    int i = 0, j = 0, k = 0, l = 0, con = 0, percent, num_rms = 0, tot_rms = 0;
    char self = 0;
    int found = 0;
    int sfc_mode = 0;
    struct Creature *vict;
    struct obj_data *obj;
    struct alias_data *a;
    struct zone_data *zone = NULL;
    struct room_data *room = NULL, *tmp_room = NULL;
    struct descriptor_data *tmp_d = NULL;
    char field[MAX_INPUT_LENGTH], value[MAX_INPUT_LENGTH];
    extern char *quest_guide;
    struct Creature *mob = NULL;
    CreatureList::iterator cit;
    MobileMap::iterator mit;
    ObjectMap::iterator oi;

    void show_shops(struct Creature *ch, char *value);

    skip_spaces(&argument);
    if (!*argument) {
        vector <string> cmdlist;
        send_to_char(ch, "Show options:\r\n");
        for (j = 0, i = 1; fields[i].level; i++) {
            if (Security::canAccess(ch, fields[i])) {
                cmdlist.push_back(fields[i].cmd);
            }
        }
        sort(cmdlist.begin(), cmdlist.end());
        for (j = 0, i = 1; i < (int)(cmdlist.size()); i++) {
            send_to_char(ch, "%-15s%s", cmdlist[i].c_str(),
                (!(++j % 5) ? "\r\n" : ""));
        }

        send_to_char(ch, "\r\n");
        return;
    }

    strcpy(arg, two_arguments(argument, field, value));

    for (l = 0; *(fields[l].cmd) != '\n'; l++)
        if (!strncmp(field, fields[l].cmd, strlen(field)))
            break;

    if (!Security::canAccess(ch, fields[l])) {
        send_to_char(ch, "You do not have that power.\r\n");
        return;
    }

    if (!strcmp(value, "."))
        self = 1;
    buf[0] = '\0';

    switch (l) {
    case 1:                    /* zone */
        {
            static const char *usage =
                "Usage: show zone [ . | all | <begin#> <end#> | name <partial name>\r\n"
		"                     | fullcontrol | owner | co-owner | past | future\r\n"
		"                     | timeless | lawless | norecalc ]\r\n";
            Tokenizer tokens(arg);
            if (value[0] == '\0') {
                send_to_char(ch, usage);
                return;
            } else if (self) {
                print_zone_to_buf(ch, buf, ch->in_room->zone);
            } else if (is_number(value)) {    // show a range ( from a to b )
                int a = atoi(value);
                int b = a;
                list <zone_data *>zone_list;
                if (tokens.next(value))
                    b = atoi(value);
                for (zone = zone_table; zone; zone = zone->next) {
                    if (zone->number >= a && zone->number <= b) {
                        zone_list.push_front(zone);
                    }
                }
                if (zone_list.size() <= 0) {
                    send_to_char(ch, "That is not a valid zone.\r\n");
                    return;
                }
                zone_list.reverse();
                list <zone_data *>::iterator it = zone_list.begin();
                for (; it != zone_list.end(); it++) {
                    print_zone_to_buf(ch, buf, *it);
                }
            } else if (strcasecmp("owner", value) == 0 && tokens.next(value)) {    // Show by name
                for (zone = zone_table; zone; zone = zone->next) {
                    const char *ownerName = playerIndex.getName(zone->owner_idnum);
                    if (ownerName && strcasecmp(value, ownerName) == 0) {
                        print_zone_to_buf(ch, buf, zone);
                    }
                }
            } else if (strcasecmp("co-owner", value) == 0 && tokens.next(value)) {    // Show by name
                for (zone = zone_table; zone; zone = zone->next) {
                    const char *ownerName = playerIndex.getName(zone->co_owner_idnum);
                    if (ownerName && strcasecmp(value, ownerName) == 0) {
                        print_zone_to_buf(ch, buf, zone);
                    }
                }
            } else if (strcasecmp("name", value) == 0 && tokens.next(value)) {    // Show by name
                for (zone = zone_table; zone; zone = zone->next)
                    if (stristr(zone->name, value))
                        print_zone_to_buf(ch, buf, zone);
            } else if (strcasecmp(value, "all") == 0) {
                for (zone = zone_table; zone; zone = zone->next)
                    print_zone_to_buf(ch, buf, zone);
            } else if (strcasecmp(value, "fullcontrol") == 0) {
                for (zone = zone_table; zone; zone = zone->next)
                    if (ZONE_FLAGGED(zone, ZONE_FULLCONTROL))
                        print_zone_to_buf(ch, buf, zone);
            } else if (strcasecmp(value, "norecalc") == 0) {
                for (zone = zone_table; zone; zone = zone->next)
                    if (ZONE_FLAGGED(zone, ZONE_NORECALC))
                        print_zone_to_buf(ch, buf, zone);
            } else if (strcasecmp(value, "lawless") == 0) {
                for (zone = zone_table; zone; zone = zone->next)
                    if (ZONE_FLAGGED(zone, ZONE_NOLAW))
                        print_zone_to_buf(ch, buf, zone);
            } else if (strcasecmp(value, "past") == 0) {
				for (zone = zone_table; zone; zone = zone->next)
					if (zone->time_frame == TIME_PAST)
                        print_zone_to_buf(ch, buf, zone);
            } else if (strcasecmp(value, "future") == 0) {
				for (zone = zone_table; zone; zone = zone->next)
					if (zone->time_frame == TIME_FUTURE)
                        print_zone_to_buf(ch, buf, zone);
            } else if (strcasecmp(value, "timeless") == 0) {
				for (zone = zone_table; zone; zone = zone->next)
					if (zone->time_frame == TIME_TIMELESS)
                        print_zone_to_buf(ch, buf, zone);
            } else {
                send_to_char(ch, usage);
                return;
            }
            page_string(ch->desc, buf);
            break;
        }

    case 2:                    /* player */
        show_player(ch, value);
        break;
    case 3:
		send_to_char(ch, "Disabled.\r\n");
		return;
    case 4:
        do_show_stats(ch);
        break;
    case 5:
        break;
        strcpy(buf, "Errant Rooms\r\n------------\r\n");
        for (zone = zone_table; zone; zone = zone->next)
            for (room = zone->world; room; room = room->next)
                for (j = 0; j < NUM_OF_DIRS; j++)
                    if (room->dir_option[j]
                        && room->dir_option[j]->to_room == NULL)
                        send_to_char(ch, "%s%2d: [%5d] %s\r\n", buf, ++k,
                            room->number, room->name);
        break;
    case 6:
        strcpy(buf, "Death Traps\r\n-----------\r\n");
        for (zone = zone_table; zone; zone = zone->next)
            for (j = 0, room = zone->world; room; room = room->next)
                if (IS_SET(ROOM_FLAGS(room), ROOM_DEATH)) {
                    sprintf(buf, "%s%2d: %s[%5d]%s %s%s", buf, ++j,
                        CCRED(ch, C_NRM), room->number, CCCYN(ch, C_NRM),
                        room->name, CCNRM(ch, C_NRM));
                    if (room->contents)
                        strcat(buf, "  (has objects)\r\n");
                    else
                        strcat(buf, "\r\n");
                }
        page_string(ch->desc, buf);
        break;
    case 7:
#define GOD_ROOMS_ZONE 12
        strcpy(buf, "Godrooms\r\n--------------------------\r\n");
        for (j = 0, zone = zone_table; zone; zone = zone->next)
            for (room = zone->world; room; room = room->next)
                if (room->zone->number == GOD_ROOMS_ZONE ||
                    ROOM_FLAGGED(room, ROOM_GODROOM))
                    send_to_char(ch, "%s%2d: [%5d] %s\r\n", buf, j++, room->number,
                        room->name);
        break;
    case 8:
        send_to_char(ch, "Disabled.\r\n"); break;
    case 9:
        if (*value && isdigit(*value)) {
            j = atoi(value);
            if (j > 0)
                show_file(ch, BUG_FILE, j);
        } else {
            show_file(ch, BUG_FILE, 0);
        }
        break;
    case 10:
        if (*value && isdigit(*value)) {
            j = atoi(value);
            if (j > 0)
                show_file(ch, TYPO_FILE, j);
        } else {
            show_file(ch, TYPO_FILE, 0);
        }
        break;
    case 11:
        if (*value && isdigit(*value)) {
            j = atoi(value);
            if (j > 0)
                show_file(ch, IDEA_FILE, j);
        } else {
            show_file(ch, IDEA_FILE, 0);
        }
        break;
    case 12:
        if (!*value) {
            send_to_char(ch, "View who's skills?\r\n");
        } else if (!(vict = get_player_vis(ch, value, 0))) {
            if (!Security::isMember(ch, "AdminBasic")) {
                send_to_char(ch, 
                    "Getting that data from file requires basic administrative rights.\r\n");
            } else {
                vict = new Creature(true);
				if (!playerIndex.exists(value))
                    send_to_char(ch, "There is no such player.\r\n");
                else if (!vict->loadFromXML(playerIndex.getID(value)))
					send_to_char(ch, "Error loading player.\r\n");
				else if (GET_LEVEL(vict) > GET_LEVEL(ch) && GET_IDNUM(ch) != 1)
					send_to_char(ch, "Sorry, you can't do that.\r\n");
				else
					list_skills_to_char(ch, vict);
				delete vict;
                vict = NULL;
            }
        } else if (IS_MOB(vict)) {
            send_to_char(ch, "Mobs don't have skills.  All mobs are assumed to\r\n"
                "have a (50 + level)%% skill proficiency.\r\n");
        } else {
            list_skills_to_char(ch, vict);
        }
        break;
    case 13:                    /* spells */
        send_to_char(ch, "Try using 'spells list <class>' instead.\r\n");
        break;

    case 14:
        if (!*value)
            list_residents_to_char(ch, -1);
        else {
            if (is_abbrev(value, "modrian"))
                list_residents_to_char(ch, HOME_MODRIAN);
            else if (is_abbrev(value, "new thalos"))
                list_residents_to_char(ch, HOME_NEW_THALOS);
            else if (is_abbrev(value, "electro centralis"))
                list_residents_to_char(ch, HOME_ELECTRO);
            else if (is_abbrev(value, "elven village"))
                list_residents_to_char(ch, HOME_ELVEN_VILLAGE);
            else if (is_abbrev(value, "istan"))
                list_residents_to_char(ch, HOME_ISTAN);
            else if (is_abbrev(value, "arena"))
                list_residents_to_char(ch, HOME_ARENA);
            else if (is_abbrev(value, "kromguard"))
                list_residents_to_char(ch, HOME_KROMGUARD);
            else
                send_to_char(ch, "Unrecognized City.\r\n");
        }
        break;
    case 15:                    // aliases
        if (!(vict = get_char_vis(ch, value)))
            send_to_char(ch, "You cannot see any such person.\r\n");
        else if (IS_MOB(vict))
            send_to_char(ch, "Now... what would a mob need with aliases??\r\n");
        else if (GET_LEVEL(vict) > GET_LEVEL(ch)) {
            act("You cannot determine what $S aliases might be.", FALSE, ch, 0,
                vict, TO_CHAR);
            act("$n has attempted to examine your aliases.", FALSE, ch, 0,
                vict, TO_VICT);
        } else {                /* no argument specified -- list currently defined aliases */
            sprintf(buf, "Currently defined aliases for %s:\r\n",
                GET_NAME(vict));
            if ((a = GET_ALIASES(vict)) == NULL)
                strcat(buf, " None.\r\n");
            else {
                while (a != NULL) {
                    sprintf(buf, "%s%s%-15s%s %s\r\n", buf, CCCYN(ch, C_NRM),
                        a->alias, CCNRM(ch, C_NRM), a->replacement);
                    a = a->next;
                }
            }
            page_string(ch->desc, buf);
        }
        break;
    case 16:
        do_stat_memory(ch);
        break;
    case 17:                    /* social */
        show_social_messages(ch, value);
        break;
    case 18:
        send_to_char(ch, "Command not implemented yet.\r\n");
        break;
    case 19:                    // questguide
        page_string(ch->desc, quest_guide);
        break;
    case 20:                    // zoneusage 
        show_zoneusage(ch, value);
        break;
    case 21:
        sprintf(buf, "POPULATION RECORD:\r\n");
        for (i = 0; i < NUM_HOMETOWNS; i++) {
            if (isalpha(home_towns[i][0]))
                sprintf(buf, "%s%-30s -- [%4d]\r\n", buf, home_towns[i],
                    population_record[i]);
        }
        page_string(ch->desc, buf);
        break;
    case 22:                    // topzones
        show_topzones(ch, tmp_strcat(value, arg));
        break;
    case 23:                    /* nomaterial */
        strcpy(buf, "Objects without material types:\r\n");
        oi = objectPrototypes.begin();
        for (i = 1; oi != objectPrototypes.end(); ++oi) {
            obj = oi->second;
            if (GET_OBJ_MATERIAL(obj) == MAT_NONE &&
                !IS_OBJ_TYPE(obj, ITEM_SCRIPT)) {
                if (strlen(buf) > (MAX_STRING_LENGTH - 130)) {
                    strcat(buf, "**OVERFLOW**\r\n");
                    break;
                }
                sprintf(buf, "%s%3d. [%5d] %s%-36s%s  (%s)\r\n", buf, i,
                    GET_OBJ_VNUM(obj), CCGRN(ch, C_NRM),
                    obj->name, CCNRM(ch, C_NRM), obj->aliases);
                i++;
            }
        }
        page_string(ch->desc, buf);
        break;
    
    case 24:  /** objects **/
        do_show_objects(ch, value, arg);
        break;

    case 25:  /** broken **/

        strcpy(buf, "Broken objects in the game:\r\n");

        for (obj = object_list, i = 1; obj; obj = obj->next) {
            if ((GET_OBJ_DAM(obj) < (GET_OBJ_MAX_DAM(obj) >> 1)) ||
                IS_OBJ_STAT2(obj, ITEM2_BROKEN)) {

                if (GET_OBJ_DAM(obj) == -1 || GET_OBJ_DAM(obj) == -1)
                    j = 100;
                else if (GET_OBJ_DAM(obj) == 0 || GET_OBJ_DAM(obj) == 0)
                    j = 0;
                else
                    j = (GET_OBJ_DAM(obj) * 100 / GET_OBJ_MAX_DAM(obj));

                sprintf(buf2, "%3d. [%5d] %s%-34s%s [%3d percent] %s\r\n",
                    i, GET_OBJ_VNUM(obj), CCGRN(ch, C_NRM),
                    obj->name,
                    CCNRM(ch, C_NRM),
                    j, IS_OBJ_STAT2(obj, ITEM2_BROKEN) ? "<broken>" : "");
                if ((strlen(buf) + strlen(buf2) + 128) > MAX_STRING_LENGTH) {
                    strcat(buf, "**OVERFLOW**\r\n");
                    break;
                }
                strcat(buf, buf2);
                i++;
            }
        }

        page_string(ch->desc, buf);
        break;

    case 26:  /** quiz **/
        send_to_char(ch, "Disabled.\r\n");
        return;
    case 27:     /** clans **/
        if (GET_LEVEL(ch) < LVL_ELEMENT)
            do_show_clan(ch, NULL);
        else
            do_show_clan(ch, clan_by_name(value));
        break;
    case 28:    /** specials **/
        do_show_specials(ch, value);
        break;
    case 29:  /** implants **/
        if (!*value) {
            strcpy(buf2, "OBJECTS IMPLANTED IN THE GAME:\r\n");
            for (obj = object_list, i = 0; obj; obj = obj->next)
                if (IS_IMPLANT(obj) && obj->worn_by &&
                    GET_IMPLANT(obj->worn_by, obj->worn_on) &&
                    GET_IMPLANT(obj->worn_by, obj->worn_on) == obj) {
                    sprintf(buf, "%3d. %30s - implanted in %s%s%s (%s)\r\n",
                        i++, obj->name,
                        !IS_NPC(obj->worn_by) ? CCYEL(ch, C_NRM) : "",
                        GET_NAME(obj->worn_by),
                        !IS_NPC(obj->worn_by) ? CCNRM(ch, C_NRM) : "",
                        wear_implantpos[(int)obj->worn_on]);
                    if ((strlen(buf) + strlen(buf2) + 128) > MAX_STRING_LENGTH) {
                        strcat(buf2, "**OVERFLOW**\r\n");
                        break;
                    } else
                        strcat(buf2, buf);
                }

            page_string(ch->desc, buf2);
            return;
        }
        if (!(vict = get_char_vis(ch, value))) {
            send_to_char(ch, "You cannot see any such person.\r\n");
            return;
        }

        if ((tmp_d = vict->desc)) {
            tmp_d->creature = NULL;
        }
        ch->desc->creature = vict;
        vict->desc = ch->desc;
        do_equipment(vict, arg, 0, SCMD_IMPLANTS, 0);
        ch->desc->creature = ch;
        if ((vict->desc = tmp_d))
            tmp_d->creature = vict;
        break;

    case 30:                    /*paths */
        show_path(ch, value);
        break;

    case 31:                    /* pathobjs */
        show_pathobjs(ch);
        break;

    case 32:                    /* searches */
        show_searches(ch, value, arg);
        break;

    case 33:                    /* str_app */
        strcpy(buf, "STR      to_hit    to_dam    max_encum    max_weap\r\n");
        for (i = 0; i < 35; i++) {
            if (i > 25)
                sprintf(buf2, "/%-2d", j = (i - 25) * 10);
            else
                strcpy(buf2, "");
            sprintf(buf,
                "%s%2d%-4s     %2d         %2d         %4d          %2d\r\n",
                buf, i > 25 ? 18 : i, buf2, str_app[i].tohit, str_app[i].todam,
                str_app[i].carry_w, str_app[i].wield_w);
        }
        sprintf(buf, "%s"
            "18/99      %2d         %2d         %4d          %2d\r\n"
            "18/00      %2d         %2d         %4d          %2d\r\n",
            buf, str_app[35].tohit, str_app[35].todam, str_app[35].carry_w,
            str_app[35].wield_w,
            str_app[36].tohit, str_app[36].todam, str_app[36].carry_w,
            str_app[36].wield_w);
        page_string(ch->desc, buf);
        break;

    case 34:                    /* timezones */
        if (!*value) {
            send_to_char(ch, "What time frame?\r\n");
            return;
        }
        if ((i = search_block(value, time_frames, 0)) < 0) {
            send_to_char(ch, "That is not a valid time frame.\r\n");
            return;
        }
        sprintf(buf, "TIME ZONES for: %s PRIME.\r\n", time_frames[i]);

        /* j is hour mod index, i is time_frame value */
        for (j = -3, found = 0; j < 3; found = 0, j++) {
            for (zone = zone_table; zone; zone = zone->next) {
                if (zone->time_frame == i && zone->hour_mod == j) {
                    if (!found)
                        sprintf(buf, "%s\r\nTime zone (%d):\r\n", buf, j);
                    ++found;
                    sprintf(buf, "%s%3d. %25s   weather: [%d]\r\n", buf,
                        zone->number, zone->name, zone->weather->sky);
                }
            }
            if (strlen(buf) > (size_t) (MAX_STRING_LENGTH * (11 + j)) / 16) {
                strcat(buf, "STOP\r\n");
                break;
            }
        }
        page_string(ch->desc, buf);
        break;
    case 35:                    /* char_class */
        if (!*value) {
            send_to_char(ch, "Show which PC char_class?\r\n");
            return;
        }
        if ((con = parse_char_class(value)) == CLASS_UNDEFINED) {
            send_to_char(ch, "That is not a char_class.\r\n");
            return;
        }
        if (con >= NUM_CLASSES) {
            send_to_char(ch, "That is not a PC char_class.\r\n");
            return;
        }
        show_char_class_skills(ch, con, TRUE, 0);
        break;
    case 36:                    /* unapproved */
        strcpy(buf, "ZCMD approved zones:\r\n");
        for (zone = zone_table; zone; zone = zone->next)
            if (ZONE_FLAGGED(zone, ZONE_ZCMDS_APPROVED))
                sprintf(buf, "%s %3d. %s\r\n", buf, zone->number, zone->name);

        page_string(ch->desc, buf);
        break;
    case 37:
		send_to_char(ch, "This command has been disabled.\r\n");
        break;

    case 38:
        if (!*value) {
            send_to_char(ch, "Show free_create mobs, objs, or rooms?\r\n");
            return;
        }
        if (is_abbrev(value, "objs") || is_abbrev(value, "objects"))
            sfc_mode = SFC_OBJ;
        else if (is_abbrev(value, "mobs") || is_abbrev(value, "mobiless"))
            sfc_mode = SFC_MOB;
        else if (is_abbrev(value, "rooms"))
            sfc_mode = SFC_ROOM;
        else {
            send_to_char(ch, "You must show free_create obj, mob, or room.\r\n");
            return;
        }

        strcpy(buf, "Free_create for this zone:\r\n");

        for (i = ch->in_room->zone->number * 100; i < ch->in_room->zone->top;
            i++) {

            if (sfc_mode == SFC_OBJ && !real_object_proto(i))
                sprintf(buf, "%s[%5d]\r\n", buf, i);
            else if (sfc_mode == SFC_MOB && !real_mobile_proto(i))
                sprintf(buf, "%s[%5d]\r\n", buf, i);
            else if (sfc_mode == SFC_ROOM && !real_room(i))
                sprintf(buf, "%s[%5d]\r\n", buf, i);
        }
        page_string(ch->desc, buf);
        return;

    case 39:                    /* exp_percent */
        if (!*arg || !*value) {
            send_to_char(ch, "Usage: show exp <min percent> <max percent>\r\n");
            return;
        }
        if ((k = atoi(value)) < 0 || (l = atoi(arg)) < 0) {
            send_to_char(ch, "Try using positive numbers, asshole.\r\n");
            return;
        }
        if (k >= l) {
            send_to_char(ch, "Second number must be larger than first.\r\n");
            return;
        }
        if (l - k > 200) {
            send_to_char(ch, "Increment too large.  Stay smaller than 200.\r\n");
            return;
        }
        strcpy(buf, "Mobs with exp in given range:\r\n");
        mit = mobilePrototypes.begin();
        for (; mit != mobilePrototypes.end(); ++mit) {
            mob = mit->second;
            percent = GET_EXP(mob) * 100;
            if ((j = mobile_experience(mob)))
                percent /= j;
            else
                percent = 0;

            if (percent >= k && percent <= j) {
                if (strlen(buf) + 256 > MAX_STRING_LENGTH) {
                    strcat(buf, "**OVERFOW**\r\n");
                    break;
                }
                i++;
                sprintf(buf,
                    "%s%4d. %s[%s%5d%s]%s %35s%s exp:[%9d] pct:%s[%s%3d%s]%s\r\n",
                    buf, i,
                    CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                    GET_MOB_VNUM(mob),
                    CCGRN(ch, C_NRM), CCYEL(ch, C_NRM),
                    GET_NAME(mob), CCNRM(ch, C_NRM),
                    GET_EXP(mob),
                    CCRED(ch, C_NRM), CCCYN(ch, C_NRM),
                    percent, CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
            }
        }
        page_string(ch->desc, buf);
        break;

    case 40:                    /* demand_items */
        if (!*value) {
            send_to_char(ch, "What demand level (in total units existing)?\r\n");
            return;
        }

        if ((k = atoi(value)) <= 0) {
            send_to_char(ch, "-- You are so far out. -- level > 0 !!\r\n");
            return;
        }

        strcpy(buf, "");
        oi = objectPrototypes.begin();
        for (; oi != objectPrototypes.end(); ++oi) {
            obj = oi->second;
            if (obj->shared->number >= k) {
                if (strlen(buf) + 256 > MAX_STRING_LENGTH) {
                    strcat(buf, "**OVERFOW**\r\n");
                    break;
                }
                sprintf(buf,
                    "%s%4d. %s[%s%5d%s] %40s%s Tot:[%4d], House:[%4d]\r\n",
                    buf, ++i, CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                    GET_OBJ_VNUM(obj), CCGRN(ch, C_NRM),
                    obj->name, CCNRM(ch, C_NRM),
                    obj->shared->number, obj->shared->house_count);
            }
        }
        if (i)
            page_string(ch->desc, buf);
        else
            send_to_char(ch, "No items.\r\n");
        break;

    case 42: {                   /* fighting */
        strcpy(buf, "Fighting characters:\r\n");

        CombatDataList::iterator it;
        cit = combatList.begin();
        for (; cit != combatList.end(); ++cit) {
            vict = *cit;
    
            if (!can_see_creature(ch, vict))
                continue;

            if (strlen(buf) > MAX_STRING_LENGTH - 128)
                break;

            sprintf(buf, "%s%3d: %s[%s%5d%s] %s%s%s\r\n", buf, ++i,
                    CCRED(ch, C_NRM), 
                    CCGRN(ch, C_NRM),
                    vict->in_room->number,
                    CCRED(ch, C_NRM),
                    IS_NPC(vict) ? CCCYN(ch, C_NRM) : CCYEL(ch, C_NRM),
                    GET_NAME(vict),
                    CCNRM(ch, C_NRM)); 

            it = vict->getCombatList()->begin();
            for (; it != vict->getCombatList()->end(); ++it) {
                sprintf(buf, "%s             - %s%s%s\r\n", buf,
                        CCWHT(ch, C_NRM), 
                        GET_NAME(it->getOpponent()),
                        CCNRM(ch, C_NRM));
            }

        }

        page_string(ch->desc, buf);
        break;
    }
    case 43:                    /* quad */

        strcpy(buf, "Characters with Quad Damage:\r\n");

        cit = characterList.begin();
        for (; cit != characterList.end(); ++cit) {
            vict = *cit;

            if (!can_see_creature(ch, vict)
                || !affected_by_spell(vict, SPELL_QUAD_DAMAGE))
                continue;

            if (strlen(buf) > MAX_STRING_LENGTH - 128)
                break;

            sprintf(buf, "%s %3d. %28s --- %s [%d]\r\n", buf, ++i,
                GET_NAME(vict), vict->in_room->name, vict->in_room->number);

        }

        page_string(ch->desc, buf);
        break;

    case 44:
        do_show_mobiles(ch, value, arg);
        break;

    case 45:                    /* hunting */

        strcpy(buf, "Characters hunting:\r\n");
        cit = characterList.begin();
        for (; cit != characterList.end(); ++cit) {
            vict = *cit;
            if (!vict->isHunting() || !vict->isHunting()->in_room
                || !can_see_creature(ch, vict))
                continue;

            if (strlen(buf) > MAX_STRING_LENGTH - 128)
                break;

            sprintf(buf, "%s %3d. %23s [%5d] ---> %20s [%5d]\r\n", buf, ++i,
                GET_NAME(vict), vict->in_room->number,
                GET_NAME(vict->isHunting()), vict->isHunting()->in_room->number);
        }

        page_string(ch->desc, buf);
        break;

    case 46:                    /* last_cmd */

        strcpy(buf, "Last cmds:\r\n");
        for (i = 0; i < NUM_SAVE_CMDS; i++)
            sprintf(buf, "%s %2d. (%4d) %25s - '%s'\r\n", buf,
                i, last_cmd[i].idnum, playerIndex.getName(last_cmd[i].idnum),
                last_cmd[i].string);

        page_string(ch->desc, buf);
        break;

    case 47:                    // duperooms

        strcpy(buf,
            " Zone  Name                          Rooms       Bytes\r\n");
        for (zone = zone_table, k = con = tot_rms = 0; zone; zone = zone->next) {
            i = j = 0;
            for (room = (zone->world ? zone->world->next : NULL), num_rms = 1;
                room; room = room->next, num_rms++) {
                if (room->description) {
                    l = strlen(room->description);
                    for (tmp_room = zone->world; tmp_room;
                        tmp_room = tmp_room->next) {
                        if (tmp_room == room)
                            break;
                        if (!tmp_room->description)
                            continue;
                        if (!strncasecmp(tmp_room->description,
                                room->description, l)) {
                            i++;    // number of duplicate rooms in zone 
                            j += l;    // total length of duplicates in zone;
                            break;
                        }
                    }
                }
            }
            if (zone->world)
                tot_rms += num_rms;
            k += i;                // number of duplicate rooms total;
            con += j;            // total length of duplicates in world;
            if (i) {
                sprintf(buf, "%s [%3d] %s%-30s%s  %3d/%3d  %6d\r\n", buf,
                    zone->number, CCCYN(ch, C_NRM), zone->name,
                    CCNRM(ch, C_NRM), i, num_rms, j);
            }
        }
        sprintf(buf,
            "%s------------------------------------------------------\r\n"
            " Totals:                              %4d/%3d  %6d\r\n", buf, k,
            tot_rms, con);
        page_string(ch->desc, buf);
        break;

    case 48:                    // specialization
        if (!(vict = get_char_vis(ch, value))) {
            send_to_char(ch, "You cannot see any such person.\r\n");
            return;
        }
        sprintf(buf, "Weapon specialization for %s:\r\n", GET_NAME(vict));
        for (obj = NULL, i = 0; i < MAX_WEAPON_SPEC; i++, obj = NULL) {
            if (GET_WEAP_SPEC(vict, i).vnum > 0)
                obj = real_object_proto(GET_WEAP_SPEC(vict, i).vnum);
            sprintf(buf, "%s [%5d] %-30s [%2d]\r\n", buf,
                GET_WEAP_SPEC(vict, i).vnum,
                obj ? obj->name : "--- ---",
                GET_WEAP_SPEC(vict, i).level);
        }
        page_string(ch->desc, buf);
        break;
    case 49:                    // p_index
		send_to_char(ch, "Disabled.");
        return;

#define ZEXITS_USAGE "Usage: zexit <f|t> <zone>\r\n"

    case 50:                    // zexits

        if (!*arg || !*value) {
            send_to_char(ch, ZEXITS_USAGE);
            return;
        }
        k = atoi(arg);

        if (!(zone = real_zone(k))) {
            send_to_char(ch, "Zone %d does not exist.\r\n", k);
            return;
        }
        // check for exits FROM zone k, easy to do
        if (*value == 'f') {

            sprintf(buf, "Rooms with exits FROM zone %d:\r\n", k);

            for (room = zone->world, j = 0; room; room = room->next) {
                for (i = 0; i < NUM_DIRS; i++) {
                    if (room->dir_option[i] && room->dir_option[i]->to_room &&
                        room->dir_option[i]->to_room->zone != zone) {
                        if (strlen(buf) > MAX_STRING_LENGTH - 128) {
                            strcat(buf, "**OVERFLOW**\r\n");
                            break;
                        }

                        sprintf(buf, "%s%3d. [%5d] %-40s %5s -> %7d\r\n", buf,
                            ++j, room->number, room->name, dirs[i],
                            room->dir_option[i]->to_room->number);
                    }
                }
            }
            page_string(ch->desc, buf);
            return;
        }
        // check for exits TO zone k, a slow operation
        if (*value == 't') {

            sprintf(buf, "Rooms with exits TO zone %d:\r\n", k);

            for (zone = zone_table, j = 0, found = 0; zone; zone = zone->next) {
                if (zone->number == k)
                    continue;
                for (room = zone->world; room; room = room->next) {
                    for (i = 0; i < NUM_DIRS; i++) {
                        if (room->dir_option[i] && room->dir_option[i]->to_room
                            && room->dir_option[i]->to_room->zone->number ==
                            k) {
                            if (strlen(buf) > MAX_STRING_LENGTH - 128) {
                                strcat(buf, "**OVERFLOW**\r\n");
                                break;
                            }
                            if (!found) {
                                strcat(buf, "------------\r\n");
                                ++found;
                            }
                            sprintf(buf, "%s%3d. [%5d] %-40s %5s -> %7d\r\n",
                                buf, ++j, room->number, room->name, dirs[i],
                                room->dir_option[i]->to_room->number);
                        }
                    }
                }
            }
            page_string(ch->desc, buf);
            return;
        }

        send_to_char(ch, "First argument must be 'to' or 'from'.\r\n");
        send_to_char(ch, ZEXITS_USAGE);

        break;

    case 51:                    // mlevels

        show_mlevels(ch, value, arg);
        break;

    case 52:                    // plevels
        send_to_char(ch, "Unimped.\r\n");
        break;

    case 53:                    // mobkills
        show_mobkills(ch, value, arg);
        break;

    case 54:                    // wizcommands
        show_wizcommands(ch);
        break;

    case 55:                    // timewarps
        show_timewarps(ch);
        break;
    case 56:                    // zone commands
        strcpy(buf, value);
        strcat(buf, arg);
        do_zone_cmdlist(ch, ch->in_room->zone, buf);
        break;
    case 57:
        show_multi(ch, value);
        break;
    case 58:
        show_file(ch, "log/help.log", 0); 
        break;
    case 59:
        show_account( ch, value ); 
        break;
    case 60: // rooms
        show_rooms(ch, value, arg);
        break;
	case 61: // boards
		gen_board_show(ch);
    default:
        send_to_char(ch, "Sorry, I don't understand that.\r\n");
        break;
    }
}


#define PC   1
#define NPC  2
#define BOTH 3

#define MISC        0
#define BINARY        1
#define NUMBER        2

#define SET_OR_REMOVE(flagset, flags) { \
        if (on) SET_BIT(flagset, flags); \
        else if (off) REMOVE_BIT(flagset, flags); }

#define RANGE(low, high) (value = MAX((low), MIN((high), (value))))

ACMD(do_set)
{
    int i, l, tp;
    struct room_data *room;
    struct Creature *vict = NULL, *vict2 = NULL;
    struct Creature *cbuf = NULL;
    char *field, *name;
    char *arg1, *arg2;
    int on = 0, off = 0, value = 0;
    char is_file = 0, is_mob = 0, is_player = 0;
    int parse_char_class(char *arg);
    int parse_race(char *arg);

    static struct set_struct fields[] = {
        {"brief", LVL_IMMORT, PC, BINARY, "WizardFull"},    /* 0 */
        {"invstart", LVL_IMMORT, PC, BINARY, "WizardFull"},    /* 1 */
        {"title", LVL_IMMORT, PC, MISC, "AdminBasic"},
        {"nosummon", LVL_IMMORT, PC, BINARY, "WizardFull"},
        {"maxhit", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"maxmana", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},    /* 5 */
        {"maxmove", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"hit", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"mana", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"move", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"align", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},    /* 10 */
        {"str", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"stradd", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"int", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"wis", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"dex", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},    /* 15 */
        {"con", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"sex", LVL_IMMORT, BOTH, MISC, "WizardFull"},
        {"ac", LVL_IMMORT, BOTH, NUMBER, "Coder"},
        {"gold", LVL_IMMORT, BOTH, NUMBER, "AdminFull"},
        {"bank", LVL_IMMORT, PC, NUMBER, "AdminFull"},    /* 20 */
        {"exp", LVL_IMMORT, BOTH, NUMBER, "Coder"},
        {"hitroll", LVL_IMMORT, BOTH, NUMBER, "Coder"},
        {"damroll", LVL_IMMORT, BOTH, NUMBER, "Coder"},
        {"invis", LVL_IMMORT, PC, NUMBER, "WizardFull"},
        {"nohassle", LVL_IMMORT, PC, BINARY, "WizardFull"},    /* 25 */
        {"frozen", LVL_IMMORT, PC, BINARY, "AdminFull"},
        {"nowho", LVL_IMMORT, PC, BINARY, "WizardFull"},
        {"lessons", LVL_IMMORT, PC, NUMBER, "WizardFull"},
        {"drunk", LVL_IMMORT, BOTH, MISC, "WizardFull"},
        {"hunger", LVL_IMMORT, BOTH, MISC, "WizardFull"},    /* 30 */
        {"thirst", LVL_IMMORT, BOTH, MISC, "WizardFull"},
        {"killer", LVL_IMMORT, PC, BINARY, "AdminFull"},
        {"thief", LVL_IMMORT, PC, BINARY, "AdminFull"},
        {"level", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"room", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},    /* 35 */
        {"roomflag", LVL_IMMORT, PC, BINARY, "WizardFull"},
        {"siteok", LVL_IMMORT, PC, BINARY, "AdminFull"},
		{"name", LVL_IMMORT, BOTH, MISC, "AdminFull"},
        {"class", LVL_IMMORT, BOTH, MISC, "WizardFull"},
        {"nowizlist", LVL_IMMORT, PC, BINARY, "WizardAdmin"},    /* 40 */
        {"quest", LVL_IMMORT, PC, NUMBER, "Coder"},
        {"loadroom", LVL_IMMORT, PC, MISC, "Coder"},
        {"color", LVL_IMMORT, PC, BINARY, "Coder"},
        {"idnum", LVL_GRIMP, PC, NUMBER, "Coder"},
        {"passwd", LVL_LUCIFER, PC, MISC, "Coder"},    /* 45 */
        {"nodelete", LVL_IMMORT, PC, BINARY, "WizardFull"},
        {"cha", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"hometown", LVL_IMMORT, PC, NUMBER, "WizardFull"},
        {"race", LVL_IMMORT, BOTH, MISC, "WizardFull"},
        {"height", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},    /* 50 */
        {"weight", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"nosnoop", LVL_ENTITY, PC, BINARY, "WizardAdmin"},
        {"clan", LVL_IMMORT, PC, MISC, "WizardFull"},
        {"leader", LVL_IMMORT, PC, BINARY, "WizardFull"},
        {"life", LVL_IMMORT, PC, NUMBER, "AdminFull"},    /* 55 */
        {"debug", LVL_IMMORT, PC, BINARY, "WizardBasic"},
        {"page", LVL_IMMORT, PC, NUMBER, "WizardFull"},
        {"screen", LVL_IMMORT, PC, NUMBER, "Coder"},
        {"remort_class", LVL_IMMORT, BOTH, MISC, "WizardFull"},
        {"hunting", LVL_IMMORT, NPC, MISC, "Coder"},    /* 60 */
        {"fighting", LVL_IMMORT, BOTH, MISC, "Coder"},
        {"mobkills", LVL_IMMORT, PC, NUMBER, "Coder"},
        {"pkills", LVL_IMMORT, PC, NUMBER, "AdminFull"},
        {"newbiehelper", LVL_ETERNAL, PC, BINARY, "Coder"},
        {"holylight", LVL_IMMORT, PC, BINARY, "Coder"},    /* 65 */
        {"notitle", LVL_IMMORT, PC, BINARY, "AdminFull"},
        {"remortinvis", LVL_IMMORT, PC, NUMBER, "AdminBasic"},
        {"toughguy", LVL_IMMORT, PC, BINARY, "AdminFull"},
        {"nointwiz", LVL_ELEMENT, PC, BINARY, "WizardFull"},
        {"halted", LVL_IMMORT, PC, BINARY, "WizardFull"},    /* 70 */
        {"syslog", LVL_IMMORT, PC, MISC, "WizardFull"},
        {"broken", LVL_IMMORT, PC, NUMBER, "WizardFull"},
        {"totaldamage", LVL_IMMORT, PC, NUMBER, "Coder"},
        {"oldchar_class", LVL_IMMORT, PC, MISC, "WizardFull"},
        {"olcgod", LVL_IMMORT, PC, BINARY, "OLCAdmin"},    /* 75 */
        {"tester", LVL_IMMORT, PC, BINARY, "OlcWorldWrite"},
        {"mortalized", LVL_IMPL, PC, BINARY, "Coder"},
        {"noaffects", LVL_IMMORT, PC, BINARY, "Coder"},
        {"questor", LVL_ELEMENT, PC, BINARY, "Coder"},
        {"age_adjust", LVL_IMMORT, PC, NUMBER, "Coder"},    /* 80 */
        {"cash", LVL_IMMORT, BOTH, NUMBER, "AdminFull"},
        {"generation", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"path", LVL_LUCIFER, NPC, MISC, "Coder"},
        {"lightread", LVL_IMMORT, PC, BINARY, "Coder"},
        {"remort_tough", LVL_IMMORT, PC, BINARY, "AdminFull"},    /* 85 */
        {"council", LVL_IMMORT, PC, BINARY, "WizardFull"},
        {"nopost", LVL_IMMORT, PC, BINARY, "AdminBasic"},
        {"logging", LVL_IMMORT, PC, BINARY, "WizardAdmin"},
        {"noshout", LVL_IMMORT, PC, BINARY, "WizardFull"},
        {"nopk", LVL_IMMORT, PC, BINARY, "AdminFull"},    /* 90 */
        {"soilage", LVL_IMMORT, PC, MISC, "WizardBasic"},
        {"econet", LVL_IMMORT, BOTH, NUMBER, "AdminFull"},
        {"specialization", LVL_IMMORT, PC, MISC, "AdminFull"},
        {"qpallow", LVL_IMMORT, PC, NUMBER, "QuestorAdmin,WizardFull"},    /*  95 */
        {"soulless", LVL_IMMORT, BOTH, BINARY, "WizardFull"},
        {"buried", LVL_IMMORT, PC, BINARY, "AdminFull"},
        {"speed", LVL_IMMORT, PC, NUMBER, "Coder"},
        {"badge", LVL_IMMORT, PC, MISC, "AdminFull"},
        {"skill", LVL_IMMORT, PC, MISC, "WizardFull"},
        {"reputation", LVL_IMMORT, PC, NUMBER, "AdminFull"},
        {"language", LVL_IMMORT, BOTH, MISC, "AdminFull"},
        {"\n", 0, BOTH, MISC, ""}
    };

    name = tmp_getword(&argument);
    if (!strcmp(name, "file")) {
        is_file = 1;
        name = tmp_getword(&argument);
    } else if (!str_cmp(name, "player")) {
        is_player = 1;
        name = tmp_getword(&argument);
    } else if (!str_cmp(name, "mob")) {
        is_mob = 1;
        name = tmp_getword(&argument);
    }
    field = tmp_getword(&argument);

    if (!*name || !*field) {
        send_to_char(ch, "Usage: set <victim> <field> <value>\r\n");
        vector <string> cmdlist;
        for (i = 0; fields[i].level != 0; i++) {
            if (!Security::canAccess(ch, fields[i]))
                continue;
            cmdlist.push_back(fields[i].cmd);
        }
        sort(cmdlist.begin(), cmdlist.end());

        for (i = 0, l = 1; i < (int)(cmdlist.size()); i++) {
            send_to_char(ch, "%12s%s", cmdlist[i].c_str(),
                ((l++) % 5) ? " " : "\n");
        }

        if ((l - 1) % 5)
            send_to_char(ch, "\r\n");
        return;
    }
    if (!is_file) {
        if (is_player) {
            if (!(vict = get_player_vis(ch, name, 0))) {
                send_to_char(ch, "There is no such player.\r\n");
                return;
            }
        } else {
            if (!(vict = get_char_vis(ch, name))) {
                send_to_char(ch, "There is no such creature.\r\n");
                return;
            }
        }
    } else if (is_file) {
        cbuf = new Creature(true);
		vict = NULL;
		if (!playerIndex.exists(name))
            send_to_char(ch, "There is no such player.\r\n");
		else if (!cbuf->loadFromXML(playerIndex.getID(name)))
			send_to_char(ch, "Error loading player file.\r\n");
		else if (GET_LEVEL(cbuf) >= GET_LEVEL(ch) && GET_IDNUM(ch) != 1)
			send_to_char(ch, "Sorry, you can't do that.\r\n");
        else
            vict = cbuf;
		if (vict != cbuf) {
			delete cbuf;
			return;
		}
    }

    if (GET_LEVEL(ch) < LVL_LUCIFER) {
        if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict) && vict != ch) {
            send_to_char(ch, "Maybe that's not such a great idea...\r\n");
            return;
        }
    }

    for (l = 0; *(fields[l].cmd) != '\n'; l++) {
        if (!strncmp(field, fields[l].cmd, strlen(field)))
            break;
    }

    if (GET_LEVEL(ch) < fields[l].level && subcmd != SCMD_TESTER_SET ) {
        send_to_char(ch, "You are not able to perform this godly deed!\r\n");
        return;
    }

    if (!Security::canAccess(ch, fields[l]) && subcmd != SCMD_TESTER_SET) {
        send_to_char(ch, "You do not have that power.\r\n");
        return;
    }


    if (IS_NPC(vict) && !(fields[l].pcnpc & NPC)) {
        send_to_char(ch, "You can't do that to a beast!\r\n");
        return;
    } else if (!IS_NPC(vict) && !(fields[l].pcnpc & PC)) {
        send_to_char(ch, "That can only be done to a beast!\r\n");
        return;
    }

    if (fields[l].type == BINARY) {
        if (!strcmp(argument, "on") || !strcmp(argument, "yes"))
            on = 1;
        else if (!strcmp(argument, "off") || !strcmp(argument, "no"))
            off = 1;
        if (!(on || off)) {
            send_to_char(ch, "Value must be on or off.\r\n");
            return;
        }
    } else if (fields[l].type == NUMBER) {
        value = atoi(argument);
    }

    strcpy(buf, "Okay.");        /* can't use OK macro here 'cause of \r\n */


    switch (l) {
    case 0:
        SET_OR_REMOVE(PRF_FLAGS(vict), PRF_BRIEF);
        break;
    case 1:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_INVSTART);
        break;
    case 2:
        set_title(vict, argument);
        sprintf(buf, "%s's title is now: %s", GET_NAME(vict), GET_TITLE(vict));
        slog("%s has changed %s's title to : '%s'.", GET_NAME(ch),
            GET_NAME(vict), GET_TITLE(vict));
        break;
    case 3:
		send_to_char(ch, "Nosummon disabled.\r\n"); break;
    case 4:
        vict->points.max_hit = RANGE(1, 30000);
        affect_total(vict);
        break;
    case 5:
        vict->points.max_mana = RANGE(1, 30000);
        affect_total(vict);
        break;
    case 6:
        vict->points.max_move = RANGE(1, 30000);
        affect_total(vict);
        break;
    case 7:
        vict->points.hit = RANGE(-9, vict->points.max_hit);
        affect_total(vict);
        break;
    case 8:
        vict->points.mana = RANGE(0, vict->points.max_mana);
        affect_total(vict);
        break;
    case 9:
        vict->points.move = RANGE(0, vict->points.max_move);
        affect_total(vict);
        break;
    case 10:
        GET_ALIGNMENT(vict) = RANGE(-1000, 1000);
        affect_total(vict);
        break;
    case 11:
        RANGE(3, 25);
        if (value != 18)
            vict->real_abils.str_add = 0;
        /*    else if (value > 18)
           value += 10; */
        vict->real_abils.str = value;
        affect_total(vict);
        break;
    case 12:
        if (vict->real_abils.str != 18) {
            send_to_char(ch, 
                "Stradd is only applicable with 18 str... asshole.\r\n");
            return;
        }
        vict->real_abils.str_add = RANGE(0, 100);
        affect_total(vict);
        break;
    case 13:
        if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
            RANGE(3, 25);
        else
            RANGE(3, MIN(25, GET_REMORT_GEN(vict) + 18));
        vict->real_abils.intel = value;
        affect_total(vict);
        break;
    case 14:
        if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
            RANGE(3, 25);
        else
            RANGE(3, MIN(25, GET_REMORT_GEN(vict) + 18));
        vict->real_abils.wis = value;
        affect_total(vict);
        break;
    case 15:
        if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
            RANGE(3, 25);
        else
            RANGE(3, MIN(25, GET_REMORT_GEN(vict) + 18));
        vict->real_abils.dex = value;
        affect_total(vict);
        break;
    case 16:
        if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
            RANGE(3, 25);
        else
            RANGE(3, MIN(25, GET_REMORT_GEN(vict) + 18));
        vict->real_abils.con = value;
        affect_total(vict);
        break;
    case 17:
        if (!str_cmp(argument, "male"))
            vict->player.sex = SEX_MALE;
        else if (!str_cmp(argument, "female"))
            vict->player.sex = SEX_FEMALE;
        else if (!str_cmp(argument, "neutral"))
            vict->player.sex = SEX_NEUTRAL;
        else {
            send_to_char(ch, "Must be 'male', 'female', or 'neutral'.\r\n");
            return;
        }
        break;
    case 18:
        vict->points.armor = RANGE(-200, 100);
        affect_total(vict);
        break;
    case 19:
        GET_GOLD(vict) = RANGE(0, 1000000000);
        break;
    case 20:
		send_to_char(ch, "Disabled.  Use aset instead.\r\n");
        break;
    case 21:
        vict->points.exp = RANGE(0, 1500000000);
        break;
    case 22:
        vict->points.hitroll = RANGE(-200, 200);
        affect_total(vict);
        break;
    case 23:
        vict->points.damroll = RANGE(-200, 200);
        affect_total(vict);
        break;
    case 24:
        if (GET_LEVEL(ch) < GET_LEVEL(vict)) {
            send_to_char(ch, "You aren't godly enough for that!\r\n");
            return;
        }
        GET_INVIS_LVL(vict) = RANGE(0, GET_LEVEL(vict));
        if (GET_INVIS_LVL(vict) > LVL_LUCIFER)
            GET_INVIS_LVL(vict) = LVL_LUCIFER;
        break;
    case 25:
        if (GET_LEVEL(ch) < GET_LEVEL(vict)) {
            send_to_char(ch, "You aren't godly enough for that!\r\n");
            return;
        }
        SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOHASSLE);
        break;
    case 26:
        if (ch == vict) {
            send_to_char(ch, "Better not -- could be a long winter!\r\n");
            return;
        }
        slog("(GC) %s set %s %sfrozen.", 
                GET_NAME(ch), 
                GET_NAME(vict),
                PLR_FLAGGED(vict, PLR_FROZEN) ? "" : "UN-"); 
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_FROZEN);
        break;
    case 27:
		SET_OR_REMOVE(PRF2_FLAGS(vict), PRF2_NOWHO); break;
    case 28:
		send_to_char(ch, "No more pracs.\r\n"); break;
    case 29:
    case 30:
    case 31:
        if (!str_cmp(argument, "off")) {
            GET_COND(vict, (l - 29)) = (char)-1;
            sprintf(buf, "%s's %s now off.", GET_NAME(vict), fields[l].cmd);
        } else if (is_number(argument)) {
            value = atoi(argument);
            RANGE(0, 24);
            GET_COND(vict, (l - 29)) = (char)value;
            sprintf(buf, "%s's %s set to %d.", GET_NAME(vict), fields[l].cmd,
                value);
        } else {
            send_to_char(ch, "Must be 'off' or a value from 0 to 24.\r\n");
            return;
        }
        break;
    case 32:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_KILLER);
        break;
    case 33:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_THIEF);
        break;
    case 34:
        if ((value > GET_LEVEL(ch) && GET_IDNUM(ch) != 1) || value > LVL_GRIMP) {
            send_to_char(ch, "You can't do that.\r\n");
            return;
        }

        RANGE(0, LVL_GRIMP);

        if (value >= 50 && !Security::isMember(ch, "AdminFull")) {
            send_to_char(ch, "That should be done with advance.\r\n");
            return;
        }

        vict->player.level = (byte) value;
        break;
    case 35:
        if ((room = real_room(value)) == NULL) {
            send_to_char(ch, "No room exists with that number.\r\n");
            return;
        }
        char_from_room(vict,false);
        char_to_room(vict, room,false);
        break;
    case 36:
        SET_OR_REMOVE(PRF_FLAGS(vict), PRF_ROOMFLAGS);
        break;
    case 37:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_SITEOK);
        slog("(GC) %s %ssiteok'd %s.", GET_NAME(ch), PLR_FLAGGED(vict,
                PLR_SITEOK) ? "" : "UN-", GET_NAME(vict));

        break;
    case 38:
		if (IS_PC(vict)) {
            if (!(Security::isMember(ch, "AdminFull")
                  && Valid_Name(argument))) {
                send_to_char(ch, "That character name is invalid.\r\n");
                return;
            }

            if (playerIndex.exists(argument)) {
                send_to_char(ch, "There is already a player by that name.\r\n");
                return;
            }
		}

		slog("%s%s set %s %s's name to '%s'",
			(IS_NPC(vict) ? "":"(GC) "),
			GET_NAME(ch),
			(IS_NPC(vict) ? "NPC":"PC"),
			GET_NAME(vict),
			argument);

		free(GET_NAME(vict));
		GET_NAME(vict) = strdup(argument);
		// Set name
		if (IS_PC(vict)) {
			long acct_id;

			acct_id = playerIndex.getAccountID(GET_IDNUM(vict));
			sql_exec("update players set name='%s' where idnum=%ld",
				tmp_sqlescape(argument), GET_IDNUM(vict));
			vict->saveToXML();
		}
		break;
    case 39:
        if ((i = parse_char_class(argument)) == CLASS_UNDEFINED) {
            send_to_char(ch, argument);
            send_to_char(ch, "\r\n");
            send_to_char(ch, "That is not a char_class.\r\n");
            return;
        }
        GET_CLASS(vict) = i;
        break;
    case 40:
		send_to_char(ch, "Disabled.");
        break;
    case 41:
		GET_QUEST(vict) = value;
        break;
    case 42:
		if (real_room(i = atoi(argument)) != NULL) {
			GET_HOMEROOM(vict) = i;
			sprintf(buf, "%s will enter at %d.", GET_NAME(vict),
				GET_HOMEROOM(vict));
		} else
			sprintf(buf, "That room does not exist!");
        break;
    case 43:
        SET_OR_REMOVE(PRF_FLAGS(vict), (PRF_COLOR_1 | PRF_COLOR_2));
        break;
    case 44:
        if (GET_IDNUM(ch) != 1 || !IS_NPC(vict))
            return;
        GET_IDNUM(vict) = value;
        break;
    case 45:
        if (!is_file) {
            send_to_char(ch, "You must set password of player in file.\r\n");
            return;
        }
        if (GET_LEVEL(ch) < LVL_LUCIFER || !Security::isMember(ch,"Coder") ) {
            send_to_char(ch, "Please don't use this command, yet.\r\n");
            return;
        }
        if (GET_LEVEL(vict) >= GET_LEVEL(ch) && GET_IDNUM(ch) != 1) {
            send_to_char(ch, "You cannot change that.\r\n");
            return;
        }
		send_to_char(ch, "Disabled.");
        return;
    case 46:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NODELETE);
        break;
    case 47:
        if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
            RANGE(3, 25);
        else
            RANGE(3, 18);
        vict->real_abils.cha = value;
        affect_total(vict);
        break;
    case 48:
        if (IS_NPC(vict)) {
            send_to_char(ch, "That's what the zone file is for, fool!\r\n");
            return;
        }
        GET_HOME(vict) = RANGE(0, 11);
        send_to_char(ch, "%s will have home at %s.\r\n", GET_NAME(vict),
            home_towns[(int)GET_HOME(vict)]);
        break;
    case 49:
        if ((i = parse_race(argument)) == RACE_UNDEFINED) {
            send_to_char(ch, "That is not a race.\r\n");
            return;
        }
        GET_RACE(vict) = i;
        break;
    case 50:
        if ((value > 400) && (GET_LEVEL(ch) < LVL_CREATOR)) {
            send_to_char(ch, "That is too tall.\r\n");
            return;
        } else
            GET_HEIGHT(vict) = value;
        break;
    case 51:
        if ((value > 800) && (GET_LEVEL(ch) < LVL_CREATOR)) {
            send_to_char(ch, "That is too heavy.\r\n");
            return;
        } else
            GET_WEIGHT(vict) = value;
        break;
    case 52:
        SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOSNOOP);
        if (!is_file && vict->desc->snoop_by.size()) {
            for (unsigned x = 0; x < vict->desc->snoop_by.size(); x++) {
                send_to_char(vict->desc->snoop_by[x]->creature, "Your snooping session has ended.\r\n");
                stop_snooping(vict->desc->snoop_by[x]->creature);
            }
        }
        break;
    case 53: {
        struct clan_data *clan = real_clan(GET_CLAN(ch));

        if (is_number(argument) && !atoi(argument)) {
            if (clan && (clan->owner == GET_IDNUM(ch)))
                clan->owner = 0;
            GET_CLAN(vict) = 0;
            send_to_char(ch, "Clan set to none.\r\n");
        } else if (!clan_by_name(argument)) {
            send_to_char(ch, "There is no such clan.\r\n");
            return;
        } else {
            if (clan && (clan->owner == GET_IDNUM(ch)))
                clan->owner = 0;
            GET_CLAN(vict) = clan_by_name(argument)->number;
        }
        break;
    }
    case 54:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_CLAN_LEADER);
        break;
    case 55:
        GET_LIFE_POINTS(vict) = RANGE(0, 100);
        break;
    case 56:
        SET_OR_REMOVE(PRF2_FLAGS(vict), PRF2_DEBUG);
        break;
    case 57:
    case 58:
		send_to_char(ch, "Disabled.\r\n");
        return;
    case 59:
        if ((i = parse_char_class(argument)) == CLASS_UNDEFINED) {
            send_to_char(ch, "That is not a char_class.\r\n");
        }
        GET_REMORT_CLASS(vict) = i;
        break;
    case 60:
        if (!(vict2 = get_char_vis(ch, argument))) {
            send_to_char(ch, "No such target character around.\r\n");
        } else {
            vict->startHunting(vict2);
            send_to_char(ch, "%s now hunts %s.\r\n", GET_NAME(vict),
                GET_NAME(vict2));
        }
        return;
    case 61:
        if (!(vict2 = get_char_vis(ch, argument))) {
            send_to_char(ch, "No such target character around.\r\n");
        } else {
            vict->addCombat(vict2, false);
            send_to_char(ch, "%s is now fighting %s.\r\n", GET_NAME(vict),
                GET_NAME(vict2));
        }
        return;
    case 62:
        GET_MOBKILLS(vict) = value;
        break;
    case 63:
        GET_PKILLS(vict) = value;
        break;
    case 64:
        SET_OR_REMOVE(PRF2_FLAGS(vict), PRF2_NEWBIE_HELPER);
        break;
    case 65:
        SET_OR_REMOVE(PRF_FLAGS(vict), PRF_HOLYLIGHT);
        break;
    case 66:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOTITLE);
        break;
    case 67:
        GET_INVIS_LVL(vict) = RANGE(0, GET_LEVEL(vict));
        break;
    case 68:
		send_to_char(ch, "Disabled.\r\n");
        return;
    case 69:
		send_to_char(ch, "Disabled.\r\n");
        return;
    case 70:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_HALT);
        break;
    case 71:
        if (((tp = search_block(argument, logtypes, FALSE)) == -1)) {
            send_to_char(ch, 
                "Usage: syslog { Off | Brief | Normal | Complete }\r\n");
            return;
        }
        REMOVE_BIT(PRF_FLAGS(vict), PRF_LOG1 | PRF_LOG2);
        SET_BIT(PRF_FLAGS(vict),
            (PRF_LOG1 * (tp & 1)) | (PRF_LOG2 * (tp & 2) >> 1));

        send_to_char(ch, "%s's syslog is now %s.\r\n", GET_NAME(vict),
            logtypes[tp]);
        break;
    case 72:
        GET_BROKE(vict) = RANGE(0, NUM_COMPS);
        break;
    case 73:
        GET_TOT_DAM(vict) = RANGE(0, 1000000000);
        break;
    case 74:
        if (IS_VAMPIRE(vict)) {
            if ((i = parse_char_class(argument)) == CLASS_UNDEFINED) {
                send_to_char(ch, "That is not a char_class.\r\n");
                return;
            }
        } else
            i = atoi(argument);
        GET_OLD_CLASS(vict) = i;
        break;
    case 75:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_OLCGOD);
        break;
    case 76:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_TESTER);
        break;
    case 77:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_MORTALIZED);
        break;
    case 78:
        SET_OR_REMOVE(PRF2_FLAGS(vict), PRF2_NOAFFECTS);
        break;
    case 79:
        send_to_char(ch, "Disabled.\r\n");
        break;
    case 80:
        vict->player.age_adjust = (byte) RANGE(-125, 125);
        break;
    case 81:
        GET_CASH(vict) = RANGE(0, 1000000000);
        break;
    case 82:
        GET_REAL_GEN(vict) = RANGE(0, 1000);
        break;
    case 83:
        if (add_path_to_mob(vict, argument)) {
            sprintf(buf, "%s now follows the path titled: %s.",
                GET_NAME(vict), argument);
        } else
            sprintf(buf, "Could not assign that path to mobile.");
        break;
    case 84:
    case 85:
    case 86:
		send_to_char(ch, "Disabled.\r\n");
        return;
    case 87:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOPOST);
        break;
    case 88:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_LOG);
        slog("(GC) Logging %sactivated for %s by %s.", 
                PLR_FLAGGED(vict, PLR_LOG) ? "" : "de-", 
                GET_NAME(ch), 
                GET_NAME(vict) );
        break;
    case 89:
		send_to_char(ch, "Disabled.\r\n"); 
		return;
    case 90:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOPK);
        break;
    case 91:
        arg1 = tmp_getword(&argument);
        arg2 = tmp_getword(&argument);
        if (!*arg1 || !*arg2) {
            send_to_char(ch, "Usage: set <vict> soilage <pos> <value>\r\n");
            return;
        }
        if ((i = search_block(arg1, wear_eqpos, FALSE)) < 0) {
            send_to_char(ch, "No such eqpos.\r\n");
            return;
        }
        CHAR_SOILAGE(vict, i) = atoi(arg2);
        break;
    case 92:
		send_to_char(ch, "Disabled.  Use aset instead.\r\n");
        break;
    case 93:                    // specialization
        arg1 = tmp_getword(&argument);
        arg2 = tmp_getword(&argument);
        if (!*arg1 || !*arg2) {
            send_to_char(ch, "Usage: set <vict> spec <vnum> <level>\r\n");
            return;
        }
        value = atoi(arg1);
        tp = atoi(arg2);
        tp = MIN(MAX(0, tp), 10);
        l = 0;
        for (i = 0; i < MAX_WEAPON_SPEC; i++) {
            if (GET_WEAP_SPEC(vict, i).vnum == value) {
                if (!(GET_WEAP_SPEC(vict, i).level = tp))
                    GET_WEAP_SPEC(vict, i).vnum = 0;
                l = 1;
            }
        }
        if (!l) {
            send_to_char(ch, "No such spec on this person.\r\n");
            return;
        }
        send_to_char(ch, "[%d] spec level set to %d.\r\n", value, tp);
        return;
        // qpoints
    case 94:
        GET_QUEST_ALLOWANCE(vict) = RANGE(0, 100);
        break;

    case 95:
        if (IS_NPC(vict)) {
            SET_OR_REMOVE(MOB_FLAGS(vict), MOB_SOULLESS);
        } else {
            SET_OR_REMOVE(PLR2_FLAGS(vict), PLR2_SOULLESS);
        }
        break;
    case 96:
        if (IS_NPC(vict)) {
            send_to_char(ch, "Just kill the bugger!\r\n");
            break;
        }
		send_to_char(ch, "Disabled.  Use the 'delete' command.\r\n");
		break;
    case 97:                    // Set Speed
        vict->setSpeed(RANGE(0, 100));
        break;
    case 98:
        if(!argument || !*argument) {
            send_to_char(ch, "You have to specify the badge.\r\n");
            return;
        }
		if (IS_NPC(vict)) {
            send_to_char( ch, "As good an idea as it might seem, it just won't work.\r\n");
            return;
        }
		if (strlen(argument) > MAX_BADGE_LENGTH) {
			send_to_char(ch, "The badge must not be more than seven characters.\r\n");
			return;
		}
		strcpy(BADGE(vict), argument);
		// Convert to uppercase
		arg1 = BADGE(vict);
		for (arg1 = BADGE(vict);*arg1;arg1++)
			*arg1 = toupper(*arg1);
		sprintf(buf, "You set %s's badge to %s", GET_NAME(vict),
			BADGE(vict));

        break;

    case 99:
        name = tmp_getquoted(&argument);
        arg1 = tmp_getword(&argument);
        perform_skillset(ch, vict, name, atoi(arg1));
        break;

    case 100:
        vict->set_reputation(RANGE(0, 1000));
        break;
    case 101:
        arg1 = tmp_getword(&argument);
        arg2 = tmp_getword(&argument);

        if (*arg1 && *arg2) {
            i = find_tongue_idx_by_name(arg1);
            value = atoi(arg2);
            if (i == TONGUE_NONE) {
                send_to_char(ch, "%s is not a valid language.\r\n", arg1);
                return;
            } else if (!is_number(arg2) || value < 0 || value > 100) {
                send_to_char(ch, "Language fluency must be a number from 0 to 100.\r\n");
                return;
            } else {
                SET_TONGUE(vict, i, value);
                sprintf(buf, "You set %s's fluency in %s to %d",
                        GET_NAME(vict), tongue_name(i), value);
            }
        } else {
            send_to_char(ch, "Usage: set <vict> language <language> <fluency>\r\n");
            return;
        }
        break;
    default:
        sprintf(buf, "Can't set that!");
        break;
    }

    if (fields[l].type == BINARY) {
        send_to_char(ch, "%s %s for %s.\r\n", fields[l].cmd, ONOFF(on),
            GET_NAME(vict));
    } else if (fields[l].type == NUMBER) {
        send_to_char(ch, "%s's %s set to %d.\r\n", GET_NAME(vict),
            fields[l].cmd, value);
    } else
        send_to_char(ch, "%s\r\n", buf);

    if (!IS_NPC(vict))
        vict->saveToXML();

    if (is_file) {
        delete cbuf;
        send_to_char(ch, "Saved in file.\r\n");
    }
}

ACMD(do_aset)
{
    static struct set_struct fields[] = {
        {"past_bank", LVL_IMMORT, PC, NUMBER, "AdminFull"},
        {"future_bank", LVL_IMMORT, PC, NUMBER, "AdminFull"},
        {"reputation", LVL_IMMORT, PC, NUMBER, "AdminFull"},
        {"qpoints", LVL_IMMORT, PC, NUMBER, "QuestorAdmin,WizardFull"},
		{"qbanned", LVL_IMMORT, PC, BINARY, "QuestorAdmin,AdminFull"},
        {"password", LVL_IMMORT, PC, MISC, "AdminFull"},
        {"email", LVL_IMMORT, PC, MISC, "AdminFull"},
        {"banned", LVL_IMMORT, PC, BINARY, "AdminFull"},
        {"\n", 0, BOTH, MISC, ""} };
	char *name, *field;
	int i, l, value = 0;
	Account *account;
	bool on = false, off = false;
	
    name = tmp_getword(&argument);
    field = tmp_getword(&argument);

    if (!*name || !*field) {
        send_to_char(ch, "Usage: aset <victim> <field> <value>\r\n");
        vector <string> cmdlist;
        for (i = 0; fields[i].level != 0; i++) {
            if (!Security::canAccess(ch, fields[i]))
                continue;
            cmdlist.push_back(fields[i].cmd);
        }
        sort(cmdlist.begin(), cmdlist.end());

        for (i = 0, l = 1; i < (int)(cmdlist.size()); i++) {
            send_to_char(ch, "%12s%s", cmdlist[i].c_str(),
                ((l++) % 5) ? " " : "\n");
        }

        if ((l - 1) % 5)
            send_to_char(ch, "\r\n");
        return;
    }

	if (*name == '.')
		account = Account::retrieve(playerIndex.getAccountID(name + 1));
	else if (is_number(name))
		account = Account::retrieve(atoi(name));
	else
		account = Account::retrieve(name);
	
	if (!account) {
		send_to_char(ch, "There is no such account.\r\n");
		return;
	}

    for (l = 0; *(fields[l].cmd) != '\n'; l++) {
        if (!strncmp(field, fields[l].cmd, strlen(field)))
            break;
    }

    if (GET_LEVEL(ch) < fields[l].level && subcmd != SCMD_TESTER_SET ) {
        send_to_char(ch, "You are not able to perform this godly deed!\r\n");
        return;
    }

    if (!Security::canAccess(ch, fields[l]) && subcmd != SCMD_TESTER_SET) {
        send_to_char(ch, "You do not have that power.\r\n");
        return;
    }

    if (fields[l].type == BINARY) {
        if (!strcmp(argument, "on") || !strcmp(argument, "yes"))
            on = 1;
        else if (!strcmp(argument, "off") || !strcmp(argument, "no"))
            off = 1;
        if (!(on || off)) {
            send_to_char(ch, "Value must be on or off.\r\n");
            return;
        }
    } else if (fields[l].type == NUMBER) {
        value = atoi(argument);
    }

    strcpy(buf, "Okay.");        /* can't use OK macro here 'cause of \r\n */


    switch (l) {
    case 0:
		account->set_past_bank(value); break;
	case 1:
		account->set_future_bank(value); break;
	case 2:
		account->set_reputation(value); break;
	case 3:
		account->set_quest_points(value); break;
	case 4:
	    account->set_quest_banned(on); break;
	case 5:
		account->set_password(argument);
		sprintf(buf, "Password for account %s[%d] has been set.",
			account->get_name(), account->get_idnum());
        slog("(GC) %s set password for account %s[%d]",
             GET_NAME(ch),
             account->get_name(),
             account->get_idnum());
		break;
    case 6:
        account->set_email_addr(argument);
        sprintf(buf, "Email for account %s[%d] has been set to <%s>",
                account->get_name(),
                account->get_idnum(),
                account->get_email_addr());
        slog("(GC) %s set email for account %s[%d] to %s",
             GET_NAME(ch),
             account->get_name(),
             account->get_idnum(),
             account->get_email_addr());
        break;
    case 7:
        account->set_banned(on); break;
    default:
        sprintf(buf, "Can't set that!");
        break;
    }

    if (fields[l].type == BINARY) {
        slog("(GC) %s set %s for account %s[%d] to %s",
             GET_NAME(ch),
             fields[l].cmd,
             account->get_name(),
             account->get_idnum(),
             ONOFF(on));
        send_to_char(ch, "%s %s for %s.\r\n", fields[l].cmd, ONOFF(on),
			account->get_name());
    } else if (fields[l].type == NUMBER) {
        slog("(GC) %s set %s for account %s[%d] to %d",
             GET_NAME(ch),
             fields[l].cmd,
             account->get_name(),
             account->get_idnum(),
             value);
        send_to_char(ch, "%s's %s set to %d.\r\n", account->get_name(),
            fields[l].cmd, value);
    } else
        send_to_char(ch, "%s\r\n", buf);
}


ACMD(do_syslog)
{
    int tp;

    one_argument(argument, arg);

    if (!*arg) {
        tp = ((PRF_FLAGGED(ch, PRF_LOG1) ? 1 : 0) +
            (PRF_FLAGGED(ch, PRF_LOG2) ? 2 : 0));
        send_to_char(ch, "Your syslog is currently %s.\r\n", logtypes[tp]);
        return;
    }
    if (((tp = search_block(arg, logtypes, FALSE)) == -1)) {
        send_to_char(ch, "Usage: syslog { Off | Brief | Normal | Complete }\r\n");
        return;
    }
    REMOVE_BIT(PRF_FLAGS(ch), PRF_LOG1 | PRF_LOG2);
    SET_BIT(PRF_FLAGS(ch), (PRF_LOG1 * (tp & 1)) | (PRF_LOG2 * (tp & 2) >> 1));

    send_to_char(ch, "Your syslog is now %s.\r\n", logtypes[tp]);
}


ACMD(do_rlist)
{
    struct room_data *room;
    struct zone_data *zone;
    bool has_desc, stop = FALSE;
	char *read_pt;

    int first, last, found = 0;

    two_arguments(argument, buf, buf2);

    if (!*buf) {
        first = ch->in_room->zone->number * 100;
        last = ch->in_room->zone->top;
    } else if (!*buf2) {
        send_to_char(ch, "Usage: rlist <begining number> <ending number>\r\n");
        return;
    } else {
        first = atoi(buf);
        last = atoi(buf2);
    }

    if ((first < 0) || (first > 99999) || (last < 0) || (last > 99999)) {
        send_to_char(ch, "Values must be between 0 and 99999.\r\n");
        return;
    }

    if (first >= last) {
        send_to_char(ch, "Second value must be greater than first.\r\n");
        return;
    }
	acc_string_clear();
    for (zone = zone_table; zone && !stop; zone = zone->next)
        for (room = zone->world; room; room = room->next) {
            if (room->number > last) {
                stop = TRUE;
                break;
            }
            if (room->number >= first) {
				has_desc = false;
				if (room->description) {
					for (read_pt = room->description;*read_pt;read_pt++)
						if (!isspace(*read_pt)) {
							has_desc = true;
							break;
						}	
				}
                acc_sprintf("%5d. %s[%s%5d%s]%s %s%-30s%s %s%s\r\n",
							++found,
							CCGRN(ch, C_NRM),
							CCNRM(ch, C_NRM),
							room->number,
							CCGRN(ch, C_NRM),
							CCNRM(ch, C_NRM),
							CCCYN(ch, C_NRM),
							room->name,
							CCNRM(ch, C_NRM),
							has_desc ? "" : "(nodesc)",
							room->prog ? "(prog)" : "");
			}
        }

    if (!found)
        send_to_char(ch, "No rooms were found in those parameters.\r\n");
    else
	  page_string(ch->desc, acc_get_string());
}


ACMD(do_xlist)
{
    struct special_search_data *srch = NULL;
    struct room_data *room = NULL;
    struct zone_data *zone = NULL, *zn = NULL;
    int count = 0, srch_type = -1;
    bool overflow = 0, found = 0;
	char *arg;

	arg = tmp_getword(&argument);
    while (*arg) {
        if (is_abbrev(arg, "zone")) {
		  arg = tmp_getword(&argument);
            if (!*arg) {
                send_to_char(ch, "No zone specified.\r\n");
                return;
            }
            if (!(zn = real_zone(atoi(arg)))) {
                send_to_char(ch, "No such zone ( %s ).\r\n", arg);
                return;
            }
        }
        if (is_abbrev(arg, "type")) {
		  arg = tmp_getword(&argument);
            if (!*arg) {
                send_to_char(ch, "No type specified.\r\n");
                return;
            }
            if ((srch_type = search_block(arg, search_commands, FALSE)) < 0) {
                send_to_char(ch, "No such search type ( %s ).\r\n", arg);
                return;
            }
        }
        if (is_abbrev(arg, "help")) {
            send_to_char(ch, "Usage: xlist [type <type>] [zone <zone>]\r\n");
			return;
        }
		arg = tmp_getword(&argument);
    }

    // If there's no zone argument, set it to the current zone.
    if (!zn) {
        zone = ch->in_room->zone;
    } else {
        zone = zn;
    }
    // can this person actually look at this zones searches?
    if (!OLCIMP(ch)
        && !(zone->owner_idnum == GET_IDNUM(ch))
        && !(zone->co_owner_idnum == GET_IDNUM(ch))
        && !OLCGOD(ch)
        && (GET_LEVEL(ch) < LVL_FORCE)) {
        send_to_char(ch, "You aren't godly enough to do that here.\r\n");
        return;
    }

	acc_string_clear();
	acc_strcat("Searches: \r\n", NULL);

    for (room = zone->world, found = FALSE; room && !overflow;
        found = FALSE, room = room->next) {

        for (srch = room->search, count = 0; srch && !overflow;
            srch = srch->next, count++) {

            if (srch_type >= 0 && srch_type != srch->command)
                continue;

            if (!found) {
                acc_sprintf("Room [%s%5d%s]:\n", CCCYN(ch, C_NRM),
                    room->number, CCNRM(ch, C_NRM));
                found = TRUE;
            }

            print_search_data_to_buf(ch, room, srch, buf);
			acc_strcat(buf, NULL);
        }
    }
    page_string(ch->desc, acc_get_string());
}

ACMD(do_mlist)
{
    int first, last, found = 0;
    two_arguments(argument, buf, buf2);

    if (!*buf) {
        first = ch->in_room->zone->number * 100;
        last = ch->in_room->zone->top;
    } else if (!*buf2) {
        send_to_char(ch, "Usage: mlist <begining number> <ending number>\r\n");
        return;
    } else {
        first = atoi(buf);
        last = atoi(buf2);
    }

    if ((first < 0) || (first > 99999) || (last < 0) || (last > 99999)) {
        send_to_char(ch, "Values must be between 0 and 99999.\r\n");
        return;
    }

    if (first >= last) {
        send_to_char(ch, "Second value must be greater than first.\r\n");
        return;
    }

	acc_string_clear();
    MobileMap::iterator mit = mobilePrototypes.begin();
    Creature *mob;
    for (;
        mit != mobilePrototypes.end()
        && ((mit->second)->mob_specials.shared->vnum <= last); ++mit) {
        mob = mit->second;
        if (mob->mob_specials.shared->vnum >= first)
		  acc_sprintf("%5d. %s[%s%5d%s]%s %-40s%s  [%2d] <%3d> %s%s\r\n",
					  ++found,
					  CCGRN(ch, C_NRM),
					  CCNRM(ch, C_NRM),
					  mob->mob_specials.shared->vnum,
					  CCGRN(ch, C_NRM),
					  CCYEL(ch, C_NRM),
					  mob->player.short_descr,
					  CCNRM(ch, C_NRM),
					  mob->player.level,
					  MOB_SHARED(mob)->number,
					  MOB2_FLAGGED((mob), MOB2_UNAPPROVED) ? "(!ap)" : "",
					  GET_MOB_PROG(mob) ? "(prog)" : "");
    }

    if (!found)
        send_to_char(ch, "No mobiles were found in those parameters.\r\n");
    else
	  page_string(ch->desc, acc_get_string());
}


ACMD(do_olist)
{
    struct obj_data *obj = NULL;
    int first, last, found = 0;

    two_arguments(argument, buf, buf2);

    if (!*buf) {
        first = ch->in_room->zone->number * 100;
        last = ch->in_room->zone->top;
    } else if (!*buf2) {
        send_to_char(ch, "Usage: olist <begining number> <ending number>\r\n");
        return;
    } else {
        first = atoi(buf);
        last = atoi(buf2);
    }

    if ((first < 0) || (first > 99999) || (last < 0) || (last > 99999)) {
        send_to_char(ch, "Values must be between 0 and 99999.\r\n");
        return;
    }

    if (first >= last) {
        send_to_char(ch, "Second value must be greater than first.\r\n");
        return;
    }

	acc_string_clear();

    ObjectMap::iterator oi;
    for (oi = objectPrototypes.lower_bound(first);
         oi != objectPrototypes.upper_bound(last); ++oi) {
        obj = oi->second;
        acc_sprintf("%5d. %s[%s%5d%s]%s %-36s%s %s %s\r\n", ++found,
            CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), obj->shared->vnum,
            CCGRN(ch, C_NRM), CCGRN(ch, C_NRM),
            obj->name, CCNRM(ch, C_NRM),
            !P_OBJ_APPROVED(obj) ? "(!aprvd)" : "", (!(obj->line_desc)
                || !(*(obj->line_desc))) ? "(nodesc)" : "");
    }

    if (!found)
        send_to_char(ch, "No objects were found in those parameters.\r\n");
    else
	  page_string(ch->desc, acc_get_string());
}


ACMD(do_rename)
{
    struct obj_data *obj;
    struct Creature *vict = NULL;
    char new_desc[MAX_INPUT_LENGTH], logbuf[MAX_STRING_LENGTH];

    half_chop(argument, arg, new_desc);

    if (!new_desc) {
        send_to_char(ch, "What do you want to call it?\r\b");
        return;
    }
    if (!arg) {
        send_to_char(ch, "Rename usage: rename <target> <string>\r\n");
        return;
    }
    if (strlen(new_desc) >= MAX_INPUT_LENGTH) {
        send_to_char(ch, "Desript too long.\r\n");
        return;
    }

    if (!(obj = get_obj_in_list_all(ch, arg, ch->carrying))) {
        vict = get_char_room_vis(ch, arg);
        if (!vict) {
            if (!(obj = get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
                send_to_char(ch, "No such object or mobile around.\r\n");
                return;
            }
        } else if (!IS_NPC(vict)) {
            send_to_char(ch, "You can do this only with NPC's.\r\n");
            return;
        }
    }

    if (obj) {
        sprintf(logbuf, "%s has renamed %s '%s'.", GET_NAME(ch),
            obj->name, new_desc);
        obj->name = str_dup(new_desc);
        sprintf(buf, "%s has been left here.", new_desc);
        strcpy(buf, CAP(buf));
        obj->line_desc = str_dup(buf);
    } else if (vict) {
        sprintf(logbuf, "%s has renamed %s '%s'.", GET_NAME(ch),
            GET_NAME(vict), new_desc);
        vict->player.short_descr = str_dup(new_desc);
        if (vict->getPosition() == POS_FLYING)
            sprintf(buf, "%s is hovering here.", new_desc);
        else
            sprintf(buf, "%s is standing here.", new_desc);
        strcpy(buf, CAP(buf));
        vict->player.long_descr = str_dup(buf);
    }
    send_to_char(ch, "Okay, you do it.\r\n");
    mudlog(MAX(LVL_ETERNAL, GET_INVIS_LVL(ch)), CMP, true, "%s", logbuf);
    return;
}

ACMD(do_addname)
{
    struct obj_data *obj;
    struct Creature *vict = NULL;
    char new_name[MAX_INPUT_LENGTH];
    Tokenizer args(argument);

    if(!args.hasNext()) {
        send_to_char(ch, "Addname usage: addname <target> <strings>\r\n");
        return;
    }
    
    args.next(arg);
    
    if(!args.hasNext()) {
        send_to_char(ch, "What name keywords do you want to add?\r\b");
        return;
    }
    
    while(args.next(new_name)) {

        if (!(obj = get_obj_in_list_all(ch, arg, ch->carrying))) {
            vict = get_char_room_vis(ch, arg);
            if (!(vict)) {
                if (!(obj = get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
                    send_to_char(ch, "No such object or mobile around.\r\n");
                    return;
                }
            } else if (!IS_NPC(vict)) {
                send_to_char(ch, "You can do this only with NPC's.\r\n");
                return;
            }
        }
        
        if (strlen(new_name) >= MAX_INPUT_LENGTH) {
            send_to_char(ch, "Name: \'%s\' too long.\r\n", new_name);
            continue;
        }
        //check to see if the name is already in the list
        
        
        if (obj) {
            
            if(strstr(obj->aliases, new_name) != NULL) {
                send_to_char(ch, "Name: \'%s\' is already an alias.\r\n", new_name);
                continue;
            }
            snprintf(buf, EXDSCR_LENGTH, "%s %s", obj->aliases, new_name);
            obj->aliases = str_dup(buf);
        } else if (vict) {
            if(strstr(vict->player.name, new_name) != NULL) {
                send_to_char(ch, "Name: \'%s\' is already an alias.\r\n", new_name);
                continue;
            }
            
            sprintf(buf, "%s ", new_name);
            strcat(buf, vict->player.name);
            vict->player.name = str_dup(buf);
        }
    }
    send_to_char(ch, "Okay, you do it.\r\n");
    return;
}

ACMD(do_addpos)
{
    struct obj_data *obj;
    char new_pos[MAX_INPUT_LENGTH];
    int bit = -1;
    static const char *keywords[] = {
        "take",                    /*   0   */
        "finger",
        "neck",
        "body",
        "head",
        "legs",                    /*   5   */
        "feet",
        "hands",
        "arms",
        "shield",
        "about",                /*   10  */
        "waist",
        "wrist",
        "wield",
        "hold",
        "crotch",                /*  15   */
        "eyes",
        "back",
        "belt",
        "face",
        "ear",                    /*   20   */
        "ass",
        "\n"
    };

    two_arguments(argument, arg, new_pos);

    if (!*new_pos) {
        send_to_char(ch, "What new position?\r\n");
        return;
    }
    if (!*arg) {
        send_to_char(ch, "Addpos usage: addpos <target> <new pos>\r\n");
        send_to_char(ch, "Valid positions are:\r\n"
            "take finger neck body about legs feet hands arms shield waist ass\r\n"
            "wrist wield hold crotch eyes back belt face ear\r\n");
        return;
    }

    obj = get_obj_in_list_all(ch, arg, ch->carrying);

    if (!obj) {
        send_to_char(ch, "You carry no such thing.\r\n");
        return;
    }
    bit = search_block(new_pos, keywords, FALSE);
    if (bit < 0) {
        send_to_char(ch, "That is not a valid position!\r\n");
        return;
    }

    bit = (1 << bit);

    TOGGLE_BIT(obj->obj_flags.wear_flags, bit);
    send_to_char(ch, "Bit %s for %s now %s.\r\n", new_pos, obj->name,
        IS_SET(obj->obj_flags.wear_flags, bit) ? "ON" : "OFF");
    return;
}

ACMD(do_nolocate)
{
    struct obj_data *obj;

    one_argument(argument, arg);

    if (!*arg) {
        send_to_char(ch, "Nolocate usage: nolocate <target>\r\n");
        return;
    }

    obj = get_obj_in_list_all(ch, arg, ch->carrying);

    if (!obj) {
        send_to_char(ch, "You carry no such thing.\r\n");
        return;
    }
    TOGGLE_BIT(obj->obj_flags.extra2_flags, ITEM2_NOLOCATE);
    send_to_char(ch, "NoLocate is now %s for %s.\r\n",
        IS_SET(obj->obj_flags.extra2_flags, ITEM2_NOLOCATE) ? "ON" : "OFF",
        obj->name);
    return;
}

ACMD(do_mudwipe)
{
    struct obj_data *obj, *obj_tmp;
    struct Creature *mob;
    struct zone_data *zone = NULL;
    struct room_data *room = NULL;
    int mode, i;
    skip_spaces(&argument);

    if (!strncasecmp(argument, "tempus clean", 12)) {
        mode = 0;
        sprintf(buf, "Mud WIPE CLEAN called by %s.", GET_NAME(ch));
    } else if (!strncasecmp(argument, "tempus objects", 14)) {
        mode = 1;
        sprintf(buf, "Mud WIPE objects called by %s.", GET_NAME(ch));
    } else if (!strncasecmp(argument, "tempus mobiles", 14)) {
        mode = 2;
        sprintf(buf, "Mud WIPE mobiles called by %s.", GET_NAME(ch));
    } else if (!strncasecmp(argument, "tempus fullmobs", 15)) {
        mode = 3;
        sprintf(buf, "Mud WIPE fullmobs called by %s.", GET_NAME(ch));
    } else if (!strncasecmp(argument, "tempus trails", 13)) {
        mode = 4;
        sprintf(buf, "Mud WIPE trails called by %s.", GET_NAME(ch));
    } else {
        send_to_char(ch, "Improper syntax.  Options are:\r\n"
            "objects, mobiles, fullmobs, or clean.\r\n");
        return;
    }
    mudlog(GET_INVIS_LVL(ch), BRF, true, "%s", buf);

    if (mode == 4) {
        retire_trails();
        send_to_char(ch, "Done.  trails retired\r\n");
    }
    if (mode == 3) {
        CreatureList::iterator cit = characterList.begin();
        for (; cit != characterList.end(); ++cit) {
            mob = *cit;
            if (!IS_NPC(mob))
                continue;
            for (obj = mob->carrying; obj; obj = obj_tmp) {
                obj_tmp = obj->next_content;
                extract_obj(obj);
            }
            for (i = 0; i < NUM_WEARS; i++)
                if (GET_EQ(mob, i))
                    extract_obj(GET_EQ(mob, i));
        }
        send_to_char(ch, "DONE.  Mobiles cleared of objects.\r\n");
    }
    if (mode == 3 || mode == 2 || mode == 0) {
        CreatureList::iterator mit = characterList.begin();
        for (; mit != characterList.end(); ++mit) {
            if (IS_NPC(*mit)) {
                (*mit)->purge(true);
            }
        }
        send_to_char(ch, "DONE.  Mud cleaned of all mobiles.\r\n");
    }
    if (mode == 0 || mode == 1) {
        for (zone = zone_table; zone; zone = zone->next)
            for (room = zone->world; room; room = room->next) {
                if (ROOM_FLAGGED(room,
                        ROOM_GODROOM | ROOM_CLAN_HOUSE | ROOM_HOUSE))
                    continue;
                for (obj = room->contents; obj; obj = obj_tmp) {
                    obj_tmp = obj->next_content;
                    extract_obj(obj);
                }
            }
        send_to_char(ch, "DONE.  Mud cleaned of in_room contents.\r\n");
    }
}

ACMD(do_zonepurge)
{
    struct obj_data *obj = NULL, *next_obj = NULL;
    struct zone_data *zone = NULL;
    int j, mob_count = 0, obj_count = 0;
    struct room_data *rm = NULL;

    skip_spaces(&argument);
    if (!*argument)
        zone = ch->in_room->zone;
    else if (*argument == '.')
        zone = ch->in_room->zone;
    else if (!is_number(argument)) {
        send_to_char(ch, "You must specify the zone with its vnum number.\r\n");
        return;
    } else {
        j = atoi(argument);
        for (zone = zone_table; zone; zone = zone->next)
            if (zone->number == j)
                break;
    }

    if (zone) {
        for (rm = zone->world; rm; rm = rm->next) {
            if (rm->number < (zone->number * 100) || rm->number > zone->top)
                continue;
            if (ROOM_FLAGGED(rm, ROOM_GODROOM) || ROOM_FLAGGED(rm, ROOM_HOUSE))
                continue;


            CreatureList::iterator it = rm->people.begin();
            for (; it != rm->people.end(); ++it) {
                if (IS_MOB((*it))) {
                    (*it)->purge(true);
                    mob_count++;
                } else {
                    send_to_char(*it, "You feel a rush of heat wash over you!\r\n");
                }
            }

            for (obj = rm->contents; obj; obj = next_obj) {
                next_obj = obj->next_content;
                extract_obj(obj);
                obj_count++;
            }

        }

        send_to_char(ch, "Zone %d cleared of %d mobiles.\r\n", zone->number,
            mob_count);
        send_to_char(ch, "Zone %d cleared of %d objects.\r\n", zone->number,
            obj_count);

        sprintf(buf, "(GC) %s %spurged zone %d (%s)", GET_NAME(ch),
            subcmd == SCMD_OLC ? "olc-" : "", zone->number, zone->name);
        if (subcmd != SCMD_OLC) {
            mudlog(GET_INVIS_LVL(ch),
                subcmd == SCMD_OLC ? CMP : NRM,
                true, "%s", buf);
        } else {
            slog("%s", buf);
        }

    } else {
        send_to_char(ch, "Invalid zone number.\r\n");
    }
}


ACMD(do_searchfor)
{
    struct Creature *mob = NULL;
    struct obj_data *obj = NULL;
    byte mob_found = FALSE, obj_found = FALSE;
    CreatureList::iterator cit = characterList.begin();
    for (; cit != characterList.end(); ++cit) {
        mob = *cit;
        if (!mob->in_room) {
            mob_found = TRUE;
            send_to_char(ch, "Char out of room: %s,  Master is: %s.\r\n",
                GET_NAME(mob), mob->master ? GET_NAME(mob->master) : "N");
        }
    }
    for (obj = object_list; obj; obj = obj->next) {
        if (!obj->in_room && !obj->carried_by && !obj->worn_by && !obj->in_obj) {
            obj_found = TRUE;

#ifdef TRACK_OBJS
            sprintf(buf, "Object: %s [%s] %s", obj->name,
                obj->obj_flags.tracker.string,
                ctime(&obj->obj_flags.tracker.lost_time));
#else
            send_to_char(ch, "Object: %s\r\n", obj->name);
#endif
        }
    }
    if (!mob_found)
        send_to_char(ch, "No mobs out of room.\r\n");
    if (!obj_found)
        send_to_char(ch, "No objects out of room.\r\n");
}

#define OSET_FORMAT "Usage: oset <object> <parameter> <value>.\r\n"

ACMD(do_oset)
{
    struct obj_data *obj = NULL;
    struct Creature *vict = NULL;
    int where_worn = 0;
    char arg1[MAX_INPUT_LENGTH];
    int equip_mode = EQUIP_WORN;

    if (!*argument) {
        send_to_char(ch, OSET_FORMAT);
        return;
    }
    argument = one_argument(argument, arg1);

    if (!(obj = get_obj_vis(ch, arg1))) {
        send_to_char(ch, "No such object around.\r\n");
        return;
    }

    if ((vict = obj->worn_by)) {
        where_worn = obj->worn_on;
        if (obj == GET_EQ(obj->worn_by, obj->worn_on))
            equip_mode = EQUIP_WORN;
        else if (obj == GET_IMPLANT(obj->worn_by, obj->worn_on))
            equip_mode = EQUIP_IMPLANT;
        else if (obj == GET_TATTOO(obj->worn_by, obj->worn_on))
            equip_mode = EQUIP_TATTOO;
        unequip_char(vict, where_worn, equip_mode);
    }
    perform_oset(ch, obj, argument, NORMAL_OSET);
    if (vict) {
        if (equip_char(vict, obj, where_worn, equip_mode))
            return;
        if (!IS_NPC(vict))
            vict->saveToXML();
    }

}

static const char *show_mob_keys[] = {
    "hitroll",
    "damroll",
    "gold",
    "flags",
    "extreme",
    "special",
    "race",
    "class",
    "\n"
};

#define NUM_SHOW_MOB 8

void
do_show_mobiles(struct Creature *ch, char *value, char *arg)
{

    int i, j, k, l, command;
    struct Creature *mob = NULL;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    MobileMap::iterator mit;
    if (!*value || (command = search_block(value, show_mob_keys, 0)) < 0) {
        send_to_char(ch, 
            "Show mobiles:  utility to search the mobile prototype list.\r\n"
            "Usage: show mob <option> <argument>\r\n" "Options:\r\n");
        for (i = 0; i < NUM_SHOW_MOB; i++) {
            send_to_char(ch, show_mob_keys[i]);
            send_to_char(ch, "\r\n");
        }
        return;
    }

    switch (command) {
    case 0:                    /* hitroll */
        if (!*arg) {
            send_to_char(ch, "What hitroll minimum?\r\n");
            return;
        }
        k = atoi(arg);

        sprintf(buf, "Mobs with hitroll greater than %d:r\n", k);
        mit = mobilePrototypes.begin();
        for (i = 0;
            (mit != mobilePrototypes.end()
                && strlen(buf) < (MAX_STRING_LENGTH - 128)); ++mit) {
            mob = mit->second;
            if (GET_HITROLL(mob) >= k)
                sprintf(buf,
                    "%s %3d. [%5d] %-30s (%2d) %2d\r\n",
                    buf, ++i, GET_MOB_VNUM(mob), GET_NAME(mob), GET_LEVEL(mob),
                    GET_HITROLL(mob));
        }
        page_string(ch->desc, buf);
        break;
    case 2:                    /* gold */
        arg = two_arguments(arg, arg1, arg2);
        if (!*arg1) {
            send_to_char(ch, "What gold minimum?\r\n");
            return;
        }
        k = atoi(arg1);

        if (*arg2 && !strcmp(arg2, "noshop"))
            j = 1;
        else
            j = 0;

        sprintf(buf, "Mobs with gold greater than %d:r\n", k);
        mit = mobilePrototypes.begin();
        for (i = 0;
            (mit != mobilePrototypes.end()
                && strlen(buf) < (MAX_STRING_LENGTH - 128)); ++mit) {
            mob = mit->second;

            if (MOB2_FLAGGED(mob, MOB2_UNAPPROVED))
                continue;
            if (GET_GOLD(mob) >= k &&
                (!j || vendor != mob->mob_specials.shared->func))
                sprintf(buf,
                    "%s %3d. [%5d] %-30s (%2d) %2d\r\n",
                    buf, ++i, GET_MOB_VNUM(mob), GET_NAME(mob), GET_LEVEL(mob),
                    GET_GOLD(mob));
        }
        page_string(ch->desc, buf);
        break;
    case 3:                    /* flags */

        if (!*arg) {
            send_to_char(ch, "Show mobiles with what flag?\r\n");
            return;
        }

        skip_spaces(&arg);

        if ((!(i = 1) || (j = search_block(arg, action_bits_desc, 0)) < 0) &&
            (!(i = 2) || (j = search_block(arg, action2_bits_desc, 0)) < 0)) {
            send_to_char(ch, "There is no mobile flag '%s'.\r\n", arg);
            return;
        }

        sprintf(buf, "Mobiles with flag %s%s%s:\r\n", CCYEL(ch, C_NRM),
            (i == 1) ? action_bits_desc[j] : action2_bits_desc[j],
            CCNRM(ch, C_NRM));

        mit = mobilePrototypes.begin();
        for (k = 0; mit != mobilePrototypes.end(); ++mit) {
            mob = mit->second;

            if ((i == 1 && MOB_FLAGGED(mob, (1 << j))) ||
                (i == 2 && MOB2_FLAGGED(mob, (1 << j)))) {
                sprintf(buf2, "%3d. %s[%s%5d%s]%s %40s%s%s\r\n", ++k,
                    CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_MOB_VNUM(mob),
                    CCGRN(ch, C_NRM), CCYEL(ch, C_NRM),
                    GET_NAME(mob), CCNRM(ch, C_NRM),
                    MOB2_FLAGGED(mob, MOB2_UNAPPROVED) ? " (!appr)" : "");
                if (strlen(buf) + strlen(buf2) > MAX_STRING_LENGTH - 128) {
                    strcat(buf, "**OVERFLOW**\r\n");
                    break;
                }
                strcat(buf, buf2);
            }
        }

        page_string(ch->desc, buf);
        return;
    case 4:                    /* extreme gold */

        if (GET_LEVEL(ch) < LVL_GOD)
            break;

        arg = two_arguments(arg, arg1, arg2);
        if (!*arg1) {
            send_to_char(ch, 
                "Usage: show mob extreme <gold | cash> [multiplier]\r\n");
            return;
        }

        if (is_abbrev(arg1, "gold"))
            i = 0;
        else if (is_abbrev(arg1, "cash"))
            i = 1;
        else {
            sprintf(buf,
                "You must specify gold or cash.  '%s' is not valid.\r\n",
                arg1);
            send_to_char(ch, "%s", buf);
            return;
        }

        if (!*arg2)
            l = 1000;
        else
            l = atoi(arg2);

        sprintf(buf, "Mobs with extreme %s >= (level * %d):\r\n",
            i ? "cash" : "gold", l);


        mit = mobilePrototypes.begin();
        for (k = 0; mit != mobilePrototypes.end(); ++mit) {
            mob = mit->second;
            if (MOB2_FLAGGED(mob, MOB2_UNAPPROVED))
                continue;

            if (i && (GET_CASH(mob) > (GET_LEVEL(mob) * l))) {
                sprintf(buf, "%s %3d. [%5d] %30s (%2d) (%6d)\r\n",
                    buf, ++k, GET_MOB_VNUM(mob), GET_NAME(mob),
                    GET_LEVEL(mob), GET_GOLD(mob));

            } else if (!i && (GET_GOLD(mob) > (GET_LEVEL(mob) * l))) {

                sprintf(buf, "%s %3d. [%5d] %30s (%2d) (%6d)\r\n",
                    buf, ++k, GET_MOB_VNUM(mob), GET_NAME(mob),
                    GET_LEVEL(mob), GET_GOLD(mob));
            }

        }
        page_string(ch->desc, buf);
        break;

    case 5:                    // special

        if (!*arg) {
            send_to_char(ch, "Show mobiles with what special?\r\n");
            return;
        }

        skip_spaces(&arg);

        if ((i = find_spec_index_arg(arg)) < 0) {
            send_to_char(ch, "Type show specials for a valid list.\r\n");
            return;
        }

        sprintf(buf, "Mobiles with special %s%s%s:\r\n", CCYEL(ch, C_NRM),
            spec_list[i].tag, CCNRM(ch, C_NRM));

        mit = mobilePrototypes.begin();
        for (j = 0; mit != mobilePrototypes.end(); ++mit) {
            mob = mit->second;
            if (spec_list[i].func == GET_MOB_SPEC(mob)) {
                sprintf(buf2, "%3d. %s[%s%5d%s]%s %s%s\r\n", ++j,
                    CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_MOB_VNUM(mob),
                    CCGRN(ch, C_NRM), CCYEL(ch, C_NRM),
                    GET_NAME(mob), CCNRM(ch, C_NRM));
                if (strlen(buf) + strlen(buf2) > MAX_STRING_LENGTH - 128) {
                    strcat(buf, "**OVERFLOW**\r\n");
                    break;
                }
                strcat(buf, buf2);
            }
        }
        page_string(ch->desc, buf);
        return;



        break;
    case 6:  // race
        if( !*arg){
            send_to_char(ch, "Show mobiles with what race?\r\n");
            return;
                        
        }
        skip_spaces(&arg);

        if ((i = search_block(arg, player_race, FALSE)) < 0) {
            send_to_char(ch, "Type olchelp race for a valid list.\r\n");
            return;
        }

        sprintf(buf, "Mobiles with race %s%s%s:\r\n", CCYEL(ch, C_NRM),
            player_race[i], CCNRM(ch, C_NRM));

        mit = mobilePrototypes.begin();
        for (j = 0; mit != mobilePrototypes.end(); ++mit) {
            mob = mit->second;
            if ( i == GET_RACE(mob)) {
                sprintf(buf2, "%3d. %s[%s%5d%s]%s %s%s\r\n", ++j,
                    CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_MOB_VNUM(mob),
                    CCGRN(ch, C_NRM), CCYEL(ch, C_NRM),
                    GET_NAME(mob), CCNRM(ch, C_NRM));
                if (strlen(buf) + strlen(buf2) > MAX_STRING_LENGTH - 128) {
                    strcat(buf, "**OVERFLOW**\r\n");
                    break;
                }
                strcat(buf, buf2);
            }
        }
        page_string(ch->desc, buf);
        return;
    case 7:  // class
        if( !*arg){
            send_to_char(ch, "Show mobiles with what class?\r\n");
            return;
                        
        }
        skip_spaces(&arg);

        if ((i = search_block(arg, class_names, FALSE)) < 0) {
            send_to_char(ch, "Type olchelp class for a valid list.\r\n");
            return;
        }

        sprintf(buf, "Mobiles with class %s%s%s:\r\n", CCYEL(ch, C_NRM),
            class_names[i], CCNRM(ch, C_NRM));

        mit = mobilePrototypes.begin();
        for (j = 0; mit != mobilePrototypes.end(); ++mit) {
            mob = mit->second;
            if ( i == GET_CLASS(mob) || mob->player.remort_char_class == i ) {
                sprintf(buf2, "%3d. %s[%s%5d%s]%s %s%s\r\n", ++j,
                    CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_MOB_VNUM(mob),
                    CCGRN(ch, C_NRM), CCYEL(ch, C_NRM),
                    GET_NAME(mob), CCNRM(ch, C_NRM));
                if (strlen(buf) + strlen(buf2) > MAX_STRING_LENGTH - 128) {
                    strcat(buf, "**OVERFLOW**\r\n");
                    break;
                }
                strcat(buf, buf2);
            }
        }
        page_string(ch->desc, buf);
        return;
    default:
        send_to_char(ch, "DOH!!!!\r\n");
        break;
    }
}

ACMD(do_xlag)
{

    int raw = 0;
    struct Creature *vict = NULL;

    argument = two_arguments(argument, buf, buf2);

    if (!*buf || !*buf2) {
        send_to_char(ch, "Usage: xlag <victim> <0.10 seconds>\r\n");
        return;
    }
    if (!(vict = get_char_vis(ch, buf))) {
        send_to_char(ch, "No-one by that name here.\r\n");
        return;
    }
    if (!vict->desc) {
        send_to_char(ch, "Xlag is pointless on null descriptors.\r\n");
        return;
    }
    if ((raw = atoi(buf2)) <= 0) {
        send_to_char(ch, "Real funny xlag time, buddy.\r\n");
        return;
    }
    if (ch != vict && GET_LEVEL(vict) >= GET_LEVEL(ch)) {
        send_to_char(ch, "Nice try, sucker.\r\n");
        ch->desc->wait = raw;
        return;
    }

    vict->desc->wait = raw;
    send_to_char(ch, "%s lagged %d cycles.\r\n", GET_NAME(vict), raw);

}

ACMD(do_peace)
{
    bool found = 0;


    CreatureList::iterator it = ch->in_room->people.begin();
    for (; it != ch->in_room->people.end(); ++it) {
        found = 1;
        (*it)->removeAllCombat();
    }

    if (!found)
        send_to_char(ch, "No-one here is fighting!\r\n");
    else {
        send_to_char(ch, "You have brought peace to this room.\r\n");
        act("A feeling of peacefulness descends on the room.",
            FALSE, ch, 0, 0, TO_ROOM);
    }
}

ACMD(do_severtell)
{
    struct Creature *vict = NULL;

    skip_spaces(&argument);

    if (!*argument) {
        send_to_char(ch, "Sever reply of what character?\r\n");
        return;
    }

    if (!(vict = get_char_vis(ch, argument))) {
        send_to_char(ch, "Nobody by that name around here.\r\n");
        return;
    }

    if (GET_IDNUM(ch) != GET_LAST_TELL_FROM(vict) && GET_IDNUM(ch) != GET_LAST_TELL_TO(vict))
        send_to_char(ch, "You are not on that person's reply buffer, pal.\r\n");
    else {
        GET_LAST_TELL_FROM(vict) = -1;
        GET_LAST_TELL_TO(vict) = -1;
        send_to_char(ch, "Reply severed.\r\n");
    }
}

ACMD(do_qpreload)
{

    qp_reload();
    send_to_char(ch, "Reloading Quest Points.\r\n");
    mudlog(LVL_GRGOD, NRM, true, "(GC) %s has reloaded QP", GET_NAME(ch));

}

static const char* TESTER_UTIL_USAGE =
    "Options are:\r\n"        \
    "    advance <level>\r\n"   \
    "    unaffect\r\n"          \
    "    reroll\r\n"            \
    "    stat\r\n"              \
    "    goto\r\n"              \
    "    restore\r\n"           \
    "    class <char_class>\r\n"     \
    "    race <race>\r\n"       \
    "    remort <char_class>\r\n"    \
    "    maxhit <value>\r\n"    \
    "    maxmana <value>\r\n"   \
    "    maxmove <value>\r\n"   \
    "    nohassle\r\n"          \
    "    roomflags\r\n"         \
    "    align\r\n"             \
    "    generation\r\n"        \
    "    debug\r\n"                \
    "    str|con|int|wis|dex|cha <val>\r\n";

static const char *tester_cmds[] = {
  "advance",
  "unaffect",
  "reroll",
  "stat",
  "goto",
  "restore",          /* 5 */
  "class",
  "race",
  "remort",
  "maxhit",
  "maxmana",          /* 10 */
  "maxmove",
  "nohassle",
  "roomflags",
  "align",
  "generation",
  "debug",
  "str",
  "int",
  "wis",
  "con",
  "dex",
  "cha",
  "maxstat",
  "\n"
};

static const char* CODER_UTIL_USAGE = 
                    "Usage: coderutil <command> <args>\r\n"
                    "Commands: \r\n"
                    "      tick - forces a mud-wide tick to occur.\r\n"
					"      recalc - recalculates all mobs and saves.\r\n"
					"    cmdusage - shows commands and usage counts.\r\n"
					"  unusedcmds - shows unused commands.\r\n"
					"      verify - run tempus integrity check.\r\n"
					"       chaos - makes every mob berserk.\r\n"
					"    loadzone - load zone files for zone.\r\n"
                    ;

bool
load_single_zone(int zone_num)
{
    void discrete_load(FILE *fl, int mode);
    void load_zones(FILE * fl, char *zonename);
    void reset_zone(struct zone_data *zone);
    void assign_mobiles(void);
    void assign_objects(void);
    void assign_rooms(void);
    void assign_artisans(void);
    void compile_all_progs(void);
    void renum_world(void);
    void renum_zone_table(void);
    
    FILE *db_file;
    zone_data *zone;

    db_file = fopen(tmp_sprintf("world/zon/%d.zon", zone_num), "r");
    if (db_file) {
        load_zones(db_file, tmp_sprintf("zon/%d.zon", zone_num));
        fclose(db_file);
    }
    db_file = fopen(tmp_sprintf("world/wld/%d.wld", zone_num), "r");
    if (db_file) {
        discrete_load(db_file, DB_BOOT_WLD);
        renum_world();
        fclose(db_file);
    }
    db_file = fopen(tmp_sprintf("world/mob/%d.mob", zone_num), "r");
    if (db_file) {
        discrete_load(db_file, DB_BOOT_MOB);
        fclose(db_file);
    }
    db_file = fopen(tmp_sprintf("world/obj/%d.obj", zone_num), "r");
    if (db_file) {
        discrete_load(db_file, DB_BOOT_OBJ);
        fclose(db_file);
    }
    for (zone = zone_table; zone; zone = zone->next)
        if (zone->number == zone_num)
            break;
    if (zone) {
        reset_zone(zone);
        renum_zone_table();
        compile_all_progs();
        assign_mobiles();
        assign_objects();
        assign_rooms();
        assign_artisans();
        return true;
    } else {
        return false;
    }
}
ACMD(do_coderutil)
{
    Tokenizer tokens(argument);
	int idx, cmd_num, len = 0;
    char token[MAX_INPUT_LENGTH];

    if(!tokens.next(token)) {
        send_to_char( ch, CODER_UTIL_USAGE );
        return;
    }
    if (strcmp(token, "tick") == 0) {
        point_update();
		send_to_char(ch, "*tick*\r\n");
	} else if (strcmp(token, "sunday") == 0) {
		last_sunday_time = time(0);
		send_to_char(ch, "Ok, it's now Sunday (kinda).\r\n");
	} else if (strcmp(token, "recalc") == 0) {
		tokens.next(token);
		recalc_all_mobs(ch, token);
	} else if (strcmp(token, "cmdusage") == 0) {
		for (idx = 1;idx < num_of_cmds;idx++) {
			cmd_num = cmd_sort_info[idx].sort_pos;
			if (!cmd_info[cmd_num].usage)
				continue;

			len += sprintf(buf + len, "%-15s %7lu   ",
				cmd_info[cmd_num].command, cmd_info[cmd_num].usage);
			if (!(idx % 3)) {
				strcpy(buf + len, "\r\n");
				len += 2;
			}
		}
		strcpy(buf + len, "\r\n");
		page_string(ch->desc, buf);
	} else if (strcmp(token, "unusedcmds") == 0) {
		for (idx = 1;idx < num_of_cmds;idx++) {
			cmd_num = cmd_sort_info[idx].sort_pos;
			if (cmd_info[cmd_num].usage)
				continue;

			len += sprintf(buf + len, "%-16s", cmd_info[cmd_num].command);
			if (!(idx % 5)) {
				strcpy(buf + len, "\r\n");
				len += 2;
			}
		}
		strcpy(buf + len, "\r\n");
		page_string(ch->desc, buf);
	} else if (strcmp(token, "verify") == 0) {
		verify_tempus_integrity(ch);
	} else if (strcmp(token, "chaos") == 0) {
		CreatureList::iterator cit = characterList.begin();

		for (;cit != characterList.end();++cit) {
			int ret_flags;
			Creature *attacked;
			perform_barb_berserk(*cit, &attacked, &ret_flags);
		}
		send_to_char(ch, "The entire world goes mad...\r\n");
		slog("%s has doomed the world to chaos.", GET_NAME(ch));
    } else if (strcmp(token, "loadzone") == 0) {
        int zone_num;
        zone_data *zone;

        tokens.next(token);
        if (!is_number(token)) {
            send_to_char(ch, "You must specify a zone number.\r\n");
            return;
        }
        zone_num = atoi(token);
        for (zone = zone_table; zone; zone = zone->next)
            if (zone->number == zone_num)
                break;
        if (zone) {
            send_to_char(ch, "Zone %d is already loaded!\r\n", zone_num);
            return;
        }

        if (load_single_zone(zone_num)) {
            send_to_char(ch, "Zone %d loaded and reset.\r\n", zone_num);
        } else {
            send_to_char(ch, "Zone could not be found or reset.\r\n");
        }
    } else
        send_to_char(ch, CODER_UTIL_USAGE);
}



static const char* ACCOUNT_USAGE = 
                    "Usage: account <command> <args>\r\n"
                    "Commands: \r\n"
                    "      disable <id>\r\n"
					"      enable <id>\r\n"
					"      movechar <Char ID> <to ID>\r\n"
					"      exhume <account ID> <Character ID>\r\n"
					"      reload <account ID>\r\n"
                    ;
ACMD(do_account)
{
	
    Tokenizer tokens(argument);
    char token[MAX_INPUT_LENGTH];
	int account_id = 0;
	long vict_id = 0;
	Account *account = NULL;
	Creature *vict;
    
    if(!tokens.next(token)) {
        send_to_char( ch, ACCOUNT_USAGE );
        return;
    }

    if (strcmp(token, "disable") == 0) {
		send_to_char(ch, "Not Implemented.\r\n");
	} else if (strcmp(token, "enable") == 0) {
		send_to_char(ch, "Not Implemented.\r\n");
	} else if (strcmp(token, "movechar") == 0) {
		Account *dst_account;

		if (!tokens.next(token)) {
			send_to_char(ch, "Specify a character id.\r\n");
			return;
		}
		vict_id = atol(token);
		if (!playerIndex.exists(vict_id)) {
			send_to_char(ch, "That player does not exist.\r\n");
			return;
		}
		if (!tokens.next(token)) {
			send_to_char(ch, "Specify an account id.\r\n");
			return;
		}
			
		account_id = playerIndex.getAccountID(vict_id);
		account = Account::retrieve(account_id);
		
        account_id = atoi(token);
		if (!Account::exists(account_id)) {
            send_to_char(ch, "That account does not exist.\r\n");
            return;
        }
        dst_account = Account::retrieve(account_id);
		account->move_char(vict_id, dst_account);

		// Update descriptor
		vict = get_char_in_world_by_idnum(vict_id);
		if (vict && vict->desc)
			vict->desc->account = dst_account;

		send_to_char(ch, "%s[%ld] has been moved from account %s[%d] to %s[%d]\r\n",
			playerIndex.getName(vict_id), vict_id,
			account->get_name(), account->get_idnum(),
			dst_account->get_name(), dst_account->get_idnum());
	} else if (strcmp(token, "exhume") == 0) {
		if(!tokens.next(token) ) {
			send_to_char(ch, "Specify an account id.\r\n");
			return;
		}
		account_id = atoi(token);

		if(!tokens.next(token) ) {
			send_to_char(ch, "Specify a character id.\r\n");
			return;
		}
		vict_id = atol(token);
		account = Account::retrieve(account_id);
		if( account == NULL ) {
			send_to_char(ch, "No such account: %d\r\n",account_id);
			return;
		}
		account->exhume_char(ch, vict_id);
	} else if (strcmp(token, "reload") == 0) {
		if(!tokens.next(token) ) {
			send_to_char(ch, "Specify an account id.\r\n");
			return;
		}

		account_id = atoi(token);
		account = Account::retrieve(account_id);
		if (!account) {
			send_to_char(ch, "No such account: %s\r\n", token);
			return;
		}

		if (account->reload())
			send_to_char(ch, "Account successfully reloaded from db\r\n");
		else
			send_to_char(ch, "Error: Account could not be reloaded\r\n");
		return;
	} else {
		send_to_char(ch, ACCOUNT_USAGE);
	}
}

ACMD(do_tester)
{
    ACMD(do_gen_tog);
    void do_start(struct Creature *ch, int mode);
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    byte tcmd;
    int i;

    if( ! ch->isTester() || GET_LEVEL(ch) >= LVL_AMBASSADOR ) {
        send_to_char(ch, "You are not a tester.\r\n");
        return;
    }

    two_arguments(argument, arg1, arg2);

    if (!*arg1) {
        send_to_char(ch, "Test what?\r\n");
        send_to_char(ch, TESTER_UTIL_USAGE);
        return;
    }

    if ((tcmd = search_block(arg1, tester_cmds, FALSE)) < 0) {
        send_to_char(ch, TESTER_UTIL_USAGE);
        return;
    }

    switch (tcmd) {
        case 0:                    /* advance */
            if (!*arg2)
                send_to_char(ch, "Advance to what level?\r\n");
            else if (!is_number(arg2))
                send_to_char(ch, "The argument must be a number.\r\n");
            else if ((i = atoi(arg2)) <= 0)
                send_to_char(ch, "That's not a level!\r\n");
            else if (i >= LVL_AMBASSADOR)
                send_to_char(ch, "Advance: I DON'T THINK SO!\r\n");
            else {
                if (i < GET_LEVEL(ch)) {
                    do_start(ch, TRUE);
                    GET_LEVEL(ch) = i;
                  } 
              
                  send_to_char(ch, "Your body vibrates for a moment.... you feel different!\r\n");
                  gain_exp_regardless(ch, exp_scale[i] - GET_EXP(ch));

                  for (i = 1; i < MAX_SKILLS; i++) {
                      if (spell_info[i].min_level[(int)GET_CLASS(ch)] <= GET_LEVEL(ch) 
                      || (IS_REMORT(ch) 
                      &&  spell_info[i].min_level[(int)CHECK_REMORT_CLASS(ch)] <= GET_LEVEL(ch))) 
                      {
                            GET_SKILL(ch, i) = LEARNED(ch);
                      }
                  }
                  GET_HIT(ch) = GET_MAX_HIT(ch);
                  GET_MANA(ch) = GET_MAX_MANA(ch);
                  GET_MOVE(ch) = GET_MAX_MOVE(ch);
                  ch->saveToXML();
            }
            break;
        case 1:                    /* unaffect */
            do_wizutil(ch, "self", 0, SCMD_UNAFFECT, 0);
            break;
        case 2:                    /* reroll */
            do_wizutil(ch, "self", 0, SCMD_REROLL, 0);
            break;
        case 3:                    /* stat */
            do_stat(ch, arg2, 0, 0, 0);
            break;
        case 4:                    /* goto */
            do_goto(ch, arg2, 0, 0, 0);
            break;
        case 5:                    /* restore */
            GET_HIT(ch) = GET_MAX_HIT(ch);
            GET_MANA(ch) = GET_MAX_MANA(ch);
            GET_MOVE(ch) = GET_MAX_MOVE(ch);
            send_to_char(ch, "You are fully healed!\r\n");
            break;
        case 6:                    /* char_class  */
        case 7:                    /* race   */
        case 8:                    /* remort */
        case 9:                    /* maxhit */
        case 10:                   /* maxmana */
        case 11:                   /* maxmove */
            sprintf(buf, "self %s %s", arg1, arg2);
            do_set(ch, buf, 0, SCMD_TESTER_SET, 0);
            break;
        case 12:
            do_gen_tog(ch, "", CMD_TESTER, SCMD_NOHASSLE, 0);
            break;
        case 13:
            do_gen_tog(ch, "", CMD_TESTER, SCMD_ROOMFLAGS, 0);
            break;
        case 14:
            if (!*arg2)
              send_to_char(ch, "Set align to what?\r\n");
            else {
              GET_ALIGNMENT(ch) = atoi(arg2);
              send_to_char(ch, "Align set to %d.\r\n", GET_ALIGNMENT(ch));
            }
            break;
        case 15:
            if (!*arg2)
              send_to_char(ch, "Set gen to what?\r\n");
            else {
              GET_REAL_GEN(ch) = atoi(arg2);
              send_to_char(ch, "gen set to %d.\r\n", GET_REAL_GEN(ch));
            }
            break;
        case 16:
            do_gen_tog(ch, "", CMD_TESTER, SCMD_DEBUG, 0);
            break;
        case 17: // strength
        case 18: // intelligence
        case 19: // wisdom
        case 20: // constitution
        case 21: // dexterity
        case 22: // charisma
            sprintf(buf, "self %s %s", arg1, arg2);
            do_set(ch, buf, 0, SCMD_TESTER_SET, 0);
            break;
        case 23: // Max Stats
            do_set(ch, "self str 25", 0, SCMD_TESTER_SET, 0);
            do_set(ch, "self int 25", 0, SCMD_TESTER_SET, 0);
            do_set(ch, "self wis 25", 0, SCMD_TESTER_SET, 0);
            do_set(ch, "self con 25", 0, SCMD_TESTER_SET, 0);
            do_set(ch, "self dex 25", 0, SCMD_TESTER_SET, 0);
            do_set(ch, "self cha 25", 0, SCMD_TESTER_SET, 0);
            break;
        default:
            sprintf(buf, "$p: Invalid command '%s'.", arg1);
            send_to_char(ch, TESTER_UTIL_USAGE);
            break;
    }
    return;
}

int do_freeze_char(char *argument, Creature *vict, Creature *ch)
{
    static const int ONE_MINUTE = 60;
    static const int ONE_HOUR   = 3600;
    static const int ONE_DAY    = 86400;
    static const int ONE_YEAR   = 31536000;

    if (ch == vict) {
        send_to_char(ch, "Oh, yeah, THAT'S real smart...\r\n");
        return 0;
    }

    char *thaw_time_str = tmp_getword(&argument);
    time_t now = time(NULL);
    time_t thaw_time;
    char *msg = "";

    if (!*thaw_time_str) {
        thaw_time = now + ONE_DAY;
        msg = tmp_sprintf("%s frozen for 1 day", vict->player.name);
    }
    else if (isname_exact(thaw_time_str, "forever")) {
        thaw_time = -1;
        msg = tmp_sprintf("%s frozen forever", vict->player.name);
    }
    else {
        int amt = atoi(thaw_time_str);
        switch (thaw_time_str[strlen(thaw_time_str) - 1]) {
            case 's':
                thaw_time = now + amt;
                msg = tmp_sprintf("%s frozen for %d second%s",
                             vict->player.name, amt, (amt == 1) ? "":"s");
                break;
            case 'm':
                thaw_time = now + (amt * ONE_MINUTE);
                msg = tmp_sprintf("%s frozen for %d minute%s",
                             vict->player.name, amt, (amt == 1) ? "":"s");
                break;
            case 'h':
                thaw_time = now + (amt * ONE_HOUR);
                msg = tmp_sprintf("%s frozen for %d hour%s",
                             vict->player.name, amt, (amt == 1) ? "":"s");
                break;
            case 'd':
                thaw_time = now + (amt * ONE_DAY);
                msg = tmp_sprintf("%s frozen for %d day%s",
                             vict->player.name, amt, (amt == 1) ? "":"s");
                break;
            default:
                send_to_char(ch, 
                             "Usage: freeze <target> [ XXXs | XXXm | XXXh | XXXd | forever]\r\n");
                return false;
        }

        if ((thaw_time - now) >= ONE_YEAR) {
            send_to_char(ch, "Come now, Let's be reasonable, shall we?\r\n");
            return false;
        }
    }
    
    SET_BIT(PLR_FLAGS(vict), PLR_FROZEN);
    GET_FREEZE_LEV(vict) = GET_LEVEL(ch);
    vict->player_specials->thaw_time = thaw_time;
    vict->player_specials->freezer_id = GET_IDNUM(ch);
     
    send_to_char(vict, "A bitter wind suddenly rises and drains every erg "
                       "of heat from your body!\r\nYou feel frozen!\r\n");
    
    send_to_char(ch, "%s\r\n", msg);
    mudlog(MAX(LVL_POWER, GET_INVIS_LVL(ch)), BRF, true, "(GC) %s by %s", 
           msg, GET_NAME(ch)); 
    
    if (vict->in_room)
        act("A sudden cold wind conjured from nowhere freezes $N!", false,
            ch, 0, vict, TO_ROOM);
    
    return true;
}

#define USERS_USAGE \
"format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-c char_claslist] [-o] [-p]\r\n"

ACMD(do_users)
{
    long find_char_class_bitvector(char arg);

	char *name_search = NULL, *host_search = NULL;
    const char *char_name, *account_name, *state;
	struct Creature *tch;
	struct descriptor_data *d;
	int low = 0, high = LVL_GRIMP, i, num_can_see = 0;
	int showchar_class = 0, outlaws = 0, playing = 0, deadweight = 0;
    char *arg;
    char timebuf[27], idletime[10];

    while (*(arg = tmp_getword(&argument)) != '\0') {
		if (*arg == '-') {
			switch (*(arg + 1)) {
			case 'o':
			case 'k':
				outlaws = 1;
				playing = 1;
				break;
			case 'p':
				playing = 1;
				break;
			case 'd':
				deadweight = 1;
				break;
			case 'l':
				playing = 1;
                arg = tmp_getword(&argument);
				sscanf(arg, "%d-%d", &low, &high);
				break;
			case 'n':
				playing = 1;
                name_search = tmp_getword(&argument);
				break;
			case 'h':
				playing = 1;
                host_search = tmp_getword(&argument);
				break;
			case 'c':
				playing = 1;
                arg = tmp_getword(&argument);
				for (i = 0; i < (int)strlen(arg); i++)
					showchar_class |= find_char_class_bitvector(arg[i]);
				break;
			default:
				send_to_char(ch, USERS_USAGE);
				return;
				break;
			}					/* end of switch */

		} else {				/* endif */
			send_to_char(ch, USERS_USAGE);
			return;
		}
	}							/* end while (parser) */

    acc_string_clear();
    acc_strcat(
		" Num Account      Character     State          Idl Login@   Site\r\n",
		" --- ------------ ------------- --------------- -- -------- ---------------\r\n",
        NULL);

	for (d = descriptor_list; d; d = d->next) {
		if (IS_PLAYING(d) && playing)
			continue;
		if (STATE(d) == CXN_PLAYING && deadweight)
			continue;
		if (STATE(d) == CXN_PLAYING) {
			if (d->original)
				tch = d->original;
			else if (!(tch = d->creature))
				continue;
			if (host_search && !strstr(d->host, host_search))
				continue;
			if (name_search && str_cmp(GET_NAME(tch), name_search))
				continue;
			if (GET_LEVEL(ch) < LVL_LUCIFER)
				if (!can_see_creature(ch, tch) || GET_LEVEL(tch) < low
					|| GET_LEVEL(tch) > high)
					continue;
			if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
				!PLR_FLAGGED(tch, PLR_THIEF))
				continue;
			if (showchar_class && !(showchar_class & (1 << GET_CLASS(tch))))
				continue;
			if (GET_LEVEL(ch) < LVL_LUCIFER &&
                GET_INVIS_LVL(ch) > GET_LEVEL(ch))
                continue;
            if (!can_see_creature(ch, d->creature))
                continue;
		}

        strftime(timebuf, 24, "%H:%M:%S", localtime(&d->login_time));
		if (STATE(d) == CXN_PLAYING && d->original)
            state = "switched";
		else
            state = strlist_aref(STATE(d), desc_modes);

		if (d->creature && STATE(d) == CXN_PLAYING &&
			(GET_LEVEL(d->creature) < GET_LEVEL(ch)
				|| GET_LEVEL(ch) >= LVL_LUCIFER))
			sprintf(idletime, "%2d",
				d->creature->char_specials.timer * SECS_PER_MUD_HOUR /
				SECS_PER_REAL_MIN);
		else
			sprintf(idletime, "%2d", d->idle);

        if (!d->creature)
            char_name = "      -      ";
        else if (d->original)
            char_name = tmp_sprintf("%s%-13s%s",
                                    CCCYN(ch, C_NRM),
                                    d->original->player.name,
                                    CCNRM(ch, C_NRM));
        else if (IS_IMMORT(d->creature))
            char_name = tmp_sprintf("%s%-13s%s",
                                    CCGRN(ch, C_NRM),
                                    d->creature->player.name,
                                    CCNRM(ch, C_NRM));
        else
            char_name = tmp_sprintf("%-13s", d->creature->player.name);

        if (d->account)
            account_name = tmp_substr(d->account->get_name(), 0, 12);
        else
            account_name = "   -   ";

        acc_sprintf("%4d %-12s %s %-15s %-2s %-8s ",
                    d->desc_num,
                    account_name,
                    char_name,
                    state, idletime, timebuf);

		if (d->host && *d->host)
			acc_sprintf("%s%s%s\r\n", isbanned(d->host,
					buf) ? (d->creature
					&& PLR_FLAGGED(d->creature, PLR_SITEOK) ? CCMAG(ch,
						C_NRM) : CCRED(ch, C_SPR)) : CCGRN(ch, C_NRM), d->host,
				CCNRM(ch, C_SPR));
		else
			acc_strcat(CCRED_BLD(ch, C_SPR),
                       "[unknown]\r\n",
                       CCNRM(ch, C_SPR));

        num_can_see++;
	}

	acc_sprintf("\r\n%d visible sockets connected.\r\n", num_can_see);
	page_string(ch->desc, acc_get_string());
}


ACMD(do_edit)
{
    char *arg;

    if (GET_LEVEL(ch) < LVL_GOD) {
        send_to_char(ch, "You don't see anything you can edit.\r\n");
        return;
    }

    arg = tmp_getword(&argument);

    if (isname(arg, "bugs")) {
        act("$n begins to edit the bug file.", false, ch, 0, 0, TO_ROOM);
        start_editing_file(ch->desc, BUG_FILE);
    }
    else if (isname(arg, "typos")) {
        act("$n begins to edit the typo file.", false, ch, 0, 0, TO_ROOM);
        start_editing_file(ch->desc, TYPO_FILE);
    }
    else if (isname(arg, "ideas")) {
        act("$n begins to edit the idea file.", false, ch, 0, 0, TO_ROOM);
        start_editing_file(ch->desc, IDEA_FILE);
    }
    else
        send_to_char(ch, "Edit what?!?\r\n");

    return;
}

ACMD(do_delete)
{
	char *name;
	Creature *vict;
	Account *acct;
	int vict_id, acct_id;
	bool from_file;
	char *tmp_name;

	name = tmp_getword(&argument);
	if (!Security::isMember(ch, "AdminFull") &&
			!Security::isMember(ch, "WizardFull")) {
		send_to_char(ch, "No way!  You're not authorized!\r\n");
		mudlog(GET_INVIS_LVL(ch), BRF, true, "%s denied deletion of '%s'",
			GET_NAME(ch), name);
		return;
	}

	if (!*name) {
		send_to_char(ch, "Come, come.  Don't you want to delete SOMEONE?\r\n");
		return;
	}
	
	if (!playerIndex.exists(name)) {
		send_to_char(ch, "That player does not exist.\r\n");
		return;
	}

	acct_id = playerIndex.getAccountID(name);
	acct = Account::retrieve(acct_id);
	if (!acct) {
		errlog("Victim found without account");
		send_to_char(ch, "The command mysteriously failed (XYZZY)\r\n");
		return;
	}

	vict_id = playerIndex.getID(name);
	vict = get_char_in_world_by_idnum(vict_id);
	from_file = !vict;
	if (from_file) {
		vict = new Creature(true);
		if (!vict->loadFromXML(vict_id)) {
			delete vict;
			send_to_char(ch, "Error loading char from file.\r\n");
			return;
		}
	}

	tmp_name = tmp_strdup(GET_NAME(vict));

	acct->delete_char(vict);
    acct->reload();

	send_to_char(ch, "Character '%s' has been deleted from account %s.\r\n",
		tmp_name, acct->get_name());
	mudlog(LVL_IMMORT, NRM, true,
		"%s has deleted %s[%d] from account %s[%d]",
		GET_NAME(ch),
		tmp_name,
		vict_id,
		acct->get_name(),
		acct->get_idnum());

	if (from_file)
		delete vict;
}

void
check_log(Creature *ch, const char *fmt, ...)
{
    const int MAX_FRAMES = 10;	
	va_list args;
	const char *backtrace_str = "";
    void *ret_addrs[MAX_FRAMES + 1];
    int x = 0;
    char *msg;

	va_start(args, fmt);
    msg = tmp_vsprintf(fmt, args);
	va_end(args);

    if (ch)
        send_to_char(ch, "%s\r\n", msg);

    memset(ret_addrs, 0x0, sizeof(ret_addrs));
    backtrace(ret_addrs, MAX_FRAMES);

    while (x < MAX_FRAMES && ret_addrs[x]) {
		backtrace_str = tmp_sprintf("%s%p%s", backtrace_str, ret_addrs[x],
                (ret_addrs[x + 1]) ? " < " : "");
        x++;
    }

	mlog(Security::NOONE, LVL_AMBASSADOR, NRM, true, "CHECK: %s", msg);
	mlog(Security::NOONE, LVL_AMBASSADOR, NRM, true,
		"TRACE: %s", backtrace_str);
}

bool
check_ptr(Creature *ch, void *ptr, size_t expected_len, const char *str, int vnum)
{
#ifdef MEMTRACK
	dbg_mem_blk *mem;
	
	if (!ptr)
		return true;
    
        mem = dbg_get_block(ptr);
        if (!mem) {
            check_log(ch, "Invalid pointer found in %s %d", str, vnum);
            return false;
        }

        if (expected_len > 0 && mem->size != expected_len) {
            check_log(ch, "Expected block of size %d, got size %d in %s %d",
                      expected_len, mem->size, str, vnum);
            return false;
        }
    }
	return true;
#else
    return true;
#endif
}

void
verify_tempus_integrity(Creature *ch)
{
	MobileMap::iterator mit;
    CreatureList::iterator cit;
	Creature *vict;
	obj_data *obj, *contained;
	room_data *room;
	zone_data *zone;
	extra_descr_data *cur_exdesc;
	memory_rec_struct *cur_mem;
	const char *err;
    tmp_obj_affect *obj_aff;
    int idx;

	// Check prototype mobs
	for (mit = mobilePrototypes.begin();mit != mobilePrototypes.end();mit++) {
		vict = mit->second;

		if (!vict->player.name)
			check_log(ch, "Alias of creature proto #%d (%s) is NULL!",
				MOB_IDNUM(vict), vict->player.short_descr);
		check_ptr(ch, vict->player.name, 0,
			"aliases of creature proto", MOB_IDNUM(vict));
		check_ptr(ch, vict->player.short_descr, 0,
			"name of creature proto", MOB_IDNUM(vict));
		check_ptr(ch, vict->player.long_descr, 0,
			"linedesc of creature proto", MOB_IDNUM(vict));
		check_ptr(ch, vict->player.description, 0,
			"description of creature proto", MOB_IDNUM(vict));
		check_ptr(ch, vict->player.description, 0,
			"description of creature proto", MOB_IDNUM(vict));
		check_ptr(ch, vict->mob_specials.func_data, 0,
			"func_data of creature proto", MOB_IDNUM(vict));

		// Loop through memory
		for (cur_mem = vict->mob_specials.memory;cur_mem;cur_mem = cur_mem->next) {
			if (!check_ptr(ch, cur_mem, sizeof(memory_rec_struct),
					"memory of creature proto", MOB_IDNUM(vict)))
				break;
		}
		// Mobile shared structure
		if (check_ptr(ch, vict->mob_specials.func_data,
				sizeof(mob_shared_data), "shared struct of creature proto",
				MOB_IDNUM(vict))) {

			check_ptr(ch, vict->mob_specials.shared->move_buf, 0,
				"move_buf of creature proto", MOB_IDNUM(vict));
			check_ptr(ch, vict->mob_specials.shared->func_param, 0,
				"func_param of creature proto", MOB_IDNUM(vict));
			check_ptr(ch, vict->mob_specials.shared->func_param, 0,
				"load_param of creature proto", MOB_IDNUM(vict));
		}

		if (vict->mob_specials.shared->proto != vict)
			check_log(ch, "Prototype of prototype is not itself on mob %d",
				MOB_IDNUM(vict));
	}

	// Check prototype objects
    ObjectMap::iterator oi = objectPrototypes.begin();
    for (; oi != objectPrototypes.end(); ++oi) {
        obj = oi->second;
		check_ptr(ch, obj, sizeof(obj_data),
			"object proto", -1);
		check_ptr(ch, obj->name, 0,
			"name of object proto", GET_OBJ_VNUM(obj));
		check_ptr(ch, obj->aliases, 0,
			"aliases of object proto", GET_OBJ_VNUM(obj));
		check_ptr(ch, obj->line_desc, 0,
			"line desc of object proto", GET_OBJ_VNUM(obj));
		check_ptr(ch, obj->action_desc, 0,
			"action desc of object proto", GET_OBJ_VNUM(obj));
		for (cur_exdesc = obj->ex_description;cur_exdesc;cur_exdesc = cur_exdesc->next) {
			if (!check_ptr(ch, cur_exdesc, sizeof(extra_descr_data),
					"extradesc of object proto", GET_OBJ_VNUM(obj)))
				break;
			check_ptr(ch, cur_exdesc->keyword, 0,
				"keyword of extradesc of object proto", GET_OBJ_VNUM(obj));
			check_ptr(ch, cur_exdesc->description, 0,
				"description of extradesc of object proto", GET_OBJ_VNUM(obj));
		}
		check_ptr(ch, obj->shared, sizeof(obj_shared_data),
			"shared struct of object proto", GET_OBJ_VNUM(obj));
		check_ptr(ch, obj->shared->func_param, 0,
			"func param of object proto", GET_OBJ_VNUM(obj));
	}

	// Check rooms
	for (zone = zone_table;zone;zone = zone->next) {
		for (room = zone->world;room;room = room->next) {
			check_ptr(ch, room->name, 0,
				"title of room", room->number);
			check_ptr(ch, room->name, 0,
				"description of room", room->number);
			check_ptr(ch, room->sounds, 0,
				"sounds of room", room->number);
			for (cur_exdesc = room->ex_description;cur_exdesc;cur_exdesc = cur_exdesc->next) {
				if (!check_ptr(ch, cur_exdesc, sizeof(extra_descr_data),
						"extradesc of room", room->number))
					break;
				check_ptr(ch, cur_exdesc->keyword, 0,
					"keyword of extradesc of room", room->number);
				check_ptr(ch, cur_exdesc->description, 0,
					"description of extradesc of room", room->number);
			}
			if (ABS_EXIT(room, NORTH)
					&& check_ptr(ch, ABS_EXIT(room, NORTH),
					sizeof(room_direction_data),
					"north exit of room", room->number)) {
				check_ptr(ch, ABS_EXIT(room, NORTH)->general_description, 0,
					"description of north exit of room", room->number);
				check_ptr(ch, ABS_EXIT(room, NORTH)->keyword, 0,
					"keywords of north exit of room", room->number);
				check_ptr(ch, ABS_EXIT(room, NORTH)->to_room, sizeof(room_data),
					"destination of north exit of room", room->number);
			}
			if (ABS_EXIT(room, SOUTH)
					&& check_ptr(ch, ABS_EXIT(room, SOUTH),
					sizeof(room_direction_data),
					"south exit of room", room->number)) {
				check_ptr(ch, ABS_EXIT(room, SOUTH)->general_description, 0,
					"description of south exit of room", room->number);
				check_ptr(ch, ABS_EXIT(room, SOUTH)->keyword, 0,
					"keywords of south exit of room", room->number);
				check_ptr(ch, ABS_EXIT(room, SOUTH)->to_room, sizeof(room_data),
					"destination of south exit of room", room->number);
			}
			if (ABS_EXIT(room, EAST)
					&& check_ptr(ch, ABS_EXIT(room, EAST),
					sizeof(room_direction_data),
					"east exit of room", room->number)) {
				check_ptr(ch, ABS_EXIT(room, EAST)->general_description, 0,
					"description of east exit of room", room->number);
				check_ptr(ch, ABS_EXIT(room, EAST)->keyword, 0,
					"keywords of east exit of room", room->number);
				check_ptr(ch, ABS_EXIT(room, EAST)->to_room, sizeof(room_data),
					"destination of east exit of room", room->number);
			}
			if (ABS_EXIT(room, WEST)
					&& check_ptr(ch, ABS_EXIT(room, WEST),
					sizeof(room_direction_data),
					"west exit of room", room->number)) {
				check_ptr(ch, ABS_EXIT(room, WEST)->general_description, 0,
					"description of west exit of room", room->number);
				check_ptr(ch, ABS_EXIT(room, WEST)->keyword, 0,
					"keywords of west exit of room", room->number);
				check_ptr(ch, ABS_EXIT(room, WEST)->to_room, sizeof(room_data),
					"destination of west exit of room", room->number);
			}
			if (ABS_EXIT(room, UP)
					&& check_ptr(ch, ABS_EXIT(room, UP),
					sizeof(room_direction_data),
					"up exit of room", room->number)) {
				check_ptr(ch, ABS_EXIT(room, UP)->general_description, 0,
					"description of up exit of room", room->number);
				check_ptr(ch, ABS_EXIT(room, UP)->keyword, 0,
					"keywords of up exit of room", room->number);
				check_ptr(ch, ABS_EXIT(room, UP)->to_room, sizeof(room_data),
					"destination of up exit of room", room->number);
			}
			if (ABS_EXIT(room, DOWN)
					&& check_ptr(ch, ABS_EXIT(room, DOWN),
					sizeof(room_direction_data),
					"down exit of room", room->number)) {
				check_ptr(ch, ABS_EXIT(room, DOWN)->general_description, 0,
					"description of down exit of room", room->number);
				check_ptr(ch, ABS_EXIT(room, DOWN)->keyword, 0,
					"keywords of down exit of room", room->number);
				check_ptr(ch, ABS_EXIT(room, DOWN)->to_room, sizeof(room_data),
					"destination of down exit of room", room->number);
			}
			if (ABS_EXIT(room, PAST)
					&& check_ptr(ch, ABS_EXIT(room, PAST),
					sizeof(room_direction_data),
					"past exit of room", room->number)) {
				check_ptr(ch, ABS_EXIT(room, PAST)->general_description, 0,
					"description of past exit of room", room->number);
				check_ptr(ch, ABS_EXIT(room, PAST)->keyword, 0,
					"keywords of past exit of room", room->number);
				check_ptr(ch, ABS_EXIT(room, PAST)->to_room, sizeof(room_data),
					"destination of past exit of room", room->number);
			}
			if (ABS_EXIT(room, FUTURE)
					&& check_ptr(ch, ABS_EXIT(room, FUTURE),
					sizeof(room_direction_data),
					"future exit of room", room->number)) {
				check_ptr(ch, ABS_EXIT(room, FUTURE)->general_description, 0,
					"description of future exit of room", room->number);
				check_ptr(ch, ABS_EXIT(room, FUTURE)->keyword, 0,
					"keywords of future exit of room", room->number);
				check_ptr(ch, ABS_EXIT(room, FUTURE)->to_room, sizeof(room_data),
					"destination of future exit of room", room->number);
			}
            for (contained = room->contents;
                 contained;
                 contained = contained->next_content) {
                if (!check_ptr(ch, contained, sizeof(obj_data),
                               "object in room", room->number))
                    break;
                if (contained->in_room != room) {
                    check_log(ch, "expected object %ld carrier = room %d (%p), got %p",
                                 contained->unique_id,
                                 room->number,
                                 room,
                                 contained->in_room);
                }
            }
            
		}
	}

	// Check tmpstr module
	err = tmp_string_test();
	if (err)
		check_log(ch, "%s", err);

	// Check zones
	// Check mobiles in game
    for (cit = characterList.begin();
         cit != characterList.end();
         ++cit) {
        vict = *cit;

        check_ptr(ch, vict, sizeof(Creature),
                             "mobile", GET_MOB_VNUM(vict));
        for (idx = 0;idx < NUM_WEARS;idx++) {
            if (GET_EQ(vict, idx)
                && check_ptr(ch, GET_EQ(vict, idx), sizeof(obj_data),
                             "object worn by mobile", GET_MOB_VNUM(vict))) {
                if (GET_EQ(vict, idx)->worn_by != vict)
                    check_log(ch, "expected object wearer wrong!");
                
            }
            if (GET_IMPLANT(vict, idx)
                && check_ptr(ch, GET_IMPLANT(vict, idx), sizeof(obj_data),
                             "object implanted in mobile", GET_MOB_VNUM(vict))) {
                if (GET_IMPLANT(vict, idx)->worn_by != vict)
                    check_log(ch, "expected object implanted wrong!");
                
            }
            if (GET_TATTOO(vict, idx)
                && check_ptr(ch, GET_TATTOO(vict, idx), sizeof(obj_data),
                             "object tattooed in mobile", GET_MOB_VNUM(vict))) {
                if (GET_TATTOO(vict, idx)->worn_by != vict)
                    check_log(ch, "expected object tattooed wrong!");
            }
        }

        for (contained = vict->carrying;
             contained;
             contained = contained->next_content) {
            if (!check_ptr(ch, contained, sizeof(obj_data),
                                      "object carried by mobile vnum %d", GET_MOB_VNUM(vict)))
                break;
            if (contained->carried_by != vict) {
                check_log(ch, "expected object %ld carrier = mob %d (%p), got %p",
                             contained->unique_id,
                             GET_MOB_VNUM(vict),
                             vict,
                             contained->carried_by);
            }
        }
        
    }

	// Check objects in game
    for (obj = object_list; obj; obj = obj->next) {
        check_ptr(ch, obj, sizeof(obj_data),
                             "object", obj->unique_id);
        check_ptr(ch, obj->name, 0,
                             "name of object", obj->unique_id);
        check_ptr(ch, obj->aliases, 0,
                             "aliases of object", obj->unique_id);
        check_ptr(ch, obj->line_desc, 0,
                             "line desc of object", obj->unique_id);
        check_ptr(ch, obj->action_desc, 0,
                             "action desc of object", obj->unique_id);
        for (cur_exdesc = obj->ex_description;cur_exdesc;cur_exdesc = cur_exdesc->next) {
            if (!check_ptr(ch, cur_exdesc, sizeof(extra_descr_data),
                                      "extradesc of obj", obj->unique_id))
                break;
            check_ptr(ch, cur_exdesc->keyword, 0,
                                 "keyword of extradesc of obj", obj->unique_id);
            check_ptr(ch, cur_exdesc->description, 0,
                                 "description of extradesc of room", obj->unique_id);
        }

        check_ptr(ch, obj->shared, sizeof(obj_shared_data),
                             "shared data of object", obj->unique_id);
        check_ptr(ch, obj->in_room, sizeof(room_data),
                             "room location of object", obj->unique_id);
        check_ptr(ch, obj->in_obj, sizeof(obj_data),
                             "object location of object", obj->unique_id);
        check_ptr(ch, obj->carried_by, sizeof(Creature),
                             "carried_by location of object", obj->unique_id);
        check_ptr(ch, obj->worn_by, sizeof(Creature),
                             "worn_by location of object", obj->unique_id);
        check_ptr(ch, obj->func_data, 0,
                             "special data of object", obj->unique_id);
        if (!(obj->in_room ||
              obj->in_obj ||
              obj->carried_by ||
              obj->worn_by))
            check_log(ch, "object %s (#%d) (uid %lu) stuck in limbo",
				obj->name, GET_OBJ_VNUM(obj),
				obj->unique_id);

        for (contained = obj->contains;
             contained;
             contained = contained->next_content) {
            if (!check_ptr(ch, contained, sizeof(obj_data),
                                      "object contained by object", obj->unique_id))
                break;
            if (contained->in_obj != obj) {
                check_log(ch, "expected object %lu's in_obj = obj %lu(%p), got %p",
                             contained->unique_id,
                             obj->unique_id,
                             obj,
                             contained->in_obj);
            }
        }
        for (obj_aff = obj->tmp_affects;
             obj_aff;
             obj_aff = obj_aff->next) {
            if (!check_ptr(ch, obj_aff, sizeof(tmp_obj_affect),
                           "tmp obj affect in object", obj->unique_id))
                break;
        }
        
        // Check object for empty alias list
        char *alias;

        alias = obj->aliases;
        skip_spaces(&alias);
        if (*alias == '\0') {
            check_log(ch, "object %s (#%d) (uid %lu) has no alias - setting to obj%lu",
				obj->name, GET_OBJ_VNUM(obj),
                      obj->unique_id, obj->unique_id);
            free(obj->aliases);
            obj->aliases = strdup(tmp_sprintf("obj%lu", obj->unique_id));
        }
    }
        
	// Check accounts
	// Check descriptors
	// Check players in game
	// Check dyntext items
	// Check help
	// Check security groups
}
