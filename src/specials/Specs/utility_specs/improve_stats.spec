//
// File: improve_stats.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define MODE_STR    0
#define MODE_INT    1
#define MODE_WIS    2
#define MODE_DEX    3
#define MODE_CON    4
#define MODE_CHA    5

char *improve_modes[7] = {
  "strength", "intelligence", "wisdom", 
  "dexterity", "constitution", "charisma",
  "\n"
};

#define REAL_STAT     (mode == MODE_STR ? ch->real_abils.str :   \
		       mode == MODE_INT ? ch->real_abils.intel : \
		       mode == MODE_WIS ? ch->real_abils.wis :   \
		       mode == MODE_DEX ? ch->real_abils.dex :   \
		       mode == MODE_CON ? ch->real_abils.con :   \
		       ch->real_abils.cha)
		       
int 
do_gen_improve(struct char_data *ch, int cmd, int mode, char *argument)
{

  int gold, life_cost;
  int old_stat = REAL_STAT;

  if ((!CMD_IS("improve") && !CMD_IS("train")) || IS_NPC(ch))
    return FALSE;

  if (GET_LEVEL(ch) < 10) {
    send_to_char("You are not yet ready to improve this way.\r\n", ch);
    send_to_char("Come back when you are level 10 or above.\r\n", ch);
    return TRUE;
  }

  gold = REAL_STAT * GET_LEVEL(ch) * 50;
  if (mode == MODE_STR && IS_MAGE(ch))
    gold <<= 1;
  life_cost = MAX(6, (REAL_STAT << 1) - (GET_WIS(ch)));

  skip_spaces(&argument);

  if (!*argument) {
    if ((mode != MODE_STR &&
	 REAL_STAT >= MIN(18 + GET_REMORT_GEN(ch), 22)) ||
	(mode == MODE_STR &&
	 ((IS_REMORT(ch) && REAL_STAT >= MIN(18 + GET_REMORT_GEN(ch), 22)) ||
	  (!IS_REMORT(ch) && 
	   (REAL_STAT > 18 || ch->real_abils.str_add >= 100))))) {
      sprintf(buf, "%sYour %s cannot be improved further.%s\r\n",
	      CCCYN(ch, C_NRM), improve_modes[mode], CCNRM(ch, C_NRM));
      send_to_char(buf, ch);
      return TRUE;
    }

    sprintf(buf, 
	    "It will cost you %d coins and %d life points to improve your %s.\r\n", gold, life_cost, improve_modes[mode]);
    send_to_char(buf, ch);
    sprintf(buf,
	    "$n considers the implications of improving $s %s.",
	    improve_modes[mode]);
    act(buf, TRUE, ch, 0, 0, TO_ROOM);
    if (GET_GOLD(ch) < gold)
      send_to_char("But you do not have enough gold on you for that.\r\n", ch);
    else if (GET_LIFE_POINTS(ch) < life_cost)
      send_to_char("But you do not have enough life points for that.\r\n", ch);
   
    return TRUE;
  }
   
  if (!is_abbrev(argument, improve_modes[mode])) {
    sprintf(buf, "The only thing you can improve here is %s.\r\n",
	    improve_modes[mode]);
    send_to_char(buf, ch);
    return TRUE;
  }

    if ( (mode == MODE_STR && 
             (  (GET_REMORT_GEN(ch)==0 && REAL_STAT == 18 && ch->real_abils.str_add >= 100)
             || ( IS_REMORT(ch) && REAL_STAT >= MIN(18 + GET_REMORT_GEN(ch), 25)) 
             )
         ) || (mode != MODE_STR && REAL_STAT >= MIN(18 + GET_REMORT_GEN(ch), 22)) ) {
    sprintf(buf, "%sYour %s cannot be improved further.%s\r\n",
	    CCCYN(ch, C_NRM), improve_modes[mode], CCNRM(ch, C_NRM));
    send_to_char(buf, ch);
    return TRUE;
  }

  if (GET_GOLD(ch) < gold) {
    sprintf(buf, "You cannot afford it.  The cost is %d coins.\r\n", gold);
    send_to_char(buf, ch);
    return 1;
  }
  if (GET_LIFE_POINTS(ch) < life_cost) {
    sprintf(buf,
	    "You have not gained sufficient life points to do this.\r\n"
	    "It requires %d.\r\n", life_cost);
    send_to_char(buf, ch);
    return 1;
  }

  while (ch->affected)
    affect_remove(ch, ch->affected);

  GET_GOLD(ch) = MAX(0, GET_GOLD(ch) - gold);
  GET_LIFE_POINTS(ch) -= life_cost;
  
    if (mode == MODE_STR) {
        if (REAL_STAT == 18)
            REAL_STAT += (ch->real_abils.str_add / 10);
        else if (REAL_STAT > 18)
            REAL_STAT += 10;

        REAL_STAT += 1;
        ch->real_abils.str_add = 0;    

        if (REAL_STAT > 18) {
            if (REAL_STAT > 28) {
                REAL_STAT = 18 + (REAL_STAT - 28);
            } else {
                ch->real_abils.str_add = (REAL_STAT - 18) * 10;
                REAL_STAT = 18;
            }
        } else
          ch->real_abils.str_add = 0;
    } else {
    REAL_STAT += 1;
    }
  if(mode != MODE_STR)
      sprintf(buf, "%s improved %s from %d to %d at %d.", 
          GET_NAME(ch), improve_modes[mode],old_stat, REAL_STAT,ch->in_room->number);
  else
      sprintf(buf, "%s improved %s from %d to %d/%d at %d.", 
          GET_NAME(ch), improve_modes[mode],old_stat, REAL_STAT,ch->real_abils.str_add,
          ch->in_room->number);
  slog(buf);

  send_to_char("You begin your training.\r\n", ch);
  act("$n begins to train.", FALSE, ch, 0, 0, TO_ROOM);
  WAIT_STATE(ch, REAL_STAT RL_SEC);
  save_char(ch, NULL);

  return TRUE;
}

SPECIAL(improve_dex)
{
  return (do_gen_improve(ch, cmd, MODE_DEX, argument));
}
SPECIAL(improve_str)
{
  return (do_gen_improve(ch, cmd, MODE_STR, argument));
}
SPECIAL(improve_int)
{
  return (do_gen_improve(ch, cmd, MODE_INT, argument));
}
SPECIAL(improve_wis)
{
  return (do_gen_improve(ch, cmd, MODE_WIS, argument));
}
SPECIAL(improve_con)
{
  return (do_gen_improve(ch, cmd, MODE_CON, argument));
}
SPECIAL(improve_cha)
{
  return (do_gen_improve(ch, cmd, MODE_CHA, argument));
}

#undef MODE_STR
#undef MODE_INT
#undef MODE_WIS
#undef MODE_DEX
#undef MODE_CON
#undef MODE_CHA
#undef REAL_STAT
