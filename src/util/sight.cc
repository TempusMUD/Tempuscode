#include <unistd.h>
#include <stdlib.h>
#include "defs.h"
#include "creature.h"
#include "utils.h"
#include "spells.h"

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
		AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY) ||
		AFF_FLAGGED(ch, AFF_RETINA) ||
		IS_RACE_INFRA(ch) ||
		CHECK_SKILL(ch, SKILL_NIGHT_VISION));
}


bool
LIGHT_OK(Creature *sub)
{
	return ((!IS_AFFECTED(sub, AFF_BLIND) && 
                          (IS_LIGHT(sub->in_room) || 
                           CAN_SEE_IN_DARK(sub))));
}

bool
LIGHT_OK_ROOM(Creature *sub, room_data *room)
{
	return ((!IS_AFFECTED(sub, AFF_BLIND) && 
		(IS_LIGHT(room) ||  
		CAN_SEE_IN_DARK(sub))));
}

bool
ROOM_OK(Creature *sub)
{
	if (!sub->in_room) {
		slog("SYSERR: ROOM_OK called on mob without room");
		return false;
	}	

	return (!ROOM_FLAGGED(sub->in_room, ROOM_SMOKE_FILLED) || 
		AFF3_FLAGGED(sub, AFF3_SONIC_IMAGERY));
}

// Can subject see in room?
bool
CHAR_CAN_SEE(Creature *ch, room_data *room)
{
	if (!room)
		room = ch->in_room;

	if (IS_AFFECTED(ch, AFF_BLIND) ||
			ROOM_FLAGGED(room, ROOM_SMOKE_FILLED)) {
		if (!AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY))
			return false;
	}
	if (IS_DARK(room) && !CAN_SEE_IN_DARK(ch))
		return false;

	return true;
}

bool
INVIS_OK(Creature *sub, Creature *obj)
{
	// Other players can't see invis'd immortals.  Also checks for switched
	// mobiles
	if (IS_PC(sub) && IS_PC(obj) && IS_IMMORT(sub) && !IS_IMMORT(obj))
		return true;

	if (IS_PC(sub) && IS_PC(obj) && IS_IMMORT(obj) && 
			!sub->desc &&
			GET_LEVEL(sub) < GET_INVIS_LVL(obj))
		return false;

	// Holy is the light that shines on the chosen
	if (PRF_FLAGGED(sub, PRF_HOLYLIGHT))
		return true;

	// Remort invis.  (mobs don't have it and aren't affected by it.)
	if (IS_PC(sub) && IS_PC(obj) &&
			GET_LEVEL(sub) < GET_INVIS_LVL(obj) &&
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
	if (IS_IMMORT(obj) && GET_LEVEL(sub) < GET_INVIS_LVL(obj))
		return false;

	// Mortals can't see unapproved mobs
	
	if (!MOB2_FLAGGED(sub, MOB2_UNAPPROVED) &&
			MOB2_FLAGGED(obj, MOB2_UNAPPROVED) &&
			!IS_IMMORT(sub) && !sub->isTester())
		return false;

	// Non-tester mortal players can't see testers
	if (!IS_IMMORT(sub) && !IS_NPC(sub) && !sub->isTester() && obj->isTester())
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
