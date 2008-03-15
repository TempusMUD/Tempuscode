#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include "object_map.h"
#include "utils.h"

ObjectMap::ObjectMap() : map<int, obj_data *>()
{
}

ObjectMap::~ObjectMap()
{
}

bool ObjectMap::add(obj_data *obj)
{
    int vnum = 0;
    
    if (!obj || !obj->shared) {
        mudlog(LVL_GRGOD, NRM, true, "WARNING:  Attempt to add NULL object to ObjectMap");
        return false;
    }

    if ((vnum = obj->shared->vnum) == 0) {
        mudlog(LVL_GRGOD, NRM, true, "WARNING:  Attempt to add object with vnum 0 to ObjectMap");
        return false;
    }
    if (count(vnum) > 0) {
        mudlog(LVL_GRGOD, NRM, true, "WARNING:  ObjectMap::add() tried to insert "
               "existing vnum [%d].", vnum);
        return false;
    }
    
    (*this)[vnum] = obj;

    return true;
}

bool ObjectMap::remove(obj_data *obj)
{
    int vnum = 0;

    if (!obj || !obj->shared)
        return false;

    if ((vnum = obj->shared->vnum) == 0)
        return false;

    if (count(vnum) <= 0)
        return false;

    return erase(vnum);

}

bool ObjectMap::remove(int vnum)
{
    return erase(vnum);
}

obj_data *ObjectMap::find(int vnum)
{
    if (count(vnum) <= 0)
        return NULL;

    return (*this)[vnum];
}

ObjectMap objectPrototypes;
