//
// File: languagemaster.spec                              -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#include "language.h"
#include "comm.h"

#define TONGUE_COST 150000

SPECIAL(languagemaster)
{
    struct Creature *master = (struct Creature *)me;
	int check_only = 0, tongue_idx = TONGUE_NONE;
    int cost = TONGUE_COST;

    cost += (cost * ch->getCostModifier(master)) / 100;
    
	if (spec_mode != SPECIAL_CMD)
		return FALSE;

	if (IS_NPC(ch))
		return 0;

	if (CMD_IS("offer"))
		check_only = 1;
	else if (!(CMD_IS("learn") || CMD_IS("practice") || CMD_IS("train")))
		return 0;

	skip_spaces(&argument);

	if (!*argument) {
        send_to_char(ch, "What is it you wish to learn?\r\n");
		return 1;
    }
    
    tongue_idx = find_tongue_idx_by_name(argument);

    if (tongue_idx == TONGUE_NONE ||
        CHECK_TONGUE(master, tongue_idx) < 100) {
        perform_tell(master, ch, 
                     "I'm sorry, but I can't teach that language.");
        return 1;
    }

    if (CHECK_TONGUE(ch, tongue_idx) >= 50) {
        perform_tell(master, ch, "Sorry, but I can't teach you any more.");
        return 1;
    }
    
	send_to_char(ch, "It will cost you %d gold coins to learn to speak %s%s\r\n",
                 cost,
                 tongue_name(tongue_idx),
                 cost > GET_GOLD(ch) ? ", which I see you don't have." : ".");

	if (check_only || cost > GET_GOLD(ch))
		return 1;

	GET_GOLD(ch) -= cost;
    SET_TONGUE(ch, tongue_idx,
               CHECK_TONGUE(ch, tongue_idx) + number(3, GET_INT(ch)));

	act(tmp_sprintf("Your fluency in %s increases!", tongue_name(tongue_idx)),
        false, ch, NULL, 0, TO_CHAR);

    WAIT_STATE(ch, 2 RL_SEC);

	ch->saveToXML();
	return 1;
}

#undef TONGUE_COST
