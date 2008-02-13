#include "generic_skill.h"
#include "room_data.h"
#include "comm.h"
#include "utils.h"

extern void send_to_char(struct Creature *ch, const char *str, ...);

bool
PsionicSpell::perform(ExecutableVector &ev, Creature *target) {
    if (!GenericSpell::perform(ev, target))
        return false;
    
    if (!canTrigger())
        return false;

    ExecutableObject *eo = generateSkillMessage(target, NULL);
    if (eo)
        ev.insert_id(1, eo);

    return true;
}

bool
PsionicSpell::perform(ExecutableVector &ev, obj_data *obj) {
    if (!GenericSpell::perform(ev, obj))
        return false;

    if (!canTrigger())
        return false;

    ExecutableObject *eo = generateSkillMessage(NULL, obj);
    if (eo)
        ev.insert_id(1, eo);

    return true;
}

bool
PsionicSpell::perform(ExecutableVector &ev, char *targets) {
    if (!GenericSpell::perform(ev))
        return false;
    
    if (!canTrigger())
        return false;

    ExecutableObject *eo = generateSkillMessage(NULL, NULL);
    if (eo)
        ev.insert_id(1, eo);

    return true;
}


bool
PsionicSpell::canTrigger() {
    if (_objectMagic)
        return true;

    if (!_owner->isImmortal()) {
        if (canBeLearned(_owner) == PracticeAbility::CannotLearn) {
            send_to_char(_owner, "Trigger what?!?\r\n");
            return false;
        }

        if (ROOM_FLAGGED(_owner->in_room, ROOM_NOPSIONICS)) {
            send_to_char(_owner, "You cannot establish a mental link.\r\n");
            act("$n appears to be psionically challenged.", false, _owner,
                0, 0, TO_ROOM);
            return false;
        }
    }

    gainSkillProficiency();

    return true;
}
ExecutableObject *
PsionicSpell::generateSkillMessage(Creature *victim, obj_data *obj)
{
    char *toChar = NULL, *toRoom = NULL, *toVict = NULL;

    if (_objectMagic)
        return NULL;

    if (victim && victim->in_room == _owner->in_room) {
        if (victim == _owner) {
            toRoom = tmp_sprintf(
            "$n momentarily closes $s eyes and concentrates.");
            toChar = tmp_sprintf("You close your eyes and concentrate.");
        }
        else {
            toRoom = tmp_sprintf(
            "$n closes $s eyes and touches $N with a mental finger.");
            toChar = tmp_sprintf(
            "You close your eyes and touch $N with a mental finger.");
            toVict = tmp_sprintf(
            "$n closes $s eyes and connects with your mind.");
        }
    }
    else if (obj &&
        ((obj->in_room == _owner->in_room) || (obj->carried_by == _owner))) {
        toRoom = tmp_sprintf(
        "$n closes $s eyes and touches $p with a mental finger.");
        toChar = tmp_sprintf(
        "You close your eyes and youch $p with a mental finger.");
    }
    else {
        toRoom = tmp_sprintf(
        "$n closes $s eyes and slips into the psychic world.");
        toChar = tmp_sprintf(
        "You close your eyes and slip into the psychic world.");
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
