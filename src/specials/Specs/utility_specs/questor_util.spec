//
// File: questor_util.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

extern int quest_status;

SPECIAL(questor_util)
{

  struct obj_data *obj = (struct obj_data *) me;
  int found = 0;
  struct char_data *vict = NULL;

  if (GET_LEVEL(ch) < LVL_IMMORT ||
      (!CMD_IS("activate") && !CMD_IS("deactivate")))
    return 0;

  skip_spaces(&argument);
 
  if (!isname(argument, obj->name))
    return 0;
  
  if (!PRF_FLAGGED(ch, PRF_QUEST)) {
    send_to_char("You aren't even a part of the quest!\r\n", ch);
    return 1;
  }

  TOGGLE_BIT(PLR_FLAGS(ch), PLR_QUESTOR);
  
  sprintf(buf, "(GC) %s has %s QUESTOR flag.", GET_NAME(ch), 
	  PLR_FLAGGED(ch, PLR_QUESTOR) ? "obtained" : "removed");
  mudlog(buf, NRM, GET_INVIS_LEV(ch), TRUE);
  
  send_to_char("Questor flag toggled.\r\n", ch);
  act("$n flips some switches on $p.", FALSE, ch, obj, 0, TO_ROOM);
  
  if (!PLR_FLAGGED(ch, PLR_QUESTOR)) {
    for (vict = character_list; vict; vict = vict->next)
      if (PLR_FLAGGED(ch, PLR_QUESTOR))
	found = TRUE;

    if (!found) {
      mudlog("Quest_status disabled.", NRM, GET_INVIS_LEV(ch), TRUE);
      quest_status = 0;
    }
  } else if (!quest_status) {
    mudlog("Quest_status enabled.", NRM, GET_INVIS_LEV(ch), TRUE);
    quest_status =1 ;
  }
  return 1;
}
  
  
