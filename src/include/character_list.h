#ifndef __TEMPUS_CHARACTER_LIST_H
#define __TEMPUS_CHARACTER_LIST_H
#include <signal.h>
#include "char_data.h"
#include "safe_list.h"

class CharacterList : public SafeList<char_data*> {
    public:
        CharacterList(bool prepend = false)
            : SafeList<char_data*>(prepend) { }
        ~CharacterList() {}
        inline operator bool(){
            return size() > 0;
        }
        inline operator char_data*(){
            if( size() == 0 )
                return NULL;
            else
                return *(begin());
        }
    protected:
    private:
        // Prevent a list from ever being converted to an int
        inline operator int(){
            raise(SIGSEGV);
            return -1;
        }
};

/* prototypes for mobs		 */
extern CharacterList mobilePrototypes; 
extern CharacterList characterList;
extern CharacterList combatList;


#endif

