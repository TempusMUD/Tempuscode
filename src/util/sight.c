#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <libxml/parser.h>
#include <glib.h>

#include "utils.h"
#include "constants.h"
#include "security.h"
#include "handler.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "zone_data.h"
#include "race.h"
#include "creature.h"
#include "tmpstr.h"
#include "spells.h"
#include "obj_data.h"
#include "weather.h"

// Returns true if the room is outside and sunny, otherwise returns false
bool
room_is_sunny(struct room_data *room)
{
    int sunlight;

    // Explicitly dark
    if (ROOM_FLAGGED(room, ROOM_DARK)) {
        return false;
    }
    // Only the prime material plane has a sun
    if (!PRIME_MATERIAL_ROOM(room)) {
        return false;
    }
    // Sun doesn't reach indoors
    if (ROOM_FLAGGED(room, ROOM_INDOORS)) {
        return false;
    }
    // Or rooms that are inside
    if (SECT(room) == SECT_INSIDE) {
        return false;
    }

    // Actual sunlight check
    sunlight = room->zone->weather->sunlight;
    return (sunlight == SUN_RISE || sunlight == SUN_LIGHT);
}

// Returns true if the room is too dark to see, false if the room has enough
// light to see
bool
room_is_dark(struct room_data *room)
{
    if (!room) {
        errlog("room_is_dark() called with NULL room");
        return false;
    }

    if (!room->zone) {
        errlog("room_is_dark() called with NULL room->zone [%d]",
               room->number);
        return false;
    }

    if (!room->zone->weather) {
        errlog("room_is_dark() called with NULL room->zone->weather [%d]",
               room->number);
        return false;
    }

    if (room->light) {
        return false;
    }
    if (SECT(room) == SECT_ELEMENTAL_OOZE) {
        return true;
    }
    if (ROOM_FLAGGED(room, ROOM_DARK)) {
        return true;
    }
    if (!PRIME_MATERIAL_ROOM(room)) {
        return false;
    }
    if (ROOM_FLAGGED(room, ROOM_INDOORS)) {
        return false;
    }
    if (SECT(room) == SECT_INSIDE) {
        return false;
    }
    if (SECT(room) == SECT_VEHICLE) {
        return false;
    }
    if (SECT(room) == SECT_CITY) {
        return false;
    }

    return !room_is_sunny(room);
}

// Opposite of room_is_dark()
bool
room_is_light(struct room_data *room)
{
    return !room_is_dark(room);
}

// Returns true if the player can see at all, regardless of other influences
bool
check_sight_self(struct creature *self)
{
    return !AFF_FLAGGED(self, AFF_BLIND) ||
           AFF3_FLAGGED(self, AFF3_SONIC_IMAGERY);
}

bool
has_dark_sight(struct creature *self)
{
    return (AFF_FLAGGED(self, AFF_INFRAVISION) ||
            PRF_FLAGGED(self, PRF_HOLYLIGHT) ||
            AFF3_FLAGGED(self, AFF3_SONIC_IMAGERY) ||
            AFF_FLAGGED(self, AFF_RETINA) ||
            CHECK_SKILL(self, SKILL_NIGHT_VISION) ||
            HAS_RACIAL_NIGHT_VISION(self));
}

// Returns true if the player can see in the room
bool
check_sight_room(struct creature *self, struct room_data *room)
{
    if (!room) {
        errlog("check_sight_room() called with NULL room");
        return false;
    }

    if (room_is_dark(room) && !has_dark_sight(self)) {
        return false;
    }

    if (ROOM_FLAGGED(room, ROOM_SMOKE_FILLED) &&
        !AFF3_FLAGGED(self, AFF3_SONIC_IMAGERY)) {
        return false;
    }

    return true;
}

// Returns true if a creature can see an object
bool
check_sight_object(struct creature *self, struct obj_data *obj)
{
    if (PRF_FLAGGED(self, PRF_HOLYLIGHT)) {
        return true;
    }

    if (!OBJ_APPROVED(obj) && !NPC2_FLAGGED(self, NPC2_UNAPPROVED) &&
        !IS_IMMORT(self) && !is_authorized(self, TESTER, NULL)) {
        return false;
    }

    if (IS_OBJ_STAT(obj, ITEM_TRANSPARENT) &&
        !(AFF3_FLAGGED(self, AFF3_SONIC_IMAGERY) ||
          AFF_FLAGGED(self, AFF_RETINA) ||
          affected_by_spell(self, ZEN_AWARENESS))) {
        return false;
    }

    if (AFF_FLAGGED(self, AFF_DETECT_INVIS) ||
        AFF2_FLAGGED(self, AFF2_TRUE_SEEING)) {
        return true;
    }

    if (IS_OBJ_STAT(obj, ITEM_INVISIBLE)) {
        return false;
    }

    return true;
}

// Returns true if a creature can see another creature
bool
check_sight_vict(struct creature *self, struct creature *vict)
{
    // Immortals players can always see non-immortal players
    if (IS_IMMORT(self) && !IS_IMMORT(vict)) {
        return true;
    }

    // Nothing at all gets through immort invis
    if (IS_PC(self) && IS_IMMORT(vict)
        && GET_LEVEL(self) < GET_INVIS_LVL(vict)) {
        return false;
    }

    // Mortals can't see unapproved mobs
    if (!NPC2_FLAGGED(self, NPC2_UNAPPROVED) &&
        NPC2_FLAGGED(vict, NPC2_UNAPPROVED) &&
        !IS_IMMORT(self) && !is_authorized(self, TESTER, NULL)) {
        return false;
    }

    // Non-tester mortal players can't see testers
    if (!IS_IMMORT(self)
        && !IS_NPC(self)
        && !IS_IMMORT(vict)
        && !is_authorized(self, TESTER, NULL)
        && is_authorized(vict, TESTER, NULL)) {
        return false;
    }

    // Holy is the light that shines on the chosen
    if (PRF_FLAGGED(self, PRF_HOLYLIGHT)) {
        return true;
    }

    // Newbies can see all players.
    if (GET_LEVEL(self) <= 6 && GET_REMORT_GEN(self) == 0 && IS_PC(vict)) {
        return true;
    }

    // Sonic imagery and retina detects transparent creatures
    if (AFF2_FLAGGED(vict, AFF2_TRANSPARENT) &&
        !(AFF3_FLAGGED(self, AFF3_SONIC_IMAGERY) ||
          AFF_FLAGGED(self, AFF_RETINA) ||
          affected_by_spell(self, ZEN_AWARENESS))) {
        return false;
    }

    // True seeing and detect invisibility counteract all magical invis
    if (AFF2_FLAGGED(self, AFF2_TRUE_SEEING) ||
        AFF_FLAGGED(self, AFF_DETECT_INVIS)) {
        return true;
    }

    // Invis/Transparent
    if (AFF_FLAGGED(vict, AFF_INVISIBLE)) {
        return false;
    }

    // Invis to Undead
    if (IS_UNDEAD(self) && AFF2_FLAGGED(vict, AFF2_INVIS_TO_UNDEAD)) {
        return false;
    }

    return true;
}

bool
can_see_creature(struct creature *self, struct creature *vict)
{
    // Can always see self
    if (self == vict) {
        return true;
    }

    // Immortals players can always see non-immortal players
    if (IS_IMMORT(self) && !IS_IMMORT(vict)) {
        return true;
    }

    // only immortals can see utility mobs
    if (IS_NPC(vict) && NPC_FLAGGED(vict, NPC_UTILITY) && !IS_IMMORT(self)) {
        return false;
    }
    // Nothing at all gets through immort invis
    if (IS_IMMORT(vict) && GET_LEVEL(self) < GET_INVIS_LVL(vict)) {
        return false;
    }

    if (PRF_FLAGGED(self, PRF_HOLYLIGHT)) {
        return true;
    }

    if (!check_sight_self(self)) {
        return false;
    }

    if (!check_sight_room(self, vict->in_room)) {
        return false;
    }

    if (!check_sight_vict(self, vict)) {
        return false;
    }

    return true;
}

bool
can_see_object(struct creature *self, struct obj_data *obj)
{
    // If they can't see the object itself, none of the rest of it is going
    // to matter much
    if (!check_sight_object(self, obj)) {
        return false;
    }

    // After the object itself, we only care about the outermost object
    while (obj->in_obj) {
        obj = obj->in_obj;
    }

    // If the object is being carried by someone, it also inherits visibility
    if (obj->carried_by) {
        if (obj->carried_by == self) {
            return true;
        } else {
            return can_see_creature(self, obj->carried_by);
        }
    }
    // If the object is being worn by someone, it inherits their visibility
    if (obj->worn_by) {
        if (obj->worn_by == self) {
            return true;
        } else {
            return can_see_creature(self, obj->worn_by);
        }
    }
    // If they are carrying or wearing the item, they can see it even if blind
    if (!check_sight_self(self)) {
        return false;
    }

    // It must be in a room
    if (!check_sight_room(self, obj->in_room)) {
        return false;
    }

    return true;
}

bool
can_see_room(struct creature *self, struct room_data *room)
{
    if (!check_sight_self(self)) {
        return false;
    }

    return check_sight_room(self, room);
}
