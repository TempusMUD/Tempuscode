#include <signal.h>
#include "creature.h"
#include "spells.h"
#include "screen.h"
#include "comm.h"
#include "utils.h"
#include "fight.h"
#include "executable_object.h"

using namespace std;

extern bool ok_damage_vendor(Creature *ch, Creature *victim);
//extern void update_pos(struct Creature *victim);

struct damage_word_type {
    char *self;
    char *others;
};

extern const char *hit_location_translator[];
extern const struct attack_hit_type attack_hit_text[];
//extern struct damage_word_type;
extern const struct damage_word_type damage_words[];

// Normal Constructor
ExecutableObject::ExecutableObject()
{
    _modifiable = true;
    _executed = false;
    _char = NULL;
    _victim = NULL;
    _obj = NULL;
    _toCharMessages.clear();
    _toVictMessages.clear();
    _toRoomMessages.clear();
}

// Normal Destructor
ExecutableObject::~ExecutableObject()
{
}

// Copy constructor
ExecutableObject::ExecutableObject(const ExecutableObject &c) : _toCharMessages(c._toCharMessages),
                                                                _toVictMessages(c._toVictMessages),
                                                                _toRoomMessages(c._toRoomMessages)
{
    _modifiable = c.getModifiable();
    _executed = c.getExecuted();
    _char = c.getChar();
    _victim = c.getVictim();
    _obj = c.getObject();
}


// Operators 
ExecutableObject 
&ExecutableObject::operator =(ExecutableObject &c)
{
    _modifiable = c.getModifiable();
    _executed = c.getExecuted();
    _char = c.getChar();
    _victim = c.getVictim();
    _obj = c.getObject();
    _toCharMessages = c._toCharMessages;
    _toVictMessages = c._toVictMessages;
    _toRoomMessages = c._toRoomMessages;
    return *this;
}

bool 
ExecutableObject::operator ==(ExecutableObject &c) const
{
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
ExecutableObject::setModifiable(bool modifiable)
{
    _modifiable = modifiable;
}

void
ExecutableObject::setExecuted(bool executed)
{
    _executed = executed;
}

void
ExecutableObject::setChar(Creature *ch)
{
    _char = ch;
}

void
ExecutableObject::setVictim(Creature *victim)
{
    _victim = victim;
}

void
ExecutableObject::setObject(obj_data *obj)
{
    _obj = obj;
}
    
int 
ExecutableObject::addToCharMessage(string message)
{
    _toCharMessages.push_back(message);

    return _toCharMessages.size();
}

int
ExecutableObject::addToVictMessage(string message)
{
    _toVictMessages.push_back(message);

    return _toVictMessages.size();
}

int
ExecutableObject::addToRoomMessage(string message)
{
    _toRoomMessages.push_back(message);

    return _toRoomMessages.size();
}

int
ExecutableObject::removeToCharMessage(int index)
{
    vector<string>::iterator vi = _toCharMessages.begin();

    for (int x = 0; x <= index; ++x, ++vi);

    _toCharMessages.erase(vi);

    return _toCharMessages.size();
}

int
ExecutableObject::removeToVictMessage(int index)
{
    vector<string>::iterator vi = _toVictMessages.begin();

    for (int x = 0; x <= index; ++x, ++vi);

    _toVictMessages.erase(vi);

    return _toVictMessages.size();
}

int
ExecutableObject::removeToRoomMessage(int index)
{
    vector<string>::iterator vi = _toRoomMessages.begin();

    for (int x = 0; x <= index; ++x, ++vi);

    _toRoomMessages.erase(vi);

    return _toRoomMessages.size();
}

bool 
ExecutableObject::getModifiable() const
{
    return _modifiable;
}

bool
ExecutableObject::getExecuted() const
{
    return _executed;
}

bool
ExecutableObject::isVictimDead() const
{
    return _victimKilled;
}

Creature *
ExecutableObject::getChar() const
{
    return _char;
}

Creature *
ExecutableObject::getVictim() const
{
    return _victim;
}

obj_data *
ExecutableObject::getObject() const
{
    return _obj;
}

// Normal members
void 
ExecutableObject::execute()
{
    sendMessages();
    setExecuted();
}

void 
ExecutableObject::sendMessages()
{
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
    for (unsigned int x = 0; x < _toRoomMessages.size(); x++)
        act(_toRoomMessages[x].c_str(), false, _char, _obj, 
            _victim, TO_NOTVICT | TO_SLEEP);
}


// Normal constructor & destructor
DamageObject::DamageObject() 
{
    ExecutableObject::ExecutableObject();
    _damage = 0;
    _damageType = TYPE_HIT;
    _damageLocation = WEAR_BODY; //default to body incase it doesn't get set;
    _victimKilled = false;
    _damageFailed = false;
    _genMessages = true;
}

DamageObject::~DamageObject()
{
};

// Copy constructor
DamageObject::DamageObject(const DamageObject &c) : ExecutableObject(c)
{
    _damage = c.getDamage();
    _damageType = c.getDamageType();
    _damageLocation = c.getDamageLocation();
    _victimKilled = c.isVictimDead();
    _damageFailed = c.getDamageFailed();
    _genMessages = c.getGenMessages();
}

// Operators
DamageObject &DamageObject::operator =(DamageObject &c)
{
    ExecutableObject::operator =(c);
    _damage = c.getDamage();
    _damageType = c.getDamageType();
    _damageLocation = c.getDamageLocation();
    _victimKilled = c.isVictimDead();
    _damageFailed = c.getDamageFailed();
    _genMessages = c.getGenMessages();
    
    return *this;
}

bool DamageObject::operator >(DamageObject &c) const
{
    if (_damage > c.getDamage())
        return true;

    return false;
}

bool DamageObject::operator <(DamageObject &c) const
{
    if (_damage < c.getDamage())
        return true;

    return false;
}

bool 
DamageObject::operator ==(DamageObject &c) const
{
    if (!ExecutableObject::operator ==(c))
        return false;
    
    if (_damage != c.getDamage())
        return false;

    if (_damageType != c.getDamageType())
        return false;

    if (_damageLocation != c.getDamageLocation())
        return false;

    if (_victimKilled != c.isVictimDead())
        return false;

    if (_damageFailed != c.getDamageFailed())
        return false;

    if (_genMessages != c.getGenMessages())
        return false;

    return true;
}

// Accessors
void 
DamageObject::setDamage(int damage)
{
    _damage = damage;
}

void 
DamageObject::setDamageType(int type)
{
    _damageType = type;
}

void 
DamageObject::setDamageLocation(int location)
{
    _damageLocation = location;
}

void 
DamageObject::setVictimKilled(bool victimKilled)
{
    _victimKilled = victimKilled;
}

void 
DamageObject::setDamageFailed(bool damageFailed)
{
    _damageFailed = damageFailed;
}

void
DamageObject::setGenMessages(bool gen) 
{
    _genMessages = gen;
}

void
DamageObject::setWeapon(obj_data *weap)
{
    _obj = weap;
}

Creature *
DamageObject::getAttacker() const
{
    return getChar();
}

int 
DamageObject::getDamage() const
{
    return _damage;
}

int 
DamageObject::getDamageType() const
{
    return _damageType;
}

int 
DamageObject::getDamageLocation() const
{
    return _damageLocation;
}



bool 
DamageObject::getDamageFailed() const
{
    return _damageFailed;
}

bool
DamageObject::getGenMessages() const
{
    return _genMessages;
}

obj_data *
DamageObject::getWeapon() const
{
    return _obj;
}

// Normal members
void 
DamageObject::execute()
{
    if (isVictimDead()) {
        return;
    }
    
    string damager = _char ? _char->getName() : "NULL";

    // If there is no victim there's nothing to do here.  But since
    // that should never happen, let's log an error
    if (!_victim) {
        errlog("No victim in DamageObject::execute()");
        _damageFailed = true;
        return;
    }

    // If _victim is in the void, no damage sould be done
    if (_victim->in_room == zone_table->world) {
        _damageFailed = true;
        return;
    }

    // Is the victim in a room?
    if (_victim->in_room == NULL) {
        errlog("Attempt to damage a char with null in_room "
               "ch=%s, vict=%s, type=%d.", damager.c_str(), _victim->getName(),
               _damageType);
        raise(SIGSEGV);
    }

    // Is the victim already dead?
    if (_victim->getPosition() <= POS_DEAD) {
        errlog("Attempt to damage a corpse: ch=%s, vict=%s, type=%d.",
               damager.c_str(), _victim->getName(), _damageType);
        _victimKilled = true;
        _damageFailed = true;
        return;
    }

    // At -10 hp, a creature should die and be extracted.  So if we try
    // to damage someone with -10, that's also an error
    if (_victim->getHit() < -10) {
        errlog("Attempt to damage a creature with hps %d: "
               "ch=%s, vict=%s, type=%d.", _victim->getHit(), damager.c_str(), 
               _victim->getName(), _damageType);
        _victimKilled = true;
        _damageFailed = true;
        return;
    }

    // If the damagee is writing then we'll refuse to do the damage
    if (_victim->isWriting()) {
        _char->removeCombat(_victim);
        _victim->removeAllCombat();
        _damageFailed = true;
        send_to_char(_char, "NO!  Do you want to be ANNIHILATED by the gods?!\r\n");
        return;
    }

    if (!ok_damage_vendor(_char, _victim)) {
        _damageFailed = true;
        return;
    }

    // Enough of that crap!  Let's do some damage!
    // If the creatures postion is less than fighting, up the damage
    
    if (_victim->getPosition() < POS_FIGHTING)
        _damage += (_damage * (POS_FIGHTING - _victim->getPosition())) / 3;
    
    // Cap the damage
    cap();

    // Have the victim actually take what's left of the dealt damage
    _victim->takeDamage(_damage);
    update_pos(_victim);

    // The attacker should gain exp for damage delt
    int exp = MIN(_char->getLevel()^3, _victim->getLevel() * _damage);
    exp = _char->getPenalizedExperience(exp, _victim);
    
    if (_char->in_room == _victim->in_room)
        gain_exp(_char, exp);


    if (_genMessages)
        generateDamageMessages();

    ExecutableObject::execute();

    if (_victim->getPosition() == POS_DEAD) {
        _victimKilled = true;
        die(getVictim(), getAttacker(), getDamageType(), 
              (getAttacker()->isNPC()) ? true : false);
    }

}

void 
DamageObject::cap()
{
    // Cap the damage here;
    int hard_damcap;

    if (_char)
        hard_damcap = MAX(20 + _char->getLevel() + (_char->getLevel() * 2),
                          _char->getLevel() * 20 + (_char->getLevel() * 
                          _char->getLevel() * 2));
    else
        hard_damcap = 7000;

    _damage = MIN(_damage, hard_damcap);

    if (_damage < 0)
        _damage = 0;
}


void
DamageObject::sendDamageMessages()
{
    ExecutableObject::sendMessages();
}

void
DamageObject::generateDamageMessages()
{
    extern int search_nomessage;

    int damage_index;
    string message, loc_string(""), loc_string2(""), weap_string(""), weap_string2("");
    
    if (search_nomessage)
        return;
    
    if (_damage < 45)
        damage_index = (_damage / 5) + 1;
    else
        damage_index = MIN(22, (_damage / 10) + 1);

    if (_damageLocation && random_binary() &&
        strcmp("", hit_location_translator[_damageLocation])) {
        loc_string += "'s ";
        loc_string2 += " your ";
        loc_string += hit_location_translator[_damageLocation]; 
        loc_string2 += hit_location_translator[_damageLocation]; 
    }
    else
        loc_string2 = " you";

    weap_string += " with ";
    weap_string2 += " with ";
    if (random_binary() && _obj) {
        weap_string2 += " with ";
        weap_string += "a ";
        weap_string += attack_hit_text[_damageType].singular;
        weap_string += " from $p";
        weap_string2 = weap_string;
    }
    else {
        _obj = NULL;
        string mod("");
        weap_string += "your ";
        weap_string2 += "$s ";
        if (_damage < 20)   
            mod = "";
        else if (_damage < 40)
            mod = "powerful ";
        else if (_damage < 60)
            mod = "terrible ";
        else if (_damage < 80)
            mod = "horrible ";
        else if (_damage < 100)
            mod = "vicious ";
        else if (_damage < 120)
            mod = "unbelievable ";
        else
            mod = "ultra ";

        weap_string += mod;
        weap_string2 += mod;

        weap_string += attack_hit_text[_damageType].singular;
        weap_string2 += attack_hit_text[_damageType].singular;
    }
    weap_string += "!";
    weap_string2 += "!"; 

    message.clear();
    // To Char
    message += "You ";
    message += damage_words[damage_index].self;
    message += " $N";
    message += loc_string;
    message += weap_string;

    if (_damageLocation == WEAR_MSHIELD)
        message = string(CCMAG(_char, C_NRM)) + message + 
                         string(CCNRM(_char, C_NRM));
    else
        message = string(CCYEL(_char, C_NRM)) + message + 
                         string(CCNRM(_char, C_NRM));
    _toCharMessages.push_back(message);
    
    message.clear();
    // To Room
    message += "$n ";
    message += damage_words[damage_index].others;
    message += " $N";
    message += loc_string;
    message += weap_string2;

    _toRoomMessages.push_back(message);

    message.clear();
    // To Vict
    message += "$n ";
    message += damage_words[damage_index].others;
    message += loc_string2;
    message += weap_string2;

    if (_damageLocation == WEAR_MSHIELD)
        message = string(CCCYN(_victim, C_NRM)) + message + 
                         string(CCNRM(_victim, C_NRM));
    else
        message = string(CCRED(_victim, C_NRM)) + message + 
                         string(CCNRM(_victim, C_NRM));
    _toVictMessages.push_back(message);
}

/*************************************************************
                  Begin ExecutableVector stuff
 *************************************************************/

// Normal contstructor & destructor
ExecutableVector::ExecutableVector()
{
    _maxAttacks = 0;
}

ExecutableVector::~ExecutableVector()
{
    for (unsigned int x = 0; x < size(); x++) {
        delete (*this)[x];
    }
}

// Copy constructor
ExecutableVector::ExecutableVector(ExecutableVector &c)
{
    clear();
    ExecutableVector::iterator vi = this->begin();
    for (; vi != this->end(); ++vi)
        push_back(*vi);
    _maxAttacks = c.getMaxAttacks();
}

// Accessors
void 
ExecutableVector::setMaxAttacks(int maxAttacks)
{
    _maxAttacks = maxAttacks;
}

int 
ExecutableVector::getMaxAttacks()
{
    return _maxAttacks;
}

// Normal members        
void 
ExecutableVector::execute()
{
    for (unsigned int x = 0; x < size(); x++) {
        ExecutableObject *eo = (*this)[x];
        if (!eo->getExecuted()) {
            eo->execute();
            // Nathan, the code below HAS to be here.  Please
            // leave it.  And let's not use iterators over
            // vectors.  It's horribly inefficient compared to
            // using a simple integer
            if (eo->isVictimDead()) {
                for (unsigned int y = 0; y < size(); y++) {
                    ExecutableObject *eo2 = (*this)[y];
                    if (eo->getVictim() == eo2->getVictim())
                        eo2->setExecuted(true);
                }
            }
        }
    }
}
