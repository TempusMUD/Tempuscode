//
// File: temple_healer.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(temple_healer)
{
  struct char_data *vict;
  int found = 0;
  if( spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK ) return 0;
  if (cmd)
    return FALSE;
  if (ch->getPosition() == POS_FIGHTING && FIGHTING(ch) && 
                  FIGHTING(ch)->in_room == ch->in_room) {
    switch (number(0, 20)) {
    case 0:
      do_say(ch, "Now you pay!!", 0, 0);
      cast_spell(ch, FIGHTING(ch), NULL, SPELL_FLAME_STRIKE);
      return (1);
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
      if (!IS_AFFECTED(ch, AFF_SANCTUARY)) {
        do_say(ch, "Guiharia, aid me now!!", 0, 0);
        call_magic(ch, ch, NULL, SPELL_SANCTUARY, 50, CAST_SPELL);
        return (1);
      } else return FALSE;
    case 8:
      cast_spell(ch, ch, NULL, SPELL_GREATER_HEAL);
      return(1);
    default:
    return(0);
    }
    return 0;
  }

  if (ch->getPosition() != POS_FIGHTING && !FIGHTING(ch)) {
    switch (number(0,18)) {
    case 0:
      do_say(ch, "Rest here, adventurer.  Your wounds will be tended.", 0, 0);
      return TRUE;
    case 1:
      do_say(ch, "You are in the hands of Guiharia here, traveller.", 0, 0);
      return TRUE; 
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:{
      found = FALSE;
      CharacterList::iterator it = ch->in_room->people.begin();
      for( ; it != ch->in_room->people.end() && !found; ++it ) {
        vict = *it;
        if (ch == vict || !CAN_SEE(ch, vict) || !number(0, 2))
          continue;
        if (GET_MOB_VNUM(ch) == 11000 && IS_EVIL(vict)) {
          if (!number(0, 20)) {
            act("$n looks at you with distaste.", FALSE,ch,0,vict, TO_VICT);
            act("$n looks at $N with distaste.", FALSE,ch,0,vict,TO_NOTVICT);
          } else if (!number(0, 20)) {
            act("$n looks at you scornfully.", FALSE, ch, 0, vict, TO_VICT);
            act("$n looks at $N scornfully.", FALSE, ch, 0, vict, TO_NOTVICT);
          }
          continue;  
        } 
        if (IS_NPC(vict))
          continue;
        
        if (GET_HIT(vict) < GET_MAX_HIT(vict)) {
          act("$n touches your forehead, and your pain subsides.", 
              TRUE, ch, 0, vict, TO_VICT);
          act("$n touches $N, and heals $M.", TRUE, ch, 0, vict, TO_NOTVICT);
          
          cast_spell(ch, vict, 0,
                     GET_LEVEL(vict) <= 10 ? SPELL_CURE_LIGHT :
                     GET_LEVEL(vict) <= 20 ? SPELL_CURE_CRITIC :
                     GET_LEVEL(vict) <= 30 ? SPELL_HEAL : SPELL_GREATER_HEAL);
        } else
          continue;
        
        return 1;
      }
      break;
      }
    case 14:
    case 15:
    case 16:{
    
      CharacterList::iterator it = ch->in_room->people.begin();
      for( ; it != ch->in_room->people.end() && !found; ++it ) {
        vict = *it;
        if (ch == vict || IS_NPC(vict) || !CAN_SEE(ch, vict) || !number(0, 2))
          continue;

        if (IS_POISONED(vict) || IS_SICK(vict)) {
          act("$n sweeps $s hand over your body, and your sickness ceases.", 
              FALSE, ch, 0, vict, TO_VICT);
          act("$n sweeps $s hand over $N's body.", 
              TRUE, ch, 0, vict, TO_NOTVICT);
          act("You sweep your hand over $N's body.", 
              FALSE, ch, 0, vict, TO_CHAR);
          
          if (IS_POISONED(vict))
            call_magic(ch, vict, 0, SPELL_REMOVE_POISON, 
                       GET_LEVEL(ch), CAST_SPELL);
          if (IS_SICK(vict))
            call_magic(ch, vict, 0, SPELL_REMOVE_SICKNESS, 
                       GET_LEVEL(ch), CAST_SPELL);
          return TRUE;
        }
      }
      break;
      }
    case 17:
    case 18:{
      found = FALSE;
      CharacterList::iterator it = ch->in_room->people.begin();
      for( ; it != ch->in_room->people.end() && !found; ++it ) {
        vict = *it;

        if (ch==vict || IS_NPC(vict) || !CAN_SEE(ch, vict) || !number(0, 2))
          continue;

        if (affected_by_spell(vict, SPELL_BLINDNESS) || 
            affected_by_spell(vict, SKILL_GOUGE)) {
          act("$n touches your eyes, and your vision returns.", 
              FALSE, ch, 0, vict, TO_VICT);
          act("$n touches the eyes of $N.", TRUE, ch, 0, vict, TO_NOTVICT);
          act("You touch the eyes of $N.", FALSE, ch, 0, vict, TO_CHAR);
          call_magic(ch, vict, 0, SPELL_CURE_BLIND, GET_LEVEL(ch), CAST_SPELL);
          return TRUE;
          
        } 
      }
      break;
      }
    default:
      return FALSE;
    }
    return FALSE;
  }
  return 0;
}
