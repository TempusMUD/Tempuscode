//
// File: languagemaster.spec                              -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#include "language.h"
#include "comm.h"

#define LANGUAGE_COST 1500000

SPECIAL(languagemaster)
{
    struct Creature *master = (struct Creature *)me;
	int check_only = 0, language_idx = -1;
    char *buf;

	if (spec_mode != SPECIAL_CMD)
		return FALSE;

	if (IS_NPC(ch))
		return 0;

	if (CMD_IS("offer"))
		check_only = 1;
	else if (!CMD_IS("learn"))
		return 0;

	skip_spaces(&argument);

	if (!*argument)
		return 1;
    
    language_idx = find_language_idx_by_name(argument);

    if (!can_speak_language(master, language_idx)) {
        perform_tell(master, ch, 
                     "I'm sorry, but I don't know that language.");
        return 1;
    }

    if (can_speak_language(ch, language_idx)) {
        perform_tell(master, ch, 
                     "But you are already fluent in that language.");
        return 1;
    }
    
    if (known_languages(ch) >= (GET_INT(ch) / 2) - 2) {
        perform_tell(master, ch,
                     "You can't learn any more languages now.  "
                     "Try again later.");
        return 1;
    }

	send_to_char(ch,
		"It will cost you %d gold coins to learn to speak %s.\r\n%s",
		LANGUAGE_COST, tmp_capitalize(argument),
		LANGUAGE_COST > GET_GOLD(ch) ? "Which you don't have.\r\n" : "");

	if (check_only || LANGUAGE_COST > GET_GOLD(ch))
		return 1;

	GET_GOLD(ch) -= LANGUAGE_COST;
    learn_language(ch, language_idx);
    
    buf = tmp_sprintf("You learn to speak %s!", tmp_capitalize(argument));
	act(buf, FALSE, ch, NULL, 0, TO_CHAR);
    buf = tmp_sprintf("$n learns to speak %s!", tmp_capitalize(argument)); 
	act(buf, FALSE, ch, NULL, 0, TO_ROOM);
	ch->saveToXML();
	return 1;
}

#undef LANGUAGE_COST
