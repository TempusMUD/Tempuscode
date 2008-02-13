#include <string>
#include "skill_info.h"
#include "creature.h"
#include "char_class.h"
#include "utils.h"

using namespace std;

PracticeAbility PracticeAbility::CannotLearn(CANNOT_LEARN);
PracticeAbility PracticeAbility::CanLearn(CAN_LEARN);
PracticeAbility PracticeAbility::CanLearnLater(CAN_LEARN_LATER);
PracticeAbility PracticeAbility::WrongAlignment(WRONG_ALIGN);

SkillInfo::SkillInfo() 
{
    _minMana = 0;
    _maxMana = 0;
    _manaChange = 0;
    _minMove = 0;
    _maxMove = 0;
    _moveChange = 0;
    _minHit = 0;
    _maxHit = 0;
    _hitChange = 0;

    _skillNum = 0;
    _skillLevel = 0;

    _arcane = false;
    _divine = false;
    _physics = false;
    _psionic = false;
    _program = false;
    _zen = false;
    _song = false;
    _skill = false;
    _attackType = false;
    
    _minPosition = 0;
    
    _violent = false;

    _owner = NULL;
    _skillName = "";
    _className = "";
    _practiceInfo.clear();
}
string
SkillInfo::getTypeDescription() const
{
    if (this->isDivine() || this->isArcane())
        return "spell";
    if (this->isPhysics())
        return "alteration";
    if (this->isPsionic())
        return "trigger";
    if (this->isSong())
        return "song";
    if (this->isZen())
        return "zen art";
    if (this->isProgram())
        return "program";
    if (this->isAttackType())
        return "physical attack";
    
    return "skill";
}

//This is how we make new objects of the appropriate type without
//knowing the right type in advance
SkillInfo *
SkillInfo::constructSkill(int skillNum) {
    if (skillNum <= 0)
        return NULL;
    if (!skillExists(skillNum)) {
        return NULL;
    }
    return (*getSkillMap())[skillNum]->createNewInstance();
}

//this should never be called from this function.
SkillInfo* 
SkillInfo::createNewInstance() {
    return NULL;
}

PracticeAbility
SkillInfo::canBeLearned(Creature *ch) const {
    if (ch->isImmortal())
        return PracticeAbility::CanLearn;
    
    PracticeAbility p = PracticeAbility::CannotLearn;

    if ((this->isGood() && !ch->isGood()) ||
        (this->isEvil() && !ch->isEvil())) {
        p = PracticeAbility::WrongAlignment;
        return p; 
    }
    for (unsigned int i = 0; i < _practiceInfo.size(); i++) {
        if (ch->getPrimaryClass() == _practiceInfo[i].allowedClass) {
            if (ch->getLevel() >= _practiceInfo[i].minLevel 
                && ch->getGen() >= _practiceInfo[i].minGen) {
                return PracticeAbility::CanLearn;
            } 
            else {
                p = PracticeAbility::CanLearnLater;
            }
        } 
        else if (ch->getSecondaryClass() == _practiceInfo[i].allowedClass) {
            if (ch->getLevel() >= _practiceInfo[i].minLevel 
                && _practiceInfo[i].minGen == 0) {
                return PracticeAbility::CanLearn;
            } 
            else if (_practiceInfo[i].minGen == 0) {
                p = PracticeAbility::CanLearnLater;
            }
        } 
        else if (_practiceInfo[i].allowedClass == CLASS_NONE) {
            if (ch->getLevel() >= _practiceInfo[i].minLevel 
                && ch->getGen() >= _practiceInfo[i].minGen) {
                return PracticeAbility::CanLearn;
            } 
            else {
                p = PracticeAbility::CanLearnLater;
            }
        }
    }
    return p;
}

bool
SkillInfo::hasBeenLearned() {
    if (!getOwner()) {
        return false;
    } 
    else if (getSkillLevel() >= LEARNED(getOwner())) {
        return true;
    }
    return false;
}

bool 
SkillInfo::extractTrainingCost() {
    return true;
}

int
SkillInfo::getTrainingCost() {
    return 5000000;
}

void
SkillInfo::learn() {
    WAIT_STATE(getOwner(), 2 RL_SEC);

    int percent = getSkillLevel();
    percent += number(MINGAIN(getOwner()), MAXGAIN(getOwner()));

    if (percent > LEARNED(getOwner()))
        percent -= (percent - LEARNED(getOwner())) >> 1;

    setSkillLevel(percent);
}

bool
SkillInfo::operator==(const SkillInfo *skill) const {
    return this->getSkillNumber() == skill->getSkillNumber();
}

bool
SkillInfo::operator<(const SkillInfo &c) const {
    return _skillNum < c.getSkillNumber();
}

string
SkillInfo::getNameByNumber(int skillNum) {
    if (!skillExists(skillNum))
        return "!UNKNOWN!";

    return getSkillInfo(skillNum)->getName();
}

unsigned short
SkillInfo::getNumberByName(const char *name) {
    int ok;
    char *spellname, *checkname;
    char *first, *first2;
    SkillMap::iterator mi = _getSkillMap()->begin();

    for (; mi != _getSkillMap()->end(); ++mi) {
        if (mi->second->getName().empty())
            continue;
        
        if (is_abbrev(name, mi->second->getName().c_str()))
            return mi->first;

        ok = 1;
        spellname = tmp_strdup(mi->second->getName().c_str());
        checkname = tmp_strdup(name);
        first = tmp_getword(&spellname);
        first2 = tmp_getword(&checkname);
        while (*first && *first2 && ok) {
            if (!is_abbrev(first2, first))
                ok = 0;
            first = tmp_getword(&spellname);
            first2 = tmp_getword(&checkname);
        }

        if (ok && !*first2)
            return mi->first;
    }

    return 0;
}

void
SkillInfo::saveToXML(FILE *out) {
    fprintf(out, "<skill num=\"%d\" level=\"%d\"/>\n",
            getSkillNumber(), getSkillLevel());
}

SkillInfo *
SkillInfo::loadFromXML(xmlNode *node) {
    int skillNum = xmlGetIntProp(node, "num");
    int skillLevel = xmlGetIntProp(node, "level");

    SkillInfo* skill = SkillInfo::constructSkill(skillNum);
    if (skill) {
        skill->setSkillLevel(skillLevel);
        return skill;

    }

    return NULL;
}

//*********************************************************
//
//  New info subclasses go here
//
//*********************************************************

class SpellAirWalkInfo : public SkillInfo
{
    public:
        SpellAirWalkInfo() : SkillInfo()
        {
            _skillNum = SPELL_AIR_WALK;
            _skillName = "air walk";
            _className = "SpellAirWalk";
            _divine = true;
            _minMana = 40;
            _maxMana = 60;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_CLERIC, 32, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellAirWalkInfo();
        }
};

class SpellArmorInfo : public SkillInfo
{
    public:
        SpellArmorInfo() : SkillInfo()
        {
            _skillNum = SPELL_ARMOR;
            _skillName = "armor";
            _className = "SpellArmor";
            _arcane = true;
            _divine = true;
            _minMana = 15;
            _maxMana = 45;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_MAGE, 4, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 1, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_KNIGHT, 5, 0};
            _practiceInfo.push_back(p2);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellArmorInfo();
        }
};

class SpellAstralSpellInfo : public SkillInfo
{
    public:
        SpellAstralSpellInfo() : SkillInfo()
        {
            _skillNum = SPELL_ASTRAL_SPELL;
            _skillName = "astral spell";
            _className = "SpellAstralSpell";
            _arcane = true;
            _divine = true;
            _psionic = true;
            _minMana = 105;
            _maxMana = 175;
            _manaChange = 8;
            PracticeInfo p0 = {CLASS_MAGE, 43, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 47, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_PSIONIC, 45, 0};
            _practiceInfo.push_back(p2);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellAstralSpellInfo();
        }
};

class SpellControlUndeadInfo : public SkillInfo
{
    public:
        SpellControlUndeadInfo() : SkillInfo()
        {
            _skillNum = SPELL_CONTROL_UNDEAD;
            _skillName = "control undead";
            _className = "SpellControlUndead";
            _arcane = true;
            _divine = true;
            _minMana = 20;
            _maxMana = 50;
            _manaChange = 1;
            _violent = true;
            PracticeInfo p0 = {CLASS_CLERIC, 26, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellControlUndeadInfo();
        }
};

class SpellTeleportInfo : public SkillInfo
{
    public:
        SpellTeleportInfo() : SkillInfo()
        {
            _skillNum = SPELL_TELEPORT;
            _skillName = "teleport";
            _className = "SpellTeleport";
            _arcane = true;
            _minMana = 50;
            _maxMana = 75;
            _manaChange = 3;
            PracticeInfo p0 = {CLASS_MAGE, 33, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellTeleportInfo();
        }
};

class SpellLocalTeleportInfo : public SkillInfo
{
    public:
        SpellLocalTeleportInfo() : SkillInfo()
        {
            _skillNum = SPELL_LOCAL_TELEPORT;
            _skillName = "local teleport";
            _className = "SpellLocalTeleport";
            _arcane = true;
            _minMana = 30;
            _maxMana = 45;
            _manaChange = 3;
            PracticeInfo p0 = {CLASS_MAGE, 18, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellLocalTeleportInfo();
        }
};

class SpellBlurInfo : public SkillInfo
{
    public:
        SpellBlurInfo() : SkillInfo()
        {
            _skillNum = SPELL_BLUR;
            _skillName = "blur";
            _className = "SpellBlur";
            _arcane = true;
            _minMana = 20;
            _maxMana = 45;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_MAGE, 13, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellBlurInfo();
        }
};

class SpellBlessInfo : public SkillInfo
{
    public:
        SpellBlessInfo() : SkillInfo()
        {
            _skillNum = SPELL_BLESS;
            _skillName = "bless";
            _className = "SpellBless";
            _divine = true;
            _minMana = 15;
            _maxMana = 35;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_CLERIC, 7, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_KNIGHT, 10, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellBlessInfo();
        }
};

class SpellDamnInfo : public SkillInfo
{
    public:
        SpellDamnInfo() : SkillInfo()
        {
            _skillNum = SPELL_DAMN;
            _skillName = "damn";
            _className = "SpellDamn";
            _divine = true;
            _minMana = 15;
            _maxMana = 35;
            _manaChange = 1;
            _violent = true;
            PracticeInfo p0 = {CLASS_CLERIC, 7, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_KNIGHT, 10, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellDamnInfo();
        }
};

class SpellCalmInfo : public SkillInfo
{
    public:
        SpellCalmInfo() : SkillInfo()
        {
            _skillNum = SPELL_CALM;
            _skillName = "calm";
            _className = "SpellCalm";
            _divine = true;
            _minMana = 15;
            _maxMana = 35;
            _manaChange = 1;
            _violent = false;
            PracticeInfo p0 = {CLASS_KNIGHT, 36, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellCalmInfo();
        }
};

class SpellBlindnessInfo : public SkillInfo
{
    public:
        SpellBlindnessInfo() : SkillInfo()
        {
            _skillNum = SPELL_BLINDNESS;
            _skillName = "blindness";
            _className = "SpellBlindness";
            _arcane = true;
            _minMana = 25;
            _maxMana = 35;
            _manaChange = 1;
            _violent = true;
            PracticeInfo p0 = {CLASS_MAGE, 16, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 13, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellBlindnessInfo();
        }
};

class SpellBreatheWaterInfo : public SkillInfo
{
    public:
        SpellBreatheWaterInfo() : SkillInfo()
        {
            _skillNum = SPELL_BREATHE_WATER;
            _skillName = "breathe water";
            _className = "SpellBreatheWater";
            _arcane = true;
            _minMana = 20;
            _maxMana = 45;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_MAGE, 40, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellBreatheWaterInfo();
        }
};

class SpellBurningHandsInfo : public SkillInfo
{
    public:
        SpellBurningHandsInfo() : SkillInfo()
        {
            _skillNum = SPELL_BURNING_HANDS;
            _skillName = "burning hands";
            _className = "SpellBurningHands";
            _arcane = true;
            _minMana = 10;
            _maxMana = 30;
            _manaChange = 3;
            _violent = true;
            PracticeInfo p0 = {CLASS_MAGE, 11, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellBurningHandsInfo();
        }
};

class SpellCallLightningInfo : public SkillInfo
{
    public:
        SpellCallLightningInfo() : SkillInfo()
        {
            _skillNum = SPELL_CALL_LIGHTNING;
            _skillName = "call lightning";
            _className = "SpellCallLightning";
            _divine = true;
            _minMana = 25;
            _maxMana = 40;
            _manaChange = 3;
            _violent = true;
            PracticeInfo p0 = {CLASS_CLERIC, 25, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_RANGER, 30, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellCallLightningInfo();
        }
};

class SpellEnvenomInfo : public SkillInfo
{
    public:
        SpellEnvenomInfo() : SkillInfo()
        {
            _skillNum = SPELL_ENVENOM;
            _skillName = "envenom";
            _className = "SpellEnvenom";
            _arcane = true;
            _minMana = 10;
            _maxMana = 25;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_RANGER, 5, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellEnvenomInfo();
        }
};

class SpellCharmInfo : public SkillInfo
{
    public:
        SpellCharmInfo() : SkillInfo()
        {
            _skillNum = SPELL_CHARM;
            _skillName = "charm";
            _className = "SpellCharm";
            _arcane = true;
            _minMana = 50;
            _maxMana = 75;
            _manaChange = 2;
            _violent = true;
            PracticeInfo p0 = {CLASS_MAGE, 22, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellCharmInfo();
        }
};

class SpellCharmAnimalInfo : public SkillInfo
{
    public:
        SpellCharmAnimalInfo() : SkillInfo()
        {
            _skillNum = SPELL_CHARM_ANIMAL;
            _skillName = "charm animal";
            _className = "SpellCharmAnimal";
            _divine = true;
            _minMana = 50;
            _maxMana = 75;
            _manaChange = 2;
            _violent = true;
            PracticeInfo p0 = {CLASS_RANGER, 23, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellCharmAnimalInfo();
        }
};

class SpellChillTouchInfo : public SkillInfo
{
    public:
        SpellChillTouchInfo() : SkillInfo()
        {
            _skillNum = SPELL_CHILL_TOUCH;
            _skillName = "chill touch";
            _className = "SpellChillTouch";
            _arcane = true;
            _minMana = 10;
            _maxMana = 30;
            _manaChange = 3;
            _violent = true;
            PracticeInfo p0 = {CLASS_MAGE, 5, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellChillTouchInfo();
        }
};

class SpellClairvoyanceInfo : public SkillInfo
{
    public:
        SpellClairvoyanceInfo() : SkillInfo()
        {
            _skillNum = SPELL_CLAIRVOYANCE;
            _skillName = "clairvoyance";
            _className = "SpellClairvoyance";
            _arcane = true;
            _minMana = 60;
            _maxMana = 90;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_MAGE, 35, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellClairvoyanceInfo();
        }
};

class SpellCallRodentInfo : public SkillInfo
{
    public:
        SpellCallRodentInfo() : SkillInfo()
        {
            _skillNum = SPELL_CALL_RODENT;
            _skillName = "call rodent";
            _className = "SpellCallRodent";
            _arcane = true;
            _minMana = 60;
            _maxMana = 90;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_RANGER, 15, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellCallRodentInfo();
        }
};

class SpellCallBirdInfo : public SkillInfo
{
    public:
        SpellCallBirdInfo() : SkillInfo()
        {
            _skillNum = SPELL_CALL_BIRD;
            _skillName = "call bird";
            _className = "SpellCallBird";
            _arcane = true;
            _minMana = 60;
            _maxMana = 90;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_RANGER, 22, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellCallBirdInfo();
        }
};

class SpellCallReptileInfo : public SkillInfo
{
    public:
        SpellCallReptileInfo() : SkillInfo()
        {
            _skillNum = SPELL_CALL_REPTILE;
            _skillName = "call reptile";
            _className = "SpellCallReptile";
            _arcane = true;
            _minMana = 60;
            _maxMana = 90;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_RANGER, 29, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellCallReptileInfo();
        }
};

class SpellCallBeastInfo : public SkillInfo
{
    public:
        SpellCallBeastInfo() : SkillInfo()
        {
            _skillNum = SPELL_CALL_BEAST;
            _skillName = "call beast";
            _className = "SpellCallBeast";
            _arcane = true;
            _minMana = 60;
            _maxMana = 90;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_RANGER, 35, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellCallBeastInfo();
        }
};

class SpellCallPredatorInfo : public SkillInfo
{
    public:
        SpellCallPredatorInfo() : SkillInfo()
        {
            _skillNum = SPELL_CALL_PREDATOR;
            _skillName = "call predator";
            _className = "SpellCallPredator";
            _arcane = true;
            _minMana = 60;
            _maxMana = 90;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_RANGER, 41, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellCallPredatorInfo();
        }
};

class SpellCloneInfo : public SkillInfo
{
    public:
        SpellCloneInfo() : SkillInfo()
        {
            _skillNum = SPELL_CLONE;
            _skillName = "clone";
            _className = "SpellClone";
            _arcane = true;
            _minMana = 65;
            _maxMana = 80;
            _manaChange = 5;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellCloneInfo();
        }
};

class SpellColorSprayInfo : public SkillInfo
{
    public:
        SpellColorSprayInfo() : SkillInfo()
        {
            _skillNum = SPELL_COLOR_SPRAY;
            _skillName = "color spray";
            _className = "SpellColorSpray";
            _arcane = true;
            _minMana = 45;
            _maxMana = 90;
            _manaChange = 3;
            _violent = true;
            PracticeInfo p0 = {CLASS_MAGE, 18, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellColorSprayInfo();
        }
};

class SpellCommandInfo : public SkillInfo
{
    public:
        SpellCommandInfo() : SkillInfo()
        {
            _skillNum = SPELL_COMMAND;
            _skillName = "command";
            _className = "SpellCommand";
            _divine = true;
            _minMana = 35;
            _maxMana = 50;
            _manaChange = 5;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellCommandInfo();
        }
};

class SpellConeOfColdInfo : public SkillInfo
{
    public:
        SpellConeOfColdInfo() : SkillInfo()
        {
            _skillNum = SPELL_CONE_COLD;
            _skillName = "cone cold";
            _className = "SpellConeOfCold";
            _arcane = true;
            _divine = true;
            _minMana = 55;
            _maxMana = 140;
            _manaChange = 4;
            _violent = true;
            PracticeInfo p0 = {CLASS_MAGE, 36, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellConeOfColdInfo();
        }
};

class SpellConjureElementalInfo : public SkillInfo
{
    public:
        SpellConjureElementalInfo() : SkillInfo()
        {
            _skillNum = SPELL_CONJURE_ELEMENTAL;
            _skillName = "conjure elemental";
            _className = "SpellConjureElemental";
            _arcane = true;
            _minMana = 90;
            _maxMana = 175;
            _manaChange = 4;
            PracticeInfo p0 = {CLASS_MAGE, 20, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellConjureElementalInfo();
        }
};

class SpellControlWeatherInfo : public SkillInfo
{
    public:
        SpellControlWeatherInfo() : SkillInfo()
        {
            _skillNum = SPELL_CONTROL_WEATHER;
            _skillName = "control weather";
            _className = "SpellControlWeather";
            _arcane = true;
            _minMana = 25;
            _maxMana = 75;
            _manaChange = 5;
            PracticeInfo p0 = {CLASS_CLERIC, 22, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellControlWeatherInfo();
        }
};

class SpellCreateFoodInfo : public SkillInfo
{
    public:
        SpellCreateFoodInfo() : SkillInfo()
        {
            _skillNum = SPELL_CREATE_FOOD;
            _skillName = "create food";
            _className = "SpellCreateFood";
            _divine = true;
            _minMana = 5;
            _maxMana = 30;
            _manaChange = 3;
            PracticeInfo p0 = {CLASS_CLERIC, 5, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellCreateFoodInfo();
        }
};

class SpellCreateWaterInfo : public SkillInfo
{
    public:
        SpellCreateWaterInfo() : SkillInfo()
        {
            _skillNum = SPELL_CREATE_WATER;
            _skillName = "create water";
            _className = "SpellCreateWater";
            _divine = true;
            _minMana = 5;
            _maxMana = 30;
            _manaChange = 4;
            PracticeInfo p0 = {CLASS_CLERIC, 3, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellCreateWaterInfo();
        }
};

class SpellCureBlindInfo : public SkillInfo
{
    public:
        SpellCureBlindInfo() : SkillInfo()
        {
            _skillNum = SPELL_CURE_BLIND;
            _skillName = "cure blind";
            _className = "SpellCureBlind";
            _divine = true;
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_CLERIC, 6, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_KNIGHT, 26, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellCureBlindInfo();
        }
};

class SpellCureCriticInfo : public SkillInfo
{
    public:
        SpellCureCriticInfo() : SkillInfo()
        {
            _skillNum = SPELL_CURE_CRITIC;
            _skillName = "cure critic";
            _className = "SpellCureCritic";
            _divine = true;
            _minMana = 10;
            _maxMana = 40;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_CLERIC, 10, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_KNIGHT, 14, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellCureCriticInfo();
        }
};

class SpellCureLightInfo : public SkillInfo
{
    public:
        SpellCureLightInfo() : SkillInfo()
        {
            _skillNum = SPELL_CURE_LIGHT;
            _skillName = "cure light";
            _className = "SpellCureLight";
            _divine = true;
            _minMana = 5;
            _maxMana = 30;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_CLERIC, 2, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_KNIGHT, 6, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellCureLightInfo();
        }
};

class SpellCurseInfo : public SkillInfo
{
    public:
        SpellCurseInfo() : SkillInfo()
        {
            _skillNum = SPELL_CURSE;
            _skillName = "curse";
            _className = "SpellCurse";
            _arcane = true;
            _minMana = 50;
            _maxMana = 80;
            _manaChange = 2;
            _violent = true;
            PracticeInfo p0 = {CLASS_MAGE, 19, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellCurseInfo();
        }
};

class SpellDetectAlignInfo : public SkillInfo
{
    public:
        SpellDetectAlignInfo() : SkillInfo()
        {
            _skillNum = SPELL_DETECT_ALIGN;
            _skillName = "detect align";
            _className = "SpellDetectAlign";
            _divine = true;
            _minMana = 10;
            _maxMana = 20;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_MAGE, 24, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 1, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_KNIGHT, 1, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_RANGER, 13, 0};
            _practiceInfo.push_back(p3);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellDetectAlignInfo();
        }
};

class SpellDetectInvisInfo : public SkillInfo
{
    public:
        SpellDetectInvisInfo() : SkillInfo()
        {
            _skillNum = SPELL_DETECT_INVIS;
            _skillName = "detect invis";
            _className = "SpellDetectInvis";
            _arcane = true;
            _minMana = 10;
            _maxMana = 20;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_MAGE, 14, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellDetectInvisInfo();
        }
};

class SpellDetectMagicInfo : public SkillInfo
{
    public:
        SpellDetectMagicInfo() : SkillInfo()
        {
            _skillNum = SPELL_DETECT_MAGIC;
            _skillName = "detect magic";
            _className = "SpellDetectMagic";
            _arcane = true;
            _minMana = 10;
            _maxMana = 20;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_MAGE, 7, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 14, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_KNIGHT, 20, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_RANGER, 27, 0};
            _practiceInfo.push_back(p3);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellDetectMagicInfo();
        }
};

class SpellDetectPoisonInfo : public SkillInfo
{
    public:
        SpellDetectPoisonInfo() : SkillInfo()
        {
            _skillNum = SPELL_DETECT_POISON;
            _skillName = "detect poison";
            _className = "SpellDetectPoison";
            _divine = true;
            _minMana = 5;
            _maxMana = 15;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_MAGE, 20, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 4, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_KNIGHT, 12, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_RANGER, 6, 0};
            _practiceInfo.push_back(p3);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellDetectPoisonInfo();
        }
};

class SpellDetectScryingInfo : public SkillInfo
{
    public:
        SpellDetectScryingInfo() : SkillInfo()
        {
            _skillNum = SPELL_DETECT_SCRYING;
            _skillName = "detect scrying";
            _className = "SpellDetectScrying";
            _arcane = true;
            _minMana = 30;
            _maxMana = 50;
            _manaChange = 4;
            PracticeInfo p0 = {CLASS_MAGE, 44, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 44, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellDetectScryingInfo();
        }
};

class SpellDimensionDoorInfo : public SkillInfo
{
    public:
        SpellDimensionDoorInfo() : SkillInfo()
        {
            _skillNum = SPELL_DIMENSION_DOOR;
            _skillName = "dimension door";
            _className = "SpellDimensionDoor";
            _arcane = true;
            _minMana = 60;
            _maxMana = 90;
            _manaChange = 3;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellDimensionDoorInfo();
        }
};

class SpellDispelEvilInfo : public SkillInfo
{
    public:
        SpellDispelEvilInfo() : SkillInfo()
        {
            _skillNum = SPELL_DISPEL_EVIL;
            _skillName = "dispel evil";
            _className = "SpellDispelEvil";
            _divine = true;
            _minMana = 25;
            _maxMana = 40;
            _manaChange = 3;
            PracticeInfo p0 = {CLASS_CLERIC, 20, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellDispelEvilInfo();
        }
};

class SpellDispelGoodInfo : public SkillInfo
{
    public:
        SpellDispelGoodInfo() : SkillInfo()
        {
            _skillNum = SPELL_DISPEL_GOOD;
            _skillName = "dispel good";
            _className = "SpellDispelGood";
            _divine = true;
            _minMana = 25;
            _maxMana = 40;
            _manaChange = 3;
            PracticeInfo p0 = {CLASS_CLERIC, 20, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellDispelGoodInfo();
        }
};

class SpellDispelMagicInfo : public SkillInfo
{
    public:
        SpellDispelMagicInfo() : SkillInfo()
        {
            _skillNum = SPELL_DISPEL_MAGIC;
            _skillName = "dispel magic";
            _className = "SpellDispelMagic";
            _arcane = true;
            _minMana = 55;
            _maxMana = 90;
            _manaChange = 5;
            PracticeInfo p0 = {CLASS_MAGE, 39, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellDispelMagicInfo();
        }
};

class SpellDisruptionInfo : public SkillInfo
{
    public:
        SpellDisruptionInfo() : SkillInfo()
        {
            _skillNum = SPELL_DISRUPTION;
            _skillName = "disruption";
            _className = "SpellDisruption";
            _divine = true;
            _minMana = 75;
            _maxMana = 150;
            _manaChange = 7;
            _violent = true;
            PracticeInfo p0 = {CLASS_CLERIC, 38, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellDisruptionInfo();
        }
};

class SpellDisplacementInfo : public SkillInfo
{
    public:
        SpellDisplacementInfo() : SkillInfo()
        {
            _skillNum = SPELL_DISPLACEMENT;
            _skillName = "displacement";
            _className = "SpellDisplacement";
            _arcane = true;
            _minMana = 30;
            _maxMana = 55;
            _manaChange = 4;
            PracticeInfo p0 = {CLASS_MAGE, 46, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellDisplacementInfo();
        }
};

class SpellEarthquakeInfo : public SkillInfo
{
    public:
        SpellEarthquakeInfo() : SkillInfo()
        {
            _skillNum = SPELL_EARTHQUAKE;
            _skillName = "earthquake";
            _className = "SpellEarthquake";
            _divine = true;
            _minMana = 25;
            _maxMana = 40;
            _manaChange = 3;
            _violent = true;
            PracticeInfo p0 = {CLASS_CLERIC, 30, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellEarthquakeInfo();
        }
};

class SpellEnchantArmorInfo : public SkillInfo
{
    public:
        SpellEnchantArmorInfo() : SkillInfo()
        {
            _skillNum = SPELL_ENCHANT_ARMOR;
            _skillName = "enchant armor";
            _className = "SpellEnchantArmor";
            _arcane = true;
            _minMana = 80;
            _maxMana = 180;
            _manaChange = 3;
            PracticeInfo p0 = {CLASS_MAGE, 16, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellEnchantArmorInfo();
        }
};

class SpellEnchantWeaponInfo : public SkillInfo
{
    public:
        SpellEnchantWeaponInfo() : SkillInfo()
        {
            _skillNum = SPELL_ENCHANT_WEAPON;
            _skillName = "enchant weapon";
            _className = "SpellEnchantWeapon";
            _arcane = true;
            _minMana = 100;
            _maxMana = 200;
            _manaChange = 3;
            PracticeInfo p0 = {CLASS_MAGE, 21, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellEnchantWeaponInfo();
        }
};

class SpellEndureColdInfo : public SkillInfo
{
    public:
        SpellEndureColdInfo() : SkillInfo()
        {
            _skillNum = SPELL_ENDURE_COLD;
            _skillName = "endure cold";
            _className = "SpellEndureCold";
            _arcane = true;
            _divine = true;
            _minMana = 40;
            _maxMana = 100;
            _manaChange = 5;
            PracticeInfo p0 = {CLASS_CLERIC, 15, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_KNIGHT, 16, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 16, 0};
            _practiceInfo.push_back(p2);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellEndureColdInfo();
        }
};

class SpellEnergyDrainInfo : public SkillInfo
{
    public:
        SpellEnergyDrainInfo() : SkillInfo()
        {
            _skillNum = SPELL_ENERGY_DRAIN;
            _skillName = "energy drain";
            _className = "SpellEnergyDrain";
            _arcane = true;
            _minMana = 25;
            _maxMana = 40;
            _manaChange = 1;
            _violent = true;
            PracticeInfo p0 = {CLASS_MAGE, 24, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellEnergyDrainInfo();
        }
};

class SpellFlyInfo : public SkillInfo
{
    public:
        SpellFlyInfo() : SkillInfo()
        {
            _skillNum = SPELL_FLY;
            _skillName = "fly";
            _className = "SpellFly";
            _arcane = true;
            _minMana = 40;
            _maxMana = 60;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_MAGE, 33, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellFlyInfo();
        }
};

class SpellFlameStrikeInfo : public SkillInfo
{
    public:
        SpellFlameStrikeInfo() : SkillInfo()
        {
            _skillNum = SPELL_FLAME_STRIKE;
            _skillName = "flame strike";
            _className = "SpellFlameStrike";
            _divine = true;
            _minMana = 40;
            _maxMana = 60;
            _manaChange = 2;
            _violent = true;
            PracticeInfo p0 = {CLASS_CLERIC, 33, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_KNIGHT, 35, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellFlameStrikeInfo();
        }
};

class SpellFlameOfFaithInfo : public SkillInfo
{
    public:
        SpellFlameOfFaithInfo() : SkillInfo()
        {
            _skillNum = SPELL_FLAME_OF_FAITH;
            _skillName = "flame of faith";
            _className = "SpellFlameOfFaith";
            _divine = true;
            _minMana = 40;
            _maxMana = 60;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_CLERIC, 12, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellFlameOfFaithInfo();
        }
};

class SpellGoodberryInfo : public SkillInfo
{
    public:
        SpellGoodberryInfo() : SkillInfo()
        {
            _skillNum = SPELL_GOODBERRY;
            _skillName = "goodberry";
            _className = "SpellGoodberry";
            _arcane = true;
            _minMana = 5;
            _maxMana = 30;
            _manaChange = 4;
            PracticeInfo p0 = {CLASS_RANGER, 7, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellGoodberryInfo();
        }
};

class SpellGustOfWindInfo : public SkillInfo
{
    public:
        SpellGustOfWindInfo() : SkillInfo()
        {
            _skillNum = SPELL_GUST_OF_WIND;
            _skillName = "gust of wind";
            _className = "SpellGustOfWind";
            _arcane = true;
            _minMana = 55;
            _maxMana = 80;
            _manaChange = 4;
            PracticeInfo p0 = {CLASS_MAGE, 40, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellGustOfWindInfo();
        }
};

class SpellBarkskinInfo : public SkillInfo
{
    public:
        SpellBarkskinInfo() : SkillInfo()
        {
            _skillNum = SPELL_BARKSKIN;
            _skillName = "barkskin";
            _className = "SpellBarkskin";
            _arcane = true;
            _minMana = 25;
            _maxMana = 60;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_RANGER, 4, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellBarkskinInfo();
        }
};

class SpellIcyBlastInfo : public SkillInfo
{
    public:
        SpellIcyBlastInfo() : SkillInfo()
        {
            _skillNum = SPELL_ICY_BLAST;
            _skillName = "icy blast";
            _className = "SpellIcyBlast";
            _divine = true;
            _minMana = 50;
            _maxMana = 80;
            _manaChange = 3;
            _violent = true;
            PracticeInfo p0 = {CLASS_CLERIC, 41, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellIcyBlastInfo();
        }
};

class SpellInvisToUndeadInfo : public SkillInfo
{
    public:
        SpellInvisToUndeadInfo() : SkillInfo()
        {
            _skillNum = SPELL_INVIS_TO_UNDEAD;
            _skillName = "invis to undead";
            _className = "SpellInvisToUndead";
            _arcane = true;
            _minMana = 40;
            _maxMana = 60;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_MAGE, 30, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_RANGER, 21, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellInvisToUndeadInfo();
        }
};

class SpellAnimalKinInfo : public SkillInfo
{
    public:
        SpellAnimalKinInfo() : SkillInfo()
        {
            _skillNum = SPELL_ANIMAL_KIN;
            _skillName = "animal kin";
            _className = "SpellAnimalKin";
            _arcane = true;
            _minMana = 40;
            _maxMana = 60;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_RANGER, 17, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellAnimalKinInfo();
        }
};

class SpellGreaterEnchantInfo : public SkillInfo
{
    public:
        SpellGreaterEnchantInfo() : SkillInfo()
        {
            _skillNum = SPELL_GREATER_ENCHANT;
            _skillName = "greater enchant";
            _className = "SpellGreaterEnchant";
            _arcane = true;
            _minMana = 120;
            _maxMana = 230;
            _manaChange = 5;
            PracticeInfo p0 = {CLASS_MAGE, 34, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellGreaterEnchantInfo();
        }
};

class SpellGreaterInvisInfo : public SkillInfo
{
    public:
        SpellGreaterInvisInfo() : SkillInfo()
        {
            _skillNum = SPELL_GREATER_INVIS;
            _skillName = "greater invis";
            _className = "SpellGreaterInvis";
            _arcane = true;
            _minMana = 65;
            _maxMana = 85;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_MAGE, 38, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellGreaterInvisInfo();
        }
};

class SpellGroupArmorInfo : public SkillInfo
{
    public:
        SpellGroupArmorInfo() : SkillInfo()
        {
            _skillNum = SPELL_GROUP_ARMOR;
            _skillName = "group armor";
            _className = "SpellGroupArmor";
            _divine = true;
            _minMana = 30;
            _maxMana = 50;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_CLERIC, 19, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellGroupArmorInfo();
        }
};

class SpellFireballInfo : public SkillInfo
{
    public:
        SpellFireballInfo() : SkillInfo()
        {
            _skillNum = SPELL_FIREBALL;
            _skillName = "fireball";
            _className = "SpellFireball";
            _arcane = true;
            _minMana = 40;
            _maxMana = 130;
            _manaChange = 5;
            _violent = true;
            PracticeInfo p0 = {CLASS_MAGE, 31, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellFireballInfo();
        }
};

class SpellFireShieldInfo : public SkillInfo
{
    public:
        SpellFireShieldInfo() : SkillInfo()
        {
            _skillNum = SPELL_FIRE_SHIELD;
            _skillName = "fire shield";
            _className = "SpellFireShield";
            _arcane = true;
            _minMana = 30;
            _maxMana = 70;
            _manaChange = 3;
            PracticeInfo p0 = {CLASS_MAGE, 17, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellFireShieldInfo();
        }
};

class SpellGreaterHealInfo : public SkillInfo
{
    public:
        SpellGreaterHealInfo() : SkillInfo()
        {
            _skillNum = SPELL_GREATER_HEAL;
            _skillName = "greater heal";
            _className = "SpellGreaterHeal";
            _divine = true;
            _minMana = 90;
            _maxMana = 120;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_CLERIC, 34, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellGreaterHealInfo();
        }
};

class SpellGroupHealInfo : public SkillInfo
{
    public:
        SpellGroupHealInfo() : SkillInfo()
        {
            _skillNum = SPELL_GROUP_HEAL;
            _skillName = "group heal";
            _className = "SpellGroupHeal";
            _divine = true;
            _minMana = 60;
            _maxMana = 80;
            _manaChange = 5;
            PracticeInfo p0 = {CLASS_CLERIC, 31, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellGroupHealInfo();
        }
};

class SpellHarmInfo : public SkillInfo
{
    public:
        SpellHarmInfo() : SkillInfo()
        {
            _skillNum = SPELL_HARM;
            _skillName = "harm";
            _className = "SpellHarm";
            _divine = true;
            _minMana = 45;
            _maxMana = 95;
            _manaChange = 2;
            _violent = true;
            PracticeInfo p0 = {CLASS_CLERIC, 29, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellHarmInfo();
        }
};

class SpellHealInfo : public SkillInfo
{
    public:
        SpellHealInfo() : SkillInfo()
        {
            _skillNum = SPELL_HEAL;
            _skillName = "heal";
            _className = "SpellHeal";
            _divine = true;
            _minMana = 30;
            _maxMana = 60;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_CLERIC, 24, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_KNIGHT, 28, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellHealInfo();
        }
};

class SpellHasteInfo : public SkillInfo
{
    public:
        SpellHasteInfo() : SkillInfo()
        {
            _skillNum = SPELL_HASTE;
            _skillName = "haste";
            _className = "SpellHaste";
            _arcane = true;
            _minMana = 40;
            _maxMana = 60;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_MAGE, 44, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellHasteInfo();
        }
};

class SpellInfravisionInfo : public SkillInfo
{
    public:
        SpellInfravisionInfo() : SkillInfo()
        {
            _skillNum = SPELL_INFRAVISION;
            _skillName = "infravision";
            _className = "SpellInfravision";
            _arcane = true;
            _minMana = 10;
            _maxMana = 25;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_MAGE, 3, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellInfravisionInfo();
        }
};

class SpellInvisibleInfo : public SkillInfo
{
    public:
        SpellInvisibleInfo() : SkillInfo()
        {
            _skillNum = SPELL_INVISIBLE;
            _skillName = "invisible";
            _className = "SpellInvisible";
            _arcane = true;
            _minMana = 25;
            _maxMana = 35;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_MAGE, 12, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellInvisibleInfo();
        }
};

class SpellGlowlightInfo : public SkillInfo
{
    public:
        SpellGlowlightInfo() : SkillInfo()
        {
            _skillNum = SPELL_GLOWLIGHT;
            _skillName = "glowlight";
            _className = "SpellGlowlight";
            _arcane = true;
            _minMana = 20;
            _maxMana = 30;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_MAGE, 8, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellGlowlightInfo();
        }
};

class SpellKnockInfo : public SkillInfo
{
    public:
        SpellKnockInfo() : SkillInfo()
        {
            _skillNum = SPELL_KNOCK;
            _skillName = "knock";
            _className = "SpellKnock";
            _arcane = true;
            _minMana = 20;
            _maxMana = 50;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_MAGE, 27, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellKnockInfo();
        }
};

class SpellLightningBoltInfo : public SkillInfo
{
    public:
        SpellLightningBoltInfo() : SkillInfo()
        {
            _skillNum = SPELL_LIGHTNING_BOLT;
            _skillName = "lightning bolt";
            _className = "SpellLightningBolt";
            _arcane = true;
            _minMana = 15;
            _maxMana = 50;
            _manaChange = 1;
            _violent = true;
            PracticeInfo p0 = {CLASS_MAGE, 15, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellLightningBoltInfo();
        }
};

class SpellLocateObjectInfo : public SkillInfo
{
    public:
        SpellLocateObjectInfo() : SkillInfo()
        {
            _skillNum = SPELL_LOCATE_OBJECT;
            _skillName = "locate object";
            _className = "SpellLocateObject";
            _arcane = true;
            _minMana = 20;
            _maxMana = 25;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_MAGE, 23, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellLocateObjectInfo();
        }
};

class SpellMagicMissileInfo : public SkillInfo
{
    public:
        SpellMagicMissileInfo() : SkillInfo()
        {
            _skillNum = SPELL_MAGIC_MISSILE;
            _skillName = "magic missile";
            _className = "SpellMagicMissile";
            _arcane = true;
            _minMana = 8;
            _maxMana = 20;
            _manaChange = 3;
            _violent = true;
            PracticeInfo p0 = {CLASS_MAGE, 1, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellMagicMissileInfo();
        }
};

class SpellMinorIdentifyInfo : public SkillInfo
{
    public:
        SpellMinorIdentifyInfo() : SkillInfo()
        {
            _skillNum = SPELL_MINOR_IDENTIFY;
            _skillName = "minor identify";
            _className = "SpellMinorIdentify";
            _arcane = true;
            _minMana = 50;
            _maxMana = 75;
            _manaChange = 3;
            PracticeInfo p0 = {CLASS_MAGE, 15, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 46, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellMinorIdentifyInfo();
        }
};

class SpellMagicalProtInfo : public SkillInfo
{
    public:
        SpellMagicalProtInfo() : SkillInfo()
        {
            _skillNum = SPELL_MAGICAL_PROT;
            _skillName = "magical prot";
            _className = "SpellMagicalProt";
            _arcane = true;
            _minMana = 10;
            _maxMana = 40;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_MAGE, 32, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 26, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellMagicalProtInfo();
        }
};

class SpellMagicalVestmentInfo : public SkillInfo
{
    public:
        SpellMagicalVestmentInfo() : SkillInfo()
        {
            _skillNum = SPELL_MAGICAL_VESTMENT;
            _skillName = "magical vestment";
            _className = "SpellMagicalVestment";
            _divine = true;
            _minMana = 80;
            _maxMana = 100;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_CLERIC, 11, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_KNIGHT, 4, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellMagicalVestmentInfo();
        }
};

class SpellMeteorStormInfo : public SkillInfo
{
    public:
        SpellMeteorStormInfo() : SkillInfo()
        {
            _skillNum = SPELL_METEOR_STORM;
            _skillName = "meteor storm";
            _className = "SpellMeteorStorm";
            _arcane = true;
            _minMana = 110;
            _maxMana = 180;
            _manaChange = 25;
            _violent = true;
            PracticeInfo p0 = {CLASS_MAGE, 48, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellMeteorStormInfo();
        }
};

class SpellChainLightningInfo : public SkillInfo
{
    public:
        SpellChainLightningInfo() : SkillInfo()
        {
            _skillNum = SPELL_CHAIN_LIGHTNING;
            _skillName = "chain lightning";
            _className = "SpellChainLightning";
            _arcane = true;
            _minMana = 30;
            _maxMana = 120;
            _manaChange = 5;
            _violent = true;
            PracticeInfo p0 = {CLASS_MAGE, 23, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellChainLightningInfo();
        }
};

class SpellHailstormInfo : public SkillInfo
{
    public:
        SpellHailstormInfo() : SkillInfo()
        {
            _skillNum = SPELL_HAILSTORM;
            _skillName = "hailstorm";
            _className = "SpellHailstorm";
            _arcane = true;
            _minMana = 110;
            _maxMana = 180;
            _manaChange = 10;
            _violent = true;
            PracticeInfo p0 = {CLASS_RANGER, 37, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellHailstormInfo();
        }
};

class SpellIceStormInfo : public SkillInfo
{
    public:
        SpellIceStormInfo() : SkillInfo()
        {
            _skillNum = SPELL_ICE_STORM;
            _skillName = "ice storm";
            _className = "SpellIceStorm";
            _arcane = true;
            _minMana = 60;
            _maxMana = 130;
            _manaChange = 15;
            _violent = true;
            PracticeInfo p0 = {CLASS_CLERIC, 49, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellIceStormInfo();
        }
};

class SpellPoisonInfo : public SkillInfo
{
    public:
        SpellPoisonInfo() : SkillInfo()
        {
            _skillNum = SPELL_POISON;
            _skillName = "poison";
            _className = "SpellPoison";
            _arcane = true;
            _minMana = 20;
            _maxMana = 50;
            _manaChange = 3;
            _violent = true;
            PracticeInfo p0 = {CLASS_MAGE, 20, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellPoisonInfo();
        }
};

class SpellPrayInfo : public SkillInfo
{
    public:
        SpellPrayInfo() : SkillInfo()
        {
            _skillNum = SPELL_PRAY;
            _skillName = "pray";
            _className = "SpellPray";
            _divine = true;
            _minMana = 50;
            _maxMana = 80;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_CLERIC, 27, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_KNIGHT, 30, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellPrayInfo();
        }
};

class SpellPrismaticSprayInfo : public SkillInfo
{
    public:
        SpellPrismaticSprayInfo() : SkillInfo()
        {
            _skillNum = SPELL_PRISMATIC_SPRAY;
            _skillName = "prismatic spray";
            _className = "SpellPrismaticSpray";
            _arcane = true;
            _minMana = 80;
            _maxMana = 160;
            _manaChange = 10;
            _violent = true;
            PracticeInfo p0 = {CLASS_MAGE, 42, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellPrismaticSprayInfo();
        }
};

class SpellProtectFromDevilsInfo : public SkillInfo
{
    public:
        SpellProtectFromDevilsInfo() : SkillInfo()
        {
            _skillNum = SPELL_PROTECT_FROM_DEVILS;
            _skillName = "protect from devils";
            _className = "SpellProtectFromDevils";
            _divine = true;
            _minMana = 15;
            _maxMana = 40;
            _manaChange = 2;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellProtectFromDevilsInfo();
        }
};

class SpellProtFromEvilInfo : public SkillInfo
{
    public:
        SpellProtFromEvilInfo() : SkillInfo()
        {
            _skillNum = SPELL_PROT_FROM_EVIL;
            _skillName = "prot from evil";
            _className = "SpellProtFromEvil";
            _divine = true;
            _minMana = 15;
            _maxMana = 40;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_CLERIC, 16, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_KNIGHT, 9, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellProtFromEvilInfo();
        }
};

class SpellProtFromGoodInfo : public SkillInfo
{
    public:
        SpellProtFromGoodInfo() : SkillInfo()
        {
            _skillNum = SPELL_PROT_FROM_GOOD;
            _skillName = "prot from good";
            _className = "SpellProtFromGood";
            _divine = true;
            _minMana = 15;
            _maxMana = 40;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_CLERIC, 16, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_KNIGHT, 9, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellProtFromGoodInfo();
        }
};

class SpellProtFromLightningInfo : public SkillInfo
{
    public:
        SpellProtFromLightningInfo() : SkillInfo()
        {
            _skillNum = SPELL_PROT_FROM_LIGHTNING;
            _skillName = "prot from lightning";
            _className = "SpellProtFromLightning";
            _arcane = true;
            _minMana = 15;
            _maxMana = 40;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_RANGER, 9, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellProtFromLightningInfo();
        }
};

class SpellProtFromFireInfo : public SkillInfo
{
    public:
        SpellProtFromFireInfo() : SkillInfo()
        {
            _skillNum = SPELL_PROT_FROM_FIRE;
            _skillName = "prot from fire";
            _className = "SpellProtFromFire";
            _arcane = true;
            _minMana = 15;
            _maxMana = 40;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_CLERIC, 16, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_RANGER, 19, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellProtFromFireInfo();
        }
};

class SpellRemoveCurseInfo : public SkillInfo
{
    public:
        SpellRemoveCurseInfo() : SkillInfo()
        {
            _skillNum = SPELL_REMOVE_CURSE;
            _skillName = "remove curse";
            _className = "SpellRemoveCurse";
            _divine = true;
            _minMana = 25;
            _maxMana = 45;
            _manaChange = 5;
            PracticeInfo p0 = {CLASS_MAGE, 26, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 27, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_KNIGHT, 31, 0};
            _practiceInfo.push_back(p2);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellRemoveCurseInfo();
        }
};

class SpellRemoveSicknessInfo : public SkillInfo
{
    public:
        SpellRemoveSicknessInfo() : SkillInfo()
        {
            _skillNum = SPELL_REMOVE_SICKNESS;
            _skillName = "remove sickness";
            _className = "SpellRemoveSickness";
            _divine = true;
            _minMana = 85;
            _maxMana = 145;
            _manaChange = 5;
            PracticeInfo p0 = {CLASS_CLERIC, 37, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellRemoveSicknessInfo();
        }
};

class SpellRejuvenateInfo : public SkillInfo
{
    public:
        SpellRejuvenateInfo() : SkillInfo()
        {
            _skillNum = SPELL_REJUVENATE;
            _skillName = "rejuvenate";
            _className = "SpellRejuvenate";
            _divine = true;
            _minMana = 40;
            _maxMana = 100;
            _manaChange = 8;
            PracticeInfo p0 = {CLASS_CLERIC, 40, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellRejuvenateInfo();
        }
};

class SpellRefreshInfo : public SkillInfo
{
    public:
        SpellRefreshInfo() : SkillInfo()
        {
            _skillNum = SPELL_REFRESH;
            _skillName = "refresh";
            _className = "SpellRefresh";
            _arcane = true;
            _minMana = 40;
            _maxMana = 100;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_RANGER, 15, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellRefreshInfo();
        }
};

class SpellRegenerateInfo : public SkillInfo
{
    public:
        SpellRegenerateInfo() : SkillInfo()
        {
            _skillNum = SPELL_REGENERATE;
            _skillName = "regenerate";
            _className = "SpellRegenerate";
            _arcane = true;
            _minMana = 100;
            _maxMana = 140;
            _manaChange = 10;
            PracticeInfo p0 = {CLASS_MAGE, 49, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 45, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellRegenerateInfo();
        }
};

class SpellRetrieveCorpseInfo : public SkillInfo
{
    public:
        SpellRetrieveCorpseInfo() : SkillInfo()
        {
            _skillNum = SPELL_RETRIEVE_CORPSE;
            _skillName = "retrieve corpse";
            _className = "SpellRetrieveCorpse";
            _divine = true;
            _minMana = 65;
            _maxMana = 125;
            _manaChange = 15;
            PracticeInfo p0 = {CLASS_CLERIC, 48, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellRetrieveCorpseInfo();
        }
};

class SpellSanctuaryInfo : public SkillInfo
{
    public:
        SpellSanctuaryInfo() : SkillInfo()
        {
            _skillNum = SPELL_SANCTUARY;
            _skillName = "sanctuary";
            _className = "SpellSanctuary";
            _divine = true;
            _minMana = 85;
            _maxMana = 110;
            _manaChange = 5;
            PracticeInfo p0 = {CLASS_CLERIC, 28, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellSanctuaryInfo();
        }
};

class SpellShockingGraspInfo : public SkillInfo
{
    public:
        SpellShockingGraspInfo() : SkillInfo()
        {
            _skillNum = SPELL_SHOCKING_GRASP;
            _skillName = "shocking grasp";
            _className = "SpellShockingGrasp";
            _arcane = true;
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 3;
            _violent = true;
            PracticeInfo p0 = {CLASS_MAGE, 9, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellShockingGraspInfo();
        }
};

class SpellShroudObscurementInfo : public SkillInfo
{
    public:
        SpellShroudObscurementInfo() : SkillInfo()
        {
            _skillNum = SPELL_SHROUD_OBSCUREMENT;
            _skillName = "shroud obscurement";
            _className = "SpellShroudObscurement";
            _arcane = true;
            _minMana = 65;
            _maxMana = 145;
            _manaChange = 10;
            PracticeInfo p0 = {CLASS_MAGE, 36, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellShroudObscurementInfo();
        }
};

class SpellSleepInfo : public SkillInfo
{
    public:
        SpellSleepInfo() : SkillInfo()
        {
            _skillNum = SPELL_SLEEP;
            _skillName = "sleep";
            _className = "SpellSleep";
            _arcane = true;
            _minMana = 25;
            _maxMana = 40;
            _manaChange = 5;
            _violent = true;
            PracticeInfo p0 = {CLASS_MAGE, 10, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellSleepInfo();
        }
};

class SpellSlowInfo : public SkillInfo
{
    public:
        SpellSlowInfo() : SkillInfo()
        {
            _skillNum = SPELL_SLOW;
            _skillName = "slow";
            _className = "SpellSlow";
            _arcane = true;
            _minMana = 60;
            _maxMana = 120;
            _manaChange = 6;
            PracticeInfo p0 = {CLASS_MAGE, 38, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellSlowInfo();
        }
};

class SpellSpiritHammerInfo : public SkillInfo
{
    public:
        SpellSpiritHammerInfo() : SkillInfo()
        {
            _skillNum = SPELL_SPIRIT_HAMMER;
            _skillName = "spirit hammer";
            _className = "SpellSpiritHammer";
            _divine = true;
            _minMana = 10;
            _maxMana = 40;
            _manaChange = 1;
            _violent = true;
            PracticeInfo p0 = {CLASS_CLERIC, 10, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_KNIGHT, 15, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellSpiritHammerInfo();
        }
};

class SpellStrengthInfo : public SkillInfo
{
    public:
        SpellStrengthInfo() : SkillInfo()
        {
            _skillNum = SPELL_STRENGTH;
            _skillName = "strength";
            _className = "SpellStrength";
            _arcane = true;
            _minMana = 30;
            _maxMana = 65;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_MAGE, 6, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellStrengthInfo();
        }
};

class SpellSummonInfo : public SkillInfo
{
    public:
        SpellSummonInfo() : SkillInfo()
        {
            _skillNum = SPELL_SUMMON;
            _skillName = "summon";
            _className = "SpellSummon";
            _arcane = true;
            _minMana = 180;
            _maxMana = 320;
            _manaChange = 7;
            PracticeInfo p0 = {CLASS_MAGE, 29, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 17, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellSummonInfo();
        }
};

class SpellSwordInfo : public SkillInfo
{
    public:
        SpellSwordInfo() : SkillInfo()
        {
            _skillNum = SPELL_SWORD;
            _skillName = "sword";
            _className = "SpellSword";
            _arcane = true;
            _minMana = 180;
            _maxMana = 265;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_MAGE, 30, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellSwordInfo();
        }
};

class SpellSymbolOfPainInfo : public SkillInfo
{
    public:
        SpellSymbolOfPainInfo() : SkillInfo()
        {
            _skillNum = SPELL_SYMBOL_OF_PAIN;
            _skillName = "symbol of pain";
            _className = "SpellSymbolOfPain";
            _divine = true;
            _minMana = 80;
            _maxMana = 140;
            _manaChange = 5;
            _violent = true;
            PracticeInfo p0 = {CLASS_CLERIC, 38, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellSymbolOfPainInfo();
        }
};

class SpellStoneToFleshInfo : public SkillInfo
{
    public:
        SpellStoneToFleshInfo() : SkillInfo()
        {
            _skillNum = SPELL_STONE_TO_FLESH;
            _skillName = "stone to flesh";
            _className = "SpellStoneToFlesh";
            _divine = true;
            _minMana = 230;
            _maxMana = 380;
            _manaChange = 20;
            PracticeInfo p0 = {CLASS_CLERIC, 36, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_RANGER, 44, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellStoneToFleshInfo();
        }
};

class SpellWordStunInfo : public SkillInfo
{
    public:
        SpellWordStunInfo() : SkillInfo()
        {
            _skillNum = SPELL_WORD_STUN;
            _skillName = "word stun";
            _className = "SpellWordStun";
            _arcane = true;
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 5;
            _violent = true;
            PracticeInfo p0 = {CLASS_MAGE, 37, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellWordStunInfo();
        }
};

class SpellTrueSeeingInfo : public SkillInfo
{
    public:
        SpellTrueSeeingInfo() : SkillInfo()
        {
            _skillNum = SPELL_TRUE_SEEING;
            _skillName = "true seeing";
            _className = "SpellTrueSeeing";
            _arcane = true;
            _minMana = 65;
            _maxMana = 210;
            _manaChange = 15;
            PracticeInfo p0 = {CLASS_MAGE, 41, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 42, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellTrueSeeingInfo();
        }
};

class SpellWordOfRecallInfo : public SkillInfo
{
    public:
        SpellWordOfRecallInfo() : SkillInfo()
        {
            _skillNum = SPELL_WORD_OF_RECALL;
            _skillName = "word of recall";
            _className = "SpellWordOfRecall";
            _divine = true;
            _minMana = 20;
            _maxMana = 100;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_CLERIC, 15, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_KNIGHT, 25, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellWordOfRecallInfo();
        }
};

class SpellWordOfIntellectInfo : public SkillInfo
{
    public:
        SpellWordOfIntellectInfo() : SkillInfo()
        {
            _skillNum = SPELL_WORD_OF_INTELLECT;
            _skillName = "word of intellect";
            _className = "SpellWordOfIntellect";
            _divine = true;
            _minMana = 40;
            _maxMana = 100;
            _manaChange = 3;
            PracticeInfo p0 = {CLASS_CLERIC, 35, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellWordOfIntellectInfo();
        }
};

class SpellPeerInfo : public SkillInfo
{
    public:
        SpellPeerInfo() : SkillInfo()
        {
            _skillNum = SPELL_PEER;
            _skillName = "peer";
            _className = "SpellPeer";
            _arcane = true;
            _minMana = 40;
            _maxMana = 100;
            _manaChange = 6;
            PracticeInfo p0 = {CLASS_MAGE, 40, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellPeerInfo();
        }
};

class SpellRemovePoisonInfo : public SkillInfo
{
    public:
        SpellRemovePoisonInfo() : SkillInfo()
        {
            _skillNum = SPELL_REMOVE_POISON;
            _skillName = "remove poison";
            _className = "SpellRemovePoison";
            _divine = true;
            _minMana = 8;
            _maxMana = 40;
            _manaChange = 4;
            PracticeInfo p0 = {CLASS_CLERIC, 8, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_KNIGHT, 19, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 12, 0};
            _practiceInfo.push_back(p2);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellRemovePoisonInfo();
        }
};

class SpellRestorationInfo : public SkillInfo
{
    public:
        SpellRestorationInfo() : SkillInfo()
        {
            _skillNum = SPELL_RESTORATION;
            _skillName = "restoration";
            _className = "SpellRestoration";
            _divine = true;
            _minMana = 80;
            _maxMana = 210;
            _manaChange = 11;
            PracticeInfo p0 = {CLASS_CLERIC, 43, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellRestorationInfo();
        }
};

class SpellSenseLifeInfo : public SkillInfo
{
    public:
        SpellSenseLifeInfo() : SkillInfo()
        {
            _skillNum = SPELL_SENSE_LIFE;
            _skillName = "sense life";
            _className = "SpellSenseLife";
            _arcane = true;
            _minMana = 80;
            _maxMana = 120;
            _manaChange = 4;
            PracticeInfo p0 = {CLASS_MAGE, 27, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 23, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellSenseLifeInfo();
        }
};

class SpellUndeadProtInfo : public SkillInfo
{
    public:
        SpellUndeadProtInfo() : SkillInfo()
        {
            _skillNum = SPELL_UNDEAD_PROT;
            _skillName = "undead prot";
            _className = "SpellUndeadProt";
            _arcane = true;
            _minMana = 10;
            _maxMana = 60;
            _manaChange = 3;
            PracticeInfo p0 = {CLASS_MAGE, 41, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 18, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_KNIGHT, 18, 0};
            _practiceInfo.push_back(p2);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellUndeadProtInfo();
        }
};

class SpellWaterwalkInfo : public SkillInfo
{
    public:
        SpellWaterwalkInfo() : SkillInfo()
        {
            _skillNum = SPELL_WATERWALK;
            _skillName = "waterwalk";
            _className = "SpellWaterwalk";
            _arcane = true;
            _minMana = 40;
            _maxMana = 60;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_MAGE, 32, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellWaterwalkInfo();
        }
};

class SpellIdentifyInfo : public SkillInfo
{
    public:
        SpellIdentifyInfo() : SkillInfo()
        {
            _skillNum = SPELL_IDENTIFY;
            _skillName = "identify";
            _className = "SpellIdentify";
            _arcane = true;
            _minMana = 50;
            _maxMana = 95;
            _manaChange = 3;
            PracticeInfo p0 = {CLASS_MAGE, 25, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellIdentifyInfo();
        }
};

class SpellWallOfThornsInfo : public SkillInfo
{
    public:
        SpellWallOfThornsInfo() : SkillInfo()
        {
            _skillNum = SPELL_WALL_OF_THORNS;
            _skillName = "wall of thorns";
            _className = "SpellWallOfThorns";
            _arcane = true;
            _minMana = 20;
            _maxMana = 50;
            _manaChange = 1;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellWallOfThornsInfo();
        }
};

class SpellWallOfStoneInfo : public SkillInfo
{
    public:
        SpellWallOfStoneInfo() : SkillInfo()
        {
            _skillNum = SPELL_WALL_OF_STONE;
            _skillName = "wall of stone";
            _className = "SpellWallOfStone";
            _arcane = true;
            _minMana = 20;
            _maxMana = 50;
            _manaChange = 1;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellWallOfStoneInfo();
        }
};

class SpellWallOfIceInfo : public SkillInfo
{
    public:
        SpellWallOfIceInfo() : SkillInfo()
        {
            _skillNum = SPELL_WALL_OF_ICE;
            _skillName = "wall of ice";
            _className = "SpellWallOfIce";
            _arcane = true;
            _minMana = 20;
            _maxMana = 50;
            _manaChange = 1;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellWallOfIceInfo();
        }
};

class SpellWallOfFireInfo : public SkillInfo
{
    public:
        SpellWallOfFireInfo() : SkillInfo()
        {
            _skillNum = SPELL_WALL_OF_FIRE;
            _skillName = "wall of fire";
            _className = "SpellWallOfFire";
            _arcane = true;
            _minMana = 20;
            _maxMana = 50;
            _manaChange = 1;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellWallOfFireInfo();
        }
};

class SpellWallOfIronInfo : public SkillInfo
{
    public:
        SpellWallOfIronInfo() : SkillInfo()
        {
            _skillNum = SPELL_WALL_OF_IRON;
            _skillName = "wall of iron";
            _className = "SpellWallOfIron";
            _arcane = true;
            _minMana = 20;
            _maxMana = 50;
            _manaChange = 1;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellWallOfIronInfo();
        }
};

class SpellThornTrapInfo : public SkillInfo
{
    public:
        SpellThornTrapInfo() : SkillInfo()
        {
            _skillNum = SPELL_THORN_TRAP;
            _skillName = "thorn trap";
            _className = "SpellThornTrap";
            _arcane = true;
            _minMana = 20;
            _maxMana = 50;
            _manaChange = 1;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellThornTrapInfo();
        }
};

class SpellFierySheetInfo : public SkillInfo
{
    public:
        SpellFierySheetInfo() : SkillInfo()
        {
            _skillNum = SPELL_FIERY_SHEET;
            _skillName = "fiery sheet";
            _className = "SpellFierySheet";
            _arcane = true;
            _minMana = 20;
            _maxMana = 50;
            _manaChange = 1;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellFierySheetInfo();
        }
};

class SpellPowerInfo : public SkillInfo
{
    public:
        SpellPowerInfo() : SkillInfo()
        {
            _skillNum = SPELL_POWER;
            _skillName = "power";
            _className = "SpellPower";
            _psionic = true;
            _minMana = 50;
            _maxMana = 80;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_PSIONIC, 15, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellPowerInfo();
        }
};

class SpellIntellectInfo : public SkillInfo
{
    public:
        SpellIntellectInfo() : SkillInfo()
        {
            _skillNum = SPELL_INTELLECT;
            _skillName = "intellect";
            _className = "SpellIntellect";
            _psionic = true;
            _minMana = 40;
            _maxMana = 60;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_PSIONIC, 23, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellIntellectInfo();
        }
};

class SpellConfusionInfo : public SkillInfo
{
    public:
        SpellConfusionInfo() : SkillInfo()
        {
            _skillNum = SPELL_CONFUSION;
            _skillName = "confusion";
            _className = "SpellConfusion";
            _psionic = true;
            _minMana = 80;
            _maxMana = 110;
            _manaChange = 5;
            _violent = true;
            PracticeInfo p0 = {CLASS_PSIONIC, 36, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellConfusionInfo();
        }
};

class SpellFearInfo : public SkillInfo
{
    public:
        SpellFearInfo() : SkillInfo()
        {
            _skillNum = SPELL_FEAR;
            _skillName = "fear";
            _className = "SpellFear";
            _psionic = true;
            _minMana = 30;
            _maxMana = 60;
            _manaChange = 2;
            _violent = true;
            PracticeInfo p0 = {CLASS_PSIONIC, 29, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellFearInfo();
        }
};

class SpellSatiationInfo : public SkillInfo
{
    public:
        SpellSatiationInfo() : SkillInfo()
        {
            _skillNum = SPELL_SATIATION;
            _skillName = "satiation";
            _className = "SpellSatiation";
            _psionic = true;
            _minMana = 10;
            _maxMana = 30;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_PSIONIC, 3, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellSatiationInfo();
        }
};

class SpellQuenchInfo : public SkillInfo
{
    public:
        SpellQuenchInfo() : SkillInfo()
        {
            _skillNum = SPELL_QUENCH;
            _skillName = "quench";
            _className = "SpellQuench";
            _psionic = true;
            _minMana = 15;
            _maxMana = 35;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_PSIONIC, 4, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellQuenchInfo();
        }
};

class SpellConfidenceInfo : public SkillInfo
{
    public:
        SpellConfidenceInfo() : SkillInfo()
        {
            _skillNum = SPELL_CONFIDENCE;
            _skillName = "confidence";
            _className = "SpellConfidence";
            _psionic = true;
            _minMana = 40;
            _maxMana = 60;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_PSIONIC, 8, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellConfidenceInfo();
        }
};

class SpellNopainInfo : public SkillInfo
{
    public:
        SpellNopainInfo() : SkillInfo()
        {
            _skillNum = SPELL_NOPAIN;
            _skillName = "nopain";
            _className = "SpellNopain";
            _psionic = true;
            _minMana = 100;
            _maxMana = 130;
            _manaChange = 5;
            PracticeInfo p0 = {CLASS_PSIONIC, 30, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellNopainInfo();
        }
};

class SpellDermalHardeningInfo : public SkillInfo
{
    public:
        SpellDermalHardeningInfo() : SkillInfo()
        {
            _skillNum = SPELL_DERMAL_HARDENING;
            _skillName = "dermal hardening";
            _className = "SpellDermalHardening";
            _psionic = true;
            _minMana = 30;
            _maxMana = 70;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_PSIONIC, 12, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellDermalHardeningInfo();
        }
};

class SpellWoundClosureInfo : public SkillInfo
{
    public:
        SpellWoundClosureInfo() : SkillInfo()
        {
            _skillNum = SPELL_WOUND_CLOSURE;
            _skillName = "wound closure";
            _className = "SpellWoundClosure";
            _psionic = true;
            _minMana = 30;
            _maxMana = 80;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_PSIONIC, 10, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellWoundClosureInfo();
        }
};

class SpellAntibodyInfo : public SkillInfo
{
    public:
        SpellAntibodyInfo() : SkillInfo()
        {
            _skillNum = SPELL_ANTIBODY;
            _skillName = "antibody";
            _className = "SpellAntibody";
            _psionic = true;
            _minMana = 30;
            _maxMana = 80;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_PSIONIC, 19, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellAntibodyInfo();
        }
};

class SpellRetinaInfo : public SkillInfo
{
    public:
        SpellRetinaInfo() : SkillInfo()
        {
            _skillNum = SPELL_RETINA;
            _skillName = "retina";
            _className = "SpellRetina";
            _psionic = true;
            _minMana = 25;
            _maxMana = 60;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_PSIONIC, 13, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellRetinaInfo();
        }
};

class SpellAdrenalineInfo : public SkillInfo
{
    public:
        SpellAdrenalineInfo() : SkillInfo()
        {
            _skillNum = SPELL_ADRENALINE;
            _skillName = "adrenaline";
            _className = "SpellAdrenaline";
            _psionic = true;
            _minMana = 60;
            _maxMana = 80;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_PSIONIC, 27, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellAdrenalineInfo();
        }
};

class SpellBreathingStasisInfo : public SkillInfo
{
    public:
        SpellBreathingStasisInfo() : SkillInfo()
        {
            _skillNum = SPELL_BREATHING_STASIS;
            _skillName = "breathing stasis";
            _className = "SpellBreathingStasis";
            _psionic = true;
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_PSIONIC, 21, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellBreathingStasisInfo();
        }
};

class SpellVertigoInfo : public SkillInfo
{
    public:
        SpellVertigoInfo() : SkillInfo()
        {
            _skillNum = SPELL_VERTIGO;
            _skillName = "vertigo";
            _className = "SpellVertigo";
            _psionic = true;
            _minMana = 50;
            _maxMana = 70;
            _manaChange = 1;
            _violent = true;
            PracticeInfo p0 = {CLASS_PSIONIC, 31, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellVertigoInfo();
        }
};

class SpellMetabolismInfo : public SkillInfo
{
    public:
        SpellMetabolismInfo() : SkillInfo()
        {
            _skillNum = SPELL_METABOLISM;
            _skillName = "metabolism";
            _className = "SpellMetabolism";
            _psionic = true;
            _minMana = 30;
            _maxMana = 50;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_PSIONIC, 9, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellMetabolismInfo();
        }
};

class SpellEgoWhipInfo : public SkillInfo
{
    public:
        SpellEgoWhipInfo() : SkillInfo()
        {
            _skillNum = SPELL_EGO_WHIP;
            _skillName = "ego whip";
            _className = "SpellEgoWhip";
            _psionic = true;
            _minMana = 30;
            _maxMana = 50;
            _manaChange = 1;
            _violent = true;
            PracticeInfo p0 = {CLASS_PSIONIC, 22, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellEgoWhipInfo();
        }
};

class SpellPsychicCrushInfo : public SkillInfo
{
    public:
        SpellPsychicCrushInfo() : SkillInfo()
        {
            _skillNum = SPELL_PSYCHIC_CRUSH;
            _skillName = "psychic crush";
            _className = "SpellPsychicCrush";
            _psionic = true;
            _minMana = 30;
            _maxMana = 130;
            _manaChange = 15;
            _violent = true;
            PracticeInfo p0 = {CLASS_PSIONIC, 35, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellPsychicCrushInfo();
        }
};

class SpellRelaxationInfo : public SkillInfo
{
    public:
        SpellRelaxationInfo() : SkillInfo()
        {
            _skillNum = SPELL_RELAXATION;
            _skillName = "relaxation";
            _className = "SpellRelaxation";
            _psionic = true;
            _minMana = 40;
            _maxMana = 100;
            _manaChange = 3;
            PracticeInfo p0 = {CLASS_PSIONIC, 7, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellRelaxationInfo();
        }
};

class SpellWeaknessInfo : public SkillInfo
{
    public:
        SpellWeaknessInfo() : SkillInfo()
        {
            _skillNum = SPELL_WEAKNESS;
            _skillName = "weakness";
            _className = "SpellWeakness";
            _psionic = true;
            _minMana = 50;
            _maxMana = 70;
            _manaChange = 1;
            _violent = true;
            PracticeInfo p0 = {CLASS_PSIONIC, 16, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellWeaknessInfo();
        }
};

class SpellFortressOfWillInfo : public SkillInfo
{
    public:
        SpellFortressOfWillInfo() : SkillInfo()
        {
            _skillNum = SPELL_FORTRESS_OF_WILL;
            _skillName = "fortress of will";
            _className = "SpellFortressOfWill";
            _psionic = true;
            _minMana = 30;
            _maxMana = 30;
            _manaChange = 1;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellFortressOfWillInfo();
        }
};

class SpellCellRegenInfo : public SkillInfo
{
    public:
        SpellCellRegenInfo() : SkillInfo()
        {
            _skillNum = SPELL_CELL_REGEN;
            _skillName = "cell regen";
            _className = "SpellCellRegen";
            _psionic = true;
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_PSIONIC, 34, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellCellRegenInfo();
        }
};

class SpellPsishieldInfo : public SkillInfo
{
    public:
        SpellPsishieldInfo() : SkillInfo()
        {
            _skillNum = SPELL_PSISHIELD;
            _skillName = "psishield";
            _className = "SpellPsishield";
            _psionic = true;
            _minMana = 60;
            _maxMana = 120;
            _manaChange = 5;
            PracticeInfo p0 = {CLASS_PSIONIC, 30, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellPsishieldInfo();
        }
};

class SpellPsychicSurgeInfo : public SkillInfo
{
    public:
        SpellPsychicSurgeInfo() : SkillInfo()
        {
            _skillNum = SPELL_PSYCHIC_SURGE;
            _skillName = "psychic surge";
            _className = "SpellPsychicSurge";
            _psionic = true;
            _minMana = 60;
            _maxMana = 90;
            _manaChange = 4;
            _violent = true;
            PracticeInfo p0 = {CLASS_PSIONIC, 40, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellPsychicSurgeInfo();
        }
};

class SpellPsychicConduitInfo : public SkillInfo
{
    public:
        SpellPsychicConduitInfo() : SkillInfo()
        {
            _skillNum = SPELL_PSYCHIC_CONDUIT;
            _skillName = "psychic conduit";
            _className = "SpellPsychicConduit";
            _psionic = true;
            _minMana = 50;
            _maxMana = 70;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_PSIONIC, 26, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellPsychicConduitInfo();
        }
};

class SpellPsionicShatterInfo : public SkillInfo
{
    public:
        SpellPsionicShatterInfo() : SkillInfo()
        {
            _skillNum = SPELL_PSIONIC_SHATTER;
            _skillName = "psionic shatter";
            _className = "SpellPsionicShatter";
            _psionic = true;
            _minMana = 30;
            _maxMana = 80;
            _manaChange = 3;
            _violent = true;
            PracticeInfo p0 = {CLASS_PSIONIC, 33, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellPsionicShatterInfo();
        }
};

class SpellMelatonicFloodInfo : public SkillInfo
{
    public:
        SpellMelatonicFloodInfo() : SkillInfo()
        {
            _skillNum = SPELL_MELATONIC_FLOOD;
            _skillName = "melatonic flood";
            _className = "SpellMelatonicFlood";
            _psionic = true;
            _minMana = 30;
            _maxMana = 90;
            _manaChange = 5;
            _violent = true;
            PracticeInfo p0 = {CLASS_PSIONIC, 18, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellMelatonicFloodInfo();
        }
};

class SpellMotorSpasmInfo : public SkillInfo
{
    public:
        SpellMotorSpasmInfo() : SkillInfo()
        {
            _skillNum = SPELL_MOTOR_SPASM;
            _skillName = "motor spasm";
            _className = "SpellMotorSpasm";
            _psionic = true;
            _minMana = 70;
            _maxMana = 100;
            _manaChange = 5;
            PracticeInfo p0 = {CLASS_PSIONIC, 37, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellMotorSpasmInfo();
        }
};

class SpellPsychicResistanceInfo : public SkillInfo
{
    public:
        SpellPsychicResistanceInfo() : SkillInfo()
        {
            _skillNum = SPELL_PSYCHIC_RESISTANCE;
            _skillName = "psychic resistance";
            _className = "SpellPsychicResistance";
            _psionic = true;
            _minMana = 30;
            _maxMana = 80;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_PSIONIC, 20, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellPsychicResistanceInfo();
        }
};

class SpellMassHysteriaInfo : public SkillInfo
{
    public:
        SpellMassHysteriaInfo() : SkillInfo()
        {
            _skillNum = SPELL_MASS_HYSTERIA;
            _skillName = "mass hysteria";
            _className = "SpellMassHysteria";
            _psionic = true;
            _minMana = 30;
            _maxMana = 100;
            _manaChange = 1;
            _violent = true;
            PracticeInfo p0 = {CLASS_PSIONIC, 48, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellMassHysteriaInfo();
        }
};

class SpellGroupConfidenceInfo : public SkillInfo
{
    public:
        SpellGroupConfidenceInfo() : SkillInfo()
        {
            _skillNum = SPELL_GROUP_CONFIDENCE;
            _skillName = "group confidence";
            _className = "SpellGroupConfidence";
            _psionic = true;
            _minMana = 45;
            _maxMana = 90;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_PSIONIC, 14, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellGroupConfidenceInfo();
        }
};

class SpellClumsinessInfo : public SkillInfo
{
    public:
        SpellClumsinessInfo() : SkillInfo()
        {
            _skillNum = SPELL_CLUMSINESS;
            _skillName = "clumsiness";
            _className = "SpellClumsiness";
            _psionic = true;
            _minMana = 50;
            _maxMana = 70;
            _manaChange = 1;
            _violent = true;
            PracticeInfo p0 = {CLASS_PSIONIC, 28, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellClumsinessInfo();
        }
};

class SpellEnduranceInfo : public SkillInfo
{
    public:
        SpellEnduranceInfo() : SkillInfo()
        {
            _skillNum = SPELL_ENDURANCE;
            _skillName = "endurance";
            _className = "SpellEndurance";
            _psionic = true;
            _minMana = 50;
            _maxMana = 90;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_PSIONIC, 11, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellEnduranceInfo();
        }
};

class SpellNullpsiInfo : public SkillInfo
{
    public:
        SpellNullpsiInfo() : SkillInfo()
        {
            _skillNum = SPELL_NULLPSI;
            _skillName = "nullpsi";
            _className = "SpellNullpsi";
            _psionic = true;
            _minMana = 50;
            _maxMana = 90;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_PSIONIC, 17, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellNullpsiInfo();
        }
};

class SpellTelepathyInfo : public SkillInfo
{
    public:
        SpellTelepathyInfo() : SkillInfo()
        {
            _skillNum = SPELL_TELEPATHY;
            _skillName = "telepathy";
            _className = "SpellTelepathy";
            _psionic = true;
            _minMana = 62;
            _maxMana = 95;
            _manaChange = 4;
            _violent = false;
            PracticeInfo p0 = {CLASS_PSIONIC, 41, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellTelepathyInfo();
        }
};

class SpellDistractionInfo : public SkillInfo
{
    public:
        SpellDistractionInfo() : SkillInfo()
        {
            _skillNum = SPELL_DISTRACTION;
            _skillName = "distraction";
            _className = "SpellDistraction";
            _psionic = true;
            _minMana = 10;
            _maxMana = 40;
            _manaChange = 4;
            _violent = false;
            PracticeInfo p0 = {CLASS_PSIONIC, 2, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellDistractionInfo();
        }
};

class SkillPsiblastInfo : public SkillInfo
{
    public:
        SkillPsiblastInfo() : SkillInfo()
        {
            _skillNum = SKILL_PSIBLAST;
            _skillName = "psiblast";
            _className = "SkillPsiblast";
            _psionic = true;
            _minMana = 50;
            _maxMana = 50;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_PSIONIC, 5, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillPsiblastInfo();
        }
};

class SkillPsidrainInfo : public SkillInfo
{
    public:
        SkillPsidrainInfo() : SkillInfo()
        {
            _skillNum = SKILL_PSIDRAIN;
            _skillName = "psidrain";
            _className = "SkillPsidrain";
            _psionic = true;
            _minMana = 30;
            _maxMana = 50;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_PSIONIC, 24, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillPsidrainInfo();
        }
};

class SpellElectrostaticFieldInfo : public SkillInfo
{
    public:
        SpellElectrostaticFieldInfo() : SkillInfo()
        {
            _skillNum = SPELL_ELECTROSTATIC_FIELD;
            _skillName = "electrostatic field";
            _className = "SpellElectrostaticField";
            _physics = true;
            _minMana = 20;
            _maxMana = 40;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_PHYSIC, 1, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellElectrostaticFieldInfo();
        }
};

class SpellEntropyFieldInfo : public SkillInfo
{
    public:
        SpellEntropyFieldInfo() : SkillInfo()
        {
            _skillNum = SPELL_ENTROPY_FIELD;
            _skillName = "entropy field";
            _className = "SpellEntropyField";
            _physics = true;
            _minMana = 60;
            _maxMana = 101;
            _manaChange = 8;
            _violent = true;
            PracticeInfo p0 = {CLASS_PHYSIC, 27, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellEntropyFieldInfo();
        }
};

class SpellAcidityInfo : public SkillInfo
{
    public:
        SpellAcidityInfo() : SkillInfo()
        {
            _skillNum = SPELL_ACIDITY;
            _skillName = "acidity";
            _className = "SpellAcidity";
            _physics = true;
            _minMana = 20;
            _maxMana = 40;
            _manaChange = 2;
            _violent = true;
            PracticeInfo p0 = {CLASS_PHYSIC, 2, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellAcidityInfo();
        }
};

class SpellAttractionFieldInfo : public SkillInfo
{
    public:
        SpellAttractionFieldInfo() : SkillInfo()
        {
            _skillNum = SPELL_ATTRACTION_FIELD;
            _skillName = "attraction field";
            _className = "SpellAttractionField";
            _physics = true;
            _minMana = 40;
            _maxMana = 60;
            _manaChange = 1;
            _violent = true;
            PracticeInfo p0 = {CLASS_PHYSIC, 8, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellAttractionFieldInfo();
        }
};

class SpellNuclearWastelandInfo : public SkillInfo
{
    public:
        SpellNuclearWastelandInfo() : SkillInfo()
        {
            _skillNum = SPELL_NUCLEAR_WASTELAND;
            _skillName = "nuclear wasteland";
            _className = "SpellNuclearWasteland";
            _physics = true;
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 10;
            _violent = true;
            PracticeInfo p0 = {CLASS_PHYSIC, 40, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellNuclearWastelandInfo();
        }
};

class SpellSpacetimeImprintInfo : public SkillInfo
{
    public:
        SpellSpacetimeImprintInfo() : SkillInfo()
        {
            _skillNum = SPELL_SPACETIME_IMPRINT;
            _skillName = "spacetime imprint";
            _className = "SpellSpacetimeImprint";
            _physics = true;
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 10;
            _violent = true;
            PracticeInfo p0 = {CLASS_PHYSIC, 30, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellSpacetimeImprintInfo();
        }
};

class SpellSpacetimeRecallInfo : public SkillInfo
{
    public:
        SpellSpacetimeRecallInfo() : SkillInfo()
        {
            _skillNum = SPELL_SPACETIME_RECALL;
            _skillName = "spacetime recall";
            _className = "SpellSpacetimeRecall";
            _physics = true;
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 10;
            _violent = true;
            PracticeInfo p0 = {CLASS_PHYSIC, 30, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellSpacetimeRecallInfo();
        }
};

class SpellFluoresceInfo : public SkillInfo
{
    public:
        SpellFluoresceInfo() : SkillInfo()
        {
            _skillNum = SPELL_FLUORESCE;
            _skillName = "fluoresce";
            _className = "SpellFluoresce";
            _physics = true;
            _minMana = 10;
            _maxMana = 50;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_PHYSIC, 3, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellFluoresceInfo();
        }
};

class SpellGammaRayInfo : public SkillInfo
{
    public:
        SpellGammaRayInfo() : SkillInfo()
        {
            _skillNum = SPELL_GAMMA_RAY;
            _skillName = "gamma ray";
            _className = "SpellGammaRay";
            _physics = true;
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 2;
            _violent = true;
            PracticeInfo p0 = {CLASS_PHYSIC, 25, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellGammaRayInfo();
        }
};

class SpellGravityWellInfo : public SkillInfo
{
    public:
        SpellGravityWellInfo() : SkillInfo()
        {
            _skillNum = SPELL_GRAVITY_WELL;
            _skillName = "gravity well";
            _className = "SpellGravityWell";
            _physics = true;
            _minMana = 70;
            _maxMana = 130;
            _manaChange = 4;
            _violent = true;
            PracticeInfo p0 = {CLASS_PHYSIC, 38, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellGravityWellInfo();
        }
};

class SpellCapacitanceBoostInfo : public SkillInfo
{
    public:
        SpellCapacitanceBoostInfo() : SkillInfo()
        {
            _skillNum = SPELL_CAPACITANCE_BOOST;
            _skillName = "capacitance boost";
            _className = "SpellCapacitanceBoost";
            _physics = true;
            _minMana = 45;
            _maxMana = 70;
            _manaChange = 3;
            PracticeInfo p0 = {CLASS_PHYSIC, 6, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellCapacitanceBoostInfo();
        }
};

class SpellElectricArcInfo : public SkillInfo
{
    public:
        SpellElectricArcInfo() : SkillInfo()
        {
            _skillNum = SPELL_ELECTRIC_ARC;
            _skillName = "electric arc";
            _className = "SpellElectricArc";
            _physics = true;
            _minMana = 50;
            _maxMana = 90;
            _manaChange = 1;
            _violent = true;
            PracticeInfo p0 = {CLASS_PHYSIC, 21, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellElectricArcInfo();
        }
};

class SpellTemporalCompressionInfo : public SkillInfo
{
    public:
        SpellTemporalCompressionInfo() : SkillInfo()
        {
            _skillNum = SPELL_TEMPORAL_COMPRESSION;
            _skillName = "temporal compression";
            _className = "SpellTemporalCompression";
            _physics = true;
            _minMana = 50;
            _maxMana = 80;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_PHYSIC, 29, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellTemporalCompressionInfo();
        }
};

class SpellTemporalDilationInfo : public SkillInfo
{
    public:
        SpellTemporalDilationInfo() : SkillInfo()
        {
            _skillNum = SPELL_TEMPORAL_DILATION;
            _skillName = "temporal dilation";
            _className = "SpellTemporalDilation";
            _physics = true;
            _minMana = 60;
            _maxMana = 120;
            _manaChange = 6;
            PracticeInfo p0 = {CLASS_PHYSIC, 36, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellTemporalDilationInfo();
        }
};

class SpellHalflifeInfo : public SkillInfo
{
    public:
        SpellHalflifeInfo() : SkillInfo()
        {
            _skillNum = SPELL_HALFLIFE;
            _skillName = "halflife";
            _className = "SpellHalflife";
            _physics = true;
            _minMana = 70;
            _maxMana = 120;
            _manaChange = 5;
            _violent = true;
            PracticeInfo p0 = {CLASS_PHYSIC, 30, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellHalflifeInfo();
        }
};

class SpellMicrowaveInfo : public SkillInfo
{
    public:
        SpellMicrowaveInfo() : SkillInfo()
        {
            _skillNum = SPELL_MICROWAVE;
            _skillName = "microwave";
            _className = "SpellMicrowave";
            _physics = true;
            _minMana = 20;
            _maxMana = 45;
            _manaChange = 2;
            _violent = true;
            PracticeInfo p0 = {CLASS_PHYSIC, 18, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellMicrowaveInfo();
        }
};

class SpellOxidizeInfo : public SkillInfo
{
    public:
        SpellOxidizeInfo() : SkillInfo()
        {
            _skillNum = SPELL_OXIDIZE;
            _skillName = "oxidize";
            _className = "SpellOxidize";
            _physics = true;
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _violent = true;
            PracticeInfo p0 = {CLASS_PHYSIC, 10, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellOxidizeInfo();
        }
};

class SpellRandomCoordinatesInfo : public SkillInfo
{
    public:
        SpellRandomCoordinatesInfo() : SkillInfo()
        {
            _skillNum = SPELL_RANDOM_COORDINATES;
            _skillName = "random coordinates";
            _className = "SpellRandomCoordinates";
            _physics = true;
            _minMana = 20;
            _maxMana = 100;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_PHYSIC, 20, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellRandomCoordinatesInfo();
        }
};

class SpellRepulsionFieldInfo : public SkillInfo
{
    public:
        SpellRepulsionFieldInfo() : SkillInfo()
        {
            _skillNum = SPELL_REPULSION_FIELD;
            _skillName = "repulsion field";
            _className = "SpellRepulsionField";
            _physics = true;
            _minMana = 20;
            _maxMana = 30;
            _manaChange = 1;
            _violent = false;
            PracticeInfo p0 = {CLASS_PHYSIC, 16, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellRepulsionFieldInfo();
        }
};

class SpellVacuumShroudInfo : public SkillInfo
{
    public:
        SpellVacuumShroudInfo() : SkillInfo()
        {
            _skillNum = SPELL_VACUUM_SHROUD;
            _skillName = "vacuum shroud";
            _className = "SpellVacuumShroud";
            _physics = true;
            _minMana = 40;
            _maxMana = 60;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_PHYSIC, 31, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellVacuumShroudInfo();
        }
};

class SpellAlbedoShieldInfo : public SkillInfo
{
    public:
        SpellAlbedoShieldInfo() : SkillInfo()
        {
            _skillNum = SPELL_ALBEDO_SHIELD;
            _skillName = "albedo shield";
            _className = "SpellAlbedoShield";
            _physics = true;
            _minMana = 50;
            _maxMana = 90;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_PHYSIC, 34, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellAlbedoShieldInfo();
        }
};

class SpellTransmittanceInfo : public SkillInfo
{
    public:
        SpellTransmittanceInfo() : SkillInfo()
        {
            _skillNum = SPELL_TRANSMITTANCE;
            _skillName = "transmittance";
            _className = "SpellTransmittance";
            _physics = true;
            _minMana = 30;
            _maxMana = 60;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_PHYSIC, 14, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellTransmittanceInfo();
        }
};

class SpellTimeWarpInfo : public SkillInfo
{
    public:
        SpellTimeWarpInfo() : SkillInfo()
        {
            _skillNum = SPELL_TIME_WARP;
            _skillName = "time warp";
            _className = "SpellTimeWarp";
            _physics = true;
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 8;
            PracticeInfo p0 = {CLASS_PHYSIC, 35, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellTimeWarpInfo();
        }
};

class SpellRadioimmunityInfo : public SkillInfo
{
    public:
        SpellRadioimmunityInfo() : SkillInfo()
        {
            _skillNum = SPELL_RADIOIMMUNITY;
            _skillName = "radioimmunity";
            _className = "SpellRadioimmunity";
            _physics = true;
            _minMana = 20;
            _maxMana = 80;
            _manaChange = 8;
            PracticeInfo p0 = {CLASS_PHYSIC, 20, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellRadioimmunityInfo();
        }
};

class SpellDensifyInfo : public SkillInfo
{
    public:
        SpellDensifyInfo() : SkillInfo()
        {
            _skillNum = SPELL_DENSIFY;
            _skillName = "densify";
            _className = "SpellDensify";
            _physics = true;
            _minMana = 20;
            _maxMana = 80;
            _manaChange = 8;
            PracticeInfo p0 = {CLASS_PHYSIC, 26, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellDensifyInfo();
        }
};

class SpellLatticeHardeningInfo : public SkillInfo
{
    public:
        SpellLatticeHardeningInfo() : SkillInfo()
        {
            _skillNum = SPELL_LATTICE_HARDENING;
            _skillName = "lattice hardening";
            _className = "SpellLatticeHardening";
            _physics = true;
            _minMana = 40;
            _maxMana = 100;
            _manaChange = 4;
            PracticeInfo p0 = {CLASS_PHYSIC, 28, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellLatticeHardeningInfo();
        }
};

class SpellChemicalStabilityInfo : public SkillInfo
{
    public:
        SpellChemicalStabilityInfo() : SkillInfo()
        {
            _skillNum = SPELL_CHEMICAL_STABILITY;
            _skillName = "chemical stability";
            _className = "SpellChemicalStability";
            _physics = true;
            _minMana = 20;
            _maxMana = 80;
            _manaChange = 8;
            PracticeInfo p0 = {CLASS_PHYSIC, 9, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellChemicalStabilityInfo();
        }
};

class SpellRefractionInfo : public SkillInfo
{
    public:
        SpellRefractionInfo() : SkillInfo()
        {
            _skillNum = SPELL_REFRACTION;
            _skillName = "refraction";
            _className = "SpellRefraction";
            _physics = true;
            _minMana = 20;
            _maxMana = 80;
            _manaChange = 8;
            PracticeInfo p0 = {CLASS_PHYSIC, 33, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellRefractionInfo();
        }
};

class SpellNullifyInfo : public SkillInfo
{
    public:
        SpellNullifyInfo() : SkillInfo()
        {
            _skillNum = SPELL_NULLIFY;
            _skillName = "nullify";
            _className = "SpellNullify";
            _physics = true;
            _minMana = 55;
            _maxMana = 90;
            _manaChange = 5;
            PracticeInfo p0 = {CLASS_PHYSIC, 11, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellNullifyInfo();
        }
};

class SpellAreaStasisInfo : public SkillInfo
{
    public:
        SpellAreaStasisInfo() : SkillInfo()
        {
            _skillNum = SPELL_AREA_STASIS;
            _skillName = "area stasis";
            _className = "SpellAreaStasis";
            _physics = true;
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 5;
            PracticeInfo p0 = {CLASS_PHYSIC, 40, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellAreaStasisInfo();
        }
};

class SpellEmpPulseInfo : public SkillInfo
{
    public:
        SpellEmpPulseInfo() : SkillInfo()
        {
            _skillNum = SPELL_EMP_PULSE;
            _skillName = "emp pulse";
            _className = "SpellEmpPulse";
            _physics = true;
            _minMana = 75;
            _maxMana = 145;
            _manaChange = 5;
            PracticeInfo p0 = {CLASS_PHYSIC, 34, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellEmpPulseInfo();
        }
};

class SpellFissionBlastInfo : public SkillInfo
{
    public:
        SpellFissionBlastInfo() : SkillInfo()
        {
            _skillNum = SPELL_FISSION_BLAST;
            _skillName = "fission blast";
            _className = "SpellFissionBlast";
            _physics = true;
            _minMana = 70;
            _maxMana = 150;
            _manaChange = 10;
            _violent = true;
            PracticeInfo p0 = {CLASS_PHYSIC, 43, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellFissionBlastInfo();
        }
};

class SpellDimensionalVoidInfo : public SkillInfo
{
    public:
        SpellDimensionalVoidInfo() : SkillInfo()
        {
            _skillNum = SPELL_DIMENSIONAL_VOID;
            _skillName = "dimensional void";
            _className = "SpellDimensionalVoid";
            _physics = true;
            _violent = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellDimensionalVoidInfo();
        }
};

class SkillAmbushInfo : public SkillInfo
{
    public:
        SkillAmbushInfo() : SkillInfo()
        {
            _skillNum = SKILL_AMBUSH;
            _skillName = "ambush";
            _className = "SkillAmbush";
            _skill = true;
            _violent = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillAmbushInfo();
        }
};

class ZenHealingInfo : public SkillInfo
{
    public:
        ZenHealingInfo() : SkillInfo()
        {
            _skillNum = ZEN_HEALING;
            _skillName = "healing";
            _className = "ZenHealing";
            _zen = true;
            _minMana = 9;
            _maxMana = 30;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_MONK, 36, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new ZenHealingInfo();
        }
};

class ZenOfAwarenessInfo : public SkillInfo
{
    public:
        ZenOfAwarenessInfo() : SkillInfo()
        {
            _skillNum = ZEN_AWARENESS;
            _skillName = "awareness";
            _className = "ZenOfAwareness";
            _zen = true;
            _minMana = 5;
            _maxMana = 30;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_MONK, 21, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new ZenOfAwarenessInfo();
        }
};

class ZenOblivityInfo : public SkillInfo
{
    public:
        ZenOblivityInfo() : SkillInfo()
        {
            _skillNum = ZEN_OBLIVITY;
            _skillName = "oblivity";
            _className = "ZenOblivity";
            _zen = true;
            _minMana = 10;
            _maxMana = 40;
            _manaChange = 4;
            PracticeInfo p0 = {CLASS_MONK, 43, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new ZenOblivityInfo();
        }
};

class ZenMotionInfo : public SkillInfo
{
    public:
        ZenMotionInfo() : SkillInfo()
        {
            _skillNum = ZEN_MOTION;
            _skillName = "motion";
            _className = "ZenMotion";
            _zen = true;
            _minMana = 7;
            _maxMana = 30;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_MONK, 28, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new ZenMotionInfo();
        }
};

class ZenDispassionInfo : public SkillInfo
{
    public:
        ZenDispassionInfo() : SkillInfo()
        {
            _skillNum = ZEN_DISPASSION;
            _skillName = "dispassion";
            _className = "ZenDispassion";
            _zen = true;
            _minMana = 50;
            _maxMana = 30;
            _manaChange = 30;
            PracticeInfo p0 = {CLASS_MONK, 45, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new ZenDispassionInfo();
        }
};

class SongDriftersDittyInfo : public SkillInfo
{
    public:
        SongDriftersDittyInfo() : SkillInfo()
        {
            _skillNum = SONG_DRIFTERS_DITTY;
            _skillName = "drifters ditty";
            _className = "SongDriftersDitty";
            _song = true;
            _minMana = 75;
            _maxMana = 90;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_BARD, 4, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongDriftersDittyInfo();
        }
};

class SongAriaOfArmamentInfo : public SkillInfo
{
    public:
        SongAriaOfArmamentInfo() : SkillInfo()
        {
            _skillNum = SONG_ARIA_OF_ARMAMENT;
            _skillName = "aria of armament";
            _className = "SongAriaOfArmament";
            _song = true;
            _minMana = 50;
            _maxMana = 75;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_BARD, 3, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongAriaOfArmamentInfo();
        }
};

class SongVerseOfVulnerabilityInfo : public SkillInfo
{
    public:
        SongVerseOfVulnerabilityInfo() : SkillInfo()
        {
            _skillNum = SONG_VERSE_OF_VULNERABILITY;
            _skillName = "verse of vulnerability";
            _className = "SongVerseOfVulnerability";
            _song = true;
            _minMana = 20;
            _maxMana = 50;
            _manaChange = 3;
            _violent = true;
            PracticeInfo p0 = {CLASS_BARD, 8, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongVerseOfVulnerabilityInfo();
        }
};

class SongMelodyOfMettleInfo : public SkillInfo
{
    public:
        SongMelodyOfMettleInfo() : SkillInfo()
        {
            _skillNum = SONG_MELODY_OF_METTLE;
            _skillName = "melody of mettle";
            _className = "SongMelodyOfMettle";
            _song = true;
            _minMana = 75;
            _maxMana = 100;
            _manaChange = 3;
            PracticeInfo p0 = {CLASS_BARD, 11, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongMelodyOfMettleInfo();
        }
};

class SongRegalersRhapsodyInfo : public SkillInfo
{
    public:
        SongRegalersRhapsodyInfo() : SkillInfo()
        {
            _skillNum = SONG_REGALERS_RHAPSODY;
            _skillName = "regalers rhapsody";
            _className = "SongRegalersRhapsody";
            _song = true;
            _minMana = 50;
            _maxMana = 65;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_BARD, 5, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongRegalersRhapsodyInfo();
        }
};

class SongDefenseDittyInfo : public SkillInfo
{
    public:
        SongDefenseDittyInfo() : SkillInfo()
        {
            _skillNum = SONG_DEFENSE_DITTY;
            _skillName = "defense ditty";
            _className = "SongDefenseDitty";
            _song = true;
            _minMana = 95;
            _maxMana = 120;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_BARD, 39, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongDefenseDittyInfo();
        }
};

class SongAlronsAriaInfo : public SkillInfo
{
    public:
        SongAlronsAriaInfo() : SkillInfo()
        {
            _skillNum = SONG_ALRONS_ARIA;
            _skillName = "alrons aria";
            _className = "SongAlronsAria";
            _song = true;
            _minMana = 95;
            _maxMana = 120;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_BARD, 24, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongAlronsAriaInfo();
        }
};

class SongLustrationMelismaInfo : public SkillInfo
{
    public:
        SongLustrationMelismaInfo() : SkillInfo()
        {
            _skillNum = SONG_LUSTRATION_MELISMA;
            _skillName = "lustration melisma";
            _className = "SongLustrationMelisma";
            _song = true;
            _minMana = 75;
            _maxMana = 95;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_BARD, 2, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongLustrationMelismaInfo();
        }
};

class SongExposureOvertureInfo : public SkillInfo
{
    public:
        SongExposureOvertureInfo() : SkillInfo()
        {
            _skillNum = SONG_EXPOSURE_OVERTURE;
            _skillName = "exposure overture";
            _className = "SongExposureOverture";
            _song = true;
            _minMana = 75;
            _maxMana = 95;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_BARD, 25, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongExposureOvertureInfo();
        }
};

class SongVerseOfValorInfo : public SkillInfo
{
    public:
        SongVerseOfValorInfo() : SkillInfo()
        {
            _skillNum = SONG_VERSE_OF_VALOR;
            _skillName = "verse of valor";
            _className = "SongVerseOfValor";
            _song = true;
            _minMana = 75;
            _maxMana = 95;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_BARD, 10, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongVerseOfValorInfo();
        }
};

class SongWhiteNoiseInfo : public SkillInfo
{
    public:
        SongWhiteNoiseInfo() : SkillInfo()
        {
            _skillNum = SONG_WHITE_NOISE;
            _skillName = "white noise";
            _className = "SongWhiteNoise";
            _song = true;
            _minMana = 40;
            _maxMana = 80;
            _manaChange = 2;
            _violent = true;
            PracticeInfo p0 = {CLASS_BARD, 35, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongWhiteNoiseInfo();
        }
};

class SongHomeSweetHomeInfo : public SkillInfo
{
    public:
        SongHomeSweetHomeInfo() : SkillInfo()
        {
            _skillNum = SONG_HOME_SWEET_HOME;
            _skillName = "home sweet home";
            _className = "SongHomeSweetHome";
            _song = true;
            _minMana = 20;
            _maxMana = 100;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_BARD, 9, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongHomeSweetHomeInfo();
        }
};

class SongChantOfLightInfo : public SkillInfo
{
    public:
        SongChantOfLightInfo() : SkillInfo()
        {
            _skillNum = SONG_CHANT_OF_LIGHT;
            _skillName = "chant of light";
            _className = "SongChantOfLight";
            _song = true;
            _minMana = 20;
            _maxMana = 30;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_BARD, 1, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongChantOfLightInfo();
        }
};

class SongInsidiousRhythmInfo : public SkillInfo
{
    public:
        SongInsidiousRhythmInfo() : SkillInfo()
        {
            _skillNum = SONG_INSIDIOUS_RHYTHM;
            _skillName = "insidious rhythm";
            _className = "SongInsidiousRhythm";
            _song = true;
            _minMana = 55;
            _maxMana = 90;
            _manaChange = 1;
            _violent = true;
            PracticeInfo p0 = {CLASS_BARD, 20, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongInsidiousRhythmInfo();
        }
};

class SongEaglesOvertureInfo : public SkillInfo
{
    public:
        SongEaglesOvertureInfo() : SkillInfo()
        {
            _skillNum = SONG_EAGLES_OVERTURE;
            _skillName = "eagles overture";
            _className = "SongEaglesOverture";
            _song = true;
            _minMana = 25;
            _maxMana = 55;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_BARD, 25, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongEaglesOvertureInfo();
        }
};

class SongWeightOfTheWorldInfo : public SkillInfo
{
    public:
        SongWeightOfTheWorldInfo() : SkillInfo()
        {
            _skillNum = SONG_WEIGHT_OF_THE_WORLD;
            _skillName = "weight of the world";
            _className = "SongWeightOfTheWorld";
            _song = true;
            _minMana = 150;
            _maxMana = 180;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_BARD, 21, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongWeightOfTheWorldInfo();
        }
};

class SongGuihariasGloryInfo : public SkillInfo
{
    public:
        SongGuihariasGloryInfo() : SkillInfo()
        {
            _skillNum = SONG_GUIHARIAS_GLORY;
            _skillName = "guiharias glory";
            _className = "SongGuihariasGlory";
            _song = true;
            _minMana = 80;
            _maxMana = 120;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_BARD, 16, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongGuihariasGloryInfo();
        }
};

class SongRhapsodyOfRemedyInfo : public SkillInfo
{
    public:
        SongRhapsodyOfRemedyInfo() : SkillInfo()
        {
            _skillNum = SONG_RHAPSODY_OF_REMEDY;
            _skillName = "rhapsody of remedy";
            _className = "SongRhapsodyOfRemedy";
            _song = true;
            _minMana = 50;
            _maxMana = 80;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_BARD, 28, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongRhapsodyOfRemedyInfo();
        }
};

class SongUnladenSwallowSongInfo : public SkillInfo
{
    public:
        SongUnladenSwallowSongInfo() : SkillInfo()
        {
            _skillNum = SONG_UNLADEN_SWALLOW_SONG;
            _skillName = "unladen swallow song";
            _className = "SongUnladenSwallowSong";
            _song = true;
            _minMana = 85;
            _maxMana = 105;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_BARD, 19, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongUnladenSwallowSongInfo();
        }
};

class SongClarifyingHarmoniesInfo : public SkillInfo
{
    public:
        SongClarifyingHarmoniesInfo() : SkillInfo()
        {
            _skillNum = SONG_CLARIFYING_HARMONIES;
            _skillName = "clarifying harmonies";
            _className = "SongClarifyingHarmonies";
            _song = true;
            _minMana = 20;
            _maxMana = 45;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_BARD, 12, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongClarifyingHarmoniesInfo();
        }
};

class SongPowerOvertureInfo : public SkillInfo
{
    public:
        SongPowerOvertureInfo() : SkillInfo()
        {
            _skillNum = SONG_POWER_OVERTURE;
            _skillName = "power overture";
            _className = "SongPowerOverture";
            _song = true;
            _minMana = 25;
            _maxMana = 50;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_BARD, 13, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongPowerOvertureInfo();
        }
};

class SongRhythmOfRageInfo : public SkillInfo
{
    public:
        SongRhythmOfRageInfo() : SkillInfo()
        {
            _skillNum = SONG_RHYTHM_OF_RAGE;
            _skillName = "rhythm of rage";
            _className = "SongRhythmOfRage";
            _song = true;
            _minMana = 180;
            _maxMana = 325;
            _manaChange = 3;
            PracticeInfo p0 = {CLASS_BARD, 36, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongRhythmOfRageInfo();
        }
};

class SongUnravellingDiapasonInfo : public SkillInfo
{
    public:
        SongUnravellingDiapasonInfo() : SkillInfo()
        {
            _skillNum = SONG_UNRAVELLING_DIAPASON;
            _skillName = "unravelling diapason";
            _className = "SongUnravellingDiapason";
            _song = true;
            _minMana = 75;
            _maxMana = 120;
            _manaChange = 3;
            PracticeInfo p0 = {CLASS_BARD, 40, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongUnravellingDiapasonInfo();
        }
};

class SongInstantAudienceInfo : public SkillInfo
{
    public:
        SongInstantAudienceInfo() : SkillInfo()
        {
            _skillNum = SONG_INSTANT_AUDIENCE;
            _skillName = "instant audience";
            _className = "SongInstantAudience";
            _song = true;
            _minMana = 100;
            _maxMana = 190;
            _manaChange = 5;
            PracticeInfo p0 = {CLASS_BARD, 26, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongInstantAudienceInfo();
        }
};

class SongWoundingWhispersInfo : public SkillInfo
{
    public:
        SongWoundingWhispersInfo() : SkillInfo()
        {
            _skillNum = SONG_WOUNDING_WHISPERS;
            _skillName = "wounding whispers";
            _className = "SongWoundingWhispers";
            _song = true;
            _minMana = 60;
            _maxMana = 120;
            _manaChange = 3;
            PracticeInfo p0 = {CLASS_BARD, 36, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongWoundingWhispersInfo();
        }
};

class SongRhythmOfAlarmInfo : public SkillInfo
{
    public:
        SongRhythmOfAlarmInfo() : SkillInfo()
        {
            _skillNum = SONG_RHYTHM_OF_ALARM;
            _skillName = "rhythm of alarm";
            _className = "SongRhythmOfAlarm";
            _song = true;
            _minMana = 75;
            _maxMana = 100;
            _manaChange = 1;
            _violent = true;
            PracticeInfo p0 = {CLASS_BARD, 41, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongRhythmOfAlarmInfo();
        }
};

class SkillTumblingInfo : public SkillInfo
{
    public:
        SkillTumblingInfo() : SkillInfo()
        {
            _skillNum = SKILL_TUMBLING;
            _skillName = "tumbling";
            _className = "SkillTumbling";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARD, 30, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillTumblingInfo();
        }
};

class SkillScreamInfo : public SkillInfo
{
    public:
        SkillScreamInfo() : SkillInfo()
        {
            _skillNum = SKILL_SCREAM;
            _skillName = "scream";
            _className = "SkillScream";
            _song = true;
            _minMana = 35;
            _maxMana = 35;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_BARD, 5, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillScreamInfo();
        }
};

class SongSonicDisruptionInfo : public SkillInfo
{
    public:
        SongSonicDisruptionInfo() : SkillInfo()
        {
            _skillNum = SONG_SONIC_DISRUPTION;
            _skillName = "sonic disruption";
            _className = "SongSonicDisruption";
            _song = true;
            _minMana = 60;
            _maxMana = 100;
            _manaChange = 5;
            _violent = true;
            PracticeInfo p0 = {CLASS_BARD, 17, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongSonicDisruptionInfo();
        }
};

class SongDirgeInfo : public SkillInfo
{
    public:
        SongDirgeInfo() : SkillInfo()
        {
            _skillNum = SONG_DIRGE;
            _skillName = "dirge";
            _className = "SongDirge";
            _song = true;
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 5;
            _violent = true;
            PracticeInfo p0 = {CLASS_BARD, 6, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongDirgeInfo();
        }
};

class SongMisdirectionMelismaInfo : public SkillInfo
{
    public:
        SongMisdirectionMelismaInfo() : SkillInfo()
        {
            _skillNum = SONG_MISDIRECTION_MELISMA;
            _skillName = "misdirection melisma";
            _className = "SongMisdirectionMelisma";
            _song = true;
            _minMana = 50;
            _maxMana = 75;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_BARD, 34, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongMisdirectionMelismaInfo();
        }
};

class SongHymnOfPeaceInfo : public SkillInfo
{
    public:
        SongHymnOfPeaceInfo() : SkillInfo()
        {
            _skillNum = SONG_HYMN_OF_PEACE;
            _skillName = "hymn of peace";
            _className = "SongHymnOfPeace";
            _song = true;
            _minMana = 50;
            _maxMana = 75;
            _manaChange = 3;
            PracticeInfo p0 = {CLASS_BARD, 33, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongHymnOfPeaceInfo();
        }
};

class SkillVentriloquismInfo : public SkillInfo
{
    public:
        SkillVentriloquismInfo() : SkillInfo()
        {
            _skillNum = SKILL_VENTRILOQUISM;
            _skillName = "ventriloquism";
            _className = "SkillVentriloquism";
            _skill = true;
            _violent = false;
            PracticeInfo p0 = {CLASS_BARD, 25, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillVentriloquismInfo();
        }
};

class SpellTidalSpacewarpInfo : public SkillInfo
{
    public:
        SpellTidalSpacewarpInfo() : SkillInfo()
        {
            _skillNum = SPELL_TIDAL_SPACEWARP;
            _skillName = "tidal spacewarp";
            _className = "SpellTidalSpacewarp";
            _physics = true;
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_PHYSIC, 15, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellTidalSpacewarpInfo();
        }
};

class SkillCleaveInfo : public SkillInfo
{
    public:
        SkillCleaveInfo() : SkillInfo()
        {
            _skillNum = SKILL_CLEAVE;
            _skillName = "cleave";
            _className = "SkillCleave";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 30, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillCleaveInfo();
        }
};

class SkillStrikeInfo : public SkillInfo
{
    public:
        SkillStrikeInfo() : SkillInfo()
        {
            _skillNum = SKILL_STRIKE;
            _skillName = "strike";
            _className = "SkillStrike";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 17, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillStrikeInfo();
        }
};

class SkillHamstringInfo : public SkillInfo
{
    public:
        SkillHamstringInfo() : SkillInfo()
        {
            _skillNum = SKILL_HAMSTRING;
            _skillName = "hamstring";
            _className = "SkillHamstring";
            _skill = true;
            PracticeInfo p0 = {CLASS_MERCENARY, 32, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillHamstringInfo();
        }
};

class SkillDragInfo : public SkillInfo
{
    public:
        SkillDragInfo() : SkillInfo()
        {
            _skillNum = SKILL_DRAG;
            _skillName = "drag";
            _className = "SkillDrag";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 40, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillDragInfo();
        }
};

class SkillSnatchInfo : public SkillInfo
{
    public:
        SkillSnatchInfo() : SkillInfo()
        {
            _skillNum = SKILL_SNATCH;
            _skillName = "snatch";
            _className = "SkillSnatch";
            _skill = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillSnatchInfo();
        }
};

class SkillArcheryInfo : public SkillInfo
{
    public:
        SkillArcheryInfo() : SkillInfo()
        {
            _skillNum = SKILL_ARCHERY;
            _skillName = "archery";
            _className = "SkillArchery";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 14, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_BARB, 24, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 9, 0};
            _practiceInfo.push_back(p2);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillArcheryInfo();
        }
};

class SkillBowFletchInfo : public SkillInfo
{
    public:
        SkillBowFletchInfo() : SkillInfo()
        {
            _skillNum = SKILL_BOW_FLETCH;
            _skillName = "bow fletch";
            _className = "SkillBowFletch";
            _skill = true;
            PracticeInfo p0 = {CLASS_RANGER, 17, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillBowFletchInfo();
        }
};

class SkillReadScrollsInfo : public SkillInfo
{
    public:
        SkillReadScrollsInfo() : SkillInfo()
        {
            _skillNum = SKILL_READ_SCROLLS;
            _skillName = "read scrolls";
            _className = "SkillReadScrolls";
            _skill = true;
            PracticeInfo p0 = {CLASS_MAGE, 1, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 2, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_THIEF, 3, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_PSIONIC, 35, 0};
            _practiceInfo.push_back(p3);
            PracticeInfo p4 = {CLASS_KNIGHT, 21, 0};
            _practiceInfo.push_back(p4);
            PracticeInfo p5 = {CLASS_RANGER, 21, 0};
            _practiceInfo.push_back(p5);
            PracticeInfo p6 = {CLASS_BARD, 5, 0};
            _practiceInfo.push_back(p6);
            PracticeInfo p7 = {CLASS_MONK, 7, 0};
            _practiceInfo.push_back(p7);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillReadScrollsInfo();
        }
};

class SkillUseWandsInfo : public SkillInfo
{
    public:
        SkillUseWandsInfo() : SkillInfo()
        {
            _skillNum = SKILL_USE_WANDS;
            _skillName = "use wands";
            _className = "SkillUseWands";
            _skill = true;
            PracticeInfo p0 = {CLASS_MAGE, 4, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 9, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_THIEF, 10, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_PSIONIC, 20, 0};
            _practiceInfo.push_back(p3);
            PracticeInfo p4 = {CLASS_KNIGHT, 21, 0};
            _practiceInfo.push_back(p4);
            PracticeInfo p5 = {CLASS_RANGER, 29, 0};
            _practiceInfo.push_back(p5);
            PracticeInfo p6 = {CLASS_BARD, 5, 0};
            _practiceInfo.push_back(p6);
            PracticeInfo p7 = {CLASS_MONK, 35, 0};
            _practiceInfo.push_back(p7);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillUseWandsInfo();
        }
};

class SkillBackstabInfo : public SkillInfo
{
    public:
        SkillBackstabInfo() : SkillInfo()
        {
            _skillNum = SKILL_BACKSTAB;
            _skillName = "backstab";
            _className = "SkillBackstab";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 5, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillBackstabInfo();
        }
};

class SkillCircleInfo : public SkillInfo
{
    public:
        SkillCircleInfo() : SkillInfo()
        {
            _skillNum = SKILL_CIRCLE;
            _skillName = "circle";
            _className = "SkillCircle";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 33, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillCircleInfo();
        }
};

class SkillHideInfo : public SkillInfo
{
    public:
        SkillHideInfo() : SkillInfo()
        {
            _skillNum = SKILL_HIDE;
            _skillName = "hide";
            _className = "SkillHide";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 3, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_RANGER, 15, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_MERCENARY, 30, 0};
            _practiceInfo.push_back(p2);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillHideInfo();
        }
};

class SkillKickInfo : public SkillInfo
{
    public:
        SkillKickInfo() : SkillInfo()
        {
            _skillNum = SKILL_KICK;
            _skillName = "kick";
            _className = "SkillKick";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 2, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CYBORG, 3, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 3, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_MERCENARY, 2, 0};
            _practiceInfo.push_back(p3);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillKickInfo();
        }
};

class SkillBashInfo : public SkillInfo
{
    public:
        SkillBashInfo() : SkillInfo()
        {
            _skillNum = SKILL_BASH;
            _skillName = "bash";
            _className = "SkillBash";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 10, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_KNIGHT, 9, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 14, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_MERCENARY, 4, 0};
            _practiceInfo.push_back(p3);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillBashInfo();
        }
};

class SkillBreakDoorInfo : public SkillInfo
{
    public:
        SkillBreakDoorInfo() : SkillInfo()
        {
            _skillNum = SKILL_BREAK_DOOR;
            _skillName = "break door";
            _className = "SkillBreakDoor";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 26, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_MERCENARY, 1, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillBreakDoorInfo();
        }
};

class SkillHeadbuttInfo : public SkillInfo
{
    public:
        SkillHeadbuttInfo() : SkillInfo()
        {
            _skillNum = SKILL_HEADBUTT;
            _skillName = "headbutt";
            _className = "SkillHeadbutt";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 13, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_RANGER, 16, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillHeadbuttInfo();
        }
};

class SkillHotwireInfo : public SkillInfo
{
    public:
        SkillHotwireInfo() : SkillInfo()
        {
            _skillNum = SKILL_HOTWIRE;
            _skillName = "hotwire";
            _className = "SkillHotwire";
            _skill = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillHotwireInfo();
        }
};

class SkillGougeInfo : public SkillInfo
{
    public:
        SkillGougeInfo() : SkillInfo()
        {
            _skillNum = SKILL_GOUGE;
            _skillName = "gouge";
            _className = "SkillGouge";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 30, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_MONK, 36, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillGougeInfo();
        }
};

class SkillStunInfo : public SkillInfo
{
    public:
        SkillStunInfo() : SkillInfo()
        {
            _skillNum = SKILL_STUN;
            _skillName = "stun";
            _className = "SkillStun";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 35, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillStunInfo();
        }
};

class SkillFeignInfo : public SkillInfo
{
    public:
        SkillFeignInfo() : SkillInfo()
        {
            _skillNum = SKILL_FEIGN;
            _skillName = "feign";
            _className = "SkillFeign";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 25, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillFeignInfo();
        }
};

class SkillConcealInfo : public SkillInfo
{
    public:
        SkillConcealInfo() : SkillInfo()
        {
            _skillNum = SKILL_CONCEAL;
            _skillName = "conceal";
            _className = "SkillConceal";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 15, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_MERCENARY, 30, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillConcealInfo();
        }
};

class SkillPlantInfo : public SkillInfo
{
    public:
        SkillPlantInfo() : SkillInfo()
        {
            _skillNum = SKILL_PLANT;
            _skillName = "plant";
            _className = "SkillPlant";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 20, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_MERCENARY, 35, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillPlantInfo();
        }
};

class SkillPickLockInfo : public SkillInfo
{
    public:
        SkillPickLockInfo() : SkillInfo()
        {
            _skillNum = SKILL_PICK_LOCK;
            _skillName = "pick lock";
            _className = "SkillPickLock";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 1, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillPickLockInfo();
        }
};

class SkillRescueInfo : public SkillInfo
{
    public:
        SkillRescueInfo() : SkillInfo()
        {
            _skillNum = SKILL_RESCUE;
            _skillName = "rescue";
            _className = "SkillRescue";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 12, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_KNIGHT, 8, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 12, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_MONK, 16, 0};
            _practiceInfo.push_back(p3);
            PracticeInfo p4 = {CLASS_MERCENARY, 26, 0};
            _practiceInfo.push_back(p4);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillRescueInfo();
        }
};

class SkillSneakInfo : public SkillInfo
{
    public:
        SkillSneakInfo() : SkillInfo()
        {
            _skillNum = SKILL_SNEAK;
            _skillName = "sneak";
            _className = "SkillSneak";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 3, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_RANGER, 8, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_BARD, 10, 0};
            _practiceInfo.push_back(p2);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillSneakInfo();
        }
};

class SkillStealInfo : public SkillInfo
{
    public:
        SkillStealInfo() : SkillInfo()
        {
            _skillNum = SKILL_STEAL;
            _skillName = "steal";
            _className = "SkillSteal";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 2, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_BARD, 15, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillStealInfo();
        }
};

class SkillTrackInfo : public SkillInfo
{
    public:
        SkillTrackInfo() : SkillInfo()
        {
            _skillNum = SKILL_TRACK;
            _skillName = "track";
            _className = "SkillTrack";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 8, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_BARB, 14, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_CYBORG, 11, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_RANGER, 11, 0};
            _practiceInfo.push_back(p3);
            PracticeInfo p4 = {CLASS_MERCENARY, 14, 0};
            _practiceInfo.push_back(p4);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillTrackInfo();
        }
};

class SkillPunchInfo : public SkillInfo
{
    public:
        SkillPunchInfo() : SkillInfo()
        {
            _skillNum = SKILL_PUNCH;
            _skillName = "punch";
            _className = "SkillPunch";
            _skill = true;
            PracticeInfo p0 = {CLASS_NONE, 1, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillPunchInfo();
        }
};

class SkillPiledriveInfo : public SkillInfo
{
    public:
        SkillPiledriveInfo() : SkillInfo()
        {
            _skillNum = SKILL_PILEDRIVE;
            _skillName = "piledrive";
            _className = "SkillPiledrive";
            _skill = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillPiledriveInfo();
        }
};

class SkillShieldMasteryInfo : public SkillInfo
{
    public:
        SkillShieldMasteryInfo() : SkillInfo()
        {
            _skillNum = SKILL_SHIELD_MASTERY;
            _skillName = "shield mastery";
            _className = "SkillShieldMastery";
            _skill = true;
            PracticeInfo p0 = {CLASS_KNIGHT, 22, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillShieldMasteryInfo();
        }
};

class SkillSleeperInfo : public SkillInfo
{
    public:
        SkillSleeperInfo() : SkillInfo()
        {
            _skillNum = SKILL_SLEEPER;
            _skillName = "sleeper";
            _className = "SkillSleeper";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 25, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillSleeperInfo();
        }
};

class SkillElbowInfo : public SkillInfo
{
    public:
        SkillElbowInfo() : SkillInfo()
        {
            _skillNum = SKILL_ELBOW;
            _skillName = "elbow";
            _className = "SkillElbow";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 8, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CYBORG, 9, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillElbowInfo();
        }
};

class SkillKneeInfo : public SkillInfo
{
    public:
        SkillKneeInfo() : SkillInfo()
        {
            _skillNum = SKILL_KNEE;
            _skillName = "knee";
            _className = "SkillKnee";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 6, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillKneeInfo();
        }
};

class SkillAutopsyInfo : public SkillInfo
{
    public:
        SkillAutopsyInfo() : SkillInfo()
        {
            _skillNum = SKILL_AUTOPSY;
            _skillName = "autopsy";
            _className = "SkillAutopsy";
            _skill = true;
            PracticeInfo p0 = {CLASS_CYBORG, 40, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_RANGER, 36, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_MERCENARY, 10, 0};
            _practiceInfo.push_back(p2);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillAutopsyInfo();
        }
};

class SkillBerserkInfo : public SkillInfo
{
    public:
        SkillBerserkInfo() : SkillInfo()
        {
            _skillNum = SKILL_BERSERK;
            _skillName = "berserk";
            _className = "SkillBerserk";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 22, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillBerserkInfo();
        }
};

class SkillBattleCryInfo : public SkillInfo
{
    public:
        SkillBattleCryInfo() : SkillInfo()
        {
            _skillNum = SKILL_BATTLE_CRY;
            _skillName = "battle cry";
            _className = "SkillBattleCry";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 33, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillBattleCryInfo();
        }
};

class SkillKiaInfo : public SkillInfo
{
    public:
        SkillKiaInfo() : SkillInfo()
        {
            _skillNum = SKILL_KIA;
            _skillName = "kia";
            _className = "SkillKia";
            _skill = true;
            PracticeInfo p0 = {CLASS_MONK, 23, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillKiaInfo();
        }
};

class SkillCryFromBeyondInfo : public SkillInfo
{
    public:
        SkillCryFromBeyondInfo() : SkillInfo()
        {
            _skillNum = SKILL_CRY_FROM_BEYOND;
            _skillName = "cry from beyond";
            _className = "SkillCryFromBeyond";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 42, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillCryFromBeyondInfo();
        }
};

class SkillStompInfo : public SkillInfo
{
    public:
        SkillStompInfo() : SkillInfo()
        {
            _skillNum = SKILL_STOMP;
            _skillName = "stomp";
            _className = "SkillStomp";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 5, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_BARB, 4, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillStompInfo();
        }
};

class SkillBodyslamInfo : public SkillInfo
{
    public:
        SkillBodyslamInfo() : SkillInfo()
        {
            _skillNum = SKILL_BODYSLAM;
            _skillName = "bodyslam";
            _className = "SkillBodyslam";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 20, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillBodyslamInfo();
        }
};

class SkillChokeInfo : public SkillInfo
{
    public:
        SkillChokeInfo() : SkillInfo()
        {
            _skillNum = SKILL_CHOKE;
            _skillName = "choke";
            _className = "SkillChoke";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 23, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_BARB, 16, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillChokeInfo();
        }
};

class SkillClotheslineInfo : public SkillInfo
{
    public:
        SkillClotheslineInfo() : SkillInfo()
        {
            _skillNum = SKILL_CLOTHESLINE;
            _skillName = "clothesline";
            _className = "SkillClothesline";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 18, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_MERCENARY, 12, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillClotheslineInfo();
        }
};

class SkillTagInfo : public SkillInfo
{
    public:
        SkillTagInfo() : SkillInfo()
        {
            _skillNum = SKILL_TAG;
            _skillName = "tag";
            _className = "SkillTag";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 27, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillTagInfo();
        }
};

class SkillIntimidateInfo : public SkillInfo
{
    public:
        SkillIntimidateInfo() : SkillInfo()
        {
            _skillNum = SKILL_INTIMIDATE;
            _skillName = "intimidate";
            _className = "SkillIntimidate";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 35, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillIntimidateInfo();
        }
};

class SkillSpeedHealingInfo : public SkillInfo
{
    public:
        SkillSpeedHealingInfo() : SkillInfo()
        {
            _skillNum = SKILL_SPEED_HEALING;
            _skillName = "speed healing";
            _className = "SkillSpeedHealing";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 39, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillSpeedHealingInfo();
        }
};

class SkillStalkInfo : public SkillInfo
{
    public:
        SkillStalkInfo() : SkillInfo()
        {
            _skillNum = SKILL_STALK;
            _skillName = "stalk";
            _className = "SkillStalk";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 40, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillStalkInfo();
        }
};

class SkillHearingInfo : public SkillInfo
{
    public:
        SkillHearingInfo() : SkillInfo()
        {
            _skillNum = SKILL_HEARING;
            _skillName = "hearing";
            _className = "SkillHearing";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 11, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillHearingInfo();
        }
};

class SkillBearhugInfo : public SkillInfo
{
    public:
        SkillBearhugInfo() : SkillInfo()
        {
            _skillNum = SKILL_BEARHUG;
            _skillName = "bearhug";
            _className = "SkillBearhug";
            _skill = true;
            PracticeInfo p0 = {CLASS_RANGER, 25, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillBearhugInfo();
        }
};

class SkillBiteInfo : public SkillInfo
{
    public:
        SkillBiteInfo() : SkillInfo()
        {
            _skillNum = SKILL_BITE;
            _skillName = "bite";
            _className = "SkillBite";
            _skill = true;
            PracticeInfo p0 = {CLASS_RANGER, 5, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillBiteInfo();
        }
};

class SkillDblAttackInfo : public SkillInfo
{
    public:
        SkillDblAttackInfo() : SkillInfo()
        {
            _skillNum = SKILL_DBL_ATTACK;
            _skillName = "dbl attack";
            _className = "SkillDblAttack";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 45, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_BARB, 37, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_KNIGHT, 25, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_RANGER, 30, 0};
            _practiceInfo.push_back(p3);
            PracticeInfo p4 = {CLASS_BARD, 35, 0};
            _practiceInfo.push_back(p4);
            PracticeInfo p5 = {CLASS_MONK, 24, 0};
            _practiceInfo.push_back(p5);
            PracticeInfo p6 = {CLASS_MERCENARY, 25, 0};
            _practiceInfo.push_back(p6);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillDblAttackInfo();
        }
};

class SkillNightVisionInfo : public SkillInfo
{
    public:
        SkillNightVisionInfo() : SkillInfo()
        {
            _skillNum = SKILL_NIGHT_VISION;
            _skillName = "night vision";
            _className = "SkillNightVision";
            _skill = true;
            PracticeInfo p0 = {CLASS_RANGER, 2, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillNightVisionInfo();
        }
};

class SkillTurnInfo : public SkillInfo
{
    public:
        SkillTurnInfo() : SkillInfo()
        {
            _skillNum = SKILL_TURN;
            _skillName = "turn";
            _className = "SkillTurn";
            _skill = true;
            _minMana = 30;
            _maxMana = 90;
            _manaChange = 5;
            PracticeInfo p0 = {CLASS_CLERIC, 1, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_KNIGHT, 3, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillTurnInfo();
        }
};

class SkillAnalyzeInfo : public SkillInfo
{
    public:
        SkillAnalyzeInfo() : SkillInfo()
        {
            _skillNum = SKILL_ANALYZE;
            _skillName = "analyze";
            _className = "SkillAnalyze";
            _skill = true;
            PracticeInfo p0 = {CLASS_PHYSIC, 12, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CYBORG, 20, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillAnalyzeInfo();
        }
};

class SkillEvaluateInfo : public SkillInfo
{
    public:
        SkillEvaluateInfo() : SkillInfo()
        {
            _skillNum = SKILL_EVALUATE;
            _skillName = "evaluate";
            _className = "SkillEvaluate";
            _skill = true;
            PracticeInfo p0 = {CLASS_PHYSIC, 10, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CYBORG, 20, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillEvaluateInfo();
        }
};

class SkillCureDiseaseInfo : public SkillInfo
{
    public:
        SkillCureDiseaseInfo() : SkillInfo()
        {
            _skillNum = SKILL_CURE_DISEASE;
            _skillName = "cure disease";
            _className = "SkillCureDisease";
            _skill = true;
            _minMana = 10;
            _maxMana = 30;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_KNIGHT, 43, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillCureDiseaseInfo();
        }
};

class SkillHolyTouchInfo : public SkillInfo
{
    public:
        SkillHolyTouchInfo() : SkillInfo()
        {
            _skillNum = SKILL_HOLY_TOUCH;
            _skillName = "holy touch";
            _className = "SkillHolyTouch";
            _skill = true;
            _minMana = 10;
            _maxMana = 30;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_KNIGHT, 2, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillHolyTouchInfo();
        }
};

class SkillBandageInfo : public SkillInfo
{
    public:
        SkillBandageInfo() : SkillInfo()
        {
            _skillNum = SKILL_BANDAGE;
            _skillName = "bandage";
            _className = "SkillBandage";
            _skill = true;
            _minMana = 30;
            _maxMana = 40;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_BARB, 34, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_RANGER, 10, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_MONK, 17, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_MERCENARY, 3, 0};
            _practiceInfo.push_back(p3);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillBandageInfo();
        }
};

class SkillFirstaidInfo : public SkillInfo
{
    public:
        SkillFirstaidInfo() : SkillInfo()
        {
            _skillNum = SKILL_FIRSTAID;
            _skillName = "firstaid";
            _className = "SkillFirstaid";
            _skill = true;
            _minMana = 60;
            _maxMana = 70;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_RANGER, 19, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_MONK, 30, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_MERCENARY, 16, 0};
            _practiceInfo.push_back(p2);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillFirstaidInfo();
        }
};

class SkillMedicInfo : public SkillInfo
{
    public:
        SkillMedicInfo() : SkillInfo()
        {
            _skillNum = SKILL_MEDIC;
            _skillName = "medic";
            _className = "SkillMedic";
            _skill = true;
            _minMana = 80;
            _maxMana = 90;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_RANGER, 40, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillMedicInfo();
        }
};

class SkillLeatherworkingInfo : public SkillInfo
{
    public:
        SkillLeatherworkingInfo() : SkillInfo()
        {
            _skillNum = SKILL_LEATHERWORKING;
            _skillName = "leatherworking";
            _className = "SkillLeatherworking";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 32, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_BARB, 26, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_KNIGHT, 29, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_RANGER, 28, 0};
            _practiceInfo.push_back(p3);
            PracticeInfo p4 = {CLASS_MONK, 32, 0};
            _practiceInfo.push_back(p4);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillLeatherworkingInfo();
        }
};

class SkillMetalworkingInfo : public SkillInfo
{
    public:
        SkillMetalworkingInfo() : SkillInfo()
        {
            _skillNum = SKILL_METALWORKING;
            _skillName = "metalworking";
            _className = "SkillMetalworking";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 41, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_KNIGHT, 32, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 36, 0};
            _practiceInfo.push_back(p2);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillMetalworkingInfo();
        }
};

class SkillConsiderInfo : public SkillInfo
{
    public:
        SkillConsiderInfo() : SkillInfo()
        {
            _skillNum = SKILL_CONSIDER;
            _skillName = "consider";
            _className = "SkillConsider";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 11, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_BARB, 9, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_KNIGHT, 13, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_RANGER, 18, 0};
            _practiceInfo.push_back(p3);
            PracticeInfo p4 = {CLASS_MONK, 13, 0};
            _practiceInfo.push_back(p4);
            PracticeInfo p5 = {CLASS_MERCENARY, 13, 0};
            _practiceInfo.push_back(p5);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillConsiderInfo();
        }
};

class SkillGlanceInfo : public SkillInfo
{
    public:
        SkillGlanceInfo() : SkillInfo()
        {
            _skillNum = SKILL_GLANCE;
            _skillName = "glance";
            _className = "SkillGlance";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 4, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillGlanceInfo();
        }
};

class SkillShootInfo : public SkillInfo
{
    public:
        SkillShootInfo() : SkillInfo()
        {
            _skillNum = SKILL_SHOOT;
            _skillName = "shoot";
            _className = "SkillShoot";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 10, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CYBORG, 20, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 26, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_BARD, 1, 0};
            _practiceInfo.push_back(p3);
            PracticeInfo p4 = {CLASS_MERCENARY, 3, 0};
            _practiceInfo.push_back(p4);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillShootInfo();
        }
};

class SkillBeheadInfo : public SkillInfo
{
    public:
        SkillBeheadInfo() : SkillInfo()
        {
            _skillNum = SKILL_BEHEAD;
            _skillName = "behead";
            _className = "SkillBehead";
            _skill = true;
            PracticeInfo p0 = {CLASS_KNIGHT, 40, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillBeheadInfo();
        }
};

class SkillEmpowerInfo : public SkillInfo
{
    public:
        SkillEmpowerInfo() : SkillInfo()
        {
            _skillNum = SKILL_EMPOWER;
            _skillName = "empower";
            _className = "SkillEmpower";
            _skill = true;
            PracticeInfo p0 = {CLASS_MAGE, 5, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillEmpowerInfo();
        }
};

class SkillDisarmInfo : public SkillInfo
{
    public:
        SkillDisarmInfo() : SkillInfo()
        {
            _skillNum = SKILL_DISARM;
            _skillName = "disarm";
            _className = "SkillDisarm";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 13, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_BARB, 32, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_KNIGHT, 20, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_RANGER, 20, 0};
            _practiceInfo.push_back(p3);
            PracticeInfo p4 = {CLASS_MONK, 31, 0};
            _practiceInfo.push_back(p4);
            PracticeInfo p5 = {CLASS_MERCENARY, 25, 0};
            _practiceInfo.push_back(p5);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillDisarmInfo();
        }
};

class SkillSpinkickInfo : public SkillInfo
{
    public:
        SkillSpinkickInfo() : SkillInfo()
        {
            _skillNum = SKILL_SPINKICK;
            _skillName = "spinkick";
            _className = "SkillSpinkick";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 17, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillSpinkickInfo();
        }
};

class SkillRoundhouseInfo : public SkillInfo
{
    public:
        SkillRoundhouseInfo() : SkillInfo()
        {
            _skillNum = SKILL_ROUNDHOUSE;
            _skillName = "roundhouse";
            _className = "SkillRoundhouse";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 19, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_RANGER, 20, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_MERCENARY, 5, 0};
            _practiceInfo.push_back(p2);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillRoundhouseInfo();
        }
};

class SkillSidekickInfo : public SkillInfo
{
    public:
        SkillSidekickInfo() : SkillInfo()
        {
            _skillNum = SKILL_SIDEKICK;
            _skillName = "sidekick";
            _className = "SkillSidekick";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 29, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_RANGER, 35, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillSidekickInfo();
        }
};

class SkillSpinfistInfo : public SkillInfo
{
    public:
        SkillSpinfistInfo() : SkillInfo()
        {
            _skillNum = SKILL_SPINFIST;
            _skillName = "spinfist";
            _className = "SkillSpinfist";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 7, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_KNIGHT, 6, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 7, 0};
            _practiceInfo.push_back(p2);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillSpinfistInfo();
        }
};

class SkillJabInfo : public SkillInfo
{
    public:
        SkillJabInfo() : SkillInfo()
        {
            _skillNum = SKILL_JAB;
            _skillName = "jab";
            _className = "SkillJab";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 6, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillJabInfo();
        }
};

class SkillHookInfo : public SkillInfo
{
    public:
        SkillHookInfo() : SkillInfo()
        {
            _skillNum = SKILL_HOOK;
            _skillName = "hook";
            _className = "SkillHook";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 23, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_KNIGHT, 22, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 23, 0};
            _practiceInfo.push_back(p2);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillHookInfo();
        }
};

class SkillSweepkickInfo : public SkillInfo
{
    public:
        SkillSweepkickInfo() : SkillInfo()
        {
            _skillNum = SKILL_SWEEPKICK;
            _skillName = "sweepkick";
            _className = "SkillSweepkick";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 28, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_RANGER, 27, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_MONK, 25, 0};
            _practiceInfo.push_back(p2);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillSweepkickInfo();
        }
};

class SkillTripInfo : public SkillInfo
{
    public:
        SkillTripInfo() : SkillInfo()
        {
            _skillNum = SKILL_TRIP;
            _skillName = "trip";
            _className = "SkillTrip";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 18, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillTripInfo();
        }
};

class SkillUppercutInfo : public SkillInfo
{
    public:
        SkillUppercutInfo() : SkillInfo()
        {
            _skillNum = SKILL_UPPERCUT;
            _skillName = "uppercut";
            _className = "SkillUppercut";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 11, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_KNIGHT, 15, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 13, 0};
            _practiceInfo.push_back(p2);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillUppercutInfo();
        }
};

class SkillGroinkickInfo : public SkillInfo
{
    public:
        SkillGroinkickInfo() : SkillInfo()
        {
            _skillNum = SKILL_GROINKICK;
            _skillName = "groinkick";
            _className = "SkillGroinkick";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 12, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillGroinkickInfo();
        }
};

class SkillClawInfo : public SkillInfo
{
    public:
        SkillClawInfo() : SkillInfo()
        {
            _skillNum = SKILL_CLAW;
            _skillName = "claw";
            _className = "SkillClaw";
            _skill = true;
            PracticeInfo p0 = {CLASS_RANGER, 32, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillClawInfo();
        }
};

class SkillRabbitpunchInfo : public SkillInfo
{
    public:
        SkillRabbitpunchInfo() : SkillInfo()
        {
            _skillNum = SKILL_RABBITPUNCH;
            _skillName = "rabbitpunch";
            _className = "SkillRabbitpunch";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 10, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillRabbitpunchInfo();
        }
};

class SkillImpaleInfo : public SkillInfo
{
    public:
        SkillImpaleInfo() : SkillInfo()
        {
            _skillNum = SKILL_IMPALE;
            _skillName = "impale";
            _className = "SkillImpale";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 43, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_KNIGHT, 30, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 45, 0};
            _practiceInfo.push_back(p2);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillImpaleInfo();
        }
};

class SkillPeleKickInfo : public SkillInfo
{
    public:
        SkillPeleKickInfo() : SkillInfo()
        {
            _skillNum = SKILL_PELE_KICK;
            _skillName = "pele kick";
            _className = "SkillPeleKick";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 41, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_BARB, 40, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 41, 0};
            _practiceInfo.push_back(p2);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillPeleKickInfo();
        }
};

class SkillLungePunchInfo : public SkillInfo
{
    public:
        SkillLungePunchInfo() : SkillInfo()
        {
            _skillNum = SKILL_LUNGE_PUNCH;
            _skillName = "lunge punch";
            _className = "SkillLungePunch";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 32, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_KNIGHT, 35, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 42, 0};
            _practiceInfo.push_back(p2);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillLungePunchInfo();
        }
};

class SkillTornadoKickInfo : public SkillInfo
{
    public:
        SkillTornadoKickInfo() : SkillInfo()
        {
            _skillNum = SKILL_TORNADO_KICK;
            _skillName = "tornado kick";
            _className = "SkillTornadoKick";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 45, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillTornadoKickInfo();
        }
};

class SkillTripleAttackInfo : public SkillInfo
{
    public:
        SkillTripleAttackInfo() : SkillInfo()
        {
            _skillNum = SKILL_TRIPLE_ATTACK;
            _skillName = "triple attack";
            _className = "SkillTripleAttack";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 48, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_KNIGHT, 45, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 43, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_MONK, 41, 0};
            _practiceInfo.push_back(p3);
            PracticeInfo p4 = {CLASS_MERCENARY, 38, 0};
            _practiceInfo.push_back(p4);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillTripleAttackInfo();
        }
};

class SkillCounterAttackInfo : public SkillInfo
{
    public:
        SkillCounterAttackInfo() : SkillInfo()
        {
            _skillNum = SKILL_COUNTER_ATTACK;
            _skillName = "counter attack";
            _className = "SkillCounterAttack";
            _skill = true;
            PracticeInfo p0 = {CLASS_RANGER, 31, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillCounterAttackInfo();
        }
};

class SkillSwimmingInfo : public SkillInfo
{
    public:
        SkillSwimmingInfo() : SkillInfo()
        {
            _skillNum = SKILL_SWIMMING;
            _skillName = "swimming";
            _className = "SkillSwimming";
            _skill = true;
            PracticeInfo p0 = {CLASS_MAGE, 1, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 1, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_THIEF, 1, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_BARB, 1, 0};
            _practiceInfo.push_back(p3);
            PracticeInfo p4 = {CLASS_PSIONIC, 2, 0};
            _practiceInfo.push_back(p4);
            PracticeInfo p5 = {CLASS_PHYSIC, 1, 0};
            _practiceInfo.push_back(p5);
            PracticeInfo p6 = {CLASS_KNIGHT, 1, 0};
            _practiceInfo.push_back(p6);
            PracticeInfo p7 = {CLASS_RANGER, 1, 0};
            _practiceInfo.push_back(p7);
            PracticeInfo p8 = {CLASS_BARD, 1, 0};
            _practiceInfo.push_back(p8);
            PracticeInfo p9 = {CLASS_MONK, 7, 0};
            _practiceInfo.push_back(p9);
            PracticeInfo p10 = {CLASS_MERCENARY, 1, 0};
            _practiceInfo.push_back(p10);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillSwimmingInfo();
        }
};

class SkillThrowingInfo : public SkillInfo
{
    public:
        SkillThrowingInfo() : SkillInfo()
        {
            _skillNum = SKILL_THROWING;
            _skillName = "throwing";
            _className = "SkillThrowing";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 7, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_BARB, 15, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 9, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_BARD, 3, 0};
            _practiceInfo.push_back(p3);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillThrowingInfo();
        }
};

class SkillRidingInfo : public SkillInfo
{
    public:
        SkillRidingInfo() : SkillInfo()
        {
            _skillNum = SKILL_RIDING;
            _skillName = "riding";
            _className = "SkillRiding";
            _skill = true;
            PracticeInfo p0 = {CLASS_MAGE, 1, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 1, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_THIEF, 1, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_BARB, 1, 0};
            _practiceInfo.push_back(p3);
            PracticeInfo p4 = {CLASS_KNIGHT, 1, 0};
            _practiceInfo.push_back(p4);
            PracticeInfo p5 = {CLASS_RANGER, 1, 0};
            _practiceInfo.push_back(p5);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillRidingInfo();
        }
};

class SkillAppraiseInfo : public SkillInfo
{
    public:
        SkillAppraiseInfo() : SkillInfo()
        {
            _skillNum = SKILL_APPRAISE;
            _skillName = "appraise";
            _className = "SkillAppraise";
            _skill = true;
            PracticeInfo p0 = {CLASS_MAGE, 10, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 10, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_THIEF, 10, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_BARB, 10, 0};
            _practiceInfo.push_back(p3);
            PracticeInfo p4 = {CLASS_PSIONIC, 10, 0};
            _practiceInfo.push_back(p4);
            PracticeInfo p5 = {CLASS_PHYSIC, 10, 0};
            _practiceInfo.push_back(p5);
            PracticeInfo p6 = {CLASS_CYBORG, 10, 0};
            _practiceInfo.push_back(p6);
            PracticeInfo p7 = {CLASS_KNIGHT, 10, 0};
            _practiceInfo.push_back(p7);
            PracticeInfo p8 = {CLASS_RANGER, 10, 0};
            _practiceInfo.push_back(p8);
            PracticeInfo p9 = {CLASS_MONK, 10, 0};
            _practiceInfo.push_back(p9);
            PracticeInfo p10 = {CLASS_MERCENARY, 10, 0};
            _practiceInfo.push_back(p10);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillAppraiseInfo();
        }
};

class SkillPipemakingInfo : public SkillInfo
{
    public:
        SkillPipemakingInfo() : SkillInfo()
        {
            _skillNum = SKILL_PIPEMAKING;
            _skillName = "pipemaking";
            _className = "SkillPipemaking";
            _skill = true;
            PracticeInfo p0 = {CLASS_MAGE, 40, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_RANGER, 15, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_BARD, 13, 0};
            _practiceInfo.push_back(p2);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillPipemakingInfo();
        }
};

class SkillSecondWeaponInfo : public SkillInfo
{
    public:
        SkillSecondWeaponInfo() : SkillInfo()
        {
            _skillNum = SKILL_SECOND_WEAPON;
            _skillName = "second weapon";
            _className = "SkillSecondWeapon";
            _skill = true;
            PracticeInfo p0 = {CLASS_RANGER, 14, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_MERCENARY, 18, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillSecondWeaponInfo();
        }
};

class SkillScanningInfo : public SkillInfo
{
    public:
        SkillScanningInfo() : SkillInfo()
        {
            _skillNum = SKILL_SCANNING;
            _skillName = "scanning";
            _className = "SkillScanning";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 31, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_RANGER, 24, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_MERCENARY, 22, 0};
            _practiceInfo.push_back(p2);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillScanningInfo();
        }
};

class SkillRetreatInfo : public SkillInfo
{
    public:
        SkillRetreatInfo() : SkillInfo()
        {
            _skillNum = SKILL_RETREAT;
            _skillName = "retreat";
            _className = "SkillRetreat";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 16, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_RANGER, 18, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_BARD, 17, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_MONK, 33, 0};
            _practiceInfo.push_back(p3);
            PracticeInfo p4 = {CLASS_MERCENARY, 22, 0};
            _practiceInfo.push_back(p4);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillRetreatInfo();
        }
};

class SkillGunsmithingInfo : public SkillInfo
{
    public:
        SkillGunsmithingInfo() : SkillInfo()
        {
            _skillNum = SKILL_GUNSMITHING;
            _skillName = "gunsmithing";
            _className = "SkillGunsmithing";
            _skill = true;
            PracticeInfo p0 = {CLASS_MERCENARY, 20, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillGunsmithingInfo();
        }
};

class SkillPistolwhipInfo : public SkillInfo
{
    public:
        SkillPistolwhipInfo() : SkillInfo()
        {
            _skillNum = SKILL_PISTOLWHIP;
            _skillName = "pistolwhip";
            _className = "SkillPistolwhip";
            _skill = true;
            PracticeInfo p0 = {CLASS_MERCENARY, 20, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillPistolwhipInfo();
        }
};

class SkillCrossfaceInfo : public SkillInfo
{
    public:
        SkillCrossfaceInfo() : SkillInfo()
        {
            _skillNum = SKILL_CROSSFACE;
            _skillName = "crossface";
            _className = "SkillCrossface";
            _skill = true;
            PracticeInfo p0 = {CLASS_MERCENARY, 26, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillCrossfaceInfo();
        }
};

class SkillWrenchInfo : public SkillInfo
{
    public:
        SkillWrenchInfo() : SkillInfo()
        {
            _skillNum = SKILL_WRENCH;
            _skillName = "wrench";
            _className = "SkillWrench";
            _skill = true;
            PracticeInfo p0 = {CLASS_MERCENARY, 6, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillWrenchInfo();
        }
};

class SkillElusionInfo : public SkillInfo
{
    public:
        SkillElusionInfo() : SkillInfo()
        {
            _skillNum = SKILL_ELUSION;
            _skillName = "elusion";
            _className = "SkillElusion";
            _skill = true;
            PracticeInfo p0 = {CLASS_MERCENARY, 11, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillElusionInfo();
        }
};

class SkillInfiltrateInfo : public SkillInfo
{
    public:
        SkillInfiltrateInfo() : SkillInfo()
        {
            _skillNum = SKILL_INFILTRATE;
            _skillName = "infiltrate";
            _className = "SkillInfiltrate";
            _skill = true;
            PracticeInfo p0 = {CLASS_MERCENARY, 27, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillInfiltrateInfo();
        }
};

class SkillShoulderThrowInfo : public SkillInfo
{
    public:
        SkillShoulderThrowInfo() : SkillInfo()
        {
            _skillNum = SKILL_SHOULDER_THROW;
            _skillName = "shoulder throw";
            _className = "SkillShoulderThrow";
            _skill = true;
            PracticeInfo p0 = {CLASS_MERCENARY, 39, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillShoulderThrowInfo();
        }
};

class SkillProfPoundInfo : public SkillInfo
{
    public:
        SkillProfPoundInfo() : SkillInfo()
        {
            _skillNum = SKILL_PROF_POUND;
            _skillName = "prof pound";
            _className = "SkillProfPound";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 3, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillProfPoundInfo();
        }
};

class SkillProfWhipInfo : public SkillInfo
{
    public:
        SkillProfWhipInfo() : SkillInfo()
        {
            _skillNum = SKILL_PROF_WHIP;
            _skillName = "prof whip";
            _className = "SkillProfWhip";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 11, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillProfWhipInfo();
        }
};

class SkillProfPierceInfo : public SkillInfo
{
    public:
        SkillProfPierceInfo() : SkillInfo()
        {
            _skillNum = SKILL_PROF_PIERCE;
            _skillName = "prof pierce";
            _className = "SkillProfPierce";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 18, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillProfPierceInfo();
        }
};

class SkillProfSlashInfo : public SkillInfo
{
    public:
        SkillProfSlashInfo() : SkillInfo()
        {
            _skillNum = SKILL_PROF_SLASH;
            _skillName = "prof slash";
            _className = "SkillProfSlash";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 26, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillProfSlashInfo();
        }
};

class SkillProfCrushInfo : public SkillInfo
{
    public:
        SkillProfCrushInfo() : SkillInfo()
        {
            _skillNum = SKILL_PROF_CRUSH;
            _skillName = "prof crush";
            _className = "SkillProfCrush";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 34, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillProfCrushInfo();
        }
};

class SkillProfBlastInfo : public SkillInfo
{
    public:
        SkillProfBlastInfo() : SkillInfo()
        {
            _skillNum = SKILL_PROF_BLAST;
            _skillName = "prof blast";
            _className = "SkillProfBlast";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 42, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillProfBlastInfo();
        }
};

class SkillGarotteInfo : public SkillInfo
{
    public:
        SkillGarotteInfo() : SkillInfo()
        {
            _skillNum = SKILL_GAROTTE;
            _skillName = "garotte";
            _className = "SkillGarotte";
            _skill = true;
            _violent = true;
            PracticeInfo p0 = {CLASS_MERCENARY, 21, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillGarotteInfo();
        }
};

class SkillDemolitionsInfo : public SkillInfo
{
    public:
        SkillDemolitionsInfo() : SkillInfo()
        {
            _skillNum = SKILL_DEMOLITIONS;
            _skillName = "demolitions";
            _className = "SkillDemolitions";
            _skill = true;
            PracticeInfo p0 = {CLASS_MERCENARY, 19, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillDemolitionsInfo();
        }
};

class SkillReconfigureInfo : public SkillInfo
{
    public:
        SkillReconfigureInfo() : SkillInfo()
        {
            _skillNum = SKILL_RECONFIGURE;
            _skillName = "reconfigure";
            _className = "SkillReconfigure";
            _skill = true;
            PracticeInfo p0 = {CLASS_CYBORG, 11, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillReconfigureInfo();
        }
};

class SkillRebootInfo : public SkillInfo
{
    public:
        SkillRebootInfo() : SkillInfo()
        {
            _skillNum = SKILL_REBOOT;
            _skillName = "reboot";
            _className = "SkillReboot";
            _skill = true;
            PracticeInfo p0 = {CLASS_CYBORG, 1, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillRebootInfo();
        }
};

class SkillMotionSensorInfo : public SkillInfo
{
    public:
        SkillMotionSensorInfo() : SkillInfo()
        {
            _skillNum = SKILL_MOTION_SENSOR;
            _skillName = "motion sensor";
            _className = "SkillMotionSensor";
            _program = true;
            PracticeInfo p0 = {CLASS_CYBORG, 16, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillMotionSensorInfo();
        }
};

class SkillStasisInfo : public SkillInfo
{
    public:
        SkillStasisInfo() : SkillInfo()
        {
            _skillNum = SKILL_STASIS;
            _skillName = "stasis";
            _className = "SkillStasis";
            _program = true;
            PracticeInfo p0 = {CLASS_CYBORG, 10, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillStasisInfo();
        }
};

class SkillEnergyFieldInfo : public SkillInfo
{
    public:
        SkillEnergyFieldInfo() : SkillInfo()
        {
            _skillNum = SKILL_ENERGY_FIELD;
            _skillName = "energy field";
            _className = "SkillEnergyField";
            _program = true;
            PracticeInfo p0 = {CLASS_CYBORG, 32, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillEnergyFieldInfo();
        }
};

class SkillReflexBoostInfo : public SkillInfo
{
    public:
        SkillReflexBoostInfo() : SkillInfo()
        {
            _skillNum = SKILL_REFLEX_BOOST;
            _skillName = "reflex boost";
            _className = "SkillReflexBoost";
            _program = true;
            PracticeInfo p0 = {CLASS_CYBORG, 18, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillReflexBoostInfo();
        }
};

class SkillImplantWInfo : public SkillInfo
{
    public:
        SkillImplantWInfo() : SkillInfo()
        {
            _skillNum = SKILL_IMPLANT_W;
            _skillName = "implant w";
            _className = "SkillImplantW";
            _skill = true;
            PracticeInfo p0 = {CLASS_CYBORG, 26, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillImplantWInfo();
        }
};

class SkillOffensivePosInfo : public SkillInfo
{
    public:
        SkillOffensivePosInfo() : SkillInfo()
        {
            _skillNum = SKILL_OFFENSIVE_POS;
            _skillName = "offensive pos";
            _className = "SkillOffensivePos";
            _program = true;
            PracticeInfo p0 = {CLASS_CYBORG, 15, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillOffensivePosInfo();
        }
};

class SkillDefensivePosInfo : public SkillInfo
{
    public:
        SkillDefensivePosInfo() : SkillInfo()
        {
            _skillNum = SKILL_DEFENSIVE_POS;
            _skillName = "defensive pos";
            _className = "SkillDefensivePos";
            _program = true;
            PracticeInfo p0 = {CLASS_CYBORG, 15, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillDefensivePosInfo();
        }
};

class SkillMeleeCombatTacInfo : public SkillInfo
{
    public:
        SkillMeleeCombatTacInfo() : SkillInfo()
        {
            _skillNum = SKILL_MELEE_COMBAT_TAC;
            _skillName = "melee combat tac";
            _className = "SkillMeleeCombatTac";
            _program = true;
            PracticeInfo p0 = {CLASS_CYBORG, 24, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillMeleeCombatTacInfo();
        }
};

class SkillNeuralBridgingInfo : public SkillInfo
{
    public:
        SkillNeuralBridgingInfo() : SkillInfo()
        {
            _skillNum = SKILL_NEURAL_BRIDGING;
            _skillName = "neural bridging";
            _className = "SkillNeuralBridging";
            _program = true;
            PracticeInfo p0 = {CLASS_CYBORG, 35, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillNeuralBridgingInfo();
        }
};

class SkillNaniteReconstructionInfo : public SkillInfo
{
    public:
        SkillNaniteReconstructionInfo() : SkillInfo()
        {
            _skillNum = SKILL_NANITE_RECONSTRUCTION;
            _skillName = "nanite reconstruction";
            _className = "SkillNaniteReconstruction";
            _program = true;
            PracticeInfo p0 = {CLASS_CYBORG, 33, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillNaniteReconstructionInfo();
        }
};

class SkillOptimmunalRespInfo : public SkillInfo
{
    public:
        SkillOptimmunalRespInfo() : SkillInfo()
        {
            _skillNum = SKILL_OPTIMMUNAL_RESP;
            _skillName = "optimmunal resp";
            _className = "SkillOptimmunalResp";
            _program = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillOptimmunalRespInfo();
        }
};

class SkillAdrenalMaximizerInfo : public SkillInfo
{
    public:
        SkillAdrenalMaximizerInfo() : SkillInfo()
        {
            _skillNum = SKILL_ADRENAL_MAXIMIZER;
            _skillName = "adrenal maximizer";
            _className = "SkillAdrenalMaximizer";
            _program = true;
            PracticeInfo p0 = {CLASS_CYBORG, 37, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillAdrenalMaximizerInfo();
        }
};

class SkillPowerBoostInfo : public SkillInfo
{
    public:
        SkillPowerBoostInfo() : SkillInfo()
        {
            _skillNum = SKILL_POWER_BOOST;
            _skillName = "power boost";
            _className = "SkillPowerBoost";
            _program = true;
            PracticeInfo p0 = {CLASS_CYBORG, 18, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillPowerBoostInfo();
        }
};

class SkillFastbootInfo : public SkillInfo
{
    public:
        SkillFastbootInfo() : SkillInfo()
        {
            _skillNum = SKILL_FASTBOOT;
            _skillName = "fastboot";
            _className = "SkillFastboot";
            _skill = true;
            PracticeInfo p0 = {CLASS_CYBORG, 12, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillFastbootInfo();
        }
};

class SkillSelfDestructInfo : public SkillInfo
{
    public:
        SkillSelfDestructInfo() : SkillInfo()
        {
            _skillNum = SKILL_SELF_DESTRUCT;
            _skillName = "self destruct";
            _className = "SkillSelfDestruct";
            _skill = true;
            PracticeInfo p0 = {CLASS_CYBORG, 12, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillSelfDestructInfo();
        }
};

class SkillBioscanInfo : public SkillInfo
{
    public:
        SkillBioscanInfo() : SkillInfo()
        {
            _skillNum = SKILL_BIOSCAN;
            _skillName = "bioscan";
            _className = "SkillBioscan";
            _skill = true;
            PracticeInfo p0 = {CLASS_CYBORG, 12, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillBioscanInfo();
        }
};

class SkillDischargeInfo : public SkillInfo
{
    public:
        SkillDischargeInfo() : SkillInfo()
        {
            _skillNum = SKILL_DISCHARGE;
            _skillName = "discharge";
            _className = "SkillDischarge";
            _skill = true;
            PracticeInfo p0 = {CLASS_CYBORG, 14, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillDischargeInfo();
        }
};

class SkillSelfrepairInfo : public SkillInfo
{
    public:
        SkillSelfrepairInfo() : SkillInfo()
        {
            _skillNum = SKILL_SELFREPAIR;
            _skillName = "selfrepair";
            _className = "SkillSelfrepair";
            _skill = true;
            PracticeInfo p0 = {CLASS_CYBORG, 20, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillSelfrepairInfo();
        }
};

class SkillCyborepairInfo : public SkillInfo
{
    public:
        SkillCyborepairInfo() : SkillInfo()
        {
            _skillNum = SKILL_CYBOREPAIR;
            _skillName = "cyborepair";
            _className = "SkillCyborepair";
            _skill = true;
            PracticeInfo p0 = {CLASS_CYBORG, 30, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillCyborepairInfo();
        }
};

class SkillOverhaulInfo : public SkillInfo
{
    public:
        SkillOverhaulInfo() : SkillInfo()
        {
            _skillNum = SKILL_OVERHAUL;
            _skillName = "overhaul";
            _className = "SkillOverhaul";
            _skill = true;
            PracticeInfo p0 = {CLASS_CYBORG, 45, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillOverhaulInfo();
        }
};

class SkillDamageControlInfo : public SkillInfo
{
    public:
        SkillDamageControlInfo() : SkillInfo()
        {
            _skillNum = SKILL_DAMAGE_CONTROL;
            _skillName = "damage control";
            _className = "SkillDamageControl";
            _program = true;
            PracticeInfo p0 = {CLASS_CYBORG, 12, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillDamageControlInfo();
        }
};

class SkillElectronicsInfo : public SkillInfo
{
    public:
        SkillElectronicsInfo() : SkillInfo()
        {
            _skillNum = SKILL_ELECTRONICS;
            _skillName = "electronics";
            _className = "SkillElectronics";
            _skill = true;
            PracticeInfo p0 = {CLASS_PHYSIC, 13, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CYBORG, 6, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillElectronicsInfo();
        }
};

class SkillHackingInfo : public SkillInfo
{
    public:
        SkillHackingInfo() : SkillInfo()
        {
            _skillNum = SKILL_HACKING;
            _skillName = "hacking";
            _className = "SkillHacking";
            _skill = true;
            PracticeInfo p0 = {CLASS_CYBORG, 20, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillHackingInfo();
        }
};

class SkillCyberscanInfo : public SkillInfo
{
    public:
        SkillCyberscanInfo() : SkillInfo()
        {
            _skillNum = SKILL_CYBERSCAN;
            _skillName = "cyberscan";
            _className = "SkillCyberscan";
            _skill = true;
            PracticeInfo p0 = {CLASS_CYBORG, 30, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillCyberscanInfo();
        }
};

class SkillCyboSurgeryInfo : public SkillInfo
{
    public:
        SkillCyboSurgeryInfo() : SkillInfo()
        {
            _skillNum = SKILL_CYBO_SURGERY;
            _skillName = "cybo surgery";
            _className = "SkillCyboSurgery";
            _skill = true;
            PracticeInfo p0 = {CLASS_CYBORG, 45, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillCyboSurgeryInfo();
        }
};

class SkillHyperscanningInfo : public SkillInfo
{
    public:
        SkillHyperscanningInfo() : SkillInfo()
        {
            _skillNum = SKILL_HYPERSCAN;
            _skillName = "hyperscan";
            _className = "SkillHyperscanning";
            _program = true;
            PracticeInfo p0 = {CLASS_CYBORG, 25, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillHyperscanningInfo();
        }
};

class SkillOverdrainInfo : public SkillInfo
{
    public:
        SkillOverdrainInfo() : SkillInfo()
        {
            _skillNum = SKILL_OVERDRAIN;
            _skillName = "overdrain";
            _className = "SkillOverdrain";
            _skill = true;
            PracticeInfo p0 = {CLASS_CYBORG, 8, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillOverdrainInfo();
        }
};

class SkillEnergyWeaponsInfo : public SkillInfo
{
    public:
        SkillEnergyWeaponsInfo() : SkillInfo()
        {
            _skillNum = SKILL_ENERGY_WEAPONS;
            _skillName = "energy weapons";
            _className = "SkillEnergyWeapons";
            _skill = true;
            PracticeInfo p0 = {CLASS_CYBORG, 20, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_MERCENARY, 7, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillEnergyWeaponsInfo();
        }
};

class SkillProjWeaponsInfo : public SkillInfo
{
    public:
        SkillProjWeaponsInfo() : SkillInfo()
        {
            _skillNum = SKILL_PROJ_WEAPONS;
            _skillName = "proj weapons";
            _className = "SkillProjWeapons";
            _skill = true;
            PracticeInfo p0 = {CLASS_CYBORG, 20, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_MERCENARY, 8, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillProjWeaponsInfo();
        }
};

class SkillSpeedLoadingInfo : public SkillInfo
{
    public:
        SkillSpeedLoadingInfo() : SkillInfo()
        {
            _skillNum = SKILL_SPEED_LOADING;
            _skillName = "speed loading";
            _className = "SkillSpeedLoading";
            _skill = true;
            PracticeInfo p0 = {CLASS_CYBORG, 25, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_MERCENARY, 15, 0};
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillSpeedLoadingInfo();
        }
};

class SkillPalmStrikeInfo : public SkillInfo
{
    public:
        SkillPalmStrikeInfo() : SkillInfo()
        {
            _skillNum = SKILL_PALM_STRIKE;
            _skillName = "palm strike";
            _className = "SkillPalmStrike";
            _skill = true;
            PracticeInfo p0 = {CLASS_MONK, 4, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillPalmStrikeInfo();
        }
};

class SkillThroatStrikeInfo : public SkillInfo
{
    public:
        SkillThroatStrikeInfo() : SkillInfo()
        {
            _skillNum = SKILL_THROAT_STRIKE;
            _skillName = "throat strike";
            _className = "SkillThroatStrike";
            _skill = true;
            PracticeInfo p0 = {CLASS_MONK, 10, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillThroatStrikeInfo();
        }
};

class SkillWhirlwindInfo : public SkillInfo
{
    public:
        SkillWhirlwindInfo() : SkillInfo()
        {
            _skillNum = SKILL_WHIRLWIND;
            _skillName = "whirlwind";
            _className = "SkillWhirlwind";
            _skill = true;
            PracticeInfo p0 = {CLASS_MONK, 40, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillWhirlwindInfo();
        }
};

class SkillHipTossInfo : public SkillInfo
{
    public:
        SkillHipTossInfo() : SkillInfo()
        {
            _skillNum = SKILL_HIP_TOSS;
            _skillName = "hip toss";
            _className = "SkillHipToss";
            _skill = true;
            PracticeInfo p0 = {CLASS_MONK, 27, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillHipTossInfo();
        }
};

class SkillComboInfo : public SkillInfo
{
    public:
        SkillComboInfo() : SkillInfo()
        {
            _skillNum = SKILL_COMBO;
            _skillName = "combo";
            _className = "SkillCombo";
            _skill = true;
            PracticeInfo p0 = {CLASS_MONK, 33, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillComboInfo();
        }
};

class SkillDeathTouchInfo : public SkillInfo
{
    public:
        SkillDeathTouchInfo() : SkillInfo()
        {
            _skillNum = SKILL_DEATH_TOUCH;
            _skillName = "death touch";
            _className = "SkillDeathTouch";
            _skill = true;
            PracticeInfo p0 = {CLASS_MONK, 49, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillDeathTouchInfo();
        }
};

class SkillCraneKickInfo : public SkillInfo
{
    public:
        SkillCraneKickInfo() : SkillInfo()
        {
            _skillNum = SKILL_CRANE_KICK;
            _skillName = "crane kick";
            _className = "SkillCraneKick";
            _skill = true;
            PracticeInfo p0 = {CLASS_MONK, 15, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillCraneKickInfo();
        }
};

class SkillRidgehandInfo : public SkillInfo
{
    public:
        SkillRidgehandInfo() : SkillInfo()
        {
            _skillNum = SKILL_RIDGEHAND;
            _skillName = "ridgehand";
            _className = "SkillRidgehand";
            _skill = true;
            PracticeInfo p0 = {CLASS_MONK, 30, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillRidgehandInfo();
        }
};

class SkillScissorKickInfo : public SkillInfo
{
    public:
        SkillScissorKickInfo() : SkillInfo()
        {
            _skillNum = SKILL_SCISSOR_KICK;
            _skillName = "scissor kick";
            _className = "SkillScissorKick";
            _skill = true;
            PracticeInfo p0 = {CLASS_MONK, 20, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillScissorKickInfo();
        }
};

class SkillPinchAlphaInfo : public SkillInfo
{
    public:
        SkillPinchAlphaInfo() : SkillInfo()
        {
            _skillNum = SKILL_PINCH_ALPHA;
            _skillName = "pinch alpha";
            _className = "SkillPinchAlpha";
            _skill = true;
            PracticeInfo p0 = {CLASS_MONK, 6, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillPinchAlphaInfo();
        }
};

class SkillPinchBetaInfo : public SkillInfo
{
    public:
        SkillPinchBetaInfo() : SkillInfo()
        {
            _skillNum = SKILL_PINCH_BETA;
            _skillName = "pinch beta";
            _className = "SkillPinchBeta";
            _skill = true;
            PracticeInfo p0 = {CLASS_MONK, 12, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillPinchBetaInfo();
        }
};

class SkillPinchGammaInfo : public SkillInfo
{
    public:
        SkillPinchGammaInfo() : SkillInfo()
        {
            _skillNum = SKILL_PINCH_GAMMA;
            _skillName = "pinch gamma";
            _className = "SkillPinchGamma";
            _skill = true;
            PracticeInfo p0 = {CLASS_MONK, 35, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillPinchGammaInfo();
        }
};

class SkillPinchDeltaInfo : public SkillInfo
{
    public:
        SkillPinchDeltaInfo() : SkillInfo()
        {
            _skillNum = SKILL_PINCH_DELTA;
            _skillName = "pinch delta";
            _className = "SkillPinchDelta";
            _skill = true;
            PracticeInfo p0 = {CLASS_MONK, 19, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillPinchDeltaInfo();
        }
};

class SkillPinchEpsilonInfo : public SkillInfo
{
    public:
        SkillPinchEpsilonInfo() : SkillInfo()
        {
            _skillNum = SKILL_PINCH_EPSILON;
            _skillName = "pinch epsilon";
            _className = "SkillPinchEpsilon";
            _skill = true;
            PracticeInfo p0 = {CLASS_MONK, 26, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillPinchEpsilonInfo();
        }
};

class SkillPinchOmegaInfo : public SkillInfo
{
    public:
        SkillPinchOmegaInfo() : SkillInfo()
        {
            _skillNum = SKILL_PINCH_OMEGA;
            _skillName = "pinch omega";
            _className = "SkillPinchOmega";
            _skill = true;
            PracticeInfo p0 = {CLASS_MONK, 39, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillPinchOmegaInfo();
        }
};

class SkillPinchZetaInfo : public SkillInfo
{
    public:
        SkillPinchZetaInfo() : SkillInfo()
        {
            _skillNum = SKILL_PINCH_ZETA;
            _skillName = "pinch zeta";
            _className = "SkillPinchZeta";
            _skill = true;
            PracticeInfo p0 = {CLASS_MONK, 32, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillPinchZetaInfo();
        }
};

class SkillMeditateInfo : public SkillInfo
{
    public:
        SkillMeditateInfo() : SkillInfo()
        {
            _skillNum = SKILL_MEDITATE;
            _skillName = "meditate";
            _className = "SkillMeditate";
            _skill = true;
            PracticeInfo p0 = {CLASS_MONK, 9, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillMeditateInfo();
        }
};

class SkillKataInfo : public SkillInfo
{
    public:
        SkillKataInfo() : SkillInfo()
        {
            _skillNum = SKILL_KATA;
            _skillName = "kata";
            _className = "SkillKata";
            _skill = true;
            PracticeInfo p0 = {CLASS_MONK, 5, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillKataInfo();
        }
};

class SkillEvasionInfo : public SkillInfo
{
    public:
        SkillEvasionInfo() : SkillInfo()
        {
            _skillNum = SKILL_EVASION;
            _skillName = "evasion";
            _className = "SkillEvasion";
            _skill = true;
            PracticeInfo p0 = {CLASS_MONK, 22, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillEvasionInfo();
        }
};

class SkillFlyingInfo : public SkillInfo
{
    public:
        SkillFlyingInfo() : SkillInfo()
        {
            _skillNum = SKILL_FLYING;
            _skillName = "flying";
            _className = "SkillFlying";
            _skill = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillFlyingInfo();
        }
};

class SkillSummonInfo : public SkillInfo
{
    public:
        SkillSummonInfo() : SkillInfo()
        {
            _skillNum = SKILL_SUMMON;
            _skillName = "summon";
            _className = "SkillSummon";
            _skill = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillSummonInfo();
        }
};

class SkillFeedInfo : public SkillInfo
{
    public:
        SkillFeedInfo() : SkillInfo()
        {
            _skillNum = SKILL_FEED;
            _skillName = "feed";
            _className = "SkillFeed";
            _skill = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillFeedInfo();
        }
};

class SkillBeguileInfo : public SkillInfo
{
    public:
        SkillBeguileInfo() : SkillInfo()
        {
            _skillNum = SKILL_BEGUILE;
            _skillName = "beguile";
            _className = "SkillBeguile";
            _skill = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillBeguileInfo();
        }
};

class SkillDrainInfo : public SkillInfo
{
    public:
        SkillDrainInfo() : SkillInfo()
        {
            _skillNum = SKILL_DRAIN;
            _skillName = "drain";
            _className = "SkillDrain";
            _skill = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillDrainInfo();
        }
};

class SkillCreateVampireInfo : public SkillInfo
{
    public:
        SkillCreateVampireInfo() : SkillInfo()
        {
            _skillNum = SKILL_CREATE_VAMPIRE;
            _skillName = "create vampire";
            _className = "SkillCreateVampire";
            _skill = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillCreateVampireInfo();
        }
};

class SkillControlUndeadInfo : public SkillInfo
{
    public:
        SkillControlUndeadInfo() : SkillInfo()
        {
            _skillNum = SKILL_CONTROL_UNDEAD;
            _skillName = "control undead";
            _className = "SkillControlUndead";
            _skill = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillControlUndeadInfo();
        }
};

class SkillTerrorizeInfo : public SkillInfo
{
    public:
        SkillTerrorizeInfo() : SkillInfo()
        {
            _skillNum = SKILL_TERRORIZE;
            _skillName = "terrorize";
            _className = "SkillTerrorize";
            _skill = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillTerrorizeInfo();
        }
};

class SkillLectureInfo : public SkillInfo
{
    public:
        SkillLectureInfo() : SkillInfo()
        {
            _skillNum = SKILL_LECTURE;
            _skillName = "lecture";
            _className = "SkillLecture";
            _skill = true;
            PracticeInfo p0 = {CLASS_PHYSIC, 30, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillLectureInfo();
        }
};

class SkillEnergyConversionInfo : public SkillInfo
{
    public:
        SkillEnergyConversionInfo() : SkillInfo()
        {
            _skillNum = SKILL_ENERGY_CONVERSION;
            _skillName = "energy conversion";
            _className = "SkillEnergyConversion";
            _skill = true;
            PracticeInfo p0 = {CLASS_PHYSIC, 22, 0};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillEnergyConversionInfo();
        }
};

class SpellHellFireInfo : public SkillInfo
{
    public:
        SpellHellFireInfo() : SkillInfo()
        {
            _skillNum = SPELL_HELL_FIRE;
            _skillName = "hell fire";
            _className = "SpellHellFire";
            _skill = true;
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _violent = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellHellFireInfo();
        }
};

class SpellHellFrostInfo : public SkillInfo
{
    public:
        SpellHellFrostInfo() : SkillInfo()
        {
            _skillNum = SPELL_HELL_FROST;
            _skillName = "hell frost";
            _className = "SpellHellFrost";
            _skill = true;
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _violent = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellHellFrostInfo();
        }
};

class SpellHellFireStormInfo : public SkillInfo
{
    public:
        SpellHellFireStormInfo() : SkillInfo()
        {
            _skillNum = SPELL_HELL_FIRE_STORM;
            _skillName = "hell fire storm";
            _className = "SpellHellFireStorm";
            _skill = true;
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _violent = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellHellFireStormInfo();
        }
};

class SpellHellFrostStormInfo : public SkillInfo
{
    public:
        SpellHellFrostStormInfo() : SkillInfo()
        {
            _skillNum = SPELL_HELL_FROST_STORM;
            _skillName = "hell frost storm";
            _className = "SpellHellFrostStorm";
            _skill = true;
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _violent = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellHellFrostStormInfo();
        }
};

class SpellTrogStenchInfo : public SkillInfo
{
    public:
        SpellTrogStenchInfo() : SkillInfo()
        {
            _skillNum = SPELL_TROG_STENCH;
            _skillName = "trog stench";
            _className = "SpellTrogStench";
            _skill = true;
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _violent = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellTrogStenchInfo();
        }
};

class SpellFrostBreathInfo : public SkillInfo
{
    public:
        SpellFrostBreathInfo() : SkillInfo()
        {
            _skillNum = SPELL_FROST_BREATH;
            _skillName = "frost breath";
            _className = "SpellFrostBreath";
            _skill = true;
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _violent = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellFrostBreathInfo();
        }
};

class SpellFireBreathInfo : public SkillInfo
{
    public:
        SpellFireBreathInfo() : SkillInfo()
        {
            _skillNum = SPELL_FIRE_BREATH;
            _skillName = "fire breath";
            _className = "SpellFireBreath";
            _skill = true;
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _violent = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellFireBreathInfo();
        }
};

class SpellGasBreathInfo : public SkillInfo
{
    public:
        SpellGasBreathInfo() : SkillInfo()
        {
            _skillNum = SPELL_GAS_BREATH;
            _skillName = "gas breath";
            _className = "SpellGasBreath";
            _skill = true;
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _violent = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellGasBreathInfo();
        }
};

class SpellAcidBreathInfo : public SkillInfo
{
    public:
        SpellAcidBreathInfo() : SkillInfo()
        {
            _skillNum = SPELL_ACID_BREATH;
            _skillName = "acid breath";
            _className = "SpellAcidBreath";
            _skill = true;
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _violent = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellAcidBreathInfo();
        }
};

class SpellLightningBreathInfo : public SkillInfo
{
    public:
        SpellLightningBreathInfo() : SkillInfo()
        {
            _skillNum = SPELL_LIGHTNING_BREATH;
            _skillName = "lightning breath";
            _className = "SpellLightningBreath";
            _skill = true;
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _violent = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellLightningBreathInfo();
        }
};

class SpellManaRestoreInfo : public SkillInfo
{
    public:
        SpellManaRestoreInfo() : SkillInfo()
        {
            _skillNum = SPELL_MANA_RESTORE;
            _skillName = "mana restore";
            _className = "SpellManaRestore";
            _skill = true;
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellManaRestoreInfo();
        }
};

class SpellPetrifyInfo : public SkillInfo
{
    public:
        SpellPetrifyInfo() : SkillInfo()
        {
            _skillNum = SPELL_PETRIFY;
            _skillName = "petrify";
            _className = "SpellPetrify";
            _skill = true;
            _minMana = 215;
            _maxMana = 330;
            _manaChange = 21;
            _violent = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellPetrifyInfo();
        }
};

class SpellSicknessInfo : public SkillInfo
{
    public:
        SpellSicknessInfo() : SkillInfo()
        {
            _skillNum = SPELL_SICKNESS;
            _skillName = "sickness";
            _className = "SpellSickness";
            _skill = true;
            _minMana = 215;
            _maxMana = 330;
            _manaChange = 21;
            _violent = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellSicknessInfo();
        }
};

class SpellEssenceOfEvilInfo : public SkillInfo
{
    public:
        SpellEssenceOfEvilInfo() : SkillInfo()
        {
            _skillNum = SPELL_ESSENCE_OF_EVIL;
            _skillName = "essence of evil";
            _className = "SpellEssenceOfEvil";
            _skill = true;
            _minMana = 215;
            _maxMana = 330;
            _manaChange = 21;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellEssenceOfEvilInfo();
        }
};

class SpellEssenceOfGoodInfo : public SkillInfo
{
    public:
        SpellEssenceOfGoodInfo() : SkillInfo()
        {
            _skillNum = SPELL_ESSENCE_OF_GOOD;
            _skillName = "essence of good";
            _className = "SpellEssenceOfGood";
            _skill = true;
            _minMana = 215;
            _maxMana = 330;
            _manaChange = 21;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellEssenceOfGoodInfo();
        }
};

class SpellShadowBreathInfo : public SkillInfo
{
    public:
        SpellShadowBreathInfo() : SkillInfo()
        {
            _skillNum = SPELL_SHADOW_BREATH;
            _skillName = "shadow breath";
            _className = "SpellShadowBreath";
            _skill = true;
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _violent = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellShadowBreathInfo();
        }
};

class SpellSteamBreathInfo : public SkillInfo
{
    public:
        SpellSteamBreathInfo() : SkillInfo()
        {
            _skillNum = SPELL_STEAM_BREATH;
            _skillName = "steam breath";
            _className = "SpellSteamBreath";
            _skill = true;
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _violent = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellSteamBreathInfo();
        }
};

class SpellQuadDamageInfo : public SkillInfo
{
    public:
        SpellQuadDamageInfo() : SkillInfo()
        {
            _skillNum = SPELL_QUAD_DAMAGE;
            _skillName = "quad damage";
            _className = "SpellQuadDamage";
            _skill = true;
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellQuadDamageInfo();
        }
};

class SpellZhengisFistOfAnnihilationInfo : public SkillInfo
{
    public:
        SpellZhengisFistOfAnnihilationInfo() : SkillInfo()
        {
            _skillNum = SPELL_ZHENGIS_FIST_OF_ANNIHILATION;
            _skillName = "zhengis fist of annihilation";
            _className = "SpellZhengisFistOfAnnihilation";
            _skill = true;
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _violent = true;
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellZhengisFistOfAnnihilationInfo();
        }
};

class SkillSnipeInfo : public SkillInfo
{
    public:
        SkillSnipeInfo() : SkillInfo()
        {
            _skillNum = SKILL_SNIPE;
            _skillName = "snipe";
            _className = "SkillSnipe";
            _skill = true;
            PracticeInfo p0 = {CLASS_MERCENARY, 37, 2};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillSnipeInfo();
        }
};

class SkillAdvImplantWInfo : public SkillInfo
{
    public:
        SkillAdvImplantWInfo() : SkillInfo()
        {
            _skillNum = SKILL_ADV_IMPLANT_W;
            _skillName = "adv implant w";
            _className = "SkillAdvImplantW";
            _skill = true;
            PracticeInfo p0 = {CLASS_CYBORG, 28, 5};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillAdvImplantWInfo();
        }
};

class SongAriaOfAsylumInfo : public SkillInfo
{
    public:
        SongAriaOfAsylumInfo() : SkillInfo()
        {
            _skillNum = SONG_ARIA_OF_ASYLUM;
            _skillName = "aria of asylum";
            _className = "SongAriaOfAsylum";
            _song = true;
            _minMana = 70;
            _maxMana = 95;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_BARD, 35, 3};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongAriaOfAsylumInfo();
        }
};

class SongLamentOfLongingInfo : public SkillInfo
{
    public:
        SongLamentOfLongingInfo() : SkillInfo()
        {
            _skillNum = SONG_LAMENT_OF_LONGING;
            _skillName = "lament of longing";
            _className = "SongLamentOfLonging";
            _song = true;
            _minMana = 180;
            _maxMana = 320;
            _manaChange = 5;
            PracticeInfo p0 = {CLASS_BARD, 40, 4};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongLamentOfLongingInfo();
        }
};

class SongWallOfSoundInfo : public SkillInfo
{
    public:
        SongWallOfSoundInfo() : SkillInfo()
        {
            _skillNum = SONG_WALL_OF_SOUND;
            _skillName = "wall of sound";
            _className = "SongWallOfSound";
            _song = true;
            _minMana = 90;
            _maxMana = 150;
            _manaChange = 4;
            PracticeInfo p0 = {CLASS_BARD, 42, 1};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongWallOfSoundInfo();
        }
};

class SkillLingeringSongInfo : public SkillInfo
{
    public:
        SkillLingeringSongInfo() : SkillInfo()
        {
            _skillNum = SKILL_LINGERING_SONG;
            _skillName = "lingering song";
            _className = "SkillLingeringSong";
            _skill = true;
            _violent = false;
            PracticeInfo p0 = {CLASS_BARD, 20, 2};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillLingeringSongInfo();
        }
};

class SongFortissimoInfo : public SkillInfo
{
    public:
        SongFortissimoInfo() : SkillInfo()
        {
            _skillNum = SONG_FORTISSIMO;
            _skillName = "fortissimo";
            _className = "SongFortissimo";
            _song = true;
            _minMana = 75;
            _maxMana = 150;
            _manaChange = 5;
            PracticeInfo p0 = {CLASS_BARD, 14, 3};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongFortissimoInfo();
        }
};

class SongLichsLyricsInfo : public SkillInfo
{
    public:
        SongLichsLyricsInfo() : SkillInfo()
        {
            _skillNum = SONG_LICHS_LYRICS;
            _skillName = "lichs lyrics";
            _className = "SongLichsLyrics";
            _song = true;
            _minMana = 200;
            _maxMana = 300;
            _manaChange = 25;
            _violent = true;
            PracticeInfo p0 = {CLASS_BARD, 45, 5};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongLichsLyricsInfo();
        }
};

class SongMirrorImageMelodyInfo : public SkillInfo
{
    public:
        SongMirrorImageMelodyInfo() : SkillInfo()
        {
            _skillNum = SONG_MIRROR_IMAGE_MELODY;
            _skillName = "mirror image melody";
            _className = "SongMirrorImageMelody";
            _song = true;
            _minMana = 50;
            _maxMana = 75;
            _manaChange = 10;
            PracticeInfo p0 = {CLASS_BARD, 46, 6};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SongMirrorImageMelodyInfo();
        }
};

class SpellFireBreathingInfo : public SkillInfo
{
    public:
        SpellFireBreathingInfo() : SkillInfo()
        {
            _skillNum = SPELL_FIRE_BREATHING;
            _skillName = "fire breathing";
            _className = "SpellFireBreathing";
            _arcane = true;
            _minMana = 180;
            _maxMana = 280;
            _manaChange = 10;
            PracticeInfo p0 = {CLASS_MAGE, 35, 1};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellFireBreathingInfo();
        }
};

class SpellVestigialRuneInfo : public SkillInfo
{
    public:
        SpellVestigialRuneInfo() : SkillInfo()
        {
            _skillNum = SPELL_VESTIGIAL_RUNE;
            _skillName = "vestigial rune";
            _className = "SpellVestigialRune";
            _arcane = true;
            _minMana = 380;
            _maxMana = 450;
            _manaChange = 10;
            PracticeInfo p0 = {CLASS_MAGE, 45, 1};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellVestigialRuneInfo();
        }
};

class SpellPrismaticSphereInfo : public SkillInfo
{
    public:
        SpellPrismaticSphereInfo() : SkillInfo()
        {
            _skillNum = SPELL_PRISMATIC_SPHERE;
            _skillName = "prismatic sphere";
            _className = "SpellPrismaticSphere";
            _arcane = true;
            _minMana = 30;
            _maxMana = 170;
            _manaChange = 3;
            PracticeInfo p0 = {CLASS_MAGE, 40, 1};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellPrismaticSphereInfo();
        }
};

class SpellQuantumRiftInfo : public SkillInfo
{
    public:
        SpellQuantumRiftInfo() : SkillInfo()
        {
            _skillNum = SPELL_QUANTUM_RIFT;
            _skillName = "quantum rift";
            _className = "SpellQuantumRift";
            _physics = true;
            _minMana = 300;
            _maxMana = 400;
            _manaChange = 5;
            PracticeInfo p0 = {CLASS_PHYSIC, 45, 5};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellQuantumRiftInfo();
        }
};

class SpellStoneskinInfo : public SkillInfo
{
    public:
        SpellStoneskinInfo() : SkillInfo()
        {
            _skillNum = SPELL_STONESKIN;
            _skillName = "stoneskin";
            _className = "SpellStoneskin";
            _arcane = true;
            _minMana = 25;
            _maxMana = 60;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_RANGER, 20, 4};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellStoneskinInfo();
        }
};

class SpellShieldOfRighteousnessInfo : public SkillInfo
{
    public:
        SpellShieldOfRighteousnessInfo() : SkillInfo()
        {
            _skillNum = SPELL_SHIELD_OF_RIGHTEOUSNESS;
            _skillName = "shield of righteousness";
            _className = "SpellShieldOfRighteousness";
            _divine = true;
            _minMana = 20;
            _maxMana = 50;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_CLERIC, 32, 2};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellShieldOfRighteousnessInfo();
        }
};

class SpellSunRayInfo : public SkillInfo
{
    public:
        SpellSunRayInfo() : SkillInfo()
        {
            _skillNum = SPELL_SUN_RAY;
            _skillName = "sun ray";
            _className = "SpellSunRay";
            _divine = true;
            _minMana = 40;
            _maxMana = 100;
            _manaChange = 3;
            _violent = true;
            PracticeInfo p0 = {CLASS_CLERIC, 20, 3};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellSunRayInfo();
        }
};

class SpellTaintInfo : public SkillInfo
{
    public:
        SpellTaintInfo() : SkillInfo()
        {
            _skillNum = SPELL_TAINT;
            _skillName = "taint";
            _className = "SpellTaint";
            _divine = true;
            _minMana = 180;
            _maxMana = 250;
            _manaChange = 10;
            _violent = true;
            PracticeInfo p0 = {CLASS_KNIGHT, 42, 9};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellTaintInfo();
        }
};

class SpellBlackmantleInfo : public SkillInfo
{
    public:
        SpellBlackmantleInfo() : SkillInfo()
        {
            _skillNum = SPELL_BLACKMANTLE;
            _skillName = "blackmantle";
            _className = "SpellBlackmantle";
            _divine = true;
            _minMana = 80;
            _maxMana = 120;
            _manaChange = 5;
            _violent = true;
            PracticeInfo p0 = {CLASS_CLERIC, 42, 1};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellBlackmantleInfo();
        }
};

class SpellSanctificationInfo : public SkillInfo
{
    public:
        SpellSanctificationInfo() : SkillInfo()
        {
            _skillNum = SPELL_SANCTIFICATION;
            _skillName = "sanctification";
            _className = "SpellSanctification";
            _divine = true;
            _minMana = 60;
            _maxMana = 100;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_KNIGHT, 33, 1};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellSanctificationInfo();
        }
};

class SpellStigmataInfo : public SkillInfo
{
    public:
        SpellStigmataInfo() : SkillInfo()
        {
            _skillNum = SPELL_STIGMATA;
            _skillName = "stigmata";
            _className = "SpellStigmata";
            _divine = true;
            _minMana = 30;
            _maxMana = 60;
            _manaChange = 1;
            _violent = true;
            PracticeInfo p0 = {CLASS_CLERIC, 23, 1};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellStigmataInfo();
        }
};

class SpellDivinePowerInfo : public SkillInfo
{
    public:
        SpellDivinePowerInfo() : SkillInfo()
        {
            _skillNum = SPELL_DIVINE_POWER;
            _skillName = "divine power";
            _className = "SpellDivinePower";
            _divine = true;
            _minMana = 80;
            _maxMana = 150;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_CLERIC, 30, 5};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellDivinePowerInfo();
        }
};

class SpellDeathKnellInfo : public SkillInfo
{
    public:
        SpellDeathKnellInfo() : SkillInfo()
        {
            _skillNum = SPELL_DEATH_KNELL;
            _skillName = "death knell";
            _className = "SpellDeathKnell";
            _divine = true;
            _minMana = 30;
            _maxMana = 100;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_CLERIC, 25, 1};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellDeathKnellInfo();
        }
};

class SpellManaShieldInfo : public SkillInfo
{
    public:
        SpellManaShieldInfo() : SkillInfo()
        {
            _skillNum = SPELL_MANA_SHIELD;
            _skillName = "mana shield";
            _className = "SpellManaShield";
            _arcane = true;
            _minMana = 30;
            _maxMana = 60;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_MAGE, 29, 3};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellManaShieldInfo();
        }
};

class SpellEntangleInfo : public SkillInfo
{
    public:
        SpellEntangleInfo() : SkillInfo()
        {
            _skillNum = SPELL_ENTANGLE;
            _skillName = "entangle";
            _className = "SpellEntangle";
            _arcane = true;
            _minMana = 30;
            _maxMana = 70;
            _manaChange = 2;
            _violent = true;
            PracticeInfo p0 = {CLASS_RANGER, 22, 1};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellEntangleInfo();
        }
};

class SpellElementalBrandInfo : public SkillInfo
{
    public:
        SpellElementalBrandInfo() : SkillInfo()
        {
            _skillNum = SPELL_ELEMENTAL_BRAND;
            _skillName = "elemental brand";
            _className = "SpellElementalBrand";
            _arcane = true;
            _minMana = 120;
            _maxMana = 160;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_RANGER, 31, 1};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellElementalBrandInfo();
        }
};

class SpellSummonLegionInfo : public SkillInfo
{
    public:
        SpellSummonLegionInfo() : SkillInfo()
        {
            _skillNum = SPELL_SUMMON_LEGION;
            _skillName = "summon legion";
            _className = "SpellSummonLegion";
            _divine = true;
            _minMana = 100;
            _maxMana = 200;
            _manaChange = 5;
            PracticeInfo p0 = {CLASS_KNIGHT, 27, 5};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellSummonLegionInfo();
        }
};

class SpellAntiMagicShellInfo : public SkillInfo
{
    public:
        SpellAntiMagicShellInfo() : SkillInfo()
        {
            _skillNum = SPELL_ANTI_MAGIC_SHELL;
            _skillName = "anti magic shell";
            _className = "SpellAntiMagicShell";
            _arcane = true;
            _minMana = 70;
            _maxMana = 150;
            _manaChange = 4;
            PracticeInfo p0 = {CLASS_MAGE, 21, 2};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellAntiMagicShellInfo();
        }
};

class SpellWardingSigilInfo : public SkillInfo
{
    public:
        SpellWardingSigilInfo() : SkillInfo()
        {
            _skillNum = SPELL_WARDING_SIGIL;
            _skillName = "warding sigil";
            _className = "SpellWardingSigil";
            _arcane = true;
            _minMana = 250;
            _maxMana = 300;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_MAGE, 17, 1};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellWardingSigilInfo();
        }
};

class SpellDivineInterventionInfo : public SkillInfo
{
    public:
        SpellDivineInterventionInfo() : SkillInfo()
        {
            _skillNum = SPELL_DIVINE_INTERVENTION;
            _skillName = "divine intervention";
            _className = "SpellDivineIntervention";
            _divine = true;
            _minMana = 70;
            _maxMana = 120;
            _manaChange = 3;
            PracticeInfo p0 = {CLASS_CLERIC, 21, 4};
            PracticeInfo p1 = {CLASS_KNIGHT, 23, 6};
            _practiceInfo.push_back(p0);
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellDivineInterventionInfo();
        }
};

class SpellSphereOfDesecrationInfo : public SkillInfo
{
    public:
        SpellSphereOfDesecrationInfo() : SkillInfo()
        {
            _skillNum = SPELL_SPHERE_OF_DESECRATION;
            _skillName = "sphere of desecration";
            _className = "SpellSphereOfDesecration";
            _divine = true;
            _minMana = 70;
            _maxMana = 120;
            _manaChange = 3;
            PracticeInfo p0 = {CLASS_CLERIC, 21, 4};
            PracticeInfo p1 = {CLASS_KNIGHT, 23, 6};
            _practiceInfo.push_back(p0);
            _practiceInfo.push_back(p1);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellSphereOfDesecrationInfo();
        }
};

class SpellAmnesiaInfo : public SkillInfo
{
    public:
        SpellAmnesiaInfo() : SkillInfo()
        {
            _skillNum = SPELL_AMNESIA;
            _skillName = "amnesia";
            _className = "SpellAmnesia";
            _psionic = true;
            _minMana = 30;
            _maxMana = 70;
            _manaChange = 2;
            _violent = true;
            PracticeInfo p0 = {CLASS_PSIONIC, 25, 1};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellAmnesiaInfo();
        }
};

class SpellPsychicFeedbackInfo : public SkillInfo
{
    public:
        SpellPsychicFeedbackInfo() : SkillInfo()
        {
            _skillNum = SPELL_PSYCHIC_FEEDBACK;
            _skillName = "psychic feedback";
            _className = "SpellPsychicFeedback";
            _psionic = true;
            _minMana = 30;
            _maxMana = 75;
            _manaChange = 3;
            PracticeInfo p0 = {CLASS_PSIONIC, 32, 3};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellPsychicFeedbackInfo();
        }
};

class SpellIdInsinuationInfo : public SkillInfo
{
    public:
        SpellIdInsinuationInfo() : SkillInfo()
        {
            _skillNum = SPELL_ID_INSINUATION;
            _skillName = "id insinuation";
            _className = "SpellIdInsinuation";
            _psionic = true;
            _minMana = 80;
            _maxMana = 110;
            _manaChange = 5;
            _violent = true;
            PracticeInfo p0 = {CLASS_PSIONIC, 32, 1};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellIdInsinuationInfo();
        }
};

class ZenTranslocationInfo : public SkillInfo
{
    public:
        ZenTranslocationInfo() : SkillInfo()
        {
            _skillNum = ZEN_TRANSLOCATION;
            _skillName = "translocation";
            _className = "ZenTranslocation";
            _zen = true;
            _minMana = 20;
            _maxMana = 40;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_MONK, 37, 1};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new ZenTranslocationInfo();
        }
};

class ZenCelerityInfo : public SkillInfo
{
    public:
        ZenCelerityInfo() : SkillInfo()
        {
            _skillNum = ZEN_CELERITY;
            _skillName = "celerity";
            _className = "ZenCelerity";
            _zen = true;
            _minMana = 20;
            _maxMana = 40;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_MONK, 25, 2};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new ZenCelerityInfo();
        }
};

class SkillDisguiseInfo : public SkillInfo
{
    public:
        SkillDisguiseInfo() : SkillInfo()
        {
            _skillNum = SKILL_DISGUISE;
            _skillName = "disguise";
            _className = "SkillDisguise";
            _skill = true;
            _minMana = 20;
            _maxMana = 40;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_THIEF, 17, 1};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillDisguiseInfo();
        }
};

class SkillUncannyDodgeInfo : public SkillInfo
{
    public:
        SkillUncannyDodgeInfo() : SkillInfo()
        {
            _skillNum = SKILL_UNCANNY_DODGE;
            _skillName = "uncanny dodge";
            _className = "SkillUncannyDodge";
            _skill = true;
            PracticeInfo p0 = {CLASS_THIEF, 22, 2};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillUncannyDodgeInfo();
        }
};

class SkillDeEnergizeInfo : public SkillInfo
{
    public:
        SkillDeEnergizeInfo() : SkillInfo()
        {
            _skillNum = SKILL_DE_ENERGIZE;
            _skillName = "de energize";
            _className = "SkillDeEnergize";
            _skill = true;
            _violent = true;
            PracticeInfo p0 = {CLASS_CYBORG, 22, 1};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillDeEnergizeInfo();
        }
};

class SkillAssimilateInfo : public SkillInfo
{
    public:
        SkillAssimilateInfo() : SkillInfo()
        {
            _skillNum = SKILL_ASSIMILATE;
            _skillName = "assimilate";
            _className = "SkillAssimilate";
            _skill = true;
            _violent = true;
            PracticeInfo p0 = {CLASS_CYBORG, 22, 3};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillAssimilateInfo();
        }
};

class SkillRadionegationInfo : public SkillInfo
{
    public:
        SkillRadionegationInfo() : SkillInfo()
        {
            _skillNum = SKILL_RADIONEGATION;
            _skillName = "radionegation";
            _className = "SkillRadionegation";
            _program = true;
            _violent = true;
            PracticeInfo p0 = {CLASS_CYBORG, 17, 1};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillRadionegationInfo();
        }
};

class SpellMaleficViolationInfo : public SkillInfo
{
    public:
        SpellMaleficViolationInfo() : SkillInfo()
        {
            _skillNum = SPELL_MALEFIC_VIOLATION;
            _skillName = "malefic violation";
            _className = "SpellMaleficViolation";
            _divine = true;
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 5;
            PracticeInfo p0 = {CLASS_CLERIC, 39, 5};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellMaleficViolationInfo();
        }
};

class SpellRighteousPenetrationInfo : public SkillInfo
{
    public:
        SpellRighteousPenetrationInfo() : SkillInfo()
        {
            _skillNum = SPELL_RIGHTEOUS_PENETRATION;
            _skillName = "righteous penetration";
            _className = "SpellRighteousPenetration";
            _divine = true;
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 5;
            PracticeInfo p0 = {CLASS_CLERIC, 39, 5};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellRighteousPenetrationInfo();
        }
};

class SkillWormholeInfo : public SkillInfo
{
    public:
        SkillWormholeInfo() : SkillInfo()
        {
            _skillNum = SKILL_WORMHOLE;
            _skillName = "wormhole";
            _className = "SkillWormhole";
            _skill = true;
            _minMana = 20;
            _maxMana = 50;
            _manaChange = 2;
            PracticeInfo p0 = {CLASS_PHYSIC, 30, 1};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillWormholeInfo();
        }
};

class SpellGaussShieldInfo : public SkillInfo
{
    public:
        SpellGaussShieldInfo() : SkillInfo()
        {
            _skillNum = SPELL_GAUSS_SHIELD;
            _skillName = "gauss shield";
            _className = "SpellGaussShield";
            _physics = true;
            _minMana = 70;
            _maxMana = 90;
            _manaChange = 1;
            PracticeInfo p0 = {CLASS_PHYSIC, 32, 3};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellGaussShieldInfo();
        }
};

class SpellDimensionalShiftInfo : public SkillInfo
{
    public:
        SpellDimensionalShiftInfo() : SkillInfo()
        {
            _skillNum = SPELL_DIMENSIONAL_SHIFT;
            _skillName = "dimensional shift";
            _className = "SpellDimensionalShift";
            _physics = true;
            _minMana = 65;
            _maxMana = 100;
            _manaChange = 3;
            PracticeInfo p0 = {CLASS_PHYSIC, 35, 4};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellDimensionalShiftInfo();
        }
};

class SpellUnholyStalkerInfo : public SkillInfo
{
    public:
        SpellUnholyStalkerInfo() : SkillInfo()
        {
            _skillNum = SPELL_UNHOLY_STALKER;
            _skillName = "unholy stalker";
            _className = "SpellUnholyStalker";
            _divine = true;
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 5;
            _violent = true;
            PracticeInfo p0 = {CLASS_CLERIC, 25, 3};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellUnholyStalkerInfo();
        }
};

class SpellAnimateDeadInfo : public SkillInfo
{
    public:
        SpellAnimateDeadInfo() : SkillInfo()
        {
            _skillNum = SPELL_ANIMATE_DEAD;
            _skillName = "animate dead";
            _className = "SpellAnimateDead";
            _divine = true;
            _minMana = 30;
            _maxMana = 60;
            _manaChange = 1;
            _violent = true;
            PracticeInfo p0 = {CLASS_CLERIC, 24, 2};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellAnimateDeadInfo();
        }
};

class SpellInfernoInfo : public SkillInfo
{
    public:
        SpellInfernoInfo() : SkillInfo()
        {
            _skillNum = SPELL_INFERNO;
            _skillName = "inferno";
            _className = "SpellInferno";
            _divine = true;
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 7;
            _violent = true;
            PracticeInfo p0 = {CLASS_CLERIC, 35, 1};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellInfernoInfo();
        }
};

class SpellVampiricRegenerationInfo : public SkillInfo
{
    public:
        SpellVampiricRegenerationInfo() : SkillInfo()
        {
            _skillNum = SPELL_VAMPIRIC_REGENERATION;
            _skillName = "vampiric regeneration";
            _className = "SpellVampiricRegeneration";
            _divine = true;
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 5;
            _violent = true;
            PracticeInfo p0 = {CLASS_CLERIC, 37, 4};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellVampiricRegenerationInfo();
        }
};

class SpellBanishmentInfo : public SkillInfo
{
    public:
        SpellBanishmentInfo() : SkillInfo()
        {
            _skillNum = SPELL_BANISHMENT;
            _skillName = "banishment";
            _className = "SpellBanishment";
            _divine = true;
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 5;
            _violent = true;
            PracticeInfo p0 = {CLASS_CLERIC, 29, 4};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellBanishmentInfo();
        }
};

class SkillDisciplineOfSteelInfo : public SkillInfo
{
    public:
        SkillDisciplineOfSteelInfo() : SkillInfo()
        {
            _skillNum = SKILL_DISCIPLINE_OF_STEEL;
            _skillName = "discipline of steel";
            _className = "SkillDisciplineOfSteel";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 10, 1};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillDisciplineOfSteelInfo();
        }
};

class SkillGreatCleaveInfo : public SkillInfo
{
    public:
        SkillGreatCleaveInfo() : SkillInfo()
        {
            _skillNum = SKILL_GREAT_CLEAVE;
            _skillName = "great cleave";
            _className = "SkillGreatCleave";
            _skill = true;
            PracticeInfo p0 = {CLASS_BARB, 40, 2};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SkillGreatCleaveInfo();
        }
};

class SpellLocustRegenerationInfo : public SkillInfo
{
    public:
        SpellLocustRegenerationInfo() : SkillInfo()
        {
            _skillNum = SPELL_LOCUST_REGENERATION;
            _skillName = "locust regeneration";
            _className = "SpellLocustRegeneration";
            _arcane = true;
            _minMana = 60;
            _maxMana = 125;
            _manaChange = 5;
            _violent = true;
            PracticeInfo p0 = {CLASS_MAGE, 34, 5};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellLocustRegenerationInfo();
        }
};

class SpellThornSkinInfo : public SkillInfo
{
    public:
        SpellThornSkinInfo() : SkillInfo()
        {
            _skillNum = SPELL_THORN_SKIN;
            _skillName = "thorn skin";
            _className = "SpellThornSkin";
            _arcane = true;
            _minMana = 80;
            _maxMana = 110;
            _manaChange = 1;
            _violent = false;
            PracticeInfo p0 = {CLASS_RANGER, 38, 6};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellThornSkinInfo();
        }
};

class SpellSpiritTrackInfo : public SkillInfo
{
    public:
        SpellSpiritTrackInfo() : SkillInfo()
        {
            _skillNum = SPELL_SPIRIT_TRACK;
            _skillName = "spirit track";
            _className = "SpellSpiritTrack";
            _arcane = true;
            _minMana = 60;
            _maxMana = 120;
            _manaChange = 10;
            _violent = false;
            PracticeInfo p0 = {CLASS_RANGER, 38, 1};
            _practiceInfo.push_back(p0);
        }

        SkillInfo *createNewInstance() {
            return (SkillInfo *)new SpellSpiritTrackInfo();
        }
};
void boot_skill_info()
{
    SkillInfo::insertSkill(new SpellAirWalkInfo());
    SkillInfo::insertSkill(new SpellArmorInfo());
    SkillInfo::insertSkill(new SpellAstralSpellInfo());
    SkillInfo::insertSkill(new SpellControlUndeadInfo());
    SkillInfo::insertSkill(new SpellTeleportInfo());
    SkillInfo::insertSkill(new SpellLocalTeleportInfo());
    SkillInfo::insertSkill(new SpellBlurInfo());
    SkillInfo::insertSkill(new SpellBlessInfo());
    SkillInfo::insertSkill(new SpellDamnInfo());
    SkillInfo::insertSkill(new SpellCalmInfo());
    SkillInfo::insertSkill(new SpellBlindnessInfo());
    SkillInfo::insertSkill(new SpellBreatheWaterInfo());
    SkillInfo::insertSkill(new SpellBurningHandsInfo());
    SkillInfo::insertSkill(new SpellCallLightningInfo());
    SkillInfo::insertSkill(new SpellEnvenomInfo());
    SkillInfo::insertSkill(new SpellCharmInfo());
    SkillInfo::insertSkill(new SpellCharmAnimalInfo());
    SkillInfo::insertSkill(new SpellChillTouchInfo());
    SkillInfo::insertSkill(new SpellClairvoyanceInfo());
    SkillInfo::insertSkill(new SpellCallRodentInfo());
    SkillInfo::insertSkill(new SpellCallBirdInfo());
    SkillInfo::insertSkill(new SpellCallReptileInfo());
    SkillInfo::insertSkill(new SpellCallBeastInfo());
    SkillInfo::insertSkill(new SpellCallPredatorInfo());
    SkillInfo::insertSkill(new SpellCloneInfo());
    SkillInfo::insertSkill(new SpellColorSprayInfo());
    SkillInfo::insertSkill(new SpellCommandInfo());
    SkillInfo::insertSkill(new SpellConeOfColdInfo());
    SkillInfo::insertSkill(new SpellConjureElementalInfo());
    SkillInfo::insertSkill(new SpellControlWeatherInfo());
    SkillInfo::insertSkill(new SpellCreateFoodInfo());
    SkillInfo::insertSkill(new SpellCreateWaterInfo());
    SkillInfo::insertSkill(new SpellCureBlindInfo());
    SkillInfo::insertSkill(new SpellCureCriticInfo());
    SkillInfo::insertSkill(new SpellCureLightInfo());
    SkillInfo::insertSkill(new SpellCurseInfo());
    SkillInfo::insertSkill(new SpellDetectAlignInfo());
    SkillInfo::insertSkill(new SpellDetectInvisInfo());
    SkillInfo::insertSkill(new SpellDetectMagicInfo());
    SkillInfo::insertSkill(new SpellDetectPoisonInfo());
    SkillInfo::insertSkill(new SpellDetectScryingInfo());
    SkillInfo::insertSkill(new SpellDimensionDoorInfo());
    SkillInfo::insertSkill(new SpellDispelEvilInfo());
    SkillInfo::insertSkill(new SpellDispelGoodInfo());
    SkillInfo::insertSkill(new SpellDispelMagicInfo());
    SkillInfo::insertSkill(new SpellDisruptionInfo());
    SkillInfo::insertSkill(new SpellDisplacementInfo());
    SkillInfo::insertSkill(new SpellEarthquakeInfo());
    SkillInfo::insertSkill(new SpellEnchantArmorInfo());
    SkillInfo::insertSkill(new SpellEnchantWeaponInfo());
    SkillInfo::insertSkill(new SpellEndureColdInfo());
    SkillInfo::insertSkill(new SpellEnergyDrainInfo());
    SkillInfo::insertSkill(new SpellFlyInfo());
    SkillInfo::insertSkill(new SpellFlameStrikeInfo());
    SkillInfo::insertSkill(new SpellFlameOfFaithInfo());
    SkillInfo::insertSkill(new SpellGoodberryInfo());
    SkillInfo::insertSkill(new SpellGustOfWindInfo());
    SkillInfo::insertSkill(new SpellBarkskinInfo());
    SkillInfo::insertSkill(new SpellIcyBlastInfo());
    SkillInfo::insertSkill(new SpellInvisToUndeadInfo());
    SkillInfo::insertSkill(new SpellAnimalKinInfo());
    SkillInfo::insertSkill(new SpellGreaterEnchantInfo());
    SkillInfo::insertSkill(new SpellGreaterInvisInfo());
    SkillInfo::insertSkill(new SpellGroupArmorInfo());
    SkillInfo::insertSkill(new SpellFireballInfo());
    SkillInfo::insertSkill(new SpellFireShieldInfo());
    SkillInfo::insertSkill(new SpellGreaterHealInfo());
    SkillInfo::insertSkill(new SpellGroupHealInfo());
    SkillInfo::insertSkill(new SpellHarmInfo());
    SkillInfo::insertSkill(new SpellHealInfo());
    SkillInfo::insertSkill(new SpellHasteInfo());
    SkillInfo::insertSkill(new SpellInfravisionInfo());
    SkillInfo::insertSkill(new SpellInvisibleInfo());
    SkillInfo::insertSkill(new SpellGlowlightInfo());
    SkillInfo::insertSkill(new SpellKnockInfo());
    SkillInfo::insertSkill(new SpellLightningBoltInfo());
    SkillInfo::insertSkill(new SpellLocateObjectInfo());
    SkillInfo::insertSkill(new SpellMagicMissileInfo());
    SkillInfo::insertSkill(new SpellMinorIdentifyInfo());
    SkillInfo::insertSkill(new SpellMagicalProtInfo());
    SkillInfo::insertSkill(new SpellMagicalVestmentInfo());
    SkillInfo::insertSkill(new SpellMeteorStormInfo());
    SkillInfo::insertSkill(new SpellChainLightningInfo());
    SkillInfo::insertSkill(new SpellHailstormInfo());
    SkillInfo::insertSkill(new SpellIceStormInfo());
    SkillInfo::insertSkill(new SpellPoisonInfo());
    SkillInfo::insertSkill(new SpellPrayInfo());
    SkillInfo::insertSkill(new SpellPrismaticSprayInfo());
    SkillInfo::insertSkill(new SpellProtectFromDevilsInfo());
    SkillInfo::insertSkill(new SpellProtFromEvilInfo());
    SkillInfo::insertSkill(new SpellProtFromGoodInfo());
    SkillInfo::insertSkill(new SpellProtFromLightningInfo());
    SkillInfo::insertSkill(new SpellProtFromFireInfo());
    SkillInfo::insertSkill(new SpellRemoveCurseInfo());
    SkillInfo::insertSkill(new SpellRemoveSicknessInfo());
    SkillInfo::insertSkill(new SpellRejuvenateInfo());
    SkillInfo::insertSkill(new SpellRefreshInfo());
    SkillInfo::insertSkill(new SpellRegenerateInfo());
    SkillInfo::insertSkill(new SpellRetrieveCorpseInfo());
    SkillInfo::insertSkill(new SpellSanctuaryInfo());
    SkillInfo::insertSkill(new SpellShockingGraspInfo());
    SkillInfo::insertSkill(new SpellShroudObscurementInfo());
    SkillInfo::insertSkill(new SpellSleepInfo());
    SkillInfo::insertSkill(new SpellSlowInfo());
    SkillInfo::insertSkill(new SpellSpiritHammerInfo());
    SkillInfo::insertSkill(new SpellStrengthInfo());
    SkillInfo::insertSkill(new SpellSummonInfo());
    SkillInfo::insertSkill(new SpellSwordInfo());
    SkillInfo::insertSkill(new SpellSymbolOfPainInfo());
    SkillInfo::insertSkill(new SpellStoneToFleshInfo());
    SkillInfo::insertSkill(new SpellWordStunInfo());
    SkillInfo::insertSkill(new SpellTrueSeeingInfo());
    SkillInfo::insertSkill(new SpellWordOfRecallInfo());
    SkillInfo::insertSkill(new SpellWordOfIntellectInfo());
    SkillInfo::insertSkill(new SpellPeerInfo());
    SkillInfo::insertSkill(new SpellRemovePoisonInfo());
    SkillInfo::insertSkill(new SpellRestorationInfo());
    SkillInfo::insertSkill(new SpellSenseLifeInfo());
    SkillInfo::insertSkill(new SpellUndeadProtInfo());
    SkillInfo::insertSkill(new SpellWaterwalkInfo());
    SkillInfo::insertSkill(new SpellIdentifyInfo());
    SkillInfo::insertSkill(new SpellWallOfThornsInfo());
    SkillInfo::insertSkill(new SpellWallOfStoneInfo());
    SkillInfo::insertSkill(new SpellWallOfIceInfo());
    SkillInfo::insertSkill(new SpellWallOfFireInfo());
    SkillInfo::insertSkill(new SpellWallOfIronInfo());
    SkillInfo::insertSkill(new SpellThornTrapInfo());
    SkillInfo::insertSkill(new SpellFierySheetInfo());
    SkillInfo::insertSkill(new SpellPowerInfo());
    SkillInfo::insertSkill(new SpellIntellectInfo());
    SkillInfo::insertSkill(new SpellConfusionInfo());
    SkillInfo::insertSkill(new SpellFearInfo());
    SkillInfo::insertSkill(new SpellSatiationInfo());
    SkillInfo::insertSkill(new SpellQuenchInfo());
    SkillInfo::insertSkill(new SpellConfidenceInfo());
    SkillInfo::insertSkill(new SpellNopainInfo());
    SkillInfo::insertSkill(new SpellDermalHardeningInfo());
    SkillInfo::insertSkill(new SpellWoundClosureInfo());
    SkillInfo::insertSkill(new SpellAntibodyInfo());
    SkillInfo::insertSkill(new SpellRetinaInfo());
    SkillInfo::insertSkill(new SpellAdrenalineInfo());
    SkillInfo::insertSkill(new SpellBreathingStasisInfo());
    SkillInfo::insertSkill(new SpellVertigoInfo());
    SkillInfo::insertSkill(new SpellMetabolismInfo());
    SkillInfo::insertSkill(new SpellEgoWhipInfo());
    SkillInfo::insertSkill(new SpellPsychicCrushInfo());
    SkillInfo::insertSkill(new SpellRelaxationInfo());
    SkillInfo::insertSkill(new SpellWeaknessInfo());
    SkillInfo::insertSkill(new SpellFortressOfWillInfo());
    SkillInfo::insertSkill(new SpellCellRegenInfo());
    SkillInfo::insertSkill(new SpellPsishieldInfo());
    SkillInfo::insertSkill(new SpellPsychicSurgeInfo());
    SkillInfo::insertSkill(new SpellPsychicConduitInfo());
    SkillInfo::insertSkill(new SpellPsionicShatterInfo());
    SkillInfo::insertSkill(new SpellMelatonicFloodInfo());
    SkillInfo::insertSkill(new SpellMotorSpasmInfo());
    SkillInfo::insertSkill(new SpellPsychicResistanceInfo());
    SkillInfo::insertSkill(new SpellMassHysteriaInfo());
    SkillInfo::insertSkill(new SpellGroupConfidenceInfo());
    SkillInfo::insertSkill(new SpellClumsinessInfo());
    SkillInfo::insertSkill(new SpellEnduranceInfo());
    SkillInfo::insertSkill(new SpellNullpsiInfo());
    SkillInfo::insertSkill(new SpellTelepathyInfo());
    SkillInfo::insertSkill(new SpellDistractionInfo());
    SkillInfo::insertSkill(new SkillPsiblastInfo());
    SkillInfo::insertSkill(new SkillPsidrainInfo());
    SkillInfo::insertSkill(new SpellElectrostaticFieldInfo());
    SkillInfo::insertSkill(new SpellEntropyFieldInfo());
    SkillInfo::insertSkill(new SpellAcidityInfo());
    SkillInfo::insertSkill(new SpellAttractionFieldInfo());
    SkillInfo::insertSkill(new SpellNuclearWastelandInfo());
    SkillInfo::insertSkill(new SpellSpacetimeImprintInfo());
    SkillInfo::insertSkill(new SpellSpacetimeRecallInfo());
    SkillInfo::insertSkill(new SpellFluoresceInfo());
    SkillInfo::insertSkill(new SpellGammaRayInfo());
    SkillInfo::insertSkill(new SpellGravityWellInfo());
    SkillInfo::insertSkill(new SpellCapacitanceBoostInfo());
    SkillInfo::insertSkill(new SpellElectricArcInfo());
    SkillInfo::insertSkill(new SpellTemporalCompressionInfo());
    SkillInfo::insertSkill(new SpellTemporalDilationInfo());
    SkillInfo::insertSkill(new SpellHalflifeInfo());
    SkillInfo::insertSkill(new SpellMicrowaveInfo());
    SkillInfo::insertSkill(new SpellOxidizeInfo());
    SkillInfo::insertSkill(new SpellRandomCoordinatesInfo());
    SkillInfo::insertSkill(new SpellRepulsionFieldInfo());
    SkillInfo::insertSkill(new SpellVacuumShroudInfo());
    SkillInfo::insertSkill(new SpellAlbedoShieldInfo());
    SkillInfo::insertSkill(new SpellTransmittanceInfo());
    SkillInfo::insertSkill(new SpellTimeWarpInfo());
    SkillInfo::insertSkill(new SpellRadioimmunityInfo());
    SkillInfo::insertSkill(new SpellDensifyInfo());
    SkillInfo::insertSkill(new SpellLatticeHardeningInfo());
    SkillInfo::insertSkill(new SpellChemicalStabilityInfo());
    SkillInfo::insertSkill(new SpellRefractionInfo());
    SkillInfo::insertSkill(new SpellNullifyInfo());
    SkillInfo::insertSkill(new SpellAreaStasisInfo());
    SkillInfo::insertSkill(new SpellEmpPulseInfo());
    SkillInfo::insertSkill(new SpellFissionBlastInfo());
    SkillInfo::insertSkill(new SpellDimensionalVoidInfo());
    SkillInfo::insertSkill(new SkillAmbushInfo());
    SkillInfo::insertSkill(new ZenHealingInfo());
    SkillInfo::insertSkill(new ZenOfAwarenessInfo());
    SkillInfo::insertSkill(new ZenOblivityInfo());
    SkillInfo::insertSkill(new ZenMotionInfo());
    SkillInfo::insertSkill(new ZenDispassionInfo());
    SkillInfo::insertSkill(new SongDriftersDittyInfo());
    SkillInfo::insertSkill(new SongAriaOfArmamentInfo());
    SkillInfo::insertSkill(new SongVerseOfVulnerabilityInfo());
    SkillInfo::insertSkill(new SongMelodyOfMettleInfo());
    SkillInfo::insertSkill(new SongRegalersRhapsodyInfo());
    SkillInfo::insertSkill(new SongDefenseDittyInfo());
    SkillInfo::insertSkill(new SongAlronsAriaInfo());
    SkillInfo::insertSkill(new SongLustrationMelismaInfo());
    SkillInfo::insertSkill(new SongExposureOvertureInfo());
    SkillInfo::insertSkill(new SongVerseOfValorInfo());
    SkillInfo::insertSkill(new SongWhiteNoiseInfo());
    SkillInfo::insertSkill(new SongHomeSweetHomeInfo());
    SkillInfo::insertSkill(new SongChantOfLightInfo());
    SkillInfo::insertSkill(new SongInsidiousRhythmInfo());
    SkillInfo::insertSkill(new SongEaglesOvertureInfo());
    SkillInfo::insertSkill(new SongWeightOfTheWorldInfo());
    SkillInfo::insertSkill(new SongGuihariasGloryInfo());
    SkillInfo::insertSkill(new SongRhapsodyOfRemedyInfo());
    SkillInfo::insertSkill(new SongUnladenSwallowSongInfo());
    SkillInfo::insertSkill(new SongClarifyingHarmoniesInfo());
    SkillInfo::insertSkill(new SongPowerOvertureInfo());
    SkillInfo::insertSkill(new SongRhythmOfRageInfo());
    SkillInfo::insertSkill(new SongUnravellingDiapasonInfo());
    SkillInfo::insertSkill(new SongInstantAudienceInfo());
    SkillInfo::insertSkill(new SongWoundingWhispersInfo());
    SkillInfo::insertSkill(new SongRhythmOfAlarmInfo());
    SkillInfo::insertSkill(new SkillTumblingInfo());
    SkillInfo::insertSkill(new SkillScreamInfo());
    SkillInfo::insertSkill(new SongSonicDisruptionInfo());
    SkillInfo::insertSkill(new SongDirgeInfo());
    SkillInfo::insertSkill(new SongMisdirectionMelismaInfo());
    SkillInfo::insertSkill(new SongHymnOfPeaceInfo());
    SkillInfo::insertSkill(new SkillVentriloquismInfo());
    SkillInfo::insertSkill(new SpellTidalSpacewarpInfo());
    SkillInfo::insertSkill(new SkillCleaveInfo());
    SkillInfo::insertSkill(new SkillStrikeInfo());
    SkillInfo::insertSkill(new SkillHamstringInfo());
    SkillInfo::insertSkill(new SkillDragInfo());
    SkillInfo::insertSkill(new SkillSnatchInfo());
    SkillInfo::insertSkill(new SkillArcheryInfo());
    SkillInfo::insertSkill(new SkillBowFletchInfo());
    SkillInfo::insertSkill(new SkillReadScrollsInfo());
    SkillInfo::insertSkill(new SkillUseWandsInfo());
    SkillInfo::insertSkill(new SkillBackstabInfo());
    SkillInfo::insertSkill(new SkillCircleInfo());
    SkillInfo::insertSkill(new SkillHideInfo());
    SkillInfo::insertSkill(new SkillKickInfo());
    SkillInfo::insertSkill(new SkillBashInfo());
    SkillInfo::insertSkill(new SkillBreakDoorInfo());
    SkillInfo::insertSkill(new SkillHeadbuttInfo());
    SkillInfo::insertSkill(new SkillHotwireInfo());
    SkillInfo::insertSkill(new SkillGougeInfo());
    SkillInfo::insertSkill(new SkillStunInfo());
    SkillInfo::insertSkill(new SkillFeignInfo());
    SkillInfo::insertSkill(new SkillConcealInfo());
    SkillInfo::insertSkill(new SkillPlantInfo());
    SkillInfo::insertSkill(new SkillPickLockInfo());
    SkillInfo::insertSkill(new SkillRescueInfo());
    SkillInfo::insertSkill(new SkillSneakInfo());
    SkillInfo::insertSkill(new SkillStealInfo());
    SkillInfo::insertSkill(new SkillTrackInfo());
    SkillInfo::insertSkill(new SkillPunchInfo());
    SkillInfo::insertSkill(new SkillPiledriveInfo());
    SkillInfo::insertSkill(new SkillShieldMasteryInfo());
    SkillInfo::insertSkill(new SkillSleeperInfo());
    SkillInfo::insertSkill(new SkillElbowInfo());
    SkillInfo::insertSkill(new SkillKneeInfo());
    SkillInfo::insertSkill(new SkillAutopsyInfo());
    SkillInfo::insertSkill(new SkillBerserkInfo());
    SkillInfo::insertSkill(new SkillBattleCryInfo());
    SkillInfo::insertSkill(new SkillKiaInfo());
    SkillInfo::insertSkill(new SkillCryFromBeyondInfo());
    SkillInfo::insertSkill(new SkillStompInfo());
    SkillInfo::insertSkill(new SkillBodyslamInfo());
    SkillInfo::insertSkill(new SkillChokeInfo());
    SkillInfo::insertSkill(new SkillClotheslineInfo());
    SkillInfo::insertSkill(new SkillTagInfo());
    SkillInfo::insertSkill(new SkillIntimidateInfo());
    SkillInfo::insertSkill(new SkillSpeedHealingInfo());
    SkillInfo::insertSkill(new SkillStalkInfo());
    SkillInfo::insertSkill(new SkillHearingInfo());
    SkillInfo::insertSkill(new SkillBearhugInfo());
    SkillInfo::insertSkill(new SkillBiteInfo());
    SkillInfo::insertSkill(new SkillDblAttackInfo());
    SkillInfo::insertSkill(new SkillNightVisionInfo());
    SkillInfo::insertSkill(new SkillTurnInfo());
    SkillInfo::insertSkill(new SkillAnalyzeInfo());
    SkillInfo::insertSkill(new SkillEvaluateInfo());
    SkillInfo::insertSkill(new SkillCureDiseaseInfo());
    SkillInfo::insertSkill(new SkillHolyTouchInfo());
    SkillInfo::insertSkill(new SkillBandageInfo());
    SkillInfo::insertSkill(new SkillFirstaidInfo());
    SkillInfo::insertSkill(new SkillMedicInfo());
    SkillInfo::insertSkill(new SkillLeatherworkingInfo());
    SkillInfo::insertSkill(new SkillMetalworkingInfo());
    SkillInfo::insertSkill(new SkillConsiderInfo());
    SkillInfo::insertSkill(new SkillGlanceInfo());
    SkillInfo::insertSkill(new SkillShootInfo());
    SkillInfo::insertSkill(new SkillBeheadInfo());
    SkillInfo::insertSkill(new SkillEmpowerInfo());
    SkillInfo::insertSkill(new SkillDisarmInfo());
    SkillInfo::insertSkill(new SkillSpinkickInfo());
    SkillInfo::insertSkill(new SkillRoundhouseInfo());
    SkillInfo::insertSkill(new SkillSidekickInfo());
    SkillInfo::insertSkill(new SkillSpinfistInfo());
    SkillInfo::insertSkill(new SkillJabInfo());
    SkillInfo::insertSkill(new SkillHookInfo());
    SkillInfo::insertSkill(new SkillSweepkickInfo());
    SkillInfo::insertSkill(new SkillTripInfo());
    SkillInfo::insertSkill(new SkillUppercutInfo());
    SkillInfo::insertSkill(new SkillGroinkickInfo());
    SkillInfo::insertSkill(new SkillClawInfo());
    SkillInfo::insertSkill(new SkillRabbitpunchInfo());
    SkillInfo::insertSkill(new SkillImpaleInfo());
    SkillInfo::insertSkill(new SkillPeleKickInfo());
    SkillInfo::insertSkill(new SkillLungePunchInfo());
    SkillInfo::insertSkill(new SkillTornadoKickInfo());
    SkillInfo::insertSkill(new SkillTripleAttackInfo());
    SkillInfo::insertSkill(new SkillCounterAttackInfo());
    SkillInfo::insertSkill(new SkillSwimmingInfo());
    SkillInfo::insertSkill(new SkillThrowingInfo());
    SkillInfo::insertSkill(new SkillRidingInfo());
    SkillInfo::insertSkill(new SkillAppraiseInfo());
    SkillInfo::insertSkill(new SkillPipemakingInfo());
    SkillInfo::insertSkill(new SkillSecondWeaponInfo());
    SkillInfo::insertSkill(new SkillScanningInfo());
    SkillInfo::insertSkill(new SkillRetreatInfo());
    SkillInfo::insertSkill(new SkillGunsmithingInfo());
    SkillInfo::insertSkill(new SkillPistolwhipInfo());
    SkillInfo::insertSkill(new SkillCrossfaceInfo());
    SkillInfo::insertSkill(new SkillWrenchInfo());
    SkillInfo::insertSkill(new SkillElusionInfo());
    SkillInfo::insertSkill(new SkillInfiltrateInfo());
    SkillInfo::insertSkill(new SkillShoulderThrowInfo());
    SkillInfo::insertSkill(new SkillProfPoundInfo());
    SkillInfo::insertSkill(new SkillProfWhipInfo());
    SkillInfo::insertSkill(new SkillProfPierceInfo());
    SkillInfo::insertSkill(new SkillProfSlashInfo());
    SkillInfo::insertSkill(new SkillProfCrushInfo());
    SkillInfo::insertSkill(new SkillProfBlastInfo());
    SkillInfo::insertSkill(new SkillGarotteInfo());
    SkillInfo::insertSkill(new SkillDemolitionsInfo());
    SkillInfo::insertSkill(new SkillReconfigureInfo());
    SkillInfo::insertSkill(new SkillRebootInfo());
    SkillInfo::insertSkill(new SkillMotionSensorInfo());
    SkillInfo::insertSkill(new SkillStasisInfo());
    SkillInfo::insertSkill(new SkillEnergyFieldInfo());
    SkillInfo::insertSkill(new SkillReflexBoostInfo());
    SkillInfo::insertSkill(new SkillImplantWInfo());
    SkillInfo::insertSkill(new SkillOffensivePosInfo());
    SkillInfo::insertSkill(new SkillDefensivePosInfo());
    SkillInfo::insertSkill(new SkillMeleeCombatTacInfo());
    SkillInfo::insertSkill(new SkillNeuralBridgingInfo());
    SkillInfo::insertSkill(new SkillNaniteReconstructionInfo());
    SkillInfo::insertSkill(new SkillOptimmunalRespInfo());
    SkillInfo::insertSkill(new SkillAdrenalMaximizerInfo());
    SkillInfo::insertSkill(new SkillPowerBoostInfo());
    SkillInfo::insertSkill(new SkillFastbootInfo());
    SkillInfo::insertSkill(new SkillSelfDestructInfo());
    SkillInfo::insertSkill(new SkillBioscanInfo());
    SkillInfo::insertSkill(new SkillDischargeInfo());
    SkillInfo::insertSkill(new SkillSelfrepairInfo());
    SkillInfo::insertSkill(new SkillCyborepairInfo());
    SkillInfo::insertSkill(new SkillOverhaulInfo());
    SkillInfo::insertSkill(new SkillDamageControlInfo());
    SkillInfo::insertSkill(new SkillElectronicsInfo());
    SkillInfo::insertSkill(new SkillHackingInfo());
    SkillInfo::insertSkill(new SkillCyberscanInfo());
    SkillInfo::insertSkill(new SkillCyboSurgeryInfo());
    SkillInfo::insertSkill(new SkillHyperscanningInfo());
    SkillInfo::insertSkill(new SkillOverdrainInfo());
    SkillInfo::insertSkill(new SkillEnergyWeaponsInfo());
    SkillInfo::insertSkill(new SkillProjWeaponsInfo());
    SkillInfo::insertSkill(new SkillSpeedLoadingInfo());
    SkillInfo::insertSkill(new SkillPalmStrikeInfo());
    SkillInfo::insertSkill(new SkillThroatStrikeInfo());
    SkillInfo::insertSkill(new SkillWhirlwindInfo());
    SkillInfo::insertSkill(new SkillHipTossInfo());
    SkillInfo::insertSkill(new SkillComboInfo());
    SkillInfo::insertSkill(new SkillDeathTouchInfo());
    SkillInfo::insertSkill(new SkillCraneKickInfo());
    SkillInfo::insertSkill(new SkillRidgehandInfo());
    SkillInfo::insertSkill(new SkillScissorKickInfo());
    SkillInfo::insertSkill(new SkillPinchAlphaInfo());
    SkillInfo::insertSkill(new SkillPinchBetaInfo());
    SkillInfo::insertSkill(new SkillPinchGammaInfo());
    SkillInfo::insertSkill(new SkillPinchDeltaInfo());
    SkillInfo::insertSkill(new SkillPinchEpsilonInfo());
    SkillInfo::insertSkill(new SkillPinchOmegaInfo());
    SkillInfo::insertSkill(new SkillPinchZetaInfo());
    SkillInfo::insertSkill(new SkillMeditateInfo());
    SkillInfo::insertSkill(new SkillKataInfo());
    SkillInfo::insertSkill(new SkillEvasionInfo());
    SkillInfo::insertSkill(new SkillFlyingInfo());
    SkillInfo::insertSkill(new SkillSummonInfo());
    SkillInfo::insertSkill(new SkillFeedInfo());
    SkillInfo::insertSkill(new SkillBeguileInfo());
    SkillInfo::insertSkill(new SkillDrainInfo());
    SkillInfo::insertSkill(new SkillCreateVampireInfo());
    SkillInfo::insertSkill(new SkillControlUndeadInfo());
    SkillInfo::insertSkill(new SkillTerrorizeInfo());
    SkillInfo::insertSkill(new SkillLectureInfo());
    SkillInfo::insertSkill(new SkillEnergyConversionInfo());
    SkillInfo::insertSkill(new SpellHellFireInfo());
    SkillInfo::insertSkill(new SpellHellFrostInfo());
    SkillInfo::insertSkill(new SpellHellFireStormInfo());
    SkillInfo::insertSkill(new SpellHellFrostStormInfo());
    SkillInfo::insertSkill(new SpellTrogStenchInfo());
    SkillInfo::insertSkill(new SpellFrostBreathInfo());
    SkillInfo::insertSkill(new SpellFireBreathInfo());
    SkillInfo::insertSkill(new SpellGasBreathInfo());
    SkillInfo::insertSkill(new SpellAcidBreathInfo());
    SkillInfo::insertSkill(new SpellLightningBreathInfo());
    SkillInfo::insertSkill(new SpellManaRestoreInfo());
    SkillInfo::insertSkill(new SpellPetrifyInfo());
    SkillInfo::insertSkill(new SpellSicknessInfo());
    SkillInfo::insertSkill(new SpellEssenceOfEvilInfo());
    SkillInfo::insertSkill(new SpellEssenceOfGoodInfo());
    SkillInfo::insertSkill(new SpellShadowBreathInfo());
    SkillInfo::insertSkill(new SpellSteamBreathInfo());
    SkillInfo::insertSkill(new SpellQuadDamageInfo());
    SkillInfo::insertSkill(new SpellZhengisFistOfAnnihilationInfo());
    SkillInfo::insertSkill(new SkillSnipeInfo());
    SkillInfo::insertSkill(new SkillAdvImplantWInfo());
    SkillInfo::insertSkill(new SongAriaOfAsylumInfo());
    SkillInfo::insertSkill(new SongLamentOfLongingInfo());
    SkillInfo::insertSkill(new SongWallOfSoundInfo());
    SkillInfo::insertSkill(new SkillLingeringSongInfo());
    SkillInfo::insertSkill(new SongFortissimoInfo());
    SkillInfo::insertSkill(new SongLichsLyricsInfo());
    SkillInfo::insertSkill(new SongMirrorImageMelodyInfo());
    SkillInfo::insertSkill(new SpellFireBreathingInfo());
    SkillInfo::insertSkill(new SpellVestigialRuneInfo());
    SkillInfo::insertSkill(new SpellPrismaticSphereInfo());
    SkillInfo::insertSkill(new SpellQuantumRiftInfo());
    SkillInfo::insertSkill(new SpellStoneskinInfo());
    SkillInfo::insertSkill(new SpellShieldOfRighteousnessInfo());
    SkillInfo::insertSkill(new SpellSunRayInfo());
    SkillInfo::insertSkill(new SpellTaintInfo());
    SkillInfo::insertSkill(new SpellBlackmantleInfo());
    SkillInfo::insertSkill(new SpellSanctificationInfo());
    SkillInfo::insertSkill(new SpellStigmataInfo());
    SkillInfo::insertSkill(new SpellDivinePowerInfo());
    SkillInfo::insertSkill(new SpellDeathKnellInfo());
    SkillInfo::insertSkill(new SpellManaShieldInfo());
    SkillInfo::insertSkill(new SpellEntangleInfo());
    SkillInfo::insertSkill(new SpellElementalBrandInfo());
    SkillInfo::insertSkill(new SpellSummonLegionInfo());
    SkillInfo::insertSkill(new SpellAntiMagicShellInfo());
    SkillInfo::insertSkill(new SpellWardingSigilInfo());
    SkillInfo::insertSkill(new SpellDivineInterventionInfo());
    SkillInfo::insertSkill(new SpellDivineInterventionInfo());
    SkillInfo::insertSkill(new SpellSphereOfDesecrationInfo());
    SkillInfo::insertSkill(new SpellSphereOfDesecrationInfo());
    SkillInfo::insertSkill(new SpellAmnesiaInfo());
    SkillInfo::insertSkill(new SpellPsychicFeedbackInfo());
    SkillInfo::insertSkill(new SpellIdInsinuationInfo());
    SkillInfo::insertSkill(new ZenTranslocationInfo());
    SkillInfo::insertSkill(new ZenCelerityInfo());
    SkillInfo::insertSkill(new SkillDisguiseInfo());
    SkillInfo::insertSkill(new SkillUncannyDodgeInfo());
    SkillInfo::insertSkill(new SkillDeEnergizeInfo());
    SkillInfo::insertSkill(new SkillAssimilateInfo());
    SkillInfo::insertSkill(new SkillRadionegationInfo());
    SkillInfo::insertSkill(new SpellMaleficViolationInfo());
    SkillInfo::insertSkill(new SpellRighteousPenetrationInfo());
    SkillInfo::insertSkill(new SkillWormholeInfo());
    SkillInfo::insertSkill(new SpellGaussShieldInfo());
    SkillInfo::insertSkill(new SpellDimensionalShiftInfo());
    SkillInfo::insertSkill(new SpellUnholyStalkerInfo());
    SkillInfo::insertSkill(new SpellAnimateDeadInfo());
    SkillInfo::insertSkill(new SpellInfernoInfo());
    SkillInfo::insertSkill(new SpellVampiricRegenerationInfo());
    SkillInfo::insertSkill(new SpellBanishmentInfo());
    SkillInfo::insertSkill(new SkillDisciplineOfSteelInfo());
    SkillInfo::insertSkill(new SkillGreatCleaveInfo());
    SkillInfo::insertSkill(new SpellLocustRegenerationInfo());
    SkillInfo::insertSkill(new SpellThornSkinInfo());
    SkillInfo::insertSkill(new SpellSpiritTrackInfo());
}
