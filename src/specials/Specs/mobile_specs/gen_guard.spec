//
// File: gen_guard.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define MODE_NORTH       (1 << 0)
#define MODE_EAST        (1 << 1)
#define MODE_SOUTH       (1 << 2)
#define MODE_WEST        (1 << 3)
#define MODE_UP          (1 << 4)
#define MODE_DOWN        (1 << 5)
#define MODE_CLASS_ALLOW (1 << 8)
#define MODE_NOGOOD      (1 << 9)
#define MODE_NONEUTRAL   (1 << 10)
#define MODE_NOEVIL      (1 << 11)
#define MODE_CLAN_ALLOW  (1 << 12)
#define MODE_RACE_ALLOW  (1 << 13)

#define GUARD_MODE    (GET_OBJ_VAL(obj, 1))

/* note on clan_allow:  set obj val 0 to be the vnum of the clan. */

#define GUARD_ALIGN_OK(ch)                                            \
  ((!IS_SET(GUARD_MODE, MODE_NOGOOD) || !IS_GOOD(ch)) &&              \
   (!IS_SET(GUARD_MODE, MODE_NONEUTRAL) || !IS_NEUTRAL(ch)) &&        \
   (!IS_SET(GUARD_MODE, MODE_NOEVIL) || !IS_EVIL(ch)))

SPECIAL(gen_guard)
{
  struct char_data *guard = (struct char_data *) me;
  struct obj_data *obj = GET_IMPLANT(guard, WEAR_ASS);
  char *desc = NULL, *c, buf[EXDSCR_LENGTH];

  if (!cmd || cmd > DOWN + 1 || !obj || GET_LEVEL(ch) > LVL_IMMORT ||
      !IS_SET(GUARD_MODE, (1 << (cmd-1))) ||
      // flag stuff:
      (GUARD_ALIGN_OK(ch) &&
       // clan allow
       ((IS_SET(GUARD_MODE, MODE_CLAN_ALLOW) && 
	 GET_CLAN(ch) == GET_OBJ_VAL(obj, 0)) ||
	// char_class allow
	(IS_SET(GUARD_MODE, MODE_CLASS_ALLOW) &&
	 (GET_CLASS(guard) == GET_CLASS(ch) ||
	  (IS_REMORT(guard) && 
	   GET_REMORT_CLASS(guard) == GET_REMORT_CLASS(ch)))) ||
	// race allow
	(IS_SET(GUARD_MODE, MODE_RACE_ALLOW) && 
	 GET_RACE(ch) == GET_RACE(guard)))) ||
      // end flag stuff
      !AWAKE(guard) || !CAN_SEE(guard, ch) ||
      (AFF_FLAGGED(guard, AFF_CHARM) && ch == guard->master) ||
      affected_by_spell(guard, SPELL_FEAR) ||
      affected_by_spell(guard, SKILL_INTIMIDATE))
    return 0;

  if ((desc = find_exdesc("to_vict", obj->ex_description))) {
    strcpy(buf, desc);
    if ((c = strrchr(buf, '\n')))
      *c = '\0';
    if ((c = strrchr(buf, '\r')))
      *c = '\0';
  } else
    strcpy(buf, "You are blocked by $n.");
  act(buf, FALSE, guard, 0, ch, TO_VICT);

  if ((desc = find_exdesc("to_room", obj->ex_description))) {
    strcpy(buf, desc);
    if ((c = strrchr(buf, '\n')))
      *c = '\0';
    if ((c = strrchr(buf, '\r')))
      *c = '\0';
  } else
    strcpy(buf, "$N is blocked by $n.");
  act(buf, FALSE, guard, 0, ch, TO_NOTVICT);
  
  if (MOB_FLAGGED(guard, MOB_HELPER) &&
      !PRF_FLAGGED(ch, PRF_NOHASSLE))
    hit(guard, ch, TYPE_UNDEFINED);
  
  return 1;
}

