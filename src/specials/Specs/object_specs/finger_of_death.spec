//
// File: finger_of_death.spec   -- Part of TempusMUD
//
// Copyright 2002 by John D. Rothe, all rights reserved.
//
SPECIAL(finger_of_death)
{
    if( spec_mode != SPECIAL_CMD )
        return 0;
	obj_data *finger = (struct obj_data *)me;

	if (!CMD_IS("activate"))
		return 0;
	
	Tokenizer tokens(argument);
	char token[256];

	if(!tokens.next(token))
		return 0;
	
	if (!isname(token, finger->name))
		return 0;

	if (!tokens.next(token)) {
        send_to_char(ch, "%s has %d charges remaining.\r\n", 
                     finger->short_description, GET_OBJ_VAL(finger,0) );
        send_to_char(ch, "Usage: 'activate %s <mobile name>'\r\n",
                     finger->short_description);
		return 1;
	} else if( GET_OBJ_VAL(finger,0) <= 0 ) {
        send_to_char(ch,"%s is powerless.\r\n", finger->short_description);
        return 1;
    }

    Creature *target = get_char_room_vis(ch, token);
    if( target == NULL ) {
        send_to_char(ch, "There doesn't seem to be a '%s' here.\r\n");
    } else if( IS_PC(target) ) {
        act("$n gives you the finger.", TRUE, ch, finger, target, TO_VICT);
        act("You give $N the finger.", TRUE, ch, finger, target, TO_CHAR);
    } else {
        act("$n disintegrates $N with $p.", FALSE, ch, finger, target, TO_NOTVICT);
        act("$n disintegrates you with $p.", FALSE, ch, finger, target, TO_VICT);
        act("You give $N the finger, destroying it utterly.", TRUE, ch, finger, target, TO_CHAR);
		mudlog( 0, BRF, true, "(f0d) %s has purged %s with %s at %d",
				GET_NAME(ch), GET_NAME(target), 
				finger->short_description, target->in_room->number);
        target->extract(false, true, CON_CLOSE);
        GET_OBJ_VAL(finger,0) -= 1;
    }
	return 1;
}
