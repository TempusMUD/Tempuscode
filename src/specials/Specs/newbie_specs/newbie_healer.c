//
// File: newbie_healer.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(newbie_healer)
{
    ACMD(do_drop);
    struct creature *i;

    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK) {
        return 0;
    }
    if (cmd) {
        return 0;
    }

    for (GList *it = first_living(ch->in_room->people); it; it = next_living(it)) {
        i = it->data;
        if (i == ch) {
            continue;
        }
        if (IS_NPC(i)) {
            act("$n banishes $N!", false, ch, NULL, i, TO_ROOM);
            creature_purge(i, false);
            return 1;
        }
        if (!IS_NPC(i) && GET_LEVEL(i) < 5 && !number(0, GET_LEVEL(i))) {
            if (GET_HIT(i) < GET_MAX_HIT(i)) {
                cast_spell(ch, i, NULL, NULL, SPELL_CURE_CRITIC);
            } else if (AFF_FLAGGED(i, AFF_POISON)) {
                cast_spell(ch, i, NULL, NULL, SPELL_REMOVE_POISON);
            } else if (!affected_by_spell(i, SPELL_BLESS)) {
                cast_spell(ch, i, NULL, NULL, SPELL_BLESS);
            } else if (!affected_by_spell(i, SPELL_ARMOR)) {
                cast_spell(ch, i, NULL, NULL, SPELL_ARMOR);
            } else if (!affected_by_spell(i, SPELL_DETECT_MAGIC)) {
                cast_spell(ch, i, NULL, NULL, SPELL_DETECT_MAGIC);
            } else {
                return 0;
            }
            return 1;
        }
    }

    struct obj_data *p = ch->carrying;
    if (p != NULL) {
        act("$p.", false, ch, p, NULL, TO_CHAR);
        if (IS_OBJ_TYPE(p, ITEM_WORN)) {
            cast_spell(ch, NULL, p, NULL, SPELL_MAGICAL_VESTMENT);
        } else {
            send_to_char(ch, "No WEAR.\r\n");
        }
        do_drop(ch, fname(p->aliases), 0, 0);
        return 1;
    }
    return 0;
}
