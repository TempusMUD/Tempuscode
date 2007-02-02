#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include "creature_list.h"

/**
 *  Global CreatureList objects stored here.
**/

/** The list of in-game characters. (i.e. players & mobs) **/
CreatureList characterList(false);

/** The list of characters currently involved in combat **/
CreatureList combatList(false);

/** The list of characters currently defending another character **/
CreatureList defendingList(false);

/** The list of characters currently hunting another character **/
CreatureList huntingList(false);

/** The list of characters currently mounted on another character **/
CreatureList mountedList(false);
