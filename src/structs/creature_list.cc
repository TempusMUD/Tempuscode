#include "structs.h"
#include "creature_list.h"

/**
 *  Global CreatureList objects stored here.
**/

/** The list of in-game characters. (i.e. players & mobs) **/
CreatureList characterList(false);
/** The list of characters currently involved in combat **/
CreatureList combatList(false);
