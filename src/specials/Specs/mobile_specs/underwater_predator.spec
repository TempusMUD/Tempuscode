//
// File: underwater_predator.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(underwater_predator)
{

  struct char_data *pred = (struct char_data *) me;
  struct char_data *vict = NULL;
  struct room_data *troom = NULL;

  if (cmd || pred->getPosition() < POS_RESTING || 
      AFF2_FLAGGED(pred, AFF2_PETRIFIED) || !AWAKE(pred))
    return 0;

  if ((vict = FIGHTING(pred)) && 
      SECT_TYPE(pred->in_room) != SECT_UNDERWATER && !number(0, 3)) {

    if (EXIT(pred, DOWN) && 
	((troom = EXIT(pred, DOWN)->to_room) != NULL) &&
	SECT_TYPE(troom) == SECT_UNDERWATER && 
	!ROOM_FLAGGED(troom, ROOM_GODROOM | ROOM_DEATH | ROOM_PEACEFUL)) {

      if ((STRENGTH_APPLY_INDEX(pred) + number(1, 6) >
	   STRENGTH_APPLY_INDEX(vict))) {

	act("$n drags you under!!!", FALSE, pred, 0, vict, TO_VICT);
	act("$n drags $N under!!!", FALSE, pred, 0, vict, TO_NOTVICT);
	char_from_room(pred);
	char_to_room(pred, troom);
	char_from_room(vict);
	char_to_room(vict, troom);
	look_at_room(vict, vict->in_room, 0);

	act("$N is dragged, struggling,  in from above by $n!!",
	    FALSE, pred, 0, vict, TO_NOTVICT);

	WAIT_STATE(pred, PULSE_VIOLENCE);
	return 1;
      }
    }
  } else if (!FIGHTING(pred) && MOB_FLAGGED(pred, MOB_AGGRESSIVE) &&
	     SECT_TYPE(pred->in_room) == SECT_UNDERWATER &&
	     EXIT(pred, UP) && 
	     ((troom = EXIT(pred, UP)->to_room) != NULL) &&
	     !ROOM_FLAGGED(troom, ROOM_NOMOB | ROOM_PEACEFUL |
			   ROOM_DEATH | ROOM_GODROOM)) {

    for (vict = troom->people; vict; vict = vict->next_in_room) {

      if ((IS_MOB(vict) && !MOB2_FLAGGED(pred, MOB2_ATK_MOBS)) ||
	  (!IS_NPC(vict) && !vict->desc) ||
	  PRF_FLAGGED(vict, PRF_NOHASSLE) || !CAN_SEE(pred, vict) ||
	  PLR_FLAGGED(vict, PLR_OLC | PLR_WRITING | PLR_MAILING) ||
	  vict->getPosition() == POS_FLYING ||
	  (MOUNTED(vict) && (MOUNTED(vict))->getPosition() == POS_FLYING))
	continue;

      act("$n cruises up out of sight with deadly intention.",
	  TRUE, pred, 0, 0, TO_ROOM);
      char_from_room(pred);
      
      char_to_room(pred, troom);
      act("$n emerges from the depths and attacks!!!",
	  TRUE, pred, 0, 0, TO_ROOM);

      hit(pred, vict, TYPE_UNDEFINED);
      return 1;
    }
  }
  return 0;
}

  
	     
	
    
