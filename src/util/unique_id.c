#include "unique_id.h"
#include <limits.h>

long UniqueID__topID = 0;
bool UniqueID__modified = false;

long
UniqueID_getNextID() {
    _modified = true;
    if (_topID < LONG_MAX) {
        return _topID++;
    } else {
        _topID = 0;
        return _topID;
    }
}

long
UniqueID_peekNextID() {
    return _topID;
}

void
UniqueID_setTopID(long topID) {
    _topID = topID;
}

bool
UniqueID_modified() {
    return _modified;
}

void
UniqueID_saved() {
    _modified = false;
}
