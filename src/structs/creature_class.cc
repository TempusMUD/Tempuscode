#include "creature_class.h"
#include "creature.h"

CreatureClass *
CreatureClass::constructClass(int classNum) {

    if (!classExists(classNum))
        return NULL;

    return (*_getClassMap())[classNum]->createNewInstance();
}

void
CreatureClass::insertClass(CreatureClass *theClass) {
    (*_getClassMap())[theClass->_classNum] = theClass;
}

class ClassMage : public CreatureClass {
    public:
        ClassMage() : CreatureClass() {
            _classNum = 0;
        };
        ~ClassMage() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassMage();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassCleric : public CreatureClass {
    public:
        ClassCleric() : CreatureClass() {
            _classNum = 1;
        };
        ~ClassCleric() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassCleric();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassThief : public CreatureClass {
    public:
        ClassThief() : CreatureClass() {
            _classNum = 2;
        };
        ~ClassThief() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassThief();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassWarrior : public CreatureClass {
    public:
        ClassWarrior() : CreatureClass() {
            _classNum = 3;
        };
        ~ClassWarrior() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassWarrior();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassSpare1 : public CreatureClass {
    public:
        ClassSpare1() : CreatureClass() {
            _classNum = 4;
        };
        ~ClassSpare1() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassSpare1();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassPsionic : public CreatureClass {
    public:
        ClassPsionic() : CreatureClass() {
            _classNum = 5;
        };
        ~ClassPsionic() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassPsionic();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassPhysic : public CreatureClass {
    public:
        ClassPhysic() : CreatureClass() {
            _classNum = 6;
        };
        ~ClassPhysic() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassPhysic();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassCyborg : public CreatureClass {
    public:
        ClassCyborg() : CreatureClass() {
            _classNum = 7;
        };
        ~ClassCyborg() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassCyborg();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassPaladin : public CreatureClass {
    public:
        ClassPaladin() : CreatureClass() {
            _classNum = 8;
        };
        ~ClassPaladin() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassPaladin();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassRanger : public CreatureClass {
    public:
        ClassRanger() : CreatureClass() {
            _classNum = 9;
        };
        ~ClassRanger() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassRanger();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassBard : public CreatureClass {
    public:
        ClassBard() : CreatureClass() {
            _classNum = 10;
        };
        ~ClassBard() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassBard();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassMonk : public CreatureClass {
    public:
        ClassMonk() : CreatureClass() {
            _classNum = 11;
        };
        ~ClassMonk() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassMonk();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassVampire : public CreatureClass {
    public:
        ClassVampire() : CreatureClass() {
            _classNum = 12;
        };
        ~ClassVampire() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassVampire();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassMercenary : public CreatureClass {
    public:
        ClassMercenary() : CreatureClass() {
            _classNum = 13;
        };
        ~ClassMercenary() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassMercenary();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassBlackguard : public CreatureClass {
    public:
        ClassBlackguard() : CreatureClass() {
            _classNum = 14;
        };
        ~ClassBlackguard() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassBlackguard();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassNecromancer : public CreatureClass {
    public:
        ClassNecromancer() : CreatureClass() {
            _classNum = 15;
        };
        ~ClassNecromancer() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassNecromancer();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassSpare3 : public CreatureClass {
    public:
        ClassSpare3() : CreatureClass() {
            _classNum = 16;
        };
        ~ClassSpare3() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassSpare3();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassNormal : public CreatureClass {
    public:
        ClassNormal() : CreatureClass() {
            _classNum = 50;
        };
        ~ClassNormal() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassNormal();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassBird : public CreatureClass {
    public:
        ClassBird() : CreatureClass() {
            _classNum = 51;
        };
        ~ClassBird() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassBird();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassPredator : public CreatureClass {
    public:
        ClassPredator() : CreatureClass() {
            _classNum = 52;
        };
        ~ClassPredator() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassPredator();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassSnake : public CreatureClass {
    public:
        ClassSnake() : CreatureClass() {
            _classNum = 53;
        };
        ~ClassSnake() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassSnake();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassHorse : public CreatureClass {
    public:
        ClassHorse() : CreatureClass() {
            _classNum = 54;
        };
        ~ClassHorse() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassHorse();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassSmall : public CreatureClass {
    public:
        ClassSmall() : CreatureClass() {
            _classNum = 55;
        };
        ~ClassSmall() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassSmall();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassMedium : public CreatureClass {
    public:
        ClassMedium() : CreatureClass() {
            _classNum = 56;
        };
        ~ClassMedium() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassMedium();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassLarge : public CreatureClass {
    public:
        ClassLarge() : CreatureClass() {
            _classNum = 57;
        };
        ~ClassLarge() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassLarge();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassScientist : public CreatureClass {
    public:
        ClassScientist() : CreatureClass() {
            _classNum = 58;
        };
        ~ClassScientist() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassScientist();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassSkeleton : public CreatureClass {
    public:
        ClassSkeleton() : CreatureClass() {
            _classNum = 60;
        };
        ~ClassSkeleton() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassSkeleton();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassGhoul : public CreatureClass {
    public:
        ClassGhoul() : CreatureClass() {
            _classNum = 61;
        };
        ~ClassGhoul() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassGhoul();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassShadow : public CreatureClass {
    public:
        ClassShadow() : CreatureClass() {
            _classNum = 62;
        };
        ~ClassShadow() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassShadow();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassWight : public CreatureClass {
    public:
        ClassWight() : CreatureClass() {
            _classNum = 63;
        };
        ~ClassWight() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassWight();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassWraith : public CreatureClass {
    public:
        ClassWraith() : CreatureClass() {
            _classNum = 64;
        };
        ~ClassWraith() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassWraith();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassMummy : public CreatureClass {
    public:
        ClassMummy() : CreatureClass() {
            _classNum = 65;
        };
        ~ClassMummy() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassMummy();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassSpectre : public CreatureClass {
    public:
        ClassSpectre() : CreatureClass() {
            _classNum = 66;
        };
        ~ClassSpectre() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassSpectre();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassNpcVampire : public CreatureClass {
    public:
        ClassNpcVampire() : CreatureClass() {
            _classNum = 67;
        };
        ~ClassNpcVampire() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassNpcVampire();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassGhost : public CreatureClass {
    public:
        ClassGhost() : CreatureClass() {
            _classNum = 68;
        };
        ~ClassGhost() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassGhost();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassLich : public CreatureClass {
    public:
        ClassLich() : CreatureClass() {
            _classNum = 69;
        };
        ~ClassLich() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassLich();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassZombie : public CreatureClass {
    public:
        ClassZombie() : CreatureClass() {
            _classNum = 70;
        };
        ~ClassZombie() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassZombie();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassEarth : public CreatureClass {
    public:
        ClassEarth() : CreatureClass() {
            _classNum = 81;
        };
        ~ClassEarth() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassEarth();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassFire : public CreatureClass {
    public:
        ClassFire() : CreatureClass() {
            _classNum = 82;
        };
        ~ClassFire() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassFire();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassWater : public CreatureClass {
    public:
        ClassWater() : CreatureClass() {
            _classNum = 83;
        };
        ~ClassWater() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassWater();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassAir : public CreatureClass {
    public:
        ClassAir() : CreatureClass() {
            _classNum = 84;
        };
        ~ClassAir() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassAir();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassLightning : public CreatureClass {
    public:
        ClassLightning() : CreatureClass() {
            _classNum = 85;
        };
        ~ClassLightning() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassLightning();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassGreen : public CreatureClass {
    public:
        ClassGreen() : CreatureClass() {
            _classNum = 91;
        };
        ~ClassGreen() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassGreen();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassWhite : public CreatureClass {
    public:
        ClassWhite() : CreatureClass() {
            _classNum = 92;
        };
        ~ClassWhite() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassWhite();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassBlack : public CreatureClass {
    public:
        ClassBlack() : CreatureClass() {
            _classNum = 93;
        };
        ~ClassBlack() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassBlack();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassBlue : public CreatureClass {
    public:
        ClassBlue() : CreatureClass() {
            _classNum = 94;
        };
        ~ClassBlue() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassBlue();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassRed : public CreatureClass {
    public:
        ClassRed() : CreatureClass() {
            _classNum = 95;
        };
        ~ClassRed() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassRed();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassSilver : public CreatureClass {
    public:
        ClassSilver() : CreatureClass() {
            _classNum = 96;
        };
        ~ClassSilver() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassSilver();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassShadowD : public CreatureClass {
    public:
        ClassShadowD() : CreatureClass() {
            _classNum = 97;
        };
        ~ClassShadowD() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassShadowD();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassDeep : public CreatureClass {
    public:
        ClassDeep() : CreatureClass() {
            _classNum = 98;
        };
        ~ClassDeep() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassDeep();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassTurtle : public CreatureClass {
    public:
        ClassTurtle() : CreatureClass() {
            _classNum = 99;
        };
        ~ClassTurtle() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassTurtle();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassLeast : public CreatureClass {
    public:
        ClassLeast() : CreatureClass() {
            _classNum = 101;
        };
        ~ClassLeast() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassLeast();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassLesser : public CreatureClass {
    public:
        ClassLesser() : CreatureClass() {
            _classNum = 102;
        };
        ~ClassLesser() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassLesser();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassGreater : public CreatureClass {
    public:
        ClassGreater() : CreatureClass() {
            _classNum = 103;
        };
        ~ClassGreater() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassGreater();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassDuke : public CreatureClass {
    public:
        ClassDuke() : CreatureClass() {
            _classNum = 104;
        };
        ~ClassDuke() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassDuke();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassArch : public CreatureClass {
    public:
        ClassArch() : CreatureClass() {
            _classNum = 105;
        };
        ~ClassArch() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassArch();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassHill : public CreatureClass {
    public:
        ClassHill() : CreatureClass() {
            _classNum = 111;
        };
        ~ClassHill() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassHill();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassStone : public CreatureClass {
    public:
        ClassStone() : CreatureClass() {
            _classNum = 112;
        };
        ~ClassStone() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassStone();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassFrost : public CreatureClass {
    public:
        ClassFrost() : CreatureClass() {
            _classNum = 113;
        };
        ~ClassFrost() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassFrost();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassFireG : public CreatureClass {
    public:
        ClassFireG() : CreatureClass() {
            _classNum = 114;
        };
        ~ClassFireG() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassFireG();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassCloud : public CreatureClass {
    public:
        ClassCloud() : CreatureClass() {
            _classNum = 115;
        };
        ~ClassCloud() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassCloud();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassStorm : public CreatureClass {
    public:
        ClassStorm() : CreatureClass() {
            _classNum = 116;
        };
        ~ClassStorm() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassStorm();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassSlaadRed : public CreatureClass {
    public:
        ClassSlaadRed() : CreatureClass() {
            _classNum = 120;
        };
        ~ClassSlaadRed() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassSlaadRed();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassSlaadBlue : public CreatureClass {
    public:
        ClassSlaadBlue() : CreatureClass() {
            _classNum = 121;
        };
        ~ClassSlaadBlue() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassSlaadBlue();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassSlaadGreen : public CreatureClass {
    public:
        ClassSlaadGreen() : CreatureClass() {
            _classNum = 122;
        };
        ~ClassSlaadGreen() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassSlaadGreen();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassSlaadGrey : public CreatureClass {
    public:
        ClassSlaadGrey() : CreatureClass() {
            _classNum = 123;
        };
        ~ClassSlaadGrey() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassSlaadGrey();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassSlaadDeath : public CreatureClass {
    public:
        ClassSlaadDeath() : CreatureClass() {
            _classNum = 124;
        };
        ~ClassSlaadDeath() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassSlaadDeath();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassSlaadLord : public CreatureClass {
    public:
        ClassSlaadLord() : CreatureClass() {
            _classNum = 125;
        };
        ~ClassSlaadLord() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassSlaadLord();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassDemonI : public CreatureClass {
    public:
        ClassDemonI() : CreatureClass() {
            _classNum = 130;
        };
        ~ClassDemonI() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassDemonI();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassDemonIi : public CreatureClass {
    public:
        ClassDemonIi() : CreatureClass() {
            _classNum = 131;
        };
        ~ClassDemonIi() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassDemonIi();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassDemonIii : public CreatureClass {
    public:
        ClassDemonIii() : CreatureClass() {
            _classNum = 132;
        };
        ~ClassDemonIii() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassDemonIii();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassDemonIv : public CreatureClass {
    public:
        ClassDemonIv() : CreatureClass() {
            _classNum = 133;
        };
        ~ClassDemonIv() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassDemonIv();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassDemonV : public CreatureClass {
    public:
        ClassDemonV() : CreatureClass() {
            _classNum = 134;
        };
        ~ClassDemonV() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassDemonV();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassDemonVi : public CreatureClass {
    public:
        ClassDemonVi() : CreatureClass() {
            _classNum = 135;
        };
        ~ClassDemonVi() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassDemonVi();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassDemonSemi : public CreatureClass {
    public:
        ClassDemonSemi() : CreatureClass() {
            _classNum = 136;
        };
        ~ClassDemonSemi() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassDemonSemi();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassDemonMinor : public CreatureClass {
    public:
        ClassDemonMinor() : CreatureClass() {
            _classNum = 137;
        };
        ~ClassDemonMinor() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassDemonMinor();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassDemonMajor : public CreatureClass {
    public:
        ClassDemonMajor() : CreatureClass() {
            _classNum = 138;
        };
        ~ClassDemonMajor() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassDemonMajor();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassDemonLord : public CreatureClass {
    public:
        ClassDemonLord() : CreatureClass() {
            _classNum = 139;
        };
        ~ClassDemonLord() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassDemonLord();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassDemonPrince : public CreatureClass {
    public:
        ClassDemonPrince() : CreatureClass() {
            _classNum = 140;
        };
        ~ClassDemonPrince() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassDemonPrince();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassDevaAstral : public CreatureClass {
    public:
        ClassDevaAstral() : CreatureClass() {
            _classNum = 150;
        };
        ~ClassDevaAstral() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassDevaAstral();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassDevaMonadic : public CreatureClass {
    public:
        ClassDevaMonadic() : CreatureClass() {
            _classNum = 151;
        };
        ~ClassDevaMonadic() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassDevaMonadic();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassDevaMovanic : public CreatureClass {
    public:
        ClassDevaMovanic() : CreatureClass() {
            _classNum = 152;
        };
        ~ClassDevaMovanic() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassDevaMovanic();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassMephitFire : public CreatureClass {
    public:
        ClassMephitFire() : CreatureClass() {
            _classNum = 160;
        };
        ~ClassMephitFire() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassMephitFire();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassMephitLava : public CreatureClass {
    public:
        ClassMephitLava() : CreatureClass() {
            _classNum = 161;
        };
        ~ClassMephitLava() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassMephitLava();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassMephitSmoke : public CreatureClass {
    public:
        ClassMephitSmoke() : CreatureClass() {
            _classNum = 162;
        };
        ~ClassMephitSmoke() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassMephitSmoke();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassMephitSteam : public CreatureClass {
    public:
        ClassMephitSteam() : CreatureClass() {
            _classNum = 163;
        };
        ~ClassMephitSteam() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassMephitSteam();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassDaemonArcana : public CreatureClass {
    public:
        ClassDaemonArcana() : CreatureClass() {
            _classNum = 170;
        };
        ~ClassDaemonArcana() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassDaemonArcana();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassDaemonCharona : public CreatureClass {
    public:
        ClassDaemonCharona() : CreatureClass() {
            _classNum = 171;
        };
        ~ClassDaemonCharona() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassDaemonCharona();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassDaemonDergho : public CreatureClass {
    public:
        ClassDaemonDergho() : CreatureClass() {
            _classNum = 172;
        };
        ~ClassDaemonDergho() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassDaemonDergho();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassDaemonHydro : public CreatureClass {
    public:
        ClassDaemonHydro() : CreatureClass() {
            _classNum = 173;
        };
        ~ClassDaemonHydro() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassDaemonHydro();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassDaemonPisco : public CreatureClass {
    public:
        ClassDaemonPisco() : CreatureClass() {
            _classNum = 174;
        };
        ~ClassDaemonPisco() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassDaemonPisco();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassDaemonUltro : public CreatureClass {
    public:
        ClassDaemonUltro() : CreatureClass() {
            _classNum = 175;
        };
        ~ClassDaemonUltro() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassDaemonUltro();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassDaemonYagno : public CreatureClass {
    public:
        ClassDaemonYagno() : CreatureClass() {
            _classNum = 176;
        };
        ~ClassDaemonYagno() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassDaemonYagno();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassDaemonPyro : public CreatureClass {
    public:
        ClassDaemonPyro() : CreatureClass() {
            _classNum = 177;
        };
        ~ClassDaemonPyro() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassDaemonPyro();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassGodling : public CreatureClass {
    public:
        ClassGodling() : CreatureClass() {
            _classNum = 178;
        };
        ~ClassGodling() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassGodling();
        };

        void modifyEO(ExecutableObject *eo) {};
};

class ClassDiety : public CreatureClass {
    public:
        ClassDiety() : CreatureClass() {
            _classNum = 179;
        };
        ~ClassDiety() {};

        CreatureClass *createNewInstance() {
            return (CreatureClass *) new ClassDiety();
        };

        void modifyEO(ExecutableObject *eo) {};
};

void register_classes() {
    CreatureClass::insertClass(new ClassMage());
    CreatureClass::insertClass(new ClassCleric());
    CreatureClass::insertClass(new ClassThief());
    CreatureClass::insertClass(new ClassWarrior());
    CreatureClass::insertClass(new ClassPsionic());
    CreatureClass::insertClass(new ClassPhysic());
    CreatureClass::insertClass(new ClassCyborg());
    CreatureClass::insertClass(new ClassPaladin());
    CreatureClass::insertClass(new ClassRanger());
    CreatureClass::insertClass(new ClassBard());
    CreatureClass::insertClass(new ClassMonk());
    CreatureClass::insertClass(new ClassVampire());
    CreatureClass::insertClass(new ClassMercenary());
    CreatureClass::insertClass(new ClassBlackguard());
    CreatureClass::insertClass(new ClassNecromancer());
    CreatureClass::insertClass(new ClassSpare3());
    CreatureClass::insertClass(new ClassNormal());
    CreatureClass::insertClass(new ClassBird());
    CreatureClass::insertClass(new ClassPredator());
    CreatureClass::insertClass(new ClassSnake());
    CreatureClass::insertClass(new ClassHorse());
    CreatureClass::insertClass(new ClassSmall());
    CreatureClass::insertClass(new ClassMedium());
    CreatureClass::insertClass(new ClassLarge());
    CreatureClass::insertClass(new ClassScientist());
    CreatureClass::insertClass(new ClassSkeleton());
    CreatureClass::insertClass(new ClassGhoul());
    CreatureClass::insertClass(new ClassShadow());
    CreatureClass::insertClass(new ClassWight());
    CreatureClass::insertClass(new ClassWraith());
    CreatureClass::insertClass(new ClassMummy());
    CreatureClass::insertClass(new ClassSpectre());
    CreatureClass::insertClass(new ClassNpcVampire());
    CreatureClass::insertClass(new ClassGhost());
    CreatureClass::insertClass(new ClassLich());
    CreatureClass::insertClass(new ClassZombie());
    CreatureClass::insertClass(new ClassEarth());
    CreatureClass::insertClass(new ClassFire());
    CreatureClass::insertClass(new ClassWater());
    CreatureClass::insertClass(new ClassAir());
    CreatureClass::insertClass(new ClassLightning());
    CreatureClass::insertClass(new ClassGreen());
    CreatureClass::insertClass(new ClassWhite());
    CreatureClass::insertClass(new ClassBlack());
    CreatureClass::insertClass(new ClassBlue());
    CreatureClass::insertClass(new ClassRed());
    CreatureClass::insertClass(new ClassSilver());
    CreatureClass::insertClass(new ClassShadowD());
    CreatureClass::insertClass(new ClassDeep());
    CreatureClass::insertClass(new ClassTurtle());
    CreatureClass::insertClass(new ClassLeast());
    CreatureClass::insertClass(new ClassLesser());
    CreatureClass::insertClass(new ClassGreater());
    CreatureClass::insertClass(new ClassDuke());
    CreatureClass::insertClass(new ClassArch());
    CreatureClass::insertClass(new ClassHill());
    CreatureClass::insertClass(new ClassStone());
    CreatureClass::insertClass(new ClassFrost());
    CreatureClass::insertClass(new ClassFireG());
    CreatureClass::insertClass(new ClassCloud());
    CreatureClass::insertClass(new ClassStorm());
    CreatureClass::insertClass(new ClassSlaadRed());
    CreatureClass::insertClass(new ClassSlaadBlue());
    CreatureClass::insertClass(new ClassSlaadGreen());
    CreatureClass::insertClass(new ClassSlaadGrey());
    CreatureClass::insertClass(new ClassSlaadDeath());
    CreatureClass::insertClass(new ClassSlaadLord());
    CreatureClass::insertClass(new ClassDemonI());
    CreatureClass::insertClass(new ClassDemonIi());
    CreatureClass::insertClass(new ClassDemonIii());
    CreatureClass::insertClass(new ClassDemonIv());
    CreatureClass::insertClass(new ClassDemonV());
    CreatureClass::insertClass(new ClassDemonVi());
    CreatureClass::insertClass(new ClassDemonSemi());
    CreatureClass::insertClass(new ClassDemonMinor());
    CreatureClass::insertClass(new ClassDemonMajor());
    CreatureClass::insertClass(new ClassDemonLord());
    CreatureClass::insertClass(new ClassDemonPrince());
    CreatureClass::insertClass(new ClassDevaAstral());
    CreatureClass::insertClass(new ClassDevaMonadic());
    CreatureClass::insertClass(new ClassDevaMovanic());
    CreatureClass::insertClass(new ClassMephitFire());
    CreatureClass::insertClass(new ClassMephitLava());
    CreatureClass::insertClass(new ClassMephitSmoke());
    CreatureClass::insertClass(new ClassMephitSteam());
    CreatureClass::insertClass(new ClassDaemonArcana());
    CreatureClass::insertClass(new ClassDaemonCharona());
    CreatureClass::insertClass(new ClassDaemonDergho());
    CreatureClass::insertClass(new ClassDaemonHydro());
    CreatureClass::insertClass(new ClassDaemonPisco());
    CreatureClass::insertClass(new ClassDaemonUltro());
    CreatureClass::insertClass(new ClassDaemonYagno());
    CreatureClass::insertClass(new ClassDaemonPyro());
    CreatureClass::insertClass(new ClassGodling());
    CreatureClass::insertClass(new ClassDiety());

}
