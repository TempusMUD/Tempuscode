#include "generic_skill.h"
#include "room_data.h"
#include "comm.h"
#include "utils.h"
#include "spells.h"

extern void send_to_char(struct Creature *ch, const char *str, ...);

bool
PhysicsSpell::perform(ExecutableVector &ev, Creature *target) {
    if (!GenericSpell::perform(ev, target))
        return false;

    if (!canAlter())
        return false;

    ExecutableObject *eo = generateSkillMessage(target, NULL);
    if (eo)
        ev.insert_id(1, eo);

    return true;
}

bool
PhysicsSpell::perform(ExecutableVector &ev, obj_data *obj) {
    if (!GenericSpell::perform(ev, obj))
        return false;

    if (!canAlter())
        return false;

    ExecutableObject *eo = generateSkillMessage(NULL, obj);
    if (eo)
        ev.insert_id(1, eo);

    return true;
}

bool
PhysicsSpell::perform(ExecutableVector &ev, char *targets) {
    if (!GenericSpell::perform(ev))
        return false;

    if (!canAlter())
        return false;

    ExecutableObject *eo = generateSkillMessage(NULL, NULL);
    if (eo)
        ev.insert_id(1, eo);

    return true;
}

bool
PhysicsSpell::canAlter() {
    if (_objectMagic)
        return true;

    if (!_owner->isImmortal()) {
        if (canBeLearned(_owner) == PracticeAbility::CannotLearn) {
            send_to_char(_owner, "Alter what?!?\r\n");
            return false;
        }
        
        if (ROOM_FLAGGED(_owner->in_room, ROOM_NOSCIENCE)) {
            send_to_char(_owner, "You are unable to alter physical "
            "reality in this space.\r\n");
            act("$n tries to solve an elaborate equasion but fails..",
            false, _owner, 0, 0, TO_ROOM);
            return false;
        }
        
        if (SPELL_USES_GRAVITY(_skillNum) &&
        NOGRAV_ZONE(_owner->in_room->zone)) {
            send_to_char(_owner, "There is no gravity here to alter.\r\n");
            act("$n tries to solve an elaborate equation, but fails.",
            false, _owner, 0, 0, TO_ROOM);
            return false;
        }
    }
    
    gainSkillProficiency();

    return true;
}
ExecutableObject *
PhysicsSpell::generateSkillMessage(Creature *victim, obj_data *obj)
{
    char *toChar = NULL, *toRoom = NULL, *toVict = NULL;

    if (_objectMagic)
        return NULL;

    if (victim && victim->in_room == _owner->in_room) {
        if (victim == _owner) {
            toRoom = tmp_sprintf(
                "$n closes $s eyes and makes a calculation.");
            toChar = tmp_sprintf("You close your eyes and make a calculation.");
        }
        else {
            toRoom = tmp_sprintf(
                "$n looks at $N and makes a calculation.");
            toChar = tmp_sprintf(
                "You look at $N and make a calculation.");
            toVict = tmp_sprintf(
                "$n closes $s eyes and alters the reality around you.");
        }
    }
    else if (obj &&
        ((obj->in_room == _owner->in_room) || (obj->carried_by == _owner))) {
        toRoom = tmp_sprintf(
        "$n looks directly at $p and makes a calculation.");
        toChar = tmp_sprintf(
        "You look directly at $p and make a calculation.");
    }
    else {
        toRoom = tmp_sprintf(
        "$n closes $s eyes and slips into a deep calculation.");
        toChar = tmp_sprintf(
        "You close your eyes and slip into a deep calculation.");
    }

    ExecutableObject *eo = new ExecutableObject(getOwner());
    eo->setVictim(victim);
    if (toChar)
        eo->addToCharMessage(string(toChar));
    if (toVict)
        eo->addToVictMessage(string(toVict));
    if (toRoom)
        eo->addToRoomMessage(string(toRoom));

    return eo;
}

