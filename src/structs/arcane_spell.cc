#include "generic_skill.h"
#include "creature.h"
#include "utils.h"
#include "comm.h"

extern void send_to_char(struct Creature *ch, const char *str, ...);

bool
ArcaneSpell::perform(ExecutableVector &ev, Creature *target) {
    if (!GenericSpell::perform(ev, target))
        return false;

    if (!canCast())
        return false;

    ExecutableObject *eo = generateSkillMessage(target, NULL);
    if (eo)
        ev.insert_id(1, eo);

    return true;
}

bool
ArcaneSpell::perform(ExecutableVector &ev, obj_data *obj) {
    if (!GenericSpell::perform(ev, obj))
        return false;
    
    if (!canCast())
        return false;

    ExecutableObject *eo = generateSkillMessage(NULL, obj);
    if (eo)
        ev.insert_id(1, eo);

    return true;
}

bool
ArcaneSpell::perform(ExecutableVector &ev, char *targets) {
    if (!GenericSpell::perform(ev))
        return false;

    if (!canCast())
        return false;

    ExecutableObject *eo = generateSkillMessage(NULL, NULL);
    if (eo)
        ev.insert_id(1, eo);

    return true;

}

bool
ArcaneSpell::canCast() {
    if (!_owner->isImmortal()) {
        if (canBeLearned(_owner) == PracticeAbility::CannotLearn) {
            send_to_char(_owner, "Cast what?!?\r\n");
            return false;
        }
        
        if (IS_WEARING_W(_owner) > (CAN_CARRY_W(_owner) * 0.90)) {
            send_to_char(_owner, "Your equipment is too heavy and bulky "
            "for you to cast anything useful!\r\n");
            return false;
        }
        
        if (GET_EQ(_owner, WEAR_WIELD) &&
        IS_OBJ_STAT2(GET_EQ(_owner, WEAR_WIELD), ITEM2_TWO_HANDED)) {
            send_to_char(_owner,
            "You can't cast spells while wielding a two handed weapon!\r\n");
            return false;
        }
        
        if (ROOM_FLAGGED(_owner->in_room, ROOM_NOMAGIC)) {
            send_to_char(_owner, "Your magic fizzles out and dies.\r\n");
            act("$n's magic fizzles out and dies.", false, _owner, 
            0, 0, TO_ROOM);
            return false;
        }
    }
    
    gainSkillProficiency();

    return true;
}
