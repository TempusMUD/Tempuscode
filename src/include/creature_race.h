#ifndef _RACES_H_
#define _RACES_H_

#include <vector>
#include <map>

class ExecutableObject;
class CreatureRace;
class Creature;

using namespace std;

typedef map<int, CreatureRace *> RaceMap;

class CreatureRace {
    public:
        CreatureRace();
        virtual ~CreatureRace() {};

        char getMaxStr();
        char getMaxInt();
        char getMaxWis();
        char getMaxDex();
        char getMaxCon();
        char getMaxCha();

        void applyRacialSkills();

        virtual void modifyEO(ExecutableObject *eo) = 0;
        virtual CreatureRace* createNewInstance() = 0;

        static CreatureRace *constructRace(int raceNum);
        static void insertRace(CreatureRace *race);

        // Accessors
        Creature *getOwner() { return _owner; };
        char getStrMod() { return _strMod; };
        char getIntMod() { return _intMod; };
        char getWisMod() { return _wisMod; };
        char getDexMod() { return _dexMod; };
        char getConMod() { return _conMod; };
        char getChaMod() { return _chaMod; };

        // Manipulators
        void setOwner(Creature *owner) { _owner = owner; applyRacialSkills(); };

    protected:
        char _strMod;
        char _intMod;
        char _wisMod;
        char _dexMod;
        char _conMod;
        char _chaMod;
        int _raceNum;

        Creature *_owner;
        vector<int> _racialSkills;

        static RaceMap* _getRaceMap() {
            static RaceMap race;
            return &race;
        }
};

#endif
