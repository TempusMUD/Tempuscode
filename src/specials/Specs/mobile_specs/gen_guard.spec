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

#define GUARD_MODE    ( GET_OBJ_VAL( obj, 1 ) )

/* note on clan_allow:  set obj val 0 to be the vnum of the clan. */

#define GUARD_ALIGN_OK( ch )                                             \
  ( ( !IS_SET( GUARD_MODE, MODE_NOGOOD )    || !IS_GOOD( ch ) ) &&       \
    ( !IS_SET( GUARD_MODE, MODE_NONEUTRAL ) || !IS_NEUTRAL( ch ) ) &&    \
    ( !IS_SET( GUARD_MODE, MODE_NOEVIL )    || !IS_EVIL( ch ) ) )

int parse_char_class(char *arg);
int parse_race(char *arg);

const char *GUARD_HELP =
"    gen_guard is a more complete version of the now obsolete guildguard\r\n"
"special.  Its purpose is to block a certain exit from other creatures given\r\n"
"easily judged criteria.  A specparam is used to configure it, where each\r\n"
"line of the param gives a single directive.\r\n"
"   r\n" \
"    The directives are listed here:\r\n"
"\r\n"
"                allow <class>|<align>|<race>|<clan>|all\r\n"
"                deny <class>|<align>|<race>|<clan>|all\r\n"
"                tovict <message sent to blocked creature>\r\n"
"                toroom <message sent to everyone else>\r\n"
"                attack yes|no\r\n"
"                dir <direction to block>\r\n"
"\r\n"
"Example:\r\n"
"    For a guard that allows all mages, but\r\n"
"blocks all evil non-mages, but lets non-evil people through, you may use\r\n"
"\r\n"
"                allow mage\r\n"
"                deny evil\r\n"
"                allow all\r\n";


bool
guard_match(Creature *ch, char *desc)
{
	clan_data *clan;
	int val;

	if (is_abbrev(desc, "all") || is_abbrev(desc, "any"))
		return true;

	// Check align
	if (is_abbrev(desc, "good"))
		return IS_GOOD(ch);
	if (is_abbrev(desc, "evil"))
		return IS_EVIL(ch);
	if (is_abbrev(desc, "neutral"))
		return IS_NEUTRAL(ch);

	// Check class
	val = parse_char_class(desc);
	if (val != -1)
		return (GET_CLASS(ch) == val || GET_REMORT_CLASS(ch) == val);

	// Check race
	val = parse_race(desc);
	if (val != -1)
		return (GET_RACE(ch) == val);
	
	// Check clan
	clan = clan_by_name(desc);
	if (clan)
		return (GET_CLAN(ch) == clan->number);

	return false;
}

SPECIAL(gen_guard)
{
	struct Creature *guard = (struct Creature *)me;
	struct obj_data *obj;
	char *desc = NULL, *c, buf[EXDSCR_LENGTH];
	int cmd_idx;

	if (spec_mode == SPECIAL_HELP) {
		page_string(ch->desc, GUARD_HELP);
		return 1;
	}

	if (spec_mode != SPECIAL_CMD)
		return 0;

	if (GET_MOB_PARAM(guard)) {
		char *to_vict = "You are blocked by $n";
		char *to_room = "$n is blocked by $N";
		char *str, *line, *param_key;
		bool attack = false;
		enum {
			GUARD_UNDECIDED,
			GUARD_ALLOW,
			GUARD_DENY
		} action = GUARD_UNDECIDED;

		str = GET_MOB_PARAM(guard);
		line = tmp_getline(&str);
		while (line) {
			param_key = tmp_getword(&line);
			if (!strcmp(param_key, "dir")) {
				cmd_idx = find_command(tmp_getword(&line));
				if (cmd_idx == -1 || !IS_MOVE(cmd_idx)) {
					mudlog(LVL_IMMORT, NRM, true, "ERR: bad direction in gen_guard [%d]\r\n",
						guard->in_room->number);
					return false;
				}
				if (cmd_idx != cmd)
					return false;
			} else if (!strcmp(param_key, "allow")) {
			 	if (GUARD_UNDECIDED == action && guard_match(ch, line))
					action = GUARD_ALLOW;
			} else if (!strcmp(param_key, "deny")) {
			 	if (GUARD_UNDECIDED == action && guard_match(ch, line))
					action = GUARD_DENY;
			} else if (!strcmp(param_key, "tovict")) {
				to_vict = line;
			} else if (!strcmp(param_key, "toroom")) {
				to_room = line;
			} else if (!strcmp(param_key, "attack")) {
				attack = (is_abbrev(line, "yes") || is_abbrev(line, "on") ||
					is_abbrev(line, "1") || is_abbrev(line, "true"));
			} else {
				mudlog(LVL_IMMORT, NRM, true, "ERR: Invalid directive '%s' in gen_guard [%d]\r\n",
					param_key, guard->in_room->number);
				return false;
			}
			line = tmp_getline(&str);
		}

		if (GUARD_ALLOW == action)
			return false;

		// Set to deny if undecided
		act(to_vict, FALSE, guard, 0, ch, TO_VICT);
		act(to_room, FALSE, guard, 0, ch, TO_NOTVICT);
		if (attack && !PRF_FLAGGED(ch, PRF_NOHASSLE))
			hit(guard, ch, TYPE_UNDEFINED);
		return true;
	}

	// the worst conditional statement in coding history
	obj = GET_IMPLANT(guard, WEAR_ASS);
	if (!cmd || cmd > DOWN + 1 || !obj || GET_LEVEL(ch) > LVL_IMMORT ||
		!IS_SET(GUARD_MODE, (1 << (cmd - 1))) ||
		// flag stuff:
		(GUARD_ALIGN_OK(ch) &&
			// clan allow
			((IS_SET(GUARD_MODE, MODE_CLAN_ALLOW) &&
					GET_CLAN(ch) == GET_OBJ_VAL(obj, 0)) ||
				// char_class allow
				(IS_SET(GUARD_MODE, MODE_CLASS_ALLOW) &&
					(GET_CLASS(guard) == GET_CLASS(ch) ||
						(IS_REMORT(guard) &&
							GET_REMORT_CLASS(guard) == GET_REMORT_CLASS(ch))))
				||
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
		!PRF_FLAGGED(ch, PRF_NOHASSLE) && !FIGHTING(guard))
		hit(guard, ch, TYPE_UNDEFINED);

	return 1;
}
