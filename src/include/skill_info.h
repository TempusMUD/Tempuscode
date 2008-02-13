#ifndef _SKILL_INFO_H_
#define _SKILL_INFO_H_

#include <map>
#include <string>
#include <vector>
#include "xml_utils.h"

using namespace std;

class SkillInfo;
class Creature;

typedef map<int, SkillInfo *> SkillMap;

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

class SkillInfo {
    public:    
        SkillInfo();
        virtual ~SkillInfo(){};
        
        
        bool SkillInfo::operator==(const SkillInfo *skill) const;
        bool SkillInfo::operator<(const SkillInfo &c) const;
        
        // Some of the accessor functions we'll need
        virtual bool isDivine() const { return _divine; }
        virtual bool isArcane() const { return _arcane; }
        virtual bool isPhysics() const { return _physics; }
        virtual bool isPsionic() const { return _psionic; }
        virtual bool isSong() const { return _song; }
        virtual bool isZen() const { return _zen; }
        virtual bool isProgram() const { return _program; }
        virtual bool isAttackType() const { return _attackType; }
        virtual bool isViolent() const { return _violent; }
        // Fake these for now
        virtual bool isGood() const { return false; };
        virtual bool isEvil() const { return false; };
        
        virtual short getSkillNumber() const { return _skillNum; }
        virtual short getSkillLevel() const { return _skillLevel; }
        virtual string getTypeDescription() const;
        virtual string getClassName() const { return _className; }
        virtual string getSkillName() const { return _skillName; }
        
        //intentionally keeping these with skillinfo and not in the EO because
        //we need them for information purposes, EO's can look them up from
        //the skillinfo objects since they should only be stored in one spot
        virtual unsigned short getManaCost() { return _maxMana; }
        virtual unsigned short getMoveCost() { return _maxMove; }
        virtual unsigned short getHitCost() { return _maxHit; }
        
        virtual Creature *getOwner() const { return _owner; };
        
        virtual void setSkillLevel(short skillLev) { _skillLevel = skillLev; }
        virtual void setOwner(Creature *ch) { _owner = ch; }
        virtual PracticeAbility canBeLearned(Creature *ch) const;
        
        virtual void saveToXML(FILE *out);
        static SkillInfo *loadFromXML(xmlNode *node);
        virtual bool hasBeenLearned();
        virtual bool extractTrainingCost();
        virtual int getTrainingCost();
        virtual void learn();

        string getName() const { return _skillName; }
        
        static string getNameByNumber(int skillNum);
        static unsigned short getNumberByName(const char *name);
        static SkillInfo* constructSkill(int skillNum);
        
        static bool isDivine(int skillNum) { 
            if (!skillExists(skillNum))
                return false;

            return getSkillInfo(skillNum)->isDivine();
        }
        static bool isArcane(int skillNum) { 
            if (!skillExists(skillNum))
                return false;

            return getSkillInfo(skillNum)->isArcane();
        }
        static bool isPhysics(int skillNum) { 
            if (!skillExists(skillNum))
                return false;

            return getSkillInfo(skillNum)->isPhysics();
        }
        static bool isPsionic(int skillNum) { 
            if (!skillExists(skillNum))
                return false;

            return getSkillInfo(skillNum)->isPsionic();
        }
        static bool isSong(int skillNum) { 
            if (!skillExists(skillNum))
                return false;

            return getSkillInfo(skillNum)->isSong();
        }
        static bool isZen(int skillNum) { 
            if (!skillExists(skillNum))
                return false;

            return getSkillInfo(skillNum)->isZen();
        }
        static bool isProgram(int skillNum) { 
            if (!skillExists(skillNum))
                return false;

            return getSkillInfo(skillNum)->isProgram();
        }
        static bool isAttackType(int skillNum) {
            if (!skillExists(skillNum))
                return false;

            return getSkillInfo(skillNum)->isAttackType();
        }
        static void insertSkill(SkillInfo *skill) {
            (*_getSkillMap())[skill->getSkillNumber()]=skill;
        }
        static SkillMap * getSkillMap() {
            return _getSkillMap();
        }

        static SkillInfo const *getSkillInfo(int skillNum) {
            if (!skillExists(skillNum))
                return NULL;

            return (*_getSkillMap())[skillNum];
        }
        
        static SkillInfo const *getSkillInfo(string skillName) {
            unsigned short skillNum = getNumberByName(skillName.c_str());

            if (skillNum > 0)
                return getSkillInfo(skillNum);
            
            return NULL;
        }

        //this can be implemented by "return (SkillInfo *)new SkillName();"
        virtual SkillInfo* createNewInstance();

        
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

        bool _arcane;
        bool _divine;
        bool _psionic;
        bool _physics;
        bool _skill;
        bool _zen;
        bool _song;
        bool _program;
        bool _attackType;
        
        char _minPosition;

        char _skillLevel;
        bool _violent;
        Creature *_owner;
        string _skillName;
        string _className;
        vector<PracticeInfo> _practiceInfo;

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
};

#endif
