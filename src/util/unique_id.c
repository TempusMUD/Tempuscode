#include "unique_id.h"
#include <limits.h>

long UniqueID::_topID = 0;
bool UniqueID::_modified = false;

long
UniqueID::getNextID() {
    _modified = true;
    if (_topID < LONG_MAX) {
        return _topID++;
    } else {
        _topID = 0;
        return _topID;
    }
}

long
UniqueID::peekNextID() {
    return _topID;
}

void
UniqueID::setTopID(long topID) {
    _topID = topID;
}

bool
UniqueID::modified() {
    return _modified;
}

void
UniqueID::saved() {
    _modified = false;
}
