#include "executable_object.h"
#include "generic_affect.h"
#include "utils.h"
#include "comm.h"

extern char *NOEFFECT;

AffectObject::AffectObject(string affectClass, Creature *owner, 
        Creature *target) : ExecutableObject(owner) {
    CreatureAffect *cast_aff = NULL;

    _char = owner;
    _victim = target;

    _affect = GenericAffect::constructAffect(affectClass);
    _affect->setOwner(owner->getIdNum());
    
    // If you're using this invocation of the constructor the
    // following should always be true.
    if ((cast_aff = dynamic_cast<CreatureAffect *>(_affect))) {
        cast_aff->setTarget(target);
    }
    else {
        slog("ERROR:  Bad cast in %s!", __FUNCTION__);
    }
}

AffectObject::AffectObject(string affectClass, Creature *owner, 
        obj_data *target) : ExecutableObject(owner) {
    ObjectAffect *cast_aff = NULL;

    _char = owner;
    _obj = target;

    _affect = GenericAffect::constructAffect(affectClass);
    _affect->setOwner(owner->getIdNum());
    
    // If you're using this invocation of the constructor the
    // following should always be true.
    if ((cast_aff = dynamic_cast<ObjectAffect *>(_affect))) {
        cast_aff->setTarget(target);
    }
    else {
        slog("ERROR:  Bad cast in %s!", __FUNCTION__);
    }
}

AffectObject::AffectObject(string affectClass, Creature *owner, 
        room_data *target) : ExecutableObject(owner) {
    RoomAffect *cast_aff = NULL;

    _char = owner;
    _room = target;

    _affect = GenericAffect::constructAffect(affectClass);
    _affect->setOwner(owner->getIdNum());
    
    // If you're using this invocation of the constructor the
    // following should always be true.
    if ((cast_aff = dynamic_cast<RoomAffect *>(_affect))) {
        cast_aff->setTarget(target);
    }
    else {
        slog("ERROR:  Bad cast in %s!", __FUNCTION__);
    }
}

void
AffectObject::cancel() {
    delete _affect;
    ExecutableObject::cancel();
}
void
AffectObject::execute() {
    if (getExecuted())
        return;
    
    if (!_victim && !_obj && !_room) {
        errlog("No _victim in AffectObject::execute()");
        return;
    }

    if (!_affect->getOwner())
        _affect->setOwner(getChar()->getIdNum());

    if (!_affect->affectJoin()) {
        // act() can't send messages to people who aren't currently
        // in a room
        getToCharMessages().clear();
        if (getChar() && getChar()->in_room)
            addToCharMessage(NOEFFECT);
        getToVictMessages().clear();
        if (getChar() && getVictim() && getChar() != getVictim() && getVictim()->in_room) {
            addToVictMessage(NOEFFECT);
        }
        getToRoomMessages().clear();
        if (getChar() && getChar()->in_room)
            addToRoomMessage(NOEFFECT);
    }

    ExecutableObject::execute(); 
}

// **********************************************************
//
// RemoveAffectObject Implementation
//
// **********************************************************

RemoveAffectObject::RemoveAffectObject(string affectClass, Creature *owner, 
        Creature *target) : ExecutableObject(owner) {
    GenericAffect *aff = NULL;

    _char = owner;
    _victim = target;

    aff = GenericAffect::constructAffect(affectClass);
    
    if (aff) {
        _affectNum = aff->getSkillNum();
        delete aff;
    }
}

RemoveAffectObject::RemoveAffectObject(string affectClass, Creature *owner, 
        obj_data *target) : ExecutableObject(owner) {
    GenericAffect *aff = NULL;

    _char = owner;
    _obj = target;

    aff = GenericAffect::constructAffect(affectClass);
    
    if (aff) {
        _affectNum = aff->getSkillNum();
        delete aff;
    }
}

RemoveAffectObject::RemoveAffectObject(short skillNum, Creature *owner, 
        Creature *target) : ExecutableObject(owner) {
    _char = owner;
    _victim = target;

    _affectNum = skillNum;
}

RemoveAffectObject::RemoveAffectObject(short skillNum, Creature *owner, 
        obj_data *target) : ExecutableObject(owner) {
    _char = owner;
    _obj = target;

    _affectNum = skillNum;
}

void
RemoveAffectObject::execute() {
    CreatureAffect *aff;

    if (getExecuted())
        return;
    
    if (!_victim) {
        errlog("No _victim in RemoveAffectObject::execute()");
        return;
    }

    if (_affectNum == 0) {
        errlog("_affectNum == 0 in AffectObject::execute()");
        return;
    }

    if ((aff = (CreatureAffect *)_victim->affectedBy(_affectNum)) &&
            !dynamic_cast<MobAffect *>(aff) && !dynamic_cast<EquipAffect *>(aff)) {
        aff->affectModify(false);
        _victim->getAffectList()->remove(aff);
    }
    else {
        getToCharMessages().clear();
        if (getChar()->in_room)
            addToCharMessage(NOEFFECT);
        getToVictMessages().clear();
        if (getChar() != getVictim() && getVictim()->in_room) {
            addToVictMessage(NOEFFECT);
        }
        getToRoomMessages().clear();
        if (getChar()->in_room)
            addToRoomMessage(NOEFFECT);
    }

    ExecutableObject::execute(); 
}
