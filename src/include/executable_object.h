#ifndef _EXECUTABLE_OBJECT_H_
#define _EXECUTABLE_OBJECT_H_

#include <vector>
#include <string>

#include "room_data.h"

struct Creature;
struct obj_data;
class GenericAffect;
class ExecutableVector;

using namespace std;

// This is arbitrary but exists to prevent infinite loops.
static const int MAX_PREPARATION_DEPTH = 20; 

enum EOType { 
    EExecutableObject, 
    EDamageObject, 
    EAffectObject,
    ECostObject,
    EWaitObject,
    ETeleportObject,
    ESummonsObject,
    EPositionObject,
    EStartCombatObject,
    EStopCombatObject,
    EModifyItemObject,
    ERemoveAffectObject,
    EDamageItemObject,
    EDestroyItemObject,
    EDistractionObject,
    ESatiationObject,
    EPointsObject,
    ENullPsiObject
};

class ExecutableObject {
    friend class ExecutableVector;
    public:
        // Normal constructor & destructor
        ExecutableObject(Creature *character);
        virtual ~ExecutableObject();
        
        // Copy constructor
        ExecutableObject(const ExecutableObject &c);
        
        // Operators 
        ExecutableObject &operator =(ExecutableObject &c);
        bool operator ==(ExecutableObject &c) const;
// Accessors
        virtual void setModifiable(bool modifiable = true);
        virtual void setExecuted(bool executed = true);
        virtual void setChar(Creature *ch);
        virtual void setVictim(Creature *victim);
        virtual void setObject(obj_data *obj);
        virtual void setRoom(room_data *room);
        virtual void setID(long ID) { _ID = ID; }
        virtual void setSendMessages(bool send) { _sendMessages = send; };
        virtual void setDepth(int depth) { _depth = depth; }
        virtual void setEV(ExecutableVector *ev) { _ev = ev; }
        virtual void setSkill(int skillNum) {_skillNum = skillNum;}
        virtual int addToCharMessage(string message);
        virtual int addToVictMessage(string message);
        virtual int addToRoomMessage(string message, int flags = 0);
        virtual int removeToCharMessage(int index);
        virtual int removeToVictMessage(int index);
        virtual int removeToRoomMessage(int index);
        virtual bool getModifiable() const;
        virtual bool getExecuted() const;
        virtual int getDepth() const { return _depth; }
        virtual ExecutableVector* getEV() const { return _ev; }
        virtual int getSkill() const {return _skillNum;}
        virtual Creature *getChar() const;
        virtual Creature *getVictim() const;
        virtual obj_data *getObject() const;
        virtual room_data *getRoom() const;
        virtual long getID() const { return _ID; }
        virtual vector<string> &getToCharMessages() {
            return _toCharMessages;
        }
        virtual vector<string> &getToVictMessages() {
            return _toVictMessages;
        }
        virtual vector<string> &getToRoomMessages() {
            return _toRoomMessages;
        }

        // Normal members
        virtual void cancelByDepth();
        virtual void cancelByID();
        virtual void cancelAll();
        virtual void cancel();
        virtual void execute();
        virtual void sendMessages();
        virtual bool isVictimDead() const;
        virtual bool isA(EOType type) const;
        
    protected:
        bool _modifiable;
        bool _executed;
        bool _victimKilled;
        bool _sendMessages;
        Creature *_char;
        Creature *_victim;
        obj_data *_obj;
        room_data *_room;
        long _ID;
        int _depth;
        int _skillNum;
        int _actRoomFlags;
        vector<string> _toCharMessages;
        vector<string> _toVictMessages;
        vector<string> _toRoomMessages;
        virtual void prepare();
        ExecutableVector *_ev;
};

class AffectObject : public ExecutableObject {
    public:

        AffectObject(string className, Creature *owner, Creature *target);
        AffectObject(string className, Creature *owner, obj_data *target);
        AffectObject(string className, Creature *owner, room_data *target);
        virtual ~AffectObject() {};

        void setAffect(GenericAffect *affect) {_affect = affect;}
        GenericAffect *getAffect() {return _affect;}
        virtual void execute();
        virtual void cancel();

    private:
        GenericAffect *_affect;
};

class RemoveAffectObject : public ExecutableObject {
    public:
        RemoveAffectObject(string className, Creature *owner, Creature *target);
        RemoveAffectObject(string className, Creature *owner, obj_data *target);
        RemoveAffectObject(short skillNum, Creature *owner, Creature *target);
        RemoveAffectObject(short skillNum, Creature *owner, obj_data *target);
        virtual ~RemoveAffectObject() {};

        virtual void execute();

    private:
        int _affectNum;
};

class DamageObject : public ExecutableObject {
    public:
        // Normal constructor & destructor
        DamageObject(Creature *character, Creature *vict);
        ~DamageObject();

        // Copy constructor
        DamageObject(const DamageObject &c);

        // Operators
        DamageObject &operator =(DamageObject &c);
        bool operator >(DamageObject &c) const;
        bool operator <(DamageObject &c) const;
        bool operator ==(DamageObject &c) const;

        // Accessors
        void setWeapon(obj_data *weap);
        void setDamage(int damage);
        void setDamageType(int type);
        void setDamageLocation(int location);
        void setVictimKilled(bool victimKilled);
        void setDamageFailed(bool damageFailed);
        void setGenMessages(bool gen = true);
        void initiateCombat(bool init) { _initiateCombat = init; }
        
        Creature *getAttacker() const;
        obj_data *getWeapon() const;
        int getDamage() const;
        int getDamageType() const;
        int getDamageLocation() const;
        bool getDamageFailed() const;
        bool getGenMessages() const;
        
        // Normal members
        void execute();
        void cancel();
        void cap();
        void generateDamageMessages();
        bool cannotDamage(struct Creature *vict);

        // This is just temporary, if I include spells.h here we 
        // have big problems
        bool isWeaponType() {
            return (_damageType >= 800 && _damageType < 820);
        }

    private:
        int _damage;
        int _manaLoss;
        int _damageType;
        int _damageLocation;
        bool _damageFailed;
        bool _genMessages;        
        bool _initiateCombat;
};

class CostObject : public ExecutableObject {
    public:
        CostObject(Creature* character);
        
        virtual ~CostObject() {};

        void setMana(int mana);
        void setMove(int move);
        void setHit(int hit);
        
        int getMove() const;
        int getMana() const;
        int getHit() const;
        
        virtual void execute();
        
    private:
        int _mana;
        int _move;
        int _hit;
};

class WaitObject : public ExecutableObject {
    public:
        WaitObject(Creature *ch, char amount);
        virtual ~WaitObject() {};

        virtual void execute();
        
    private:
        int _wait;
};

class DamageItemObject : public ExecutableObject {
    public:
        DamageItemObject(Creature *ch, Creature *vict, short position, short amount, short type = -1);
        DamageItemObject(Creature *vict, short position, short amount, short type = -1);
        DamageItemObject(struct obj_data *obj, short amount, short type = -1);

        virtual ~DamageItemObject() {};

        short getPosition() { return _position; };
        short getAmount() { return _amount; };
        short getType() { return _type; };

        void setPosition(short p) { _position = p; };
        void setAmount(short a) { _position = a; };
        void setType(short t) { _position = t; };

        virtual void execute();
        
    private:
        short _position;
        short _amount;
        short _type;
};

class TeleportObject : public ExecutableObject {
    public:
        enum TeleportType {
            Teleport,
            LocalTeleport,
            RandomCoordinates,
            VStone,
            Summon
        };

        TeleportObject(Creature *character, TeleportType type) 
            : ExecutableObject(character) {
            _char = character;
            _type = type;
        }
        ~TeleportObject() {};

        void setType(TeleportType type) { _type = type; };
        TeleportType getType() { return _type; };

        virtual void execute();

    private:
        bool teleportOK();
        bool summonExecute();
        void vstoneExecute();

        TeleportType _type;
};

class SummonsObject : public ExecutableObject {
    public:
        enum SummonsType {
            ConjureElemental,
            PhantasmicSword,
            SummonLeigon,
            UnholyStalker
        };

        SummonsObject(Creature *character, SummonsType type) 
            : ExecutableObject(character) {
            _char = character;
            _type = type;
        }
        ~SummonsObject() {};

        void setType(SummonsType type) { _type = type; };
        SummonsType getType() { return _type; };

        virtual void execute();

    private:
        Creature *getCreatureType();

        SummonsType _type;
};

class PositionObject : public ExecutableObject {
    public:
    
        PositionObject(Creature *character, Creature *victim, int position) 
            : ExecutableObject(character) {
            _position = position;
            _char = character;
            _victim = victim;
        }
        
        void setPosition(int position) { _position = position; }
        int getPosition() { return _position; }
        
        virtual void execute();
        
    private:
        int _position;
};

class StartCombatObject : public ExecutableObject {
    public:
        StartCombatObject(Creature *character, Creature *victim, bool victimOnly=false) : 
            ExecutableObject(character) { 
            _char = character;
            _victim = victim;
            _victimOnly = victimOnly;
        }
        
        void setVictimOnly(bool victimOnly) { _victimOnly = victimOnly; }
        bool getVictimOnly() { return _victimOnly; }
        
        virtual void execute();

    private:
        bool _victimOnly;
};


class StopCombatObject : public ExecutableObject {
    public:
        StopCombatObject(Creature *character, Creature *victim, bool victimOnly=false) : 
            ExecutableObject(character) { 
            _char = character;
            _victim = victim;
            _victimOnly = victimOnly;
        }
        
        void setVictimOnly(bool victimOnly) { _victimOnly = victimOnly; }
        bool getVictimOnly() { return _victimOnly; }
        
        virtual void execute();

    private:
        bool _victimOnly;
};

class ModifyItemObject : public ExecutableObject {
    public:
        struct locModType {
            short location;
            short modifier;
        };

        ModifyItemObject(Creature *character, obj_data *obj) : ExecutableObject(character), 
            _addBits(3, 0), _delBits(3, 0) {
            _char = character;
            _obj = obj;

            for (int x = 0; x < 4; x++)
                _valueModifier[x] = -127;

            _damChange = 0;
            _maxDamChange = 0;
            _addToWear = 0;
            _removeFromWear = 0;
            _weightModifier = 0;
            _typeModifier = 0;
        }

        // Manipulators
        void modifyDam(int c) { _damChange = c; };
        void modifyMaxDam(int c) { _maxDamChange = c; };
        void addToWear(int bits) { _addToWear = bits; };
        void removeFromWear(int bits) { _removeFromWear = bits; };
        void modifyWeight(int c) { _weightModifier = c; };
        void setType(char t) { _typeModifier = t; };

        void setExtraBits(int which, int value) { 
            _addBits[which] = value; 
        };

        void remExtraBits(int which, int value) { 
            _delBits[which] = value; 
        };

        void setValue(char which, char amount) {
            if (which < 0 || which > 3)
                return;

            _valueModifier[(short)which] = amount;
        };

        void modifyApply(int which, int value) {
            struct locModType lm;

            if (value > 125)
                value = 125;

            if (value < -125)
                value = -125;

            lm.location = which;
            lm.modifier = value;

            _affectMod.push_back(lm);
        };

        // Accessors
        virtual void execute();
        
    private:
        int _damChange;                 // Current damage modifier (+/-)
        int _maxDamChange;              // Maximum damage modifier (+/-)
        int _addToWear;                 // Add these bits to worn_on
        int _removeFromWear;            // Remove these bits from wear
        int _weightModifier;             // Weight modifier (+/-)
        char _valueModifier[4];          // Set values 1-4 to these values
        char _typeModifier;             // Change type to this value
        vector<int> _addBits;           // Add these bits to extra bits
        vector<int> _delBits;           // Remove these bits from extra bits
        vector<locModType> _affectMod;  // Change item applies by these values (+/-)
};

class DestroyItemObject : public ExecutableObject {
    public:
        // Parameters are:  Creature responsible for the damage, obj itself, type of damage
        DestroyItemObject(Creature *ch, obj_data *obj, int type) : ExecutableObject(ch) { 
            _obj = obj;
            _type = type;
            _char = ch;
        }
        
        virtual void execute();
        
    private:
        obj_data *_obj;
        int _type;
};

class DistractionObject : public ExecutableObject {
    public:
        DistractionObject(Creature *ch, Creature *vict) : ExecutableObject(ch) {
            _victim = vict;
        };

        virtual void execute();
};

class SatiationObject : public ExecutableObject {
    public:
        SatiationObject(Creature *ch, Creature *vict) : ExecutableObject(ch) {
            _victim = vict;
            _hunger = 0;
            _thirst = 0;
            _drunk = 0;
        };

        // Accessors
        void setHunger(short h) { _hunger = h; };
        void setThirst(short t) { _thirst = t; };
        void setDrunk(short d) { _drunk = d; };

        virtual void execute();

    private:
        short _hunger;
        short _thirst;
        short _drunk;
};

class PointsObject : public ExecutableObject {
    public:
        PointsObject(Creature *ch, Creature *vict) : ExecutableObject(ch) {
            _victim = vict;
            _hit = 0;
            _mana = 0;
            _move = 0;
        };

        // Accessors
        void setHit(short h) { _hit = h; };
        void setMana(short m) { _mana = m; };
        void setMove(short m) { _move = m; };

        virtual void execute();

    private:
        short _hit;
        short _mana;
        short _move;
};

class NullPsiObject : public ExecutableObject {
    public:
        NullPsiObject(Creature *ch, Creature *vict) : ExecutableObject(ch) {
            _victim = vict;
        };

        virtual void execute();
};

//the only safe way to add an ExecutableObject to an ExecutableVector is using
//one of the two specially overloaded insert functions or push_back.  Anything
//else including assignment operators on pointers is a big no no.
class ExecutableVector : public vector<ExecutableObject *> {
    public:
        // Normal contstructor & destructor
        ExecutableVector();
        ~ExecutableVector();

        // Copy constructor
        ExecutableVector(ExecutableVector &c);

        // Operators
        ExecutableObject * const operator[](unsigned int);

        // Accessors
        void setMaxSize(int maxSize);
        int getMaxSize();

        // Normal members        
        void execute();
        void insert(unsigned pos, ExecutableObject *x);
        void insert(ExecutableObject *pos, ExecutableObject *x);
        void insert_id(unsigned pos, ExecutableObject *x);
        void insert_id(ExecutableObject *pos, ExecutableObject *x);
        void push_back(ExecutableObject *x);
        void push_front(ExecutableObject *x);
        void push_back_id(ExecutableObject *x);
        void push_front_id(ExecutableObject *x);
        void suppressMessages(bool suppress);
        void suppressToCharMessages(bool suppress);
        void suppressToVictMessages(bool suppress);
        void suppressToRoomMessages(bool suppress);
        bool isVictimDead(Creature *ch);

    private:
        bool _addOk;
        bool _noToCharMessages;
        bool _noToVictMessages;
        bool _noToRoomMessages;
        int _maxSize;
        void prepare();
        bool modifiable();
};

#endif
