#include <signal.h>
#include "creature.h"
#include "spells.h"
#include "comm.h"
#include "utils.h"
#include "fight.h"
#include "executable_object.h"
#include "macros.h"
#include "zone_data.h"
#include "obj_data.h"
#include "screen.h"
#include "custom_affects.h"

using namespace std;

extern bool ok_damage_vendor(Creature *ch, Creature *victim);
extern void gain_kill_exp(struct Creature *ch, struct Creature *victim);
extern void blood_spray(Creature *ch, Creature *victim, int dam, int w_type);
extern int choose_random_limb(Creature *victim);

struct damage_word_type {
    char *self;
    char *others;
};

extern const struct attack_hit_type attack_hit_text[];

const char *hit_location_translator[] = {
    "", //light
    "finger",
    "finger",
    "neck",
    "throat",
    "body",
    "head",
    "legs",
    "feet",
    "hands",
    "arms",
    "arms",
    "", //about
    "waist",
    "wrist",
    "wrist",
    "", //wield
    "", //hold
    "crotch",
    "face",
    "back",
    "abdomen",
    "face",
    "ear",
    "ear",
    "", //wield 2
    "", //wear ass
    "", // wear random
    "", // num_wears
    "mana shield"
};

const struct damage_word_type damage_words[] = {
    {"miss", "misses"},
    {"tickle", "tickles"},
    {"hit", "hits"},
    {"bruise", "bruises"},
    {"scratch", "scratches"},
    {"smite", "smites"},
    {"massacre", "massacres"},
    {"devastate", "devastates"},
    {"disfigure", "disfigures"},
    {"punish", "punishes"},
    {"mutilate", "mutilates"},
    {"OBLITERATE", "OBLITERATES"},
    {"DEMOLISH", "DEMOLISHES"},
    {"-SHATTER-", "-SHATTERS-"},
    {"-DESTROY-", "-DESTROYS-"},
    {"-+RAVAGE+-", "-+RAVAGES+-"},
    {"-+PULVERIZE+-", "-+PULVERIZES+-"},
    {"++SLAUGHTER++", "++SLAUGHTERS++"},
    {"++DECIMATE++", "++DECIMATES++"},
    {"*LIQUEFY*", "*LIQUEFIES*"},
    {"*DISINTIGRATE*", "*DISINTIGRATES*"},
    {"**VAPORIZE**", "**VAPORIZES**"},
    {"**ANNIHILATE**", "**ANNIHILATES**"},
};

// Normal constructor & destructor
DamageObject::DamageObject(Creature *character, Creature *vict) : ExecutableObject(character)
{
    _char = character;
    _victim = vict;
    _damage = 0;
    _manaLoss = 0;
    _damageType = TYPE_HIT;
    _damageLocation = WEAR_BODY; //default to body incase it doesn't get set;
    _victimKilled = false;
    _damageFailed = true;
    _genMessages = false;
    _initiateCombat = true;
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
    if (getExecuted())
        return;
    
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


    // All ok to damage checks should take place in this function which also
    // handles messaging.
    if (_char && !_char->isOkToAttack(_victim)) {
        _damageFailed = true;
        // This may be an overly paranoid check here but it won't hurt
        if (_victim->getPosition() <= POS_DEAD) {
            _victimKilled = true;
        }
        return;
    }
    
    // Enough of that crap!  Let's do some damage!
    // If the creatures postion is less than fighting, up the damage

    if (_char && _victim && _initiateCombat) {
        _char->addCombat(_victim, true);
        _victim->addCombat(_char, false);
        if (_victim->isNPC()) {
            if (MOB_FLAGGED(_victim, MOB_MEMORY))
                remember(_victim, _char);
            if (MOB2_FLAGGED(_victim, MOB2_HUNT))
                _victim->startHunting(_char);
        }
    }
    
    if (_victim->getPosition() < POS_STANDING)
        _damage += (_damage * (POS_STANDING - _victim->getPosition())) / 3;
    
    // Cap the damage
    cap();

    // Have the victim actually take what's left of the dealt damage
    CreatureAffect *aff = NULL;
    if (_damageLocation == WEAR_MSHIELD
        && (aff = (CreatureAffect *)_victim->affectedBy(SPELL_MANA_SHIELD))) {
        _manaLoss = (_damage * GET_MSHIELD_PCT(_victim)) / 100;
        _manaLoss = 
            MIN(GET_MANA(_victim) - GET_MSHIELD_LOW(_victim), _manaLoss);
        _manaLoss = MAX(_manaLoss, 0);
        _damage = MAX(0, _damage - _manaLoss);
        _manaLoss -= (int)(_manaLoss * MIN(_victim->getDamReduction(), 0.50));
        _victim->setMana(_victim->getMana() - _manaLoss);

        if (_victim->getMana() <= GET_MSHIELD_LOW(_victim)) {
            aff->affectModify(false);
            _victim->getAffectList()->remove(aff);
        }

        if (_manaLoss == 0)
            _damageLocation = WEAR_RANDOM;
        if (_damage)
            _victim->takeDamage(_damage);
    }
    else {
        _victim->takeDamage(_damage);
    }

    _damageFailed = false;
    
    if (_damageLocation == WEAR_RANDOM)
        _damageLocation = choose_random_limb(_victim);

    // Spray Blood if applicable.  We may want to move this to a creature
    // funciton when we start whacking macros
    if (_char && BLOODLET(_victim, _damage, _damageType))
        blood_spray(_char, _victim, _damage, _damageType);

    update_pos(_victim);

    // The attacker should gain exp for damage delt
    if (_char) {
        int exp = MIN(_char->getLevel()^3, _victim->getLevel() * _damage);
        exp = _char->getPenalizedExperience(exp, _victim);

        if (_char->in_room == _victim->in_room)
            gain_exp(_char, exp);
    }

    if (_char && PRF2_FLAGGED(_char, PRF2_DEBUG))
        send_to_char(_char,
                "%s[DAMAGE] %24s: dam: [%d]  wait: [%d]  pos: [%d]%s\r\n", 
                CCCYN(_char, C_NRM),
                _victim->getName(), _damage, 
                _victim->isNPC() ? GET_MOB_WAIT(_victim) : _victim->desc ?
                _victim->desc->wait : 0, _victim->getPosition(), 
                CCNRM(_char, C_NRM));

    if (_victim && PRF2_FLAGGED(_victim, PRF2_DEBUG))
        send_to_char(_victim,
                "%s[DAMAGE] %24s: dam: [%d]  wait: [%d]  pos: [%d]%s\r\n", 
                CCCYN(_victim, C_NRM),
                _victim->getName(), _damage, 
                _victim->isNPC() ? GET_MOB_WAIT(_victim) : _victim->desc ?
                _victim->desc->wait : 0, _victim->getPosition(), 
                CCNRM(_victim, C_NRM));

    if (_genMessages)
        generateDamageMessages();
    else {
        for (unsigned i = 0; i < _toCharMessages.size(); i++) {
            string toChar = _toCharMessages[i];
            _toCharMessages[i] = 
                CCYEL(_char, C_NRM) + toChar + CCNRM(_char, C_NRM);
        }

        for (unsigned i = 0; i < _toVictMessages.size(); i++) {
            string toVict = _toVictMessages[i];
            _toVictMessages[i] = 
                CCRED(_victim, C_NRM) + toVict + CCNRM(_victim, C_NRM);
        }
    }

    ExecutableObject::execute();

    if (_victim->getPosition() == POS_DEAD) {
        _victimKilled = true;
        act("$n is dead!  R.I.P.", false, _victim, 0, 0, TO_ROOM);
                send_to_char(_victim, "You are dead!  Sorry...\r\n");
        for (unsigned int i=0; i < getEV()->size(); i++) {
            if (!(*getEV())[i]->getExecuted() &&
                ((*getEV())[i]->getChar() == this->getVictim() ||
                 (*getEV())[i]->getVictim() == this->getVictim())) {
                //cancel anything that hasn't been executed yet and contains
                //our now dead victim
                (*getEV())[i]->cancel();
            }
        }
        if (!_char) {
            die(getVictim(), NULL, getDamageType(), true);
        }
        else {
            gain_kill_exp(_char, _victim);
            die(getVictim(), getAttacker(), getDamageType(), 
                    (getAttacker()->isNPC()) ? true : false);
        }
    }

}

void
DamageObject::cancel() {
    setDamageFailed(true);
    ExecutableObject::cancel();
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
DamageObject::generateDamageMessages()
{
    int damage_index, damType = _damageType - TYPE_HIT;
    string message, loc_string(""), loc_string2(""), weap_string(""), weap_string2("");
    int dam = MAX(_damage, _manaLoss);
    
    if (dam == 0) {
        damage_index = 0;
    }
    else if (dam < 45) {
        damage_index = (dam / 5) + 1;
    }
    else {
        damage_index = MIN(22, (dam / 10) + 1);
    }

    if (_damageLocation == WEAR_MSHIELD)
        loc_string += "'s your mana shield";
    else if (_damageLocation && random_binary() &&
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
        weap_string += attack_hit_text[damType].singular;
        weap_string += " from $p";
        weap_string2 = weap_string;
    }
    else {
        _obj = NULL;
        string mod("");
        weap_string += "your ";
        weap_string2 += "$s ";
        if (dam < 20)   
            mod = "";
        else if (dam < 40)
            mod = "powerful ";
        else if (dam < 60)
            mod = "terrible ";
        else if (dam < 80)
            mod = "horrible ";
        else if (dam < 100)
            mod = "vicious ";
        else if (dam < 120)
            mod = "unbelievable ";
        else
            mod = "ultra ";

        weap_string += mod;
        weap_string2 += mod;

        weap_string += attack_hit_text[damType].singular;
        weap_string2 += attack_hit_text[damType].singular;
    }
    weap_string += "!";
    weap_string2 += "!"; 

    message.clear();
    // To Char
    message += "You ";
    if (_damageLocation == WEAR_MSHIELD)
        message += "hit";
    else
        message += damage_words[damage_index].self;
    message += " $N";
    message += loc_string;
    message += weap_string;

    if (dam && _damageLocation == WEAR_MSHIELD)
        message = string(CCMAG(_char, C_NRM)) + message + 
                         string(CCNRM(_char, C_NRM));
    else
        message = string(CCYEL(_char, C_NRM)) + message + 
                         string(CCNRM(_char, C_NRM));

    if (!PRF_FLAGGED(getChar(), PRF_GAGMISS) || dam > 0)
        _toCharMessages.push_back(message);
    
    message.clear();
    // To Room
    message += tmp_capitalize("$n ");
    if (_damageLocation == WEAR_MSHIELD)
        message += "hits";
    else
        message += damage_words[damage_index].others;
    message += " $N";
    message += loc_string;
    message += weap_string2;

    _toRoomMessages.push_back(message);

    message.clear();
    // To Vict
    message += tmp_capitalize("$n ");
    if (_damageLocation == WEAR_MSHIELD)
        message += "hits";
    else
        message += damage_words[damage_index].others;
    message += loc_string2;
    message += weap_string2;

    if (dam && _damageLocation == WEAR_MSHIELD)
        message = string(CCCYN(_victim, C_NRM)) + message + 
                         string(CCNRM(_victim, C_NRM));
    else
        message = string(CCRED(_victim, C_NRM)) + message + 
                         string(CCNRM(_victim, C_NRM));

    if (!PRF_FLAGGED(getVictim(), PRF_GAGMISS) || dam > 0)
        _toVictMessages.push_back(message);
}

bool DamageObject::cannotDamage(Creature *vict) {
    if (!vict->isNPC() && vict->getLevel() >= LVL_AMBASSADOR &&
        !PLR_FLAGGED(_victim, PLR_MORTALIZED))
        return true;

    if (NON_CORPOREAL_UNDEAD(_victim) || IS_RAKSHASA(_victim) ||
        IS_GREATER_DEVIL(_victim)) {

        if (!IS_WEAPON(_damageType))
            return false;

        if (IS_WEAPON(_damageType) && _obj && IS_OBJ_STAT(_obj, ITEM_MAGIC))
            return false;

        if (IS_WEAPON(_damageType) && !_obj && _char && 
            _char->getLevelBonus(SKILL_KATA) >= 50 &&
            _char->affectedBy(SKILL_KATA))
            return false;

        return true;
    }

    return false;
}
