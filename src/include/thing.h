#ifndef THING_H
#define THING_H

#include <signal.h>

class Creature;
class obj_data;
class room_data;
class zone_data;

enum thing_kind {
    CREATURE,
    OBJECT,
    ROOM,
    ZONE
};

class thing {
public:
    thing(thing_kind kind) { _kind = kind; }
    thing(const thing &o) { _kind = o._kind; }
    virtual ~thing(void) {};

    inline thing_kind kind(void) const { return _kind; }

    inline bool is_c(void) const { return _kind == CREATURE; }
    inline bool is_o(void) const { return _kind == OBJECT; }
    inline bool is_r(void) const { return _kind == ROOM; }
    inline bool is_z(void) const { return _kind == ZONE; }

    inline Creature *to_c(void) const {
        if (_kind != CREATURE)
            raise(SIGSEGV);
        return reinterpret_cast<Creature *>(const_cast<thing *>(this));
    }
    inline obj_data *to_o(void) const {
        if (_kind != OBJECT)
            raise(SIGSEGV);
        return reinterpret_cast<obj_data *>(const_cast<thing *>(this));
    }
    inline room_data *to_r(void) const {
        if (_kind != ROOM)
            raise(SIGSEGV);
        return reinterpret_cast<room_data *>(const_cast<thing *>(this));
    }
    inline zone_data *to_z(void) const {
        if (_kind != ZONE)
            raise(SIGSEGV);
        return reinterpret_cast<zone_data *>(const_cast<thing *>(this));
    }
private:
    thing_kind _kind;
};

#endif
