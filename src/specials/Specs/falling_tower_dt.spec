//
// File: falling_tower_dt.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(falling_tower_dt)
{
  struct room_data *under_room;
  
  if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK) return FALSE;

  if ((under_room = real_room(5241)) == NULL)
    return 0;
  if (ch->getPosition() == POS_FLYING)
    return 0;
  if (CMD_IS("down")) {
    send_to_char("As you begin to descend the ladder, a rung breaks, sending you\r\n"
                 "plummeting downwards to the street!\r\n", ch);
    char_from_room(ch);
    char_to_room(ch, under_room);
    send_to_char("You hit the ground hard!!\r\n", ch);
    act("$n falls out of the tower above, and slams into the street hard!", FALSE, ch, 0, 0, TO_ROOM);
    look_at_room(ch, ch->in_room, 0);
    GET_HIT(ch) = MAX(-8, GET_HIT(ch) - 
            dice(10, 100 - GET_DEX(ch) - 40*IS_AFFECTED(ch, AFF_INFLIGHT)));
    if (GET_HIT(ch) > 0)
      ch->setPosition( POS_SITTING );
    else if (GET_HIT(ch) < -6)
      ch->setPosition( POS_MORTALLYW );
    else if (GET_HIT(ch) < -3)
      ch->setPosition( POS_INCAP );
    else 
      ch->setPosition( POS_STUNNED );
    return 1;
  }
  return 0;
}
