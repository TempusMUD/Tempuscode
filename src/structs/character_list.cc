#include "structs.h"
#include "character_list.h"

/**
 *  Global CharacterList objects stored here.
**/

/** The list of mobile prototypes or "virtual mobiles" **/
CharacterList mobilePrototypes; 
/** The list of in-game characters. (i.e. players & mobs) **/
CharacterList characterList;
/** The list of characters currently involved in combat **/
CharacterList combatList;

