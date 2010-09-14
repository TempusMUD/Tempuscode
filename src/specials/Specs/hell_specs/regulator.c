//
// File: regulator.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define REG_SPINED_VNUM 16809

SPECIAL(hell_regulator)
{
	struct creature *vict = NULL, *devil = NULL;
    int dam_retval = 0;
	if (spec_mode != SPECIAL_TICK)
		return 0;

	if (cmd)
		return 0;

	if (GET_MANA(ch) < 100) {
		if (!ZONE_IS_HELL(ch->in_room->zone)) {
			act("$n vanishes into the mouth of an interplanar conduit.",
				false, ch, 0, 0, TO_ROOM);
			creature_purge(ch, true);
			return 1;
		}
		return 0;
	}

    for (GList *cit = ch->in_room->people;cit;cit = cit->next) {
            vict = cit->data;
		if (vict == ch)
			continue;

		// REGULATOR doesn't want anyone attacking him
		if (g_list_find(vict->fighting, ch)) {

			if (!(devil = read_mobile(REG_SPINED_VNUM))) {
				errlog("REGULATOR failed to load REG_SPINED_VNUM for defense.");
				// set mana to zero so he will go away on the next loop
				GET_MANA(ch) = 0;
				return 1;
			}

			char_to_room(devil, ch->in_room, false);
			act("$n gestures... A glowing conduit flashes into existence!",
				false, ch, 0, vict, TO_ROOM);
			act("...$n leaps out and attacks $N!", false, devil, 0, vict,
				TO_NOTVICT);
			act("...$n leaps out and attacks you!", false, devil, 0, vict,
				TO_VICT);

            remove_combat(vict, ch);
			dam_retval = hit(devil, vict, TYPE_UNDEFINED);
            if (dam_retval != DAM_VICT_KILLED)
			    WAIT_STATE(vict, 1 RL_SEC);

			return 1;
		}

		if (IS_DEVIL(vict) && IS_NPC(vict) &&
			GET_HIT(vict) < (GET_MAX_HIT(vict) - 500)) {

			act("$n opens a conduit of streaming energy to $N!\r\n"
				"...$N's wounds appear to regenerate!", false, ch, 0, vict,
				TO_ROOM);

			GET_HIT(vict) = MIN(GET_MAX_HIT(vict), GET_HIT(vict) + 500);
			GET_MANA(ch) -= 100;
			return 1;
		}

	}

	return 0;
}

#undef REG_SPINED_VNUM
