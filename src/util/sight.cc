#include <unistd.h>
#include <stdlib.h>
#include "defs.h"
#include "creature.h"
#include "utils.h"

bool
NIGHT_VIS(Creature *ch)
{
	return (CHECK_SKILL(ch, 605) >= 70);
}

bool
IS_RACE_INFRA(Creature *ch)
{
	return ((GET_RACE(ch) == RACE_ELF) ||     
		(GET_RACE(ch) == RACE_DROW) ||    
		(GET_RACE(ch) == RACE_DWARF) ||    
		(GET_RACE(ch) == RACE_HALF_ORC) || 
		(GET_RACE(ch) == RACE_TABAXI) ||   
		(GET_RACE(ch) == RACE_UNDEAD) ||   
		(GET_RACE(ch) == RACE_DRAGON) ||   
		(GET_RACE(ch) == RACE_ORC) ||      
		(GET_RACE(ch) == RACE_OGRE) ||      
		(GET_RACE(ch) == RACE_GOBLIN) ||   
		(GET_RACE(ch) == RACE_TROLL) ||    
		(GET_RACE(ch) == RACE_BUGBEAR)  ||   
		(GET_CLASS(ch) == CLASS_VAMPIRE && IS_EVIL(ch)));
}

bool
CAN_SEE_IN_DARK(Creature *ch)
{
	return (AFF_FLAGGED(ch, AFF_INFRAVISION) ||
		PRF_FLAGGED(ch, PRF_HOLYLIGHT) ||
		AFF_FLAGGED(ch, AFF_RETINA) ||
		IS_RACE_INFRA(ch) || NIGHT_VIS(ch));
}


bool
LIGHT_OK(Creature *sub)
{
	return ((!IS_AFFECTED(sub, AFF_BLIND) && 
                          (IS_LIGHT((sub)->in_room) || 
                           CAN_SEE_IN_DARK(sub))) ||
                         AFF3_FLAGGED(sub, AFF3_SONIC_IMAGERY));
}

bool
LIGHT_OK_ROOM(Creature *sub, room_data *room)
{
	return ((!IS_AFFECTED(sub, AFF_BLIND) && 
		(IS_LIGHT(room) ||  
		CAN_SEE_IN_DARK(sub))) || 
		AFF3_FLAGGED(sub, AFF3_SONIC_IMAGERY));
}

bool
ROOM_OK(Creature *sub)
{
	return (!sub->in_room ||                   
		!ROOM_FLAGGED(sub->in_room, ROOM_SMOKE_FILLED) || 
		AFF3_FLAGGED(sub, AFF3_SONIC_IMAGERY));
}

bool
MORT_CAN_SEE(Creature *sub, Creature *obj)
{
	return (LIGHT_OK(sub) && ROOM_OK(sub) && 
		INVIS_OK(sub, obj) &&     
		(GET_LEVEL(sub) > LVL_IMMORT  || 
		!MOB_UNAPPROVED(obj)) && 
		(!obj->isTester()  || 
		sub->isTester() || 
		IS_NPC(sub)));
}

/* End of CAN_SEE */

bool
MOB_UNAPPROVED(Creature *ch)
{
	return (MOB2_FLAGGED(ch, MOB2_UNAPPROVED));
}

/* Can subject see character "obj"? */

bool
CHAR_CAN_SEE(Creature *ch, room_data *room = NULL)
{
	if (!room)
		room = ch->in_room;

	if (IS_AFFECTED(ch, AFF_BLIND) ||
			ROOM_FLAGGED(room, ROOM_SMOKE_FILLED)) {
		if (!AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY))
			return false;
	}
	if (IS_DARK(room)) {
		if (!AFF_FLAGGED(ch, AFF_INFRAVISION) &&
				!AFF_FLAGGED(ch, AFF_RETINA) &&
				!IS_RACE_INFRA(ch) &&
				!AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY) &&
				CHECK_SKILL(ch, 605) < 70)
			return false;
	}
	return true;
}

bool
INVIS_OK(Creature *sub, Creature *obj)
{
	// Can't see invis'd immortals
	if (GET_LEVEL(sub) < GET_INVIS_LEV(obj))
		return false;

	// Holy is the light that shines on the chosen
	if (PRF_FLAGGED(sub, PRF_HOLYLIGHT))
		return true;

	// Remort invis.  (mobs don't have it and aren't affected by it.)
	if (!IS_NPC(sub) && !IS_NPC(obj) &&
			GET_LEVEL(sub) < GET_REMORT_INVIS(obj) &&
			GET_REMORT_GEN(sub) < GET_REMORT_GEN(obj))
		return false;

	if (IS_AFFECTED_2(sub, AFF2_TRUE_SEEING) ||
			AFF_FLAGGED(sub, AFF_DETECT_INVIS))
		return true;

	// Invis/Transparent
	if (IS_AFFECTED(obj, AFF_INVISIBLE)
			|| IS_AFFECTED_2(obj, AFF2_TRANSPARENT))
		return false;

	// Invis to Undead
	if (IS_UNDEAD(sub) && IS_AFFECTED_2(obj, AFF2_INVIS_TO_UNDEAD))
		return false;

	// Invis to animals
	if (IS_ANIMAL(sub) && IS_AFFECTED_2(obj, AFF2_INVIS_TO_ANIMALS))
		return false;

	return true;
}

bool
CAN_SEE(Creature *sub, Creature *obj)
{
	// Can always see self
	if (sub == obj)
		return true;

	// Nothing gets at all gets through immort invis
	if (GET_LEVEL(sub) < GET_INVIS_LEV(obj))
		return false;

	// Mortals can't see unapproved mobs
	if (GET_LEVEL(sub) < LVL_IMMORT && MOB_UNAPPROVED(obj))
		return false;

	// Non-tester mortals can't see testers
	if (GET_LEVEL(sub) < LVL_IMMORT && !sub->isTester() && obj->isTester())
		return false;

	// Holy light sees over all in-game affects
	if (PRF_FLAGGED(sub, PRF_HOLYLIGHT))
		return true;

	// Blindness independant of object
	if (!CHAR_CAN_SEE(sub))
		return false;

	// Object-dependant blindness/invisibility
	if (!INVIS_OK(sub, obj))
		return false;
	return true;
}
