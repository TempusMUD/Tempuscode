//
// File: fountain_youth.spec
//
// Copyright 2001 by Daniel Carruth
//
// Part of TempusMUD:
// Copyright 1998 by John Watson, all rights reserved
//
/***************************************************/

SPECIAL(fountain_youth)
{
    /* applies the affects of youth to anyone
       drinking from the fountain */
    /* Obj Vnum is 23315 */

    struct affected_type af, af1, af2, af3;
    struct obj_data *fountain = (struct obj_data *)me;

    init_affect(&af);
    init_affect(&af1);
    init_affect(&af2);
    init_affect(&af3);

    if (spec_mode != SPECIAL_CMD || !CMD_IS("drink")) {
        return 0;
    }

    if (GET_POSITION(ch) == POS_SLEEPING) {
        send_to_char(ch, "You'll have to wake up first!\r\n");
        return 1;
    }

    /* check here for argument */
    skip_spaces(&argument);
    if (!isname(argument, fountain->aliases)) {
        return 0;
    }

    if (affected_by_spell(ch, SPELL_YOUTH)) {
        act("You drink from $p.", true, ch, fountain, NULL, TO_CHAR);
        act("$n drinks from $p.", true, ch, fountain, NULL, TO_ROOM);
        return 1;
    }

    af.type = SPELL_YOUTH;
    af.duration = 60;
    af.location = APPLY_AGE;
    af.modifier = -(GET_AGE(ch) / 4);
    af.level = 1;
    af.is_instant = false;
    af.owner = GET_IDNUM(ch);
    affect_to_char(ch, &af);

    af1.type = SPELL_YOUTH;
    af1.duration = 60;
    af1.location = APPLY_CON;
    af1.modifier = 3;
    af1.level = 1;
    af1.is_instant = false;
    af1.owner = GET_IDNUM(ch);
    affect_to_char(ch, &af1);

    af2.type = SPELL_YOUTH;
    af2.duration = 60;
    af2.location = APPLY_STR;
    af2.modifier = 2;
    af2.level = 1;
    af2.is_instant = false;
    af2.owner = GET_IDNUM(ch);
    affect_to_char(ch, &af2);

    af3.type = SPELL_YOUTH;
    af3.duration = 60;
    af3.location = APPLY_WIS;
    af3.modifier = -4;
    af3.level = 1;
    af3.is_instant = false;
    af3.owner = GET_IDNUM(ch);
    affect_to_char(ch, &af3);

    act("As you drink from the pool, your reflection becomes visibly younger!",
        true, ch, NULL, NULL, TO_CHAR);
    act("As $n drinks from the pool, $e becomes visibly younger!",
        true, ch, NULL, NULL, TO_ROOM);
    return 1;
}
