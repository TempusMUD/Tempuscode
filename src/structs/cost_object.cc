#include "executable_object.h"
#include "creature.h"
#include "comm.h"

CostObject::CostObject() : ExecutableObject() {
    _mana = 0;
    _move = 0;
    _hit = 0;
}

int
CostObject::getMana() const {
    return _mana;
}

int
CostObject::getMove() const {
    return _move;
}

int
CostObject::getHit() const {
    return _hit;
}

void
CostObject::setMana(int mana) {
    _mana = mana;
}

void
CostObject::setMove(int move) {
    _move = move;
}

void
CostObject::setHit(int hit) {
    _hit = hit;
}

void
CostObject::execute() {
    if (getExecuted())
        return;
    
    if (getChar()->getMove() > getMove() && 
        getChar()->getMana() > getMana() &&
        getChar()->getHit() > getHit()) 
    {
        getChar()->setMove(getChar()->getMove() - this->getMove());
        getChar()->setMana(getChar()->getMana() - this->getMana());
        getChar()->setHit(getChar()->getHit() - this->getHit());
    } else {
        if (getChar()->getMove() > getMove())
            send_to_char(this->getChar(), "You are too tired to take this action.\r\n");
        else if (getChar()->getMana() > getMana())
            send_to_char(this->getChar(), "You lack the spiritual strength to take this action.\r\n");
        else if (getChar()->getHit() > getHit())
            send_to_char(this->getChar(), "You are not healthy enough to take this action.\r\n");
        this->cancelAll();
    }
    ExecutableObject::execute();
}
