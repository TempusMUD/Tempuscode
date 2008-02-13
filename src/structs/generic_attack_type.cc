#include "executable_object.h"
#include "generic_skill.h"
#include "utils.h"
#include "comm.h"

extern int choose_random_limb(Creature *victim);
extern int invalid_char_class(struct Creature *ch, struct obj_data *obj);

DamageObject *
GenericAttackType::generate(obj_data *weap, Creature *target) {
    if (!weap)
        return false;
    
    DamageObject *damage = new DamageObject(getOwner(),target);
    damage->setGenMessages(true);
    damage->setWeapon(weap);
    
    int dam = 0;
    
    if (IS_OBJ_TYPE(weap, ITEM_WEAPON)) {
        dam += dice(GET_OBJ_VAL(weap, 1), GET_OBJ_VAL(weap, 2));
        if (invalid_char_class(this->getOwner(), weap)) {
            dam >>= 1;
        }
        dam += str_app[STRENGTH_APPLY_INDEX(this->getOwner())].todam;
        dam += this->getOwner()->getDamroll();
        if (this->getOwner()->isNPC()) {
            dam += dice(this->getOwner()->mob_specials.shared->damnodice,
            this->getOwner()->mob_specials.shared->damsizedice);
        }
    } 
    else {
        dam += dice(1, 4) + number(0, weap->getWeight());
    }
    
    dam += this->getOwner()->getDamroll();
    
    
    damage->setDamage(dam);
    damage->setDamageType(this->getSkillNumber());
    damage->setDamageLocation(choose_random_limb(damage->getVictim()));
    
    return damage;
}

DamageObject *
GenericAttackType::generate(Creature *target) {
    DamageObject *damage = new DamageObject(getOwner(),target);
    damage->setGenMessages(true);
    
    int dam = 0;
    
    dam = number(0,3);
    dam += this->getOwner()->getDamroll();
    
    damage->setDamage(dam);
    damage->setDamageType(this->getSkillNumber());
    damage->setDamageLocation(choose_random_limb(damage->getVictim()));
    
    return damage;
}

DamageObject *
GenericAttackType::generate(obj_data *weap) {
    Creature *victim = this->getOwner()->findRandomCombat();

    if (this->getOwner()->in_room != victim->in_room) {
        this->getOwner()->removeCombat(victim);
        victim->removeCombat(this->getOwner());
        return NULL;
    }
    return this->generate(weap, victim);
}

DamageObject *
GenericAttackType::generate() {
    Creature *victim = this->getOwner()->findRandomCombat();

    if (this->getOwner()->in_room != victim->in_room) {
        this->getOwner()->removeCombat(victim);
        victim->removeCombat(this->getOwner());
        return NULL;
    }
    return this->generate(victim);
}

