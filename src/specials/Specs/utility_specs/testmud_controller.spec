//
// File: testmud_controller.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(testmud_controller)
{
	extern int test_mud;
	struct obj_data *obj = (struct obj_data *)me;

	ACMD(do_zreset);
	ACMD(do_goto);
	ACMD(do_trans);
	ACMD(do_shutdown);
	ACMD(do_mload);
	ACMD(do_purge);
	ACMD(do_advance);
	ACMD(do_restore);
	ACMD(do_force);
	ACMD(do_wizlock);
	ACMD(do_zonepurge);
	ACMD(do_stat);
	ACMD(do_vnum);
	ACMD(do_wizutil);

	if (!test_mud) {
		return 0;
	}

	if (CMD_IS("zreset")) {
		act("$n twiddles some controls on $p.", TRUE, ch, obj, 0, TO_ROOM);
		do_zreset(ch, argument, 0, 0);
	} else if (CMD_IS("goto")) {
		act("$n twiddles some controls on $p.", TRUE, ch, obj, 0, TO_ROOM);
		do_goto(ch, argument, 0, 0);
	} else if (CMD_IS("transfer")) {
		act("$n twiddles some controls on $p.", TRUE, ch, obj, 0, TO_ROOM);
		do_trans(ch, argument, 0, 0);
	} else if (CMD_IS("shutdown")) {
		if (GET_LEVEL(ch) < LVL_IMMORT) {
			send_to_char(ch, "You'd better let an immortal do that.\r\n");
		} else {
			act("$n twiddles some controls on $p.", TRUE, ch, obj, 0, TO_ROOM);
			do_shutdown(ch, argument, 0, SCMD_SHUTDOWN);
		}
	} else if (CMD_IS("mload")) {
		act("$n twiddles some controls on $p.", TRUE, ch, obj, 0, TO_ROOM);
		do_mload(ch, argument, 0, 0);
	} else if (CMD_IS("purge")) {
		act("$n twiddles some controls on $p.", TRUE, ch, obj, 0, TO_ROOM);
		do_purge(ch, argument, 0, 0);
	} else if (CMD_IS("advance")) {
		if (GET_LEVEL(ch) < LVL_IMMORT) {
			send_to_char(ch, "You'd better let an immortal do that.\r\n");
		} else {
			act("$n twiddles some controls on $p.", TRUE, ch, obj, 0, TO_ROOM);
			do_advance(ch, argument, 0, 0);
		}
	} else if (CMD_IS("restore")) {
		if (GET_LEVEL(ch) < LVL_IMMORT) {
			send_to_char(ch, "You'd better let an immortal do that.\r\n");
		} else {
			act("$n twiddles some controls on $p.", TRUE, ch, obj, 0, TO_ROOM);
			do_restore(ch, argument, 0, 0);
		}
	} else if (CMD_IS("force")) {
		act("$n twiddles some controls on $p.", TRUE, ch, obj, 0, TO_ROOM);
		do_force(ch, argument, 0, 0);
	} else if (CMD_IS("wizlock")) {
		if (GET_LEVEL(ch) < LVL_IMMORT) {
			send_to_char(ch, "You'd better let an immortal do that.\r\n");
		} else {
			act("$n twiddles some controls on $p.", TRUE, ch, obj, 0, TO_ROOM);
			do_wizlock(ch, argument, 0, 0);
		}
	} else if (CMD_IS("zonepurge")) {
		act("$n twiddles some controls on $p.", TRUE, ch, obj, 0, TO_ROOM);
		do_zonepurge(ch, argument, 0, 0);
	} else if (CMD_IS("stat")) {
		act("$n twiddles some controls on $p.", TRUE, ch, obj, 0, TO_ROOM);
		do_stat(ch, argument, 0, 0);
	} else if (CMD_IS("vnum")) {
		act("$n twiddles some controls on $p.", TRUE, ch, obj, 0, TO_ROOM);
		do_vnum(ch, argument, 0, 0);
	} else if (CMD_IS("reroll")) {
		if (GET_LEVEL(ch) < LVL_IMMORT) {
			send_to_char(ch, "You'd better let an immortal do that.\r\n");
		} else {
			act("$n twiddles some controls on $p.", TRUE, ch, obj, 0, TO_ROOM);
			do_wizutil(ch, argument, 0, SCMD_REROLL);
		}
	} else {
		return 0;
	}
	return 1;
}
