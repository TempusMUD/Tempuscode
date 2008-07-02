//
// File: christmas_quest.cc                     -- Part of TempusMUD
//
// Copyright 2005 by Lenon Kitchens, all rights reserved.
//

#include <vector>
#include <algorithm>

using namespace std;

int do_remort(struct Creature *ch);

SPECIAL(christmas_quest)
{
    ACMD(do_echo);
    ACMD(do_advance);
    static int grinch_timer = 0;
    bool topZoneComparison(const struct zone_data* a, 
            const struct zone_data* b);

    extern int top_of_zone_table;

	Creature *grinch = (Creature *)me;

	if (spec_mode == SPECIAL_CMD) {
        if (!cmd)
            return 0;

        if (CMD_IS("reset") && GET_LEVEL(ch) == 72) {
            sql_exec("delete from christmas_quest");
            send_to_char(ch, "Database reset!\r\n");
            return 1;
        }
        if (CMD_IS("reward") && GET_LEVEL(ch) < LVL_IMMORT) {

            PGresult *res = sql_query("select idnum from christmas_quest where "
                    "idnum=%ld", GET_IDNUM(ch));
            long count = PQntuples(res);
            if (count) {
                // They've already gotten their reward.  Tell them to
                // fuck off.
                perform_say_to(grinch, ch, "I remember you!  I already "
                               "rewarded you once!  Leave before I go mad!");
                return 1;
            }

            perform_say_to(grinch, ch, "Ah, you have found me!  Well done, "
                           "well done indeed!");
            perform_say_to(grinch, ch, "As a reward for your diligence, I "
                           "will give you a gift of power beyond your "
                           "wildest dreams!");
            do_echo(grinch, tmp_strdup("grins and a blinding flash of light engulfs the room."),
                    0, SCMD_EMOTE, NULL);
            ch->setPosition(POS_RESTING);

            if (GET_LEVEL(ch) < 49) {
                do_advance(grinch, tmp_sprintf("%s 49", GET_NAME(ch)), 
                        0, 0, NULL);
                ch->saveToXML();
            }
            else if (GET_REMORT_GEN(ch) == 10) {
                GET_MAX_HIT(ch) += 100;
                GET_MAX_MOVE(ch) += 100;
                GET_MAX_MANA(ch) += 100;

                ch->saveToXML();
                send_to_char(ch, "You feel slightly stronger...\r\n");
            }
            else {
                struct obj_data *cont = read_object(92039);
                if (!cont) {
                    mlog(Security::CODER, LVL_GRIMP, NRM, false, 
                            "Couldn't load sack from vnum 92039!");
                    return 1;
                }
                struct obj_data *obj, *next_obj;

                for (obj = ch->carrying; obj; obj = next_obj) {
                    next_obj = obj->next_content;
                    obj_from_char(obj);
                    obj_to_obj(obj, cont, false);
                    obj = next_obj;
                }

                for (int i = 0; i < NUM_WEARS; i++) {
                    if (GET_EQ(ch, i)) {
                        // This may be redundant but I want to make sure
                        // all is as it should be
                        obj = unequip_char(ch, i, EQUIP_WORN, true);
                        obj_to_char(obj, ch, false);
                        obj_from_char(obj);
                        obj_to_obj(obj, cont, false);
                    }
                }

                while (ch->affected)
                    affect_remove(ch, ch->affected);

                do_remort(ch);

                obj_to_char(cont, ch);
                send_to_char(ch, "You feel very strange...\r\n");
                ch->saveToXML();

                ch->remort();
            }

            grinch_timer = 0;
            sql_exec("insert into christmas_quest (idnum) values(%ld)",
                    GET_IDNUM(ch));

            return 1;
        }
        else if (CMD_IS("reward") && GET_LEVEL(ch) >= LVL_IMMORT) {
            send_to_char(ch, "What the hell would you need a reward for? "
                    "Dumbass...\r\n");
            return 1;
        }

        return 0;
    }
    else if (spec_mode == SPECIAL_TICK) {
        if (grinch->in_room)
            grinch->in_room->zone->idle_time = 0;

        if (grinch->isFighting()) {
            grinch_timer = 0;
            GET_HIT(grinch) = GET_MAX_HIT(grinch);
            return 0;
        }

        if (!grinch->in_room)
            return 0;

        if (grinch_timer > 0) {
            grinch_timer -= 10;
            return 1;
        }

        grinch_timer = 60 * dice(30, 6);

        // Move him to one of the least used zones
        vector<struct zone_data *> zone_list;
        struct zone_data *zone, *dest_zone;
        struct room_data *dest;
        int num_zones, zone_index;

        zone = zone_table;
        for (int i = 0; i < top_of_zone_table; i++, zone = zone->next) {
            // Skip zones that aren't completely approved
            if (IS_SET(zone->flags, ZONE_MOBS_APPROVED) || 
                IS_SET(zone->flags, ZONE_OBJS_APPROVED) || 
                IS_SET(zone->flags, ZONE_ROOMS_APPROVED) ||
                IS_SET(zone->flags, ZONE_ZCMDS_APPROVED) || 
                IS_SET(zone->flags, ZONE_SEARCH_APPROVED) ||
                IS_SET(zone->flags, ZONE_SHOPS_APPROVED))
                continue;

            if (zone->number < 700)
                zone_list.push_back(zone);
        }
        
        sort(zone_list.begin(), zone_list.end(), topZoneComparison);
        num_zones = 50;

        // Which zone? (index)
        zone_index = number(0, num_zones - 1); 
        if (!number(0, 20)) {
            // One in 20 chance to hit a most used zone instead of 
            // least used zone;
            dest_zone = zone_list[zone_index]; 
        }
        else {
            dest_zone = zone_list[(zone_list.size() - 1) - zone_index];
        }
        
        if (!(dest = real_room(dest_zone->number * 100))) {
            // We missed this time.  Try again...
            grinch_timer = 0;
            return 0;
        }

        mlog(Security::CODER, LVL_GRIMP, NRM, false, "GRINCH: Moving to %d. "
                "Timer reset to %d", dest->number, grinch_timer);

        act("$n disappears up a nearby chimney!", false, grinch, 0, 0, TO_ROOM);
        char_from_room(grinch, false);
        char_to_room(grinch, dest, false);
        grinch->in_room->zone->idle_time = 0;
        act("$n falls out of a nearby chimney!", false, grinch, 0, 0, TO_ROOM);

        return 1;
    }
    else {
        return 0;
    }

    return 0;
}

int do_remort(struct Creature *ch)
{
    int i;

    void do_start(struct Creature *ch, int mode);
    // Wipe thier skills
    for (i = 1; i <= MAX_SKILLS; i++)
        SET_SKILL(ch, i, 0);

    do_start(ch, FALSE);

    REMOVE_BIT(PRF_FLAGS(ch),
        PRF_NOPROJECT | PRF_ROOMFLAGS | PRF_HOLYLIGHT | PRF_NOHASSLE |
        PRF_LOG1 | PRF_LOG2 | PRF_NOWIZ);

    REMOVE_BIT(PLR_FLAGS(ch), PLR_HALT | PLR_INVSTART |  PLR_MORTALIZED |
        PLR_OLCGOD);

    GET_INVIS_LVL(ch) = 0;
    GET_COND(ch, DRUNK) = 0;
    GET_COND(ch, FULL) = 0;
    GET_COND(ch, THIRST) = 0;

    // Give em another gen
    if (GET_REMORT_GEN(ch) > 10)
        ch->account->set_quest_points(ch->account->get_quest_points() + 1);
    else
        GET_REMORT_GEN(ch)++;

    // At gen 1 they enter the world of pk, like it or not
    if (GET_REMORT_GEN(ch) >= 1 && GET_REPUTATION(ch) <= 0)
        ch->gain_reputation(5);

    // Whack thier remort invis
    GET_WIMP_LEV(ch) = 0;   // wimpy
    GET_TOT_DAM(ch) = 0;    // cyborg damage

    // Tell everyone that they remorted
    char *msg = tmp_sprintf( "%s completed gen %d with the help of the Grinch!",
    GET_NAME(ch), GET_REMORT_GEN(ch));
    mudlog(LVL_IMMORT, BRF, false,msg);

    return 1;
}
