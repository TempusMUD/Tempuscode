//
// File: weaponsmaster.spec										 -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

extern struct {double multiplier;int max;} weap_spec_char_class[];
#define weap_spec GET_WEAP_SPEC(ch, i)	
SPECIAL(weaponsmaster)
{
	struct obj_data *weap;
	int cost, i, char_class, check_only = 0;
	int found = 0;

    if (spec_mode == SPECIAL_DEATH) return FALSE;

	if (IS_NPC(ch))
		return 0;

	if (CMD_IS("offer"))
		check_only = 1;
	else if (!CMD_IS("train"))
		return 0;

	skip_spaces(&argument);
	
	if (!*argument) {
		send_to_char("To see how much specialization will cost, type 'offer <weapon name>'.\r\n",ch);
		send_to_char("To specialize in a weapon, type 'train <weapon name>'.\r\n",ch);
		return 1;
	}
    weap  = GET_EQ(ch, WEAR_WIELD);
    if(weap && isname(argument, weap->name))
        found = 1;
    if(!found )
        weap = GET_EQ(ch, WEAR_HANDS);
    if(weap && isname(argument, weap->name))
        found = 1;
    if(!found) {
        send_to_char("You must be wielding or wearing the weapon you want to specialize in.\r\n", ch);
        return 1;
    }
    if( weap->worn_on == WEAR_HANDS && GET_CLASS(ch) != CLASS_MONK) {
        send_to_char("Only monks can train a weapon of this type.\r\n",ch);
        return 1;
    }
	if (GET_OBJ_VNUM(weap) < 0 || invalid_char_class(ch, weap)) {
		send_to_char("You can't specialize with that.\r\n", ch);
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
		if (weap_spec_char_class[char_class].max < weap_spec_char_class[GET_REMORT_CLASS(ch)].max)
			char_class = GET_REMORT_CLASS(ch);
	}
	
	if (i >= weap_spec_char_class[char_class].max) {
		sprintf(buf, "The %s char_class can only specialize in %d weapons.\r\n", 
			pc_char_class_types[char_class], weap_spec_char_class[char_class].max);
		send_to_char(buf, ch);
		return 1;
	}

	if (weap_spec.level >= 5) {
		send_to_char("You have maxed out specialization in this weapon.\r\n", ch);
		return 1;
	}
		 
	cost = (int) ( weap_spec_char_class[char_class].multiplier * (weap_spec.level + 1) );
	
	sprintf(buf, "It will cost you %d practice%s to train your specialization with %s to level %d.\r\n%s", cost, cost == 1 ? "" : "s", 
		weap->short_description, weap_spec.level+1,
		cost > GET_PRACTICES(ch) ? "Which you don't have.\r\n" : "");
	send_to_char(buf, ch);
	
	if (check_only || cost > GET_PRACTICES(ch))
		return 1;
	
	GET_PRACTICES(ch) -= cost;
	weap_spec.vnum = GET_OBJ_VNUM(weap);
	weap_spec.level++;
	act("You improve your fighting aptitude with $p.",FALSE,ch,weap,0,TO_CHAR);
	act("$n improves $s fighting aptitude with $p.",FALSE,ch,weap,0,TO_ROOM);
	save_char(ch, NULL);
	return 1;
}
	
#undef weap_spec

