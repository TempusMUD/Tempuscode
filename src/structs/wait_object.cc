#include "executable_object.h"
#include "creature.h"
#include "constants.h"
#include "utils.h"

WaitObject::WaitObject(Creature *ch, char amount) : ExecutableObject() {
    setChar(ch);
    _wait = amount;
}

void
WaitObject::execute() {
    WAIT_STATE(_char, _wait RL_SEC);
    ExecutableObject::execute();
}
