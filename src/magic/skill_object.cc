#include "skill_object.h"
#include "skill_affect.h"
#include "creature.h"
#include "fight.h"
#include "damage_object.h"
#include "constants.h"
#include "macros.h"
#include "utils.h"

struct affected_type *affected_by_spell(struct Creature *ch, short type);
int affect_from_char(struct Creature *ch, short type);
int dice(int number, int size);
void send_to_char(struct Creature *ch, const char *str, ...);
int number(int from, int to);

void
SkillObject::perform(Creature *ch, Creature *target, ExecutableObject *eo) 
{
    eo->setChar(ch);

    if (eo->isA(ESkillAffect)) {
        SkillAffect *sa = dynamic_cast<SkillAffect *>(eo);
        sa->setSkillNum(_skillNum);
    }
  
    eo->setVictim(target);
}

void 
SkillObject::defend(ExecutableVector &ev)
{
    ExecutableVector::iterator vi;
    for (vi = ev.begin(); vi != ev.end(); ++vi) {
        bool save = mag_savingthrow((*vi)->getVictim(), 
                                    (*vi)->getChar()->getLevelBonus(_skillNum) / 2, _saveType);
        if ((*vi)->isA(ESkillAffect)) {
            SkillAffect *sa = dynamic_cast<SkillAffect *>(*vi);
            if (sa->getSkillNum() == _skillNum) {
                if (save) {
                   ev.erase(vi); 
                   vi = ev.begin();
                }
            }
        }
        else if ((*vi)->isA(EDamageObject)) {
            DamageObject *do1 = dynamic_cast<DamageObject *>(*vi);
            if (do1->getDamageType() == _skillNum) {
                if (save) {
                    ev.erase(vi);
                    vi = ev.begin();
                }
            }
        }
    }
}

/* The SkillObject class defines the following members.  The defaults
   are listed below.  Please set what you need accordingly.
   
   Valid positions are:
   POS_DEAD, POS_MORTALLYW, POS_INCAP, POS_STUNNED, POS_SLEEPING,
   POS_RESTING, POS_SITTING, POS_FIGHTING, POS_STANDING, POS_FLYING,
   POS_MOUNTED, and POS_SWIMMING

   Valid flags are:
   MAG_DAMAGE, MAG_AFFECTS, MAG_UNAFFECTS, MAG_POINTS, MAG_ALTER_OBJS
   MAG_GROUPS, MAG_MASSES, MAG_AREAS, MAG_SUMMONS, MAG_CREATIONS,
   MAG_MANUAL, MAG_OBJECTS, MAG_TOUCH, MAG_MAGIC, MAG_DIVINE, MAG_PHYSICS
   MAG_PSIONIC, MAG_BIOLOGIC, CYB_ACTIVATE, MAG_EVIL, MAG_GOOD, MAG_EXITS
   MAG_OUTDOORS, MAG_NOWATER, MAG_WATERZAP, MAG_NOSUN, MAG_ZEN and
   MAG_MERCENARY
   
   Mages will receive this spell at level 5
   Valid classes are:
   CLASS_MAGE, CLASS_CLERIC, CLASS_THIEF, CLASS_WARRIOR, CLASS_BARB
   CLASS_PSIONIC, CLASS_PHYSIC, CLASS_CYBORG, CLASS_KNIGHT, CLASS_RANGER
   CLASS_HOOD, CLASS_MONK, CLASS_VAMPIRE, and CLASS_MERCENARY

   _gen = 0;
   _minPos = POS_STANDING;
   _maxMana = 0;
   _minMana = 0;
   _manaChange = 0;
   _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
   _violent = false;
   _flags = MAG_AFFECTS;
   _avatar = 0;
   _saveType = SAVE_NONE;

   See below for the use of _level*/

class SpellChillTouch : public SkillObject
{
    public:
        SpellChillTouch() : SkillObject(SPELL_CHILL_TOUCH) 
        {
            _minPos = POS_FIGHTING;
            _maxMana = 20;
            _minMana = 10;
            _manaChange = 3;
            _violent = true;

            _flags = MAG_MAGIC | MAG_TOUCH | MAG_DAMAGE | MAG_AFFECTS;

            _level[CLASS_MAGE] = 5;           
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
            SkillAffect *sa = new SkillAffect();
            struct affected_type *af;

            SkillObject::perform(ch, target, sa);

            memset(&af, 0x0, sizeof(struct affected_type));
            af->type = _skillNum;
            af->location = APPLY_STR;
            af->level = ch->getLevelBonus(_skillNum);
            af->modifier = -((ch->getLevelBonus(_skillNum) / 25) + 1);
            af->duration = 4;

            sa->setAccumDuration(true);
            sa->addAffect(af);
            sa->addToCharMessage("You grin maniacally as you sap $n's strength!");
            sa->addToRoomMessage("$N grins maniacally as $E saps $n's strength!");
            sa->addToVictMessage("You feel your strength wither!");

            ev.push_back(sa);
        }
};

class SpellAirWalk : public SkillObject
{    public:
        SpellAirWalk() : SkillObject(SPELL_AIR_WALK)
        {
            _maxMana = 60;
            _minMana = 40;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_DIVINE|MAG_AFFECTS|MAG_NOWATER;
            _minPos = POS_STANDING;

            _level[CLASS_CLERIC] = 32;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellArmor : public SkillObject
{    public:
        SpellArmor() : SkillObject(SPELL_ARMOR)
        {
            _maxMana = 45;
            _minMana = 15;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_DIVINE|MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_MAGE] = 4;
            _level[CLASS_CLERIC] = 1;
            _level[CLASS_KNIGHT] = 5;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
            SkillAffect *sa = new SkillAffect();
            struct affected_type *af;

            SkillObject::perform(ch, target, sa);

            memset(&af, 0x0, sizeof(struct affected_type));
            af->type = _skillNum;
            af->location = APPLY_AC;
            af->level = ch->getLevelBonus(_skillNum);
            af->modifier = -((ch->getLevelBonus(_skillNum) >> 2) + 20);
            af->duration = MAX(24, ch->getLevelBonus(_skillNum) >> 2);

            sa->setAccumDuration(true);
            sa->addAffect(af);
            sa->addToVictMessage("You feel someone protecting you.");

            ev.push_back(sa);
        }
};

class SpellAstralSpell : public SkillObject
{    public:
        SpellAstralSpell() : SkillObject(SPELL_ASTRAL_SPELL)
        {
            _maxMana = 175;
            _minMana = 105;
            _manaChange = 8;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_MAGIC|MAG_DIVINE|MAG_PSIONIC|MAG_MANUAL;
            _minPos = POS_FIGHTING;

            _level[CLASS_MAGE] = 43;
            _level[CLASS_CLERIC] = 47;
            _level[CLASS_PSIONIC] = 45;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellControlUndead : public SkillObject
{    public:
        SpellControlUndead() : SkillObject(SPELL_CONTROL_UNDEAD)
        {
            _maxMana = 50;
            _minMana = 20;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_NOT_SELF;
            _flags = MAG_MAGIC|MAG_MANUAL|MAG_EVIL|MAG_DIVINE;
            _minPos = POS_STANDING;

            _level[CLASS_CLERIC] = 26;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellTeleport : public SkillObject
{    public:
        SpellTeleport() : SkillObject(SPELL_TELEPORT)
        {
            _maxMana = 75;
            _minMana = 50;
            _manaChange = 3;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_OBJ_ROOM|TAR_OBJ_INV;
            _flags = MAG_MAGIC|MAG_MANUAL;
            _minPos = POS_FIGHTING;

            _level[CLASS_MAGE] = 33;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellLocalTeleport : public SkillObject
{    public:
        SpellLocalTeleport() : SkillObject(SPELL_LOCAL_TELEPORT)
        {
            _maxMana = 45;
            _minMana = 30;
            _manaChange = 3;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_MAGIC|MAG_MANUAL;
            _minPos = POS_FIGHTING;

            _level[CLASS_MAGE] = 18;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellBlur : public SkillObject
{    public:
        SpellBlur() : SkillObject(SPELL_BLUR)
        {
            _maxMana = 45;
            _minMana = 20;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_MAGE] = 13;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellBless : public SkillObject
{    public:
        SpellBless() : SkillObject(SPELL_BLESS)
        {
            _maxMana = 35;
            _minMana = 15;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_OBJ_INV;
            _flags = MAG_DIVINE|MAG_MANUAL|MAG_GOOD;
            _minPos = POS_STANDING;

            _level[CLASS_CLERIC] = 7;
            _level[CLASS_KNIGHT] = 10;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellDamn : public SkillObject
{    public:
        SpellDamn() : SkillObject(SPELL_DAMN)
        {
            _maxMana = 35;
            _minMana = 15;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_OBJ_INV;
            _flags = MAG_DIVINE|MAG_MANUAL|MAG_EVIL;
            _minPos = POS_STANDING;

            _level[CLASS_CLERIC] = 7;
            _level[CLASS_KNIGHT] = 10;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellCalm : public SkillObject
{    public:
        SpellCalm() : SkillObject(SPELL_CALM)
        {
            _maxMana = 35;
            _minMana = 15;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_DIVINE|MAG_MANUAL;
            _minPos = POS_STANDING;

            _level[CLASS_KNIGHT] = 36;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellBlindness : public SkillObject
{    public:
        SpellBlindness() : SkillObject(SPELL_BLINDNESS)
        {
            _maxMana = 35;
            _minMana = 25;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_NOT_SELF;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 16;
            _level[CLASS_CLERIC] = 13;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellBreatheWater : public SkillObject
{    public:
        SpellBreatheWater() : SkillObject(SPELL_BREATHE_WATER)
        {
            _maxMana = 45;
            _minMana = 20;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_MAGE] = 40;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellBurningHands : public SkillObject
{    public:
        SpellBurningHands() : SkillObject(SPELL_BURNING_HANDS)
        {
            _maxMana = 30;
            _minMana = 10;
            _manaChange = 3;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_MAGIC|MAG_TOUCH|MAG_DAMAGE|MAG_NOWATER;
            _minPos = POS_FIGHTING;

            _level[CLASS_MAGE] = 11;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellCallLightning : public SkillObject
{    public:
        SpellCallLightning() : SkillObject(SPELL_CALL_LIGHTNING)
        {
            _maxMana = 40;
            _minMana = 25;
            _manaChange = 3;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_DIVINE|MAG_DAMAGE|MAG_NOWATER|MAG_OUTDOORS;
            _minPos = POS_FIGHTING;

            _level[CLASS_CLERIC] = 25;
            _level[CLASS_RANGER] = 30;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellEnvenom : public SkillObject
{    public:
        SpellEnvenom() : SkillObject(SPELL_ENVENOM)
        {
            _maxMana = 25;
            _minMana = 10;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_OBJ_EQUIP|TAR_OBJ_ROOM|TAR_OBJ_INV;
            _flags = MAG_MAGIC|MAG_ALTER_OBJS;
            _minPos = POS_FIGHTING;

            _level[CLASS_RANGER] = 5;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellCharm : public SkillObject
{    public:
        SpellCharm() : SkillObject(SPELL_CHARM)
        {
            _maxMana = 75;
            _minMana = 50;
            _manaChange = 2;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_NOT_SELF;
            _flags = MAG_MAGIC|MAG_MANUAL;
            _minPos = POS_FIGHTING;

            _level[CLASS_MAGE] = 22;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellCharmAnimal : public SkillObject
{    public:
        SpellCharmAnimal() : SkillObject(SPELL_CHARM_ANIMAL)
        {
            _maxMana = 75;
            _minMana = 50;
            _manaChange = 2;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_NOT_SELF;
            _flags = MAG_DIVINE|MAG_MANUAL;
            _minPos = POS_FIGHTING;

            _level[CLASS_RANGER] = 23;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellClairvoyance : public SkillObject
{    public:
        SpellClairvoyance() : SkillObject(SPELL_CLAIRVOYANCE)
        {
            _maxMana = 90;
            _minMana = 60;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_WORLD;
            _flags = MAG_MAGIC|MAG_MANUAL;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 35;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellCallRodent : public SkillObject
{    public:
        SpellCallRodent() : SkillObject(SPELL_CALL_RODENT)
        {
            _maxMana = 90;
            _minMana = 60;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_MANUAL;
            _minPos = POS_STANDING;

            _level[CLASS_RANGER] = 15;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellCallBird : public SkillObject
{    public:
        SpellCallBird() : SkillObject(SPELL_CALL_BIRD)
        {
            _maxMana = 90;
            _minMana = 60;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_MANUAL;
            _minPos = POS_STANDING;

            _level[CLASS_RANGER] = 22;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellCallReptile : public SkillObject
{    public:
        SpellCallReptile() : SkillObject(SPELL_CALL_REPTILE)
        {
            _maxMana = 90;
            _minMana = 60;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_MANUAL;
            _minPos = POS_STANDING;

            _level[CLASS_RANGER] = 29;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellCallBeast : public SkillObject
{    public:
        SpellCallBeast() : SkillObject(SPELL_CALL_BEAST)
        {
            _maxMana = 90;
            _minMana = 60;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_MANUAL;
            _minPos = POS_STANDING;

            _level[CLASS_RANGER] = 35;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellCallPredator : public SkillObject
{    public:
        SpellCallPredator() : SkillObject(SPELL_CALL_PREDATOR)
        {
            _maxMana = 90;
            _minMana = 60;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_MANUAL;
            _minPos = POS_STANDING;

            _level[CLASS_RANGER] = 41;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellClone : public SkillObject
{    public:
        SpellClone() : SkillObject(SPELL_CLONE)
        {
            _maxMana = 80;
            _minMana = 65;
            _manaChange = 5;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_MANUAL;
            _minPos = POS_STANDING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellColorSpray : public SkillObject
{    public:
        SpellColorSpray() : SkillObject(SPELL_COLOR_SPRAY)
        {
            _maxMana = 90;
            _minMana = 45;
            _manaChange = 3;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_MAGIC|MAG_DAMAGE;
            _minPos = POS_FIGHTING;

            _level[CLASS_MAGE] = 18;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellCommand : public SkillObject
{    public:
        SpellCommand() : SkillObject(SPELL_COMMAND)
        {
            _maxMana = 50;
            _minMana = 35;
            _manaChange = 5;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_NOT_SELF;
            _flags = MAG_DIVINE|MAG_MANUAL;
            _minPos = POS_STANDING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellConeCold : public SkillObject
{    public:
        SpellConeCold() : SkillObject(SPELL_CONE_COLD)
        {
            _maxMana = 140;
            _minMana = 55;
            _manaChange = 4;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_DIVINE|MAG_MAGIC|MAG_DAMAGE|MAG_AFFECTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_MAGE] = 36;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
            SkillAffect *sa = new SkillAffect();
            struct affected_type *af;

            SkillObject::perform(ch, target, sa);

            memset(&af, 0x0, sizeof(struct affected_type));
            af->type = _skillNum;
            af->location = APPLY_STR;
            af->level = ch->getLevelBonus(_skillNum);
            af->modifier = -((ch->getLevelBonus(_skillNum) >> 4) + 1);
            af->duration = 4;

            sa->setAccumDuration(true);
            sa->addAffect(af);
            sa->addToCharMessage("A storm of ice explodes from your outstretched fingers!");
            sa->addToRoomMessage("A storm of ice explodes from $N's outstretched fingers!");
            sa->addToVictMessage("The bitter cold around you saps your strength!");

            ev.push_back(sa);
        }
};

class SpellConjureElemental : public SkillObject
{    public:
        SpellConjureElemental() : SkillObject(SPELL_CONJURE_ELEMENTAL)
        {
            _maxMana = 175;
            _minMana = 90;
            _manaChange = 4;
            _violent = false;
            _targets = TAR_IGNORE;
            _flags = MAG_MAGIC|MAG_MANUAL;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 20;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellControlWeather : public SkillObject
{    public:
        SpellControlWeather() : SkillObject(SPELL_CONTROL_WEATHER)
        {
            _maxMana = 75;
            _minMana = 25;
            _manaChange = 5;
            _violent = false;
            _targets = TAR_IGNORE;
            _flags = MAG_MAGIC|MAG_MANUAL|MAG_NOWATER|MAG_OUTDOORS;
            _minPos = POS_STANDING;

            _level[CLASS_CLERIC] = 22;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellCreateFood : public SkillObject
{    public:
        SpellCreateFood() : SkillObject(SPELL_CREATE_FOOD)
        {
            _maxMana = 30;
            _minMana = 5;
            _manaChange = 3;
            _violent = false;
            _targets = TAR_IGNORE;
            _flags = MAG_DIVINE|MAG_CREATIONS;
            _minPos = POS_STANDING;

            _level[CLASS_CLERIC] = 5;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellCreateWater : public SkillObject
{    public:
        SpellCreateWater() : SkillObject(SPELL_CREATE_WATER)
        {
            _maxMana = 30;
            _minMana = 5;
            _manaChange = 4;
            _violent = false;
            _targets = TAR_OBJ_INV|TAR_OBJ_EQUIP;
            _flags = MAG_DIVINE|MAG_OBJECTS;
            _minPos = POS_STANDING;

            _level[CLASS_CLERIC] = 3;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellCureBlind : public SkillObject
{    public:
        SpellCureBlind() : SkillObject(SPELL_CURE_BLIND)
        {
            _maxMana = 30;
            _minMana = 15;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_DIVINE|MAG_UNAFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_CLERIC] = 6;
            _level[CLASS_KNIGHT] = 26;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellCureCritic : public SkillObject
{    public:
        SpellCureCritic() : SkillObject(SPELL_CURE_CRITIC)
        {
            _maxMana = 40;
            _minMana = 10;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_DIVINE|MAG_POINTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_CLERIC] = 10;
            _level[CLASS_KNIGHT] = 14;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellCureLight : public SkillObject
{    public:
        SpellCureLight() : SkillObject(SPELL_CURE_LIGHT)
        {
            _maxMana = 30;
            _minMana = 5;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_DIVINE|MAG_POINTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_CLERIC] = 2;
            _level[CLASS_KNIGHT] = 6;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellCurse : public SkillObject
{    public:
        SpellCurse() : SkillObject(SPELL_CURSE)
        {
            _maxMana = 80;
            _minMana = 50;
            _manaChange = 2;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_OBJ_INV|TAR_OBJ_ROOM;
            _flags = MAG_MAGIC|MAG_AFFECTS|MAG_ALTER_OBJS;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 19;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellDetectAlign : public SkillObject
{    public:
        SpellDetectAlign() : SkillObject(SPELL_DETECT_ALIGN)
        {
            _maxMana = 20;
            _minMana = 10;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_DIVINE|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 24;
            _level[CLASS_CLERIC] = 1;
            _level[CLASS_KNIGHT] = 1;
            _level[CLASS_RANGER] = 13;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellDetectInvis : public SkillObject
{    public:
        SpellDetectInvis() : SkillObject(SPELL_DETECT_INVIS)
        {
            _maxMana = 20;
            _minMana = 10;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 14;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellDetectMagic : public SkillObject
{    public:
        SpellDetectMagic() : SkillObject(SPELL_DETECT_MAGIC)
        {
            _maxMana = 20;
            _minMana = 10;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 7;
            _level[CLASS_CLERIC] = 14;
            _level[CLASS_KNIGHT] = 20;
            _level[CLASS_RANGER] = 27;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellDetectPoison : public SkillObject
{    public:
        SpellDetectPoison() : SkillObject(SPELL_DETECT_POISON)
        {
            _maxMana = 15;
            _minMana = 5;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_OBJ_INV|TAR_OBJ_ROOM|TAR_OBJ_INV|TAR_CHAR_ROOM;
            _flags = MAG_DIVINE|MAG_MANUAL;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 20;
            _level[CLASS_CLERIC] = 4;
            _level[CLASS_KNIGHT] = 12;
            _level[CLASS_RANGER] = 6;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellDetectScrying : public SkillObject
{    public:
        SpellDetectScrying() : SkillObject(SPELL_DETECT_SCRYING)
        {
            _maxMana = 50;
            _minMana = 30;
            _manaChange = 4;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_SITTING;

            _level[CLASS_MAGE] = 44;
            _level[CLASS_CLERIC] = 44;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellDimensionDoor : public SkillObject
{    public:
        SpellDimensionDoor() : SkillObject(SPELL_DIMENSION_DOOR)
        {
            _maxMana = 90;
            _minMana = 60;
            _manaChange = 3;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_MAGIC|MAG_MANUAL;
            _minPos = POS_STANDING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellDispelEvil : public SkillObject
{    public:
        SpellDispelEvil() : SkillObject(SPELL_DISPEL_EVIL)
        {
            _maxMana = 40;
            _minMana = 25;
            _manaChange = 3;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_OBJ_INV;
            _flags = MAG_DIVINE|MAG_MANUAL|MAG_GOOD;
            _minPos = POS_FIGHTING;

            _level[CLASS_CLERIC] = 20;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellDispelGood : public SkillObject
{    public:
        SpellDispelGood() : SkillObject(SPELL_DISPEL_GOOD)
        {
            _maxMana = 40;
            _minMana = 25;
            _manaChange = 3;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_OBJ_INV;
            _flags = MAG_DIVINE|MAG_MANUAL|MAG_EVIL;
            _minPos = POS_FIGHTING;

            _level[CLASS_CLERIC] = 20;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellDispelMagic : public SkillObject
{    public:
        SpellDispelMagic() : SkillObject(SPELL_DISPEL_MAGIC)
        {
            _maxMana = 90;
            _minMana = 55;
            _manaChange = 5;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_OBJ_INV|TAR_OBJ_ROOM;
            _flags = MAG_MAGIC|MAG_MANUAL;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 39;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellDisruption : public SkillObject
{    public:
        SpellDisruption() : SkillObject(SPELL_DISRUPTION)
        {
            _maxMana = 150;
            _minMana = 75;
            _manaChange = 7;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_DIVINE|MAG_DAMAGE;
            _minPos = POS_FIGHTING;

            _level[CLASS_CLERIC] = 38;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellDisplacement : public SkillObject
{    public:
        SpellDisplacement() : SkillObject(SPELL_DISPLACEMENT)
        {
            _maxMana = 55;
            _minMana = 30;
            _manaChange = 4;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 46;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellEarthquake : public SkillObject
{    public:
        SpellEarthquake() : SkillObject(SPELL_EARTHQUAKE)
        {
            _maxMana = 40;
            _minMana = 25;
            _manaChange = 3;
            _violent = true;
            _targets = TAR_IGNORE;
            _flags = MAG_DIVINE|MAG_AREAS;
            _minPos = POS_FIGHTING;

            _level[CLASS_CLERIC] = 30;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellEnchantArmor : public SkillObject
{    public:
        SpellEnchantArmor() : SkillObject(SPELL_ENCHANT_ARMOR)
        {
            _maxMana = 180;
            _minMana = 80;
            _manaChange = 3;
            _violent = false;
            _targets = TAR_OBJ_INV;
            _flags = MAG_MAGIC|MAG_MANUAL;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 16;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellEnchantWeapon : public SkillObject
{    public:
        SpellEnchantWeapon() : SkillObject(SPELL_ENCHANT_WEAPON)
        {
            _maxMana = 200;
            _minMana = 100;
            _manaChange = 3;
            _violent = false;
            _targets = TAR_OBJ_INV|TAR_OBJ_EQUIP;
            _flags = MAG_MAGIC|MAG_MANUAL;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 21;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellEndureCold : public SkillObject
{    public:
        SpellEndureCold() : SkillObject(SPELL_ENDURE_COLD)
        {
            _maxMana = 100;
            _minMana = 40;
            _manaChange = 5;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_DIVINE|MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_CLERIC] = 15;
            _level[CLASS_KNIGHT] = 16;
            _level[CLASS_RANGER] = 16;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellEnergyDrain : public SkillObject
{    public:
        SpellEnergyDrain() : SkillObject(SPELL_ENERGY_DRAIN)
        {
            _maxMana = 40;
            _minMana = 25;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_MAGIC|MAG_DAMAGE|MAG_MANUAL;
            _minPos = POS_FIGHTING;

            _level[CLASS_MAGE] = 24;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellFly : public SkillObject
{    public:
        SpellFly() : SkillObject(SPELL_FLY)
        {
            _maxMana = 60;
            _minMana = 40;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 33;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellFlameStrike : public SkillObject
{    public:
        SpellFlameStrike() : SkillObject(SPELL_FLAME_STRIKE)
        {
            _maxMana = 60;
            _minMana = 40;
            _manaChange = 2;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_DIVINE|MAG_DAMAGE|MAG_NOWATER;
            _minPos = POS_FIGHTING;

            _level[CLASS_CLERIC] = 33;
            _level[CLASS_KNIGHT] = 35;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellFlameOfFaith : public SkillObject
{    public:
        SpellFlameOfFaith() : SkillObject(SPELL_FLAME_OF_FAITH)
        {
            _maxMana = 60;
            _minMana = 40;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_OBJ_EQUIP|TAR_OBJ_ROOM|TAR_OBJ_INV;
            _flags = MAG_DIVINE|MAG_ALTER_OBJS|MAG_NOWATER;
            _minPos = POS_STANDING;

            _level[CLASS_CLERIC] = 12;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellGoodberry : public SkillObject
{    public:
        SpellGoodberry() : SkillObject(SPELL_GOODBERRY)
        {
            _maxMana = 30;
            _minMana = 5;
            _manaChange = 4;
            _violent = false;
            _targets = TAR_IGNORE;
            _flags = MAG_MAGIC|MAG_CREATIONS;
            _minPos = POS_STANDING;

            _level[CLASS_RANGER] = 7;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellGustOfWind : public SkillObject
{    public:
        SpellGustOfWind() : SkillObject(SPELL_GUST_OF_WIND)
        {
            _maxMana = 80;
            _minMana = 55;
            _manaChange = 4;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_OBJ_ROOM;
            _flags = MAG_MAGIC|MAG_MANUAL|MAG_NOWATER;
            _minPos = POS_FIGHTING;

            _level[CLASS_MAGE] = 40;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellBarkskin : public SkillObject
{    public:
        SpellBarkskin() : SkillObject(SPELL_BARKSKIN)
        {
            _maxMana = 60;
            _minMana = 25;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_RANGER] = 4;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
            SkillAffect *sa = new SkillAffect();
            struct affected_type *af;

            SkillObject::perform(ch, target, sa);

            memset(&af, 0x0, sizeof(struct affected_type));
            if (affected_by_spell(target, SPELL_STONESKIN)) {
                affect_from_char(target, SPELL_STONESKIN);
                if (*spell_wear_off_msg[SPELL_STONESKIN]) {
                    send_to_char(target, spell_wear_off_msg[SPELL_STONESKIN]);
                    send_to_char(target,"\r\n");
                }
            }
            af->location = APPLY_AC;
            af->duration = dice(4, ch->getLevelBonus(_skillNum) >> 3 + 1);
            af->modifier = -10;
            af->level = ch->getLevelBonus(_skillNum);
            sa->addAffect(af);
            sa->setAccumDuration(true);
            sa->addToVictMessage("Your skin tightens up and hardens.");

            ev.push_back(sa);
        }
};

class SpellIcyBlast : public SkillObject
{    public:
        SpellIcyBlast() : SkillObject(SPELL_ICY_BLAST)
        {
            _maxMana = 80;
            _minMana = 50;
            _manaChange = 3;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_DIVINE|MAG_DAMAGE;
            _minPos = POS_FIGHTING;

            _level[CLASS_CLERIC] = 41;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellInvisToUndead : public SkillObject
{    public:
        SpellInvisToUndead() : SkillObject(SPELL_INVIS_TO_UNDEAD)
        {
            _maxMana = 60;
            _minMana = 40;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_AFFECTS|MAG_ALTER_OBJS;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 30;
            _level[CLASS_RANGER] = 21;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellAnimalKin : public SkillObject
{    public:
        SpellAnimalKin() : SkillObject(SPELL_ANIMAL_KIN)
        {
            _maxMana = 60;
            _minMana = 40;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_AFFECTS|MAG_ALTER_OBJS;
            _minPos = POS_STANDING;

            _level[CLASS_RANGER] = 17;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellGreaterEnchant : public SkillObject
{    public:
        SpellGreaterEnchant() : SkillObject(SPELL_GREATER_ENCHANT)
        {
            _maxMana = 230;
            _minMana = 120;
            _manaChange = 5;
            _violent = false;
            _targets = TAR_OBJ_INV;
            _flags = MAG_MAGIC|MAG_MANUAL;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 34;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellGreaterInvis : public SkillObject
{    public:
        SpellGreaterInvis() : SkillObject(SPELL_GREATER_INVIS)
        {
            _maxMana = 85;
            _minMana = 65;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 38;
            _level[CLASS_VAMPIRE] = 44;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellGroupArmor : public SkillObject
{    public:
        SpellGroupArmor() : SkillObject(SPELL_GROUP_ARMOR)
        {
            _maxMana = 50;
            _minMana = 30;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_IGNORE;
            _flags = MAG_DIVINE|MAG_GROUPS;
            _minPos = POS_STANDING;

            _level[CLASS_CLERIC] = 19;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellFireball : public SkillObject
{    public:
        SpellFireball() : SkillObject(SPELL_FIREBALL)
        {
            _maxMana = 130;
            _minMana = 40;
            _manaChange = 5;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_MAGIC|MAG_DAMAGE|MAG_NOWATER;
            _minPos = POS_FIGHTING;

            _level[CLASS_MAGE] = 31;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellFireShield : public SkillObject
{    public:
        SpellFireShield() : SkillObject(SPELL_FIRE_SHIELD)
        {
            _maxMana = 70;
            _minMana = 30;
            _manaChange = 3;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_AFFECTS|MAG_NOWATER;
            _minPos = POS_FIGHTING;

            _level[CLASS_MAGE] = 17;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellGreaterHeal : public SkillObject
{    public:
        SpellGreaterHeal() : SkillObject(SPELL_GREATER_HEAL)
        {
            _maxMana = 120;
            _minMana = 90;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_DIVINE|MAG_POINTS|MAG_UNAFFECTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_CLERIC] = 34;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellGroupHeal : public SkillObject
{    public:
        SpellGroupHeal() : SkillObject(SPELL_GROUP_HEAL)
        {
            _maxMana = 80;
            _minMana = 60;
            _manaChange = 5;
            _violent = false;
            _targets = TAR_IGNORE;
            _flags = MAG_DIVINE|MAG_GROUPS;
            _minPos = POS_STANDING;

            _level[CLASS_CLERIC] = 31;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellHarm : public SkillObject
{    public:
        SpellHarm() : SkillObject(SPELL_HARM)
        {
            _maxMana = 95;
            _minMana = 45;
            _manaChange = 2;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_DIVINE|MAG_DAMAGE;
            _minPos = POS_FIGHTING;

            _level[CLASS_CLERIC] = 29;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellHeal : public SkillObject
{    public:
        SpellHeal() : SkillObject(SPELL_HEAL)
        {
            _maxMana = 60;
            _minMana = 30;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_DIVINE|MAG_POINTS|MAG_UNAFFECTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_CLERIC] = 24;
            _level[CLASS_KNIGHT] = 28;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellDivineIllumination : public SkillObject
{    public:
        SpellDivineIllumination() : SkillObject(SPELL_DIVINE_ILLUMINATION)
        {
            _maxMana = 40;
            _minMana = 20;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_DIVINE|MAG_AFFECTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_CLERIC] = 9;
            _level[CLASS_KNIGHT] = 7;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellHaste : public SkillObject
{    public:
        SpellHaste() : SkillObject(SPELL_HASTE)
        {
            _maxMana = 60;
            _minMana = 40;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_MAGE] = 44;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellInfravision : public SkillObject
{    public:
        SpellInfravision() : SkillObject(SPELL_INFRAVISION)
        {
            _maxMana = 25;
            _minMana = 10;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 3;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellInvisible : public SkillObject
{    public:
        SpellInvisible() : SkillObject(SPELL_INVISIBLE)
        {
            _maxMana = 35;
            _minMana = 25;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_OBJ_INV|TAR_OBJ_ROOM;
            _flags = MAG_MAGIC|MAG_AFFECTS|MAG_ALTER_OBJS;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 12;
            _level[CLASS_VAMPIRE] = 16;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellGlowlight : public SkillObject
{    public:
        SpellGlowlight() : SkillObject(SPELL_GLOWLIGHT)
        {
            _maxMana = 30;
            _minMana = 20;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_MAGE] = 8;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellKnock : public SkillObject
{    public:
        SpellKnock() : SkillObject(SPELL_KNOCK)
        {
            _maxMana = 50;
            _minMana = 20;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_DOOR;
            _flags = MAG_MAGIC|MAG_MANUAL;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 27;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellLightningBolt : public SkillObject
{    public:
        SpellLightningBolt() : SkillObject(SPELL_LIGHTNING_BOLT)
        {
            _maxMana = 50;
            _minMana = 15;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_MAGIC|MAG_DAMAGE|MAG_WATERZAP;
            _minPos = POS_FIGHTING;

            _level[CLASS_MAGE] = 15;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellLocateObject : public SkillObject
{    public:
        SpellLocateObject() : SkillObject(SPELL_LOCATE_OBJECT)
        {
            _maxMana = 25;
            _minMana = 20;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_OBJ_WORLD;
            _flags = MAG_MAGIC|MAG_MANUAL;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 23;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellMagicMissile : public SkillObject
{    public:
        SpellMagicMissile() : SkillObject(SPELL_MAGIC_MISSILE)
        {
            _maxMana = 20;
            _minMana = 8;
            _manaChange = 3;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_MAGIC|MAG_DAMAGE;
            _minPos = POS_FIGHTING;

            _level[CLASS_MAGE] = 1;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellMinorIdentify : public SkillObject
{    public:
        SpellMinorIdentify() : SkillObject(SPELL_MINOR_IDENTIFY)
        {
            _maxMana = 75;
            _minMana = 50;
            _manaChange = 3;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_OBJ_INV|TAR_OBJ_ROOM;
            _flags = MAG_MAGIC|MAG_MANUAL;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 15;
            _level[CLASS_CLERIC] = 46;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellMagicalProt : public SkillObject
{    public:
        SpellMagicalProt() : SkillObject(SPELL_MAGICAL_PROT)
        {
            _maxMana = 40;
            _minMana = 10;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 32;
            _level[CLASS_CLERIC] = 26;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellMagicalVestment : public SkillObject
{    public:
        SpellMagicalVestment() : SkillObject(SPELL_MAGICAL_VESTMENT)
        {
            _maxMana = 100;
            _minMana = 80;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_OBJ_INV;
            _flags = MAG_MANUAL|MAG_DIVINE;
            _minPos = POS_STANDING;

            _level[CLASS_CLERIC] = 11;
            _level[CLASS_KNIGHT] = 4;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellMeteorStorm : public SkillObject
{    public:
        SpellMeteorStorm() : SkillObject(SPELL_METEOR_STORM)
        {
            _maxMana = 180;
            _minMana = 110;
            _manaChange = 25;
            _violent = true;
            _targets = TAR_IGNORE;
            _flags = MAG_MAGIC|MAG_AREAS|MAG_NOWATER;
            _minPos = POS_FIGHTING;

            _level[CLASS_MAGE] = 48;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellChainLightning : public SkillObject
{    public:
        SpellChainLightning() : SkillObject(SPELL_CHAIN_LIGHTNING)
        {
            _maxMana = 120;
            _minMana = 30;
            _manaChange = 5;
            _violent = true;
            _targets = TAR_IGNORE;
            _flags = MAG_MAGIC|MAG_MASSES|MAG_WATERZAP;
            _minPos = POS_FIGHTING;

            _level[CLASS_MAGE] = 23;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellHailstorm : public SkillObject
{    public:
        SpellHailstorm() : SkillObject(SPELL_HAILSTORM)
        {
            _maxMana = 180;
            _minMana = 110;
            _manaChange = 10;
            _violent = true;
            _targets = TAR_IGNORE;
            _flags = MAG_MAGIC|MAG_MASSES|MAG_OUTDOORS;
            _minPos = POS_FIGHTING;

            _level[CLASS_RANGER] = 37;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellIceStorm : public SkillObject
{    public:
        SpellIceStorm() : SkillObject(SPELL_ICE_STORM)
        {
            _maxMana = 130;
            _minMana = 60;
            _manaChange = 15;
            _violent = true;
            _targets = TAR_IGNORE;
            _flags = MAG_MAGIC|MAG_MASSES;
            _minPos = POS_FIGHTING;

            _level[CLASS_CLERIC] = 49;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellPoison : public SkillObject
{    public:
        SpellPoison() : SkillObject(SPELL_POISON)
        {
            _maxMana = 50;
            _minMana = 20;
            _manaChange = 3;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_NOT_SELF|TAR_OBJ_INV;
            _flags = MAG_MAGIC|MAG_OBJECTS|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 20;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellPray : public SkillObject
{    public:
        SpellPray() : SkillObject(SPELL_PRAY)
        {
            _maxMana = 80;
            _minMana = 50;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_DIVINE|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_CLERIC] = 27;
            _level[CLASS_KNIGHT] = 30;
        }
        
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
            SkillAffect *sa = new SkillAffect();
            struct affected_type *af, *af2;

            SkillObject::perform(ch, target, sa);

            memset(&af, 0x0, sizeof(struct affected_type));
            af->location = APPLY_HITROLL;
            af->modifier = 1 + (ch->getLevelBonus(_skillNum) >> 3);
            af->duration = 4 + (ch->getLevelBonus(_skillNum) >> 4);
            af->type = _skillNum;
            af->level = ch->getLevelBonus(_skillNum);

            af2->location = APPLY_SAVING_SPELL;
            af2->modifier = -af->modifier;
            af2->duration = af->duration;

            sa->setAccumDuration(true);
            sa->addAffect(af);
            sa->addAffect(af2);
            sa->addToVictMessage("You feel someone protecting you.");

            ev.push_back(sa);
        }
};

class SpellPrismaticSpray : public SkillObject
{    public:
        SpellPrismaticSpray() : SkillObject(SPELL_PRISMATIC_SPRAY)
        {
            _maxMana = 160;
            _minMana = 80;
            _manaChange = 10;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_MAGIC|MAG_DAMAGE;
            _minPos = POS_FIGHTING;

            _level[CLASS_MAGE] = 42;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellProtectFromDevils : public SkillObject
{    public:
        SpellProtectFromDevils() : SkillObject(SPELL_PROTECT_FROM_DEVILS)
        {
            _maxMana = 40;
            _minMana = 15;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_DIVINE|MAG_AFFECTS;
            _minPos = POS_STANDING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellProtFromEvil : public SkillObject
{    public:
        SpellProtFromEvil() : SkillObject(SPELL_PROT_FROM_EVIL)
        {
            _maxMana = 40;
            _minMana = 15;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_DIVINE|MAG_AFFECTS|MAG_GOOD;
            _minPos = POS_STANDING;

            _level[CLASS_CLERIC] = 16;
            _level[CLASS_KNIGHT] = 9;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellProtFromGood : public SkillObject
{    public:
        SpellProtFromGood() : SkillObject(SPELL_PROT_FROM_GOOD)
        {
            _maxMana = 40;
            _minMana = 15;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_DIVINE|MAG_AFFECTS|MAG_EVIL;
            _minPos = POS_STANDING;

            _level[CLASS_CLERIC] = 16;
            _level[CLASS_KNIGHT] = 9;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellProtFromLightning : public SkillObject
{    public:
        SpellProtFromLightning() : SkillObject(SPELL_PROT_FROM_LIGHTNING)
        {
            _maxMana = 40;
            _minMana = 15;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_RANGER] = 9;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellProtFromFire : public SkillObject
{    public:
        SpellProtFromFire() : SkillObject(SPELL_PROT_FROM_FIRE)
        {
            _maxMana = 40;
            _minMana = 15;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_CLERIC] = 16;
            _level[CLASS_RANGER] = 19;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellRemoveCurse : public SkillObject
{    public:
        SpellRemoveCurse() : SkillObject(SPELL_REMOVE_CURSE)
        {
            _maxMana = 45;
            _minMana = 25;
            _manaChange = 5;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_OBJ_INV;
            _flags = MAG_DIVINE|MAG_UNAFFECTS|MAG_ALTER_OBJS;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 26;
            _level[CLASS_CLERIC] = 27;
            _level[CLASS_KNIGHT] = 31;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellRemoveSickness : public SkillObject
{    public:
        SpellRemoveSickness() : SkillObject(SPELL_REMOVE_SICKNESS)
        {
            _maxMana = 145;
            _minMana = 85;
            _manaChange = 5;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_DIVINE|MAG_UNAFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_CLERIC] = 37;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellRejuvenate : public SkillObject
{    public:
        SpellRejuvenate() : SkillObject(SPELL_REJUVENATE)
        {
            _maxMana = 100;
            _minMana = 40;
            _manaChange = 8;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_DIVINE|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_CLERIC] = 40;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellRefresh : public SkillObject
{    public:
        SpellRefresh() : SkillObject(SPELL_REFRESH)
        {
            _maxMana = 100;
            _minMana = 40;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_POINTS;
            _minPos = POS_STANDING;

            _level[CLASS_RANGER] = 15;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellRegenerate : public SkillObject
{    public:
        SpellRegenerate() : SkillObject(SPELL_REGENERATE)
        {
            _maxMana = 140;
            _minMana = 100;
            _manaChange = 10;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 49;
            _level[CLASS_CLERIC] = 45;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellRetrieveCorpse : public SkillObject
{    public:
        SpellRetrieveCorpse() : SkillObject(SPELL_RETRIEVE_CORPSE)
        {
            _maxMana = 125;
            _minMana = 65;
            _manaChange = 15;
            _violent = false;
            _targets = TAR_CHAR_WORLD;
            _flags = MAG_DIVINE|MAG_MANUAL;
            _minPos = POS_STANDING;

            _level[CLASS_CLERIC] = 48;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellSanctuary : public SkillObject
{    public:
        SpellSanctuary() : SkillObject(SPELL_SANCTUARY)
        {
            _maxMana = 110;
            _minMana = 85;
            _manaChange = 5;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_DIVINE|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_CLERIC] = 28;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellShockingGrasp : public SkillObject
{    public:
        SpellShockingGrasp() : SkillObject(SPELL_SHOCKING_GRASP)
        {
            _maxMana = 30;
            _minMana = 15;
            _manaChange = 3;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_MAGIC|MAG_TOUCH|MAG_DAMAGE;
            _minPos = POS_FIGHTING;

            _level[CLASS_MAGE] = 9;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellShroudObscurement : public SkillObject
{    public:
        SpellShroudObscurement() : SkillObject(SPELL_SHROUD_OBSCUREMENT)
        {
            _maxMana = 145;
            _minMana = 65;
            _manaChange = 10;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = MAG_MAGIC|POS_STANDING;

            _level[CLASS_MAGE] = 36;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellSleep : public SkillObject
{    public:
        SpellSleep() : SkillObject(SPELL_SLEEP)
        {
            _maxMana = 40;
            _minMana = 25;
            _manaChange = 5;
            _violent = true;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 10;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellSlow : public SkillObject
{    public:
        SpellSlow() : SkillObject(SPELL_SLOW)
        {
            _maxMana = 120;
            _minMana = 60;
            _manaChange = 6;
            _violent = false;
            _targets = TAR_UNPLEASANT|TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_MAGE] = 38;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellSpiritHammer : public SkillObject
{    public:
        SpellSpiritHammer() : SkillObject(SPELL_SPIRIT_HAMMER)
        {
            _maxMana = 40;
            _minMana = 10;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_DIVINE|MAG_DAMAGE;
            _minPos = POS_FIGHTING;

            _level[CLASS_CLERIC] = 10;
            _level[CLASS_KNIGHT] = 15;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellStrength : public SkillObject
{    public:
        SpellStrength() : SkillObject(SPELL_STRENGTH)
        {
            _maxMana = 65;
            _minMana = 30;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 6;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellSummon : public SkillObject
{    public:
        SpellSummon() : SkillObject(SPELL_SUMMON)
        {
            _maxMana = 320;
            _minMana = 180;
            _manaChange = 7;
            _violent = false;
            _targets = TAR_CHAR_WORLD|TAR_NOT_SELF;
            _flags = MAG_MAGIC|MAG_MANUAL;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 29;
            _level[CLASS_CLERIC] = 17;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellSword : public SkillObject
{    public:
        SpellSword() : SkillObject(SPELL_SWORD)
        {
            _maxMana = 265;
            _minMana = 180;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_IGNORE;
            _flags = MAG_MAGIC|MAG_MANUAL;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 30;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellSymbolOfPain : public SkillObject
{    public:
        SpellSymbolOfPain() : SkillObject(SPELL_SYMBOL_OF_PAIN)
        {
            _maxMana = 140;
            _minMana = 80;
            _manaChange = 5;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_DIVINE|MAG_DAMAGE|MAG_AFFECTS|MAG_EVIL;
            _minPos = POS_FIGHTING;

            _level[CLASS_CLERIC] = 38;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellStoneToFlesh : public SkillObject
{    public:
        SpellStoneToFlesh() : SkillObject(SPELL_STONE_TO_FLESH)
        {
            _maxMana = 380;
            _minMana = 230;
            _manaChange = 20;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_DIVINE|MAG_UNAFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_CLERIC] = 36;
            _level[CLASS_RANGER] = 44;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellTelekinesis : public SkillObject
{    public:
        SpellTelekinesis() : SkillObject(SPELL_TELEKINESIS)
        {
            _maxMana = 165;
            _minMana = 70;
            _manaChange = 5;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 28;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellWordStun : public SkillObject
{    public:
        SpellWordStun() : SkillObject(SPELL_WORD_STUN)
        {
            _maxMana = 100;
            _minMana = 50;
            _manaChange = 5;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_NOT_SELF;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 37;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellTrueSeeing : public SkillObject
{    public:
        SpellTrueSeeing() : SkillObject(SPELL_TRUE_SEEING)
        {
            _maxMana = 210;
            _minMana = 65;
            _manaChange = 15;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 41;
            _level[CLASS_CLERIC] = 42;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellWordOfRecall : public SkillObject
{    public:
        SpellWordOfRecall() : SkillObject(SPELL_WORD_OF_RECALL)
        {
            _maxMana = 100;
            _minMana = 20;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_OBJ_ROOM|TAR_OBJ_INV;
            _flags = MAG_DIVINE|MAG_MANUAL;
            _minPos = POS_FIGHTING;

            _level[CLASS_CLERIC] = 15;
            _level[CLASS_KNIGHT] = 25;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellWordOfIntellect : public SkillObject
{    public:
        SpellWordOfIntellect() : SkillObject(SPELL_WORD_OF_INTELLECT)
        {
            _maxMana = 100;
            _minMana = 40;
            _manaChange = 3;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_DIVINE|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_CLERIC] = 35;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellPeer : public SkillObject
{    public:
        SpellPeer() : SkillObject(SPELL_PEER)
        {
            _maxMana = 100;
            _minMana = 40;
            _manaChange = 6;
            _violent = false;
            _targets = TAR_OBJ_ROOM|TAR_OBJ_INV|TAR_OBJ_EQUIP;
            _flags = MAG_MAGIC|MAG_MANUAL;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 40;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellRemovePoison : public SkillObject
{    public:
        SpellRemovePoison() : SkillObject(SPELL_REMOVE_POISON)
        {
            _maxMana = 40;
            _minMana = 8;
            _manaChange = 4;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_OBJ_INV|TAR_OBJ_ROOM;
            _flags = MAG_DIVINE|MAG_UNAFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_CLERIC] = 8;
            _level[CLASS_KNIGHT] = 19;
            _level[CLASS_RANGER] = 12;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellRestoration : public SkillObject
{    public:
        SpellRestoration() : SkillObject(SPELL_RESTORATION)
        {
            _maxMana = 210;
            _minMana = 80;
            _manaChange = 11;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_DIVINE|MAG_POINTS;
            _minPos = POS_STANDING;

            _level[CLASS_CLERIC] = 43;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellSenseLife : public SkillObject
{    public:
        SpellSenseLife() : SkillObject(SPELL_SENSE_LIFE)
        {
            _maxMana = 120;
            _minMana = 80;
            _manaChange = 4;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 27;
            _level[CLASS_CLERIC] = 23;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellUndeadProt : public SkillObject
{    public:
        SpellUndeadProt() : SkillObject(SPELL_UNDEAD_PROT)
        {
            _maxMana = 60;
            _minMana = 10;
            _manaChange = 3;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 41;
            _level[CLASS_CLERIC] = 18;
            _level[CLASS_KNIGHT] = 18;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellWaterwalk : public SkillObject
{    public:
        SpellWaterwalk() : SkillObject(SPELL_WATERWALK)
        {
            _maxMana = 60;
            _minMana = 40;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 32;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellIdentify : public SkillObject
{    public:
        SpellIdentify() : SkillObject(SPELL_IDENTIFY)
        {
            _maxMana = 95;
            _minMana = 50;
            _manaChange = 3;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_OBJ_INV|TAR_OBJ_ROOM;
            _flags = MAG_MAGIC|MAG_MANUAL;
            _minPos = POS_STANDING;

            _level[CLASS_MAGE] = 25;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellWallOfThorns : public SkillObject
{    public:
        SpellWallOfThorns() : SkillObject(SPELL_WALL_OF_THORNS)
        {
            _maxMana = 50;
            _minMana = 20;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_DOOR;
            _flags = MAG_MAGIC|MAG_EXITS;
            _minPos = POS_STANDING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellWallOfStone : public SkillObject
{    public:
        SpellWallOfStone() : SkillObject(SPELL_WALL_OF_STONE)
        {
            _maxMana = 50;
            _minMana = 20;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_DOOR;
            _flags = MAG_MAGIC|MAG_EXITS;
            _minPos = POS_STANDING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellWallOfIce : public SkillObject
{    public:
        SpellWallOfIce() : SkillObject(SPELL_WALL_OF_ICE)
        {
            _maxMana = 50;
            _minMana = 20;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_DOOR;
            _flags = MAG_MAGIC|MAG_EXITS;
            _minPos = POS_STANDING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellWallOfFire : public SkillObject
{    public:
        SpellWallOfFire() : SkillObject(SPELL_WALL_OF_FIRE)
        {
            _maxMana = 50;
            _minMana = 20;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_DOOR;
            _flags = MAG_MAGIC|MAG_EXITS|MAG_NOWATER;
            _minPos = POS_STANDING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellWallOfIron : public SkillObject
{    public:
        SpellWallOfIron() : SkillObject(SPELL_WALL_OF_IRON)
        {
            _maxMana = 50;
            _minMana = 20;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_DOOR;
            _flags = MAG_MAGIC|MAG_EXITS;
            _minPos = POS_STANDING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellThornTrap : public SkillObject
{    public:
        SpellThornTrap() : SkillObject(SPELL_THORN_TRAP)
        {
            _maxMana = 50;
            _minMana = 20;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_DOOR;
            _flags = MAG_MAGIC|MAG_EXITS;
            _minPos = POS_STANDING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellFierySheet : public SkillObject
{    public:
        SpellFierySheet() : SkillObject(SPELL_FIERY_SHEET)
        {
            _maxMana = 50;
            _minMana = 20;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_DOOR;
            _flags = MAG_MAGIC|MAG_EXITS|MAG_NOWATER;
            _minPos = POS_STANDING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellPower : public SkillObject
{    public:
        SpellPower() : SkillObject(SPELL_POWER)
        {
            _maxMana = 80;
            _minMana = 50;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PSIONIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_PSIONIC] = 15;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellIntellect : public SkillObject
{    public:
        SpellIntellect() : SkillObject(SPELL_INTELLECT)
        {
            _maxMana = 60;
            _minMana = 40;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PSIONIC|MAG_AFFECTS|MAG_UNAFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_PSIONIC] = 23;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellConfusion : public SkillObject
{    public:
        SpellConfusion() : SkillObject(SPELL_CONFUSION)
        {
            _maxMana = 110;
            _minMana = 80;
            _manaChange = 5;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_PSIONIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_PSIONIC] = 36;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellFear : public SkillObject
{    public:
        SpellFear() : SkillObject(SPELL_FEAR)
        {
            _maxMana = 60;
            _minMana = 30;
            _manaChange = 2;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_PSIONIC|MAG_AFFECTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_PSIONIC] = 29;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellSatiation : public SkillObject
{    public:
        SpellSatiation() : SkillObject(SPELL_SATIATION)
        {
            _maxMana = 30;
            _minMana = 10;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PSIONIC|MAG_POINTS;
            _minPos = POS_STANDING;

            _level[CLASS_PSIONIC] = 3;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellQuench : public SkillObject
{    public:
        SpellQuench() : SkillObject(SPELL_QUENCH)
        {
            _maxMana = 35;
            _minMana = 15;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PSIONIC|MAG_POINTS;
            _minPos = POS_STANDING;

            _level[CLASS_PSIONIC] = 4;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellConfidence : public SkillObject
{    public:
        SpellConfidence() : SkillObject(SPELL_CONFIDENCE)
        {
            _maxMana = 60;
            _minMana = 40;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PSIONIC|MAG_AFFECTS|MAG_UNAFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_PSIONIC] = 8;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellNopain : public SkillObject
{    public:
        SpellNopain() : SkillObject(SPELL_NOPAIN)
        {
            _maxMana = 130;
            _minMana = 100;
            _manaChange = 5;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PSIONIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_PSIONIC] = 30;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellDermalHardening : public SkillObject
{    public:
        SpellDermalHardening() : SkillObject(SPELL_DERMAL_HARDENING)
        {
            _maxMana = 70;
            _minMana = 30;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PSIONIC|MAG_AFFECTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_PSIONIC] = 12;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellWoundClosure : public SkillObject
{    public:
        SpellWoundClosure() : SkillObject(SPELL_WOUND_CLOSURE)
        {
            _maxMana = 80;
            _minMana = 30;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PSIONIC|MAG_POINTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_PSIONIC] = 10;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellAntibody : public SkillObject
{    public:
        SpellAntibody() : SkillObject(SPELL_ANTIBODY)
        {
            _maxMana = 80;
            _minMana = 30;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PSIONIC|MAG_UNAFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_PSIONIC] = 19;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellRetina : public SkillObject
{    public:
        SpellRetina() : SkillObject(SPELL_RETINA)
        {
            _maxMana = 60;
            _minMana = 25;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PSIONIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_PSIONIC] = 13;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellAdrenaline : public SkillObject
{    public:
        SpellAdrenaline() : SkillObject(SPELL_ADRENALINE)
        {
            _maxMana = 80;
            _minMana = 60;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PSIONIC|MAG_AFFECTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_PSIONIC] = 27;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellBreathingStasis : public SkillObject
{    public:
        SpellBreathingStasis() : SkillObject(SPELL_BREATHING_STASIS)
        {
            _maxMana = 100;
            _minMana = 50;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PSIONIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_PSIONIC] = 21;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellVertigo : public SkillObject
{    public:
        SpellVertigo() : SkillObject(SPELL_VERTIGO)
        {
            _maxMana = 70;
            _minMana = 50;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_PSIONIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_PSIONIC] = 31;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellMetabolism : public SkillObject
{    public:
        SpellMetabolism() : SkillObject(SPELL_METABOLISM)
        {
            _maxMana = 50;
            _minMana = 30;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PSIONIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_PSIONIC] = 9;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellEgoWhip : public SkillObject
{    public:
        SpellEgoWhip() : SkillObject(SPELL_EGO_WHIP)
        {
            _maxMana = 50;
            _minMana = 30;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_DAMAGE|MAG_PSIONIC;
            _minPos = POS_FIGHTING;

            _level[CLASS_PSIONIC] = 22;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellPsychicCrush : public SkillObject
{    public:
        SpellPsychicCrush() : SkillObject(SPELL_PSYCHIC_CRUSH)
        {
            _maxMana = 130;
            _minMana = 30;
            _manaChange = 15;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_DAMAGE|MAG_PSIONIC|MAG_AFFECTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_PSIONIC] = 35;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellRelaxation : public SkillObject
{    public:
        SpellRelaxation() : SkillObject(SPELL_RELAXATION)
        {
            _maxMana = 100;
            _minMana = 40;
            _manaChange = 3;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PSIONIC|MAG_AFFECTS|MAG_UNAFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_PSIONIC] = 7;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellWeakness : public SkillObject
{    public:
        SpellWeakness() : SkillObject(SPELL_WEAKNESS)
        {
            _maxMana = 70;
            _minMana = 50;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_PSIONIC|MAG_AFFECTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_PSIONIC] = 16;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellFortressOfWill : public SkillObject
{    public:
        SpellFortressOfWill() : SkillObject(SPELL_FORTRESS_OF_WILL)
        {
            _maxMana = 30;
            _minMana = 30;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PSIONIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellCellRegen : public SkillObject
{    public:
        SpellCellRegen() : SkillObject(SPELL_CELL_REGEN)
        {
            _maxMana = 100;
            _minMana = 50;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PSIONIC|MAG_POINTS|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_PSIONIC] = 34;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellPsishield : public SkillObject
{    public:
        SpellPsishield() : SkillObject(SPELL_PSISHIELD)
        {
            _maxMana = 120;
            _minMana = 60;
            _manaChange = 5;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PSIONIC|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_PSIONIC] = 30;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellPsychicSurge : public SkillObject
{    public:
        SpellPsychicSurge() : SkillObject(SPELL_PSYCHIC_SURGE)
        {
            _maxMana = 90;
            _minMana = 60;
            _manaChange = 4;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_NOT_SELF;
            _flags = MAG_PSIONIC|MAG_DAMAGE;
            _minPos = POS_STANDING;

            _level[CLASS_PSIONIC] = 40;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellPsychicConduit : public SkillObject
{    public:
        SpellPsychicConduit() : SkillObject(SPELL_PSYCHIC_CONDUIT)
        {
            _maxMana = 70;
            _minMana = 50;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_NOT_SELF;
            _flags = MAG_PSIONIC|MAG_POINTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_PSIONIC] = 26;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellPsionicShatter : public SkillObject
{    public:
        SpellPsionicShatter() : SkillObject(SPELL_PSIONIC_SHATTER)
        {
            _maxMana = 80;
            _minMana = 30;
            _manaChange = 3;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_NOT_SELF|TAR_FIGHT_VICT;
            _flags = MAG_PSIONIC|MAG_UNAFFECTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_PSIONIC] = 33;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellIdInsinuation : public SkillObject
{    public:
        SpellIdInsinuation() : SkillObject(SPELL_ID_INSINUATION)
        {
            _maxMana = 110;
            _minMana = 80;
            _manaChange = 5;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_NOT_SELF|TAR_FIGHT_VICT;
            _flags = MAG_PSIONIC|MAG_MANUAL;
            _minPos = POS_STANDING;

            _level[CLASS_PSIONIC] = 32;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellMelatonicFlood : public SkillObject
{    public:
        SpellMelatonicFlood() : SkillObject(SPELL_MELATONIC_FLOOD)
        {
            _maxMana = 90;
            _minMana = 30;
            _manaChange = 5;
            _violent = true;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PSIONIC|MAG_AFFECTS|MAG_UNAFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_PSIONIC] = 18;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellMotorSpasm : public SkillObject
{    public:
        SpellMotorSpasm() : SkillObject(SPELL_MOTOR_SPASM)
        {
            _maxMana = 100;
            _minMana = 70;
            _manaChange = 5;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_NOT_SELF|TAR_FIGHT_VICT;
            _flags = MAG_DAMAGE|MAG_AFFECTS|MAG_PSIONIC;
            _minPos = POS_FIGHTING;

            _level[CLASS_PSIONIC] = 37;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellPsychicResistance : public SkillObject
{    public:
        SpellPsychicResistance() : SkillObject(SPELL_PSYCHIC_RESISTANCE)
        {
            _maxMana = 80;
            _minMana = 30;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_AFFECTS|MAG_PSIONIC;
            _minPos = POS_FIGHTING;

            _level[CLASS_PSIONIC] = 20;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellMassHysteria : public SkillObject
{    public:
        SpellMassHysteria() : SkillObject(SPELL_MASS_HYSTERIA)
        {
            _maxMana = 100;
            _minMana = 30;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_IGNORE;
            _flags = MAG_PSIONIC|MAG_AREAS;
            _minPos = POS_STANDING;

            _level[CLASS_PSIONIC] = 48;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellGroupConfidence : public SkillObject
{    public:
        SpellGroupConfidence() : SkillObject(SPELL_GROUP_CONFIDENCE)
        {
            _maxMana = 90;
            _minMana = 45;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_IGNORE;
            _flags = MAG_PSIONIC|MAG_GROUPS;
            _minPos = POS_STANDING;

            _level[CLASS_PSIONIC] = 14;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellClumsiness : public SkillObject
{    public:
        SpellClumsiness() : SkillObject(SPELL_CLUMSINESS)
        {
            _maxMana = 70;
            _minMana = 50;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_PSIONIC|MAG_AFFECTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_PSIONIC] = 28;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellEndurance : public SkillObject
{    public:
        SpellEndurance() : SkillObject(SPELL_ENDURANCE)
        {
            _maxMana = 90;
            _minMana = 50;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PSIONIC|MAG_AFFECTS|MAG_POINTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_PSIONIC] = 11;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellNullpsi : public SkillObject
{    public:
        SpellNullpsi() : SkillObject(SPELL_NULLPSI)
        {
            _maxMana = 90;
            _minMana = 50;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PSIONIC|MAG_UNAFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_PSIONIC] = 17;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellTelepathy : public SkillObject
{    public:
        SpellTelepathy() : SkillObject(SPELL_TELEPATHY)
        {
            _maxMana = 95;
            _minMana = 62;
            _manaChange = 4;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_PSIONIC|MAG_AFFECTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_PSIONIC] = 41;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellDistraction : public SkillObject
{    public:
        SpellDistraction() : SkillObject(SPELL_DISTRACTION)
        {
            _maxMana = 40;
            _minMana = 10;
            _manaChange = 4;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PSIONIC|MAG_MANUAL;
            _minPos = POS_STANDING;

            _level[CLASS_PSIONIC] = 2;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillPsiblast : public SkillObject
{    public:
        SkillPsiblast() : SkillObject(SKILL_PSIBLAST)
        {
            _maxMana = 50;
            _minMana = 50;
            _manaChange = 1;
            _violent = 0;
            _targets = 0;
            _flags = MAG_DAMAGE|MAG_PSIONIC;
            _minPos = 0;

            _level[CLASS_PSIONIC] = 5;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillPsilocate : public SkillObject
{    public:
        SkillPsilocate() : SkillObject(SKILL_PSILOCATE)
        {
            _maxMana = 100;
            _minMana = 30;
            _manaChange = 5;
            _violent = 0;
            _targets = 0;
            _flags = MAG_PSIONIC;
            _minPos = 0;

            _level[CLASS_PSIONIC] = 25;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillPsidrain : public SkillObject
{    public:
        SkillPsidrain() : SkillObject(SKILL_PSIDRAIN)
        {
            _maxMana = 50;
            _minMana = 30;
            _manaChange = 1;
            _violent = 0;
            _targets = 0;
            _flags = MAG_PSIONIC;
            _minPos = 0;

            _level[CLASS_PSIONIC] = 24;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellElectrostaticField : public SkillObject
{    public:
        SpellElectrostaticField() : SkillObject(SPELL_ELECTROSTATIC_FIELD)
        {
            _maxMana = 40;
            _minMana = 20;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_PHYSICS|MAG_AFFECTS;
            _minPos = POS_SITTING;

            _level[CLASS_PHYSIC] = 1;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellEntropyField : public SkillObject
{    public:
        SpellEntropyField() : SkillObject(SPELL_ENTROPY_FIELD)
        {
            _maxMana = 101;
            _minMana = 60;
            _manaChange = 8;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_PHYSICS|MAG_AFFECTS|MAG_DAMAGE;
            _minPos = POS_SITTING;

            _level[CLASS_PHYSIC] = 27;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellAcidity : public SkillObject
{    public:
        SpellAcidity() : SkillObject(SPELL_ACIDITY)
        {
            _maxMana = 40;
            _minMana = 20;
            _manaChange = 2;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_PHYSICS|MAG_DAMAGE|MAG_AFFECTS;
            _minPos = POS_SITTING;

            _level[CLASS_PHYSIC] = 2;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellAttractionField : public SkillObject
{    public:
        SpellAttractionField() : SkillObject(SPELL_ATTRACTION_FIELD)
        {
            _maxMana = 60;
            _minMana = 40;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_OBJ_ROOM|TAR_OBJ_INV|TAR_OBJ_EQUIP;
            _flags = MAG_PHYSICS|MAG_AFFECTS|MAG_ALTER_OBJS;
            _minPos = POS_SITTING;

            _level[CLASS_PHYSIC] = 8;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellNuclearWasteland : public SkillObject
{    public:
        SpellNuclearWasteland() : SkillObject(SPELL_NUCLEAR_WASTELAND)
        {
            _maxMana = 100;
            _minMana = 50;
            _manaChange = 10;
            _violent = true;
            _targets = TAR_IGNORE;
            _flags = MAG_PHYSICS|MAG_MANUAL;
            _minPos = POS_SITTING;

            _level[CLASS_PHYSIC] = 40;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellSpacetimeImprint : public SkillObject
{    public:
        SpellSpacetimeImprint() : SkillObject(SPELL_SPACETIME_IMPRINT)
        {
            _maxMana = 100;
            _minMana = 50;
            _manaChange = 10;
            _violent = true;
            _targets = TAR_IGNORE;
            _flags = MAG_PHYSICS|MAG_MANUAL;
            _minPos = POS_SITTING;

            _level[CLASS_PHYSIC] = 30;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellSpacetimeRecall : public SkillObject
{    public:
        SpellSpacetimeRecall() : SkillObject(SPELL_SPACETIME_RECALL)
        {
            _maxMana = 100;
            _minMana = 50;
            _manaChange = 10;
            _violent = true;
            _targets = TAR_IGNORE;
            _flags = MAG_PHYSICS|MAG_MANUAL;
            _minPos = POS_SITTING;

            _level[CLASS_PHYSIC] = 30;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellFluoresce : public SkillObject
{    public:
        SpellFluoresce() : SkillObject(SPELL_FLUORESCE)
        {
            _maxMana = 50;
            _minMana = 10;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PHYSICS|MAG_AFFECTS;
            _minPos = POS_SITTING;

            _level[CLASS_PHYSIC] = 3;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellGammaRay : public SkillObject
{    public:
        SpellGammaRay() : SkillObject(SPELL_GAMMA_RAY)
        {
            _maxMana = 100;
            _minMana = 50;
            _manaChange = 2;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_PHYSICS|MAG_DAMAGE|MAG_AFFECTS;
            _minPos = POS_SITTING;

            _level[CLASS_PHYSIC] = 25;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellGravityWell : public SkillObject
{    public:
        SpellGravityWell() : SkillObject(SPELL_GRAVITY_WELL)
        {
            _maxMana = 130;
            _minMana = 70;
            _manaChange = 4;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_PHYSICS|MAG_DAMAGE|MAG_AFFECTS;
            _minPos = POS_SITTING;

            _level[CLASS_PHYSIC] = 38;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellCapacitanceBoost : public SkillObject
{    public:
        SpellCapacitanceBoost() : SkillObject(SPELL_CAPACITANCE_BOOST)
        {
            _maxMana = 70;
            _minMana = 45;
            _manaChange = 3;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PHYSICS|MAG_AFFECTS|MAG_POINTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_PHYSIC] = 6;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellElectricArc : public SkillObject
{    public:
        SpellElectricArc() : SkillObject(SPELL_ELECTRIC_ARC)
        {
            _maxMana = 90;
            _minMana = 50;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_PHYSICS|MAG_DAMAGE|MAG_WATERZAP;
            _minPos = POS_FIGHTING;

            _level[CLASS_PHYSIC] = 21;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellTemporalCompression : public SkillObject
{    public:
        SpellTemporalCompression() : SkillObject(SPELL_TEMPORAL_COMPRESSION)
        {
            _maxMana = 80;
            _minMana = 50;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PHYSICS|MAG_AFFECTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_PHYSIC] = 29;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellTemporalDilation : public SkillObject
{    public:
        SpellTemporalDilation() : SkillObject(SPELL_TEMPORAL_DILATION)
        {
            _maxMana = 120;
            _minMana = 60;
            _manaChange = 6;
            _violent = false;
            _targets = TAR_UNPLEASANT|TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_PHYSICS|MAG_AFFECTS;
            _minPos = POS_FIGHTING;

            _level[CLASS_PHYSIC] = 36;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellHalflife : public SkillObject
{    public:
        SpellHalflife() : SkillObject(SPELL_HALFLIFE)
        {
            _maxMana = 120;
            _minMana = 70;
            _manaChange = 5;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_OBJ_EQUIP|TAR_OBJ_INV|TAR_OBJ_ROOM;
            _flags = MAG_PHYSICS|MAG_AFFECTS|MAG_ALTER_OBJS;
            _minPos = POS_SITTING;

            _level[CLASS_PHYSIC] = 30;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellMicrowave : public SkillObject
{    public:
        SpellMicrowave() : SkillObject(SPELL_MICROWAVE)
        {
            _maxMana = 45;
            _minMana = 20;
            _manaChange = 2;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_PHYSICS|MAG_DAMAGE;
            _minPos = POS_SITTING;

            _level[CLASS_PHYSIC] = 18;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellOxidize : public SkillObject
{    public:
        SpellOxidize() : SkillObject(SPELL_OXIDIZE)
        {
            _maxMana = 30;
            _minMana = 15;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_PHYSICS|MAG_DAMAGE;
            _minPos = POS_SITTING;

            _level[CLASS_PHYSIC] = 10;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellRandomCoordinates : public SkillObject
{    public:
        SpellRandomCoordinates() : SkillObject(SPELL_RANDOM_COORDINATES)
        {
            _maxMana = 100;
            _minMana = 20;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PHYSICS|MAG_MANUAL;
            _minPos = POS_SITTING;

            _level[CLASS_PHYSIC] = 20;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellRepulsionField : public SkillObject
{    public:
        SpellRepulsionField() : SkillObject(SPELL_REPULSION_FIELD)
        {
            _maxMana = 30;
            _minMana = 20;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_OBJ_ROOM|TAR_OBJ_EQUIP|TAR_OBJ_INV;
            _flags = MAG_PHYSICS|MAG_AFFECTS|MAG_ALTER_OBJS;
            _minPos = POS_SITTING;

            _level[CLASS_PHYSIC] = 16;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellVacuumShroud : public SkillObject
{    public:
        SpellVacuumShroud() : SkillObject(SPELL_VACUUM_SHROUD)
        {
            _maxMana = 60;
            _minMana = 40;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PHYSICS|MAG_AFFECTS;
            _minPos = POS_SITTING;

            _level[CLASS_PHYSIC] = 31;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellAlbedoShield : public SkillObject
{    public:
        SpellAlbedoShield() : SkillObject(SPELL_ALBEDO_SHIELD)
        {
            _maxMana = 90;
            _minMana = 50;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PHYSICS|MAG_AFFECTS;
            _minPos = POS_SITTING;

            _level[CLASS_PHYSIC] = 34;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellTransmittance : public SkillObject
{    public:
        SpellTransmittance() : SkillObject(SPELL_TRANSMITTANCE)
        {
            _maxMana = 60;
            _minMana = 30;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_OBJ_ROOM|TAR_OBJ_EQUIP|TAR_OBJ_INV;
            _flags = MAG_PHYSICS|MAG_AFFECTS|MAG_ALTER_OBJS;
            _minPos = POS_SITTING;

            _level[CLASS_PHYSIC] = 14;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellTimeWarp : public SkillObject
{    public:
        SpellTimeWarp() : SkillObject(SPELL_TIME_WARP)
        {
            _maxMana = 100;
            _minMana = 50;
            _manaChange = 8;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PHYSICS|MAG_MANUAL;
            _minPos = POS_SITTING;

            _level[CLASS_PHYSIC] = 35;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellRadioimmunity : public SkillObject
{    public:
        SpellRadioimmunity() : SkillObject(SPELL_RADIOIMMUNITY)
        {
            _maxMana = 80;
            _minMana = 20;
            _manaChange = 8;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PHYSICS|MAG_AFFECTS;
            _minPos = POS_SITTING;

            _level[CLASS_PHYSIC] = 20;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellDensify : public SkillObject
{    public:
        SpellDensify() : SkillObject(SPELL_DENSIFY)
        {
            _maxMana = 80;
            _minMana = 20;
            _manaChange = 8;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_OBJ_EQUIP|TAR_OBJ_ROOM|TAR_OBJ_INV;
            _flags = MAG_PHYSICS|MAG_AFFECTS|MAG_ALTER_OBJS;
            _minPos = POS_SITTING;

            _level[CLASS_PHYSIC] = 26;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellLatticeHardening : public SkillObject
{    public:
        SpellLatticeHardening() : SkillObject(SPELL_LATTICE_HARDENING)
        {
            _maxMana = 100;
            _minMana = 40;
            _manaChange = 4;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_OBJ_ROOM|TAR_OBJ_INV;
            _flags = MAG_PHYSICS|MAG_AFFECTS|MAG_ALTER_OBJS;
            _minPos = POS_SITTING;

            _level[CLASS_PHYSIC] = 28;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellChemicalStability : public SkillObject
{    public:
        SpellChemicalStability() : SkillObject(SPELL_CHEMICAL_STABILITY)
        {
            _maxMana = 80;
            _minMana = 20;
            _manaChange = 8;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PHYSICS|MAG_AFFECTS|MAG_UNAFFECTS;
            _minPos = POS_SITTING;

            _level[CLASS_PHYSIC] = 9;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellRefraction : public SkillObject
{    public:
        SpellRefraction() : SkillObject(SPELL_REFRACTION)
        {
            _maxMana = 80;
            _minMana = 20;
            _manaChange = 8;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PHYSICS|MAG_AFFECTS;
            _minPos = POS_SITTING;

            _level[CLASS_PHYSIC] = 33;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellNullify : public SkillObject
{    public:
        SpellNullify() : SkillObject(SPELL_NULLIFY)
        {
            _maxMana = 90;
            _minMana = 55;
            _manaChange = 5;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PHYSICS|MAG_UNAFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_PHYSIC] = 11;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellAreaStasis : public SkillObject
{    public:
        SpellAreaStasis() : SkillObject(SPELL_AREA_STASIS)
        {
            _maxMana = 100;
            _minMana = 50;
            _manaChange = 5;
            _violent = false;
            _targets = TAR_IGNORE;
            _flags = MAG_PHYSICS|MAG_MANUAL;
            _minPos = POS_SITTING;

            _level[CLASS_PHYSIC] = 40;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellEmpPulse : public SkillObject
{    public:
        SpellEmpPulse() : SkillObject(SPELL_EMP_PULSE)
        {
            _maxMana = 145;
            _minMana = 75;
            _manaChange = 5;
            _violent = false;
            _targets = TAR_IGNORE;
            _flags = MAG_PHYSICS|MAG_MANUAL;
            _minPos = POS_SITTING;

            _level[CLASS_PHYSIC] = 34;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellFissionBlast : public SkillObject
{    public:
        SpellFissionBlast() : SkillObject(SPELL_FISSION_BLAST)
        {
            _maxMana = 150;
            _minMana = 70;
            _manaChange = 10;
            _violent = true;
            _targets = TAR_IGNORE;
            _flags = MAG_PHYSICS|MAG_AREAS;
            _minPos = POS_FIGHTING;

            _level[CLASS_PHYSIC] = 43;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillAmbush : public SkillObject
{    public:
        SkillAmbush() : SkillObject(SKILL_AMBUSH)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = true;
            _targets = TAR_IGNORE;
            _flags = MAG_MANUAL;
            _minPos = POS_STANDING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class ZenHealing : public SkillObject
{    public:
        ZenHealing() : SkillObject(ZEN_HEALING)
        {
            _maxMana = 30;
            _minMana = 9;
            _manaChange = 2;
            _violent = 0;
            _targets = 0;
            _flags = MAG_ZEN;
            _minPos = 0;

            _level[CLASS_MONK] = 36;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class ZenAwareness : public SkillObject
{    public:
        ZenAwareness() : SkillObject(ZEN_AWARENESS)
        {
            _maxMana = 30;
            _minMana = 5;
            _manaChange = 1;
            _violent = 0;
            _targets = 0;
            _flags = MAG_ZEN;
            _minPos = 0;

            _level[CLASS_MONK] = 21;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class ZenOblivity : public SkillObject
{    public:
        ZenOblivity() : SkillObject(ZEN_OBLIVITY)
        {
            _maxMana = 40;
            _minMana = 10;
            _manaChange = 4;
            _violent = 0;
            _targets = 0;
            _flags = MAG_ZEN;
            _minPos = 0;

            _level[CLASS_MONK] = 43;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class ZenMotion : public SkillObject
{    public:
        ZenMotion() : SkillObject(ZEN_MOTION)
        {
            _maxMana = 30;
            _minMana = 7;
            _manaChange = 1;
            _violent = 0;
            _targets = 0;
            _flags = MAG_ZEN;
            _minPos = 0;

            _level[CLASS_MONK] = 28;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellTidalSpacewarp : public SkillObject
{    public:
        SpellTidalSpacewarp() : SkillObject(SPELL_TIDAL_SPACEWARP)
        {
            _maxMana = 100;
            _minMana = 50;
            _manaChange = 2;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_PHYSICS|MAG_AFFECTS;
            _minPos = POS_STANDING;

            _level[CLASS_PHYSIC] = 15;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillCleave : public SkillObject
{    public:
        SkillCleave() : SkillObject(SKILL_CLEAVE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_BARB] = 30;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillStrike : public SkillObject
{    public:
        SkillStrike() : SkillObject(SKILL_STRIKE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_BARB] = 17;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillHamstring : public SkillObject
{    public:
        SkillHamstring() : SkillObject(SKILL_HAMSTRING)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_HOOD] = 32;
            _level[CLASS_MERCENARY] = 32;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillDrag : public SkillObject
{    public:
        SkillDrag() : SkillObject(SKILL_DRAG)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_BARB] = 40;
            _level[CLASS_HOOD] = 30;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillSnatch : public SkillObject
{    public:
        SkillSnatch() : SkillObject(SKILL_SNATCH)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_HOOD] = 40;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillArchery : public SkillObject
{    public:
        SkillArchery() : SkillObject(SKILL_ARCHERY)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 14;
            _level[CLASS_WARRIOR] = 5;
            _level[CLASS_BARB] = 24;
            _level[CLASS_RANGER] = 9;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillBowFletch : public SkillObject
{    public:
        SkillBowFletch() : SkillObject(SKILL_BOW_FLETCH)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 10;
            _level[CLASS_RANGER] = 17;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillReadScrolls : public SkillObject
{    public:
        SkillReadScrolls() : SkillObject(SKILL_READ_SCROLLS)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MAGE] = 1;
            _level[CLASS_CLERIC] = 2;
            _level[CLASS_THIEF] = 3;
            _level[CLASS_PSIONIC] = 35;
            _level[CLASS_KNIGHT] = 21;
            _level[CLASS_RANGER] = 21;
            _level[CLASS_MONK] = 7;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillUseWands : public SkillObject
{    public:
        SkillUseWands() : SkillObject(SKILL_USE_WANDS)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MAGE] = 4;
            _level[CLASS_CLERIC] = 9;
            _level[CLASS_THIEF] = 10;
            _level[CLASS_PSIONIC] = 20;
            _level[CLASS_KNIGHT] = 21;
            _level[CLASS_RANGER] = 29;
            _level[CLASS_MONK] = 35;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillBackstab : public SkillObject
{    public:
        SkillBackstab() : SkillObject(SKILL_BACKSTAB)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 5;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillCircle : public SkillObject
{    public:
        SkillCircle() : SkillObject(SKILL_CIRCLE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 33;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillHide : public SkillObject
{    public:
        SkillHide() : SkillObject(SKILL_HIDE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 3;
            _level[CLASS_RANGER] = 15;
            _level[CLASS_HOOD] = 7;
            _level[CLASS_MERCENARY] = 30;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillKick : public SkillObject
{    public:
        SkillKick() : SkillObject(SKILL_KICK)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 2;
            _level[CLASS_CYBORG] = 3;
            _level[CLASS_RANGER] = 3;
            _level[CLASS_MERCENARY] = 2;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillBash : public SkillObject
{    public:
        SkillBash() : SkillObject(SKILL_BASH)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 10;
            _level[CLASS_KNIGHT] = 9;
            _level[CLASS_RANGER] = 14;
            _level[CLASS_MERCENARY] = 4;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillBreakDoor : public SkillObject
{    public:
        SkillBreakDoor() : SkillObject(SKILL_BREAK_DOOR)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_BARB] = 26;
            _level[CLASS_MERCENARY] = 1;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillHeadbutt : public SkillObject
{    public:
        SkillHeadbutt() : SkillObject(SKILL_HEADBUTT)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 13;
            _level[CLASS_RANGER] = 16;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillHotwire : public SkillObject
{    public:
        SkillHotwire() : SkillObject(SKILL_HOTWIRE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillGouge : public SkillObject
{    public:
        SkillGouge() : SkillObject(SKILL_GOUGE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 30;
            _level[CLASS_MONK] = 36;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillStun : public SkillObject
{    public:
        SkillStun() : SkillObject(SKILL_STUN)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = MAG_BIOLOGIC;
            _minPos = 0;

            _level[CLASS_THIEF] = 35;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillFeign : public SkillObject
{    public:
        SkillFeign() : SkillObject(SKILL_FEIGN)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 25;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillConceal : public SkillObject
{    public:
        SkillConceal() : SkillObject(SKILL_CONCEAL)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 15;
            _level[CLASS_HOOD] = 16;
            _level[CLASS_MERCENARY] = 30;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillPlant : public SkillObject
{    public:
        SkillPlant() : SkillObject(SKILL_PLANT)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 20;
            _level[CLASS_MERCENARY] = 35;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillPickLock : public SkillObject
{    public:
        SkillPickLock() : SkillObject(SKILL_PICK_LOCK)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 1;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillRescue : public SkillObject
{    public:
        SkillRescue() : SkillObject(SKILL_RESCUE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 3;
            _level[CLASS_BARB] = 12;
            _level[CLASS_KNIGHT] = 8;
            _level[CLASS_RANGER] = 12;
            _level[CLASS_MONK] = 16;
            _level[CLASS_MERCENARY] = 26;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillSneak : public SkillObject
{    public:
        SkillSneak() : SkillObject(SKILL_SNEAK)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 3;
            _level[CLASS_RANGER] = 8;
            _level[CLASS_VAMPIRE] = 7;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillSteal : public SkillObject
{    public:
        SkillSteal() : SkillObject(SKILL_STEAL)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 2;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillTrack : public SkillObject
{    public:
        SkillTrack() : SkillObject(SKILL_TRACK)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 8;
            _level[CLASS_WARRIOR] = 9;
            _level[CLASS_BARB] = 14;
            _level[CLASS_CYBORG] = 11;
            _level[CLASS_RANGER] = 11;
            _level[CLASS_VAMPIRE] = 5;
            _level[CLASS_MERCENARY] = 14;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillPunch : public SkillObject
{    public:
        SkillPunch() : SkillObject(SKILL_PUNCH)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MAGE] = 1;
            _level[CLASS_CLERIC] = 1;
            _level[CLASS_THIEF] = 1;
            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 1;
            _level[CLASS_PSIONIC] = 1;
            _level[CLASS_PHYSIC] = 1;
            _level[CLASS_CYBORG] = 1;
            _level[CLASS_KNIGHT] = 1;
            _level[CLASS_RANGER] = 1;
            _level[CLASS_HOOD] = 1;
            _level[CLASS_MONK] = 1;
            _level[CLASS_VAMPIRE] = 1;
            _level[CLASS_MERCENARY] = 1;
            _level[CLASS_SPARE1] = 1;
            _level[CLASS_SPARE2] = 1;
            _level[CLASS_SPARE3] = 1;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillPiledrive : public SkillObject
{    public:
        SkillPiledrive() : SkillObject(SKILL_PILEDRIVE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillShieldMastery : public SkillObject
{    public:
        SkillShieldMastery() : SkillObject(SKILL_SHIELD_MASTERY)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_KNIGHT] = 22;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillSleeper : public SkillObject
{    public:
        SkillSleeper() : SkillObject(SKILL_SLEEPER)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = MAG_BIOLOGIC;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 25;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillElbow : public SkillObject
{    public:
        SkillElbow() : SkillObject(SKILL_ELBOW)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 8;
            _level[CLASS_CYBORG] = 9;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillKnee : public SkillObject
{    public:
        SkillKnee() : SkillObject(SKILL_KNEE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 6;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillAutopsy : public SkillObject
{    public:
        SkillAutopsy() : SkillObject(SKILL_AUTOPSY)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_CYBORG] = 40;
            _level[CLASS_RANGER] = 36;
            _level[CLASS_MERCENARY] = 10;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillBerserk : public SkillObject
{    public:
        SkillBerserk() : SkillObject(SKILL_BERSERK)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = MAG_BIOLOGIC;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 22;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillBattleCry : public SkillObject
{    public:
        SkillBattleCry() : SkillObject(SKILL_BATTLE_CRY)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 33;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillKia : public SkillObject
{    public:
        SkillKia() : SkillObject(SKILL_KIA)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_MONK] = 23;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillCryFromBeyond : public SkillObject
{    public:
        SkillCryFromBeyond() : SkillObject(SKILL_CRY_FROM_BEYOND)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 42;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillStomp : public SkillObject
{    public:
        SkillStomp() : SkillObject(SKILL_STOMP)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 5;
            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 4;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillBodyslam : public SkillObject
{    public:
        SkillBodyslam() : SkillObject(SKILL_BODYSLAM)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 20;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillChoke : public SkillObject
{    public:
        SkillChoke() : SkillObject(SKILL_CHOKE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 23;
            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 16;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillClothesline : public SkillObject
{    public:
        SkillClothesline() : SkillObject(SKILL_CLOTHESLINE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 18;
            _level[CLASS_MERCENARY] = 12;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillTag : public SkillObject
{    public:
        SkillTag() : SkillObject(SKILL_TAG)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 27;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillIntimidate : public SkillObject
{    public:
        SkillIntimidate() : SkillObject(SKILL_INTIMIDATE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 35;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillSpeedHealing : public SkillObject
{    public:
        SkillSpeedHealing() : SkillObject(SKILL_SPEED_HEALING)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 10;
            _level[CLASS_BARB] = 39;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillStalk : public SkillObject
{    public:
        SkillStalk() : SkillObject(SKILL_STALK)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 40;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillHearing : public SkillObject
{    public:
        SkillHearing() : SkillObject(SKILL_HEARING)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 11;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillBearhug : public SkillObject
{    public:
        SkillBearhug() : SkillObject(SKILL_BEARHUG)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_RANGER] = 25;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillBite : public SkillObject
{    public:
        SkillBite() : SkillObject(SKILL_BITE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_RANGER] = 5;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillDblAttack : public SkillObject
{    public:
        SkillDblAttack() : SkillObject(SKILL_DBL_ATTACK)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 45;
            _level[CLASS_WARRIOR] = 30;
            _level[CLASS_BARB] = 37;
            _level[CLASS_KNIGHT] = 25;
            _level[CLASS_RANGER] = 30;
            _level[CLASS_HOOD] = 42;
            _level[CLASS_MONK] = 24;
            _level[CLASS_MERCENARY] = 25;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillNightVision : public SkillObject
{    public:
        SkillNightVision() : SkillObject(SKILL_NIGHT_VISION)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_RANGER] = 2;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillTurn : public SkillObject
{    public:
        SkillTurn() : SkillObject(SKILL_TURN)
        {
            _maxMana = 90;
            _minMana = 30;
            _manaChange = 5;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_CLERIC] = 1;
            _level[CLASS_KNIGHT] = 3;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillAnalyze : public SkillObject
{    public:
        SkillAnalyze() : SkillObject(SKILL_ANALYZE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_PHYSIC] = 12;
            _level[CLASS_CYBORG] = 20;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillEvaluate : public SkillObject
{    public:
        SkillEvaluate() : SkillObject(SKILL_EVALUATE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_PHYSIC] = 10;
            _level[CLASS_CYBORG] = 20;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillCureDisease : public SkillObject
{    public:
        SkillCureDisease() : SkillObject(SKILL_CURE_DISEASE)
        {
            _maxMana = 30;
            _minMana = 10;
            _manaChange = 1;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_KNIGHT] = 43;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillHolyTouch : public SkillObject
{    public:
        SkillHolyTouch() : SkillObject(SKILL_HOLY_TOUCH)
        {
            _maxMana = 30;
            _minMana = 10;
            _manaChange = 1;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_KNIGHT] = 2;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillBandage : public SkillObject
{    public:
        SkillBandage() : SkillObject(SKILL_BANDAGE)
        {
            _maxMana = 40;
            _minMana = 30;
            _manaChange = 1;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 34;
            _level[CLASS_RANGER] = 10;
            _level[CLASS_MONK] = 17;
            _level[CLASS_MERCENARY] = 3;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillFirstaid : public SkillObject
{    public:
        SkillFirstaid() : SkillObject(SKILL_FIRSTAID)
        {
            _maxMana = 70;
            _minMana = 60;
            _manaChange = 1;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_RANGER] = 19;
            _level[CLASS_MONK] = 30;
            _level[CLASS_MERCENARY] = 16;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillMedic : public SkillObject
{    public:
        SkillMedic() : SkillObject(SKILL_MEDIC)
        {
            _maxMana = 90;
            _minMana = 80;
            _manaChange = 1;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_RANGER] = 40;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillLeatherworking : public SkillObject
{    public:
        SkillLeatherworking() : SkillObject(SKILL_LEATHERWORKING)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 32;
            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 26;
            _level[CLASS_KNIGHT] = 29;
            _level[CLASS_RANGER] = 28;
            _level[CLASS_MONK] = 32;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillMetalworking : public SkillObject
{    public:
        SkillMetalworking() : SkillObject(SKILL_METALWORKING)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 41;
            _level[CLASS_KNIGHT] = 32;
            _level[CLASS_RANGER] = 36;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillConsider : public SkillObject
{    public:
        SkillConsider() : SkillObject(SKILL_CONSIDER)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 11;
            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 9;
            _level[CLASS_KNIGHT] = 13;
            _level[CLASS_RANGER] = 18;
            _level[CLASS_MONK] = 13;
            _level[CLASS_MERCENARY] = 13;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillGlance : public SkillObject
{    public:
        SkillGlance() : SkillObject(SKILL_GLANCE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 4;
            _level[CLASS_VAMPIRE] = 14;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillShoot : public SkillObject
{    public:
        SkillShoot() : SkillObject(SKILL_SHOOT)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 10;
            _level[CLASS_CYBORG] = 20;
            _level[CLASS_RANGER] = 26;
            _level[CLASS_HOOD] = 20;
            _level[CLASS_VAMPIRE] = 10;
            _level[CLASS_MERCENARY] = 3;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillBehead : public SkillObject
{    public:
        SkillBehead() : SkillObject(SKILL_BEHEAD)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_KNIGHT] = 40;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillEmpower : public SkillObject
{    public:
        SkillEmpower() : SkillObject(SKILL_EMPOWER)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MAGE] = 5;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillDisarm : public SkillObject
{    public:
        SkillDisarm() : SkillObject(SKILL_DISARM)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 13;
            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 32;
            _level[CLASS_KNIGHT] = 20;
            _level[CLASS_RANGER] = 20;
            _level[CLASS_MONK] = 31;
            _level[CLASS_MERCENARY] = 25;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillSpinkick : public SkillObject
{    public:
        SkillSpinkick() : SkillObject(SKILL_SPINKICK)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 17;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillRoundhouse : public SkillObject
{    public:
        SkillRoundhouse() : SkillObject(SKILL_ROUNDHOUSE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 19;
            _level[CLASS_RANGER] = 20;
            _level[CLASS_MERCENARY] = 5;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillSidekick : public SkillObject
{    public:
        SkillSidekick() : SkillObject(SKILL_SIDEKICK)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 29;
            _level[CLASS_RANGER] = 35;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillSpinfist : public SkillObject
{    public:
        SkillSpinfist() : SkillObject(SKILL_SPINFIST)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 7;
            _level[CLASS_KNIGHT] = 6;
            _level[CLASS_RANGER] = 7;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillJab : public SkillObject
{    public:
        SkillJab() : SkillObject(SKILL_JAB)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 6;
            _level[CLASS_WARRIOR] = 1;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillHook : public SkillObject
{    public:
        SkillHook() : SkillObject(SKILL_HOOK)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 23;
            _level[CLASS_KNIGHT] = 22;
            _level[CLASS_RANGER] = 23;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillSweepkick : public SkillObject
{    public:
        SkillSweepkick() : SkillObject(SKILL_SWEEPKICK)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 28;
            _level[CLASS_RANGER] = 27;
            _level[CLASS_MONK] = 25;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillTrip : public SkillObject
{    public:
        SkillTrip() : SkillObject(SKILL_TRIP)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 18;
            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_HOOD] = 15;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillUppercut : public SkillObject
{    public:
        SkillUppercut() : SkillObject(SKILL_UPPERCUT)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 11;
            _level[CLASS_KNIGHT] = 15;
            _level[CLASS_RANGER] = 13;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillGroinkick : public SkillObject
{    public:
        SkillGroinkick() : SkillObject(SKILL_GROINKICK)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 12;
            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_HOOD] = 7;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillClaw : public SkillObject
{    public:
        SkillClaw() : SkillObject(SKILL_CLAW)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_RANGER] = 32;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillRabbitpunch : public SkillObject
{    public:
        SkillRabbitpunch() : SkillObject(SKILL_RABBITPUNCH)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 10;
            _level[CLASS_WARRIOR] = 1;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillImpale : public SkillObject
{    public:
        SkillImpale() : SkillObject(SKILL_IMPALE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 43;
            _level[CLASS_KNIGHT] = 30;
            _level[CLASS_RANGER] = 45;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillPeleKick : public SkillObject
{    public:
        SkillPeleKick() : SkillObject(SKILL_PELE_KICK)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 41;
            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 40;
            _level[CLASS_RANGER] = 41;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillLungePunch : public SkillObject
{    public:
        SkillLungePunch() : SkillObject(SKILL_LUNGE_PUNCH)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 32;
            _level[CLASS_KNIGHT] = 35;
            _level[CLASS_RANGER] = 42;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillTornadoKick : public SkillObject
{    public:
        SkillTornadoKick() : SkillObject(SKILL_TORNADO_KICK)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_BARB] = 45;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillTripleAttack : public SkillObject
{    public:
        SkillTripleAttack() : SkillObject(SKILL_TRIPLE_ATTACK)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_WARRIOR] = 40;
            _level[CLASS_BARB] = 48;
            _level[CLASS_KNIGHT] = 45;
            _level[CLASS_RANGER] = 43;
            _level[CLASS_MONK] = 41;
            _level[CLASS_MERCENARY] = 38;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillCounterAttack : public SkillObject
{    public:
        SkillCounterAttack() : SkillObject(SKILL_COUNTER_ATTACK)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_RANGER] = 31;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillSwimming : public SkillObject
{    public:
        SkillSwimming() : SkillObject(SKILL_SWIMMING)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MAGE] = 1;
            _level[CLASS_CLERIC] = 1;
            _level[CLASS_THIEF] = 1;
            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 1;
            _level[CLASS_PSIONIC] = 2;
            _level[CLASS_PHYSIC] = 1;
            _level[CLASS_KNIGHT] = 1;
            _level[CLASS_RANGER] = 1;
            _level[CLASS_HOOD] = 1;
            _level[CLASS_MONK] = 7;
            _level[CLASS_MERCENARY] = 1;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillThrowing : public SkillObject
{    public:
        SkillThrowing() : SkillObject(SKILL_THROWING)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 7;
            _level[CLASS_WARRIOR] = 5;
            _level[CLASS_BARB] = 15;
            _level[CLASS_RANGER] = 9;
            _level[CLASS_HOOD] = 3;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillRiding : public SkillObject
{    public:
        SkillRiding() : SkillObject(SKILL_RIDING)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MAGE] = 1;
            _level[CLASS_CLERIC] = 1;
            _level[CLASS_THIEF] = 1;
            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_BARB] = 1;
            _level[CLASS_KNIGHT] = 1;
            _level[CLASS_RANGER] = 1;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillAppraise : public SkillObject
{    public:
        SkillAppraise() : SkillObject(SKILL_APPRAISE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MAGE] = 10;
            _level[CLASS_CLERIC] = 10;
            _level[CLASS_THIEF] = 10;
            _level[CLASS_BARB] = 10;
            _level[CLASS_PSIONIC] = 10;
            _level[CLASS_PHYSIC] = 10;
            _level[CLASS_CYBORG] = 10;
            _level[CLASS_KNIGHT] = 10;
            _level[CLASS_RANGER] = 10;
            _level[CLASS_MONK] = 10;
            _level[CLASS_MERCENARY] = 10;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillPipemaking : public SkillObject
{    public:
        SkillPipemaking() : SkillObject(SKILL_PIPEMAKING)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MAGE] = 40;
            _level[CLASS_WARRIOR] = 1;
            _level[CLASS_RANGER] = 15;
            _level[CLASS_HOOD] = 13;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillSecondWeapon : public SkillObject
{    public:
        SkillSecondWeapon() : SkillObject(SKILL_SECOND_WEAPON)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_RANGER] = 14;
            _level[CLASS_MERCENARY] = 18;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillScanning : public SkillObject
{    public:
        SkillScanning() : SkillObject(SKILL_SCANNING)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 31;
            _level[CLASS_RANGER] = 24;
            _level[CLASS_MERCENARY] = 22;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillRetreat : public SkillObject
{    public:
        SkillRetreat() : SkillObject(SKILL_RETREAT)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_THIEF] = 16;
            _level[CLASS_WARRIOR] = 16;
            _level[CLASS_RANGER] = 18;
            _level[CLASS_HOOD] = 17;
            _level[CLASS_MONK] = 33;
            _level[CLASS_MERCENARY] = 22;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillGunsmithing : public SkillObject
{    public:
        SkillGunsmithing() : SkillObject(SKILL_GUNSMITHING)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MERCENARY] = 20;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillPistolwhip : public SkillObject
{    public:
        SkillPistolwhip() : SkillObject(SKILL_PISTOLWHIP)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MERCENARY] = 20;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillCrossface : public SkillObject
{    public:
        SkillCrossface() : SkillObject(SKILL_CROSSFACE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MERCENARY] = 26;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillWrench : public SkillObject
{    public:
        SkillWrench() : SkillObject(SKILL_WRENCH)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MERCENARY] = 6;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillElusion : public SkillObject
{    public:
        SkillElusion() : SkillObject(SKILL_ELUSION)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MERCENARY] = 11;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillInfiltrate : public SkillObject
{    public:
        SkillInfiltrate() : SkillObject(SKILL_INFILTRATE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MERCENARY] = 27;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillShoulderThrow : public SkillObject
{    public:
        SkillShoulderThrow() : SkillObject(SKILL_SHOULDER_THROW)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MERCENARY] = 39;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillProfPound : public SkillObject
{    public:
        SkillProfPound() : SkillObject(SKILL_PROF_POUND)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_BARB] = 3;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillProfWhip : public SkillObject
{    public:
        SkillProfWhip() : SkillObject(SKILL_PROF_WHIP)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_BARB] = 11;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillProfPierce : public SkillObject
{    public:
        SkillProfPierce() : SkillObject(SKILL_PROF_PIERCE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_BARB] = 18;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillProfSlash : public SkillObject
{    public:
        SkillProfSlash() : SkillObject(SKILL_PROF_SLASH)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_BARB] = 26;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillProfCrush : public SkillObject
{    public:
        SkillProfCrush() : SkillObject(SKILL_PROF_CRUSH)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_BARB] = 34;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillProfBlast : public SkillObject
{    public:
        SkillProfBlast() : SkillObject(SKILL_PROF_BLAST)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_BARB] = 42;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellDecoy : public SkillObject
{    public:
        SpellDecoy() : SkillObject(SPELL_DECOY)
        {
            _maxMana = 75;
            _minMana = 30;
            _manaChange = 7;
            _violent = false;
            _targets = TAR_IGNORE;
            _flags = MAG_MERCENARY|MAG_MANUAL;
            _minPos = POS_STANDING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillGarotte : public SkillObject
{    public:
        SkillGarotte() : SkillObject(SKILL_GAROTTE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = true;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MERCENARY] = 21;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillDemolitions : public SkillObject
{    public:
        SkillDemolitions() : SkillObject(SKILL_DEMOLITIONS)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MERCENARY] = 19;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillReconfigure : public SkillObject
{    public:
        SkillReconfigure() : SkillObject(SKILL_RECONFIGURE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_CYBORG] = 11;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillReboot : public SkillObject
{    public:
        SkillReboot() : SkillObject(SKILL_REBOOT)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_CYBORG] = 1;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillMotionSensor : public SkillObject
{    public:
        SkillMotionSensor() : SkillObject(SKILL_MOTION_SENSOR)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = CYB_ACTIVATE;
            _minPos = 0;

            _level[CLASS_CYBORG] = 16;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillStasis : public SkillObject
{    public:
        SkillStasis() : SkillObject(SKILL_STASIS)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = CYB_ACTIVATE;
            _minPos = 0;

            _level[CLASS_CYBORG] = 10;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillEnergyField : public SkillObject
{    public:
        SkillEnergyField() : SkillObject(SKILL_ENERGY_FIELD)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = CYB_ACTIVATE;
            _minPos = 0;

            _level[CLASS_CYBORG] = 32;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillReflexBoost : public SkillObject
{    public:
        SkillReflexBoost() : SkillObject(SKILL_REFLEX_BOOST)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = CYB_ACTIVATE;
            _minPos = 0;

            _level[CLASS_CYBORG] = 18;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillImplantW : public SkillObject
{    public:
        SkillImplantW() : SkillObject(SKILL_IMPLANT_W)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_CYBORG] = 26;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillOffensivePos : public SkillObject
{    public:
        SkillOffensivePos() : SkillObject(SKILL_OFFENSIVE_POS)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = CYB_ACTIVATE;
            _minPos = 0;

            _level[CLASS_CYBORG] = 15;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillDefensivePos : public SkillObject
{    public:
        SkillDefensivePos() : SkillObject(SKILL_DEFENSIVE_POS)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = CYB_ACTIVATE;
            _minPos = 0;

            _level[CLASS_CYBORG] = 15;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillMeleeCombatTac : public SkillObject
{    public:
        SkillMeleeCombatTac() : SkillObject(SKILL_MELEE_COMBAT_TAC)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = CYB_ACTIVATE;
            _minPos = 0;

            _level[CLASS_CYBORG] = 24;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillNeuralBridging : public SkillObject
{    public:
        SkillNeuralBridging() : SkillObject(SKILL_NEURAL_BRIDGING)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = CYB_ACTIVATE;
            _minPos = 0;

            _level[CLASS_CYBORG] = 35;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillNaniteReconstruction : public SkillObject
{    public:
        SkillNaniteReconstruction() : SkillObject(SKILL_NANITE_RECONSTRUCTION)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = CYB_ACTIVATE;
            _minPos = 0;

            _level[CLASS_CYBORG] = 33;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillArterialFlow : public SkillObject
{    public:
        SkillArterialFlow() : SkillObject(SKILL_ARTERIAL_FLOW)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = CYB_ACTIVATE;
            _minPos = 0;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillOptimmunalResp : public SkillObject
{    public:
        SkillOptimmunalResp() : SkillObject(SKILL_OPTIMMUNAL_RESP)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = CYB_ACTIVATE;
            _minPos = 0;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillAdrenalMaximizer : public SkillObject
{    public:
        SkillAdrenalMaximizer() : SkillObject(SKILL_ADRENAL_MAXIMIZER)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = CYB_ACTIVATE;
            _minPos = 0;

            _level[CLASS_CYBORG] = 37;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillPowerBoost : public SkillObject
{    public:
        SkillPowerBoost() : SkillObject(SKILL_POWER_BOOST)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = CYB_ACTIVATE;
            _minPos = 0;

            _level[CLASS_CYBORG] = 18;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillFastboot : public SkillObject
{    public:
        SkillFastboot() : SkillObject(SKILL_FASTBOOT)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_CYBORG] = 12;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillSelfDestruct : public SkillObject
{    public:
        SkillSelfDestruct() : SkillObject(SKILL_SELF_DESTRUCT)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_CYBORG] = 12;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillBioscan : public SkillObject
{    public:
        SkillBioscan() : SkillObject(SKILL_BIOSCAN)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_CYBORG] = 12;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillDischarge : public SkillObject
{    public:
        SkillDischarge() : SkillObject(SKILL_DISCHARGE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_CYBORG] = 14;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillSelfrepair : public SkillObject
{    public:
        SkillSelfrepair() : SkillObject(SKILL_SELFREPAIR)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_CYBORG] = 20;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillCyborepair : public SkillObject
{    public:
        SkillCyborepair() : SkillObject(SKILL_CYBOREPAIR)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_CYBORG] = 30;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillOverhaul : public SkillObject
{    public:
        SkillOverhaul() : SkillObject(SKILL_OVERHAUL)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_CYBORG] = 45;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillDamageControl : public SkillObject
{    public:
        SkillDamageControl() : SkillObject(SKILL_DAMAGE_CONTROL)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = CYB_ACTIVATE;
            _minPos = 0;

            _level[CLASS_CYBORG] = 12;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillElectronics : public SkillObject
{    public:
        SkillElectronics() : SkillObject(SKILL_ELECTRONICS)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_PHYSIC] = 13;
            _level[CLASS_CYBORG] = 6;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillHacking : public SkillObject
{    public:
        SkillHacking() : SkillObject(SKILL_HACKING)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_CYBORG] = 20;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillCyberscan : public SkillObject
{    public:
        SkillCyberscan() : SkillObject(SKILL_CYBERSCAN)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_CYBORG] = 30;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillCyboSurgery : public SkillObject
{    public:
        SkillCyboSurgery() : SkillObject(SKILL_CYBO_SURGERY)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_CYBORG] = 45;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillHyperscan : public SkillObject
{    public:
        SkillHyperscan() : SkillObject(SKILL_HYPERSCAN)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = CYB_ACTIVATE;
            _minPos = 0;

            _level[CLASS_CYBORG] = 25;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillOverdrain : public SkillObject
{    public:
        SkillOverdrain() : SkillObject(SKILL_OVERDRAIN)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = false;
            _targets = TAR_OBJ_INV|TAR_OBJ_EQUIP|TAR_OBJ_ROOM;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_CYBORG] = 8;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillEnergyWeapons : public SkillObject
{    public:
        SkillEnergyWeapons() : SkillObject(SKILL_ENERGY_WEAPONS)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_CYBORG] = 20;
            _level[CLASS_HOOD] = 30;
            _level[CLASS_MERCENARY] = 7;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillProjWeapons : public SkillObject
{    public:
        SkillProjWeapons() : SkillObject(SKILL_PROJ_WEAPONS)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_CYBORG] = 20;
            _level[CLASS_HOOD] = 10;
            _level[CLASS_MERCENARY] = 8;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillSpeedLoading : public SkillObject
{    public:
        SkillSpeedLoading() : SkillObject(SKILL_SPEED_LOADING)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_CYBORG] = 25;
            _level[CLASS_HOOD] = 30;
            _level[CLASS_MERCENARY] = 15;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillPalmStrike : public SkillObject
{    public:
        SkillPalmStrike() : SkillObject(SKILL_PALM_STRIKE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MONK] = 4;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillThroatStrike : public SkillObject
{    public:
        SkillThroatStrike() : SkillObject(SKILL_THROAT_STRIKE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MONK] = 10;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillWhirlwind : public SkillObject
{    public:
        SkillWhirlwind() : SkillObject(SKILL_WHIRLWIND)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MONK] = 40;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillHipToss : public SkillObject
{    public:
        SkillHipToss() : SkillObject(SKILL_HIP_TOSS)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MONK] = 27;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillCombo : public SkillObject
{    public:
        SkillCombo() : SkillObject(SKILL_COMBO)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MONK] = 33;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillDeathTouch : public SkillObject
{    public:
        SkillDeathTouch() : SkillObject(SKILL_DEATH_TOUCH)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MONK] = 49;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillCraneKick : public SkillObject
{    public:
        SkillCraneKick() : SkillObject(SKILL_CRANE_KICK)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MONK] = 15;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillRidgehand : public SkillObject
{    public:
        SkillRidgehand() : SkillObject(SKILL_RIDGEHAND)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MONK] = 30;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillScissorKick : public SkillObject
{    public:
        SkillScissorKick() : SkillObject(SKILL_SCISSOR_KICK)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MONK] = 20;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillPinchAlpha : public SkillObject
{    public:
        SkillPinchAlpha() : SkillObject(SKILL_PINCH_ALPHA)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = MAG_BIOLOGIC;
            _minPos = 0;

            _level[CLASS_MONK] = 6;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillPinchBeta : public SkillObject
{    public:
        SkillPinchBeta() : SkillObject(SKILL_PINCH_BETA)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = MAG_BIOLOGIC;
            _minPos = 0;

            _level[CLASS_MONK] = 12;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillPinchGamma : public SkillObject
{    public:
        SkillPinchGamma() : SkillObject(SKILL_PINCH_GAMMA)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = MAG_BIOLOGIC;
            _minPos = 0;

            _level[CLASS_MONK] = 35;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillPinchDelta : public SkillObject
{    public:
        SkillPinchDelta() : SkillObject(SKILL_PINCH_DELTA)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = MAG_BIOLOGIC;
            _minPos = 0;

            _level[CLASS_MONK] = 19;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillPinchEpsilon : public SkillObject
{    public:
        SkillPinchEpsilon() : SkillObject(SKILL_PINCH_EPSILON)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = MAG_BIOLOGIC;
            _minPos = 0;

            _level[CLASS_MONK] = 26;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillPinchOmega : public SkillObject
{    public:
        SkillPinchOmega() : SkillObject(SKILL_PINCH_OMEGA)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = MAG_BIOLOGIC;
            _minPos = 0;

            _level[CLASS_MONK] = 39;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillPinchZeta : public SkillObject
{    public:
        SkillPinchZeta() : SkillObject(SKILL_PINCH_ZETA)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = MAG_BIOLOGIC;
            _minPos = 0;

            _level[CLASS_MONK] = 32;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillMeditate : public SkillObject
{    public:
        SkillMeditate() : SkillObject(SKILL_MEDITATE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MONK] = 9;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillKata : public SkillObject
{    public:
        SkillKata() : SkillObject(SKILL_KATA)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_MONK] = 5;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillEvasion : public SkillObject
{    public:
        SkillEvasion() : SkillObject(SKILL_EVASION)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = MAG_BIOLOGIC;
            _minPos = 0;

            _level[CLASS_MONK] = 22;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillFlying : public SkillObject
{    public:
        SkillFlying() : SkillObject(SKILL_FLYING)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_VAMPIRE] = 33;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillSummon : public SkillObject
{    public:
        SkillSummon() : SkillObject(SKILL_SUMMON)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_VAMPIRE] = 9;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillFeed : public SkillObject
{    public:
        SkillFeed() : SkillObject(SKILL_FEED)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_VAMPIRE] = 1;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillBeguile : public SkillObject
{    public:
        SkillBeguile() : SkillObject(SKILL_BEGUILE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_VAMPIRE] = 26;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillDrain : public SkillObject
{    public:
        SkillDrain() : SkillObject(SKILL_DRAIN)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_VAMPIRE] = 36;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillCreateVampire : public SkillObject
{    public:
        SkillCreateVampire() : SkillObject(SKILL_CREATE_VAMPIRE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillControlUndead : public SkillObject
{    public:
        SkillControlUndead() : SkillObject(SKILL_CONTROL_UNDEAD)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_VAMPIRE] = 23;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillTerrorize : public SkillObject
{    public:
        SkillTerrorize() : SkillObject(SKILL_TERRORIZE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_VAMPIRE] = 29;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillLecture : public SkillObject
{    public:
        SkillLecture() : SkillObject(SKILL_LECTURE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_PHYSIC] = 30;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillEnergyConversion : public SkillObject
{    public:
        SkillEnergyConversion() : SkillObject(SKILL_ENERGY_CONVERSION)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;

            _level[CLASS_PHYSIC] = 22;
        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellHellFire : public SkillObject
{    public:
        SpellHellFire() : SkillObject(SPELL_HELL_FIRE)
        {
            _maxMana = 30;
            _minMana = 15;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_DAMAGE;
            _minPos = POS_FIGHTING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellHellFrost : public SkillObject
{    public:
        SpellHellFrost() : SkillObject(SPELL_HELL_FROST)
        {
            _maxMana = 30;
            _minMana = 15;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_DAMAGE|MAG_AFFECTS;
            _minPos = POS_FIGHTING;

        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
            SkillAffect *sa = new SkillAffect();
            struct affected_type *af;

            SkillObject::perform(ch, target, sa);

            memset(&af, 0x0, sizeof(struct affected_type));
            af->type = _skillNum;
            af->location = APPLY_STR;
            af->level = ch->getLevelBonus(_skillNum);
            af->modifier = -((ch->getLevelBonus(_skillNum) >> 4) + 1);
            af->duration = 4;

            sa->setAccumDuration(true);
            sa->addAffect(af);
            sa->addToVictMessage("You feel your strength wither as you vomit on the floor!");
            if (!number(0, 1))
                sa->addToRoomMessage("$n vomits all over the place!");
            else if (!number(0, 1))
                sa->addToRoomMessage("$n pukes all over $mself!");
            else if (!number(0, 1))
                sa->addToRoomMessage("$n blows chunks all over you!");
            else
                sa->addToRoomMessage("$n start vomiting uncontrollably!");

            ev.push_back(sa);
        }
};

class SpellHellFireStorm : public SkillObject
{    public:
        SpellHellFireStorm() : SkillObject(SPELL_HELL_FIRE_STORM)
        {
            _maxMana = 30;
            _minMana = 15;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_IGNORE;
            _flags = MAG_MASSES;
            _minPos = POS_FIGHTING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellHellFrostStorm : public SkillObject
{    public:
        SpellHellFrostStorm() : SkillObject(SPELL_HELL_FROST_STORM)
        {
            _maxMana = 30;
            _minMana = 15;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_IGNORE;
            _flags = MAG_MASSES;
            _minPos = POS_FIGHTING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellTrogStench : public SkillObject
{    public:
        SpellTrogStench() : SkillObject(SPELL_TROG_STENCH)
        {
            _maxMana = 30;
            _minMana = 15;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_AFFECTS;
            _minPos = POS_FIGHTING;

        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
            SkillAffect *sa = new SkillAffect();
            struct affected_type *af;

            SkillObject::perform(ch, target, sa);

            memset(&af, 0x0, sizeof(struct affected_type));
            af->type = _skillNum;
            af->location = APPLY_STR;
            af->modifier = -(number(1, 8));
            af->duration = 4;

            sa->setAccumDuration(true);
            sa->addAffect(af);
            sa->setChar(ch);
            sa->setVictim(target);

            ev.push_back(sa);
        }
};

class SpellFrostBreath : public SkillObject
{    public:
        SpellFrostBreath() : SkillObject(SPELL_FROST_BREATH)
        {
            _maxMana = 30;
            _minMana = 15;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_DAMAGE;
            _minPos = POS_FIGHTING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellFireBreath : public SkillObject
{    public:
        SpellFireBreath() : SkillObject(SPELL_FIRE_BREATH)
        {
            _maxMana = 30;
            _minMana = 15;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_DAMAGE|MAG_NOWATER;
            _minPos = POS_FIGHTING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellGasBreath : public SkillObject
{    public:
        SpellGasBreath() : SkillObject(SPELL_GAS_BREATH)
        {
            _maxMana = 30;
            _minMana = 15;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_DAMAGE|MAG_AFFECTS;
            _minPos = POS_FIGHTING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellAcidBreath : public SkillObject
{    public:
        SpellAcidBreath() : SkillObject(SPELL_ACID_BREATH)
        {
            _maxMana = 30;
            _minMana = 15;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_DAMAGE|MAG_AFFECTS;
            _minPos = POS_FIGHTING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellLightningBreath : public SkillObject
{    public:
        SpellLightningBreath() : SkillObject(SPELL_LIGHTNING_BREATH)
        {
            _maxMana = 30;
            _minMana = 15;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_DAMAGE|MAG_WATERZAP;
            _minPos = POS_FIGHTING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellManaRestore : public SkillObject
{    public:
        SpellManaRestore() : SkillObject(SPELL_MANA_RESTORE)
        {
            _maxMana = 30;
            _minMana = 15;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_POINTS;
            _minPos = POS_FIGHTING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellPetrify : public SkillObject
{    public:
        SpellPetrify() : SkillObject(SPELL_PETRIFY)
        {
            _maxMana = 330;
            _minMana = 215;
            _manaChange = 21;
            _violent = true;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_AFFECTS;
            _minPos = POS_FIGHTING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellSickness : public SkillObject
{    public:
        SpellSickness() : SkillObject(SPELL_SICKNESS)
        {
            _maxMana = 330;
            _minMana = 215;
            _manaChange = 21;
            _violent = true;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_AFFECTS;
            _minPos = POS_FIGHTING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellEssenceOfEvil : public SkillObject
{    public:
        SpellEssenceOfEvil() : SkillObject(SPELL_ESSENCE_OF_EVIL)
        {
            _maxMana = 330;
            _minMana = 215;
            _manaChange = 21;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_POINTS|MAG_EVIL|MAG_DAMAGE;
            _minPos = POS_SITTING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellEssenceOfGood : public SkillObject
{    public:
        SpellEssenceOfGood() : SkillObject(SPELL_ESSENCE_OF_GOOD)
        {
            _maxMana = 330;
            _minMana = 215;
            _manaChange = 21;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_POINTS|MAG_GOOD|MAG_DAMAGE;
            _minPos = POS_SITTING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellShadowBreath : public SkillObject
{    public:
        SpellShadowBreath() : SkillObject(SPELL_SHADOW_BREATH)
        {
            _maxMana = 30;
            _minMana = 15;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_MANUAL|MAG_DAMAGE;
            _minPos = POS_FIGHTING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellSteamBreath : public SkillObject
{    public:
        SpellSteamBreath() : SkillObject(SPELL_STEAM_BREATH)
        {
            _maxMana = 30;
            _minMana = 15;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_DAMAGE;
            _minPos = POS_FIGHTING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellQuadDamage : public SkillObject
{    public:
        SpellQuadDamage() : SkillObject(SPELL_QUAD_DAMAGE)
        {
            _maxMana = 30;
            _minMana = 15;
            _manaChange = 1;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_AFFECTS;
            _minPos = POS_FIGHTING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellZhengisFistOfAnnihilation : public SkillObject
{    public:
        SpellZhengisFistOfAnnihilation() : SkillObject(SPELL_ZHENGIS_FIST_OF_ANNIHILATION)
        {
            _maxMana = 30;
            _minMana = 15;
            _manaChange = 1;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_DAMAGE;
            _minPos = POS_FIGHTING;

        }
        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

//*****************************************************************************
//
// Remort spells and skills go here
//
//*****************************************************************************

class SpellFrostBreathing : public SkillObject
{    public:
        SpellFrostBreathing() : SkillObject(SPELL_FROST_BREATHING)
        {
            _maxMana = 280;
            _minMana = 180;
            _manaChange = 180;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_STANDING;
            _gen = 1;

            _level[CLASS_MAGE] = 35;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellFireBreathing : public SkillObject
{    public:
        SpellFireBreathing() : SkillObject(SPELL_FIRE_BREATHING)
        {
            _maxMana = 280;
            _minMana = 180;
            _manaChange = 180;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_STANDING;
            _gen = 1;

            _level[CLASS_MAGE] = 35;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellVestigialRune : public SkillObject
{    public:
        SpellVestigialRune() : SkillObject(SPELL_VESTIGIAL_RUNE)
        {
            _maxMana = 450;
            _minMana = 380;
            _manaChange = 380;
            _violent = false;
            _targets = TAR_OBJ_ROOM|TAR_OBJ_INV;
            _flags = MAG_MAGIC|MAG_MANUAL;
            _minPos = POS_STANDING;
            _gen = 1;

            _level[CLASS_MAGE] = 45;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellPrismaticSphere : public SkillObject
{    public:
        SpellPrismaticSphere() : SkillObject(SPELL_PRISMATIC_SPHERE)
        {
            _maxMana = 170;
            _minMana = 30;
            _manaChange = 30;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_STANDING;
            _gen = 1;

            _level[CLASS_MAGE] = 40;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellQuantumRift : public SkillObject
{    public:
        SpellQuantumRift() : SkillObject(SPELL_QUANTUM_RIFT)
        {
            _maxMana = 400;
            _minMana = 300;
            _manaChange = 300;
            _violent = false;
            _targets = TAR_IGNORE;
            _flags = MAG_PHYSICS|MAG_MANUAL;
            _minPos = POS_SITTING;
            _gen = 5;

            _level[CLASS_PHYSIC] = 45;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellStoneskin : public SkillObject
{    public:
        SpellStoneskin() : SkillObject(SPELL_STONESKIN)
        {
            _maxMana = 60;
            _minMana = 25;
            _manaChange = 25;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_FIGHTING;
            _gen = 4;

            _level[CLASS_RANGER] = 20;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
            SkillAffect *sa = new SkillAffect();
            struct affected_type *af, *af2;

            SkillObject::perform(ch, target, sa);

            memset(&af, 0x0, sizeof(struct affected_type));
            if (affected_by_spell(target, SPELL_BARKSKIN)) {
                affect_from_char(target, SPELL_BARKSKIN);
                if (*spell_wear_off_msg[SPELL_BARKSKIN]) {
                    send_to_char(target, spell_wear_off_msg[SPELL_BARKSKIN]);
                    send_to_char(target, "\r\n");
                }
            }
            af->level = af2->level = ch->getLevelBonus(_skillNum);
            af->location = APPLY_AC;
            af2->location = APPLY_DEX;
            af->duration = af2->duration = dice(4, ch->getLevelBonus(_skillNum) >> 3 + 1);
            af->modifier = -20;
            af2->modifier = -2;
            sa->addAffect(af);
            sa->addAffect(af2);
            sa->setAccumDuration(true);
            sa->addToVictMessage("Your skin hardens to a rock-like shell.");
            sa->addToRoomMessage("$n's skin turns a pale, rough grey.");

            ev.push_back(sa);
        }
};

class SpellShieldOfRighteousness : public SkillObject
{    public:
        SpellShieldOfRighteousness() : SkillObject(SPELL_SHIELD_OF_RIGHTEOUSNESS)
        {
            _maxMana = 50;
            _minMana = 20;
            _manaChange = 20;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_DIVINE|MAG_AFFECTS|MAG_GROUPS|MAG_GOOD;
            _minPos = POS_STANDING;
            _gen = 2;

            _level[CLASS_CLERIC] = 32;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellSunRay : public SkillObject
{    public:
        SpellSunRay() : SkillObject(SPELL_SUN_RAY)
        {
            _maxMana = 100;
            _minMana = 40;
            _manaChange = 40;
            _violent = true;
            _targets = TAR_IGNORE;
            _flags = MAG_DIVINE|MAG_MANUAL|MAG_GOOD;
            _minPos = POS_FIGHTING;
            _gen = 3;

            _level[CLASS_CLERIC] = 20;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellTaint : public SkillObject
{    public:
        SpellTaint() : SkillObject(SPELL_TAINT)
        {
            _maxMana = 250;
            _minMana = 180;
            _manaChange = 180;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_DIVINE|MAG_DAMAGE|MAG_AFFECTS|MAG_EVIL;
            _minPos = POS_FIGHTING;
            _gen = 9;

            _level[CLASS_KNIGHT] = 42;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellBlackmantle : public SkillObject
{    public:
        SpellBlackmantle() : SkillObject(SPELL_BLACKMANTLE)
        {
            _maxMana = 120;
            _minMana = 80;
            _manaChange = 80;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_DIVINE|MAG_AFFECTS|MAG_EVIL;
            _minPos = POS_FIGHTING;
            _gen = 1;

            _level[CLASS_CLERIC] = 42;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellSanctification : public SkillObject
{    public:
        SpellSanctification() : SkillObject(SPELL_SANCTIFICATION)
        {
            _maxMana = 100;
            _minMana = 60;
            _manaChange = 60;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_DIVINE|MAG_AFFECTS|MAG_GOOD;
            _minPos = POS_FIGHTING;
            _gen = 1;

            _level[CLASS_KNIGHT] = 33;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellStigmata : public SkillObject
{    public:
        SpellStigmata() : SkillObject(SPELL_STIGMATA)
        {
            _maxMana = 60;
            _minMana = 30;
            _manaChange = 30;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_DIVINE|MAG_AFFECTS|MAG_GOOD;
            _minPos = POS_FIGHTING;
            _gen = 1;

            _level[CLASS_CLERIC] = 23;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellDivinePower : public SkillObject
{    public:
        SpellDivinePower() : SkillObject(SPELL_DIVINE_POWER)
        {
            _maxMana = 150;
            _minMana = 80;
            _manaChange = 80;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_DIVINE|MAG_AFFECTS|MAG_GOOD;
            _minPos = POS_STANDING;
            _gen = 5;

            _level[CLASS_CLERIC] = 30;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellDeathKnell : public SkillObject
{    public:
        SpellDeathKnell() : SkillObject(SPELL_DEATH_KNELL)
        {
            _maxMana = 100;
            _minMana = 30;
            _manaChange = 30;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            _flags = MAG_DIVINE|MAG_DAMAGE|MAG_MANUAL|MAG_EVIL;
            _minPos = POS_FIGHTING;
            _gen = 1;

            _level[CLASS_CLERIC] = 25;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellManaShield : public SkillObject
{    public:
        SpellManaShield() : SkillObject(SPELL_MANA_SHIELD)
        {
            _maxMana = 60;
            _minMana = 30;
            _manaChange = 30;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_STANDING;
            _gen = 3;

            _level[CLASS_MAGE] = 29;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellEntangle : public SkillObject
{    public:
        SpellEntangle() : SkillObject(SPELL_ENTANGLE)
        {
            _maxMana = 70;
            _minMana = 30;
            _manaChange = 30;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_OUTDOORS|MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_FIGHTING;
            _gen = 1;

            _level[CLASS_RANGER] = 22;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellElementalBrand : public SkillObject
{    public:
        SpellElementalBrand() : SkillObject(SPELL_ELEMENTAL_BRAND)
        {
            _maxMana = 160;
            _minMana = 120;
            _manaChange = 120;
            _violent = false;
            _targets = TAR_OBJ_EQUIP|TAR_OBJ_ROOM|TAR_OBJ_INV;
            _flags = MAG_MAGIC|MAG_ALTER_OBJS;
            _minPos = POS_STANDING;
            _gen = 1;

            _level[CLASS_RANGER] = 31;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellSummonLegion : public SkillObject
{    public:
        SpellSummonLegion() : SkillObject(SPELL_SUMMON_LEGION)
        {
            _maxMana = 200;
            _minMana = 100;
            _manaChange = 100;
            _violent = false;
            _targets = TAR_IGNORE;
            _flags = MAG_DIVINE|MAG_MANUAL|MAG_EVIL;
            _minPos = POS_STANDING;
            _gen = 5;

            _level[CLASS_KNIGHT] = 27;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellAntiMagicShell : public SkillObject
{    public:
        SpellAntiMagicShell() : SkillObject(SPELL_ANTI_MAGIC_SHELL)
        {
            _maxMana = 150;
            _minMana = 70;
            _manaChange = 70;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_STANDING;
            _gen = 2;

            _level[CLASS_MAGE] = 21;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellWardingSigil : public SkillObject
{    public:
        SpellWardingSigil() : SkillObject(SPELL_WARDING_SIGIL)
        {
            _maxMana = 300;
            _minMana = 250;
            _manaChange = 250;
            _violent = false;
            _targets = TAR_OBJ_ROOM|TAR_OBJ_INV;
            _flags = MAG_MAGIC|MAG_ALTER_OBJS;
            _minPos = POS_STANDING;
            _gen = 1;

            _level[CLASS_MAGE] = 17;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellDivineIntervention : public SkillObject
{    public:
        SpellDivineIntervention() : SkillObject(SPELL_DIVINE_INTERVENTION)
        {
            _maxMana = 120;
            _minMana = 70;
            _manaChange = 70;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_DIVINE|MAG_AFFECTS|MAG_GOOD;
            _minPos = POS_STANDING;
            _gen = 4;

            _level[CLASS_CLERIC] = 21;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellSphereOfDesecration : public SkillObject
{    public:
        SpellSphereOfDesecration() : SkillObject(SPELL_SPHERE_OF_DESECRATION)
        {
            _maxMana = 120;
            _minMana = 70;
            _manaChange = 70;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_DIVINE|MAG_AFFECTS|MAG_EVIL;
            _minPos = POS_STANDING;
            _gen = 4;

            _level[CLASS_CLERIC] = 21;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellAmnesia : public SkillObject
{    public:
        SpellAmnesia() : SkillObject(SPELL_AMNESIA)
        {
            _maxMana = 70;
            _minMana = 30;
            _manaChange = 30;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT;
            _flags = MAG_PSIONIC|MAG_AFFECTS;
            _minPos = POS_STANDING;
            _gen = 1;

            _level[CLASS_PSIONIC] = 25;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class ZenTranslocation : public SkillObject
{    public:
        ZenTranslocation() : SkillObject(ZEN_TRANSLOCATION)
        {
            _maxMana = 40;
            _minMana = 20;
            _manaChange = 20;
            _violent = 0;
            _targets = 0;
            _flags = MAG_ZEN;
            _minPos = 0;
            _gen = 1;

            _level[CLASS_MONK] = 37;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class ZenCelerity : public SkillObject
{    public:
        ZenCelerity() : SkillObject(ZEN_CELERITY)
        {
            _maxMana = 40;
            _minMana = 20;
            _manaChange = 20;
            _violent = 0;
            _targets = 0;
            _flags = MAG_ZEN;
            _minPos = 0;
            _gen = 2;

            _level[CLASS_MONK] = 25;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillDisguise : public SkillObject
{    public:
        SkillDisguise() : SkillObject(SKILL_DISGUISE)
        {
            _maxMana = 40;
            _minMana = 20;
            _manaChange = 20;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;
            _gen = 1;

            _level[CLASS_THIEF] = 17;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillUncannyDodge : public SkillObject
{    public:
        SkillUncannyDodge() : SkillObject(SKILL_UNCANNY_DODGE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;
            _gen = 2;

            _level[CLASS_THIEF] = 22;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillDeEnergize : public SkillObject
{    public:
        SkillDeEnergize() : SkillObject(SKILL_DE_ENERGIZE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = true;
            _targets = 0;
            _flags = 0;
            _minPos = POS_FIGHTING;
            _gen = 1;

            _level[CLASS_CYBORG] = 22;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillAssimilate : public SkillObject
{    public:
        SkillAssimilate() : SkillObject(SKILL_ASSIMILATE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = true;
            _targets = 0;
            _flags = 0;
            _minPos = POS_FIGHTING;
            _gen = 3;

            _level[CLASS_CYBORG] = 22;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillRadionegation : public SkillObject
{    public:
        SkillRadionegation() : SkillObject(SKILL_RADIONEGATION)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = true;
            _targets = 0;
            _flags = CYB_ACTIVATE;
            _minPos = POS_FIGHTING;
            _gen = 1;

            _level[CLASS_CYBORG] = 17;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellMaleficViolation : public SkillObject
{    public:
        SpellMaleficViolation() : SkillObject(SPELL_MALEFIC_VIOLATION)
        {
            _maxMana = 100;
            _minMana = 50;
            _manaChange = 50;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_DIVINE|MAG_AFFECTS|MAG_EVIL;
            _minPos = POS_STANDING;
            _gen = 5;

            _level[CLASS_CLERIC] = 39;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellRighteousPenetration : public SkillObject
{    public:
        SpellRighteousPenetration() : SkillObject(SPELL_RIGHTEOUS_PENETRATION)
        {
            _maxMana = 100;
            _minMana = 50;
            _manaChange = 50;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_DIVINE|MAG_AFFECTS|MAG_GOOD;
            _minPos = POS_STANDING;
            _gen = 5;

            _level[CLASS_CLERIC] = 39;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillWormhole : public SkillObject
{    public:
        SkillWormhole() : SkillObject(SKILL_WORMHOLE)
        {
            _maxMana = 50;
            _minMana = 20;
            _manaChange = 20;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;
            _gen = 1;

            _level[CLASS_PHYSIC] = 30;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellGaussShield : public SkillObject
{    public:
        SpellGaussShield() : SkillObject(SPELL_GAUSS_SHIELD)
        {
            _maxMana = 90;
            _minMana = 70;
            _manaChange = 70;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_PHYSICS|MAG_AFFECTS;
            _minPos = POS_STANDING;
            _gen = 3;

            _level[CLASS_PHYSIC] = 32;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellUnholyStalker : public SkillObject
{    public:
        SpellUnholyStalker() : SkillObject(SPELL_UNHOLY_STALKER)
        {
            _maxMana = 100;
            _minMana = 50;
            _manaChange = 50;
            _violent = true;
            _targets = TAR_CHAR_WORLD;
            _flags = MAG_DIVINE|MAG_MANUAL|MAG_EVIL|MAG_NOSUN;
            _minPos = POS_STANDING;
            _gen = 3;

            _level[CLASS_CLERIC] = 25;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellAnimateDead : public SkillObject
{    public:
        SpellAnimateDead() : SkillObject(SPELL_ANIMATE_DEAD)
        {
            _maxMana = 60;
            _minMana = 30;
            _manaChange = 30;
            _violent = true;
            _targets = TAR_OBJ_ROOM;
            _flags = MAG_DIVINE|MAG_MANUAL|MAG_EVIL|MAG_NOSUN;
            _minPos = POS_STANDING;
            _gen = 2;

            _level[CLASS_CLERIC] = 24;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellInferno : public SkillObject
{    public:
        SpellInferno() : SkillObject(SPELL_INFERNO)
        {
            _maxMana = 100;
            _minMana = 50;
            _manaChange = 50;
            _violent = true;
            _targets = TAR_IGNORE;
            _flags = MAG_DIVINE|MAG_MANUAL|MAG_EVIL;
            _minPos = POS_FIGHTING;
            _gen = 1;

            _level[CLASS_CLERIC] = 35;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellVampiricRegeneration : public SkillObject
{    public:
        SpellVampiricRegeneration() : SkillObject(SPELL_VAMPIRIC_REGENERATION)
        {
            _maxMana = 100;
            _minMana = 50;
            _manaChange = 50;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_NOT_SELF;
            _flags = MAG_DIVINE|MAG_AFFECTS|MAG_EVIL;
            _minPos = POS_FIGHTING;
            _gen = 4;

            _level[CLASS_CLERIC] = 37;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellBanishment : public SkillObject
{    public:
        SpellBanishment() : SkillObject(SPELL_BANISHMENT)
        {
            _maxMana = 100;
            _minMana = 50;
            _manaChange = 50;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_NOT_SELF;
            _flags = MAG_DIVINE|MAG_MANUAL|MAG_EVIL;
            _minPos = POS_FIGHTING;
            _gen = 4;

            _level[CLASS_CLERIC] = 29;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillDisciplineOfSteel : public SkillObject
{    public:
        SkillDisciplineOfSteel() : SkillObject(SKILL_DISCIPLINE_OF_STEEL)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;
            _gen = 1;

            _level[CLASS_BARB] = 10;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillGreatCleave : public SkillObject
{    public:
        SkillGreatCleave() : SkillObject(SKILL_GREAT_CLEAVE)
        {
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _violent = 0;
            _targets = 0;
            _flags = 0;
            _minPos = 0;
            _gen = 2;

            _level[CLASS_BARB] = 40;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellLocustRegeneration : public SkillObject
{    public:
        SpellLocustRegeneration() : SkillObject(SPELL_LOCUST_REGENERATION)
        {
            _maxMana = 125;
            _minMana = 60;
            _manaChange = 60;
            _violent = true;
            _targets = TAR_CHAR_ROOM|TAR_FIGHT_VICT|TAR_NOT_SELF;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_FIGHTING;
            _gen = 5;

            _level[CLASS_MAGE] = 34;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SpellThornSkin : public SkillObject
{    public:
        SpellThornSkin() : SkillObject(SPELL_THORN_SKIN)
        {
            _maxMana = 110;
            _minMana = 80;
            _manaChange = 80;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_STANDING;
            _gen = 6;

            _level[CLASS_RANGER] = 38;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
            SkillAffect *sa = new SkillAffect();
            DamageObject *do1 = new DamageObject();

            struct affected_type *af = new affected_type;

            SkillObject::perform(ch, target, sa);

            af->type = _skillNum;
            af->location = APPLY_AC;
            af->level = ch->getLevelBonus(_skillNum);
            af->modifier = -(ch->getLevelBonus(_skillNum) / 10 + 5);
            af->duration = dice(3, (ch->getLevelBonus(_skillNum) >> 3) + 1);

            sa->setAccumDuration(true);
            sa->addAffect(af);
            sa->addToVictMessage("You feel someone protecting you.");

            do1->setVictim(ch);
            do1->setDamageType(SPELL_THORN_SKIN);
            int dam = 150 + ch->getLevelBonus(SKILL_CHARGE);
            do1->setDamage(dam);
            do1->setGenMessages(false);
            do1->addToVictMessage("You howl in pain as thorns erupt from your skin!");

            ev.push_back(sa);
            ev.push_back(do1);
        }
};

class SpellSpiritTrack : public SkillObject
{    public:
        SpellSpiritTrack() : SkillObject(SPELL_SPIRIT_TRACK)
        {
            _maxMana = 120;
            _minMana = 60;
            _manaChange = 60;
            _violent = false;
            _targets = TAR_CHAR_ROOM|TAR_SELF_ONLY;
            _flags = MAG_MAGIC|MAG_AFFECTS;
            _minPos = POS_FIGHTING;
            _gen = 1;

            _level[CLASS_RANGER] = 38;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillSnipe : public SkillObject
{    public:
        SkillSnipe() : SkillObject(SKILL_SNIPE)
        {
            _gen = 2;

            _level[CLASS_MERCENARY] = 37;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillAdvImplantW : public SkillObject
{    public:
        SkillAdvImplantW() : SkillObject(SKILL_ADV_IMPLANT_W)
        {
            _gen = 5;

            _level[CLASS_CYBORG] = 28;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
        }
};

class SkillCharge : public SkillObject
{    public:
        SkillCharge() : SkillObject(SKILL_CHARGE)
        {
            _level[CLASS_BARB] = 27;
        }

        void perform(Creature *ch, Creature *target, ExecutableVector &ev)
        {
            struct affected_type *af;
            SkillAffect *sa = new SkillAffect;
            DamageObject *do1 = new DamageObject();
            
            SkillObject::perform(ch, target, sa);

            if (CHECK_SKILL(ch, SKILL_CHARGE) < 50) {
                send_to_char(ch, "Do you really think you know what you're doing?\r\n");
                return;
            }

            if (target == ch) {
                send_to_char(ch, "You charge in and scare yourself silly!\r\n");
            }

            if (!target) {
                send_to_char(ch, "Charge who?\r\n");
                return;
            }

            af->is_instant = 1;
            af->type = SKILL_CHARGE;
            af->duration = 1;
            af->modifier = ch->getLevelBonus(SKILL_CHARGE) / 10;
            af->aff_index = 3;
            af->bitvector = AFF3_INST_AFF;
            af->owner = ch->getIdNum();

            do1->setVictim(ch);
            do1->setDamageType(SKILL_CHARGE);
            int dam = 150 + ch->getLevelBonus(SKILL_CHARGE);
            do1->setDamage(dam);
            do1->setGenMessages(false);
            do1->addToVictMessage("You howl in pain as thorns erupt from your skin!");

            // TODO: Add messages
            sa->addAffect(af);
            ev.push_back(sa);
        }
};


// Make sure you add your new spells and skills thusly
void
mag_assign_spells(void) {
    new SpellAirWalk();
    new SpellArmor();
    new SpellAstralSpell();
    new SpellControlUndead();
    new SpellTeleport();
    new SpellLocalTeleport();
    new SpellBlur();
    new SpellBless();
    new SpellDamn();
    new SpellCalm();
    new SpellBlindness();
    new SpellBreatheWater();
    new SpellBurningHands();
    new SpellCallLightning();
    new SpellEnvenom();
    new SpellCharm();
    new SpellCharmAnimal();
    new SpellChillTouch();
    new SpellClairvoyance();
    new SpellCallRodent();
    new SpellCallBird();
    new SpellCallReptile();
    new SpellCallBeast();
    new SpellCallPredator();
    new SpellClone();
    new SpellColorSpray();
    new SpellCommand();
    new SpellConeCold();
    new SpellConjureElemental();
    new SpellControlWeather();
    new SpellCreateFood();
    new SpellCreateWater();
    new SpellCureBlind();
    new SpellCureCritic();
    new SpellCureLight();
    new SpellCurse();
    new SpellDetectAlign();
    new SpellDetectInvis();
    new SpellDetectMagic();
    new SpellDetectPoison();
    new SpellDetectScrying();
    new SpellDimensionDoor();
    new SpellDispelEvil();
    new SpellDispelGood();
    new SpellDispelMagic();
    new SpellDisruption();
    new SpellDisplacement();
    new SpellEarthquake();
    new SpellEnchantArmor();
    new SpellEnchantWeapon();
    new SpellEndureCold();
    new SpellEnergyDrain();
    new SpellFly();
    new SpellFlameStrike();
    new SpellFlameOfFaith();
    new SpellGoodberry();
    new SpellGustOfWind();
    new SpellBarkskin();
    new SpellIcyBlast();
    new SpellInvisToUndead();
    new SpellAnimalKin();
    new SpellGreaterEnchant();
    new SpellGreaterInvis();
    new SpellGroupArmor();
    new SpellFireball();
    new SpellFireShield();
    new SpellGreaterHeal();
    new SpellGroupHeal();
    new SpellHarm();
    new SpellHeal();
    new SpellDivineIllumination();
    new SpellHaste();
    new SpellInfravision();
    new SpellInvisible();
    new SpellGlowlight();
    new SpellKnock();
    new SpellLightningBolt();
    new SpellLocateObject();
    new SpellMagicMissile();
    new SpellMinorIdentify();
    new SpellMagicalProt();
    new SpellMagicalVestment();
    new SpellMeteorStorm();
    new SpellChainLightning();
    new SpellHailstorm();
    new SpellIceStorm();
    new SpellPoison();
    new SpellPray();
    new SpellPrismaticSpray();
    new SpellProtectFromDevils();
    new SpellProtFromEvil();
    new SpellProtFromGood();
    new SpellProtFromLightning();
    new SpellProtFromFire();
    new SpellRemoveCurse();
    new SpellRemoveSickness();
    new SpellRejuvenate();
    new SpellRefresh();
    new SpellRegenerate();
    new SpellRetrieveCorpse();
    new SpellSanctuary();
    new SpellShockingGrasp();
    new SpellShroudObscurement();
    new SpellSleep();
    new SpellSlow();
    new SpellSpiritHammer();
    new SpellStrength();
    new SpellSummon();
    new SpellSword();
    new SpellSymbolOfPain();
    new SpellStoneToFlesh();
    new SpellTelekinesis();
    new SpellWordStun();
    new SpellTrueSeeing();
    new SpellWordOfRecall();
    new SpellWordOfIntellect();
    new SpellPeer();
    new SpellRemovePoison();
    new SpellRestoration();
    new SpellSenseLife();
    new SpellUndeadProt();
    new SpellWaterwalk();
    new SpellIdentify();
    new SpellWallOfThorns();
    new SpellWallOfStone();
    new SpellWallOfIce();
    new SpellWallOfFire();
    new SpellWallOfIron();
    new SpellThornTrap();
    new SpellFierySheet();
    new SpellPower();
    new SpellIntellect();
    new SpellConfusion();
    new SpellFear();
    new SpellSatiation();
    new SpellQuench();
    new SpellConfidence();
    new SpellNopain();
    new SpellDermalHardening();
    new SpellWoundClosure();
    new SpellAntibody();
    new SpellRetina();
    new SpellAdrenaline();
    new SpellBreathingStasis();
    new SpellVertigo();
    new SpellMetabolism();
    new SpellEgoWhip();
    new SpellPsychicCrush();
    new SpellRelaxation();
    new SpellWeakness();
    new SpellFortressOfWill();
    new SpellCellRegen();
    new SpellPsishield();
    new SpellPsychicSurge();
    new SpellPsychicConduit();
    new SpellPsionicShatter();
    new SpellIdInsinuation();
    new SpellMelatonicFlood();
    new SpellMotorSpasm();
    new SpellPsychicResistance();
    new SpellMassHysteria();
    new SpellGroupConfidence();
    new SpellClumsiness();
    new SpellEndurance();
    new SpellNullpsi();
    new SpellTelepathy();
    new SpellDistraction();
    new SkillPsiblast();
    new SkillPsilocate();
    new SkillPsidrain();
    new SpellElectrostaticField();
    new SpellEntropyField();
    new SpellAcidity();
    new SpellAttractionField();
    new SpellNuclearWasteland();
    new SpellSpacetimeImprint();
    new SpellSpacetimeRecall();
    new SpellFluoresce();
    new SpellGammaRay();
    new SpellGravityWell();
    new SpellCapacitanceBoost();
    new SpellElectricArc();
    new SpellTemporalCompression();
    new SpellTemporalDilation();
    new SpellHalflife();
    new SpellMicrowave();
    new SpellOxidize();
    new SpellRandomCoordinates();
    new SpellRepulsionField();
    new SpellVacuumShroud();
    new SpellAlbedoShield();
    new SpellTransmittance();
    new SpellTimeWarp();
    new SpellRadioimmunity();
    new SpellDensify();
    new SpellLatticeHardening();
    new SpellChemicalStability();
    new SpellRefraction();
    new SpellNullify();
    new SpellAreaStasis();
    new SpellEmpPulse();
    new SpellFissionBlast();
    new SkillAmbush();
    new ZenHealing();
    new ZenAwareness();
    new ZenOblivity();
    new ZenMotion();
    new SpellTidalSpacewarp();
    new SkillCleave();
    new SkillStrike();
    new SkillHamstring();
    new SkillDrag();
    new SkillSnatch();
    new SkillArchery();
    new SkillBowFletch();
    new SkillReadScrolls();
    new SkillUseWands();
    new SkillBackstab();
    new SkillCircle();
    new SkillHide();
    new SkillKick();
    new SkillBash();
    new SkillBreakDoor();
    new SkillHeadbutt();
    new SkillHotwire();
    new SkillGouge();
    new SkillStun();
    new SkillFeign();
    new SkillConceal();
    new SkillPlant();
    new SkillPickLock();
    new SkillRescue();
    new SkillSneak();
    new SkillSteal();
    new SkillTrack();
    new SkillPunch();
    new SkillPiledrive();
    new SkillShieldMastery();
    new SkillSleeper();
    new SkillElbow();
    new SkillKnee();
    new SkillAutopsy();
    new SkillBerserk();
    new SkillBattleCry();
    new SkillKia();
    new SkillCryFromBeyond();
    new SkillStomp();
    new SkillBodyslam();
    new SkillChoke();
    new SkillClothesline();
    new SkillTag();
    new SkillIntimidate();
    new SkillSpeedHealing();
    new SkillStalk();
    new SkillHearing();
    new SkillBearhug();
    new SkillBite();
    new SkillDblAttack();
    new SkillNightVision();
    new SkillTurn();
    new SkillAnalyze();
    new SkillEvaluate();
    new SkillCureDisease();
    new SkillHolyTouch();
    new SkillBandage();
    new SkillFirstaid();
    new SkillMedic();
    new SkillLeatherworking();
    new SkillMetalworking();
    new SkillConsider();
    new SkillGlance();
    new SkillShoot();
    new SkillBehead();
    new SkillEmpower();
    new SkillDisarm();
    new SkillSpinkick();
    new SkillRoundhouse();
    new SkillSidekick();
    new SkillSpinfist();
    new SkillJab();
    new SkillHook();
    new SkillSweepkick();
    new SkillTrip();
    new SkillUppercut();
    new SkillGroinkick();
    new SkillClaw();
    new SkillRabbitpunch();
    new SkillImpale();
    new SkillPeleKick();
    new SkillLungePunch();
    new SkillTornadoKick();
    new SkillTripleAttack();
    new SkillCounterAttack();
    new SkillSwimming();
    new SkillThrowing();
    new SkillRiding();
    new SkillAppraise();
    new SkillPipemaking();
    new SkillSecondWeapon();
    new SkillScanning();
    new SkillRetreat();
    new SkillGunsmithing();
    new SkillPistolwhip();
    new SkillCrossface();
    new SkillWrench();
    new SkillElusion();
    new SkillInfiltrate();
    new SkillShoulderThrow();
    new SkillProfPound();
    new SkillProfWhip();
    new SkillProfPierce();
    new SkillProfSlash();
    new SkillProfCrush();
    new SkillProfBlast();
    new SpellDecoy();
    new SkillGarotte();
    new SkillDemolitions();
    new SkillReconfigure();
    new SkillReboot();
    new SkillMotionSensor();
    new SkillStasis();
    new SkillEnergyField();
    new SkillReflexBoost();
    new SkillImplantW();
    new SkillOffensivePos();
    new SkillDefensivePos();
    new SkillMeleeCombatTac();
    new SkillNeuralBridging();
    new SkillNaniteReconstruction();
    new SkillArterialFlow();
    new SkillOptimmunalResp();
    new SkillAdrenalMaximizer();
    new SkillPowerBoost();
    new SkillFastboot();
    new SkillSelfDestruct();
    new SkillBioscan();
    new SkillDischarge();
    new SkillSelfrepair();
    new SkillCyborepair();
    new SkillOverhaul();
    new SkillDamageControl();
    new SkillElectronics();
    new SkillHacking();
    new SkillCyberscan();
    new SkillCyboSurgery();
    new SkillHyperscan();
    new SkillOverdrain();
    new SkillEnergyWeapons();
    new SkillProjWeapons();
    new SkillSpeedLoading();
    new SkillPalmStrike();
    new SkillThroatStrike();
    new SkillWhirlwind();
    new SkillHipToss();
    new SkillCombo();
    new SkillDeathTouch();
    new SkillCraneKick();
    new SkillRidgehand();
    new SkillScissorKick();
    new SkillPinchAlpha();
    new SkillPinchBeta();
    new SkillPinchGamma();
    new SkillPinchDelta();
    new SkillPinchEpsilon();
    new SkillPinchOmega();
    new SkillPinchZeta();
    new SkillMeditate();
    new SkillKata();
    new SkillEvasion();
    new SkillFlying();
    new SkillSummon();
    new SkillFeed();
    new SkillBeguile();
    new SkillDrain();
    new SkillCreateVampire();
    new SkillControlUndead();
    new SkillTerrorize();
    new SkillLecture();
    new SkillEnergyConversion();
    new SpellHellFire();
    new SpellHellFrost();
    new SpellHellFireStorm();
    new SpellHellFrostStorm();
    new SpellTrogStench();
    new SpellFrostBreath();
    new SpellFireBreath();
    new SpellGasBreath();
    new SpellAcidBreath();
    new SpellLightningBreath();
    new SpellManaRestore();
    new SpellPetrify();
    new SpellSickness();
    new SpellEssenceOfEvil();
    new SpellEssenceOfGood();
    new SpellShadowBreath();
    new SpellSteamBreath();
    new SpellQuadDamage();
    new SpellZhengisFistOfAnnihilation();
    new SpellFrostBreathing();
    new SpellFireBreathing();
    new SpellVestigialRune();
    new SpellPrismaticSphere();
    new SpellQuantumRift();
    new SpellStoneskin();
    new SpellShieldOfRighteousness();
    new SpellSunRay();
    new SpellTaint();
    new SpellBlackmantle();
    new SpellSanctification();
    new SpellStigmata();
    new SpellDivinePower();
    new SpellDeathKnell();
    new SpellManaShield();
    new SpellEntangle();
    new SpellElementalBrand();
    new SpellSummonLegion();
    new SpellAntiMagicShell();
    new SpellWardingSigil();
    new SpellDivineIntervention();
    new SpellSphereOfDesecration();
    new SpellAmnesia();
    new ZenTranslocation();
    new ZenCelerity();
    new SkillDisguise();
    new SkillUncannyDodge();
    new SkillDeEnergize();
    new SkillAssimilate();
    new SkillRadionegation();
    new SpellMaleficViolation();
    new SpellRighteousPenetration();
    new SkillWormhole();
    new SpellGaussShield();
    new SpellUnholyStalker();
    new SpellAnimateDead();
    new SpellInferno();
    new SpellVampiricRegeneration();
    new SpellBanishment();
    new SkillDisciplineOfSteel();
    new SkillGreatCleave();
    new SpellLocustRegeneration();
    new SpellThornSkin();
    new SpellSpiritTrack();
    new SkillSnipe();
    new SkillAdvImplantW();
}
