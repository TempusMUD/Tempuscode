//
// File: maze_switcher.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(maze_switcher)
{
	if (!number(0, 4)) {
		if (number(0, 1)) {
			(real_room(8305))->dir_option[3]->to_room = real_room(8307);
		} else {
			(real_room(8305))->dir_option[3]->to_room = real_room(8357);
		}
	}
	return 0;
}
