//
// File: magic_books.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define MODE_STR    0
#define MODE_INT    1
#define MODE_WIS    2
#define MODE_DEX    3
#define MODE_CON    4
#define MODE_CHA    5

#define WHICH_STAT (GET_OBJ_VAL(obj, 0))
#define REAL_STAT     (WHICH_STAT == MODE_STR ? ch->real_abils.str :   \
		       WHICH_STAT == MODE_INT ? ch->real_abils.intel : \
		       WHICH_STAT == MODE_WIS ? ch->real_abils.wis :   \
		       WHICH_STAT == MODE_DEX ? ch->real_abils.dex :   \
		       WHICH_STAT == MODE_CON ? ch->real_abils.con :   \
		       ch->real_abils.cha)
		       
SPECIAL(improve_stat_book)
{
  struct obj_data *obj = (struct obj_data *) me;
  char arg1[MAX_INPUT_LENGTH];

  if (!CMD_IS("read"))
    return 0;

  one_argument(argument, arg1);

  if (!*arg1 || !isname(arg1, obj->name))
    return 0;

  if (obj->in_room) {
    send_to_char(ch, "You must take it first.\r\n");
    return 1;
  }

  if (ch != obj->carried_by && ch != obj->worn_by)
    return 0;

  act("You spend several hours reading the magical writing inscribed upon\r\n"
      "the pages of $p, which afterwards\r\nslowly fades from existance.\r\n",
      FALSE, ch, obj, 0, TO_CHAR);
  act("$n spends several hours reading the magical writing inscribed upon\r\n"
      "the pages of $p, which afterwards\r\nslowly fades from existance.",
      FALSE, ch, obj, 0, TO_ROOM);
  
  if ((IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch))) {
    send_to_char(ch, "You feel terrible.\r\n");
    GET_HIT(ch) = 1;
    GET_MANA(ch) = 1;
    GET_MOVE(ch) = 1;
    GET_LIFE_POINTS(ch) = MAX(0, GET_LIFE_POINTS(ch) - 1);
  } else if (REAL_STAT >= 18 ||	invalid_char_class(ch, obj)) {
    send_to_char(ch, "You feel no different than before.\r\n");
  } else {
    send_to_char(ch, "You feel that your %s has increased!\r\n",
	    WHICH_STAT == MODE_STR ? "strength" :
	    WHICH_STAT == MODE_INT ? "intellegence" :
	    WHICH_STAT == MODE_WIS ? "wisdom" :
	    WHICH_STAT == MODE_DEX ? "dex" :
	    WHICH_STAT == MODE_CON ? "constitution" : "charisma");
    REAL_STAT++;
  }
  extract_obj(obj);
  save_char(ch, NULL);
  return 1;
}
#undef MODE_STR
#undef MODE_INT
#undef MODE_WIS
#undef MODE_DEX
#undef MODE_CON
#undef MODE_CHA
#undef WHICH_STAT 
#undef REAL_STAT  

SPECIAL(improve_prac_book)
{

  struct obj_data *obj = (struct obj_data *) me;
  char arg1[MAX_INPUT_LENGTH];

  if (!CMD_IS("read"))
    return 0;

  one_argument(argument, arg1);

  if (!*arg1 || !isname(arg1, obj->name))
    return 0;

  if (obj->in_room) {
    send_to_char(ch, "You must take it first.\r\n");
    return 1;
  }

  if (ch != obj->carried_by && ch != obj->worn_by)
    return 0;

  act("You spend several hours studying the writings within $p,\r\n"
      "which afterwards fades from existance.\r\n",
      FALSE, ch, obj, 0, TO_CHAR);
  act("$n spends several hours studying the writings within $p,\r\n"
      "which afterwards fades from existance.\r\n",
      FALSE, ch, obj, 0, TO_ROOM);
  
  if ((IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch))) {
    send_to_char(ch, "You feel terrible.\r\n");
    GET_HIT(ch) = 1;
    GET_MANA(ch) = 1;
    GET_MOVE(ch) = 1;
    GET_PRACTICES(ch) = MAX(0, GET_PRACTICES(ch) - 1);
  } else if (invalid_char_class(ch, obj)) {
    send_to_char(ch, "You feel no different than before.\r\n");
  } else {
    send_to_char(ch, "You feel an increased ability to learn new skills.\r\n");
    GET_PRACTICES(ch) += MIN(10, GET_OBJ_VAL(obj, 0));
  }
  extract_obj(obj);
  save_char(ch, NULL);
  return 1;

}
