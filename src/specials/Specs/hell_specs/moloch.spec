//
// File: moloch.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(moloch)
{

  struct char_data *moloch = (struct char_data *) me, *vict = NULL;
  struct room_data *targ_room = NULL;
  int throne_rooms[4] = {16645, 16692, 16692, 16623}, index;


  if (cmd)
    return 0;

  if (FIGHTING(moloch) && GET_MOB_WAIT(moloch) <= 0) {
    if (!number(0, 10)) {
      call_magic(moloch,FIGHTING(moloch),0,SPELL_FLAME_STRIKE,50,CAST_BREATH);
      return 1;
    } else if (!number(0, 10)) {
      call_magic(moloch,FIGHTING(moloch),0,SPELL_BURNING_HANDS,50,CAST_SPELL);
      return 1;
    } else if (!number(0, 8) && GET_DEX(FIGHTING(moloch)) < number(10, 25)) { 
      act("$n picks you up in his jaws and flails you around!!",
	  FALSE, moloch, 0, FIGHTING(moloch), TO_VICT);
      act("$n picks $N up in his jaws and flails $M around!!",
	  FALSE, moloch, 0, FIGHTING(moloch), TO_NOTVICT);
      damage(moloch, FIGHTING(moloch), dice(30, 29), TYPE_RIP, WEAR_BODY);
      WAIT_STATE(moloch, 7 RL_SEC);
      return 1;
    }
    return 0;
  }
      
  if (FIGHTING(moloch) || number(0, 20))
    return 0;

  if (ch->in_room->number == throne_rooms[(index = number(0, 3))])
    return 0;

  if (!(targ_room = real_room(throne_rooms[index])))
    return 0;

  act("A devilish voice rumbles from all around, in an unknown language.",
      FALSE, moloch, 0, 0, TO_ROOM);
  act("$n slowly fades from existence.",TRUE, moloch, 0, 0, TO_ROOM);
  char_from_room(moloch);
  char_to_room(moloch, targ_room);
  act("$n slowly appears from another place.",TRUE, moloch, 0, 0, TO_ROOM);

  for (vict = moloch->in_room->people, index = 0; vict; 
       vict = vict->next_in_room) {
    if (vict != moloch) {
      if (!IS_DEVIL(vict))
	index = 1;
      else {
	index = 0;
	break;
      }
    }
  }

  if (index)
    cast_spell(moloch, NULL, 0, SPELL_METEOR_STORM);

  return 1;

}
  
