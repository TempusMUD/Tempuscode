#ifndef _MOBILE_MAP_H
#define _MOBILE_MAP_H

#include <map>
#include "creature.h"

using namespace std;

class MobileMap : public map<int, Creature *>
{
    public:
        MobileMap();
        ~MobileMap();

        bool add(Creature *ch);
        bool remove(Creature *ch);
        bool remove(int vnum);
        Creature *find(int vnum);

    private:
};

extern MobileMap mobilePrototypes;

#endif
