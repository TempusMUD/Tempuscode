//
// File: questor_util.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

extern int quest_status;

SPECIAL(questor_util)
{

	struct obj_data *obj = (struct obj_data *)me;
	int found = 0;

	if (spec_mode != SPECIAL_CMD)
		return false;

	if (GET_LEVEL(ch) < LVL_IMMORT ||
		(!CMD_IS("activate") && !CMD_IS("deactivate")))
		return 0;

	skip_spaces(&argument);

	if (!isname(argument, obj->name))
		return 0;

	if (!PRF_FLAGGED(ch, PRF_QUEST)) {
		send_to_char(ch, "You aren't even a part of the quest!\r\n");
		return 1;
	}

	TOGGLE_BIT(PLR_FLAGS(ch), PLR_QUESTOR);

	mudlog(GET_INVIS_LVL(ch), NRM, true,
		"(GC) %s has %s QUESTOR flag.", GET_NAME(ch),
		PLR_FLAGGED(ch, PLR_QUESTOR) ? "obtained" : "removed");

	send_to_char(ch, "Questor flag toggled.\r\n");
	act("$n flips some switches on $p.", FALSE, ch, obj, 0, TO_ROOM);

	if (!PLR_FLAGGED(ch, PLR_QUESTOR)) {
		CreatureList::iterator cit = characterList.begin();
		for (; cit != characterList.end(); ++cit) {
			if (PLR_FLAGGED((*cit), PLR_QUESTOR)) {
				found = TRUE;
				break;
			}
		}
		if (!found) {
			mudlog(GET_INVIS_LVL(ch), NRM, true, "Quest_status disabled");
			quest_status = 0;
		}
	} else if (!quest_status) {
		mudlog(GET_INVIS_LVL(ch), NRM, true, "Quest_status enabled");
		quest_status = 1;
	}
	return 1;
}
