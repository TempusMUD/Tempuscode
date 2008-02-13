#ifndef _GENERIC_SKILL_H_
#define _GENERIC_SKILL_H_

#include <signal.h>
#include <map>
#include <string>
#include <vector>
#include "utils.h"
#include "safe_list.h"
#include "xml_utils.h"
#include "executable_object.h"

struct Creature;
struct CreatureList;
struct obj_data;
class GenericSkill;

#define MUD_MINUTES * SECS_PER_MUD_HOUR / 60
#define MUD_HOURS   * SECS_PER_MUD_HOUR
#define MUD_DAYS    * 24 MUD_HOURS
#define MUD_MONTHS  * 35 MUD_DAYS
#define MUD_YEARS   * 16 MUD_MONTHS

using namespace std;

typedef map<int, GenericSkill *> SkillMap;

int is_abbrev(const char *needle, const char *haystack, int count);
    
struct PracticeInfo {
    short allowedClass;
    unsigned char minLevel;
    unsigned char minGen;
};

// This is the return value for canBeLearned().
// You should ONLY access this class via its static members.
// Think of this as an enum with some steroids.
class PracticeAbility {
    private:
    enum PAValue { CANNOT_LEARN, CAN_LEARN, CAN_LEARN_LATER, WRONG_ALIGN };
    PAValue p;
    
    public:
    operator bool() { return p == CAN_LEARN; }
    PracticeAbility(PAValue val) { p = val; }
        bool operator== (PracticeAbility pa) { 
        return (p == pa.p); 
    }
    static PracticeAbility CanLearn;
    static PracticeAbility CannotLearn;
    static PracticeAbility CanLearnLater;
    static PracticeAbility WrongAlignment; 
};

// This is the skill superclass.  Each character will have 
// a collection of these.
class GenericSkill {
    public:
        // Constructors / Destructors
        virtual ~GenericSkill(){};

        // Operators
        bool operator==(const GenericSkill* skill) const;
        bool operator<(const GenericSkill &c) const;
        GenericSkill &operator=(GenericSkill &c);

        virtual bool perform(ExecutableVector &ev, CreatureList &list); 
        virtual bool perform(ExecutableVector &ev, Creature *target);
        virtual bool perform(ExecutableVector &ev, obj_data *obj);
        virtual bool perform(ExecutableVector &ev, char *targets = NULL);
        
        // This is a static function that is essentially a generic 
        // constructor.  Pass it a skill number and get back a new 
        // instance of that skill.
        static GenericSkill *constructSkill(int skillNum);
        static string getNameByNumber(int skillNum);
        static unsigned short getNumberByName(const char *name);
        static GenericSkill const *getSkill(int skillNum);
        static GenericSkill const *getSkill(string skillName);
        static GenericSkill *loadFromXML(xmlNode *node);
        

        // Some of the accessor functions we'll need
        virtual bool isDivine() const { return false; }
        virtual bool isArcane() const { return false; }
        virtual bool isPhysics() const { return false; }
        virtual bool isPsionic() const { return false; }
        virtual bool isSong() const { return false; }
        virtual bool isZen() const { return false; }
        virtual bool isProgram() const { return false; }
        virtual bool isAttackType() const { return false; }
        virtual bool isViolent() const { return _violent; }
        virtual bool isGood() const { return _good; }
        virtual bool isEvil() const { return _evil; }
        virtual bool doNotList() const { return _noList; }
        
        virtual short getSkillNumber() const { return _skillNum; }
        virtual short getSkillLevel() const { return _skillLevel; }
        virtual int getLevel(int charClass) const;
        virtual int getGen(int charClass) const;
        virtual unsigned int getFlags() { return _flags; }
        virtual unsigned short getManaCost();
        virtual unsigned short getMoveCost();
        virtual unsigned short getHitCost();
        virtual unsigned short getMaxMana() { return _maxMana; }
        virtual unsigned short getMaxMove() { return _maxMove; }
        virtual unsigned short getMaxHit() { return _maxHit; }
        virtual unsigned short getMinMana() { return _minMana; }
        virtual unsigned short getminMove() { return _minMove; }
        virtual unsigned short getMinHit() { return _minHit; }
        virtual unsigned short getManaChange() { return _manaChange; }
        virtual unsigned short getMoveChange() { return _moveChange; }
        virtual unsigned short getHitChange() { return _hitChange; }
        virtual time_t getLastUsedTime() { return _lastUsed; };
        virtual time_t getCooldownTime() { return _cooldownTime; };
        
        virtual Creature *getOwner() const { return _owner; };
        
        virtual void setSkillLevel(short skillLev) { _skillLevel = skillLev; }
        virtual void setLastUsedTime(time_t lastUsed) { _lastUsed = lastUsed; };
        virtual void gainSkillProficiency();
        virtual void setOwner(Creature *ch) { _owner = ch; }
        virtual PracticeAbility canBeLearned(Creature *ch) const;
        virtual char getLearnedPercent() { return _learnedPercent; };
        
        virtual void saveToXML(FILE *out);
        virtual bool hasBeenLearned();
        virtual bool extractTrainingCost();
        virtual int getTrainingCost();
        virtual void learn();

        string getName() const { return _skillName; }
        string getTypeDescription();
        bool flagIsSet(int flag);
        bool targetIsSet(int flag);
        void setPowerLevel(unsigned short level);
        unsigned short getPowerLevel();
        const PracticeInfo *getPracticeInfo(int classNum) const;

        static bool isDivine(int skillNum);
        static bool isArcane(int skillNum);
        static bool isPhysics(int skillNum);
        static bool isPsionic(int skillNum);
        static bool isSong(int skillNum);
        static bool isZen(int skillNum);
        static bool isProgram(int skillNum);
        static bool isAttackType(int skillNum);
        static void insertSkill(GenericSkill *skill);
        static SkillMap * getSkillMap();

    protected:
        unsigned short _minMana;
        unsigned short _maxMana;
        unsigned short _manaChange;
        unsigned short _minMove;
        unsigned short _maxMove;
        unsigned short _moveChange;
        unsigned short _minHit;
        unsigned short _maxHit;
        unsigned short _hitChange;
        unsigned short _skillNum;
        unsigned int _flags;
        unsigned short int _targets;
        char _minPosition;
        char _skillLevel;
        char _charWait;
        char _victWait;
        bool _violent;
        bool _good;
        bool _evil;
        bool _noList;
        bool _objectMagic;
        bool _audible;
        bool _nonCombative;
        // Time since the skill was last used
        time_t _lastUsed;
        // Cooldown period in mud hours
        time_t _cooldownTime;

        Creature *_owner;
        string _skillName;
        vector<PracticeInfo> _practiceInfo;

        // These control the learning of a skill.
        // _learnedPercent is the percentage at which a skill is considered
        // "learned" by the guildmaster.  It's impossible to practice a
        // skill beyond this level, although your last practice may put
        // your percentage above it.
        // _minPercentGain is the smallest amount you can possibly gain
        // through practice.
        // _maxPercentGain is the largest amount you can possible gain
        // through practice.
        char _learnedPercent;
        char _minPercentGain;
        char _maxPercentGain;

        //this can be implemented by "return (GenericSkill *)new SkillName();"
        virtual GenericSkill* createNewInstance();
        
        CostObject* generateCost();
        WaitObject* generateWait();

        GenericSkill();

    private:
        static SkillMap* _getSkillMap() {
            static SkillMap skills;
            return &skills;
        }

        static bool skillExists(int skillNum) {
            SkillMap::iterator mi;
            mi = _getSkillMap()->find(skillNum);
            return mi != _getSkillMap()->end();
        }

        unsigned short _powerLevel;
        bool canPerform();
};

class GenericSpell : virtual public GenericSkill {
    public:
        virtual ~GenericSpell() = 0;
        virtual bool perform(ExecutableVector &ev, Creature *target);
        virtual bool perform(ExecutableVector &ev, obj_data *obj);
        virtual bool perform(ExecutableVector &ev, char *targets = NULL);

        virtual ExecutableObject *generateSkillMessage(Creature *victim, 
                obj_data *obj);

    protected:
        GenericSpell() : GenericSkill() {}
    private:
        bool canCast();
};

class ArcaneSpell : virtual public GenericSpell {
    public:
        virtual ~ArcaneSpell() {};

        virtual bool perform(ExecutableVector &ev, Creature *target);
        virtual bool perform(ExecutableVector &ev, obj_data *obj);
        virtual bool perform(ExecutableVector &ev, char *targets = NULL);
        virtual bool isArcane() const { return true; }
                
    protected:
        ArcaneSpell() : GenericSpell() { }
    private:
        bool canCast();
};

class DivineSpell : virtual public GenericSpell {
    public:
        virtual ~DivineSpell() {};

        virtual bool perform(ExecutableVector &ev, Creature *target);
        virtual bool perform(ExecutableVector &ev, obj_data *obj);
        virtual bool perform(ExecutableVector &ev, char *targets = NULL);
        virtual bool isDivine() const { return true; }
        
    protected:
        DivineSpell() : GenericSpell() {  }
    private:
        bool canCast();
};

class PsionicSpell : virtual public GenericSpell {
    public:
        virtual ~PsionicSpell() {};

        virtual bool perform(ExecutableVector &ev, Creature *target);
        virtual bool perform(ExecutableVector &ev, obj_data *obj);
        virtual bool perform(ExecutableVector &ev, char *targets = NULL);
        virtual bool isPsionic() const { return true; }
        
        virtual ExecutableObject *generateSkillMessage(Creature *victim, 
                obj_data *obj);

    protected:
        PsionicSpell() : GenericSpell() { }
    private:
        bool canTrigger();
};

class PhysicsSpell : virtual public GenericSpell {
    public:
        virtual ~PhysicsSpell() {};

        virtual bool perform(ExecutableVector &ev, Creature *target);
        virtual bool perform(ExecutableVector &ev, obj_data *obj);
        virtual bool perform(ExecutableVector &ev, char *targets = NULL);
        virtual bool isPhysics() const { return true; }
        
        virtual ExecutableObject *generateSkillMessage(Creature *victim, 
                obj_data *obj);

    protected:
        PhysicsSpell() : GenericSpell() { } 
    private:
        bool canAlter();
};

class SongSpell : virtual public GenericSpell {
    public:
        virtual ~SongSpell() {};

        virtual bool perform(ExecutableVector &ev, Creature *target);
        virtual bool perform(ExecutableVector &ev, obj_data *obj);
        virtual bool perform(ExecutableVector &ev, char *targets = NULL);
        virtual bool isSong() const { return true; }
        
    protected:
        SongSpell() : GenericSpell() {  }
    private:
        bool canPerform();
};

class GenericProgram : virtual public GenericSkill {
    public:
        virtual ~GenericProgram() {};

        virtual bool perform(ExecutableVector &ev, char *targets = NULL);
        virtual bool activate(ExecutableVector &ev) = 0;
        virtual bool deactivate();
        virtual bool isProgram() const { return true; }
        
    protected:
        GenericProgram() : GenericSkill() {}
};

class GenericZen : virtual public GenericSkill {
    public:
        virtual ~GenericZen() {};

        virtual bool perform(ExecutableVector &ev, Creature *target);
        virtual bool perform(ExecutableVector &ev, obj_data *obj);
        virtual bool perform(ExecutableVector &ev, char *targets = NULL);
        virtual bool isZen() const { return true; }

    protected:
        GenericZen() : GenericSkill() {  }
};

class GenericAttackType : virtual public GenericSkill {
    public:
        virtual ~GenericAttackType() {};
        
        virtual DamageObject *generate(obj_data *weap, Creature *target);
        virtual DamageObject *generate(obj_data *weap);
        virtual DamageObject *generate(Creature *target);
        virtual DamageObject *generate();
        virtual bool isAttackType() const { return true; }
        
    protected:
        GenericAttackType() : GenericSkill () { }
};

#endif
