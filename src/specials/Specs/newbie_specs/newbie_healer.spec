//
// File: newbie_healer.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(newbie_healer)
{
	ACCMD(do_drop);
	struct Creature *i;
	struct obj_data *p;

	if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
		return 0;
	if (cmd)
		return 0;

	CreatureList::iterator it = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end(); ++it) {
		i = *it;
		if (i == ch)
			continue;
		if (IS_NPC(i)) {
			act("$n banishes $N!", FALSE, ch, 0, i, TO_ROOM);
			i->extract(true, false, CXN_MENU);
			continue;
		}
		if (!IS_NPC(i) && GET_LEVEL(i) < 5 && !number(0, GET_LEVEL(i))) {
			if (GET_HIT(i) < GET_MAX_HIT(i))
				cast_spell(ch, i, 0, SPELL_CURE_CRITIC);
			else if (IS_AFFECTED(i, AFF_POISON))
				cast_spell(ch, i, 0, SPELL_REMOVE_POISON);
			else if (!affected_by_spell(i, SPELL_BLESS))
				cast_spell(ch, i, 0, SPELL_BLESS);
			else if (!affected_by_spell(i, SPELL_ARMOR))
				cast_spell(ch, i, 0, SPELL_ARMOR);
			else if (!affected_by_spell(i, SPELL_DETECT_MAGIC))
				cast_spell(ch, i, 0, SPELL_DETECT_MAGIC);
			else
				return 0;
			return 1;
		}
	}
	for (p = ch->carrying; p; p = p->next_content) {
		act("$p.", FALSE, ch, p, 0, TO_CHAR);
		if (GET_OBJ_TYPE(p) == ITEM_WORN)
			cast_spell(ch, 0, p, SPELL_MAGICAL_VESTMENT);
		else
			send_to_char(ch, "No WEAR.\r\n");
		do_drop(ch, fname(p->name), 0, 0, 0);
		return 1;
	}
	return 0;
}
