#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include "mobile_map.h"
#include "utils.h"

MobileMap::MobileMap() : map<int, Creature *>()
{
}

MobileMap::~MobileMap()
{
}

bool MobileMap::add(Creature *ch)
{
    int vnum = 0;
    
    if (!ch || !ch->mob_specials.shared)
        return false;

    if ((vnum = ch->mob_specials.shared->vnum) == 0)
        return false;

    if (count(vnum) > 0) {
        mudlog(LVL_GRGOD, NRM, true, "WARNING:  MobileMap::add() tried to insert "
               "existing vnum [%d].", vnum);
        return false;
    }
    
    (*this)[vnum] = ch;

    return true;
}

bool MobileMap::remove(Creature *ch)
{
    int vnum = 0;

    if (!ch || !ch->mob_specials.shared)
        return false;

    if ((vnum = ch->mob_specials.shared->vnum) == 0)
        return false;

    if (count(vnum) <= 0)
        return false;

    return erase(vnum);

}

bool MobileMap::remove(int vnum)
{
    return erase(vnum);
}

Creature *MobileMap::find(int vnum)
{
    if (count(vnum) <= 0)
        return NULL;

    return (*this)[vnum];

}

MobileMap mobilePrototypes;
