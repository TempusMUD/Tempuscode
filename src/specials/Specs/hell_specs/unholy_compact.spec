//
// File: unholy_compact.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John, all rights reserved.
//

#define LVL_CAN_SELL_SOUL 20     /* <-----Set desired level here */
SPECIAL(unholy_compact)
{
  struct Creature *dude = (struct Creature *) me;
  int gold, life_cost;
  int con_cost = 5;
  char buf[MAX_STRING_LENGTH];
  int min_gen=0;

  if( spec_mode != SPECIAL_CMD ) 
    return 0;

  if (!CMD_IS("sell"))
    return 0;
  skip_spaces(&argument);
  if (IS_NPC(ch)) {
        return 1;
  }
  life_cost = 25;
  gold = 10000*GET_LEVEL(ch);
  if ( IS_KNIGHT(ch) )
          min_gen = 6;
  if ( IS_CLERIC(ch) )
          min_gen = 4;
          
  if (!*argument) {
    send_to_char(ch, "Go sell your crap elsewhere! I only buy souls!\r\n");
        return 1;
  } else if ( GET_CLASS(ch) != CLASS_CLERIC && GET_CLASS(ch) != CLASS_KNIGHT ) {
     perform_tell(dude, ch, "You really have no idea, do you.");
         return 1;
  } else if (GET_LEVEL(ch) < LVL_CAN_SELL_SOUL || GET_REMORT_GEN(ch) < min_gen ) {
    send_to_char(ch, "Your soul is worthless to me.\r\n");
        return 1;
  } else if ( GET_ALIGNMENT(ch) > 0 ) {
     perform_tell(dude, ch, "Your soul is tainted. It cannot be sold.");
         return 1;
  } else if (!is_abbrev(argument, "soul")) {
          perform_tell(dude, ch, "Go sell your crap elsewhere! I only buy souls!");
          return 1;
  } 
        if(PLR2_FLAGGED(ch, PLR2_SOULLESS)) {
      sprintf(buf, "I appreciate your devotion sir. But you only had one soul to sell.");
      perform_tell(dude,ch, buf);
      return 1;
    }
        if (GET_GOLD(ch) < gold) {
      perform_tell(dude, ch, "You think I work for free?!?");
      sprintf(buf, "Bring me %d gold coins and I will make the compact.", gold);
      perform_tell(dude, ch, buf);
    } else if (GET_LIFE_POINTS(ch) < life_cost) {
      perform_tell(dude, ch, "Your essence cannot sustain the compact.");
    } else if (ch->real_abils.con  < con_cost + 3) {
      sprintf(buf, "Your body cannot survive without it's soul. Yet.");
      perform_tell(dude, ch, buf);
    } else {
      act("$n burns the mark of evil into $N's forehead.", TRUE, dude, 0, ch, TO_NOTVICT);
          act("$n burns the mark of evil in your forehead.  All hope is lost...", TRUE, dude, 0, ch, TO_VICT);
      act("$N screams in agony as $S soul is ripped from $S body!", TRUE, dude, 0, ch, TO_NOTVICT);
          ch->setPosition( POS_SLEEPING );
      GET_LIFE_POINTS(ch) -= life_cost;
      GET_GOLD(ch) -= gold;
          SET_BIT(PLR2_FLAGS(ch), PLR2_SOULLESS);
          ch->real_abils.con -= con_cost;
          save_char( ch, NULL );
      send_to_char(ch, "The torturous cries of hell haunt your dreams.\r\n");
      slog("%s sign's the unholy compact, joining the soulless masses.", GET_NAME(ch));
      return 1;
    }
  return 1;
}
