//
// File: hobbes.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(cheeky_monkey)
{
 struct char_data *vict;
 
 if (cmd || !AWAKE(ch) || FIGHTING(ch) || number(0, 3))
   return FALSE;

 for (vict = ch->in_room->people; vict; vict = vict->next_in_room)
   if (vict != ch && CAN_SEE(ch, vict) && 
       (!number(0,5) || !vict->next_in_room))
     break;

 if (!vict || vict == ch)
   return 0;
 
 switch(number(0,10)) {
 case 0:
   act("$n grabs your ass!", TRUE, ch, 0, vict, TO_VICT);
   act("$n reaches around and grabs $N's ass!", TRUE, ch, 0, vict, TO_NOTVICT);
   break;
 default:
   return FALSE;
 }
 return TRUE;
}


SPECIAL(hobbes_speaker)
{
  if (cmd)
    return 0;
  
  switch(number(0, 10)) {
    
  case 0:
    do_say(ch, "THIS IS THE TEXT HIS MOB WILL SAY.", 0, 0);
    return TRUE;
  case 1:
    do_say(ch, "THIS IS MORE TEXT HIS MOB WILL SAY.", 0, 0);
    return TRUE;
  default:
    return FALSE;
  }
}
