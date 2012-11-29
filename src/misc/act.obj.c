/* ************************************************************************
*   File: act.obj1.c                                    Part of CircleMUD *
*  Usage: object handling routines -- get/drop and container handling     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: act.obj.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#ifdef HAS_CONFIG_H
#endif

#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
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
#include "char_class.h"
#include "players.h"
#include "tmpstr.h"
#include "account.h"
#include "spells.h"
#include "vehicle.h"
#include "materials.h"
#include "bomb.h"
#include "fight.h"
#include "obj_data.h"
#include "strutil.h"
#include "guns.h"
#include "prog.h"
#include "smokes.h"

/* extern variables */
extern struct room_data *world;
extern struct obj_data *object_list;
int total_coins = 0;
int total_credits = 0;
extern struct obj_data *dam_object;

int char_hands_free(struct creature *ch);
int empty_to_obj(struct obj_data *obj, struct obj_data *container,
    struct creature *ch);
bool junkable(struct obj_data *obj);
ACMD(do_stand);
ACMD(do_throw);
ACMD(do_activate);
ACMD(do_give);
ACMD(do_extinguish);
ACMD(do_split);

const long MONEY_LOG_LIMIT = 50000000;

struct obj_data *
get_random_uncovered_implant(struct creature *ch, int type)
{
    int possibles = 0;
    int implant = 0;
    int pos_imp = 0;
    int i;
    struct obj_data *o = NULL;

    if (!ch)
        return NULL;
    for (i = 0; i < NUM_WEARS; i++) {
        if (IS_WEAR_EXTREMITY(i)) {
            if ((o = GET_IMPLANT(ch, i))) {
                if (type != -1 && !IS_OBJ_TYPE(o, type))
                    continue;
                else
                    possibles++;
            }
        }
    }
    if (possibles) {
        implant = number(1, possibles);
        pos_imp = 0;
        for (i = 0; pos_imp < implant && i < NUM_WEARS; i++) {
            if (IS_WEAR_EXTREMITY(i) && !GET_EQ(ch, i)) {
                if ((o = GET_IMPLANT(ch, i))) {
                    if (type != -1 && !IS_OBJ_TYPE(o, type))
                        continue;
                    else
                        pos_imp++;
                }
            }
        }
        if (pos_imp == implant) {
            o = GET_IMPLANT(ch, i - 1);
            return o;
        }
    }
    return NULL;
}

//
// returns value is same as damage()
//

int
explode_sigil(struct creature *ch, struct obj_data *obj)
{

    int ret = 0;
    int dam = 0;
    bool loaded = false;

    if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL) ||
        ch->in_room->zone->pk_style == ZONE_NO_PK) {
        act("$p feels rather warm to the touch, and shudders violently.",
            false, ch, obj, NULL, TO_CHAR);
        return 0;
    }
    dam = number(GET_OBJ_SIGIL_LEVEL(obj), GET_OBJ_SIGIL_LEVEL(obj) << 2);
    if (mag_savingthrow(ch, GET_OBJ_SIGIL_LEVEL(obj), SAVING_SPELL))
        dam >>= 1;

    act("$p explodes when you pick it up!!", false, ch, obj, NULL, TO_CHAR);
    act("$p explodes when $n picks it up!!", false, ch, obj, NULL, TO_ROOM);

    dam_object = obj;

    int obj_id = GET_OBJ_SIGIL_IDNUM(obj);
    if (!obj_id)
        return 0;

    struct creature *killer = get_char_in_world_by_idnum(obj_id);

    // load the bastich from file.
    if (!killer) {
        killer = load_player_from_xml(obj_id);
        if (killer) {
            killer->account =
                account_by_idnum(player_account_by_idnum(obj_id));
            loaded = true;
        }
    }
    // the piece o shit has a bogus killer idnum on it!
    if (!killer) {
        errlog("bogus idnum %d on object %s damaging %s.",
            obj_id, obj->name, GET_NAME(ch));
        return 0;
    }

    ret = damage(killer, ch, NULL, dam, SPELL_WARDING_SIGIL, WEAR_HANDS);

    // save the sonuvabitch to file
    crashsave(killer);

    if (loaded)
        free_creature(killer);

    GET_OBJ_SIGIL_IDNUM(obj) = GET_OBJ_SIGIL_LEVEL(obj) = 0;

    dam_object = NULL;

    return ret;
}

//
// return value is same as damage()
//
int
explode_all_sigils(struct creature *ch)
{
    struct obj_data *obj, *next_obj;

    for (obj = ch->carrying; obj; obj = next_obj) {
        next_obj = obj->next_content;

        if (GET_OBJ_SIGIL_IDNUM(obj)
            && GET_OBJ_SIGIL_IDNUM(obj) != GET_IDNUM(ch)) {
            return explode_sigil(ch, obj);
        }
    }
    return 0;
}

//
// this method removes all MONEY objects from a character's inventory
// increments his gold/credit counter, and emits messages
//

void
consolidate_char_money(struct creature *ch)
{
    struct obj_data *obj = NULL, *next_obj = NULL;
    money_t num_gold = 0;
    money_t num_credits = 0;

    for (obj = ch->carrying; obj; obj = next_obj) {
        next_obj = obj->next_content;

        if (IS_OBJ_TYPE(obj, ITEM_MONEY)) {
            if (OBJ_APPROVED(obj) || is_authorized(ch, APPROVE_ZONE, NULL)) {
                if (GET_OBJ_VAL(obj, 1) == 1)
                    num_credits += GET_OBJ_VAL(obj, 0);
                else
                    num_gold += GET_OBJ_VAL(obj, 0);
            }
            extract_obj(obj);
        }
    }

    if (num_gold) {
        if (num_gold > MONEY_LOG_LIMIT)
            slog("MONEY: %s picked up %'" PRId64 " gold in room #%d (%s)",
                GET_NAME(ch), num_gold, ch->in_room->number,
                ch->in_room->name);
        GET_GOLD(ch) += num_gold;
        if (num_gold == 1)
            send_to_char(ch, "There was only a single gold coin.\r\n");
        else
            send_to_char(ch, "There were %'" PRId64 " coins.\r\n", num_gold);

        if (AFF_FLAGGED(ch, AFF_GROUP) && PRF2_FLAGGED(ch, PRF2_AUTOSPLIT))
            do_split(ch, tmp_sprintf("%" PRId64, num_gold), 0, 0);
    }

    if (num_credits) {
        if (num_credits > MONEY_LOG_LIMIT)
            slog("MONEY: %s picked up %'" PRId64 " credits in room #%d (%s)",
                GET_NAME(ch), num_gold, ch->in_room->number,
                ch->in_room->name);
        GET_CASH(ch) += num_credits;
        if (num_credits == 1)
            send_to_char(ch, "There was only a single credit.\r\n");
        else
            send_to_char(ch, "There were %'" PRId64 " credits.\r\n", num_credits);

        if (AFF_FLAGGED(ch, AFF_GROUP) && PRF2_FLAGGED(ch, PRF2_AUTOSPLIT))
            do_split(ch, tmp_sprintf("%" PRId64 " credits", num_credits), 0, 0);
    }
}

//
// returns true if a quad is found and activated
//

bool
activate_char_quad(struct creature *ch)
{

    struct obj_data *obj = NULL, *next_obj = NULL;

    for (obj = ch->carrying; obj; obj = next_obj) {
        next_obj = obj->next_content;

        if (GET_OBJ_VNUM(obj) == QUAD_VNUM) {
            call_magic(ch, ch, NULL, NULL, SPELL_QUAD_DAMAGE, LVL_GRIMP,
                CAST_SPELL);
            extract_obj(obj);
            slog("%s got the Quad Damage at %d.", GET_NAME(ch),
                ch->in_room->number);

            struct room_data *room = NULL;
            struct zone_data *zone = NULL;

            //
            // notify the world of this momentous event
            //

            if (!ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)) {
                if (ch->in_room->zone->plane > MAX_PRIME_PLANE ||
                    !ZONE_FLAGGED(ch->in_room->zone, ZONE_SOUNDPROOF |
                        ZONE_ISOLATED)) {
                    for (zone = zone_table; zone; zone = zone->next) {
                        if (zone != ch->in_room->zone &&
                            zone->time_frame == ch->in_room->zone->time_frame
                            && ((zone->plane <= MAX_PRIME_PLANE
                                    && ch->in_room->zone->plane <=
                                    MAX_PRIME_PLANE)
                                || ch->in_room->zone->plane == zone->plane)
                            && !ZONE_FLAGGED(zone,
                                ZONE_ISOLATED | ZONE_SOUNDPROOF)) {
                            for (room = zone->world; room; room = room->next) {
                                if (room->people &&
                                    !ROOM_FLAGGED(room, ROOM_SOUNDPROOF))
                                    send_to_room
                                        ("You hear a screaming roar in the distance.\r\n",
                                        room);
                            }
                        }
                    }
                }
                for (room = ch->in_room->zone->world; room; room = room->next) {
                    if (room->people && room != ch->in_room &&
                        !ROOM_FLAGGED(room, ROOM_SOUNDPROOF))
                        send_to_room("You hear a screaming roar nearby!\r\n",
                            room);
                }
            }
            return true;
        }
    }
    return false;
}

bool
perform_put(struct creature * ch, struct obj_data * obj,
    struct obj_data * cont, int display)
{
    int capacity = 0;
    struct room_data *to_room = NULL;

    if (IS_OBJ_TYPE(cont, ITEM_PIPE)) {
        if (GET_OBJ_TYPE(obj) != ITEM_TOBACCO) {
            errlog("obj %d '%s' attempted to pack.",
                GET_OBJ_VNUM(obj), obj->name);
            send_to_char(ch, "Sorry, there is an error here.\r\n");
        } else if (CUR_DRAGS(cont) && SMOKE_TYPE(cont) != SMOKE_TYPE(obj))
            act("You need to clear out $p before packing it with $P.",
                false, ch, cont, obj, TO_CHAR);
        else {
            capacity = GET_OBJ_VAL(cont, 1) - GET_OBJ_VAL(cont, 0);
            capacity = MIN(MAX_DRAGS(obj), capacity);
            if (MAX_DRAGS(obj) <= 0) {
                send_to_char(ch, "Tobacco error: please report.\r\n");
                return false;
            }
            if (capacity <= 0) {
                act("Sorry, $p is fully packed.", false, ch, cont, NULL, TO_CHAR);
                return false;
            } else {

                GET_OBJ_VAL(cont, 0) += capacity;
                GET_OBJ_VAL(cont, 2) = GET_OBJ_VAL(obj, 0); /* Type of tobacco */
                MAX_DRAGS(obj) -= capacity;

                if (display) {
                    act("You pack $p in $P.", false, ch, obj, cont, TO_CHAR);
                    act("$n packs $p in $P.", true, ch, obj, cont, TO_ROOM);
                }

                obj_from_char(obj);
                obj_to_obj(obj, cont);

                return true;
            }
        }
    } else if (IS_OBJ_TYPE(cont, ITEM_VEHICLE)) {
        if (CAR_CLOSED(cont))
            act("$p is closed.", false, ch, cont, NULL, TO_CHAR);
        else if ((to_room = real_room(ROOM_NUMBER(cont))) == NULL)
            act("Sorry, $p doesn't seem to have an interior right now.",
                false, ch, cont, NULL, TO_CHAR);
        else {
            if (display) {
                act("You toss $P into $p.", false, ch, cont, obj, TO_CHAR);
                act("$n tosses $P into $p.", false, ch, cont, obj, TO_ROOM);
            }

            obj_from_char(obj);
            obj_to_room(obj, to_room);

            if (display)
                act("$p is tossed into the car by $N.", false, NULL, obj, ch,
                    TO_ROOM);
            return true;
        }

        return false;
    } else {
        if (weigh_contained_objs(cont) + GET_OBJ_WEIGHT(obj) >
            GET_OBJ_VAL(cont, 0))
            act("$p won't fit in $P.", false, ch, obj, cont, TO_CHAR);
        else if (IS_OBJ_STAT(obj, ITEM_NODROP)
            && GET_LEVEL(ch) < LVL_TIMEGOD && !NPC_FLAGGED(ch, NPC_UTILITY))
            act("$p must be cursed!  You can't seem to let go of it...", false,
                ch, obj, NULL, TO_CHAR);
        else {
            obj_from_char(obj);
            obj_to_obj(obj, cont);
            if (display == true) {
                act("You put $p in $P.", false, ch, obj, cont, TO_CHAR);
                act("$n puts $p in $P.", true, ch, obj, cont, TO_ROOM);
            }
            return true;
        }
    }
    return false;
}

/* The following put modes are supported by the code below:

	1) put <object> <container>
	2) put all.<object> <container>
	3) put all <container>

	<container> must be in inventory or on ground.
	all objects to be put into container must be in inventory.
*/

ACMD(do_put)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char cntbuf[80];
    struct obj_data *obj, *next_obj, *cont, *save_obj = NULL;
    int obj_dotmode, cont_dotmode, found = 0, counter = 0;
    bool bomb = false;

    two_arguments(argument, arg1, arg2);
    obj_dotmode = find_all_dots(arg1);
    cont_dotmode = find_all_dots(arg2);

    if (!*arg1)
        send_to_char(ch, "Put what in what?\r\n");
    else if (cont_dotmode != FIND_INDIV)
        send_to_char(ch,
            "You can only put things into one container at a time.\r\n");
    else if (!*arg2) {
        send_to_char(ch, "What do you want to put %s in?\r\n",
            ((obj_dotmode == FIND_INDIV) ? "it" : "them"));
    } else {
        generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_EQUIP_CONT | FIND_OBJ_ROOM,
            ch, NULL, &cont);
        if (!cont) {
            send_to_char(ch, "You don't see %s %s here.\r\n", AN(arg2), arg2);
        } else if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER &&
            GET_OBJ_TYPE(cont) != ITEM_VEHICLE &&
            GET_OBJ_TYPE(cont) != ITEM_PIPE)
            act("$p is not a container.", false, ch, cont, NULL, TO_CHAR);
        else if (GET_OBJ_TYPE(cont) != ITEM_PIPE &&
            IS_SET(GET_OBJ_VAL(cont, 1), CONT_CLOSED))
            send_to_char(ch, "You'd better open it first!\r\n");
        else {
            if (obj_dotmode == FIND_INDIV) {    /* put <obj> <container> */
                if (!(obj = get_obj_in_list_all(ch, arg1, ch->carrying))) {
                    send_to_char(ch, "You aren't carrying %s %s.\r\n",
                        AN(arg1), arg1);
                } else if (obj == cont)
                    send_to_char(ch,
                        "You attempt to fold it into itself, but fail.\r\n");
                else if (IS_OBJ_TYPE(cont, ITEM_PIPE)
                    && GET_OBJ_TYPE(obj) != ITEM_TOBACCO)
                    act("You can't pack $p with $P!", false, ch, cont, obj,
                        TO_CHAR);
                else if (IS_BOMB(obj) && obj->contains
                    && IS_FUSE(obj->contains) && FUSE_STATE(obj->contains))
                    send_to_char(ch,
                        "It would really be best if you didn't do that.\r\n");
                else
                    perform_put(ch, obj, cont, true);
            } else {
                for (obj = ch->carrying; obj; obj = next_obj) {
                    next_obj = obj->next_content;
                    if (obj == cont)
                        continue;
                    if (!can_see_object(ch, obj))
                        continue;
                    if (!(obj_dotmode == FIND_ALL
                            || isname(arg1, obj->aliases)))
                        continue;
                    if (IS_BOMB(obj) &&
                        obj->contains &&
                        IS_FUSE(obj->contains) && FUSE_STATE(obj->contains)) {
                        act("It would really be best if you didn't put $p in $P.", false, ch, obj, cont, TO_CHAR);
                        bomb = true;
                        continue;
                    }
                    if (IS_PIPE(cont) && !IS_TOBACCO(obj))
                        continue;

                    if (save_obj != NULL
                        && strcasecmp(save_obj->name,
                            obj->name) != 0 && counter > 0) {
                        if (counter == 1)
                            sprintf(cntbuf, "You put $p in $P.");
                        else
                            sprintf(cntbuf, "You put $p in $P. (x%d)",
                                counter);
                        act(cntbuf, false, ch, save_obj, cont, TO_CHAR);

                        if (counter == 1)
                            sprintf(cntbuf, "$n puts $p in $P.");
                        else
                            sprintf(cntbuf, "$n puts $p in $P. (x%d)",
                                counter);
                        act(cntbuf, true, ch, save_obj, cont, TO_ROOM);
                        counter = 0;
                    }
                    found = 1;
                    save_obj = obj;
                    if (perform_put(ch, obj, cont, false))
                        counter++;
                }
                if (found && counter > 0) {
                    if (counter == 1)
                        sprintf(cntbuf, "You put $p in $P.");
                    else
                        sprintf(cntbuf, "You put $p in $P. (x%d)", counter);
                    act(cntbuf, false, ch, save_obj, cont, TO_CHAR);
                    if (counter == 1)
                        sprintf(cntbuf, "$n puts $p in $P.");
                    else
                        sprintf(cntbuf, "$n puts $p in $P. (x%d)", counter);
                    act(cntbuf, true, ch, save_obj, cont, TO_ROOM);
                }
                if (!found && !bomb) {
                    if (obj_dotmode == FIND_ALL)
                        send_to_char(ch,
                            "You don't seem to have anything to put in it.\r\n");
                    else {
                        send_to_char(ch, "You don't seem to have any %ss.\r\n",
                            arg1);
                    }
                }
            }
            // If the object is a pipe, extract all objects within it -
            // pipes don't have objects
            if (IS_OBJ_TYPE(cont, ITEM_PIPE))
                while (cont->contains)
                    extract_obj(cont->contains);
        }
    }
}

bool
can_take_obj(struct creature *ch, struct obj_data *obj, bool check_weight,
    bool print)
{

    if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
        sprintf(buf, "$p: you can't carry that many items.");
    } else if (check_weight
        && (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch)) {
        sprintf(buf, "$p: you can't carry that much weight.");
    } else if (!(CAN_WEAR(obj, ITEM_WEAR_TAKE)) && GET_LEVEL(ch) < LVL_GOD) {
        sprintf(buf, "$p: you can't take that!");
    } else {
        return true;
    }

    if (print)
        act(buf, false, ch, obj, NULL, TO_CHAR);

    return false;
}

//
// get_check_money extracts the money obj from the game and increments
// the character's cash or gold counter.
//
// returns 1 if item is extracted, 0 otherwise

int
get_check_money(struct creature *ch, struct obj_data **obj_p, int display)
{
    struct obj_data *obj = *obj_p;

    if ((IS_OBJ_TYPE(obj, ITEM_MONEY)) && (GET_OBJ_VAL(obj, 0) > 0)) {
        obj_from_char(obj);
        if (GET_OBJ_VAL(obj, 0) > 1 && display == 1) {
            send_to_char(ch, "There were %d %s.\r\n", GET_OBJ_VAL(obj, 0),
                GET_OBJ_VAL(obj, 1) ? "credits" : "coins");
        }
        if (!IS_OBJ_STAT2(obj, ITEM2_UNAPPROVED)) {
            if (GET_OBJ_VAL(obj, 0) > MONEY_LOG_LIMIT)
                slog("MONEY: %s picked up %d %s in room #%d (%s)",
                    GET_NAME(ch), GET_OBJ_VAL(obj, 0),
                    GET_OBJ_VAL(obj, 1) ? "credits" : "coins",
                    ch->in_room->number, ch->in_room->name);

            if (GET_OBJ_VAL(obj, 1)) {
                GET_CASH(ch) += GET_OBJ_VAL(obj, 0);
                total_credits += GET_OBJ_VAL(obj, 0);
            } else {
                GET_GOLD(ch) += GET_OBJ_VAL(obj, 0);
                total_coins += GET_OBJ_VAL(obj, 0);
            }
        }
        extract_obj(obj);

        obj_p = NULL;

        if (GET_LEVEL(ch) >= LVL_AMBASSADOR && GET_LEVEL(ch) < LVL_GOD) {
            total_coins = 0;
            total_credits = 0;
        }
        return 1;
    }
    return 0;
}

//
// returns true if object is gotten from container
//

bool
perform_get_from_container(struct creature * ch,
    struct obj_data * obj,
    struct obj_data * cont, bool already_has, bool display, int counter)
{

    bool retval = false;

    if (!can_take_obj(ch, obj, already_has, true)) {
        if (counter > 1) {
            display = true;
            --counter;
        } else {
            return false;
        }
    }

    else {
        obj_from_obj(obj);
        obj_to_char(obj, ch);
        retval = true;

        //
        // log corpse looting
        //

        if (GET_OBJ_VAL(cont, 3)
            && CORPSE_IDNUM(cont) > 0
            && player_idnum_exists(CORPSE_IDNUM(cont))
            && CORPSE_IDNUM(cont) != GET_IDNUM(ch)
            && (!ch->account ||
                player_account_by_idnum(CORPSE_IDNUM(cont)) !=
                ch->account->id)) {
            mudlog(LVL_DEMI, CMP, true, "%s looted %s from %s.", GET_NAME(ch),
                obj->name, cont->name);
        }
        // Also resave corpse file at this point
        if (IS_CORPSE(cont) && CORPSE_IDNUM(cont) > 0 &&
            player_idnum_exists(CORPSE_IDNUM(cont))) {
            char *fname;
            FILE *corpse_file;

            fname = get_corpse_file_path(CORPSE_IDNUM(cont));
            if ((corpse_file = fopen(fname, "w+")) != NULL) {
                fprintf(corpse_file, "<corpse>");
                save_object_to_xml(cont, corpse_file);
                fprintf(corpse_file, "</corpse>");
                fclose(corpse_file);
            } else {
                errlog("Failed to open corpse file [%s] (%s)", fname,
                    strerror(errno));
            }
        }
    }

    if (display == true && counter > 0) {
        if (counter == 1) {
            strcpy(buf, "You get $p from $P.");
            strcpy(buf2, "$n gets $p from $P.");
        } else {
            sprintf(buf, "You get $p from $P. (x%d)", counter);
            sprintf(buf2, "$n gets $p from $P. (x%d)", counter);
        }
        act(buf, false, ch, obj, cont, TO_CHAR);
        act(buf2, true, ch, obj, cont, TO_ROOM);
    }

    return retval;
}

void
perform_autoloot(struct creature *ch, struct obj_data *corpse)
{
    struct obj_data *obj, *next_obj = NULL;
    bool found = false, display;
    int counter = 1;

    // If they can't see the corse, they can't loot it
    if (!corpse || !can_see_object(ch, corpse)
        || ch->in_room != corpse->in_room)
        return;

    // Can't loot player corpses in NPK zones
    if (ch->in_room->zone->pk_style == ZONE_NEUTRAL_PK &&
        !IS_NPC(ch) && GET_LEVEL(ch) < LVL_AMBASSADOR &&
        IS_CORPSE(corpse) &&
        CORPSE_IDNUM(corpse) != GET_IDNUM(ch) && CORPSE_IDNUM(corpse) > 0)
        return;

    // Iterate through the corpse's contents, looking for money
    for (obj = corpse->contains; obj; obj = next_obj) {
        next_obj = obj->next_content;

        if (!can_see_object(ch, obj))
            continue;
        if (!IS_OBJ_TYPE(obj, ITEM_MONEY))
            continue;

        display = (!next_obj ||
            next_obj->name != obj->name || !can_see_object(ch, next_obj)
            || !can_take_obj(ch, next_obj, true, false));

        if (perform_get_from_container(ch, obj, corpse, false,
                display, counter)) {
            found = true;
            counter = (display) ? 1 : (1 + counter);
        } else {
            counter = 1;
        }
    }
    if (found)
        consolidate_char_money(ch);
}

//
// return value same as damage()
//

int
get_from_container(struct creature *ch, struct obj_data *cont, char *arg)
{

    struct obj_data *obj, *next_obj;

    int dotmode;
    int retval = 0;

    bool quad_found = false;
    bool sigil_found = false;
    bool money_found = false;

    char *match_name = NULL;       // the char* keyword which matches on .<item>

    bool check_weight = false;

    if (IS_SET(GET_OBJ_VAL(cont, 1), CONT_CLOSED) && !GET_OBJ_VAL(cont, 3) &&
        GET_LEVEL(ch) < LVL_CREATOR) {
        act("$p is closed.", false, ch, cont, NULL, TO_CHAR);
        return 0;
    }

    dotmode = find_all_dots(arg);

    if (cont->in_room)
        check_weight = true;

    if (dotmode == FIND_INDIV) {

        if (!(obj = get_obj_in_list_all(ch, arg, cont->contains))) {
            sprintf(buf, "There doesn't seem to be %s %s in $p.", AN(arg),
                arg);
            act(buf, false, ch, cont, NULL, TO_CHAR);
            return 0;
        }
        if (IS_IMPLANT(obj) && IS_CORPSE(cont) &&
            !CAN_WEAR(obj, ITEM_WEAR_TAKE)) {
            if (GET_LEVEL(ch) < LVL_GOD) {
                sprintf(buf, "There doesn't seem to be %s %s in $p.", AN(arg),
                    arg);
                act(buf, false, ch, cont, NULL, TO_CHAR);
                return 0;
            } else {
                SET_BIT(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
            }
        }

        if (IS_CORPSE(cont) && CORPSE_IDNUM(cont) != GET_IDNUM(ch) &&
            ch->in_room->zone->pk_style == ZONE_NEUTRAL_PK &&
            !IS_NPC(ch) && GET_LEVEL(ch) < LVL_AMBASSADOR &&
            CORPSE_IDNUM(cont) > 0) {
            send_to_char(ch, "You may not loot corpses in NPK zones.\r\n");
            return 0;
        }
        if (!perform_get_from_container(ch, obj, cont, check_weight, true, 1))
            return 0;

        if (IS_CORPSE(cont) && CORPSE_IDNUM(cont) > 0) {
            WAIT_STATE(ch, (1 RL_SEC) / 4);
        }

        if (IS_OBJ_TYPE(obj, ITEM_MONEY))
            money_found = true;
        else if (GET_OBJ_VNUM(obj) == QUAD_VNUM)
            quad_found = true;
        else if (GET_OBJ_SIGIL_IDNUM(obj))
            sigil_found = true;
    }
    //
    // 'get all' or 'get all.item'
    //

    else {
        if (ch->in_room->zone->pk_style == ZONE_NEUTRAL_PK &&
            !IS_NPC(ch) && GET_LEVEL(ch) < LVL_AMBASSADOR &&
            IS_CORPSE(cont) &&
            CORPSE_IDNUM(cont) != GET_IDNUM(ch) && CORPSE_IDNUM(cont) > 0) {
            send_to_char(ch, "You may not loot corpses in NPK zones.\r\n");
            return 0;
        }
        if (IS_CORPSE(cont) && CORPSE_IDNUM(cont) > 0
            && CORPSE_IDNUM(cont) != GET_IDNUM(ch)
            && GET_LEVEL(ch) < LVL_AMBASSADOR) {
            sprintf(buf, "You may only take things one at a time from $P.");
            act(buf, false, ch, NULL, cont, TO_CHAR);
            return 0;
        }
        if (dotmode == FIND_ALLDOT) {
            match_name = arg;
        }

        bool found = false;
        bool display = false;
        int counter = 1;

        for (obj = cont->contains; obj; obj = next_obj) {
            next_obj = obj->next_content;

            if (!can_see_object(ch, obj)) {
                continue;
            }
            // Gods can get all.corpse and get implants.
            // Players cannot.
            if (IS_IMPLANT(obj) && IS_CORPSE(cont)
                && !CAN_WEAR(obj, ITEM_WEAR_TAKE)) {
                if (GET_LEVEL(ch) < LVL_GOD) {
                    continue;
                } else {
                    SET_BIT(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
                }
            }
            //
            // match_name is set if this is a find alldot get
            //

            if (match_name && !isname(match_name, obj->aliases))
                continue;

            if (!next_obj ||
                next_obj->name != obj->name || !can_see_object(ch, next_obj)
                || !can_take_obj(ch, next_obj, check_weight, false)) {
                display = true;
            }

            if (!perform_get_from_container(ch, obj, cont, check_weight,
                    display, counter)) {
                counter = 1;
            }

            else {
                found = true;

                if (IS_OBJ_TYPE(obj, ITEM_MONEY))
                    money_found = true;
                else if (GET_OBJ_VNUM(obj) == QUAD_VNUM)
                    quad_found = true;
                else if (GET_OBJ_SIGIL_IDNUM(obj)
                    && GET_OBJ_SIGIL_IDNUM(obj) != GET_IDNUM(ch))
                    sigil_found = true;

                if (!display) {
                    ++counter;
                } else {
                    counter = 1;
                }
            }

            display = false;
        }

        if (!found) {

            if (match_name) {
                sprintf(buf,
                    "You didn't find anything in $P to take that looks like a '%s'",
                    match_name);
                act(buf, false, ch, NULL, cont, TO_CHAR);

            } else {
                sprintf(buf, "You didn't find anything in $P.");
                act(buf, false, ch, NULL, cont, TO_CHAR);
            }
            return 0;
        }
    }

    if (money_found)
        consolidate_char_money(ch);

    if (quad_found)
        activate_char_quad(ch);

    if (sigil_found) {
        retval = explode_all_sigils(ch);
    }

    return retval;
}

//
// perform_get_from_room cannot extract the obj
//
// returns true on successful get
//

bool
perform_get_from_room(struct creature * ch,
    struct obj_data * obj, bool display, int counter)
{

    bool retval = false;

    if (!can_take_obj(ch, obj, true, true)) {
        if (counter > 1) {
            display = true;
            --counter;
        } else {
            return false;
        }
    }

    else {
        obj_from_room(obj);
        obj_to_char(obj, ch);
        retval = true;
    }

    if (display == true && counter > 0) {
        if (counter == 1) {
            strcpy(buf, "You get $p.");
            strcpy(buf2, "$n gets $p.");
        } else {
            sprintf(buf, "You get $p. (x%d)", counter);
            sprintf(buf2, "$n gets $p. (x%d)", counter);
        }
        act(buf, false, ch, obj, NULL, TO_CHAR);
        act(buf2, true, ch, obj, NULL, TO_ROOM);

    }

    return retval;
}

//
// return value is same as damage()
//

int
get_from_room(struct creature *ch, char *arg)
{

    struct obj_data *obj, *next_obj;

    int dotmode;
    int retval = 0;

    bool quad_found = false;
    bool sigil_found = false;
    bool money_found = false;

    char *match_name = NULL;       // the char* keyword which matches

    dotmode = find_all_dots(arg);

    //
    // 'get <item>'
    //

    if (dotmode == FIND_INDIV) {
        if (!(obj = get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
            send_to_char(ch, "You don't see %s %s here.\r\n", AN(arg), arg);
            return 0;
        }

        if (IS_CORPSE(obj)
            && CORPSE_IDNUM(obj) != GET_IDNUM(ch)
            && GET_LEVEL(ch) < LVL_AMBASSADOR
            && ch->in_room->zone->pk_style != ZONE_CHAOTIC_PK && IS_PC(ch)
            && CORPSE_IDNUM(obj) > 0) {
            send_to_char(ch, "You can only take PC corpses in CPK zones!\r\n");
            return 0;
        }

        if (!perform_get_from_room(ch, obj, true, 1))
            return 0;

        if (IS_OBJ_TYPE(obj, ITEM_MONEY))
            money_found = true;
        else if (GET_OBJ_VNUM(obj) == QUAD_VNUM)
            quad_found = true;
        else if (GET_OBJ_SIGIL_IDNUM(obj)
            && GET_OBJ_SIGIL_IDNUM(obj) != GET_IDNUM(ch))
            sigil_found = true;

    }
    //
    // 'get all' or 'get all.item'
    //

    else {

        if (dotmode == FIND_ALLDOT) {
            match_name = arg;
        }

        bool found = false;
        bool display = false;
        int counter = 1;

        for (obj = ch->in_room->contents; obj; obj = next_obj) {
            next_obj = obj->next_content;

            if (IS_CORPSE(obj) && CORPSE_IDNUM(obj) != GET_IDNUM(ch) &&
                GET_LEVEL(ch) < LVL_AMBASSADOR &&
                ch->in_room->zone->pk_style == ZONE_NEUTRAL_PK &&
                IS_PC(ch) && CORPSE_IDNUM(obj) > 0) {
                send_to_char(ch, "You can't take corpses in NPK zones!\r\n");
                continue;
            }

            if (!can_see_object(ch, obj)) {
                continue;
            }
            //
            // match_name is set if this is a find alldot get
            //

            if (match_name && !isname(match_name, obj->aliases))
                continue;

            if (!next_obj ||
                next_obj->name != obj->name || !can_see_object(ch, next_obj)
                || !can_take_obj(ch, next_obj, true, false)) {
                display = true;
            }

            if (!perform_get_from_room(ch, obj, display, counter)) {
                counter = 1;
            } else {
                found = true;

                if (IS_OBJ_TYPE(obj, ITEM_MONEY))
                    money_found = true;
                else if (GET_OBJ_VNUM(obj) == QUAD_VNUM)
                    quad_found = true;
                else if (GET_OBJ_SIGIL_IDNUM(obj)
                    && GET_OBJ_SIGIL_IDNUM(obj) != GET_IDNUM(ch))
                    sigil_found = true;

                if (!display) {
                    ++counter;
                } else {
                    counter = 1;
                }
            }

            display = false;
        }

        if (!found) {

            if (match_name) {
                sprintf(buf,
                    "You didn't find anything to take that looks like a '%s'.\r\n",
                    match_name);
                send_to_char(ch, "%s", buf);
            } else {
                send_to_char(ch, "You didn't find anything to take.\r\n");
            }
            return 0;
        }
    }

    if (money_found)
        consolidate_char_money(ch);

    if (quad_found)
        activate_char_quad(ch);

    if (sigil_found) {
        retval = explode_all_sigils(ch);
    }

    return retval;
}

ACMD(do_get)
{
    int cont_dotmode = -1;
    struct obj_data *cont = NULL;
    char *arg1 = tmp_getword(&argument);
    char *arg2 = tmp_getword(&argument);
    char *tmp_args;

    total_coins = total_credits = 0;

    if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
        send_to_char(ch, "Your arms are already full!\r\n");
        return;
    }

    if (!*arg1) {
        send_to_char(ch, "Get what?\r\n");
        return;
    }

    if (!*arg2) {
        get_from_room(ch, arg1);
        return;
    }

    cont_dotmode = find_all_dots(arg2);

    if (cont_dotmode == FIND_INDIV) {

        generic_find(arg2,
            FIND_OBJ_INV | FIND_OBJ_EQUIP_CONT | FIND_OBJ_ROOM,
            ch, NULL, &cont);

        if (!cont) {
            send_to_char(ch, "You don't have %s %s.\r\n", AN(arg2), arg2);
            return;
        }

        if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER) {
            act("$p is not a container.", false, ch, cont, NULL, TO_CHAR);
            return;
        }

        get_from_container(ch, cont, arg1);
        return;
    }

    char *match_container_name = NULL;

    // cont_dotmode != FIND_INDIV

    if (cont_dotmode == FIND_ALLDOT) {
        if (!*arg2) {
            send_to_char(ch, "Get from all of what?\r\n");
            return;
        }
        match_container_name = arg2;
    }

    bool container_found = false;

    //
    // scan inventory for matching containers
    //

    for (cont = ch->carrying; cont; cont = cont->next_content) {

        if (can_see_object(ch, cont) &&
            (!match_container_name
                || isname(match_container_name, cont->aliases))) {

            // If we don't have a match_container_name then this was a
            // get (x. | all.)y from all.  In that case, we really shouldn't
            // scream about everything in their inventory that isn't a
            // container.
            if (!IS_OBJ_TYPE(cont, ITEM_CONTAINER)) {
                if (match_container_name)
                    act("$p is not a container.", false, ch, cont, NULL, TO_CHAR);
                continue;
            }

            container_found = true;

            // We need to copy off the data in arg1 here because if
            // it contains dots we want to preserve those for every
            // call to get_from_container();
            tmp_args = tmp_strdup(arg1);
            get_from_container(ch, cont, tmp_args);
        }
    }

    //
    // scan room for matching containers
    //

    for (cont = ch->in_room->contents; cont; cont = cont->next_content) {

        if (can_see_object(ch, cont) &&
            (!match_container_name
                || isname(match_container_name, cont->aliases))) {

            // If we don't have a match_container_name then this was a
            // get (x. | all.)y from all.  In that case, we really shouldn't
            // scream about everything in the room that isn't a
            // container.
            if (!IS_OBJ_TYPE(cont, ITEM_CONTAINER)) {
                if (match_container_name)
                    act("$p is not a container.", false, ch, cont, NULL, TO_CHAR);
                continue;
            }

            container_found = true;

            // We need to copy off the data in arg1 here because if
            // it contains dots we want to preserve those for every
            // call to get_from_container();
            tmp_args = tmp_strdup(arg1);
            get_from_container(ch, cont, tmp_args);
        }
    }

    if (!container_found) {
        if (match_container_name) {
            send_to_char(ch, "You can't seem to find any %ss here.\r\n", arg2);
        } else {
            send_to_char(ch, "You can't seem to find any containers.\r\n");
        }
    }
}

void
perform_drop_gold(struct creature *ch, int amount,
    int8_t mode, struct room_data *RDR)
{
    struct obj_data *obj;

    if (amount <= 0)
        send_to_char(ch, "Heh heh heh.. we are jolly funny today, eh?\r\n");
    else if (GET_GOLD(ch) < amount)
        send_to_char(ch, "You don't have that many coins!\r\n");
    else {
        if (mode != SCMD_JUNK) {
            WAIT_STATE(ch, PULSE_VIOLENCE); /* to prevent coin-bombing */
            obj = create_money(amount, 0);
            if (mode == SCMD_DONATE) {
                send_to_char(ch,
                    "You throw some gold into the air where it disappears in a puff of smoke!\r\n");
                act("$n throws some gold into the air where it disappears in a puff of smoke!", false, ch, NULL, NULL, TO_ROOM);
                obj_to_room(obj, RDR);
                act("$p suddenly appears in a puff of orange smoke!", 0, NULL,
                    obj, NULL, TO_ROOM);
            } else {
                obj_to_room(obj, ch->in_room);
                send_to_char(ch, "You drop some gold.\r\n");
                act("$n drops $p.", false, ch, obj, NULL, TO_ROOM);
            }
        } else {
            sprintf(buf, "$n drops %s which disappears in a puff of smoke!",
                money_desc(amount, 0));
            act(buf, false, ch, NULL, NULL, TO_ROOM);
            send_to_char(ch,
                "You drop some gold which disappears in a puff of smoke!\r\n");
        }
        GET_GOLD(ch) -= amount;
        if (amount >= MONEY_LOG_LIMIT)
            switch (mode) {
            case SCMD_DONATE:
                slog("MONEY: %s has donated %d coins in room #%d (%s)",
                    GET_NAME(ch), amount, ch->in_room->number,
                    ch->in_room->name);
                break;
            case SCMD_JUNK:
                slog("MONEY: %s has junked %d coins in room #%d (%s)",
                    GET_NAME(ch), amount, ch->in_room->number,
                    ch->in_room->name);
                break;
            default:
                slog("MONEY: %s has dropped %d coins in room #%d (%s)",
                    GET_NAME(ch), amount, ch->in_room->number,
                    ch->in_room->name);
                break;
            }
    }
}

void
perform_drop_credits(struct creature *ch, int amount,
    int8_t mode, struct room_data *RDR)
{
    struct obj_data *obj;

    if (amount <= 0)
        send_to_char(ch, "Heh heh heh.. we are jolly funny today, eh?\r\n");
    else if (GET_CASH(ch) < amount)
        send_to_char(ch, "You don't have that much cash!\r\n");
    else {
        if (mode != SCMD_JUNK) {
            WAIT_STATE(ch, PULSE_VIOLENCE); /* to prevent coin-bombing */
            obj = create_money(amount, 1);
            if (mode == SCMD_DONATE) {
                send_to_char(ch,
                    "You throw some cash into the air where it disappears in a puff of smoke!\r\n");
                act("$n throws some cash into the air where it disappears in a puff of smoke!", false, ch, NULL, NULL, TO_ROOM);
                obj_to_room(obj, RDR);
                act("$p suddenly appears in a puff of orange smoke!", 0, NULL,
                    obj, NULL, TO_ROOM);
            } else {
                obj_to_room(obj, ch->in_room);
                act("You drop $p.", false, ch, obj, NULL, TO_CHAR);
                act("$n drops $p.", false, ch, obj, NULL, TO_ROOM);
            }
        } else {
            sprintf(buf, "$n drops %s which disappears in a puff of smoke!",
                money_desc(amount, 1));
            act(buf, false, ch, NULL, NULL, TO_ROOM);
            send_to_char(ch,
                "You drop some cash which disappears in a puff of smoke!\r\n");
        }
        GET_CASH(ch) -= amount;
        if (amount >= MONEY_LOG_LIMIT) {
            slog("MONEY: %s has dropped %d credits in room #%d (%s)",
                GET_NAME(ch), amount, ch->in_room->number, ch->in_room->name);
        }
    }
}

#define VANISH(mode) ((mode == SCMD_DONATE || mode == SCMD_JUNK) ? \
		      "  It vanishes in a puff of smoke!" : "")
#define PROTO_SDESC(vnum) (real_object_proto(vnum)->name)

bool
is_undisposable(struct creature *ch, const char *cmdstr, struct obj_data *obj,
    bool display)
{
    if (IS_CORPSE(obj) && CORPSE_IDNUM(obj) > 0 && obj->contains &&
        !is_authorized(ch, DESTROY_CORPSES, NULL)) {
        send_to_char(ch,
            "You can't %s a player's corpse while it still has objects in it.\r\n",
            cmdstr);
        return true;
    }

    if (IS_OBJ_TYPE(obj, ITEM_CONTAINER) && !IS_CORPSE(obj) &&
        obj->contains) {
        if (display)
            send_to_char(ch, "You can't %s a container with items in it!\r\n",
                cmdstr);
        return true;
    }

    if (GET_LEVEL(ch) > LVL_SPIRIT)
        return false;

    if (obj->shared && obj->shared->vnum >= 0 &&
        strcmp(obj->name, PROTO_SDESC(obj->shared->vnum))) {
        if (display)
            send_to_char(ch, "You can't %s a renamed object!\r\n", cmdstr);
        return true;
    }

    return false;
}

int
perform_drop(struct creature *ch, struct obj_data *obj,
    int8_t mode, const char *sname, struct room_data *RDR, int display)
{
    int value;

    if (!obj)
        return 0;

    if (IS_OBJ_STAT(obj, ITEM_NODROP)) {
        if (GET_LEVEL(ch) < LVL_TIMEGOD && !NPC_FLAGGED(ch, NPC_UTILITY)) {
            sprintf(buf, "You can't %s $p, it must be CURSED!", sname);
            act(buf, false, ch, obj, NULL, TO_CHAR);
            return 0;
        } else
            act("You peel $p off your hand...", false, ch, obj, NULL, TO_CHAR);
    }

    if ((mode == SCMD_DONATE || mode == SCMD_JUNK) &&
        is_undisposable(ch, (mode == SCMD_JUNK) ? "junk" : "donate", obj,
            true))
        return 0;

    if (display == true) {
        sprintf(buf, "You %s $p.%s", sname, VANISH(mode));
        act(buf, false, ch, obj, NULL, TO_CHAR);
        sprintf(buf, "$n %ss $p.%s", sname, VANISH(mode));
        act(buf, true, ch, obj, NULL, TO_ROOM);
    }

    if ((mode == SCMD_DONATE) && (IS_OBJ_STAT(obj, ITEM_NODONATE) ||
            !OBJ_APPROVED(obj)))
        mode = SCMD_JUNK;

    switch (mode) {
    case SCMD_DROP:
        obj_from_char(obj);
        obj_to_room(obj, ch->in_room);
        if (room_is_open_air(ch->in_room) &&
            EXIT(ch, DOWN) &&
            EXIT(ch, DOWN)->to_room &&
            !IS_SET(EXIT(ch, DOWN)->exit_info, EX_CLOSED)) {
            act("$p falls downward through the air, out of sight!",
                false, ch, obj, NULL, TO_ROOM);
            act("$p falls downward through the air, out of sight!",
                false, ch, obj, NULL, TO_CHAR);
            obj_from_room(obj);
            obj_to_room(obj, EXIT(ch, DOWN)->to_room);
            act("$p falls from the sky and lands by your feet.",
                false, NULL, obj, NULL, TO_ROOM);
        }
        return 1;
        break;
    case SCMD_DONATE:
        obj_from_char(obj);
        obj_to_room(obj, RDR);
        act("$p suddenly appears in a puff a smoke!", false, NULL, obj, NULL,
            TO_ROOM);
        if (!IS_OBJ_TYPE(obj, ITEM_OTHER) && !IS_OBJ_TYPE(obj, ITEM_TREASURE)
            && !IS_OBJ_TYPE(obj, ITEM_METAL))
            SET_BIT(GET_OBJ_EXTRA(obj), ITEM_NOSELL);
        return 1;
        break;
    case SCMD_JUNK:
        value = MAX(1, MIN(200, GET_OBJ_COST(obj) >> 4));
        // Don't actually extract the object here, since we may need it for
        // display purposes
        return value;
        break;
    default:
        errlog("Incorrect argument passed to perform_drop");
        break;
    }

    return 0;
}

ACMD(do_drop)
{
    extern room_num donation_room_1;    /* Modrian */
    extern room_num donation_room_2;    /* EC */
    extern room_num donation_room_3;    /* New Thalos */
    extern room_num donation_room_istan;    /* Istan */
    extern room_num donation_room_solace;
    extern room_num donation_room_skullport_common;
    extern room_num donation_room_skullport_dwarven;

    extern int guild_donation_info[][4];
    int i;
    struct obj_data *obj, *next_obj = NULL;
    struct room_data *RDR = NULL;
    int8_t oldmode;
    int8_t mode = SCMD_DROP;
    int dotmode, amount = 0, counter = 0, found;
    const char *sname = NULL;
    char *arg1, *arg2;
    char *to_char, *to_room;

    if (subcmd == SCMD_GUILD_DONATE) {
        for (i = 0; guild_donation_info[i][0] != -1; i++) {
            if (guild_donation_info[i][0] == GET_CLASS(ch) &&
                !((guild_donation_info[i][1] == EVIL && IS_GOOD(ch)) ||
                    (guild_donation_info[i][1] == GOOD && IS_EVIL(ch))) &&
                (guild_donation_info[i][2] == GET_HOME(ch))) {
                RDR = real_room(guild_donation_info[i][3]);
                break;
            }
        }
        subcmd = SCMD_DONATE;
        mode = subcmd;
        sname = "donate";
    }

    if (RDR == NULL) {
        switch (subcmd) {
        case SCMD_JUNK:
            sname = "junk";
            mode = SCMD_JUNK;
            break;
        case SCMD_DONATE:
            sname = "donate";
            mode = SCMD_DONATE;
            switch (number(0, 18)) {
            case 0:
            case 1:
                mode = SCMD_JUNK;
                RDR = NULL;
                break;
            case 2:
            case 3:
            case 4:
            case 5:
                RDR = real_room(donation_room_1);
                break;          /* Modrian */
            case 6:
            case 7:
                RDR = real_room(donation_room_3);
                break;          /* New Thalos */
            case 8:
            case 9:
                RDR = real_room(donation_room_istan);
                break;          /* Istan */
            case 10:
            case 11:
                RDR = real_room(donation_room_solace);
                break;          /* Solace Cove */
            case 12:
            case 13:
            case 14:
                RDR = real_room(donation_room_skullport_common);
                break;          /* sk */
            case 15:
                RDR = real_room(donation_room_skullport_dwarven);
                break;
            case 16:
            case 17:
            case 18:
                RDR = real_room(donation_room_2);
                break;
            }
            if (RDR == NULL && mode != SCMD_JUNK) {
                send_to_char(ch,
                    "Sorry, you can't donate anything right now.\r\n");
                return;
            }
            break;
        default:
            sname = "drop";
            break;
        }
    }

    arg1 = tmp_getword(&argument);
    arg2 = tmp_getword(&argument);

    if (!*arg1) {
        send_to_char(ch, "What do you want to %s?\r\n", sname);
        return;
    } else if (is_number(arg1)) {
        amount = atoi(arg1);

        if (!strcasecmp("coins", arg2) || !strcasecmp("coin", arg2) || !strcasecmp("gold", arg2))
            perform_drop_gold(ch, amount, mode, RDR);
        else if (!strcasecmp("credits", arg2) || !strcasecmp("credit", arg2))
            perform_drop_credits(ch, amount, mode, RDR);
        else {
            /* code to drop multiple items.  anyone want to write it? -je */
            send_to_char(ch,
                "Sorry, you can't do that to more than one item at a time.\r\n");
        }
        return;
    }

    dotmode = find_all_dots(arg1);

    // Can't junk or donate all
    if (dotmode == FIND_ALL && subcmd == SCMD_JUNK) {
        send_to_char(ch, "Go to the dump if you want to junk EVERYTHING!\r\n");
        return;
    }

    if (dotmode == FIND_ALL && subcmd == SCMD_DONATE) {
        send_to_char(ch,
            "Go to the donation room if you want to donate EVERYTHING!\r\n");
        return;
    }
    // Get initial object
    if (dotmode == FIND_ALL)
        obj = ch->carrying;
    else
        obj = get_obj_in_list_all(ch, arg1, ch->carrying);

    // Do we have the initial object?
    if (!obj) {
        if (dotmode == FIND_ALL)
            send_to_char(ch, "You don't seem to be carrying anything.\r\n");
        else
            send_to_char(ch, "You don't seem to have any %ss.\r\n", arg1);
        return;
    }

    if (subcmd == SCMD_JUNK && GET_MOVE(ch) == 0) {
        send_to_char(ch,
            "You don't have the energy to be junking anything!\r\n");
        return;
    }
    // Iterate through objects
    counter = 0;
    while (obj) {
        switch (dotmode) {
        case FIND_ALL:
            next_obj = obj->next_content;
            break;
        case FIND_ALLDOT:
            next_obj = get_obj_in_list_all(ch, arg1, obj->next_content);
            break;
        case FIND_INDIV:
            next_obj = NULL;
            break;
        default:
            errlog("Can't happen at %s:%d", __FILE__, __LINE__);
        }

        oldmode = mode;
        if (IS_OBJ_TYPE(obj, ITEM_KEY) && !GET_OBJ_VAL(obj, 1) &&
            mode != SCMD_DROP)
            mode = SCMD_JUNK;
        if (IS_BOMB(obj) && obj->contains && IS_FUSE(obj->contains))
            mode = SCMD_DROP;

        found = perform_drop(ch, obj, mode, sname, RDR, false);
        mode = oldmode;

        if (found) {
            counter++;

            amount += found;

            if (subcmd == SCMD_JUNK && !IS_IMMORT(ch)) {
                GET_MOVE(ch) -= 1;
                if (GET_MOVE(ch) == 0) {
                    send_to_char(ch,
                                 "You only have the energy to junk %d item%s.\r\n",
                                 counter, (counter == 1) ? "" : "s");
                    next_obj = NULL;
                }
            }

            if (!next_obj || strcmp(next_obj->name, obj->name)) {
                if (counter > 0) {
                    if (counter == 1) {
                        to_char = tmp_sprintf("You %s $p.%s", sname, VANISH(mode));
                        to_room = tmp_sprintf("$n %ss $p.%s", sname, VANISH(mode));
                    } else {
                        to_char = tmp_sprintf("You %s $p.%s (x%d)", sname,
                                              VANISH(mode), counter);
                        to_room = tmp_sprintf("$n %ss $p.%s (x%d)", sname,
                                              VANISH(mode), counter);
                    }
                    act(to_char, false, ch, obj, NULL, TO_CHAR);
                    act(to_room, true, ch, obj, NULL, TO_ROOM);
                }
                counter = 0;
            }
        // We needed the object until we had displayed the message.
        // Now that the message has been displayed, it's no longer
        // necessary
            if (subcmd == SCMD_JUNK)
                extract_obj(obj);
        }

        obj = next_obj;
    }

    if (subcmd == SCMD_JUNK && amount && !IS_IMMORT(ch)) {
        send_to_char(ch, "You have been rewarded by the gods!\r\n");
        act("$n has been rewarded by the gods!", true, ch, NULL, NULL, TO_ROOM);
        GET_GOLD(ch) += amount;
    }
}

int
perform_give(struct creature *ch, struct creature *vict,
    struct obj_data *obj, int display)
{
    int i;

    if (IS_OBJ_STAT(obj, ITEM_NODROP)) {
        if (GET_LEVEL(ch) < LVL_TIMEGOD) {
            act("You can't let go of $p!!  Yeech!", false, ch, obj, NULL,
                TO_CHAR);
            return 0;
        } else
            act("You peel $p off your hand...", false, ch, obj, NULL, TO_CHAR);
    }
    if (GET_LEVEL(ch) < LVL_IMMORT && GET_POSITION(vict) <= POS_SLEEPING) {
        act("$E is currently unconscious.", false, ch, NULL, vict, TO_CHAR);
        return 0;
    }
    if (IS_CARRYING_N(vict) >= CAN_CARRY_N(vict)) {
        act("$N seems to have $S hands full.", false, ch, NULL, vict, TO_CHAR);
        return 0;
    }
    if (GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(vict) > CAN_CARRY_W(vict)) {
        act("$E can't carry that much weight.", false, ch, NULL, vict, TO_CHAR);
        return 0;
    }
    obj_from_char(obj);
    obj_to_char(obj, vict);
    if (display == true) {
        act("You give $p to $N.", false, ch, obj, vict, TO_CHAR);
        act("$n gives you $p.", false, ch, obj, vict, TO_VICT);
        act("$n gives $p to $N.", true, ch, obj, vict, TO_NOTVICT);
    }

    if (IS_OBJ_TYPE(obj, ITEM_MONEY) && GET_OBJ_VAL(obj, 0) > MONEY_LOG_LIMIT)
        slog("MONEY: %s has given obj #%d (%s) worth %d %s to %s in room #%d (%s)", GET_NAME(ch), GET_OBJ_VNUM(obj), obj->name, GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1) ? "credits" : "gold", GET_NAME(vict), vict->in_room->number, vict->in_room->name);

    if (IS_NPC(vict) && AWAKE(vict) && !AFF_FLAGGED(vict, AFF_CHARM) &&
        can_see_object(vict, obj)) {
        if (IS_BOMB(obj)) {
            if ((isname("bomb", obj->aliases)
                    || isname("grenade", obj->aliases)
                    || isname("explosives", obj->aliases)
                    || isname("explosive", obj->aliases))
                && (CHECK_SKILL(vict, SKILL_DEMOLITIONS) < number(10, 50)
                    || (obj->contains && IS_FUSE(obj->contains)
                        && FUSE_STATE(obj->contains)))) {
                if (!vict->fighting
                    && CHECK_SKILL(vict, SKILL_DEMOLITIONS) > number(40, 60))
                    if (FUSE_IS_BURN(obj->contains))
                        do_extinguish(vict, fname(obj->aliases), 0, 0);
                    else
                        do_activate(vict, fname(obj->aliases), 0, 1);
                else {
                    if (GET_POSITION(vict) < POS_FIGHTING)
                        do_stand(vict, NULL, 0, 0);
                    for (i = 0; i < NUM_DIRS; i++) {
                        if (ch->in_room->dir_option[i] &&
                            ch->in_room->dir_option[i]->to_room && i != UP &&
                            !IS_SET(ch->in_room->dir_option[i]->exit_info,
                                EX_CLOSED)) {
                            sprintf(buf, "%s %s", fname(obj->aliases),
                                dirs[i]);
                            do_throw(vict, buf, 0, 0);
                            return 1;
                        }
                    }
                    if (can_see_creature(vict, ch)) {
                        sprintf(buf, "%s %s", fname(obj->aliases),
                            fname(ch->player.name));
                        do_give(vict, buf, 0, 0);
                    } else
                        do_throw(vict, fname(obj->aliases), 0, 0);
                }
            }
            return 1;
        }
        if ((IS_CARRYING_W(vict) + IS_WEARING_W(vict)) > (CAN_CARRY_W(vict) / 2))  // i don't want that heavy shit
            do_drop(vict, fname(obj->aliases), 0, 0);
    }
    return 1;
}

void
perform_plant(struct creature *ch, struct creature *vict, struct obj_data *obj)
{
    if (IS_OBJ_STAT(obj, ITEM_NODROP)) {
        act("You can't let go of $p!!  Yeech!", false, ch, obj, NULL, TO_CHAR);
        return;
    }
    if (IS_CARRYING_N(vict) >= CAN_CARRY_N(vict)) {
        act("$N seems to have $S hands full.", false, ch, NULL, vict, TO_CHAR);
        return;
    }
    if (GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(vict) > CAN_CARRY_W(vict)) {
        act("$E can't carry that much weight.", false, ch, NULL, vict, TO_CHAR);
        return;
    }
    obj_from_char(obj);
    obj_to_char(obj, vict);
    act("You plant $p on $N.", false, ch, obj, vict, TO_CHAR);
    if ((CHECK_SKILL(ch, SKILL_PLANT) + GET_DEX(ch)) < (number(0, 83) + GET_WIS(vict)) ||
        AFF2_FLAGGED(vict, AFF2_TRUE_SEEING))
        act("$n puts $p in your pocket.", false, ch, obj, vict, TO_VICT);
    else
        gain_skill_prof(ch, SKILL_PLANT);
}

/* utility function for give */
struct creature *
give_find_vict(struct creature *ch, char *arg)
{
    struct creature *vict;

    if (!*arg) {
        send_to_char(ch, "To who?\r\n");
        return NULL;
    } else if (!(vict = get_char_room_vis(ch, arg))) {
        send_to_char(ch, "%s", NOPERSON);
        return NULL;
    } else if (vict == ch) {
        send_to_char(ch, "What's the point of that?\r\n");
        return NULL;
    } else
        return vict;
}

void
transfer_money(struct creature *from, struct creature *to, money_t amt,
    int currency, bool plant)
{
    const char *currency_str, *cmd_str;
    money_t on_hand;

    if (currency == 1) {
        currency_str = "cred";
        on_hand = GET_CASH(from);
    } else {
        currency_str = "gold coin";
        on_hand = GET_GOLD(from);
    }

    if (amt <= 0) {
        send_to_char(from,
            "Heh heh heh ... we are jolly funny today, eh?\r\n");
        return;
    }
    if (on_hand < amt) {
        send_to_char(from, "You don't have that many %ss!\r\n", currency_str);
        return;
    }

    if (!plant && GET_LEVEL(from) < LVL_IMMORT
        && GET_POSITION(to) <= POS_SLEEPING) {
        act("$E is currently unconscious.", false, from, NULL, to, TO_CHAR);
        return;
    }

    if (plant) {
        send_to_char(from, "You plant %'" PRId64 " %s%s on %s.\r\n", amt, currency_str,
            amt == 1 ? "" : "s", PERS(to, from));
        if (IS_IMMORT(to) || (GET_SKILL(from, SKILL_PLANT) + GET_DEX(from)) <
            (number(0, 83) + GET_WIS(to))) {
            act(tmp_sprintf("%'" PRId64 " %s%s planted in your pocket by $n.", amt,
                    currency_str, amt == 1 ? "is" : "s are"),
                false, from, NULL, to, TO_VICT);
        }
        cmd_str = "planted";
    } else {
        send_to_char(from, "You give %'" PRId64 " %s%s to %s.\r\n", amt, currency_str,
            amt == 1 ? "" : "s", PERS(to, from));
        act(tmp_sprintf("You are given %'" PRId64 " %s%s by $n.", amt, currency_str,
                amt == 1 ? "" : "s"), false, from, NULL, to, TO_VICT);
        act(tmp_sprintf("$n gives %s to $N.", money_desc(amt, currency)),
            true, from, NULL, to, TO_NOTVICT);
        cmd_str = "given";
    }

    if (currency == 1) {
        GET_CASH(from) -= amt;
        GET_CASH(to) += amt;
    } else {
        GET_GOLD(from) -= amt;
        GET_GOLD(to) += amt;
    }

    if (amt >= MONEY_LOG_LIMIT)
        slog("MONEY: %s has %s %'" PRId64 " %s%s to %s in room #%d (%s)",
            GET_NAME(from), cmd_str, amt, currency_str, (amt == 1) ? "" : "s",
            GET_NAME(to), to->in_room->number, to->in_room->name);

    if (IS_PC(from))
        crashsave(from);
    if (IS_PC(to))
        crashsave(to);
}

ACMD(do_give)
{
    struct creature *vict;
    struct obj_data *obj, *next_obj = NULL;
    int dotmode, amount = 0, counter = 0, found;
    char *arg1, *arg2, *arg3;
    char *to_char, *to_vict, *to_room;

    arg1 = tmp_getword(&argument);
    arg2 = tmp_getword(&argument);
    arg3 = tmp_getword(&argument);

    if (!*arg1) {
        send_to_char(ch, "What do you want to give?\r\n");
        return;
    }
    if (!*arg2) {
        if (!strcasecmp(arg1, "all"))
            send_to_char(ch, "To whom do you wish to give everything?\r\n");
        else
            send_to_char(ch, "To whom do you wish to give %s %s?\r\n",
                AN(arg1), arg1);
        return;
    }

    if (is_number(arg1)) {
        amount = atoi(arg1);

        vict = give_find_vict(ch, arg3);
        if (!vict)
            return;

        if (!strcasecmp("coins", arg2) || !strcasecmp("coin", arg2) || !strcasecmp("gold", arg2))
            transfer_money(ch, vict, amount, 0, false);
        else if (!strcasecmp("credits", arg2) || !strcasecmp("credit", arg2))
            transfer_money(ch, vict, amount, 1, false);
        else
            /* code to give multiple items.  anyone want to write it? -je */
            send_to_char(ch,
                "Sorry, you can't do that to more than one item at a time.\r\n");
        return;
    }

    vict = give_find_vict(ch, arg2);
    if (!vict)
        return;

    dotmode = find_all_dots(arg1);

    // Can't junk or donate all
    if (dotmode == FIND_ALL && subcmd == SCMD_JUNK) {
        send_to_char(ch, "Go to the dump if you want to junk EVERYTHING!\r\n");
        return;
    }

    if (dotmode == FIND_ALL && subcmd == SCMD_DONATE) {
        send_to_char(ch,
            "Go do the donation room if you want to donate EVERYTHING!\r\n");
        return;
    }
    // Get initial object
    if (dotmode == FIND_ALL)
        obj = ch->carrying;
    else
        obj = get_obj_in_list_all(ch, arg1, ch->carrying);

    // Do we have the initial object?
    if (!obj) {
        if (dotmode == FIND_ALL)
            send_to_char(ch, "You don't seem to be carrying anything.\r\n");
        else
            send_to_char(ch, "You don't seem to have any %ss.\r\n", arg1);
        return;
    }
    // Iterate through objects
    counter = 0;
    while (obj) {
        switch (dotmode) {
        case FIND_ALL:
            next_obj = obj->next_content;
            break;
        case FIND_ALLDOT:
            next_obj = get_obj_in_list_all(ch, arg1, obj->next_content);
            break;
        case FIND_INDIV:
            next_obj = NULL;
            break;
        default:
            errlog("Can't happen at %s:%d", __FILE__, __LINE__);
        }

        found = perform_give(ch, vict, obj, false);

        if (found)
            counter++;

        if (!next_obj || next_obj->name != obj->name) {
            if (counter > 0) {
                if (counter == 1) {
                    to_char = tmp_sprintf("You give $p to $N.");
                    to_vict = tmp_sprintf("$n gives you $p.");
                    to_room = tmp_sprintf("$n gives $p to $N.");
                } else {
                    to_char = tmp_sprintf("You give $p to $N. (x%d)", counter);
                    to_vict = tmp_sprintf("$n gives you $p. (x%d)", counter);
                    to_room = tmp_sprintf("$n gives $p to $N. (x%d)", counter);
                }
                act(to_char, false, ch, obj, vict, TO_CHAR);
                act(to_vict, false, ch, obj, vict, TO_VICT);
                act(to_room, true, ch, obj, vict, TO_NOTVICT);
            }
            trigger_prog_give(ch, vict, obj);
            counter = 0;
        }
        obj = next_obj;
    }
}

ACMD(do_plant)
{
    int amount, dotmode;
    struct creature *vict;
    struct obj_data *obj, *next_obj;

    char *arg = tmp_getword(&argument);

    if (!*arg)
        send_to_char(ch, "Plant what on who?\r\n");
    else if (is_number(arg)) {
        amount = atoi(arg);
        argument = one_argument(argument, arg);
        if (!strcasecmp("coins", arg) || !strcasecmp("coin", arg)) {
            argument = one_argument(argument, arg);
            if ((vict = give_find_vict(ch, arg)))
                transfer_money(ch, vict, amount, 0, true);
            return;
        } else if (!strcasecmp("credits", arg) || !strcasecmp("credit", arg)) {
            argument = one_argument(argument, arg);
            if ((vict = give_find_vict(ch, arg)))
                transfer_money(ch, vict, amount, 1, true);
            return;
        } else {
            /* code to give multiple items.  anyone want to write it? -je */
            send_to_char(ch,
                "You can't give more than one item at a time.\r\n");
            return;
        }
    } else {
        one_argument(argument, buf1);
        if (!(vict = give_find_vict(ch, buf1)))
            return;
        dotmode = find_all_dots(arg);
        if (dotmode == FIND_INDIV) {
            if (!(obj = get_obj_in_list_all(ch, arg, ch->carrying))) {
                send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg),
                    arg);
            } else
                perform_plant(ch, vict, obj);
        } else {
            if (dotmode == FIND_ALLDOT && !*arg) {
                send_to_char(ch, "All of what?\r\n");
                return;
            }
            if (!ch->carrying)
                send_to_char(ch, "You don't seem to be holding anything.\r\n");
            else
                for (obj = ch->carrying; obj; obj = next_obj) {
                    next_obj = obj->next_content;
                    if (can_see_object(ch, obj) &&
                        ((dotmode == FIND_ALL || isname(arg, obj->aliases))))
                        perform_plant(ch, vict, obj);
                }
        }
    }
}

void
name_from_drinkcon(struct obj_data *obj, int type)
{
    char *new_name;
    struct obj_data *proto = NULL;

    // Remove the name from the alias list
    new_name = tmp_gsubi(obj->aliases, drinknames[type], "");
    // Remove double spaces that might be left over
    new_name = tmp_gsub(new_name, "  ", " ");
    // Remove leading and trailing spaces that might be left over
    new_name = tmp_trim(new_name);

    proto = real_object_proto(GET_OBJ_VNUM(obj));
    if (!proto || GET_OBJ_VNUM(proto) < 0 || obj->aliases != proto->aliases)
        free(obj->aliases);
    obj->aliases = strdup(new_name);
}

void
name_to_drinkcon(struct obj_data *obj, int type)
{
    char *new_name;
    struct obj_data *proto = NULL;

    // Don't add the drink to the aliases if it's already in there
    if (isname_exact(obj->aliases, drinknames[type]))
        return;

    new_name = strdup(tmp_sprintf("%s %s", obj->aliases, drinknames[type]));

    proto = real_object_proto(GET_OBJ_VNUM(obj));
    if (GET_OBJ_VNUM(obj) < 0 || obj->aliases != proto->aliases)
        free(obj->aliases);

    obj->aliases = new_name;
}

ACMD(do_drink)
{
    struct obj_data *temp;
    struct affected_type af;
    int amount;
    float weight;
    int drunk = 0, full = 0, thirst = 0;
    int on_ground = 0;

    init_affect(&af);

    char *arg = tmp_getword(&argument);

    if (!*arg) {
        send_to_char(ch, "Drink from what?\r\n");
        return;
    }
    if (!(temp = get_obj_in_list_all(ch, arg, ch->carrying))) {
        if (!(temp = get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
            act("You can't find it!", false, ch, NULL, NULL, TO_CHAR);
            return;
        } else
            on_ground = 1;
    }
    if (IS_OBJ_TYPE(temp, ITEM_POTION)) {
        send_to_char(ch, "You must QUAFF that!\r\n");
        return;
    }
    if ((GET_OBJ_TYPE(temp) != ITEM_DRINKCON) &&
        (GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN)) {
        send_to_char(ch, "You can't drink from that!\r\n");
        return;
    }
    if (on_ground && (IS_OBJ_TYPE(temp, ITEM_DRINKCON))) {
        send_to_char(ch, "You have to be holding that to drink from it.\r\n");
        return;
    }
    if (GET_COND(ch, DRUNK) > GET_CON(ch) || GET_COND(ch, DRUNK) == 24) {
        /* The pig is drunk */
        send_to_char(ch,
            "You can't seem to get it close enough to your mouth.\r\n");
        act("$n tries to drink but misses $s mouth!", true, ch, NULL, NULL, TO_ROOM);
        return;
    }
    if ((GET_COND(ch, FULL) > 20) && (GET_COND(ch, THIRST) > 0)) {
        send_to_char(ch, "Your stomach can't contain anymore!\r\n");
        return;
    }
    if (!GET_OBJ_VAL(temp, 1)) {
        send_to_char(ch, "It's empty.\r\n");
        return;
    }
    if (subcmd == SCMD_DRINK) {
        if (!ch->desc || !ch->desc->repeat_cmd_count) {
            sprintf(buf, "$n drinks %s from $p.", drinks[GET_OBJ_VAL(temp,
                        2)]);
            act(buf, true, ch, temp, NULL, TO_ROOM);
        }

        if (temp->action_desc && *temp->action_desc)
            send_to_char(ch, "%s\r\n", temp->action_desc);
        else
            send_to_char(ch, "You drink the %s.\r\n", drinks[GET_OBJ_VAL(temp,
                        2)]);

        if (GET_OBJ_VAL(temp, 1) > -1)
            amount = MIN(GET_OBJ_VAL(temp, 1), number(1, 3));
        else
            amount = number(1, 3);

    } else {
        act("$n sips from $p.", true, ch, temp, NULL, TO_ROOM);
        send_to_char(ch, "It tastes like %s.\r\n", drinks[GET_OBJ_VAL(temp,
                    2)]);
        amount = 1;
    }

    if ((GET_OBJ_VAL(temp, 1) != -1) && (GET_OBJ_VNUM(temp) != -1)) {
        weight = GET_OBJ_WEIGHT(real_object_proto(GET_OBJ_VNUM(temp)));
        weight += GET_OBJ_VAL(temp, 1) / 10;
        set_obj_weight(temp, weight);
    }

    drunk = (int)drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK] * amount;
    drunk /= 4;
    if (IS_MONK(ch))
        drunk >>= 2;
    full = (int)drink_aff[GET_OBJ_VAL(temp, 2)][FULL] * amount / 4;
    thirst = (int)drink_aff[GET_OBJ_VAL(temp, 2)][THIRST] * amount / 4;

    if (PRF2_FLAGGED(ch, PRF2_DEBUG))
        send_to_char(ch,
            "%s[DRINK] amount:%d   drunk:%d   full:%d   thirst:%d%s\r\n",
            CCCYN(ch, C_NRM), amount, drunk, full, thirst, CCCYN(ch, C_NRM));

    gain_condition(ch, DRUNK, drunk);
    gain_condition(ch, FULL, full);
    gain_condition(ch, THIRST, thirst);

    if (GET_COND(ch, DRUNK) > GET_CON(ch) || GET_COND(ch, DRUNK) == 24)
        send_to_char(ch, "You are on the verge of passing out!\r\n");
    else if (GET_COND(ch, DRUNK) > GET_CON(ch) * 3 / 4)
        send_to_char(ch, "You are feeling no pain.\r\n");
    else if (GET_COND(ch, DRUNK) > GET_CON(ch) / 2)
        send_to_char(ch, "You feel pretty damn drunk.\r\n");
    else if (GET_COND(ch, DRUNK) > GET_CON(ch) / 4)
        send_to_char(ch, "You feel pretty good.\r\n");

    if (GET_COND(ch, THIRST) > 20)
        send_to_char(ch, "Your thirst has been quenched.\r\n");

    if (GET_COND(ch, FULL) > 20)
        send_to_char(ch, "Your belly is satiated.\r\n");

    if (GET_OBJ_VAL(temp, 2) == LIQ_STOUT)
        GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + number(0, 3));

    if (OBJ_IS_RAD(temp)) {     // The shit was radioactive!!
    }
    if (GET_OBJ_VAL(temp, 3)) { // The shit was poisoned !
        send_to_char(ch, "Oops, it tasted rather strange!\r\n");
        if (!number(0, 1))
            act("$n chokes and utters some strange sounds.", false, ch, NULL, NULL,
                TO_ROOM);
        else
            act("$n sputters and coughs.", false, ch, NULL, NULL, TO_ROOM);

        af.location = APPLY_STR;
        af.modifier = -2;
        af.duration = amount * 3;
        af.type = SPELL_POISON;
        af.level = 30;

        if (GET_OBJ_VAL(temp, 3) == 2) {
            if (HAS_POISON_3(ch))
                af.duration = amount;
            else
                af.bitvector = AFF3_POISON_2;
            af.aff_index = 3;
        } else if (GET_OBJ_VAL(temp, 3) == 3) {
            af.bitvector = AFF3_POISON_3;
            af.aff_index = 3;
        } else if (GET_OBJ_VAL(temp, 3) == 4) {
            af.type = SPELL_SICKNESS;
            af.bitvector = AFF3_SICKNESS;
            af.aff_index = 3;
        } else {
            if (HAS_POISON_3(ch) || HAS_POISON_2(ch))
                af.duration = amount;
            else
                af.bitvector = AFF_POISON;
            af.aff_index = 1;
        }

        affect_join(ch, &af, true, true, true, false);
    }
    /* empty the container, and no longer poison. */

    if (GET_OBJ_VAL(temp, 1) != -1) {
        GET_OBJ_VAL(temp, 1) -= amount;
        if (!GET_OBJ_VAL(temp, 1)) {    /* The last bit */
            GET_OBJ_VAL(temp, 2) = 0;
            GET_OBJ_VAL(temp, 3) = 0;
            name_from_drinkcon(temp, GET_OBJ_VAL(temp, 2));
        }
    }
    return;
}

ACMD(do_eat)
{
    struct obj_data *food;
    struct affected_type af;
    int amount;

    init_affect(&af);

    char *arg = tmp_getword(&argument);

    if (!*arg) {
        send_to_char(ch, "Eat what?\r\n");
        return;
    }
    if (!(food = get_obj_in_list_all(ch, arg, ch->carrying))) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
        return;
    }
    if (subcmd == SCMD_TASTE && ((IS_OBJ_TYPE(food, ITEM_DRINKCON)) ||
            (IS_OBJ_TYPE(food, ITEM_FOUNTAIN)))) {
        do_drink(ch, argument, 0, SCMD_SIP);
        return;
    }
    if (!IS_OBJ_TYPE(food, ITEM_FOOD) &&
        !is_authorized(ch, EAT_ANYTHING, NULL)) {
        send_to_char(ch, "You can't eat THAT!\r\n");
        return;
    }
    if (GET_COND(ch, FULL) > 20) {  /* Stomach full */
        act("Your belly is stuffed to capacity!", false, ch, NULL, NULL, TO_CHAR);
        return;
    }
    if (subcmd == SCMD_EAT) {
        act("You eat $p.", false, ch, food, NULL, TO_CHAR);
        act("$n eats $p.", true, ch, food, NULL, TO_ROOM);
    } else {
        act("You nibble a little bit of $p.", false, ch, food, NULL, TO_CHAR);
        act("$n tastes a little bit of $p.", true, ch, food, NULL, TO_ROOM);
    }

    amount = (subcmd == SCMD_EAT ? GET_OBJ_VAL(food, 0) : 1);

    gain_condition(ch, FULL, amount);

    // Avoid the food being destroyed by being in the char's inventory
    // when the char is rented out by arena death
    obj_from_char(food);
    obj_to_room(food, ch->in_room);

    if (IS_OBJ_TYPE(food, ITEM_FOOD)) {
        if (GET_OBJ_VAL(food, 1) != 0
            && GET_OBJ_VAL(food, 2) != 0
            && subcmd == SCMD_EAT) {
            mag_objectmagic(ch, food, buf);
            if (is_dead(ch)) {
                extract_obj(food);
                return;
            }
        }
    }
    if (IS_OBJ_TYPE(food, ITEM_FOOD) && GET_COND(ch, FULL) > 20)
        act("You are satiated.", false, ch, NULL, NULL, TO_CHAR);

    if (GET_OBJ_VAL(food, 3) && (GET_LEVEL(ch) < LVL_AMBASSADOR)) {
        /* The shit was poisoned ! */
        send_to_char(ch, "Oops, that tasted rather strange!\r\n");
        if (!number(0, 1))
            act("$n coughs and utters some strange sounds.", false, ch, NULL, NULL,
                TO_ROOM);
        else
            act("$n sputters and chokes.", false, ch, NULL, NULL, TO_ROOM);

        af.location = APPLY_STR;
        af.modifier = -2;
        af.duration = amount * 3;
        af.type = SPELL_POISON;

        if (GET_OBJ_VAL(food, 3) == 2) {
            if (HAS_POISON_3(ch))
                af.duration = amount;
            else
                af.bitvector = AFF3_POISON_2;
            af.aff_index = 3;
        } else if (GET_OBJ_VAL(food, 3) == 3) {
            af.bitvector = AFF3_POISON_3;
            af.aff_index = 3;
        } else if (GET_OBJ_VAL(food, 3) == 4) {
            af.type = SPELL_SICKNESS;
            af.bitvector = AFF3_SICKNESS;
            af.aff_index = 3;
            send_to_char(ch, "You feel sick.\r\n");
        } else {
            if (HAS_POISON_3(ch) || HAS_POISON_2(ch))
                af.duration = amount;
            else
                af.bitvector = AFF_POISON;
            af.aff_index = 1;
        }

        if (subcmd != SCMD_EAT)
            af.duration >>= 1;
        affect_join(ch, &af, true, true, true, false);
    }

    if (subcmd == SCMD_EAT)
        extract_obj(food);
    else {
        if (!(--GET_OBJ_VAL(food, 0))) {
            send_to_char(ch, "There's nothing left now.\r\n");
            extract_obj(food);
        }
    }
}

ACMD(do_pour)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    struct obj_data *from_obj = NULL;
    struct obj_data *to_obj = NULL;
    int amount;
    float weight;

    two_arguments(argument, arg1, arg2);

    if (subcmd == SCMD_POUR) {
        if (!*arg1) {           /* No arguments */
            act("What do you want to pour from?", false, ch, NULL, NULL, TO_CHAR);
            return;
        }
        if (!(from_obj = get_obj_in_list_all(ch, arg1, ch->carrying))) {
            act("You can't find it!", false, ch, NULL, NULL, TO_CHAR);
            return;
        }
        if (GET_OBJ_TYPE(from_obj) != ITEM_DRINKCON) {
            act("You can't pour from that!", false, ch, NULL, NULL, TO_CHAR);
            return;
        }
    }
    if (subcmd == SCMD_FILL) {
        if (!*arg1) {           /* no arguments */
            send_to_char(ch,
                "What do you want to fill?  And what are you filling it from?\r\n");
            return;
        }
        if (!(to_obj = get_obj_in_list_all(ch, arg1, ch->carrying))) {
            send_to_char(ch, "You can't find it!\r\n");
            return;
        }
        if (GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) {
            act("You can't fill $p!", false, ch, to_obj, NULL, TO_CHAR);
            return;
        }
        if (GET_OBJ_VAL(to_obj, 0) == -1) {
            act("No need to fill $p, because it never runs dry!",
                false, ch, to_obj, NULL, TO_CHAR);
            return;
        }
        if (!*arg2) {           /* no 2nd argument */
            act("What do you want to fill $p from?", false, ch, to_obj, NULL,
                TO_CHAR);
            return;
        }
        if (!(from_obj = get_obj_in_list_vis(ch, arg2, ch->in_room->contents))) {
            send_to_char(ch, "There doesn't seem to be %s %s here.\r\n",
                AN(arg2), arg2);
            return;
        }
        if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN) {
            act("You can't fill something from $p.", false, ch, from_obj, NULL,
                TO_CHAR);
            return;
        }
    }
    if (GET_OBJ_VAL(from_obj, 1) == 0) {
        act("$p is empty.", false, ch, from_obj, NULL, TO_CHAR);
        return;
    }

    if (subcmd == SCMD_POUR) {  /* pour */
        if (!*arg2) {
            act("Where do you want it?  Out or in what?", false, ch, NULL, NULL,
                TO_CHAR);
            return;
        }
        if (!strcasecmp(arg2, "out")) {
            if (GET_OBJ_VAL(from_obj, 0) == -1) {
                send_to_char(ch,
                    "Now that would be an exercise in futility!\r\n");
                return;
            }
            act("$n empties $p.", true, ch, from_obj, NULL, TO_ROOM);
            act("You empty $p.", false, ch, from_obj, NULL, TO_CHAR);

            // Set the weight of the from obj to wehatever the weight of the
            // prototype is to avoid errors.
            GET_OBJ_VAL(from_obj, 1) = 0;
            GET_OBJ_VAL(from_obj, 2) = 0;
            GET_OBJ_VAL(from_obj, 3) = 0;
            if (GET_OBJ_VNUM(from_obj) != -1) {
                weight =
                    GET_OBJ_WEIGHT(real_object_proto(GET_OBJ_VNUM(from_obj)));
                weight += GET_OBJ_VAL(from_obj, 1) / 10;
                set_obj_weight(from_obj, weight);
            }
            name_from_drinkcon(from_obj, GET_OBJ_VAL(from_obj, 2));

            return;
        }
        if (!(to_obj = get_obj_in_list_all(ch, arg2, ch->carrying))) {
            act("You can't find it!", false, ch, NULL, NULL, TO_CHAR);
            return;
        }
        if ((GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) &&
            (GET_OBJ_TYPE(to_obj) != ITEM_FOUNTAIN)) {
            act("You can't pour anything into that.", false, ch, NULL, NULL,
                TO_CHAR);
            return;
        }
    }
    if (to_obj == from_obj) {
        act("A most unproductive effort.", false, ch, NULL, NULL, TO_CHAR);
        return;
    }
    if ((GET_OBJ_VAL(to_obj, 1) != 0) &&
        (GET_OBJ_VAL(to_obj, 2) != GET_OBJ_VAL(from_obj, 2))) {
        act("There is already another liquid in it!", false, ch, NULL, NULL,
            TO_CHAR);
        return;
    }
    if (!(GET_OBJ_VAL(to_obj, 1) < GET_OBJ_VAL(to_obj, 0)) ||
        GET_OBJ_VAL(to_obj, 0) == -1) {
        act("There is no room for more.", false, ch, NULL, NULL, TO_CHAR);
        return;
    }
    if (GET_OBJ_VAL(from_obj, 0) == -1) {
        act("You cannot seem to pour from $p.", false, ch, from_obj, NULL,
            TO_CHAR);
        return;
    }
    if (subcmd == SCMD_POUR) {
        send_to_char(ch, "You pour the %s into the %s.\r\n",
            drinks[GET_OBJ_VAL(from_obj, 2)], arg2);
    }
    if (subcmd == SCMD_FILL) {
        act("You gently fill $p from $P.", false, ch, to_obj, from_obj,
            TO_CHAR);
        act("$n gently fills $p from $P.", true, ch, to_obj, from_obj,
            TO_ROOM);
    }

    /* Calculate how much to pour */
    amount = MIN(GET_OBJ_VAL(to_obj, 0) - GET_OBJ_VAL(to_obj, 1),
                 GET_OBJ_VAL(from_obj, 1));

    /* New alias */
    if (GET_OBJ_VAL(to_obj, 1) == 0)
        name_to_drinkcon(to_obj, GET_OBJ_VAL(from_obj, 2));

    /* First same type liq. */
    GET_OBJ_VAL(to_obj, 2) = GET_OBJ_VAL(from_obj, 2);

    /* Then the liquid amount boogie */
    GET_OBJ_VAL(from_obj, 1) -= amount;
    GET_OBJ_VAL(to_obj, 1) += amount;

    /* Then the poison boogie */
    GET_OBJ_VAL(to_obj, 3) =
        (GET_OBJ_VAL(to_obj, 3) || GET_OBJ_VAL(from_obj, 3));

    /* And the weight boogie */
    if ((GET_OBJ_VNUM(from_obj)) != -1 && (GET_OBJ_VNUM(to_obj) != -1)) {
        weight = GET_OBJ_WEIGHT(real_object_proto(GET_OBJ_VNUM(from_obj)));
        weight += GET_OBJ_VAL(from_obj, 1) / 10;
        set_obj_weight(from_obj, weight);

        weight = GET_OBJ_WEIGHT(real_object_proto(GET_OBJ_VNUM(to_obj)));
        weight += GET_OBJ_VAL(to_obj, 1) / 10;
        set_obj_weight(to_obj, weight);
    }

    return;
}

void
wear_message(struct creature *ch, struct obj_data *obj, int where)
{
    const char *wear_messages[][2] = {
        {"$n lights $p and holds it.",
            "You light $p and hold it."},

        {"$n slides $p on to $s right ring finger.",
            "You slide $p on to your right ring finger."},

        {"$n slides $p on to $s left ring finger.",
            "You slide $p on to your left ring finger."},

        {"$n wears $p around $s neck.",
            "You wear $p around your neck."},

        {"$n wears $p around $s neck.",
            "You wear $p around your neck."},

        {"$n wears $p on $s body.",
            "You wear $p on your body.",},

        {"$n wears $p on $s head.",
            "You wear $p on your head."},

        {"$n puts $p on $s legs.",
            "You put $p on your legs."},

        {"$n wears $p on $s feet.",
            "You wear $p on your feet."},

        {"$n puts $p on $s hands.",
            "You put $p on your hands."},

        {"$n wears $p on $s arms.",
            "You wear $p on your arms."},

        {"$n straps $p around $s arm as a shield.",
            "You start to use $p as a shield."},

        {"$n wears $p about $s body.",
            "You wear $p around your body."},

        {"$n wears $p around $s waist.",
            "You wear $p around your waist."},

        {"$n puts $p around $s right wrist.",
            "You put $p around your right wrist."},

        {"$n puts $p around $s left wrist.",
            "You put $p around your left wrist."},

        {"$n wields $p.",
            "You wield $p."},

        {"$n grabs $p.",
            "You grab $p."},

        {"$n puts $p on $s crotch.",
            "You put $p on your crotch."},

        {"$n puts $p on $s eyes.",
            "You put $p on your eyes."},

        {"$n wears $p on $s back.",
            "You wear $p on your back."},

        {"$n attaches $p to $s belt.",
            "You attach $p to your belt."},

        {"$n wears $p on $s face.",
            "You wear $p on your face."},

        {"$n puts $p in $s left ear lobe.",
            "You wear $p in your left ear."},

        {"$n puts $p in $s right ear lobe.",
            "You wear $p in your right ear."},

        {"$n wields $p in $s off hand.",
            "You wield $p in your off hand."},

        {"$n RAMS $p up $s ass!!!",
            "You RAM $p up your ass!!!"}

    };

    act(wear_messages[where][0], true, ch, obj, NULL, TO_ROOM);
    act(wear_messages[where][1], false, ch, obj, NULL, TO_CHAR);
}

/* returns same as equip_char(), true = killed da ho */
int
perform_wear(struct creature *ch, struct obj_data *obj, int where)
{
    int i;
    const char *already_wearing[] = {
        "You're already using a light.\r\n",
        "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
        "You're already wearing something on both of your ring fingers.\r\n",
        "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
        "You can't wear anything else around your neck.\r\n",
        "You're already wearing something on your body.\r\n",
        "You're already wearing something on your head.\r\n",
        "You're already wearing something on your legs.\r\n",
        "You're already wearing something on your feet.\r\n",
        "You're already wearing something on your hands.\r\n",
        "You're already wearing something on your arms.\r\n",
        "You're already using a shield.\r\n",
        "You're already wearing something about your body.\r\n",
        "You already have something around your waist.\r\n",
        "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
        "You're already wearing something around both of your wrists.\r\n",
        "You're already wielding a weapon.\r\n",
        "You're already holding something.\r\n",
        "You've already got something on your crotch.\r\n",
        "You already have something on your eyes.\r\n",
        "You already have something on your back.\r\n",
        "There is already something attached to your belt.\r\n",
        "You are already wearing something on your face.\r\n",
        "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
        "You are wearing something in both ears already.\r\n",
        "You are already wielding something in your off hand.\r\n",
        "You already have something stuck up yer butt!\r\n"
    };

    /* first, make sure that the wear position is valid. */
    if (IS_ANIMAL(ch)) {
        send_to_char(ch, "Animals don't wear things.\r\n");
        return 0;
    }
    /* lights are always worn in the light position */
    if (IS_OBJ_TYPE(obj, ITEM_LIGHT)) {
        if (!GET_OBJ_VAL(obj, 2)) {
            act("$p is no longer usable as a light.", false, ch, obj, NULL, TO_CHAR);
            return 0;
        }
        where = WEAR_LIGHT;
    }
    if (!CAN_WEAR(obj, wear_bitvectors[where])) {
        act("You can't wear $p there.", false, ch, obj, NULL, TO_CHAR);
        return 0;
    }
    if (IS_IMPLANT(obj) && where != WEAR_HOLD) {
        act("You cannot wear $p.", false, ch, obj, NULL, TO_CHAR);
        return 0;
    }
    /* for neck, finger, and wrist, ears try pos 2 if pos 1 is already full */
    if ((where == WEAR_FINGER_R) || (where == WEAR_NECK_1) ||
        (where == WEAR_WRIST_R) || (where == WEAR_EAR_L))
        if (GET_EQ(ch, where))
            where++;

    if (IS_OBJ_TYPE(obj, ITEM_TATTOO)) {
        act("You cannot wear $p.", false, ch, obj, NULL, TO_CHAR);
        return 0;
    }

    if (GET_EQ(ch, where)) {
        send_to_char(ch, "%s", already_wearing[where]);
        return 0;
    }
    if ((where == WEAR_BELT) && (!GET_EQ(ch, WEAR_WAIST))) {
        act("You need to be WEARING a belt to put $p on it.",
            false, ch, obj, NULL, TO_CHAR);
        return 0;
    }
    if (where == WEAR_HANDS && IS_OBJ_STAT2(obj, ITEM2_TWO_HANDED)) {
        if (GET_EQ(ch, WEAR_WIELD)) {
            act("You can't be using your hands for anything else to use $p.",
                false, ch, obj, NULL, TO_CHAR);
            return 0;
        }
    }
    if (where == WEAR_SHIELD || where == WEAR_HOLD) {
        if (GET_EQ(ch, WEAR_WIELD)) {
            if (IS_OBJ_STAT2(GET_EQ(ch, WEAR_WIELD), ITEM2_TWO_HANDED)) {
                act("You can't use $p while wielding your two handed weapon.",
                    false, ch, obj, NULL, TO_CHAR);
                return 0;
            } else if (GET_EQ(ch, WEAR_WIELD_2)) {
                act("You can't use $p while dual wielding.",
                    false, ch, obj, NULL, TO_CHAR);
                return 0;
            } else if (GET_EQ(ch, WEAR_HOLD)) {
                act("You are currently wielding $p and holding $P.",
                    false, ch, GET_EQ(ch, WEAR_WIELD), GET_EQ(ch, WEAR_HOLD),
                    TO_CHAR);
                return 0;
            } else if (GET_EQ(ch, WEAR_SHIELD)) {
                act("You are currently wielding $p and using $P.",
                    false, ch, GET_EQ(ch, WEAR_WIELD), GET_EQ(ch, WEAR_SHIELD),
                    TO_CHAR);
                return 0;
            }
        }
        if (GET_EQ(ch, WEAR_HANDS)) {
            if (IS_OBJ_STAT2(GET_EQ(ch, WEAR_HANDS), ITEM2_TWO_HANDED)) {
                act("You can't use $p while wearing $P on your hands.",
                    false, ch, obj, GET_EQ(ch, WEAR_HANDS), TO_CHAR);
                return 0;
            }
        }
    }
    // If the shield's too heavy, they cant make good use of it.
    if (where == WEAR_SHIELD
        && GET_OBJ_WEIGHT(obj) >
        1.5 * strength_wield_weight(GET_STR(ch))) {
        send_to_char(ch, "It's too damn heavy.\r\n");
        return 0;
    }
    if (!OBJ_APPROVED(obj) && GET_LEVEL(ch) < LVL_AMBASSADOR && !is_tester(ch)) {
        act("$p has not been approved for mortal use.",
            false, ch, obj, NULL, TO_CHAR);
        return 0;
    }
    if (IS_OBJ_STAT2(obj, ITEM2_BROKEN) && GET_LEVEL(ch) < LVL_GOD) {
        act("$p is broken, and unusable until it is repaired.",
            false, ch, obj, NULL, TO_CHAR);
        return 0;
    }
    if ((where == WEAR_WIELD || where == WEAR_WIELD_2) &&
        GET_OBJ_TYPE(obj) != ITEM_WEAPON &&
        !IS_ENERGY_GUN(obj) && !IS_GUN(obj)) {
        send_to_char(ch, "You cannot wield non-weapons.\r\n");
        return 0;
    }

    if (IS_PC(ch) && obj->shared->owner_id != 0 &&
        obj->shared->owner_id != GET_IDNUM(ch)) {
        const char *name = player_name_by_idnum(obj->shared->owner_id);
        char *msg = tmp_sprintf("$p can only be used by %s.",
            name ? name : "someone else");
        act(msg, false, ch, obj, NULL, TO_CHAR);
        return 0;
    }

    if (IS_OBJ_STAT2(obj, ITEM2_NO_MORT) && GET_LEVEL(ch) < LVL_AMBASSADOR &&
        !IS_REMORT(ch)) {
        act("You feel a strange sensation as you attempt to use $p.\r\n"
            "The universe slowly spins around you, and the fabric of space\r\n"
            "and time appears to warp.", false, ch, obj, NULL, TO_CHAR);
        return 0;
    }

    if (IS_OBJ_STAT2(obj, ITEM2_REQ_MORT) && GET_LEVEL(ch) < LVL_AMBASSADOR &&
        IS_REMORT(ch)) {
        act("You feel a strange sensation as you attempt to use $p.\r\n"
            "The universe slowly spins around you, and the fabric of space\r\n"
            "and time appears to warp.", false, ch, obj, NULL, TO_CHAR);
        return 0;
    }

    if (IS_OBJ_STAT2(obj, ITEM2_SINGULAR)) {
        for (i = 0; i < NUM_WEARS; i++) {
            if (GET_EQ(ch, i) &&
                (GET_OBJ_VNUM(GET_EQ(ch, i)) == GET_OBJ_VNUM(obj))) {
                act("$p:  You cannot use more than one of this item.",
                    false, ch, obj, NULL, TO_CHAR);
                return 0;
            }
        }
    }
    if (IS_OBJ_STAT2(obj, ITEM2_GODEQ) && GET_LEVEL(ch) < LVL_IMPL) {
        for (i = 0; i < NUM_WEARS; i++) {
            if (GET_EQ(ch, i) && !IS_OBJ_STAT2(GET_EQ(ch, i), ITEM2_GODEQ)) {
                act("$p swiftly slides off your body, into your hands.",
                    false, ch, GET_EQ(ch, i), NULL, TO_CHAR);
                obj_to_char(unequip_char(ch, i, EQUIP_WORN), ch);
            }
        }
    } else if (GET_LEVEL(ch) < LVL_IMPL) {
        for (i = 0; i < NUM_WEARS; i++) {
            if (GET_EQ(ch, i) && IS_OBJ_STAT2(GET_EQ(ch, i), ITEM2_GODEQ)) {
                act("$p swiftly leaps off your body, into your hands.",
                    false, ch, GET_EQ(ch, i), NULL, TO_CHAR);
                obj_to_char(unequip_char(ch, i, EQUIP_WORN), ch);
            }
        }
    }

    wear_message(ch, obj, where);
    if ((IS_OBJ_TYPE(obj, ITEM_WORN) ||
            IS_OBJ_TYPE(obj, ITEM_ARMOR) ||
            IS_OBJ_TYPE(obj, ITEM_WEAPON)) && obj->action_desc) {
        act(obj->action_desc, false, ch, obj, NULL, TO_CHAR);
    }
    obj_from_char(obj);
    if (equip_char(ch, obj, where, EQUIP_WORN))
        return 1;
    return check_eq_align(ch);
}

int
find_eq_pos(struct creature *ch, struct obj_data *obj, char *arg)
{
    int where = -1;

    if (!arg || !*arg) {
        if (CAN_WEAR(obj, ITEM_WEAR_FINGER))
            where = WEAR_FINGER_R;
        if (CAN_WEAR(obj, ITEM_WEAR_NECK))
            where = WEAR_NECK_1;
        if (CAN_WEAR(obj, ITEM_WEAR_BODY))
            where = WEAR_BODY;
        if (CAN_WEAR(obj, ITEM_WEAR_HEAD))
            where = WEAR_HEAD;
        if (CAN_WEAR(obj, ITEM_WEAR_LEGS))
            where = WEAR_LEGS;
        if (CAN_WEAR(obj, ITEM_WEAR_FEET))
            where = WEAR_FEET;
        if (CAN_WEAR(obj, ITEM_WEAR_HANDS))
            where = WEAR_HANDS;
        if (CAN_WEAR(obj, ITEM_WEAR_ARMS))
            where = WEAR_ARMS;
        if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))
            where = WEAR_SHIELD;
        if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))
            where = WEAR_ABOUT;
        if (CAN_WEAR(obj, ITEM_WEAR_WAIST))
            where = WEAR_WAIST;
        if (CAN_WEAR(obj, ITEM_WEAR_WRIST))
            where = WEAR_WRIST_R;
        if (CAN_WEAR(obj, ITEM_WEAR_CROTCH))
            where = WEAR_CROTCH;
        if (CAN_WEAR(obj, ITEM_WEAR_EYES))
            where = WEAR_EYES;
        if (CAN_WEAR(obj, ITEM_WEAR_BACK))
            where = WEAR_BACK;
        if (CAN_WEAR(obj, ITEM_WEAR_BELT))
            where = WEAR_BELT;
        if (CAN_WEAR(obj, ITEM_WEAR_FACE))
            where = WEAR_FACE;
        if (CAN_WEAR(obj, ITEM_WEAR_EAR))
            where = WEAR_EAR_L;
        if (CAN_WEAR(obj, ITEM_WEAR_ASS))
            where = WEAR_ASS;
    } else {
        if ((where = search_block(arg, wear_keywords, false)) < 0) {
            send_to_char(ch, "'%s'?  What part of your body is THAT?\r\n",
                arg);
        }
    }

    return where;
}

ACMD(do_wear)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    struct obj_data *obj, *next_obj;
    int where, dotmode, items_worn = 0;

    two_arguments(argument, arg1, arg2);

    if (!*arg1) {
        send_to_char(ch, "Wear what?\r\n");
        return;
    }
    dotmode = find_all_dots(arg1);

    if (*arg2 && (dotmode != FIND_INDIV)) {
        send_to_char(ch,
            "You can't specify the same body location for more than one item!\r\n");
        return;
    }
    if (dotmode == FIND_ALL) {
        for (obj = ch->carrying; obj; obj = next_obj) {
            next_obj = obj->next_content;
            if (can_see_object(ch, obj)
                && (where = find_eq_pos(ch, obj, NULL)) >= 0) {
                items_worn++;
                if (perform_wear(ch, obj, where))
                    return;
            }
        }
        if (!items_worn)
            send_to_char(ch, "You don't seem to have anything wearable.\r\n");
    } else if (dotmode == FIND_ALLDOT) {
        if (!*arg1) {
            send_to_char(ch, "Wear all of what?\r\n");
            return;
        }
        if (!(obj = get_obj_in_list_all(ch, arg1, ch->carrying))) {
            send_to_char(ch, "You don't seem to have any %ss.\r\n", arg1);
        } else
            while (obj) {
                next_obj = get_obj_in_list_all(ch, arg1, obj->next_content);
                if ((where = find_eq_pos(ch, obj, NULL)) >= 0) {
                    if (perform_wear(ch, obj, where))
                        return;
                } else
                    act("You can't wear $p.", false, ch, obj, NULL, TO_CHAR);
                obj = next_obj;
            }
    } else {
        if (!(obj = get_obj_in_list_all(ch, arg1, ch->carrying))) {
            send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg1),
                arg1);
        } else {
            if ((where = find_eq_pos(ch, obj, arg2)) >= 0) {
                perform_wear(ch, obj, where);
            } else if (!*arg2)
                act("You can't wear $p.", false, ch, obj, NULL, TO_CHAR);
        }
    }
}

ACMD(do_wield)
{
    struct obj_data *obj;
    int hands_free = 2;

    char *arg = tmp_getword(&argument);

    if (!*arg) {
        send_to_char(ch, "Wield what?\r\n");
        return;
    }

    if (!(obj = get_obj_in_list_all(ch, arg, ch->carrying))) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
        return;
    }

    if (IS_ANIMAL(ch)) {
        send_to_char(ch, "Animals don't wield weapons.\r\n");
        return;
    }

    if (!CAN_WEAR(obj, ITEM_WEAR_WIELD)) {
        send_to_char(ch, "You can't wield that.\r\n");
        return;
    }

    if (GET_OBJ_WEIGHT(obj) > strength_wield_weight(GET_STR(ch))) {
        send_to_char(ch, "It's too damn heavy.\r\n");
        return;
    }

    if (IS_CLERIC(ch) && !IS_EVIL(ch) &&
        (IS_OBJ_TYPE(obj, ITEM_WEAPON)) &&
        ((GET_OBJ_VAL(obj, 3) == TYPE_SLASH - TYPE_HIT) ||
            (GET_OBJ_VAL(obj, 3) == TYPE_PIERCE - TYPE_HIT) ||
            (GET_OBJ_VAL(obj, 3) == TYPE_STAB - TYPE_HIT) ||
            (GET_OBJ_VAL(obj, 3) == TYPE_RIP - TYPE_HIT) ||
            (GET_OBJ_VAL(obj, 3) == TYPE_CHOP - TYPE_HIT) ||
            (GET_OBJ_VAL(obj, 3) == TYPE_CLAW - TYPE_HIT))) {
        send_to_char(ch, "You can't wield that as a cleric.\r\n");
        return;
    }

    hands_free = char_hands_free(ch);

    if (!hands_free) {
        send_to_char(ch, "You don't have a hand free to wield it with.\r\n");
        return;
    }

    if (hands_free != 2 && IS_OBJ_STAT2(obj, ITEM2_TWO_HANDED)) {
        act("You need both hands free to wield $p.", false, ch, obj, NULL,
            TO_CHAR);
        return;
    }

    if (GET_EQ(ch, WEAR_HANDS) &&
        IS_OBJ_STAT2(GET_EQ(ch, WEAR_HANDS), ITEM2_TWO_HANDED)) {
        act("You can't wield anything while wearing $p on your hands.",
            false, ch, GET_EQ(ch, WEAR_HANDS), NULL, TO_CHAR);
        return;
    }

    if (GET_EQ(ch, WEAR_WIELD)) {
        // Guns and Mercs have to be handled a bit differently, but I hate they
        // way I've done this.  However, I can't think of a better way ATM...
        // feel free to clean this up if you like
        if (IS_MERC(ch) && IS_ANY_GUN(GET_EQ(ch, WEAR_WIELD))
            && IS_ANY_GUN(obj))
            perform_wear(ch, obj, WEAR_WIELD_2);

        else if (GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD)) <= 6 ?
            (GET_OBJ_WEIGHT(obj) > GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD))) :
            (GET_OBJ_WEIGHT(obj) > GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD)) / 2))
            send_to_char(ch,
                "Your secondary weapon must weigh less than half of your primary weapon,\r\nif your primary weighs more than 6 lbs.\r\n");
        else
            perform_wear(ch, obj, WEAR_WIELD_2);
    } else
        perform_wear(ch, obj, WEAR_WIELD);
}

ACMD(do_grab)
{
    struct obj_data *obj;

    char *arg = tmp_getword(&argument);

    if (!*arg)
        send_to_char(ch, "Hold what?\r\n");
    else if (!(obj = get_obj_in_list_all(ch, arg, ch->carrying))) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
    } else {
        perform_wear(ch, obj, WEAR_HOLD);
    }
}

void
perform_remove(struct creature *ch, int pos)
{
    struct obj_data *obj;

    if (!(obj = GET_EQ(ch, pos))) {
        errlog("Error in perform_remove: bad pos passed.");
        return;
    }
    if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
        act("$p: you can't carry that many items!", false, ch, obj, NULL,
            TO_CHAR);
    else if ((GET_POSITION(ch) == POS_FLYING)
        && (IS_OBJ_TYPE(ch->equipment[pos], ITEM_WINGS))
        && (!AFF_FLAGGED(ch, AFF_INFLIGHT))) {
        act("$p: you probably shouldn't remove those while flying!", false, ch,
            obj, NULL, TO_CHAR);
    } else if (IS_OBJ_TYPE(obj, ITEM_TATTOO))
        act("$p: you must have this removed by a professional.",
            false, ch, obj, NULL, TO_CHAR);
    else if (IS_OBJ_STAT2(obj, ITEM2_NOREMOVE)
        && GET_LEVEL(ch) < LVL_AMBASSADOR)
        act("$p: you cannot convince yourself to stop using it.", false, ch,
            obj, NULL, TO_CHAR);
    else {
        if (pos == WEAR_WIELD && GET_EQ(ch, WEAR_WIELD_2)) {
            act("You stop using $p, and wield $P.",
                false, ch, obj, GET_EQ(ch, WEAR_WIELD_2), TO_CHAR);
            act("$n stops using $p, and wields $P.",
                true, ch, obj, GET_EQ(ch, WEAR_WIELD_2), TO_ROOM);
        } else {
            act("You stop using $p.", false, ch, obj, NULL, TO_CHAR);
            act("$n stops using $p.", true, ch, obj, NULL, TO_ROOM);
        }
        obj_to_char(unequip_char(ch, pos, EQUIP_WORN), ch);
    }
}

int
char_hands_free(struct creature *ch)
{
    struct obj_data *obj;
    int hands_free;

    hands_free = 2;

    obj = GET_EQ(ch, WEAR_WIELD);
    if (obj)
        hands_free -= IS_OBJ_STAT2(obj, ITEM2_TWO_HANDED) ? 2 : 1;
    obj = GET_EQ(ch, WEAR_WIELD_2);
    if (obj)
        hands_free -= IS_OBJ_STAT2(obj, ITEM2_TWO_HANDED) ? 2 : 1;
    obj = GET_EQ(ch, WEAR_SHIELD);
    if (obj)
        hands_free -= IS_OBJ_STAT2(obj, ITEM2_TWO_HANDED) ? 2 : 1;
    obj = GET_EQ(ch, WEAR_HOLD);
    if (obj)
        hands_free -= IS_OBJ_STAT2(obj, ITEM2_TWO_HANDED) ? 2 : 1;

    if (hands_free < 0) {
        // Report it to the immortals
        errlog("%s has negative hands free", GET_NAME(ch));
        // Now fix the problem
        act("Oops... You dropped everything that was in your hands.",
            false, ch, NULL, NULL, TO_CHAR);
        if (GET_EQ(ch, WEAR_WIELD))
            perform_remove(ch, WEAR_WIELD);
        if (GET_EQ(ch, WEAR_WIELD_2))
            perform_remove(ch, WEAR_WIELD_2);
        if (GET_EQ(ch, WEAR_SHIELD))
            perform_remove(ch, WEAR_SHIELD);
        if (GET_EQ(ch, WEAR_HOLD))
            perform_remove(ch, WEAR_HOLD);
        hands_free = 2;
    }
    return hands_free;
}

ACMD(do_remove)
{
    struct obj_data *obj;
    int i, dotmode, found;

    char *arg = tmp_getword(&argument);

    if (!*arg) {
        send_to_char(ch, "Remove what?\r\n");
        return;
    }
    if (AFF3_FLAGGED(ch, AFF3_ATTRACTION_FIELD) && !NOHASS(ch)) {
        send_to_char(ch,
            "You cannot remove anything while generating an attraction field!\r\n");
        return;
    }

    dotmode = find_all_dots(arg);

    if (dotmode == FIND_ALL) {
        found = 0;
        for (i = 0; i < NUM_WEARS; i++)
            if (GET_EQ(ch, i)) {
                perform_remove(ch, i);
                found = 1;
                if (GET_EQ(ch, i))
                    perform_remove(ch, i);
            }
        if (!found)
            send_to_char(ch, "You're not using anything.\r\n");
    } else if (dotmode == FIND_ALLDOT) {
        if (!*arg)
            send_to_char(ch, "Remove all of what?\r\n");
        else {
            found = 0;
            for (i = 0; i < NUM_WEARS; i++)
                if (GET_EQ(ch, i) && can_see_object(ch, GET_EQ(ch, i)) &&
                    isname(arg, GET_EQ(ch, i)->aliases)) {
                    perform_remove(ch, i);
                    found = 1;
                }
            if (!found) {
                send_to_char(ch, "You don't seem to be using any %ss.\r\n",
                    arg);
            }
        }
    } else {
        if (!(obj = get_object_in_equip_all(ch, arg, ch->equipment, &i))) {
            send_to_char(ch, "You don't seem to be using %s %s.\r\n", AN(arg),
                arg);
        } else if (IS_OBJ_STAT2(obj, ITEM2_NOREMOVE) &&
            GET_LEVEL(ch) < LVL_AMBASSADOR)
            act("Ack!  You cannot seem to let go of $p!", false, ch, obj, NULL,
                TO_CHAR);
        else
            perform_remove(ch, i);
    }
}

int
prototype_obj_value(struct obj_data *obj)
{
    int value = 0, prev_value = 0, j;

    if (!obj) {
        errlog("NULL obj passed to prototype_obj_value.");
        return 0;
    }

    /***** First the item-type specifics. ********/

    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_LIGHT:
        if (GET_OBJ_VAL(obj, 2) < 0)
            value += 10000;
        else
            value += (GET_OBJ_VAL(obj, 2));

        if (IS_METAL_TYPE(obj))
            value *= 2;
        break;

    case ITEM_CONTAINER:
        value += GET_OBJ_VAL(obj, 0);
        value <<= 2;
        value += ((30 - GET_OBJ_WEIGHT(obj)) / 4);
        if (IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSEABLE))
            value += 100;
        if (GET_OBJ_VAL(obj, 2)) {
            value += 100;
            if (IS_SET(GET_OBJ_VAL(obj, 1), CONT_PICKPROOF))
                value += 100;
        }
        if (GET_OBJ_VAL(obj, 3))                   /** corpse **/
            value = 0;
        break;

    case ITEM_WEAPON:
        value += GET_OBJ_VAL(obj, 1) * GET_OBJ_VAL(obj, 1) *
            GET_OBJ_VAL(obj, 1) *
            GET_OBJ_VAL(obj, 2) * GET_OBJ_VAL(obj, 2) * 4;

        if (GET_OBJ_VAL(obj, 0) && IS_OBJ_STAT2(obj, ITEM2_CAST_WEAPON)) {
            prev_value = value;
            value += MIN(value << 2, 200000);
            value = (prev_value + value) >> 1;
        }
        break;

    case ITEM_GUN:
        if (GUN_TYPE(obj) >= 0 && GUN_TYPE(obj) < NUM_GUN_TYPES)
            value += (gun_damage[GUN_TYPE(obj)][0] *
                gun_damage[GUN_TYPE(obj)][1]) << 4;
        if (!MAX_LOAD(obj))
            value *= 15;
        else
            value *= MAX_LOAD(obj);
        break;

    case ITEM_BULLET:
        if (GUN_TYPE(obj) >= 0 && GUN_TYPE(obj) < NUM_GUN_TYPES)
            value +=
                (gun_damage[GUN_TYPE(obj)][0] * gun_damage[GUN_TYPE(obj)][1]);
        value += (BUL_DAM_MOD(obj) * 100);
        break;

    case ITEM_ENERGY_GUN:
        value += GET_OBJ_VAL(obj, 1) * GET_OBJ_VAL(obj, 1) *
            GET_OBJ_VAL(obj, 1) *
            GET_OBJ_VAL(obj, 2) * GET_OBJ_VAL(obj, 2) * 4;

        value += 1000 * GET_OBJ_VAL(obj, 3);
        break;

    case ITEM_BATTERY:
        value += MAX_ENERGY(obj) * 10;
        break;

    case ITEM_ENERGY_CELL:
        value += MAX_ENERGY(obj) * 20;
        break;

    case ITEM_ARMOR:
        value += (GET_OBJ_VAL(obj, 0) * GET_OBJ_VAL(obj, 0) *
            GET_OBJ_VAL(obj, 0) * GET_OBJ_VAL(obj, 0));
        if (CAN_WEAR(obj, ITEM_WEAR_BODY)) {
            value *= 3;
            prev_value = value;
            value += ((50 - GET_OBJ_WEIGHT(obj)) * 8);
            value = (prev_value + value) >> 1;
        } else if (CAN_WEAR(obj, ITEM_WEAR_HEAD)
            || CAN_WEAR(obj, ITEM_WEAR_LEGS)) {
            value *= 2;
            prev_value = value;
            value += ((25 - GET_OBJ_WEIGHT(obj)) * 8);
            value = (prev_value + value) >> 1;
        } else {
            prev_value = value;
            value += ((25 - GET_OBJ_WEIGHT(obj)) * 8);
            value = (prev_value + value) >> 1;
        }
        if (IS_IMPLANT(obj))
            value <<= 1;
        break;

    default:
        value = GET_OBJ_COST(obj);  /* resets value for un-def'd item types ** */
        return value;
        break;
    }

    /***** Make sure its not negative... if it is maybe make it pos. **/
    value = MAX(value, -(value >> 1));

    /**** Next the values of the item's affections *****/
    for (j = 0; j < MAX_OBJ_AFFECT; j++) {

        switch (obj->affected[j].location) {
        case APPLY_STR:
        case APPLY_DEX:
        case APPLY_INT:
        case APPLY_WIS:
        case APPLY_CON:
            prev_value = value;
            value += (obj->affected[j].modifier) * value / 4;
            value = (prev_value + value) >> 1;
            break;

        case APPLY_CHA:
            prev_value = value;
            value += (obj->affected[j].modifier) * value / 5;
            value = (prev_value + value) >> 1;
            break;

        case APPLY_HIT:
        case APPLY_MOVE:
        case APPLY_MANA:
            prev_value = value;
            value += (obj->affected[j].modifier) * value / 25;
            value = (prev_value + value) >> 1;
            break;

        case APPLY_AC:
            prev_value = value;
            value -= (obj->affected[j].modifier) * value / 10;
            value = (prev_value + value) >> 1;
            break;

        case APPLY_HITROLL:
            prev_value = value;
            value += (obj->affected[j].modifier) * value / 5;
            value = (prev_value + value) >> 1;
            break;

        case APPLY_DAMROLL:
            prev_value = value;
            value += (obj->affected[j].modifier) * value / 4;
            value = (prev_value + value) >> 1;
            break;

        case APPLY_SAVING_PARA:
        case APPLY_SAVING_ROD:
        case APPLY_SAVING_PETRI:
        case APPLY_SAVING_BREATH:
        case APPLY_SAVING_SPELL:
            prev_value = value;
            value -= (obj->affected[j].modifier) * value / 4;
            value = (prev_value + value) >> 1;
            break;

        case APPLY_SNEAK:
        case APPLY_HIDE:
        case APPLY_BACKSTAB:
        case APPLY_PICK_LOCK:
        case APPLY_PUNCH:
        case APPLY_SHOOT:
        case APPLY_KICK:
        case APPLY_TRACK:
        case APPLY_IMPALE:
        case APPLY_BEHEAD:
        case APPLY_THROWING:
            prev_value = value;
            value += (obj->affected[j].modifier) * value / 15;
            value = (prev_value + value) >> 1;
            break;

        }
    }

    /***** Make sure its not negative... if it is maybe make it pos. **/
    value = MAX(value, -(value >> 1));

    /***** The value of items which set bitvectors ***** */
    for (j = 0; j < 3; j++)
        if (obj->obj_flags.bitvector[j]) {
            prev_value = value;
            value += 120000;
            value = (prev_value + value) >> 1;
        }

    switch (GET_OBJ_MATERIAL(obj)) {
    case MAT_GOLD:
        value *= 3;
        break;
    case MAT_SILVER:
        value *= 2;
        break;
    case MAT_PLATINUM:
        value *= 4;
        break;
    case MAT_KEVLAR:
        value *= 2;
        break;
    }

    return value;
}

int
set_maxdamage(struct obj_data *obj)
{

    int dam = 0;

    dam = (GET_OBJ_WEIGHT(obj) / 2);

    if (IS_LEATHER_TYPE(obj))
        dam *= 3;
    else if (IS_CLOTH_TYPE(obj))
        dam *= 2;
    else if (IS_WOOD_TYPE(obj))
        dam *= 4;
    else if (IS_METAL_TYPE(obj))
        dam *= 5;
    else if (IS_PLASTIC_TYPE(obj))
        dam *= 3;
    else if (IS_STONE_TYPE(obj))
        dam *= 6;

    if (GET_OBJ_MATERIAL(obj) == MAT_HARD_LEATHER)
        dam *= 2;
    else if (GET_OBJ_MATERIAL(obj) == MAT_SCALES)
        dam *= 3;

    switch (obj->obj_flags.type_flag) {
    case ITEM_WEAPON:
        dam *= 30;
        break;
    case ITEM_ARMOR:
        dam *= 30;
        dam *= MAX(1, GET_OBJ_VAL(obj, 0));
        break;
    case ITEM_ENGINE:
        dam *= 20;
        break;
    case ITEM_VEHICLE:
        dam *= 18;
        break;
    case ITEM_SCUBA_TANK:
        dam *= 5;
        break;
    case ITEM_BOAT:
        dam *= 10;
        break;
    case ITEM_BATTERY:
        dam *= 3;
        break;
    case ITEM_ENERGY_GUN:
        dam *= 20;
        break;
    case ITEM_METAL:
        dam *= 30;
        break;
    case ITEM_CHIT:
        dam *= 5;
        break;
    case ITEM_CONTAINER:
        dam *= 5;
        break;
    case ITEM_HOLY_SYMB:
        dam *= (MAX(1, GET_OBJ_VAL(obj, 2) / 10));
        dam *= (MAX(1, GET_OBJ_VAL(obj, 3) / 10));
        break;
    case ITEM_FOUNTAIN:
        dam *= 6;
        break;
    }

    if (IS_OBJ_STAT(obj, ITEM_MAGIC))
        dam <<= 1;
    if (IS_OBJ_STAT(obj, ITEM_MAGIC_NODISPEL))
        dam <<= 2;
    if (IS_OBJ_STAT(obj, ITEM_BLESS))
        dam <<= 1;
    if (IS_OBJ_STAT(obj, ITEM_DAMNED))
        dam <<= 1;
    if (IS_OBJ_STAT2(obj, ITEM2_CAST_WEAPON))
        dam <<= 1;
    if (IS_OBJ_STAT2(obj, ITEM2_CURSED_PERM))
        dam <<= 1;
    if (IS_OBJ_STAT2(obj, ITEM2_GODEQ))
        dam <<= 1;

    dam = MAX(100, dam);

    return (dam);

}

int
choose_material(struct obj_data *obj)
{
    int i;
    char name[128], aliases[512], *ptr;

    for (i = 0; i < TOP_MATERIAL; i++)
        if (isname(material_names[i], obj->aliases))
            return (i);

    strcpy(aliases, obj->name);
    ptr = aliases;

    ptr = one_argument(ptr, name);
    while (*name) {
        if (strcasecmp(name, "a") && strcasecmp(name, "an")
            && strcasecmp(name, "the")
            && strcasecmp(name, "some") && strcasecmp(name, "of")
            && strcasecmp(name, "black") && strcasecmp(name, "white")
            && strcasecmp(name, "green") && strcasecmp(name, "brown")) {
            for (i = 0; i < TOP_MATERIAL; i++)
                if (isname(material_names[i], name))
                    return (i);
        }
        ptr = one_argument(ptr, name);
    }

    if (obj->line_desc) {
        strcpy(aliases, obj->line_desc);
        ptr = aliases;

        ptr = one_argument(ptr, name);
        while (*name) {
            if (strcasecmp(name, "a") && strcasecmp(name, "an")
                && strcasecmp(name, "the") && strcasecmp(name, "some")
                && strcasecmp(name, "of") && strcasecmp(name, "black")
                && strcasecmp(name, "white") && strcasecmp(name, "green")
                && strcasecmp(name, "brown") && strcasecmp(name, "lying")
                && strcasecmp(name, "has") && strcasecmp(name, "been")
                && strcasecmp(name, "is") && strcasecmp(name, "left")
                && strcasecmp(name, "here") && strcasecmp(name, "set")
                && strcasecmp(name, "lies") && strcasecmp(name, "looking")
                && strcasecmp(name, "pair") && strcasecmp(name, "someone")
                && strcasecmp(name, "there") && strcasecmp(name, "has")
                && strcasecmp(name, "on") && strcasecmp(name, "dropped")) {
                for (i = 0; i < TOP_MATERIAL; i++)
                    if (isname(material_names[i], name))
                        return (i);
            }
            ptr = one_argument(ptr, name);
        }
    }

    if (isname("bottle", obj->aliases) || isname("glasses", obj->aliases) ||
        isname("jar", obj->aliases) || isname("mirror", obj->aliases))
        return (MAT_GLASS);

    if (isname("meat", obj->aliases) || isname("beef", obj->aliases)) {
        if (isname("raw", obj->aliases))
            return (MAT_MEAT_RAW);
        else
            return (MAT_MEAT_COOKED);
    }

    if (isname("burger", obj->aliases) || isname("hamburger", obj->aliases) ||
        isname("cheeseburger", obj->aliases))
        return (MAT_MEAT_COOKED);

    if (isname("oaken", obj->aliases) || isname("oak", obj->aliases))
        return (MAT_OAK);

    if (isname("tree", obj->aliases) || isname("branch", obj->aliases) ||
        isname("board", obj->aliases) || isname("wooden", obj->aliases) ||
        isname("barrel", obj->aliases) || isname("log", obj->aliases) ||
        isname("logs", obj->aliases) || isname("stick", obj->aliases) ||
        isname("pencil", obj->aliases))
        return (MAT_WOOD);

    if (isname("jean", obj->aliases) || isname("jeans", obj->aliases))
        return (MAT_DENIM);

    if (isname("hide", obj->aliases) || isname("pelt", obj->aliases) ||
        isname("skins", obj->aliases) || isname("viscera", obj->aliases))
        return (MAT_SKIN);

    if (isname("golden", obj->aliases) || isname("coin", obj->aliases) ||
        isname("coins", obj->aliases))
        return (MAT_GOLD);

    if (isname("silvery", obj->aliases))
        return (MAT_SILVER);

    if (isname("computer", obj->aliases))
        return (MAT_PLASTIC);

    if (isname("candle", obj->aliases))
        return (MAT_WAX);

    if (isname("corpse", obj->aliases) || isname("body", obj->aliases) ||
        isname("bodies", obj->aliases) || isname("corpses", obj->aliases) ||
        isname("carcass", obj->aliases) || isname("cadaver", obj->aliases))
        return (MAT_FLESH);

    if (isname("can", obj->aliases) || isname("bucket", obj->aliases))
        return (MAT_ALUMINUM);

    if (isname("claw", obj->aliases) || isname("claws", obj->aliases) ||
        isname("bones", obj->aliases) || isname("spine", obj->aliases) ||
        isname("skull", obj->aliases) || isname("tusk", obj->aliases) ||
        isname("tusks", obj->aliases) || isname("talons", obj->aliases) ||
        isname("talon", obj->aliases) || isname("teeth", obj->aliases) ||
        isname("tooth", obj->aliases))
        return (MAT_BONE);

    if (isname("jewel", obj->aliases) || isname("gem", obj->aliases))
        return (MAT_STONE);

    if (isname("khaki", obj->aliases) || isname("fabric", obj->aliases))
        return (MAT_CLOTH);

    if (isname("rock", obj->aliases) || isname("boulder", obj->aliases))
        return (MAT_STONE);

    if (isname("brick", obj->aliases))
        return (MAT_CONCRETE);

    if (isname("adamantite", obj->aliases))
        return (MAT_ADAMANTIUM);

    if (isname("rusty", obj->aliases) || isname("rusted", obj->aliases))
        return (MAT_IRON);

    if (isname("wicker", obj->aliases))
        return (MAT_RATTAN);

    if (isname("rope", obj->aliases))
        return (MAT_HEMP);

    if (isname("mug", obj->aliases))
        return (MAT_PORCELAIN);

    if (isname("tire", obj->aliases) || isname("tires", obj->aliases))
        return (MAT_RUBBER);

    if (isname("rug", obj->aliases))
        return (MAT_CARPET);

    if (IS_OBJ_TYPE(obj, ITEM_FOOD) && (isname("loaf", obj->aliases) ||
            isname("pastry", obj->aliases) ||
            isname("muffin", obj->aliases) ||
            isname("cracker", obj->aliases) ||
            isname("bisquit", obj->aliases) || isname("cake", obj->aliases)))
        return (MAT_BREAD);

    if (IS_OBJ_TYPE(obj, ITEM_FOOD) && (isname("fish", obj->aliases) ||
            isname("trout", obj->aliases) ||
            isname("steak", obj->aliases) ||
            isname("roast", obj->aliases) ||
            isname("fried", obj->aliases) ||
            isname("smoked", obj->aliases) ||
            isname("jerky", obj->aliases) ||
            isname("sausage", obj->aliases) || isname("fillet", obj->aliases)))
        return (MAT_MEAT_COOKED);

    if (IS_ENGINE(obj) || IS_VEHICLE(obj))
        return (MAT_STEEL);

    if (IS_OBJ_TYPE(obj, ITEM_SCROLL) || IS_OBJ_TYPE(obj, ITEM_NOTE) ||
        IS_OBJ_TYPE(obj, ITEM_CIGARETTE) || isname("papers", obj->aliases))
        return (MAT_PAPER);

    if (IS_OBJ_TYPE(obj, ITEM_KEY))
        return (MAT_IRON);

    if (IS_OBJ_TYPE(obj, ITEM_POTION))
        return (MAT_GLASS);

    if (IS_OBJ_TYPE(obj, ITEM_TOBACCO))
        return (MAT_LEAF);

    if (IS_OBJ_TYPE(obj, ITEM_WINDOW))
        return (MAT_GLASS);

    if (IS_OBJ_TYPE(obj, ITEM_TATTOO))
        return (MAT_SKIN);

    if (IS_OBJ_TYPE(obj, ITEM_WAND) || IS_OBJ_TYPE(obj, ITEM_STAFF))
        return (MAT_WOOD);

    if (IS_OBJ_TYPE(obj, ITEM_SYRINGE)) {
        if (isname("disposable", obj->aliases))
            return (MAT_PLASTIC);
        else
            return (MAT_GLASS);
    }

    if (IS_OBJ_TYPE(obj, ITEM_WORN))
        return (MAT_CLOTH);

    if (IS_OBJ_TYPE(obj, ITEM_ARMOR)) {
        if (isname("plate", obj->aliases) || isname("chain", obj->aliases) ||
            isname("plates", obj->aliases) || isname("plated", obj->aliases) ||
            isname("helmet", obj->aliases)
            || isname("breastplate", obj->aliases)
            || isname("shield", obj->aliases) || isname("mail", obj->aliases)
            || ((isname("scale", obj->aliases)
                    || isname("scales", obj->aliases)) && obj->name
                && !isname("dragon", obj->name)
                && !isname("dragons", obj->name)
                && !isname("dragon's", obj->name)
                && !isname("tiamat", obj->name)))
            return (MAT_STEEL);
        if (isname("boots", obj->aliases))
            return (MAT_LEATHER);
    }

    if (IS_OBJ_TYPE(obj, ITEM_WEAPON)) {
        if (isname("blade", obj->aliases) || isname("sword", obj->aliases) ||
            isname("dagger", obj->aliases) || isname("knife", obj->aliases) ||
            isname("axe", obj->aliases) || isname("longsword", obj->aliases) ||
            isname("scimitar", obj->aliases) || isname("halberd", obj->aliases)
            || isname("sabre", obj->aliases) || isname("cutlass", obj->aliases)
            || isname("rapier", obj->aliases) || isname("pike", obj->aliases)
            || isname("spear", obj->aliases))
            return (MAT_STEEL);
        if (isname("club", obj->aliases))
            return (MAT_WOOD);
        if (isname("whip", obj->aliases))
            return (MAT_LEATHER);
    }

    if (isname("chain", obj->aliases) || isname("chains", obj->aliases))
        return (MAT_METAL);

    if (isname("cloak", obj->aliases))
        return (MAT_CLOTH);

    if (isname("torch", obj->aliases))
        return (MAT_WOOD);

    return (MAT_NONE);
}

ACMD(do_objupdate)
{
    return;
}

ACMD(do_attach)
{
    struct obj_data *obj1 = NULL, *obj2 = NULL;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int i;

    skip_spaces(&argument);
    half_chop(argument, arg1, arg2);

    if (!*argument) {
        send_to_char(ch, "%stach what %s what?\r\n", subcmd ? "De" : "At",
            subcmd ? "from" : "to");
        return;
    }

    if (!(obj1 = get_obj_in_list_vis(ch, arg1, ch->carrying)) &&
        !(obj1 = get_object_in_equip_all(ch, arg1, ch->equipment, &i))) {
        send_to_char(ch, "You don't have %s '%s'.\r\n", AN(arg1), arg1);
        return;
    }

    if (!*arg2) {
        sprintf(buf, "%statch $p %s what?", subcmd ? "De" : "At",
            subcmd ? "from" : "to");
        act(buf, false, ch, obj1, NULL, TO_CHAR);
        return;
    }

    if (!(obj2 = get_obj_in_list_vis(ch, arg2, ch->carrying)) &&
        !(obj2 = get_object_in_equip_all(ch, arg2, ch->equipment, &i))) {
        send_to_char(ch, "You don't have %s '%s'.\r\n", AN(arg2), arg2);
        return;
    }

    if (!subcmd) {
        switch (GET_OBJ_TYPE(obj1)) {
        case ITEM_SCUBA_MASK:
        case ITEM_SCUBA_TANK:
            if ((IS_OBJ_TYPE(obj1, ITEM_SCUBA_MASK) &&
                    GET_OBJ_TYPE(obj2) != ITEM_SCUBA_TANK) ||
                (IS_OBJ_TYPE(obj2, ITEM_SCUBA_MASK) &&
                    GET_OBJ_TYPE(obj1) != ITEM_SCUBA_TANK)) {
                act("You cannot attach $p to $P.", false, ch, obj1, obj2,
                    TO_CHAR);
                return;
            }
            if (obj1->aux_obj)
                act("$p is already attached to $P.",
                    false, ch, obj1, obj1->aux_obj, TO_CHAR);
            else if (obj2->aux_obj)
                act("$p is already attached to $P.",
                    false, ch, obj2, obj2->aux_obj, TO_CHAR);
            else {
                obj1->aux_obj = obj2;
                obj2->aux_obj = obj1;
                act("You attach $p to $P.", false, ch, obj1, obj2, TO_CHAR);
                act("$n attaches $p to $P.", false, ch, obj1, obj2, TO_ROOM);
            }

            break;
        case ITEM_FUSE:
            if (!IS_BOMB(obj2)) {
                act("You cannot attach $p to $P.", false, ch, obj1, obj2,
                    TO_CHAR);
            } else if (obj2->contains) {
                if (IS_FUSE(obj2->contains))
                    act("$p is already attached to $P.",
                        false, ch, obj2->contains, obj2, TO_CHAR);
                else
                    act("$P is jammed with $p.",
                        false, ch, obj2->contains, obj2, TO_CHAR);
            } else if (FUSE_STATE(obj1)) {
                act("Better not do that -- $p is active!",
                    false, ch, obj1, NULL, TO_CHAR);
            } else if (!obj1->carried_by) {
                act("You can't attach $p while you are using it.",
                    false, ch, obj1, NULL, TO_CHAR);
            } else {
                obj_from_char(obj1);
                obj_to_obj(obj1, obj2);
                act("You attach $p to $P.", false, ch, obj1, obj2, TO_CHAR);
                act("$n attaches $p to $P.", false, ch, obj1, obj2, TO_ROOM);
            }
            break;
        default:
            act("You cannot attach $p to $P.", false, ch, obj1, obj2, TO_CHAR);
            break;
        }
    } else {

        if (!obj1->aux_obj || obj2 != obj1->aux_obj) {
            act("$p is not attached to $P.", false, ch, obj1, obj2, TO_CHAR);
            return;
        }

        switch (GET_OBJ_TYPE(obj1)) {
        case ITEM_SCUBA_MASK:
        case ITEM_SCUBA_TANK:
            if (obj1->aux_obj->aux_obj && obj1->aux_obj->aux_obj == obj1)
                obj1->aux_obj->aux_obj = NULL;
            obj1->aux_obj = NULL;

            act("You detach $p from $P.", false, ch, obj1, obj2, TO_CHAR);
            act("$n detaches $p from $P.", false, ch, obj1, obj2, TO_ROOM);
            break;

        default:
            send_to_char(ch, "...\n");
            break;
        }
    }
}

ACMD(do_conceal)
{

    struct obj_data *obj = NULL;
    int i;

    skip_spaces(&argument);
    if (!*argument) {
        send_to_char(ch, "Conceal what?\r\n");
        return;
    }

    if (!(obj = get_obj_in_list_vis(ch, argument, ch->carrying)) &&
        !(obj = get_object_in_equip_vis(ch, argument, ch->equipment, &i)) &&
        !(obj = get_obj_in_list_vis(ch, argument, ch->in_room->contents))) {
        send_to_char(ch, "You can't find %s '%s' here.\r\n", AN(argument),
            argument);
        return;
    }

    act("You conceal $p.", false, ch, obj, NULL, TO_CHAR);
    if (obj->in_room)
        act("$n conceals $p.", true, ch, obj, NULL, TO_ROOM);
    else
        act("$n conceals something.", true, ch, obj, NULL, TO_ROOM);

    WAIT_STATE(ch, 1 RL_SEC);
    if (obj->in_room && !CAN_GET_OBJ(ch, obj))
        return;

    if (CHECK_SKILL(ch, SKILL_CONCEAL) + GET_LEVEL(ch) > number(60, 120)) {
        SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_HIDDEN);
        gain_skill_prof(ch, SKILL_CONCEAL);
    }
}

ACMD(do_sacrifice)
{
    struct creature *load_corpse_owner(struct obj_data *obj);

    struct creature *orig_char;
    struct obj_data *obj;
    int mana;

    skip_spaces(&argument);

    if (!*argument) {
        send_to_char(ch, "Sacrifice what object?\r\n");
        return;
    }
    if (!(obj = get_obj_in_list_vis(ch, argument, ch->in_room->contents))) {
        send_to_char(ch, "You can't find any '%s' in the room.\r\n", argument);
        return;
    }
    if (!(CAN_WEAR(obj, ITEM_WEAR_TAKE))
        && !is_authorized(ch, DESTROY_NOTAKE, NULL)) {
        send_to_char(ch, "You can't sacrifice that.\r\n");
        return;
    }

    if (is_undisposable(ch, "sacrifice", obj, true))
        return;

    act("$n sacrifices $p.", false, ch, obj, NULL, TO_ROOM);
    act("You sacrifice $p.", false, ch, obj, NULL, TO_CHAR);
    if (IS_CORPSE(obj)) {
        orig_char = load_corpse_owner(obj);
        if (orig_char) {
            mana = number(1, level_bonus(orig_char, true));
            if (IS_PC(orig_char))
                free_creature(orig_char);
        } else
            mana = 0;
    } else
        mana = MAX(0, GET_OBJ_COST(obj) / 100000);

    if (mana) {
        send_to_char(ch, "You sense your deity's favor upon you.\r\n");
        GET_MANA(ch) += mana;
        if (GET_MANA(ch) > GET_MAX_MANA(ch))
            GET_MANA(ch) = GET_MAX_MANA(ch);
    }
    extract_obj(obj);
}

ACMD(do_empty)
{
    struct obj_data *obj = NULL;    // the object that will be emptied
    struct obj_data *next_obj = NULL;
    struct obj_data *o = NULL;
    struct obj_data *container = NULL;
    int bits;
    int bits2;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int objs_moved = 0;

    two_arguments(argument, arg1, arg2);

    if (!*arg1) {
        send_to_char(ch, "What do you want to empty?\r\n");
        return;
    }

    if (!(bits = generic_find(arg1, FIND_OBJ_INV, ch, NULL, &obj))) {
        send_to_char(ch, "You can't find any %s to empty\r\n.", arg1);
        return;
    }

    if (IS_OBJ_TYPE(obj, ITEM_DRINKCON)) {
        if (!*arg2)
            do_pour(ch, tmp_strcat(argument, " out", NULL), 0, 0);
        else
            do_pour(ch, argument, 0, 0);
        return;
    }

    if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER) {
        send_to_char(ch, "You can't empty that.\r\n");
        return;
    }
    if (!IS_CORPSE(obj)
        && IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSEABLE)
        && IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) {
        send_to_char(ch, "It seems to be closed.\r\n");
        return;

    }

    if (IS_CORPSE(obj) && CORPSE_IDNUM(obj) > 0
        && CORPSE_IDNUM(obj) != GET_IDNUM(ch)
        && !is_authorized(ch, EMPTY_CORPSES, NULL)) {
        send_to_char(ch, "You can't empty a player's corpse.");
        return;
    }

    if (*arg2 && obj) {

        if (!(bits2 =
                generic_find(arg2,
                    FIND_OBJ_EQUIP | FIND_OBJ_INV | FIND_OBJ_ROOM, ch, NULL,
                    &container))) {
            send_to_char(ch, "Empty %s into what?\r\n", obj->name);
            return;
        } else {
            empty_to_obj(obj, container, ch);
            return;
        }
    }

    if (obj->contains) {
        for (next_obj = obj->contains; next_obj; next_obj = o) {
            if (next_obj->in_obj) {
                o = next_obj->next_content;
                if (!(IS_OBJ_STAT(next_obj, ITEM_NODROP)) &&
                    (IS_SET(GET_OBJ_WEAR(next_obj), ITEM_WEAR_TAKE))) {
                    obj_from_obj(next_obj);
                    obj_to_room(next_obj, ch->in_room);
                    objs_moved++;
                }
            } else {
                o = NULL;
            }
        }
    }
    if (objs_moved) {
        act("$n empties the contents of $p.", false, ch, obj, NULL, TO_ROOM);
        act("You carefully empty the contents of $p.", false, ch, obj, NULL,
            TO_CHAR);
    } else {
        send_to_char(ch, "%s is already empty.\r\n", obj->name);
    }
}

int
empty_to_obj(struct obj_data *obj, struct obj_data *container,
    struct creature *ch)
{

    //
    // To clarify, obj is the object that contains the equipment originally and container is
    // the object that the eq is being transferred into.
    //

    struct obj_data *o = NULL;
    struct obj_data *next_obj = NULL;
    bool can_fit = true;
    int objs_moved = 0;

    if (!(obj) || !(container) || !(ch)) {
        errlog("Null value passed into empty_to_obj.\r\n");
        return 0;
    }

    if (obj == container) {
        send_to_char(ch,
            "Why would you want to empty something into itself?\r\n");
        return 0;
    }

    if (GET_OBJ_TYPE(container) != ITEM_CONTAINER) {
        send_to_char(ch, "You can't put anything in that.\r\n");
        return 0;
    }

    if (!obj->contains) {
        send_to_char(ch, "There is nothing to empty!\r\n");
        return 0;
    }

    if (obj->contains) {
        for (next_obj = obj->contains; next_obj; next_obj = o) {
            if (next_obj->in_obj && next_obj) {
                o = next_obj->next_content;
                if (!(IS_OBJ_STAT(next_obj, ITEM_NODROP)) &&
                    (IS_SET(GET_OBJ_WEAR(next_obj), ITEM_WEAR_TAKE))) {
                    if (GET_OBJ_WEIGHT(container) + GET_OBJ_WEIGHT(next_obj) >
                        GET_OBJ_VAL(container, 0)) {
                        can_fit = false;
                    }
                    if (can_fit) {
                        obj_from_obj(next_obj);
                        obj_to_obj(next_obj, container);
                        objs_moved++;
                    }
                    can_fit = true;
                }
            }
        }
        if (objs_moved) {
            sprintf(buf, "$n carefully empties the contents of $p into %s.",
                container->name);
            act(buf, false, ch, obj, NULL, TO_ROOM);
            sprintf(buf, "You carefully empty the contents of $p into %s.",
                container->name);
            act(buf, false, ch, obj, NULL, TO_CHAR);
        } else {
            act("$p couldn't hold anything.", false, ch, container, NULL,
                TO_CHAR);
            return 0;
        }

    }

    return 1;
}
