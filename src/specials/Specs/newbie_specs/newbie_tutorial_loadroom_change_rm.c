//
// File: newbie_tutorial_loadroom_change_rm.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//
// This special is designed to change the starting room for a newbie. It changes a character's
// hometown to the "completed tutorial" hometown once they make it to the top of the Greenhorn Tower.
// It only affects those characters whose hometown is HOME_NEWBIE_SCHOOL, which would be real newbies,
// and shouldn't affect any other characters that wander in.
// This allows new players to start at the beginning of the tutorial, but once completed,
// they can load after the tutorial in the fighting/transport area.

SPECIAL(newbie_tutorial_loadroom_change_rm)
{
    if (spec_mode != SPECIAL_ENTER) {
        return 0;
    }

    if (GET_HOME(ch) == HOME_NEWBIE_SCHOOL) {
        GET_HOME(ch) = HOME_NEWBIE_TUTORIAL_COMPLETE;
        send_to_char(ch,
            "Congratulations!\r\n"
            "You will now enter the realm here, until you reach level 6 or register in a town.\r\n");
        return 1;
    }
    return 0;
}
