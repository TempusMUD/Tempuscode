//
// File: cyber_cock.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(cyber_cock)
{
	if (cmd)
		return 0;

	if (!ch->numCombatants()) {
		switch (number(0, 40)) {
		case 0:
			act("$n scratches the ground with $s metal claw.", TRUE, ch, 0, 0,
				TO_ROOM);
			send_to_char(ch, "You scratch.\r\n");
			return 1;
		case 1:
			act("$n emits a loud squealing sound!", FALSE, ch, 0, 0, TO_ROOM);
			send_to_char(ch, "You squawk!\r\n");
			return 1;
		case 2:
			act("$n struts around proudly.", TRUE, ch, 0, 0, TO_ROOM);
			send_to_char(ch, "You strut.\r\n");
			return 1;
		default:
			return 0;
		}
	}
    Creature *vict = ch->findRandomCombat();
	switch (number(0, 16)) {
	case 0:
		act("$n leaps into the air, stubby chrome wings flapping!", TRUE, ch,
			0, 0, TO_ROOM);
		send_to_char(ch, "You leap.\r\n");
		return 1;
	case 1:
		send_to_room("Oil sprays everywhere!\r\n", ch->in_room);
		return 1;
	case 2:
		act("$N screams as $E attacks you!", FALSE, vict, 0, ch,
			TO_CHAR);
		act("$N screams as $E attacks $n!", FALSE, vict, 0, ch,
			TO_ROOM);
		return 1;
	default:
		return 0;
	}
	return 0;
}
