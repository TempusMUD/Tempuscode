//
// File: ressurector.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define RESS_IS_DEVIL_VNUM(vnum) ( vnum >= 16100 && vnum <= 17199 )

SPECIAL(hell_ressurector)
{

	struct obj_data *corpse = NULL, *obj = NULL;
	struct Creature *vict = NULL;
	if (spec_mode != SPECIAL_TICK)
		return 0;

	if (cmd || GET_MANA(ch) < 200)
		return 0;

	//
	// check the room for corpses
	//

	for (corpse = ch->in_room->contents; corpse; corpse = corpse->next_content) {

		if (!IS_CORPSE(corpse) || !RESS_IS_DEVIL_VNUM(-CORPSE_IDNUM(corpse))) {
			continue;
		}

		vict = real_mobile_proto(-CORPSE_IDNUM(corpse));

		if (!vict) {
			errlog("hell resurrector unable to real_mobile_proto(%d).",
				-CORPSE_IDNUM(corpse));
			continue;
		}
		//
		// remove stray corpses to prevent bogusness
		//

		if (!IS_DEVIL(vict)) {
			act("$n disintegrates $p in a flash of light!", FALSE, ch, corpse,
				NULL, TO_ROOM);
			extract_obj(corpse);
			GET_MANA(ch) =
				MIN(GET_MAX_MANA(ch), GET_MANA(ch) + (GET_LEVEL(vict) << 1));
			return 1;
		}
		//
		// ressurect the corpse by loading a new mobile
		//

		if (!(vict = read_mobile(-CORPSE_IDNUM(corpse)))) {
			errlog(" hell ressurector unable to read_mobile(%d).",
				-CORPSE_IDNUM(corpse));
			return 0;
		}

		GET_MANA(ch) -= 150;

		//
		// put the vict (ressurected mob) in the room to be safe
		//

		char_to_room(vict, ch->in_room, false);

		//
		// transfer EQ from corpse to ressurected body
		//

		while ((obj = corpse->contains)) {
			obj_from_obj(obj);
			obj_to_char(obj, vict);
		}

		//
		// messages
		//

		act("$n slams $s hands together, accompanied by a deafening thunderclap!\n" " ... An aura of flame appears around $p...\n" " ... You watch in terror as $N slowly rises from the dead.", FALSE, ch, corpse, vict, TO_ROOM);

		//
		// get rid of the corpse
		//

		extract_obj(corpse);

		return 1;
	}

	//
	// fall through the loop and out
	//

	return 0;
}



#undef RESS_IS_DEVIL_VNUM
