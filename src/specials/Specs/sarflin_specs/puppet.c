//
// File: puppet.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(puppet)
{
    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
        return 0;
    struct creature *me2 = (struct creature *)me;
    struct affected_type af;

    init_affect(&af);

    if (!cmd && !me2->fighting && !number(0, 3)) {
        switch (number(0, 10)) {
        case 0:
            act("The puppet says: I am now the willing slave of $N.", false,
                ch, 0, me2, TO_ROOM);
            break;
        case 1:
            act("The puppet says: That @^#%* wizard always makeing me touch my toes!", false, ch, 0, me2, TO_ROOM);
            break;
        case 2:
            act("The puppet says: WOW, so this is what the world looks like!",
                false, ch, 0, me2, TO_ROOM);
            break;
        case 3:
            act("The puppet says: You know I have always thought that you don't need to worry about the guy that ", false, ch, 0, me2, TO_ROOM);
            act("pulls a sword from a stone. You need to worry about the guy that put it there! ", false, ch, 0, me2, TO_ROOM);
            break;
        case 4:
            act("The puppet says: I am board, lets go get killed!", false, ch,
                0, me2, TO_ROOM);
            break;
        case 5:
            act("The puppet says: I know that pesky imp is around here some where.", false, ch, 0, me2, TO_ROOM);
            break;
        case 6:
            act("The puppet says: HEY do you owe me gold?", false, ch, 0, me2,
                TO_ROOM);
            break;
        case 7:
            act("The puppet says: GOOOLLLDDD All I need is GOOLLDD!", false,
                ch, 0, me2, TO_ROOM);
            break;
        case 8:
            act("The puppet says: You arn't so tough. I could kill you with ... UMMM one giant!", false, ch, 0, me2, TO_ROOM);
            break;
        case 9:
            act("The puppet says: OHHH I don't feel so good. Must have been someone I ate! ", false, ch, 0, me2, TO_ROOM);
            break;
        case 10:
            {
                if (!number(0, 10)) {
                    act("The puppet turns grey and explodes", false, ch, 0,
                        me2, TO_ROOM);
                    make_corpse(me2, me2, TYPE_CLAW);
                }
                break;
            }

        }
    }
    if (!CMD_IS("say") && !CMD_IS("'"))
        return 0;

    skip_spaces(&argument);
    if (!*argument)
        return 0;
    half_chop(argument, buf, buf2);

    if (!strncasecmp(buf, "simonsez", 8) && !strncasecmp(buf2, "obey me", 7)) {
        if (me2->master)
            stop_follower(me2);
        add_follower(me2, ch);

        af.type = SPELL_CHARM;

        af.duration = 24 * 20;

        af.bitvector = AFF_CHARM;
        af.owner = GET_IDNUM(ch);
        affect_to_char(me2, &af);

    } else {
        send_to_char(ch, "What do you mean by %s %s", buf, buf2);
    }
    return 0;
}
