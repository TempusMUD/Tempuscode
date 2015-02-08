//
// File: finger_of_death.spec   -- Part of TempusMUD
//
// Copyright 2002 by John D. Rothe, all rights reserved.
//
SPECIAL(finger_of_death)
{
    if (spec_mode != SPECIAL_CMD) {
        return 0;
    }
    struct obj_data *finger = (struct obj_data *)me;
    char *token;

    if (!CMD_IS("activate")) {
        return 0;
    }

    token = tmp_getword(&argument);
    if (!token) {
        return 0;
    }

    if (!isname(token, finger->aliases)) {
        return 0;
    }

    token = tmp_getword(&argument);
    if (!token) {
        send_to_char(ch, "%s has %d charges remaining.\r\n",
                     finger->name, GET_OBJ_VAL(finger, 0));
        send_to_char(ch, "Usage: 'activate %s <mobile name>'\r\n",
                     finger->name);
        return 1;
    } else if (GET_OBJ_VAL(finger, 0) <= 0) {
        send_to_char(ch, "%s is powerless.\r\n", finger->name);
        return 1;
    }

    struct creature *target = get_char_room_vis(ch, token);
    if (target == NULL) {
        send_to_char(ch, "There doesn't seem to be a '%s' here.\r\n", token);
    } else if (IS_PC(target)) {
        act("$n gives you the finger.", true, ch, finger, target, TO_VICT);
        act("You give $N the finger.", true, ch, finger, target, TO_CHAR);
    } else {
        act("$n disintegrates $N with $p.", false, ch, finger, target,
            TO_NOTVICT);
        act("$n disintegrates you with $p.", false, ch, finger, target,
            TO_VICT);
        act("You give $N the finger, destroying it utterly.", true, ch, finger,
            target, TO_CHAR);
        mudlog(0, BRF, true, "(f0d) %s has purged %s with %s at %d",
               GET_NAME(ch), GET_NAME(target), finger->name,
               target->in_room->number);
        creature_purge(target, true);
        GET_OBJ_VAL(finger, 0) -= 1;
    }
    return 1;
}
