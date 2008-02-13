#include "executable_object.h"

using namespace std;

// Normal contstructor & destructor
ExecutableVector::ExecutableVector() {
    _maxSize = 0;
    _noToCharMessages = false;
    _noToVictMessages = false;
    _noToRoomMessages = false;
    _addOk = true;
}

ExecutableVector::~ExecutableVector() {
    for (unsigned int x = 0; x < size(); x++) {
        delete (*this)[x];
    }
}

// Copy constructor
ExecutableVector::ExecutableVector(ExecutableVector &c) {
    clear();
    ExecutableVector::iterator vi = this->begin();
    for (; vi != this->end(); ++vi)
        push_back(*vi);
    _maxSize = c.getMaxSize();
}

// Accessors
void 
ExecutableVector::setMaxSize(int maxSize) {
    _maxSize = maxSize;
}

int 
ExecutableVector::getMaxSize() {
    return _maxSize;
}

// Normal members        
void 
ExecutableVector::execute() {
    prepare();
    for (unsigned int x = 0; x < size(); x++) {
        ExecutableObject *eo = (*this)[x];
        if (eo->getExecuted())
            continue;

        if (_noToCharMessages)
            eo->getToCharMessages().clear();
        if (_noToVictMessages)
            eo->getToVictMessages().clear();
        if (_noToRoomMessages)
            eo->getToRoomMessages().clear();
        eo->execute();
        if (eo->isVictimDead()) {
            for (unsigned int y = 0; y < size(); y++) {
                ExecutableObject *eo2 = (*this)[y];
                if (!eo->getExecuted() && 
                    (eo->getVictim() == eo2->getVictim() ||
                    eo->getVictim() == eo2->getChar()))
                    eo2->cancel();
            }
        }
    }
}

void
ExecutableVector::prepare() {
    //bool prepared = true;
    while (modifiable()) {
        for (unsigned int x = 0; x < size(); x++) {
            (*this)[x]->prepare();
        }
    }

    _addOk = false;
}

bool
ExecutableVector::modifiable() {
    bool modifiable = false;
    for (unsigned int x = 0; x < size(); x++) {
        if ((*this)[x]->getModifiable()) {
            modifiable = true;
        }
    }
    return modifiable;
}

void
ExecutableVector::insert_id(unsigned pos, ExecutableObject *x) {
    if (!_addOk || (_maxSize && (size() >= (unsigned)_maxSize)))
        return;

    if (!empty())
        x->setID((*this)[0]->getID());

    insert(pos, x);
    x->setEV(this);
}

void 
ExecutableVector::insert(unsigned pos, ExecutableObject *x) {
    if (!_addOk || (_maxSize && (size() >= (unsigned)_maxSize)))
        return;

    x->setEV(this);
    unsigned counter = 0;;

    if (pos >= size()) {
        push_back(x);
        return;
    }

    ExecutableVector::iterator vi = this->begin();
    for (; vi != this->end(); ++vi) {
        if (counter == pos) {
            vector<ExecutableObject *>::insert(vi, x);
            break;
        }
        counter++;
    }
}

void
ExecutableVector::insert_id(ExecutableObject *pos, ExecutableObject *x) {
    if (!_addOk || (_maxSize && (size() >= (unsigned)_maxSize)))
        return;

    if (!empty())
        x->setID((*this)[0]->getID());

    insert(pos, x);
    x->setEV(this);
}

void 
ExecutableVector::insert(ExecutableObject *pos, ExecutableObject *x) {
    if (!_addOk || (_maxSize && (size() >= (unsigned)_maxSize)))
        return;

    x->setEV(this);
    ExecutableVector::iterator vi = this->begin();
    for (; vi != this->end(); ++vi) {
        if (*(*vi) == *pos) {
            vector<ExecutableObject *>::insert(vi, x);
            return;
        }
    }

    push_back(x);
}

void
ExecutableVector::push_front(ExecutableObject *x) {
    if (!_addOk || (_maxSize && (size() >= (unsigned)_maxSize)))
        return;

    if (x == NULL)
        return;

    x->setEV(this);
    vector<ExecutableObject*>::insert(this->begin(), x);
}

void
ExecutableVector::push_front_id(ExecutableObject *x) {
    if (!_addOk || (_maxSize && (size() >= (unsigned)_maxSize)))
        return;

    if (x == NULL)
        return;

    if (!empty()) {
        x->setID((*this)[0]->getID());
    }
    push_front(x);
    x->setEV(this);
}

void
ExecutableVector::push_back(ExecutableObject *x) {
    if (!_addOk || (_maxSize && (size() >= (unsigned)_maxSize)))
        return;

    if (x == NULL)
        return;

    x->setEV(this);
    vector<ExecutableObject *>::push_back(x);
}

void
ExecutableVector::push_back_id(ExecutableObject *x) {
    if (!_addOk || (_maxSize && (size() >= (unsigned)_maxSize)))
        return;

    if (x == NULL)
        return;

    if (!empty()) {
        x->setID((*this)[0]->getID());
    }
    push_back(x);
    x->setEV(this);
}

ExecutableObject* const 
ExecutableVector::operator[](unsigned int pos) {
    return vector<ExecutableObject *>::operator[](pos);
}

void
ExecutableVector::suppressToCharMessages(bool suppress) {
    _noToCharMessages = suppress;
}

void
ExecutableVector::suppressToVictMessages(bool suppress) {
    _noToVictMessages = suppress;
}

void
ExecutableVector::suppressToRoomMessages(bool suppress) {
    _noToRoomMessages = suppress;
}

void
ExecutableVector::suppressMessages(bool suppress) {
    suppressToCharMessages(suppress);
    suppressToVictMessages(suppress);
    suppressToRoomMessages(suppress);
}

bool
ExecutableVector::isVictimDead(Creature *ch) {
    ExecutableVector::iterator vi = begin();
    for (; vi != end(); ++vi) {
        if ((ch == (*vi)->getVictim()) && (*vi)->isVictimDead())
            return true;
    }

    return false;
}
