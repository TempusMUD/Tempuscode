//
// File: geryon.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(geryon)
{
    struct creature *vict = NULL;
    struct obj_data *horn = NULL;
    ACMD(do_order);

    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
        return 0;
    if (cmd || !is_fighting(ch))
        return 0;

    if ((!ch->followers || !ch->followers->next)
        && (horn = GET_EQ(ch, WEAR_HOLD)) && GET_OBJ_VNUM(horn) == 16144
        && GET_OBJ_VAL(horn, 0)) {
        command_interpreter(ch, tmp_strdup("wind horn"));
        do_order(ch, tmp_strdup("minotaur assist geryon"), 0, 0);
        return 1;
    } else if (number(0, 2))
        return 0;
    for (GList * cit = ch->in_room->people; cit; cit = cit->next) {
        struct creature *tch = cit->data;
        if (g_list_find(tch->fighting, ch) &&
            !number(0, 4) && !affected_by_spell(tch, SPELL_POISON)) {
            vict = tch;
            break;
        }
    }

    if (!vict || !number(0, 3) || vict == ch)
        vict = random_opponent(ch);

    act("$n stings you with a mighty lash of $s deadly tail!", false, ch, 0,
        vict, TO_VICT);
    act("$n stings $N with a mighty lash of $s deadly tail!", false, ch, 0,
        vict, TO_NOTVICT);
    GET_HIT(vict) -= dice(2, 6);
    call_magic(ch, vict, 0, NULL, SPELL_POISON, GET_LEVEL(ch), CAST_POTION);
    return 1;
}
