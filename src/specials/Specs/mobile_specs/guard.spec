//
// File: guard.spec                     -- Part of TempusMUD
//
// Copyright 2002 by Daniel Lowe, all rights reserved.
//

const char *GUARD_HELP =
"    guard is a more complete version of the now obsolete guildguard and\r\n"
"gen_guard specials.  Its purpose is to block a certain exit from other\r\n"
"creatures given a number of blocking criteria.  A specparam is used to\r\n"
"configure it, where each line of the param gives a single directive.\r\n"
"\r\n" 
"    The directives are listed here:\r\n"
"\r\n"
"                allow [not] <class>|<align>|<race>|<clan>|all\r\n"
"                deny [not] <class>|<align>|<race>|<clan>|all\r\n"
"                tovict <message sent to blocked creature>\r\n"
"                toroom <message sent to everyone else>\r\n"
"                attack yes|no\r\n"
"                dir <direction to block> [<more directions>...]\r\n"
"\r\n"
"Example:\r\n"
"    For a guard that allows all mages, but\r\n"
"blocks all evil non-mages, but lets non-evil people through, you may use\r\n"
"\r\n"
"                allow mage\r\n"
"                deny evil\r\n"
"                allow all\r\n";

SPECIAL(guard)
{
	struct Creature *self = (struct Creature *)me;
	int cmd_idx, lineno, dir = -1;
	Reaction reaction;
	char *to_vict = "You are blocked by $n.";
	char *to_room = "$N is blocked by $n.";
	char *dir_str, *str, *line, *param_key;
	bool got_dir = false, attack = false;
	char *err = NULL;


	if (spec_mode == SPECIAL_HELP) {
		page_string(ch->desc, GUARD_HELP);
		return 1;
	}
	
	if (spec_mode != SPECIAL_CMD || !IS_MOVE(cmd) || !GET_MOB_PARAM(self))
		return 0;

	str = GET_MOB_PARAM(self);
	for (line = tmp_getline(&str), lineno = 1; line;
			line = tmp_getline(&str), lineno++) {

		if (reaction.add_reaction(line))
			continue;

		param_key = tmp_getword(&line);
		if (!strcmp(param_key, "dir")) {
			dir_str = tmp_getword(&line);
			while (dir == -1 && *dir_str) {
				cmd_idx = find_command(dir_str, true);
				if (cmd_idx == -1 || !IS_MOVE(cmd_idx)) {
					err = "a bad direction";
					break;
				}
				got_dir = true;
				if (cmd_idx == cmd)
					dir = cmd_idx;
				dir_str = tmp_getword(&line);
			}
		} else if (!strcmp(param_key, "tovict")) {
			to_vict = line;
		} else if (!strcmp(param_key, "toroom")) {
			to_room = line;
		} else if (!strcmp(param_key, "attack")) {
			attack = (is_abbrev(line, "yes") || is_abbrev(line, "on") ||
				is_abbrev(line, "1") || is_abbrev(line, "true"));
		} else {
			err = "an invalid directive";
			break;
		}
	}
	if (!got_dir)
		err = "no direction";
	else if (dir == -1)
		return false;

	if (err) {
		// Specparam error
		if (IS_PC(ch)) {
			if (IS_IMMORT(ch))
				perform_tell(self, ch, tmp_sprintf(
					"I have %s in line %d of my specparam", err, lineno));
			else {
				mudlog(LVL_IMMORT, NRM, true,
					"ERR: Mobile %d has %s in line %d of specparam",
					GET_MOB_VNUM(self), err, lineno);
				do_say(self, tmp_sprintf(
					"%s Sorry.  I'm broken, but a god has already been notified.",
					GET_NAME(ch)), 0, SCMD_SAY_TO, NULL);
			}
		}
	} else if (IS_IMMORT(ch) || ALLOW == reaction.react(ch))
		return false;

	// Set to deny if undecided
	act(to_vict, FALSE, self, 0, ch, TO_VICT);
	act(to_room, FALSE, self, 0, ch, TO_NOTVICT);
	if (!err && attack && IS_PC(ch) && !PRF_FLAGGED(ch, PRF_NOHASSLE))
		set_fighting(ch, self, true);
	return true;
}
