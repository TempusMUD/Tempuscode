//
// File: search.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

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
#include "screen.h"
#include "clan.h"
#include "tmpstr.h"
#include "account.h"
#include "spells.h"
#include "vehicle.h"
#include "fight.h"
#include "obj_data.h"
#include "actions.h"
#include "search.h"
#include "prog.h"
#include "strutil.h"

/* extern variables */
extern struct room_data *world;
extern struct obj_data *object_list;

int search_nomessage = 0;

int general_search(struct creature *ch, struct special_search_data *srch,
                   int mode);
bool room_tele_ok(struct creature *ch, struct room_data *room);
bool process_load_param(struct creature *ch);

int
search_trans_character(struct creature *ch,
                       struct special_search_data *srch, struct room_data *targ_room)
{
    struct room_data *was_in;

    if (!is_authorized(ch, ENTER_ROOM, targ_room)) {
        return 0;
    }

    was_in = ch->in_room;
    char_from_room(ch, true);
    char_to_room(ch, targ_room, true);
    ch->in_room->zone->enter_count++;
    if (!SRCH_FLAGGED(srch, SRCH_NO_LOOK)) {
        look_at_room(ch, ch->in_room, 0);
    }

    if (ch->followers) {
        struct follow_type *k, *next;

        for (k = ch->followers; k; k = next) {
            next = k->next;
            if (was_in == k->follower->in_room &&
                GET_LEVEL(k->follower) >= LVL_AMBASSADOR &&
                !PLR_FLAGGED(k->follower, PLR_WRITING)
                && can_see_creature(k->follower, ch)) {
                perform_goto(k->follower, ch->in_room, true);
            }
        }
    }

    if (GET_LEVEL(ch) < LVL_ETERNAL && !SRCH_FLAGGED(srch, SRCH_REPEATABLE)) {
        SET_BIT(srch->flags, SRCH_TRIPPED);
    }

    if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_DEATH)) {
        if (GET_LEVEL(ch) < LVL_AMBASSADOR) {
            log_death_trap(ch);
            death_cry(ch);
            creature_die(ch);
            return 2;
        } else {
            mudlog(LVL_GOD, NRM, true,
                   "(GC) %s trans-searched into deathtrap %d.",
                   GET_NAME(ch), ch->in_room->number);
        }
    }
    // can't double trans, to prevent loops
    for (srch = ch->in_room->search; srch; srch = srch->next) {
        if (SRCH_FLAGGED(srch, SRCH_TRIG_ENTER) && SRCH_OK(ch, srch) &&
            srch->command != SEARCH_COM_TRANSPORT) {
            if (general_search(ch, srch, 0) == 2) {
                return 2;
            }
        }
    }

    return 1;
}

#define SRCH_DOOR (targ_room->dir_option[srch->arg[1]]->exit_info)
#define SRCH_REV_DOOR (other_rm->dir_option[rev_dir[srch->arg[1]]]->exit_info)

//
//
//

int
general_search(struct creature *ch, struct special_search_data *srch, int mode)
{
    struct obj_data *obj = NULL;
    static struct creature *mob = NULL;
    struct room_data *rm = ch->in_room, *other_rm = NULL;
    struct room_data *targ_room = NULL;
    int add = 0, killed = 0, found = 0;
    int bits = 0, i, j;
    int maxlevel = 0;

    if (!mode) {
        obj = NULL;
        mob = NULL;
    }

    if (SRCH_FLAGGED(srch, SRCH_NEWBIE_ONLY) &&
        GET_LEVEL(ch) > 6 && !NOHASS(ch)) {
        send_to_char(ch,
                     "This can only be done here by players less than level 7.\r\n");
        return 1;
    }
    if (SRCH_FLAGGED(srch, SRCH_REMORT_ONLY) && !IS_REMORT(ch) && !NOHASS(ch)) {
        send_to_char(ch, "This can only be done here by remorts.\r\n");
        return 1;
    }

    if (srch->fail_chance && number(0, 100) < srch->fail_chance) {
        if (SRCH_FLAGGED(srch, SRCH_FAIL_TRIP)) {
            SET_BIT(srch->flags, SRCH_TRIPPED);
        }
        return 1;
    }

    switch (srch->command) {
    case (SEARCH_COM_OBJECT):
        if (!(obj = real_object_proto(srch->arg[0]))) {
            zerrlog(rm->zone, "search in room %d, object %d nonexistent.",
                    rm->number, srch->arg[0]);
            return 0;
        }
        if ((targ_room = real_room(srch->arg[1])) == NULL) {
            zerrlog(rm->zone, "search in room %d, targ room %d nonexistent.",
                    rm->number, srch->arg[1]);
            return 0;
        }
        if (obj->shared->number - obj->shared->house_count >= srch->arg[2]) {
            return 0;
        }
        if (!(obj = read_object(srch->arg[0]))) {
            zerrlog(rm->zone, "search cannot load object #%d, room %d.",
                    srch->arg[0], ch->in_room->number);
            return 0;
        }
        obj->creation_method = CREATED_SEARCH;
        obj->creator = ch->in_room->number;
        if (ZONE_FLAGGED(ch->in_room->zone, ZONE_ZCMDS_APPROVED)) {
            SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_UNAPPROVED);
        }
        obj_to_room(obj, targ_room);
        if (srch->to_remote && targ_room->people) {
            act(srch->to_remote, false, targ_room->people->data, obj, mob,
                TO_ROOM);
        }
        break;

    case SEARCH_COM_MOBILE:
        if (!(mob = real_mobile_proto(srch->arg[0]))) {
            zerrlog(rm->zone, "search in room %d, mobile %d nonexistent.",
                    rm->number, srch->arg[0]);
            return 0;
        }
        if ((targ_room = real_room(srch->arg[1])) == NULL) {
            zerrlog(rm->zone, "search in room %d, targ room %d nonexistent.",
                    rm->number, srch->arg[1]);
            return 0;
        }
        if (mob->mob_specials.shared->number >= srch->arg[2]) {
            return 0;
        }
        if (!(mob = read_mobile(srch->arg[0]))) {
            zerrlog(rm->zone, "search cannot load mobile #%d, room %d.",
                    srch->arg[0], ch->in_room->number);
            return 0;
        }
        char_to_room(mob, targ_room, false);
        if (process_load_param(mob)) {
            // Mobile Died in load_param
        } else {
            if (srch->to_remote && targ_room->people) {
                act(srch->to_remote, false, targ_room->people->data, obj, mob,
                    TO_ROOM);
            }
            if (GET_NPC_PROGOBJ(mob)) {
                trigger_prog_load(mob);
            }
        }
        break;

    case SEARCH_COM_EQUIP:
        if (!(obj = real_object_proto(srch->arg[1]))) {
            zerrlog(rm->zone,
                    "search in room %d, equip object %d nonexistent.", rm->number,
                    srch->arg[1]);
            return 0;
        }
        if (srch->arg[2] < 0 || srch->arg[2] >= NUM_WEARS) {
            zerrlog(rm->zone, "search trying to equip obj %d to badpos.",
                    obj->shared->vnum);
            return 0;
        }
        if (!(obj = read_object(srch->arg[1]))) {
            zerrlog(rm->zone, "search cannot load equip object #%d, room %d.",
                    srch->arg[0], ch->in_room->number);
            return 0;
        }
        obj->creation_method = CREATED_SEARCH;
        obj->creator = ch->in_room->number;
        if (ZONE_FLAGGED(ch->in_room->zone, ZONE_ZCMDS_APPROVED)) {
            SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_UNAPPROVED);
        }
        if (IS_IMPLANT(obj)) {
            if (ch->implants[srch->arg[2]]) {
                obj_to_char(obj, ch);
            } else if (equip_char(ch, obj, srch->arg[2], EQUIP_IMPLANT)) {
                return 2;
            }
        } else {
            if (ch->equipment[srch->arg[2]]) {
                obj_to_char(obj, ch);
            } else if (equip_char(ch, obj, srch->arg[2], EQUIP_WORN)) {
                return 2;
            }
        }
        break;

    case SEARCH_COM_GIVE:
        if (!(obj = real_object_proto(srch->arg[1]))) {
            return 0;
        }

        if (obj->shared->number - obj->shared->house_count >= srch->arg[2]) {
            return 0;
        }

        if (!(obj = read_object(srch->arg[1]))) {
            return 0;
        }

        obj->creation_method = CREATED_SEARCH;
        obj->creator = ch->in_room->number;
        if (ZONE_FLAGGED(ch->in_room->zone, ZONE_ZCMDS_APPROVED)) {
            SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_UNAPPROVED);
        }
        obj_to_char(obj, ch);
        break;

    case SEARCH_COM_TRANSPORT:
        if ((targ_room = real_room(srch->arg[0])) == NULL) {
            zerrlog(rm->zone, "search in room %d, targ room %d nonexistent.",
                    rm->number, srch->arg[0]);
            return 0;
        }
        if (srch->arg[1] == -1 || srch->arg[1] == 0) {
            if (!is_authorized(ch, ENTER_ROOM, targ_room)) {
                return 0;
            }

            if (srch->to_room) {
                act(srch->to_room, false, ch, obj, mob, TO_ROOM);
            }
            if (srch->to_vict) {
                act(srch->to_vict, false, ch, obj, mob, TO_CHAR);
            } else if (!SRCH_FLAGGED(srch, SRCH_NOMESSAGE)) {
                send_to_char(ch, "Okay.\r\n");
            }

            char_from_room(ch, true);
            char_to_room(ch, targ_room, true);
            ch->in_room->zone->enter_count++;
            if (!SRCH_FLAGGED(srch, SRCH_NO_LOOK)) {
                look_at_room(ch, ch->in_room, 0);
            }

            if (srch->to_remote) {
                act(srch->to_remote, false, ch, obj, mob, TO_ROOM);
            }

            if (GET_LEVEL(ch) < LVL_ETERNAL
                && !SRCH_FLAGGED(srch, SRCH_REPEATABLE)) {
                SET_BIT(srch->flags, SRCH_TRIPPED);
            }

            if (ch->followers) {
                struct follow_type *k, *next;

                for (k = ch->followers; k; k = next) {
                    next = k->next;
                    if (rm == k->follower->in_room &&
                        GET_LEVEL(k->follower) >= LVL_AMBASSADOR &&
                        !PLR_FLAGGED(k->follower, PLR_WRITING)
                        && can_see_creature(k->follower, ch)) {
                        perform_goto(k->follower, targ_room, true);
                    }
                }
            }

            if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_DEATH)) {
                if (GET_LEVEL(ch) < LVL_AMBASSADOR) {
                    log_death_trap(ch);
                    death_cry(ch);
                    creature_die(ch);
                    return 2;
                } else {
                    mudlog(LVL_GOD, NRM, true,
                           "(GC) %s trans-searched into deathtrap %d.",
                           GET_NAME(ch), ch->in_room->number);
                }
            }
            // can't double trans, to prevent loops
            for (srch = ch->in_room->search; srch; srch = srch->next) {
                if (SRCH_FLAGGED(srch, SRCH_TRIG_ENTER) && SRCH_OK(ch, srch) &&
                    srch->command != SEARCH_COM_TRANSPORT) {
                    if (general_search(ch, srch, 0) == 2) {
                        return 2;
                    }
                }
            }

            return 1;
        }

        if (srch->arg[1] == 1) {
            if (srch->to_vict) {
                act(srch->to_vict, false, ch, obj, mob, TO_CHAR);
            } else if (!SRCH_FLAGGED(srch, SRCH_NOMESSAGE)) {
                send_to_char(ch, "Okay.\r\n");
            }

            if (srch->to_remote && targ_room->people) {
                act(srch->to_remote, false, targ_room->people->data, obj, mob,
                    TO_ROOM);
                act(srch->to_remote, false, targ_room->people->data, obj, mob,
                    TO_CHAR);
            }

            int rc = 1;
            struct room_data *src_room = ch->in_room;
            GList *trans_list = g_list_copy(src_room->people);
            rc = search_trans_character(ch, srch, targ_room);

            for (GList *it = trans_list; it; it = next_living(it)) {
                mob = it->data;
                if (mob->in_room != src_room) {
                    continue;
                }
                if (SRCH_FLAGGED(srch, SRCH_NOAFFMOB) && IS_NPC(mob)) {
                    continue;
                }
                if (srch->to_room) {
                    act(srch->to_room, false, ch, obj, mob, TO_VICT);
                }
                int r = search_trans_character(mob, srch, targ_room);
                if (rc != 2) {
                    rc = r;
                }
            }
            g_list_free(trans_list);
            return rc;
        }
        break;
    case SEARCH_COM_DOOR:
        /************  Targ Room nonexistent ************/
        if ((targ_room = real_room(srch->arg[0])) == NULL) {
            zerrlog(rm->zone, "search in room %d, targ room %d nonexistent.",
                    rm->number, srch->arg[0]);
            return 0;
        }
        if (srch->arg[1] >= NUM_DIRS || !targ_room->dir_option[srch->arg[1]]) {
            zerrlog(rm->zone, "search in room %d, direction nonexistent.",
                    rm->number);
            return 0;
        }
        switch (srch->arg[2]) {
        case 0:
            add = 0;
            bits = EX_CLOSED | EX_LOCKED | EX_HIDDEN;
            break;
        case 1:
            add = 1;
            bits = EX_CLOSED;
            break;
        case 2: /** close and lock door **/
            add = 1;
            bits = EX_CLOSED | EX_LOCKED;
            break;
        case 3: /** remove hidden flag **/
            add = 0;
            bits = EX_HIDDEN;
            break;
        case 4:                // close, lock, and hide the door
            add = 1;
            bits = EX_CLOSED | EX_LOCKED | EX_HIDDEN;
            break;

        default:
            zerrlog(rm->zone, "search bunk doorcmd %d in rm %d.",
                    srch->arg[2], rm->number);
            return 0;
        }
        if (!(other_rm = targ_room->dir_option[srch->arg[1]]->to_room) ||
            !other_rm->dir_option[rev_dir[srch->arg[1]]] ||
            other_rm->dir_option[rev_dir[srch->arg[1]]]->to_room != targ_room) {
            other_rm = NULL;
        }

        if (add) {
            for (found = 0, i = 0; i < 32; i++) {
                if (IS_SET(bits, (j = (1 << i))) &&
                    (!IS_SET(SRCH_DOOR, j) ||
                     (other_rm && !IS_SET(SRCH_REV_DOOR, j)))) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                if (!SRCH_FLAGGED(srch, SRCH_TRIG_ENTER | SRCH_TRIG_FALL)) {
                    send_to_char(ch, "You can't do that right now.\r\n");
                }
                return 1;
            }
            SET_BIT(targ_room->dir_option[srch->arg[1]]->exit_info, bits);
            if (other_rm) {
                SET_BIT(other_rm->dir_option[rev_dir[srch->arg[1]]]->exit_info,
                        bits);
            }
        } else {
            for (found = 0, i = 0; i < 32; i++) {
                if (IS_SET(bits, (j = (1 << i))) &&
                    (IS_SET(SRCH_DOOR, j) ||
                     (other_rm && IS_SET(SRCH_REV_DOOR, j)))) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                if (!SRCH_FLAGGED(srch, SRCH_TRIG_ENTER | SRCH_TRIG_FALL)) {
                    send_to_char(ch, "You can't do that right now.\r\n");
                }
                return 1;
            }

            REMOVE_BIT(targ_room->dir_option[srch->arg[1]]->exit_info, bits);
            if (other_rm) {
                REMOVE_BIT(other_rm->dir_option[rev_dir[srch->
                                                        arg[1]]]->exit_info, bits);
            }
        }

        if (srch->to_remote) {
            if (targ_room != ch->in_room && targ_room->people) {
                act(srch->to_remote, false, targ_room->people->data, obj, ch,
                    TO_NOTVICT);
            }
            if (other_rm && other_rm != ch->in_room && other_rm->people) {
                act(srch->to_remote, false, other_rm->people->data, obj, ch,
                    TO_NOTVICT);
            }
        }
        break;

    case SEARCH_COM_SPELL:

        targ_room = real_room(srch->arg[1]);

        if (srch->arg[0] < 0 || srch->arg[0] > LVL_GRIMP) {
            zerrlog(rm->zone, "search in room %d, spell level %d invalid.",
                    rm->number, srch->arg[0]);
            return 0;
        }

        if (srch->arg[2] <= 0 || srch->arg[2] > TOP_NPC_SPELL) {
            zerrlog(rm->zone, "search in room %d, spell number %d invalid.",
                    rm->number, srch->arg[2]);
            return 0;
        }

        if (srch->to_room) {
            act(srch->to_room, false, ch, obj, mob, TO_ROOM);
        }
        if (srch->to_vict) {
            act(srch->to_vict, false, ch, obj, mob, TO_CHAR);
        } else if (!SRCH_FLAGGED(srch, SRCH_NOMESSAGE)) {
            send_to_char(ch, "Okay.\r\n");
        }

        if (GET_LEVEL(ch) < LVL_ETERNAL
            && !SRCH_FLAGGED(srch, SRCH_REPEATABLE)) {
            SET_BIT(srch->flags, SRCH_TRIPPED);
        }

        // turn messaging off for damage(  ) call
        if (SRCH_FLAGGED(srch, SRCH_NOMESSAGE)) {
            search_nomessage = 1;
        }

        if (targ_room) {

            if (srch->to_remote && ch->in_room != targ_room
                && targ_room->people) {
                act(srch->to_remote, false, targ_room->people->data, obj, mob,
                    TO_ROOM);
                act(srch->to_remote, false, targ_room->people->data, obj, mob,
                    TO_CHAR);
            }
            for (GList *it = first_living(targ_room->people); it; it = next_living(it)) {
                mob = it->data;

                if (SRCH_FLAGGED(srch, SRCH_NOAFFMOB) && IS_NPC(mob)) {
                    continue;
                }
                if (affected_by_spell(ch, srch->arg[2])) {
                    continue;
                }

                if (mob == ch) {
                    call_magic(ch, ch, NULL, NULL, srch->arg[2], srch->arg[0],
                               (SPELL_IS_MAGIC(srch->arg[2]) ||
                                SPELL_IS_DIVINE(srch->arg[2])) ? CAST_SPELL :
                               (SPELL_IS_PHYSICS(srch->arg[2])) ? CAST_PHYSIC :
                               (SPELL_IS_PSIONIC(srch->arg[2])) ? CAST_PSIONIC :
                               CAST_ROD);
                    if (is_dead(ch)) {
                        break;
                    }
                } else {
                    call_magic(ch, mob, NULL, NULL, srch->arg[2], srch->arg[0],
                               (SPELL_IS_MAGIC(srch->arg[2]) ||
                                SPELL_IS_DIVINE(srch->arg[2])) ? CAST_SPELL :
                               (SPELL_IS_PHYSICS(srch->arg[2])) ? CAST_PHYSIC :
                               (SPELL_IS_PSIONIC(srch->arg[2])) ? CAST_PSIONIC :
                               CAST_ROD);
                }
            }
        } else if (!targ_room && !affected_by_spell(ch, srch->arg[2])) {
            call_magic(ch, ch, NULL, NULL, srch->arg[2], srch->arg[0],
                       (SPELL_IS_MAGIC(srch->arg[2]) ||
                        SPELL_IS_DIVINE(srch->arg[2])) ? CAST_SPELL :
                       (SPELL_IS_PHYSICS(srch->arg[2])) ? CAST_PHYSIC :
                       (SPELL_IS_PSIONIC(srch->arg[2])) ? CAST_PSIONIC : CAST_ROD);
        }

        search_nomessage = 0;

        return 2;

        break;

    case SEARCH_COM_DAMAGE: /** damage a room or person */

        killed = 0;
        targ_room = real_room(srch->arg[1]);

        if (srch->arg[0] < 0 || srch->arg[0] > 500) {
            zerrlog(rm->zone, "search in room %d, damage level %d invalid.",
                    rm->number, srch->arg[0]);
            return 0;
        }

        if (srch->arg[2] <= 0 || srch->arg[2] >= TOP_NPC_SPELL) {
            zerrlog(rm->zone, "search in room %d, damage number %d invalid.",
                    rm->number, srch->arg[2]);
            return 0;
        }

        if (srch->to_room) {
            act(srch->to_room, false, ch, obj, mob, TO_ROOM);
        }
        if (srch->to_vict) {
            act(srch->to_vict, false, ch, obj, mob, TO_CHAR);
        } else if (!SRCH_FLAGGED(srch, SRCH_NOMESSAGE)) {
            send_to_char(ch, "Okay.\r\n");
        }

        if (GET_LEVEL(ch) < LVL_ETERNAL
            && !SRCH_FLAGGED(srch, SRCH_REPEATABLE)) {
            SET_BIT(srch->flags, SRCH_TRIPPED);
        }

        // turn messaging off for damage(  ) call
        if (SRCH_FLAGGED(srch, SRCH_NOMESSAGE)) {
            search_nomessage = 1;
        }

        if (targ_room) {

            if (srch->to_remote && ch->in_room != targ_room
                && targ_room->people) {
                act(srch->to_remote, false, targ_room->people->data, obj, mob,
                    TO_ROOM);
                act(srch->to_remote, false, targ_room->people->data, obj, mob,
                    TO_CHAR);
            }
            for (GList *it = first_living(targ_room->people); it; it = next_living(it)) {
                mob = it->data;

                if (SRCH_FLAGGED(srch, SRCH_NOAFFMOB) && IS_NPC(mob)) {
                    continue;
                }

                if (mob == ch) {
                    killed = damage(NULL, mob, NULL,
                                    dice(srch->arg[0], srch->arg[0]), srch->arg[2],
                                    WEAR_RANDOM);
                } else {
                    damage(NULL, mob, NULL,
                           dice(srch->arg[0], srch->arg[0]), srch->arg[2],
                           WEAR_RANDOM);
                }

            }
        } else {
            killed = damage(NULL, ch, NULL,
                            dice(srch->arg[0], srch->arg[0]), srch->arg[2], WEAR_RANDOM);
        }

        // turn messaging back on
        search_nomessage = 0;

        if (killed) {
            return 2;
        }

        if (!IS_SET(srch->flags, SRCH_IGNORE)) {
            return 1;
        } else {
            return 0;
        }

        break;

    case SEARCH_COM_SPAWN: {    /* spawn a room full of mobs to another room */
        if (!(other_rm = real_room(srch->arg[0]))) {
            return 0;
        }
        if (!(targ_room = real_room(srch->arg[1]))) {
            targ_room = ch->in_room;
        }

        if (srch->to_room) {
            act(srch->to_room, false, ch, obj, mob, TO_ROOM);
        }
        if (srch->to_remote && targ_room->people) {
            act(srch->to_remote, false, targ_room->people->data, obj, mob,
                TO_ROOM);
            act(srch->to_remote, false, targ_room->people->data, obj, mob,
                TO_CHAR);
        }
        if (srch->to_vict) {
            act(srch->to_vict, false, ch, obj, mob, TO_CHAR);
        } else if (!SRCH_FLAGGED(srch, SRCH_TRIG_ENTER | SRCH_TRIG_FALL)) {
            send_to_char(ch, "Okay.\r\n");
        }

        if (GET_LEVEL(ch) < LVL_ETERNAL
            && !SRCH_FLAGGED(srch, SRCH_REPEATABLE)) {
            SET_BIT(srch->flags, SRCH_TRIPPED);
        }

        for (GList *it = first_living(other_rm->people), *next = NULL; it; it = next) {
            next = next_living(it);
            mob = it->data;
            if (!IS_NPC(mob)) {
                continue;
            }
            if (other_rm != targ_room) {
                act("$n suddenly disappears.", true, mob, NULL, NULL, TO_ROOM);
                char_from_room(mob, false);
                char_to_room(mob, targ_room, false);
                act("$n suddenly appears.", true, mob, NULL, NULL, TO_ROOM);
            }
            if (srch->arg[2]) {
                start_hunting(mob, ch);
            }
        }

        if (!IS_SET(srch->flags, SRCH_IGNORE)) {
            return 1;
        } else {
            return 0;
        }
        break;

    }
    case SEARCH_COM_LOADROOM:
        maxlevel = srch->arg[1];
        targ_room = real_room(srch->arg[0]);

        if ((GET_LEVEL(ch) < maxlevel) && (targ_room)) {
            if (room_tele_ok(ch, targ_room)) {
                GET_LOADROOM(ch) = srch->arg[0];
            }
        } else {
            return 0;
        }

        break;

    case SEARCH_COM_NONE:      /* simple echo search */
        if ((targ_room = real_room(srch->arg[1])) &&
            srch->to_remote && ch->in_room != targ_room && targ_room->people) {
            act(srch->to_remote, false, targ_room->people->data, obj, mob,
                TO_ROOM);
            act(srch->to_remote, false, targ_room->people->data, obj, mob,
                TO_CHAR);
        }

        break;

    default:
        slog("Unknown rsearch command in do_search(  )..room %d.", rm->number);

    }

    if (srch->to_room) {
        act(srch->to_room, false, ch, obj, mob, TO_ROOM);
    }
    if (srch->to_vict) {
        act(srch->to_vict, false, ch, obj, mob, TO_CHAR);
    } else if (!SRCH_FLAGGED(srch, SRCH_NOMESSAGE)) {
        send_to_char(ch, "Okay.\r\n");
    }

    if (GET_LEVEL(ch) < LVL_ETERNAL && !SRCH_FLAGGED(srch, SRCH_REPEATABLE)) {
        SET_BIT(srch->flags, SRCH_TRIPPED);
    }

    if (!IS_SET(srch->flags, SRCH_IGNORE)) {
        return 1;
    } else {
        return 0;
    }
}

int
triggers_search(struct creature *ch, int cmd, char *arg,
                struct special_search_data *srch)
{

    char *cur_arg, *next_arg;

    skip_spaces(&arg);

    if (SRCH_FLAGGED(srch, SRCH_CLANPASSWD)) {
        int clan_id = clan_owning_room(ch->in_room);
        struct clan_data *clan = real_clan(clan_id);
        if (!clan
            || !*clan->password
            || !isname_exact(arg, clan->password)) {
            return 0;
        }
    } else if (srch->keywords) {
        if (SRCH_FLAGGED(srch, SRCH_NOABBREV)) {
            if (!isname_exact(arg, srch->keywords)) {
                return 0;
            }
        } else {
            if (!isname(arg, srch->keywords)) {
                return 0;
            }
        }
    }

    if (IS_SET(srch->flags, SRCH_TRIPPED)) {
        return 0;
    }

    if (GET_LEVEL(ch) < LVL_IMMORT && !SRCH_OK(ch, srch)) {
        return 0;
    }

    if (!srch->command_keys) {
        return 1;
    }
    next_arg = srch->command_keys;
    cur_arg = tmp_getword(&next_arg);

    while (*cur_arg) {
        if (IS_SET(srch->flags, SRCH_MATCH_ALL)) {
            if (!CMD_IS(cur_arg)) {
                return 0;
            }
        } else {
            if (CMD_IS(cur_arg)) {
                return 1;
            }
        }
        cur_arg = tmp_getword(&next_arg);
    }

    return IS_SET(srch->flags, SRCH_MATCH_ALL) ? 1 : 0;
}

#undef __search_c__
