#ifndef __TEMPUS_CHARACTER_LIST_H
#define __TEMPUS_CHARACTER_LIST_H
#include <signal.h>
#include "char_data.h"
#include "safe_list.h"

class CharacterList : public SafeList<char_data*> {
    public:
        CharacterList()
            : SafeList<char_data*>() { }
        ~CharacterList() {}
        SafeList<char_data*>::remove;
        SafeList<char_data*>::add;
        SafeList<char_data*>::begin;
        SafeList<char_data*>::end;
        SafeList<char_data*>::empty;
        SafeList<char_data*>::size; 
        SafeList<char_data*>::insert; 
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

