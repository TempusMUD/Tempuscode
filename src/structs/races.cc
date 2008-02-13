#include "races.h"
#include "creature.h"
#include "spells.h"
#include "utils.h"

CreatureRace::CreatureRace() {
    _strMod = 0;
    _intMod = 0;
    _wisMod = 0;
    _dexMod = 0;
    _conMod = 0;
    _chaMod = 0;
    _raceNum = -1;

    _racialSkills.push_back(SKILL_PUNCH);
    _racialSkills.push_back(SKILL_SWIMMING);
}

CreatureRace *
CreatureRace::constructRace(int raceNum) {
    return (*_getRaceMap())[raceNum]->createNewInstance();
}

void
CreatureRace::insertRace(CreatureRace *race) {
    (*_getRaceMap())[race->_raceNum] = race;
}

void
CreatureRace::applyRacialSkills() {
    if (!_owner)
        return;

    if (_owner->isNPC())
        return;
     
    for (unsigned x = 0; x < _racialSkills.size(); x++) {
        // Don't do anything if they already know it
        if (_owner->getSkill(_racialSkills[x]))
            continue;

        GenericSkill *skill = GenericSkill::constructSkill(_racialSkills[x]);
        if (skill) {
            skill->setSkillLevel(65); 
            _owner->addSkill(skill);
        }
    }
}

char 
CreatureRace::getMaxStr() { 
    if (!_owner)
        return 0;

    return MIN(25, 18 + _owner->getGen() + _strMod); 
}

char 
CreatureRace::getMaxInt() {
    if (!_owner)
        return 0;

    return MIN(25, 18 + _owner->getGen() + _intMod); 
}

char 
CreatureRace::getMaxWis() { 
    if (!_owner)
        return 0;

    return MIN(25, 18 + _owner->getGen() + _wisMod); 
}

char 
CreatureRace::getMaxDex() { 
    if (!_owner)
        return 0;

    return MIN(25, 18 + _owner->getGen() + _dexMod); 
}

char 
CreatureRace::getMaxCon() { 
    if (!_owner)
        return 0;

    return MIN(25, 18 + _owner->getGen() + _conMod); 
}

char 
CreatureRace::getMaxCha() { 
    if (!_owner)
        return 0;

    return MIN(25, 18 + _owner->getGen() + _chaMod); 
}

class RaceHuman : public CreatureRace {
    public:
        RaceHuman() : CreatureRace() {
            _raceNum = 0;
        };
        ~RaceHuman() {};

        CreatureRace *createNewInstance() {
            return (CreatureRace *) new RaceHuman();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class RaceElf : public CreatureRace {
    public:
        RaceElf() : CreatureRace() {
            _intMod = 1;
            _dexMod = 1;
            _conMod = -1;
            _raceNum = 1;

            _racialSkills.push_back(SPELL_INFRAVISION);
            _racialSkills.push_back(SKILL_ARCHERY);
        };
        ~RaceElf() {};

        CreatureRace *createNewInstance() {
            return (CreatureRace *) new RaceElf();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class RaceDwarf : public CreatureRace {
    public:
        RaceDwarf() : CreatureRace() {
            _strMod = 1;
            _chaMod = -1;
            _conMod = 1;
            _raceNum = 2;

            _racialSkills.push_back(SPELL_INFRAVISION);
        };
        ~RaceDwarf() {};

        CreatureRace *createNewInstance() {
            return (CreatureRace *) new RaceDwarf();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class RaceOrc : public CreatureRace {
    public:
        RaceOrc() : CreatureRace() {
            _strMod = 2;
            _intMod = -1;
            _wisMod = -2;
            _conMod = 2;
            _chaMod = -3; 
            _raceNum = 16;

            _racialSkills.push_back(SPELL_INFRAVISION);
            _racialSkills.push_back(SKILL_BASH);
        };
        ~RaceOrc() {};

        CreatureRace *createNewInstance() {
            return (CreatureRace *) new RaceOrc();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class RaceHalfOrc : public CreatureRace {
    public:
        RaceHalfOrc() : CreatureRace() {
            _strMod = 2;
            _intMod = -1;
            _wisMod = -2;
            _conMod = 1;
            _chaMod = -3; 
            _raceNum = 3;

            _racialSkills.push_back(SPELL_INFRAVISION);
            _racialSkills.push_back(SKILL_BASH);
        };
        ~RaceHalfOrc() {};

        CreatureRace *createNewInstance() {
            return (CreatureRace *) new RaceHalfOrc();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class RaceMinotaur : public CreatureRace {
    public:
        RaceMinotaur() : CreatureRace() {
            _strMod = 2;
            _intMod = -2;
            _wisMod = -2;
            _conMod = 1;
            _chaMod = -2; 
            _raceNum = 19;

            _racialSkills.push_back(SKILL_BASH);
        };
        ~RaceMinotaur() {};

        CreatureRace *createNewInstance() {
            return (CreatureRace *) new RaceMinotaur();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class RaceTabaxi : public CreatureRace {
    public:
        RaceTabaxi() : CreatureRace() {
            _intMod = -1;
            _wisMod = -2;
            _conMod = 1;
            _chaMod = -2; 
            _raceNum = 6;

            _racialSkills.push_back(SKILL_CLAW);
            _racialSkills.push_back(SKILL_BITE);
        };
        ~RaceTabaxi() {};

        CreatureRace *createNewInstance() {
            return (CreatureRace *) new RaceTabaxi();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class RaceDrow : public CreatureRace {
    public:
        RaceDrow() : CreatureRace() {
            _intMod = 1;
            _dexMod = 1;
            _conMod = -1; 
            _raceNum = 7;

            _racialSkills.push_back(SPELL_INFRAVISION);
            _racialSkills.push_back(SKILL_ARCHERY);
        };
        ~RaceDrow() {};

        CreatureRace *createNewInstance() {
            return (CreatureRace *) new RaceDrow();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class RaceHalfling : public CreatureRace {
    public:
        RaceHalfling() : CreatureRace() {
            _strMod = -2;
            _dexMod = 2;
            _conMod = -1; 
            _raceNum = 5;

            _racialSkills.push_back(SKILL_SNEAK);
            _racialSkills.push_back(SKILL_STEAL);
        };
        ~RaceHalfling() {};

        CreatureRace *createNewInstance() {
            return (CreatureRace *) new RaceHalfling();
        };

        void modifyEO(ExecutableObject *eo) {};
};

// ******************************************************
//
// NPC Races
//
// ******************************************************

class RaceKlingon : public CreatureRace {
	public:
		RaceKlingon() : CreatureRace() {
			_raceNum = 4;
		};
		~RaceKlingon() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceKlingon();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceUndefined1 : public CreatureRace {
	public:
		RaceUndefined1() : CreatureRace() {
			_raceNum = 8;
		};
		~RaceUndefined1() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceUndefined1();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceUndefined2 : public CreatureRace {
	public:
		RaceUndefined2() : CreatureRace() {
			_raceNum = 9;
		};
		~RaceUndefined2() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceUndefined2();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceMobile : public CreatureRace {
	public:
		RaceMobile() : CreatureRace() {
			_raceNum = 10;
		};
		~RaceMobile() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceMobile();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceUndead : public CreatureRace {
	public:
		RaceUndead() : CreatureRace() {
			_raceNum = 11;
		};
		~RaceUndead() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceUndead();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceHumanoid : public CreatureRace {
	public:
		RaceHumanoid() : CreatureRace() {
			_raceNum = 12;
		};
		~RaceHumanoid() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceHumanoid();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceAnimal : public CreatureRace {
	public:
		RaceAnimal() : CreatureRace() {
			_raceNum = 13;
		};
		~RaceAnimal() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceAnimal();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceDragon : public CreatureRace {
	public:
		RaceDragon() : CreatureRace() {
			_raceNum = 14;
		};
		~RaceDragon() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceDragon();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceGiant : public CreatureRace {
	public:
		RaceGiant() : CreatureRace() {
			_raceNum = 15;
		};
		~RaceGiant() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceGiant();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceGoblin : public CreatureRace {
	public:
		RaceGoblin() : CreatureRace() {
			_raceNum = 17;
		};
		~RaceGoblin() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceGoblin();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceHafling : public CreatureRace {
	public:
		RaceHafling() : CreatureRace() {
			_raceNum = 18;
		};
		~RaceHafling() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceHafling();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceTroll : public CreatureRace {
	public:
		RaceTroll() : CreatureRace() {
			_raceNum = 20;
		};
		~RaceTroll() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceTroll();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceGolem : public CreatureRace {
	public:
		RaceGolem() : CreatureRace() {
			_raceNum = 21;
		};
		~RaceGolem() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceGolem();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceElemental : public CreatureRace {
	public:
		RaceElemental() : CreatureRace() {
			_raceNum = 22;
		};
		~RaceElemental() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceElemental();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceOgre : public CreatureRace {
	public:
		RaceOgre() : CreatureRace() {
			_raceNum = 23;
		};
		~RaceOgre() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceOgre();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceDevil : public CreatureRace {
	public:
		RaceDevil() : CreatureRace() {
			_raceNum = 24;
		};
		~RaceDevil() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceDevil();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceTroglodyte : public CreatureRace {
	public:
		RaceTroglodyte() : CreatureRace() {
			_raceNum = 25;
		};
		~RaceTroglodyte() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceTroglodyte();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceManticore : public CreatureRace {
	public:
		RaceManticore() : CreatureRace() {
			_raceNum = 26;
		};
		~RaceManticore() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceManticore();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceBugbear : public CreatureRace {
	public:
		RaceBugbear() : CreatureRace() {
			_raceNum = 27;
		};
		~RaceBugbear() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceBugbear();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceDraconian : public CreatureRace {
	public:
		RaceDraconian() : CreatureRace() {
			_raceNum = 28;
		};
		~RaceDraconian() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceDraconian();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceDuergar : public CreatureRace {
	public:
		RaceDuergar() : CreatureRace() {
			_raceNum = 29;
		};
		~RaceDuergar() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceDuergar();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceSlaad : public CreatureRace {
	public:
		RaceSlaad() : CreatureRace() {
			_raceNum = 30;
		};
		~RaceSlaad() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceSlaad();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceRobot : public CreatureRace {
	public:
		RaceRobot() : CreatureRace() {
			_raceNum = 31;
		};
		~RaceRobot() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceRobot();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceDemon : public CreatureRace {
	public:
		RaceDemon() : CreatureRace() {
			_raceNum = 32;
		};
		~RaceDemon() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceDemon();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceDeva : public CreatureRace {
	public:
		RaceDeva() : CreatureRace() {
			_raceNum = 33;
		};
		~RaceDeva() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceDeva();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RacePlant : public CreatureRace {
	public:
		RacePlant() : CreatureRace() {
			_raceNum = 34;
		};
		~RacePlant() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RacePlant();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceArchon : public CreatureRace {
	public:
		RaceArchon() : CreatureRace() {
			_raceNum = 35;
		};
		~RaceArchon() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceArchon();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RacePudding : public CreatureRace {
	public:
		RacePudding() : CreatureRace() {
			_raceNum = 36;
		};
		~RacePudding() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RacePudding();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceAlien1 : public CreatureRace {
	public:
		RaceAlien1() : CreatureRace() {
			_raceNum = 37;
		};
		~RaceAlien1() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceAlien1();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RacePredAlien : public CreatureRace {
	public:
		RacePredAlien() : CreatureRace() {
			_raceNum = 38;
		};
		~RacePredAlien() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RacePredAlien();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceSlime : public CreatureRace {
	public:
		RaceSlime() : CreatureRace() {
			_raceNum = 39;
		};
		~RaceSlime() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceSlime();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceIllithid : public CreatureRace {
	public:
		RaceIllithid() : CreatureRace() {
			_raceNum = 40;
		};
		~RaceIllithid() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceIllithid();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceFish : public CreatureRace {
	public:
		RaceFish() : CreatureRace() {
			_raceNum = 41;
		};
		~RaceFish() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceFish();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceBeholder : public CreatureRace {
	public:
		RaceBeholder() : CreatureRace() {
			_raceNum = 42;
		};
		~RaceBeholder() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceBeholder();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceGaseous : public CreatureRace {
	public:
		RaceGaseous() : CreatureRace() {
			_raceNum = 43;
		};
		~RaceGaseous() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceGaseous();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceGithyanki : public CreatureRace {
	public:
		RaceGithyanki() : CreatureRace() {
			_raceNum = 44;
		};
		~RaceGithyanki() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceGithyanki();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceInsect : public CreatureRace {
	public:
		RaceInsect() : CreatureRace() {
			_raceNum = 45;
		};
		~RaceInsect() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceInsect();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceDaemon : public CreatureRace {
	public:
		RaceDaemon() : CreatureRace() {
			_raceNum = 46;
		};
		~RaceDaemon() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceDaemon();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceMephit : public CreatureRace {
	public:
		RaceMephit() : CreatureRace() {
			_raceNum = 47;
		};
		~RaceMephit() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceMephit();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceKobold : public CreatureRace {
	public:
		RaceKobold() : CreatureRace() {
			_raceNum = 48;
		};
		~RaceKobold() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceKobold();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceUmberHulk : public CreatureRace {
	public:
		RaceUmberHulk() : CreatureRace() {
			_raceNum = 49;
		};
		~RaceUmberHulk() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceUmberHulk();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceWemic : public CreatureRace {
	public:
		RaceWemic() : CreatureRace() {
			_raceNum = 50;
		};
		~RaceWemic() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceWemic();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceRakshasa : public CreatureRace {
	public:
		RaceRakshasa() : CreatureRace() {
			_raceNum = 51;
		};
		~RaceRakshasa() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceRakshasa();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceSpider : public CreatureRace {
	public:
		RaceSpider() : CreatureRace() {
			_raceNum = 52;
		};
		~RaceSpider() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceSpider();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceGriffin : public CreatureRace {
	public:
		RaceGriffin() : CreatureRace() {
			_raceNum = 53;
		};
		~RaceGriffin() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceGriffin();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceRotarian : public CreatureRace {
	public:
		RaceRotarian() : CreatureRace() {
			_raceNum = 54;
		};
		~RaceRotarian() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceRotarian();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceHalfElf : public CreatureRace {
	public:
		RaceHalfElf() : CreatureRace() {
			_raceNum = 55;
		};
		~RaceHalfElf() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceHalfElf();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceCelestial : public CreatureRace {
	public:
		RaceCelestial() : CreatureRace() {
			_raceNum = 56;
		};
		~RaceCelestial() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceCelestial();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceGuardinal : public CreatureRace {
	public:
		RaceGuardinal() : CreatureRace() {
			_raceNum = 57;
		};
		~RaceGuardinal() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceGuardinal();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceOlympian : public CreatureRace {
	public:
		RaceOlympian() : CreatureRace() {
			_raceNum = 58;
		};
		~RaceOlympian() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceOlympian();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceYugoloth : public CreatureRace {
	public:
		RaceYugoloth() : CreatureRace() {
			_raceNum = 59;
		};
		~RaceYugoloth() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceYugoloth();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceRowlahr : public CreatureRace {
	public:
		RaceRowlahr() : CreatureRace() {
			_raceNum = 60;
		};
		~RaceRowlahr() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceRowlahr();
		};

		void modifyEO(ExecutableObject *eo) {};
};

class RaceGithzerai : public CreatureRace {
	public:
		RaceGithzerai() : CreatureRace() {
			_raceNum = 61;
		};
		~RaceGithzerai() {};

		CreatureRace *createNewInstance() {
			return (CreatureRace *) new RaceGithzerai();
		};

		void modifyEO(ExecutableObject *eo) {};
};
void register_races() {
    CreatureRace::insertRace(new RaceHuman());
    CreatureRace::insertRace(new RaceElf());
    CreatureRace::insertRace(new RaceDwarf());
    CreatureRace::insertRace(new RaceOrc());
    CreatureRace::insertRace(new RaceHalfOrc());
    CreatureRace::insertRace(new RaceMinotaur());
    CreatureRace::insertRace(new RaceTabaxi());
    CreatureRace::insertRace(new RaceDrow());
    CreatureRace::insertRace(new RaceHalfling());
    CreatureRace::insertRace(new RaceKlingon());
    CreatureRace::insertRace(new RaceUndefined1());
    CreatureRace::insertRace(new RaceUndefined2());
    CreatureRace::insertRace(new RaceHalfling());
    CreatureRace::insertRace(new RaceMobile());
    CreatureRace::insertRace(new RaceUndead());
    CreatureRace::insertRace(new RaceHumanoid());
    CreatureRace::insertRace(new RaceAnimal());
    CreatureRace::insertRace(new RaceDragon());
    CreatureRace::insertRace(new RaceGiant());
    CreatureRace::insertRace(new RaceGoblin());
    CreatureRace::insertRace(new RaceHafling());
    CreatureRace::insertRace(new RaceTroll());
    CreatureRace::insertRace(new RaceGolem());
    CreatureRace::insertRace(new RaceElemental());
    CreatureRace::insertRace(new RaceOgre());
    CreatureRace::insertRace(new RaceDevil());
    CreatureRace::insertRace(new RaceTroglodyte());
    CreatureRace::insertRace(new RaceManticore());
    CreatureRace::insertRace(new RaceBugbear());
    CreatureRace::insertRace(new RaceDraconian());
    CreatureRace::insertRace(new RaceDuergar());
    CreatureRace::insertRace(new RaceSlaad());
    CreatureRace::insertRace(new RaceRobot());
    CreatureRace::insertRace(new RaceDemon());
    CreatureRace::insertRace(new RaceDeva());
    CreatureRace::insertRace(new RacePlant());
    CreatureRace::insertRace(new RaceArchon());
    CreatureRace::insertRace(new RacePudding());
    CreatureRace::insertRace(new RaceAlien1());
    CreatureRace::insertRace(new RacePredAlien());
    CreatureRace::insertRace(new RaceSlime());
    CreatureRace::insertRace(new RaceIllithid());
    CreatureRace::insertRace(new RaceFish());
    CreatureRace::insertRace(new RaceBeholder());
    CreatureRace::insertRace(new RaceGaseous());
    CreatureRace::insertRace(new RaceGithyanki());
    CreatureRace::insertRace(new RaceInsect());
    CreatureRace::insertRace(new RaceDaemon());
    CreatureRace::insertRace(new RaceMephit());
    CreatureRace::insertRace(new RaceKobold());
    CreatureRace::insertRace(new RaceUmberHulk());
    CreatureRace::insertRace(new RaceWemic());
    CreatureRace::insertRace(new RaceRakshasa());
    CreatureRace::insertRace(new RaceSpider());
    CreatureRace::insertRace(new RaceGriffin());
    CreatureRace::insertRace(new RaceRotarian());
    CreatureRace::insertRace(new RaceHalfElf());
    CreatureRace::insertRace(new RaceCelestial());
    CreatureRace::insertRace(new RaceGuardinal());
    CreatureRace::insertRace(new RaceOlympian());
    CreatureRace::insertRace(new RaceYugoloth());
    CreatureRace::insertRace(new RaceRowlahr());
    CreatureRace::insertRace(new RaceGithzerai());
}
