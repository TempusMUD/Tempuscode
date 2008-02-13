#include "creature.h"
#include "comm.h"
#include "executable_object.h"
#include "macros.h"
#include "generic_affect.h"
#include "unique_id.h"
#include "constants.h"
#include "utils.h"
#include "spells.h"
#include "db.h"
#include "house.h"
#include "clan.h"
#include "handler.h"
#include "fight.h"
#include "quest.h"
#include "vehicle.h"
#include "materials.h"
#include "generic_skill.h"

extern void add_follower(struct Creature *ch, struct Creature *leader);
extern void eqdam_extract_obj(struct obj_data *obj);
void gain_condition(struct Creature *ch, int condition, int value);

using namespace std;

// Normal Constructor
ExecutableObject::ExecutableObject(Creature *character) {
    _modifiable = true;
    _executed = false;
    _victimKilled = false;
    _char = character;
    _victim = NULL;
    _obj = NULL;
    _room = NULL;
    _toCharMessages.clear();
    _toVictMessages.clear();
    _toRoomMessages.clear();
    _ID = UniqueID::getNextID();
    _skillNum = 0;
    _depth = 0;
    _actRoomFlags = 0;
}

// Normal Destructor
ExecutableObject::~ExecutableObject() {
}

// Copy constructor
ExecutableObject::ExecutableObject(const ExecutableObject &c) : _toCharMessages(c._toCharMessages),
                                                                _toVictMessages(c._toVictMessages),
                                                                _toRoomMessages(c._toRoomMessages)
{
    _modifiable = c.getModifiable();
    _executed = c.getExecuted();
    _victimKilled = c.isVictimDead();
    _char = c.getChar();
    _victim = c.getVictim();
    _obj = c.getObject();
    _room = c.getRoom();
    _ID = c.getID();
    _skillNum = c.getSkill();
    _depth = c._depth;
    _actRoomFlags = c._actRoomFlags;
}


// Operators 
ExecutableObject 
&ExecutableObject::operator =(ExecutableObject &c) {
    _modifiable = c.getModifiable();
    _executed = c.getExecuted();
    _char = c.getChar();
    _victim = c.getVictim();
    _obj = c.getObject();
    _room = c.getRoom();
    _ID = c.getID();
    _toCharMessages = c._toCharMessages;
    _toVictMessages = c._toVictMessages;
    _toRoomMessages = c._toRoomMessages;
    _depth = c._depth;
    _actRoomFlags = c._actRoomFlags;
    return *this;
}

bool 
ExecutableObject::operator ==(ExecutableObject &c) const {
    if (_modifiable != c.getModifiable())
        return false;

    if (_executed != c.getExecuted())
        return false;

    if (_char != c.getChar())
        return false;

    if (_victim != c.getVictim())
        return false;

    if (_obj != c.getObject())
        return false;

    if (_room != c.getRoom())
        return false;

    if (_toCharMessages != c._toCharMessages)
        return false;
    
    if (_toVictMessages != c._toVictMessages)
        return false;
    
    if (_toRoomMessages != c._toRoomMessages)
        return false;
    
    return true;
}

// Accessors
void 
ExecutableObject::setModifiable(bool modifiable) {
    _modifiable = modifiable;
}

void
ExecutableObject::setExecuted(bool executed) {
    _executed = executed;
}

void
ExecutableObject::setChar(Creature *ch) {
    _char = ch;
}

void
ExecutableObject::setVictim(Creature *victim) {
    _victim = victim;
}

void
ExecutableObject::setObject(obj_data *obj) {
    _obj = obj;
}
    
void
ExecutableObject::setRoom(room_data *room) {
    _room = room;
}

int 
ExecutableObject::addToCharMessage(string message) {
    _toCharMessages.push_back(message);

    return _toCharMessages.size();
}

int
ExecutableObject::addToVictMessage(string message) {
    _toVictMessages.push_back(message);

    return _toVictMessages.size();
}

int
ExecutableObject::addToRoomMessage(string message, int flags) {
    if (flags != 0)
        _actRoomFlags = flags;

    _toRoomMessages.push_back(message);

    return _toRoomMessages.size();
}

int
ExecutableObject::removeToCharMessage(int index) {
    vector<string>::iterator vi = _toCharMessages.begin();

    for (int x = 0; x <= index; ++x, ++vi);

    _toCharMessages.erase(vi);

    return _toCharMessages.size();
}

int
ExecutableObject::removeToVictMessage(int index) {
    vector<string>::iterator vi = _toVictMessages.begin();

    for (int x = 0; x <= index; ++x, ++vi);

    _toVictMessages.erase(vi);

    return _toVictMessages.size();
}

int
ExecutableObject::removeToRoomMessage(int index) {
    vector<string>::iterator vi = _toRoomMessages.begin();

    for (int x = 0; x <= index; ++x, ++vi);

    _toRoomMessages.erase(vi);

    return _toRoomMessages.size();
}

bool 
ExecutableObject::getModifiable() const {
    return _modifiable;
}

bool
ExecutableObject::getExecuted() const {
    return _executed;
}

bool
ExecutableObject::isVictimDead() const {
    return _victimKilled;
}

Creature *
ExecutableObject::getChar() const {
    return _char;
}

Creature *
ExecutableObject::getVictim() const {
    return _victim;
}

obj_data *
ExecutableObject::getObject() const {
    return _obj;
}

room_data *
ExecutableObject::getRoom() const {
    return _room;
}

// Normal members
void 
ExecutableObject::execute() {
    if (getExecuted())
        return;
    
    if (_sendMessages)
        sendMessages();

    setExecuted();
}

void 
ExecutableObject::sendMessages() {
    if (_char) {
        for (unsigned int x = 0; x < _toCharMessages.size(); x++)
            act(_toCharMessages[x].c_str(), false, _char, _obj, 
            _victim, TO_CHAR);
    }
    if (_victim) {
        for (unsigned int x = 0; x < _toVictMessages.size(); x++)
            act(_toVictMessages[x].c_str(), false, _char, _obj, 
            _victim, TO_VICT | TO_SLEEP);
    }
    if (!_actRoomFlags) {
        for (unsigned int x = 0; x < _toRoomMessages.size(); x++)
            act(_toRoomMessages[x].c_str(), false, _char, _obj, 
                _victim, TO_ROOM | TO_SLEEP);
    }
    else {
        for (unsigned int x = 0; x < _toRoomMessages.size(); x++)
            act(_toRoomMessages[x].c_str(), false, _char, _obj, 
                _victim, _actRoomFlags);
    }
}


bool
ExecutableObject::isA(EOType type) const {
    switch (type) {
        case EExecutableObject: 
            return true; 
            break;
        case EDamageObject:
            if (dynamic_cast<const DamageObject * const>(this))
                return true;
            break;
        case EAffectObject:
            if (dynamic_cast<const AffectObject * const>(this))
                return true;
            break;
        case ECostObject:
            if (dynamic_cast<const CostObject * const>(this))
                return true;
            break;
        case EWaitObject:
            if (dynamic_cast<const WaitObject * const>(this))
                return true;
            break;
        case ETeleportObject:
            if (dynamic_cast<const TeleportObject * const>(this))
                return true;
            break;
        case ESummonsObject:
            if (dynamic_cast<const SummonsObject * const>(this))
                return true;
            break;
        case ERemoveAffectObject:
            if (dynamic_cast<const RemoveAffectObject * const>(this))
                return true;
            break;
        case EModifyItemObject:
            if (dynamic_cast<const ModifyItemObject * const>(this))
                return true;
            break;
        case EDamageItemObject:
            if (dynamic_cast<const DamageItemObject * const>(this))
                return true;
            break;
        case EDestroyItemObject:
            if (dynamic_cast<const DestroyItemObject * const>(this))
                return true;
            break;
        case EDistractionObject:
            if (dynamic_cast<const DistractionObject * const>(this))
                return true;
            break;
        case ESatiationObject:
            if (dynamic_cast<const SatiationObject * const>(this))
                return true;
            break;
        case EPointsObject:
            if (dynamic_cast<const PointsObject * const>(this))
                return true;
            break;
        case ENullPsiObject:
            if (dynamic_cast<const NullPsiObject * const>(this))
                return true;
            break;
        case EPositionObject:
            if (dynamic_cast<const PositionObject * const>(this))
                return true;
            break;
        case EStopCombatObject:
            if (dynamic_cast<const StopCombatObject * const>(this))
                return true;
            break;
        case EStartCombatObject:
            if (dynamic_cast<const StartCombatObject * const>(this))
                return true;
            break;
    }

    return false;
}

void
ExecutableObject::cancelByID() {
    for (unsigned int i=0; i < getEV()->size(); i++) {
        if ((*getEV())[i]->getID() == this->getID()) {
            (*getEV())[i]->setExecuted();
        }
    }
}


void
ExecutableObject::cancelAll() {
    this->cancelByID();
    this->cancelByDepth();
}

void
ExecutableObject::cancelByDepth() {
    for (unsigned int i=0; i < getEV()->size(); i++) {
        if ((*getEV())[i]->getDepth() > this->getDepth()) {
            (*getEV())[i]->setExecuted();
        }
    }
}

void
ExecutableObject::cancel() {
    setExecuted();
}

void
ExecutableObject::prepare() {
    if (!_modifiable)
        return;

    if (_char) {
        _char->modifyEO(this);
    }
    
    if (_victim) {
        _victim->modifyEO(this);
    }
    
    if (_obj) {
        _obj->modifyEO(this);
    }

    //once we have code for rooms to do this add it in
    /*
    if (_room) {
        _room->modifyEO(this);
    }*/

    setModifiable(false);
}

// *************************************************************
//
// CostObject
//
// *************************************************************

CostObject::CostObject(Creature *character) : ExecutableObject(character) {
    _char = character;
    _mana = 0;
    _move = 0;
    _hit = 0;
}

int
CostObject::getMana() const {
    return _mana;
}

int
CostObject::getMove() const {
    return _move;
}

int
CostObject::getHit() const {
    return _hit;
}

void
CostObject::setMana(int mana) {
    _mana = mana;
}

void
CostObject::setMove(int move) {
    _move = move;
}

void
CostObject::setHit(int hit) {
    _hit = hit;
}

void
CostObject::execute() {
    if (getExecuted())
        return;
    
    if (getChar()->getMove() >= getMove() && 
        getChar()->getMana() >= getMana() &&
        getChar()->getHit() >= getHit()) 
    {
        getChar()->setMove(getChar()->getMove() - this->getMove());
        getChar()->setMana(getChar()->getMana() - this->getMana());
        getChar()->setHit(getChar()->getHit() - this->getHit());
    } else { if (getChar()->getMove() < getMove()) send_to_char(this->getChar(), "You are too tired to take this action.\r\n");
        else if (getChar()->getMana() < getMana())
            send_to_char(this->getChar(), "You lack the spiritual strength to take this action.\r\n");
        else if (getChar()->getHit() < getHit())
            send_to_char(this->getChar(), "You are not healthy enough to take this action.\r\n");
        this->cancelAll();
    }
    ExecutableObject::execute();
}

// *************************************************************
//
// WaitObject
//
// *************************************************************

WaitObject::WaitObject(Creature *ch, char amount) : ExecutableObject(ch) {
    setChar(ch);
    _wait = amount;
}

void
WaitObject::execute() {
    WAIT_STATE(_char, _wait RL_SEC);
    ExecutableObject::execute();
}

// *************************************************************
//
// DamageItemObject
//
// ************************************************************

DamageItemObject::DamageItemObject(Creature *ch, Creature *victim, short position, 
        short amount, short type) : ExecutableObject(ch) {
    if (!victim) {
        errlog("victim is NULL in DamageItemObject contructor #1");
        raise(SIGSEGV);
    }
    setChar(ch);
    setVictim(victim);
    _position = position;
    _type = type;
    _amount = amount;
    _obj = GET_EQ(victim, position);

    if (!_obj)
        _obj = GET_IMPLANT(victim, position);
}

DamageItemObject::DamageItemObject(Creature *victim, short position, 
        short amount, short type) : ExecutableObject(NULL) {
    if (!victim) {
        errlog("victim is NULL in DamageItemObject contructor #2");
        raise(SIGSEGV);
    }
    setVictim(victim);
    _position = position;
    _type = type;
    _amount = amount;
    _obj = GET_EQ(victim, position);

    if (!_obj)
        _obj = GET_IMPLANT(victim, position);
}

DamageItemObject::DamageItemObject(struct obj_data *obj, short amount, short type) : 
    ExecutableObject(NULL) {

    _obj = obj;
    _position = obj->worn_on;
    _type = type;
    _amount = amount;
}

void
DamageItemObject::execute() {

    if (!_obj && !_victim) {
        errlog("No object target and no victim in DamageItemObject::prepare()");
        cancel();
        return;
    }

    if (_victim && !_obj) {
        if (_position == WEAR_RANDOM)
            _position = number(0, NUM_WEARS);

        _obj = (GET_EQ(_victim, _position)) ? 
            GET_EQ(_victim, _position) : GET_IMPLANT(_victim, _position);
    }

    if (!_obj) {
        cancel();
        return;
    }

    if (!_victim)
        _victim = (_obj->worn_by) ? _obj->worn_by : _obj->carried_by;
    
    // If we still don't have a victim...
    if (!_victim) {
        struct obj_data *tmp_obj;

        tmp_obj = _obj->in_obj;
        while (tmp_obj && tmp_obj->in_obj)
            tmp_obj = tmp_obj->in_obj;

        if (tmp_obj)
            _victim = (tmp_obj->worn_by) ? tmp_obj->worn_by : tmp_obj->carried_by;
    }

    // If we still don't have a victim at this point, it means the object is laying on
    // the floor of a room, or in a container which is on the floor in a room, in which
    // case, no messages need to be sent.  Ever.
    if (!_victim) {
        if (!_obj->in_obj)
            _victim = _obj->in_room->people;
    }

    // Item is !break
    if (GET_OBJ_DAM(_obj) < 0 || GET_OBJ_MAX_DAM(_obj) < 0) {
        cancel();
        return;
    }
    
    if (GET_OBJ_TYPE(_obj) == ITEM_MONEY || GET_OBJ_TYPE(_obj) == ITEM_SCRIPT ||
        GET_OBJ_TYPE(_obj) == ITEM_KEY) {
        cancel();
        return;
    }

    // Objects in Limbo don't take damage
    if (_obj->in_room == zone_table->world) {
        cancel();
        return;
    }

    if (_char && _char->getLevel() < LVL_IMMORT && !CAN_WEAR(_obj, ITEM_WEAR_TAKE)) {
        cancel();
        return;
    }

    if ((_victim->in_room && ROOM_FLAGGED(_victim->in_room, ROOM_ARENA)) ||
        (_obj->in_room && ROOM_FLAGGED(_obj->in_room, ROOM_ARENA)) ||
        (_victim && GET_QUEST(_victim) && 
         QUEST_FLAGGED(quest_by_vnum(GET_QUEST(_victim)), QUEST_ARENA))) {
        cancel();
        return;
    }

    float newPercent = ((float((GET_OBJ_DAM(_obj)) - _amount) / (float)GET_OBJ_MAX_DAM(_obj)) * 100);
    float curPercent = ((float(GET_OBJ_DAM(_obj)) / (float)GET_OBJ_MAX_DAM(_obj)) * 100);

    char *message = NULL;
    if (newPercent <= 12.5 && curPercent > 12.5) {
        SET_BIT(GET_OBJ_EXTRA2(_obj), ITEM2_BROKEN);
        if (_obj->getEquipPos())
            obj_to_char(unequip_char(_obj->worn_by, _obj->worn_on, MODE_EQ), _obj->worn_by);
        else if (_obj->getImplantPos() && IS_DEVICE(_obj))
            ENGINE_STATE(_obj) = 0;
        message = tmp_sprintf("$p %s been severely damaged!!", HAS_HAVE(_obj->getName()));
    }
    else if (newPercent <= 25 && curPercent > 25) {
        message = tmp_sprintf("$p %s starting to look pretty ", ISARE(_obj->getName()));
        if (IS_METAL_TYPE(_obj))
            message = tmp_strcat(message, "mangled.", NULL);
        else if (IS_LEATHER_TYPE(_obj) || IS_CLOTH_TYPE(_obj))
            message = tmp_strcat(message, "ripped up.", NULL);
        else
            message = tmp_strcat(message, "bad.", NULL);;
    }
    else if (newPercent <= 50 && curPercent > 50) {
        message = tmp_sprintf("$p %s starting to show signs of wear.", ISARE(_obj->getName()));
    }

    if (_victim && message) {
        addToVictMessage(message);
        addToRoomMessage(message);
    }

    GET_OBJ_DAM(_obj) -= _amount;

    ExecutableObject::execute();
}

// *************************************************************
//
// TeleportObject
//
// *************************************************************

void
TeleportObject::execute() {
    struct room_data *to_room = NULL;
    int level = 49;

    if (_type == VStone) {
        if (!_obj) {
            addToCharMessage("What the hell??  No object?  REPORT THIS!");
            errlog("Nonexistant object in TeleportObject::execute() but "
                   "_type is VStone!");
        }
        else
            vstoneExecute();

        ExecutableObject::execute();
        look_at_room(_char, _char->in_room, 0);

        if (!GET_OBJ_VAL(_obj, 2)) {
            addToCharMessage("$p becomes dim, ceases to glow, and vanishes.");
            extract_obj(_obj);
        }
        return;
    }
    else if (_type == Summon) {
        if (!_char || !_victim) {
            errlog("No _char or _victim in TeleportObject::execute() with "
                   "_type == Summon");
            return;
        }

        bool success = summonExecute();
        ExecutableObject::execute();
        if (success)
            look_at_room(_victim, _victim->in_room, 0);

        
        return;
    }

    if (!_victim)
        return;

    if (_char && _type == LocalTeleport)
        level = _char->getLevelBonus(SPELL_LOCAL_TELEPORT);
    if (_char && _type == Teleport)
        level = _char->getLevelBonus(SPELL_TELEPORT);
    if (_char && _type == RandomCoordinates)
        level = _char->getLevelBonus(SPELL_RANDOM_COORDINATES);

    if (_type == Teleport && _char->in_room->zone->number == 400 
        || GET_PLANE(_char->in_room) == PLANE_DOOM
        || ZONE_FLAGGED(_char->in_room->zone, ZONE_ISOLATED)) {
        _type = LocalTeleport;
    }

    if (_char && AFF_FLAGGED(_char, AFF_CHARM) && _char->master
        && _char->master->in_room == _victim->in_room) {
        if (_char == _victim) {
            _char = _victim->master;
            addToVictMessage("You just can't stand the thought of leaving "
                    "$n behind.");
        }
        
        if (_victim == _char->master)
            addToCharMessage("You really can't stand the thought of parting "
                    "with $N.");
        
        ExecutableObject::execute();
        return;
    }

    if (_char && _char != _victim 
        && ROOM_FLAGGED(_victim->in_room, ROOM_PEACEFUL)) {
        addToVictMessage("You feel strange as $n attempts to teleport you.");
        addToCharMessage("You fail.  $N is in a non-violence zone!.");
        ExecutableObject::execute();
        return;
    }
   
    if (_char && _char != _victim
        && !_char->isNPC() && !_victim->isNPC() 
        && _victim->in_room->zone->getPKStyle() == ZONE_NO_PK) {
        addToVictMessage("You feel strange as $n attempts to teleport you.");
        addToCharMessage("You fail.  $N is in a !PK zone!.");
        ExecutableObject::execute();
        return;
    }

    if (_victim->isImmortal() && _victim->getLevel() > _char->getLevel()) {
        addToCharMessage("$N sneers at you with disgust.");
        addToVictMessage("You sneer at $n with disgust.");
        addToCharMessage("$N sneers at $n with disgust.");
        ExecutableObject::execute();
        return;
    }

    if (!teleportOK()) {
        addToCharMessage("$N resists your attempt to teleport $M!");
        addToCharMessage("You resist $n's attempt to teleport you!");
        ExecutableObject::execute();
        return;
    }

    if (MOB_FLAGGED(_victim, MOB_NOSUMMON)
        || (_victim->isNPC()
        && mag_savingthrow(_victim, level, SAVING_SPELL))) {
        addToCharMessage("You fail.");
        ExecutableObject::execute();
        return;
    }

    if ((_type == LocalTeleport || _type == Teleport)
        && ROOM_FLAGGED(_victim->in_room, ROOM_NORECALL)
        || ROOM_FLAGGED(_victim->in_room, ROOM_NOTEL)) {
        addToVictMessage("You fade out for a moment...\r\n"
                "The magic quickly dissipates!");
        addToRoomMessage("$n fades out for a moment but quickly "
                "flickers back into view.");
        ExecutableObject::execute();
        return;
    }

    if (_type == RandomCoordinates
        && ROOM_FLAGGED(_victim->in_room, ROOM_NORECALL)
        || ROOM_FLAGGED(_victim->in_room, ROOM_NOPHYSIC)) {
        addToVictMessage("You fade out for a moment...\r\n"
                "The fabric of reality snaps you back into existence!");
        addToRoomMessage("$n fades out for a moment but quickly "
                "flickers back into view.");
        ExecutableObject::execute();
        return;
    }

    int count = 0;
    struct zone_data *zone = _victim->in_room->zone;
    if (_type == Teleport || _type == RandomCoordinates) {
        do {
            zone = real_zone(number(0, 699));
            count++;
        } while (count < 1000 
                && (!zone || zone->plane != _char->in_room->zone->plane
                    || zone->time_frame != _char->in_room->zone->time_frame
                    || !IS_APPR(zone) || ZONE_FLAGGED(zone, ZONE_ISOLATED)));
    }

    bool found = false;
    int room_num = 0;
    while (!found && count++ < 1000) {
        room_num = number(zone->number * 100, zone->top);
        if (!(to_room = real_room(room_num)))
            continue;

        if (to_room->zone->number == 400 && zone != _victim->in_room->zone)
            continue;

        if (ROOM_FLAGGED(to_room, ROOM_HOUSE)
            && !House_can_enter(_char, to_room->number))
            continue;

        if (ROOM_FLAGGED(to_room, ROOM_CLAN_HOUSE)
            && !clan_house_can_enter(_char, to_room))
            continue;

        if (_type == RandomCoordinates && ROOM_FLAGGED(to_room, ROOM_NOPHYSIC))
            continue;

        if ((_type == LocalTeleport || _type == Teleport)
            && ROOM_FLAGGED(to_room, ROOM_NOMAGIC | ROOM_NOTEL))
            continue;

        if (ROOM_FLAGGED(to_room, ROOM_GODROOM | ROOM_NORECALL | ROOM_DEATH))
            continue;

        if (!can_travel_sector(_char, SECT_TYPE(to_room), 0))
            continue;

        found = true;
    }
            
    if (count >= 1000 || !found || !to_room)
        to_room = _char->in_room;

    act("$n disappears in a whirling flash of light.", false, 
            _victim, 0, 0, TO_ROOM);
    addToVictMessage("Your vision slowly fades to blackness...");
    addToVictMessage("A new scene unfolds before you!\r\n");
    addToRoomMessage("$N appears out of a whirling flash.");
    char_from_room(_victim);
    char_to_room(_victim, to_room);

    ExecutableObject::execute();

    look_at_room(_victim, _victim->in_room, 0);
}

bool
TeleportObject::teleportOK() {
    if (!_char->isImmortal() && _victim->isImmortal())
        return false;

    if (!_victim->isNPC() &&
			_victim->desc &&
			_victim->desc->input_mode == CXN_NETWORK)
        return false;

    if (_victim->trusts(_char))
        return true;

    int level = 49;

    if (_char && _type == LocalTeleport)
        level = _char->getLevelBonus(SPELL_LOCAL_TELEPORT);
    else if (_char && _type == Teleport)
        level = _char->getLevelBonus(SPELL_TELEPORT);
    else if (_char && _type == RandomCoordinates)
        level = _char->getLevelBonus(SPELL_RANDOM_COORDINATES);
    else if (_char && _type == Summon)
        level = _char->getLevelBonus(SPELL_SUMMON);

    return !mag_savingthrow(_victim, level, SAVING_SPELL);
}

void
TeleportObject::vstoneExecute() {
    struct room_data *load_room = NULL, *was_in = NULL;

    if (!IS_OBJ_TYPE(_obj, ITEM_VSTONE)
        || (GET_OBJ_VAL(_obj, 2) != -1 && !GET_OBJ_VAL(_obj, 2))) {
        addToCharMessage("Nothing seems to happen.");
        return;
    }

    if (GET_OBJ_VAL(_obj, 1) > 0 && GET_OBJ_VAL(_obj, 1) != GET_IDNUM(_char)) {
        addToCharMessage("$p hums loudly and zaps you!");
        addToRoomMessage("$p hums loudly and zaps $n!");
        return;
    }

    if (GET_OBJ_VAL(_obj, 0) == 0 ||
        (load_room = real_room(GET_OBJ_VAL(_obj, 0))) == NULL) {
        addToCharMessage("$p is not linked with a real location.");
        return;
    }

    if (load_room->zone != _char->in_room->zone && 
        (ZONE_FLAGGED(load_room->zone, ZONE_ISOLATED) ||
         ZONE_FLAGGED(_char->in_room->zone, ZONE_ISOLATED))) {
        addToCharMessage("You cannot get to there from here.");
    }

    addToCharMessage("You slowly fade into nothingness.");
    act("$p glows brightly as $n fades from view.", FALSE, _char, _obj, 0,
            TO_ROOM);

    was_in = _char->in_room;
    char_from_room(_char);
    char_to_room(_char, load_room);
    load_room->zone->enter_count++;

    if (GET_OBJ_VAL(_obj, 2) > 0)
        GET_OBJ_VAL(_obj, 2)--;

    if (!House_can_enter(_char, _char->in_room->number)
        || (ROOM_FLAGGED(_char->in_room, ROOM_GODROOM)
            && GET_LEVEL(_char) < LVL_CREATOR)
        || ROOM_FLAGGED(_char->in_room,
            ROOM_DEATH | ROOM_NOTEL | ROOM_NOMAGIC | ROOM_NORECALL)
        || (ZONE_FLAGGED(was_in->zone, ZONE_ISOLATED)
            && was_in->zone != load_room->zone)) {
        addToCharMessage("Your gut wrenches as you are slung violently "
                "through spacetime.");
        act("$n is jerked violently back into the void!", FALSE, _char, 0, 0,
            TO_ROOM);

        char_from_room(_char, false);
        char_to_room(_char, was_in, false);

        addToRoomMessage("$n reappears, clenching $s gut in pain."); 
        GET_MOVE(_char) = MAX(0, GET_MOVE(_char) - 30);
        GET_MANA(_char) = MAX(0, GET_MANA(_char) - 30);
        return;
    }

    addToRoomMessage("$n slowly fades into view, $p brightly glowing.");

    return;
}

bool
TeleportObject::summonExecute() {
    int level = _char->getLevelBonus(SPELL_SUMMON);

    if (_char->in_room == _victim->in_room) {
        addToCharMessage("Nothing happens.");
        return false;
    }

    if (ROOM_FLAGGED(_victim->in_room, ROOM_NORECALL)) {
        addToVictMessage("You fade out for a moment...");
        addToVictMessage("The magic quickly dissipates!");
        addToRoomMessage("$N fades out for a moment but quickly flickers back "
                "into view.");
        addToCharMessage("You failed.");
        return false;
    }

    if (!_victim->isNPC() && _victim->distrusts(_char)) {
        addToCharMessage("They must trust you to be summoned to this place.");
        return false;
    }

    if (ROOM_FLAGGED(_char->in_room, ROOM_NORECALL)) {
        addToCharMessage("This magic cannot penetrate here!");
        return false;
    }

    if (_char->in_room->people.size() >= 
            (unsigned)_char->in_room->max_occupancy) {
        addToCharMessage("This room is too crowded to summon anyone!");
        return false;
    }

    if (_victim->getLevel() > 
            MIN(LVL_AMBASSADOR - 1, level + _char->getInt())) {
        addToCharMessage("You failed.");
        return false;
    }

    if (!teleportOK()) {
        addToCharMessage("$N resists your attempt to summon $M!");
        addToVictMessage("You resist $n's attempt to summon you!");
        slog("%s failed summoning %s to %s[%d]", _char->getName(),
            _victim->getName(), _char->in_room->name, _char->in_room->number);
    }

    //FIXME:  MOB_NOSUMMON needs to be handled by perm mob affects
   
    if ((ROOM_FLAGGED(_char->in_room, ROOM_ARENA)
        && _char->in_room->zone->number == 400)
        || (ROOM_FLAGGED(_victim->in_room, ROOM_ARENA)
        && _victim->in_room->zone->number == 400)) {
        addToCharMessage("You cannot summon players into or out of arenas.");

        if (_char != _victim)
            addToVictMessage("$n has attempted to summon you.");
        return false;
    }

    if (_char != _victim && ROOM_FLAGGED(_char->in_room, ROOM_CLAN_HOUSE)
        && !clan_house_can_enter(_victim, _char->in_room)) {
        addToCharMessage("You cannot summon non-members into the clan house.");
        addToVictMessage("$n has attempted to summon you to $s clan house!!");
        mudlog(MAX(GET_INVIS_LVL(_char), GET_INVIS_LVL(_victim)), CMP, true,
               "%s has attempted to summon %s into %s (clan).",
               GET_NAME(_char), GET_NAME(_victim), _char->in_room->name);
        return false;
    }

    if (_char != _victim && ROOM_FLAGGED(_victim->in_room, ROOM_CLAN_HOUSE) &&
        !clan_house_can_enter(_char, _victim->in_room) &&
        !PLR_FLAGGED(_victim, PLR_KILLER)) {
        addToCharMessage("You cannot summon clan members from "
            "their clan house.");
        tmp_sprintf("$n has attempted to summon you to %s!!\r\n"
            "$e failed because you are in your clan house.",
            _char->in_room->name);
        mudlog(MAX(GET_INVIS_LVL(_char), GET_INVIS_LVL(_victim)), CMP, true,
            "%s has attempted to summon %s from %s (clan).",
            GET_NAME(_char), GET_NAME(_victim), _victim->in_room->name);

        return false;
    }

    if (_victim->in_room->zone != _char->in_room->zone
        && (ZONE_FLAGGED(_victim->in_room->zone, ZONE_ISOLATED)
        || ZONE_FLAGGED(_char->in_room->zone, ZONE_ISOLATED))) {
        addToCharMessage("The place $E exists is completely "
                "isolated from this.");
        return false;
    }

    room_data *targ_room = NULL;
    if (GET_PLANE(_victim->in_room) != GET_PLANE(_char->in_room)) {
        if (GET_PLANE(_char->in_room) == PLANE_DOOM) {
            addToCharMessage("You cannot summon characters into VR.");
            return false;
        }
        else if (GET_PLANE(_victim->in_room) == PLANE_DOOM) {
            addToCharMessage("You cannot summon characters out of VR.");
            return false;
        }
        else if (GET_PLANE(_victim->in_room) != PLANE_ASTRAL) {
            if (number(0, 120) > 
                    (_char->checkSkill(SPELL_SUMMON) + _char->getInt())) {
                if ((targ_room = real_room(number(41100, 41863))) != NULL) {
                    addToCharMessage("You fail, sending $N hurtling into the "
                        "Astral Plane!!!");
                    addToVictMessage("$n attempts to summon you, but something "
                        "goes wrong!!");
                    addToVictMessage("You are sent hurtling into the Astral "
                        "Plane!!");
                    act("$N is suddenly sucked into an astral void.",
                        true, _char, 0, _victim, TO_NOTVICT);

                    char_from_room(_victim);
                    char_to_room(_victim, targ_room);
                    addToRoomMessage("$N is suddenly pulled into the Astral "
                        "Plane!");
                    return true;
                }
            }
        }
    }

    int prob = 0;
    CreatureList::iterator it = _victim->in_room->people.begin();
    for (; it != _victim->in_room->people.end(); ++it) {
        if (AFF3_FLAGGED((*it), AFF3_SHROUD_OBSCUREMENT))
            prob +=
                (*it) == _victim ? (*it)->getLevel() : (*it)->getLevel() >> 1;
    }

    if (GET_PLANE(_victim->in_room) != GET_PLANE(_char->in_room))
        prob += GET_LEVEL(_victim) >> 1;

    if (GET_TIME_FRAME(_char->in_room) != GET_TIME_FRAME(_victim->in_room))
        prob += GET_LEVEL(_victim) >> 1;

    if (prob > number(10, _char->checkSkill(SPELL_SUMMON))) {
        addToCharMessage("You cannot discern the location of your victim.");
        return false;
    }

    act("$n disappears suddenly.", true, _victim, NULL, NULL, TO_ROOM);

    char_from_room(_victim);
    char_to_room(_victim, _char->in_room);

    addToCharMessage("$N arrives suddenly.");
    addToRoomMessage("$N arrives suddenly.");
    addToVictMessage("$n has summoned you!");

    if (!_victim->isNPC()) {
        mudlog(LVL_AMBASSADOR, BRF, true, "%s has%s summoned %s to %s (%d)",
               _char->getName(), (_victim->distrusts(_char)) ? " forcibly" : "",
               _victim->getName(), _char->in_room->name, 
               _char->in_room->number);
    }

    return true;
}

// *************************************************************
//
// SummonsObject
//
// *************************************************************

void
SummonsObject::execute() {
    Creature *mob;

    int level = _char->getLevelBonus(SPELL_CONJURE_ELEMENTAL);

    if (!(mob = getCreatureType())) {
        if (_type == ConjureElemental)
            addToCharMessage("You were unable to make the conjuration.");
        else
            addToCharMessage("You lose your concentration!");

        ExecutableObject::execute();
        return;
    }

    setVictim(mob);

    float mult = MAX(0.5, (float)(level * 1.5) / 100);

    int stat;
    stat = (int)MIN((int)mob->getHitroll() * mult, 60);
    mob->setHitroll(stat);
    
    stat = (int)MIN((int)mob->getDamroll() * mult, 75);
    mob->setDamroll(stat);

    stat = (int)MIN((int)mob->getMaxHit() * mult, 30000);
    mob->setMaxHit(stat);

    char_to_room(mob, _char->in_room);

    if (_type == ConjureElemental) {
        addToCharMessage("You have conjured $N from $S home plane!");
        addToRoomMessage("$n has conjured $N from $S home plane!");
    }
    else {
        addToCharMessage("You have summoned a follower!");
        addToCharMessage("$n has summoned a follower!");
    }

    SET_BIT(MOB_FLAGS(mob), MOB_PET);

    if ((number(0, 101) + mob->getLevel() - _char->getLevel() - 
        _char->getInt() - _char->getWis()) > 60
        || !can_charm_more(_char)) {
        addToCharMessage("Uh oh!  $N doesn't look happy with you!");
        addToRoomMessage("$N doesn't look happy with $n!");
        mob->startHunting(_char);
        remember(mob, _char);
    }
    else {
        if (mob->master)
            stop_follower(mob);

        add_follower(mob, _char);

        AffectObject *ao = new AffectObject("CharmAffect", _char, mob);
        CreatureAffect *ca = (CreatureAffect *)ao->getAffect();
        ca->setLevel(_char->getLevelBonus(SPELL_CHARM));
        ca->setDuration(_char->getLevel() * 18 / 1 + mob->getInt());
        getEV()->push_back_id(ao);
    }

    ExecutableObject::execute();
}

Creature *
SummonsObject::getCreatureType() {
    Creature *mob = NULL;
    int level = _char ? _char->getLevelBonus(SPELL_CONJURE_ELEMENTAL) : 49;

    if (_type == ConjureElemental) {
        int sect_type = _char->in_room->sector_type;
        
        if (level >= 50
            && number(0, _char->getInt()) > 3
            && sect_type != SECT_WATER_SWIM
            && sect_type != SECT_WATER_NOSWIM
            && sect_type != SECT_UNDERWATER
            && sect_type != SECT_DEEP_OCEAN) {
            // Air Elemental
            mob = read_mobile(1283); 
        }
        else if (level >= 40
            && number(0, _char->getInt()) > 3
            && (sect_type == SECT_WATER_SWIM
            || sect_type == SECT_WATER_NOSWIM
            || sect_type == SECT_UNDERWATER
            || sect_type == SECT_DEEP_OCEAN)) {
            // Water Elemental
            mob = read_mobile(1282); 
        }
        else if (level >= 30
            && number(0, _char->getInt()) > 3
            && sect_type != SECT_WATER_SWIM
            && sect_type != SECT_WATER_NOSWIM
            && sect_type != SECT_UNDERWATER
            && sect_type != SECT_DEEP_OCEAN
            && !_char->in_room->isOpenAir()) {
            // Fire Elemental
            mob = read_mobile(1281);
        }
        else if (level >= 20
            && number(0, _char->getInt()) > 3
            && sect_type != SECT_WATER_SWIM
            && sect_type != SECT_WATER_NOSWIM
            && sect_type != SECT_UNDERWATER
            && sect_type != SECT_DEEP_OCEAN
            && !_char->in_room->isOpenAir()) {
            // Earth Elemental
            mob = read_mobile(1280);
        }
        else {
            mob = NULL;
        }
    }

    return mob;
}

////////////////////////////////////////////////////
// PositionObject
////////////////////////////////////////////////////

void
PositionObject::execute() {
    _victim->setPosition(getPosition());

    ExecutableObject::execute();
}

////////////////////////////////////////////////////
// StopCombatObject
////////////////////////////////////////////////////

void
StopCombatObject::execute() {
    getVictim()->removeCombat(getChar());
    if (!getVictimOnly()) {
        getChar()->removeCombat(getVictim());
    }

    ExecutableObject::execute();
}

////////////////////////////////////////////////////
// StartCombatObject
////////////////////////////////////////////////////

void
StartCombatObject::execute() {
    getChar()->addCombat(getVictim(), true);
    getVictim()->addCombat(getChar(), false);

    ExecutableObject::execute();
}

////////////////////////////////////////////////////
// ModifyItemObject
////////////////////////////////////////////////////

void
ModifyItemObject::execute() {
    if (_damChange)
        _obj->obj_flags.damage += _damChange;

    if (_maxDamChange)
        _obj->obj_flags.max_dam += _damChange;

    if (_addToWear) {
        check_bits_32(_obj->obj_flags.wear_flags, &_addToWear);
        _obj->obj_flags.wear_flags |= _addToWear;
    }

    if (_removeFromWear)
        _obj->obj_flags.wear_flags &= ~(_removeFromWear);

    if (_weightModifier)
        _obj->setWeight(_obj->getWeight() + _weightModifier);

    for (int x = 0; x < 4; x++) {
        if (_valueModifier[x] != -127)
            _obj->obj_flags.value[x] = _valueModifier[x];
    }

    if (_typeModifier)
        _obj->obj_flags.type_flag = _typeModifier;

    if (_addBits[0]) {
        check_bits_32(_obj->obj_flags.extra_flags, &_addBits[0]);
        _obj->obj_flags.extra_flags |= _addBits[0];
    }

    if (_addBits[1]) {
        check_bits_32(_obj->obj_flags.extra2_flags, &_addBits[1]);
        _obj->obj_flags.extra2_flags |= _addBits[1];
    }

    if (_addBits[2]) {
        check_bits_32(_obj->obj_flags.extra3_flags, &_addBits[2]);
        _obj->obj_flags.extra3_flags |= _addBits[2];
    }

    if (_delBits[0]) 
        _obj->obj_flags.extra_flags &= ~(_delBits[0]);

    if (_delBits[1]) 
        _obj->obj_flags.extra2_flags &= ~(_delBits[1]);

    if (_delBits[2]) 
        _obj->obj_flags.extra3_flags &= ~(_delBits[2]);

    vector<locModType>::iterator vi;
    for (vi = _affectMod.begin(); vi != _affectMod.end(); ++vi) {
        int i; 
        if (vi->location != APPLY_NONE) {
            bool found = false;
            for (i = 0; i < MAX_OBJ_AFFECT; i++) {
                if (_obj->affected[i].location == vi->location) {
                    found = true;
                    _obj->affected[i].modifier = MAX(-125, 
                            MIN(125, vi->modifier + _obj->affected[i].modifier));
                    break;
                }
            }

            if (found) {
                if (_obj->affected[i].modifier == 0)
                    _obj->affected[i].location = APPLY_NONE;
            }

            else {
                for (i = 0; i < MAX_OBJ_AFFECT; i++) {
                    if (_obj->affected[i].location == APPLY_NONE) {
                        found = true;
                        _obj->affected[i].location = vi->location;
                        _obj->affected[i].modifier = vi->modifier;
                    }
                }
            }

            _obj->normalizeApplies();
        }
    }

    ExecutableObject::execute();
}

// *************************************************************
//
// DestroyItemObject
//
// *************************************************************

void 
DestroyItemObject::execute() {
    char *material, *msg;
    int mat_idx;
    obj_data *new_obj;

    if (!_obj) {
        errlog("No object in DestroyItemObject::execute()");
        return;
    }
    
    mat_idx = GET_OBJ_MATERIAL(_obj);

    if (mat_idx) 
        material = tmp_strdup(material_names[mat_idx]);
    else
        material = tmp_strdup("material");

    new_obj = create_obj();
    new_obj->shared = null_obj_shared;
    GET_OBJ_MATERIAL(new_obj) = mat_idx;
    new_obj->setWeight(_obj->getWeight());
    GET_OBJ_TYPE(new_obj) = ITEM_TRASH;
    GET_OBJ_WEAR(new_obj) = ITEM_WEAR_TAKE;
    GET_OBJ_EXTRA(new_obj) = ITEM_NODONATE + ITEM_NOSELL;
    GET_OBJ_VAL(new_obj, 0) = _obj->shared->vnum;
    GET_OBJ_MAX_DAM(new_obj) = 100;
    GET_OBJ_DAM(new_obj) = 100;

    if (_type == SPELL_OXIDIZE && !IS_BURNABLE_TYPE(_obj)) {
        msg = tmp_sprintf("$p dissolve%s into a pile of rust", PLUR(_obj->getName()) ? "" : "s");
        new_obj->aliases = str_dup("pile rust");
        new_obj->name = str_dup("a pike of rust");
        new_obj->line_desc = str_dup(tmp_capitalize(tmp_strcat(new_obj->name, " is lying here.")));
        GET_OBJ_MATERIAL(new_obj) = MAT_RUST;
    }
    else if (_type == SPELL_OXIDIZE && IS_BURNABLE_TYPE(_obj)) {
        msg = tmp_sprintf("$p %s incinerated!!", ISARE(_obj->getName()));
        new_obj->aliases = str_dup("pile ash");
        new_obj->name = str_dup("a pile of ash");
        new_obj->line_desc = str_dup(tmp_capitalize(tmp_strcat(new_obj->name, " is lying here.")));
        GET_OBJ_MATERIAL(new_obj) = MAT_ASH;
    }
    if (_type == SPELL_ACIDITY || _type == TYPE_ACID_BURN && !IS_CORPSE(_obj)) {
        if (!IS_BURNABLE_TYPE(_obj)) {
            msg = tmp_sprintf("$p melt%s into a sizzling pool of %s!!", material,
                    PLUR(_obj->getName()) ? "" : "s");
        }
        else {
            msg = tmp_sprintf("$p %s dissolved completely by the acid!!", ISARE(_obj->getName()));
        }
        // Nothing else matters.  We're not even going to leave the remains behind...
    }

    else if (_type == SPELL_BLESS) {
        msg = tmp_sprintf("$p glow%s bright blue and shatters into pieces!!",
                PLUR(_obj->getName()) ? "" : "s");
        new_obj->aliases = str_dup(tmp_sprintf("%s %s shattered fragments",
                    material_names[mat_idx], _obj->aliases));
        new_obj->name = str_dup(tmp_strcat("shattered fragments of ", material));
        new_obj->line_desc = str_dup(tmp_capitalize(tmp_strcat(new_obj->name, " are lying here.")));
    }
    else if (_type == SPELL_DAMN) {
        msg = tmp_sprintf("$p glow%s bright red and shatters into pieces!!",
                PLUR(_obj->getName()) ? "" : "s");
        new_obj->aliases = str_dup(tmp_sprintf("%s %s shattered fragments",
                    material_names[mat_idx], _obj->aliases));
        new_obj->name = str_dup(tmp_strcat("shattered fragments of ", material));
        new_obj->line_desc = str_dup(tmp_capitalize(tmp_strcat(new_obj->name, " are lying here.")));
    }
    else if (IS_METAL_TYPE(_obj)) {
        msg = tmp_sprintf("$p %s reduced to a mangled heap of scrap!!", ISARE(_obj->getName()));
        new_obj->aliases = str_dup(tmp_sprintf("%s %s mangled heap",
                    material_names[mat_idx], _obj->aliases));
        new_obj->name = str_dup(tmp_strcat("a mangled heap of ", material));
        new_obj->line_desc = str_dup(tmp_capitalize(tmp_strcat(new_obj->name, " is lying here.")));
    }
    else if (IS_STONE_TYPE(_obj) || IS_GLASS_TYPE(_obj)) {
        msg = tmp_sprintf("$p shatter%s into a thousand fragments!!", 
                PLUR(_obj->getName()) ? "" : "s");
        new_obj->aliases = str_dup(tmp_sprintf("%s %s shattered fragments",
                    material_names[mat_idx], _obj->aliases));
        new_obj->name = str_dup(tmp_strcat("a shattered fragments of ", material));
        new_obj->line_desc = str_dup(tmp_capitalize(tmp_strcat(new_obj->name, " are lying here.")));
    }
    else {
        msg = tmp_sprintf("$p %s been destroyed!!", HAS_HAVE(_obj->getName()));
        new_obj->aliases = str_dup(tmp_sprintf("%s %s mutilated heap",
                    material_names[mat_idx], _obj->aliases));
        new_obj->name = str_dup(tmp_strcat("a mutilated heap of ", material));
        new_obj->line_desc = str_dup(tmp_capitalize(tmp_strcat(new_obj->name, " is lying here.")));

        if (IS_CORPSE(_obj)) {
            GET_OBJ_TYPE(new_obj) = ITEM_CONTAINER;
            GET_OBJ_VAL(new_obj, 0) = GET_OBJ_VAL(_obj, 0);
            GET_OBJ_VAL(new_obj, 1) = GET_OBJ_VAL(_obj, 1);
            GET_OBJ_VAL(new_obj, 2) = GET_OBJ_VAL(_obj, 2);
            GET_OBJ_VAL(new_obj, 3) = GET_OBJ_VAL(_obj, 3);
            GET_OBJ_TIMER(new_obj) = GET_OBJ_TIMER(_obj);
        }
    }

    if (IS_OBJ_STAT2(_obj, ITEM2_IMPLANT))
        SET_BIT(GET_OBJ_EXTRA2(new_obj), ITEM2_IMPLANT);

    if (_victim || (_victim = _obj->worn_by) || (_victim = _obj->carried_by)) {
        addToVictMessage(msg);
        addToRoomMessage(msg, TO_VICT_RM);
    }
    else if (!_obj->in_obj && _obj->in_room && (_victim = _obj->in_room->people)) {
        addToVictMessage(msg);
        addToRoomMessage(msg, TO_VICT_RM);
    }

    ExecutableObject::execute();
    
    if (!_obj->shared->proto) {
        eqdam_extract_obj(_obj);
        extract_obj(new_obj);
        return;
    }

    if (_type == SPELL_ACIDITY || _type == TYPE_ACID_BURN && !IS_CORPSE(_obj)) {
        eqdam_extract_obj(_obj);
        extract_obj(new_obj);
        return;
    }

    if ((_victim = _obj->worn_by) || (_victim = _obj->carried_by)) {
        int pos;
        eqdam_extract_obj(_obj);
        if ((pos = _obj->getImplantPos())) {
            equip_char(_victim, new_obj, pos, MODE_IMPLANT);
            return;
        }
        else
            obj_to_char(new_obj, _victim);
    }
    else if (_obj->in_room) {
        room_data *room = _obj->in_room;
        eqdam_extract_obj(_obj);
        obj_to_room(new_obj, room);
    }
    else if (_obj->in_obj) {
        obj_data *in_obj = _obj->in_obj;
        eqdam_extract_obj(_obj);
        obj_to_obj(new_obj, in_obj);
    }
    else {
        errlog("Invalid location of obj and new_obj in DestroyItemObject::execute()");
        eqdam_extract_obj(_obj);
    }
}

//**************************************************************************
//
// DistractionObject
//
//**************************************************************************

void
DistractionObject::execute() {
    memory_rec *curr, *next_curr;

    if (!_victim)
        return;

    for (curr = MEMORY(_victim); curr; curr = next_curr) {
        next_curr = curr->next;
        free(curr);
    }

    MEMORY(_victim) = NULL;
    _victim->stopHunting();

    ExecutableObject::execute();
    return;
}

//**************************************************************************
//
// SatiationObject
//
//**************************************************************************

void
SatiationObject::execute() {

    if (!_victim)
            return;

    if (_drunk) {
        if (!(GET_COND(_victim, DRUNK) < 0)) {
            GET_COND(_victim, DRUNK) += _drunk;
            GET_COND(_victim, DRUNK) = MAX(0, GET_COND(_victim, DRUNK));
            GET_COND(_victim, DRUNK) = MIN(24, GET_COND(_victim, DRUNK));
        }
    }
    if (_thirst) {
        if (!(GET_COND(_victim, FULL) < 0)) {
            GET_COND(_victim, FULL) += _hunger;
            GET_COND(_victim, FULL) = MAX(0, GET_COND(_victim, FULL));
            GET_COND(_victim, FULL) = MIN(24, GET_COND(_victim, FULL));
        }
    }
    if (_thirst) {
        if (!(GET_COND(_victim, THIRST) < 0)) {
            GET_COND(_victim, THIRST) += _thirst;
            GET_COND(_victim, THIRST) = MAX(0, GET_COND(_victim, THIRST));
            GET_COND(_victim, THIRST) = MIN(24, GET_COND(_victim, THIRST));
        }
    }

    ExecutableObject::execute();
}

//**************************************************************************
//
// PointsObject
//
//**************************************************************************

void
PointsObject::execute() {

    if (!_victim)
        return;

    if (_hit)
        GET_HIT(_victim) = MIN(GET_MAX_HIT(_victim), GET_HIT(_victim) + _hit);
    if (_mana)
        GET_MANA(_victim) = MIN(GET_MAX_MANA(_victim), GET_MANA(_victim) + _mana);
    if (_move)
        GET_MOVE(_victim) = MIN(GET_MAX_MOVE(_victim), GET_MOVE(_victim) + _hit);

    ExecutableObject::execute();
}

//**************************************************************************
//
// NullPsiObject
//
//**************************************************************************

void
NullPsiObject::execute() {
    const GenericSkill *skill;
    int prob = 0;

    if (!_victim)
        return;

    CreatureAffectList::iterator li = _victim->getAffectList()->begin();
    for (; li != _victim->getAffectList()->end(); ++li) {
        if (!(*li))
            continue;

        if (dynamic_cast<EquipAffect *>((*li)) || dynamic_cast<MobAffect *>((*li)))
            continue;

        int num = (*li)->getSkillNum();
        skill = GenericSkill::getSkill(num);

        if (!skill) {
            errlog("Affect applied by invalid skill in NullPsiObject::execute()");
            continue;
        }

        if (skill->isPsionic()) {
            if (_char)
                prob = number(_char->getLevelBonus(SPELL_NULLPSI) << 2, 
                        _char->getLevelBonus(SPELL_NULLPSI));
            else
                prob = number(1, 70);

            if ((*li)->getLevel() < prob)
                (*li)->affectRemove();
        }
    }

    ExecutableObject::execute();
}
