#include "generic_skill.h"
#include "room_data.h"
#include "utils.h"
#include "comm.h"

extern void send_to_char(struct Creature *ch, const char *str, ...);

bool
DivineSpell::perform(ExecutableVector &ev, Creature *target) {
    if (!GenericSpell::perform(ev))
        return false;

    if (!canCast())
        return false;

    ExecutableObject *eo = generateSkillMessage(target, NULL);
    if (eo)
        ev.insert_id(1, eo);

    return true;
}

bool
DivineSpell::perform(ExecutableVector &ev, obj_data *obj) {
    if (!GenericSpell::perform(ev))
        return false;

    if (!canCast())
        return false;

    ExecutableObject *eo = generateSkillMessage(NULL, obj);
    if (eo)
        ev.insert_id(1, eo);
    return true;
}

bool
DivineSpell::perform(ExecutableVector &ev, char *targets) {
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
DivineSpell::canCast() {
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
            act("$n's magic fizzles out and dies.", false, _owner, 0, 0, TO_ROOM);
            return false;
        }
        
        if (_owner->getLevel() > 3 && _owner->isImmortal() && !_owner->isNPC() &&
        (_owner->isCleric() || _owner->isPaladin())) {
            bool need_symbol = true;
            int gen = _owner->getGen();
            struct obj_data *holy_symbol = NULL;
            
            if (IS_SOULLESS(_owner)) {
                if (_owner->isPrimeNecromancer() && gen > 4)
                    need_symbol = false;
                if (_owner->isPrimePaladin() && gen > 6)
                    need_symbol = false;
            }
            if (need_symbol) {
                for (int i = 0; i < NUM_WEARS; i++) {
                    if (_owner->equipment[i]) {
                        if (GET_OBJ_TYPE(_owner->equipment[i]) == 
                        ITEM_HOLY_SYMB) {
                            holy_symbol = _owner->equipment[i];
                            break;
                        }
                    }
                }
                if (!holy_symbol) {
                    send_to_char(_owner,
                    "You do not even wear the symbol of your faith!\r\n");
                    return false;
                }
                if ((_owner->isGood() && (GET_OBJ_VAL(holy_symbol, 0) == 2)) ||
                (_owner->isEvil() && (GET_OBJ_VAL(holy_symbol, 0) == 0))) {
                    send_to_char(_owner, "You are not aligned with "
                    "your holy symbol!\r\n ");
                    return false;
                }
                if (_owner->getPrimaryClass() != GET_OBJ_VAL(holy_symbol, 1) &&
                _owner->getSecondaryClass() != GET_OBJ_VAL(holy_symbol, 1)) {
                    send_to_char(_owner, "The holy symbol you wear is not "
                    "of your faith!\r\n");
                    return false;
                }
                if (_owner->getLevel() < GET_OBJ_VAL(holy_symbol, 2)) {
                    send_to_char(_owner,
                    "You are not powerful enough to utilize your "
                    "holy symbol!\r\n");
                    return false;
                }
                if (_owner->getLevel() > GET_OBJ_VAL(holy_symbol, 3)) {
                    act("$p will no longer support your power!",
                    FALSE, _owner, holy_symbol, 0, TO_CHAR);
                    return false;
                }
            }
        }
    } 
    
    gainSkillProficiency();

    return true;
}


