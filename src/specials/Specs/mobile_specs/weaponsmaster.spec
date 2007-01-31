//
// File: weaponsmaster.spec                                      -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

extern weap_spec_info weap_spec_char_class[];
#define weap_spec GET_WEAP_SPEC(ch, i)

SPECIAL(weaponsmaster)
{
	struct Creature *master = (struct Creature *)me;
	struct obj_data *weap = NULL;
	int pos, cost, i, char_class, check_only = 0;

	if (spec_mode != SPECIAL_CMD)
		return FALSE;

	if (IS_NPC(ch))
		return 0;

	if (CMD_IS("offer"))
		check_only = 1;
	else if (!CMD_IS("train"))
		return 0;

	skip_spaces(&argument);

	if (!*argument) {
		return 1;
	}
	for (pos = 0; pos < NUM_WEARS && !weap; pos++) {
		weap = GET_EQ(ch, pos);
		if (weap && 
            (GET_OBJ_TYPE(weap) == ITEM_WEAPON || GET_OBJ_TYPE(weap) == ITEM_ENERGY_GUN)
            && isname(argument, weap->aliases))
			break;

		weap = GET_IMPLANT(ch, pos);
		if (weap && GET_OBJ_TYPE(weap) == ITEM_WEAPON &&
			isname(argument, weap->aliases))
			break;

		weap = NULL;
	}

	if (!weap) {
		send_to_char(ch,
			"You must be wielding or wearing the weapon you want to specialize in.\r\n");
		return 1;
	}

	if (weap->worn_on == WEAR_HANDS && GET_CLASS(ch) != CLASS_MONK) {
		return 1;
	}
	if (GET_OBJ_VNUM(weap) < 0 || invalid_char_class(ch, weap)) {
		send_to_char(ch, "You can't specialize with that.\r\n");
		return 1;
	}
	for (i = 0; i < MAX_WEAPON_SPEC; i++) {
		if (!weap_spec.vnum ||
			!real_object_proto(weap_spec.vnum) ||
			weap_spec.vnum == GET_OBJ_VNUM(weap))
			break;
	}

	if ((char_class = GET_CLASS(ch)) >= NUM_CLASSES)
		char_class = CLASS_WARRIOR;
	if (IS_REMORT(ch) && GET_REMORT_CLASS(ch) < NUM_CLASSES) {
		if (weap_spec_char_class[char_class].max <
			weap_spec_char_class[GET_REMORT_CLASS(ch)].max)
			char_class = GET_REMORT_CLASS(ch);
	}

	if (i >= weap_spec_char_class[char_class].max) {
		send_to_char(ch,
			"The %s char_class can only specialize in %d weapons.\r\n",
			pc_char_class_types[char_class],
			weap_spec_char_class[char_class].max);
		return 1;
	}

	if (weap_spec.level >= 5) {
		send_to_char(ch,
			"You have maxed out specialization in this weapon.\r\n");
		return 1;
	}

	cost = (weap_spec.level + 1) * 300000;
    cost += (cost*ch->getCostModifier(master))/100;
    
	send_to_char(ch,
		"It will cost you %d gold coin%s to train your specialization with %s to level %d.\r\n%s",
		cost, cost == 1 ? "" : "s", weap->name,
		weap_spec.level + 1,
		cost > GET_GOLD(ch) ? "Which you don't have.\r\n" : "");

	if (check_only || cost > GET_GOLD(ch))
		return 1;

	GET_GOLD(ch) -= cost;
	weap_spec.vnum = GET_OBJ_VNUM(weap);
	weap_spec.level++;
	act("You improve your fighting aptitude with $p.", FALSE, ch, weap, 0,
		TO_CHAR);
	act("$n improves $s fighting aptitude with $p.", FALSE, ch, weap, 0,
		TO_ROOM);
	ch->saveToXML();
	return 1;
}

#undef weap_spec
