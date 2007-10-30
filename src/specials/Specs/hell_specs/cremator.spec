SPECIAL(cremator)
{
	if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
		return 0;
	if (cmd || !ch->isFighting())
		return 0;

    Creature *vict = ch->findRandomCombat();
	switch (number(0, 5)) {
	case 0:
		act("$n attempts to throw you into the furnace!", FALSE, ch, 0,
			vict, TO_VICT);
		act("$n attempts to throw $N into the furnace!", FALSE, ch, 0,
			vict, TO_NOTVICT);
		break;
	case 1:
		act("$n lifts you and almosts hurls you into the furnace!", FALSE, ch,
			0, vict, TO_VICT);
		act("$n lifts $N and almosts hurls $M into the furnace!", FALSE, ch, 0,
			vict, TO_NOTVICT);
		break;
	case 2:
		act("$n shoves you toward the blazing furnace!", FALSE, ch, 0,
			vict, TO_VICT);
		act("$n shoves $N toward the blazing furnace!", FALSE, ch, 0,
			vict, TO_NOTVICT);
		break;
	default:
		return 0;
	}

	return 1;
}
