#include "generic_affect.h"
#include "executable_object.h"
#include "creature.h"
#include "utils.h"
#include "spells.h"
#include "comm.h"
#include "unique_id.h"
#include "tmpstr.h"
#include "screen.h"
#include "custom_affects.h"
#include "handler.h"

// ********************************************************************
//
// Generic Affect
//
// ********************************************************************

// Constructor & destructor
GenericAffect::GenericAffect()
{
    _skillNum = 0;
    _duration = 0;
    _level = 0;
    _accumDuration = false;
    _accumAffect = false;
    _averageDuration = false;
    _averageAffect = false;
    _isPermenant = false;
    _ownerID = 0;
    _ID = UniqueID::getNextID();
    _active = false;
    _updateInterval = 0;
}

GenericAffect::~GenericAffect()
{
}

void
GenericAffect::modifyEO(ExecutableObject *eo) {
    //by default we do nothing, override where appropriate
    return;
}

GenericAffect*
GenericAffect::createNewInstance() {
    return NULL;
}

//This is how we make new objects of the appropriate type without
//knowing the right type in advance
GenericAffect *
GenericAffect::constructAffect(string affectName) {
    return (*_getAffectMap())[affectName]->createNewInstance();
}

string
GenericAffect::getClassName()
{
    const char *name = typeid(*this).name();

    while (isdigit(*name))
        name++;

    return string(name);
}

void 
GenericAffect::insertAffect(GenericAffect *affect) {
    (*_getAffectMap())[affect->getClassName()] = affect;
}

// ********************************************************************
//
// Creature Affect
//
// ********************************************************************

CreatureAffect::CreatureAffect() : GenericAffect(), _bitvector(3, 0)
{
    _skill.clear();
    _apply.clear();
    _position = -1;
    _ownerID = 0;
    _wearoffToChar.clear();
    _wearoffToRoom.clear();
}

CreatureAffect::CreatureAffect(Creature *owner) : GenericAffect(), 
                                                  _bitvector(3, 0)
{
    _skill.clear();
    _apply.clear();
    _position = -1;
    _ownerID = owner->getIdNum();
    _wearoffToChar.clear();
    _wearoffToRoom.clear();
}

CreatureAffect::CreatureAffect(CreatureAffect &c) : _bitvector(c._bitvector),
                                                    _skill(c._skill),
                                                    _apply(c._apply)
{
    _skillNum = c.getSkillNum();
    _duration = c.getDuration();
    _level = c.getLevel();
    _position = c.getPosition();
    _accumDuration = c.getAccumDuration();
    _accumAffect = c.getAccumAffect();
    _averageDuration = c.getAverageDuration();
    _averageAffect = c.getAverageAffect();
    _isPermenant = c.getIsPermenant();
    _ownerID = c.getOwner();
    _wearoffToChar = c.getWearoffToChar();
    _wearoffToRoom = c.getWearoffToRoom();
    _target = c.getTarget();
}

CreatureAffect::~CreatureAffect() {
}

GenericAffect*
CreatureAffect::createNewInstance() {
    return (GenericAffect *)new CreatureAffect();
}

void
CreatureAffect::addApply(short location, short modifier) {
    locModType app;

    app.location = location;
    app.modifier = modifier;

    vector<locModType>::iterator vi = _apply.begin();
    for (; vi != _apply.end(); ++vi) {
        if (vi->location == location) {
            if (_accumAffect)
                vi->modifier += modifier;
            if (_averageAffect)
                vi->modifier = (vi->modifier + modifier) / 2;
            return;
        }
    }

    _apply.push_back(app);
}

void
CreatureAffect::setApply(short location, short modifier) {
    locModType app;

    app.location = location;
    app.modifier = modifier;

    vector<locModType>::iterator vi = _apply.begin();
    for (; vi != _apply.end(); ++vi) {
        if (vi->location == location) {
            vi->modifier = modifier;
            return;
        }
    }

    _apply.push_back(app);
}

short
CreatureAffect::getApplyMod(short location) {
    vector<locModType>::iterator vi = _apply.begin();

    for (; vi != _apply.end(); ++vi) {
        if (vi->location == location)
            return vi->modifier;
    }

    return 0;
}

void
CreatureAffect::addSkill(short location, short modifier) {
    locModType app;

    app.location = location;
    app.modifier = modifier;

    vector<locModType>::iterator vi = _skill.begin();
    for (; vi != _skill.end(); ++vi) {
        if (vi->location == location) {
            if (_accumAffect)
                vi->modifier += modifier;
            if (_averageAffect)
                vi->modifier = (vi->modifier + modifier) / 2;
            return;
        }
    }

    _skill.push_back(app);
}

short
CreatureAffect::getSkillMod(short location) {
    vector<locModType>::iterator vi = _skill.begin();

    for (; vi != _skill.end(); ++vi) {
        if (vi->location == location)
            return vi->modifier;
    }

    return 0;
}

bool
CreatureAffect::canJoin() {
   /*
    * If this is a mob that has this affect set in its mob file, do not
    * perform the affect.  This prevents people from un-sancting mobs
    * by sancting them and waiting for it to fade, for example.
    */

    // I don't think we need this anymore.  I'm commenting it for now, just in case
    /*if (IS_NPC(_target)) {
        if (_bitvector[0]) {
            if (IS_AFFECTED(_target, _bitvector[0]) &&
                !_target->affectedBy(_skillNum)) {
                return false;
            }
        } else if (_bitvector[1]) {
            if (IS_AFFECTED_2(_target, _bitvector[1]) &&
                !_target->affectedBy(_skillNum)) {
                return false;
            }
        } else if (_bitvector[2]) {
            if (IS_AFFECTED_3(_target, _bitvector[2]) &&
                !_target->affectedBy(_skillNum)) {
               return false;
            }
        }
    }*/
    
    /* If the victim is already affected by this spell, and the spell does
    * not have an accumulative effect, then fail the spell.
    */
    CreatureAffect *aff = (CreatureAffect *)_target->affectedBy(_skillNum);
    if ((aff && *aff == *this) && 
        !(_accumDuration || _accumAffect)) {
        return false;
    }
    return true;
}

bool 
CreatureAffect::affectJoin() {
    if (!_target) {
        errlog("No _target in CreatureAffect::affectJoin()");
        return false;
    }
    
    if (!canJoin())
        return false;

    CreatureAffectList::iterator li = _target->getAffectList()->begin();
    for (; li != _target->getAffectList()->end(); ++li) {
        if (*(*li) == *this) {
            // We found something that matched, let's turn it off
            (*li)->affectModify(false);
            if (_accumDuration)
                (*li)->setDuration(_duration + (*li)->getDuration());
            else if (_averageDuration)
                (*li)->setDuration(((*li)->getDuration() + _duration) / 2);

            vector<locModType>::iterator vi = _apply.begin();
            for (; vi != _apply.end(); ++vi) {
                (*li)->addApply(vi->location, vi->modifier);

                // Impose some caps
                if ((*li)->getDuration() > 666)
                    (*li)->setDuration(666);

                if ((*li)->getApplyMod(vi->location) > 666)
                    (*li)->setApply(vi->location, 666);
                if ((*li)->getApplyMod(vi->location) < -666)
                    (*li)->setApply(vi->location, -666);
            }
            
            // After modifying the affect appropriately we'll turn it
            // back on and total the affects on the target
            (*li)->affectModify(true);
            _target->affectTotal();
            return true;
        }
    }

    affectModify(true);
    _target->getAffectList()->add_back(this);
    _target->affectTotal();

    return true;
}

void CreatureAffect::affectRemove() {
    if (!this) return;
    
    if (!_target) {
        errlog("No _target in CreatureAffect::affectRemove()");
        return;
    }

    affectModify(false);
    CreatureAffectList *list = getTarget()->getAffectList();
    CreatureAffectList::iterator li;
    li = find(list->begin(), list->end(), this);
    if (li == list->end()) {
        errlog("Failed to get iterator to *this in "
               "CreatureAffect::affectRemove()!");
        return;
    }
    getTarget()->getAffectList()->remove(li);

    _target->affectTotal();

    delete this;
}

void
CreatureAffect::affectModify(bool add) {
    if (!_target) {
        errlog("No _target in CreatureAffect::affectApply()");
        return;
    }

    // If I've thought this through properly, this should
    // prevent us from accidentally adding or removing the
    // same affect multiple times.
    if (_active == add)
        return;
    else
        _active = add;
    
    if (add) {
        if (getBitvector(0)) {
            SET_BIT(AFF_FLAGS(_target), _bitvector[0]);
        }
        else if (getBitvector(1)) {
            SET_BIT(AFF2_FLAGS(_target), _bitvector[1]);
        }
        else if (getBitvector(2)) {
            SET_BIT(AFF3_FLAGS(_target), _bitvector[2]);
        }
    }
    else {
        if (getBitvector(0)) {
            REMOVE_BIT(AFF_FLAGS(_target), _bitvector[0]);
        }
        else if (getBitvector(1)) {
            REMOVE_BIT(AFF2_FLAGS(_target), _bitvector[1]);
        }
        else if (getBitvector(2)) {
            REMOVE_BIT(AFF3_FLAGS(_target), _bitvector[2]);
        }
    }

    if (_position > -1)
        _target->setPosition(_position);

    vector<locModType>::iterator vi;

    for (vi = _skill.begin(); vi != _skill.end(); vi++) {
        int mod = vi->modifier;
        if (!add)
            mod = -mod;
        if (vi->location < MAX_SKILLS) {
            GenericSkill *sp = _target->getSkill(vi->location);
            if (sp)
                sp->setSkillLevel(sp->getSkillLevel() + mod);
        }
    }

    for (vi = _apply.begin(); vi != _apply.end(); ++vi) {
        int mod = vi->modifier;
        int newLevel = 0;
        if (!add)
            mod = -mod;

        switch (vi->location) {
            // We're not going to do skill applies here anymore.  We're going
            // to handle them via the _skillLocation variable.  See above.  We
            // have to figure out how to convert the existing applies to a 
            // useful system though.
            case APPLY_SNEAK:
            case APPLY_HIDE:
            case APPLY_BACKSTAB:
            case APPLY_PICK_LOCK:
            case APPLY_PUNCH:
            case APPLY_SHOOT:
            case APPLY_KICK:
            case APPLY_TRACK:
            case APPLY_IMPALE:
            case APPLY_BEHEAD:
            case APPLY_THROWING:
            case APPLY_RIDING:
            case APPLY_TURN:
            case APPLY_CASTER:
            case APPLY_WEAPONSPEED:
            case APPLY_DISGUISE:
            case APPLY_NONE:
                break;
            case APPLY_STR:
                GET_STR(_target) += mod;
                GET_STR(_target) += GET_ADD(_target) / 10;
                GET_ADD(_target) = 0;
                break;
            case APPLY_LEVEL:
                newLevel = _target->getLevel() + mod;
                if (newLevel < 1) {
                    vi->modifier -= (newLevel - 1);
                    newLevel = 1;
                }
                if (newLevel >= LVL_GRIMP)
                    newLevel = LVL_GRIMP - 1;
                _target->setLevel(newLevel);
                break;
            case APPLY_DEX:
                GET_DEX(_target) += mod;
                break;
            case APPLY_INT:
                GET_INT(_target) += mod;
                break;
            case APPLY_WIS:
                GET_WIS(_target) += mod;
                break;
            case APPLY_CON:
                GET_CON(_target) += mod;
                break;
            case APPLY_CHA:
                GET_CHA(_target) += mod;
                break;
            case APPLY_AGE:
                _target->player.age_adjust += mod;
                break;
            case APPLY_CHAR_WEIGHT:
                GET_WEIGHT(_target) += mod;
                break;
            case APPLY_CHAR_HEIGHT:
                GET_HEIGHT(_target) += mod;
                break;
            case APPLY_MANA:
                GET_MAX_MANA(_target) += mod;
                if (GET_MANA(_target) > GET_MAX_MANA(_target))
                    GET_MANA(_target) = GET_MAX_MANA(_target);
                break;
            case APPLY_HIT:
                GET_MAX_HIT(_target) += mod;
                if (GET_HIT(_target) > GET_MAX_HIT(_target))
                    GET_HIT(_target) = GET_MAX_HIT(_target);
                break;
            case APPLY_MOVE:
                GET_MAX_MOVE(_target) += mod;
                if (GET_MOVE(_target) > GET_MAX_MOVE(_target))
                    GET_MOVE(_target) = GET_MAX_MOVE(_target);
                break;
            case APPLY_AC:
                GET_AC(_target) += mod;
                break;
            case APPLY_HITROLL:
                if (GET_HITROLL(_target) + mod > 125)
                    GET_HITROLL(_target) = MIN(125, GET_HITROLL(_target) + mod);
                else if (GET_HITROLL(_target) + mod < -125)
                    GET_HITROLL(_target) = MAX(125, GET_HITROLL(_target) + mod);
                else
                    GET_HITROLL(_target) += mod;
                break;
            case APPLY_DAMROLL:
                if (GET_DAMROLL(_target) + mod > 125)
                    GET_DAMROLL(_target) = MIN(125, GET_DAMROLL(_target) + mod);
                else if (GET_DAMROLL(_target) + mod < -125)
                    GET_DAMROLL(_target) = MAX(125, GET_DAMROLL(_target) + mod);
                else
                    GET_DAMROLL(_target) += mod;
                break;
            case APPLY_SAVING_PARA:
                GET_SAVE(_target, SAVING_PARA) += mod;
                break;
            case APPLY_SAVING_ROD:
                GET_SAVE(_target, SAVING_ROD) += mod;
                break;
            case APPLY_SAVING_PETRI:
                GET_SAVE(_target, SAVING_PETRI) += mod;
                break;
            case APPLY_SAVING_BREATH:
                GET_SAVE(_target, SAVING_BREATH) += mod;
                break;
            case APPLY_SAVING_SPELL:
                GET_SAVE(_target, SAVING_SPELL) += mod;
                break;
            case APPLY_SAVING_CHEM:
                GET_SAVE(_target, SAVING_CHEM) += mod;
                break;
            case APPLY_SAVING_PSI:
                GET_SAVE(_target, SAVING_PSI) += mod;
                break;
            case APPLY_SAVING_PHY:
                GET_SAVE(_target, SAVING_PHY) += mod;
                break;
            case APPLY_RACE:
                if (mod >= 0 && mod < NUM_RACES)
                    GET_RACE(_target) = mod;
                break;
            case APPLY_SEX:
                if (mod == 0 || mod == 1 || mod == 2)
                    GET_SEX(_target) = mod;
                break;
            case APPLY_ALIGN:
                GET_ALIGNMENT(_target) = 
                    MAX(-1000, MIN(1000, GET_ALIGNMENT(_target) + mod));
                break;
            case APPLY_NOTHIRST:
                if (IS_NPC(_target))
                    break;
                if (GET_COND(_target, THIRST) != -1) {
                    if (mod > 0)
                        GET_COND(_target, THIRST) = -2;
                    else
                        GET_COND(_target, THIRST) = 0;
                }
                break;
            case APPLY_NOHUNGER:
                if (IS_NPC(_target))
                    break;
                if (GET_COND(_target, FULL) != -1) {
                    if (mod > 0)
                        GET_COND(_target, FULL) = -2;
                    else
                        GET_COND(_target, FULL) = 0;
                }
                break;
            case APPLY_NODRUNK:
                if (IS_NPC(_target))
                    break;
                    if (mod > 0)
                        GET_COND(_target, DRUNK) = -1;
                    else
                        GET_COND(_target, DRUNK) = 0;
                break;
            case APPLY_SPEED:
                _target->setSpeed(_target->getSpeed() + mod);
                break;
            default:
                errlog("Unknown apply adjust attempt on %20s %3d + %3d "
                       "in CreatureAffect::affectModify(). add = %s",
                       _target->getName(), vi->location, mod, 
                       add ? "true" : false);
                break;
        }
    }
    
}

void
CreatureAffect::saveToXML(FILE *ouf) {
    vector<locModType>::iterator vi;
    vector<long>::iterator bvi;

    fprintf(ouf, "<affect skillNum=\"%d\" "
                 "name=\"%s\" "
                 "duration=\"%d\" "
                 "level=\"%d\" "
                 "accumDuration=\"%d\" "
                 "accumAffect=\"%d\" "
                 "averageDuration=\"%d\" "
                 "averageAffect=\"%d\" "
                 "isPermenant=\"%d\" "
                 "owner=\"%ld\" "
                 "id=\"%ld\" "
                 "wearoffToChar=\"%s\" "
                 "wearoffToRoom=\"%s\" ",
                 _skillNum, getClassName().c_str(), _duration, _level, 
                 _accumDuration, _accumAffect, _averageDuration, 
                 _averageAffect, _isPermenant, _ownerID, _ID,  
                 _wearoffToChar.c_str(), _wearoffToRoom.c_str());

    if (_apply.size()) {
        fprintf(ouf, "apply=\"");
        for (vi = _apply.begin(); vi != _apply.end(); ++vi) {
            fprintf(ouf, "%d/%d ", vi->location, vi->modifier);
        }
        fprintf(ouf, "\" ");
    }

    if (_skill.size()) {
        fprintf(ouf, "skill=\"");
        for (vi = _skill.begin(); vi != _skill.end(); ++vi) {
            fprintf(ouf, "%d/%d ", vi->location, vi->modifier);
        }
        fprintf(ouf, "\" ");
    }

    if (_bitvector.size()) {
        int counter = 0;
        fprintf(ouf, "bitvector=\"");
        for (bvi = _bitvector.begin(); bvi != _bitvector.end(); ++bvi) {
            if (*bvi != 0)
                fprintf(ouf, "%d/%ld ", counter, *bvi);
            counter++;
        }
        fprintf(ouf, "\" ");
    }

    fprintf(ouf, "/>\n");
}

void
CreatureAffect::loadFromXML(xmlNodePtr node)
{
    char *value, *arg;

    setSkillNum(xmlGetIntProp(node, "skillNum"));
    setDuration(xmlGetIntProp(node, "duration"));
    setLevel((short)xmlGetIntProp(node, "level"));
    if (xmlGetIntProp(node, "accumDuration"))
        setAccumDuration(true);
    else
        setAccumDuration(false);

    if (xmlGetIntProp(node, "accumAffect"))
        setAccumAffect(true);
    else
        setAccumAffect(false);

    if (xmlGetIntProp(node, "averageDuration"))
        setAverageDuration(true);
    else
        setAverageDuration(false);

    if (xmlGetIntProp(node, "averageAffect"))
        setAverageAffect(true);
    else
        setAverageAffect(false);

    if (xmlGetIntProp(node, "isPermenant"))
        setIsPermenant(true);
    else
        setIsPermenant(false);

    setOwner(xmlGetIntProp(node, "owner"));
    setID(xmlGetIntProp(node, "id"));

    if ((value = xmlGetProp(node, "apply"))) {
        arg = tmp_getword(&value);
        while (*arg) {
            char *loc, *mod;
            arg = tmp_gsub(arg, "/", " ");
            loc = tmp_getword(&arg);
            mod = tmp_getword(&arg);

            if (!*loc || !*mod) {
                errlog("loc or mod has no data while loading applies in "
                        "CreatureAffect::loadFromXML()!");
            }
            else
                addApply(atoi(loc), atoi(mod));

            arg = tmp_getword(&value);
        }
    }

    if ((value = xmlGetProp(node, "skill"))) {
        arg = tmp_getword(&value);
        while (*arg) {
            char *loc, *mod;
            arg = tmp_gsub(arg, "/", " ");
            loc = tmp_getword(&arg);
            mod = tmp_getword(&arg);

            if (!*loc || !*mod) {
                errlog("loc or mod has no data while loading skills in "
                        "CreatureAffect::loadfromxml()!");
            }
            else {
                //addskill(loc, mod);
            }

            arg = tmp_getword(&value);
        }
    }

    if ((value = xmlGetProp(node,"bitvector"))) {
        arg = tmp_getword(&value);
        while (*arg) {
            char *loc, *mod;
            arg = tmp_gsub(arg, "/", " ");
            loc = tmp_getword(&arg);
            mod = tmp_getword(&arg);

            if (!*loc || !*mod) {
                errlog("loc or mod has no data while loading bitvectors in "
                        "CreatureAffect::loadFromXML()!");
            }
            else
                setBitvector(atoi(loc), atol(mod));

            arg = tmp_getword(&value);
        }
    }

    value = xmlGetProp(node, "wearoffToChar");
    if (value)
        setWearoffToChar(value);

    value = xmlGetProp(node, "wearoffToRoom");
    if (value)
        setWearoffToRoom(value);
}

void
CreatureAffect::handleTick()
{
    if (!getTarget()->getAffectList()) {
        errlog("getVictim()->getAffectList() returned NULL in "
               "CreatureAffect::handleTick()!");
        return;
    }

    if (_isPermenant)
        return;

    if (_duration >= 1) {
        _duration--;
        return;
    }
    else {
        if (PLR_FLAGGED(getTarget(), PLR_WRITING | PLR_MAILING | PLR_OLC))
            return;

        if (!_wearoffToChar.empty()) {
            send_to_char(getTarget(), "%s\r\n", _wearoffToChar.c_str());
        }
        if (!_wearoffToRoom.empty()) {
            char *buf = tmp_sprintf("%s\r\n", _wearoffToRoom.c_str());
            act(buf, FALSE, getTarget(), NULL, getTarget(), TO_ROOM);
        }

        affectModify(false);
        CreatureAffectList *list = getTarget()->getAffectList();
        CreatureAffectList::iterator li;
        li = find(list->begin(), list->end(), this);
        if (li == list->end()) {
            errlog("Failed to get iterator to *this in "
                   "CreatureAffect::handleTick()!");
            return;
        }
        getTarget()->getAffectList()->remove(li);
    }

    _target->affectTotal();
}

void 
CreatureAffect::handleDeath() { }

void CreatureAffect::performUpdate() { }

// ********************************************************************
//
// EquipAffect
//
// ********************************************************************

bool
EquipAffect::canJoin() {
    // Check that we don't somehow already have an affect for this piece
    // piece of eq
    CreatureAffectList::iterator li = _target->getAffectList()->begin();
    for (; li != _target->getAffectList()->end(); ++li) {
        if ((*li)->getID() == this->getID())
            return false;
    }

    return true;
}

bool 
EquipAffect::affectJoin() {
    if (!_target) {
        errlog("No _target in CreatureAffect::affectJoin()");
        return false;
    }
    
    if (!canJoin())
        return false;

    affectModify(true);
    _target->getAffectList()->add_back(this);

    _target->affectTotal();
    return true;
}

GenericAffect*
EquipAffect::createNewInstance() {
    return (GenericAffect *)new EquipAffect();
}

void
EquipAffect::handleTick()
{
    // EquipAffects also don't do anything on a tick
    if (!getTarget()->getAffectList()) {
        errlog("getVictim()->getAffectList() returned NULL in "
               "CreatureAffect::handleTick()!");
    }

    return;
}

void
register_affects() {
    GenericAffect::insertAffect(new CreatureAffect());
    GenericAffect::insertAffect(new EquipAffect());
    GenericAffect::insertAffect(new ObjectAffect());
    GenericAffect::insertAffect(new RoomAffect());
    GenericAffect::insertAffect(new ElectrostaticFieldAffect());    
    GenericAffect::insertAffect(new PoisonAffect());    
    GenericAffect::insertAffect(new SicknessAffect());    
    GenericAffect::insertAffect(new AblazeAffect());    
    GenericAffect::insertAffect(new InvisAffect());    
    GenericAffect::insertAffect(new BlurAffect());    
    GenericAffect::insertAffect(new BlindnessAffect());    
    GenericAffect::insertAffect(new FireShieldAffect());    
    GenericAffect::insertAffect(new CurseAffect());    
    GenericAffect::insertAffect(new CharmAffect());    
    GenericAffect::insertAffect(new AntiMagicShellAffect());    
    GenericAffect::insertAffect(new ManaShieldAffect());    
    GenericAffect::insertAffect(new AcidityAffect());
    GenericAffect::insertAffect(new ChemicalStabilityAffect());
    GenericAffect::insertAffect(new FearAffect());
    GenericAffect::insertAffect(new PsishieldAffect());
    GenericAffect::insertAffect(new PsychicFeedbackAffect());
    GenericAffect::insertAffect(new RegenAffect());
    GenericAffect::insertAffect(new PsychicCrushAffect());
}

// ********************************************************************
//
// Object Affect
//
// ********************************************************************

ObjectAffect::ObjectAffect() : GenericAffect(), _bitvector(3, 0)
{
    _damMod = 0;
    _maxDamMod = 0;
    _wornMod = 0;
    _weightMod = 0;
    memset(_valMod, 0x0, sizeof(_valMod));
    _typeMod = 0;
    _oldType = 0;
    _apply.clear();
    _ownerID = 0;
    _wearoffToChar.clear();
    _wearoffToRoom.clear();
}

ObjectAffect::ObjectAffect(Creature *owner) : GenericAffect(), 
                                                  _bitvector(3, 0)
{
    _damMod = 0;
    _maxDamMod = 0;
    _wornMod = 0;
    _weightMod = 0;
    memset(_valMod, 0x0, sizeof(_valMod));
    _typeMod = 0;
    _oldType = 0;
    _apply.clear();
    _ownerID = owner->getIdNum();
    _wearoffToChar.clear();
    _wearoffToRoom.clear();
}

ObjectAffect::ObjectAffect(ObjectAffect &c) : _bitvector(c._bitvector),
                                                    _apply(c._apply)
{
    _skillNum = c.getSkillNum();
    _duration = c.getDuration();
    _level = c.getLevel();
    _accumDuration = c.getAccumDuration();
    _accumAffect = c.getAccumAffect();
    _averageDuration = c.getAverageDuration();
    _averageAffect = c.getAverageAffect();
    _isPermenant = c.getIsPermenant();
    _ownerID = c.getOwner();
    _damMod = c.getDamMod();
    _maxDamMod = c.getMaxDamMod();
    _wornMod = c.getWornMod();
    _weightMod = c.getWeightMod();
    _typeMod = c.getTypeMod();
    for (int i = 0; i < 4; i++) {
        _valMod[i] = c.getValMod(i);
    }
    _oldType = c._oldType;
    _target = c.getTarget();
    _wearoffToChar = c._wearoffToChar;
    _wearoffToRoom = c._wearoffToRoom;
}

ObjectAffect::~ObjectAffect() {
}

GenericAffect*
ObjectAffect::createNewInstance() {
    return (GenericAffect *)new ObjectAffect();
}

void
ObjectAffect::addApply(short location, short modifier) {
    locModType app;

    app.location = location;
    app.modifier = modifier;

    vector<locModType>::iterator vi = _apply.begin();
    for (; vi != _apply.end(); ++vi) {
        if (vi->location == location) {
            if (_accumAffect)
                vi->modifier += modifier;
            if (_averageAffect)
                vi->modifier = (vi->modifier + modifier) / 2;
            return;
        }
    }

    _apply.push_back(app);
}

void
ObjectAffect::addValue(short location, short modifier) {

    if (location < 0 || location > 3)
        return;

    if (_valMod[location]) {
        if (_accumAffect)
            _valMod[location] += modifier;
        if (_averageAffect)
            _valMod[location] += (_valMod[location] + modifier) / 2;
    }
    
    _valMod[location] = modifier;
}

void
ObjectAffect::addWeight(short modifier) {

    if (_weightMod) {
        if (_accumAffect)
            _weightMod += modifier;
        if (_averageAffect)
            _weightMod = (_weightMod + modifier) / 2;
    }

    _weightMod = modifier;
}

void
ObjectAffect::addDam(short modifier) {

    if (_damMod) {
        if (_accumAffect)
            _damMod += modifier;
        if (_averageAffect)
            _damMod = (_damMod + modifier) / 2;
    }

    _damMod = modifier;
}

void
ObjectAffect::addMaxDam(short modifier) {

    if (_maxDamMod) {
        if (_accumAffect)
            _maxDamMod += modifier;
        if (_averageAffect)
            _maxDamMod = (_maxDamMod + modifier) / 2;
    }

    _maxDamMod = modifier;
}

void
ObjectAffect::setApply(short location, short modifier) {
    locModType app;

    app.location = location;
    app.modifier = modifier;

    vector<locModType>::iterator vi = _apply.begin();
    for (; vi != _apply.end(); ++vi) {
        if (vi->location == location) {
            vi->modifier = modifier;
            return;
        }
    }

    _apply.push_back(app);
}

short
ObjectAffect::getApplyMod(short location) {
    vector<locModType>::iterator vi = _apply.begin();

    for (; vi != _apply.end(); ++vi) {
        if (vi->location == location)
            return vi->modifier;
    }

    return 0;
}

bool
ObjectAffect::canJoin() {
    ObjectAffect *aff = (ObjectAffect *)_target->affectedBy(_skillNum);
    if ((aff && *aff == *this) && 
        !(_accumDuration || _accumAffect)) {
        return false;
    }
    return true;
}

bool 
ObjectAffect::affectJoin() {
    if (!_target) {
        errlog("No _target in ObjectAffect::affectJoin()");
        return false;
    }
    
    if (!canJoin())
        return false;

    ObjectAffectList::iterator li = _target->getAffectList()->begin();
    for (; li != _target->getAffectList()->end(); ++li) {
        if (*(*li) == *this) {
            // We found something that matched, let's turn it off
            (*li)->affectModify(false);
            if (_accumDuration)
                (*li)->setDuration(_duration + (*li)->getDuration());
            else if (_averageDuration)
                (*li)->setDuration(((*li)->getDuration() + _duration) / 2);

            for (int i = 0; i < 4; i++)
                (*li)->addValue(i, _valMod[i]);

            vector<locModType>::iterator vi = _apply.begin();
            for (; vi != _apply.end(); ++vi) {
                (*li)->addApply(vi->location, vi->modifier);

                // Impose some caps
                if ((*li)->getDuration() > 666)
                    (*li)->setDuration(666);

                if ((*li)->getApplyMod(vi->location) > 666)
                    (*li)->setApply(vi->location, 666);
                if ((*li)->getApplyMod(vi->location) < -666)
                    (*li)->setApply(vi->location, -666);
            }
            
            if (getWeightMod())
                (*li)->addWeight(getWeightMod());
            if (getDamMod())
                (*li)->addDam(getWeightMod());
            if (getMaxDamMod())
                (*li)->addMaxDam(getWeightMod());

            // After modifying the affect appropriately we'll turn it
            // back on
            (*li)->affectModify(true);
            return true;
        }
    }

    affectModify(true);
    _target->getAffectList()->add_back(this);

    return true;
}

void ObjectAffect::affectRemove() {
    if (!this) return;
    
    if (!_target) {
        errlog("No _target in ObjectAffect::affectRemove()");
        return;
    }

    affectModify(false);
    ObjectAffectList *list = getTarget()->getAffectList();
    ObjectAffectList::iterator li;
    li = find(list->begin(), list->end(), this);
    if (li == list->end()) {
        errlog("Failed to get iterator to *this in "
               "ObjectAffect::affectRemove()!");
        return;
    }
    getTarget()->getAffectList()->remove(li);

    delete this;
}

void
ObjectAffect::affectModify(bool add) {
    if (!_target) {
        errlog("No _target in ObjectAffect::affectApply()");
        return;
    }

    // If I've thought this through properly, this should
    // prevent us from accidentally adding or removing the
    // same affect multiple times.
    if (_active == add)
        return;
    else
        _active = add;
    
    int pos, pos_mode;
    Creature *owner = NULL;
    // Now remove and replace the eq to update the EquipAffect on the character
    if (_target->worn_by) {
        pos_mode = MODE_EQ;
        pos = _target->getEquipPos();
        if (pos < 0) {
            pos_mode = MODE_IMPLANT;
            pos = _target->getImplantPos();
        }
        if (pos >= 0) {
            owner = _target->worn_by;
            unequip_char(owner, pos, pos_mode, true);
        }
    }

    // Set or restore damage
    if (_damMod) {
        if (add)
            _target->obj_flags.damage += _damMod;
        else
            _target->obj_flags.damage -= _damMod;
    }
    // Set or restore maxdam
    if (_maxDamMod) {
        if (add)
            _target->obj_flags.max_dam += _maxDamMod;
        else
            _target->obj_flags.max_dam -= _maxDamMod;
    }
    // Set or reset the value mods
    for (int i = 0; i < 4; i++) {
        if (_valMod[i]) {
            if (add)
                _target->obj_flags.value[i] += _valMod[i];
            else
                _target->obj_flags.value[i] -= _valMod[i];
        }
    }
    // Set or restore type
    if (_typeMod) {
        if (add) {
            _oldType = _target->obj_flags.type_flag;
            _target->obj_flags.type_flag = _typeMod;
        }
        else {
            _target->obj_flags.type_flag = _oldType;
        }
    }
    // Set or reset wear positions
    if (_wornMod) {
        if (add) {
            check_bits_32(_target->obj_flags.wear_flags, &_wornMod);
            _target->obj_flags.wear_flags |= _wornMod;
        }
        else {
            _target->obj_flags.wear_flags &= ~_wornMod;
        }
    }
    // Set or reset weight
    if (_weightMod) {
        if (add)
            _target->setWeight(_target->getWeight() + _weightMod);
        else
            _target->setWeight(_target->getWeight() - _weightMod);
    }
    // Set or reset extra flags
    if (add) {
        if (_bitvector[0]) {
            check_bits_32(_target->obj_flags.extra_flags, (int *)&_bitvector[0]);
            _target->obj_flags.extra_flags |= _bitvector[0];
        }
        if (_bitvector[1]) {
            check_bits_32(_target->obj_flags.extra2_flags, (int *)&_bitvector[1]);
            _target->obj_flags.extra2_flags |= _bitvector[1];
        }
        if (_bitvector[2]) {
            check_bits_32(_target->obj_flags.extra3_flags, (int *)&_bitvector[2]);
            _target->obj_flags.extra3_flags |= _bitvector[2];
        }
    }
    else {
        if (_bitvector[0])
            _target->obj_flags.extra_flags &= ~_bitvector[0];
        if (_bitvector[1])
            _target->obj_flags.extra2_flags &= ~_bitvector[1];
        if (_bitvector[3])
            _target->obj_flags.extra_flags &= ~_bitvector[2];
    }

    vector<locModType>::iterator vi;
    for (vi = _apply.begin(); vi != _apply.end(); ++vi) {
        int mod = vi->modifier;
        int loc = vi->location;
        int i;
        bool found = false;;

        if (mod > 125)
            mod = 125;
        else if (mod < -125)
            mod = -125;

        for (i = 0; i < MAX_OBJ_AFFECT; i++) {
            if (_target->affected[i].location == loc) {
                found = true;
                if (add)
                    _target->affected[i].modifier += mod;
                else
                    _target->affected[i].modifier -= mod;
                break;
            }
        }

        if (found) {
            if (_target->affected[i].modifier == 0) {
                _target->affected[i].location = APPLY_NONE;
            }
        }

        if (!found) {
            for (int j = 0; j < MAX_OBJ_AFFECT; j++) {
                if (_target->affected[j].location == APPLY_NONE) {
                    found = true;
                    _target->affected[j].location = loc;
                    _target->affected[j].modifier = mod;
                    break;
                }
            }
        }

        _target->normalizeApplies();

        if (!found) {
            errlog("No affect locations trying to alter object affect on obj vnum %d, id %ld",
                    GET_OBJ_VNUM(_target), _target->unique_id);
        }
    }

    if (owner)
        equip_char(owner, _target, pos, pos_mode);
}

void
ObjectAffect::saveToXML(FILE *ouf) {
    vector<locModType>::iterator vi;
    vector<long>::iterator bvi;

    fprintf(ouf, "<tmpaffect skillNum=\"%d\" "
                 "name=\"%s\" "
                 "duration=\"%d\" "
                 "level=\"%d\" "
                 "accumDuration=\"%d\" "
                 "accumAffect=\"%d\" "
                 "averageDuration=\"%d\" "
                 "averageAffect=\"%d\" "
                 "isPermenant=\"%d\" "
                 "owner=\"%ld\" "
                 "id=\"%ld\" "
                 "wearoffToChar=\"%s\" "
                 "wearoffToRoom=\"%s\" "
                 "dam_mod=\"%d\" "
                 "maxdam_mod=\"%d\" "
                 "val_mod1=\"%d\" "
                 "val_mod2=\"%d\" "
                 "val_mod3=\"%d\" "
                 "val_mod4=\"%d\" "
                 "type_mod=\"%d\" "
                 "old_type=\"%d\" "
                 "worn_mod=\"%d\" "
                 "weight_mod=\"%d\" ",
                 _skillNum, getClassName().c_str(), _duration, _level, 
                 _accumDuration, _accumAffect, _averageDuration, 
                 _averageAffect, _isPermenant, _ownerID, _ID,  
                 _wearoffToChar.c_str(), _wearoffToRoom.c_str(), _damMod, _maxDamMod, _valMod[0],
                 _valMod[1], _valMod[2], _valMod[3], _typeMod, _oldType, _wornMod, _weightMod);


    if (_apply.size()) {
        fprintf(ouf, "apply=\"");
        for (vi = _apply.begin(); vi != _apply.end(); ++vi) {
            fprintf(ouf, "%d/%d ", vi->location, vi->modifier);
        }
        fprintf(ouf, "\" ");
    }

    if (_bitvector.size()) {
        int counter = 0;
        fprintf(ouf, "bitvector=\"");
        for (bvi = _bitvector.begin(); bvi != _bitvector.end(); ++bvi) {
            if (*bvi != 0)
                fprintf(ouf, "%d/%ld ", counter, *bvi);
            counter++;
        }
        fprintf(ouf, "\" ");
    }

    fprintf(ouf, "/>\n");
}

void
ObjectAffect::loadFromXML(xmlNodePtr node)
{
    char *value, *arg;
    int numValue;

    setSkillNum(xmlGetIntProp(node, "skillNum"));
    setDuration(xmlGetIntProp(node, "duration"));
    setLevel((short)xmlGetIntProp(node, "level"));
    if (xmlGetIntProp(node, "accumDuration"))
        setAccumDuration(true);
    else
        setAccumDuration(false);

    if (xmlGetIntProp(node, "accumAffect"))
        setAccumAffect(true);
    else
        setAccumAffect(false);

    if (xmlGetIntProp(node, "averageDuration"))
        setAverageDuration(true);
    else
        setAverageDuration(false);

    if (xmlGetIntProp(node, "averageAffect"))
        setAverageAffect(true);
    else
        setAverageAffect(false);

    if (xmlGetIntProp(node, "isPermenant"))
        setIsPermenant(true);
    else
        setIsPermenant(false);

    setOwner(xmlGetIntProp(node, "owner"));
    setID(xmlGetIntProp(node, "id"));

    if ((value = xmlGetProp(node, "apply"))) {
        arg = tmp_getword(&value);
        while (*arg) {
            char *loc, *mod;
            arg = tmp_gsub(arg, "/", " ");
            loc = tmp_getword(&arg);
            mod = tmp_getword(&arg);

            if (!*loc || !*mod) {
                errlog("loc or mod has no data while loading applies in "
                        "ObjectAffect::loadFromXML()!");
            }
            else
                addApply(atoi(loc), atoi(mod));

            arg = tmp_getword(&value);
        }
    }

    if ((value = xmlGetProp(node,"bitvector"))) {
        arg = tmp_getword(&value);
        while (*arg) {
            char *loc, *mod;
            arg = tmp_gsub(arg, "/", " ");
            loc = tmp_getword(&arg);
            mod = tmp_getword(&arg);

            if (!*loc || !*mod) {
                errlog("loc or mod has no data while loading bitvectors in "
                        "ObjectAffect::loadFromXML()!");
            }
            else
                setBitvector(atoi(loc), atol(mod));

            arg = tmp_getword(&value);
        }
    }

    if ((numValue = xmlGetIntProp(node, "dam_mod")))
        _damMod = numValue;

    if ((numValue = xmlGetIntProp(node, "maxdam_mod")))
        _maxDamMod = numValue;

    if ((numValue = xmlGetIntProp(node, "val_mod1")))
        _valMod[0] = numValue;

    if ((numValue = xmlGetIntProp(node, "val_mod2")))
        _valMod[1] = numValue;

    if ((numValue = xmlGetIntProp(node, "val_mod3")))
        _valMod[2] = numValue;

    if ((numValue = xmlGetIntProp(node, "val_mod4")))
        _valMod[3] = numValue;

    if ((numValue = xmlGetIntProp(node, "type_mod")))
        _typeMod = numValue;

    if ((numValue = xmlGetIntProp(node, "old_type")))
        _oldType = numValue;

    if ((numValue = xmlGetIntProp(node, "worn_mod")))
        _wornMod = numValue;

    if ((numValue = xmlGetIntProp(node, "weight_mod")))
        _weightMod = numValue;

    value = xmlGetProp(node, "wearoffToChar");
    if (value)
        setWearoffToChar(value);

    value = xmlGetProp(node, "wearoffToRoom");
    if (value)
        setWearoffToRoom(value);
}

void
ObjectAffect::handleTick()
{
    if (!getTarget()->getAffectList()) {
        errlog("getVictim()->getAffectList() returned NULL in "
               "ObjectAffect::handleTick()!");
        return;
    }

    if (_isPermenant)
        return;

    if (_duration >= 1) {
        _duration--;
        return;
    }
    else {
        affectModify(false);
        ObjectAffectList *list = getTarget()->getAffectList();
        ObjectAffectList::iterator li;
        li = find(list->begin(), list->end(), this);
        if (li == list->end()) {
            errlog("Failed to get iterator to *this in "
                   "ObjectAffect::handleTick()!");
            return;
        }
        getTarget()->getAffectList()->remove(li);

        Creature *owner = (_target->worn_by) ? _target->worn_by : 
            (_target->carried_by) ? _target->carried_by : NULL;

        if (owner &&  !PLR_FLAGGED(owner, PLR_WRITING | PLR_MAILING | PLR_OLC)) {
            if (!_wearoffToChar.empty()) {
                char *buf = tmp_sprintf("%s\r\n", _wearoffToChar.c_str());
                act(buf, FALSE, owner, _target, owner, TO_CHAR);
            }
            if (!_wearoffToRoom.empty()) {
                char *buf = tmp_sprintf("%s\r\n", _wearoffToRoom.c_str());
                act(buf, FALSE, owner, _target, owner, TO_ROOM);
            }
        }
    }
}

// This function fires when the creature wearing or carrying the object dies.
void 
ObjectAffect::handleDeath() { }

void ObjectAffect::performUpdate() { }

// ********************************************************************
//
// Room Affect
//
// ********************************************************************

RoomAffect::RoomAffect() : GenericAffect(), _exitInfo(NUM_DIRS, 0) {
    _roomFlags = 0;
    _wearoffToRoom.clear();
    _target = NULL;
}

RoomAffect::RoomAffect(Creature *owner) : GenericAffect(), _exitInfo(NUM_DIRS, 0) {
    _roomFlags = 0;
    _ownerID = owner->getIdNum();
    _wearoffToRoom.clear();
    _target = NULL;
}

RoomAffect::RoomAffect(RoomAffect &c) : _exitInfo(c._exitInfo) {
    _skillNum = c.getSkillNum();
    _duration = c.getDuration();
    _level = c.getLevel();
    _accumDuration = c.getAccumDuration();
    _accumAffect = c.getAccumAffect();
    _averageDuration = c.getAverageDuration();
    _averageAffect = c.getAverageAffect();
    _isPermenant = c.getIsPermenant();
    _ownerID = c.getOwner();
    _target = c.getTarget();
    _wearoffToRoom = c._wearoffToRoom;
    _description = c._description;
    _name = c._name;
    _roomFlags = c._roomFlags;
}

RoomAffect::~RoomAffect() {
}

GenericAffect*
RoomAffect::createNewInstance() {
    return (GenericAffect *)new RoomAffect();
}

bool
RoomAffect::canJoin() {
    RoomAffect *aff = (RoomAffect *)_target->affectedBy(_skillNum);

    if (aff && *aff == *this)
        return false;

    return true;
}

bool 
RoomAffect::affectJoin() {
    if (!_target) {
        errlog("No _target in RoomAffect::affectJoin()");
        return false;
    }
    
    if (!canJoin())
        return false;

    affectModify(true);
    _target->getAffectList()->add_back(this);

    return true;
}

void
RoomAffect::affectRemove() {
    if (!this) 
        return;
    
    if (!_target) {
        errlog("No _target in RoomAffect::affectRemove()");
        return;
    }

    affectModify(false);
    RoomAffectList *list = getTarget()->getAffectList();
    RoomAffectList::iterator li;
    li = find(list->begin(), list->end(), this);
    if (li == list->end()) {
        errlog("Failed to get iterator to *this in "
               "RoomAffect::affectRemove()!");
        return;
    }
    _target->getAffectList()->remove(li);

    delete this;
}

void
RoomAffect::affectModify(bool add) {
    if (!_target) {
        errlog("No _target in RoomAffect::affectApply()");
        return;
    }

    // If I've thought this through properly, this should
    // prevent us from accidentally adding or removing the
    // same affect multiple times.
    if (_active == add)
        return;
    else
        _active = add;
    
    for (int i = 0; i < NUM_DIRS; i++) {
        if (_exitInfo[i]) {
            if (add) {
                check_bits_32(_target->dir_option[i]->exit_info, (int *)&_exitInfo[i]);
                _target->dir_option[i]->exit_info |= _exitInfo[i];
            }
            else {
                _target->dir_option[i]->exit_info &= ~(_exitInfo[i]);
            }
        }
    }

    if (_roomFlags) {
        if (add) {
            check_bits_32(_target->room_flags, (int *)&_roomFlags);
            _target->room_flags |= _roomFlags;
        }
        else {
            _target->room_flags &= ~(_roomFlags);
        }
    }
}

void
RoomAffect::handleTick() {
    if (!getTarget()->getAffectList()) {
        errlog("getVictim()->getAffectList() returned NULL in "
               "RoomAffect::handleTick()!");
        return;
    }

    if (_isPermenant)
        return;

    if (_duration >= 1) {
        _duration--;
        return;
    }
    else {
        affectModify(false);
        RoomAffectList *list = getTarget()->getAffectList();
        RoomAffectList::iterator li;
        li = find(list->begin(), list->end(), this);
        if (li == list->end()) {
            errlog("Failed to get iterator to *this in "
                   "RoomAffect::handleTick()!");
            return;
        }
        _target->getAffectList()->remove(li);

        Creature *owner = get_char_in_world_by_idnum(_ownerID);

        if (!owner)
            return;

        if (owner && owner->in_room == _target && 
                !PLR_FLAGGED(owner, PLR_WRITING | PLR_MAILING | PLR_OLC)) {
            if (!_wearoffToRoom.empty()) {
                char *buf = tmp_sprintf("%s\r\n", _wearoffToRoom.c_str());
                act(buf, FALSE, owner, NULL, NULL, TO_ROOM);
                act(buf, FALSE, owner, NULL, NULL, TO_CHAR);
            }
        }
    }
}

// This function should fire when anyone in the room dies
void
RoomAffect::handleDeath() {
}

void
RoomAffect::performUpdate() {
}
