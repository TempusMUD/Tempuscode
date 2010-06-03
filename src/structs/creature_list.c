#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include "creature_list.h"

/**
 *  Global struct creatureList objects stored here.
**/

/** The list of in-game characters. (i.e. players & mobs) **/
struct creatureList characterList(false);
std_map<int, struct creature *> characterMap;

/** The list of characters currently involved in combat **/
struct creatureList combatList(false);

/** The list of characters currently defending another character **/
struct creatureList defendingList(false);

/** The list of characters currently hunting another character **/
struct creatureList huntingList(false);

/** The list of characters currently mounted on another character **/
struct creatureList mountedList(false);
