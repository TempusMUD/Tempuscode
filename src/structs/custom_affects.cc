#include "custom_affects.h"
#include "comm.h"

#define BAD_ATTACK_TYPE(attacktype) (attacktype == TYPE_BLEED || \
                                             attacktype == SPELL_POISON || \
                                             attacktype == TYPE_ABLAZE || \
                                             attacktype == TYPE_ACID_BURN || \
                                             attacktype == TYPE_TAINT_BURN || \
                                             attacktype == TYPE_PRESSURE || \
                                             attacktype == TYPE_SUFFOCATING || \
                                             attacktype == TYPE_ANGUISH || \
                                             attacktype == TYPE_OVERLOAD || \
                                             attacktype == TYPE_SUFFERING || \
                                             attacktype == SPELL_STIGMATA || \
                                             attacktype == TYPE_DROWNING || \
                                             attacktype == SPELL_SICKNESS || \
                                             attacktype == TYPE_RAD_SICKNESS || \
                                             attacktype == SKILL_HOLY_TOUCH)

// ************************************************************
//
// This file is ONLY for the definition of modifyEO() functions
// for custom affects.
//
// ************************************************************

struct Creature *get_char_in_world_by_idnum(int nr);

void 
ElectrostaticFieldAffect::modifyEO(ExecutableObject *eo) {
    int level = 
        eo->getVictim()->getLevelBonus(SPELL_ELECTROSTATIC_FIELD);
    if (_ownerID == eo->getChar()->getIdNum())
        return;

    if (CHAR_WITHSTANDS_ELECTRIC(eo->getChar()))
        return;

    if (mag_savingthrow(eo->getChar(), level >> 2, SAVING_ROD))
        return;

    if (eo->getExecuted())
        return;

    if (eo->isA(EDamageObject) && eo->getModifiable()) {
        DamageObject *damage = dynamic_cast<DamageObject *>(eo);

        GenericSkill const *skill;
        skill = GenericSkill::getSkill(damage->getDamageType());
        if (!skill) {
            return;
            errlog("WTF?  Damage in %s with no type!", __FUNCTION__);
        }

        // Don't hit vs these spell types.
        if (skill->isPsionic() || skill->isArcane() || 
            skill->isDivine() || skill->isPhysics() || 
            skill->isSong())
            return;

        if (_duration > 1)
            _duration--;

        // Set up a new damage object
        DamageObject *ret = new DamageObject(damage->getVictim(), damage->getChar());
        ret->setDamageType(SPELL_ELECTROSTATIC_FIELD);
        int dam = dice(2, level);
        ret->setDamage(dam);

        string toChar = "$N is zapped by your electrostatic field!";
        ret->addToCharMessage(toChar);
        
        string toVict = "You are zapped by $n's electrostatic field!";
        ret->addToVictMessage(toVict);

        string toRoom = "$N is zapped by $n's electrostatic field!";
        ret->addToRoomMessage(toRoom);

        // Cancel this DO versus us
        damage->cancelByID();

        // Insert the new damage object before the one we just
        // canceled.
        damage->getEV()->insert(damage, ret);
    }
}

void 
PoisonAffect::modifyEO(ExecutableObject *eo) {
}

void PoisonAffect::performUpdate() {
    ExecutableVector ev;

    // Set up a new damage object
    DamageObject *damObj = new DamageObject(get_char_in_world_by_idnum(_ownerID), _target);
    damObj->setDamageType(SPELL_POISON);
    int dam = dice(2, getLevel() >> 1);

    if (_type == Poison_I)
        damObj->setDamage(dam);
    if (_type == Poison_II)
        damObj->setDamage(dam * 2);
    if (_type == Poison_III)
        damObj->setDamage(dam * 3);

    string toVict = "You feel poison burning in your blood and suffer.";
    damObj->addToVictMessage(toVict);

    ev.push_back(damObj);
    ev.execute();
}

void 
SicknessAffect::modifyEO(ExecutableObject *eo) {
}

void 
AblazeAffect::modifyEO(ExecutableObject *eo) {
}

void 
InvisAffect::modifyEO(ExecutableObject *eo) {
}

void 
BlurAffect::modifyEO(ExecutableObject *eo) {
}

void 
BlindnessAffect::modifyEO(ExecutableObject *eo) {
}

void 
FireShieldAffect::modifyEO(ExecutableObject *eo) {
}

void 
CurseAffect::modifyEO(ExecutableObject *eo) {
}

void 
CharmAffect::modifyEO(ExecutableObject *eo) {
}

void 
AntiMagicShellAffect::modifyEO(ExecutableObject *eo) {
}

void
FearAffect::modifyEO(ExecutableObject *eo) {
}

void FearAffect::performUpdate() {
    int do_flee(Creature *, char *, int, int, int*);
    if (_target->numCombatants() &&
        !number(0, (_target->getLevel() >> 3) + 1)
        && _target->getHit() > 0) {
        int retval;
        do_flee(_target, "", 0, 0, &retval);
    }
}

void
PsishieldAffect::modifyEO(ExecutableObject *eo) {
    int skillNum = eo->getSkill();
    GenericSkill const *skill = GenericSkill::getSkill(skillNum);
    Creature *owner = eo->getChar();
    bool failed = false;
    int prob, percent;

    if (!owner || owner == _target)
        return;

    if (!_target->distrusts(owner))
        return;

    if (!skill || !skill->isPsionic())
        return;

    if (skillNum == SPELL_PSIONIC_SHATTER && 
            !mag_savingthrow(_target, owner->getLevel(), SAVING_PSI)) {
        failed = true;
    }

    prob = owner->checkSkill(skillNum) + GET_INT(owner);
    prob += owner->getLevelBonus(skillNum);

    percent = _target->getLevelBonus(SPELL_PSISHIELD);
    percent += number(1, 120);

    if (mag_savingthrow(_target, owner->getLevel(), SAVING_PSI))
        percent <<= 1;

    if (_target->getInt() < owner->getInt())
        percent += (_target->getInt() - owner->getInt()) << 3;

    if (percent >= prob)
        failed = true;

    if (failed) {
        ExecutableObject *neo = new ExecutableObject(owner); 
        neo->setVictim(_target);
        neo->addToCharMessage("Your psychic attack is deflected by $N's psishield!");
        neo->addToVictMessage("$n's psychic attack is deflected by your psishield!");
        neo->addToRoomMessage("$n's psychic attack is deflected by $N's psishield!");
        eo->cancel();
        eo->getEV()->insert_id(eo, neo);
    }
}

void
PsychicFeedbackAffect::modifyEO(ExecutableObject *eo) {
    if (eo->isA(EDamageObject)) {
        DamageObject *do1 = dynamic_cast<DamageObject *>(eo);

        if (mag_savingthrow(do1->getChar(), getLevel(), SAVING_PSI))
            return;

        if (ROOM_FLAGGED(_target->in_room, ROOM_NOPSIONICS))
            return;

        if (NULL_PSI(do1->getChar()))
            return;

        if (BAD_ATTACK_TYPE(do1->getDamageType()))
            return;

        if (do1->getDamageType() == SPELL_PSYCHIC_FEEDBACK)
            return;

        int feedback_dam = (do1->getDamage() * getLevel()) / 200;
        int feedback_mana = feedback_dam / 15;

        if (_target->getMana() < feedback_mana) {
            feedback_mana = _target->getMana();
            feedback_dam = feedback_mana * 15;
            affectRemove();
        }

        if (feedback_dam > 0) {
            PointsObject *po = new PointsObject(_target, _target);
            po->setSkill(SPELL_PSYCHIC_FEEDBACK);
            po->setMana(-(MAX(1, feedback_mana)));

            DamageObject *do2 = new DamageObject(_target, do1->getChar());
            do2->setDamageType(SPELL_PSYCHIC_FEEDBACK);
            do2->setDamage(feedback_dam);

            do2->addToCharMessage("$N's brain shakes from your psychic feedback!");
            do2->addToVictMessage("Your brain shakes under the force of $n's psychic feedback!");
            do2->addToRoomMessage("$N's brain shakes under the force of $n's psychic feedback!");

            do1->getEV()->push_back(do2);
        }
    }
}

void
ManaShieldAffect::modifyEO(ExecutableObject *eo) {
    if (!eo->isA(EDamageObject))
        return;

    if (_ownerID == eo->getChar()->getIdNum())
        return;

    DamageObject *do1 = dynamic_cast<DamageObject *>(eo);

    // Theoretically, any attack types such as BLEEDING, ACIDITY, etc
    // should not be modifiable.
    if (do1->getModifiable())
        do1->setDamageLocation(WEAR_MSHIELD);
}

void AcidityAffect::modifyEO(ExecutableObject *eo) {
    // FIXME:  If we find a generic attack eo here we should cause damage to
    // the weapon or burn the attacker if they aren't using a weapon
}

void AcidityAffect::performUpdate() {
    ExecutableVector ev;
    
    // At every update this should damage the target as well as his eq
    Creature *owner = get_char_in_world_by_idnum(_ownerID);
    DamageObject *do1 = new DamageObject(owner, _target);
    int dam = _duration >> 2;
    do1->setDamageType(TYPE_ACID_BURN);
    do1->setDamage(dam);

    if (owner && (owner->in_room == _target->in_room))
        do1->addToCharMessage("$N screams in pain as the acid burns $M!");
    do1->addToVictMessage("You scream in pain as the acid eats away at your flesh!");
    do1->addToRoomMessage("$N screams in pain as the acid burns $M!", TO_VICT_RM);
    do1->setSkill(_skillNum);
    ev.push_back(do1);

    // Now, damage all the eq they're wearing
    for (int i = 0; i < NUM_WEARS; i++) {
        DamageItemObject *obj = 
            new DamageItemObject(owner, _target, i, dam, TYPE_ACID_BURN);
        obj->setSkill(_skillNum);
        ev.push_back(obj);
    }

    ev.execute();
}

void ChemicalStabilityAffect::modifyEO(ExecutableObject *eo)
{
    // First, determine if appropriate skill type is at play.
    if(eo->getSkill() == TYPE_ACID_BURN || eo->getSkill() == SPELL_ACIDITY || 
       eo->getSkill() == SPELL_ACID_BREATH || eo->getSkill() == SPELL_OXIDIZE || 
       eo->getSkill() == SPELL_MELFS_ACID_ARROW) {

        // determine the nature of the affect.
        if(eo->isA(EAffectObject) || eo->isA(EDamageItemObject)) {
	        eo->cancel();
        }
        else if(eo->isA(EDamageObject)) {
            DamageObject *dam_obj = dynamic_cast<DamageObject *>(eo);

            // different damage modifier for oxidize
            if(eo->getSkill() == SPELL_OXIDIZE)
                dam_obj->setDamage(dam_obj->getDamage() >> 2);
            else
                dam_obj->setDamage(dam_obj->getDamage() >> 1);
        }
    }
}

void RegenAffect::modifyEO(ExecutableObject *eo) {
}

void RegenAffect::performUpdate() {
    ExecutableVector ev;
    CreatureAffect *aff;
    if ((aff = (CreatureAffect *)_target->affectedBy(SKILL_HAMSTRING))) {
        aff->affectRemove();        
        ExecutableObject *eo = new ExecutableObject(_target);
        eo->addToCharMessage("The gaping wound on your leg closes!");
        eo->addToRoomMessage("The gaping wound on $n's leg closes.");
        ev.push_back(eo);
    }

    PointsObject *po = new PointsObject(_target, _target);
    po->setSkill(getSkillNum());
    int heal = dice(4, getLevel() / 4) + _target->getCon();
    po->setHit(heal);
    ev.push_back(po);

    ev.execute();
}

void PsychicCrushAffect::modifyEO(ExecutableObject *eo) {
}

void PsychicCrushAffect::performUpdate() {
    ExecutableVector ev;

    Creature *owner = get_char_in_world_by_idnum(_ownerID);

    if (mag_savingthrow(_target, getLevel(), SAVING_PSI) && !random_fractional_4()) {
        ExecutableObject *eo = new ExecutableObject(owner);
        eo->setVictim(_target);
        eo->setSkill(getSkillNum());

        if (owner->in_room == _target->in_room)
            eo->addToCharMessage("$N shakes off the effect of a crushing psychic force!");
        eo->addToVictMessage("You shake off the effect of a crushing psychic force!");
        eo->addToCharMessage("$N shakes off the effect of a crushing psychic force!");
        ev.push_back(eo);

        return;
    }
    PointsObject *po = new PointsObject(owner, _target);
    po->setSkill(getSkillNum());
    po->setHit(-(50 + dice(4, getLevel() >> 3)));

    if (owner->in_room == _target->in_room)
        po->addToCharMessage("$N howls with pain as a psychic force crushes $S skull!");
    po->addToVictMessage("You howl with pain as a psychic force crushes your skull!");
    po->addToRoomMessage("$N howls with pain as a psychic force crushes $S skull!");
    ev.push_back(po);

    ev.execute();
}
