#include <vector>

#include "actions.h"
#include "db.h"
#include "comm.h"
#include "fight.h"
#include "handler.h"
#include "interpreter.h"
#include "tmpstr.h"
#include "screen.h"
#include "utils.h"

const char *GUARD_HELP =
"Required Directives:\r\n"
"    guard <direction to block> <room number> {REQUIRED}\r\n"
"        The guard will block the exit from denied creatures while in\r\n"
"        the specified room.\r\n"
"\r\n"    
"Optional Directives:\r\n"
"    allow [not] <class>|<align>|<race>|<clan>|player|all\r\n"
"    deny [not] <class>|<align>|<race>|<clan>|player|all\r\n"
"        These two directives determine who the guard should block.\r\n"
"        They are processed in the order given in the spec-param.\r\n"
"        A creature matching the 'allow' directive will immediately be\r\n"
"        let through, while a creature matching a 'deny' directive will\r\n"
"        be blocked.  If none of the allow/deny directives match, the\r\n"
"        default is to deny access.\r\n"
"    tovict <message sent to blocked creature>\r\n"
"    toroom <message sent to everyone else>\r\n"
"        These are emits shown when the guard blocks a creature.  Within\r\n"
"        them, $n refers to the guard, and $N refers to the creature being\r\n"
"        blocked.\r\n"
"    attack yes|no\r\n"
"        If this directive is set to yes, the guard will begin attacking\r\n"
"        any creature blocked, if not already in combat.  The default is\r\n"
"        not to attack.\r\n"
"    fallible yes|no\r\n"
"        If this directive is set to yes, there is a chance the guard\r\n"
"        will not notice sneaking, infiltrating, or invisible creatures.\r\n"
"        The default is to always block.\r\n"
"\r\n"
"Example:\r\n"
"    For a guard that allows all humans or good knights to pass:\r\n"
"\r\n"
"                allow human\r\n"
"                deny not good\r\n"
"                allow knight\r\n";


SPECIAL(guard)
{
	struct Creature *self = (struct Creature *)me;
	int cmd_idx, lineno, dir = -1;
	Reaction reaction;
	char *to_vict = "You are blocked by $n.";
	char *to_room = "$N is blocked by $n.";
	char *str, *line, *param_key, *dir_str, *room_str;
	bool attack = false, fallible = false;
	char *err = NULL;
	long room_num = -1;


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
		if (!strcmp(param_key, "guard")) {
			dir_str = tmp_getword(&line);
			room_str = tmp_getword(&line);
			room_num = 0;
			if (*room_str) {
				// Ignore directions that don't pertain to the room
				room_num = atoi(room_str);
				if (!room_num || !real_room(room_num)) {
					err = "a bad room number";
					break;
				}
			}
			cmd_idx = find_command(dir_str, true);
			if (cmd_idx == -1 || !IS_MOVE(cmd_idx)) {
				err = "a bad direction";
				break;
			}
			if (room_num != ch->in_room->number)
				continue;
			if (cmd_idx == cmd)
				dir = cmd_idx;
		} else if (!strcmp(param_key, "tovict")) {
			to_vict = line;
		} else if (!strcmp(param_key, "toroom")) {
			to_room = line;
		} else if (!strcmp(param_key, "attack")) {
			attack = (is_abbrev(line, "yes") || is_abbrev(line, "on") ||
				is_abbrev(line, "1") || is_abbrev(line, "true"));
		} else if (!strcmp(param_key, "fallible")) {
			fallible = (is_abbrev(line, "yes") || is_abbrev(line, "on") ||
				is_abbrev(line, "1") || is_abbrev(line, "true"));
		} else {
			err = "an invalid directive";
			break;
		}
	}
	if (dir == -1)
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
	} else if (ch == self || IS_IMMORT(ch) || ALLOW == reaction.react(ch))
		return false;

	// If we're a fallible guard, check to see if they can get past us
	if (fallible && check_sneak(ch, self, true, true) == SNEAK_OK)
		return false;

	// Set to deny if undecided
	act(to_vict, FALSE, self, 0, ch, TO_VICT);
	act(to_room, FALSE, self, 0, ch, TO_NOTVICT);
	if (!err && attack && IS_PC(ch) && !PRF_FLAGGED(ch, PRF_NOHASSLE))
		set_fighting(ch, self, true);

	WAIT_STATE(ch, 1 RL_SEC);
	return true;
}
