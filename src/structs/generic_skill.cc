#include <algorithm>
#include <math.h>
#include "creature.h"
#include "generic_skill.h"
#include "creature_list.h"
#include "executable_object.h"
#include "generic_affect.h"
#include "spells.h"
#include "utils.h"
#include "char_class.h"
#include "comm.h"
#include "room_data.h"
#include "screen.h"
#include "smokes.h"
#include "player_table.h"
#include "custom_affects.h"
#include "tmpstr.h"
#include "vehicle.h"
#include "handler.h"
#include "fight.h"
#include "vendor.h"

extern void add_follower(struct Creature *ch, struct Creature *leader);
extern room_data *where_obj(struct obj_data *obj);
extern room_data *real_room(int vnum);
struct zone_data *real_zone(int number);

PracticeAbility PracticeAbility::CannotLearn(CANNOT_LEARN);
PracticeAbility PracticeAbility::CanLearn(CAN_LEARN);
PracticeAbility PracticeAbility::CanLearnLater(CAN_LEARN_LATER);
PracticeAbility PracticeAbility::WrongAlignment(WRONG_ALIGN);


int RAW_EQ_DAM(Creature *ch, int pos, int *var);

GenericSkill::GenericSkill() {
    _minMana = 0;
    _maxMana = 0;
    _manaChange = 0;
    _minMove = 0;
    _maxMove = 0;
    _moveChange = 0;
    _minHit = 0;
    _maxHit = 0;
    _hitChange = 0;
    _violent = 0;
    _minPosition = POS_DEAD;
    _flags = 0;
    _targets = 0;
    _skillNum = 0;
    _skillLevel = 0;
    _owner = NULL;
    _good = false;
    _evil = false;
    _noList = false;
    _powerLevel = 0;
    _objectMagic = false;
    _learnedPercent = 75;
    _minPercentGain = 15;
    _maxPercentGain = 45 ;
    _audible = false;
    _charWait = 2;
    _victWait = 0;
    _nonCombative = false;
    _lastUsed = 0;
    _cooldownTime = 0;
}

string
GenericSkill::getTypeDescription()
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

//just call perform on each element of the list
bool
GenericSkill::perform(ExecutableVector &ev, CreatureList &list) {
    if (!canPerform())
        return false;
   
    _lastUsed = time(NULL);

    CreatureList::iterator it;
    for (it = list.begin(); it != list.end(); ++it) {
        //any victim specific checks need to be done here 
        //and return false if they fail
        if (_owner && (targetIsSet(TAR_UNPLEASANT) || _violent) &&  
                _owner->checkReputations(*it)) {
            list.remove(*it);
            continue;
        }
        WaitObject *wo = new WaitObject(*it, _victWait);
        wo->setSkill(getSkillNumber());
        ev.push_back_id(wo);
    }

    if (!_objectMagic) {
        ev.push_front_id(generateCost());
        ev.push_back_id(generateWait());
    }

    return true;
}

bool
GenericSkill::perform(ExecutableVector &ev, Creature *target) {
    if (!canPerform())
        return false;
    
    if (_owner && (targetIsSet(TAR_UNPLEASANT) || _violent) && _owner->checkReputations(target))
        return false;

    _lastUsed = time(NULL);

    if (!_objectMagic) {
        ev.push_front_id(generateCost());
        ev.push_back_id(generateWait());
    }

    WaitObject *wo = new WaitObject(target, _victWait);
    wo->setSkill(getSkillNumber());
    ev.push_back_id(wo);

    return true;
}

bool
GenericSkill::perform(ExecutableVector &ev, obj_data *obj) {
    if (!canPerform())
        return false;
    
    _lastUsed = time(NULL);

    if (!_objectMagic) {
        ev.push_front_id(generateCost());
        ev.push_back_id(generateWait());
    }
    return true;
}

bool
GenericSkill::perform(ExecutableVector &ev, char *targets) {
    if (!canPerform())
        return false;

    _lastUsed = time(NULL);

    if (!_objectMagic) {
        ev.push_front_id(generateCost());
        ev.push_back_id(generateWait());
    }
    return true;
}

CostObject*
GenericSkill::generateCost() {
    CostObject *co = new CostObject(getOwner());
    co->setChar(getOwner());
    co->setMove(getMoveCost());
    co->setMana(getManaCost());
    co->setHit(getHitCost());
    co->setSkill(getSkillNumber());
    return co;
}

WaitObject*
GenericSkill::generateWait() {
    WaitObject *wo = new WaitObject(_owner, _charWait);
    wo->setSkill(getSkillNumber());
    return wo;
}

bool
GenericSkill::canPerform() {
    if (!_owner)
        return false;

    if (_objectMagic)
        return true;

    if (_owner->isImmortal())
        return true;

    if (_lastUsed) {
        time_t now = time(NULL);

        if (now <= _lastUsed + _cooldownTime) {
            send_to_char(_owner, tmp_sprintf("You must wait longer before you can use "
                        "that %s again.\r\n", getTypeDescription().c_str()));
            return false;
        }
    }

    if (canBeLearned(_owner) == PracticeAbility::CanLearnLater) {
        send_to_char(_owner, tmp_sprintf("You are not yet powerful "
            "enough to use that %s.\r\n",
        getTypeDescription().c_str()));
        return false;
    }

    if (canBeLearned(_owner) == PracticeAbility::WrongAlignment) {
        send_to_char(_owner, "You have no business dealing in "
            "such arts!.\r\n");
        return false;
    }

    if (flagIsSet(MAG_GROUPS) && !IS_AFFECTED(_owner, AFF_GROUP)) {
        send_to_char(_owner, "You can't do that if you're not in a group!\r\n");
        return false;
    }

    if (_owner->getPosition() < _minPosition) {
        switch (_owner->getPosition()) {
            case POS_SLEEPING:
                send_to_char(_owner, "You dream about amazing powers.\r\n");
                break;
            case POS_RESTING:
                send_to_char(_owner, "You can't do that while resting!\r\n");
                break;
            case POS_SITTING:
                send_to_char(_owner, "You can't do that while sitting!\r\n");
                break;
            default:
                send_to_char(_owner, "You can't do much of anything "
                        "like this!\r\n");
                break;
        }
        return false;
    }

    if (_nonCombative && _owner->numCombatants()) {
        send_to_char(_owner, "Impossible!  You can't concentrate enough!\r\n");
        if (_owner->isNPC()) {
            errlog("%s tried to use skill %d in battle.",
                    _owner->getName(), _skillNum);
        }

        return false;
    }

    return true;
}

//This is how we make new objects of the appropriate type without
//knowing the right type in advance
GenericSkill *
GenericSkill::constructSkill(int skillNum) {
    if (skillNum <= 0)
        return NULL;

    if (!skillExists(skillNum))
        return NULL;

    return (*_getSkillMap())[skillNum]->createNewInstance();
}

//this should never be called from this function.
GenericSkill* 
GenericSkill::createNewInstance() {
    return NULL;
}

PracticeAbility
GenericSkill::canBeLearned(Creature *ch) const {
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

//returns 0 if never
int
GenericSkill::getLevel(int charClass) const {
    for (unsigned int i = 0; i < _practiceInfo.size(); i++) {
        if (_practiceInfo[i].allowedClass == charClass)
            return _practiceInfo[i].minLevel;
    }
    return 0;
}

//returns 0 if never
int
GenericSkill::getGen(int charClass) const {
    for (unsigned int i = 0; i < _practiceInfo.size(); i++) {
        if (_practiceInfo[i].allowedClass == charClass)
            return _practiceInfo[i].minGen;
    }
    return 0;
}


bool
GenericSkill::hasBeenLearned() {
    if (!getOwner()) {
        return false;
    } 
    else if (getSkillLevel() >= (_learnedPercent + getOwner()->getGen() * 2.5)) {
        return true;
    }
    return false;
}

void
GenericSkill::gainSkillProficiency() {
    if (!_owner)
        return;

    if (_owner->isNPC())
        return;

    if (!canBeLearned(_owner))
        return;

    if (_skillLevel >= MIN(125, _owner->getLevelBonus(_skillNum) + 20))
        return;
   
    // If we pass all of the checks above then we shall succeed in
    // getting better at the skill 1% of the time.  May be a little
    // harsh, but for now we'll see how it goes.
    if (random_fractional_100())
        return;

    _skillLevel++;
}

bool 
GenericSkill::extractTrainingCost() {
    char tzone = _owner->in_room->zone->time_frame == TIME_ELECTRO ? 1 : 0;
    int cost = getTrainingCost();

    if (tzone) {
        if (GET_CASH(_owner) < cost)
            return false;
        GET_CASH(_owner) -= cost;
    }
    else {
        if (GET_GOLD(_owner) < cost)
            return false;
        GET_GOLD(_owner) -= cost;
    }

    return true;
}

int
GenericSkill::getTrainingCost() {
    short skill_lvl = 0;
    short skill_gen = 0;
    int cost;

    for (unsigned int i = 0; i < _practiceInfo.size(); i++) {
        if (_owner->getPrimaryClass() == _practiceInfo[i].allowedClass) {
            skill_lvl = _practiceInfo[i].minLevel;
            skill_gen = _practiceInfo[i].minGen;
            break;
        }
        if (_owner->getSecondaryClass() == _practiceInfo[i].allowedClass
            && _practiceInfo[i].minGen <= 0)
            skill_lvl = _practiceInfo[i].minLevel;
    }

    cost = skill_lvl * skill_lvl * 100 * (skill_gen + 1);

    return cost;
}

void
GenericSkill::learn() {
    WAIT_STATE(getOwner(), 2 RL_SEC);

    int percent = getSkillLevel();
    int gain_amt = number(_minPercentGain + (getOwner()->getGen() * 2), 
            _maxPercentGain + getOwner()->getGen());

    // This is the limit of an unsigned char.  We've been having
    // problems for ages with this rolling over during training.
    // This should fix the issue though.
    if ((percent + gain_amt) > 125)
        percent = 125;
    else
        percent += gain_amt;

    if (percent > _learnedPercent + (getOwner()->getGen() * 2.5))
        percent -= (percent - _learnedPercent) >> 1;

    setSkillLevel(percent);
}

bool
GenericSkill::targetIsSet(int flag) {
    return _targets & flag;
}

bool
GenericSkill::flagIsSet(int flag) {
    return _flags & flag;
}

bool
GenericSkill::operator==(const GenericSkill *skill) const {
    return this->getSkillNumber() == skill->getSkillNumber();
}

bool
GenericSkill::operator<(const GenericSkill &c) const {
    return _skillNum < c.getSkillNumber();
}

string
GenericSkill::getNameByNumber(int skillNum) {
    if (!skillExists(skillNum))
        return "!UNKNOWN!";

    return getSkill(skillNum)->getName();
}

unsigned short
GenericSkill::getNumberByName(const char *name) {
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
GenericSkill::saveToXML(FILE *out) {
    fprintf(out, "<skill num=\"%d\" level=\"%d\" last_used=\"%d\"/>\n",
            getSkillNumber(), getSkillLevel(), (int)_lastUsed);
}

GenericSkill *
GenericSkill::loadFromXML(xmlNode *node) {
    int skillNum = xmlGetIntProp(node, "num");
    int skillLevel = xmlGetIntProp(node, "level");
    time_t lastUsed = xmlGetIntProp(node, "last_used");

    if (!GenericSkill::skillExists(skillNum))
        return NULL;

    GenericSkill* skill = GenericSkill::constructSkill(skillNum);
    if (skill) {
        skill->setSkillLevel(skillLevel);
        skill->setLastUsedTime(lastUsed);
        return skill;

    }

    return NULL;
}

unsigned short
GenericSkill::getManaCost() {
    short cost;

    if (!_owner)
        return _maxMana;

    cost = _maxMana - ((_owner->getLevel() - 1) * _manaChange);

    return (unsigned short)MAX(cost, _minMana);
}

unsigned short
GenericSkill::getMoveCost() {
    short cost;

    if (!_owner)
        return _maxMove;

    cost = _maxMove - ((_owner->getLevel() - 1) * _moveChange);

    return (unsigned short)MAX(cost, _minMove);
}

unsigned short
GenericSkill::getHitCost() {
    short cost;

    if (!_owner)
        return _maxHit;

    cost = _maxHit - ((_owner->getLevel() - 1) * _hitChange);

    return (unsigned short)MAX(cost, _minHit);
}

bool 
GenericSkill::isDivine(int skillNum) { 
    if (!skillExists(skillNum))
        return false;

    return getSkill(skillNum)->isDivine();
}

bool 
GenericSkill::isArcane(int skillNum) { 
    if (!skillExists(skillNum))
        return false;

    return getSkill(skillNum)->isArcane();
}

bool 
GenericSkill::isPhysics(int skillNum) { 
    if (!skillExists(skillNum))
        return false;

    return getSkill(skillNum)->isPhysics();
}

bool 
GenericSkill::isPsionic(int skillNum) { 
    if (!skillExists(skillNum))
        return false;

    return getSkill(skillNum)->isPsionic();
}

bool 
GenericSkill::isSong(int skillNum) { 
    if (!skillExists(skillNum))
        return false;

    return getSkill(skillNum)->isSong();
}

bool 
GenericSkill::isZen(int skillNum) { 
    if (!skillExists(skillNum))
        return false;

    return getSkill(skillNum)->isZen();
}

bool 
GenericSkill::isProgram(int skillNum) { 
    if (!skillExists(skillNum))
        return false;

    return getSkill(skillNum)->isProgram();
}

bool 
GenericSkill::isAttackType(int skillNum) {
    if (!skillExists(skillNum))
        return false;

    return getSkill(skillNum)->isAttackType();
}

void 
GenericSkill::insertSkill(GenericSkill *skill) {
    (*_getSkillMap())[skill->getSkillNumber()]=skill;
}

SkillMap *
GenericSkill::getSkillMap() {
    return _getSkillMap();
}

GenericSkill const *
GenericSkill::getSkill(int skillNum) {
    if (!skillExists(skillNum))
        return NULL;

    return (*_getSkillMap())[skillNum];
}

GenericSkill const *
GenericSkill::getSkill(string skillName) {
    unsigned short skillNum = getNumberByName(skillName.c_str());

    if (skillNum > 0)
        return getSkill(skillNum);
    
    return NULL;
}

unsigned short
GenericSkill::getPowerLevel() {
    if (_powerLevel)
        return _powerLevel;

    if (!_owner)
        return 0;

    return _owner->getLevelBonus(_skillNum);
}

void
GenericSkill::setPowerLevel(unsigned short level) {
    _objectMagic = true;

    // Items simply cannot cast at higher than a mort level.
    _powerLevel = MIN(level, 49);
}

const PracticeInfo *
GenericSkill::getPracticeInfo(int classNum) const {
    for (unsigned x = 0; x < _practiceInfo.size(); x++) {
        if (_practiceInfo[x].allowedClass == classNum)
            return &_practiceInfo[x];
    }

    return NULL;
}

// ****************************************************************
//
// SKILL IMPLEMENTATIONS START HERE
//
// ****************************************************************

class SpellAirWalk : public DivineSpell
{
    public:
        SpellAirWalk() : DivineSpell()
        {
            _skillNum = SPELL_AIR_WALK;
            _skillName = "air walk";
            _minMana = 40;
            _maxMana = 60;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_CLERIC, 32, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellAirWalk();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellAblaze : public GenericSkill
{
    public:
        SpellAblaze() : GenericSkill()
        {
            _skillNum = SPELL_ABLAZE;
            _skillName = "!ABLAZE!";
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellAirWalk();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            return false;

        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            return false;
        }
};

class SpellArmor : public ArcaneSpell
{
    public:
        SpellArmor() : ArcaneSpell()
        {
            _skillNum = SPELL_ARMOR;
            _skillName = "armor";
            _minMana = 15;
            _maxMana = 45;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_MAGE, 4, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellArmor();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;

            string toVictMessage = "You feel someone protecting you.";

            int level = getPowerLevel();
            AffectObject *ao = new AffectObject("CreatureAffect", 
                    _owner, target);
            CreatureAffect *af = (CreatureAffect *)ao->getAffect();
            af->setSkillNum(SPELL_ARMOR);
            af->setWearoffToChar("You feel less protected.");
            af->setLevel(level);
            af->setDuration(((level >> 3) + 5) MUD_HOURS);

            int modifier = -(af->getLevel() * 60 / 100);
            af->addApply(APPLY_AC, modifier);
            af->setAccumDuration(true);
            ao->addToVictMessage(toVictMessage);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SpellAstralSpell : public ArcaneSpell
{
    public:
        SpellAstralSpell() : ArcaneSpell()
        {
            _skillNum = SPELL_ASTRAL_SPELL;
            _skillName = "astral spell";
            _minMana = 105;
            _maxMana = 175;
            _manaChange = 8;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_MAGE, 43, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 47, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_PSIONIC, 45, 0};
            _practiceInfo.push_back(p2);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellAstralSpell();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev) {
            return false;
        }
};

class SpellControlUndead : public DivineSpell
{
    public:
        SpellControlUndead() : DivineSpell()
        {
            _skillNum = SPELL_CONTROL_UNDEAD;
            _skillName = "control undead";
            _minMana = 20;
            _maxMana = 50;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _violent = true;
            _evil = true;
            _targets = TAR_CHAR_ROOM | TAR_NOT_SELF;
            PracticeInfo p0 = {CLASS_CLERIC, 26, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellControlUndead();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellTeleport : public ArcaneSpell
{
    public:
        SpellTeleport() : ArcaneSpell()
        {
            _skillNum = SPELL_TELEPORT;
            _skillName = "teleport";
            _minMana = 50;
            _maxMana = 75;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM | TAR_OBJ_ROOM | TAR_OBJ_INV;
            PracticeInfo p0 = {CLASS_MAGE, 33, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellTeleport();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            TeleportObject *to = new TeleportObject(getOwner(), TeleportObject::Teleport);

            to->setChar(_owner);
            to->setVictim(target);

            to->setSkill(getSkillNumber());
            ev.push_back_id(to);
            return true;
        }

        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            TeleportObject *to = 
                new TeleportObject(getOwner(), TeleportObject::VStone);

            to->setObject(obj);

            to->setSkill(getSkillNumber());
            ev.push_back_id(to);
            return true;
        }
};

class SpellLocalTeleport : public ArcaneSpell
{
    public:
        SpellLocalTeleport() : ArcaneSpell()
        {
            _skillNum = SPELL_LOCAL_TELEPORT;
            _skillName = "local teleport";
            _minMana = 30;
            _maxMana = 45;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_MAGE, 18, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellLocalTeleport();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;

            TeleportObject *to = 
                new TeleportObject(getOwner(), TeleportObject::LocalTeleport);

            to->setVictim(target);

            to->setSkill(getSkillNumber());
            ev.push_back_id(to);
            return true;
        }
};

class SpellBlur : public ArcaneSpell
{
    public:
        SpellBlur() : ArcaneSpell()
        {
            _skillNum = SPELL_BLUR;
            _skillName = "blur";
            _minMana = 20;
            _maxMana = 45;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP;
            PracticeInfo p0 = {CLASS_MAGE, 13, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellBlur();
        }


        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;
            
            int level = getPowerLevel();

            if (IS_AFFECTED(target, AFF_BLUR)) {
                ExecutableObject *eo = new ExecutableObject(getOwner());
                eo->setVictim(target);

                if (target != _owner)
                    eo->addToCharMessage("Nothing seems to happen.");

                eo->addToVictMessage("Nothing seems to happen.");
                eo->addToRoomMessage("Nothing seems to happen.");

                eo->setSkill(getSkillNumber());
                ev.push_back_id(eo);

                return true;
            }

            AffectObject *ao = new AffectObject("BlurAffect", _owner, target);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();

            ca->setDuration((2 + (level >> 2)) MUD_HOURS);
            ca->setLevel(getPowerLevel());

            // Use setApply() here instead of addApply() to override
            // the APPLY_AC in the constructor.  See the code for addApply()
            // if you have questions.
            ca->setApply(APPLY_AC, -(level / 5));
            if (target != _owner)
                ao->addToCharMessage("The image of $N suddenly starts to blur and shift.");
            ao->addToVictMessage("Your image suddenly starts to blur "
                    "and shift.");
            ao->addToRoomMessage("The image of $N suddenly starts to blur and shift.");

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);
            return true;
        }

        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev, obj))
                return false;

            ModifyItemObject *mio = new ModifyItemObject(getOwner(), obj);
            mio->setSkill(getSkillNumber());
            ev.push_back_id(mio);

            if (IS_OBJ_STAT(obj, ITEM_BLURRED)) {
                mio->addToCharMessage("Nothing seems to happen.");
                mio->addToRoomMessage("Nothing seems to happen.");
            }
            else {
                mio->setExtraBits(0, ITEM_BLURRED);
                mio->addToCharMessage("The image of $p becomes blurred.");
                mio->addToRoomMessage("The image of $p becomes blurred.");
            }

            return true;
        }
};

class SpellBless : public DivineSpell
{
    public:
        SpellBless() : DivineSpell()
        {
            _skillNum = SPELL_BLESS;
            _skillName = "bless";
            _minMana = 15;
            _maxMana = 35;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _good = true;
            _targets = TAR_CHAR_ROOM | TAR_OBJ_INV;
            PracticeInfo p0 = {CLASS_CLERIC, 7, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_PALADIN, 10, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellBless();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellDamn : public DivineSpell
{
    public:
        SpellDamn() : DivineSpell()
        {
            _skillNum = SPELL_DAMN;
            _skillName = "damn";
            _minMana = 15;
            _maxMana = 35;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _violent = true;
            _evil = true;
            _targets = TAR_CHAR_ROOM | TAR_OBJ_INV;
            PracticeInfo p0 = {CLASS_BLACKGUARD, 10, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellDamn();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellCalm : public DivineSpell
{
    public:
        SpellCalm() : DivineSpell()
        {
            _skillNum = SPELL_CALM;
            _skillName = "calm";
            _minMana = 15;
            _maxMana = 35;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PALADIN, 36, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellCalm();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellBlindness : public ArcaneSpell
{
    public:
        SpellBlindness() : ArcaneSpell()
        {
            _skillNum = SPELL_BLINDNESS;
            _skillName = "blindness";
            _minMana = 25;
            _maxMana = 35;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_NOT_SELF;
            PracticeInfo p0 = {CLASS_MAGE, 16, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellBlindness();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;

            // FIXME: Eventually we should subclass affects for all mob flags
            // and put them on permenantly when the flag is set in the
            // file.  However, I'm not dealing with that right now and
            // this works in the short term.  It's easy enough to make
            // that change later.
            if (MOB_FLAGGED(target, MOB_NOBLIND)) {
                ExecutableObject *eo = new ExecutableObject(getOwner());
                eo->addToCharMessage("You fail.");
                eo->setSkill(getSkillNumber());
                ev.push_back_id(eo);
                return true;
            }

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("BlindnessAffect", _owner,
                    target);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setLevel(level);
            ca->setApply(APPLY_AC, level / 2);
            ca->setApply(APPLY_HITROLL, -(level / 12));

            ao->addToCharMessage("$N has been struck blind!");
            ao->addToVictMessage("You have been struck blind!");
            ao->addToRoomMessage("$n has been struck blind!");
            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SpellBreatheWater : public ArcaneSpell
{
    public:
        SpellBreatheWater() : ArcaneSpell()
        {
            _skillNum = SPELL_BREATHE_WATER;
            _skillName = "breathe water";
            _minMana = 20;
            _maxMana = 45;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_MAGE, 40, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellBreatheWater();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellBurningHands : public ArcaneSpell
{
    public:
        SpellBurningHands() : ArcaneSpell()
        {
            _skillNum = SPELL_BURNING_HANDS;
            _skillName = "burning hands";
            _minMana = 10;
            _maxMana = 30;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            _flags = MAG_NOWATER | MAG_TOUCH;
            PracticeInfo p0 = {CLASS_MAGE, 11, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellBurningHands();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            DamageObject *do1 = new DamageObject(getOwner(), target);
            int dam = dice(6, level) + (_owner->getInt() << 1);
            do1->setDamage(dam);
            do1->setDamageType(SPELL_BURNING_HANDS);

            string toChar = "$N screams as your burning hands grab $M.";
            string toVict = "You scream as $n's burning hands grab you.";
            string toRoom = "$N screams as $n's burning hands grab $M.";

            do1->addToCharMessage(toChar);
            do1->addToVictMessage(toVict);
            do1->addToRoomMessage(toRoom);

            AffectObject *ao = new AffectObject("AblazeAffect", _owner, target);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setLevel(level);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);
            do1->setSkill(getSkillNumber());
            ev.push_back_id(do1);

            return true;
        }
};

class SpellCallLightning : public DivineSpell
{
    public:
        SpellCallLightning() : DivineSpell()
        {
            _skillNum = SPELL_CALL_LIGHTNING;
            _skillName = "call lightning";
            _minMana = 25;
            _maxMana = 40;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_CLERIC, 25, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_RANGER, 30, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellCallLightning();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellEnvenom : public ArcaneSpell
{
    public:
        SpellEnvenom() : ArcaneSpell()
        {
            _skillNum = SPELL_ENVENOM;
            _skillName = "envenom";
            _minMana = 10;
            _maxMana = 25;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _targets = TAR_OBJ_EQUIP | TAR_OBJ_ROOM | TAR_OBJ_INV;
            PracticeInfo p0 = {CLASS_RANGER, 5, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellEnvenom();
        }

        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev, obj))
                return false;

            if (!(GET_OBJ_TYPE(obj) == ITEM_WEAPON)) {
                if (!_owner)
                    return false;
                ExecutableObject *eo = new ExecutableObject(_owner);
                eo->addToCharMessage("You can only envenom weapons.");
                return false;
            }

            if (obj->affectedBy(_skillNum)) {
                if (!_owner)
                    return false;
                ExecutableObject *eo = new ExecutableObject(_owner);
                eo->addToCharMessage("That weapon is already envenomed!");
                return false;
            }

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("ObjectAffect", _owner, obj);
            ObjectAffect *aff = (ObjectAffect *)ao->getAffect();
            aff->setLevel(level);
            aff->setSkillNum(_skillNum);
            aff->setDuration((level >> 4) MUD_HOURS);
            aff->setValMod(0, SPELL_POISON - GET_OBJ_VAL(obj, 0));
            /*
            aff->setValMod(1, 3);
            aff->setValMod(2, 4);
            aff->setValMod(3, 1);
            aff->setWornMod(ITEM_WEAR_ASS);
            aff->setBitvector(2, ITEM3_REQ_RANGER);
            aff->setApply(APPLY_HITROLL, 10);
            aff->setApply(APPLY_WIS, 10);
            aff->setWeightMod(-5);
            aff->setDamMod(5000);
            aff->setMaxDamMod(5000);
            */
            aff->setBitvector(0, ITEM_MAGIC);
            aff->setBitvector(1, ITEM2_CAST_WEAPON);
            aff->setApply(APPLY_DAMROLL, 10);
            aff->setWearoffToChar("$p ceases to drip poison.");

            ao->setSkill(_skillNum);
            ao->addToCharMessage("$p begins to drip poison.");
            ev.push_back_id(ao);

            return true;
        }
};

class SpellCharm : public ArcaneSpell
{
    public:
        SpellCharm() : ArcaneSpell()
        {
            _skillNum = SPELL_CHARM;
            _skillName = "charm person";
            _minMana = 50;
            _maxMana = 75;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _violent = false;
            _targets = TAR_CHAR_ROOM | TAR_NOT_SELF;
            PracticeInfo p0 = {CLASS_MAGE, 22, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellCharm();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;

            if (!_owner || !target)
                return false;

            int level = getPowerLevel();

            if (ROOM_FLAGGED(_owner->in_room, ROOM_ARENA) &&
                !PRF_FLAGGED(_owner, PRF_NOHASSLE)) {
                send_to_char(_owner, "You cannot charm people in "
                        "arena rooms.\r\n");
                return false;
            }

            if (target == _owner) {
                send_to_char(_owner, "You like yourself even better!\r\n");
                return false;
            }

            if (!target->isNPC() && !(target->desc)) {
                send_to_char(_owner, "You cannot charm linkess players!\r\n");
                send_to_char(_owner, "A large hand suddenly appears from "
                        "the sky and backhands you!\r\n");
                if (!_owner->affectedBy(SPELL_CHARM)) {
                    _owner->setMaxHit(_owner->getMaxHit() - 20);
                    _owner->setHit(_owner->getMaxHit());
                }
                return false;
            }

            if (!can_charm_more(_owner)) {
                send_to_char(_owner, "Your victim resists!\r\n");
                if (MOB_FLAGGED(target, MOB_MEMORY))
                    remember(target, _owner);
                return false;
            }

            if (circle_follow(target, _owner)) {
                send_to_char(_owner, "Following in circles is not allowed.\r\n");
                return false;
            }

             if (!ok_damage_vendor(_owner, target)) {
                 AffectObject *ao = 
                     new AffectObject("CharmAffect", target, _owner);
                 CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
                 ca->setLevel(target->getLevelBonus(SPELL_CHARM));
                 ao->addToCharMessage("$n falls down laughing at you!");
                 ao->addToCharMessage("$n peers deeply into your eyes...");
                 ao->addToRoomMessage("$n falls down laughing at $N!");
                 ao->addToRoomMessage("$n peers deeply into $N's eyes...");
                 ao->addToCharMessage("Isn't $n just such a great friend?");
                 ao->setModifiable(false);
                 if (_owner->master)
                     stop_follower(_owner);
                 add_follower(_owner, target);
                 ev.push_back_id(ao);
                 return false;
             }

             // FIXME:
             // Sanctuary's modifyEO() should prevent charm
             // Mob's permenant NOCHARM flag should prevent charm
             // CharmAffects modifyEO() should prevent a charmed person from
             // charming others.
             // Elves/Drow should have a chance to resist via racial modifyEO()
             // Charming undead should always fail via racial modifyEO()
             // Charming animals should always fail via racial modifyEO() 
                        
             if (target->isImmortal()
                 && _owner->getLevel() < target->getLevel()) {
                 ExecutableObject *eo = new ExecutableObject(getOwner());
                 eo->setVictim(target);
                 eo->addToCharMessage("$N sneers at you with disgust.\r\n");
                 eo->addToVictMessage("You sneer at $n with disgust.\r\n");
                 eo->addToRoomMessage("$N sneers at $n with disgust.\r\n");
                 ev.push_back_id(eo);
                 eo->setSkill(SPELL_CHARM);
                 return false;
             }

             AffectObject *ao = new AffectObject("CharmAffect", _owner, target);
             CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
             ca->setLevel(level);
             ca->setDuration((_owner->getLevel() * 18 / 1 + target->getInt()) MUD_HOURS);
             if (target->master)
                 stop_follower(target);
             add_follower(target, _owner);
             ev.push_back_id(ao);

             ao->addToVictMessage("Isn't $n just such a nice person?");
             ao->addToCharMessage("$N looks at you with adoration.");
             ao->addToRoomMessage("$N looks at $n with adoration.");

             ao->setSkill(SPELL_CHARM);

             // FIXME: This REALLY needs to be done through an EO
             if (target->isNPC()) {
                 REMOVE_BIT(MOB_FLAGS(target), MOB_AGGRESSIVE);
                 REMOVE_BIT(MOB_FLAGS(target), MOB_SPEC);
             }

             return true;
        }
};

class SpellCharmAnimal : public DivineSpell
{
    public:
        SpellCharmAnimal() : DivineSpell()
        {
            _skillNum = SPELL_CHARM_ANIMAL;
            _skillName = "charm animal";
            _minMana = 50;
            _maxMana = 75;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_NOT_SELF;
            PracticeInfo p0 = {CLASS_RANGER, 23, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellCharmAnimal();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellChillTouch : public ArcaneSpell
{
    public:
        SpellChillTouch() : ArcaneSpell()
        {
            _skillNum = SPELL_CHILL_TOUCH;
            _skillName = "chill touch";
            _minMana = 10;
            _maxMana = 30;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            _flags = MAG_TOUCH;
            PracticeInfo p0 = {CLASS_MAGE, 5, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellChillTouch();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;
            
            int level = getPowerLevel();

            AffectObject *ao = 
                new AffectObject("CreatureAffect", _owner, target);
            ao->addToVictMessage("You feel your strength wither!");

            // Set up the affect
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(SPELL_CHILL_TOUCH);
            ca->setWearoffToChar("You feel your strength return.");
            ca->setLevel(level);
            ca->setDuration(4 MUD_HOURS);
            int modifier  = -((ca->getLevel() >> 4) + 2);
            ca->addApply(APPLY_STR, modifier);
            ca->setAccumDuration(true);
            
            // Set up the damage object
            DamageObject *do1 = new DamageObject(getOwner(), target);
            int dam = dice(3, level) + (_owner->getInt() << 1);
            do1->setDamage(dam);
            do1->setDamageType(SPELL_CHILL_TOUCH);

            string toChar = "You chill $N with your icy touch!";
            string toVict = "$n chills you with $s icy touch!";
            string toRoom = "$n chills $N with $s icy touch!";

            toChar = CCCYN(_owner, C_NRM) + toChar + CCNRM(_owner, C_NRM);
            do1->addToCharMessage(toChar);
            do1->addToVictMessage(toVict);
            do1->addToRoomMessage(toRoom);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);
            do1->setSkill(getSkillNumber());
            ev.push_back_id(do1);

            return true;
        }
};

class SpellClairvoyance : public ArcaneSpell
{
    public:
        SpellClairvoyance() : ArcaneSpell()
        {
            _skillNum = SPELL_CLAIRVOYANCE;
            _skillName = "clairvoyance";
            _minMana = 60;
            _maxMana = 90;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_WORLD;
            PracticeInfo p0 = {CLASS_MAGE, 35, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellClairvoyance();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellCallRodent : public ArcaneSpell
{
    public:
        SpellCallRodent() : ArcaneSpell()
        {
            _skillNum = SPELL_CALL_RODENT;
            _skillName = "call rodent";
            _minMana = 60;
            _maxMana = 90;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_RANGER, 15, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellCallRodent();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellCallBird : public ArcaneSpell
{
    public:
        SpellCallBird() : ArcaneSpell()
        {
            _skillNum = SPELL_CALL_BIRD;
            _skillName = "call bird";
            _minMana = 60;
            _maxMana = 90;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_RANGER, 22, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellCallBird();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellCallReptile : public ArcaneSpell
{
    public:
        SpellCallReptile() : ArcaneSpell()
        {
            _skillNum = SPELL_CALL_REPTILE;
            _skillName = "call reptile";
            _minMana = 60;
            _maxMana = 90;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_RANGER, 29, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellCallReptile();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellCallBeast : public ArcaneSpell
{
    public:
        SpellCallBeast() : ArcaneSpell()
        {
            _skillNum = SPELL_CALL_BEAST;
            _skillName = "call beast";
            _minMana = 60;
            _maxMana = 90;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_RANGER, 35, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellCallBeast();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellCallPredator : public ArcaneSpell
{
    public:
        SpellCallPredator() : ArcaneSpell()
        {
            _skillNum = SPELL_CALL_PREDATOR;
            _skillName = "call predator";
            _minMana = 60;
            _maxMana = 90;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_RANGER, 41, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellCallPredator();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellClone : public ArcaneSpell
{
    public:
        SpellClone() : ArcaneSpell()
        {
            _skillNum = SPELL_CLONE;
            _skillName = "clone";
            _minMana = 65;
            _maxMana = 80;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellClone();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellColorSpray : public ArcaneSpell
{
    public:
        SpellColorSpray() : ArcaneSpell()
        {
            _skillNum = SPELL_COLOR_SPRAY;
            _skillName = "color spray";
            _minMana = 180;
            _maxMana = 250;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _violent = true;
            _flags = MAG_MASSES;
            PracticeInfo p0 = {CLASS_MAGE, 18, 0};
            _practiceInfo.push_back(p0);
            _charWait = 5;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellColorSpray();
        }

        bool perform(ExecutableVector &ev, CreatureList &clist) {
            if (!ArcaneSpell::perform(ev, clist))
                return false;

            int level = getPowerLevel();
            char num = 1 + level / 10;
            int count = 0;
            Creature *opponent = NULL;
            AffectObject *ao;

            ExecutableObject *eo = new ExecutableObject(getOwner());
            
            eo->addToCharMessage("A spiraling cone of color erupts from "
                    "your outstretched hand!");
            eo->addToRoomMessage("A spiraling cone of color erupts from "
                    "$n's outstretched hand!");
            eo->setSkill(getSkillNumber());
            ev.push_back_id(eo);

            CreatureList::iterator cit = clist.begin();
            for (; cit != clist.end(); ++cit) {
                if (count >= num)
                    break;

                opponent = (*cit);

                if (IS_UNDEAD(opponent))
                    continue;

                if (random_fractional_20()) {
                    count++;
                    ao = new AffectObject("BlindnessAffect", _owner, opponent);
                    CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
                    ca->setLevel(level);
                    ca->setApply(APPLY_AC, level / 2);
                    ca->setApply(APPLY_HITROLL, -(level / 12));

                    ao->addToCharMessage("$N has been struck blind!");
                    ao->addToVictMessage("You have been struck blind!");
                    ao->addToRoomMessage("$n has been struck blind!");
                    ao->setSkill(getSkillNumber());
                    ev.push_back_id(ao);
                    continue;
                }

                if ((((level / 10) + 
                    number(1, _owner->getInt() - 5)) >= opponent->getDex())
                    && random_fractional_4()) {
                    count++;
                    StopCombatObject *sco = 
                        new StopCombatObject(getOwner(), opponent);
                    ev.push_back_id(sco);
                    
                    PositionObject *eo = 
                        new PositionObject(_owner, opponent, POS_STUNNED);
                    
                    eo->addToCharMessage("Your color spray hits $N who "
                            "falls stunned to the ground.");
                    eo->addToVictMessage("$n's color spray hits you and "
                            "the lights go out...");
                    eo->addToRoomMessage("$N spins stunned to the floor");
                    eo->setSkill(getSkillNumber());
                    ev.push_back_id(eo);

                    continue;
                }

                if ((((level / 10) + 
                    number(1, _owner->getInt() - 10)) >= opponent->getCon())
                    && random_fractional_5()) {
                    count++;
                    StopCombatObject *sco = 
                        new StopCombatObject(getOwner(), opponent);
                    ev.push_back_id(sco);
                    AffectObject *ao = new AffectObject("CreatureAffect", 
                            _owner, opponent);
                    CreatureAffect *af = (CreatureAffect *)ao->getAffect();
                    af->setSkillNum(SPELL_SLEEP);
                    af->setWearoffToChar("You feel less tired.");
                    af->setLevel(level);
                    af->setDuration((1 + level / 10) MUD_HOURS);
                    af->setBitvector(0, AFF_SLEEP);
                    af->setPosition(POS_SLEEPING);

                    ao->addToCharMessage("$N flies across the room and lands "
                            "in a crumpled heap!");
                    ao->addToVictMessage("You fly across the room and land "
                            "in a crumpled heap!");
                    ao->addToRoomMessage("$N flies across the room and lands "
                            "in a crumpled heap!");
                    ao->setSkill(getSkillNumber());
                    ev.push_back_id(ao);
                    
                    continue;
                }
            }

            if (count == 0)
                eo->addToCharMessage("Nothing seems to happen.");

            return true;
        }
};

class SpellCommand : public DivineSpell
{
    public:
        SpellCommand() : DivineSpell()
        {
            _skillNum = SPELL_COMMAND;
            _skillName = "command";
            _minMana = 35;
            _maxMana = 50;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_NOT_SELF;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellCommand();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellConeOfCold : public ArcaneSpell
{
    public:
        SpellConeOfCold() : ArcaneSpell()
        {
            _skillNum = SPELL_CONE_COLD;
            _skillName = "cone of cold";
            _minMana = 55;
            _maxMana = 140;
            _manaChange = 4;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_MAGE, 36, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellConeOfCold();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = 
                new AffectObject("CreatureAffect", _owner, target);
            ao->addToVictMessage("You feel your strength wither!");

            // Set up the affect
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(SPELL_CONE_COLD);
            ca->setWearoffToChar("You feel your strength return.");
            ca->setLevel(level);
            ca->setDuration(4 MUD_HOURS);
            int modifier  = -((ca->getLevel() >> 4) + 2);
            ca->addApply(APPLY_STR, modifier);
            ca->setAccumDuration(true);
            
            // Set up the damage object
            DamageObject *do1 = new DamageObject(getOwner(), target);
            int dam = dice(6, level) + (_owner->getInt() << 1);
            do1->setDamageType(SPELL_CONE_COLD);
            do1->setDamage(dam);

            string toChar = "You blast $N with your icy cone of coldness!";
            string toVict = "$n blasts you across the room as a cone of "
                "ice shoots from his fingers!";
            string toRoom = "$n blasts $N across the room with a "
                "cone of magical ice!";

            toChar = CCCYN(_owner, C_NRM) + toChar + CCNRM(_owner, C_NRM);
            do1->addToCharMessage(toChar);
            do1->addToVictMessage(toVict);
            do1->addToRoomMessage(toRoom);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);
            do1->setSkill(getSkillNumber());
            ev.push_back_id(do1);

            return true;
        }
};

class SpellConjureElemental : public ArcaneSpell
{
    public:
        SpellConjureElemental() : ArcaneSpell()
        {
            _skillNum = SPELL_CONJURE_ELEMENTAL;
            _skillName = "conjure elemental";
            _minMana = 90;
            _maxMana = 175;
            _manaChange = 4;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_MAGE, 20, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellConjureElemental();
        }

        bool perform(ExecutableVector &ev, char *argument) {
            if (!ArcaneSpell::perform(ev, argument))
                return false;

            SummonsObject *so = 
                new SummonsObject(getOwner(), SummonsObject::ConjureElemental);

            so->setSkill(getSkillNumber());
            ev.push_back_id(so);
            return true;
        }
};

class SpellControlWeather : public ArcaneSpell
{
    public:
        SpellControlWeather() : ArcaneSpell()
        {
            _skillNum = SPELL_CONTROL_WEATHER;
            _skillName = "control weather";
            _minMana = 25;
            _maxMana = 75;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_CLERIC, 22, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellControlWeather();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev, obj))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellCreateFood : public DivineSpell
{
    public:
        SpellCreateFood() : DivineSpell()
        {
            _skillNum = SPELL_CREATE_FOOD;
            _skillName = "create food";
            _minMana = 5;
            _maxMana = 30;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_CLERIC, 5, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellCreateFood();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellCreateWater : public DivineSpell
{
    public:
        SpellCreateWater() : DivineSpell()
        {
            _skillNum = SPELL_CREATE_WATER;
            _skillName = "create water";
            _minMana = 5;
            _maxMana = 30;
            _manaChange = 4;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_OBJ_INV | TAR_OBJ_EQUIP;
            PracticeInfo p0 = {CLASS_CLERIC, 3, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellCreateWater();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellCureBlind : public DivineSpell
{
    public:
        SpellCureBlind() : DivineSpell()
        {
            _skillNum = SPELL_CURE_BLIND;
            _skillName = "cure blind";
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_CLERIC, 6, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_PALADIN, 26, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellCureBlind();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellCureCritic : public DivineSpell
{
    public:
        SpellCureCritic() : DivineSpell()
        {
            _skillNum = SPELL_CURE_CRITIC;
            _skillName = "cure critic";
            _minMana = 10;
            _maxMana = 40;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_CLERIC, 10, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_PALADIN, 14, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellCureCritic();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellCureLight : public DivineSpell
{
    public:
        SpellCureLight() : DivineSpell()
        {
            _skillNum = SPELL_CURE_LIGHT;
            _skillName = "cure light";
            _minMana = 5;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_CLERIC, 2, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_PALADIN, 6, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellCureLight();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellCurse : public ArcaneSpell
{
    public:
        SpellCurse() : ArcaneSpell()
        {
            _skillNum = SPELL_CURSE;
            _skillName = "curse";
            _minMana = 50;
            _maxMana = 80;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM;
            PracticeInfo p0 = {CLASS_MAGE, 19, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellCurse();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("CurseAffect", _owner, target);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setDuration((1 + level >> 2) MUD_HOURS);
            ca->setLevel(level);
            ca->setApply(APPLY_DAMROLL, -(1 + level / 10));
            ca->setApply(APPLY_HITROLL, -(1 + level / 7));

            ao->addToCharMessage("$N briefly glows with a sick red light!");
            ao->addToVictMessage("You feel very uncomfortable.");
            ao->addToRoomMessage("$N briefly glows with a sick red light!");

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }

        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev, obj))
                return false;

            ExecutableObject *eo = new ExecutableObject(getOwner());
            eo->setObject(obj);

            if (!IS_OBJ_STAT(obj, ITEM_NODROP)) {
                SET_BIT(obj->obj_flags.extra_flags, ITEM_NODROP);
                eo->addToCharMessage("$p becomes cursed.");
            }
            else
                eo->addToCharMessage("Nothing seems to happen.");

            eo->setSkill(getSkillNumber());
            ev.push_back_id(eo);

            return true;
        }
};

class SpellDetectAlign : public DivineSpell
{
    public:
        SpellDetectAlign() : DivineSpell()
        {
            _skillNum = SPELL_DETECT_ALIGN;
            _skillName = "detect align";
            _minMana = 10;
            _maxMana = 20;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p1 = {CLASS_CLERIC, 1, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_PALADIN, 1, 0};
            _practiceInfo.push_back(p2);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellDetectAlign();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellDetectInvis : public ArcaneSpell
{
    public:
        SpellDetectInvis() : ArcaneSpell()
        {
            _skillNum = SPELL_DETECT_INVIS;
            _skillName = "detect invisibility";
            _minMana = 10;
            _maxMana = 20;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_MAGE, 14, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellDetectInvis();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("CreatureAffect",
                    _owner, target);

            CreatureAffect *af = (CreatureAffect *)ao->getAffect();

            af->setSkillNum(SPELL_DETECT_INVIS);
            af->setLevel(level);
            af->setDuration((12 + level >> 1) MUD_HOURS);
            af->setBitvector(0, AFF_DETECT_INVIS);
            af->setWearoffToChar("Your eyes stop tingling.");

            ao->addToVictMessage("Your eyes tingle.");

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);
            return true;
        }
};

class SpellDetectMagic : public ArcaneSpell
{
    public:
        SpellDetectMagic() : ArcaneSpell()
        {
            _skillNum = SPELL_DETECT_MAGIC;
            _skillName = "detect magic";
            _minMana = 10;
            _maxMana = 20;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_MAGE, 7, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 14, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_PALADIN, 20, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_RANGER, 27, 0};
            _practiceInfo.push_back(p3);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellDetectMagic();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            string toVictMessage = "Your eyes tingle.";

            AffectObject *ao = new AffectObject("CreatureAffect", 
                    _owner, _owner);
            CreatureAffect *af = (CreatureAffect *)ao->getAffect();
            af->setSkillNum(SPELL_DETECT_MAGIC);
            af->setWearoffToChar("You are no longer sensetive to the presence "
                    "of magic.");
            af->setLevel(level);
            af->setDuration((level  + number(1, 20)) MUD_HOURS);
            af->setBitvector(0, AFF_DETECT_MAGIC);

            ao->addToVictMessage(toVictMessage);
            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SpellDetectPoison : public DivineSpell
{
    public:
        SpellDetectPoison() : DivineSpell()
        {
            _skillNum = SPELL_DETECT_POISON;
            _skillName = "detect poison";
            _minMana = 5;
            _maxMana = 15;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_CHAR_ROOM;
            PracticeInfo p1 = {CLASS_CLERIC, 4, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_PALADIN, 12, 0};
            _practiceInfo.push_back(p2);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellDetectPoison();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }

        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellDetectScrying : public ArcaneSpell
{
    public:
        SpellDetectScrying() : ArcaneSpell()
        {
            _skillNum = SPELL_DETECT_SCRYING;
            _skillName = "detect scrying";
            _minMana = 30;
            _maxMana = 50;
            _manaChange = 4;
            _minPosition = POS_SITTING;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_MAGE, 44, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 44, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellDetectScrying();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellDimensionDoor : public ArcaneSpell
{
    public:
        SpellDimensionDoor() : ArcaneSpell()
        {
            _skillNum = SPELL_DIMENSION_DOOR;
            _skillName = "dimension door";
            _minMana = 60;
            _maxMana = 90;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellDimensionDoor();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellDispelEvil : public DivineSpell
{
    public:
        SpellDispelEvil() : DivineSpell()
        {
            _skillNum = SPELL_DISPEL_EVIL;
            _skillName = "dispel evil";
            _minMana = 25;
            _maxMana = 40;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _good = true;
            _targets = TAR_CHAR_ROOM | TAR_OBJ_INV;
            PracticeInfo p0 = {CLASS_CLERIC, 20, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellDispelEvil();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellDispelGood : public DivineSpell
{
    public:
        SpellDispelGood() : DivineSpell()
        {
            _skillNum = SPELL_DISPEL_GOOD;
            _skillName = "dispel good";
            _minMana = 25;
            _maxMana = 40;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _evil = true;
            _targets = TAR_CHAR_ROOM | TAR_OBJ_INV;
            PracticeInfo p0 = {CLASS_CLERIC, 20, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellDispelGood();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellDispelMagic : public ArcaneSpell
{
    public:
        SpellDispelMagic() : ArcaneSpell()
        {
            _skillNum = SPELL_DISPEL_MAGIC;
            _skillName = "dispel magic";
            _minMana = 55;
            _maxMana = 90;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM;
            PracticeInfo p0 = {CLASS_MAGE, 39, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellDispelMagic();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellDisruption : public DivineSpell
{
    public:
        SpellDisruption() : DivineSpell()
        {
            _skillNum = SPELL_DISRUPTION;
            _skillName = "disruption";
            _minMana = 75;
            _maxMana = 150;
            _manaChange = 7;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_CLERIC, 38, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellDisruption();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellDisplacement : public ArcaneSpell
{
    public:
        SpellDisplacement() : ArcaneSpell()
        {
            _skillNum = SPELL_DISPLACEMENT;
            _skillName = "displacement";
            _minMana = 30;
            _maxMana = 55;
            _manaChange = 4;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_MAGE, 46, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellDisplacement();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellEarthquake : public DivineSpell
{
    public:
        SpellEarthquake() : DivineSpell()
        {
            _skillNum = SPELL_EARTHQUAKE;
            _skillName = "earthquake";
            _minMana = 25;
            _maxMana = 40;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_CLERIC, 30, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellEarthquake();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellEnchantArmor : public ArcaneSpell
{
    public:
        SpellEnchantArmor() : ArcaneSpell()
        {
            _skillNum = SPELL_ENCHANT_ARMOR;
            _skillName = "enchant armor";
            _minMana = 80;
            _maxMana = 180;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_OBJ_INV;
            PracticeInfo p0 = {CLASS_MAGE, 16, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellEnchantArmor();
        }

        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev, obj))
                return false;

            if (_owner == NULL || obj == NULL)
                return false;
            
            int level = getPowerLevel();

            ExecutableObject *eo = new ExecutableObject(getOwner());
            eo->setObject(obj);

            if (((GET_OBJ_TYPE(obj) != ITEM_ARMOR) 
                || IS_SET(GET_OBJ_EXTRA(obj), ITEM_MAGIC)
                || IS_SET(GET_OBJ_EXTRA(obj), ITEM_MAGIC_NODISPEL)
                || IS_SET(GET_OBJ_EXTRA(obj), ITEM_BLESS)
                || IS_SET(GET_OBJ_EXTRA(obj), ITEM_DAMNED))
                && _owner->getLevel() < LVL_GRGOD) {
                eo->addToCharMessage("Nothing seems to happen.");
                eo->setSkill(getSkillNumber());
            ev.push_back_id(eo);
                return false;
            }

            int l1 = -1, l2 = -1, l3 = -1;
            for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
                if (obj->affected[i].location == APPLY_AC ||
                    obj->affected[i].location == APPLY_SAVING_BREATH ||
                    obj->affected[i].location == APPLY_SAVING_PARA) {
                    obj->affected[i].location = APPLY_NONE;
                    obj->affected[i].modifier = 0;
                }
                if (obj->affected[i].location == APPLY_NONE) {
                    if (l1 == -1)
                        l1 = i;
                    else if (l2 == -1)
                        l2 = i;
                    else if (l3 == -1)
                        l3 = i;
                }

            }

            int saveMod = -(level / 20);

            if (IS_IMPLANT(obj)) {
                saveMod >>= 1;
            }

            if (l1 > -1) {
                obj->affected[l1].location = APPLY_AC;
                obj->affected[l1].modifier = -((level >> 3) + 1);
            }

            if (l2 > -1) {
                obj->affected[l2].location = APPLY_SAVING_BREATH;
                obj->affected[l2].modifier = saveMod;
            }

            if (l3 > -1) {
                obj->affected[l3].location = APPLY_SAVING_PARA; 
                obj->affected[l3].modifier = saveMod;
            }

            if (_owner->isGood()) {
                SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL);
                eo->addToCharMessage("$p glows blue.");    
                eo->addToRoomMessage("$p glows blue.");    
            }
            else if (_owner->isEvil()) {
                SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD);
                eo->addToCharMessage("$p glows red.");    
                eo->addToRoomMessage("$p glows red.");    
            }
            else {
                eo->addToCharMessage("$p glows yellow.");    
                eo->addToRoomMessage("$p glows yellow.");    
            }

            if (level > number(50, 75))
                SET_BIT(GET_OBJ_EXTRA(obj), ITEM_GLOW);
            SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

            if (_owner->getLevel() >= LVL_AMBASSADOR 
                && !isname("imm", obj->aliases)) {
                char *tmp;
                if (obj->aliases) {
                    tmp = tmp_sprintf(" imm %senchant", _owner->getName());
                    tmp = tmp_strcat(obj->aliases, tmp, NULL);
                }
                obj->aliases = str_dup(tmp);
                mudlog(_owner->getLevel(), CMP, true,
                       "ENCHANT: %s by %s.", obj->getName(), _owner->getName());
            }

            eo->setSkill(getSkillNumber());
            ev.push_back_id(eo);
            return true;
        }
};

class SpellEnchantWeapon : public ArcaneSpell
{
    public:
        SpellEnchantWeapon() : ArcaneSpell()
        {
            _skillNum = SPELL_ENCHANT_WEAPON;
            _skillName = "enchant weapon";
            _minMana = 100;
            _maxMana = 200;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_OBJ_INV | TAR_OBJ_EQUIP;
            PracticeInfo p0 = {CLASS_MAGE, 21, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellEnchantWeapon();
        }

        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev, obj))
                return false;

            if (!_owner || !obj)
                return false;

            int level = getPowerLevel();

            ExecutableObject *eo = new ExecutableObject(getOwner());
            eo->setObject(obj);

            if (((GET_OBJ_TYPE(obj) != ITEM_WEAPON) 
                || IS_SET(GET_OBJ_EXTRA(obj), ITEM_MAGIC)
                || IS_SET(GET_OBJ_EXTRA(obj), ITEM_MAGIC_NODISPEL)
                || IS_SET(GET_OBJ_EXTRA(obj), ITEM_BLESS)
                || IS_SET(GET_OBJ_EXTRA(obj), ITEM_DAMNED))
                && _owner->getLevel() < LVL_CREATOR) {
                eo->addToCharMessage("Nothing seems to happen.");
                eo->setSkill(getSkillNumber());
            ev.push_back_id(eo);
                return false;
            }

            int l1 = -1, l2 = -1;
            for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
                if (obj->affected[i].location == APPLY_HITROLL ||
                    obj->affected[i].location == APPLY_DAMROLL) {
                    obj->affected[i].location = APPLY_NONE;
                    obj->affected[i].modifier = 0;
                }
                if (obj->affected[i].location == APPLY_NONE) {
                    if (l1 == -1)
                        l1 = i;
                    else if (l2 == -1)
                        l2 = i;
                }

            }

            int maxDam = 0, maxHit = 0, max = 0;
            max = _owner->getInt(); 
            max += getPowerLevel();
            max += _owner->checkSkill(SPELL_ENCHANT_WEAPON);
            
            maxDam = MAX(1, max / 33);
            maxHit = MAX(1, max / 24);

            if (l1 > -1) {
                obj->affected[l1].location = APPLY_HITROLL;
                obj->affected[l1].modifier = number(maxHit >> 1, maxHit);
            }
            if (l2 > -1) {
                obj->affected[l2].location = APPLY_DAMROLL;
                obj->affected[l2].modifier = number(maxDam >> 1, maxDam);
            }

            if (_owner->isGood()) {
                SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL);
                eo->addToCharMessage("$p glows blue.");    
                eo->addToRoomMessage("$p glows blue.");    
            }
            else if (_owner->isEvil()) {
                SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD);
                eo->addToCharMessage("$p glows red.");    
                eo->addToRoomMessage("$p glows red.");    
            }
            else {
                eo->addToCharMessage("$p glows yellow.");    
                eo->addToRoomMessage("$p glows yellow.");    
            }

            if (level > number(50, 75))
                SET_BIT(GET_OBJ_EXTRA(obj), ITEM_GLOW);
            SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

            if (_owner->getLevel() >= LVL_AMBASSADOR 
                && !isname("imm", obj->aliases)) {
                char *tmp;
                if (obj->aliases) {
                    tmp = tmp_sprintf(" imm %senchant", _owner->getName());
                    tmp = tmp_strcat(obj->aliases, tmp, NULL);
                }
                obj->aliases = str_dup(tmp);
                mudlog(_owner->getLevel(), CMP, true,
                       "ENCHANT: %s by %s.", obj->getName(), _owner->getName());
            }

            eo->setSkill(getSkillNumber());
            ev.push_back_id(eo);

            return true;
        }
};

class SpellEndureCold : public ArcaneSpell
{
    public:
        SpellEndureCold() : ArcaneSpell()
        {
            _skillNum = SPELL_ENDURE_COLD;
            _skillName = "endure cold";
            _minMana = 40;
            _maxMana = 100;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_CLERIC, 15, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_PALADIN, 16, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_BLACKGUARD, 16, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_RANGER, 16, 0};
            _practiceInfo.push_back(p3);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellEndureCold();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellEnergyDrain : public ArcaneSpell
{
    public:
        SpellEnergyDrain() : ArcaneSpell()
        {
            _skillNum = SPELL_ENERGY_DRAIN;
            _skillName = "energy drain";
            _minMana = 80;
            _maxMana = 120;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_MAGE, 24, 5};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellEnergyDrain();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = 
                new AffectObject("CreatureAffect", _owner, target);
            ao->addToCharMessage("You drain $N of some of $S energy!");
            ao->addToVictMessage("$n drains some of your energy!");
            ao->addToRoomMessage("$n drains $N of some of $S energy!");

            // Set up the affect
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(SPELL_ENERGY_DRAIN);
            ca->setWearoffToChar("Your feel your energy return to normal.");
            ca->setLevel(level);
            ca->setDuration((1 + level >> 4) MUD_HOURS);
            int modifier  = -(level / 10);
            ca->addApply(APPLY_LEVEL, modifier);
            
            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SpellFly : public ArcaneSpell
{
    public:
        SpellFly() : ArcaneSpell()
        {
            _skillNum = SPELL_FLY;
            _skillName = "fly";
            _minMana = 40;
            _maxMana = 60;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_MAGE, 33, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellFly();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellFlameStrike : public DivineSpell
{
    public:
        SpellFlameStrike() : DivineSpell()
        {
            _skillNum = SPELL_FLAME_STRIKE;
            _skillName = "flame strike";
            _minMana = 40;
            _maxMana = 60;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_CLERIC, 33, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_PALADIN, 35, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_NECROMANCER, 33, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_BLACKGUARD, 35, 0};
            _practiceInfo.push_back(p3);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellFlameStrike();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellFlameOfFaith : public DivineSpell
{
    public:
        SpellFlameOfFaith() : DivineSpell()
        {
            _skillNum = SPELL_FLAME_OF_FAITH;
            _skillName = "flame of faith";
            _minMana = 40;
            _maxMana = 60;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_OBJ_EQUIP | TAR_OBJ_ROOM | TAR_OBJ_INV;
            PracticeInfo p0 = {CLASS_PALADIN, 12, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_BLACKGUARD, 8, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellFlameOfFaith();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellGoodberry : public ArcaneSpell
{
    public:
        SpellGoodberry() : ArcaneSpell()
        {
            _skillNum = SPELL_GOODBERRY;
            _skillName = "goodberry";
            _minMana = 5;
            _maxMana = 30;
            _manaChange = 4;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_RANGER, 7, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellGoodberry();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellGustOfWind : public ArcaneSpell
{
    public:
        SpellGustOfWind() : ArcaneSpell()
        {
            _skillNum = SPELL_GUST_OF_WIND;
            _skillName = "gust of wind";
            _minMana = 55;
            _maxMana = 80;
            _manaChange = 4;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_OBJ_ROOM;
            PracticeInfo p0 = {CLASS_MAGE, 40, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellGustOfWind();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellBarkskin : public ArcaneSpell
{
    public:
        SpellBarkskin() : ArcaneSpell()
        {
            _skillNum = SPELL_BARKSKIN;
            _skillName = "barkskin";
            _minMana = 25;
            _maxMana = 60;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_RANGER, 4, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellBarkskin();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellIcyBlast : public DivineSpell
{
    public:
        SpellIcyBlast() : DivineSpell()
        {
            _skillNum = SPELL_ICY_BLAST;
            _skillName = "icy blast";
            _minMana = 50;
            _maxMana = 80;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_CLERIC, 41, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellIcyBlast();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellInvisToUndead : public ArcaneSpell
{
    public:
        SpellInvisToUndead() : ArcaneSpell()
        {
            _skillNum = SPELL_INVIS_TO_UNDEAD;
            _skillName = "invis to undead";
            _minMana = 40;
            _maxMana = 60;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_MAGE, 30, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_RANGER, 21, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellInvisToUndead();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellAnimalKin : public ArcaneSpell
{
    public:
        SpellAnimalKin() : ArcaneSpell()
        {
            _skillNum = SPELL_ANIMAL_KIN;
            _skillName = "animal kin";
            _minMana = 40;
            _maxMana = 60;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_RANGER, 17, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellAnimalKin();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellGreaterEnchant : public ArcaneSpell
{
    public:
        SpellGreaterEnchant() : ArcaneSpell()
        {
            _skillNum = SPELL_GREATER_ENCHANT;
            _skillName = "greater enchant";
            _minMana = 120;
            _maxMana = 230;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_OBJ_INV;
            PracticeInfo p0 = {CLASS_MAGE, 34, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellGreaterEnchant();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellGroupArmor : public DivineSpell
{
    public:
        SpellGroupArmor() : DivineSpell()
        {
            _skillNum = SPELL_GROUP_ARMOR;
            _skillName = "group armor";
            _minMana = 30;
            _maxMana = 50;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_CLERIC, 19, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellGroupArmor();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellFireball : public ArcaneSpell
{
    public:
        SpellFireball() : ArcaneSpell()
        {
            _skillNum = SPELL_FIREBALL;
            _skillName = "fireball";
            _minMana = 40;
            _maxMana = 130;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_MAGE, 31, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellFireball();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellFireShield : public ArcaneSpell
{
    public:
        SpellFireShield() : ArcaneSpell()
        {
            _skillNum = SPELL_FIRE_SHIELD;
            _skillName = "fire shield";
            _minMana = 30;
            _maxMana = 70;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            _flags = MAG_NOWATER;
            PracticeInfo p0 = {CLASS_MAGE, 17, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellFireShield();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("FireShieldAffect", _owner,
                    target);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setLevel(level);
            ca->setApply(APPLY_AC, MAX(1, level / 9));
            ca->setAccumDuration(true);
            ca->setDuration((6 + (level >> 2)) MUD_HOURS);

            if (target != _owner)
                ao->addToCharMessage("A sheet of flame appears before $N!");
            ao->addToVictMessage("A sheet of flame appears before your body.");
            ao->addToRoomMessage("A sheet of flame appears before $N!");

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SpellGreaterHeal : public DivineSpell
{
    public:
        SpellGreaterHeal() : DivineSpell()
        {
            _skillNum = SPELL_GREATER_HEAL;
            _skillName = "greater heal";
            _minMana = 90;
            _maxMana = 120;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_CLERIC, 34, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellGreaterHeal();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellGroupHeal : public DivineSpell
{
    public:
        SpellGroupHeal() : DivineSpell()
        {
            _skillNum = SPELL_GROUP_HEAL;
            _skillName = "group heal";
            _minMana = 60;
            _maxMana = 80;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_CLERIC, 31, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellGroupHeal();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellHarm : public DivineSpell
{
    public:
        SpellHarm() : DivineSpell()
        {
            _skillNum = SPELL_HARM;
            _skillName = "harm";
            _minMana = 45;
            _maxMana = 95;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_CLERIC, 29, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellHarm();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellHeal : public DivineSpell
{
    public:
        SpellHeal() : DivineSpell()
        {
            _skillNum = SPELL_HEAL;
            _skillName = "heal";
            _minMana = 30;
            _maxMana = 60;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_CLERIC, 24, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_PALADIN, 28, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellHeal();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellHaste : public ArcaneSpell
{
    public:
        SpellHaste() : ArcaneSpell()
        {
            _skillNum = SPELL_HASTE;
            _skillName = "haste";
            _minMana = 40;
            _maxMana = 60;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_MAGE, 44, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellHaste();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellInfravision : public ArcaneSpell
{
    public:
        SpellInfravision() : ArcaneSpell()
        {
            _skillNum = SPELL_INFRAVISION;
            _skillName = "infravision";
            _minMana = 10;
            _maxMana = 25;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_MAGE, 3, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellInfravision();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();
            AffectObject *ao = new AffectObject("CreatureAffect", 
                    _owner, target);
            ao->addToVictMessage("Your eyes glow red.");

            // Set up the affect
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(SPELL_INFRAVISION);
            ca->setWearoffToChar("Your night vision fades.");
            ca->setLevel(level);
            ca->setDuration((level >> 1) MUD_HOURS);
            ca->setAccumDuration(true);
            
            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);
            return true;
        }
};

class SpellInvisible : public ArcaneSpell
{
    public:
        SpellInvisible() : ArcaneSpell()
        {
            _skillNum = SPELL_INVISIBLE;
            _skillName = "invisibility";
            _minMana = 25;
            _maxMana = 35;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM;
            PracticeInfo p0 = {CLASS_MAGE, 12, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellInvisible();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;
            
            int level = getPowerLevel();

            if (IS_AFFECTED(target, AFF_INVISIBLE)) {
                ExecutableObject *eo = new ExecutableObject(getOwner());
                eo->setVictim(target);

                eo->addToVictMessage("Nothing seems to happen.");
                eo->setSkill(getSkillNumber());
            ev.push_back_id(eo);

                return true;
            }

            AffectObject *ao = new AffectObject("InvisAffect", _owner, target);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();

            ca->setDuration((12 + (level >> 4)) MUD_HOURS);
            ca->setLevel(getPowerLevel());
            ca->setApply(APPLY_AC, 30);

            ao->addToVictMessage("You vanish.");
            ao->addToRoomMessage("$N slowly fades out of existence.");

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);
            return true;
        }

        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev, obj))
                return false;

            ExecutableObject *eo = new ExecutableObject(getOwner());
            eo->setObject(obj);
            eo->setSkill(getSkillNumber());
            ev.push_back_id(eo);

            if (IS_OBJ_STAT(obj, ITEM_NOINVIS | ITEM_INVISIBLE)) {
                eo->addToCharMessage("Nothing seems to happen.");
                return true;
            }

            SET_BIT(obj->obj_flags.extra_flags, ITEM_INVISIBLE);
            eo->addToCharMessage("$p turns invisible.");
            return true;
        }
};

class SpellGlowlight : public ArcaneSpell
{
    public:
        SpellGlowlight() : ArcaneSpell()
        {
            _skillNum = SPELL_GLOWLIGHT;
            _skillName = "glowlight";
            _minMana = 20;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_MAGE, 8, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellGlowlight();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            string toVictMessage = "The area around you is illuminated with "
                "a ghostly light.";
            string toRoomMessage = "A ghostly light appears around $n.";

            AffectObject *ao = new AffectObject("CreatureAffect", 
                    _owner, _owner);
            CreatureAffect *af = (CreatureAffect *)ao->getAffect();
            af->setSkillNum(SPELL_GLOWLIGHT);
            af->setWearoffToChar("The ghostly light around you fades.");
            af->setLevel(level);
            af->setDuration((level  + number(1, 20)) MUD_HOURS);
            af->setBitvector(0, AFF_GLOWLIGHT);

            ao->addToVictMessage(toVictMessage);
            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SpellKnock : public ArcaneSpell
{
    public:
        SpellKnock() : ArcaneSpell()
        {
            _skillNum = SPELL_KNOCK;
            _skillName = "knock";
            _minMana = 20;
            _maxMana = 50;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_DOOR;
            PracticeInfo p0 = {CLASS_MAGE, 27, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellKnock();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }

        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellLightningBolt : public ArcaneSpell
{
    public:
        SpellLightningBolt() : ArcaneSpell()
        {
            _skillNum = SPELL_LIGHTNING_BOLT;
            _skillName = "lightning bolt";
            _minMana = 15;
            _maxMana = 50;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _audible = true;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            _flags = MAG_WATERZAP;
            PracticeInfo p0 = {CLASS_MAGE, 15, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellLightningBolt();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            DamageObject *do1 = new DamageObject(getOwner(), target);
            int dam = 200 + dice(8, level + (_owner->getInt()));
            do1->setDamageType(SPELL_LIGHTNING_BOLT);
            do1->setDamage(dam);

            string toChar = "Your lightning bolt hits $N with full impact!";
            string toVict = "$n sends a lightning bolt directly into your chest!";
            string toRoom = "$n's lightning bolt hits $N with full impact!";

            do1->addToCharMessage(toChar);
            do1->addToVictMessage(toVict);
            do1->addToRoomMessage(toRoom);

            do1->setSkill(getSkillNumber());
            ev.push_back_id(do1);

            return true;
        }
};

class SpellLocateObject : public ArcaneSpell
{
    public:
        SpellLocateObject() : ArcaneSpell()
        {
            _skillNum = SPELL_LOCATE_OBJECT;
            _skillName = "locate object";
            _minMana = 20;
            _maxMana = 25;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_MAGE, 23, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellLocateObject();
        }

        bool perform(ExecutableVector &ev, char *targets) {
            if (!ArcaneSpell::perform(ev, targets))
                return false;

            if (!_owner)
                return false;

            int level = getPowerLevel();
            int max_terms = level / 25;
            // Nobody can get more than 4 terms
            char *terms[4];
            int term_count = 0, j = level >> 1, k = level >> 2, term_idx = 0;
            extern struct obj_data *object_list;
            obj_data *i;
            bool house_found = false;

            ExecutableObject *house = new ExecutableObject(getOwner());
            house->setSkill(SPELL_LOCATE_OBJECT);
            ExecutableObject *other = new ExecutableObject(getOwner());
            other->setSkill(SPELL_LOCATE_OBJECT);
            ExecutableObject *whichEO = NULL;

            while (*targets && term_count < 4) {
                terms[term_count] = tmp_getword(&targets);
                term_count++;
                // Every term past the first incurrs the skill cost again
                if (term_count > 1)
                    ev.push_front_id(generateCost());
            }
            
            if (term_count > max_terms) {
                send_to_char(_owner, "You are not powerful enough to be so "
                        "precise.\r\n");
                return false;
            }

            for (i = object_list; i && (j > 0 || k > 0); i = i->next) {
                bool found = true;
                for (term_idx = 0; term_idx < term_count && found; term_idx++) {
                    if (!isname(terms[term_idx], i->aliases))
                        found = false;
                }

                if (!found)
                    continue;

                if (!can_see_object(_owner, i))
                    continue;

                if (isname("imm", i->aliases) 
                    || IS_OBJ_TYPE(i, ITEM_SCRIPT) || !OBJ_APPROVED(i))
                    continue;

                room_data *rm = where_obj(i);

                if (!rm) {
                    errlog("%s is nowhere?  Moving to the void.", i->name);
                    rm = real_room(0);
                    SET_BIT(GET_OBJ_EXTRA2(i), ITEM2_BROKEN);
                    SET_BIT(GET_OBJ_EXTRA3(i), ITEM3_HUNTED);
                    obj_to_room(i, rm);
                    continue;
                }

                // make sure it exists in a nearby plane/time
                if (rm->zone->plane != _owner->in_room->zone->plane
                    || (rm->zone->time_frame != _owner->in_room->zone->time_frame
                        && _owner->in_room->zone->time_frame != TIME_TIMELESS 
                        && rm->zone->time_frame != TIME_TIMELESS)) {
                    if (!_owner->isImmortal())
                        continue;
                }

                if (i->in_obj)
                    rm = where_obj(i);
                else
                    rm = i->in_room;

                if (rm && ROOM_FLAGGED(rm, ROOM_HOUSE)) {
                    whichEO = house;
                    house_found = true;
                    if (k-- <= 0)
                        continue;
                }
                else {
                    whichEO = other;
                    if (j-- <= 0)
                        continue;
                }
                
                char *tmp;
                if (IS_OBJ_STAT2(i, ITEM2_NOLOCATE)) {
                    tmp = tmp_sprintf("The location of %s is "
                            "indeterminable.", i->name);
                }
                else if (IS_OBJ_STAT2(i, ITEM2_HIDDEN)) {
                    tmp = tmp_sprintf("%s is hidden somewhere.", i->name);
                }
                else if (i->carried_by) {
                    tmp = tmp_sprintf("%s is being carried by %s.",
                            i->name, PERS(i->carried_by, _owner));
                }
                else if (i->in_room != NULL 
                         && !ROOM_FLAGGED(i->in_room, ROOM_HOUSE)) {
                    tmp = tmp_sprintf("%s is in %s.", i->name,i->in_room->name);
                }
                else if (i->in_obj) {
                    tmp = tmp_sprintf("%s is in %s.", i->name, i->in_obj->name);
                }
                else if (i->worn_by) {
                    tmp = tmp_sprintf("%s is being worn by %s.", i->name,
                            PERS(i->worn_by, _owner));
                }
                else {
                    tmp = tmp_sprintf("%s's location is uncertain.", i->name);
                }

                whichEO->addToCharMessage(tmp_capitalize(tmp));
            }

            if (j == level >> 1 && k == level >> 2)
                other->addToCharMessage("You sense nothing.");

            ev.push_back_id(other);

            if (house_found) {
                other->addToCharMessage("-------------");
                ev.push_back_id(house);
            }

            return true;
        }
};

class SpellMagicMissile : public ArcaneSpell
{
    public:
        SpellMagicMissile() : ArcaneSpell()
        {
            _skillNum = SPELL_MAGIC_MISSILE;
            _skillName = "magic missile";
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_MAGE, 1, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellMagicMissile();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();
            int num_missiles = MAX(1, (level >> 1) / 10);

            // Add a little randomness to it
            num_missiles = number(1, num_missiles);

            for (int x = 0; x < num_missiles; x++) {
                // Add the skill cost again for each missile
                ev.push_back_id(generateCost());

                DamageObject *do1 = new DamageObject(getOwner(), target);
                int dam = dice(2, level) + _owner->getInt();
                if (_owner->numCombatants())
                    do1->setVictim(_owner->findRandomCombat());
                else
                    do1->setVictim(target);
                do1->setDamageType(SPELL_MAGIC_MISSILE);
                do1->setDamage(dam);
                int num = number(1, 3);
                string toChar;
                string toVict;
                string toRoom;
                switch (num) {
                    case 1:
                        toChar = "You grin maniacally as your missile slams "
                            "into $N.";
                        toVict = "$n grins maniacally as $s missile slams "
                            "into you!";
                        toRoom = "$n grins maniacally as $s missile slams "
                            "into $N.";
                        break;
                    case 2:
                        toChar = "You laugh out loud as your missile "
                            "explodes in the face of $N!";
                        toVict = "$n laughs out loud as $s missile "
                            "explodes in your face!";
                        toRoom = "$n laughs out loud as $s missile "
                            "explodes in the face of $N!";
                        break;
                    case 3:
                        toChar = "Your magic missle slams into $N, "
                            "stunning $M briefly.";
                        toVict = "$n's magic missle slams into you, "
                            "stunning you briefly.";
                        toRoom = "$n's magic missle slams into $N, "
                            "stunning $M briefly.";
                        break;
                }

                do1->addToCharMessage(toChar);
                do1->addToVictMessage(toVict);
                do1->addToRoomMessage(toRoom);

                do1->setSkill(getSkillNumber());
                ev.push_back_id(do1);
            }

            return true;
        }
};

class SpellMelfsAcidArrow : public ArcaneSpell
{
    public:
        SpellMelfsAcidArrow() : ArcaneSpell()
        {
            _skillNum = SPELL_MELFS_ACID_ARROW;
            _skillName = "melfs acid arrow";
            _minMana = 30;
            _maxMana = 60;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM;
            PracticeInfo p0 = {CLASS_MAGE, 15, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellMelfsAcidArrow();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            int level = getPowerLevel();
            // Set up the damage object
            DamageObject *do1 = new DamageObject(getOwner(), target);
            int dam = dice(6, level) + (_owner->getInt() << 1);
            do1->setDamageType(SPELL_MELFS_ACID_ARROW);
            do1->setDamage(dam);

            string toChar = "An arrow of acid springs from your hand and "
                "explodes into $N!";
            string toVict = "An arrow of acid springs from $n's hand and "
                "explodes into your chest!";
            string toRoom = "An arrow of acid springs from $n's hand and "
                "explodes into $N!";

            do1->addToCharMessage(toChar);
            do1->addToVictMessage(toVict);
            do1->addToRoomMessage(toRoom);

	    AffectObject *ao = new AffectObject("AcidityAffect", 
						_owner, target);
	    ao->addToVictMessage("Your skin begins to bubble and burn!");
	    
	    // Set up the affect
	    CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
	    ca->setSkillNum(SPELL_MELFS_ACID_ARROW);
	    ca->setWearoffToChar("The burning in your skin subsides.");
	    ca->setLevel(level);
	    ca->setDuration(dice(1, 3) MUD_HOURS);
	    ca->setBitvector(2, AFF3_ACIDITY);
	    ca->setAccumDuration(true);
            
	    ao->setSkill(getSkillNumber());
	    ev.push_back_id(ao);

            do1->setSkill(getSkillNumber());
            ev.push_back_id(do1);

            return true;
        }
};

class SpellMinorIdentify : public DivineSpell
{
    public:
        SpellMinorIdentify() : DivineSpell()
        {
            _skillNum = SPELL_MINOR_IDENTIFY;
            _skillName = "minor identify";
            _minMana = 50;
            _maxMana = 75;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM;
            PracticeInfo p1 = {CLASS_CLERIC, 46, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellMinorIdentify();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev, target))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev, obj))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellMagicalProt : public ArcaneSpell
{
    public:
        SpellMagicalProt() : ArcaneSpell()
        {
            _skillNum = SPELL_MAGICAL_PROT;
            _skillName = "magical prot";
            _minMana = 10;
            _maxMana = 40;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_MAGE, 32, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 26, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellMagicalProt();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellMagicalVestment : public DivineSpell
{
    public:
        SpellMagicalVestment() : DivineSpell()
        {
            _skillNum = SPELL_MAGICAL_VESTMENT;
            _skillName = "magical vestment";
            _minMana = 80;
            _maxMana = 100;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_OBJ_INV;
            PracticeInfo p0 = {CLASS_CLERIC, 11, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_PALADIN, 4, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_BLACKGUARD, 4, 0};
            _practiceInfo.push_back(p2);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellMagicalVestment();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellMeteorStorm : public ArcaneSpell
{
    public:
        SpellMeteorStorm() : ArcaneSpell()
        {
            _skillNum = SPELL_METEOR_STORM;
            _skillName = "meteor storm";
            _minMana = 110;
            _maxMana = 180;
            _manaChange = 25;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_MAGE, 48, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellMeteorStorm();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellChainLightning : public ArcaneSpell
{
    public:
        SpellChainLightning() : ArcaneSpell()
        {
            _skillNum = SPELL_CHAIN_LIGHTNING;
            _skillName = "chain lightning";
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _violent = true;
            _flags = MAG_MASSES | MAG_WATERZAP;
            PracticeInfo p0 = {CLASS_MAGE, 23, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellChainLightning();
        }

        bool perform(ExecutableVector &ev, CreatureList &clist) {
            if (!ArcaneSpell::perform(ev, clist))
                return false;

            int level = getPowerLevel();
            int num_hits = 0;
            int num_dice = 8;

            CreatureList::iterator cit = clist.begin();
            for (; cit != clist.end(); ++cit) {
                Creature *target = *cit;

                num_hits++;

                // Add the skill cost for each creature
                ev.push_back_id(generateCost());

                // Now create a DamageObject for each creature and
                // do less damage every hit
                DamageObject *do1 = new DamageObject(getOwner(), target);
                num_dice = MAX(1, num_dice - num_hits); 
                int dam = 200 + dice(num_dice, level + (_owner->getInt()));
                do1->setDamageType(SPELL_CHAIN_LIGHTNING);
                do1->setDamage(dam);

                string toChar = "Your lightning bolt hits $N with full impact!";
                string toVict = "$n sends a lightning bolt directly into your chest!";
                string toRoom = "$n's lightning bolt hits $N with full impact!";

                do1->addToCharMessage(toChar);
                do1->addToVictMessage(toVict);
                do1->addToRoomMessage(toRoom);

                do1->setSkill(getSkillNumber());
                ev.push_back_id(do1);
            }

            return true;
        }
};

class SpellHailstorm : public ArcaneSpell
{
    public:
        SpellHailstorm() : ArcaneSpell()
        {
            _skillNum = SPELL_HAILSTORM;
            _skillName = "hailstorm";
            _minMana = 110;
            _maxMana = 180;
            _manaChange = 10;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_RANGER, 37, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellHailstorm();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellIceStorm : public ArcaneSpell
{
    public:
        SpellIceStorm() : ArcaneSpell()
        {
            _skillNum = SPELL_ICE_STORM;
            _skillName = "ice storm";
            _minMana = 60;
            _maxMana = 130;
            _manaChange = 15;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_CLERIC, 49, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellIceStorm();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellPoison : public ArcaneSpell
{
    public:
        SpellPoison() : ArcaneSpell()
        {
            _skillNum = SPELL_POISON;
            _skillName = "poison";
            _minMana = 20;
            _maxMana = 50;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV;
            PracticeInfo p0 = {CLASS_MAGE, 20, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellPoison();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("PoisonAffect", _owner, target);
            PoisonAffect *ca = (PoisonAffect *)ao->getAffect();
            ca->setLevel(level);
            
            // Clear it
            ca->setBitvector(0, 0);
            if (level > 80) {
                ca->setBitvector(2, AFF3_POISON_3);
                ca->setType(PoisonAffect::Poison_III); 
            }
            if (level > 50) {
                ca->setBitvector(2, AFF3_POISON_2);
                ca->setType(PoisonAffect::Poison_II); 
            }
            else {
                ca->setBitvector(0, AFF_POISON);
                ca->setType(PoisonAffect::Poison_I); 
            }

            ao->addToVictMessage("You feel very sick!");
            ao->addToRoomMessage("$n gets violently ill!");

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }

        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev, obj))
                return false;

            int level = getPowerLevel();

            ExecutableObject *eo = new ExecutableObject(getOwner());
            eo->setObject(obj);

            if (GET_OBJ_TYPE(obj) == ITEM_FOOD 
                || GET_OBJ_TYPE(obj) == ITEM_DRINKCON) {
                if (level > 80)
                    GET_OBJ_VAL(obj, 3) = 3;
                if (level > 50)
                    GET_OBJ_VAL(obj, 3) = 2;
                else
                    GET_OBJ_VAL(obj, 3) = 1;

                eo->addToCharMessage("$p is now poisoned.");
                eo->addToRoomMessage("$p glows briefly with a faint green light.");
            }
            else
                eo->addToCharMessage("Nothing seems to happen.");

            eo->setSkill(getSkillNumber());
            ev.push_back_id(eo);

            return true;
        }
};

class SpellPray : public DivineSpell
{
    public:
        SpellPray() : DivineSpell()
        {
            _skillNum = SPELL_PRAY;
            _skillName = "pray";
            _minMana = 50;
            _maxMana = 80;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_CLERIC, 27, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_PALADIN, 30, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellPray();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellPrismaticSpray : public ArcaneSpell
{
    public:
        SpellPrismaticSpray() : ArcaneSpell()
        {
            _skillNum = SPELL_PRISMATIC_SPRAY;
            _skillName = "prismatic spray";
            _minMana = 80;
            _maxMana = 160;
            _manaChange = 10;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_MAGE, 42, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellPrismaticSpray();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellProtectFromDevils : public DivineSpell
{
    public:
        SpellProtectFromDevils() : DivineSpell()
        {
            _skillNum = SPELL_PROTECT_FROM_DEVILS;
            _skillName = "protect from devils";
            _minMana = 15;
            _maxMana = 40;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellProtectFromDevils();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellProtFromEvil : public DivineSpell
{
    public:
        SpellProtFromEvil() : DivineSpell()
        {
            _skillNum = SPELL_PROT_FROM_EVIL;
            _skillName = "prot from evil";
            _minMana = 15;
            _maxMana = 40;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _good = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_CLERIC, 16, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_PALADIN, 9, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellProtFromEvil();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellProtFromGood : public DivineSpell
{
    public:
        SpellProtFromGood() : DivineSpell()
        {
            _skillNum = SPELL_PROT_FROM_GOOD;
            _skillName = "prot from good";
            _minMana = 15;
            _maxMana = 40;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _evil = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_NECROMANCER, 16, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_PALADIN, 9, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellProtFromGood();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellProtFromLightning : public ArcaneSpell
{
    public:
        SpellProtFromLightning() : ArcaneSpell()
        {
            _skillNum = SPELL_PROT_FROM_LIGHTNING;
            _skillName = "prot from lightning";
            _minMana = 15;
            _maxMana = 40;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_RANGER, 9, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellProtFromLightning();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellProtFromFire : public ArcaneSpell
{
    public:
        SpellProtFromFire() : ArcaneSpell()
        {
            _skillNum = SPELL_PROT_FROM_FIRE;
            _skillName = "prot from fire";
            _minMana = 15;
            _maxMana = 40;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_CLERIC, 16, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_RANGER, 19, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellProtFromFire();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellRemoveCurse : public ArcaneSpell
{
    public:
        SpellRemoveCurse() : ArcaneSpell()
        {
            _skillNum = SPELL_REMOVE_CURSE;
            _skillName = "remove curse";
            _minMana = 25;
            _maxMana = 45;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_OBJ_INV;
            PracticeInfo p0 = {CLASS_MAGE, 26, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 27, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_PALADIN, 31, 0};
            _practiceInfo.push_back(p2);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellRemoveCurse();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;

            RemoveAffectObject *ao = 
                new RemoveAffectObject("CurseAffect", _owner, target);
            ao->addToVictMessage("You don't feel quite so unlucky.");

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }

        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev, obj))
                return false;

            ExecutableObject *eo = new ExecutableObject(getOwner());
            eo->setObject(obj);

            int level = getPowerLevel();

            if (IS_OBJ_STAT2(obj, ITEM2_CURSED_PERM) &&
                level < 150)
                eo->addToCharMessage("$p vibrates fiercely, then stops.");
            else if (IS_OBJ_STAT(obj, ITEM_NODROP)) {
                REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_NODROP);
                eo->addToCharMessage("$p briefly glows blue.");
            }
            else {
                eo->addToCharMessage("Nothing seems to happen.");
            }

            eo->setSkill(getSkillNumber());
            ev.push_back_id(eo);

            return true;
        }

};

class SpellRemoveSickness : public DivineSpell
{
    public:
        SpellRemoveSickness() : DivineSpell()
        {
            _skillNum = SPELL_REMOVE_SICKNESS;
            _skillName = "remove sickness";
            _minMana = 85;
            _maxMana = 145;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_CLERIC, 37, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellRemoveSickness();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellRejuvenate : public DivineSpell
{
    public:
        SpellRejuvenate() : DivineSpell()
        {
            _skillNum = SPELL_REJUVENATE;
            _skillName = "rejuvenate";
            _minMana = 40;
            _maxMana = 100;
            _manaChange = 8;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_CLERIC, 40, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellRejuvenate();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellRefresh : public ArcaneSpell
{
    public:
        SpellRefresh() : ArcaneSpell()
        {
            _skillNum = SPELL_REFRESH;
            _skillName = "refresh";
            _minMana = 40;
            _maxMana = 100;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_RANGER, 15, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellRefresh();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellRegenerate : public ArcaneSpell
{
    public:
        SpellRegenerate() : ArcaneSpell()
        {
            _skillNum = SPELL_REGENERATE;
            _skillName = "regenerate";
            _minMana = 100;
            _maxMana = 140;
            _manaChange = 10;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_MAGE, 49, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 45, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellRegenerate();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellRetrieveCorpse : public DivineSpell
{
    public:
        SpellRetrieveCorpse() : DivineSpell()
        {
            _skillNum = SPELL_RETRIEVE_CORPSE;
            _skillName = "retrieve corpse";
            _minMana = 65;
            _maxMana = 125;
            _manaChange = 15;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_WORLD;
            PracticeInfo p0 = {CLASS_CLERIC, 48, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellRetrieveCorpse();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellSanctuary : public DivineSpell
{
    public:
        SpellSanctuary() : DivineSpell()
        {
            _skillNum = SPELL_SANCTUARY;
            _skillName = "sanctuary";
            _minMana = 85;
            _maxMana = 110;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_CLERIC, 28, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellSanctuary();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellShockingGrasp : public ArcaneSpell
{
    public:
        SpellShockingGrasp() : ArcaneSpell()
        {
            _skillNum = SPELL_SHOCKING_GRASP;
            _skillName = "shocking grasp";
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            _flags = MAG_TOUCH | MAG_WATERZAP;
            PracticeInfo p0 = {CLASS_MAGE, 9, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellShockingGrasp();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = 
                new AffectObject("CreatureAffect", _owner, target);

            ao->addToVictMessage("Your body shakes and twitches from "
                    "the current.");

            // Set up the affect
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(SPELL_SHOCKING_GRASP);
            ca->setWearoffToChar("Your bodily functions return to normal.");
            ca->setLevel(level);
            ca->setDuration((1 + (level >> 4)) MUD_HOURS);
            int modifier  = -(2 + (level >> 4));
            ca->addApply(APPLY_DEX, modifier);
            ca->setAccumDuration(true);
            
            // Set up the damage object
            DamageObject *do1 = new DamageObject(getOwner(), target);
            int dam = dice(7, level) + (_owner->getInt() << 1);
            do1->setDamage(dam);
            do1->setDamageType(SPELL_SHOCKING_GRASP);

            string toChar = "You grab $N and shock the hell out of $M!";
            string toVict = "$n grabs you and shocks the hell out of you!";
            string toRoom = "$n grabs $N and shocks the hell out of $M!";

            do1->addToCharMessage(toChar);
            do1->addToVictMessage(toVict);
            do1->addToRoomMessage(toRoom);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);
            do1->setSkill(getSkillNumber());
            ev.push_back_id(do1);

            return true;
        }
};

class SpellShroudObscurement : public ArcaneSpell
{
    public:
        SpellShroudObscurement() : ArcaneSpell()
        {
            _skillNum = SPELL_SHROUD_OBSCUREMENT;
            _skillName = "shroud obscurement";
            _minMana = 65;
            _maxMana = 145;
            _manaChange = 10;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_MAGE, 36, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellShroudObscurement();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellSleep : public ArcaneSpell
{
    public:
        SpellSleep() : ArcaneSpell()
        {
            _skillNum = SPELL_SLEEP;
            _skillName = "sleep";
            _minMana = 25;
            _maxMana = 40;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _violent = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_MAGE, 10, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellSleep();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            string toVictMessage = "You feel very sleepy...ZZzzzz...";
            string toCharMessage = "$N goes to sleep.";
            string toRoomMessage = "$N goes to sleep.";

            AffectObject *ao = new AffectObject("CreatureAffect", 
                    _owner, target);
            CreatureAffect *af = (CreatureAffect *)ao->getAffect();
            af->setSkillNum(SPELL_SLEEP);
            af->setWearoffToChar("You feel less tired.");
            af->setLevel(level);
            af->setDuration((1 + level / 10) MUD_HOURS);
            af->setBitvector(0, AFF_SLEEP);
            af->setPosition(POS_SLEEPING);

            ao->addToVictMessage(toVictMessage);
            ao->addToCharMessage(toCharMessage);
            ao->addToRoomMessage(toRoomMessage);
            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SpellSlow : public ArcaneSpell
{
    public:
        SpellSlow() : ArcaneSpell()
        {
            _skillNum = SPELL_SLOW;
            _skillName = "slow";
            _minMana = 60;
            _maxMana = 120;
            _manaChange = 6;
            _minPosition = POS_STANDING;
            _targets = TAR_UNPLEASANT | TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_MAGE, 38, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellSlow();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellSpiritHammer : public DivineSpell
{
    public:
        SpellSpiritHammer() : DivineSpell()
        {
            _skillNum = SPELL_SPIRIT_HAMMER;
            _skillName = "spirit hammer";
            _minMana = 10;
            _maxMana = 40;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_CLERIC, 10, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_NECROMANCER, 10, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_PALADIN, 15, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_BLACKGUARD, 15, 0};
            _practiceInfo.push_back(p3);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellSpiritHammer();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellStrength : public ArcaneSpell
{
    public:
        SpellStrength() : ArcaneSpell()
        {
            _skillNum = SPELL_STRENGTH;
            _skillName = "strength";
            _minMana = 30;
            _maxMana = 65;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_MAGE, 6, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellStrength();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            string toVictMessage = "You feel stronger!";

            AffectObject *ao = new AffectObject("CreatureAffect", 
                    _owner, target);
            CreatureAffect *af = (CreatureAffect *)ao->getAffect();
            af->setSkillNum(SPELL_STRENGTH);
            af->setWearoffToChar("Your magical strength fades.");
            af->setLevel(level);
            af->setDuration((level >> 1) MUD_HOURS);
            af->addApply(APPLY_STR, 2 + level >> 4);

            ao->addToVictMessage(toVictMessage);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SpellSummon : public ArcaneSpell
{
    public:
        SpellSummon() : ArcaneSpell()
        {
            _skillNum = SPELL_SUMMON;
            _skillName = "summon";
            _minMana = 180;
            _maxMana = 320;
            _manaChange = 7;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_WORLD | TAR_NOT_SELF;
            PracticeInfo p0 = {CLASS_MAGE, 29, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellSummon();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;

            TeleportObject *to = 
                new TeleportObject(getOwner(), TeleportObject::Summon);

            to->setVictim(target);

            to->setSkill(getSkillNumber());
            ev.push_back_id(to);
            return true;
        }
};

class SpellSword : public ArcaneSpell
{
    public:
        SpellSword() : ArcaneSpell()
        {
            _skillNum = SPELL_SWORD;
            _skillName = "sword";
            _minMana = 180;
            _maxMana = 265;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_MAGE, 30, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellSword();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellSymbolOfPain : public DivineSpell
{
    public:
        SpellSymbolOfPain() : DivineSpell()
        {
            _skillNum = SPELL_SYMBOL_OF_PAIN;
            _skillName = "symbol of pain";
            _minMana = 80;
            _maxMana = 140;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _violent = true;
            _evil = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_CLERIC, 38, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellSymbolOfPain();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellStoneToFlesh : public DivineSpell
{
    public:
        SpellStoneToFlesh() : DivineSpell()
        {
            _skillNum = SPELL_STONE_TO_FLESH;
            _skillName = "stone to flesh";
            _minMana = 230;
            _maxMana = 380;
            _manaChange = 20;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_CLERIC, 36, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_RANGER, 44, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellStoneToFlesh();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellWordStun : public ArcaneSpell
{
    public:
        SpellWordStun() : ArcaneSpell()
        {
            _skillNum = SPELL_WORD_STUN;
            _skillName = "word stun";
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_NOT_SELF;
            PracticeInfo p0 = {CLASS_MAGE, 37, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellWordStun();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellTrueSeeing : public ArcaneSpell
{
    public:
        SpellTrueSeeing() : ArcaneSpell()
        {
            _skillNum = SPELL_TRUE_SEEING;
            _skillName = "true seeing";
            _minMana = 65;
            _maxMana = 210;
            _manaChange = 15;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_MAGE, 41, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 42, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellTrueSeeing();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellWordOfRecall : public DivineSpell
{
    public:
        SpellWordOfRecall() : DivineSpell()
        {
            _skillNum = SPELL_WORD_OF_RECALL;
            _skillName = "word of recall";
            _minMana = 20;
            _maxMana = 100;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM | TAR_OBJ_ROOM | TAR_OBJ_INV;
            PracticeInfo p0 = {CLASS_CLERIC, 15, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_PALADIN, 25, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellWordOfRecall();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellWordOfIntellect : public DivineSpell
{
    public:
        SpellWordOfIntellect() : DivineSpell()
        {
            _skillNum = SPELL_WORD_OF_INTELLECT;
            _skillName = "word of intellect";
            _minMana = 40;
            _maxMana = 100;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_CLERIC, 35, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellWordOfIntellect();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellPeer : public ArcaneSpell
{
    public:
        SpellPeer() : ArcaneSpell()
        {
            _skillNum = SPELL_PEER;
            _skillName = "peer";
            _minMana = 40;
            _maxMana = 100;
            _manaChange = 6;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP;
            PracticeInfo p0 = {CLASS_MAGE, 40, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellPeer();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellRemovePoison : public DivineSpell
{
    public:
        SpellRemovePoison() : DivineSpell()
        {
            _skillNum = SPELL_REMOVE_POISON;
            _skillName = "remove poison";
            _minMana = 8;
            _maxMana = 40;
            _manaChange = 4;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM;
            PracticeInfo p0 = {CLASS_CLERIC, 8, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_PALADIN, 19, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 12, 0};
            _practiceInfo.push_back(p2);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellRemovePoison();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellRestoration : public DivineSpell
{
    public:
        SpellRestoration() : DivineSpell()
        {
            _skillNum = SPELL_RESTORATION;
            _skillName = "restoration";
            _minMana = 80;
            _maxMana = 210;
            _manaChange = 11;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_CLERIC, 43, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellRestoration();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellSenseLife : public ArcaneSpell
{
    public:
        SpellSenseLife() : ArcaneSpell()
        {
            _skillNum = SPELL_SENSE_LIFE;
            _skillName = "sense life";
            _minMana = 80;
            _maxMana = 120;
            _manaChange = 4;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_MAGE, 27, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellSenseLife();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("CreatureAffect",
                    _owner, target);

            CreatureAffect *af = (CreatureAffect *)ao->getAffect();

            af->setSkillNum(SPELL_SENSE_LIFE);
            af->setLevel(level);
            af->setDuration((12 + level >> 1) MUD_HOURS);
            af->setBitvector(0, AFF_SENSE_LIFE);
            af->setAccumDuration(true);
            af->setWearoffToChar("You feel less aware of your surroundings.");

            ao->addToVictMessage("You feel your awareness improve.");

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);
            return true;

        }
};

class SpellUndeadProt : public ArcaneSpell
{
    public:
        SpellUndeadProt() : ArcaneSpell()
        {
            _skillNum = SPELL_UNDEAD_PROT;
            _skillName = "undead prot";
            _minMana = 10;
            _maxMana = 60;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_MAGE, 41, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 18, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_PALADIN, 18, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_BLACKGUARD, 18, 0};
            _practiceInfo.push_back(p3);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellUndeadProt();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellWaterwalk : public ArcaneSpell
{
    public:
        SpellWaterwalk() : ArcaneSpell()
        {
            _skillNum = SPELL_WATERWALK;
            _skillName = "waterwalk";
            _minMana = 40;
            _maxMana = 60;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_MAGE, 32, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellWaterwalk();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellIdentify : public ArcaneSpell
{
    public:
        SpellIdentify() : ArcaneSpell()
        {
            _skillNum = SPELL_IDENTIFY;
            _skillName = "identify";
            _minMana = 50;
            _maxMana = 95;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM;
            PracticeInfo p0 = {CLASS_MAGE, 25, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellIdentify();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;

            extern struct time_info_data age(struct Creature *ch);

            char *tmp;
            ExecutableObject *eo = new ExecutableObject(getOwner());
            eo->setVictim(target);

            // Resisting the spell due to target distrusting _owner should
            // be handled somewhere outside of perform.  Either in the
            // modifyEO() functions or identify should be an EO all it's
            // own.
            /*if (target->distrusts(_owner) &&
                    mag_savingthrow(target, level, SAVING_SPELL)) {
                act("$N resists your spell!", FALSE, _owner, 0,
                        target, TO_CHAR);
                return false;
            }*/
            tmp = tmp_sprintf("You feel informed:");
            eo->addToCharMessage(tmp);

            tmp = tmp_sprintf("Name: %s", target->getName());
            eo->addToCharMessage(tmp);

            if (!IS_NPC(target)) {
                tmp = tmp_sprintf(
                    "%s is %d years, %d months, %d days and %d hours old.",
                    target->getName(), GET_AGE(target), age(target).month,
                    age(target).day, age(target).hours);
                eo->addToCharMessage(tmp);
            }

            tmp = tmp_sprintf("Race: %s, Class: %s, Alignment: %d.",
                player_race[(int)MIN(NUM_RACES, GET_RACE(target))],
                pc_char_class_types[(int)MIN(TOP_CLASS, GET_CLASS(target))],
                GET_ALIGNMENT(target));
            eo->addToCharMessage(tmp);
            
            tmp = tmp_sprintf("Height %d cm, Weight %d pounds",
                GET_HEIGHT(target), GET_WEIGHT(target));
            eo->addToCharMessage(tmp);

            tmp = tmp_sprintf("Level: %d, Hits: %d, Mana: %d",
                target->getLevel(), target->getMaxHit(), target->getMaxMana());
            eo->addToCharMessage(tmp);

            tmp = tmp_sprintf("AC: %d, Thac0: %d, Hitroll: %d (%d), "
                    "Damroll: %d",
                GET_AC(target), (int)MIN(THACO(GET_CLASS(target),
                        target->getLevel()), THACO(GET_REMORT_CLASS(target),
                        target->getLevel())), target->getHitroll(),
                GET_REAL_HITROLL(target), target->getDamroll());
            eo->addToCharMessage(tmp);

            tmp = tmp_sprintf( 
                "Str: %d/%d, Int: %d, Wis: %d, Dex: %d, Con: %d, Cha: %d",
                target->getStr(), GET_ADD(target), target->getInt(),
                target->getWis(), target->getDex(), target->getCon(),
                target->getCha());
            eo->addToCharMessage(tmp);

            eo->setSkill(getSkillNumber());
            ev.push_back_id(eo);

            return true;
        }

        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev, obj))
                return false;

            int i;
            int found;
            char buf[256];
            char *tmp;
            ExecutableObject *eo = new ExecutableObject(getOwner());
            eo->setObject(obj);

            extern const char *instrument_types[];

            tmp = tmp_sprintf("You feel informed:");
            eo->addToCharMessage(tmp);

            tmp = tmp_sprintf("Object '%s', Item type: ", obj->name);
            memset(buf, 0x0, sizeof(buf));
            sprinttype(GET_OBJ_TYPE(obj), item_types, buf);
            tmp = tmp_strcat(tmp, buf, NULL);
            eo->addToCharMessage(tmp);

           tmp = tmp_sprintf("Item will give you following abilities: ");
            strcpy(buf, "");
            if (obj->obj_flags.bitvector[0]) {
                sprintbit(obj->obj_flags.bitvector[0], affected_bits, buf);
                tmp = tmp_strcat(tmp, buf, NULL);
            } 
            if (obj->obj_flags.bitvector[1]) {
                sprintbit(obj->obj_flags.bitvector[1], affected2_bits, buf);
                tmp = tmp_strcat(tmp, " ", buf, NULL);
            }
            if (obj->obj_flags.bitvector[2]) {
                sprintbit(obj->obj_flags.bitvector[2], affected3_bits, buf);
                tmp = tmp_strcat(tmp, " ", buf, NULL);
            }
            eo->addToCharMessage(tmp);

            tmp = tmp_sprintf("Item is: ");
            if (!GET_OBJ_EXTRA(obj))
                tmp = tmp_strcat(tmp, "NOBITS", NULL);
            else
                tmp = tmp_strcat(tmp, tmp_printbits(GET_OBJ_EXTRA(obj), 
                            extra_bits) , NULL);
            eo->addToCharMessage(tmp);

            tmp = tmp_sprintf("Item is also: ");
            if (!GET_OBJ_EXTRA2(obj))
                tmp = tmp_strcat(tmp, "NOBITS", NULL);
            else
                tmp = tmp_strcat(tmp, tmp_printbits(GET_OBJ_EXTRA2(obj), 
                            extra2_bits), NULL);
            eo->addToCharMessage(tmp);

            tmp = tmp_sprintf("Item is also: ");
            if (!GET_OBJ_EXTRA3(obj))
                tmp = tmp_strcat(tmp, "NOBITS", NULL);
            else
                tmp = tmp_strcat(tmp, tmp_printbits(GET_OBJ_EXTRA3(obj), 
                            extra3_bits), NULL);
            eo->addToCharMessage(tmp);

            tmp = tmp_sprintf("Weight: %d, Value: %d, Rent: %d",
                obj->getWeight(), GET_OBJ_COST(obj), GET_OBJ_RENT(obj));
            eo->addToCharMessage(tmp);

            tmp = tmp_sprintf("Item material is %s.",
                material_names[GET_OBJ_MATERIAL(obj)]);
            eo->addToCharMessage(tmp);

            switch (GET_OBJ_TYPE(obj)) {
            case ITEM_SCROLL:
            case ITEM_POTION:
            case ITEM_PILL:
            case ITEM_SYRINGE:
                tmp = tmp_sprintf("This %s casts: ",
                    item_types[(int)GET_OBJ_TYPE(obj)]);

                if (GET_OBJ_VAL(obj, 1) >= 1)
                    tmp = tmp_strcat(tmp, spell_to_str(GET_OBJ_VAL(obj, 1)), 
                            " ", NULL);
                if (GET_OBJ_VAL(obj, 2) >= 1)
                    tmp = tmp_strcat(tmp, spell_to_str(GET_OBJ_VAL(obj, 2)), 
                            " ", NULL);
                if (GET_OBJ_VAL(obj, 3) >= 1)
                    tmp = tmp_strcat(tmp, spell_to_str(GET_OBJ_VAL(obj, 3)), 
                            " ", NULL);
                eo->addToCharMessage(tmp);
                break;
            case ITEM_WAND:
            case ITEM_STAFF:
                tmp = tmp_sprintf("This %s casts: %s at level %d",
                    item_types[(int)GET_OBJ_TYPE(obj)],
                    spell_to_str(GET_OBJ_VAL(obj, 3)),
                    GET_OBJ_VAL(obj, 0));
                eo->addToCharMessage(tmp);

                tmp = tmp_sprintf("It has %d maximum charge%s and "
                        "%d remaining.",
                    GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 1) == 1 ? "" : "s",
                    GET_OBJ_VAL(obj, 2));
                eo->addToCharMessage(tmp);
                break;
            case ITEM_WEAPON:
                tmp = tmp_sprintf(
                        "Damage Dice is '%dD%d' for an average per-round "
                        "damage of %.1f.", 
                        GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2),
                        (((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1)));
                eo->addToCharMessage(tmp);
                if (IS_OBJ_STAT2(obj, ITEM2_CAST_WEAPON)) {
                    tmp = tmp_sprintf("This weapon casts: %s",
                        spell_to_str(GET_OBJ_VAL(obj, 0)));
                    eo->addToCharMessage(tmp);
                }

                break;
            case ITEM_ARMOR:
                tmp = tmp_sprintf("AC-apply is %d", 
                        GET_OBJ_VAL(obj, 0));
                eo->addToCharMessage(tmp);
                break;
            case ITEM_HOLY_SYMB:
                tmp = tmp_sprintf(
                    "Alignment: %s, Class: %s, Min Level: %d, "
                    "Max Level: %d",
                    alignments[(int)GET_OBJ_VAL(obj, 0)],
                    char_class_abbrevs[(int)GET_OBJ_VAL(obj, 1)], 
                    GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 3));
                eo->addToCharMessage(tmp);
                break;
            case ITEM_TOBACCO:
                tmp = tmp_sprintf("Smoke type is: %s%s%s",
                    CCYEL(_owner, C_NRM), smoke_types[SMOKE_TYPE(obj)],
                    CCNRM(_owner, C_NRM));
                eo->addToCharMessage(tmp);
                break;
            case ITEM_CONTAINER:
                tmp = tmp_sprintf("This container holds a "
                        "maximum of %d pounds.",
                    GET_OBJ_VAL(obj, 0));
                eo->addToCharMessage(tmp);
                break;

            case ITEM_TOOL:
                tmp = tmp_sprintf("Tool works with: %s, "
                        "modifier: %d\r\n", spell_to_str(TOOL_SKILL(obj)), 
                        TOOL_MOD(obj));
                eo->addToCharMessage(tmp);
                break;
            case ITEM_INSTRUMENT:
                tmp = tmp_sprintf("Instrument type is: %s%s%s",
                             CCCYN(_owner, C_NRM), 
                             (GET_OBJ_VAL(obj, 0) < 2) ? 
                             instrument_types[GET_OBJ_VAL(obj, 0)] : "UNDEFINED",
                             CCNRM(_owner, C_NRM));
                eo->addToCharMessage(tmp);
                break;
            }
            found = FALSE;
            for (i = 0; i < MAX_OBJ_AFFECT; i++) {
                if ((obj->affected[i].location != APPLY_NONE) &&
                    (obj->affected[i].modifier != 0)) {
                    if (!found) {
                        eo->addToCharMessage("Can affect you as:");
                        found = TRUE;
                    }
                    sprinttype(obj->affected[i].location, apply_types, buf);
                    tmp = tmp_sprintf("   Affects: %s By %d", buf,
                        obj->affected[i].modifier);
                    eo->addToCharMessage(tmp);
                }
            }
            if (GET_OBJ_SIGIL_IDNUM(obj)) {
                const char *name;

                name = playerIndex.getName(GET_OBJ_SIGIL_IDNUM(obj));

                if (name)
                    tmp = tmp_sprintf(
                        "You detect a warding sigil bearing "
                        "the mark of %s.\r\n", name);
                else
                    tmp = tmp_sprintf(
                        "You detect an warding sigil with an "
                        "unfamiliar marking.\r\n");

                eo->addToCharMessage(tmp);
            }
            
            eo->setSkill(getSkillNumber());
            ev.push_back_id(eo);

            return true;
        }
};

class SpellWallOfThorns : public ArcaneSpell
{
    public:
        SpellWallOfThorns() : ArcaneSpell()
        {
            _skillNum = SPELL_WALL_OF_THORNS;
            _skillName = "wall of thorns";
            _minMana = 20;
            _maxMana = 50;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _noList = true;
            _targets = TAR_DOOR;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellWallOfThorns();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellWallOfStone : public ArcaneSpell
{
    public:
        SpellWallOfStone() : ArcaneSpell()
        {
            _skillNum = SPELL_WALL_OF_STONE;
            _skillName = "wall of stone";
            _minMana = 20;
            _maxMana = 50;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_DOOR;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellWallOfStone();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellWallOfIce : public ArcaneSpell
{
    public:
        SpellWallOfIce() : ArcaneSpell()
        {
            _skillNum = SPELL_WALL_OF_ICE;
            _skillName = "wall of ice";
            _minMana = 20;
            _maxMana = 50;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_DOOR;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellWallOfIce();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellWallOfFire : public ArcaneSpell
{
    public:
        SpellWallOfFire() : ArcaneSpell()
        {
            _skillNum = SPELL_WALL_OF_FIRE;
            _skillName = "wall of fire";
            _minMana = 20;
            _maxMana = 50;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_DOOR;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellWallOfFire();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellWallOfIron : public ArcaneSpell
{
    public:
        SpellWallOfIron() : ArcaneSpell()
        {
            _skillNum = SPELL_WALL_OF_IRON;
            _skillName = "wall of iron";
            _minMana = 20;
            _maxMana = 50;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_DOOR;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellWallOfIron();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellThornTrap : public ArcaneSpell
{
    public:
        SpellThornTrap() : ArcaneSpell()
        {
            _skillNum = SPELL_THORN_TRAP;
            _skillName = "thorn trap";
            _minMana = 20;
            _maxMana = 50;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_DOOR;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellThornTrap();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellFierySheet : public ArcaneSpell
{
    public:
        SpellFierySheet() : ArcaneSpell()
        {
            _skillNum = SPELL_FIERY_SHEET;
            _skillName = "fiery sheet";
            _minMana = 20;
            _maxMana = 50;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_DOOR;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellFierySheet();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellPower : public PsionicSpell
{
    public:
        SpellPower() : PsionicSpell()
        {
            _skillNum = SPELL_POWER;
            _skillName = "power";
            _minMana = 50;
            _maxMana = 80;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PSIONIC, 15, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellPower();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, target);
            ao->setSkill(getSkillNumber());
            ao->addToVictMessage("A psychic finger on your brain makes you feel stronger!");
            ev.push_back_id(ao);

            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(_skillNum);
            ca->setWearoffToChar("The strength zone of your brain relaxes.");
            ca->setTarget(target);
            ca->setLevel(level);
            ca->setDuration((1 + level >> 2) MUD_HOURS);
            ca->setApply(APPLY_STR, 1 + level >> 4);

            return true;
        }
};

class SpellIntellect : public PsionicSpell
{
    public:
        SpellIntellect() : PsionicSpell()
        {
            _skillNum = SPELL_INTELLECT;
            _skillName = "intellect";
            _minMana = 40;
            _maxMana = 60;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PSIONIC, 23, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellIntellect();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, target);
            ao->addToVictMessage("Your mental faculties improve!");
            ao->setSkill(_skillNum);

            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(_skillNum);
            ca->setWearoffToChar("Your mental center of logic relaxes.");
            ca->setTarget(target);
            ca->setLevel(level);
            ca->setDuration((level >> 2 + 4) MUD_HOURS);
            ca->setApply(APPLY_INT, 1 + dice(1, (level >> 4)));

            ev.push_back_id(ao);

            return true;
        }
};

class SpellConfusion : public PsionicSpell
{
    public:
        SpellConfusion() : PsionicSpell()
        {
            _skillNum = SPELL_CONFUSION;
            _skillName = "confusion";
            _minMana = 80;
            _maxMana = 110;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_PSIONIC, 36, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellConfusion();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PsionicSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellFear : public PsionicSpell
{
    public:
        SpellFear() : PsionicSpell()
        {
            _skillNum = SPELL_FEAR;
            _skillName = "fear";
            _minMana = 30;
            _maxMana = 60;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_UNPLEASANT;
            PracticeInfo p0 = {CLASS_PSIONIC, 29, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellFear();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            // FIXME: Devils, undead, and dragons need to resist thi via their racial modifyEOs
            // if (IS_UNDEAD(victim) || IS_DRAGON(victim) || IS_DEVIL(victim)) {
            //     act("You fail to affect $N!", FALSE, ch, 0, victim, TO_CHAR);
            //     send_to_char(ch, "You feel a wave of fear pass over you!\r\n");
            //     return;
            // }
            //
            AffectObject *ao = new AffectObject("FearAffect", _owner, target);
            ao->setSkill(getSkillNumber());
            ao->addToVictMessage("You suddenly feel very afraid!");
            ao->addToRoomMessage("$N looks very afraid!");
            ev.push_back_id(ao);

            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(_skillNum);
            ca->setTarget(target);
            ca->setLevel(level);
            ca->setDuration((1 + level >> 4) MUD_HOURS);

            return true;
        }
};

class SpellSatiation : public PsionicSpell
{
    public:
        SpellSatiation() : PsionicSpell()
        {
            _skillNum = SPELL_SATIATION;
            _skillName = "satiation";
            _minMana = 10;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PSIONIC, 3, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellSatiation();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            SatiationObject *so = new SatiationObject(_owner, target);
            so->setHunger(dice(3, MIN(3, (1 + level / 10))));
            so->setDrunk(-(dice(2, MIN(3, (1 + level / 10)))));
            so->addToVictMessage("You feel satiated.");

            ev.push_back_id(so);
            return true;
        }
};

class SpellQuench : public PsionicSpell
{
    public:
        SpellQuench() : PsionicSpell()
        {
            _skillNum = SPELL_QUENCH;
            _skillName = "quench";
            _minMana = 15;
            _maxMana = 35;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PSIONIC, 4, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellQuench();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            SatiationObject *so = new SatiationObject(_owner, target);
            so->setThirst(dice(3, MIN(3, (1 + level / 10))));
            so->addToVictMessage("Your thirst is quenched.");

            ev.push_back_id(so);
            return true;
        }
};

class SpellConfidence : public PsionicSpell
{
    public:
        SpellConfidence() : PsionicSpell()
        {
            _skillNum = SPELL_CONFIDENCE;
            _skillName = "confidence";
            _minMana = 40;
            _maxMana = 60;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PSIONIC, 8, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellConfidence();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, target);
            ao->setSkill(getSkillNumber());
            ao->addToVictMessage("You suddenly feel very confident!");
            ev.push_back_id(ao);

            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(_skillNum);
            ca->setAccumDuration(true);
            ca->setWearoffToChar("You feel less confident now.");
            ca->setTarget(target);
            ca->setLevel(level);
            ca->setDuration((3 + level >> 3) MUD_HOURS);
            ca->setApply(APPLY_HITROLL, dice(2, (level >> 4) + 1));
            ca->setBitvector(0, AFF_CONFIDENCE);

            RemoveAffectObject *rao;
            rao = new RemoveAffectObject(SPELL_FEAR, _owner, target);
            rao->setSkill(_skillNum);
            ev.push_back_id(rao);

            return true;
        }
};

class SpellNopain : public PsionicSpell
{
    public:
        SpellNopain() : PsionicSpell()
        {
            _skillNum = SPELL_NOPAIN;
            _skillName = "neural dampening";
            _minMana = 100;
            _maxMana = 130;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PSIONIC, 30, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellNopain();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            string toVictMessage = "You feel like you can take on anything!";
            string toRoomMessage = "$n ripples $s muscles and grins insanely!";

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, target);

            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(SPELL_NOPAIN);
            ca->setWearoffToChar("Your body suddenly becomes aware "
                    "of pain again.");
            ca->setTarget(target);
            ca->setBitvector(0, AFF_NOPAIN);
            ca->setLevel(level);
            ca->setDuration((6 + dice(3, (ca->getLevel() >> 4))) MUD_HOURS);
            if (_owner == target)
                ca->setDuration((ca->getDuration() * 2) MUD_HOURS);

            ao->addToVictMessage(toVictMessage);
            ao->addToRoomMessage(toRoomMessage);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);
          
            return true;
        }
};

class SpellDermalHardening : public PsionicSpell
{
    public:
        SpellDermalHardening() : PsionicSpell()
        {
            _skillNum = SPELL_DERMAL_HARDENING;
            _skillName = "dermal hardening";
            _minMana = 30;
            _maxMana = 70;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PSIONIC, 12, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellDermalHardening();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, target);
            ao->addToVictMessage("You feel your skin tighten up and thicken.");
            ao->setSkill(_skillNum);
            ev.push_back_id(ao);

            CreatureAffect *af = (CreatureAffect *)ao->getAffect();
	        af->setSkillNum(_skillNum);
            af->setWearoffToChar("You skin relaxes and thins back out.");
            af->setLevel(level);
            af->setDuration(dice(4, (level >> 3) + 1) MUD_HOURS);
            af->setAccumDuration(true);

            return true;
        }
};

class SpellWoundClosure : public PsionicSpell
{
    public:
        SpellWoundClosure() : PsionicSpell()
        {
            _skillNum = SPELL_WOUND_CLOSURE;
            _skillName = "wound closure";
            _minMana = 30;
            _maxMana = 80;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PSIONIC, 10, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellWoundClosure();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            PointsObject *po = new PointsObject(_owner, target);
            po->setSkill(_skillNum);
            po->setHit(dice(3, level));

            if (_owner != target) {
                po->addToCharMessage("Some of $N's wounds seal before your eyes.");
            }
            po->addToVictMessage("Some of your wounds seal before your eyes.");
            po->addToRoomMessage("Some of $N's wounds seal before your eyes.");

            ev.push_back_id(po);
            return true;
        }
};

class SpellAntibody : public PsionicSpell
{
    public:
        SpellAntibody() : PsionicSpell()
        {
            _skillNum = SPELL_ANTIBODY;
            _skillName = "antibody";
            _minMana = 30;
            _maxMana = 80;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PSIONIC, 19, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellAntibody();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            RemoveAffectObject *rao;
            rao = new RemoveAffectObject(SPELL_POISON, _owner, target);
            rao->addToVictMessage("A warm feeling runs through your body!");
            rao->addToRoomMessage("$N looks much better.");
            rao->setSkill(_skillNum);
            ev.push_back_id(rao);

            if (level > 30) {
                rao = new RemoveAffectObject(SPELL_SICKNESS, _owner, target);
                rao->setSkill(_skillNum);
                ev.push_back_id(rao);
            }

            return true;
        }
};

class SpellRetina : public PsionicSpell
{
    public:
        SpellRetina() : PsionicSpell()
        {
            _skillNum = SPELL_RETINA;
            _skillName = "retina";
            _minMana = 25;
            _maxMana = 60;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PSIONIC, 13, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellRetina();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, target);
            ao->addToVictMessage("The rods of your retina are stimulated");
            ao->addToRoomMessage("$n's eyes shine brightly.");
            ao->setSkill(_skillNum);
            ev.push_back_id(ao);

            CreatureAffect *af = (CreatureAffect *)ao->getAffect();
	        af->setSkillNum(_skillNum);
            af->setWearoffToChar("Your retina loses some of it's sensitivity");
            af->setLevel(level);
            af->setDuration((20 + (level >> 3)) MUD_HOURS);
            af->setBitvector(0, AFF_RETINA);

            return true;
        }
};

class SpellAdrenaline : public PsionicSpell
{
    public:
        SpellAdrenaline() : PsionicSpell()
        {
            _skillNum = SPELL_ADRENALINE;
            _skillName = "adrenaline";
            _minMana = 60;
            _maxMana = 80;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PSIONIC, 27, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellAdrenaline();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, target);
            ao->setSkill(getSkillNumber());
            ao->addToVictMessage("A rush of adrenaline hits your brain.");
            ev.push_back_id(ao);

            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(_skillNum);
            ca->setWearoffToChar("Your adrenaline flow ceases.");
            ca->setTarget(target);
            ca->setLevel(level);
            ca->setDuration((3 + level >> 3) MUD_HOURS);
            ca->setApply(APPLY_HITROLL, (dice(1, 1 + level >> 3)));
            ca->setBitvector(0, AFF_ADRENALINE);

            return true;
        }
};

class SpellBreathingStasis : public PsionicSpell
{
    public:
        SpellBreathingStasis() : PsionicSpell()
        {
            _skillNum = SPELL_BREATHING_STASIS;
            _skillName = "breathing stasis";
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PSIONIC, 21, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellBreathingStasis();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, target);
            ao->setSkill(getSkillNumber());
            ao->addToVictMessage("Your breathing rate drops into a static state.");
            ev.push_back_id(ao);

            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(_skillNum);
            ca->setWearoffToChar("You begin to breathe again.");
            ca->setTarget(target);
            ca->setLevel(level);
            ca->setDuration((dice(1, 1 + level >> 3) * (level >> 4)) MUD_HOURS);
            ca->setApply(APPLY_MOVE, -(50 - (level >> 2)));
            ca->setBitvector(2, AFF3_NOBREATHE);

            return true;
        }
};

class SpellVertigo : public PsionicSpell
{
    public:
        SpellVertigo() : PsionicSpell()
        {
            _skillNum = SPELL_VERTIGO; _skillName = "vertigo"; _minMana = 50;
            _maxMana = 70;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_NOT_SELF | TAR_UNPLEASANT;
            PracticeInfo p0 = {CLASS_PSIONIC, 31, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellVertigo();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();
            
            AffectObject *ao = new AffectObject("CreatureAffect", _owner, target);
            ao->setSkill(getSkillNumber());
            ao->addToCharMessage("$n staggers in a dazed way.");
            ao->addToVictMessage("You feel a wave of vertigo rush over you!");
            ao->addToRoomMessage("$n staggers in a dazed way.");
            ev.push_back_id(ao);

            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(_skillNum);
            ca->setTarget(target);
            ca->setLevel(level);
            ca->setWearoffToChar("You feel the vertigo fade.");
            ca->setDuration(10 MUD_HOURS);
            ca->setApply(APPLY_HITROLL, -(level >> 3));
            ca->setApply(APPLY_DEX, -(1 + level >> 4));
            ca->setBitvector(1, AFF2_VERTIGO);

            return true;
        }
};

class SpellMetabolism : public PsionicSpell
{
    public:
        SpellMetabolism() : PsionicSpell()
        {
            _skillNum = SPELL_METABOLISM;
            _skillName = "metabolism";
            _minMana = 30;
            _maxMana = 50;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PSIONIC, 9, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellMetabolism();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, target);
            ao->setSkill(getSkillNumber());
            ao->addToVictMessage("Your metabolism speeds up.");
            ev.push_back_id(ao);

            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(_skillNum);
            ca->setWearoffToChar("Your metabolism slows.");
            ca->setTarget(target);
            ca->setLevel(level);
            ca->setDuration(dice(2, (2 + level >> 3)) MUD_HOURS);
            ca->setApply(APPLY_SAVING_CHEM, level >> 3);

            return true;
        }
};

class SpellEgoWhip : public PsionicSpell
{
    public:
        SpellEgoWhip() : PsionicSpell()
        {
            _skillNum = SPELL_EGO_WHIP;
            _skillName = "ego whip";
            _minMana = 30;
            _maxMana = 50;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_PSIONIC, 22, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellEgoWhip();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();
            // Set up the damage object
            DamageObject *do1 = new DamageObject(getOwner(), target);

            do1->setDamageType(SPELL_EGO_WHIP);
            do1->setDamage(dice(5, level));

            do1->addToCharMessage("Your psychic whip lashes $N's ego!");
            do1->addToVictMessage("A psychic whip from $n lashes your ego!");
            do1->addToRoomMessage("$n's psychic whip lases $N's ego!!");

            do1->setSkill(_skillNum);
            ev.push_back_id(do1);

            if (target->getPosition() > POS_SITTING) {
                if ((number(5, level / 3) > target->getDex() || !random_fractional_10())) {
                    PositionObject *po = new PositionObject(_owner, target, POS_SITTING);
                    po->addToCharMessage("$n is knocked to the ground by the psychic attack!");
                    po->addToVictMessage("You are knocked to the ground by the psychic attack!");
                    po->addToRoomMessage("$n is knocked to the ground by the psychic attack!");
                    po->setSkill(_skillNum);
                    ev.push_back_id(po);

                }
            }
            return true;
        }
};

class SpellPsychicCrush : public PsionicSpell
{
    public:
        SpellPsychicCrush() : PsionicSpell()
        {
            _skillNum = SPELL_PSYCHIC_CRUSH;
            _skillName = "psychic crush";
            _minMana = 30;
            _maxMana = 130;
            _manaChange = 15;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_PSIONIC, 35, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellPsychicCrush();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("PsychicCrushAffect", _owner, target);
            ao->setSkill(_skillNum);
            ev.push_back_id(ao);

            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(_skillNum);
            ca->setTarget(target);
            ca->setLevel(level);
            ca->setDuration((dice(1, 1 + level >> 3) + 2) MUD_HOURS);
            ca->setBitvector(2, AFF3_PSYCHIC_CRUSH);

            DamageObject *do1 = new DamageObject(_owner, target);
            do1->setSkill(_skillNum);
            do1->setDamageType(_skillNum);
            do1->setDamage(dice(level, 10));
            do1->addToCharMessage("$N howls with pain as a psychic force crushes $S skull!");
            do1->addToVictMessage("You howl with pain as a psychic force crushes your skull!");
            do1->addToRoomMessage("$N howls with pain as a psychic force crushes $S skull!");
            ev.push_back(do1);

            ev.push_back_id(do1);

            return true;
        }
};

class SpellRelaxation : public PsionicSpell
{
    public:
        SpellRelaxation() : PsionicSpell()
        {
            _skillNum = SPELL_RELAXATION;
            _skillName = "relaxation";
            _minMana = 40;
            _maxMana = 100;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PSIONIC, 7, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellRelaxation();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, target);
            ao->setSkill(getSkillNumber());
            ao->addToVictMessage("Your mind and body relax.");
            ev.push_back_id(ao);

            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(_skillNum);
            ca->setWearoffToChar("You feel less relaxed.");
            ca->setTarget(target);
            ca->setLevel(level);
            ca->setDuration(dice(2, 2 + (level >> 3)) MUD_HOURS);
            ca->setApply(APPLY_MOVE, -(35 + level));
            ca->setApply(APPLY_STR, -1);
            ca->setBitvector(2, AFF3_MANA_TAP);

            RemoveAffectObject *rao;
            rao = new RemoveAffectObject(SPELL_MOTOR_SPASM, _owner, target);
            rao->setSkill(_skillNum);
            ev.push_back_id(rao);

            rao = new RemoveAffectObject(SPELL_ADRENALINE, _owner, target);
            rao->setSkill(_skillNum);
            ev.push_back_id(rao);

            rao = new RemoveAffectObject(SKILL_BERSERK, _owner, target);
            rao->setSkill(_skillNum);
            ev.push_back_id(rao);

            return true;
        }
};

class SpellWeakness : public PsionicSpell
{
    public:
        SpellWeakness() : PsionicSpell()
        {
            _skillNum = SPELL_WEAKNESS;
            _skillName = "weakness";
            _minMana = 50;
            _maxMana = 70;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_UNPLEASANT;
            PracticeInfo p0 = {CLASS_PSIONIC, 16, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellWeakness();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, target);
            ao->setSkill(getSkillNumber());
            ao->addToVictMessage("A psychic finger on your brain makes you feel weaker!");
            ev.push_back_id(ao);

            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(_skillNum);
            ca->setWearoffToChar("You feel your weakness pass.");
            ca->setTarget(target);
            ca->setLevel(level);
            ca->setDuration((1 + level >> 2) MUD_HOURS);
            ca->setApply(APPLY_STR, -(1 + level >> 4));

            return true;
        }
};

class SpellFortressOfWill : public PsionicSpell
{
    public:
        SpellFortressOfWill() : PsionicSpell()
        {
            _skillNum = SPELL_FORTRESS_OF_WILL;
            _skillName = "fortress of will";
            _minMana = 30;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellFortressOfWill();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PsionicSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellCellRegen : public PsionicSpell
{
    public:
        SpellCellRegen() : PsionicSpell()
        {
            _skillNum = SPELL_CELL_REGEN;
            _skillName = "cell regen";
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PSIONIC, 34, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellCellRegen();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("RegenAffect", _owner, target);
            ao->setSkill(getSkillNumber());
            ao->addToVictMessage("Your cell regeneration rate increases.");
            ev.push_back_id(ao);

            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(_skillNum);
            ca->setTarget(target);
            ca->setLevel(level);
            ca->setWearoffToChar("Your cell regeneration rate slows.");
            ca->setDuration((dice(1, 1 + level >> 3) + 6) MUD_HOURS);
            ca->setAccumDuration(true);
            ca->setApply(APPLY_CON, (1 + level / 50));

            return true;
        }
};

class SpellPsishield : public PsionicSpell
{
    public:
        SpellPsishield() : PsionicSpell()
        {
            _skillNum = SPELL_PSISHIELD;
            _skillName = "psionic shield";
            _minMana = 60;
            _maxMana = 120;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PSIONIC, 30, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellPsishield();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("PsishieldAffect", _owner, target);
            ao->setSkill(getSkillNumber());
            ao->addToVictMessage("You feel a psionic shield form around your mind.");
            ev.push_back_id(ao);

            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(_skillNum);
            ca->setTarget(target);
            ca->setLevel(level);
            ca->setDuration((dice(1, 1 + (level >> 3)) + 3) MUD_HOURS);
            ca->setBitvector(2, AFF3_PSISHIELD);

            return true;
        }
};

class SpellPsychicSurge : public PsionicSpell
{
    public:
        SpellPsychicSurge() : PsionicSpell()
        {
            _skillNum = SPELL_PSYCHIC_SURGE;
            _skillName = "psychic surge";
            _minMana = 60;
            _maxMana = 90;
            _manaChange = 4;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_NOT_SELF;
            PracticeInfo p0 = {CLASS_PSIONIC, 40, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellPsychicSurge();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PsionicSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellPsychicConduit : public PsionicSpell
{
    public:
        SpellPsychicConduit() : PsionicSpell()
        {
            _skillNum = SPELL_PSYCHIC_CONDUIT;
            _skillName = "psychic conduit";
            _minMana = 50;
            _maxMana = 70;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM | TAR_NOT_SELF;
            PracticeInfo p0 = {CLASS_PSIONIC, 26, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellPsychicConduit();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev))
                return false;

            int level = getPowerLevel();

	        int manaMod = level + _owner->checkSkill(_skillNum) / 10 + 
                    number(0, _owner->getWis()) + (_owner->getGen() << 3);
            PointsObject *po = new PointsObject(_owner, target);
            po->setSkill(_skillNum);
            po->setMana(manaMod);
            ev.push_back_id(po);
	    
            return true;

        }
};

class SpellPsionicShatter : public PsionicSpell
{
    public:
        SpellPsionicShatter() : PsionicSpell()
        {
            _skillNum = SPELL_PSIONIC_SHATTER;
            _skillName = "psionic shatter";
            _minMana = 30;
            _maxMana = 80;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_PSIONIC, 33, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellPsionicShatter();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            if (!target->affectedBy(SPELL_PSISHIELD)) {
                ExecutableObject *eo = new ExecutableObject(_owner);
                eo->setVictim(target);
                eo->addToCharMessage("$N is not pretected by a psionic shield.");
                return true;
            }

            RemoveAffectObject *rao = 
                new RemoveAffectObject(SPELL_PSISHIELD, _owner, target);
            rao->setSkill(_skillNum);
            ev.push_back_id(rao);

            return true;
        }
};

class SpellMelatonicFlood : public PsionicSpell
{
    public:
        SpellMelatonicFlood() : PsionicSpell()
        {
            _skillNum = SPELL_MELATONIC_FLOOD;
            _skillName = "melatonic flood";
            _minMana = 30;
            _maxMana = 90;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_UNPLEASANT;
            PracticeInfo p0 = {CLASS_PSIONIC, 18, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellMelatonicFlood();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            string toVictMessage = "You feel very sleepy...ZZzzzz...";
            string toCharMessage = "$N goes to sleep.";
            string toRoomMessage = "$N goes to sleep.";

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, target);
            CreatureAffect *af = (CreatureAffect *)ao->getAffect();
            af->setSkillNum(_skillNum);
            af->setWearoffToChar("You feel less tired.");
            af->setLevel(level);
            af->setDuration((1 + level / 10) MUD_HOURS);
            af->setBitvector(0, AFF_SLEEP);
            af->setPosition(POS_SLEEPING);

            ao->addToVictMessage(toVictMessage);
            ao->addToCharMessage(toCharMessage);
            ao->addToRoomMessage(toRoomMessage);
            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SpellMotorSpasm : public PsionicSpell
{
    public:
        SpellMotorSpasm() : PsionicSpell()
        {
            _skillNum = SPELL_MOTOR_SPASM;
            _skillName = "motor spasm";
            _minMana = 70;
            _maxMana = 100;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_PSIONIC, 37, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellMotorSpasm();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PsionicSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellPsychicResistance : public PsionicSpell
{
    public:
        SpellPsychicResistance() : PsionicSpell()
        {
            _skillNum = SPELL_PSYCHIC_RESISTANCE;
            _skillName = "psychic resistance";
            _minMana = 30;
            _maxMana = 80;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PSIONIC, 20, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellPsychicResistance();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, target);
            ao->setSkill(getSkillNumber());
            ao->addToVictMessage("The psychic conduits of you mind before resistant to "
                    "external energies.");
            ev.push_back_id(ao);

            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(_skillNum);
            ca->setWearoffToChar("Your psychic resistance passes.");
            ca->setTarget(target);
            ca->setLevel(level);
            ca->setDuration((dice(1, 1 + level >> 3) + 6) MUD_HOURS);
            ca->setAccumDuration(true);
            ca->setApply(APPLY_SAVING_PSI, -(1 + (level >> 2)));

            return true;
        }
};

class SpellMassHysteria : public PsionicSpell
{
    public:
        SpellMassHysteria() : PsionicSpell()
        {
            _skillNum = SPELL_MASS_HYSTERIA;
            _skillName = "mass hysteria";
            _minMana = 30;
            _maxMana = 100;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _violent = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_PSIONIC, 48, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellMassHysteria();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PsionicSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellGroupConfidence : public PsionicSpell
{
    public:
        SpellGroupConfidence() : PsionicSpell()
        {
            _skillNum = SPELL_GROUP_CONFIDENCE;
            _skillName = "group confidence";
            _minMana = 40;
            _maxMana = 60;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_IGNORE;
            _flags = MAG_GROUPS;
            PracticeInfo p0 = {CLASS_PSIONIC, 14, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellGroupConfidence();
        }

        bool perform(ExecutableVector &ev, char *targets) {
            if (!PsionicSpell::perform(ev, targets))
                return false;

            int level = getPowerLevel();

            Creature *k;
            struct follow_type *f;
            int num = 0;

            k = (_owner->master) ? _owner->master : _owner;
            for (f = k->followers; f; f = f->next) {
                if (!IS_AFFECTED(f->follower, AFF_GROUP))
                    continue;

                num++;
                if (num > 1)
                    ev.push_back_id(generateCost());

                AffectObject *ao = new AffectObject("CreatureAffect", _owner, f->follower);
                ao->setSkill(getSkillNumber());
                ao->addToVictMessage("You suddenly feel very confident!");
                ev.push_back_id(ao);

                CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
                ca->setSkillNum(_skillNum);
                ca->setAccumDuration(true);
                ca->setWearoffToChar("You feel less confident now.");
                ca->setTarget(f->follower);
                ca->setLevel(level);
                ca->setDuration((3 + level >> 3) MUD_HOURS);
                ca->setApply(APPLY_HITROLL, dice(2, (level >> 4) + 1));
                ca->setBitvector(0, AFF_CONFIDENCE);

                RemoveAffectObject *rao;
                rao = new RemoveAffectObject(SPELL_FEAR, _owner, f->follower);
                rao->setSkill(_skillNum);
                ev.push_back_id(rao);
            }

            return true;
        }
};

class SpellClumsiness : public PsionicSpell
{
    public:
        SpellClumsiness() : PsionicSpell()
        {
            _skillNum = SPELL_CLUMSINESS;
            _skillName = "clumsiness";
            _minMana = 50;
            _maxMana = 70;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_UNPLEASANT;
            PracticeInfo p0 = {CLASS_PSIONIC, 28, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellClumsiness();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, target);
            ao->setSkill(getSkillNumber());
            ao->addToVictMessage("A psychic finger on your brain makes you feel less agile!");
            ev.push_back_id(ao);

            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(_skillNum);
            ca->setWearoffToChar("You feel less clumsy.");
            ca->setTarget(target);
            ca->setLevel(level);
            ca->setDuration((6 + level >> 3) MUD_HOURS);
            ca->setApply(APPLY_DEX, -(dice(1, 1 + level >> 3)));

            return true;
        }
};

class SpellEndurance : public PsionicSpell
{
    public:
        SpellEndurance() : PsionicSpell()
        {
            _skillNum = SPELL_ENDURANCE;
            _skillName = "endurance";
            _minMana = 50;
            _maxMana = 90;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PSIONIC, 11, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellEndurance();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, target);
            ao->addToVictMessage("You feel your energy capacity rise");
            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            CreatureAffect *af = (CreatureAffect *)ao->getAffect();
	        af->setSkillNum(_skillNum);
            af->setWearoffToChar("Your energy level fades.");
            af->setLevel(level);
            af->setDuration(((level >> 2) + 1) MUD_HOURS);
            af->setAccumDuration(true);

	        int moveMod = 10 + (level << 1);
            af->addApply(APPLY_MOVE, moveMod);

            PointsObject *po = new PointsObject(_owner, target);
            po->setSkill(_skillNum);
            po->setMove(moveMod);
            ev.push_back_id(po);
	    
            return true;
        }
};

class SpellNullpsi : public PsionicSpell
{
    public:
        SpellNullpsi() : PsionicSpell()
        {
            _skillNum = SPELL_NULLPSI;
            _skillName = "nullpsi";
            _minMana = 50;
            _maxMana = 90;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PSIONIC, 17, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellNullpsi();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            NullPsiObject *no = new NullPsiObject(_owner, target);
            no->setSkill(_skillNum);
            ev.push_back(no);

            return true;
        }
};

class SpellTelepathy : public PsionicSpell
{
    public:
        SpellTelepathy() : PsionicSpell()
        {
            _skillNum = SPELL_TELEPATHY;
            _skillName = "telepathy";
            _minMana = 62;
            _maxMana = 95;
            _manaChange = 4;
            _minPosition = POS_STANDING;
            _violent = false;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_PSIONIC, 41, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellTelepathy();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PsionicSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellDistraction : public PsionicSpell
{
    public:
        SpellDistraction() : PsionicSpell()
        {
            _skillNum = SPELL_DISTRACTION;
            _skillName = "distraction";
            _minMana = 10;
            _maxMana = 40;
            _manaChange = 4;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _violent = false;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PSIONIC, 2, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellDistraction();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            if (!target->isNPC()) {
                ExecutableObject *eo = new ExecutableObject(_owner);
                eo->addToCharMessage("This trigger cannot be used against other players.");

                ev.push_back_id(eo);
                return false;
            }

            if (!MEMORY(target) && !target->isHunting()) {
                ExecutableObject *eo = new ExecutableObject(_owner);
                eo->addToCharMessage("Nothing seems to happen.");
                eo->addToRoomMessage("Nothing seems to happen.");

                ev.push_back_id(eo);
                return false;
            }

            DistractionObject *dis = new DistractionObject(_owner, target);
            dis->addToCharMessage("$N suddenly looks very distracted.");
            dis->addToVictMessage("You feel like you're missing something...");
            dis->addToRoomMessage("$N suddenly looks very distracted.");

            ev.push_back_id(dis);

            return true;
        }
};

class SkillPsiblast : public PsionicSpell
{
    public:
        SkillPsiblast() : PsionicSpell()
        {
            _skillNum = SKILL_PSIBLAST;
            _skillName = "psiblast";
            _minMana = 50;
            _maxMana = 50;
            _manaChange = 1;
            _violent = true;
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_PSIONIC, 5, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillPsiblast();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PsionicSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillPsidrain : public PsionicSpell
{
    public:
        SkillPsidrain() : PsionicSpell()
        {
            _skillNum = SKILL_PSIDRAIN;
            _skillName = "psidrain";
            _minMana = 30;
            _maxMana = 50;
            _manaChange = 1;
            _minPosition = POS_DEAD;
            _targets = TAR_UNPLEASANT;
            PracticeInfo p0 = {CLASS_PSIONIC, 24, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillPsidrain();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PsionicSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellElectrostaticField : public PhysicsSpell
{
    public:
        SpellElectrostaticField() : PhysicsSpell()
        {
            _skillNum = SPELL_ELECTROSTATIC_FIELD;
            _skillName = "electrostatic field";
            _minMana = 20;
            _maxMana = 40;
            _manaChange = 2;
            _minPosition = POS_SITTING;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_PHYSIC, 1, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellElectrostaticField();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            string toVictMessage = "An electrostatic field crackles into "
                "being around you.";
            string toRoomMessage = "An electrostatic field crackles into "
                "begin around $N.";

            AffectObject *ao = new AffectObject("ElectrostaticFieldAffect",
                    _owner, target);

            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(SPELL_ELECTROSTATIC_FIELD);
            ca->setWearoffToChar("Your electrostatic field dissipates.");
            ca->setTarget(target);
            ca->setLevel(level);
            ca->setDuration((2 + ca->getLevel() >> 2) MUD_HOURS);

            ao->addToVictMessage(toVictMessage);
            ao->addToRoomMessage(toRoomMessage);
            
            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SpellEntropyField : public PhysicsSpell
{
    public:
        SpellEntropyField() : PhysicsSpell()
        {
            _skillNum = SPELL_ENTROPY_FIELD;
            _skillName = "entropy field";
            _minMana = 60;
            _maxMana = 101;
            _manaChange = 8;
            _minPosition = POS_SITTING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_PHYSIC, 27, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellEntropyField();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellAcidity : public PhysicsSpell
{
    public:
        SpellAcidity() : PhysicsSpell()
        {
            _skillNum = SPELL_ACIDITY;
            _skillName = "acidity";
            _minMana = 20;
            _maxMana = 40;
            _manaChange = 2;
            _minPosition = POS_SITTING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_PHYSIC, 2, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellAcidity();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev, target))
                return false;

	    int level = getPowerLevel();

            AffectObject *ao = 
                new AffectObject("AcidityAffect", _owner, target);

            // Set up the affect
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(SPELL_ACIDITY);
            ca->setLevel(level);
            ca->setDuration((MAX(1, level >> 5)) MUD_HOURS);

            // Set up the damage object
            DamageObject *do1 = new DamageObject(getOwner(), target);
	  
            int dam;

            dam = dice(3, level << 1);
	    
            do1->setDamageType(SPELL_ACIDITY);
            do1->setDamage(dam);

            string toChar = "$N's body begins to acidify as $E screams in agony.";
            string toVict = "$n causes your body to acidify, you scream in agony!";
            string toRoom = "$n causes $N's body to acidify, and $E screams in agony!";

            do1->addToCharMessage(toChar);
            do1->addToVictMessage(toVict);
            do1->addToRoomMessage(toRoom);
	    
            ao->setSkill(this->getSkillNumber());
            ev.push_back_id(ao);
            do1->setSkill(this->getSkillNumber());
            ev.push_back_id(do1);

            return true;
        }
};

class SpellAttractionField : public PhysicsSpell
{
    public:
        SpellAttractionField() : PhysicsSpell()
        {
            _skillNum = SPELL_ATTRACTION_FIELD;
            _skillName = "attraction field";
            _minMana = 40;
            _maxMana = 60;
            _manaChange = 1;
            _minPosition = POS_SITTING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP;
            PracticeInfo p0 = {CLASS_PHYSIC, 8, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellAttractionField();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
	        if (!PhysicsSpell::perform(ev, target))
                return false;
	    
            int level = getPowerLevel();
	    
            string toVictMessage = "You feel very attractive -- to weapons.";
            string toRoomMessage = "$n suddenly becomes attractive like a magnet!";

            AffectObject *ao = new AffectObject("CreatureAffect", 
                    _owner, target);
            CreatureAffect *af = (CreatureAffect *)ao->getAffect();

	        af->setSkillNum(this->_skillNum);
            af->setWearoffToChar("The attraction field around you dissipates.");
            af->setLevel(level);
            af->setDuration((1 + (level >> 2)) MUD_HOURS);

	        int acMod = 10 + level;

	        af->addApply(APPLY_AC, acMod);

	        af->addBitsToVector(2, AFF3_ATTRACTION_FIELD);

	        ao->addToVictMessage(toVictMessage);
	        ao->addToRoomMessage(toRoomMessage);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }

        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev, obj))
                return false;

            // FIXME:  Need object affects first!
            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellNuclearWasteland : public PhysicsSpell
{
    public:
        SpellNuclearWasteland() : PhysicsSpell()
        {
            _skillNum = SPELL_NUCLEAR_WASTELAND;
            _skillName = "nuclear wasteland";
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 10;
            _minPosition = POS_SITTING;
            _violent = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_PHYSIC, 40, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellNuclearWasteland();
        }

        bool perform(ExecutableVector &ev, char *argument) {
            if (!PhysicsSpell::perform(ev))
                return false;

            int level = getPowerLevel();

            string toCharMessage = "A blinding flash heralds the beginning of a local "
                "nuclear winter!";
            string toRoomMessage = toCharMessage;

            AffectObject *ao = new AffectObject("RoomAffect", _owner, _owner->in_room);
            RoomAffect *af = (RoomAffect *)ao->getAffect();

	        af->setSkillNum(this->_skillNum);
            af->setWearoffToRoom("The nuclear winter passes.");
            af->setDescription("   This area bears a string resemblance to a radioactive wasteland.");
            af->setLevel(level);
            af->setDuration((level >> 1) MUD_HOURS);
            af->setRoomFlags(ROOM_RADIOACTIVE);

            ao->addToCharMessage(toCharMessage);
            ao->addToRoomMessage(toRoomMessage);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SpellSpacetimeImprint : public PhysicsSpell
{
    public:
        SpellSpacetimeImprint() : PhysicsSpell()
        {
            _skillNum = SPELL_SPACETIME_IMPRINT;
            _skillName = "spacetime imprint";
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 10;
            _minPosition = POS_SITTING;
            _violent = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_PHYSIC, 30, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellSpacetimeImprint();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellSpacetimeRecall : public PhysicsSpell
{
    public:
        SpellSpacetimeRecall() : PhysicsSpell()
        {
            _skillNum = SPELL_SPACETIME_RECALL;
            _skillName = "spacetime recall";
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 10;
            _minPosition = POS_SITTING;
            _violent = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_PHYSIC, 30, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellSpacetimeRecall();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellFluoresce : public PhysicsSpell
{
    public:
        SpellFluoresce() : PhysicsSpell()
        {
            _skillNum = SPELL_FLUORESCE;
            _skillName = "fluorescence";
            _minMana = 10;
            _maxMana = 50;
            _manaChange = 1;
            _minPosition = POS_SITTING;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PHYSIC, 3, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellFluoresce();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
	    if (!PhysicsSpell::perform(ev, target))
                return false;
	    
            int level = getPowerLevel();
	    
            string toVictMessage = "The area around you is illuminated with fluorescent atoms.";
            string toRoomMessage = "The light of fluorescent atoms surrounds $N.";

            AffectObject *ao = new AffectObject("CreatureAffect", 
                    _owner, target);
            CreatureAffect *af = (CreatureAffect *)ao->getAffect();

	        af->setSkillNum(this->_skillNum);
            af->setWearoffToChar("The atoms around you cease to fluoresce.");
            af->setLevel(level);
            af->setDuration((8 + level >> 1) MUD_HOURS);

	        af->addBitsToVector(1, AFF2_FLUORESCENT);

            ao->addToVictMessage(toVictMessage);
            ao->addToRoomMessage(toRoomMessage);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);
            return true;
        }
};

class SpellGammaRay : public PhysicsSpell
{
    public:
        SpellGammaRay() : PhysicsSpell()
        {
            _skillNum = SPELL_GAMMA_RAY;
            _skillName = "gamma ray";
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 2;
            _minPosition = POS_SITTING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_PHYSIC, 25, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellGammaRay();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellGravityWell : public PhysicsSpell
{
    public:
        SpellGravityWell() : PhysicsSpell()
        {
            _skillNum = SPELL_GRAVITY_WELL;
            _skillName = "gravity well";
            _minMana = 70;
            _maxMana = 130;
            _manaChange = 4;
            _minPosition = POS_SITTING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_PHYSIC, 38, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellGravityWell();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellCapacitanceBoost : public PhysicsSpell
{
    public:
        SpellCapacitanceBoost() : PhysicsSpell()
        {
            _skillNum = SPELL_CAPACITANCE_BOOST;
            _skillName = "capacitance boost";
            _minMana = 45;
            _maxMana = 70;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PHYSIC, 6, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellCapacitanceBoost();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, target);
            ao->addToVictMessage("You feel your energy capacity rise.");
            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            CreatureAffect *af = (CreatureAffect *)ao->getAffect();
	        af->setSkillNum(_skillNum);
            af->setWearoffToChar("Your energy level fades.");
            af->setLevel(level);
            af->setDuration(((level >> 2) + 1) MUD_HOURS);
            af->setAccumDuration(true);

	        int moveMod = 10 + (level << 1);
            af->addApply(APPLY_MOVE, moveMod);

            PointsObject *po = new PointsObject(_owner, target);
            po->setSkill(_skillNum);
            po->setMove(moveMod);
            ev.push_back_id(po);
	    
            return true;
        }
};

class SpellElectricArc : public PhysicsSpell
{
    public:
        SpellElectricArc() : PhysicsSpell()
        {
            _skillNum = SPELL_ELECTRIC_ARC;
            _skillName = "electric arc";
            _minMana = 50;
            _maxMana = 90;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_PHYSIC, 21, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellElectricArc();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellTemporalCompression : public PhysicsSpell
{
    public:
        SpellTemporalCompression() : PhysicsSpell()
        {
            _skillNum = SPELL_TEMPORAL_COMPRESSION;
            _skillName = "temporal compression";
            _minMana = 50;
            _maxMana = 80;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PHYSIC, 29, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellTemporalCompression();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellTemporalDilation : public PhysicsSpell
{
    public:
        SpellTemporalDilation() : PhysicsSpell()
        {
            _skillNum = SPELL_TEMPORAL_DILATION;
            _skillName = "temporal dilation";
            _minMana = 60;
            _maxMana = 120;
            _manaChange = 6;
            _minPosition = POS_STANDING;
            _targets = TAR_UNPLEASANT | TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_PHYSIC, 36, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellTemporalDilation();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellHalflife : public PhysicsSpell
{
    public:
        SpellHalflife() : PhysicsSpell()
        {
            _skillNum = SPELL_HALFLIFE;
            _skillName = "halflife";
            _minMana = 70;
            _maxMana = 120;
            _manaChange = 5;
            _minPosition = POS_SITTING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_OBJ_EQUIP | TAR_OBJ_INV | TAR_OBJ_ROOM;
            PracticeInfo p0 = {CLASS_PHYSIC, 30, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellHalflife();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellMicrowave : public PhysicsSpell
{
    public:
        SpellMicrowave() : PhysicsSpell()
        {
            _skillNum = SPELL_MICROWAVE;
            _skillName = "microwave";
            _minMana = 20;
            _maxMana = 45;
            _manaChange = 2;
            _minPosition = POS_SITTING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_PHYSIC, 18, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellMicrowave();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellOxidize : public PhysicsSpell
{
    public:
        SpellOxidize() : PhysicsSpell()
        {
            _skillNum = SPELL_OXIDIZE;
            _skillName = "oxidize";
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_SITTING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_PHYSIC, 10, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellOxidize();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellRandomCoordinates : public PhysicsSpell
{
    public:
        SpellRandomCoordinates() : PhysicsSpell()
        {
            _skillNum = SPELL_RANDOM_COORDINATES;
            _skillName = "random coordinates";
            _minMana = 20;
            _maxMana = 100;
            _manaChange = 2;
            _minPosition = POS_SITTING;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PHYSIC, 20, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellRandomCoordinates();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            TeleportObject *to = new 
                TeleportObject(getOwner(), TeleportObject::RandomCoordinates);

            to->setVictim(target);

            to->setSkill(getSkillNumber());
            ev.push_back_id(to);
            return true;
        }
};

class SpellRepulsionField : public PhysicsSpell
{
    public:
        SpellRepulsionField() : PhysicsSpell()
        {
            _skillNum = SPELL_REPULSION_FIELD;
            _skillName = "repulsion field";
            _minMana = 20;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_SITTING;
            _violent = false;
            _targets = TAR_CHAR_ROOM | TAR_OBJ_ROOM | TAR_OBJ_EQUIP | TAR_OBJ_INV;
            PracticeInfo p0 = {CLASS_PHYSIC, 16, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellRepulsionField();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellVacuumShroud : public PhysicsSpell
{
    public:
        SpellVacuumShroud() : PhysicsSpell()
        {
            _skillNum = SPELL_VACUUM_SHROUD;
            _skillName = "vacuum shroud";
            _minMana = 40;
            _maxMana = 60;
            _manaChange = 1;
            _minPosition = POS_SITTING;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PHYSIC, 31, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellVacuumShroud();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellAlbedoShield : public PhysicsSpell
{
    public:
        SpellAlbedoShield() : PhysicsSpell()
        {
            _skillNum = SPELL_ALBEDO_SHIELD;
            _skillName = "albedo shield";
            _minMana = 50;
            _maxMana = 90;
            _manaChange = 1;
            _minPosition = POS_SITTING;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PHYSIC, 34, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellAlbedoShield();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellTransmittance : public PhysicsSpell
{
    public:
        SpellTransmittance() : PhysicsSpell()
        {
            _skillNum = SPELL_TRANSMITTANCE;
            _skillName = "transmittance";
            _minMana = 30;
            _maxMana = 60;
            _manaChange = 1;
            _minPosition = POS_SITTING;
            _targets = TAR_CHAR_ROOM | TAR_OBJ_ROOM | TAR_OBJ_EQUIP | TAR_OBJ_INV;
            PracticeInfo p0 = {CLASS_PHYSIC, 14, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellTransmittance();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellTimeWarp : public PhysicsSpell
{
    public:
        SpellTimeWarp() : PhysicsSpell()
        {
            _skillNum = SPELL_TIME_WARP;
            _skillName = "time warp";
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 8;
            _minPosition = POS_SITTING;
            _targets = TAR_SELF_ONLY | TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PHYSIC, 35, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellTimeWarp();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellRadioimmunity : public PhysicsSpell
{
    public:
        SpellRadioimmunity() : PhysicsSpell()
        {
            _skillNum = SPELL_RADIOIMMUNITY;
            _skillName = "radioimmunity";
            _minMana = 20;
            _maxMana = 80;
            _manaChange = 8;
            _minPosition = POS_SITTING;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PHYSIC, 20, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellRadioimmunity();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellDensify : public PhysicsSpell
{
    public:
        SpellDensify() : PhysicsSpell()
        {
            _skillNum = SPELL_DENSIFY;
            _skillName = "densify";
            _minMana = 20;
            _maxMana = 80;
            _manaChange = 8;
            _minPosition = POS_SITTING;
            _targets = TAR_CHAR_ROOM | TAR_OBJ_EQUIP | TAR_OBJ_ROOM | TAR_OBJ_INV;
            PracticeInfo p0 = {CLASS_PHYSIC, 26, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellDensify();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellLatticeHardening : public PhysicsSpell
{
    public:
        SpellLatticeHardening() : PhysicsSpell()
        {
            _skillNum = SPELL_LATTICE_HARDENING;
            _skillName = "lattice hardening";
            _minMana = 40;
            _maxMana = 100;
            _manaChange = 4;
            _minPosition = POS_SITTING;
            _targets = TAR_CHAR_ROOM | TAR_OBJ_ROOM | TAR_OBJ_INV;
            PracticeInfo p0 = {CLASS_PHYSIC, 28, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellLatticeHardening();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellChemicalStability : public PhysicsSpell
{
    public:
        SpellChemicalStability() : PhysicsSpell()
        {
            _skillNum = SPELL_CHEMICAL_STABILITY;
            _skillName = "chemical stability";
            _minMana = 20;
            _maxMana = 80;
            _manaChange = 8;
            _minPosition = POS_SITTING;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PHYSIC, 9, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellChemicalStability();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev, target))
                return false;

	    int level = this->getPowerLevel();

	    AffectObject *ao = new AffectObject("ChemicalStabilityAffect", this->getOwner(), target);
	    CreatureAffect *ca = (CreatureAffect *) ao->getAffect();

	    ao->addToVictMessage("You feel more chemically inert.");
	    ao->addToRoomMessage("$n begins looking more chemically inert.");
	    ao->addToCharMessage("$n begins looking more chemically inert.");
	    ao->setSkill(this->getSkillNumber());
	    
	    ca->setSkillNum(this->getSkillNumber());
	    ca->setWearoffToChar("You feel less chemically inert.");
	    ca->setTarget(target);
	    ca->setLevel(level);
	    ca->setDuration(level >> 2);

	    ev.push_back_id(ao);

	    return true;
	    
	}
};

class SpellRefraction : public PhysicsSpell
{
    public:
        SpellRefraction() : PhysicsSpell()
        {
            _skillNum = SPELL_REFRACTION;
            _skillName = "refraction";
            _minMana = 20;
            _maxMana = 80;
            _manaChange = 8;
            _minPosition = POS_SITTING;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PHYSIC, 33, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellRefraction();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellNullify : public PhysicsSpell
{
    public:
        SpellNullify() : PhysicsSpell()
        {
            _skillNum = SPELL_NULLIFY;
            _skillName = "nullify";
            _minMana = 55;
            _maxMana = 90;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PHYSIC, 11, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellNullify();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellAreaStasis : public PhysicsSpell
{
    public:
        SpellAreaStasis() : PhysicsSpell()
        {
            _skillNum = SPELL_AREA_STASIS;
            _skillName = "area stasis";
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 5;
            _minPosition = POS_SITTING;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_PHYSIC, 40, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellAreaStasis();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellEmpPulse : public PhysicsSpell
{
    public:
        SpellEmpPulse() : PhysicsSpell()
        {
            _skillNum = SPELL_EMP_PULSE;
            _skillName = "emp pulse";
            _minMana = 75;
            _maxMana = 145;
            _manaChange = 5;
            _minPosition = POS_SITTING;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_PHYSIC, 34, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellEmpPulse();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellFissionBlast : public PhysicsSpell
{
    public:
        SpellFissionBlast() : PhysicsSpell()
        {
            _skillNum = SPELL_FISSION_BLAST;
            _skillName = "fission blast";
            _minMana = 70;
            _maxMana = 150;
            _manaChange = 10;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_PHYSIC, 43, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellFissionBlast();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellDimensionalVoid : public PhysicsSpell
{
    public:
        SpellDimensionalVoid() : PhysicsSpell()
        {
            _skillNum = SPELL_DIMENSIONAL_VOID;
            _skillName = "dimensional void";
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_IGNORE;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellDimensionalVoid();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class ZenOfHealing : public GenericZen
{
    public:
        ZenOfHealing() : GenericZen()
        {
            _skillNum = ZEN_HEALING;
            _skillName = "zen of healing";
            _minMana = 9;
            _maxMana = 30;
            _manaChange = 2;
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MONK, 36, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new ZenOfHealing();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericZen::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericZen::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class ZenOfAwareness : public GenericZen
{
    public:
        ZenOfAwareness() : GenericZen()
        {
            _skillNum = ZEN_AWARENESS;
            _skillName = "zen of awareness";
            _minMana = 5;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MONK, 21, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new ZenOfAwareness();
        }

        bool perform(ExecutableVector &ev) {
            if (!GenericZen::perform(ev))
                return false;

            int level = getPowerLevel();
            string toVictMessage = "You become one with the zen of awareness.";

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, _owner);

            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(ZEN_AWARENESS);
            ca->setLevel(level);
            ca->setWearoffToChar("You feel your heightened awareness fade.");
            ca->setTarget(_owner);
            if (_owner->getLevel() < 36) {
                ca->setBitvector(0, AFF_DETECT_INVIS);
            }
            else {
                ca->setBitvector(1, AFF2_TRUE_SEEING);
            }
            ca->setDuration((ca->getLevel() / 3) MUD_HOURS);

            ao->addToVictMessage(toVictMessage);
            
            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;

        }
};

class ZenOfOblivity : public GenericZen
{
    public:
        ZenOfOblivity() : GenericZen()
        {
            _skillNum = ZEN_OBLIVITY;
            _skillName = "zen of oblivity";
            _minMana = 10;
            _maxMana = 40;
            _manaChange = 4;
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MONK, 43, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new ZenOfOblivity();
        }

        bool perform(ExecutableVector &ev) {
            if (!GenericZen::perform(ev))
                return false;

            int level = getPowerLevel();
            string toVictMessage = "You begin to perceive the zen of oblivity.";

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, _owner);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(ZEN_OBLIVITY);
            ca->setWearoffToChar("You are no longer oblivious to pain.");
            ca->setTarget(_owner);
            ca->setBitvector(1, AFF2_OBLIVITY);
            ca->setLevel(level);
            ca->setDuration((ca->getLevel() / 3) MUD_HOURS);

            ao->addToVictMessage(toVictMessage);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class ZenOfMotion : public GenericZen
{
    public:
        ZenOfMotion() : GenericZen()
        {
            _skillNum = ZEN_MOTION;
            _skillName = "zen of motion";
            _minMana = 7;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MONK, 28, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new ZenOfMotion();
        }

        bool perform(ExecutableVector &ev) {
            if (!GenericZen::perform(ev))
                return false;

            int level = getPowerLevel();
            string toVictMessage = "You have attained the zen of motion.";

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, _owner);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();;
            ca->setSkillNum(ZEN_MOTION);
            ca->setWearoffToChar("Your motions begin to feel less free.");
            ca->setTarget(_owner);
            ca->setLevel(level);
            if (_owner->getPrimaryClass() == CLASS_MONK 
                && ca->getLevel() >= 50) {
                ca->setBitvector(0, AFF_INFLIGHT);
            }
            ca->setDuration((ca->getLevel() / 3) MUD_HOURS);

            ao->addToVictMessage(toVictMessage);
            
            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class ZenOfDispassion : public GenericZen
{
    public:
        ZenOfDispassion() : GenericZen()
        {
            _skillNum = ZEN_DISPASSION;
            _skillName = "zen of dispassion";
            _minMana = 50;
            _maxMana = 30;
            _manaChange = 30;
            _minPosition = 5;
            PracticeInfo p0 = {CLASS_MONK, 45, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new ZenOfDispassion();
        }

        bool perform(ExecutableVector &ev) {
            if (!GenericZen::perform(ev))
                return false;

            int level = getPowerLevel();
            string toVictMessage = "You have grasped the zen of dispassion.";

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, _owner);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(ZEN_DISPASSION);
            ca->setWearoffToChar("You have lost the zen of dispassion.");
            ca->setTarget(_owner);
            ca->setLevel(level);
            ca->setDuration((ca->getLevel() / 4) MUD_HOURS);

            ao->addToVictMessage(toVictMessage);
            
            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SongDriftersDitty : public SongSpell
{
    public:
        SongDriftersDitty() : SongSpell()
        {
            _skillNum = SONG_DRIFTERS_DITTY;
            _skillName = "drifters ditty";
            _minMana = 75;
            _maxMana = 90;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_BARD, 4, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongDriftersDitty();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongAriaOfArmament : public SongSpell
{
    public:
        SongAriaOfArmament() : SongSpell()
        {
            _skillNum = SONG_ARIA_OF_ARMAMENT;
            _skillName = "aria of armament";
            _minMana = 50;
            _maxMana = 75;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_BARD, 3, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongAriaOfArmament();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongVerseOfVulnerability : public SongSpell
{
    public:
        SongVerseOfVulnerability() : SongSpell()
        {
            _skillNum = SONG_VERSE_OF_VULNERABILITY;
            _skillName = "verse of vulnerability";
            _minMana = 20;
            _maxMana = 50;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_BARD, 8, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongVerseOfVulnerability();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongMelodyOfMettle : public SongSpell
{
    public:
        SongMelodyOfMettle() : SongSpell()
        {
            _skillNum = SONG_MELODY_OF_METTLE;
            _skillName = "melody of mettle";
            _minMana = 75;
            _maxMana = 100;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_BARD, 11, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongMelodyOfMettle();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongRegalersRhapsody : public SongSpell
{
    public:
        SongRegalersRhapsody() : SongSpell()
        {
            _skillNum = SONG_REGALERS_RHAPSODY;
            _skillName = "regalers rhapsody";
            _minMana = 50;
            _maxMana = 65;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_BARD, 5, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongRegalersRhapsody();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongDefenseDitty : public SongSpell
{
    public:
        SongDefenseDitty() : SongSpell()
        {
            _skillNum = SONG_DEFENSE_DITTY;
            _skillName = "defense ditty";
            _minMana = 95;
            _maxMana = 120;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_BARD, 39, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongDefenseDitty();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongAlronsAria : public SongSpell
{
    public:
        SongAlronsAria() : SongSpell()
        {
            _skillNum = SONG_ALRONS_ARIA;
            _skillName = "alrons aria";
            _minMana = 95;
            _maxMana = 120;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_BARD, 24, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongAlronsAria();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongLustrationMelisma : public SongSpell
{
    public:
        SongLustrationMelisma() : SongSpell()
        {
            _skillNum = SONG_LUSTRATION_MELISMA;
            _skillName = "lustration melisma";
            _minMana = 75;
            _maxMana = 95;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_BARD, 2, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongLustrationMelisma();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongExposureOverture : public SongSpell
{
    public:
        SongExposureOverture() : SongSpell()
        {
            _skillNum = SONG_EXPOSURE_OVERTURE;
            _skillName = "exposure overture";
            _minMana = 75;
            _maxMana = 95;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_BARD, 25, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongExposureOverture();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongVerseOfValor : public SongSpell
{
    public:
        SongVerseOfValor() : SongSpell()
        {
            _skillNum = SONG_VERSE_OF_VALOR;
            _skillName = "verse of valor";
            _minMana = 75;
            _maxMana = 95;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_BARD, 10, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongVerseOfValor();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongWhiteNoise : public SongSpell
{
    public:
        SongWhiteNoise() : SongSpell()
        {
            _skillNum = SONG_WHITE_NOISE;
            _skillName = "white noise";
            _minMana = 40;
            _maxMana = 80;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_BARD, 35, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongWhiteNoise();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongHomeSweetHome : public SongSpell
{
    public:
        SongHomeSweetHome() : SongSpell()
        {
            _skillNum = SONG_HOME_SWEET_HOME;
            _skillName = "home sweet home";
            _minMana = 20;
            _maxMana = 100;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_BARD, 9, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongHomeSweetHome();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongChantOfLight : public SongSpell
{
    public:
        SongChantOfLight() : SongSpell()
        {
            _skillNum = SONG_CHANT_OF_LIGHT;
            _skillName = "chant of light";
            _minMana = 20;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_BARD, 1, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongChantOfLight();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongInsidiousRhythm : public SongSpell
{
    public:
        SongInsidiousRhythm() : SongSpell()
        {
            _skillNum = SONG_INSIDIOUS_RHYTHM;
            _skillName = "insidious rhythm";
            _minMana = 55;
            _maxMana = 90;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_BARD, 20, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongInsidiousRhythm();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongEaglesOverture : public SongSpell
{
    public:
        SongEaglesOverture() : SongSpell()
        {
            _skillNum = SONG_EAGLES_OVERTURE;
            _skillName = "eagles overture";
            _minMana = 25;
            _maxMana = 55;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_BARD, 25, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongEaglesOverture();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }

        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongWeightOfTheWorld : public SongSpell
{
    public:
        SongWeightOfTheWorld() : SongSpell()
        {
            _skillNum = SONG_WEIGHT_OF_THE_WORLD;
            _skillName = "weight of the world";
            _minMana = 150;
            _maxMana = 180;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_BARD, 21, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongWeightOfTheWorld();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongGuihariasGlory : public SongSpell
{
    public:
        SongGuihariasGlory() : SongSpell()
        {
            _skillNum = SONG_GUIHARIAS_GLORY;
            _skillName = "guiharias glory";
            _minMana = 80;
            _maxMana = 120;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_BARD, 16, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongGuihariasGlory();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongRhapsodyOfRemedy : public SongSpell
{
    public:
        SongRhapsodyOfRemedy() : SongSpell()
        {
            _skillNum = SONG_RHAPSODY_OF_REMEDY;
            _skillName = "rhapsody of remedy";
            _minMana = 50;
            _maxMana = 80;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_BARD, 28, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongRhapsodyOfRemedy();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongUnladenSwallowSong : public SongSpell
{
    public:
        SongUnladenSwallowSong() : SongSpell()
        {
            _skillNum = SONG_UNLADEN_SWALLOW_SONG;
            _skillName = "unladen swallow song";
            _minMana = 85;
            _maxMana = 105;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_BARD, 19, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongUnladenSwallowSong();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongClarifyingHarmonies : public SongSpell
{
    public:
        SongClarifyingHarmonies() : SongSpell()
        {
            _skillNum = SONG_CLARIFYING_HARMONIES;
            _skillName = "clarifying harmonies";
            _minMana = 20;
            _maxMana = 45;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_OBJ_EQUIP;
            PracticeInfo p0 = {CLASS_BARD, 12, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongClarifyingHarmonies();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongPowerOverture : public SongSpell
{
    public:
        SongPowerOverture() : SongSpell()
        {
            _skillNum = SONG_POWER_OVERTURE;
            _skillName = "power overture";
            _minMana = 25;
            _maxMana = 50;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_BARD, 13, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongPowerOverture();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();
            string toVictMessage = "Your strength seems to grow as the "
                "song swells.";

            // Strength affect
            AffectObject *ao = new AffectObject("CreatureAffect", _owner, _owner);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(SONG_POWER_OVERTURE);
            ca->setWearoffToChar("Your strength fails with your memory "
                    "of the Power Overture.");
            ca->setLevel(level);
            ca->setTarget(target);
            ca->addApply(APPLY_STR, 1 + ca->getLevel() / 30);
            ca->addApply(APPLY_HITROLL, 1 + ca->getLevel() / 30);
            ca->setDuration((1 + ca->getLevel() >> 2) MUD_HOURS);

            ao->addToVictMessage(toVictMessage);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }

        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongRhythmOfRage : public SongSpell
{
    public:
        SongRhythmOfRage() : SongSpell()
        {
            _skillNum = SONG_RHYTHM_OF_RAGE;
            _skillName = "rhythm of rage";
            _minMana = 180;
            _maxMana = 325;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_BARD, 36, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongRhythmOfRage();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongUnravellingDiapason : public SongSpell
{
    public:
        SongUnravellingDiapason() : SongSpell()
        {
            _skillNum = SONG_UNRAVELLING_DIAPASON;
            _skillName = "unravelling diapason";
            _minMana = 75;
            _maxMana = 120;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM;
            PracticeInfo p0 = {CLASS_BARD, 40, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongUnravellingDiapason();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongInstantAudience : public SongSpell
{
    public:
        SongInstantAudience() : SongSpell()
        {
            _skillNum = SONG_INSTANT_AUDIENCE;
            _skillName = "instant audience";
            _minMana = 100;
            _maxMana = 190;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_BARD, 26, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongInstantAudience();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongWoundingWhispers : public SongSpell
{
    public:
        SongWoundingWhispers() : SongSpell()
        {
            _skillNum = SONG_WOUNDING_WHISPERS;
            _skillName = "wounding whispers";
            _minMana = 60;
            _maxMana = 120;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_BARD, 36, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongWoundingWhispers();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongRhythmOfAlarm : public SongSpell
{
    public:
        SongRhythmOfAlarm() : SongSpell()
        {
            _skillNum = SONG_RHYTHM_OF_ALARM;
            _skillName = "rhythm of alarm";
            _minMana = 75;
            _maxMana = 100;
            _manaChange = 1;
            _minPosition = POS_SITTING;
            _violent = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_BARD, 41, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongRhythmOfAlarm();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillTumbling : public GenericSkill
{
    public:
        SkillTumbling() : GenericSkill()
        {
            _skillNum = SKILL_TUMBLING;
            _skillName = "tumbling";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_BARD, 30, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillTumbling();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillScream : public SongSpell
{
    public:
        SkillScream() : SongSpell()
        {
            _skillNum = SKILL_SCREAM;
            _skillName = "scream";
            _minMana = 35;
            _maxMana = 35;
            _manaChange = 1;
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_BARD, 5, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillScream();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongSonicDisruption : public SongSpell
{
    public:
        SongSonicDisruption() : SongSpell()
        {
            _skillNum = SONG_SONIC_DISRUPTION;
            _skillName = "sonic disruption";
            _minMana = 60;
            _maxMana = 100;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_BARD, 17, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongSonicDisruption();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongDirge : public SongSpell
{
    public:
        SongDirge() : SongSpell()
        {
            _skillNum = SONG_DIRGE;
            _skillName = "dirge";
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_BARD, 6, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongDirge();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongMisdirectionMelisma : public SongSpell
{
    public:
        SongMisdirectionMelisma() : SongSpell()
        {
            _skillNum = SONG_MISDIRECTION_MELISMA;
            _skillName = "misdirection melisma";
            _minMana = 50;
            _maxMana = 75;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_BARD, 34, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongMisdirectionMelisma();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongHymnOfPeace : public SongSpell
{
    public:
        SongHymnOfPeace() : SongSpell()
        {
            _skillNum = SONG_HYMN_OF_PEACE;
            _skillName = "hymn of peace";
            _minMana = 50;
            _maxMana = 75;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_BARD, 33, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongHymnOfPeace();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillVentriloquism : public GenericSkill
{
    public:
        SkillVentriloquism() : GenericSkill()
        {
            _skillNum = SKILL_VENTRILOQUISM;
            _skillName = "ventriloquism";
            _minPosition = POS_DEAD;
            _violent = false;
            PracticeInfo p0 = {CLASS_BARD, 25, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillVentriloquism();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellTidalSpacewarp : public PhysicsSpell
{
    public:
        SpellTidalSpacewarp() : PhysicsSpell()
        {
            _skillNum = SPELL_TIDAL_SPACEWARP;
            _skillName = "tidal spacewarp";
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_PHYSIC, 15, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellTidalSpacewarp();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillCleave : public GenericSkill
{
    public:
        SkillCleave() : GenericSkill()
        {
            _skillNum = SKILL_CLEAVE;
            _skillName = "cleave";
            _minPosition = POS_DEAD;
            PracticeInfo p1 = {CLASS_BLACKGUARD, 30, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillCleave();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillHamstring : public GenericSkill
{
    public:
        SkillHamstring() : GenericSkill()
        {
            _skillNum = SKILL_HAMSTRING;
            _skillName = "hamstring";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MERCENARY, 32, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillHamstring();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillDrag : public GenericSkill
{
    public:
        SkillDrag() : GenericSkill()
        {
            _skillNum = SKILL_DRAG;
            _skillName = "drag";
            _minPosition = POS_DEAD;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillDrag();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillSnatch : public GenericSkill
{
    public:
        SkillSnatch() : GenericSkill()
        {
            _skillNum = SKILL_SNATCH;
            _skillName = "snatch";
            _minPosition = POS_DEAD;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillSnatch();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillArchery : public GenericSkill
{
    public:
        SkillArchery() : GenericSkill()
        {
            _skillNum = SKILL_ARCHERY;
            _skillName = "archery";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 14, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p2 = {CLASS_RANGER, 9, 0};
            _practiceInfo.push_back(p2);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillArchery();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillBowFletch : public GenericSkill
{
    public:
        SkillBowFletch() : GenericSkill()
        {
            _skillNum = SKILL_BOW_FLETCH;
            _skillName = "bow fletch";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_RANGER, 17, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillBowFletch();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillReadScrolls : public GenericSkill
{
    public:
        SkillReadScrolls() : GenericSkill()
        {
            _skillNum = SKILL_READ_SCROLLS;
            _skillName = "read scrolls";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MAGE, 1, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 2, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_THIEF, 3, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_PSIONIC, 35, 0};
            _practiceInfo.push_back(p3);
            PracticeInfo p4 = {CLASS_PALADIN, 21, 0};
            _practiceInfo.push_back(p4);
            PracticeInfo p5 = {CLASS_RANGER, 21, 0};
            _practiceInfo.push_back(p5);
            PracticeInfo p6 = {CLASS_BARD, 5, 0};
            _practiceInfo.push_back(p6);
            PracticeInfo p7 = {CLASS_MONK, 7, 0};
            _practiceInfo.push_back(p7);
            PracticeInfo p8 = {CLASS_BLACKGUARD, 21, 0};
            _practiceInfo.push_back(p8);

            _learnedPercent = 10;
            _minPercentGain = 10;
            _maxPercentGain = 10;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillReadScrolls();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            return false;
        }

        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            return false;
        }

        bool perform(ExecutableVector &ev, CreatureList &list) {
            return false;
        }
};

class SkillUseWands : public GenericSkill
{
    public:
        SkillUseWands() : GenericSkill()
        {
            _skillNum = SKILL_USE_WANDS;
            _skillName = "use wands";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MAGE, 4, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 9, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_THIEF, 10, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_PSIONIC, 20, 0};
            _practiceInfo.push_back(p3);
            PracticeInfo p4 = {CLASS_PALADIN, 21, 0};
            _practiceInfo.push_back(p4);
            PracticeInfo p5 = {CLASS_RANGER, 29, 0};
            _practiceInfo.push_back(p5);
            PracticeInfo p6 = {CLASS_BARD, 5, 0};
            _practiceInfo.push_back(p6);
            PracticeInfo p7 = {CLASS_MONK, 35, 0};
            _practiceInfo.push_back(p7);
            PracticeInfo p8 = {CLASS_BLACKGUARD, 21, 0};
            _practiceInfo.push_back(p8);

            _learnedPercent = 10;
            _minPercentGain = 10;
            _maxPercentGain = 10;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillUseWands();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            return false;
        }

        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            return false;
        }

        bool perform(ExecutableVector &ev, CreatureList &list) {
            return false;
        }
};

class SkillBackstab : public GenericSkill
{
    public:
        SkillBackstab() : GenericSkill()
        {
            _skillNum = SKILL_BACKSTAB;
            _skillName = "backstab";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 5, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillBackstab();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillCircle : public GenericSkill
{
    public:
        SkillCircle() : GenericSkill()
        {
            _skillNum = SKILL_CIRCLE;
            _skillName = "circle";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 33, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillCircle();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillHide : public GenericSkill
{
    public:
        SkillHide() : GenericSkill()
        {
            _skillNum = SKILL_HIDE;
            _skillName = "hide";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 3, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_RANGER, 15, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_MERCENARY, 30, 0};
            _practiceInfo.push_back(p2);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillHide();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillKick : public GenericSkill
{
    public:
        SkillKick() : GenericSkill()
        {
            _skillNum = SKILL_KICK;
            _skillName = "kick";
            _minMove = 2;
            _maxMove = 5;
            _moveChange = 1;
            _minPosition = POS_DEAD;
            PracticeInfo p1 = {CLASS_CYBORG, 3, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 3, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_MERCENARY, 2, 0};
            _practiceInfo.push_back(p3);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillKick();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev, target))
                return false;

            // Set up the damage object
            DamageObject *do1 = new DamageObject(getOwner(), target);
            int level = getPowerLevel();
            int dam = dice(1, level * 2);

            do1->setDamageType(SKILL_KICK);
            do1->setDamage(dam);

            string toChar = "Your boots need polishing again -- blood all over them....";
            string toVict = "$n wipes $s boots in your face!";
            string toRoom = "$n wipes $s boots in the face of $N!";

            do1->addToCharMessage(toChar);
            do1->addToVictMessage(toVict);
            do1->addToRoomMessage(toRoom);

            do1->setSkill(getSkillNumber());
            ev.push_back_id(do1);

            return true;
        }

        bool perform(ExecutableVector &ev, obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            ExecutableObject *eo = new ExecutableObject(getOwner());
            string toCharMessage;
            string toRoomMessage;

            toCharMessage = "You fiercely kick " + string(obj->getName()) +
                "!";
            toRoomMessage = string(_owner->getName()) + " fiercely kicks " + 
                string(obj->getName()) + "!";

            eo->addToCharMessage(toCharMessage);
            eo->addToRoomMessage(toRoomMessage);

            eo->setSkill(getSkillNumber());
            ev.push_back_id(eo);

            return true;
        }
};

class SkillBash : public GenericSkill
{
    public:
        SkillBash() : GenericSkill()
        {
            _skillNum = SKILL_BASH;
            _skillName = "bash";
            _minPosition = POS_DEAD;
            PracticeInfo p1 = {CLASS_PALADIN, 9, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 14, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_MERCENARY, 4, 0};
            _practiceInfo.push_back(p3);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillBash();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillBreakDoor : public GenericSkill
{
    public:
        SkillBreakDoor() : GenericSkill()
        {
            _skillNum = SKILL_BREAK_DOOR;
            _skillName = "break door";
            _minPosition = POS_DEAD;
            PracticeInfo p1 = {CLASS_MERCENARY, 1, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillBreakDoor();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillHeadbutt : public GenericSkill
{
    public:
        SkillHeadbutt() : GenericSkill()
        {
            _skillNum = SKILL_HEADBUTT;
            _skillName = "headbutt";
            _minPosition = POS_DEAD;
            PracticeInfo p1 = {CLASS_RANGER, 16, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillHeadbutt();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillHotwire : public GenericSkill
{
    public:
        SkillHotwire() : GenericSkill()
        {
            _skillNum = SKILL_HOTWIRE;
            _skillName = "hotwire";
            _minPosition = POS_DEAD;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillHotwire();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillGouge : public GenericSkill
{
    public:
        SkillGouge() : GenericSkill()
        {
            _skillNum = SKILL_GOUGE;
            _skillName = "gouge";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 30, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_MONK, 36, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillGouge();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillStun : public GenericSkill
{
    public:
        SkillStun() : GenericSkill()
        {
            _skillNum = SKILL_STUN;
            _skillName = "stun";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 35, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillStun();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillFeign : public GenericSkill
{
    public:
        SkillFeign() : GenericSkill()
        {
            _skillNum = SKILL_FEIGN;
            _skillName = "feign";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 25, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillFeign();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillConceal : public GenericSkill
{
    public:
        SkillConceal() : GenericSkill()
        {
            _skillNum = SKILL_CONCEAL;
            _skillName = "conceal";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 15, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_MERCENARY, 30, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillConceal();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillPlant : public GenericSkill
{
    public:
        SkillPlant() : GenericSkill()
        {
            _skillNum = SKILL_PLANT;
            _skillName = "plant";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 20, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_MERCENARY, 35, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillPlant();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillPickLock : public GenericSkill
{
    public:
        SkillPickLock() : GenericSkill()
        {
            _skillNum = SKILL_PICK_LOCK;
            _skillName = "pick lock";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 1, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillPickLock();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillRescue : public GenericSkill
{
    public:
        SkillRescue() : GenericSkill()
        {
            _skillNum = SKILL_RESCUE;
            _skillName = "rescue";
            _minPosition = POS_DEAD;
            PracticeInfo p1 = {CLASS_PALADIN, 8, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 12, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_MONK, 16, 0};
            _practiceInfo.push_back(p3);
            PracticeInfo p4 = {CLASS_MERCENARY, 26, 0};
            _practiceInfo.push_back(p4);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillRescue();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillSneak : public GenericSkill
{
    public:
        SkillSneak() : GenericSkill()
        {
            _skillNum = SKILL_SNEAK;
            _skillName = "sneak";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 3, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_RANGER, 8, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_BARD, 10, 0};
            _practiceInfo.push_back(p2);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillSneak();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillSteal : public GenericSkill
{
    public:
        SkillSteal() : GenericSkill()
        {
            _skillNum = SKILL_STEAL;
            _skillName = "steal";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 2, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_BARD, 15, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillSteal();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillTrack : public GenericSkill
{
    public:
        SkillTrack() : GenericSkill()
        {
            _skillNum = SKILL_TRACK;
            _skillName = "track";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 8, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p2 = {CLASS_CYBORG, 11, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_RANGER, 11, 0};
            _practiceInfo.push_back(p3);
            PracticeInfo p4 = {CLASS_MERCENARY, 14, 0};
            _practiceInfo.push_back(p4);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillTrack();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillPunch : public GenericSkill
{
    public:
        SkillPunch() : GenericSkill()
        {
            _skillNum = SKILL_PUNCH;
            _skillName = "punch";
            _minPosition = POS_DEAD;
            _flags = MAG_TOUCH;
            PracticeInfo p0 = {CLASS_NONE, 1, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillPunch();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev, target))
                return false;

            // Set up the damage object
            DamageObject *do1 = new DamageObject(getOwner(), target);
            int level = getPowerLevel();
            int dam = dice(1, level);

            do1->setDamageType(SKILL_PUNCH);
            do1->setDamage(dam);

            string toChar = "Your punch slams into $N's solar plexus.";
            string toVict = "$n's punch slams into your solar plexus.";
            string toRoom = "$n's punch slams into $N's solar plexus.";

            do1->addToCharMessage(toChar);
            do1->addToVictMessage(toVict);
            do1->addToRoomMessage(toRoom);

            do1->setSkill(getSkillNumber());
            ev.push_back_id(do1);

            return true;
        }

        bool perform(ExecutableVector &ev, obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            ExecutableObject *eo = new ExecutableObject(getOwner());
            string toCharMessage;
            string toRoomMessage;

            
            toCharMessage = "You fiercely punch " + string(obj->getName()) +
                "!";
            toRoomMessage = string(_owner->getName()) + " fiercely punches " + 
                string(obj->getName()) + "!";

            eo->addToCharMessage(toCharMessage);
            eo->addToRoomMessage(toRoomMessage);

            eo->setSkill(getSkillNumber());
            ev.push_back_id(eo);

            return true;
        }
};

class SkillPiledrive : public GenericSkill
{
    public:
        SkillPiledrive() : GenericSkill()
        {
            _skillNum = SKILL_PILEDRIVE;
            _skillName = "piledrive";
            _minPosition = POS_DEAD;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillPiledrive();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillShieldMastery : public GenericSkill
{
    public:
        SkillShieldMastery() : GenericSkill()
        {
            _skillNum = SKILL_SHIELD_MASTERY;
            _skillName = "shield mastery";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_PALADIN, 22, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_BLACKGUARD, 22, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillShieldMastery();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillElbow : public GenericSkill
{
    public:
        SkillElbow() : GenericSkill()
        {
            _skillNum = SKILL_ELBOW;
            _skillName = "elbow";
            _minPosition = POS_DEAD;
            PracticeInfo p1 = {CLASS_CYBORG, 9, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillElbow();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillKnee : public GenericSkill
{
    public:
        SkillKnee() : GenericSkill()
        {
            _skillNum = SKILL_KNEE;
            _skillName = "knee";
            _minPosition = POS_DEAD;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillKnee();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillAutopsy : public GenericSkill
{
    public:
        SkillAutopsy() : GenericSkill()
        {
            _skillNum = SKILL_AUTOPSY;
            _skillName = "autopsy";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 40, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_RANGER, 36, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_MERCENARY, 10, 0};
            _practiceInfo.push_back(p2);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillAutopsy();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillBerserk : public GenericSkill
{
    public:
        SkillBerserk() : GenericSkill()
        {
            _skillNum = SKILL_BERSERK;
            _skillName = "berserk";
            _minPosition = POS_DEAD;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillBerserk();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillBattleCry : public GenericSkill
{
    public:
        SkillBattleCry() : GenericSkill()
        {
            _skillNum = SKILL_BATTLE_CRY;
            _skillName = "battle cry";
            _minPosition = POS_DEAD;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillBattleCry();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillKia : public GenericSkill
{
    public:
        SkillKia() : GenericSkill()
        {
            _skillNum = SKILL_KIA;
            _skillName = "kia";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MONK, 23, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillKia();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillCryFromBeyond : public GenericSkill
{
    public:
        SkillCryFromBeyond() : GenericSkill()
        {
            _skillNum = SKILL_CRY_FROM_BEYOND;
            _skillName = "cry from beyond";
            _minPosition = POS_DEAD;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillCryFromBeyond();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillStomp : public GenericSkill
{
    public:
        SkillStomp() : GenericSkill()
        {
            _skillNum = SKILL_STOMP;
            _skillName = "stomp";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 5, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillStomp();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillBodyslam : public GenericSkill
{
    public:
        SkillBodyslam() : GenericSkill()
        {
            _skillNum = SKILL_BODYSLAM;
            _skillName = "bodyslam";
            _minPosition = POS_DEAD;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillBodyslam();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillChoke : public GenericSkill
{
    public:
        SkillChoke() : GenericSkill()
        {
            _skillNum = SKILL_CHOKE;
            _skillName = "choke";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 23, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillChoke();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillClothesline : public GenericSkill
{
    public:
        SkillClothesline() : GenericSkill()
        {
            _skillNum = SKILL_CLOTHESLINE;
            _skillName = "clothesline";
            _minPosition = POS_DEAD;
            PracticeInfo p1 = {CLASS_MERCENARY, 12, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillClothesline();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillIntimidate : public GenericSkill
{
    public:
        SkillIntimidate() : GenericSkill()
        {
            _skillNum = SKILL_INTIMIDATE;
            _skillName = "intimidate";
            _minPosition = POS_DEAD;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillIntimidate();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillSpeedHealing : public GenericSkill
{
    public:
        SkillSpeedHealing() : GenericSkill()
        {
            _skillNum = SKILL_SPEED_HEALING;
            _skillName = "speed healing";
            _minPosition = POS_DEAD;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillSpeedHealing();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillStalk : public GenericSkill
{
    public:
        SkillStalk() : GenericSkill()
        {
            _skillNum = SKILL_STALK;
            _skillName = "stalk";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 40, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillStalk();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillHearing : public GenericSkill
{
    public:
        SkillHearing() : GenericSkill()
        {
            _skillNum = SKILL_HEARING;
            _skillName = "hearing";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 11, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillHearing();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillBearhug : public GenericSkill
{
    public:
        SkillBearhug() : GenericSkill()
        {
            _skillNum = SKILL_BEARHUG;
            _skillName = "bearhug";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_RANGER, 25, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillBearhug();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillBite : public GenericSkill
{
    public:
        SkillBite() : GenericSkill()
        {
            _skillNum = SKILL_BITE;
            _skillName = "bite";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_RANGER, 5, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillBite();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillDblAttack : public GenericSkill
{
    public:
        SkillDblAttack() : GenericSkill()
        {
            _skillNum = SKILL_DBL_ATTACK;
            _skillName = "dbl attack";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 45, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p2 = {CLASS_PALADIN, 25, 0};
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

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillDblAttack();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillNightVision : public GenericSkill
{
    public:
        SkillNightVision() : GenericSkill()
        {
            _skillNum = SKILL_NIGHT_VISION;
            _skillName = "night vision";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_RANGER, 2, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillNightVision();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillTurn : public GenericSkill
{
    public:
        SkillTurn() : GenericSkill()
        {
            _skillNum = SKILL_TURN;
            _skillName = "turn";
            _minMana = 30;
            _maxMana = 90;
            _manaChange = 5;
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CLERIC, 1, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_PALADIN, 3, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillTurn();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillAnalyze : public GenericSkill
{
    public:
        SkillAnalyze() : GenericSkill()
        {
            _skillNum = SKILL_ANALYZE;
            _skillName = "analyze";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_PHYSIC, 12, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CYBORG, 20, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillAnalyze();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillEvaluate : public GenericSkill
{
    public:
        SkillEvaluate() : GenericSkill()
        {
            _skillNum = SKILL_EVALUATE;
            _skillName = "evaluate";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_PHYSIC, 10, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CYBORG, 20, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillEvaluate();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillCureDisease : public GenericSkill
{
    public:
        SkillCureDisease() : GenericSkill()
        {
            _skillNum = SKILL_CURE_DISEASE;
            _skillName = "cure disease";
            _minMana = 10;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_PALADIN, 43, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillCureDisease();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillHolyTouch : public GenericSkill
{
    public:
        SkillHolyTouch() : GenericSkill()
        {
            _skillNum = SKILL_HOLY_TOUCH;
            _skillName = "holy touch";
            _minMana = 10;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_PALADIN, 2, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillHolyTouch();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillBandage : public GenericSkill
{
    public:
        SkillBandage() : GenericSkill()
        {
            _skillNum = SKILL_BANDAGE;
            _skillName = "bandage";
            _minMana = 30;
            _maxMana = 40;
            _manaChange = 1;
            _minPosition = POS_DEAD;
            PracticeInfo p1 = {CLASS_RANGER, 10, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_MONK, 17, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_MERCENARY, 3, 0};
            _practiceInfo.push_back(p3);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillBandage();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillFirstaid : public GenericSkill
{
    public:
        SkillFirstaid() : GenericSkill()
        {
            _skillNum = SKILL_FIRSTAID;
            _skillName = "firstaid";
            _minMana = 60;
            _maxMana = 70;
            _manaChange = 1;
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_RANGER, 19, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_MONK, 30, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_MERCENARY, 16, 0};
            _practiceInfo.push_back(p2);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillFirstaid();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillMedic : public GenericSkill
{
    public:
        SkillMedic() : GenericSkill()
        {
            _skillNum = SKILL_MEDIC;
            _skillName = "medic";
            _minMana = 80;
            _maxMana = 90;
            _manaChange = 1;
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_RANGER, 40, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillMedic();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillLeatherworking : public GenericSkill
{
    public:
        SkillLeatherworking() : GenericSkill()
        {
            _skillNum = SKILL_LEATHERWORKING;
            _skillName = "leatherworking";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 32, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p2 = {CLASS_PALADIN, 29, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_RANGER, 28, 0};
            _practiceInfo.push_back(p3);
            PracticeInfo p4 = {CLASS_MONK, 32, 0};
            _practiceInfo.push_back(p4);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillLeatherworking();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillMetalworking : public GenericSkill
{
    public:
        SkillMetalworking() : GenericSkill()
        {
            _skillNum = SKILL_METALWORKING;
            _skillName = "metalworking";
            _minPosition = POS_DEAD;
            PracticeInfo p1 = {CLASS_PALADIN, 32, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 36, 0};
            _practiceInfo.push_back(p2);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillMetalworking();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillConsider : public GenericSkill
{
    public:
        SkillConsider() : GenericSkill()
        {
            _skillNum = SKILL_CONSIDER;
            _skillName = "consider";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 11, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p2 = {CLASS_PALADIN, 13, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_RANGER, 18, 0};
            _practiceInfo.push_back(p3);
            PracticeInfo p4 = {CLASS_MONK, 13, 0};
            _practiceInfo.push_back(p4);
            PracticeInfo p5 = {CLASS_MERCENARY, 13, 0};
            _practiceInfo.push_back(p5);
            PracticeInfo p6 = {CLASS_BLACKGUARD, 13, 0};
            _practiceInfo.push_back(p6);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillConsider();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillGlance : public GenericSkill
{
    public:
        SkillGlance() : GenericSkill()
        {
            _skillNum = SKILL_GLANCE;
            _skillName = "glance";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 4, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillGlance();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillShoot : public GenericSkill
{
    public:
        SkillShoot() : GenericSkill()
        {
            _skillNum = SKILL_SHOOT;
            _skillName = "shoot";
            _minPosition = POS_DEAD;
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

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillShoot();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillBehead : public GenericSkill
{
    public:
        SkillBehead() : GenericSkill()
        {
            _skillNum = SKILL_BEHEAD;
            _skillName = "behead";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_PALADIN, 40, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillBehead();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillEmpower : public GenericSkill
{
    public:
        SkillEmpower() : GenericSkill()
        {
            _skillNum = SKILL_EMPOWER;
            _skillName = "empower";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MAGE, 5, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillEmpower();
        }

        bool perform(ExecutableVector &ev) {
            if (!GenericSkill::perform(ev))
                return false;

            int level = getPowerLevel();

            string toVictMessage = "You redirect your energies.";
            string toRoomMessage = "$n concentrates deeply.";

            AffectObject *ao = new AffectObject("CreatureAffect", 
                    _owner, _owner);
            CreatureAffect *af = (CreatureAffect *)ao->getAffect();
            af->setSkillNum(SKILL_EMPOWER);
            af->setWearoffToChar("Your energy returns to its normal state.");
            af->setLevel(level);
            af->setDuration(((level >> 2) + 5) MUD_HOURS);

            int hitMod = MIN(_owner->getLevel(), (_owner->getHit() >> 2));
            int moveMod = MIN(_owner->getLevel(), (_owner->getMove() >> 2));

            af->addApply(APPLY_MANA, hitMod + moveMod - 10);
            af->addApply(APPLY_HIT, -hitMod);
            af->addApply(APPLY_MOVE, -moveMod);

            af->setAccumAffect(true);

            ao->addToVictMessage(toVictMessage);
            ao->addToRoomMessage(toRoomMessage);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);
            return true;
        }
};

class SkillDisarm : public GenericSkill
{
    public:
        SkillDisarm() : GenericSkill()
        {
            _skillNum = SKILL_DISARM;
            _skillName = "disarm";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 13, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p2 = {CLASS_PALADIN, 20, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_RANGER, 20, 0};
            _practiceInfo.push_back(p3);
            PracticeInfo p4 = {CLASS_MONK, 31, 0};
            _practiceInfo.push_back(p4);
            PracticeInfo p5 = {CLASS_MERCENARY, 25, 0};
            _practiceInfo.push_back(p5);
            PracticeInfo p6 = {CLASS_BLACKGUARD, 20, 0};
            _practiceInfo.push_back(p6);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillDisarm();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillSpinkick : public GenericSkill
{
    public:
        SkillSpinkick() : GenericSkill()
        {
            _skillNum = SKILL_SPINKICK;
            _skillName = "spinkick";
            _minPosition = POS_DEAD;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillSpinkick();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillRoundhouse : public GenericSkill
{
    public:
        SkillRoundhouse() : GenericSkill()
        {
            _skillNum = SKILL_ROUNDHOUSE;
            _skillName = "roundhouse";
            _minPosition = POS_DEAD;
            PracticeInfo p1 = {CLASS_RANGER, 20, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_MERCENARY, 5, 0};
            _practiceInfo.push_back(p2);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillRoundhouse();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillSidekick : public GenericSkill
{
    public:
        SkillSidekick() : GenericSkill()
        {
            _skillNum = SKILL_SIDEKICK;
            _skillName = "sidekick";
            _minPosition = POS_DEAD;
            PracticeInfo p1 = {CLASS_RANGER, 35, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillSidekick();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillSpinfist : public GenericSkill
{
    public:
        SkillSpinfist() : GenericSkill()
        {
            _skillNum = SKILL_SPINFIST;
            _skillName = "spinfist";
            _minPosition = POS_DEAD;
            PracticeInfo p1 = {CLASS_PALADIN, 6, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 7, 0};
            _practiceInfo.push_back(p2);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillSpinfist();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillJab : public GenericSkill
{
    public:
        SkillJab() : GenericSkill()
        {
            _skillNum = SKILL_JAB;
            _skillName = "jab";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 6, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillJab();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillHook : public GenericSkill
{
    public:
        SkillHook() : GenericSkill()
        {
            _skillNum = SKILL_HOOK;
            _skillName = "hook";
            _minPosition = POS_DEAD;
            PracticeInfo p1 = {CLASS_PALADIN, 22, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 23, 0};
            _practiceInfo.push_back(p2);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillHook();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillSweepkick : public GenericSkill
{
    public:
        SkillSweepkick() : GenericSkill()
        {
            _skillNum = SKILL_SWEEPKICK;
            _skillName = "sweepkick";
            _minPosition = POS_DEAD;
            PracticeInfo p1 = {CLASS_RANGER, 27, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_MONK, 25, 0};
            _practiceInfo.push_back(p2);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillSweepkick();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillTrip : public GenericSkill
{
    public:
        SkillTrip() : GenericSkill()
        {
            _skillNum = SKILL_TRIP;
            _skillName = "trip";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 18, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillTrip();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillUppercut : public GenericSkill
{
    public:
        SkillUppercut() : GenericSkill()
        {
            _skillNum = SKILL_UPPERCUT;
            _skillName = "uppercut";
            _minPosition = POS_DEAD;
            PracticeInfo p1 = {CLASS_PALADIN, 15, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 13, 0};
            _practiceInfo.push_back(p2);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillUppercut();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillGroinkick : public GenericSkill
{
    public:
        SkillGroinkick() : GenericSkill()
        {
            _skillNum = SKILL_GROINKICK;
            _skillName = "groinkick";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 12, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillGroinkick();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillClaw : public GenericSkill
{
    public:
        SkillClaw() : GenericSkill()
        {
            _skillNum = SKILL_CLAW;
            _skillName = "claw";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_RANGER, 32, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillClaw();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillRabbitpunch : public GenericSkill
{
    public:
        SkillRabbitpunch() : GenericSkill()
        {
            _skillNum = SKILL_RABBITPUNCH;
            _skillName = "rabbitpunch";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 10, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillRabbitpunch();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillImpale : public GenericSkill
{
    public:
        SkillImpale() : GenericSkill()
        {
            _skillNum = SKILL_IMPALE;
            _skillName = "impale";
            _minPosition = POS_DEAD;
            PracticeInfo p1 = {CLASS_PALADIN, 30, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 45, 0};
            _practiceInfo.push_back(p2);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillImpale();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillPeleKick : public GenericSkill
{
    public:
        SkillPeleKick() : GenericSkill()
        {
            _skillNum = SKILL_PELE_KICK;
            _skillName = "pele kick";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 41, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p2 = {CLASS_RANGER, 41, 0};
            _practiceInfo.push_back(p2);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillPeleKick();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillLungePunch : public GenericSkill
{
    public:
        SkillLungePunch() : GenericSkill()
        {
            _skillNum = SKILL_LUNGE_PUNCH;
            _skillName = "lunge punch";
            _minPosition = POS_DEAD;
            PracticeInfo p1 = {CLASS_PALADIN, 35, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 42, 0};
            _practiceInfo.push_back(p2);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillLungePunch();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillTornadoKick : public GenericSkill
{
    public:
        SkillTornadoKick() : GenericSkill()
        {
            _skillNum = SKILL_TORNADO_KICK;
            _skillName = "tornado kick";
            _minPosition = POS_DEAD;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillTornadoKick();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillTripleAttack : public GenericSkill
{
    public:
        SkillTripleAttack() : GenericSkill()
        {
            _skillNum = SKILL_TRIPLE_ATTACK;
            _skillName = "triple attack";
            _minPosition = POS_DEAD;
            PracticeInfo p1 = {CLASS_PALADIN, 45, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_RANGER, 43, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_MONK, 41, 0};
            _practiceInfo.push_back(p3);
            PracticeInfo p4 = {CLASS_MERCENARY, 38, 0};
            _practiceInfo.push_back(p4);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillTripleAttack();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillCounterAttack : public GenericSkill
{
    public:
        SkillCounterAttack() : GenericSkill()
        {
            _skillNum = SKILL_COUNTER_ATTACK;
            _skillName = "counter attack";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_RANGER, 31, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillCounterAttack();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillSwimming : public GenericSkill
{
    public:
        SkillSwimming() : GenericSkill()
        {
            _skillNum = SKILL_SWIMMING;
            _skillName = "swimming";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MAGE, 1, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 1, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_THIEF, 1, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p4 = {CLASS_PSIONIC, 2, 0};
            _practiceInfo.push_back(p4);
            PracticeInfo p5 = {CLASS_PHYSIC, 1, 0};
            _practiceInfo.push_back(p5);
            PracticeInfo p6 = {CLASS_PALADIN, 1, 0};
            _practiceInfo.push_back(p6);
            PracticeInfo p7 = {CLASS_RANGER, 1, 0};
            _practiceInfo.push_back(p7);
            PracticeInfo p8 = {CLASS_BARD, 1, 0};
            _practiceInfo.push_back(p8);
            PracticeInfo p9 = {CLASS_MONK, 7, 0};
            _practiceInfo.push_back(p9);
            PracticeInfo p10 = {CLASS_MERCENARY, 1, 0};
            _practiceInfo.push_back(p10);
            PracticeInfo p11 = {CLASS_BLACKGUARD, 1, 0};
            _practiceInfo.push_back(p11);
            PracticeInfo p12 = {CLASS_NECROMANCER, 1, 0};
            _practiceInfo.push_back(p12);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillSwimming();
        }

        // Swimming has no implementation since it doesn't DO
        // anything.
        bool perform(ExecutableVector &ev, obj_data *target) {
            return false;
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            return false;
        }

        bool perform(ExecutableVector &ev, CreatureList &list) {
            return false;
        }
};

class SkillThrowing : public GenericSkill
{
    public:
        SkillThrowing() : GenericSkill()
        {
            _skillNum = SKILL_THROWING;
            _skillName = "throwing";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 7, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p2 = {CLASS_RANGER, 9, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p3 = {CLASS_BARD, 3, 0};
            _practiceInfo.push_back(p3);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillThrowing();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillRiding : public GenericSkill
{
    public:
        SkillRiding() : GenericSkill()
        {
            _skillNum = SKILL_RIDING;
            _skillName = "riding";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MAGE, 1, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 1, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_THIEF, 1, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p4 = {CLASS_PALADIN, 1, 0};
            _practiceInfo.push_back(p4);
            PracticeInfo p5 = {CLASS_RANGER, 1, 0};
            _practiceInfo.push_back(p5);
            PracticeInfo p6 = {CLASS_BLACKGUARD, 1, 0};
            _practiceInfo.push_back(p6);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillRiding();
        }

        // Riding has no implementation since it doesn't DO
        // anything.
        bool perform(ExecutableVector &ev, obj_data *target) {
            return false;
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            return false;
        }

        bool perform(ExecutableVector &ev, CreatureList &list) {
            return false;
        }
};

class SkillAppraise : public GenericSkill
{
    public:
        SkillAppraise() : GenericSkill()
        {
            _skillNum = SKILL_APPRAISE;
            _skillName = "appraise";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MAGE, 10, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CLERIC, 10, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_THIEF, 10, 0};
            _practiceInfo.push_back(p2);
            PracticeInfo p4 = {CLASS_PSIONIC, 10, 0};
            _practiceInfo.push_back(p4);
            PracticeInfo p5 = {CLASS_PHYSIC, 10, 0};
            _practiceInfo.push_back(p5);
            PracticeInfo p6 = {CLASS_CYBORG, 10, 0};
            _practiceInfo.push_back(p6);
            PracticeInfo p7 = {CLASS_PALADIN, 10, 0};
            _practiceInfo.push_back(p7);
            PracticeInfo p8 = {CLASS_RANGER, 10, 0};
            _practiceInfo.push_back(p8);
            PracticeInfo p9 = {CLASS_MONK, 10, 0};
            _practiceInfo.push_back(p9);
            PracticeInfo p10 = {CLASS_MERCENARY, 10, 0};
            _practiceInfo.push_back(p10);
            PracticeInfo p11 = {CLASS_BLACKGUARD, 10, 0};
            _practiceInfo.push_back(p11);
            PracticeInfo p12 = {CLASS_NECROMANCER, 10, 0};
            _practiceInfo.push_back(p12);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillAppraise();
        }

        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev, obj))
                return false;

            if (!obj)
                return false;

            char buf[128];
            char *tmp;
            int level = getPowerLevel();
            unsigned long eq_req_flags;
            long cost;
           
            ExecutableObject *eo = new ExecutableObject(getOwner());
            eo->setVictim(_owner);
            
            sprinttype(GET_OBJ_TYPE(obj), item_type_descs, buf);
            tmp = tmp_sprintf("%s is %s.", tmp_capitalize(obj->name), buf);
            eo->addToCharMessage(tmp);

            if (level > 30) {
                eq_req_flags = 
                    ITEM_ANTI_GOOD | ITEM_ANTI_EVIL | ITEM_ANTI_NEUTRAL |
                    ITEM_ANTI_MAGIC_USER | ITEM_ANTI_CLERIC | ITEM_ANTI_THIEF |
                    ITEM_ANTI_WARRIOR | ITEM_NOSELL | ITEM_ANTI_NECROMANCER |
                    ITEM_ANTI_PSYCHIC | ITEM_ANTI_PHYSIC | ITEM_ANTI_CYBORG |
                    ITEM_ANTI_PALADIN | ITEM_ANTI_RANGER | ITEM_ANTI_BARD |
                    ITEM_ANTI_MONK | ITEM_BLURRED | ITEM_DAMNED | ITEM_ANTI_BLACKGUARD;
                if (GET_OBJ_EXTRA(obj) & eq_req_flags) {
                    tmp = tmp_printbits(GET_OBJ_EXTRA(obj) & eq_req_flags, 
                            extra_bits);
                    tmp = tmp_sprintf("Item is: %s", tmp);
                    eo->addToCharMessage(tmp);
                }

                eq_req_flags = ITEM2_ANTI_MERC;
                if (GET_OBJ_EXTRA2(obj) & eq_req_flags) {
                    tmp = tmp_printbits(GET_OBJ_EXTRA2(obj) & eq_req_flags, 
                            extra2_bits);
                    tmp = tmp_sprintf("Item is: %s", tmp);
                    eo->addToCharMessage(tmp);
                }

                eq_req_flags =
                    ITEM3_REQ_MAGE | ITEM3_REQ_CLERIC | ITEM3_REQ_THIEF |
                    ITEM3_REQ_WARRIOR | ITEM3_REQ_NECROMANCER | ITEM3_REQ_PSIONIC |
                    ITEM3_REQ_PHYSIC | ITEM3_REQ_CYBORG | ITEM3_REQ_PALADIN |
                    ITEM3_REQ_RANGER | ITEM3_REQ_BARD | ITEM3_REQ_MONK |
                    ITEM3_REQ_VAMPIRE | ITEM3_REQ_MERCENARY | ITEM3_REQ_BLACKGUARD;
                if (GET_OBJ_EXTRA3(obj) & eq_req_flags) {
                    tmp = tmp_printbits(GET_OBJ_EXTRA3(obj) & eq_req_flags, 
                            extra3_bits);
                    tmp = tmp_sprintf("Item is: %s", tmp);
                    eo->addToCharMessage(tmp);
                }
            }

            tmp = tmp_sprintf("Item weighs around %d lbs, and is made of %s.",
                    obj->getWeight(), material_names[GET_OBJ_MATERIAL(obj)]);
            eo->addToCharMessage(tmp);

            if (level > 100)
                cost = 0;
            else
                cost = (100 - level) * GET_OBJ_COST(obj) / 100;

            cost = GET_OBJ_COST(obj) + number(0, cost) - cost / 2;

            if (cost > 0) {
                tmp = tmp_sprintf("Item looks to be worth about %ld.", 
                        cost);
                eo->addToCharMessage(tmp);
            }
            else {
                tmp = tmp_sprintf("Item doesn't look to be worth anything.");
                eo->addToCharMessage(tmp);
            }

            switch (GET_OBJ_TYPE(obj)) {
                case ITEM_SCROLL:
                case ITEM_POTION:
                    if (level > 80) {
                        char *spell1 = NULL, *spell2 = NULL, *spell3 = NULL;
                        if (GET_OBJ_VAL(obj, 1) >= 1)
                            spell1 = tmp_sprintf( 
                                    spell_to_str(GET_OBJ_VAL(obj, 1)));
                        if (GET_OBJ_VAL(obj, 2) >= 1)
                            spell2 = tmp_sprintf( 
                                    spell_to_str(GET_OBJ_VAL(obj, 2)));
                        if (GET_OBJ_VAL(obj, 3) >= 1)
                            spell3  = tmp_sprintf(
                                    spell_to_str(GET_OBJ_VAL(obj, 3)));
                        
                        tmp = tmp_sprintf("This %s casts:",
                                item_types[(int)GET_OBJ_TYPE(obj)]);

                        if (spell1)
                            tmp = tmp_sprintf("%s %s", tmp, spell1);
                        if (spell2)
                            tmp = tmp_sprintf("%s, %s%s", tmp, 
                                    spell3 ? "" : "and ", spell2);
                        if (spell3)
                            tmp = tmp_sprintf("%s, and %s", tmp, spell3);

                        eo->addToCharMessage(tmp);
                    }
                    break;
                case ITEM_WAND:
                case ITEM_STAFF:
                    if (level > 80) {
                        tmp = tmp_sprintf("This %s casts: %s",
                                item_types[(int)GET_OBJ_TYPE(obj)],
                                spell_to_str(GET_OBJ_VAL(obj, 3)));
                        
                        eo->addToCharMessage(tmp);
                        if (level > 90) {
                            tmp = tmp_sprintf("It has %d maximum charge%s "
                                    "and %d remaining.", GET_OBJ_VAL(obj, 1),
                                    GET_OBJ_VAL(obj, 1) == 1 ? "" : "s",
                                    GET_OBJ_VAL(obj, 2));
                            eo->addToCharMessage(tmp);
                        }
                    }
                    break;
                case ITEM_WEAPON:
                    tmp = tmp_sprintf("This weapon can deal up to %d "
                            "points of damage.", 
                            GET_OBJ_VAL(obj, 2) * GET_OBJ_VAL(obj, 1));
                    eo->addToCharMessage(tmp);
                    if (IS_OBJ_STAT2(obj, ITEM2_CAST_WEAPON)) {
                        tmp = tmp_sprintf("This weapon casts an offensive "
                                "spell.");
                        eo->addToCharMessage(tmp);
                    }
                    break;
                case ITEM_ARMOR:
                    if (GET_OBJ_VAL(obj, 0) < 2)
                        tmp = tmp_sprintf("This armor provides hardly any "
                                "protection.");
                    else if (GET_OBJ_VAL(obj, 0) < 5)
                        tmp = tmp_sprintf("This armor provides a little "
                                "protection.");
                    else if (GET_OBJ_VAL(obj, 0) < 15)
                        tmp = tmp_sprintf("This armor provides some "
                                "protection.");
                    else if (GET_OBJ_VAL(obj, 0) < 20)
                        tmp = tmp_sprintf("This armor provides a lot of "
                                "protection.");
                    else if (GET_OBJ_VAL(obj, 0) < 25)
                        tmp = tmp_sprintf("This armor provides a ridiculous "
                                "amount of protection.");
                    else
                        tmp = tmp_sprintf("This armor provides an insane "
                                "amount of protection.");
                    eo->addToCharMessage(tmp);
                    break;
                case ITEM_CONTAINER:
                    tmp = tmp_sprintf("This container holds a maximum "
                            "of %d pounds.", GET_OBJ_VAL(obj, 0));
                    eo->addToCharMessage(tmp);
                    break;
                case ITEM_FOUNTAIN:
                case ITEM_DRINKCON:
                    tmp = tmp_sprintf("This container holds some %s",
                            drinks[GET_OBJ_VAL(obj, 2)]);
                    break;
            }

            for (int i = 0; i < MIN(MAX_OBJ_AFFECT, level / 25); i++) {
                if (obj->affected[i].location != APPLY_NONE) {
                    sprinttype(obj->affected[i].location, apply_types, buf);
                    if (obj->affected[i].modifier > 0)
                        tmp = tmp_sprintf("Item increases %s", buf);
                    else if (obj->affected[i].modifier < 0)
                        tmp = tmp_sprintf("Item decreases %s", buf);

                    eo->addToCharMessage(tmp);
                }
            }
                
            eo->setSkill(getSkillNumber());
            ev.push_back_id(eo);
            return true;
        }
};

class SkillPipemaking : public GenericSkill
{
    public:
        SkillPipemaking() : GenericSkill()
        {
            _skillNum = SKILL_PIPEMAKING;
            _skillName = "pipemaking";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MAGE, 40, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_RANGER, 15, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_BARD, 13, 0};
            _practiceInfo.push_back(p2);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillPipemaking();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillSecondWeapon : public GenericSkill
{
    public:
        SkillSecondWeapon() : GenericSkill()
        {
            _skillNum = SKILL_SECOND_WEAPON;
            _skillName = "second weapon";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_RANGER, 14, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_MERCENARY, 18, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillSecondWeapon();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillScanning : public GenericSkill
{
    public:
        SkillScanning() : GenericSkill()
        {
            _skillNum = SKILL_SCANNING;
            _skillName = "scanning";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 31, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_RANGER, 24, 0};
            _practiceInfo.push_back(p1);
            PracticeInfo p2 = {CLASS_MERCENARY, 22, 0};
            _practiceInfo.push_back(p2);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillScanning();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillRetreat : public GenericSkill
{
    public:
        SkillRetreat() : GenericSkill()
        {
            _skillNum = SKILL_RETREAT;
            _skillName = "retreat";
            _minPosition = POS_DEAD;
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

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillRetreat();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillGunsmithing : public GenericSkill
{
    public:
        SkillGunsmithing() : GenericSkill()
        {
            _skillNum = SKILL_GUNSMITHING;
            _skillName = "gunsmithing";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MERCENARY, 20, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillGunsmithing();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillPistolwhip : public GenericSkill
{
    public:
        SkillPistolwhip() : GenericSkill()
        {
            _skillNum = SKILL_PISTOLWHIP;
            _skillName = "pistolwhip";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MERCENARY, 20, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillPistolwhip();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillCrossface : public GenericSkill
{
    public:
        SkillCrossface() : GenericSkill()
        {
            _skillNum = SKILL_CROSSFACE;
            _skillName = "crossface";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MERCENARY, 26, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillCrossface();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillWrench : public GenericSkill
{
    public:
        SkillWrench() : GenericSkill()
        {
            _skillNum = SKILL_WRENCH;
            _skillName = "wrench";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MERCENARY, 6, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillWrench();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillElusion : public GenericSkill
{
    public:
        SkillElusion() : GenericSkill()
        {
            _skillNum = SKILL_ELUSION;
            _skillName = "elusion";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MERCENARY, 11, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillElusion();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillInfiltrate : public GenericSkill
{
    public:
        SkillInfiltrate() : GenericSkill()
        {
            _skillNum = SKILL_INFILTRATE;
            _skillName = "infiltrate";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MERCENARY, 27, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillInfiltrate();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillShoulderThrow : public GenericSkill
{
    public:
        SkillShoulderThrow() : GenericSkill()
        {
            _skillNum = SKILL_SHOULDER_THROW;
            _skillName = "shoulder throw";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MERCENARY, 39, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillShoulderThrow();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillGarotte : public GenericSkill
{
    public:
        SkillGarotte() : GenericSkill()
        {
            _skillNum = SKILL_GAROTTE;
            _skillName = "garotte";
            _minPosition = POS_DEAD;
            _violent = true;
            PracticeInfo p0 = {CLASS_MERCENARY, 21, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillGarotte();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillDemolitions : public GenericSkill
{
    public:
        SkillDemolitions() : GenericSkill()
        {
            _skillNum = SKILL_DEMOLITIONS;
            _skillName = "demolitions";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MERCENARY, 19, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillDemolitions();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillReconfigure : public GenericSkill
{
    public:
        SkillReconfigure() : GenericSkill()
        {
            _skillNum = SKILL_RECONFIGURE;
            _skillName = "reconfigure";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 11, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillReconfigure();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillReboot : public GenericSkill
{
    public:
        SkillReboot() : GenericSkill()
        {
            _skillNum = SKILL_REBOOT;
            _skillName = "reboot";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 1, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillReboot();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillMotionSensor : public GenericProgram
{
    public:
        SkillMotionSensor() : GenericProgram()
        {
            _skillNum = SKILL_MOTION_SENSOR;
            _skillName = "motion sensor";
            _minMove = 30;
            _maxMove = 30;
            _moveChange = 0;
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 16, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillMotionSensor();
        }

        bool activate(ExecutableVector &ev) {
            if (!GenericProgram::perform(ev))
                return false;

            int level = getPowerLevel();
            string toVictMessage = "Activating motion sensors.";

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, _owner);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(SKILL_MOTION_SENSOR);
            ca->setWearoffToChar("Shutting down motion sensor.");
            ca->setTarget(_owner);
            ca->setLevel(level);
            ca->addApply(APPLY_MOVE, -_minMove);
            ca->setIsPermenant();
            ao->addToVictMessage(toVictMessage);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SkillStasis : public GenericProgram
{
    public:
        SkillStasis() : GenericProgram()
        {
            _skillNum = SKILL_STASIS;
            _skillName = "stasis";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 10, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillStasis();
        }

        bool activate(ExecutableVector &ev) {
            if (!GenericProgram::perform(ev))
                return false;

            if (_owner->getPosition() >= POS_FLYING) {
                send_to_char(_owner, "Go into stasis while flying?!?!?\r\n");
                return false;
            }

            TOGGLE_BIT(AFF3_FLAGS(_owner), AFF3_STASIS);
            _owner->setPosition(POS_SLEEPING);
            WAIT_STATE(_owner, PULSE_VIOLENCE * 5);
            send_to_char(_owner, "Entering static state.  Halting "
                    "system processes.\r\n");
            act("$n lies down and enters a static state.", FALSE, _owner,
                    0, 0, TO_ROOM);

            return true;
        }
};

class SkillEnergyField : public GenericProgram
{
    public:
        SkillEnergyField() : GenericProgram()
        {
            _skillNum = SKILL_ENERGY_FIELD;
            _skillName = "energy field";
            _minMove = 40;
            _maxMove = 40;
            _moveChange = 0;
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 32, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillEnergyField();
        }

        bool activate(ExecutableVector &ev) {
            if (!GenericProgram::perform(ev))
                return false;

            int level = getPowerLevel();
            string toVictMessage = "Energy fields activated.";
            string toRoomMessage = "A field of energy hums to life around "
                "$n's body.";

            // FIXME: EnergyField needs it's own custom affect
            AffectObject *ao = new AffectObject("CreatureAffect", _owner, _owner);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(SKILL_ENERGY_FIELD);
            ca->setWearoffToChar("Energy field deactivated.");
            ca->setWearoffToRoom("$n's energy field dissipates.");
            ca->setTarget(_owner);
            ca->setLevel(level);
            ca->addApply(APPLY_MOVE, -_minMove);
            ca->setBitvector(1, AFF2_ENERGY_FIELD);
            ca->setIsPermenant();
            ao->addToVictMessage(toVictMessage);
            ao->addToRoomMessage(toVictMessage);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            int sect_type = SECT_TYPE(_owner->in_room);
            if (sect_type == SECT_UNDERWATER 
                || sect_type == SECT_DEEP_OCEAN 
                || sect_type == SECT_WATER_SWIM 
                || sect_type == SECT_WATER_NOSWIM) {
                CreatureList::iterator it = _owner->in_room->people.begin();
                for (; it != _owner->in_room->people.end(); ++it) {
                    if (*it != _owner) {
                        //Create a new damage object hrere
                        WAIT_STATE(_owner, 1 RL_SEC);
                    }
                }

                send_to_char(_owner, "DANGER:  Hazardous short detected!!  "
                        "Energy fields shutting down.");
                ca->affectRemove();

                return false;
            }

            return true;
        }
};

class SkillReflexBoost : public GenericProgram
{
    public:
        SkillReflexBoost() : GenericProgram()
        {
            _skillNum = SKILL_REFLEX_BOOST;
            _skillName = "reflex boost";
            _minMove = 40;
            _maxMove = 40;
            _moveChange = 0;
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 18, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillReflexBoost();
        }

        bool activate(ExecutableVector &ev) {
            if (!GenericProgram::perform(ev))
                return false;

            int level = getPowerLevel();
            string toVictMessage = "Activating reflex boosters.";

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, _owner);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(SKILL_REFLEX_BOOST);
            ca->setWearoffToChar("Deactivating reflex boosters.");
            ca->setTarget(_owner);
            ca->setLevel(level);
            ca->addApply(APPLY_MOVE, -_minMove);
            ca->setBitvector(1, AFF2_HASTE);
            ca->setIsPermenant();
            ao->addToVictMessage(toVictMessage);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SkillImplantW : public GenericSkill
{
    public:
        SkillImplantW() : GenericSkill()
        {
            _skillNum = SKILL_IMPLANT_W;
            _skillName = "implant w";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 26, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillImplantW();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillOffensivePos : public GenericProgram
{
    public:
        SkillOffensivePos() : GenericProgram()
        {
            _skillNum = SKILL_OFFENSIVE_POS;
            _skillName = "offensive posturing";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 15, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillOffensivePos();
        }

        bool activate(ExecutableVector &ev) {
            if (!GenericProgram::perform(ev))
                return false;

            if (_owner->affectedBy(SKILL_DEFENSIVE_POS)) {
                send_to_char(_owner, "%sERROR:%s defensive posturing is "
                        "already activated.\r\n", CCCYN(_owner, C_NRM),
                        CCNRM(_owner, C_NRM));
                return false;
            }

            int level = getPowerLevel();
            string toVictMessage = "Offensive posturing enabled.";
            string toVictWearoff = "Offensive posturing disabled.";

            // The armor affect
            AffectObject *ao = new AffectObject("CreatureAffect", _owner, _owner);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(SKILL_OFFENSIVE_POS);
            ca->setWearoffToChar(toVictWearoff);
            ca->setTarget(_owner);
            ca->setLevel(level);
            ca->addApply(APPLY_AC, ca->getLevel() * 3 / 4);
            ca->addApply(APPLY_DAMROLL, ca->getLevel() / 5);
            ca->addApply(APPLY_HITROLL, ca->getLevel() / 5);
            ca->setIsPermenant();

            ao->addToVictMessage(toVictMessage);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SkillDefensivePos : public GenericProgram
{
    public:
        SkillDefensivePos() : GenericProgram()
        {
            _skillNum = SKILL_DEFENSIVE_POS;
            _skillName = "defensive posturing";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 15, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillDefensivePos();
        }

        bool activate(ExecutableVector &ev) {
            if (!GenericProgram::perform(ev))
                return false;

            if (_owner->affectedBy(SKILL_OFFENSIVE_POS)) {
                send_to_char(_owner, "%sERROR:%s offensive posturing is "
                        "already activated.\r\n", CCCYN(_owner, C_NRM),
                        CCNRM(_owner, C_NRM));
                return false;
            }

            int level = getPowerLevel();
            string toVictMessage = "Defensive posturing enabled.";
            string toVictWearoff = "Defensive posturing disabled.";

            // The armor affect
            AffectObject *ao = new AffectObject("CreatureAffect", _owner, _owner);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(SKILL_DEFENSIVE_POS);
            ca->setWearoffToChar(toVictWearoff);
            ca->setTarget(_owner);
            ca->setLevel(level);
            ca->addApply(APPLY_AC, -(ca->getLevel() * 3 / 4));
            ca->addApply(APPLY_DAMROLL, -(ca->getLevel() /5));
            ca->addApply(APPLY_HITROLL, -(ca->getLevel() / 5));
            ca->setIsPermenant();

            ao->addToVictMessage(toVictMessage);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SkillMeleeCombatTactics : public GenericProgram
{
    public:
        SkillMeleeCombatTactics() : GenericProgram()
        {
            _skillNum = SKILL_MELEE_COMBAT_TAC;
            _skillName = "melee combat tactics";
            _minMove = 30;
            _maxMove = 30;
            _moveChange = 0;
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 24, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillMeleeCombatTactics();
        }

        bool activate(ExecutableVector &ev) {
            if (!GenericProgram::perform(ev))
                return false;

            int level = getPowerLevel();
            string toVictMessage = "Activating melee combat tactics.";

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, _owner);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(SKILL_MELEE_COMBAT_TAC);
            ca->setWearoffToChar("Deactivating melee combat tactics.");
            ca->setTarget(_owner);
            ca->setLevel(level);
            ca->addApply(APPLY_MOVE, -_minMove);
            ca->setIsPermenant();
            ao->addToVictMessage(toVictMessage);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SkillNeuralBridging : public GenericProgram
{
    public:
        SkillNeuralBridging() : GenericProgram()
        {
            _skillNum = SKILL_NEURAL_BRIDGING;
            _skillName = "cogenic neural bridging";
            _minMove = 40;
            _maxMove = 40;
            _moveChange = 0;
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 35, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillNeuralBridging();
        }

        bool activate(ExecutableVector &ev) {
            if (!GenericProgram::perform(ev))
                return false;

            int level = getPowerLevel();
            string toVictMessage = "Activating cogenic neural bridge.";

            // The negative maxmove affect
            AffectObject *ao = new AffectObject("CreatureAffect", _owner, _owner);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(SKILL_NEURAL_BRIDGING);
            ca->setWearoffToChar("Deactivating cogenic neural bridge.");
            ca->setTarget(_owner);
            ca->setLevel(level);
            ca->addApply(APPLY_MOVE, -_minMove);
            ca->setIsPermenant();
            ao->addToVictMessage(toVictMessage);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SkillNaniteReconstruction : public GenericProgram
{
    public:
        SkillNaniteReconstruction() : GenericProgram()
        {
            _skillNum = SKILL_NANITE_RECONSTRUCTION;
            _skillName = "nanite reconstruction";
            _minMove = 300;
            _maxMove = 300;
            _moveChange = 0;
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 33, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillNaniteReconstruction();
        }

        bool activate(ExecutableVector &ev) {
            if (!GenericProgram::perform(ev))
                return false;

            int level = getPowerLevel();
            string toVictMessage = "Nanite reconstruction in progress.";

            // The negative maxmove affect
            AffectObject *ao = new AffectObject("CreatureAffect", _owner, _owner);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(SKILL_NANITE_RECONSTRUCTION);
            ca->setWearoffToChar("Nanite reconstruction halted.");
            ca->setTarget(_owner);
            ca->setLevel(level);
            ca->addApply(APPLY_MOVE, -_minMove);
            ca->setIsPermenant();
            ao->addToVictMessage(toVictMessage);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;;
        }
};

class SkillAdrenalMaximizer : public GenericProgram
{
    public:
        SkillAdrenalMaximizer() : GenericProgram()
        {
            _skillNum = SKILL_ADRENAL_MAXIMIZER;
            _skillName = "shukutei adrenal maximizations";
            // Just so it shows up on the skills list
            _minMove = 30;
            _maxMove = 30;
            _moveChange = 0;
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 37, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillAdrenalMaximizer();
        }

        bool activate(ExecutableVector &ev) {
            if (!GenericProgram::perform(ev))
                return false;

            int level = getPowerLevel();
            string toVictMessage = "Shukutei adrenal maximizations enabled.";

            // The negative maxmove affect
            AffectObject *ao = new AffectObject("CreatureAffect", _owner, _owner);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(SKILL_ADRENAL_MAXIMIZER);
            ca->setWearoffToChar("Shukutei adrenal maximizations disabled.");
            ca->setTarget(_owner);
            ca->setLevel(level);
            ca->addApply(APPLY_MOVE, -_minMove);
            ca->addApply(APPLY_SPEED, ca->getLevel() / 7);
            ca->setBitvector(0, AFF_ADRENALINE);
            ca->setIsPermenant();

            ao->addToVictMessage(toVictMessage);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SkillPowerBoost : public GenericProgram
{
    public:
        SkillPowerBoost() : GenericProgram()
        {
            _skillNum = SKILL_POWER_BOOST;
            _skillName = "power boost";
            _minMove = 40;
            _maxMove = 40;
            _manaChange = 0;
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 18, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillPowerBoost();
        }

        bool activate(ExecutableVector &ev) {
            if (!GenericProgram::perform(ev))
                return false;

            int level = getPowerLevel();
            string toVictMessage = "Power boost enabled.";
            string toRoomMessage = "$n boosts $s power levels!";

            // The negative maxmove affect
            AffectObject *ao = new AffectObject("CreatureAffect", _owner, _owner);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(SKILL_POWER_BOOST);
            ca->setWearoffToChar("Unboosting power.");
            ca->setWearoffToRoom("$n reduces power.");
            ca->setTarget(_owner);
            ca->setLevel(level);
            ca->addApply(APPLY_MOVE, -_minMove);
            ca->addApply(APPLY_STR, ca->getLevel() >> 4);
            ca->setIsPermenant();

            ao->addToVictMessage(toVictMessage);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SkillFastboot : public GenericSkill
{
    public:
        SkillFastboot() : GenericSkill()
        {
            _skillNum = SKILL_FASTBOOT;
            _skillName = "fastboot";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 12, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillFastboot();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillSelfDestruct : public GenericSkill
{
    public:
        SkillSelfDestruct() : GenericSkill()
        {
            _skillNum = SKILL_SELF_DESTRUCT;
            _skillName = "self destruct";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 12, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillSelfDestruct();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillBioscan : public GenericSkill
{
    public:
        SkillBioscan() : GenericSkill()
        {
            _skillNum = SKILL_BIOSCAN;
            _skillName = "bioscan";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 12, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillBioscan();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillDischarge : public GenericSkill
{
    public:
        SkillDischarge() : GenericSkill()
        {
            _skillNum = SKILL_DISCHARGE;
            _skillName = "discharge";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 14, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillDischarge();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillSelfrepair : public GenericSkill
{
    public:
        SkillSelfrepair() : GenericSkill()
        {
            _skillNum = SKILL_SELFREPAIR;
            _skillName = "selfrepair";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 20, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillSelfrepair();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillCyborepair : public GenericSkill
{
    public:
        SkillCyborepair() : GenericSkill()
        {
            _skillNum = SKILL_CYBOREPAIR;
            _skillName = "cyborepair";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 30, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillCyborepair();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillOverhaul : public GenericSkill
{
    public:
        SkillOverhaul() : GenericSkill()
        {
            _skillNum = SKILL_OVERHAUL;
            _skillName = "overhaul";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 45, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillOverhaul();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillDamageControl : public GenericProgram
{
    public:
        SkillDamageControl() : GenericProgram()
        {
            _skillNum = SKILL_DAMAGE_CONTROL;
            _skillName = "damage control";
            _minMove = 30;
            _maxMove = 30;
            _moveChange = 0;
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 12, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillDamageControl();
        }

        bool activate(ExecutableVector &ev) {
            if (!GenericProgram::perform(ev))
                return false;

            int level = getPowerLevel();
            string toVictMessage = "Activating damage control systems.";

            // The negative maxmove affect
            AffectObject *ao = new AffectObject("CreatureAffect", _owner, _owner);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(SKILL_DAMAGE_CONTROL);
            ca->setWearoffToChar("Terminating damage control systems.");
            ca->setTarget(_owner);
            ca->setLevel(level);
            ca->addApply(APPLY_MOVE, -_minMove);
            ca->setIsPermenant();

            ao->addToVictMessage(toVictMessage);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SkillElectronics : public GenericSkill
{
    public:
        SkillElectronics() : GenericSkill()
        {
            _skillNum = SKILL_ELECTRONICS;
            _skillName = "electronics";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_PHYSIC, 13, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_CYBORG, 6, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillElectronics();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillHacking : public GenericSkill
{
    public:
        SkillHacking() : GenericSkill()
        {
            _skillNum = SKILL_HACKING;
            _skillName = "hacking";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 20, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillHacking();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillCyberscan : public GenericSkill
{
    public:
        SkillCyberscan() : GenericSkill()
        {
            _skillNum = SKILL_CYBERSCAN;
            _skillName = "cyberscan";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 30, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillCyberscan();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillCyboSurgery : public GenericSkill
{
    public:
        SkillCyboSurgery() : GenericSkill()
        {
            _skillNum = SKILL_CYBO_SURGERY;
            _skillName = "cybo surgery";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 45, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillCyboSurgery();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillHyperscanning : public GenericProgram
{
    public:
        SkillHyperscanning() : GenericProgram()
        {
            _skillNum = SKILL_HYPERSCAN;
            _skillName = "hyperscanning";
            _minMove = 10;
            _maxMove = 10;
            _moveChange = 0;
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 25, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillHyperscanning();
        }

        bool activate(ExecutableVector &ev) {
            if (!GenericProgram::perform(ev))
                return false;

            int level = getPowerLevel();
            string toVictMessage = "Activating hyperscanning device.";

            // The negative maxmove affect
            AffectObject *ao = new AffectObject("CreatureAffect", _owner, _owner);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(SKILL_HYPERSCAN);
            ca->setWearoffToChar("Shutting down hyperscanning device.");
            ca->setTarget(_owner);
            ca->setLevel(level);
            ca->addApply(APPLY_MOVE, -_minMove);
            ca->setIsPermenant();

            ao->addToVictMessage(toVictMessage);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SkillOverdrain : public GenericSkill
{
    public:
        SkillOverdrain() : GenericSkill()
        {
            _skillNum = SKILL_OVERDRAIN;
            _skillName = "overdrain";
            _minPosition = POS_DEAD;
            _targets = TAR_OBJ_INV | TAR_OBJ_EQUIP | TAR_OBJ_ROOM;
            PracticeInfo p0 = {CLASS_CYBORG, 8, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillOverdrain();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillEnergyWeapons : public GenericSkill
{
    public:
        SkillEnergyWeapons() : GenericSkill()
        {
            _skillNum = SKILL_ENERGY_WEAPONS;
            _skillName = "energy weapons";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 20, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_MERCENARY, 7, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillEnergyWeapons();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillProjWeapons : public GenericSkill
{
    public:
        SkillProjWeapons() : GenericSkill()
        {
            _skillNum = SKILL_PROJ_WEAPONS;
            _skillName = "proj weapons";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 20, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_MERCENARY, 8, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillProjWeapons();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillSpeedLoading : public GenericSkill
{
    public:
        SkillSpeedLoading() : GenericSkill()
        {
            _skillNum = SKILL_SPEED_LOADING;
            _skillName = "speed loading";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 25, 0};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_MERCENARY, 15, 0};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillSpeedLoading();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillPalmStrike : public GenericSkill
{
    public:
        SkillPalmStrike() : GenericSkill()
        {
            _skillNum = SKILL_PALM_STRIKE;
            _skillName = "palm strike";
            _minPosition = POS_STANDING;
            _nonCombative = true;
            PracticeInfo p0 = {CLASS_MONK, 4, 0};
            _practiceInfo.push_back(p0);
            _charWait = 4;
            _victWait = 2;
            _maxMove = 5;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillPalmStrike();
        }
        
        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev, target))
                return false;
            
            if (getOwner()->freeHands() >= 1) {
                int damage = dice(getPowerLevel(),getOwner()->getStr()>>2);
                damage += getOwner()->getDamroll();
                RAW_EQ_DAM(getOwner(), WEAR_HANDS, &damage); //add damage from hand eq
                
                DamageObject *damObj = new DamageObject(getOwner(), target);
                damObj->setDamage(damage);
                damObj->addToCharMessage("You deal a bone crushing palm strike to $N!");
                damObj->addToVictMessage("$n deals a bone crushing strike to your chest!");
                damObj->addToRoomMessage("$n deals a bone crushing palm strike to $N!");
                
                
                damObj->setDamageLocation(WEAR_BODY);
                damObj->setSkill(getSkillNumber());
                
                ev.push_back_id(damObj);
                
                //this just rescales power level to a number between 1 and 10
                //logarithmically.  At best this has a 10% chance of success,
                //at power level 1 it has 1.5%, power level 50 has 8.5% etc.
                if (log((float)getPowerLevel())*2.171 > random_percentage()) {
                    PositionObject *po = new PositionObject(getOwner(), target, POS_SITTING);
                    po->addToCharMessage("$N stumbles and falls to the ground!");
                    po->addToVictMessage("You stumble and fall to the ground!");
                    po->addToRoomMessage("$N stumbles and falls to the ground!");
                    
                    ev.push_back_id(po);
                }
                
                return true;
            }
            send_to_char(getOwner(),"You need a free hand to do that!");
            return false;
        }
        
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev, obj))
                return false;

            if (getOwner()->freeHands() >= 1) {
                ExecutableObject *eo = new ExecutableObject(getOwner());
                eo->setObject(obj);
                eo->setSkill(getSkillNumber());
                eo->addToCharMessage("You deal a devastating palm strike to $p!");
                eo->addToRoomMessage("$n deals a devastating palm strike to $p!");
                ev.push_back_id(eo);
                return true;
            }
            send_to_char(getOwner(),"You need a free hand to do that!");
            return false;
        }
};

class SkillThroatStrike : public GenericSkill
{
    public:
        SkillThroatStrike() : GenericSkill()
        {
            _skillNum = SKILL_THROAT_STRIKE;
            _skillName = "throat strike";
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _maxMove = 10;
            _charWait = 4;
            _victWait = 2;
            PracticeInfo p0 = {CLASS_MONK, 10, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillThroatStrike();
        }
        
        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev, target))
                return false;
            
            if (getOwner()->freeHands() >= 1) {
                int damage = dice(getPowerLevel(),(getOwner()->getStr()>>2)+(getOwner()->getDex()>>3));
                damage += getOwner()->getDamroll();
                RAW_EQ_DAM(getOwner(), WEAR_HANDS, &damage); //add damage from hand eq
                
                if (target->getPosition() <= POS_STUNNED)
                    damage *= 2;
                
                DamageObject *damObj = new DamageObject(getOwner(), target);
                damObj->setDamage(damage);
                damObj->addToCharMessage("You deal a powerful throat strike to $N!");
                damObj->addToVictMessage("$n deals a powerful strike to your throat!");
                damObj->addToRoomMessage("$n deals a powerful throat strike to $N!");
                
                damObj->setDamageLocation(WEAR_NECK_1);
                damObj->setSkill(getSkillNumber());
                
                ev.push_back_id(damObj);
                return true;
            }
            send_to_char(getOwner(),"You need a free hand to do that!");
            return false;
        }
        
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev, obj))
                return false;

            if (getOwner()->freeHands() >= 1) {
                ExecutableObject *eo = new ExecutableObject(getOwner());
                eo->setObject(obj);
                eo->setSkill(getSkillNumber());
                eo->addToCharMessage("You deal a powerful throat strike to $p!");
                eo->addToRoomMessage("$n deals a powerful throat strike to $p!");
                ev.push_back_id(eo);
                return true;
            }
            send_to_char(getOwner(),"You need a free hand to do that!");
            return false;
        }
};

class SkillWhirlwind : public GenericSkill
{
    public:
        SkillWhirlwind() : GenericSkill()
        {
            _skillNum = SKILL_WHIRLWIND;
            _skillName = "whirlwind";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MONK, 40, 0};
            _practiceInfo.push_back(p0);
            _charWait = 8;
            _maxMove = 5;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillWhirlwind();
        }

        void addWhirlwindEO(ExecutableVector &ev, Creature *target) {
            CostObject *co = new CostObject(getOwner());
            co->setMove(getMoveCost());
            ev.push_back_id(co);
            
            int dam = dice(getPowerLevel(),5);
            dam += getOwner()->getDamroll();
            DamageObject *damObj = new DamageObject(getOwner(), target);
            damObj->setDamage(dam);
            if (random_binary()) {
                damObj->addToCharMessage("You blindside $N with your whirling fist!");
                damObj->addToVictMessage("$n blindsides you with $s whirling fist!");
                damObj->addToRoomMessage("$n blindsides $N with $s whirling fist!");
            } else {
                damObj->addToCharMessage("You blindside $N with your whirling foot!");
                damObj->addToVictMessage("$n blindsides you with $s whirling foot!");
                damObj->addToRoomMessage("$n blindsides $N with $s whirling foot!");
            }
            damObj->setSkill(getSkillNumber());
            ev.push_back_id(damObj);
        }
        
        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev,target))
                return false;
            
            int numAttacks = getPowerLevel()/19 + MIN(getOwner()->numCombatants(), 6);
            
            for (int i=0; i<numAttacks; i++) {
                if (random_percentage() < 25) {
                    addWhirlwindEO(ev, target);
                } else {
                    int targetNum = random_number_zero_low(getOwner()->numCombatants()-1);
                    CombatDataList::iterator ci = getOwner()->getCombatList()->begin();
                    for (int j=0; j<targetNum; j++) {
                        ++ci;
                    }
                    addWhirlwindEO(ev, ci->getOpponent());
                }
            }

            return true;
        }


        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            ExecutableObject *eo = new ExecutableObject(getOwner());
            eo->setObject(obj);
            eo->setSkill(getSkillNumber());
            if (random_binary()) {
                eo->addToCharMessage("You blindside $p with your whirling fist!");
                eo->addToRoomMessage("$n blindsides $p with $s whirling fist!");
            } else {
                eo->addToCharMessage("You blindside $p with your whirling foot!");
                eo->addToRoomMessage("$n blindsides $p with $s whirling foot!");
            }
            ev.push_back_id(eo);
            return true;
        }
        
        
};

class SkillHipToss : public GenericSkill
{
    public:
        SkillHipToss() : GenericSkill()
        {
            _skillNum = SKILL_HIP_TOSS;
            _skillName = "hip toss";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MONK, 27, 0};
            _practiceInfo.push_back(p0);
            _charWait = 7;
            _victWait = 3;
            _maxMove = 10;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillHipToss();
        }

               bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev, target))
                return false;
            
            if (getOwner()->freeHands() >= 1) {
                int damage = dice(getPowerLevel(),1);
                damage += getOwner()->getDamroll();
                RAW_EQ_DAM(getOwner(), WEAR_WAIST, &damage); //add damage from hand eq
                
                DamageObject *damObj = new DamageObject(getOwner(), target);
                damObj->setDamage(damage);
                damObj->setSkill(getSkillNumber());
                damObj->addToCharMessage("You hiptoss $N with a loud SMACK!");
                damObj->addToVictMessage("$n hiptosses you with a loud SMACK!");
                damObj->addToRoomMessage("$n hiptosses $N with a loud SMACK!");
                
                damObj->setDamageLocation(WEAR_WAIST);
                damObj->setSkill(getSkillNumber());
                
                ev.push_back_id(damObj);
                
                PositionObject *po = new PositionObject(getOwner(), target, POS_RESTING);
                po->setSkill(getSkillNumber());
                ev.push_back_id(po);
                
                return true;
            }
            send_to_char(getOwner(),"You need a free hand to do that!");
            return false;
        }
        
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev, obj))
                return false;

            if (getOwner()->freeHands() >= 1) {
                ExecutableObject *eo = new ExecutableObject(getOwner());
                eo->setObject(obj);
                eo->setSkill(getSkillNumber());
                eo->addToCharMessage("You hiptoss $p with a loud SMACK!");
                eo->addToRoomMessage("$n hiptosses $p with a loud SMACK!");
                ev.push_back_id(eo);
                return true;
            }
            send_to_char(getOwner(),"You need a free hand to do that!");
            return false;
        }
};

class SkillCombo : public GenericSkill
{
    public:
        SkillCombo() : GenericSkill()
        {
            _skillNum = SKILL_COMBO;
            _skillName = "combo";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MONK, 33, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillCombo();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillDeathTouch : public GenericSkill
{
    public:
        SkillDeathTouch() : GenericSkill()
        {
            _skillNum = SKILL_DEATH_TOUCH;
            _skillName = "death touch";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MONK, 49, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillDeathTouch();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillCraneKick : public GenericSkill
{
    public:
        SkillCraneKick() : GenericSkill()
        {
            _skillNum = SKILL_CRANE_KICK;
            _skillName = "crane kick";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MONK, 15, 0};
            _practiceInfo.push_back(p0);
            _maxMove = 10;
            _charWait = 4;
            _victWait = 1;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillCraneKick();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev, target))
                return false;
            
            int damage = dice(getPowerLevel(),(getOwner()->getStr()/6)+getOwner()->getDex()/4);
            damage += getOwner()->getDamroll();
            RAW_EQ_DAM(getOwner(), WEAR_FEET, &damage); //add damage from hand eq
            
            DamageObject *damObj = new DamageObject(getOwner(), target);
            damObj->setDamage(damage);
            damObj->addToCharMessage("You assume the crane form and kick $N in the head!");
            damObj->addToVictMessage("$n assumes the crane form and kicks you in the head!");
            damObj->addToRoomMessage("$n assumes the crane form and kicks $N in the head!");
            
            damObj->setDamageLocation(WEAR_HEAD);
            damObj->setSkill(getSkillNumber());
            
            ev.push_back_id(damObj);
            return true;
            
        }
        
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev, obj))
                return false;
            
            ExecutableObject *eo = new ExecutableObject(getOwner());
            eo->setObject(obj);
            eo->setSkill(getSkillNumber());
            eo->addToCharMessage("You assume the crane form and kick $p!");
            eo->addToRoomMessage("$n assumes the crane form and kicks $p!");
            ev.push_back_id(eo);
            return true;
            
        }
};

class SkillRidgehand : public GenericSkill
{
    public:
        SkillRidgehand() : GenericSkill()
        {
            _skillNum = SKILL_RIDGEHAND;
            _skillName = "ridgehand";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MONK, 30, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillRidgehand();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillScissorKick : public GenericSkill
{
    public:
        SkillScissorKick() : GenericSkill()
        {
            _skillNum = SKILL_SCISSOR_KICK;
            _skillName = "scissor kick";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MONK, 20, 0};
            _practiceInfo.push_back(p0);
            _maxMove = 15;
            _charWait = 8;
            _victWait = 3;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillScissorKick();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev, target))
                return false;
            
            int damage1 = dice(getPowerLevel(),MIN(7,MAX(1,(getOwner()->getDex()-target->getDex())/2)));
            damage1 += getOwner()->getDamroll();
            RAW_EQ_DAM(getOwner(), WEAR_FEET, &damage1); //add damage from feet eq
            int damage2 = dice(getPowerLevel(),MIN(7,MAX(1,(getOwner()->getDex()-target->getDex())/2)));
            damage2 += getOwner()->getDamroll();
            RAW_EQ_DAM(getOwner(), WEAR_FEET, &damage2); //add damage from feet eq
            
            DamageObject *damObj1 = new DamageObject(getOwner(), target);
            damObj1->setDamage(damage1);
            damObj1->addToCharMessage("You leap into the air and thrust with your left leg driving your foot into $N's chest!");
            damObj1->addToVictMessage("$n leaps into the air toward you and $s his left foot smashes into your chest!");
            damObj1->addToRoomMessage("$n leaps into the air and drives his left foot into $N's chest!");
            damObj1->setDamageLocation(WEAR_BODY);
            damObj1->setSkill(getSkillNumber());
            
            DamageObject *damObj2 = new DamageObject(getOwner(), target);
            damObj2->setDamage(damage2);
            damObj2->addToCharMessage("You rotate in midair bringing your right foot around and into $N's head!");
            damObj2->addToVictMessage("$n rotates in midair whipping his right foot around and into your head!");
            damObj2->addToRoomMessage("$n rotates in midair whipping his right foot around and into your head!");
            damObj2->setDamageLocation(WEAR_HEAD);
            damObj2->setSkill(getSkillNumber());
            
            ev.push_back_id(damObj1);
            ev.push_back_id(damObj2);
            return true;
            
        }
        
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev, obj))
                return false;
            
            ExecutableObject *eo = new ExecutableObject(getOwner());
            eo->setObject(obj);
            eo->setSkill(getSkillNumber());
            eo->addToCharMessage("You performs a flying scissor kick on $p!");
            eo->addToRoomMessage("$n performs a flying scissor kick on $p!");
            ev.push_back_id(eo);
            return true;
            
        }
};

class SkillPinchAlpha : public GenericSkill
{
    public:
        SkillPinchAlpha() : GenericSkill()
        {
            _skillNum = SKILL_PINCH_ALPHA;
            _skillName = "pinch alpha";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MONK, 6, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillPinchAlpha();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillPinchBeta : public GenericSkill
{
    public:
        SkillPinchBeta() : GenericSkill()
        {
            _skillNum = SKILL_PINCH_BETA;
            _skillName = "pinch beta";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MONK, 12, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillPinchBeta();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillPinchGamma : public GenericSkill
{
    public:
        SkillPinchGamma() : GenericSkill()
        {
            _skillNum = SKILL_PINCH_GAMMA;
            _skillName = "pinch gamma";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MONK, 35, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillPinchGamma();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillPinchDelta : public GenericSkill
{
    public:
        SkillPinchDelta() : GenericSkill()
        {
            _skillNum = SKILL_PINCH_DELTA;
            _skillName = "pinch delta";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MONK, 19, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillPinchDelta();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillPinchEpsilon : public GenericSkill
{
    public:
        SkillPinchEpsilon() : GenericSkill()
        {
            _skillNum = SKILL_PINCH_EPSILON;
            _skillName = "pinch epsilon";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MONK, 26, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillPinchEpsilon();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillPinchOmega : public GenericSkill
{
    public:
        SkillPinchOmega() : GenericSkill()
        {
            _skillNum = SKILL_PINCH_OMEGA;
            _skillName = "pinch omega";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MONK, 39, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillPinchOmega();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillPinchZeta : public GenericSkill
{
    public:
        SkillPinchZeta() : GenericSkill()
        {
            _skillNum = SKILL_PINCH_ZETA;
            _skillName = "pinch zeta";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MONK, 32, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillPinchZeta();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillMeditate : public GenericSkill
{
    public:
        SkillMeditate() : GenericSkill()
        {
            _skillNum = SKILL_MEDITATE;
            _skillName = "meditate";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MONK, 9, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillMeditate();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillKata : public GenericSkill
{
    public:
        SkillKata() : GenericSkill()
        {
            _skillNum = SKILL_KATA;
            _skillName = "kata";
            _minPosition = POS_DEAD;
            _maxMana = 40;
            _minMana = 40;
            _manaChange = 0;
            _maxMove = 10;
            _minMove = 10;
            _moveChange = 0;
            PracticeInfo p0 = {CLASS_MONK, 5, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillKata();
        }

        bool perform(ExecutableVector &ev) {
            if (!GenericSkill::perform(ev))
                return false;

            int level = getPowerLevel();
            string toVictMessage = "You carefully perform your finest kata.";
            string toRoomMessage = "$n begins to perform a kata with "
                "fluid motions.";

            if (!_owner->isNeutral()) {
                send_to_char(_owner, "Nothing seems to happen.");
                return false;
            }
            
            // Hitroll affect
            AffectObject *ao = new AffectObject("CreatureAffect", _owner, _owner);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(SKILL_KATA);
            ca->setWearoffToChar("Your kata has worn off.");
            ca->setLevel(level);
            ca->setTarget(_owner);
            int modifier = 3 + ca->getLevel() >> 2;
            ca->addApply(APPLY_HITROLL, modifier);
            ca->addApply(APPLY_DAMROLL, modifier);
            ca->setDuration((1 + ca->getLevel() >> 4) MUD_HOURS);

            ao->addToVictMessage(toVictMessage);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SkillEvasion : public GenericSkill
{
    public:
        SkillEvasion() : GenericSkill()
        {
            _skillNum = SKILL_EVASION;
            _skillName = "evasion";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MONK, 22, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillEvasion();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillFlying : public GenericSkill
{
    public:
        SkillFlying() : GenericSkill()
        {
            _skillNum = SKILL_FLYING;
            _skillName = "flying";
            _minPosition = POS_DEAD;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillFlying();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillSummon : public GenericSkill
{
    public:
        SkillSummon() : GenericSkill()
        {
            _skillNum = SKILL_SUMMON;
            _skillName = "summon";
            _minPosition = POS_DEAD;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillSummon();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillFeed : public GenericSkill
{
    public:
        SkillFeed() : GenericSkill()
        {
            _skillNum = SKILL_FEED;
            _skillName = "feed";
            _minPosition = POS_DEAD;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillFeed();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillBeguile : public GenericSkill
{
    public:
        SkillBeguile() : GenericSkill()
        {
            _skillNum = SKILL_BEGUILE;
            _skillName = "beguile";
            _minPosition = POS_DEAD;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillBeguile();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillDrain : public GenericSkill
{
    public:
        SkillDrain() : GenericSkill()
        {
            _skillNum = SKILL_DRAIN;
            _skillName = "drain";
            _minPosition = POS_DEAD;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillDrain();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillCreateVampire : public GenericSkill
{
    public:
        SkillCreateVampire() : GenericSkill()
        {
            _skillNum = SKILL_CREATE_VAMPIRE;
            _skillName = "create vampire";
            _minPosition = POS_DEAD;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillCreateVampire();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillControlUndead : public GenericSkill
{
    public:
        SkillControlUndead() : GenericSkill()
        {
            _skillNum = SKILL_CONTROL_UNDEAD;
            _skillName = "control undead";
            _minPosition = POS_DEAD;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillControlUndead();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillTerrorize : public GenericSkill
{
    public:
        SkillTerrorize() : GenericSkill()
        {
            _skillNum = SKILL_TERRORIZE;
            _skillName = "terrorize";
            _minPosition = POS_DEAD;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillTerrorize();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillLecture : public GenericSkill
{
    public:
        SkillLecture() : GenericSkill()
        {
            _skillNum = SKILL_LECTURE;
            _skillName = "lecture";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_PHYSIC, 30, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillLecture();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillEnergyConversion : public GenericSkill
{
    public:
        SkillEnergyConversion() : GenericSkill()
        {
            _skillNum = SKILL_ENERGY_CONVERSION;
            _skillName = "energy conversion";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_PHYSIC, 22, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillEnergyConversion();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellHellFire : public GenericSkill
{
    public:
        SpellHellFire() : GenericSkill()
        {
            _skillNum = SPELL_HELL_FIRE;
            _skillName = "hell fire";
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellHellFire();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellHellFrost : public GenericSkill
{
    public:
        SpellHellFrost() : GenericSkill()
        {
            _skillNum = SPELL_HELL_FROST;
            _skillName = "hell frost";
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellHellFrost();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellHellFireStorm : public GenericSkill
{
    public:
        SpellHellFireStorm() : GenericSkill()
        {
            _skillNum = SPELL_HELL_FIRE_STORM;
            _skillName = "hell fire storm";
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_IGNORE;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellHellFireStorm();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellHellFrostStorm : public GenericSkill
{
    public:
        SpellHellFrostStorm() : GenericSkill()
        {
            _skillNum = SPELL_HELL_FROST_STORM;
            _skillName = "hell frost storm";
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_IGNORE;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellHellFrostStorm();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellTrogStench : public GenericSkill
{
    public:
        SpellTrogStench() : GenericSkill()
        {
            _skillNum = SPELL_TROG_STENCH;
            _skillName = "trog stench";
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellTrogStench();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellFrostBreath : public GenericSkill
{
    public:
        SpellFrostBreath() : GenericSkill()
        {
            _skillNum = SPELL_FROST_BREATH;
            _skillName = "frost breath";
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellFrostBreath();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellFireBreath : public GenericSkill
{
    public:
        SpellFireBreath() : GenericSkill()
        {
            _skillNum = SPELL_FIRE_BREATH;
            _skillName = "fire breath";
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellFireBreath();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellGasBreath : public GenericSkill
{
    public:
        SpellGasBreath() : GenericSkill()
        {
            _skillNum = SPELL_GAS_BREATH;
            _skillName = "gas breath";
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellGasBreath();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellAcidBreath : public GenericSkill
{
    public:
        SpellAcidBreath() : GenericSkill()
        {
            _skillNum = SPELL_ACID_BREATH;
            _skillName = "acid breath";
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellAcidBreath();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellLightningBreath : public GenericSkill
{
    public:
        SpellLightningBreath() : GenericSkill()
        {
            _skillNum = SPELL_LIGHTNING_BREATH;
            _skillName = "lightning breath";
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellLightningBreath();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellManaRestore : public GenericSkill
{
    public:
        SpellManaRestore() : GenericSkill()
        {
            _skillNum = SPELL_MANA_RESTORE;
            _skillName = "mana restore";
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellManaRestore();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellPetrify : public GenericSkill
{
    public:
        SpellPetrify() : GenericSkill()
        {
            _skillNum = SPELL_PETRIFY;
            _skillName = "petrify";
            _minMana = 215;
            _maxMana = 330;
            _manaChange = 21;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellPetrify();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellSickness : public GenericSkill
{
    public:
        SpellSickness() : GenericSkill()
        {
            _skillNum = SPELL_SICKNESS;
            _skillName = "sickness";
            _minMana = 215;
            _maxMana = 330;
            _manaChange = 21;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellSickness();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellEssenceOfEvil : public GenericSkill
{
    public:
        SpellEssenceOfEvil() : GenericSkill()
        {
            _skillNum = SPELL_ESSENCE_OF_EVIL;
            _skillName = "essence of evil";
            _minMana = 215;
            _maxMana = 330;
            _manaChange = 21;
            _minPosition = POS_SITTING;
            _evil = true;
            _targets = TAR_CHAR_ROOM;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellEssenceOfEvil();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellEssenceOfGood : public GenericSkill
{
    public:
        SpellEssenceOfGood() : GenericSkill()
        {
            _skillNum = SPELL_ESSENCE_OF_GOOD;
            _skillName = "essence of good";
            _minMana = 215;
            _maxMana = 330;
            _manaChange = 21;
            _minPosition = POS_SITTING;
            _good = true;
            _targets = TAR_CHAR_ROOM;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellEssenceOfGood();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellShadowBreath : public GenericSkill
{
    public:
        SpellShadowBreath() : GenericSkill()
        {
            _skillNum = SPELL_SHADOW_BREATH;
            _skillName = "shadow breath";
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellShadowBreath();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellSteamBreath : public GenericSkill
{
    public:
        SpellSteamBreath() : GenericSkill()
        {
            _skillNum = SPELL_STEAM_BREATH;
            _skillName = "steam breath";
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellSteamBreath();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellQuadDamage : public GenericSkill
{
    public:
        SpellQuadDamage() : GenericSkill()
        {
            _skillNum = SPELL_QUAD_DAMAGE;
            _skillName = "quad damage";
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellQuadDamage();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellZhengisFistOfAnnihilation : public GenericSkill
{
    public:
        SpellZhengisFistOfAnnihilation() : GenericSkill()
        {
            _skillNum = SPELL_ZHENGIS_FIST_OF_ANNIHILATION;
            _skillName = "zhengis fist of annihilation";
            _minMana = 15;
            _maxMana = 30;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            _noList = true;
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellZhengisFistOfAnnihilation();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillSnipe : public GenericSkill
{
    public:
        SkillSnipe() : GenericSkill()
        {
            _skillNum = SKILL_SNIPE;
            _skillName = "snipe";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MERCENARY, 37, 2};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillSnipe();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillAdvImplantW : public GenericSkill
{
    public:
        SkillAdvImplantW() : GenericSkill()
        {
            _skillNum = SKILL_ADV_IMPLANT_W;
            _skillName = "adv implant w";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_CYBORG, 28, 5};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillAdvImplantW();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongAriaOfAsylum : public SongSpell
{
    public:
        SongAriaOfAsylum() : SongSpell()
        {
            _skillNum = SONG_ARIA_OF_ASYLUM;
            _skillName = "aria of asylum";
            _minMana = 70;
            _maxMana = 95;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_BARD, 35, 3};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongAriaOfAsylum();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongLamentOfLonging : public SongSpell
{
    public:
        SongLamentOfLonging() : SongSpell()
        {
            _skillNum = SONG_LAMENT_OF_LONGING;
            _skillName = "lament of longing";
            _minMana = 180;
            _maxMana = 320;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_WORLD | TAR_NOT_SELF;
            PracticeInfo p0 = {CLASS_BARD, 40, 4};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongLamentOfLonging();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongWallOfSound : public SongSpell
{
    public:
        SongWallOfSound() : SongSpell()
        {
            _skillNum = SONG_WALL_OF_SOUND;
            _skillName = "wall of sound";
            _minMana = 90;
            _maxMana = 150;
            _manaChange = 4;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_DIR;
            PracticeInfo p0 = {CLASS_BARD, 42, 1};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongWallOfSound();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillLingeringSong : public GenericSkill
{
    public:
        SkillLingeringSong() : GenericSkill()
        {
            _skillNum = SKILL_LINGERING_SONG;
            _skillName = "lingering song";
            _minPosition = POS_DEAD;
            _violent = false;
            PracticeInfo p0 = {CLASS_BARD, 20, 2};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillLingeringSong();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongFortissimo : public SongSpell
{
    public:
        SongFortissimo() : SongSpell()
        {
            _skillNum = SONG_FORTISSIMO;
            _skillName = "fortissimo";
            _minMana = 75;
            _maxMana = 150;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_BARD, 14, 3};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongFortissimo();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongLichsLyrics : public SongSpell
{
    public:
        SongLichsLyrics() : SongSpell()
        {
            _skillNum = SONG_LICHS_LYRICS;
            _skillName = "lichs lyrics";
            _minMana = 200;
            _maxMana = 300;
            _manaChange = 25;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_BARD, 45, 5};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongLichsLyrics();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SongMirrorImageMelody : public SongSpell
{
    public:
        SongMirrorImageMelody() : SongSpell()
        {
            _skillNum = SONG_MIRROR_IMAGE_MELODY;
            _skillName = "mirror image melody";
            _minMana = 50;
            _maxMana = 75;
            _manaChange = 10;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_BARD, 46, 6};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SongMirrorImageMelody();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!SongSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellFireBreathing : public ArcaneSpell
{
    public:
        SpellFireBreathing() : ArcaneSpell()
        {
            _skillNum = SPELL_FIRE_BREATHING;
            _skillName = "fire breathing";
            _minMana = 180;
            _maxMana = 280;
            _manaChange = 10;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_MAGE, 35, 1};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellFireBreathing();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellVestigialRune : public ArcaneSpell
{
    public:
        SpellVestigialRune() : ArcaneSpell()
        {
            _skillNum = SPELL_VESTIGIAL_RUNE;
            _skillName = "vestigial rune";
            _minMana = 380;
            _maxMana = 450;
            _manaChange = 10;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_OBJ_ROOM | TAR_OBJ_INV;
            PracticeInfo p0 = {CLASS_MAGE, 45, 1};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellVestigialRune();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellPrismaticSphere : public ArcaneSpell
{
    public:
        SpellPrismaticSphere() : ArcaneSpell()
        {
            _skillNum = SPELL_PRISMATIC_SPHERE;
            _skillName = "prismatic sphere";
            _minMana = 30;
            _maxMana = 170;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_MAGE, 40, 1};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellPrismaticSphere();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellQuantumRift : public PhysicsSpell
{
    public:
        SpellQuantumRift() : PhysicsSpell()
        {
            _skillNum = SPELL_QUANTUM_RIFT;
            _skillName = "quantum rift";
            _minMana = 300;
            _maxMana = 400;
            _manaChange = 5;
            _minPosition = POS_SITTING;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_PHYSIC, 45, 5};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellQuantumRift();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellStoneskin : public ArcaneSpell
{
    public:
        SpellStoneskin() : ArcaneSpell()
        {
            _skillNum = SPELL_STONESKIN;
            _skillName = "stoneskin";
            _minMana = 25;
            _maxMana = 60;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_RANGER, 20, 4};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellStoneskin();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellTelekinesis : public ArcaneSpell
{
    public:
        SpellTelekinesis() : ArcaneSpell()
        {
            _skillNum = SPELL_TELEKINESIS;
            _skillName = "telekinesis";
            _minMana = 70;
            _maxMana = 165;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_MAGE, 28, 0};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellTelekinesis();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, 
                    target);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();

            ca->setSkillNum(SPELL_TELEKINESIS);
            ca->setDuration((1 + (level >> 2)) MUD_HOURS);
            ca->setLevel(getPowerLevel());
            ca->setBitvector(1, AFF2_TELEKINESIS);
            ca->setAccumDuration(true);
            ca->setWearoffToChar("Your telekinesis has worn off.");

            ao->addToVictMessage("You feel able to carry a greater load.");
            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SpellShieldOfRighteousness : public DivineSpell
{
    public:
        SpellShieldOfRighteousness() : DivineSpell()
        {
            _skillNum = SPELL_SHIELD_OF_RIGHTEOUSNESS;
            _skillName = "shield of righteousness";
            _minMana = 20;
            _maxMana = 50;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _good = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_CLERIC, 32, 2};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellShieldOfRighteousness();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellSunRay : public DivineSpell
{
    public:
        SpellSunRay() : DivineSpell()
        {
            _skillNum = SPELL_SUN_RAY;
            _skillName = "sun ray";
            _minMana = 40;
            _maxMana = 100;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _violent = true;
            _good = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_CLERIC, 20, 3};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellSunRay();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellTaint : public DivineSpell
{
    public:
        SpellTaint() : DivineSpell()
        {
            _skillNum = SPELL_TAINT;
            _skillName = "taint";
            _minMana = 180;
            _maxMana = 250;
            _manaChange = 10;
            _minPosition = POS_STANDING;
            _violent = true;
            _evil = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_BLACKGUARD, 42, 9};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellTaint();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellBlackmantle : public DivineSpell
{
    public:
        SpellBlackmantle() : DivineSpell()
        {
            _skillNum = SPELL_BLACKMANTLE;
            _skillName = "blackmantle";
            _minMana = 80;
            _maxMana = 120;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _violent = true;
            _evil = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_CLERIC, 42, 1};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellBlackmantle();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellSanctification : public DivineSpell
{
    public:
        SpellSanctification() : DivineSpell()
        {
            _skillNum = SPELL_SANCTIFICATION;
            _skillName = "sanctification";
            _minMana = 60;
            _maxMana = 100;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _good = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_PALADIN, 33, 1};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellSanctification();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellStigmata : public DivineSpell
{
    public:
        SpellStigmata() : DivineSpell()
        {
            _skillNum = SPELL_STIGMATA;
            _skillName = "stigmata";
            _minMana = 30;
            _maxMana = 60;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _violent = true;
            _good = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_CLERIC, 23, 1};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellStigmata();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellDivinePower : public DivineSpell
{
    public:
        SpellDivinePower() : DivineSpell()
        {
            _skillNum = SPELL_DIVINE_POWER;
            _skillName = "divine power";
            _minMana = 80;
            _maxMana = 150;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _good = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_CLERIC, 30, 5};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellDivinePower();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellDeathKnell : public DivineSpell
{
    public:
        SpellDeathKnell() : DivineSpell()
        {
            _skillNum = SPELL_DEATH_KNELL;
            _skillName = "death knell";
            _minMana = 30;
            _maxMana = 100;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _evil = true;
            _targets = TAR_CHAR_ROOM;
            PracticeInfo p0 = {CLASS_CLERIC, 25, 1};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellDeathKnell();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellManaShield : public ArcaneSpell
{
    public:
        SpellManaShield() : ArcaneSpell()
        {
            _skillNum = SPELL_MANA_SHIELD;
            _skillName = "mana shield";
            _minMana = 30;
            _maxMana = 60;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_MAGE, 29, 3};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellManaShield();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("ManaShieldAffect", _owner, 
                    target);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setLevel(level);
            ao->addToVictMessage("Your mana shield will now absorb "
                    "a percentage of damage.");
            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SpellEntangle : public ArcaneSpell
{
    public:
        SpellEntangle() : ArcaneSpell()
        {
            _skillNum = SPELL_ENTANGLE;
            _skillName = "entangle";
            _minMana = 30;
            _maxMana = 70;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            PracticeInfo p0 = {CLASS_RANGER, 22, 1};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellEntangle();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellElementalBrand : public ArcaneSpell
{
    public:
        SpellElementalBrand() : ArcaneSpell()
        {
            _skillNum = SPELL_ELEMENTAL_BRAND;
            _skillName = "elemental brand";
            _minMana = 120;
            _maxMana = 160;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_OBJ_EQUIP | TAR_OBJ_ROOM | TAR_OBJ_INV;
            PracticeInfo p0 = {CLASS_RANGER, 31, 1};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellElementalBrand();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellSummonLegion : public DivineSpell
{
    public:
        SpellSummonLegion() : DivineSpell()
        {
            _skillNum = SPELL_SUMMON_LEGION;
            _skillName = "summon legion";
            _minMana = 100;
            _maxMana = 200;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _evil = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_BLACKGUARD, 27, 5};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellSummonLegion();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellAntiMagicShell : public ArcaneSpell
{
    public:
        SpellAntiMagicShell() : ArcaneSpell()
        {
            _skillNum = SPELL_ANTI_MAGIC_SHELL;
            _skillName = "anti-magic shell";
            _minMana = 70;
            _maxMana = 150;
            _manaChange = 4;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_MAGE, 21, 2};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellAntiMagicShell();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();
            AffectObject *ao =
                new AffectObject("AntiMagicShellAffect", _owner, target);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setDuration((level >> 1) MUD_HOURS);
            ca->setLevel(level);
            ao->addToVictMessage("A dark and glittering translucent shell "
                    "appears around you.");
            ao->addToRoomMessage("A dark and glittering translucent shell "
                    "appears around $N");
            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SpellWardingSigil : public ArcaneSpell
{
    public:
        SpellWardingSigil() : ArcaneSpell()
        {
            _skillNum = SPELL_WARDING_SIGIL;
            _skillName = "warding sigil";
            _minMana = 250;
            _maxMana = 300;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_OBJ_ROOM | TAR_OBJ_INV;
            PracticeInfo p0 = {CLASS_MAGE, 17, 1};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellWardingSigil();
        }

        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev, obj))
                return false;

            int level = getPowerLevel();

            ExecutableObject *eo = new ExecutableObject(getOwner());
            eo->setObject(obj);
            eo->setSkill(getSkillNumber());
            ev.push_back_id(eo);

            if (IS_OBJ_TYPE(obj, ITEM_MONEY)) {
                eo->addToCharMessage("Slaaave.... to the traffic light!");
                eo->setSkill(getSkillNumber());
            ev.push_back_id(eo);
                return false;
            }

            // Try to dispel and existing sigil
            if (GET_OBJ_SIGIL_IDNUM(obj)) {
                if (GET_OBJ_SIGIL_IDNUM(obj) != _owner->getIdNum()) {
                    if (level <= GET_OBJ_SIGIL_LEVEL(obj)) {
                        eo->addToCharMessage("You fail to remove the sigil.");
                        return false;
                    }
                }
                GET_OBJ_SIGIL_IDNUM(obj) = 0;
                GET_OBJ_SIGIL_LEVEL(obj) = 0;
                eo->addToCharMessage("You have dispelled the sigil.");
                return true;
            }

            GET_OBJ_SIGIL_IDNUM(obj) = _owner->getIdNum();
            GET_OBJ_SIGIL_LEVEL(obj) = level;

            eo->addToCharMessage("A sigil of warding has been magically "
                    "etched upon $p.");
            eo->addToRoomMessage("A glowing sigil appears upon $p, then fades.");
            return true;
        }
};

class SpellDivineIntervention : public DivineSpell
{
    public:
        SpellDivineIntervention() : DivineSpell()
        {
            _skillNum = SPELL_DIVINE_INTERVENTION;
            _skillName = "divine intervention";
            _minMana = 70;
            _maxMana = 120;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _good = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_CLERIC, 21, 4};
            _practiceInfo.push_back(p0);
            PracticeInfo p1 = {CLASS_PALADIN, 23, 6};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellDivineIntervention();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellSphereOfDesecration : public DivineSpell
{
    public:
        SpellSphereOfDesecration() : DivineSpell()
        {
            _skillNum = SPELL_SPHERE_OF_DESECRATION;
            _skillName = "sphere of desecration";
            _minMana = 70;
            _maxMana = 120;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _evil = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_BLACKGUARD, 23, 6};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellSphereOfDesecration();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellAmnesia : public PsionicSpell
{
    public:
        SpellAmnesia() : PsionicSpell()
        {
            _skillNum = SPELL_AMNESIA;
            _skillName = "amnesia";
            _minMana = 30;
            _maxMana = 70;
            _manaChange = 2;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_NOT_SELF | TAR_UNPLEASANT;
            PracticeInfo p0 = {CLASS_PSIONIC, 25, 1};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellAmnesia();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, target);
            ao->addToCharMessage("A cloud of forgetfulness passes of $N's face.");
            ao->addToVictMessage("A wave of amnesia washes over your mind.");
            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(_skillNum);
            ca->setWearoffToChar("Your memories gradually return.");
            ca->setTarget(target);
            ca->setLevel(level);
            ca->setDuration((MAX(5, level >> 4))MUD_HOURS);
            ca->setApply(APPLY_INT, -(1 + (level >> 4)));

            SkillMap::iterator si = target->learnedSkills->begin();
            for (; si != target->learnedSkills->end(); ++si) {
                GenericSkill *skill = si->second;
                int newLev = (int)(skill->getSkillLevel() * ((level * 0.80) / 100));

                ca->addSkill(skill->getSkillNumber(), -newLev);
            }

            return true;
        }
};

class SpellPsychicFeedback : public PsionicSpell
{
    public:
        SpellPsychicFeedback() : PsionicSpell()
        {
            _skillNum = SPELL_PSYCHIC_FEEDBACK;
            _skillName = "psychic feedback";
            _minMana = 30;
            _maxMana = 75;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_PSIONIC, 32, 3};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellPsychicFeedback();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            int level = getPowerLevel();

            AffectObject *ao = new AffectObject("PsychicFeedbackAffect", _owner, target);
            ao->addToCharMessage("You will not send psychic feedback to anyone who attacks you.");
            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(_skillNum);
            ca->setTarget(target);
            ca->setLevel(level);
            ca->setDuration((level >> 3) + dice(1, 1 + (level >> 3)) MUD_HOURS);

            return true;
        }
};

class SpellIdInsinuation : public PsionicSpell
{
    public:
        SpellIdInsinuation() : PsionicSpell()
        {
            _skillNum = SPELL_ID_INSINUATION;
            _skillName = "id insinuation";
            _minMana = 80;
            _maxMana = 110;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_FIGHT_VICT | TAR_UNPLEASANT;
            PracticeInfo p0 = {CLASS_PSIONIC, 32, 1};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellIdInsinuation();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PsionicSpell::perform(ev, target))
                return false;

            struct Creature *pulv, *ulv = NULL; /* un-lucky-vict */
            int total = 0;

            if (target->numCombatants())
                return false;

            if ((mag_savingthrow(target, MIN(getPowerLevel(), 49), SAVING_PSI) ||
                number(0, target->getLevel()) > number(0, MIN(getPowerLevel(), 49))) &&
                can_see_creature(target, _owner)) {

                StartCombatObject *so = new StartCombatObject(_owner, target);
                so->addToCharMessage("$N attacks you in a rage!!");
                so->addToVictMessage("You feel an intense desire to KILL somone!!");
                so->addToRoomMessage("$N looks around frantically!");
                so->addToRoomMessage("$N attacks $n in a rage!!");
                so->setSkill(_skillNum);

                ev.push_back_id(so);
                return true;
            }

            ulv = NULL;
            CreatureList::iterator it = target->in_room->people.begin();
            for (; it != target->in_room->people.end(); ++it) {
                pulv = *it;
                if (pulv == target || pulv == _owner)
                    continue;
                if (PRF_FLAGGED(pulv, PRF_NOHASSLE))
                    continue;
                // prevent all reputation adjustments as a result
                // of id insinuation
                if (!ok_to_damage(pulv, target))
                    continue;

                if (!ok_to_damage(_owner, target))
                    continue;

                if (!number(0, total))
                    ulv = pulv;
                total++;
            }

            if (!ulv) {
                if (number(0, _owner->checkSkill(SPELL_ID_INSINUATION)) > 50 || 
                        !can_see_creature(target, _owner)) {
                    ExecutableObject *eo = new ExecutableObject(_owner);
                    eo->addToCharMessage("Nothing seems to happen.");
                    ev.push_back_id(eo);
                    return true;
                } 
                else
                    ulv = _owner;
            }

            // At this point, we're going to attack someone...
            StartCombatObject *so = new StartCombatObject(target, ulv);
            so->addToVictMessage("$n attacks you in a rage!!");
            so->addToCharMessage("You feel an intense desire to KILL somone!!");
            so->addToRoomMessage("$n looks around frantically!");
            so->addToRoomMessage("$n attacks $N in a rage!!");
            so->setSkill(_skillNum);

            ev.push_back_id(so);
            return true;
        }
};

class ZenOfTranslocation : public GenericZen
{
    public:
        ZenOfTranslocation() : GenericZen()
        {
            _skillNum = ZEN_TRANSLOCATION;
            _skillName = "zen of translocation";
            _minMana = 20;
            _maxMana = 40;
            _manaChange = 1;
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MONK, 37, 1};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new ZenOfTranslocation();
        }

        bool perform(ExecutableVector &ev) {
            if (!GenericZen::perform(ev))
                return false;

            int level = getPowerLevel();
            string toVictMessage = "You have attained the zen of "
                "translocation.";

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, _owner);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(ZEN_TRANSLOCATION);
            ca->setWearoffToChar("You become seperate from the zen of "
                    "translocation.");
            ca->setTarget(_owner);
            ca->setLevel(level);
            ca->setDuration((ca->getLevel() / 3) MUD_HOURS);

            ao->addToVictMessage(toVictMessage);
            
            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class ZenOfCelerity : public GenericZen
{
    public:
        ZenOfCelerity() : GenericZen()
        {
            _skillNum = ZEN_CELERITY;
            _skillName = "zen of celerity";
            _minMana = 20;
            _maxMana = 40;
            _manaChange = 1;
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_MONK, 25, 2};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new ZenOfCelerity();
        }

        bool perform(ExecutableVector &ev) {
            if (!GenericZen::perform(ev))
                return false;

            int level = getPowerLevel();
            string toVictMessage = "You have reached the zen of celerity.";

            AffectObject *ao = new AffectObject("CreatureAffect", _owner, _owner);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(ZEN_CELERITY);
            ca->setWearoffToChar("You lose the zen of celerity.");
            ca->setTarget(_owner);
            ca->setLevel(level);
            ca->setDuration((ca->getLevel() / 5) MUD_HOURS);
            ca->addApply(APPLY_SPEED, ca->getLevel() / 4);

            ao->addToVictMessage(toVictMessage);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SkillDisguise : public GenericSkill
{
    public:
        SkillDisguise() : GenericSkill()
        {
            _skillNum = SKILL_DISGUISE;
            _skillName = "disguise";
            _minMana = 20;
            _maxMana = 40;
            _manaChange = 1;
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 17, 1};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillDisguise();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillUncannyDodge : public GenericSkill
{
    public:
        SkillUncannyDodge() : GenericSkill()
        {
            _skillNum = SKILL_UNCANNY_DODGE;
            _skillName = "uncanny dodge";
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_THIEF, 22, 2};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillUncannyDodge();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillDeEnergize : public GenericSkill
{
    public:
        SkillDeEnergize() : GenericSkill()
        {
            _skillNum = SKILL_DE_ENERGIZE;
            _skillName = "de-energize";
            _minPosition = POS_STANDING;
            _violent = true;
            PracticeInfo p0 = {CLASS_CYBORG, 22, 1};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillDeEnergize();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillAssimilate : public GenericSkill
{
    public:
        SkillAssimilate() : GenericSkill()
        {
            _skillNum = SKILL_ASSIMILATE;
            _skillName = "assimilate";
            _minPosition = POS_STANDING;
            _violent = true;
            PracticeInfo p0 = {CLASS_CYBORG, 22, 3};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillAssimilate();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillRadionegation : public GenericProgram
{
    public:
        SkillRadionegation() : GenericProgram()
        {
            _skillNum = SKILL_RADIONEGATION;
            _skillName = "radionegation";
            _minMove = 15;
            _maxMove = 15;
            _moveChange = 0;
            _minPosition = POS_STANDING;
            _violent = true;
            PracticeInfo p0 = {CLASS_CYBORG, 17, 1};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillRadionegation();
        }

        bool activate(ExecutableVector &ev) {
            if (!GenericProgram::perform(ev))
                return false;

            int level = getPowerLevel();
            string toVictMessage = "Radionegation device activated.";
            string toRoomMessage = "$n starts humming loudly.";

            // The negative maxmove affect
            AffectObject *ao = new AffectObject("CreatureAffect", _owner, _owner);
            CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
            ca->setSkillNum(SKILL_RADIONEGATION);
            ca->setWearoffToChar("Radionegation device shutting down.");
            ca->setWearoffToRoom("$n stops humming.");
            ca->setTarget(_owner);
            ca->setLevel(level);
            ca->addApply(APPLY_MOVE, -_minMove);
            ca->setIsPermenant();

            ao->addToVictMessage(toVictMessage);

            ao->setSkill(getSkillNumber());
            ev.push_back_id(ao);

            return true;
        }
};

class SpellMaleficViolation : public DivineSpell
{
    public:
        SpellMaleficViolation() : DivineSpell()
        {
            _skillNum = SPELL_MALEFIC_VIOLATION;
            _skillName = "malefic violation";
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _evil = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_CLERIC, 39, 5};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellMaleficViolation();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellRighteousPenetration : public DivineSpell
{
    public:
        SpellRighteousPenetration() : DivineSpell()
        {
            _skillNum = SPELL_RIGHTEOUS_PENETRATION;
            _skillName = "righteous penetration";
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _good = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_CLERIC, 39, 5};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellRighteousPenetration();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillWormhole : public GenericSkill
{
    public:
        SkillWormhole() : GenericSkill()
        {
            _skillNum = SKILL_WORMHOLE;
            _skillName = "wormhole";
            _minMana = 20;
            _maxMana = 50;
            _manaChange = 2;
            _minPosition = POS_DEAD;
            PracticeInfo p0 = {CLASS_PHYSIC, 30, 1};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillWormhole();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellGaussShield : public PhysicsSpell
{
    public:
        SpellGaussShield() : PhysicsSpell()
        {
            _skillNum = SPELL_GAUSS_SHIELD;
            _skillName = "gauss shield";
            _minMana = 70;
            _maxMana = 90;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_PHYSIC, 32, 3};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellGaussShield();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellDimensionalShift : public PhysicsSpell
{
    public:
        SpellDimensionalShift() : PhysicsSpell()
        {
            _skillNum = SPELL_DIMENSIONAL_SHIFT;
            _skillName = "dimensional shift";
            _minMana = 65;
            _maxMana = 100;
            _manaChange = 3;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_PHYSIC, 35, 4};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellDimensionalShift();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!PhysicsSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellUnholyStalker : public DivineSpell
{
    public:
        SpellUnholyStalker() : DivineSpell()
        {
            _skillNum = SPELL_UNHOLY_STALKER;
            _skillName = "unholy stalker";
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _violent = true;
            _evil = true;
            _targets = TAR_CHAR_WORLD;
            PracticeInfo p0 = {CLASS_CLERIC, 25, 3};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellUnholyStalker();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellAnimateDead : public DivineSpell
{
    public:
        SpellAnimateDead() : DivineSpell()
        {
            _skillNum = SPELL_ANIMATE_DEAD;
            _skillName = "animate dead";
            _minMana = 30;
            _maxMana = 60;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _violent = true;
            _evil = true;
            _targets = TAR_OBJ_ROOM;
            PracticeInfo p0 = {CLASS_CLERIC, 24, 2};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellAnimateDead();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellInferno : public DivineSpell
{
    public:
        SpellInferno() : DivineSpell()
        {
            _skillNum = SPELL_INFERNO;
            _skillName = "inferno";
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 7;
            _minPosition = POS_STANDING;
            _violent = true;
            _evil = true;
            _targets = TAR_IGNORE;
            PracticeInfo p0 = {CLASS_CLERIC, 35, 1};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellInferno();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellVampiricRegeneration : public DivineSpell
{
    public:
        SpellVampiricRegeneration() : DivineSpell()
        {
            _skillNum = SPELL_VAMPIRIC_REGENERATION;
            _skillName = "vampiric regeneration";
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _violent = true;
            _evil = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_NOT_SELF;
            PracticeInfo p0 = {CLASS_CLERIC, 37, 4};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellVampiricRegeneration();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellBanishment : public DivineSpell
{
    public:
        SpellBanishment() : DivineSpell()
        {
            _skillNum = SPELL_BANISHMENT;
            _skillName = "banishment";
            _minMana = 50;
            _maxMana = 100;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _violent = true;
            _evil = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_NOT_SELF;
            PracticeInfo p0 = {CLASS_CLERIC, 29, 4};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellBanishment();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!DivineSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillDisciplineOfSteel : public GenericSkill
{
    public:
        SkillDisciplineOfSteel() : GenericSkill()
        {
            _skillNum = SKILL_DISCIPLINE_OF_STEEL;
            _skillName = "discipline of steel";
            _minPosition = POS_DEAD;
            PracticeInfo p1 = {CLASS_BLACKGUARD, 10, 1};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillDisciplineOfSteel();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SkillGreatCleave : public GenericSkill
{
    public:
        SkillGreatCleave() : GenericSkill()
        {
            _skillNum = SKILL_GREAT_CLEAVE;
            _skillName = "great cleave";
            _minPosition = POS_DEAD;
            PracticeInfo p1 = {CLASS_BLACKGUARD, 35, 1};
            _practiceInfo.push_back(p1);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SkillGreatCleave();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!GenericSkill::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellLocustRegeneration : public ArcaneSpell
{
    public:
        SpellLocustRegeneration() : ArcaneSpell()
        {
            _skillNum = SPELL_LOCUST_REGENERATION;
            _skillName = "locust regeneration";
            _minMana = 60;
            _maxMana = 125;
            _manaChange = 5;
            _minPosition = POS_STANDING;
            _violent = true;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_NOT_SELF;
            PracticeInfo p0 = {CLASS_MAGE, 34, 5};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellLocustRegeneration();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellThornSkin : public ArcaneSpell
{
    public:
        SpellThornSkin() : ArcaneSpell()
        {
            _skillNum = SPELL_THORN_SKIN;
            _skillName = "thorn skin";
            _minMana = 80;
            _maxMana = 110;
            _manaChange = 1;
            _minPosition = POS_STANDING;
            _nonCombative = true;
            _violent = false;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_RANGER, 38, 6};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellThornSkin();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};

class SpellSpiritTrack : public ArcaneSpell
{
    public:
        SpellSpiritTrack() : ArcaneSpell()
        {
            _skillNum = SPELL_SPIRIT_TRACK;
            _skillName = "spirit track";
            _minMana = 60;
            _maxMana = 120;
            _manaChange = 10;
            _minPosition = POS_STANDING;
            _violent = false;
            _targets = TAR_CHAR_ROOM | TAR_SELF_ONLY;
            PracticeInfo p0 = {CLASS_RANGER, 38, 1};
            _practiceInfo.push_back(p0);
        }

        GenericSkill *createNewInstance() {
            return (GenericSkill *)new SpellSpiritTrack();
        }

        bool perform(ExecutableVector &ev, Creature *target) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
        bool perform(ExecutableVector &ev, struct obj_data *obj) {
            if (!ArcaneSpell::perform(ev))
                return false;

            send_to_char(_owner, "Not yet implemented!\n");
            return false;
        }
};


class AttackTypeHit : public GenericAttackType
{
    public:
        AttackTypeHit() : GenericAttackType() {
            _skillNum = TYPE_HIT;
            _noList = true;
        }
        
        GenericSkill *createNewInstance() {
            return (GenericSkill *)new AttackTypeHit();
        }
};

class AttackTypeSting : public GenericAttackType
{
    public:
        AttackTypeSting() : GenericAttackType() {
            _skillNum = TYPE_STING;
            _noList = true;
        }
        
        GenericSkill *createNewInstance() {
            return (GenericSkill *)new AttackTypeSting();
        }
};

class AttackTypeWhip : public GenericAttackType
{
    public:
        AttackTypeWhip() : GenericAttackType() {
            _skillNum = TYPE_WHIP;
            _noList = true;
        }
        
        GenericSkill *createNewInstance() {
            return (GenericSkill *)new AttackTypeWhip();
        }
};

class AttackTypeSlash : public GenericAttackType
{
    public:
        AttackTypeSlash() : GenericAttackType() {
            _skillNum = TYPE_SLASH;
            _noList = true;
        }
        
        GenericSkill *createNewInstance() {
            return (GenericSkill *)new AttackTypeSlash();
        }
};

class AttackTypeBite : public GenericAttackType
{
    public:
        AttackTypeBite() : GenericAttackType() {
            _skillNum = TYPE_BITE;
            _noList = true;
        }
        
        GenericSkill *createNewInstance() {
            return (GenericSkill *)new AttackTypeBite();
        }
};

class AttackTypeBludgeon : public GenericAttackType
{
    public:
        AttackTypeBludgeon() : GenericAttackType() {
            _skillNum = TYPE_BLUDGEON;
            _noList = true;
        }
        
        GenericSkill *createNewInstance() {
            return (GenericSkill *)new AttackTypeBludgeon();
        }
};

class AttackTypeCrush : public GenericAttackType
{
    public:
        AttackTypeCrush() : GenericAttackType() {
            _skillNum = TYPE_CRUSH;
            _noList = true;
        }
        
        GenericSkill *createNewInstance() {
            return (GenericSkill *)new AttackTypeCrush();
        }
};

class AttackTypePound : public GenericAttackType
{
    public:
        AttackTypePound() : GenericAttackType() {
            _skillNum = TYPE_POUND;
            _noList = true;
        }
        
        GenericSkill *createNewInstance() {
            return (GenericSkill *)new AttackTypePound();
        }
};

class AttackTypeClaw : public GenericAttackType
{
    public:
        AttackTypeClaw() : GenericAttackType() {
            _skillNum = TYPE_CLAW;
            _noList = true;
        }
        
        GenericSkill *createNewInstance() {
            return (GenericSkill *)new AttackTypeClaw();
        }
};

class AttackTypeMaul : public GenericAttackType
{
    public:
        AttackTypeMaul() : GenericAttackType() {
            _skillNum = TYPE_MAUL;
            _noList = true;
        }
        
        GenericSkill *createNewInstance() {
            return (GenericSkill *)new AttackTypeMaul();
        }
};

class AttackTypeThrash : public GenericAttackType
{
    public:
        AttackTypeThrash() : GenericAttackType() {
            _skillNum = TYPE_THRASH;
            _noList = true;
        }
        
        GenericSkill *createNewInstance() {
            return (GenericSkill *)new AttackTypeThrash();
        }
};

class AttackTypePierce : public GenericAttackType
{
    public:
        AttackTypePierce() : GenericAttackType() {
            _skillNum = TYPE_PIERCE;
            _noList = true;
        }
        
        GenericSkill *createNewInstance() {
            return (GenericSkill *)new AttackTypePierce();
        }
};

class AttackTypeBlast : public GenericAttackType
{
    public:
        AttackTypeBlast() : GenericAttackType() {
            _skillNum = TYPE_BLAST;
            _noList = true;
        }
        
        GenericSkill *createNewInstance() {
            return (GenericSkill *)new AttackTypeBlast();
        }
};

class AttackTypePunch : public GenericAttackType
{
    public:
        AttackTypePunch() : GenericAttackType() {
            _skillNum = TYPE_PUNCH;
            _noList = true;
        }
        
        GenericSkill *createNewInstance() {
            return (GenericSkill *)new AttackTypePunch();
        }
};

class AttackTypeStab : public GenericAttackType
{
    public:
        AttackTypeStab() : GenericAttackType() {
            _skillNum = TYPE_STAB;
            _noList = true;
        }
        
        GenericSkill *createNewInstance() {
            return (GenericSkill *)new AttackTypeStab();
        }
};

class AttackTypeEnergyGun : public GenericAttackType
{
    public:
        AttackTypeEnergyGun() : GenericAttackType() {
            _skillNum = TYPE_ENERGY_GUN;
            _noList = true;
        }
        
        GenericSkill *createNewInstance() {
            return (GenericSkill *)new AttackTypeEnergyGun();
        }
};

class AttackTypeRip : public GenericAttackType
{
    public:
        AttackTypeRip() : GenericAttackType() {
            _skillNum = TYPE_RIP;
            _noList = true;
        }
        
        GenericSkill *createNewInstance() {
            return (GenericSkill *)new AttackTypeRip();
        }
};

class AttackTypeChop : public GenericAttackType
{
    public:
        AttackTypeChop() : GenericAttackType() {
            _skillNum = TYPE_CHOP;
            _noList = true;
        }
        
        GenericSkill *createNewInstance() {
            return (GenericSkill *)new AttackTypeChop();
        }
};

class AttackTypeShoot : public GenericAttackType
{
    public:
        AttackTypeShoot() : GenericAttackType() {
            _skillNum = TYPE_SHOOT;
            _noList = true;
        }
        
        GenericSkill *createNewInstance() {
            return (GenericSkill *)new AttackTypeShoot();
        }
};

class AttackTypeBarehand : public GenericAttackType
{
    public:
        AttackTypeBarehand() : GenericAttackType() {
            _skillNum = TYPE_BAREHAND;
            _noList = true;
        }
        
        GenericSkill *createNewInstance() {
            return (GenericSkill *)new AttackTypeBarehand();
        }
};

void 
register_skills()
{
    GenericSkill::insertSkill(new SpellAblaze());
    GenericSkill::insertSkill(new SkillSnipe());
    GenericSkill::insertSkill(new SkillAdvImplantW());
    GenericSkill::insertSkill(new SongAriaOfAsylum());
    GenericSkill::insertSkill(new SongLamentOfLonging());
    GenericSkill::insertSkill(new SongWallOfSound());
    GenericSkill::insertSkill(new SkillLingeringSong());
    GenericSkill::insertSkill(new SongFortissimo());
    GenericSkill::insertSkill(new SongLichsLyrics());
    GenericSkill::insertSkill(new SongMirrorImageMelody());
    GenericSkill::insertSkill(new SpellFireBreathing());
    GenericSkill::insertSkill(new SpellVestigialRune());
    GenericSkill::insertSkill(new SpellPrismaticSphere());
    GenericSkill::insertSkill(new SpellQuantumRift());
    GenericSkill::insertSkill(new SpellStoneskin());
    GenericSkill::insertSkill(new SpellTelekinesis());
    GenericSkill::insertSkill(new SpellShieldOfRighteousness());
    GenericSkill::insertSkill(new SpellSunRay());
    GenericSkill::insertSkill(new SpellTaint());
    GenericSkill::insertSkill(new SpellBlackmantle());
    GenericSkill::insertSkill(new SpellSanctification());
    GenericSkill::insertSkill(new SpellStigmata());
    GenericSkill::insertSkill(new SpellDivinePower());
    GenericSkill::insertSkill(new SpellDeathKnell());
    GenericSkill::insertSkill(new SpellManaShield());
    GenericSkill::insertSkill(new SpellEntangle());
    GenericSkill::insertSkill(new SpellElementalBrand());
    GenericSkill::insertSkill(new SpellSummonLegion());
    GenericSkill::insertSkill(new SpellAntiMagicShell());
    GenericSkill::insertSkill(new SpellWardingSigil());
    GenericSkill::insertSkill(new SpellDivineIntervention());
    GenericSkill::insertSkill(new SpellDivineIntervention());
    GenericSkill::insertSkill(new SpellSphereOfDesecration());
    GenericSkill::insertSkill(new SpellSphereOfDesecration());
    GenericSkill::insertSkill(new SpellAmnesia());
    GenericSkill::insertSkill(new SpellPsychicFeedback());
    GenericSkill::insertSkill(new SpellIdInsinuation());
    GenericSkill::insertSkill(new ZenOfTranslocation());
    GenericSkill::insertSkill(new ZenOfCelerity());
    GenericSkill::insertSkill(new SkillDisguise());
    GenericSkill::insertSkill(new SkillUncannyDodge());
    GenericSkill::insertSkill(new SkillDeEnergize());
    GenericSkill::insertSkill(new SkillAssimilate());
    GenericSkill::insertSkill(new SkillRadionegation());
    GenericSkill::insertSkill(new SpellMaleficViolation());
    GenericSkill::insertSkill(new SpellRighteousPenetration());
    GenericSkill::insertSkill(new SkillWormhole());
    GenericSkill::insertSkill(new SpellGaussShield());
    GenericSkill::insertSkill(new SpellDimensionalShift());
    GenericSkill::insertSkill(new SpellUnholyStalker());
    GenericSkill::insertSkill(new SpellAnimateDead());
    GenericSkill::insertSkill(new SpellInferno());
    GenericSkill::insertSkill(new SpellVampiricRegeneration());
    GenericSkill::insertSkill(new SpellBanishment());
    GenericSkill::insertSkill(new SkillDisciplineOfSteel());
    GenericSkill::insertSkill(new SkillGreatCleave());
    GenericSkill::insertSkill(new SpellLocustRegeneration());
    GenericSkill::insertSkill(new SpellThornSkin());
    GenericSkill::insertSkill(new SpellSpiritTrack());
    GenericSkill::insertSkill(new SpellAirWalk());
    GenericSkill::insertSkill(new SpellArmor());
    GenericSkill::insertSkill(new SpellAstralSpell());
    GenericSkill::insertSkill(new SpellControlUndead());
    GenericSkill::insertSkill(new SpellTeleport());
    GenericSkill::insertSkill(new SpellLocalTeleport());
    GenericSkill::insertSkill(new SpellBlur());
    GenericSkill::insertSkill(new SpellBless());
    GenericSkill::insertSkill(new SpellDamn());
    GenericSkill::insertSkill(new SpellCalm());
    GenericSkill::insertSkill(new SpellBlindness());
    GenericSkill::insertSkill(new SpellBreatheWater());
    GenericSkill::insertSkill(new SpellBurningHands());
    GenericSkill::insertSkill(new SpellCallLightning());
    GenericSkill::insertSkill(new SpellEnvenom());
    GenericSkill::insertSkill(new SpellCharm());
    GenericSkill::insertSkill(new SpellCharmAnimal());
    GenericSkill::insertSkill(new SpellChillTouch());
    GenericSkill::insertSkill(new SpellClairvoyance());
    GenericSkill::insertSkill(new SpellCallRodent());
    GenericSkill::insertSkill(new SpellCallBird());
    GenericSkill::insertSkill(new SpellCallReptile());
    GenericSkill::insertSkill(new SpellCallBeast());
    GenericSkill::insertSkill(new SpellCallPredator());
    GenericSkill::insertSkill(new SpellClone());
    GenericSkill::insertSkill(new SpellColorSpray());
    GenericSkill::insertSkill(new SpellCommand());
    GenericSkill::insertSkill(new SpellConeOfCold());
    GenericSkill::insertSkill(new SpellConjureElemental());
    GenericSkill::insertSkill(new SpellControlWeather());
    GenericSkill::insertSkill(new SpellCreateFood());
    GenericSkill::insertSkill(new SpellCreateWater());
    GenericSkill::insertSkill(new SpellCureBlind());
    GenericSkill::insertSkill(new SpellCureCritic());
    GenericSkill::insertSkill(new SpellCureLight());
    GenericSkill::insertSkill(new SpellCurse());
    GenericSkill::insertSkill(new SpellDetectAlign());
    GenericSkill::insertSkill(new SpellDetectInvis());
    GenericSkill::insertSkill(new SpellDetectMagic());
    GenericSkill::insertSkill(new SpellDetectPoison());
    GenericSkill::insertSkill(new SpellDetectScrying());
    GenericSkill::insertSkill(new SpellDimensionDoor());
    GenericSkill::insertSkill(new SpellDispelEvil());
    GenericSkill::insertSkill(new SpellDispelGood());
    GenericSkill::insertSkill(new SpellDispelMagic());
    GenericSkill::insertSkill(new SpellDisruption());
    GenericSkill::insertSkill(new SpellDisplacement());
    GenericSkill::insertSkill(new SpellEarthquake());
    GenericSkill::insertSkill(new SpellEnchantArmor());
    GenericSkill::insertSkill(new SpellEnchantWeapon());
    GenericSkill::insertSkill(new SpellEndureCold());
    GenericSkill::insertSkill(new SpellEnergyDrain());
    GenericSkill::insertSkill(new SpellFly());
    GenericSkill::insertSkill(new SpellFlameStrike());
    GenericSkill::insertSkill(new SpellFlameOfFaith());
    GenericSkill::insertSkill(new SpellGoodberry());
    GenericSkill::insertSkill(new SpellGustOfWind());
    GenericSkill::insertSkill(new SpellBarkskin());
    GenericSkill::insertSkill(new SpellIcyBlast());
    GenericSkill::insertSkill(new SpellInvisToUndead());
    GenericSkill::insertSkill(new SpellAnimalKin());
    GenericSkill::insertSkill(new SpellGreaterEnchant());
    GenericSkill::insertSkill(new SpellGroupArmor());
    GenericSkill::insertSkill(new SpellFireball());
    GenericSkill::insertSkill(new SpellFireShield());
    GenericSkill::insertSkill(new SpellGreaterHeal());
    GenericSkill::insertSkill(new SpellGroupHeal());
    GenericSkill::insertSkill(new SpellHarm());
    GenericSkill::insertSkill(new SpellHeal());
    GenericSkill::insertSkill(new SpellHaste());
    GenericSkill::insertSkill(new SpellInfravision());
    GenericSkill::insertSkill(new SpellInvisible());
    GenericSkill::insertSkill(new SpellGlowlight());
    GenericSkill::insertSkill(new SpellKnock());
    GenericSkill::insertSkill(new SpellLightningBolt());
    GenericSkill::insertSkill(new SpellLocateObject());
    GenericSkill::insertSkill(new SpellMagicMissile());
    GenericSkill::insertSkill(new SpellMelfsAcidArrow());
    GenericSkill::insertSkill(new SpellMinorIdentify());
    GenericSkill::insertSkill(new SpellMagicalProt());
    GenericSkill::insertSkill(new SpellMagicalVestment());
    GenericSkill::insertSkill(new SpellMeteorStorm());
    GenericSkill::insertSkill(new SpellChainLightning());
    GenericSkill::insertSkill(new SpellHailstorm());
    GenericSkill::insertSkill(new SpellIceStorm());
    GenericSkill::insertSkill(new SpellPoison());
    GenericSkill::insertSkill(new SpellPray());
    GenericSkill::insertSkill(new SpellPrismaticSpray());
    GenericSkill::insertSkill(new SpellProtectFromDevils());
    GenericSkill::insertSkill(new SpellProtFromEvil());
    GenericSkill::insertSkill(new SpellProtFromGood());
    GenericSkill::insertSkill(new SpellProtFromLightning());
    GenericSkill::insertSkill(new SpellProtFromFire());
    GenericSkill::insertSkill(new SpellRemoveCurse());
    GenericSkill::insertSkill(new SpellRemoveSickness());
    GenericSkill::insertSkill(new SpellRejuvenate());
    GenericSkill::insertSkill(new SpellRefresh());
    GenericSkill::insertSkill(new SpellRegenerate());
    GenericSkill::insertSkill(new SpellRetrieveCorpse());
    GenericSkill::insertSkill(new SpellSanctuary());
    GenericSkill::insertSkill(new SpellShockingGrasp());
    GenericSkill::insertSkill(new SpellShroudObscurement());
    GenericSkill::insertSkill(new SpellSleep());
    GenericSkill::insertSkill(new SpellSlow());
    GenericSkill::insertSkill(new SpellSpiritHammer());
    GenericSkill::insertSkill(new SpellStrength());
    GenericSkill::insertSkill(new SpellSummon());
    GenericSkill::insertSkill(new SpellSword());
    GenericSkill::insertSkill(new SpellSymbolOfPain());
    GenericSkill::insertSkill(new SpellStoneToFlesh());
    GenericSkill::insertSkill(new SpellWordStun());
    GenericSkill::insertSkill(new SpellTrueSeeing());
    GenericSkill::insertSkill(new SpellWordOfRecall());
    GenericSkill::insertSkill(new SpellWordOfIntellect());
    GenericSkill::insertSkill(new SpellPeer());
    GenericSkill::insertSkill(new SpellRemovePoison());
    GenericSkill::insertSkill(new SpellRestoration());
    GenericSkill::insertSkill(new SpellSenseLife());
    GenericSkill::insertSkill(new SpellUndeadProt());
    GenericSkill::insertSkill(new SpellWaterwalk());
    GenericSkill::insertSkill(new SpellIdentify());
    GenericSkill::insertSkill(new SpellWallOfThorns());
    GenericSkill::insertSkill(new SpellWallOfStone());
    GenericSkill::insertSkill(new SpellWallOfIce());
    GenericSkill::insertSkill(new SpellWallOfFire());
    GenericSkill::insertSkill(new SpellWallOfIron());
    GenericSkill::insertSkill(new SpellThornTrap());
    GenericSkill::insertSkill(new SpellFierySheet());
    GenericSkill::insertSkill(new SpellPower());
    GenericSkill::insertSkill(new SpellIntellect());
    GenericSkill::insertSkill(new SpellConfusion());
    GenericSkill::insertSkill(new SpellFear());
    GenericSkill::insertSkill(new SpellSatiation());
    GenericSkill::insertSkill(new SpellQuench());
    GenericSkill::insertSkill(new SpellConfidence());
    GenericSkill::insertSkill(new SpellNopain());
    GenericSkill::insertSkill(new SpellDermalHardening());
    GenericSkill::insertSkill(new SpellWoundClosure());
    GenericSkill::insertSkill(new SpellAntibody());
    GenericSkill::insertSkill(new SpellRetina());
    GenericSkill::insertSkill(new SpellAdrenaline());
    GenericSkill::insertSkill(new SpellBreathingStasis());
    GenericSkill::insertSkill(new SpellVertigo());
    GenericSkill::insertSkill(new SpellMetabolism());
    GenericSkill::insertSkill(new SpellEgoWhip());
    GenericSkill::insertSkill(new SpellPsychicCrush());
    GenericSkill::insertSkill(new SpellRelaxation());
    GenericSkill::insertSkill(new SpellWeakness());
    GenericSkill::insertSkill(new SpellFortressOfWill());
    GenericSkill::insertSkill(new SpellCellRegen());
    GenericSkill::insertSkill(new SpellPsishield());
    GenericSkill::insertSkill(new SpellPsychicSurge());
    GenericSkill::insertSkill(new SpellPsychicConduit());
    GenericSkill::insertSkill(new SpellPsionicShatter());
    GenericSkill::insertSkill(new SpellMelatonicFlood());
    GenericSkill::insertSkill(new SpellMotorSpasm());
    GenericSkill::insertSkill(new SpellPsychicResistance());
    GenericSkill::insertSkill(new SpellMassHysteria());
    GenericSkill::insertSkill(new SpellGroupConfidence());
    GenericSkill::insertSkill(new SpellClumsiness());
    GenericSkill::insertSkill(new SpellEndurance());
    GenericSkill::insertSkill(new SpellNullpsi());
    GenericSkill::insertSkill(new SpellTelepathy());
    GenericSkill::insertSkill(new SpellDistraction());
    GenericSkill::insertSkill(new SkillPsiblast());
    GenericSkill::insertSkill(new SkillPsidrain());
    GenericSkill::insertSkill(new SpellElectrostaticField());
    GenericSkill::insertSkill(new SpellEntropyField());
    GenericSkill::insertSkill(new SpellAcidity());
    GenericSkill::insertSkill(new SpellAttractionField());
    GenericSkill::insertSkill(new SpellNuclearWasteland());
    GenericSkill::insertSkill(new SpellSpacetimeImprint());
    GenericSkill::insertSkill(new SpellSpacetimeRecall());
    GenericSkill::insertSkill(new SpellFluoresce());
    GenericSkill::insertSkill(new SpellGammaRay());
    GenericSkill::insertSkill(new SpellGravityWell());
    GenericSkill::insertSkill(new SpellCapacitanceBoost());
    GenericSkill::insertSkill(new SpellElectricArc());
    GenericSkill::insertSkill(new SpellTemporalCompression());
    GenericSkill::insertSkill(new SpellTemporalDilation());
    GenericSkill::insertSkill(new SpellHalflife());
    GenericSkill::insertSkill(new SpellMicrowave());
    GenericSkill::insertSkill(new SpellOxidize());
    GenericSkill::insertSkill(new SpellRandomCoordinates());
    GenericSkill::insertSkill(new SpellRepulsionField());
    GenericSkill::insertSkill(new SpellVacuumShroud());
    GenericSkill::insertSkill(new SpellAlbedoShield());
    GenericSkill::insertSkill(new SpellTransmittance());
    GenericSkill::insertSkill(new SpellTimeWarp());
    GenericSkill::insertSkill(new SpellRadioimmunity());
    GenericSkill::insertSkill(new SpellDensify());
    GenericSkill::insertSkill(new SpellLatticeHardening());
    GenericSkill::insertSkill(new SpellChemicalStability());
    GenericSkill::insertSkill(new SpellRefraction());
    GenericSkill::insertSkill(new SpellNullify());
    GenericSkill::insertSkill(new SpellAreaStasis());
    GenericSkill::insertSkill(new SpellEmpPulse());
    GenericSkill::insertSkill(new SpellFissionBlast());
    GenericSkill::insertSkill(new SpellDimensionalVoid());
    GenericSkill::insertSkill(new ZenOfHealing());
    GenericSkill::insertSkill(new ZenOfAwareness());
    GenericSkill::insertSkill(new ZenOfOblivity());
    GenericSkill::insertSkill(new ZenOfMotion());
    GenericSkill::insertSkill(new ZenOfDispassion());
    GenericSkill::insertSkill(new SongDriftersDitty());
    GenericSkill::insertSkill(new SongAriaOfArmament());
    GenericSkill::insertSkill(new SongVerseOfVulnerability());
    GenericSkill::insertSkill(new SongMelodyOfMettle());
    GenericSkill::insertSkill(new SongRegalersRhapsody());
    GenericSkill::insertSkill(new SongDefenseDitty());
    GenericSkill::insertSkill(new SongAlronsAria());
    GenericSkill::insertSkill(new SongLustrationMelisma());
    GenericSkill::insertSkill(new SongExposureOverture());
    GenericSkill::insertSkill(new SongVerseOfValor());
    GenericSkill::insertSkill(new SongWhiteNoise());
    GenericSkill::insertSkill(new SongHomeSweetHome());
    GenericSkill::insertSkill(new SongChantOfLight());
    GenericSkill::insertSkill(new SongInsidiousRhythm());
    GenericSkill::insertSkill(new SongEaglesOverture());
    GenericSkill::insertSkill(new SongWeightOfTheWorld());
    GenericSkill::insertSkill(new SongGuihariasGlory());
    GenericSkill::insertSkill(new SongRhapsodyOfRemedy());
    GenericSkill::insertSkill(new SongUnladenSwallowSong());
    GenericSkill::insertSkill(new SongClarifyingHarmonies());
    GenericSkill::insertSkill(new SongPowerOverture());
    GenericSkill::insertSkill(new SongRhythmOfRage());
    GenericSkill::insertSkill(new SongUnravellingDiapason());
    GenericSkill::insertSkill(new SongInstantAudience());
    GenericSkill::insertSkill(new SongWoundingWhispers());
    GenericSkill::insertSkill(new SongRhythmOfAlarm());
    GenericSkill::insertSkill(new SkillTumbling());
    GenericSkill::insertSkill(new SkillScream());
    GenericSkill::insertSkill(new SongSonicDisruption());
    GenericSkill::insertSkill(new SongDirge());
    GenericSkill::insertSkill(new SongMisdirectionMelisma());
    GenericSkill::insertSkill(new SongHymnOfPeace());
    GenericSkill::insertSkill(new SkillVentriloquism());
    GenericSkill::insertSkill(new SpellTidalSpacewarp());
    GenericSkill::insertSkill(new SpellTidalSpacewarp());
    GenericSkill::insertSkill(new SkillCleave());
    GenericSkill::insertSkill(new SkillHamstring());
    GenericSkill::insertSkill(new SkillDrag());
    GenericSkill::insertSkill(new SkillSnatch());
    GenericSkill::insertSkill(new SkillArchery());
    GenericSkill::insertSkill(new SkillBowFletch());
    GenericSkill::insertSkill(new SkillReadScrolls());
    GenericSkill::insertSkill(new SkillUseWands());
    GenericSkill::insertSkill(new SkillBackstab());
    GenericSkill::insertSkill(new SkillCircle());
    GenericSkill::insertSkill(new SkillHide());
    GenericSkill::insertSkill(new SkillKick());
    GenericSkill::insertSkill(new SkillBash());
    GenericSkill::insertSkill(new SkillBreakDoor());
    GenericSkill::insertSkill(new SkillHeadbutt());
    GenericSkill::insertSkill(new SkillHotwire());
    GenericSkill::insertSkill(new SkillGouge());
    GenericSkill::insertSkill(new SkillStun());
    GenericSkill::insertSkill(new SkillFeign());
    GenericSkill::insertSkill(new SkillConceal());
    GenericSkill::insertSkill(new SkillPlant());
    GenericSkill::insertSkill(new SkillPickLock());
    GenericSkill::insertSkill(new SkillRescue());
    GenericSkill::insertSkill(new SkillSneak());
    GenericSkill::insertSkill(new SkillSteal());
    GenericSkill::insertSkill(new SkillTrack());
    GenericSkill::insertSkill(new SkillPunch());
    GenericSkill::insertSkill(new SkillPiledrive());
    GenericSkill::insertSkill(new SkillShieldMastery());
    GenericSkill::insertSkill(new SkillElbow());
    GenericSkill::insertSkill(new SkillKnee());
    GenericSkill::insertSkill(new SkillAutopsy());
    GenericSkill::insertSkill(new SkillBerserk());
    GenericSkill::insertSkill(new SkillBattleCry());
    GenericSkill::insertSkill(new SkillKia());
    GenericSkill::insertSkill(new SkillCryFromBeyond());
    GenericSkill::insertSkill(new SkillStomp());
    GenericSkill::insertSkill(new SkillBodyslam());
    GenericSkill::insertSkill(new SkillChoke());
    GenericSkill::insertSkill(new SkillClothesline());
    GenericSkill::insertSkill(new SkillIntimidate());
    GenericSkill::insertSkill(new SkillSpeedHealing());
    GenericSkill::insertSkill(new SkillStalk());
    GenericSkill::insertSkill(new SkillHearing());
    GenericSkill::insertSkill(new SkillBearhug());
    GenericSkill::insertSkill(new SkillBite());
    GenericSkill::insertSkill(new SkillDblAttack());
    GenericSkill::insertSkill(new SkillNightVision());
    GenericSkill::insertSkill(new SkillTurn());
    GenericSkill::insertSkill(new SkillAnalyze());
    GenericSkill::insertSkill(new SkillEvaluate());
    GenericSkill::insertSkill(new SkillCureDisease());
    GenericSkill::insertSkill(new SkillHolyTouch());
    GenericSkill::insertSkill(new SkillBandage());
    GenericSkill::insertSkill(new SkillFirstaid());
    GenericSkill::insertSkill(new SkillMedic());
    GenericSkill::insertSkill(new SkillLeatherworking());
    GenericSkill::insertSkill(new SkillMetalworking());
    GenericSkill::insertSkill(new SkillConsider());
    GenericSkill::insertSkill(new SkillGlance());
    GenericSkill::insertSkill(new SkillShoot());
    GenericSkill::insertSkill(new SkillBehead());
    GenericSkill::insertSkill(new SkillEmpower());
    GenericSkill::insertSkill(new SkillDisarm());
    GenericSkill::insertSkill(new SkillSpinkick());
    GenericSkill::insertSkill(new SkillRoundhouse());
    GenericSkill::insertSkill(new SkillSidekick());
    GenericSkill::insertSkill(new SkillSpinfist());
    GenericSkill::insertSkill(new SkillJab());
    GenericSkill::insertSkill(new SkillHook());
    GenericSkill::insertSkill(new SkillSweepkick());
    GenericSkill::insertSkill(new SkillTrip());
    GenericSkill::insertSkill(new SkillUppercut());
    GenericSkill::insertSkill(new SkillGroinkick());
    GenericSkill::insertSkill(new SkillClaw());
    GenericSkill::insertSkill(new SkillRabbitpunch());
    GenericSkill::insertSkill(new SkillImpale());
    GenericSkill::insertSkill(new SkillPeleKick());
    GenericSkill::insertSkill(new SkillLungePunch());
    GenericSkill::insertSkill(new SkillTornadoKick());
    GenericSkill::insertSkill(new SkillTripleAttack());
    GenericSkill::insertSkill(new SkillCounterAttack());
    GenericSkill::insertSkill(new SkillSwimming());
    GenericSkill::insertSkill(new SkillThrowing());
    GenericSkill::insertSkill(new SkillRiding());
    GenericSkill::insertSkill(new SkillAppraise());
    GenericSkill::insertSkill(new SkillPipemaking());
    GenericSkill::insertSkill(new SkillSecondWeapon());
    GenericSkill::insertSkill(new SkillScanning());
    GenericSkill::insertSkill(new SkillRetreat());
    GenericSkill::insertSkill(new SkillGunsmithing());
    GenericSkill::insertSkill(new SkillPistolwhip());
    GenericSkill::insertSkill(new SkillCrossface());
    GenericSkill::insertSkill(new SkillWrench());
    GenericSkill::insertSkill(new SkillElusion());
    GenericSkill::insertSkill(new SkillInfiltrate());
    GenericSkill::insertSkill(new SkillShoulderThrow());
    GenericSkill::insertSkill(new SkillGarotte());
    GenericSkill::insertSkill(new SkillDemolitions());
    GenericSkill::insertSkill(new SkillReconfigure());
    GenericSkill::insertSkill(new SkillReboot());
    GenericSkill::insertSkill(new SkillMotionSensor());
    GenericSkill::insertSkill(new SkillStasis());
    GenericSkill::insertSkill(new SkillEnergyField());
    GenericSkill::insertSkill(new SkillReflexBoost());
    GenericSkill::insertSkill(new SkillImplantW());
    GenericSkill::insertSkill(new SkillOffensivePos());
    GenericSkill::insertSkill(new SkillDefensivePos());
    GenericSkill::insertSkill(new SkillMeleeCombatTactics());
    GenericSkill::insertSkill(new SkillNeuralBridging());
    GenericSkill::insertSkill(new SkillNaniteReconstruction());
    GenericSkill::insertSkill(new SkillAdrenalMaximizer());
    GenericSkill::insertSkill(new SkillPowerBoost());
    GenericSkill::insertSkill(new SkillFastboot());
    GenericSkill::insertSkill(new SkillSelfDestruct());
    GenericSkill::insertSkill(new SkillBioscan());
    GenericSkill::insertSkill(new SkillDischarge());
    GenericSkill::insertSkill(new SkillSelfrepair());
    GenericSkill::insertSkill(new SkillCyborepair());
    GenericSkill::insertSkill(new SkillOverhaul());
    GenericSkill::insertSkill(new SkillDamageControl());
    GenericSkill::insertSkill(new SkillElectronics());
    GenericSkill::insertSkill(new SkillHacking());
    GenericSkill::insertSkill(new SkillCyberscan());
    GenericSkill::insertSkill(new SkillCyboSurgery());
    GenericSkill::insertSkill(new SkillHyperscanning());
    GenericSkill::insertSkill(new SkillOverdrain());
    GenericSkill::insertSkill(new SkillEnergyWeapons());
    GenericSkill::insertSkill(new SkillProjWeapons());
    GenericSkill::insertSkill(new SkillSpeedLoading());
    GenericSkill::insertSkill(new SkillPalmStrike());
    GenericSkill::insertSkill(new SkillThroatStrike());
    GenericSkill::insertSkill(new SkillWhirlwind());
    GenericSkill::insertSkill(new SkillHipToss());
    GenericSkill::insertSkill(new SkillCombo());
    GenericSkill::insertSkill(new SkillDeathTouch());
    GenericSkill::insertSkill(new SkillCraneKick());
    GenericSkill::insertSkill(new SkillRidgehand());
    GenericSkill::insertSkill(new SkillScissorKick());
    GenericSkill::insertSkill(new SkillPinchAlpha());
    GenericSkill::insertSkill(new SkillPinchBeta());
    GenericSkill::insertSkill(new SkillPinchGamma());
    GenericSkill::insertSkill(new SkillPinchDelta());
    GenericSkill::insertSkill(new SkillPinchEpsilon());
    GenericSkill::insertSkill(new SkillPinchOmega());
    GenericSkill::insertSkill(new SkillPinchZeta());
    GenericSkill::insertSkill(new SkillMeditate());
    GenericSkill::insertSkill(new SkillKata());
    GenericSkill::insertSkill(new SkillEvasion());
    GenericSkill::insertSkill(new SkillFlying());
    GenericSkill::insertSkill(new SkillSummon());
    GenericSkill::insertSkill(new SkillFeed());
    GenericSkill::insertSkill(new SkillBeguile());
    GenericSkill::insertSkill(new SkillDrain());
    GenericSkill::insertSkill(new SkillCreateVampire());
    GenericSkill::insertSkill(new SkillControlUndead());
    GenericSkill::insertSkill(new SkillTerrorize());
    GenericSkill::insertSkill(new SkillLecture());
    GenericSkill::insertSkill(new SkillEnergyConversion());
    GenericSkill::insertSkill(new SpellHellFire());
    GenericSkill::insertSkill(new SpellHellFrost());
    GenericSkill::insertSkill(new SpellHellFireStorm());
    GenericSkill::insertSkill(new SpellHellFrostStorm());
    GenericSkill::insertSkill(new SpellTrogStench());
    GenericSkill::insertSkill(new SpellFrostBreath());
    GenericSkill::insertSkill(new SpellFireBreath());
    GenericSkill::insertSkill(new SpellGasBreath());
    GenericSkill::insertSkill(new SpellAcidBreath());
    GenericSkill::insertSkill(new SpellLightningBreath());
    GenericSkill::insertSkill(new SpellManaRestore());
    GenericSkill::insertSkill(new SpellPetrify());
    GenericSkill::insertSkill(new SpellSickness());
    GenericSkill::insertSkill(new SpellEssenceOfEvil());
    GenericSkill::insertSkill(new SpellEssenceOfGood());
    GenericSkill::insertSkill(new SpellShadowBreath());
    GenericSkill::insertSkill(new SpellSteamBreath());
    GenericSkill::insertSkill(new SpellQuadDamage());
    GenericSkill::insertSkill(new SpellZhengisFistOfAnnihilation());
    GenericSkill::insertSkill(new AttackTypeHit());
    GenericSkill::insertSkill(new AttackTypeSting());
    GenericSkill::insertSkill(new AttackTypeWhip());
    GenericSkill::insertSkill(new AttackTypeSlash());
    GenericSkill::insertSkill(new AttackTypeBite());
    GenericSkill::insertSkill(new AttackTypeBludgeon());
    GenericSkill::insertSkill(new AttackTypeCrush());
    GenericSkill::insertSkill(new AttackTypePound());
    GenericSkill::insertSkill(new AttackTypeClaw());
    GenericSkill::insertSkill(new AttackTypeMaul());
    GenericSkill::insertSkill(new AttackTypeThrash());
    GenericSkill::insertSkill(new AttackTypePierce());
    GenericSkill::insertSkill(new AttackTypeBlast());
    GenericSkill::insertSkill(new AttackTypePunch());
    GenericSkill::insertSkill(new AttackTypeStab());
    GenericSkill::insertSkill(new AttackTypeEnergyGun());
    GenericSkill::insertSkill(new AttackTypeRip());
    GenericSkill::insertSkill(new AttackTypeChop());
    GenericSkill::insertSkill(new AttackTypeShoot());
    GenericSkill::insertSkill(new AttackTypeBarehand());
}

inline const char *
spell_to_str(int spell)
{
    return GenericSkill::getNameByNumber(spell).c_str();
}

int
str_to_spell(const char *spell)
{
    return GenericSkill::getNumberByName(spell);
}
