#ifndef _COMBAT_DATA_LIST_H_
#define _COMBAT_DATA_LIST_H_

#include <signal.h>
#include "safe_list.h"
#include "structs.h"

class CharCombat {
    private:
        CharCombat() {
            _initiated = false;
            _opponent = NULL;
        }
    public:
        CharCombat(Creature *ch, bool initiated) {
            _initiated = initiated;
            _opponent = ch;
        }
        CharCombat(const CharCombat &a) {
            this->_initiated = a._initiated;
            this->_opponent = a._opponent;
        }
        inline bool getInitiated() { 
            return _initiated; 
        }
        inline void setInitiated(bool init) { 
            _initiated = init;
        }
        inline void setOpponent(Creature *ch) { 
            _opponent = ch; 
        } 
        inline Creature *getOpponent() { 
            return _opponent; 
        }
        bool operator==( const Creature* c ) {
                   return _opponent == c;
        }
        bool operator==( const CharCombat& c ) {
            return _initiated == c._initiated &&
                   _opponent == c._opponent;
        }

    private:
        bool _initiated;
        struct Creature *_opponent;
};

class CombatDataList : public SafeList <CharCombat> {
    public:
        CombatDataList(bool prepend = false) : SafeList <CharCombat>(prepend) {
        }
        ~CombatDataList() {
        }

        void clear() {
            while (size() > 0) {
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

#endif
