#ifndef _CREATURE_LIST_H_
#define _CREATURE_LIST_H_

#include <signal.h>
#include "creature.h"
#include "safe_list.h"

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

class CombatDataList : public SafeList <CharCombat> {
    public:
        CombatDataList(bool prepend = false) : SafeList <CharCombat>(prepend) {
        }
        ~CombatDataList() {
        }

        void clear() {
            while (size > 0) {
                remove(*(begin()));
            }
        }

        void add_front(CharCombat c) {
            push_front(c);
        }

        void add_back(CharCombat c) {
            push_back(c);
        }

    private:
        inline operator int () {
            raise(SIGSEGV);
            return -1;
        }
};

/* prototypes for mobs		 */
extern CreatureList characterList;
extern CreatureList combatList;


#endif
