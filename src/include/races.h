#ifndef _RACES_H_
#define _RACES_H_

struct ExecutableObject;
struct creatureRace;
struct creature;

typedef map<int, struct creatureRace *> RaceMap;

struct creatureRace {
        struct creatureRace();
        virtual ~struct creatureRace() {};

        char getMaxStr();
        char getMaxInt();
        char getMaxWis();
        char getMaxDex();
        char getMaxCon();
        char getMaxCha();

        void applyRacialSkills();

        virtual void modifyEO(ExecutableObject *eo) = 0;
        virtual struct creatureRace* createNewInstance() = 0;

        static struct creatureRace *constructRace(int raceNum);
        static void insertRace(struct creatureRace *race);

        // Accessors
        struct creature *getOwner() { return _owner; };
        char getStrMod() { return _strMod; };
        char getIntMod() { return _intMod; };
        char getWisMod() { return _wisMod; };
        char getDexMod() { return _dexMod; };
        char getConMod() { return _conMod; };
        char getChaMod() { return _chaMod; };

        // Manipulators
        void setOwner(struct creature *owner) { _owner = owner; applyRacialSkills(); };

    protected:
        char _strMod;
        char _intMod;
        char _wisMod;
        char _dexMod;
        char _conMod;
        char _chaMod;
        int _raceNum;

        struct creature *_owner;
        vector<int> _racialSkills;

        static RaceMap* _getRaceMap() {
            static RaceMap race;
            return &race;
        }
};

#endif
