//
// File: underworld_goddess.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(underworld_goddess)
{
  struct char_data	*vict;
  struct char_data	*styx = NULL;
  struct room_data      *room;
  int                   DUNGEON_SE = 126
  int                   STYX_PRIESTESS = 103

  /* See if Styx is in the room */
  for (vict = ch->in_room->people; vict; vict = vict->next_in_room)
    if (!strcmp((char *)GET_NAME(vict),"Styx"))
      styx = vict;

  /* First and foremost, if I am fighting, beat the shit out of them. */
  if (!cmd && (ch->getPosition() == POS_FIGHTING))  {

    /* pseudo-randomly choose someone in the room who is fighting me */
    for (vict = ch->in_room->people; vict; vict = vict->next_in_room)
      if (FIGHTING(vict) == ch && !number(0, 2))
        break;

    /* if I didn't pick any of those, then just slam the guy I'm fighting */
    if (vict == NULL)
      vict = FIGHTING(ch);

    /* if I'm fighting styx, try to teleport him away.  */
    if (vict == styx)  {
      cast_spell(ch, vict, NULL, SPELL_RANDOM_COORDINATES);
    }

    /* Whip up some magic!  */
    switch (number(0, 9)) {
      case 0:
        cast_spell(ch, vict, NULL, SPELL_FIREBALL);
        break;
      case 1:
        cast_spell(ch, vict, NULL, SPELL_FLAME_STRIKE);
        break;
      case 2:
        cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT);
        break;
      case 3:
        cast_spell(ch, vict, NULL, SPELL_SLEEP);
        break;
      case 4:
      case 5:
      case 6:
        cast_spell(ch, ch, NULL, SPELL_GREAT_HEAL);
        break;
    }

    /*  Check to make sure I haven't killed him! */
    if (ch != FIGHTING(vict))
      return 1;

    /* And maybe say something nice! */
    switch (number(0, 7)) {
    case 0:
      sprintf(buf,"The goddess tells you, 'You are a fool to fight me %s.'\r\n", GET_NAME(vict));
      send_to_char(buf, vict);
      break;
    case 1:
      sprintf(buf,"The goddess tells you, 'Fool!  Now watch yourself perish!'\r\n");
      send_to_char(buf, vict);
      break;
    }
    return 1;
  }

  /* check to see if Styx is hurt. */
  if (!cmd && styx && ((GET_MAX_HIT(styx) - GET_HIT(styx)) > 0))  {
    switch (number(0, 1)) {
      case 0:
        send_to_char("The goddess tell you, 'My Love, you are hurt!  Let me heal you.'\r\n",styx);
        cast_spell(ch, styx, NULL, SPELL_GREAT_HEAL);
        break;
     }

     /* If Styx is fighting, send a present to his opponent. */
     if ((vict = FIGHTING(styx)))
       switch (number(0, 4)) {
        case 0:
          send_to_char("The Goddess shouts, 'Chew on this worm face!'\r\n", vict);
          cast_spell(ch, vict, NULL, SPELL_FIREBALL);
          break;
        case 1:
          send_to_char("The Goddess shouts, 'You FIEND!  Take this!'\r\n", vict);
          cast_spell(ch, vict, NULL, SPELL_FLAME_STRIKE);
          break;
        case 2:
          send_to_char("The Goddess shouts, 'Have a LIGHT sewer breath!'\r\n", vict);
          cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT);
          break;
        case 3:
          cast_spell(ch, vict, NULL, SPELL_SLEEP);
          break;
       }
     return 1;
  }

  /* If Nothing else, maybe send Styx a kiss */
  if (!cmd && styx)  {
    switch (number(0, 20)) {
      case 0:
        act("The Goddess of the Underworld kisses $n.", FALSE, styx, 0, 0, TO_ROOM);
        send_to_char("The goddess kisses you very gently.\r\n",styx);
        break;
    }
    return 1;
  }


  /* If styx is not with ME then he might be in the dungeon with that HARLET, */
  /*   so, she might as well worship him TOO! */
  if (!cmd && !styx && !number(0,5))  {
    /*  First get the room number for prison cell where that harlet stays! */
    if ((room = real_room(DUNGEON_SE)) < 0)
      return 0; 

    /* Now see if Styx is there */
    for (vict = room->people; vict; vict = vict->next_in_room)
      if (!strcmp((char *)GET_NAME(vict),"Styx"))  {
        styx = vict;
        break;  /* breaks out of for */
      }

    /*  If Styx is there, see if one of those cutsie priestess girls are there too.   */
    if (styx)  {
        for (vict = styx->in_room->people; vict; vict = vict->next_in_room)
          if ((IS_MOB(vict)) && (GET_MOB_VNUM(vict) == STYX_PRIESTESS))
            break;  /* breaks for loop! */


        if (vict)  {
          act("The young priestess starts kissing $n all over his body!", FALSE, styx, 0, 0, TO_ROOM);
          send_to_char("The young priestess starts kissing you in some very sensitive areas!.\r\n",styx);
          return TRUE;
        }
     }
  }

  /* NOW, check for the pass-phrase for stroking! MUHAHA */

  if (CMD_IS("say") && !strncasecmp(argument," styx sent me",13)) {
      act("The Goddess of the Underworld starts stroking $n's inner thigh.",FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("The Goddess of the Underworld gently strokes your inner thigh with feathery touches.\r\n",ch);
      return TRUE;
  }
      
  return FALSE;
}
