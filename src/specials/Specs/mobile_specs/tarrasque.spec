//
// File: tarrasque.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define T_SLEEP   0
#define T_ACTIVE  1
#define T_RETURN  2
#define T_ERROR   3

#define EXIT_RM_MODRIAN 24878
#define EXIT_RM_ELECTRO -1
#define LAIR_RM       24918
#define BELLY_RM      24919

#define JRM_BASE_OUT  24903
#define JRM_TOP_OUT   24900
#define JRM_BASE_IN   24914
#define JRM_TOP_IN    24916

#define T_SLEEP_LEN   100
#define T_ACTIVE_LEN  200

struct room_data *belly_rm = NULL;

room_num inner_tunnel[] = {24914, 24915, 24916, 24917};
room_num outer_tunnel[] = {24903, 24902, 24901, 24900, 24878};

void
tarrasque_jump(struct char_data *tarr, int jump_mode)
{

  struct room_data *up_room = NULL;
  int i;

  act("$n takes a flying leap upwards into the chasm!",
      FALSE, tarr, 0, 0, TO_ROOM);

  for (i = 1; i <= (jump_mode == T_ACTIVE ? 4 : 3); i++) {
    if (!(up_room = real_room(jump_mode == T_ACTIVE ?
			      outer_tunnel[i] : inner_tunnel[i]))) {
      break;
    }
    char_from_room(tarr);
    char_to_room(tarr, up_room);

    if ((jump_mode == T_ACTIVE && i == 4) ||
	(jump_mode == T_RETURN && i == 3))
      act("$n comes flying up from the chasm and lands with an earthshaking footfall!",
	  FALSE, tarr, 0, 0, TO_ROOM);
    else
      act("$n comes flying past from below, and disappears above you!",
	  FALSE, tarr, 0, 0, TO_ROOM);
  }
}
int
tarrasque_fight(struct char_data *tarr)
{
  struct char_data *vict = NULL, *vict2 = NULL;
 
  if (!FIGHTING(tarr)) {
    slog("SYSERR: FIGHTING(tarr) == NULL in tarrasque_fight!!");
    return 0;
  }

  for (vict = tarr->in_room->people; vict; vict = vict->next_in_room)
    if (vict != tarr && vict != FIGHTING(tarr) &&
	tarr == FIGHTING(vict) &&
	CAN_SEE(tarr, vict) && !PRF_FLAGGED(vict, PRF_NOHASSLE))
      break;
  
  if (vict) {
    for (vict2 = tarr->in_room->people; vict2; vict2 = vict2->next_in_room)
      if (vict2 != tarr && vict2 != FIGHTING(tarr) && vict != vict2 &&
	  tarr == FIGHTING(vict2) &&
	  CAN_SEE(tarr, vict2) && !PRF_FLAGGED(vict2, PRF_NOHASSLE))
	break;
  }
  
  if (!number(0, 2)) { /* charge */
    act("$n charges forward!!", FALSE, tarr, 0, 0, TO_ROOM);
    WAIT_STATE(tarr, 2 RL_SEC);

    if (damage(tarr, FIGHTING(tarr), 
	       (GET_DEX(FIGHTING(tarr)) < number(5, 28)) ? 
	       (dice(30, 20) + 300) : 0,
	       TYPE_GORE_HORNS, WEAR_BODY))
      return 1;
    
    if (vict && GET_DEX(vict) < number(5, 25) &&
	damage(tarr, vict, 
	       (GET_DEX(vict) < number(5, 28)) ? 
	       (dice(20, 40) + 300) : 0,
	       TYPE_TRAMPLING, WEAR_BODY))
      return 1;
    if (vict2  && GET_DEX(vict2) < number(5, 25) &&
	damage(tarr, vict2, 
	       (GET_DEX(vict2) < number(5, 28)) ? 
	       (dice(20, 40) + 300) : 0,
	       TYPE_TRAMPLING, WEAR_BODY))
      return 1;

  }

  if (!FIGHTING(tarr))
    return 1;

  if (!number(0, 1)) { /* tail lash */
    act("$n lashes out with $s tail!!", FALSE, tarr, 0, 0, TO_ROOM);
    WAIT_STATE(tarr, 3 RL_SEC);
    if (!damage(tarr, FIGHTING(tarr), 
		GET_DEX(FIGHTING(tarr)) < number(5, 28) ?  
		(dice(20, 20) + 100) : 0,
		TYPE_TAIL_LASH, WEAR_LEGS) &&
	FIGHTING(tarr) &&
	(FIGHTING(tarr))->getPosition() == POS_FIGHTING && 
	GET_DEX(FIGHTING(tarr)) < number(10, 18) &&
	!PRF_FLAGGED(FIGHTING(tarr), PRF_NOHASSLE))
      (FIGHTING(tarr))->setPosition( POS_RESTING );
    else
      return 1;
    
    if (vict) {
      if (!damage(tarr, vict, 
		  GET_DEX(vict) < number(5, 28) ?	
		  (dice(20, 20) + 100) : 0,
		  TYPE_TAIL_LASH, WEAR_LEGS) && 
	  vict->getPosition() == POS_FIGHTING && GET_DEX(vict) < number(10, 18) &&
	  !PRF_FLAGGED(vict, PRF_NOHASSLE))
	vict->setPosition( POS_RESTING);
      else
	return 1;
    }
    
    if (vict2) {
      if (!damage(tarr, vict2, 
		  GET_DEX(vict2) < number(5, 28) ?  
		  (dice(20, 20) + 100) : 0,
		  TYPE_TAIL_LASH, WEAR_LEGS) && 
	  vict2->getPosition() == POS_FIGHTING && GET_DEX(vict2) < number(10, 18) &&
	  !PRF_FLAGGED(vict2, PRF_NOHASSLE))
	vict2->setPosition( POS_RESTING );
      else
	return 1;
    }
  }

  if (!FIGHTING(tarr))
    return 1;
  
  /* biting attacks */
  if (vict) {
    if (GET_DEX(vict) < number(5, 23) &&
	!mag_savingthrow(vict, 50, SAVING_ROD)) {
      /* swallow */
      //      send_to_char("swallow.\r\n", vict);
    } else
      if (damage(tarr, vict, 
		 GET_DEX(vict) < number(5, 28) ?  
		 (dice(40, 20) + 200) : 0,
		 TYPE_BITE, WEAR_BODY))
	return 1;
  }
  else if (vict2) {
    if (GET_DEX(vict2) < number(5, 23) &&
	!mag_savingthrow(vict2, 50, SAVING_ROD)) {
      /* swallow */
      //      send_to_char("swallow.\r\n", vict2);
    } else
      if (damage(tarr, vict2, 
		 GET_DEX(vict2) < number(5, 28) ?	
		 (dice(40, 20) + 200) : 0,
		 TYPE_BITE, WEAR_BODY))
	return 1;
  }
  else {
    if (GET_DEX(FIGHTING(tarr)) < number(5, 23) &&
	!mag_savingthrow(FIGHTING(tarr), 50, SAVING_ROD)) {
      /* swallow */
      //      send_to_char("swallow.\r\n", FIGHTING(tarr));
    } else
      damage(tarr, FIGHTING(tarr), 
	     GET_DEX(FIGHTING(tarr)) < number(5, 28) ? 
	     (dice(40, 20) + 200) : 0,
	     TYPE_BITE, WEAR_BODY);
  }
  return 1;
}
  
SPECIAL(tarrasque)
{

  static int mode = 0, tframe = TIME_MODRIAN, checked = FALSE;
  static unsigned int timer = 0;
  struct char_data *tarr = (struct char_data *) me;
  struct char_data *vict = NULL, *next_vict = NULL;
  struct room_data *rm = NULL;

  if (!checked) {
    checked = TRUE;
    if (!(belly_rm = real_room(24878))) {
      slog("SYSERR: bunk room in tarrasque spec.");
      /*
      tarr->mob_specials.shared->func = NULL;
      REMOVE_BIT(MOB_FLAGS(tarr), MOB_SPEC);*/
      return 1;
    }
  }

  if (cmd) {
    if (CMD_IS("status") && GET_LEVEL(ch) >= LVL_IMMORT) {
      sprintf(buf, "Tarrasque status: mode (%d), timer (%d), tframe (%d)\r\n",
	      mode, timer, tframe);
      send_to_char(buf, ch);
      return 1;
    }
    if (CMD_IS("reload") && GET_LEVEL(ch) > LVL_DEMI) {
      mode = T_SLEEP;
      timer = 0;
      if ((rm = real_room(LAIR_RM))) {
	char_from_room(tarr);
	char_to_room(tarr, rm);
      }
      send_to_char("Tarrasque reset.\r\n", ch);
      return 1;
    }
    if (mode == T_SLEEP && !AWAKE(tarr))
      timer += T_SLEEP_LEN / 10;
    return 0;
  }
  
  if (FIGHTING(tarr) && tarr->getPosition() == POS_FIGHTING)
    return (tarrasque_fight(tarr));
  
  timer++;

  switch (mode) {
  case T_SLEEP:
    if (timer > T_SLEEP_LEN) {

      tarr->setPosition( POS_STANDING );

      /*      if (tframe == TIME_MODRIAN) {
	      tframe = TIME_ELECTRO;
	      if (!add_path_to_mob(tarr, "tarr_exit_ec")) {
	      slog("SYSERR: error assigning tarr_exit_ec path to tarrasque.");
	      mode = T_ERROR;
	      return 1;
	      }
	      } else { */
      tframe = TIME_MODRIAN;
      if (!add_path_to_mob(tarr, "tarr_exit_mod")) {
	slog("SYSERR: error assigning tarr_exit_mod path to tarrasque.");
	mode = T_ERROR;
	return 1;
      }

      mode = T_ACTIVE;
      timer = 0;
    } 
    else if (tarr->in_room->number == LAIR_RM && AWAKE(tarr)) {
      act("$n goes to sleep.", FALSE, tarr, 0, 0, TO_ROOM);
      tarr->setPosition( POS_SLEEPING );
    }
    break;

  case T_ACTIVE:
    if (timer > T_ACTIVE_LEN) {
      mode = T_RETURN;
      timer = 0;
      
      if (!add_path_to_mob(tarr, "tarr_return_mod")) {
	slog("SYSERR: error assigning tarr_return_mod path to tarrasque.");
	mode = T_ERROR;
	return 1;
      }
      return 1;
    }
    
    if (tarr->in_room->number == outer_tunnel[0]) {
      tarrasque_jump(tarr, T_ACTIVE);
      return 1;
    }

    if (tarr->next_in_room || tarr->in_room->people == tarr) {
      for (vict = tarr->in_room->people; vict; vict = next_vict)
	if (!IS_NPC(vict) && GET_LEVEL(vict) < 10) {
	  if (vict->getPosition() < POS_STANDING)
	    vict->setPosition( POS_STANDING );
	  act("You are overcome with terror at the sight of $N!",
	      FALSE, vict, 0, tarr, TO_CHAR);
	  do_flee(vict, "", 0, 0);
	}
    }
      
    break;
    
  case T_RETURN:

    if (tarr->in_room->number == LAIR_RM) {
      mode = T_SLEEP;
      tarr->setPosition( POS_SLEEPING );
      act("$n lies down and falls asleep.", FALSE, tarr, 0, 0, TO_ROOM);
      timer = 0;
      return 1;
    }

    if (tarr->in_room->number == inner_tunnel[0]) {
      tarrasque_jump(tarr, T_RETURN);
      return 1;
    }
    if (tarr->next_in_room || tarr->in_room->people == tarr) {
      for (vict = tarr->in_room->people; vict; vict = next_vict)
	if (!IS_NPC(vict) && GET_LEVEL(vict) < 10) {
	  if (vict->getPosition() < POS_STANDING)
	    vict->setPosition( POS_STANDING );
	  act("You are overcome with terror at the sight of $N!",
	      FALSE, vict, 0, tarr, TO_CHAR);
	  do_flee(vict, "", 0, 0);
	}
    }

    break;
  default:
    break;
  }
  return 0;
}
      

  

