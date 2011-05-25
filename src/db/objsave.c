/* ************************************************************************
*   File: objsave.c                                     Part of CircleMUD *
*  Usage: loading/saving player objects for rent and crash-save           *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright ( C ) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright ( C ) 1990, 1991.               *
************************************************************************ */

//
// File: objsave.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#include "structs.h"
#include "actions.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "spells.h"
#include "screen.h"
#include "vehicle.h"
#include "house.h"
#include "bomb.h"
#include "accstr.h"

/* these factors should be unique integers */
#define RENT_FACTOR         1
#define CRYO_FACTOR         4

extern struct descriptor_data *descriptor_list;
extern struct player_index_element *player_table;
extern struct time_info_data time_info;
extern int top_of_p_table;
extern int min_rent_cost;
extern int no_plrtext;

/* Extern functions */
ACMD(do_tell);
SPECIAL(receptionist);
SPECIAL(cryogenicist);

void
extract_norents(struct obj_data *obj)
{
    if (obj) {
        extract_norents(obj->contains);
        extract_norents(obj->next_content);
        if (obj_is_unrentable(obj) &&
            (!obj->worn_by || !IS_OBJ_STAT2(obj, ITEM2_NOREMOVE)) &&
            !IS_OBJ_STAT(obj, ITEM_NODROP))
            extract_obj(obj);
    }
}

/* ************************************************************************
* Routines used for the receptionist                                          *
************************************************************************* */

void
rent_deadline(struct creature *ch, struct creature *recep, long cost)
{
    long rent_deadline;

    if (!cost)
        return;

    if (ch->in_room->zone->time_frame == TIME_ELECTRO)
        rent_deadline = ((GET_CASH(ch) + GET_FUTURE_BANK(ch)) / cost);
    else
        rent_deadline = ((GET_GOLD(ch) + GET_PAST_BANK(ch)) / cost);

    const char *msg =
        tmp_sprintf
        ("You can rent for %ld day%s with the money you have on hand and in the bank.",
        rent_deadline, (rent_deadline > 1) ? "s" : "");
    if (recep)
        perform_tell(recep, ch, msg);
    else {
        send_to_char(ch, "%s\r\n", msg);
    }
}

void
append_obj_rent(const char *currency_str, int count, struct obj_data *obj)
{
    if (count == 1)
        acc_sprintf("%10u %s for %s\r\n", GET_OBJ_RENT(obj), currency_str,
            obj->name);
    else
        acc_sprintf("%10u %s for %s (x%d)\r\n",
            GET_OBJ_RENT(obj) * count, currency_str, obj->name, count);
}

long
tally_obj_rent(struct obj_data *obj, const char *currency_str, bool display)
{
    struct obj_data *last_obj, *cur_obj;
    long total_cost = 0;
    int count = 0;

    last_obj = cur_obj = obj;
    while (cur_obj) {
        if (!obj_is_unrentable(cur_obj)) {
            total_cost += GET_OBJ_RENT(cur_obj);
            if (last_obj->shared != cur_obj->shared && display) {
                if (display)
                    append_obj_rent(currency_str, count, last_obj);
                count = 1;
                last_obj = cur_obj;
            } else {
                count++;
            }
        }

        if (cur_obj->contains)
            cur_obj = cur_obj->contains;    // descend into obj
        else if (!cur_obj->next_content && cur_obj->in_obj)
            cur_obj = cur_obj->in_obj->next_content;    // ascend out of obj
        else
            cur_obj = cur_obj->next_content;    // go to next obj
    }
    if (last_obj && display)
        append_obj_rent(currency_str, count, last_obj);

    return total_cost;
}

long
calc_daily_rent(struct creature *ch, int factor, char *currency_str,
    bool display)
{
    extern int min_rent_cost;
    struct obj_data *cur_obj;
    int pos;
    long total_cost = 0;
    float f_factor = factor;
    long level_adj;

    if (real_room(GET_LOADROOM(ch)) || ch->in_room) {
        struct room_data *room = NULL;
        if (ch->in_room)
            room = ch->in_room;
        else
            room = real_room(GET_LOADROOM(ch));

        for (GList *it = first_living(room->people);it;it = next_living(it)) {
            struct creature *tch = it->data;

            if (GET_NPC_SPEC(tch) == cryogenicist ||
                GET_NPC_SPEC(tch) == receptionist) {
                f_factor += (f_factor * cost_modifier(ch, tch)) / 100;
            }
        }
    }

    if (GET_LEVEL(ch) >= LVL_AMBASSADOR)
        return 0;

    for (pos = 0; pos < NUM_WEARS; pos++) {
        cur_obj = GET_EQ(ch, pos);
        if (cur_obj)
            total_cost += tally_obj_rent(cur_obj, currency_str, display);
    }

    if (ch->carrying)
        total_cost += tally_obj_rent(ch->carrying, currency_str, display);

    level_adj = (3 * total_cost * (10 + GET_LEVEL(ch))) / 100 +
        min_rent_cost * GET_LEVEL(ch) - total_cost;
    total_cost += level_adj;
    total_cost = (int)((float)total_cost * f_factor);

    if (display) {
        acc_sprintf("%10ld %s for level adjustment\r\n",
            level_adj, currency_str);
        if (f_factor != 1)
            acc_sprintf("        x%.2f for services\r\n", f_factor);
        acc_sprintf("-------------------------------------------\r\n");
        acc_sprintf("%10ld %s TOTAL\r\n", total_cost, currency_str);
    }

    return total_cost;
}

money_t
offer_rent(struct creature *ch, struct creature *receptionist,
    int factor, bool display)
{
    long total_money;
    long cost_per_day;
    char curr[64];

    if (receptionist->in_room->zone->time_frame == TIME_ELECTRO) {
        strcpy(curr, "credits");
        total_money = GET_CASH(ch) + GET_FUTURE_BANK(ch);
    } else {
        strcpy(curr, "coins");
        total_money = GET_GOLD(ch) + GET_PAST_BANK(ch);
    }

    if (display_unrentables(ch))
        return 0;

    if (display) {
        acc_string_clear();
        acc_sprintf("%s writes up a bill and shows it to you:\r\n",
            tmp_capitalize(PERS(receptionist, ch)));
        cost_per_day = calc_daily_rent(ch, factor, curr, true);
        if (factor == RENT_FACTOR) {
            if (total_money < cost_per_day)
                acc_strcat
                    ("You don't have enough money to rent for a single day!\r\n",
                    NULL);
            else if (cost_per_day)
                acc_sprintf
                    ("Your %ld %s is enough to rent for %s%ld%s days.\r\n",
                    total_money, curr, CCCYN(ch, C_NRM),
                    total_money / cost_per_day, CCNRM(ch, C_NRM));
        }
        page_string(ch->desc, acc_get_string());
    } else {
        cost_per_day = calc_daily_rent(ch, factor, curr, false);
    }

    return cost_per_day;
}

int
gen_receptionist(struct creature *ch, struct creature *recep,
    int cmd, char *arg __attribute__ ((unused)), int mode)
{
    int cost = 0;
    extern int free_rent;
    const char *action_table[] = { "smile", "dance", "sigh", "blush", "burp",
        "cough", "fart", "twiddle", "yawn"
    };
    const char *curr;
    const char *msg;

    ACMD(do_action);

    if (recep->in_room->zone->time_frame == TIME_ELECTRO)
        curr = "credits";
    else
        curr = "coins";

    if (!ch->desc || IS_NPC(ch))
        return false;

    if (!cmd && !number(0, 5)) {
        do_action(recep, tmp_strdup(""), find_command(action_table[number(0,
                        8)]), 0);
        return false;
    }

    if (!CMD_IS("offer") && !CMD_IS("rent"))
        return false;

    if (!AWAKE(recep)) {
        act("$E is unable to talk to you...", false, ch, 0, recep, TO_CHAR);
        return true;
    }
    if (!can_see_creature(recep, ch) && GET_LEVEL(ch) <= LVL_AMBASSADOR) {
        act("$n says, 'I don't deal with people I can't see!'", false, recep,
            0, 0, TO_ROOM);
        return true;
    }
    if (free_rent) {
        perform_tell(recep, ch,
            "Rent is free here.  Just quit, and your objects will be saved!");
        return 1;
    }
    if (CMD_IS("rent")) {

        if (!(cost = offer_rent(ch, recep, mode, false)))
            return true;

        if (mode == RENT_FACTOR)
            msg = tmp_sprintf("Rent will cost you %d %s per day.", cost, curr);
        else if (mode == CRYO_FACTOR) {
            msg =
                tmp_sprintf("It will cost you %d %s to be frozen.", cost,
                curr);
        } else
            msg = "Please report this word: Arbaxyl";
        perform_tell(recep, ch, msg);

        if ((recep->in_room->zone->time_frame == TIME_ELECTRO &&
                cost > GET_CASH(ch)) ||
            (recep->in_room->zone->time_frame != TIME_ELECTRO &&
                cost > GET_GOLD(ch))) {
            perform_tell(recep, ch, "...which I see you can't afford.");
            return true;
        }

        if (cost && mode == RENT_FACTOR)
            rent_deadline(ch, recep, cost);

        act("$n helps $N into $S private chamber.",
            false, recep, 0, ch, TO_NOTVICT);

        if (mode == RENT_FACTOR) {
            act("$n stores your belongings and helps you into your private chamber.", false, recep, 0, ch, TO_VICT);
            creature_rent(ch);
        } else {                /* cryo */
            act("$n stores your belongings and helps you into your private chamber.\r\nA white mist appears in the room, chilling you to the bone...\r\nYou begin to lose consciousness...", false, recep, 0, ch, TO_VICT);
            if (recep->in_room->zone->time_frame == TIME_ELECTRO)
                GET_CASH(ch) -= cost;
            else
                GET_GOLD(ch) -= cost;
            creature_cryo(ch);
        }

    } else {
        offer_rent(ch, recep, mode, true);
        act("$N gives $n an offer.", false, ch, 0, recep, TO_ROOM);
    }
    return true;
}

SPECIAL(receptionist)
{
    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
        return 0;
    return (gen_receptionist(ch, (struct creature *)me, cmd, argument,
            RENT_FACTOR));
}

SPECIAL(cryogenicist)
{
    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
        return 0;
    return (gen_receptionist(ch, (struct creature *)me, cmd, argument,
            CRYO_FACTOR));
}
