//
// File: elven_registry.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(elven_registry)
{
  ACMD(do_say);
  ACMD(do_gen_comm);
  struct char_data *reg = (struct char_data *) me;
  struct obj_data *cert;
  char buf3[MAX_STRING_LENGTH];
  int cost;

  if (!CMD_IS("register"))
    return 0;

  if (!CAN_SEE(reg, ch)) {
    do_say(reg, "Who's there?  Come visible to register.", 0, 0);
    return 1;
  }
  cost = GET_LEVEL(ch)*50;
  if (GET_GOLD(ch) < cost) {
    sprintf(buf2, "It costs %d coins to register with us, which you do not have.", cost);
    perform_tell(reg, ch, buf2);
    return 1;
  }
  if (GET_HOME(ch) == HOME_ELVEN_VILLAGE) {
    perform_tell(reg, ch, "You are already a resident here.");
    if (IS_EVIL(ch)) {
      perform_tell(reg, ch, "But you don't need to be, EVIL scum!");
      send_to_char(ch, "You are no longer a resident of the Elven Village.\r\n");
      act("$n just lost $s residence in the village!", TRUE, ch, 0, 0, TO_ROOM);
      GET_HOME(ch) = HOME_MODRIAN;
    }
    return 1;
  }
  if (PLR_FLAGGED(ch, PLR_KILLER)) {
    sprintf(buf2, "We don't take MURDERERS here, %s!", GET_NAME(ch));
    do_gen_comm(reg, buf2, 0, SCMD_HOLLER);
    return 1;
  }
  if (PLR_FLAGGED(ch, PLR_THIEF)) {
    sprintf(buf2, "We don't take THIEVES in our fair city, %s!", GET_NAME(ch));
    do_gen_comm(reg, buf2, 0, SCMD_HOLLER);
    return 1;
  }
  if (IS_EVIL(ch)) {
    sprintf(buf2, "We don't need your kind here, %s.", GET_NAME(ch));
    perform_tell(reg, ch, buf2);
    return 1;
  } else if (!IS_ELF(ch) && !IS_GOOD(ch)) {
    sprintf(buf2, "Sorry %s, we cannot accept you.", GET_NAME(ch));
    perform_tell(reg, ch, buf2);
    return 1;
  }

  population_record[GET_HOME(ch)]--;
  population_record[HOME_ELVEN_VILLAGE]++;

  GET_HOME(ch) = HOME_ELVEN_VILLAGE;
  GET_GOLD(ch) -= cost;
  sprintf(buf3, "You are now a resident of our fine village, %s.", GET_NAME(ch));
  do_say(reg, buf3, 0, 0);
  sprintf(buf2, "That will be %d coins.", cost);
  perform_tell(reg, ch, buf2);
  if ((cert = read_object(19099))) {
    act("$n presents $N with $p.", FALSE, reg, cert, ch, TO_NOTVICT);
    act("$n presents you with $p.", FALSE, reg, cert, ch, TO_VICT);
    obj_to_char(cert, ch);
  }
mudlog(GET_INVIS_LEV(ch), CMP, true,
	"%s has registered at the Elven Village.", GET_NAME(ch));
  return 1;
}
