//
// File: fate_cauldron.spec                     -- Part of TempusMUD
//
// Copyright 1999 by John Watson & John Rothe, all rights reserved.
//

char *find_exdesc(char *word, struct extra_descr_data *list, int find_exact=0);

SPECIAL(fate_cauldron)
{
	register struct char_data *cur_ch, *next_ch;
	struct obj_data *pot = (struct obj_data *) me;
	char arg1[MAX_INPUT_LENGTH];
	struct char_data *fate = NULL;
	int fateid = 0;

	if (!CMD_IS("look") || !CAN_SEE_OBJ(ch, pot) || !AWAKE(ch))
		return 0;
	one_argument(argument,arg1);
	if(!isname(arg1, pot->name))
		return 0;
	
	act("$n gazes deeply into $p.",
		FALSE, ch, pot, 0, TO_ROOM);
	// Is he ready to remort? Or level 49 at least?
	if(GET_LEVEL(ch) < 49) {
		act("You gaze deep into $p but learn nothing.",
			FALSE, ch, pot, 0, TO_CHAR);
		return 1;
	}
	// Which fate does this guy need.
	if(GET_REMORT_GEN(ch) < 4) {
		fateid = FATE_VNUM_LOW;
	} else if(GET_REMORT_GEN(ch) < 7) {
		fateid = FATE_VNUM_MID;
	} else {
		fateid = FATE_VNUM_HIGH;
	}
	
	for (cur_ch = character_list; cur_ch; cur_ch = next_ch) {
		next_ch = cur_ch->next;
		if(GET_MOB_VNUM(cur_ch) == fateid) {
			fate = cur_ch;
			break;
		}
	}
	act("You gaze deep into $p.", FALSE, ch, pot, 0, TO_CHAR);

	// Couldn't find her
	if(!fate || !fate->in_room) {
		send_to_char("Something important seems to be missing.\r\n",ch);
		return 1;
	}
	// Yay. Finally found her!
	send_to_char("You recieve a vision of another place.\r\n",ch);
	look_at_room(ch,fate->in_room,1);
	WAIT_STATE(ch,3);
	return 1;
}



