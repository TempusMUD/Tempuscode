//
// File: abandoned_cavern.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define RUBBLE_VNUM 16151

SPECIAL(abandoned_cavern)
{

    struct room_data *cavern = (struct room_data *)me;
    struct creature *vict = NULL;
    struct obj_data *obj = NULL, *rubble = NULL;
    static int hell_VI_count = 0, darth_count = 0;
    int count, i;

    if (spec_mode != SPECIAL_CMD) {
        return 0;
    }

    if (GET_LEVEL(ch) >= LVL_IMMORT || AFF_FLAGGED(ch, AFF_SNEAK)) {
        return 0;
    }

    if (cavern->zone->number == 166) {
        if ((count = ++hell_VI_count) > 30) {
            hell_VI_count = 0;
        }
    } else {
        if ((count = ++darth_count) > 30) {
            darth_count = 0;
        }
    }

    if (count > 30) {
        count = 0;
        act("The cavern begins to shake, and rocks start falling from the ceiling!", false, ch, NULL, NULL, TO_ROOM);
        act("The cavern begins to shake, and rocks start falling from the ceiling!", false, ch, NULL, NULL, TO_CHAR | TO_SLEEP);

        for (GList *cit = cavern->people; cit; cit = cit->next) {
            vict = cit->data;
            if (number(5, 30) > GET_DEX(vict) && GET_LEVEL(vict) < LVL_IMMORT) {
                act("A shower of rubble crushes $n!", false, vict, NULL, NULL,
                    TO_ROOM);
                act("A shower of rubble buries you alive!\r\n"
                    "You die a horrible death!!", false, vict, NULL, NULL,
                    TO_CHAR | TO_SLEEP);

                mudlog(GET_INVIS_LVL(vict), BRF, true,
                       "%s killed in a cave-in at %d",
                       GET_NAME(vict), cavern->number);

                if ((rubble = read_object(RUBBLE_VNUM))) {
                    for (i = 0; i < NUM_WEARS; i++) {
                        obj = GET_EQ(vict, i);
                        if (obj) {
                            unequip_char(vict, i, EQUIP_WORN);
                            obj_to_obj(obj, rubble);
                            damage_eq(NULL, obj, dice(10, 40), -1);
                        }
                        obj = GET_IMPLANT(vict, i);
                        if (obj) {
                            unequip_char(vict, i, EQUIP_IMPLANT);
                            obj_to_obj(obj, rubble);
                            damage_eq(NULL, obj, dice(10, 60), -1);
                        }
                    }
                    while ((obj = vict->carrying)) {
                        obj_from_char(obj);
                        obj_to_obj(obj, rubble);
                    }
                    obj_to_room(rubble, cavern);
                }
                creature_die(vict);
            }
        }

        for (i = 0; i < NUM_DIRS; i++) {
            if (cavern->dir_option[i] && cavern->dir_option[i]->to_room) {
                snprintf(buf, sizeof(buf), "You hear the sound of a cave-in from %s!\r\n",
                         from_dirs[i]);
                send_to_room(buf, cavern->dir_option[i]->to_room);
            }
        }

        return 1;
    }

    if (count > 20 && !number(0, 4)) {
        send_to_room("Large rocks begin to fall from the ceiling.\r\n",
                     cavern);
    } else if (count > 10 && !number(0, 9)) {
        send_to_room("Small rocks begin to fall from the ceiling.\r\n",
                     cavern);
    } else if (count > number(0, 15) && count < 10) {
        send_to_room("You notice a faint tremor.\r\n", cavern);
    } else if (!number(0, 15)) {
        send_to_room
            ("You see the glimmer of gold from deep within the cavern.\r\n",
            cavern);
    } else if (!number(0, 15)) {
        send_to_room("You see some treasure shining deep in the cavern.\r\n",
                     cavern);
    } else if (!number(0, 14)) {
        send_to_room
            ("You hear a lady calling for help deeper in the cavern.\r\n",
            cavern);
    } else if (!number(0, 14)) {
        send_to_room
            ("You notice sunlight coming from somewhere deeper in the cavern.\r\n",
            cavern);
    }

    return 0;

}
