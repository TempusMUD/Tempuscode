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

#include <iostream>
#include <fstream>
#include <list>
#include <string.h>
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
#include "elevators.h"
#include "fight.h"
#include "defs.h"
#include "tokenizer.h"
#include "tmpstr.h"
#include "interpreter.h"
#include "utils.h"
#include "player_table.h"
#include "quest.h"


/*   external vars  */
extern FILE *player_fl;
extern struct obj_data *object_list;
extern struct descriptor_data *descriptor_list;
extern struct Creature *mob_proto;
extern struct obj_data *obj_proto;
extern struct zone_data *zone_table;
extern int top_of_zone_table;
extern int restrict;
extern int top_of_world;
extern int top_of_mobt;
extern int top_of_objt;
extern int top_of_p_table;
extern int log_cmds;
extern int olc_lock;
extern struct elevator_data *elevators;
extern struct player_index_element *player_table;
extern int lunar_stage;
extern int lunar_phase;
extern int quest_status;
extern struct Creature *combat_list;    /* head of list of fighting chars */
extern int shutdown_count;
extern int shutdown_mode;
extern int mini_mud;
extern int current_mob_idnum;
extern struct last_command_data last_cmd[NUM_SAVE_CMDS];


char *how_good(int percent);
extern char *prac_types[];
int spell_sort_info[MAX_SPELLS + 1];
int skill_sort_info[MAX_SKILLS - MAX_SPELLS + 1];
int has_mail(long idnum);
int prototype_obj_value(struct obj_data *obj);
int choose_material(struct obj_data *obj);
int set_maxdamage(struct obj_data *obj);

void build_player_table(void);
long asciiflag_conv(char *buf);
void show_social_messages(struct Creature *ch, char *arg);
void autosave_zones(int SAVE_TYPE);
void perform_oset(struct Creature *ch, struct obj_data *obj_p,
    char *argument, byte subcmd);
void do_show_objects(struct Creature *ch, char *value, char *arg);
void do_show_mobiles(struct Creature *ch, char *value, char *arg);
void show_searches(struct Creature *ch, char *value, char *arg);
void do_zone_cmdlist(struct Creature *ch, struct zone_data *zone, char *arg);
const char *stristr(const char *haystack, const char *needle);

int parse_char_class(char *arg);
void retire_trails(void);
float prac_gain(struct Creature *ch, int mode);
int skill_gain(struct Creature *ch, int mode);
void list_obj_to_char(struct obj_data *list, struct Creature *ch, int mode,
    bool show);
void save_quests(); // quests.cc - saves quest data
void export_player_table(Creature *ch);

int do_freeze_char(char *argument, Creature *vict, Creature *ch);

ACMD(do_equipment);
SPECIAL(shop_keeper);

static const char *logtypes[] = {
    "off", "brief", "normal", "complete", "\n"
};


void
show_char_class_skills(struct Creature *ch, int con, int immort, int bits)
{
    int lvl, skl;
    bool found;
    char *msg;

    msg = tmp_strcat("The ",
        pc_char_class_types[con],
        " class can learn the following ",
        IS_SET(bits, SPELL_BIT) ? "spells" :
            IS_SET(bits, TRIG_BIT) ? "triggers" :
            IS_SET(bits, ZEN_BIT) ? "zens" :
            IS_SET(bits, ALTER_BIT) ? "alterations" : "skills",
        ":\r\n", NULL);

    if (GET_LEVEL(ch) < LVL_IMMORT)
        msg = tmp_strcat(msg, "Lvl    Skill\r\n");
    else
        msg = tmp_strcat(msg,    
            "Lvl  Number   Skill           Mana: Max  Min  Chn  Flags\r\n");

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
                    msg = tmp_sprintf("%s %-2d", msg, lvl);
                else
                    msg = tmp_strcat(msg, "   ");

                if (GET_LEVEL(ch) < LVL_IMMORT)
                    msg = tmp_strcat(msg, "    ");
                else
                    msg = tmp_sprintf("%s - %3d. ", msg, skl);

                if (spell_info[skl].gen[con]) {
                    // This is wasteful, but it looks a lot better to have
                    // the gen after the spell.  The trick is that we want it
                    // to be yellow, but printf doesn't recognize the existence
                    // of escape codes for purposes of padding.
                    char *tmp;
                    int len;

                    tmp = tmp_sprintf("%s (gen %d)", spell_to_str(skl),
                        spell_info[skl].gen[con]);
                    len = strlen(tmp);
                    if (len > 33)
                        len = 33;
                    msg = tmp_sprintf("%s%s%s %s(gen %d)%s%s", msg,
                        CCGRN(ch, C_NRM), spell_to_str(skl), CCYEL(ch, C_NRM),
                        spell_info[skl].gen[con], CCNRM(ch, C_NRM),
                        tmp_pad(' ', 33 - len));
                } else {
                    msg = tmp_sprintf("%s%s%-33s%s", msg,
                        CCGRN(ch, C_NRM), spell_to_str(skl), CCNRM(ch, C_NRM));
                }

                if (GET_LEVEL(ch) >= LVL_IMMORT) {
                    sprintbit(spell_info[skl].routines, spell_bits, buf2);
                    msg = tmp_sprintf("%s%3d  %3d  %2d   %s%s%s", msg,
                        spell_info[skl].mana_max,
                        spell_info[skl].mana_min, spell_info[skl].mana_change,
                        CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
                }
                
                msg = tmp_strcat(msg, "\r\n");
                found = true;
            }
        }
    }
    page_string(ch->desc, msg);
}

void
list_residents_to_char(struct Creature *ch, int town)
{
    struct descriptor_data *d;
    byte found = 0;

    if (town < 0) {
        strcpy(buf, "       HOMETOWNS\r\n");
        for (d = descriptor_list; d; d = d->next) {
            if (!d->character || !can_see_creature(ch, d->character))
                continue;
            sprintf(buf, "%s%s%-20s%s -- %s%-30s%s\r\n", buf,
                GET_LEVEL(d->character) >= LVL_AMBASSADOR ? CCYEL(ch,
                    C_NRM) : "", GET_NAME(d->character),
                GET_LEVEL(d->character) >= LVL_AMBASSADOR ? CCNRM(ch,
                    C_NRM) : "", CCCYN(ch, C_NRM),
                home_towns[(int)GET_HOME(d->character)], CCNRM(ch, C_NRM));
        }
    } else {
        sprintf(buf, "On-line residents of %s.\r\n", home_towns[town]);
        for (d = descriptor_list; d; d = d->next) {
            if (!d->character || !can_see_creature(ch, d->character))
                continue;
            if (GET_HOME(d->character) == town) {
                sprintf(buf, "%s%s%-20s%s\r\n", buf,
                    GET_LEVEL(d->character) >= LVL_AMBASSADOR ? CCYEL(ch,
                        C_NRM) : "", GET_NAME(d->character),
                    GET_LEVEL(d->character) >= LVL_AMBASSADOR ? CCNRM(ch,
                        C_NRM) : "");
                found = 1;
            }
        }
        if (!found)
            strcat(buf, "None online.\r\n");
    }
    page_string(ch->desc, buf);
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
			argument);

        if (PRF_FLAGGED(ch, PRF_NOREPEAT))
            send_to_char(ch, OK);
        else
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
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
        send_to_char(ch, "Sent.\r\n");
    else {
        send_to_char(ch, "You send '%s' to %s.\r\n", buf, GET_NAME(vict));
        slog("(GC) %s send %s %s", GET_NAME(ch), GET_NAME(vict), buf);
    }
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

    if (POOFOUT(ch))
        msg = tmp_sprintf("$n %s", POOFOUT(ch));
    else
        msg = "$n disappears in a puff of smoke.";

    act(msg, TRUE, ch, 0, 0, TO_ROOM);
    char_from_room(ch,false);
    char_to_room(ch, room,false);
    if (room->isOpenAir())
        ch->setPosition(POS_FLYING);

    if (POOFIN(ch))
        msg = tmp_sprintf("$n %s", POOFIN(ch));
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

    one_argument(argument, buf);
    if (!*buf)
        send_to_char(ch, "Whom do you wish to transfer?\r\n");
    else if (str_cmp("all", buf)) {
        if (!(victim = get_char_vis(ch, buf))) {
            send_to_char(ch, NOPERSON);
        } else if (victim == ch)
            send_to_char(ch, "That doesn't make much sense, does it?\r\n");
        else {
            if ((GET_LEVEL(ch) < GET_LEVEL(victim)) && !IS_NPC(victim)) {
                send_to_char(ch, "Go transfer someone your own size.\r\n");
                return;
            }
            if (ch->in_room->isOpenAir()
                && victim->getPosition() != POS_FLYING) {
                send_to_char(ch, 
                    "Come now, you are in midair.  Are they flying?  I think not.\r\n");
                return;
            }
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
    } else {                    /* Trans All */
        if (GET_LEVEL(ch) < LVL_GRGOD) {
            send_to_char(ch, "I think not.\r\n");
            return;
        }

        for (i = descriptor_list; i; i = i->next)
            if (STATE(i) == CON_PLAYING && i->character && i->character != ch) {
                victim = i->character;
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
        act("$n has teleported you!", FALSE, ch, 0, (char *)victim, TO_VICT);
        look_at_room(victim, victim->in_room, 0);

        slog("(GC) %s has teleported %s to [%d] %s.", GET_NAME(ch),
            GET_NAME(victim), victim->in_room->number, victim->in_room->name);
    }
}



ACMD(do_vnum)
{
    two_arguments(argument, buf, buf2);

    if (!*buf || !*buf2 || (!is_abbrev(buf, "mob") && !is_abbrev(buf, "obj"))) {
        send_to_char(ch, "Usage: vnum { obj | mob } <name>\r\n");
        return;
    }
    if (is_abbrev(buf, "mob"))
        if (!vnum_mobile(buf2, ch))
            send_to_char(ch, "No mobiles by that name.\r\n");

    if (is_abbrev(buf, "obj"))
        if (!vnum_object(buf2, ch))
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
                /*CHARADD (sum, tmpsearch->keyword); */
                /*CHARADD (sum, tmpsearch->description); */
                tmpsearch = tmpsearch->next;
            }
        }
    }
    total = sum;
    send_to_char(ch, "%s  world structs: %9d  (%d)\r\n", buf, sum, i);

    sum = top_of_mobt * (sizeof(struct Creature));

    CreatureList::iterator mit = mobilePrototypes.begin();
    for (; mit != mobilePrototypes.end(); ++mit) {
        //for (mob = mob_proto; mob; mob = mob->next) {
        mob = *mit;
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
        tmpdesc = mob->mob_specials.response;
        while (tmpdesc) {
            sum += sizeof(struct extra_descr_data);
            CHARADD(sum, tmpdesc->keyword);
            CHARADD(sum, tmpdesc->description);
            tmpdesc = tmpdesc->next;
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
        //while (chars) {
        if (!IS_NPC(chars)) {
            //chars = chars->next;
            continue;
        }
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
        //chars = chars->next;
    }
    total += sum;
    send_to_char(ch, "%s        mobiles: %9d  (%d)\r\n", buf, sum, i);

    sum = 0;
    i = 0;
    cit = characterList.begin();
    for (; cit != characterList.end(); ++cit) {
        chars = *cit;
        if (IS_NPC(chars)) {
            //chars = chars->next;
            continue;
        }
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
        tmpdesc = chars->mob_specials.response;
        while (tmpdesc) {
            sum += sizeof(struct extra_descr_data);
            CHARADD(sum, tmpdesc->keyword);
            CHARADD(sum, tmpdesc->description);
            tmpdesc = tmpdesc->next;
        }
        fol = chars->followers;
        while (fol) {
            sum += sizeof(struct follow_type);
            fol = fol->next;
        }
        //chars = chars->next;
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

    send_to_char(ch, "Zone name: %s%s%s   #%s%d%s\r\n",
        CCCYN(ch, C_NRM), zone->name, CCNRM(ch, C_NRM),
        CCYEL(ch, C_NRM), zone->number, CCNRM(ch, C_NRM));
    send_to_char(ch,
        "Lifespan: %d  Age: %d  Rooms: [%d-%d]  Reset Mode: %s\r\n",
        zone->lifespan, zone->age, zone->number * 100, zone->top,
        reset_mode[zone->reset_mode]);

    send_to_char(ch, "TimeFrame: [%s]  Plane: [%s]   ",
        time_frames[zone->time_frame], planes[zone->plane]);

    CreatureList::iterator mit = characterList.begin();
    for (; mit != characterList.end(); ++mit)
        if (IS_NPC((*mit)) && (*mit)->in_room && (*mit)->in_room->zone == zone) {
            numm++;
            av_lev += GET_LEVEL((*mit));
        }

    if (numm)
        av_lev /= numm;
    mit = mobilePrototypes.begin();
    for (; mit != mobilePrototypes.end(); ++mit)
        if (GET_MOB_VNUM((*mit)) >= zone->number * 100 &&
            GET_MOB_VNUM((*mit)) <= zone->top && IS_NPC((*mit))) {
            numm_proto++;
            av_lev_proto += GET_LEVEL((*mit));
        }

    if (numm_proto)
        av_lev_proto /= numm_proto;


    send_to_char(ch, "Owner: %s  ", (get_name_by_id(zone->owner_idnum) ?
            get_name_by_id(zone->owner_idnum) : "None"));
    send_to_char(ch, "Co-Owner: %s  \r\n", (get_name_by_id(zone->co_owner_idnum) ?
            get_name_by_id(zone->co_owner_idnum) : "None"));

    send_to_char(ch, "Hours: [%3d]  Years: [%3d]  Idle:[%3d]\r\n",
        zone->hour_mod, zone->year_mod, zone->idle_time);

    send_to_char(ch,
        "Sun: [%s (%d)] Sky: [%s (%d)] Moon: [%s (%d)]\r\n"
        "Pres: [%3d] Chng: [%3d]\r\n\r\n",
        sun_types[(int)zone->weather->sunlight],
        zone->weather->sunlight, sky_types[(int)zone->weather->sky],
        zone->weather->sky,
        moon_sky_types[(int)zone->weather->moonlight],
        zone->weather->moonlight,
        zone->weather->pressure, zone->weather->change);

    sprintbit(zone->flags, zone_flags, buf2);
    send_to_char(ch, "Flags: %s%s%s\r\n", CCGRN(ch, C_NRM), buf2, CCNRM(ch,
            C_NRM));

    send_to_char(ch, "Created: N/A\r\nLast Updated at: N/A\r\n\r\n"
        "Zone special procedure: N/A\r\nZone reset special procedure: N/A\r\n\r\n");

    for (obj = object_list; obj; obj = obj->next)
        if (obj->in_room && obj->in_room->zone == zone)
            numo++;

    for (obj = obj_proto; obj; obj = obj->next)
        if (GET_OBJ_VNUM(obj) >= zone->number * 100 &&
            GET_OBJ_VNUM(obj) <= zone->top)
            numo_proto++;

    for (plr = descriptor_list; plr; plr = plr->next)
        if (plr->character && plr->character->in_room &&
            plr->character->in_room->zone == zone)
            nump++;

    for (rm = zone->world, numr = 0, nums = 0; rm; numr++, rm = rm->next) {
        if (!rm->description)
            numur++;
        for (srch = rm->search; srch; nums++, srch = srch->next);
    }

    send_to_char(ch, "Zone Stats :-\r\n"
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

    mytime = time(0);
    sprintf(buf, "Trail data for room [%6d]:\r\n", ch->in_room->number);

    for (i = 0, trail = ch->in_room->trail; trail; trail = trail->next) {
        timediff = mytime - trail->time;
        sprintbit(trail->flags, trail_flags, buf2);
        sprintf(buf, "%s [%2d] -- Name: '%s', (%s), Idnum: [%5d]\r\n"
            "         Time Passed: %ld minutes, %ld seconds.\r\n"
            "         From dir: %s, To dir: %s, Track: [%2d]\r\n"
            "         Flags: %s\r\n", buf,
            ++i, trail->name,
            get_char_in_world_by_idnum(trail->idnum) ? "in world" : "gone",
            trail->idnum, timediff / 60, timediff % 60,
            (trail->from_dir >= 0) ? dirs[(int)trail->from_dir] : "NONE",
            (trail->to_dir >= 0) ? dirs[(int)trail->to_dir] : "NONE",
            trail->track, buf2);
    }

    page_string(ch->desc, buf);
}

void
do_stat_room(struct Creature *ch, char *roomstr)
{
    int tmp;
    struct extra_descr_data *desc;
    struct room_data *rm = ch->in_room;
    struct room_affect_data *aff = NULL;
    struct special_search_data *cur_search = NULL;
    char out_buf[MAX_STRING_LENGTH];
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
    sprintf(out_buf, "Room name: %s%s%s\r\n", CCCYN(ch, C_NRM), rm->name,
        CCNRM(ch, C_NRM));

    sprinttype(rm->sector_type, sector_types, buf2);
    sprintf(buf,
        "Zone: [%s%3d%s], VNum: [%s%5d%s], Type: %s, Lighting: [%d], Max: [%d]\r\n",
        CCYEL(ch, C_NRM), rm->zone->number, CCNRM(ch, C_NRM),
        CCGRN(ch, C_NRM), rm->number, CCNRM(ch, C_NRM), buf2,
        rm->light, rm->max_occupancy);
    strcat(out_buf, buf);

    sprintbit((long)rm->room_flags, room_bits, buf2);
    sprintf(buf, "SpecProc: %s, Flags: %s\r\n",
        (rm->func == NULL) ? "None" :
        (i = find_spec_index_ptr(rm->func)) < 0 ? "Exists" :
        spec_list[i].tag, buf2);
    strcat(out_buf, buf);

    if (FLOW_SPEED(rm)) {
        sprintf(buf, "Flow (Direction: %s, Speed: %d, Type: %s (%d)).\r\n",
            dirs[(int)FLOW_DIR(rm)], FLOW_SPEED(rm),
            flow_types[(int)FLOW_TYPE(rm)], (int)FLOW_TYPE(rm));
        strcat(out_buf, buf);
    }

    strcat(out_buf, "Description:\r\n");
    if (rm->description)
        strcat(out_buf, rm->description);
    else
        strcat(out_buf, "  None.\r\n");

    if (rm->sounds) {
        sprintf(buf, "%sSound:%s\r\n%s", CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
            rm->sounds);
        strcat(out_buf, buf);
    }

    if (rm->ex_description) {
        sprintf(buf, "Extra descs:%s", CCCYN(ch, C_NRM));
        for (desc = rm->ex_description; desc; desc = desc->next) {
            strcat(buf, " ");
            strcat(buf, desc->keyword);
            if (desc->next);
            strcat(buf, ";");
        }
        strcat(buf, CCNRM(ch, C_NRM));
        strcat(buf, "\r\n");
        strcat(out_buf, buf);
    }

    if (rm->affects) {
        strcpy(buf, "Room affects:\r\n");
        for (aff = rm->affects; aff; aff = aff->next) {
            if (aff->type == RM_AFF_FLAGS) {
                sprintbit((long)aff->flags, room_bits, buf2);
                sprintf(buf, "%s  Room flags: %s%s%s, duration[%d]\r\n",
                    buf, CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM),
                    aff->duration);
            } else if (aff->type < NUM_DIRS) {
                sprintbit((long)aff->flags, exit_bits, buf2);
                sprintf(buf, "%s  Door flags exit [%s], %s, duration[%d]\r\n",
                    buf, dirs[(int)aff->type], buf2, aff->duration);
            } else
                sprintf(buf, "%s  ERROR: Type %d.\r\n", buf, aff->type);

            sprintf(buf, "%s  Desc: %s\r\n", buf, aff->description ?
                aff->description : "None.");
        }
        strcat(out_buf, buf);
    }

    sprintf(buf, "Chars present:%s", CCYEL(ch, C_NRM));

    CreatureList::iterator it = rm->people.begin();
    CreatureList::iterator nit = it;
    for (found = 0; it != rm->people.end(); ++it) {
        ++nit;
        k = *it;
        if (!can_see_creature(ch, k))
            continue;
        sprintf(buf2, "%s %s(%s)", found++ ? "," : "", GET_NAME(k),
            (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")));
        strcat(buf, buf2);
        if (strlen(buf) >= 62) {
            if (nit != rm->people.end())
                strcat(out_buf, strcat(buf, ",\r\n"));
            else
                strcat(out_buf, strcat(buf, "\r\n"));
            *buf = found = 0;
        }
    }

    if (*buf)
        strcat(out_buf, strcat(buf, "\r\n"));
    strcat(out_buf, CCNRM(ch, C_NRM));

    if (rm->contents) {
        sprintf(buf, "Contents:%s", CCGRN(ch, C_NRM));
        for (found = 0, j = rm->contents; j; j = j->next_content) {
            if (!can_see_object(ch, j))
                continue;
            sprintf(buf2, "%s %s", found++ ? "," : "", j->short_description);
            strcat(buf, buf2);
            if (strlen(buf) >= 62) {
                if (j->next_content)
                    strcat(out_buf, strcat(buf, ",\r\n"));
                else
                    strcat(out_buf, strcat(buf, "\r\n"));
                *buf = found = 0;
            }
        }

        if (*buf)
            strcat(out_buf, strcat(buf, "\r\n"));
        strcat(out_buf, CCNRM(ch, C_NRM));
    }

    if (rm->search) {
        strcat(out_buf, "SEARCHES:\r\n");
        for (cur_search = rm->search; cur_search;
            cur_search = cur_search->next) {

            print_search_data_to_buf(ch, rm, cur_search, buf);
            strcat(out_buf, buf);
        }
    }

    for (i = 0; i < NUM_OF_DIRS; i++) {
        if (rm->dir_option[i]) {
            if (rm->dir_option[i]->to_room == NULL) {
                sprintf(buf1, " %sNONE%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
            } else {
                sprintf(buf1, "%s%5d%s", CCCYN(ch, C_NRM),
                    rm->dir_option[i]->to_room->number, CCNRM(ch, C_NRM));
            }
            sprintbit(rm->dir_option[i]->exit_info, exit_bits, buf2);
            sprintf(buf,
                "Exit %s%-5s%s:  To: [%s], Key: [%5d], Keywrd: %s, Type: %s\r\n",
                CCCYN(ch, C_NRM), dirs[i], CCNRM(ch, C_NRM), buf1,
                rm->dir_option[i]->key, rm->dir_option[i]->keyword ?
                rm->dir_option[i]->keyword : "None", buf2);
            strcat(out_buf, buf);
            if (rm->dir_option[i]->general_description) {
                strcpy(buf, rm->dir_option[i]->general_description);
            } else {
                strcpy(buf, "  No exit description.\r\n");
            }
            strcat(out_buf, buf);
        }
    }

    page_string(ch->desc, out_buf);
}



void
do_stat_object(struct Creature *ch, struct obj_data *j)
{
    int i, found;
    struct extra_descr_data *desc;
    extern struct attack_hit_type attack_hit_text[];
    struct room_data *rm = NULL;

    if (IS_OBJ_TYPE(j, ITEM_NOTE) && isname("letter", j->name)) {
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

    send_to_char(ch, "Name: '%s%s%s', Aliases: %s\r\n", CCGRN(ch, C_NRM),
        ((j->short_description) ? j->short_description : "<None>"),
        CCNRM(ch, C_NRM), j->name);
    sprinttype(GET_OBJ_TYPE(j), item_types, buf1);

    strcpy(buf2, j->shared->func ?
        ((i = find_spec_index_ptr(j->shared->func)) < 0 ? "Exists" :
            spec_list[i].tag) : "None");

    sprintf(buf,
        "VNum: [%s%5d%s], Exist: [%3d/%3d], Type: %s, SpecProc: %s\r\n",
        CCGRN(ch, C_NRM), GET_OBJ_VNUM(j), CCNRM(ch, C_NRM), j->shared->number,
        j->shared->house_count, buf1, buf2);
    send_to_char(ch, "%s", buf);
    send_to_char(ch, "L-Des: %s%s%s\r\n", CCGRN(ch, C_NRM),
        ((j->description) ? j->description : "None"), CCNRM(ch, C_NRM));

    if (j->action_description) {
        send_to_char(ch, "Action desc: %s\r\n", j->action_description);
    }

    if (j->ex_description) {
        sprintf(buf, "Extra descs:%s", CCCYN(ch, C_NRM));
        for (desc = j->ex_description; desc; desc = desc->next) {
            strcat(buf, " ");
            strcat(buf, desc->keyword);
            strcat(buf, ";");
        }
        strcat(buf, CCNRM(ch, C_NRM));
        send_to_char(ch, strcat(buf, "\r\n"));
    }

    if (!j->description) {
        send_to_char(ch, "**This object currently has no description**\r\n");
    }

    send_to_char(ch, "Can be worn on: ");
    sprintbit(j->obj_flags.wear_flags, wear_bits, buf);
    strcat(buf, "\r\n");
    send_to_char(ch, "%s", buf);
    strcpy(buf, "Set char bits : ");
    found = FALSE;
    if (j->obj_flags.bitvector[0]) {
        sprintbit(j->obj_flags.bitvector[0], affected_bits, buf2);
        strcat(buf, strcat(buf2, "  "));
        found = TRUE;
    }
    if (j->obj_flags.bitvector[1]) {
        sprintbit(j->obj_flags.bitvector[1], affected2_bits, buf2);
        strcat(buf, strcat(buf2, "  "));
        found = TRUE;
    }
    if (j->obj_flags.bitvector[2]) {
        sprintbit(j->obj_flags.bitvector[2], affected3_bits, buf2);
        strcat(buf, buf2);
        found = TRUE;
    }
    if (found) {
        strcat(buf, "\r\n");
        send_to_char(ch, "%s", buf);
    }

    send_to_char(ch, "Extra flags   : ");
    sprintbit(GET_OBJ_EXTRA(j), extra_bits, buf);
    strcat(buf, "\r\n");
    send_to_char(ch, "%s", buf);

    send_to_char(ch, "Extra2 flags   : ");
    sprintbit(GET_OBJ_EXTRA2(j), extra2_bits, buf);
    strcat(buf, "\r\n");
    send_to_char(ch, "%s", buf);

    send_to_char(ch, "Extra3 flags   : ");
    sprintbit(GET_OBJ_EXTRA3(j), extra3_bits, buf);
    strcat(buf, "\r\n");
    send_to_char(ch, "%s", buf);

    send_to_char(ch, "Weight: %d, Cost: %d (%d), Rent: %d, Timer: %d\r\n",
        j->getWeight(), GET_OBJ_COST(j),
        prototype_obj_value(j), GET_OBJ_RENT(j), GET_OBJ_TIMER(j));

    if ((rm = where_obj(j))) {
        send_to_char(ch, "Absolute location: %s (%d)\r\n", rm->name, rm->number);

        strcpy(buf, "In room: ");
        strcat(buf, CCCYN(ch, C_NRM));
        if (j->in_room == NULL)
            strcat(buf, "NO");
        else {
            sprintf(buf2, "%d", j->in_room ? j->in_room->number : -1);
            strcat(buf, buf2);
        }
        strcat(buf, CCNRM(ch, C_NRM));
        strcat(buf, ", In obj: ");
        sprintf(buf, "%s%s%s%s", buf, CCGRN(ch, C_NRM),
            j->in_obj ? j->in_obj->short_description : "N", CCNRM(ch, C_NRM));
        strcat(buf, ", Carry: ");
        sprintf(buf, "%s%s%s%s", buf, CCYEL(ch, C_NRM),
            j->carried_by ? GET_NAME(j->carried_by) : "N", CCNRM(ch, C_NRM));
        strcat(buf, ", Worn: ");
        sprintf(buf, "%s%s%s%s%s", buf, CCYEL(ch, C_NRM),
            j->worn_by ? GET_NAME(j->worn_by) : "N",
            ((!j->worn_by || j == GET_EQ(j->worn_by, j->worn_on)) ?
                "" : " (impl)"), CCNRM(ch, C_NRM));
        strcat(buf, ", Aux: ");
        sprintf(buf, "%s%s%s%s", buf, CCGRN(ch, C_NRM),
            j->aux_obj ? j->aux_obj->short_description : "N", CCNRM(ch,
                C_NRM));
        strcat(buf, "\r\n");
        send_to_char(ch, "%s", buf);
    }
    sprintf(buf,
        "Material: [%s%s%s (%d)], Maxdamage: [%d (%d)], Damage: [%d]\r\n",
        CCYEL(ch, C_NRM), material_names[GET_OBJ_MATERIAL(j)], CCNRM(ch,
            C_NRM), GET_OBJ_MATERIAL(j), GET_OBJ_MAX_DAM(j), set_maxdamage(j),
        GET_OBJ_DAM(j));
    send_to_char(ch, "%s", buf);

    switch (GET_OBJ_TYPE(j)) {
    case ITEM_LIGHT:
        sprintf(buf, "Color: [%d], Type: [%d], Hours: [%d], Value3: [%d]",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2),
            GET_OBJ_VAL(j, 3));
        break;
    case ITEM_SCROLL:
    case ITEM_POTION:
    case ITEM_PILL:
    case ITEM_SYRINGE:
        sprintf(buf, "Level: %d, Spells: %s(%d), %s(%d), %s(%d)",
            GET_OBJ_VAL(j, 0),
            (GET_OBJ_VAL(j, 1) > 0) ? spell_to_str((int)GET_OBJ_VAL(j, 1)) :
            "None", GET_OBJ_VAL(j, 1),
            (GET_OBJ_VAL(j, 2) > 0) ? spell_to_str((int)GET_OBJ_VAL(j, 2)) :
            "None", GET_OBJ_VAL(j, 2),
            (GET_OBJ_VAL(j, 3) > 0) ? spell_to_str((int)GET_OBJ_VAL(j, 3)) :
            "None", GET_OBJ_VAL(j, 3));
        break;
    case ITEM_WAND:
    case ITEM_STAFF:
        sprintf(buf,
            "Level: %d, Max Charge: %d, Current Charge: %d, Spell: %s",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2),
            spell_to_str((int)GET_OBJ_VAL(j, 3)));
        break;
    case ITEM_FIREWEAPON:
    case ITEM_WEAPON:
        sprintf(buf,
            "Spell: %s (%d), Todam: %dd%d (av %d), Damage Type: %s (%d)",
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
    case ITEM_MISSILE:
        sprintf(buf, "Tohit: %d, Todam: %d, Type: %d", GET_OBJ_VAL(j, 0),
            GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 3));
        break;
    case ITEM_ARMOR:
        sprintf(buf, "AC-apply: [%d]", GET_OBJ_VAL(j, 0));
        break;
    case ITEM_TRAP:
        sprintf(buf, "Spell: %d, - Hitpoints: %d",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
        break;
    case ITEM_CONTAINER:
        sprintf(buf,
            "Max-contains: %d, Locktype: %d, Key/Owner: %d, Corpse: %s, Killer: %d",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2),
            GET_OBJ_VAL(j, 3) ? "Yes" : "No", CORPSE_KILLER(j));
        break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
        sprinttype(GET_OBJ_VAL(j, 2), drinks, buf2);
        sprintf(buf,
            "Max-contains: %d, Contains: %d, Liquid: %s(%d), Poisoned: %s (%d)",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), buf2, GET_OBJ_VAL(j, 2),
            GET_OBJ_VAL(j, 3) ? "Yes" : "No", GET_OBJ_VAL(j, 3));
        break;
        /*  case ITEM_NOTE:
           sprintf(buf, "Tongue: %d", GET_OBJ_VAL(j, 0));
           break; */
    case ITEM_KEY:
        sprintf(buf, "Keytype: %d, Rentable: %s, Car Number: %d",
            GET_OBJ_VAL(j, 0), YESNO(GET_OBJ_VAL(j, 1)), GET_OBJ_VAL(j, 2));
        break;
    case ITEM_FOOD:
        sprintf(buf,
            "Makes full: %d, Value 1: %d, Spell : %s(%d), Poisoned: %d",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), spell_to_str((int)GET_OBJ_VAL(j,
                    2)), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
        break;
    case ITEM_HOLY_SYMB:
        sprintf(buf,
            "Alignment: %s(%d), Class: %s(%d), Min Level: %d, Max Level: %d ",
            alignments[GET_OBJ_VAL(j, 0)], GET_OBJ_VAL(j, 0),
            char_class_abbrevs[(int)GET_OBJ_VAL(j, 1)], GET_OBJ_VAL(j, 1),
            GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
        break;
    case ITEM_BATTERY:
        sprintf(buf, "Max_Energy: %d, Cur_Energy: %d, Rate: %d, Cost/Unit: %d",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1),
            GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
        break;
    case ITEM_VEHICLE:
        sprintbit(GET_OBJ_VAL(j, 2), vehicle_types, buf2);
        sprintf(buf, "Room/Key Number: %d, Door State: %d, Type: %s, v3: %d",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), buf2, GET_OBJ_VAL(j, 3));
        break;
    case ITEM_ENGINE:
        sprintbit(GET_OBJ_VAL(j, 2), engine_flags, buf2);
        sprintf(buf,
            "Max_Energy: %d, Cur_Energy: %d, Run_State: %s, Consume_rate: %d",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), buf2, GET_OBJ_VAL(j, 3));
        break;
    case ITEM_BOMB:
        sprintf(buf, "Values: %s:[%s(%d)] %s:[%d] %s:[%d] %s:[%d]",
            item_value_types[(int)GET_OBJ_TYPE(j)][0],
            bomb_types[(int)GET_OBJ_VAL(j, 0)], GET_OBJ_VAL(j, 0),
            item_value_types[(int)GET_OBJ_TYPE(j)][1], GET_OBJ_VAL(j, 1),
            item_value_types[(int)GET_OBJ_TYPE(j)][2], GET_OBJ_VAL(j, 2),
            item_value_types[(int)GET_OBJ_TYPE(j)][3], GET_OBJ_VAL(j, 3));
        break;
    case ITEM_FUSE:
        sprintf(buf, "Values: %s:[%s(%d)] %s:[%d] %s:[%d] %s:[%d]",
            item_value_types[(int)GET_OBJ_TYPE(j)][0],
            fuse_types[(int)GET_OBJ_VAL(j, 0)], GET_OBJ_VAL(j, 0),
            item_value_types[(int)GET_OBJ_TYPE(j)][1], GET_OBJ_VAL(j, 1),
            item_value_types[(int)GET_OBJ_TYPE(j)][2], GET_OBJ_VAL(j, 2),
            item_value_types[(int)GET_OBJ_TYPE(j)][3], GET_OBJ_VAL(j, 3));
        break;

    case ITEM_TOBACCO:
        sprintf(buf, "Values: %s:[%s(%d)] %s:[%d] %s:[%d] %s:[%d]",
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
        sprintf(buf, "Values 0-3: %s:[%d] %s:[%d] %s:[%d] %s:[%s (%d)]",
            item_value_types[(int)GET_OBJ_TYPE(j)][0], GET_OBJ_VAL(j, 0),
            item_value_types[(int)GET_OBJ_TYPE(j)][1], GET_OBJ_VAL(j, 1),
            item_value_types[(int)GET_OBJ_TYPE(j)][2], GET_OBJ_VAL(j, 2),
            item_value_types[(int)GET_OBJ_TYPE(j)][3],
            (GET_OBJ_VAL(j, 3) >= 0 && GET_OBJ_VAL(j, 3) < NUM_GUN_TYPES) ?
            gun_types[GET_OBJ_VAL(j, 3)] : "ERROR", GET_OBJ_VAL(j, 3));
        break;
    case ITEM_INTERFACE:
        sprintf(buf, "Values 0-3: %s:[%s (%d)] %s:[%d] %s:[%d] %s:[%d]",
            item_value_types[(int)GET_OBJ_TYPE(j)][0],
            GET_OBJ_VAL(j, 0) >= 0 && GET_OBJ_VAL(j, 0) < NUM_INTERFACES ?
            interface_types[GET_OBJ_VAL(j, 0)] : "Error",
            GET_OBJ_VAL(j, 0),
            item_value_types[(int)GET_OBJ_TYPE(j)][1], GET_OBJ_VAL(j, 1),
            item_value_types[(int)GET_OBJ_TYPE(j)][2], GET_OBJ_VAL(j, 2),
            item_value_types[(int)GET_OBJ_TYPE(j)][3], GET_OBJ_VAL(j, 3));
        break;
    case ITEM_MICROCHIP:
        sprintf(buf, "Values 0-3: %s:[%s (%d)] %s:[%d] %s:[%d] %s:[%d]",
            item_value_types[(int)GET_OBJ_TYPE(j)][0],
            GET_OBJ_VAL(j, 0) >= 0 && GET_OBJ_VAL(j, 0) < NUM_CHIPS ?
            microchip_types[GET_OBJ_VAL(j, 0)] : "Error",
            GET_OBJ_VAL(j, 0),
            item_value_types[(int)GET_OBJ_TYPE(j)][1], GET_OBJ_VAL(j, 1),
            item_value_types[(int)GET_OBJ_TYPE(j)][2], GET_OBJ_VAL(j, 2),
            item_value_types[(int)GET_OBJ_TYPE(j)][3], GET_OBJ_VAL(j, 3));
        break;

    default:
        sprintf(buf, "Values 0-3: %s:[%d] %s:[%d] %s:[%d] %s:[%d]",
            item_value_types[(int)GET_OBJ_TYPE(j)][0], GET_OBJ_VAL(j, 0),
            item_value_types[(int)GET_OBJ_TYPE(j)][1], GET_OBJ_VAL(j, 1),
            item_value_types[(int)GET_OBJ_TYPE(j)][2], GET_OBJ_VAL(j, 2),
            item_value_types[(int)GET_OBJ_TYPE(j)][3], GET_OBJ_VAL(j, 3));
        break;
    }

    send_to_char(ch, "%s", buf);

    if (j->contains) {

        send_to_char(ch, "\r\nContents:\r\n");
        list_obj_to_char(j->contains, ch, 2, TRUE);

    } else
        send_to_char(ch, "\r\n");
    found = 0;
    send_to_char(ch, "Affections:");
    for (i = 0; i < MAX_OBJ_AFFECT; i++)
        if (j->affected[i].modifier) {
            sprinttype(j->affected[i].location, apply_types, buf2);
            send_to_char(ch, "%s %+d to %s", found++ ? "," : "",
                j->affected[i].modifier, buf2);
        }
    if (!found)
        send_to_char(ch, " None");

    send_to_char(ch, "\r\n");

    if (OBJ_SOILAGE(j)) {
        found = FALSE;
        strcpy(buf2, "Soilage: ");
        for (i = 0; i < 16; i++) {
            if (OBJ_SOILED(j, (1 << i))) {
                if (found)
                    strcat(buf2, ", ");
                else
                    found = TRUE;
                strcat(buf2, soilage_bits[i]);
            }
        }
        send_to_char(ch, "%s\r\n", buf2);
    }

    if (GET_OBJ_SIGIL_IDNUM(j)) {
        send_to_char(ch, "Warding Sigil: %s (%d), level %d.\r\n",
            get_name_by_id(GET_OBJ_SIGIL_IDNUM(j)), GET_OBJ_SIGIL_IDNUM(j),
            GET_OBJ_SIGIL_LEVEL(j));
    }
}


void
do_stat_character(struct Creature *ch, struct Creature *k)
{
    int i, num, num2, found = 0, rexp;
    struct follow_type *fol;
    struct affected_type *aff;
    extern struct attack_hit_type attack_hit_text[];
    char outbuf[MAX_STRING_LENGTH];

    if (GET_MOB_SPEC(k) == fate && GET_LEVEL(ch) < LVL_SPIRIT) {
        send_to_char(ch, "You can't stat this mob.\r\n");
        return;
    }
    switch (GET_SEX(k)) {
    case SEX_NEUTRAL:
        strcpy(outbuf, "NEUTRAL-SEX");
        break;
    case SEX_MALE:
        strcpy(outbuf, "MALE");
        break;
    case SEX_FEMALE:
        strcpy(outbuf, "FEMALE");
        break;
    default:
        strcpy(outbuf, "ILLEGAL-SEX!!");
        break;
    }

    sprintf(buf2, " %s '%s%s%s'  IDNum: [%5ld], In room %s[%s%5d%s]%s",
        (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")),
        CCYEL(ch, C_NRM), GET_NAME(k), CCNRM(ch, C_NRM),
        IS_NPC(k) ? MOB_IDNUM(k) : GET_IDNUM(k),
        CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), k->in_room ?
        k->in_room->number : -1, CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));

    if (!IS_MOB(k) && GET_LEVEL(k) >= LVL_AMBASSADOR)
        sprintf(buf2, "%s, OlcObj: [%d], OlcMob: [%d]\r\n", buf2,
            (GET_OLC_OBJ(k) ? GET_OLC_OBJ(k)->shared->vnum : (-1)),
            (GET_OLC_MOB(k) ?
                GET_OLC_MOB(k)->mob_specials.shared->vnum : (-1)));
    else
        strcat(buf2, "\r\n");

    strcat(outbuf, buf2);

    if (IS_MOB(k)) {
        sprintf(buf,
            "Alias: %s, VNum: %s[%s%5d%s]%s, Exist: [%3d], SVNum: %s[%s%5d%s]%s\r\n",
            k->player.name, CCGRN(ch, C_NRM), CCYEL(ch, C_NRM),
            GET_MOB_VNUM(k), CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
            k->mob_specials.shared->number, CCGRN(ch, C_NRM), CCYEL(ch, C_NRM),
            GET_SCRIPT_VNUM(k), CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
        strcat(outbuf, buf);
        sprintf(buf, "L-Des: %s%s%s\r\n", CCYEL(ch, C_NRM),
            (k->player.long_descr ? k->player.long_descr : "<None>"),
            CCNRM(ch, C_NRM));
        strcat(outbuf, buf);
    } else {
        sprintf(buf, "Title: %s\r\n",
            (k->player.title ? k->player.title : "<None>"));
        strcat(outbuf, buf);
    }

/* Race, Class */
    strcpy(buf, "Race: ");
    sprinttype(k->player.race, player_race, buf2);
    strcat(outbuf, strcat(buf, buf2));
    strcpy(buf, ", Class: ");
    sprinttype(k->player.char_class, pc_char_class_types, buf2);
    strcat(buf, buf2);
    sprintf(buf2, "/%s", IS_REMORT(k) ?
        pc_char_class_types[(int)GET_REMORT_CLASS(k)] : "None");
    strcat(buf, buf2);
    if (!IS_NPC(k)) {
        sprintf(buf2, " Gen: %d,", GET_REMORT_GEN(k));
        strcat(buf, buf2);
    }
    if (IS_VAMPIRE(k) || IS_CYBORG(k)) {
        sprintf(buf2, " Subclass: %s", IS_CYBORG(k) ?
            borg_subchar_class_names[GET_OLD_CLASS(k)] : IS_VAMPIRE(k) ?
            pc_char_class_types[GET_OLD_CLASS(k)] : "None");
        strcat(buf, buf2);
    }
    strcat(outbuf, strcat(buf, "\r\n"));

    if (!IS_NPC(k))
        sprintf(buf, "Lev: [%s%2d%s], XP: [%s%7d%s/%s%d%s], Align: [%4d]\r\n",
            CCYEL(ch, C_NRM), GET_LEVEL(k), CCNRM(ch, C_NRM),
            CCYEL(ch, C_NRM), GET_EXP(k), CCNRM(ch, C_NRM),
            CCCYN(ch, C_NRM), exp_scale[GET_LEVEL(k) + 1] - GET_EXP(k),
            CCNRM(ch, C_NRM), GET_ALIGNMENT(k));
    else {
        rexp = mobile_experience(k);
        sprintf(buf,
            "Lev: [%s%2d%s], XP: [%s%7d%s/%s%d%s] %s(%s%3d p%s)%s, Align: [%4d]\r\n",
            CCYEL(ch, C_NRM), GET_LEVEL(k), CCNRM(ch, C_NRM), CCYEL(ch, C_NRM),
            GET_EXP(k), CCNRM(ch, C_NRM), CCCYN(ch, C_NRM), rexp, CCNRM(ch,
                C_NRM), CCRED(ch, C_NRM), CCNRM(ch, C_NRM),
            GET_EXP(k) > 10000000 ? (((unsigned int)GET_EXP(k)) / MAX(1,
                    rexp / 100)) : (((unsigned int)(GET_EXP(k) * 100)) / MAX(1,
                    rexp)), CCRED(ch, C_NRM), CCNRM(ch, C_NRM),
            GET_ALIGNMENT(k));
    }
    strcat(outbuf, buf);

    sprintf(buf, "Height:  %d centimeters , Weight: %d pounds.\r\n",
        GET_HEIGHT(k), GET_WEIGHT(k));
    strcat(outbuf, buf);

    if (!IS_NPC(k)) {
        strcpy(buf1, (char *)asctime(localtime(&(k->player.time.birth))));
        strcpy(buf2, (char *)asctime(localtime(&(k->player.time.logon))));
        buf1[10] = buf2[10] = '\0';

        sprintf(buf, "Created: [%s], Last Logon: [%s], Played [%dh %dm], Age [%d]\r\n", buf1, buf2, k->player.time.played / 3600, ((k->player.time.played / 3600) % 60), GET_AGE(k));    //age(k).year);
        strcat(outbuf, buf);

        sprintf(buf,
            "Hometown:[%s%s%s (%d)], Loadroom: [%d/%d] %s, Clan: %s%s%s\r\n",
            CCCYN(ch, C_NRM), home_town_abbrevs[(int)GET_HOME(k)], CCNRM(ch,
                C_NRM), GET_HOLD_HOME(k), k->player_specials->saved.load_room,
            GET_HOLD_LOADROOM(k), ONOFF(PLR_FLAGGED(k, PLR_LOADROOM)),
            CCCYN(ch, C_NRM),
            real_clan(GET_CLAN(k)) ? real_clan(GET_CLAN(k))->name : "NONE",
            CCNRM(ch, C_NRM));
        strcat(outbuf, buf);

        sprintf(buf,
            "PRAC[%d] (%d/prac) (%.2f/level), Life[%d], Qpoints[%d/%d], Thac0[%d]\r\n",
            GET_PRACTICES(k), skill_gain(k, FALSE), prac_gain(k, FALSE),
            GET_LIFE_POINTS(k), GET_QUEST_POINTS(k), GET_QUEST_ALLOWANCE(k),
            (int)MIN(THACO(GET_CLASS(k), GET_LEVEL(k)), 
                     THACO(GET_REMORT_CLASS(k), GET_LEVEL(k))));
        strcat(outbuf, buf);

        sprintf(buf,
            "   %sMobKills:%s [ %4d ], %sPKills:%s [ %4d ], %sDeaths:%s [ %4d ]\r\n",
            CCYEL(ch, C_NRM), CCNRM(ch, C_NRM), GET_MOBKILLS(k), CCRED(ch,
                C_NRM), CCNRM(ch, C_NRM), GET_PKILLS(k), CCGRN(ch, C_NRM),
            CCNRM(ch, C_NRM), GET_PC_DEATHS(k));
        strcat(outbuf, buf);
    }
    sprintf(buf, "Str: [%s%d/%d%s]  Int: [%s%d%s]  Wis: [%s%d%s]  "
        "Dex: [%s%d%s]  Con: [%s%d%s]  Cha: [%s%d%s]\r\n",
        CCCYN(ch, C_NRM), GET_STR(k), GET_ADD(k), CCNRM(ch, C_NRM),
        CCCYN(ch, C_NRM), GET_INT(k), CCNRM(ch, C_NRM),
        CCCYN(ch, C_NRM), GET_WIS(k), CCNRM(ch, C_NRM),
        CCCYN(ch, C_NRM), GET_DEX(k), CCNRM(ch, C_NRM),
        CCCYN(ch, C_NRM), GET_CON(k), CCNRM(ch, C_NRM),
        CCCYN(ch, C_NRM), GET_CHA(k), CCNRM(ch, C_NRM));
    strcat(outbuf, buf);
    if (k->in_room || !IS_NPC(k)) {    // Real Mob/Char
        sprintf(buf,
            "Hit p.:[%s%d/%d+%d%s]  Mana p.:[%s%d/%d+%d%s]  Move p.:[%s%d/%d+%d%s]\r\n",
            CCGRN(ch, C_NRM), GET_HIT(k), GET_MAX_HIT(k), hit_gain(k),
            CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), GET_MANA(k), GET_MAX_MANA(k),
            mana_gain(k), CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), GET_MOVE(k),
            GET_MAX_MOVE(k), move_gain(k), CCNRM(ch, C_NRM));
        strcat(outbuf, buf);
    } else {                    // Virtual Mob
        sprintf(buf,
            "Hit p.:[%s%dd%d+%d (%d)%s]  Mana p.:[%s%d%s]  Move p.:[%s%d%s]\r\n",
            CCGRN(ch, C_NRM), GET_HIT(k), GET_MANA(k), GET_MOVE(k),
            (GET_HIT(k) * (GET_MANA(k) + 1) / 2) + GET_MOVE(k), CCNRM(ch,
                C_NRM), CCGRN(ch, C_NRM), GET_MAX_MANA(k), CCNRM(ch, C_NRM),
            CCGRN(ch, C_NRM), GET_MAX_MOVE(k), CCNRM(ch, C_NRM));
        strcat(outbuf, buf);
    }

    sprintf(buf,
        "AC: [%s%d/10%s], Hitroll: [%s%2d%s], Damroll: [%s%2d%s], Speed: [%s%2d%s], Damage Reduction: [%s%2d%s]\r\n",
        CCYEL(ch, C_NRM), GET_AC(k), CCNRM(ch, C_NRM), CCYEL(ch, C_NRM),
        k->points.hitroll, CCNRM(ch, C_NRM), CCYEL(ch, C_NRM),
        k->points.damroll, CCNRM(ch, C_NRM), CCYEL(ch, C_NRM), k->getSpeed(),
        CCNRM(ch, C_NRM), CCYEL(ch, C_NRM), (int)(k->getDamReduction() * 100),
        CCNRM(ch, C_NRM));
    strcat(outbuf, buf);

    if (!IS_NPC(k) || k->in_room) {
        sprintf(buf,
            "Pr:[%s%2d%s],Rd:[%s%2d%s],Pt:[%s%2d%s],Br:[%s%2d%s],Sp:[%s%2d%s],Ch:[%s%2d%s],Ps:[%s%2d%s],Ph:[%s%2d%s]\r\n",
            CCYEL(ch, C_NRM), GET_SAVE(k, 0), CCNRM(ch, C_NRM),
            CCYEL(ch, C_NRM), GET_SAVE(k, 1), CCNRM(ch, C_NRM),
            CCYEL(ch, C_NRM), GET_SAVE(k, 2), CCNRM(ch, C_NRM),
            CCYEL(ch, C_NRM), GET_SAVE(k, 3), CCNRM(ch, C_NRM),
            CCYEL(ch, C_NRM), GET_SAVE(k, 4), CCNRM(ch, C_NRM),
            CCYEL(ch, C_NRM), GET_SAVE(k, 5), CCNRM(ch, C_NRM),
            CCYEL(ch, C_NRM), GET_SAVE(k, 6), CCNRM(ch, C_NRM),
            CCYEL(ch, C_NRM), GET_SAVE(k, 7), CCNRM(ch, C_NRM));
        strcat(outbuf, buf);
    }
    if (IS_NPC(k))
        sprintf(buf, "Gold:[%8d], Cash:[%8d], (Total: %d)\r\n",
            GET_GOLD(k), GET_CASH(k), GET_GOLD(k) + GET_CASH(k));
    else
        sprintf(buf,
            "Au:[%8d], Bank:[%9d], Cash:[%8d], Enet:[%9d], (Total: %d)\r\n",
            GET_GOLD(k), GET_BANK_GOLD(k), GET_CASH(k), GET_ECONET(k),
            GET_GOLD(k) + GET_BANK_GOLD(k) + GET_ECONET(k) + GET_CASH(k));
    strcat(outbuf, buf);

    if (IS_NPC(k)) {
        sprintf(buf, "Pos: %s, Dpos: %s, Attack: %s",
            position_types[k->getPosition()],
            position_types[(int)k->mob_specials.shared->default_pos],
            attack_hit_text[k->mob_specials.shared->attack_type].singular);
        if (k->in_room)
            sprintf(buf, "%s, %sFT%s: %s, %sHNT%s: %s, Timer: %d", buf,
                CCRED(ch, C_NRM), CCNRM(ch, C_NRM),
                (FIGHTING(k) ? GET_NAME(FIGHTING(k)) : "N"),
                CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
                HUNTING(k) ? PERS(HUNTING(k), ch) : "N",
                k->char_specials.timer);
        strcat(strcat(outbuf, buf), "\r\n");
    } else if (k->in_room) {
        sprintf(buf, "Pos: %s, %sFT%s: %s, %sHNT%s: %s",
            position_types[k->getPosition()],
            CCRED(ch, C_NRM), CCNRM(ch, C_NRM),
            (FIGHTING(k) ? GET_NAME(FIGHTING(k)) : "N"),
            CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
            HUNTING(k) ? PERS(HUNTING(k), ch) : "N");
        strcat(outbuf, buf);
    }
    if (k->desc) {
        sprinttype(k->desc->connected, connected_types, buf2);
        sprintf(buf, "%sConnected: ", ", ");
        strcat(buf, buf2);
        sprintf(buf2, ", Idle [%d]\r\n", k->char_specials.timer);
        strcat(outbuf, strcat(buf, buf2));
    } else {
        strcat( outbuf, "\r\n" );
    }

    if (k->getPosition() == POS_MOUNTED && MOUNTED(k)) {
        strcpy(buf, "Mount: ");
        strcat(outbuf, strcat(strcat(buf, GET_NAME(MOUNTED(k))), "\r\n"));
    }

    if (IS_NPC(k)) {
        sprintbit(MOB_FLAGS(k), action_bits, buf2);
        sprintf(buf, "NPC flags: %s%s%s\r\n", CCCYN(ch, C_NRM), buf2, CCNRM(ch,
                C_NRM));
        strcat(outbuf, buf);
        sprintbit(MOB2_FLAGS(k), action2_bits, buf2);
        sprintf(buf, "NPC flags(2): %s%s%s\r\n", CCCYN(ch, C_NRM), buf2,
            CCNRM(ch, C_NRM));
        strcat(outbuf, buf);

    } else {
        if (GET_LEVEL(ch) >= LVL_CREATOR)
            sprintbit(PLR_FLAGS(k), player_bits, buf2);
        else
            sprintbit(PLR_FLAGS(k) & ~PLR_LOG, player_bits, buf2);
        sprintf(buf, "PLR: %s%s%s\r\n", CCCYN(ch, C_NRM), buf2, CCNRM(ch,
                C_NRM));
        strcat(outbuf, buf);
        sprintbit(PLR2_FLAGS(k), player2_bits, buf2);
        sprintf(buf, "PLR2: %s%s%s\r\n", CCCYN(ch, C_NRM), buf2, CCNRM(ch,
                C_NRM));
        strcat(outbuf, buf);
        sprintbit(PRF_FLAGS(k), preference_bits, buf2);
        sprintf(buf, "PRF: %s%s%s\r\n", CCGRN(ch, C_NRM), buf2, CCNRM(ch,
                C_NRM));
        strcat(outbuf, buf);
        sprintbit(PRF2_FLAGS(k), preference2_bits, buf2);
        sprintf(buf, "PRF2: %s%s%s\r\n", CCGRN(ch, C_NRM), buf2, CCNRM(ch,
                C_NRM));
        strcat(outbuf, buf);
    }

    if (IS_MOB(k)) {
        sprintf(buf,
            "Mob Spec: %s, NPC Dam: %dd%d, Morale: %d, Lair: %d, Ldr: %d\r\n",
            (k->mob_specials.shared->func ? ((i =
                        find_spec_index_ptr(k->mob_specials.shared->func)) <
                    0 ? "Exists" : spec_list[i].tag) : "None"),
            k->mob_specials.shared->damnodice,
            k->mob_specials.shared->damsizedice,
            k->mob_specials.shared->morale, k->mob_specials.shared->lair,
            k->mob_specials.shared->leader);
        strcat(outbuf, buf);

        if (MOB_SHARED(k)->move_buf)
            sprintf(outbuf, "%sMove_buf: %s\r\n", outbuf,
                MOB_SHARED(k)->move_buf);
        if (GET_MOB_PARAM(k))
            sprintf(outbuf, "%sSpec_param: %s\r\n", outbuf,
                GET_MOB_PARAM(k));
        if (k->mob_specials.mug) {
            sprintf(buf,
                "MUGGING:  victim idnum: %d, obj vnum: %d, timer: %d\r\n",
                k->mob_specials.mug->idnum, k->mob_specials.mug->vnum,
                k->mob_specials.mug->timer);
            strcat(outbuf, buf);
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
        sprintf(buf,
            "Encum : (%d inv + %d eq) = (%d tot)/%d, Number: %d/%d inv, %d eq, %d imp\r\n",
            IS_CARRYING_W(k), IS_WEARING_W(k),
            (IS_CARRYING_W(k) + IS_WEARING_W(k)), CAN_CARRY_W(k),
            IS_CARRYING_N(k), (int)CAN_CARRY_N(k), num, num2);
        if (k->getBreathCount() || GET_FALL_COUNT(k)) {
            sprintf(buf, "%sBreath_count: %d, Fall_count: %d", buf,
                k->getBreathCount(), GET_FALL_COUNT(k));
            found = TRUE;
        }
        strcat(outbuf, buf);
    }
    if (!IS_MOB(k)) {
        sprintf(outbuf, "%s%sHunger: %d, Thirst: %d, Drunk: %d\r\n", outbuf,
            found ? ", " : "",
            GET_COND(k, FULL), GET_COND(k, THIRST), GET_COND(k, DRUNK));
    } else if (found)
        strcat(outbuf, "\r\n");

    if (!IS_NPC(k) && GET_QUEST(k)) {
        char* name = "None";
        Quest *quest = quest_by_vnum( GET_QUEST(k) );
        if( quest != NULL && quest->isPlaying(GET_IDNUM(k)) )
            name = quest->name;
        sprintf(outbuf, "%sQuest [%d]: \'%s\'\r\n", outbuf, GET_QUEST(k), name );
    }

    if (k->in_room && (k->master || k->followers)) {
        sprintf(buf, "Master is: %s, Followers are:",
            ((k->master) ? GET_NAME(k->master) : "<none>"));

        for (fol = k->followers; fol; fol = fol->next) {
            sprintf(buf2, "%s %s", found++ ? "," : "", PERS(fol->follower,
                    ch));
            strcat(buf, buf2);
            if (strlen(buf) >= 62) {
                if (fol->next)
                    strcat(outbuf, strcat(buf, "\r\n"));
                else
                    strcat(outbuf, strcat(buf, "\r\n"));
                *buf = found = 0;
            }
        }

        if (*buf)
            strcat(outbuf, strcat(buf, "\r\n"));
    }
    /* Showing the bitvector */
    sprintbit(AFF_FLAGS(k), affected_bits, buf2);
    sprintf(buf, "AFF: %s%s%s\r\n", CCYEL(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
    strcat(outbuf, buf);
    if (AFF2_FLAGS(k)) {
        sprintbit(AFF2_FLAGS(k), affected2_bits, buf2);
        sprintf(buf, "AFF2: %s%s%s\r\n", CCYEL(ch, C_NRM), buf2, CCNRM(ch,
                C_NRM));
        strcat(outbuf, buf);
    }
    if (AFF3_FLAGS(k)) {
        sprintbit(AFF3_FLAGS(k), affected3_bits, buf2);
        sprintf(buf, "AFF3: %s%s%s\r\n", CCYEL(ch, C_NRM), buf2, CCNRM(ch,
                C_NRM));
        strcat(outbuf, buf);
    }
    if (k->getPosition() == POS_SITTING && IS_AFFECTED_2(k, AFF2_MEDITATE)) {
        sprintf(buf, "Meditation Timer: [%d]\r\n", MEDITATE_TIMER(k));
        strcat(outbuf, buf);
    }
    if (IS_CYBORG(k)) {
        sprintf(buf, "Broken component: [%s (%d)], Dam Count: %d/%d.\r\n",
            component_names[(int)GET_BROKE(k)][GET_OLD_CLASS(k)],
            GET_BROKE(k), GET_TOT_DAM(k), max_component_dam(k));
        strcat(outbuf, buf);

        if (AFF3_FLAGGED(k, AFF3_SELF_DESTRUCT)) {
            sprintf(buf, "Self-destruct Timer: [%d]\r\n", MEDITATE_TIMER(k));
            strcat(outbuf, buf);
        }
    }

    /* Routine to show what spells a char is affected by */
    if (k->affected) {
        for (aff = k->affected; aff; aff = aff->next) {
            *buf2 = '\0';
            sprintf(buf, "SPL: (%3d%s) [%2d] %s%-24s%s ", aff->duration + 1,
                aff->is_instant ? "sec" : "hr", aff->level,
                CCCYN(ch, C_NRM), spell_to_str(aff->type), CCNRM(ch, C_NRM));
            if (aff->modifier) {
                sprintf(buf2, "%+d to %s", aff->modifier,
                    apply_types[(int)aff->location]);
                strcat(buf, buf2);
            }
            if (aff->bitvector) {
                if (*buf2)
                    strcat(buf, ", sets ");
                else
                    strcat(buf, "sets ");
                if (aff->aff_index == 3)
                    sprintbit(aff->bitvector, affected3_bits, buf2);
                else if (aff->aff_index == 2)
                    sprintbit(aff->bitvector, affected2_bits, buf2);
                else
                    sprintbit(aff->bitvector, affected_bits, buf2);
                strcat(buf, buf2);
            }
            strcat(outbuf, strcat(buf, "\r\n"));
        }
    }
    page_string(ch->desc, outbuf);
}


void
do_stat_ticl(struct Creature *ch, int vnum)
{
    struct zone_data *zone = NULL;
    struct ticl_data *ticl = NULL;
    char tim1[30], tim2[30];

    for (zone = zone_table; zone; zone = zone->next)
        if (vnum >= zone->number * 100 && vnum <= zone->top)
            break;

    if (!zone) {
        send_to_char(ch, "Sorry, that TICL proc doesn't exist.\r\n");
        return;
    }

    for (ticl = zone->ticl_list; ticl; ticl = ticl->next)
        if (ticl->vnum == vnum)
            break;

    if (!ticl)
        send_to_char(ch, "Sorry, no TICL proc by that vnum.\r\n");
    else {
        strcpy(tim1, (char *)asctime(localtime(&(ticl->date_created))));
        strcpy(tim2, (char *)asctime(localtime(&(ticl->last_modified))));
        tim1[10] = tim2[10] = '\0';
        sprintf(buf, "\r\n"
            "VNUM:  [%s%5d%s]    Title: %s%s%s\r\n"
            "Created By:       [%s%s%s] on [%s%s%s]\r\n"
            "Last Modified By: [%s%s%s] on [%s%s%s]\r\n\r\n"
            "-----------------------------------------------------------------\r\n\r\n",
            CCGRN(ch, C_NRM),
            ticl->vnum,
            CCNRM(ch, C_NRM),
            CCCYN(ch, C_NRM),
            ticl->title,
            CCNRM(ch, C_NRM),
            CCYEL(ch, C_NRM),
            get_name_by_id(ticl->creator),
            CCNRM(ch, C_NRM),
            CCGRN(ch, C_NRM),
            tim1,
            CCNRM(ch, C_NRM),
            CCYEL(ch, C_NRM),
            get_name_by_id(ticl->last_modified_by),
            CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), tim2, CCNRM(ch, C_NRM));

        send_to_char(ch, "%s", buf);

        if (ticl->code != NULL)
            page_string(ch->desc, ticl->code);
    }
}

ACMD(do_stat)
{
    struct Creature *victim = 0;
    struct obj_data *object = 0;
    struct zone_data *zone = NULL;
    struct char_file_u tmp_store;
    int tmp, found;

    half_chop(argument, buf1, buf2);

    if (!*buf1) {
        send_to_char(ch, "Stats on who or what?\r\n");
        return;
    } else if (is_abbrev(buf1, "room")) {
        do_stat_room(ch, buf2);
    } else if (!strncmp(buf1, "trails", 6)) {
        do_stat_trails(ch);
    } else if (is_abbrev(buf1, "ticl")) {
        if (!*buf2)
            send_to_char(ch, "Stat which TICL proc?\r\n");
        else {
            if (is_number(buf2))
                do_stat_ticl(ch, atoi(buf2));
            else {
                send_to_char(ch, "Usage: stat ticl <vnum>\r\n");
            }
        }
    } else if (is_abbrev(buf1, "zone")) {
        if (!*buf2)
            do_stat_zone(ch, ch->in_room->zone);
        else {
            if (is_number(buf2)) {
                tmp = atoi(buf2);
                for (found = 0, zone = zone_table; zone && found != 1;
                    zone = zone->next)
                    if (zone->number == tmp) {
                        do_stat_zone(ch, zone);
                        found = 1;
                    }
                if (found != 1)
                    send_to_char(ch, "No zone exists with that number.\r\n");
            } else
                send_to_char(ch, "Invalid zone number.\r\n");
        }
    } else if (is_abbrev(buf1, "mob")) {
        if (!*buf2)
            send_to_char(ch, "Stats on which mobile?\r\n");
        else {
            if ((victim = get_char_vis(ch, buf2)) &&
                (IS_NPC(victim) || GET_LEVEL(ch) >= LVL_DEMI))
                do_stat_character(ch, victim);
            else
                send_to_char(ch, "No such mobile around.\r\n");
        }
    } else if (is_abbrev(buf1, "player")) {
        if (!*buf2) {
            send_to_char(ch, "Stats on which player?\r\n");
        } else {
            if ((victim = get_player_vis(ch, buf2, 0)))
                do_stat_character(ch, victim);
            else
                send_to_char(ch, "No such player around.\r\n");
        }
    } else if (is_abbrev(buf1, "file")) {
        if (GET_LEVEL(ch) < LVL_TIMEGOD && !Security::isMember(ch, "AdminFull")
            && !Security::isMember(ch, "WizardFull")) {
            send_to_char(ch, "You cannot peer into the playerfile.\r\n");
            return;
        }
        if (!*buf2) {
            send_to_char(ch, "Stats on which player?\r\n");
        } else {
            CREATE(victim, struct Creature, 1);
            clear_char(victim);
            if( mini_mud ) {
                if( playerIndex.loadPlayer(buf2, victim) ) {
                    do_stat_character(ch, victim);
                } else {
                    send_to_char(ch, "Error loading character '%s'\r\n",buf2);
                }
            } else if (load_char(buf2, &tmp_store) > -1) {
                store_to_char(&tmp_store, victim);
                if (GET_LEVEL(victim) > GET_LEVEL(ch) && GET_IDNUM(ch) != 1) {
                    send_to_char(ch, "Sorry, you can't do that.\r\n");
                } else {
                    do_stat_character(ch, victim);
                }
            } else {
                send_to_char(ch, "There is no such player.\r\n");
            }
            free(victim);
        }
    } else if (is_abbrev(buf1, "object")) {
        if (!*buf2)
            send_to_char(ch, "Stats on which object?\r\n");
        else {
            if ((object = get_obj_vis(ch, buf2)))
                do_stat_object(ch, object);
            else
                send_to_char(ch, "No such object around.\r\n");
        }
    } else {
        if ((object = get_object_in_equip_vis(ch, buf1, ch->equipment, &tmp)))
            do_stat_object(ch, object);
        else if ((object = get_obj_in_list_vis(ch, buf1, ch->carrying)))
            do_stat_object(ch, object);
        else if ((victim = get_char_room_vis(ch, buf1)))
            do_stat_character(ch, victim);
        else if ((object =
                get_obj_in_list_vis(ch, buf1, ch->in_room->contents)))
            do_stat_object(ch, object);
        else if ((victim = get_char_vis(ch, buf1)))
            do_stat_character(ch, victim);
        else if ((object = get_obj_vis(ch, buf1)))
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
        Crash_save_all();
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
            Crash_save_all();
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
        Crash_save_all();
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
        Crash_save_all();
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
        ch->desc->snooping->snoop_by = NULL;
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
        send_to_char(ch, "The gods say I dont think so!\r\n");
    else if (victim->desc->snoop_by)
        if (ch == victim->desc->snoop_by->character)
            act("You're already snooping $M.", FALSE, ch, 0, victim, TO_CHAR);
        else if (GET_LEVEL(ch) > GET_LEVEL(victim->desc->snoop_by->character)) {
            send_to_char(ch, "Busy already. (%s)\r\n",
                GET_NAME(victim->desc->snoop_by->character));
        } else
            send_to_char(ch, "Busy already. \r\n");
    else if (victim->desc->snooping == ch->desc)
        send_to_char(ch, "Don't be stupid.\r\n");
    else if (ROOM_FLAGGED(victim->in_room, ROOM_GODROOM)
        && !Security::isMember(ch, "WizardFull")) {
        send_to_char(ch, "You cannot snoop into that place.\r\n");
    } else {
        if (victim->desc->original)
            tch = victim->desc->original;
        else
            tch = victim;

        if (GET_LEVEL(tch) >= GET_LEVEL(ch)) {
            send_to_char(ch, "You can't.\r\n");
            return;
        }
        send_to_char(ch, OK);

        if (ch->desc->snooping)
            ch->desc->snooping->snoop_by = NULL;

        ch->desc->snooping = victim->desc;
        victim->desc->snoop_by = ch->desc;
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

        ch->desc->character = victim;
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
        orig->desc->character = victim;
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

    if (!IS_NPC(ch) && (CHECK_REMORT_CLASS(ch) < 0)
        && (GET_LEVEL(ch) < LVL_IMMORT)) {
        if (FIGHTING(ch)) {
            send_to_char(ch, "No way!  You're fighting for your life!\r\n");
        } else if (GET_LEVEL(ch) <= 10) {
            act("A whirling globe of multi-colored light appears and whisks you away!", FALSE, ch, NULL, NULL, TO_CHAR);
            act("A whirling globe of multi-colored light appears and whisks $n away!", FALSE, ch, NULL, NULL, TO_ROOM);
            char_from_room(ch,false);
            char_to_room(ch, GET_START_ROOM(ch),false);
            look_at_room(ch, ch->in_room, 0);
            act("A whirling globe of multi-colored light appears and deposits $n on the floor!", FALSE, ch, NULL, NULL, TO_ROOM);
        } else
            send_to_char(ch, "There is no need to return.\r\n");
        return;
    }

    if (ch->desc && (orig = ch->desc->original)) {

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
            close_socket(ch->desc->original->desc);

        ch->desc->character = ch->desc->original;
        ch->desc->original = NULL;

        ch->desc->character->desc = ch->desc;
        ch->desc = NULL;

        if (cloud_found) {
            char_from_room(orig,false);
            char_to_room(orig, ch->in_room,false);
            act("$n materializes from a cloud of gas.",
                FALSE, orig, 0, 0, TO_ROOM);
            if (subcmd != SCMD_NOEXTRACT)
                ch->extract(true, false, CON_MENU);
        }
    } else
        send_to_char(ch, "There is no need to return.\r\n");
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

}

ACMD(do_oload)
{
    struct obj_data *obj;
    int number, quantity;
    char *temp, *temp2;

    temp = temp2 = NULL;
    //one_argument(argument, buf);

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
        obj_to_room(obj, ch->in_room);
    }
    act("$n makes a strange magical gesture.", TRUE, ch, 0, 0, TO_ROOM);
    if(quantity == 1) {
        act("$n has created $p!", FALSE, ch, obj, 0, TO_ROOM);
        act("You create $p.", FALSE, ch, obj, 0, TO_CHAR);
        slog("(GC) %s loaded %s at %d.", GET_NAME(ch),
            obj->short_description, ch->in_room->number);
    }
    else {
        act(tmp_sprintf("%s has created %s! (x%d)", GET_NAME(ch), obj->short_description, 
            quantity), FALSE, ch, obj, 0, TO_ROOM);
        act(tmp_sprintf("You create %s. (x%d)", obj->short_description, quantity), FALSE,
            ch, obj, 0, TO_CHAR);
        slog("(GC) %s loaded %s at %d. (x%d)", GET_NAME(ch), obj->short_description, 
            ch->in_room->number, quantity);
    }
}

ACMD(do_pload)
{
    struct Creature *vict = NULL;
    struct obj_data *obj;
    int number, quantity;
    char *temp, *temp2;

    temp = temp2 = NULL;
    //two_arguments(argument, buf, buf2);

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
            act(tmp_sprintf("You load %s onto %s. (x%d)", obj->short_description, 
                    GET_NAME(vict), quantity), FALSE, ch, obj, vict, TO_CHAR);
            act(tmp_sprintf("%s causes %s to appear in your hands. (x%d)", 
                    GET_NAME(ch), obj->short_description, quantity), FALSE, ch, obj, 
                    vict, TO_VICT);
        }
        
        
        
    } else {
        act("$n does something suspicious and alters reality.", TRUE, ch, 0, 0,
            TO_ROOM);
        for(int i=0; i < quantity; i++) {
            obj = read_object(number);
            obj_to_char(obj, ch);
        }
        
        if(quantity == 1) {
            act("You create $p.", FALSE, ch, obj, 0, TO_CHAR);
        } 
        else {
            act(tmp_sprintf("You create %s. (x%d)", obj->short_description, 
                quantity), FALSE, ch, obj, 0, TO_CHAR);
        }
    }
    if(quantity == 1) {
        slog("(GC) %s ploaded %s on %s.", GET_NAME(ch), obj->short_description,
            vict ? GET_NAME(vict) : GET_NAME(ch));
    }
    else {
        slog("(GC) %s ploaded %s on %s. (x%d)", GET_NAME(ch), 
            obj->short_description, vict ? GET_NAME(vict) : GET_NAME(ch), 
            quantity);
    }
}


ACMD(do_vstat)
{
    struct Creature *mob;
    struct obj_data *obj;
    int number;

    two_arguments(argument, buf, buf2);

    if (!*buf || !*buf2 || !isdigit(*buf2)) {
        send_to_char(ch, "Usage: vstat { obj | mob } <number>\r\n");
        return;
    }
    if ((number = atoi(buf2)) < 0) {
        send_to_char(ch, "A NEGATIVE number??\r\n");
        return;
    }
    if (is_abbrev(buf, "mob")) {
        if (!(mob = real_mobile_proto(number)))
            send_to_char(ch, "There is no monster with that number.\r\n");
        else
            do_stat_character(ch, mob);

    } else if (is_abbrev(buf, "obj")) {
        if (!(obj = real_object_proto(number)))
            send_to_char(ch, "There is no object with that number.\r\n");
        else
            do_stat_object(ch, obj);

    } else
        send_to_char(ch, "That'll have to be either 'obj' or 'mob'.\r\n");
}

ACMD(do_rstat)
{
    struct Creature *mob;
    //  struct obj_data *obj;
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
            do_stat_character(ch, mob);
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
            vict->extract(false, true, CON_CLOSE);
        } else if ((obj = get_obj_in_list_vis(ch, buf, ch->in_room->contents))) {
            act("$n destroys $p.", FALSE, ch, obj, 0, TO_ROOM);
            slog("(GC) %s purged %s at %d.", GET_NAME(ch),
                obj->short_description, ch->in_room->number);
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
                (*it)->extract(true, false, CON_MENU);
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
    save_char(victim, NULL);
}



ACMD(do_restore)
{
    struct Creature *vict;
    int i;

    one_argument(argument, buf);
    if (!*buf)
        send_to_char(ch, "Whom do you wish to restore?\r\n");
    else if (!str_cmp(buf, "all")) {
        CreatureList::iterator cit = characterList.begin();
        for (; cit != characterList.end(); ++cit) {
            vict = *cit;
            if (IS_NPC(vict))
                continue;
            GET_HIT(vict) = GET_MAX_HIT(vict);
            GET_MANA(vict) = GET_MAX_MANA(vict);
            GET_MOVE(vict) = GET_MAX_MOVE(vict);

            if (GET_COND(vict, FULL) >= 0)
                GET_COND(vict, FULL) = 24;
            if (GET_COND(vict, THIRST) >= 0)
                GET_COND(vict, THIRST) = 24;

            update_pos(vict);
            act("You have been fully healed by $N!", FALSE, vict, 0, ch,
                TO_CHAR);
        }
        send_to_char(ch, OK);
        mudlog(GET_INVIS_LVL(ch), CMP, true,
            "The mud has been restored by %s.", GET_NAME(ch));
    } else if (!(vict = get_char_vis(ch, buf))) {
        send_to_char(ch, NOPERSON);
    } else {
        GET_HIT(vict) = GET_MAX_HIT(vict);
        GET_MANA(vict) = GET_MAX_MANA(vict);
        GET_MOVE(vict) = GET_MAX_MOVE(vict);

        if (GET_COND(vict, FULL) >= 0)
            GET_COND(vict, FULL) = 24;
        if (GET_COND(vict, THIRST) >= 0)
            GET_COND(vict, THIRST) = 24;

        if ((GET_LEVEL(ch) >= LVL_GRGOD)
            && (GET_LEVEL(vict) >= LVL_AMBASSADOR)) {
            for (i = 1; i <= MAX_SKILLS; i++)
                SET_SKILL(vict, i, 100);

            if (GET_LEVEL(vict) >= LVL_IMMORT) {
                vict->real_abils.intel = 25;
                vict->real_abils.wis = 25;
                vict->real_abils.dex = 25;
                vict->real_abils.str = 25;
                vict->real_abils.con = 25;
                vict->real_abils.cha = 25;
            }
            vict->aff_abils = vict->real_abils;
        }
        update_pos(vict);
        send_to_char(ch, OK);
        act("You have been fully healed by $N!", FALSE, vict, 0, ch, TO_CHAR);
        mudlog(GET_LEVEL(ch), CMP, true,
            "%s has been restored by %s.", GET_NAME(vict), GET_NAME(ch));
    }
}


void
perform_immort_vis(struct Creature *ch)
{
    int old_level = 0;

    if (GET_INVIS_LVL(ch) == 0 && !IS_AFFECTED(ch, AFF_HIDE | AFF_INVISIBLE)) {
        send_to_char(ch, "You are already fully visible.\r\n");
        return;
    }
    old_level = GET_INVIS_LVL(ch);
    GET_INVIS_LVL(ch) = 0;
    //appear(ch);
    send_to_char(ch, "You are now fully visible.\r\n");
    CreatureList::iterator it = ch->in_room->people.begin();
    for (; it != ch->in_room->people.end(); ++it) {
        if ((*it) == ch)
            continue;
        if (GET_LEVEL((*it)) < old_level)
            act("You suddenly realize that $n is standing beside you.", FALSE,
                ch, 0, (*it), TO_VICT);
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
        if (GET_LEVEL(*it) < level) {
            if (GET_LEVEL(ch) >= LVL_AMBASSADOR)
                act("you suddenly realize that $n is standing beside you.",
                    false, ch, 0, (*it), TO_VICT);
            else if (GET_REMORT_GEN(*it) <= GET_REMORT_GEN(ch))
                act("$n suddenly appears from the thin air beside you.",
                    false, ch, 0, (*it), TO_VICT);
        }
    }

    send_to_char(ch, "You are now fully visible.\r\n");
}


void
perform_invis(struct Creature *ch, int level)
{
    int old_level;
    
    if (IS_NPC(ch))
        return;

    if (FIGHTING(ch) && level > GET_LEVEL(FIGHTING(ch))
        && !IS_NPC(FIGHTING(ch))) {
        act("You cannot go invis over $N while fighting $M.", FALSE, ch, 0,
            FIGHTING(ch), TO_CHAR);
        return;
    }

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
            if (!pt->connected && pt->character && pt->character != ch &&
                !PRF2_FLAGGED(pt->character, PRF2_NOGECHO) &&
                !PLR_FLAGGED(pt->character, PLR_OLC | PLR_WRITING)) {
                if (GET_LEVEL(pt->character) >= 50 &&
                    GET_LEVEL(pt->character) > GET_LEVEL(ch))
                    send_to_char(pt->character, "[%s-g] %s\r\n", GET_NAME(ch), argument);
                else
                    send_to_char(pt->character, "%s\r\n", argument);
            }
        }
        if (PRF_FLAGGED(ch, PRF_NOREPEAT))
            send_to_char(ch, OK);
        else
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
            if (!pt->connected && pt->character && pt->character != ch &&
                pt->character->in_room && pt->character->in_room->zone == here
                && !PRF2_FLAGGED(pt->character, PRF2_NOGECHO)
                && !PLR_FLAGGED(pt->character, PLR_OLC | PLR_WRITING)) {
                if (GET_LEVEL(pt->character) > GET_LEVEL(ch))
                    send_to_char(pt->character, "[%s-zone] %s\r\n", GET_NAME(ch), argument);
                else
                    send_to_char(pt->character, "%s\r\n", argument);
            }
        }
        if (PRF_FLAGGED(ch, PRF_NOREPEAT))
            send_to_char(ch, OK);
        else
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

        if (PRF_FLAGGED(ch, PRF_NOREPEAT))
            send_to_char(ch, OK);
        else
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
    if (d->character && GET_LEVEL(d->character) >= GET_LEVEL(ch)) {
        send_to_char(ch, "Umm.. maybe that's not such a good idea...\r\n");
        return;
    }
    close_socket(d);
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


        for (d = descriptor_list; d; d = d->next) {

            if (d->character) {

                if (GET_LEVEL(d->character) <= level) {
                    close_socket(d);
                }
            }
        }
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
    struct char_file_u chdata;

    one_argument(argument, arg);
    if (!*arg) {
        send_to_char(ch, "For whom do you wish to search?\r\n");
        return;
    }
    if (load_char(arg, &chdata) < 0) {
        send_to_char(ch, "There is no such player.\r\n");
        return;
    }
    if ((chdata.level > GET_LEVEL(ch)) && (GET_LEVEL(ch) < LVL_GRIMP)) {
        send_to_char(ch, "You are not sufficiently godly for that!\r\n");
        return;
    }
    send_to_char(ch, "[%5ld] [%2d %s] %-12s : %-18s : %-20s",
        chdata.char_specials_saved.idnum, (int)chdata.level,
        char_class_abbrevs[(int)chdata.char_class], chdata.name,
        GET_LEVEL(ch) > LVL_ETERNAL ? chdata.host : "Unknown",
        ctime(&chdata.last_logon));
    if (GET_LEVEL(ch) >= chdata.level &&
        has_mail(chdata.char_specials_saved.idnum))
        send_to_char(ch, "Player has unread mail.\r\n");
}


ACMD(do_force)
{
    struct descriptor_data *i, *next_desc;
    struct Creature *vict;
    char to_force[MAX_INPUT_LENGTH + 2];

    half_chop(argument, arg, to_force);

    sprintf(buf1, "$n has forced you to '%s'.", to_force);

    if (!*arg || !*to_force)
        send_to_char(ch, "Whom do you wish to force do what?\r\n");
    else if ((GET_LEVEL(ch) < LVL_GRGOD) ||
        (str_cmp("all", arg) && str_cmp("room", arg))) {
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
    } else if (!str_cmp("room", arg)) {
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
    } else if (GET_LEVEL(ch) >= LVL_GRGOD) {    /* force all */
        send_to_char(ch, OK);
        mudlog(GET_LEVEL(ch), NRM, true,
            "(GC) %s forced all to %s", GET_NAME(ch), to_force);

        for (i = descriptor_list; i; i = next_desc) {
            next_desc = i->next;

            if (STATE(i) || !(vict = i->character) ||
                GET_LEVEL(vict) >= GET_LEVEL(ch))
                continue;
            act(buf1, TRUE, ch, NULL, vict, TO_VICT);
            command_interpreter(vict, to_force);
        }
    } else
        send_to_char(ch, "Arrrg!  You seem to have strained something!\r\n");
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
    delete_doubledollar(argument);

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
            if (STATE(d) == CON_PLAYING && GET_LEVEL(d->character) >=
                (subcmd == SCMD_IMMCHAT ? LVL_AMBASSADOR : LVL_DEMI) &&
                ((subcmd == SCMD_IMMCHAT &&
                        !PRF2_FLAGGED(d->character, PRF2_NOIMMCHAT)) ||
                    (subcmd == SCMD_WIZNET &&
                        Security::isMember(d->character, "WizardBasic") &&
                        !PRF_FLAGGED(d->character, PRF_NOWIZ))) &&
                (can_see_creature(ch, d->character) || GET_LEVEL(ch) == LVL_GRIMP)) {
                if (!any) {
                    sprintf(buf1, "Gods online:\r\n");
                    any = TRUE;
                }
                sprintf(buf1, "%s  %s", buf1, GET_NAME(d->character));
                if (PLR_FLAGGED(d->character, PLR_WRITING))
                    sprintf(buf1, "%s (Writing)\r\n", buf1);
                else if (PLR_FLAGGED(d->character, PLR_MAILING))
                    sprintf(buf1, "%s (Writing mail)\r\n", buf1);
                else if (PLR_FLAGGED(d->character, PLR_OLC))
                    sprintf(buf1, "%s (Creating)\r\n", buf1);
                else
                    sprintf(buf1, "%s\r\n", buf1);

            }
        }

        if (!any)                /* hacked bug fix */
            strcpy(buf1, "");

        any = FALSE;
        for (d = descriptor_list; d; d = d->next) {
            if (STATE(d) == CON_PLAYING
                && GET_LEVEL(d->character) >= LVL_AMBASSADOR
                && ((subcmd == SCMD_IMMCHAT
                        && PRF2_FLAGGED(d->character, PRF2_NOIMMCHAT))
                    || (subcmd == SCMD_WIZNET
                        && Security::isMember(d->character, "WizardBasic")
                        && PRF_FLAGGED(d->character, PRF_NOWIZ)))
                && can_see_creature(ch, d->character)) {
                if (!any) {
                    sprintf(buf1, "%sGods offline:\r\n", buf1);
                    any = TRUE;
                }
                send_to_char(ch, "%s  %s\r\n", buf1, GET_NAME(d->character));
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
        if ((STATE(d) == CON_PLAYING) && (GET_LEVEL(d->character) >= level) &&
            (subcmd != SCMD_WIZNET
                || Security::isMember(d->character, "WizardBasic"))
            && (subcmd != SCMD_WIZNET || !PRF_FLAGGED(d->character, PRF_NOWIZ))
            && (subcmd != SCMD_IMMCHAT
                || !PRF2_FLAGGED(d->character, PRF2_NOIMMCHAT))
            && (!PLR_FLAGGED(d->character,
                    PLR_WRITING | PLR_MAILING | PLR_OLC))
            && (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT)))) {

            if (subcmd == SCMD_IMMCHAT) {
                send_to_char(d->character, CCYEL(d->character, C_SPR));
            } else {
                send_to_char(d->character, CCCYN(d->character, C_SPR));
            }
            if (can_see_creature(d->character, ch))
                send_to_char(d->character, "%s", buf1);
            else
                send_to_char(d->character, "%s", buf2);

            send_to_char(d->character, CCNRM(d->character, C_SPR));
        }
    }

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
        send_to_char(ch, OK);
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
        mudlog(MAX(LVL_GRGOD, GET_INVIS_LVL(ch)),
            subcmd == SCMD_OLC ? CMP : NRM,
            true,
            "(GC) %s %sreset zone %d (%s)", GET_NAME(ch),
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

    one_argument(argument, arg);

    if (!*arg)
        send_to_char(ch, "Yes, but for whom?!?\r\n");
    else if (!(vict = get_char_vis(ch, arg)))
        send_to_char(ch, "There is no such player.\r\n");
    else if (IS_NPC(vict))
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
        case SCMD_PARDON:
            if (!PLR_FLAGGED(vict, PLR_THIEF | PLR_KILLER)) {
                send_to_char(ch, "Your victim is not flagged.\r\n");
                return;
            }
            REMOVE_BIT(PLR_FLAGS(vict), PLR_THIEF | PLR_KILLER);
            send_to_char(ch, "Pardoned.\r\n");
            send_to_char(vict, "You have been pardoned by the Gods!\r\n");
            mudlog(MAX(LVL_GOD, GET_INVIS_LVL(ch)), NRM, true,
                "(GC) %s pardoned by %s", GET_NAME(vict),
                GET_NAME(ch));
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
        case SCMD_COUNCIL: {
            result = PLR_TOG_CHK(vict, PLR_COUNCIL);
            msg = tmp_sprintf("Council %s for %s by %s.", 
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
            if (USE_XML_FILES) {
                do_freeze_char(argument, vict, ch);
                return;
            }
            
            if (ch == vict) {
                send_to_char(ch, "Oh, yeah, THAT'S real smart...\r\n");
                return;
            }
            if (PLR_FLAGGED(vict, PLR_FROZEN)) {
                send_to_char(ch, "Your victim is already pretty cold.\r\n");
                return;
            }
            SET_BIT(PLR_FLAGS(vict), PLR_FROZEN);
            GET_FREEZE_LEV(vict) = GET_LEVEL(ch);
            send_to_char(vict, 
                "A bitter wind suddenly rises and drains every erg of heat from your body!\r\nYou feel frozen!\r\n");
            send_to_char(ch, "Frozen.\r\n");
            act("A sudden cold wind conjured from nowhere freezes $n!", FALSE,
                vict, 0, 0, TO_ROOM);
            mudlog(MAX(LVL_POWER, GET_INVIS_LVL(ch)), BRF, true,
                "(GC) %s frozen by %s.", GET_NAME(vict),
                GET_NAME(ch));
            break;
        case SCMD_THAW:
            if (!PLR_FLAGGED(vict, PLR_FROZEN)) {
                send_to_char(ch, 
                    "Sorry, your victim is not morbidly encased in ice at the moment.\r\n");
                return;
            }
            if (GET_FREEZE_LEV(vict) > GET_LEVEL(ch)) {
                sprintf(buf,
                    "Sorry, a level %d God froze %s... you can't unfreeze %s.\r\n",
                    GET_FREEZE_LEV(vict), GET_NAME(vict), HMHR(vict));
                send_to_char(ch, "%s", buf);
                return;
            }
            mudlog(MAX(LVL_POWER, GET_INVIS_LVL(ch)), BRF, true,
                "(GC) %s un-frozen by %s.", GET_NAME(vict),
                GET_NAME(ch));
            REMOVE_BIT(PLR_FLAGS(vict), PLR_FROZEN);
            send_to_char(vict, 
                "A fireball suddenly explodes in front of you, melting the ice!\r\nYou feel thawed.\r\n");
            send_to_char(ch, "Thawed.\r\n");
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
                return;
            }
            break;
        default:
            slog("SYSERR: Unknown subcmd passed to do_wizutil (act.wizard.c)");
            break;
        }
        save_char(vict, NULL);
    }
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


#define LEARNED_LEVEL        0    /* % known which is considered "learned" */
#define MAX_PER_PRAC        1    /* max percent gain in skill per practice */
#define MIN_PER_PRAC        2    /* min percent gain in skill per practice */
#define PRAC_TYPE        3        /* should it say 'spell' or 'skill'?         */


void
list_skills_to_char(struct Creature *ch, struct Creature *vict)
{
    extern struct spell_info_type spell_info[];
    char buf3[MAX_STRING_LENGTH];
    int i, sortpos;

    if (!GET_PRACTICES(vict))
        sprintf(buf, "%s%s has no practice sessions remaining.%s\r\n",
            CCYEL(ch, C_NRM), PERS(vict, ch), CCNRM(ch, C_NRM));
    else
        sprintf(buf, "%s%s has %d practice session%s remaining.%s\r\n",
            CCGRN(ch, C_NRM), PERS(vict, ch), GET_PRACTICES(vict),
            (GET_PRACTICES(vict) == 1 ? "" : "s"), CCNRM(ch, C_NRM));

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

    sprintf(buf, "Current statistics of Tempus:\r\n");
    sprintf(buf, "%s  %5d players in game  %5d connected\r\n", buf, i, con);
    sprintf(buf, "%s  %5d registered\r\n", buf, top_of_p_table + 1);
    sprintf(buf, "%s  %5d mobiles          %5d prototypes (%d id'd)\r\n",
        buf, j, top_of_mobt + 1, current_mob_idnum);
    sprintf(buf, "%s  %5d objects          %5d prototypes\r\n",
        buf, k, top_of_objt + 1);
    sprintf(buf, "%s  %5d rooms            %5d zones (%d active)\r\n",
        buf, top_of_world + 1, top_of_zone_table, num_active_zones);
    sprintf(buf, "%s  %5d searches\r\n", buf, srch_count);
    sprintf(buf, "%s  %5d large bufs\r\n", buf, buf_largecount);
    sprintf(buf, "%s  %5d buf switches     %5d overflows\r\n", buf,
        buf_switches, buf_overflows);
    sprintf(buf, "%s  %5lu tmp max used     %5lu tmp overruns\r\n", buf,
        tmp_max_used, tmp_overruns);
    sprintf(buf, "%s  Lunar stage: %2d, phase: %s (%d)\r\n", buf,
        lunar_stage, lunar_phases[lunar_phase], lunar_phase);
    if (quest_status)
        sprintf(buf, "%s   %sQUEST_STATUS is ON.%s\r\n",
            buf, CCREV(ch, C_NRM), CCNRM(ch, C_NRM));
    send_to_char(ch, "%s  Trail count: %d\r\n", buf, tr_count);

    if (GET_LEVEL(ch) >= LVL_LOGALL && log_cmds) {
        send_to_char(ch, "  Logging all commands :: file is %s.\r\n",
            1 ? "OPEN" : "NULL");
    }
    if (shutdown_count >= 0) {
        sprintf(buf,
            "  Shutdown sequence ACTIVE.  Shutdown %s in [%d] seconds.\r\n",
            shutdown_mode == SHUTDOWN_DIE ? "DIE" :
            shutdown_mode == SHUTDOWN_PAUSE ? "PAUSE" : "REBOOT",
            shutdown_count);
        send_to_char(ch, "%s", buf);
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
show_player(Creature *ch, char *value)
{
    struct char_file_u vbuf;
    struct rent_info rent;
    FILE *fl;

    char birth[80];
    char last_login[80];
    char remort_desc[80];
    char fname[MAX_INPUT_LENGTH];
    char rent_type[80];
    long idnum = 0;

    if (!*value) {
        send_to_char(ch, "A name would help.\r\n");
        return;
    }

    /* added functionality for show player by idnum */
    if (is_number(value) && get_name_by_id(atoi(value))) {
        strcpy(value, get_name_by_id(atoi(value)));
    }
    if (load_char(value, &vbuf) < 0) {
        send_to_char(ch, "There is no such player.\r\n");
        return;
    }

    idnum = vbuf.char_specials_saved.idnum;
    // Load up the players rent info and check whether or not he is cryo'd

    if (!get_filename(vbuf.name, fname, CRASH_FILE)) {
        slog("Failed get_filename() in show_player().\r\n");
    }

    if (!(fl = fopen(fname, "rb"))) {
        sprintf(buf1, "%s has no rent file!", vbuf.name);
        slog(buf1);
    }


    if (fl != NULL) {
        if (!feof(fl)) {
            fread(&rent, sizeof(struct rent_info), 1, fl);
        }
        fclose(fl);
    }

    if (get_char_in_world_by_idnum(idnum) &&
        GET_INVIS_LVL(get_char_in_world_by_idnum(idnum)) < GET_LEVEL(ch)) {
        strcpy(rent_type, CCYEL(ch, C_NRM));
        strcat(rent_type, "In Game.");
    } else {
        switch (rent.rentcode) {
        case RENT_CRASH:
            strcpy(rent_type, CCRED(ch, C_NRM));
            strcat(rent_type, "Crash");
            break;
        case RENT_RENTED:
            strcpy(rent_type, "Rented");
            break;
        case RENT_CRYO:
            strcpy(rent_type, CCCYN(ch, C_NRM));
            strcat(rent_type, "Cryo'd");
            break;
        case RENT_FORCED:
            strcpy(rent_type, CCGRN(ch, C_NRM));
            strcat(rent_type, "Force");
            break;
        case RENT_TIMEDOUT:
            strcpy(rent_type, CCGRN(ch, C_NRM));
            strcat(rent_type, "Timed Out");
            break;
        default:
            strcpy(rent_type, CCRED(ch, C_NRM));
            strcat(rent_type, "Undefined");
        }
    }

    if (vbuf.player_specials_saved.remort_generation <= 0) {
        strcpy(remort_desc, "");
    } else {
        sprintf(remort_desc, "/%s",
            char_class_abbrevs[(int)vbuf.remort_char_class]);
    }
    sprintf(buf, "Player: %-12s (%s) [%2d %s %s%s]  Gen: %d", vbuf.name,
        genders[(int)vbuf.sex], vbuf.level, player_race[(int)vbuf.race],
        char_class_abbrevs[(int)vbuf.char_class], remort_desc,
        vbuf.player_specials_saved.remort_generation);
    sprintf(buf, "%s  Rent: %s%s\r\n", buf, rent_type, CCNRM(ch, C_NRM));
    sprintf(buf,
        "%sAu: %-8d  Bal: %-8d  Exp: %-8d  Align: %-5d  Pracs: %-3d\r\n",
        buf, vbuf.points.gold, vbuf.points.bank_gold, vbuf.points.exp,
        vbuf.char_specials_saved.alignment,
        vbuf.player_specials_saved.spells_to_learn);
    // Trim and fit the date to show year but not seconds.
    strcpy(birth, ctime(&vbuf.birth));
    strcpy(birth + 16, birth + 19);
    birth[21] = '\0';
    if (vbuf.level > GET_LEVEL(ch)) {
        strcpy(last_login, "Unknown");
    } else {
        // Trim and fit the date to show year but not seconds.
        strcpy(last_login, ctime(&vbuf.last_logon));
        strcpy(last_login + 16, last_login + 19);
        last_login[21] = '\0';
    }
    sprintf(buf,
        "%sStarted: %-22.21s Last: %-22.21s Played: %3dh %2dm\r\n",
        buf, birth, last_login, (int)(vbuf.played / 3600),
        (int)(vbuf.played / 60 % 60));

    if (IS_SET(vbuf.char_specials_saved.act, PLR_FROZEN))
        sprintf(buf, "%s%s%s is FROZEN!%s\r\n", buf, CCCYN(ch, C_NRM),
            vbuf.name, CCNRM(ch, C_NRM));

    if (IS_SET(vbuf.player_specials_saved.plr2_bits, PLR2_BURIED))
        sprintf(buf, "%s%s%s is BURIED!%s\r\n", buf, CCGRN(ch, C_NRM),
            vbuf.name, CCNRM(ch, C_NRM));

    if (IS_SET(vbuf.char_specials_saved.act, PLR_DELETED))
        sprintf(buf, "%s%s%s is DELETED!%s\r\n", buf, CCRED(ch, C_NRM),
            vbuf.name, CCNRM(ch, C_NRM));
    send_to_char(ch, "%s", buf);
}

void
show_multi(Creature *ch, char *arg)
{
    struct char_file_u *whole_file, *cur_pl;
    struct char_file_u chdata;
    int pl_idx, dot_pos;
    int players_listed = 0;

    if (!*arg) {
        send_to_char(ch, "For whom do you wish to search?\r\n");
        return;
    }
    if (load_char(arg, &chdata) < 0) {
        send_to_char(ch, "There is no such player.\r\n");
        return;
    }
    if ((chdata.level > GET_LEVEL(ch)) && (GET_LEVEL(ch) < LVL_GRIMP)) {
        send_to_char(ch, "You are not sufficiently godly for that!\r\n");
        return;
    }

    sprintf(buf, "[%5ld] [%2d %s] %-12s : %-18s : %-20s",
        chdata.char_specials_saved.idnum, (int)chdata.level,
        char_class_abbrevs[(int)chdata.char_class], chdata.name,
        GET_LEVEL(ch) > LVL_ETERNAL ? chdata.host : "Unknown",
        ctime(&chdata.last_logon));

    dot_pos = (strrchr(chdata.host, '.') - chdata.host) - 1;

    // Now we load in the whole player file, looking for matches
    pl_idx = top_of_p_table + 1;
    CREATE(whole_file, struct char_file_u, pl_idx);
    fseek(player_fl, 0, SEEK_SET);
    if ((int)fread(whole_file, sizeof(struct char_file_u), pl_idx,
            player_fl) != pl_idx) {
        send_to_char(ch, "Didn't read the right number of records.\r\n");
        return;
    }
    for (cur_pl = whole_file; pl_idx && players_listed < 300;
        cur_pl++, pl_idx--) {
        if (cur_pl->char_specials_saved.idnum == GET_IDNUM(ch)
            || cur_pl->char_specials_saved.idnum ==
            chdata.char_specials_saved.idnum) {
            continue;
        }
        if (!strncmp(cur_pl->host, chdata.host, dot_pos)
            && cur_pl->level <= GET_LEVEL(ch)) {
            sprintf(buf, "%s[%5ld] [%2d %s] %-12s : %-18s : %-20s", buf,
                cur_pl->char_specials_saved.idnum, (int)cur_pl->level,
                char_class_abbrevs[(int)cur_pl->char_class], cur_pl->name,
                GET_LEVEL(ch) > LVL_ETERNAL ? cur_pl->host : "Unknown",
                ctime(&cur_pl->last_logon));
            ++players_listed;
        }
    }
    free(whole_file);
    page_string(ch->desc, buf);
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
            get_name_by_id(zone->owner_idnum), CCNRM(ch, C_NRM));
    }

    page_string(ch->desc, buf);
}

void
show_topzones(Creature *ch, char *value)
{
    int i, j, num_zones = 50;
    struct zone_data *zone, **topzones = NULL;

    skip_spaces(&value);

    if (*value)
        num_zones = atoi(value);

    num_zones = MIN(top_of_zone_table, num_zones);

    if (!(topzones =
            (struct zone_data **)malloc(num_zones *
                sizeof(struct zone_data *)))) {
        send_to_char(ch, "Allocation error.\r\n");
        return;
    }

    for (i = 0; i < num_zones; i++) {
        topzones[i] = zone_table;
        for (zone = zone_table; zone; zone = zone->next)
            if (zone->enter_count >= topzones[i]->enter_count &&
                zone->number < 700) {
                for (j = 0; j < i; j++)
                    if (topzones[j] == zone)
                        break;

                if (j == i)
                    topzones[j] = zone;
            }
    }

    sprintf(buf, "TOP %d zones:\r\n", num_zones);

    for (i = 0; i < num_zones; i++)
        sprintf(buf,
            "%s%2d.[%s%3d%s] %s%-30s%s %s[%s%6d%s]%s accesses.  Owner: %s%s%s\r\n",
            buf, i, CCYEL(ch, C_NRM), topzones[i]->number, CCNRM(ch, C_NRM),
            CCCYN(ch, C_NRM), topzones[i]->name, CCNRM(ch, C_NRM), CCGRN(ch,
                C_NRM), CCNRM(ch, C_NRM), topzones[i]->enter_count, CCGRN(ch,
                C_NRM), CCNRM(ch, C_NRM), CCYEL(ch, C_NRM),
            get_name_by_id(topzones[i]->owner_idnum), CCNRM(ch, C_NRM));

    free(topzones);
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
    CreatureList::iterator cit = mobilePrototypes.begin();
    for (; cit != mobilePrototypes.end(); ++cit) {
        //for (mob = mob_proto; mob; mob = mob->next) {
        mob = *cit;
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
        CreatureList::iterator cit = mobilePrototypes.begin();
        for (; cit != mobilePrototypes.end(); ++cit) {
            mob = *cit;
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
        CreatureList::iterator cit = mobilePrototypes.begin();
        for (; cit != mobilePrototypes.end(); ++cit) {
            //for (mob = mob_proto; mob; mob = mob->next) {
            mob = *cit;
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
    struct elevator_data *e_head = NULL;
    struct elevator_elem *e_elem = NULL;
    struct Creature *mob = NULL;
    CreatureList::iterator cit;
    CreatureList::iterator mit;

    void show_shops(struct Creature *ch, char *value);

    skip_spaces(&argument);
    if (!*argument) {
        vector <string> cmdlist;
        send_to_char(ch, "Show options:\r\n");
        for (j = 0, i = 1; fields[i].level; i++) {
            if (Security::canAccess(ch, fields[i])) {
                cmdlist.push_back(fields[i].cmd);
                //sprintf(buf, "%s%-15s%s", buf, fields[i].cmd, (!(++j % 5) ? "\r\n" : ""));
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
                "Usage: show zone [ . | all | <begin#> <end#> | name <partial name> | fullcontrol | owner | co-owner ]\r\n";
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
                    const char *ownerName = get_name_by_id(zone->owner_idnum);
                    if (ownerName && strcasecmp(value, ownerName) == 0) {
                        print_zone_to_buf(ch, buf, zone);
                    }
                }
            } else if (strcasecmp("co-owner", value) == 0 && tokens.next(value)) {    // Show by name
                for (zone = zone_table; zone; zone = zone->next) {
                    const char *ownerName = get_name_by_id(zone->co_owner_idnum);
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
        Crash_listrent(ch, strcat(value, arg));
        break;
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
        show_shops(ch, argument);
        break;
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
                struct char_file_u tmp_store;
                CREATE(vict, struct Creature, 1);
                clear_char(vict);
                if (load_char(value, &tmp_store) > -1) {
                    store_to_char(&tmp_store, vict);
                    if (GET_LEVEL(vict) > GET_LEVEL(ch) && GET_IDNUM(ch) != 1)
                        send_to_char(ch, "Sorry, you can't do that.\r\n");
                    else
                        list_skills_to_char(ch, vict);
                    free_char(vict);
                } else {
                    send_to_char(ch, "There is no such player.\r\n");
                    free(vict);
                }
                vict = NULL;
            }
        } else if (IS_MOB(vict)) {
            send_to_char(ch, "Mobs don't have skills.  All mobs are assumed to\r\n"
                "have a (50 + level)% skill proficiency.\r\n");
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
        show_topzones(ch, value);
        break;
    case 23:                    /* nomaterial */
        strcpy(buf, "Objects without material types:\r\n");
        for (obj = obj_proto, i = 1; obj; obj = obj->next) {
            if (GET_OBJ_MATERIAL(obj) == MAT_NONE &&
                !IS_OBJ_TYPE(obj, ITEM_SCRIPT)) {
                if (strlen(buf) > (MAX_STRING_LENGTH - 130)) {
                    strcat(buf, "**OVERFLOW**\r\n");
                    break;
                }
                sprintf(buf, "%s%3d. [%5d] %s%-36s%s  (%s)\r\n", buf, i,
                    GET_OBJ_VNUM(obj), CCGRN(ch, C_NRM),
                    obj->short_description, CCNRM(ch, C_NRM), obj->name);
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
                    obj->short_description,
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
        break;
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
                        i++, obj->short_description,
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
            tmp_d->character = NULL;
        }
        ch->desc->character = vict;
        vict->desc = ch->desc;
        do_equipment(vict, arg, 0, SCMD_IMPLANTS, 0);
        ch->desc->character = ch;
        if ((vict->desc = tmp_d))
            tmp_d->character = vict;
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
        strcpy(buf, "Elevator list:\r\n");
        for (e_head = elevators; e_head; e_head = e_head->next) {
            sprintf(buf, "%s#%d\r\n", buf, e_head->vnum);
            for (e_elem = e_head->list; e_elem; e_elem = e_elem->next)
                sprintf(buf, "%s Room:[%5d] Key:[%5d] [%s]\r\n", buf,
                    e_elem->rm_vnum, e_elem->key, e_elem->name);
        }
        page_string(ch->desc, buf);
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
            //for (mob = mob_proto, i = 0; mob; mob = mob->next) {
            mob = *mit;
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
        for (obj = obj_proto, i = 0; obj; obj = obj->next) {
            if (obj->shared->number >= k) {
                if (strlen(buf) + 256 > MAX_STRING_LENGTH) {
                    strcat(buf, "**OVERFOW**\r\n");
                    break;
                }
                sprintf(buf,
                    "%s%4d. %s[%s%5d%s] %40s%s Tot:[%4d], House:[%4d]\r\n",
                    buf, ++i, CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                    GET_OBJ_VNUM(obj), CCGRN(ch, C_NRM),
                    obj->short_description, CCNRM(ch, C_NRM),
                    obj->shared->number, obj->shared->house_count);
            }
        }
        if (i)
            page_string(ch->desc, buf);
        else
            send_to_char(ch, "No items.\r\n");
        break;

    case 42:                    /* fighting */

        strcpy(buf, "Fighting characters:\r\n");

        cit = combatList.begin();
        for (; cit != combatList.end(); ++cit) {
            vict = *cit;

            if (!can_see_creature(ch, vict))
                continue;

            if (strlen(buf) > MAX_STRING_LENGTH - 128)
                break;

            sprintf(buf, "%s %3d. %s%28s%s -- %28s   [%6d]\r\n", buf, ++i,
                IS_NPC(vict) ? "" : CCYEL(ch, C_NRM),
                GET_NAME(vict), IS_NPC(vict) ? "" : CCNRM(ch, C_NRM),
                FIGHTING(vict) ? GET_NAME(FIGHTING(vict)) :
                "ERROR!!", vict->in_room->number);
        }

        page_string(ch->desc, buf);
        break;

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
            if (!HUNTING(vict) || !HUNTING(vict)->in_room
                || !can_see_creature(ch, vict))
                continue;

            if (strlen(buf) > MAX_STRING_LENGTH - 128)
                break;

            sprintf(buf, "%s %3d. %23s [%5d] ---> %20s [%5d]\r\n", buf, ++i,
                GET_NAME(vict), vict->in_room->number,
                GET_NAME(HUNTING(vict)), HUNTING(vict)->in_room->number);
        }

        page_string(ch->desc, buf);
        break;

    case 46:                    /* last_cmd */

        strcpy(buf, "Last cmds:\r\n");
        for (i = 0; i < NUM_SAVE_CMDS; i++)
            sprintf(buf, "%s %2d. (%4d) %25s - '%s'\r\n", buf,
                i, last_cmd[i].idnum, get_name_by_id(last_cmd[i].idnum),
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
                obj ? obj->short_description : "--- ---",
                GET_WEAP_SPEC(vict, i).level);
        }
        page_string(ch->desc, buf);
        break;
    case 49:                    // p_index
        j = k = 0;
        if (*value) {
            j = atoi(value);
            if (*arg) {
                k = atoi(arg);
                if (k < j) {
                    send_to_char(ch, 
                        "The top value must be greater than the first.\r\n");
                    return;
                }
            }
        }
        strcpy(buf, "Player Index:\r\n");
        for (i = 0; i <= top_of_p_table; i++) {
            if (j && player_table[i].id < j)
                continue;
            if (k && player_table[i].id > k)
                continue;
            if (strlen(buf) > MAX_STRING_LENGTH - 64)
                break;
            sprintf(buf, "%s %3d. [%5ld] %s\r\n", buf, i, player_table[i].id,
                player_table[i].name);
        }
        page_string(ch->desc, buf);
        break;

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
        show_file(ch, "log/help.log", 0); break;
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
    struct char_file_u tmp_store;
    char *field, *name;
    char *arg1, *arg2;
    int on = 0, off = 0, value = 0;
    char is_file = 0, is_mob = 0, is_player = 0;
    int player_i = 0;
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
        {"practices", LVL_IMMORT, PC, NUMBER, "WizardFull"},
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
        {"deleted", LVL_IMMORT, PC, BINARY, "AdminFull"},
        {"class", LVL_IMMORT, BOTH, MISC, "WizardFull"},
        {"nowizlist", LVL_IMMORT, PC, BINARY, "WizardAdmin"},    /* 40 */
        {"quest", LVL_IMMORT, PC, BINARY, "Coder"},
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
        {"generation", LVL_IMMORT, PC, NUMBER, "WizardFull"},
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
        {"qpoints", LVL_IMMORT, PC, NUMBER, "QuestorAdmin,WizardFull"},
        {"qpallow", LVL_IMMORT, PC, NUMBER, "QuestorAdmin,WizardFull"},    /*  95 */
        {"soulless", LVL_IMMORT, BOTH, BINARY, "WizardFull"},
        {"buried", LVL_IMMORT, PC, BINARY, "AdminFull"},
        {"speed", LVL_IMMORT, PC, NUMBER, "Coder"},
        {"occupation", LVL_ENTITY, PC, NUMBER, "CoderAdmin"},
        {"skill", LVL_ENTITY, PC, MISC, "WizardFull"},
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
        CREATE(cbuf, struct Creature, 1);
        clear_char(cbuf);
        if ((player_i = load_char(name, &tmp_store)) > -1) {
            store_to_char(&tmp_store, cbuf);
            if (GET_LEVEL(cbuf) >= GET_LEVEL(ch) && GET_IDNUM(ch) != 1) {
                free_char(cbuf);
                send_to_char(ch, "Sorry, you can't do that.\r\n");
                return;
            }
            vict = cbuf;
        } else {
            free(cbuf);
            send_to_char(ch, "There is no such player.\r\n");
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
        SET_OR_REMOVE(PRF_FLAGS(vict), PRF_SUMMONABLE);
        on = !on;                /* so output will be correct */
        break;
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
        GET_BANK_GOLD(vict) = RANGE(0, 1000000000);
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
    case 28:
        GET_PRACTICES(vict) = RANGE(0, 100);
        break;
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
        sprintf(buf, "(GC) %s set %s %sdeleted.", 
                GET_NAME(ch), 
                GET_NAME(vict),
                PLR_FLAGGED(vict, PLR_DELETED) ? "" : "UN-"); 
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_DELETED);
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
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOWIZLIST);
        break;
    case 41:
        SET_OR_REMOVE(PRF_FLAGS(vict), PRF_QUEST);
        break;
    case 42:
        if (!str_cmp(argument, "on"))
            SET_BIT(PLR_FLAGS(vict), PLR_LOADROOM);
        else if (!str_cmp(argument, "off"))
            REMOVE_BIT(PLR_FLAGS(vict), PLR_LOADROOM);
        else {
            if (real_room(i = atoi(argument)) != NULL) {
                GET_LOADROOM(vict) = i;
                SET_BIT(PLR_FLAGS(vict), PLR_LOADROOM);
                sprintf(buf, "%s will enter at %d.", GET_NAME(vict),
                    GET_LOADROOM(vict));
            } else
                sprintf(buf, "That room does not exist!");
        }
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
        strncpy(tmp_store.pwd, CRYPT(argument, tmp_store.name), MAX_PWD_LENGTH);
        tmp_store.pwd[MAX_PWD_LENGTH] = '\0';
        sprintf(buf, "Password changed to '%s'.", argument);
        break;
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
        if (!is_file && vict->desc->snoop_by) {
            send_to_char(vict->desc->snoop_by->character, "Your snooping session has ended.\r\n");
            stop_snooping(vict->desc->snoop_by->character);
        }
        break;
    case 53:
        if (is_number(argument) && !atoi(argument)) {
            GET_CLAN(vict) = 0;
            send_to_char(ch, "Clan set to none.\r\n");
        } else if (!clan_by_name(argument)) {
            send_to_char(ch, "There is no such clan.\r\n");
            return;
        } else
            GET_CLAN(vict) = clan_by_name(argument)->number;
        break;
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
        GET_PAGE_LENGTH(vict) = RANGE(1, 200);
        break;
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
            HUNTING(vict) = vict2;
            send_to_char(ch, "%s now hunts %s.\r\n", GET_NAME(vict),
                GET_NAME(vict2));
        }
        return;
    case 61:
        if (!(vict2 = get_char_vis(ch, argument))) {
            send_to_char(ch, "No such target character around.\r\n");
        } else {
            vict->setFighting(vict2);
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
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_TOUGHGUY);
        break;
    case 69:
        SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOINTWIZ);
        break;
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
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_QUESTOR);
        break;
    case 80:
        vict->player.age_adjust = (byte) RANGE(-125, 125);
        break;
    case 81:
        GET_CASH(vict) = RANGE(0, 1000000000);
        break;
    case 82:
        GET_REMORT_GEN(vict) = RANGE(0, 1000);
        break;
    case 83:
        if (add_path_to_mob(vict, argument)) {
            sprintf(buf, "%s now follows the path titled: %s.",
                GET_NAME(vict), argument);
        } else
            sprintf(buf, "Could not assign that path to mobile.");
        break;
    case 84:
        SET_OR_REMOVE(PRF2_FLAGS(vict), PRF2_LIGHT_READ);
        break;
    case 85:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_REMORT_TOUGHGUY);
        break;
    case 86:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_COUNCIL);
        break;
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
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOSHOUT);
        break;
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
        GET_ECONET(vict) = RANGE(0, 1000000000);
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
        GET_QUEST_POINTS(vict) = RANGE(0, 10000000);
        break;

    case 95:
        GET_QUEST_ALLOWANCE(vict) = RANGE(0, 100);
        break;

    case 96:
        if (IS_NPC(vict)) {
            SET_OR_REMOVE(MOB_FLAGS(vict), MOB_SOULLESS);
        } else {
            SET_OR_REMOVE(PLR2_FLAGS(vict), PLR2_SOULLESS);
        }
        break;
    case 97:
        if (IS_NPC(vict)) {
            send_to_char(ch, "Just kill the bugger!\r\n");
            break;
        } else {
            SET_OR_REMOVE(PLR2_FLAGS(vict), PLR2_BURIED);
            slog("(GC) %s set %s %sburied.", 
                    GET_NAME(ch), 
                    GET_NAME(vict),
                    PLR2_FLAGGED(vict, PLR2_BURIED) ? "" : "UN-"); 
        }
    case 98:                    // Set Speed
        vict->setSpeed(RANGE(0, 100));
        break;
    case 99:
        if( !argument || !*argument ) {
            send_to_char(ch, "1. BUILDER\r\n2. CODER\r\n3. ADMIN\r\n4. QUESTOR\r\n"
                             "5. P ARCH\r\n6. EC ARCH\r\n7. OP ARCH\r\n8. <custom>\r\n");
            return;
        } else if (IS_NPC(vict)) {
            send_to_char( ch, "As good an idea as it might seem, it just won't work.\r\n");
            return;
        } else {
            vict->player_specials->saved.occupation = RANGE(0, 254);
        }
        break;

    case 100:
        name = tmp_getquoted(&argument);
        arg1 = tmp_getword(&argument);
        perform_skillset(ch, vict, name, atoi(arg1));
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

    if (!is_file && !IS_NPC(vict))
        save_char(vict, NULL);

    if (is_file) {
        char_to_store(vict, &tmp_store);
        fseek(player_fl, (player_i) * sizeof(struct char_file_u), SEEK_SET);
        fwrite(&tmp_store, sizeof(struct char_file_u), 1, player_fl);
        free_char(cbuf);
        send_to_char(ch, "Saved in file.\r\n");
    }
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
    char out_list[MAX_STRING_LENGTH];
    bool stop = FALSE;

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
    strcpy(out_list, "");
    for (zone = zone_table; zone && !stop; zone = zone->next)
        for (room = zone->world; room; room = room->next) {
            if (room->number > last) {
                stop = TRUE;
                break;
            }
            if (room->number >= first) {
                sprintf(buf, "%5d. %s[%s%5d%s]%s %s%-30s%s %s \r\n", ++found,
                    CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), room->number,
                    CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                    CCCYN(ch, C_NRM), room->name, CCNRM(ch, C_NRM),
                    room->description ? "" : "(nodesc)");
                if ((strlen(out_list) + strlen(buf)) < MAX_STRING_LENGTH - 20)
                    strcat(out_list, buf);
                else if (strlen(out_list) < (MAX_STRING_LENGTH - 20)) {
                    strcat(out_list, "**OVERFLOW**\r\n");
                    stop = TRUE;
                    break;
                }
            }
        }

    if (!found)
        send_to_char(ch, "No rooms were found in those parameters.\r\n");
    else
        page_string(ch->desc, out_list);
}


ACMD(do_xlist)
{
    struct special_search_data *srch = NULL;
    struct room_data *room = NULL;
    struct zone_data *zone = NULL, *zn = NULL;
    int count = 0, srch_type = -1;
    char outbuf[MAX_STRING_LENGTH], arg1[MAX_INPUT_LENGTH];
    bool overflow = 0, found = 0;

//    strcpy( argument, strcat( value, argument ) );
    argument = one_argument(argument, arg1);
    while ((*arg1)) {
        if (is_abbrev(arg1, "zone")) {
            argument = one_argument(argument, arg1);
            if (!*arg1) {
                send_to_char(ch, "No zone specified.\r\n");
                return;
            }
            if (!(zn = real_zone(atoi(arg1)))) {
                send_to_char(ch, "No such zone ( %s ).\r\n", arg1);
                return;
            }
        }
        if (is_abbrev(arg1, "type")) {
            argument = one_argument(argument, arg1);
            if (!*arg1) {
                send_to_char(ch, "No type specified.\r\n");
                return;
            }
            if ((srch_type = search_block(arg1, search_commands, FALSE)) < 0) {
                send_to_char(ch, "No such search type ( %s ).\r\n", arg1);
                return;
            }
        }
        if (is_abbrev(arg1, "help")) {
            send_to_char(ch, "Usage: xlist [type <type>] [zone <zone>]\r\n");
        }
        argument = one_argument(argument, arg1);
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

    strcpy(outbuf, "Searches:\r\n");


    for (room = zone->world, found = FALSE; room && !overflow;
        found = FALSE, room = room->next) {

        for (srch = room->search, count = 0; srch && !overflow;
            srch = srch->next, count++) {

            if (srch_type >= 0 && srch_type != srch->command)
                continue;

            if (!found) {
                sprintf(buf, "Room [%s%5d%s]:\n", CCCYN(ch, C_NRM),
                    room->number, CCNRM(ch, C_NRM));
                strcat(outbuf, buf);
                found = TRUE;
            }

            print_search_data_to_buf(ch, room, srch, buf);

            if (strlen(outbuf) + strlen(buf) > MAX_STRING_LENGTH - 128) {
                overflow = 1;
                strcat(outbuf, "**OVERFLOW**\r\n");
                break;
            }
            strcat(outbuf, buf);
        }
    }
    page_string(ch->desc, outbuf);
}

ACMD(do_mlist)
{
    char out_list[MAX_STRING_LENGTH];

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

    strcpy(out_list, "");
    CreatureList::iterator mit = mobilePrototypes.begin();
    for (;
        mit != mobilePrototypes.end()
        && ((*mit)->mob_specials.shared->vnum <= last); ++mit) {
        if ((*mit)->mob_specials.shared->vnum >= first) {
            sprintf(buf, "%5d. %s[%s%5d%s]%s %-40s%s  [%2d] <%3d> %s\r\n",
                ++found, CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                (*mit)->mob_specials.shared->vnum, CCGRN(ch, C_NRM), CCYEL(ch,
                    C_NRM), (*mit)->player.short_descr, CCNRM(ch, C_NRM),
                (*mit)->player.level, MOB_SHARED((*mit))->number,
                MOB2_FLAGGED((*mit), MOB2_UNAPPROVED) ? "(!ap)" : "");
            if ((strlen(out_list) + strlen(buf)) < MAX_STRING_LENGTH - 20)
                strcat(out_list, buf);
            else if (strlen(out_list) < (MAX_STRING_LENGTH - 20)) {
                strcat(out_list, "**OVERFLOW**\r\n");
                break;
            }
        }
    }

    if (!found)
        send_to_char(ch, "No mobiles were found in those parameters.\r\n");
    else
        page_string(ch->desc, out_list);
}


ACMD(do_olist)
{
    extern struct obj_data *obj_proto;
    struct obj_data *obj = NULL;
    char out_list[MAX_STRING_LENGTH];

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

    strcpy(out_list, "");
    for (obj = obj_proto; obj && (obj->shared->vnum <= last); obj = obj->next) {
        if (obj->shared->vnum >= first) {
            sprintf(buf, "%5d. %s[%s%5d%s]%s %-36s%s %s %s\r\n", ++found,
                CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), obj->shared->vnum,
                CCGRN(ch, C_NRM), CCGRN(ch, C_NRM),
                obj->short_description, CCNRM(ch, C_NRM),
                !P_OBJ_APPROVED(obj) ? "(!aprvd)" : "", (!(obj->description)
                    || !(*(obj->description))) ? "(nodesc)" : "");
            if ((strlen(out_list) + strlen(buf)) < MAX_STRING_LENGTH - 20)
                strcat(out_list, buf);
            else if (strlen(out_list) < (MAX_STRING_LENGTH - 20)) {
                strcat(out_list, "**OVERFLOW**\r\n");
                break;
            }
        }
    }

    if (!found)
        send_to_char(ch, "No objects were found in those parameters.\r\n");
    else
        page_string(ch->desc, out_list);
}


ACMD(do_rename)
{
    struct obj_data *obj;
    struct Creature *vict = NULL;
    char new_desc[MAX_INPUT_LENGTH], logbuf[MAX_STRING_LENGTH];

    half_chop(argument, arg, new_desc);
    delete_doubledollar(new_desc);

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
            obj->short_description, new_desc);
        obj->short_description = str_dup(new_desc);
        sprintf(buf, "%s has been left here.", new_desc);
        strcpy(buf, CAP(buf));
        obj->description = str_dup(buf);
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

    //two_arguments(argument, arg, new_name);
    if(!args.hasNext()) {
        send_to_char(ch, "Addname usage: addname <target> <strings>\r\n");
        return;
    }
    
    //arg = the target
    args.next(arg);
    
    if(!args.hasNext()) {
        send_to_char(ch, "What name keywords do you want to add?\r\b");
        return;
    }
    
    while(args.next(new_name)) {
        delete_doubledollar(new_name);

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
            
            if(strstr(obj->name, new_name) != NULL) {
                send_to_char(ch, "Name: \'%s\' is already an alias.\r\n", new_name);
                continue;
            }
            snprintf(buf, EXDSCR_LENGTH, "%s %s", obj->name, new_name);
            obj->name = str_dup(buf);
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
    send_to_char(ch, "Bit %s for %s now %s.\r\n", new_pos, obj->short_description,
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
        obj->short_description);
    return;
}

ACMD(do_menu)
{
    struct Creature *vict;

    skip_spaces(&argument);

    if (!*argument) {
        send_to_char(ch, "Who would you like to send the menu to?\r\n");
        return;
    }
    if ((vict = get_char_vis(ch, argument))) {
        if (vict->desc && GET_LEVEL(ch) > GET_LEVEL(vict) ) {
            SEND_TO_Q("\033[H\033[J", vict->desc);
            show_menu(vict->desc);
            send_to_char(ch, "Okay.  Menu sent.\r\n");
        } else
            send_to_char(ch, 
                "There is no link from this character to a player.\r\n");
    } else
        send_to_char(ch, "Hmm... No one here seems to be going by that name.\r\n");
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
                (*mit)->extract(true, false, CON_MENU);
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
                    (*it)->extract(true, false, CON_MENU);
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
            sprintf(buf, "Object: %s [%s] %s", obj->short_description,
                obj->obj_flags.tracker.string,
                ctime(&obj->obj_flags.tracker.lost_time));
#else
            send_to_char(ch, "Object: %s\r\n", obj->short_description);
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
    bool internal = 0;

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
        if (obj == GET_IMPLANT(obj->worn_by, obj->worn_on))
            internal = 1;
        unequip_char(vict, where_worn, internal);
    }
    perform_oset(ch, obj, argument, NORMAL_OSET);
    if (vict) {
        if (equip_char(vict, obj, where_worn, internal))
            return;
        if (!IS_NPC(vict))
            save_char(vict, NULL);
    }

}

static const char *show_mob_keys[] = {
    "hitroll",
    "damroll",
    "gold",
    "flags",
    "extreme",
    "special",
    "\n"
};

#define NUM_SHOW_MOB 6

void
do_show_mobiles(struct Creature *ch, char *value, char *arg)
{

    int i, j, k, l, command;
    struct Creature *mob = NULL;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    CreatureList::iterator mit;
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
            mob = *mit;
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
            mob = *mit;

            if (MOB2_FLAGGED(mob, MOB2_UNAPPROVED))
                continue;
            if (GET_GOLD(mob) >= k &&
                (!j || shop_keeper != mob->mob_specials.shared->func))
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
            mob = *mit;

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
            mob = *mit;
            if (shop_keeper == mob->mob_specials.shared->func ||
                MOB2_FLAGGED(mob, MOB2_UNAPPROVED))
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
            mob = *mit;
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
        if (FIGHTING((*it))) {
            stop_fighting(*it);
            found = 1;
        }
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

    if (GET_IDNUM(ch) != GET_LAST_TELL(vict))
        send_to_char(ch, "You are not on that person's reply buffer, pal.\r\n");
    else {
        GET_LAST_TELL(vict) = -1;
        send_to_char(ch, "Reply severed.\r\n");
    }
}

ACMD(do_qpreload)
{

    qp_reload();
    send_to_char(ch, "Reloading Quest Points.\r\n");
    mudlog(LVL_GRGOD, NRM, true, "(GC) %s has reloaded QP", GET_NAME(ch));

}


void
stat_obj_to_file(struct obj_data *j, ofstream & out)
{

    int i, found;
    struct extra_descr_data *desc;
    extern struct attack_hit_type attack_hit_text[];

    sprintf(buf, "Name: %s', Aliases: %s\r\n",
        ((j->short_description) ? j->short_description : "<None>"), j->name);
    out << buf;
    sprinttype(GET_OBJ_TYPE(j), item_types, buf1);

    strcpy(buf2, j->shared->func ?
        ((i = find_spec_index_ptr(j->shared->func)) < 0 ? "Exists" :
            spec_list[i].tag) : "None");

    sprintf(buf, "VNum: [%5d], Exist: [%3d/%3d], Type: %s, SpecProc: %s\r\n",
        GET_OBJ_VNUM(j),
        j->shared->number, j->shared->house_count, buf1, buf2);
    out << buf;
    sprintf(buf, "L-Des: %s\r\n",
        ((j->description) ? j->description : "None"));
    out << buf;

    if (j->action_description) {
        sprintf(buf, "Action desc: %s\r\n", j->action_description);
        out << buf;
    }

    if (j->ex_description) {
        sprintf(buf, "Extra descs:");
        for (desc = j->ex_description; desc; desc = desc->next) {
            strcat(buf, " ");
            strcat(buf, desc->keyword);
            strcat(buf, ";");
        }
        out << buf << "\r\n";
    }

    if (!j->description) {
        out << "**This object currently has no description**\r\n";
    }

    out << "Can be worn on: ";
    sprintbit(j->obj_flags.wear_flags, wear_bits, buf);
    out << buf << "\r\n";

    strcpy(buf, "Set char bits : ");
    found = FALSE;
    if (j->obj_flags.bitvector[0]) {
        sprintbit(j->obj_flags.bitvector[0], affected_bits, buf2);
        strcat(buf, strcat(buf2, "  "));
        found = TRUE;
    }
    if (j->obj_flags.bitvector[1]) {
        sprintbit(j->obj_flags.bitvector[1], affected2_bits, buf2);
        strcat(buf, strcat(buf2, "  "));
        found = TRUE;
    }
    if (j->obj_flags.bitvector[2]) {
        sprintbit(j->obj_flags.bitvector[2], affected3_bits, buf2);
        strcat(buf, buf2);
        found = TRUE;
    }
    if (found) {
        out << buf << "\r\n";
    }

    out << "Extra flags   : ";
    sprintbit(GET_OBJ_EXTRA(j), extra_bits, buf);
    out << buf << "\r\n";

    out << "Extra2 flags   : ";
    sprintbit(GET_OBJ_EXTRA2(j), extra2_bits, buf);
    out << buf << "\r\n";

    out << "Extra3 flags   : ";
    sprintbit(GET_OBJ_EXTRA3(j), extra3_bits, buf);
    out << buf << "\r\n";

    sprintf(buf, "Weight: %d, Cost: %d (%d), Rent: %d, Timer: %d\r\n",
        j->getWeight(), GET_OBJ_COST(j),
        prototype_obj_value(j), GET_OBJ_RENT(j), GET_OBJ_TIMER(j));
    out << buf << "\r\n";


    sprintf(buf, "Material: [%s (%d)], Maxdamage: [%d (%d)], Damage: [%d]\r\n",
        material_names[GET_OBJ_MATERIAL(j)],
        GET_OBJ_MATERIAL(j),
        GET_OBJ_MAX_DAM(j), set_maxdamage(j), GET_OBJ_DAM(j));
    out << buf;

    switch (GET_OBJ_TYPE(j)) {
    case ITEM_LIGHT:
        sprintf(buf, "Color: [%d], Type: [%d], Hours: [%d], Value3: [%d]",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2),
            GET_OBJ_VAL(j, 3));
        break;
    case ITEM_SCROLL:
    case ITEM_POTION:
    case ITEM_PILL:
    case ITEM_SYRINGE:
        sprintf(buf, "Level: %d, Spells: %s(%d), %s(%d), %s(%d)",
            GET_OBJ_VAL(j, 0),
            (GET_OBJ_VAL(j, 1) > 0) ? spell_to_str((int)GET_OBJ_VAL(j, 1)) :
            "None", GET_OBJ_VAL(j, 1),
            (GET_OBJ_VAL(j, 2) > 0) ? spell_to_str((int)GET_OBJ_VAL(j, 2)) :
            "None", GET_OBJ_VAL(j, 2),
            (GET_OBJ_VAL(j, 3) > 0) ? spell_to_str((int)GET_OBJ_VAL(j, 3)) :
            "None", GET_OBJ_VAL(j, 3));
        break;
    case ITEM_WAND:
    case ITEM_STAFF:
        sprintf(buf,
            "Level: %d, Max Charge: %d, Current Charge: %d, Spell: %s",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2),
            spell_to_str((int)GET_OBJ_VAL(j, 3)));
        break;
    case ITEM_FIREWEAPON:
    case ITEM_WEAPON:
        sprintf(buf,
            "Spell: %s (%d), Todam: %dd%d (av %d), Damage Type: %s (%d)",
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
    case ITEM_MISSILE:
        sprintf(buf, "Tohit: %d, Todam: %d, Type: %d", GET_OBJ_VAL(j, 0),
            GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 3));
        break;
    case ITEM_ARMOR:
        sprintf(buf, "AC-apply: [%d]", GET_OBJ_VAL(j, 0));
        break;
    case ITEM_TRAP:
        sprintf(buf, "Spell: %d, - Hitpoints: %d",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
        break;
    case ITEM_CONTAINER:
        sprintf(buf,
            "Max-contains: %d, Locktype: %d, Key/Owner: %d, Corpse: %s, Killer: %d",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2),
            GET_OBJ_VAL(j, 3) ? "Yes" : "No", CORPSE_KILLER(j));
        break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
        sprinttype(GET_OBJ_VAL(j, 2), drinks, buf2);
        sprintf(buf,
            "Max-contains: %d, Contains: %d, Liquid: %s(%d), Poisoned: %s (%d)",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), buf2, GET_OBJ_VAL(j, 2),
            GET_OBJ_VAL(j, 3) ? "Yes" : "No", GET_OBJ_VAL(j, 3));
        break;
        /*  case ITEM_NOTE:
           sprintf(buf, "Tongue: %d", GET_OBJ_VAL(j, 0));
           break; */
    case ITEM_KEY:
        sprintf(buf, "Keytype: %d, Rentable: %s, Car Number: %d",
            GET_OBJ_VAL(j, 0), YESNO(GET_OBJ_VAL(j, 1)), GET_OBJ_VAL(j, 2));
        break;
    case ITEM_FOOD:
        sprintf(buf,
            "Makes full: %d, Value 1: %d, Spell : %s(%d), Poisoned: %d",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), spell_to_str((int)GET_OBJ_VAL(j,
                    2)), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
        break;
    case ITEM_HOLY_SYMB:
        sprintf(buf,
            "Alignment: %s(%d), Class: %s(%d), Min Level: %d, Max Level: %d ",
            alignments[GET_OBJ_VAL(j, 0)], GET_OBJ_VAL(j, 0),
            char_class_abbrevs[(int)GET_OBJ_VAL(j, 1)], GET_OBJ_VAL(j, 1),
            GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
        break;
    case ITEM_BATTERY:
        sprintf(buf, "Max_Energy: %d, Cur_Energy: %d, Rate: %d, Cost/Unit: %d",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1),
            GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
        break;
    case ITEM_VEHICLE:
        sprintbit(GET_OBJ_VAL(j, 2), vehicle_types, buf2);
        sprintf(buf, "Room/Key Number: %d, Door State: %d, Type: %s, v3: %d",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), buf2, GET_OBJ_VAL(j, 3));
        break;
    case ITEM_ENGINE:
        sprintbit(GET_OBJ_VAL(j, 2), engine_flags, buf2);
        sprintf(buf,
            "Max_Energy: %d, Cur_Energy: %d, Run_State: %s, Consume_rate: %d",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), buf2, GET_OBJ_VAL(j, 3));
        break;
    case ITEM_BOMB:
        sprintf(buf, "Values: %s:[%s(%d)] %s:[%d] %s:[%d] %s:[%d]",
            item_value_types[(int)GET_OBJ_TYPE(j)][0],
            bomb_types[(int)GET_OBJ_VAL(j, 0)], GET_OBJ_VAL(j, 0),
            item_value_types[(int)GET_OBJ_TYPE(j)][1], GET_OBJ_VAL(j, 1),
            item_value_types[(int)GET_OBJ_TYPE(j)][2], GET_OBJ_VAL(j, 2),
            item_value_types[(int)GET_OBJ_TYPE(j)][3], GET_OBJ_VAL(j, 3));
        break;
    case ITEM_FUSE:
        sprintf(buf, "Values: %s:[%s(%d)] %s:[%d] %s:[%d] %s:[%d]",
            item_value_types[(int)GET_OBJ_TYPE(j)][0],
            fuse_types[(int)GET_OBJ_VAL(j, 0)], GET_OBJ_VAL(j, 0),
            item_value_types[(int)GET_OBJ_TYPE(j)][1], GET_OBJ_VAL(j, 1),
            item_value_types[(int)GET_OBJ_TYPE(j)][2], GET_OBJ_VAL(j, 2),
            item_value_types[(int)GET_OBJ_TYPE(j)][3], GET_OBJ_VAL(j, 3));
        break;

    case ITEM_TOBACCO:
        sprintf(buf, "Values: %s:[%s(%d)] %s:[%d] %s:[%d] %s:[%d]",
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
        sprintf(buf, "Values 0-3: %s:[%d] %s:[%d] %s:[%d] %s:[%s (%d)]",
            item_value_types[(int)GET_OBJ_TYPE(j)][0], GET_OBJ_VAL(j, 0),
            item_value_types[(int)GET_OBJ_TYPE(j)][1], GET_OBJ_VAL(j, 1),
            item_value_types[(int)GET_OBJ_TYPE(j)][2], GET_OBJ_VAL(j, 2),
            item_value_types[(int)GET_OBJ_TYPE(j)][3],
            (GET_OBJ_VAL(j, 3) >= 0 && GET_OBJ_VAL(j, 3) < NUM_GUN_TYPES) ?
            gun_types[GET_OBJ_VAL(j, 3)] : "ERROR", GET_OBJ_VAL(j, 3));
        break;
    case ITEM_INTERFACE:
        sprintf(buf, "Values 0-3: %s:[%s (%d)] %s:[%d] %s:[%d] %s:[%d]",
            item_value_types[(int)GET_OBJ_TYPE(j)][0],
            GET_OBJ_VAL(j, 0) >= 0 && GET_OBJ_VAL(j, 0) < NUM_INTERFACES ?
            interface_types[GET_OBJ_VAL(j, 0)] : "Error",
            GET_OBJ_VAL(j, 0),
            item_value_types[(int)GET_OBJ_TYPE(j)][1], GET_OBJ_VAL(j, 1),
            item_value_types[(int)GET_OBJ_TYPE(j)][2], GET_OBJ_VAL(j, 2),
            item_value_types[(int)GET_OBJ_TYPE(j)][3], GET_OBJ_VAL(j, 3));
        break;
    case ITEM_MICROCHIP:
        sprintf(buf, "Values 0-3: %s:[%s (%d)] %s:[%d] %s:[%d] %s:[%d]",
            item_value_types[(int)GET_OBJ_TYPE(j)][0],
            GET_OBJ_VAL(j, 0) >= 0 && GET_OBJ_VAL(j, 0) < NUM_CHIPS ?
            microchip_types[GET_OBJ_VAL(j, 0)] : "Error",
            GET_OBJ_VAL(j, 0),
            item_value_types[(int)GET_OBJ_TYPE(j)][1], GET_OBJ_VAL(j, 1),
            item_value_types[(int)GET_OBJ_TYPE(j)][2], GET_OBJ_VAL(j, 2),
            item_value_types[(int)GET_OBJ_TYPE(j)][3], GET_OBJ_VAL(j, 3));
        break;

    default:
        sprintf(buf, "Values 0-3: %s:[%d] %s:[%d] %s:[%d] %s:[%d]",
            item_value_types[(int)GET_OBJ_TYPE(j)][0], GET_OBJ_VAL(j, 0),
            item_value_types[(int)GET_OBJ_TYPE(j)][1], GET_OBJ_VAL(j, 1),
            item_value_types[(int)GET_OBJ_TYPE(j)][2], GET_OBJ_VAL(j, 2),
            item_value_types[(int)GET_OBJ_TYPE(j)][3], GET_OBJ_VAL(j, 3));
        break;
    }
    out << buf << "\r\n";

    found = 0;
    out << "Affections:";
    for (i = 0; i < MAX_OBJ_AFFECT; i++)
        if (j->affected[i].modifier) {
            sprinttype(j->affected[i].location, apply_types, buf2);
            sprintf(buf, "%s %+d to %s", found++ ? "," : "",
                j->affected[i].modifier, buf2);
            out << buf;
        }
    if (!found)
        out << " None";

    out << "\r\n";

    if (OBJ_SOILAGE(j)) {
        found = FALSE;
        strcpy(buf2, "Soilage: ");
        for (i = 0; i < 16; i++) {
            if (OBJ_SOILED(j, (1 << i))) {
                if (found)
                    strcat(buf2, ", ");
                else
                    found = TRUE;
                strcat(buf2, soilage_bits[i]);
            }
        }
        out << buf2 << "\r\n";

    }
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
                    "      export - Exports the current player file to XML.\r\n"
                    "      rebuild - Rebuilds the player table from xml.\r\n"
                    "      tick - forces a mud-wide tick to occur.\r\n"
                    ;

ACMD(do_coderutil)
{
    Tokenizer tokens(argument);
    char token[MAX_INPUT_LENGTH];
    
    if(! tokens.next(token) ) {
        send_to_char( ch, CODER_UTIL_USAGE );
        return;
    }

    if( strcmp( token, "export" )  == 0 ) {
        if(ch->getLevel() < 72 ){
            send_to_char(ch, "You can't do that here.\r\n");
        } else {
            export_player_table(ch);
        }
    } else if( strcmp( token, "reload" )  == 0 ) {
        playerIndex.clear();
        build_player_table();
        send_to_char(ch, "Reloaded.\r\n");
    } else if ( strcmp( token, "tick" ) == 0 ) {
        point_update();
    } else {
        send_to_char( ch, CODER_UTIL_USAGE );
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
                  save_char(ch, NULL);
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
              GET_REMORT_GEN(ch) = atoi(arg2);
              send_to_char(ch, "gen set to %d.\r\n", GET_REMORT_GEN(ch));
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

ACMD(do_spechelp)
{
    int spec_idx;

    if (!argument || !*argument) {
        send_to_char(ch, "What special would you like help on?\r\n");
        return;
    }

    spec_idx = find_spec_index_arg(tmp_getword(&argument));
    if (spec_idx == -1) {
        send_to_char(ch, "That special does not exist.\r\n");
        return;
    }

    if (!(spec_list[spec_idx].func(ch, NULL, 0, 0, SPECIAL_HELP)))
        send_to_char(ch, "There is no help available for that special.\r\n");
}

int do_freeze_char(char *argument, Creature *vict, Creature *ch)
{
    static const int ONE_MINUTE = 60;
    static const int ONE_HOUR   = 3600;
    static const int ONE_DAY    = 86400;
    static const int ONE_YEAR   = 31536000;

    char arg1[256], arg2[256];
    char *ptr;
    time_t freeze_time, thaw_time;
    int hours, minutes, seconds, days;
     
    memset(arg1, 0x0, sizeof(arg1));
    memset(arg2, 0x0, sizeof(arg2));

    freeze_time = thaw_time = time(NULL);

    if (ch == vict) {
        send_to_char(ch, "Oh, yeah, THAT'S real smart...\r\n");
        return 0;
    }

    argument = two_arguments(argument, arg1, arg2);

    if (!*arg2) {
        thaw_time += ONE_DAY;
    }
    else if (isname_exact(arg2, "forever")) {
        thaw_time = -1;
    }
    else {
        ptr = arg2;
        while (*(ptr + 1) != '\0') {
            if (!isdigit(*ptr)) {
                send_to_char(ch, 
                             "Usage: freeze <target> [ XXXs | XXXm | XXXh | XXXd | forever]\r\n");
                return 0;
            }
            ptr++;
        }
        
        switch (*ptr) {
            case 's':
                *ptr = '\0';
                thaw_time += atoi(arg2);
            break;
            case 'm':
                *ptr = '\0';
                thaw_time += (atoi(arg2) * ONE_MINUTE);
            break;
            case 'h':
                *ptr = '\0';
                thaw_time += (atoi(arg2) * ONE_HOUR);
            break;
            case 'd':
                *ptr = '\0';
                thaw_time += (atoi(arg2) * ONE_DAY);
            break;
            default:
                send_to_char(ch, 
                             "Usage: freeze <target> [ XXXs | XXXm | XXXh | XXXd | forever]\r\n");
                return 0;
        }

        if ((thaw_time - freeze_time) >= ONE_YEAR) {
            send_to_char(ch, "Come now, Let's be reasonable, shall we?\r\n");
            return 0;
        }
    }
    
    time_t freeze_secs = thaw_time - freeze_time;

    days = freeze_secs / ONE_DAY;
    hours = (freeze_secs / ONE_HOUR) - (days * 24);
    minutes = (freeze_secs / ONE_MINUTE) - (days * (24 * ONE_MINUTE)) - (hours * ONE_MINUTE);
    seconds = freeze_secs - (days * (24 * ONE_MINUTE * 60)) - (hours * ONE_MINUTE * 60) -
              (minutes * ONE_MINUTE);
    
    SET_BIT(PLR_FLAGS(vict), PLR_FROZEN);
    GET_FREEZE_LEV(vict) = GET_LEVEL(ch);
    vict->player_specials->thaw_time = thaw_time;
    vict->player_specials->freezer_id = GET_IDNUM(ch);
     
    send_to_char(vict, "A bitter wind suddenly rises and drains every erg "
                       "of heat from your body!\r\nYou feel frozen!\r\n");
    
    send_to_char(ch, "%s frozen for %3d days, %2d hours, %2d minutes, %2d seconds.\r\n", 
                 vict->player.name, days, hours, minutes, seconds);
    
    act("A sudden cold wind conjured from nowhere freezes $n!", FALSE,
        vict, 0, 0, TO_ROOM);
    
    mudlog(MAX(LVL_POWER, GET_INVIS_LVL(ch)), BRF, true, "(GC) %s frozen by %s.", 
           GET_NAME(vict), GET_NAME(ch)); 
 
    return 1;
}
