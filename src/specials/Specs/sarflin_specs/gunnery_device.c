//
// File: gunnery_device.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(gunnery_device)
{
    int hit = 0;
    struct room_data *dest;
    struct obj_data *ob = NULL;
    struct creature *me2 = (struct creature *)me;
    struct creature *vict = NULL;
    if (spec_mode != SPECIAL_CMD) {
        return 0;
    }

    ob = GET_EQ(me2, WEAR_HOLD);
    if (ob == NULL) {
        return (0);
    }
    if (CMD_IS("say") || CMD_IS("'")) {
        short num = 0;
        if (GET_LEVEL(ch) < LVL_IMMORT) {
            return (0);
        }
        skip_spaces(&argument);
        if (!*argument) {
            return 0;
        }
        half_chop(argument, buf, buf2);
        if (strncasecmp(buf, "fire", 4)) {
            return (0);
        }

        num = atoi(buf2);
        GET_OBJ_VAL(ob, 0) = num;
        perform_say(ch, "say", argument);
        perform_say(me2, "say", "Yes, Sir.");
        return (1);
    }
    if (cmd) {
        return (0);
    }
    if (!GET_OBJ_VAL(ob, 0)) {
        return (0);
    }
    dest = real_room(GET_OBJ_VAL(ob, 1));
    if (!dest) {
        return (1);
    }
/*28*/
    switch (GET_OBJ_VAL(ob, 2)) {
    case 1:
    {
        send_to_room("With a Thump the catapults fire.\r\n", ch->in_room);
        send_to_room
            ("Then with a driven effort the crews work to reload.\r\n",
            ch->in_room);
        send_to_room
            ("From above in the tower you hear the Thwack of a catapult fireing.\r\n",
            dest);
        vict = NULL;
        hit = 0;
        for (GList *cit = dest->people; cit; cit = cit->next) {
            vict = cit->data;
            if (!number(0, 5)) {
                damage(ch, vict, NULL, 10, TYPE_CATAPULT, -1);
                hit = 1;
                break;
            }
        }
        if (!hit) {
            send_to_room
                ("A boulder THUNKS into the ground missing you all.\r\n",
                dest);
        }
        break;
    }
    case 2:
    {
        send_to_room("With a mighty TWANG the balista fire.\r\n",
                     ch->in_room);
        send_to_room
            ("Then the Crew start to winch the string back into position.\r\n",
            ch->in_room);
        send_to_room
            ("From above in the tower you hear the TWANG of a balista fireing.\r\n",
            dest);
        vict = NULL;
        hit = 0;
        for (GList *cit = dest->people; cit; cit = cit->next) {
            vict = cit->data;
            if (!number(0, 5)) {
                damage(ch, vict, NULL, 15, TYPE_BALISTA, -1);
                hit = 1;
                break;
            }
        }
        if (!hit) {
            send_to_room
                ("A Balista bolt Thunks into the ground missing you all.\r\n",
                dest);
        }
        break;
    }
    case 3:
    {
        if (number(0, 1)) {
            send_to_room
                ("With a groaning CREAK the pots of oil are tipped.\r\n",
                ch->in_room);
            send_to_room("The the crews begin refilling the pots.\r\n",
                         ch->in_room);
            send_to_room
                ("From above in the tower you hear the creak of a pot tiping\r\n",
                dest);
            vict = NULL;
            for (GList *cit = dest->people; cit; cit = cit->next) {
                damage(ch, cit->data, NULL, 15, TYPE_BOILING_OIL, -1);
            }
        } else {
            send_to_room
                ("With a groaning CREAK the pots of scalding sand are dumped.\r\n",
                ch->in_room);
            send_to_room("The the crews begin refilling the pots.\r\n",
                         ch->in_room);
            send_to_room
                ("From above in the tower you hear the creak of a pot tiping\r\n",
                dest);
            vict = NULL;
            for (GList *cit = dest->people; cit; cit = cit->next) {
                vict = cit->data;
                damage(ch, vict, NULL, 15, TYPE_BOILING_OIL, -1);
            }
        }
        break;
    }
    }
    GET_OBJ_VAL(ob, 0)--;
    return (0);
}
