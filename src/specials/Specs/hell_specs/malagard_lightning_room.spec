// room special for the battlement of malagard

SPECIAL(malagard_lightning_room)
{
	struct Creature *vict = 0;
	int retval = 0;

	if (spec_mode != SPECIAL_TICK && spec_mode != SPECIAL_ENTER)
		return 0;

	if (IS_NPC(ch) || number(0, 4))
		return 0;


	vict = ch->in_room->people;

	if (vict == NULL || IS_NPC(vict))
		vict = ch;

	if (mag_savingthrow(vict, 50, SAVING_ROD)) {
		act("A bolt of lightning strikes nearby!", FALSE, vict, 0, 0, TO_CHAR);
		act("A bolt of lightning strikes nearby!", FALSE, vict, 0, 0, TO_ROOM);
	} else {
		act("A bolt of lightning blasts down from above and hits you!", FALSE,
			vict, 0, 0, TO_CHAR);
		act("A bolt of lightning blasts down from above and hits $n!", FALSE,
			vict, 0, 0, TO_ROOM);
		retval =
			damage(NULL, vict, dice(20, 20), SPELL_LIGHTNING_BOLT, WEAR_HEAD);
		return (ch == vict ? retval : 0);
	}

	return 0;
}
