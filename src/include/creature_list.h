#ifndef _CREATURE_LIST_H_
#define _CREATURE_LIST_H_

#include <signal.h>
#include "safe_list.h"

class Creature;

class CreatureList:public SafeList <Creature *> {
  public:
	CreatureList(bool prepend = false)
	:SafeList <Creature *>(prepend) {
	} 
    ~CreatureList() {
	}
	inline operator bool() {
		return size() > 0;
	}
	inline operator Creature *() {
		if (size() == 0)
			return NULL;
		else
			return *(begin());
	}
  protected:
  private:
	// Prevent a list from ever being converted to an int
	inline operator int () {
		raise(SIGSEGV);
		return -1;
	}
};

/* prototypes for mobs		 */
extern CreatureList characterList;
extern map<int, Creature *> characterMap;
extern CreatureList combatList;


#endif
