#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include "object_map.h"
#include "utils.h"

ObjectMap_ObjectMap() : map<int, obj_data *>()
{
}

ObjectMap_~ObjectMap()
{
}

bool ObjectMap_add(obj_data *obj)
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
        mudlog(LVL_GRGOD, NRM, true, "WARNING:  ObjectMap_add() tried to insert "
               "existing vnum [%d].", vnum);
        return false;
    }

    (*this)[vnum] = obj;

    return true;
}

bool ObjectMap_remove(obj_data *obj)
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

bool ObjectMap_remove(int vnum)
{
    return erase(vnum);
}

obj_data *ObjectMap_find(int vnum)
{
    if (count(vnum) <= 0)
        return NULL;

    return (*this)[vnum];
}

ObjectMap objectPrototypes;
