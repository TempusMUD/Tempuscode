//
// File: taunting_frenchman.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(taunting_frenchman)
{
  struct char_data *vict = NULL;
  
   if( spec_mode == SPECIAL_DEATH ) return 0;
   if (cmd || !AWAKE(ch) || FIGHTING(ch) || number(0, 10))
     return (FALSE);
   CharacterList::iterator it = ch->in_room->people.begin();
   for( ; it != ch->in_room->people.end(); ++it ) {
      vict = *it;
     if (vict != ch && CAN_SEE(ch, vict) && 
	 GET_MOB_VNUM(ch) != GET_MOB_VNUM(vict) && !number(0, 3))
       break;
   }
   if (!vict || vict == ch)
     return 0;
   
   switch (number (0, 4)) {
   case 0:
     act ("$n insults your mother!", TRUE, ch, 0, vict, TO_VICT);
     act ("$n just insulted $N's mother!", TRUE, ch, 0, vict, TO_NOTVICT);
     break;
   case 1:
     act ("$n smacks you with a rubber chicken!", 
	  TRUE, ch, 0, vict, TO_VICT);
     act ("$n hits $N in the head with a rubber chicken!", 
	  TRUE, ch, 0, vict, TO_NOTVICT);
     break;
   case 2:
     act ("$n spits in your face.", TRUE, ch, 0, vict, TO_VICT);
     act ("$n spits in $N's face.", TRUE, ch, 0, vict, TO_NOTVICT);
     break;
   case 3:
     act ("$n says, 'GO AWAY, I DONT WANT TO TALK TO YOU NO MORE!", 
	  TRUE, ch, 0, 0, TO_ROOM);
     break;
   case 4:
     act ("$n ***ANNIHILATES*** you with his ultra powerfull taunt!!",
	  TRUE, ch, 0, vict, TO_VICT);
     act ("$n ***ANNIHILATES*** $N with his ultra powerfull taunt!!",
	  TRUE, ch, 0, vict, TO_NOTVICT);
     break;
   default:
     return FALSE;
   }
   return TRUE;
}





