#ifdef HAS_CONFIG_H
#endif

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <libpq-fe.h>
#include <libxml/parser.h>
#include <glib.h>

#include "interpreter.h"
#include "structs.h"
#include "utils.h"
#include "constants.h"
#include "comm.h"
#include "security.h"
#include "handler.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "zone_data.h"
#include "race.h"
#include "creature.h"
#include "db.h"
#include "house.h"
#include "players.h"
#include "tmpstr.h"
#include "spells.h"
#include "fight.h"
#include "obj_data.h"
#include "specs.h"
#include "actions.h"

struct cityguard_data {
    int targ_room;
};

// Tracks the characters who are currently under arrest by cityguards
struct memory_rec *under_arrest = NULL;

SPECIAL(cityguard);
SPECIAL(guard);

// Cityguards have the following functions:
// - stop all fights not involving a cityguard
// - they respond to calls for help by going to the room the call originated
// - when fighting, they call other guards
// - when they detect another cityguard in combat, they stun the attacker
// - when they detect a stunned criminal, they haul the person to jail
// - when idle, they do various amusing things
// - when the guard dies, specified mobs are loaded in the hq ready to move
//   to the death point

void
summon_cityguards(struct room_data *room)
{
    GList *it;
    struct cityguard_data *data;
    int distance;

    // Now get about half the cityguards in the zone to respond
    for (it = first_living(creatures); it; it = next_living(it)) {
        struct creature *tch = it->data;
        if (!tch->in_room || tch->in_room->zone != room->zone)
            continue;
        if (tch->in_room == room)
            continue;
        if (GET_NPC_SPEC(tch) != cityguard)
            continue;
        // the closer they are, the more likely they are to respond
        distance = find_distance(tch->in_room, room);
        if (distance < 1)
            continue;
        if (number(1, distance / 2) == 1) {
            data = (struct cityguard_data *)(tch->mob_specials.func_data);
            if (data && !data->targ_room)
                data->targ_room = room->number;
        }
    }
}

bool
char_is_arrested(struct creature *ch)
{
    struct memory_rec *cur_mem;

    if (IS_NPC(ch))
        return false;

    for (cur_mem = under_arrest; cur_mem; cur_mem = cur_mem->next)
        if (cur_mem->id == GET_IDNUM(ch))
            return true;

    return false;
}

void
char_under_arrest(struct creature *ch)
{
    struct memory_rec *new_mem;

    if (IS_NPC(ch))
        return;

    if (char_is_arrested(ch))
        return;

    CREATE(new_mem, struct memory_rec, 1);
    new_mem->id = GET_IDNUM(ch);
    new_mem->next = under_arrest;
    under_arrest = new_mem;
}

void
char_arrest_pardoned(struct creature *ch)
{
    struct memory_rec *cur_mem, *next_mem;

    if (IS_NPC(ch) || !under_arrest)
        return;

    if (under_arrest->id == GET_IDNUM(ch)) {
        cur_mem = under_arrest->next;
        free(under_arrest);
        under_arrest = cur_mem;
    } else {
        cur_mem = under_arrest;
        while (cur_mem->next && cur_mem->next->id != GET_IDNUM(ch))
            cur_mem = cur_mem->next;
        if (cur_mem->next) {
            next_mem = cur_mem->next->next;
            free(cur_mem->next);
            cur_mem->next = next_mem;
        }
    }
}

void
call_for_help(struct creature *ch, struct creature *attacker)
{
    const char *msg;

    // Emit a shout for help
    if (GET_NPC_SPEC(ch) == cityguard || GET_NPC_SPEC(ch) == guard) {
        switch (number(0, 10)) {
        case 0:
            msg = "To arms!  To arms!  We are under attack!";
            break;
        case 1:
            msg = "Protect the city!";
            break;
        case 2:
            if (IS_GOOD(ch))
                msg = "By Alron's will, we shall defend our fair city!";
            else if (IS_EVIL(ch))
                msg = "By Veloth, this city is ours by right!";
            else
                msg = "You shall not prevail!";
            break;
        case 3:
            msg = "Death to %s!";
            break;
        default:
            msg = "Aid me my comrades!";
            break;
        }
    } else {
        switch (number(0, 10)) {
        case 0:
            msg = "Help!  Help!  %s is trying to kill me!";
            break;
        case 1:
            msg = "I'm being attacked!  Save me!";
            break;
        case 2:
            msg = "Help me!  I'm being attacked!";
            break;
        default:
            msg = "Will no one help me?!";
            break;
        }
    }
    do_gen_comm(ch, tmp_sprintf(msg, GET_DISGUISED_NAME(ch, attacker)),
        0, SCMD_SHOUT);
    summon_cityguards(ch->in_room);
}

void
breakup_fight(struct creature *ch, struct creature *vict1,
    struct creature *vict2)
{
    GList *it;
    struct creature *tch;

    for (it = first_living(ch->in_room->people); it; it = next_living(it)) {
        tch = it->data;
        if (tch == ch)
            send_to_char(tch,
                "You break up the fight between %s and %s.\r\n",
                PERS(vict1, tch), PERS(vict2, tch));
        else if (tch == vict1)
            send_to_char(tch,
                "%s gets between you and %s, ending the fight!\r\n",
                PERS(ch, tch), PERS(vict2, tch));
        else if (tch == vict2)
            send_to_char(tch,
                "%s gets between you and %s, ending the fight!\r\n",
                PERS(ch, tch), PERS(vict1, tch));
        else
            send_to_char(tch,
                "%s gets between %s and %s, ending the fight!\r\n",
                PERS(ch, tch), PERS(vict1, tch), PERS(vict2, tch));

    }

    remove_combat(vict1, vict2);
    remove_combat(vict2, vict1);
}

int
throw_char_in_jail(struct creature *ch, struct creature *vict)
{
    room_num jail_cells[6] = { 10908, 10910, 10911, 10921, 10920, 10919 };
    struct room_data *locker_room, *cell_room;
    struct obj_data *locker = NULL, *torch = NULL, *obj = NULL, *next_obj =
        NULL;
    int i, count = 0;

    while (count < 12) {
        count++;
        cell_room = real_room(jail_cells[number(0, 5)]);
        if (cell_room == NULL || !cell_room->people)
            continue;

        break;
    }

    if (!cell_room)
        return 0;

    locker_room = real_room(ch->in_room->number + 1);
    if (locker_room)
        locker = read_object(JAIL_LOCKER_VNUM);
    if (locker && locker_room) {
        obj_to_room(locker, locker_room);

        for (i = 0; i < NUM_WEARS; i++) {
            if (GET_EQ(vict, i) && can_see_object(ch, GET_EQ(vict, i))) {
                if (GET_OBJ_TYPE(GET_EQ(vict, i)) == ITEM_KEY &&
                    !GET_OBJ_VAL(GET_EQ(vict, i), 1))
                    extract_obj(GET_EQ(vict, i));
                else if (IS_NPC(vict))
                    extract_obj(GET_EQ(vict, i));
                else if (!IS_OBJ_STAT2(GET_EQ(vict, i), ITEM2_NOREMOVE) &&
                    !IS_OBJ_STAT(GET_EQ(vict, i), ITEM_NODROP))
                    obj_to_obj(unequip_char(vict, i, EQUIP_WORN), locker);
            }
        }
        for (obj = vict->carrying; obj; obj = next_obj) {
            next_obj = obj->next_content;
            if (!IS_OBJ_STAT(obj, ITEM_NODROP) && can_see_object(ch, obj)) {
                if (IS_OBJ_TYPE(obj, ITEM_KEY) && !GET_OBJ_VAL(obj, 1))
                    extract_obj(obj);
                else if (IS_NPC(vict))
                    extract_obj(obj);
                else if (!IS_OBJ_STAT(obj, ITEM_NODROP)) {
                    obj_from_char(obj);
                    obj_to_obj(obj, locker);
                }
            }
        }
        GET_OBJ_VAL(locker, 0) = GET_IDNUM(vict);
        if (locker->contains) {
            act("$n removes all your gear and stores it in a strongbox.",
                false, ch, NULL, vict, TO_VICT);
            struct house *house = find_house_by_room(locker->in_room->number);
            if (house)
                save_house(house);
        } else {
            extract_obj(locker);
        }
    }

    act("$n throws $N into a cell and slams the door!", false, ch, NULL, vict,
        TO_NOTVICT);
    act("$n throws you into a cell and slams the door behind you!", false,
        ch, NULL, vict, TO_VICT);

    char_from_room(vict, false);
    char_to_room(vict, cell_room, false);
    if (IS_PC(vict))
        cell_room->zone->enter_count++;

    act("$n is thrown into the cell, and the door slams shut behind $m!",
        false, vict, NULL, NULL, TO_ROOM);
    affect_from_char(vict, SPELL_SLEEP);
    affect_from_char(vict, SPELL_MELATONIC_FLOOD);
    affect_from_char(vict, SKILL_SLEEPER);
    GET_POSITION(vict) = POS_RESTING;
    act("You wake up in jail, your head pounding.", false, vict, NULL, NULL,
        TO_CHAR);

    if (NPC_HUNTING(ch) && NPC_HUNTING(ch) == vict)
        stop_hunting(ch);

    if ((torch = read_object(3030)))
        obj_to_char(torch, vict);

    mudlog(GET_INVIS_LVL(vict), NRM, true,
        "%s has been thrown into jail by %s at %d.", GET_NAME(vict),
        GET_NAME(ch), ch->in_room->number);

    if (IS_NPC(vict))
        creature_purge(vict, true);
    else
        crashsave(vict);
    return 1;
}

int
drag_char_to_jail(struct creature *ch, struct creature *vict,
    struct room_data *jail_room)
{
    GList *it;
    struct cityguard_data *data;
    int dir;

    if ((IS_NPC(vict) && GET_NPC_SPEC(ch) == GET_NPC_SPEC(vict)) || !jail_room)
        return 0;

    if (ch->in_room == jail_room) {
        if (throw_char_in_jail(ch, vict)) {
            char_arrest_pardoned(vict);
            forget(ch, vict);
            return 1;
        } else
            return 0;
    }

    dir = find_first_step(ch->in_room, jail_room, STD_TRACK);
    if (dir < 0)
        return false;
    if (!CAN_GO(ch, dir)
        || !can_travel_sector(ch, SECT_TYPE(EXIT(ch, dir)->to_room), 0)
        || !CAN_GO(vict, dir)
        || !can_travel_sector(vict, SECT_TYPE(EXIT(ch, dir)->to_room), 0))
        return false;

    act(tmp_sprintf("You drag a semi-conscious $N %s.", to_dirs[dir]), false,
        ch, NULL, vict, TO_CHAR);
    if (!number(0, 1))
        act("You dimly feel yourself being dragged down the street.", false,
            ch, NULL, vict, TO_VICT | TO_SLEEP);
    act(tmp_sprintf("$n drags a semi-conscious $N %s.", to_dirs[dir]), false,
        ch, NULL, vict, TO_NOTVICT);

    // Get other guards to follow
    for (it = first_living(ch->in_room->people); it; it = next_living(it)) {
        struct creature *tch = it->data;
        if (IS_NPC(tch) && GET_NPC_SPEC(tch) == cityguard) {
            data = (struct cityguard_data *)(tch)->mob_specials.func_data;
            if (data)
                data->targ_room = EXIT(vict, dir)->to_room->number;
        }
    }

    char_from_room(ch, false);
    char_to_room(ch, EXIT(vict, dir)->to_room, false);
    char_from_room(vict, false);
    char_to_room(vict, ch->in_room, false);

    act(tmp_sprintf("$n drags $N in from %s.",
            from_dirs[dir]), false, ch, NULL, vict, TO_NOTVICT);
    WAIT_STATE(ch, 1 RL_SEC);
    return true;
}

bool
is_fighting_cityguard(struct creature *ch)
{
    for (GList *it = first_living(ch->fighting); it; it = next_living(it)) {
        struct creature *tch = it->data;
        if (GET_NPC_SPEC(tch) == cityguard)
            return true;
    }
    return false;
}

SPECIAL(cityguard)
{
    struct creature *self = (struct creature *)me;
    struct creature *tch, *target = NULL, *new_guard;
    struct cityguard_data *data;
    char *str, *line, *param_key;
    int action, dir;
    int jail_num = 0, hq_num = 0;
    struct room_data *room;
    bool lawful;
    GList *it;

    if (spec_mode != SPECIAL_TICK && spec_mode != SPECIAL_DEATH)
        return 0;

    str = GET_NPC_PARAM(self);
    if (str) {
        for (line = tmp_getline(&str); line; line = tmp_getline(&str)) {
            param_key = tmp_getword(&line);
            if (!strcmp(param_key, "jailroom"))
                jail_num = atoi(line);
            else if (!strcmp(param_key, "headquarters"))
                hq_num = atoi(line);
        }
    }

    data = (struct cityguard_data *)(self->mob_specials.func_data);
    if (!data) {
        CREATE(data, struct cityguard_data, 1);
        data->targ_room = 0;
        self->mob_specials.func_data = data;
    }

    if (spec_mode == SPECIAL_DEATH) {
        if (!hq_num)
            return 0;

        room = real_room(hq_num);
        if (!room)
            return 0;

        // make new guards that will go to the place of death
        str = GET_NPC_PARAM(self);
        if (str) {
            for (line = tmp_getline(&str); line; line = tmp_getline(&str)) {
                param_key = tmp_getword(&line);
                if (!strcmp(param_key, "deathspawn")) {
                    new_guard = read_mobile(atoi(line));
                    if (new_guard) {
                        CREATE(data, struct cityguard_data, 1);
                        new_guard->mob_specials.func_data = data;
                        data->targ_room = self->in_room->number;
                        char_to_room(new_guard, room, true);
                    }
                }
            }
        }

        return 0;
    }

    if (!AWAKE(self))
        return false;

    // We're fighting someone - call for help
    if (is_fighting(self)) {
        tch = random_opponent(self);

        // Save the newbies from themselves
        if (GET_LEVEL(tch) < 20 && GET_REMORT_GEN(tch) == 0) {
            perform_say(self, "declare", "Here now!");
            remove_combat(self, tch);
            remove_combat(tch, self);
            return true;
        }

        char_under_arrest(tch);

        if (!number(0, 3)) {
            call_for_help(self, tch);
            return true;
        }

        return false;
    }

    if (data->targ_room) {
        if (self->in_room->number != data->targ_room) {
            // We're marching to where someone shouted for help
            dir = find_first_step(self->in_room, real_room(data->targ_room),
                STD_TRACK);
            if (dir >= 0 && NPC_CAN_GO(self, dir)
                && !ROOM_FLAGGED(self->in_room->dir_option[dir]->to_room,
                    ROOM_DEATH)) {
                smart_mobile_move(self, dir);
                return true;
            }
        } else {
            // They're in the room, so stop trying to go to it
            data->targ_room = 0;
        }
    }
    // The action value is in order of importance
    // action == 0 : do emote
    // action == 1 : emote half-criminal
    // action == 2 : stop fight
    // action == 3 : drag creature to jail
    // action == 4 : attack criminal
    // action == 5 : assist guard
    action = 0;
    lawful = !ZONE_FLAGGED(self->in_room->zone, ZONE_NOLAW);

    for (it = first_living(self->in_room->people); it; it = next_living(it)) {
        tch = it->data;
        if (action < 5 && is_fighting_cityguard(tch) &&
            ok_to_attack(self, tch, false)) {
            action = 5;
            target = tch;
        }
        if (action < 4 && ((lawful && IS_CRIMINAL(tch))
                || char_is_arrested(tch))
            && can_see_creature(self, tch)
            && !PRF_FLAGGED(tch, PRF_NOHASSLE)
            && GET_POSITION(ch) > POS_SLEEPING
            && ok_to_attack(self, tch, false)
            ) {
            action = 4;
            target = tch;
        }
        if (action < 3 && ((lawful && IS_CRIMINAL(tch))
                || char_is_arrested(tch))
            && GET_POSITION(ch) <= POS_SLEEPING
            && (GET_LEVEL(tch) >= 20 || GET_REMORT_GEN(tch) > 0)) {
            action = 3;
            target = tch;
        }
        if (action < 2 && can_see_creature(self, tch)
            && !PRF_FLAGGED(tch, PRF_NOHASSLE)
            && is_fighting(tch)) {
            action = 2;
            target = tch;
        }
        if (action < 1 && lawful && can_see_creature(self, tch)
            && !PRF_FLAGGED(tch, PRF_NOHASSLE)
            && reputation_of(tch) >= CRIMINAL_REP / 2) {
            action = 1;
            target = tch;
        }
    }

    switch (action) {
    case 0:
        // do general emote
        if (number(0, 11))
            break;
        target = get_char_random_vis(self, self->in_room);
        if (!target)
            break;
        if (IS_GOOD(self) && IS_THIEF(target)) {
            if (IS_EVIL(target)) {
                act("$n looks at you suspiciously.", false,
                    self, NULL, target, TO_VICT);
                act("$n looks at $N suspiciously.", false,
                    self, NULL, target, TO_NOTVICT);
            } else {
                act("$n looks at you skeptically.", false,
                    self, NULL, target, TO_VICT);
                act("$n looks at $N skeptically.", false,
                    self, NULL, target, TO_NOTVICT);
            }
        } else if (cityguard == GET_NPC_SPEC(target)) {
            act("$n nods at $N.", false, self, NULL, target, TO_NOTVICT);
        } else if (((IS_CLERIC(target) || IS_KNIGHT(target))
                && IS_EVIL(self) == IS_EVIL(target)
                && !IS_NEUTRAL(target))
            || GET_LEVEL(target) >= LVL_AMBASSADOR) {
            act("$n bows before you.", false, self, NULL, target, TO_VICT);
            act("$n bows before $N.", false, self, NULL, target, TO_NOTVICT);
        } else if (IS_EVIL(self) != IS_EVIL(target)) {
            switch (number(0, 2)) {
            case 0:
                act("$n watches you carefully.", false, self, NULL, target,
                    TO_VICT);
                act("$n watches $N carefully.", false,
                    self, NULL, target, TO_NOTVICT);
                break;
            case 1:
                act("$n thoroughly examines you.", false,
                    self, NULL, target, TO_VICT);
                act("$n thoroughly examines $N.", false,
                    self, NULL, target, TO_NOTVICT);
                break;
            case 2:
                act("$n mutters something under $s breath.",
                    false, self, NULL, target, TO_ROOM);
                break;
            }
        }
        return true;
    case 1:
        // emote half-criminal
        if (number(0, 8))
            break;
        if (!number(0, 3)) {
            act("$n growls at you.", false, self, NULL, target, TO_VICT);
            act("$n growls at $N.", false, self, NULL, target, TO_NOTVICT);
        } else if (!number(0, 2)) {
            act("$n cracks $s knuckles.", false, self, NULL, target, TO_ROOM);
        } else if (!number(0, 1) && GET_EQ(self, WEAR_WIELD) &&
            (GET_OBJ_VAL(GET_EQ(self, WEAR_WIELD), 3) ==
                (TYPE_SLASH - TYPE_HIT) ||
                GET_OBJ_VAL(GET_EQ(self, WEAR_WIELD), 3) ==
                (TYPE_PIERCE - TYPE_HIT) ||
                GET_OBJ_VAL(GET_EQ(self, WEAR_WIELD), 3) ==
                (TYPE_STAB - TYPE_HIT))) {
            act("$n sharpens $p while watching $N.",
                false, self, GET_EQ(self, WEAR_WIELD), target, TO_NOTVICT);
            act("$n sharpens $p while watching you.",
                false, self, GET_EQ(self, WEAR_WIELD), target, TO_VICT);
        }
        return true;
    case 2:{
            // stopping fight
            switch (number(0, 2)) {
            case 0:
                perform_say(self, "bellow", "Knock it off!");
                break;
            case 1:
                perform_say(self, "bellow",
                    "Stop disturbing the peace of this city!");
                break;
            case 2:
                perform_say(self, "declare", "Here now!");
                break;
            }
            struct creature *vict = random_opponent(target);
            if (vict)
                breakup_fight(self, target, vict);
            return true;
        }
    case 3:
        // drag criminal to jail
        if (GET_POSITION(target) < POS_FIGHTING && !GET_QUEST(ch)
            && jail_num > 0)
            drag_char_to_jail(self, target, real_room(jail_num));
        return true;
    case 4:
        // attack criminal
        if (char_is_arrested(target)) {
            act("$n screams 'HEY!!!  I know who you are!!!!'",
                false, self, NULL, NULL, TO_ROOM);
            hit(self, target, TYPE_UNDEFINED);
        } else {
            act("$n screams 'HEY!!!  You're one of those CRIMINALS!!!!!!'",
                false, self, NULL, NULL, TO_ROOM);
            char_under_arrest(target);
            hit(self, target, TYPE_UNDEFINED);
        }
        return true;
    case 5:
        // assist other cityguard
        if (!number(0, 10)) {
            if (number(0, 1))
                perform_say(self, "yell", "To arms!  To arms!!");
            else
                perform_say(self, "yell", "BAAAANNNZZZZZAAAAAIIIIII!!!");
        }

        if (number(0, 1))
            hit(self, target, TYPE_UNDEFINED);

        return true;
    default:
        errlog("Can't happen at %s:%d", __FILE__, __LINE__);
        break;
    }

    return false;
}
