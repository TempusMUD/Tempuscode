#ifndef _OBJECT_MAP_H
#define _OBJECT_MAP_H

#include <obj_data.h>
#include <map>

using namespace std;

class ObjectMap : public map<int, obj_data *>
{
    public:
        ObjectMap();
        ~ObjectMap();

        bool add(obj_data *obj);
        bool remove(obj_data *ch);
        bool remove(int vnum);
        obj_data *find(int vnum);

    private:
};

extern ObjectMap objectPrototypes;

#endif
