//
// File: quantum_rift.spec                     -- Part of TempusMUD
//
// Copyright 1999 by John Watson & John Rothe, all rights reserved.
//


SPECIAL(quantum_rift)
{
	struct obj_data *rift = (struct obj_data *) me;
	char arg1[MAX_INPUT_LENGTH];

	if (!CMD_IS("enter") || !CMD_IS("listen") 
		|| !CAN_SEE_OBJ(ch, rift) || !AWAKE(ch))
		return 0;
	if(CMD_IS("listen)) {
		send_to_char("An eerie hum is coming from $p.",ch);
		return 0;
	}
	one_argument(argument,arg1);
	if(!isname(arg1, rift->name))
		return 0;
	
	// if this is the max number of folks to enter, drop the timer to 0.
}
