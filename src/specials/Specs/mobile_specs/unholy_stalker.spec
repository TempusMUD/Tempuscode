//
// File: unholy_stalker.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(unholy_stalker)
{
    if (cmd)
        return 0;
    if( spec_mode == SPECIAL_DEATH ) return 0;
    if ( !HUNTING(ch) && !FIGHTING(ch) ) {
        act("$n dematerializes, returning to the negative planes.", TRUE, ch, 0, 0, TO_ROOM);
        extract_char(ch, 0);
        return 1;
    }

    if ( FIGHTING(ch) ) {
        if (!number(0, 3)) {
            call_magic(ch, FIGHTING(ch), 0, SPELL_CHILL_TOUCH, GET_LEVEL(ch) + 10, CAST_SPELL);
        }

        if ( GET_HIT(ch) < 100 && GET_HIT(FIGHTING(ch)) > GET_HIT(ch) &&
             !ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC | ROOM_NORECALL) &&
             GET_LEVEL(ch) > number(20, 35) ) {
            call_magic(ch, ch, 0, SPELL_LOCAL_TELEPORT, 90, CAST_SPELL);
        }
    }

    return 0;
}
