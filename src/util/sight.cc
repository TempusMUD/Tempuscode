#include <unistd.h>
#include <stdlib.h>
#include "defs.h"
#include "creature.h"
#include "utils.h"
#include "spells.h"
#include "handler.h"

// Returns true if the room is too dark to see, false if the room has enough
// light to see
bool
room_is_dark(room_data *room)
{
	int sunlight;

    if (room->light)
        return false;
    if (SECT(room) == SECT_ELEMENTAL_OOZE)
        return true;
    if (ROOM_FLAGGED(room, ROOM_DARK))
        return true;
    if (!PRIME_MATERIAL_ROOM(room))
        return false;
    if (ROOM_FLAGGED(room, ROOM_INDOORS))
        return false;
    if (SECT(room) == SECT_INSIDE)
        return false;
    if (SECT(room) == SECT_VEHICLE)
        return false;
    if (SECT(room) == SECT_CITY)
        return false;

    sunlight = room->zone->weather->sunlight;
    return (sunlight == SUN_SET || sunlight == SUN_DARK);
}

// Opposite of room_is_dark()
bool
room_is_light(room_data *room)
{
	return !room_is_dark(room);
}

// Returns true if the creature possesses infravision
bool
has_infravision(Creature *ch)
{
	return (IS_AFFECTED(ch, AFF_INFRAVISION) ||
		(GET_RACE(ch) == RACE_ELF) ||     
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

// Returns true if the player can see at all, regardless of other influences
bool
check_sight_self(Creature *self)
{
	return !IS_AFFECTED(self, AFF_BLIND) ||
			AFF3_FLAGGED(self, AFF3_SONIC_IMAGERY);
}

bool
has_dark_sight(Creature *self)
{
	return (has_infravision(self) ||
			PRF_FLAGGED(self, PRF_HOLYLIGHT) ||
			AFF3_FLAGGED(self, AFF3_SONIC_IMAGERY) ||
			AFF_FLAGGED(self, AFF_RETINA) ||
			CHECK_SKILL(self, SKILL_NIGHT_VISION));
}

// Returns true if the player can see in the room
bool
check_sight_room(Creature *self, room_data *room)
{
	if (!room) {
		errlog("check_sight_room() called with NULL room");
		return false;
	}

	if (room_is_dark(room) && !has_dark_sight(self))
		return false;

	if (ROOM_FLAGGED(room, ROOM_SMOKE_FILLED) &&
			!AFF3_FLAGGED(self, AFF3_SONIC_IMAGERY))
		return false;
	
	return true;
}

// Returns true if a creature can see an object
bool
check_sight_object(Creature *self, obj_data *obj)
{
	if (PRF_FLAGGED(self, PRF_HOLYLIGHT))
		return true;

	if (!OBJ_APPROVED(obj) && !MOB2_FLAGGED(self, MOB2_UNAPPROVED) &&
			!IS_IMMORT(self) && !self->isTester())
		return false;

	if (IS_OBJ_STAT(obj, ITEM_TRANSPARENT) &&
			!(IS_AFFECTED_3(self, AFF3_SONIC_IMAGERY) ||
				IS_AFFECTED(self, AFF_RETINA) ||
				affected_by_spell(self, ZEN_AWARENESS)))
		return false;

	if (IS_AFFECTED(self, AFF_DETECT_INVIS) ||
			IS_AFFECTED_2(self, AFF2_TRUE_SEEING))
		return true;

	if (IS_OBJ_STAT(obj, ITEM_INVISIBLE))
		return false;

	return true;
}

// Returns true if a creature can see another creature
bool
check_sight_vict(Creature *self, Creature *vict)
{
	// Immortals players can always see non-immortal players
	if (IS_IMMORT(self) && !IS_IMMORT(vict))
		return true;

	// Nothing at all gets through immort invis
	if (IS_PC(self) && IS_IMMORT(vict) && GET_LEVEL(self) < GET_INVIS_LVL(vict))
		return false;

	// Mortals can't see unapproved mobs
	if (!MOB2_FLAGGED(self, MOB2_UNAPPROVED) &&
			MOB2_FLAGGED(vict, MOB2_UNAPPROVED) &&
			!IS_IMMORT(self) && !self->isTester())
		return false;

	// Non-tester mortal players can't see testers
	if (!IS_IMMORT(self) && !IS_NPC(self) &&
			!self->isTester() && vict->isTester())
		return false;

	// Holy is the light that shines on the chosen
	if (PRF_FLAGGED(self, PRF_HOLYLIGHT))
		return true;

	// Sonic imagery and retina detects transparent creatures
	if (IS_AFFECTED_2(vict, AFF2_TRANSPARENT) &&
			!(IS_AFFECTED_3(self, AFF3_SONIC_IMAGERY) ||
				IS_AFFECTED(self, AFF_RETINA) ||
				affected_by_spell(self, ZEN_AWARENESS)))
		return false;

	// True seeing and detect invisibility counteract all magical invis
	if (IS_AFFECTED_2(self, AFF2_TRUE_SEEING) ||
			AFF_FLAGGED(self, AFF_DETECT_INVIS))
		return true;

	// Invis/Transparent
	if (IS_AFFECTED(vict, AFF_INVISIBLE))
		return false;

	// Invis to Undead
	if (IS_UNDEAD(self) && IS_AFFECTED_2(vict, AFF2_INVIS_TO_UNDEAD))
		return false;

	return true;
}

bool
can_see_creature(Creature *self, Creature *vict)
{
	// Can always see self
	if (self == vict)
		return true;

	// Immortals players can always see non-immortal players
	if (IS_IMMORT(self) && !IS_IMMORT(vict))
		return true;

	// Nothing at all gets through immort invis
	if (IS_IMMORT(vict) && GET_LEVEL(self) < GET_INVIS_LVL(vict))
		return false;

	if (PRF_FLAGGED(self, PRF_HOLYLIGHT))
		return true;

	if (!check_sight_self(self))
		return false;

	if (!check_sight_room(self, vict->in_room))
		return false;

	if (!check_sight_vict(self, vict))
		return false;

	return true;
}

bool
can_see_object(Creature *self, obj_data *obj)
{
	// If they can't see the object itself, none of the rest of it is going
	// to matter much
	if (!check_sight_object(self, obj))
		return false;

	// After the object itself, we only care about the outermost object
	while (obj->in_obj)
		obj = obj->in_obj;
	
	// If the object is being carried by someone, it also inherits visibility
	if (obj->carried_by) {
		if (obj->carried_by == self)
			return true;
		else
			return can_see_creature(self, obj->carried_by);
	}

	// If the object is being worn by someone, it inherits their visibility
	if (obj->worn_by) {
		if (obj->worn_by == self)
			return true;
		else
			return can_see_creature(self, obj->worn_by);
	}

	// If they are carrying or wearing the item, they can see it even if blind
	if (!check_sight_self(self))
		return false;

	// It must be in a room
	if (!check_sight_room(self, obj->in_room))
		return false;

	return true;
}

bool
can_see_room(Creature *self, room_data *room)
{
	if (!check_sight_self(self))
		return false;

	return check_sight_room(self, self->in_room);
}
