#ifndef _CREATURE_CLASS_H_
#define _CREATURE_CLASS_H_

#include <map>

class ExecutableObject;
class Creature;
class CreatureClass;

using namespace std;

typedef map<int, CreatureClass *> ClassMap;

class CreatureClass {
    public:
        CreatureClass() {
            _classNum = 0;
        };
        virtual ~CreatureClass() {};

        virtual void modifyEO(ExecutableObject *eo) = 0;
        virtual CreatureClass *createNewInstance() = 0;

        static CreatureClass *constructClass(int classNum);
        static void insertClass(CreatureClass *theClass);

        // Accessors
        Creature *getOwner() { return _owner; };

        // Manipulators
        void setOwner(Creature *owner) { _owner = owner; };

    protected:
        int _classNum;

        Creature *_owner;

        static bool classExists(int classNum) {
            ClassMap::iterator mi;
            mi = _getClassMap()->find(classNum);
            return mi != _getClassMap()->end();
        }

        static ClassMap* _getClassMap() {
            static ClassMap classes;
            return &classes;
        }
};

#endif
