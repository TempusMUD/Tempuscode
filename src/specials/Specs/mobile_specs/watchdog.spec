//
// File: watchdog.spec                                         -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

// As people stand in the room, watchdog gets more and more irritated.  When
// he sees someone ugly enough, he snaps.

SPECIAL(watchdog)
{
    struct char_data *dog = (struct char_data *) me;
    struct char_data *vict = NULL;
    static byte indignation = 0;
	CharacterList::iterator it

    if( spec_mode == SPECIAL_DEATH ) return 0;

    if (cmd || !AWAKE(dog) || FIGHTING(dog))
            return 0;

	for( it = dog->in_room->people.begin(); it != ch->in_room->people.end(); ++it ) {
		if (*it != dog && CAN_SEE(dog, (*it)) && GET_LEVEL((*it)) < LVL_IMMORT) {
			vict = *it;
			break;
		}
	}

        if (!vict || vict == dog)
                return 0;

        count = 0;
        act("$n begins barking visciously at $N!",FALSE,dog,0,vict, TO_NOTVICT);
        act("$n begins barking visciously at you!",FALSE,dog,0,vict, TO_VICT);
        count++;
        return 1;
    
    if (count && vict ) {
        switch (number(0, 10)) {
        case 0:
            act("$n growls menacingly at $N.", FALSE, dog, 0, vict, TO_NOTVICT);
            act("$n growls menacingly at you.", FALSE, dog, 0, vict, TO_VICT);
            break;
        case 1:
            act("$n barks loudly at $N.", FALSE, dog, 0, vict, TO_NOTVICT);
            act("$n barks loudly at you.", FALSE, dog, 0, vict, TO_VICT);
            break;
        case 2:
            act("$n growls at $N.", FALSE, dog, 0, vict, TO_NOTVICT);
            act("$n growls at you.", FALSE, dog, 0, vict, TO_VICT);
            break;
        case 3:
            act("$n snarls at $N.", FALSE, dog, 0, vict, TO_NOTVICT);
            act("$n snarls at you.", FALSE, dog, 0, vict, TO_VICT);
            break;
		default:
			break;
        }
        count++;

        if (count > (GET_CHA(vict) >> 2)) {
            hit(dog, vict, TYPE_UNDEFINED);
            count = 0;
            vict = NULL;
        } 
        return 1;
    }
    return 0;
}

            
