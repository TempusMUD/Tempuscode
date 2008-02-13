 #ifndef _CUSTOM_AFFECTS_H_
#define _CUSTOM_AFFECTS_H_

#include "generic_affect.h"
#include "generic_skill.h"
#include "spells.h"
#include "utils.h"

using namespace std;

class ChemicalStabilityAffect : public CreatureAffect
{
    public:
        ChemicalStabilityAffect() : CreatureAffect() {} 
        ~ChemicalStabilityAffect() {}

        GenericAffect * createNewInstance() { 
            return new ChemicalStabilityAffect; 
        }

        void modifyEO(ExecutableObject *eo);
};

class ElectrostaticFieldAffect : public CreatureAffect
{
    public:
        ElectrostaticFieldAffect() : CreatureAffect() {};
        ~ElectrostaticFieldAffect() {};

        GenericAffect *
        createNewInstance() {
            return (GenericAffect *)new ElectrostaticFieldAffect();
        }

        void modifyEO(ExecutableObject *eo);
};

class PoisonAffect : public CreatureAffect
{
    public:
        enum PoisonType {
            Poison_I,
            Poison_II,
            Poison_III
        };
        
        PoisonAffect() : CreatureAffect() {
            setWearoffToChar("The poison burning in your veins subsides.");
            setSkillNum(SPELL_POISON);
            addApply(APPLY_STR, -2);
            setAccumDuration(true);
            setAccumAffect(true);
            setBitvector(0, AFF_POISON);
            _updateInterval = 3;
            _type = Poison_I;
        };
        ~PoisonAffect() {};

        GenericAffect *
        createNewInstance() {
            return (GenericAffect *)new PoisonAffect();
        }

        void setType(PoisonType type) { _type = type; };
        void modifyEO(ExecutableObject *eo);
        void performUpdate();

    private:
        PoisonType _type;
};

class SicknessAffect : public CreatureAffect
{
    public:
        SicknessAffect() : CreatureAffect() {
            setWearoffToChar("You feel less sick.");
            setSkillNum(SPELL_SICKNESS);
            setBitvector(2, AFF3_SICKNESS);
            addApply(APPLY_STR, 2);
            setAccumDuration(true);
            setAccumAffect(true);
        };
        ~SicknessAffect() {};

        GenericAffect *
        createNewInstance() {
            return (GenericAffect *)new SicknessAffect();
        }

        void modifyEO(ExecutableObject *eo);
};

class AblazeAffect : public CreatureAffect
{
    public:
        AblazeAffect() : CreatureAffect() { 
            setSkillNum(SPELL_ABLAZE);
            setBitvector(1, AFF2_ABLAZE);
            setDuration(-1);
        };
        ~AblazeAffect() {};

        GenericAffect *
        createNewInstance() {
            return (GenericAffect *)new AblazeAffect();
        }

        void modifyEO(ExecutableObject *eo);
};

class InvisAffect : public CreatureAffect
{
    public:
        InvisAffect() : CreatureAffect() { 
            setSkillNum(SPELL_INVISIBLE);
            setBitvector(0, AFF_INVISIBLE);
            setDuration(12 MUD_HOURS);
            addApply(APPLY_AC, -10);
            setAccumDuration(true);
            setWearoffToChar("You feel yourself exposed");
        };
        ~InvisAffect() {};

        GenericAffect *
        createNewInstance() {
            return (GenericAffect *)new InvisAffect();
        }

        void modifyEO(ExecutableObject *eo);
};

class BlurAffect : public CreatureAffect
{
    public:
        BlurAffect() : CreatureAffect() { 
            setSkillNum(SPELL_BLUR);
            setBitvector(0, AFF_BLUR);
            setDuration(12 MUD_HOURS);
            addApply(APPLY_AC, -5);
            setAccumDuration(true);
            setWearoffToChar("Your image is no longer blurred and shifting.");
        };
        ~BlurAffect() {};

        GenericAffect *
        createNewInstance() {
            return (GenericAffect *)new BlurAffect();
        }

        void modifyEO(ExecutableObject *eo);
};

class BlindnessAffect : public CreatureAffect
{
    public:
        BlindnessAffect() : CreatureAffect() { 
            setSkillNum(SPELL_BLINDNESS);
            setBitvector(0, AFF_BLIND);
            setDuration(2 MUD_HOURS);
            addApply(APPLY_AC, 15);
            addApply(APPLY_HITROLL, -2);
            setWearoffToChar("Your vision returns.");
        };
        ~BlindnessAffect() {};

        GenericAffect *
        createNewInstance() {
            return (GenericAffect *)new BlindnessAffect();
        }

        void modifyEO(ExecutableObject *eo);
};

class FireShieldAffect : public CreatureAffect
{
    public:
        FireShieldAffect() : CreatureAffect() { 
            setSkillNum(SPELL_FIRE_SHIELD);
            setBitvector(1, AFF2_FIRE_SHIELD);
            setDuration(8 MUD_HOURS);
            addApply(APPLY_AC, -5);
            setWearoffToChar("The fire shield before you evaporates.");
        };
        ~FireShieldAffect() {};

        GenericAffect *
        createNewInstance() {
            return (GenericAffect *)new FireShieldAffect();
        }

        void modifyEO(ExecutableObject *eo);
};

class CurseAffect : public CreatureAffect
{
    public:
        CurseAffect() : CreatureAffect() { 
            setSkillNum(SPELL_CURSE);
            setBitvector(0, AFF_CURSE);
            setDuration(12 MUD_HOURS);
            addApply(APPLY_DAMROLL, -6);
            addApply(APPLY_HITROLL, -8);
            setAccumDuration(true);
            setWearoffToChar("You don't feel quite so unlucky.");
        };
        ~CurseAffect() {};

        GenericAffect *
        createNewInstance() {
            return (GenericAffect *)new CurseAffect();
        }

        void modifyEO(ExecutableObject *eo);
};

class CharmAffect : public CreatureAffect
{
    public:
        CharmAffect() : CreatureAffect() { 
            setSkillNum(SPELL_CHARM);
            setBitvector(0, AFF_CHARM);
            setDuration(6 MUD_HOURS);
            setAccumDuration(true);
            setWearoffToChar("You feel more self-confident.");
        };
        ~CharmAffect() {};

        GenericAffect *
        createNewInstance() {
            return (GenericAffect *)new CharmAffect();
        }

        void modifyEO(ExecutableObject *eo);
};

class AntiMagicShellAffect : public CreatureAffect
{
    public:
        AntiMagicShellAffect() : CreatureAffect() { 
            setSkillNum(SPELL_ANTI_MAGIC_SHELL);
            setDuration(12 MUD_HOURS);
            setWearoffToChar("Your anti-magic shell dissipates.");
        };
        ~AntiMagicShellAffect() {};

        GenericAffect *
        createNewInstance() {
            return (GenericAffect *)new AntiMagicShellAffect();
        }

        void modifyEO(ExecutableObject *eo);
};

class ManaShieldAffect : public CreatureAffect
{
    public:
        ManaShieldAffect() : CreatureAffect() { 
            setSkillNum(SPELL_MANA_SHIELD);
            setIsPermenant(true);
            setWearoffToChar("Your mana shield has expired.");
        };
        ~ManaShieldAffect() {};

        GenericAffect *
        createNewInstance() {
            return (GenericAffect *)new ManaShieldAffect();
        }

        void modifyEO(ExecutableObject *eo);
};

class AcidityAffect : public CreatureAffect
{
    public:
        AcidityAffect() : CreatureAffect() {
            setSkillNum(SPELL_ACIDITY);
            setDuration(1 MUD_HOURS);
            setWearoffToChar("You feel less acidic.");
            setBitvector(2, AFF3_ACIDITY);
            setAccumDuration(true);
            _updateInterval = 3;
        }
        ~AcidityAffect() {};

        GenericAffect *
        createNewInstance() {
            return (GenericAffect *)new AcidityAffect();
        }

        void modifyEO(ExecutableObject *eo);
        void performUpdate();
};

class FearAffect : public CreatureAffect
{
    public:
        FearAffect() : CreatureAffect() {
            setSkillNum(SPELL_FEAR);
            setDuration(6 MUD_HOURS);
            setWearoffToChar("You feel less afraid."); 
            _updateInterval = 1;
            setAccumAffect(true);
            setAccumDuration(true);
        }
        ~FearAffect() {};

        GenericAffect * createNewInstance() {
            return (GenericAffect *)new FearAffect();
        }

        void modifyEO(ExecutableObject *eo);
        void performUpdate();
};

class PsishieldAffect : public CreatureAffect
{
    public:
        PsishieldAffect() : CreatureAffect() {
            setSkillNum(SPELL_FEAR);
            setDuration(6 MUD_HOURS);
            setWearoffToChar("Your psionic shield dissipates."); 
        }
        ~PsishieldAffect() {};

        GenericAffect * createNewInstance() {
            return (GenericAffect *)new PsishieldAffect();
        }

        void modifyEO(ExecutableObject *eo);
};

class PsychicFeedbackAffect : public CreatureAffect
{
    public:
        PsychicFeedbackAffect() : CreatureAffect() {
            setSkillNum(SPELL_PSYCHIC_FEEDBACK);
            setDuration(6 MUD_HOURS);
            setWearoffToChar("You are no longer providing psychic feedback to your attackers."); 
        }
        ~PsychicFeedbackAffect() {};

        GenericAffect * createNewInstance() {
            return (GenericAffect *)new PsychicFeedbackAffect();
        }

        void modifyEO(ExecutableObject *eo);
};

class RegenAffect : public CreatureAffect
{
    public:
        RegenAffect() : CreatureAffect() {
            setSkillNum(SPELL_REGENERATE);
            setDuration(6 MUD_HOURS);
            setWearoffToChar("Your body stops regenerating."); 
            _updateInterval = 3;
        }
        ~RegenAffect() {};

        GenericAffect * createNewInstance() {
            return (GenericAffect *)new RegenAffect();
        }

        void modifyEO(ExecutableObject *eo);
        void performUpdate();
};

class PsychicCrushAffect : public CreatureAffect
{
    public:
        PsychicCrushAffect() : CreatureAffect() {
            setSkillNum(SPELL_PSYCHIC_CRUSH);
            setDuration(3 MUD_HOURS);
            setWearoffToChar("You feel the crushing psychic force dissipate."); 
            _updateInterval = 3;
        }
        ~PsychicCrushAffect() {};

        GenericAffect * createNewInstance() {
            return (GenericAffect *)new PsychicCrushAffect();
        }

        void modifyEO(ExecutableObject *eo);
        void performUpdate();
};
#endif
