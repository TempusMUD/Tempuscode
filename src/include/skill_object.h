#ifndef _SKILL_OBJECT_H_
#define _SKILL_OBJECT_H_

#include <vector>
#include <map>

#include "constants.h"

struct Creature;
class ExecutableObject;
class ExecutableVector;

using namespace std;

int mag_savingthrow(struct Creature *ch, int level, int type);

// Forward declarations
struct affected_type;

// Got to have this forward declaration so we can declare the map below
// before the class SkillObject is delclared.  Since SkillObject::SkillObject()
// uses the map, it can't simply be declared after the class.  Fucked up eh?
// It gets worse...
class SkillObject;

// Global map for each skill object.  
// THERE CAN BE ONLY ONE! (instance of each skill that is)
//  --Highlander

map<int, SkillObject *> skillMap;

// Map is kind of stupid.  It doesn't know how to tell if it contains the key
// you're looking for or not.  And instead of doing something sensible when
// you use the [] operator for a key it doesn't contain, it inserts a default
// object into the map at that key.  Hence, since the & operator can't
// possibly make sense within the scope of a map, I overloaded it here to
// check if a certain key exists or not.

bool operator&(int snum, map<int, SkillObject*>& mymap) 
{
    map<int, SkillObject *>::iterator mi;

    if (mymap.empty())
        return false;

    mi = mymap.lower_bound(snum);
    if ((*mi).first == snum)
        return true;

    return false;
}

bool operator&(map<int, SkillObject*>& mymap, int snum) {
    return snum & mymap;
}


// Ok, first of all, I know this LOOKS fucked up at first.  And honestly,
// where I came up with it I don't know.  As Azimuth once put it, at this
// point I dunno whether I should be proud or hang myself.  I'll comment
// how each bit of it works.

class SkillObject {
    public:
        // This is required to exist and be public by the map template
        bool operator<(const SkillObject &c) {
            return _skillNum < c._skillNum;
        }
        virtual ~SkillObject();

        //normal members
        virtual void perform(Creature *ch, Creature *target, ExecutableObject *eo);
        virtual void defend(ExecutableVector &ev);

    protected:
        // Here's the constructor, I wanted it protected because it allows me to
        // call it from the initialization list of a derived class, but nowhere
        // elese.  So an object of class SkillObject can never be instantiated.
        // That's good.
        // First we set this instances _skillNum variable, but we're only going
        // to insert it into the global map of skills if it doesn't already
        // exist.  The other option was to write singleton patterns, which for
        // various reasons I decided against.
        SkillObject(int snum) {
            _skillNum = snum;

            if (!(skillMap & snum))
                skillMap[snum] = this;

            // Reasonable defaults (I hope)
            _gen = 0;
            _minPos = POS_STANDING;
            _maxMana = 0;
            _minMana = 0;
            _manaChange = 0;
            _targets = TAR_CHAR_ROOM | TAR_FIGHT_VICT;
            _violent = false;
            _flags = MAG_AFFECTS;
            _avatar = 0;
            _saveType = SAVING_NONE;

            for (short i = 0; i < NUM_CLASSES; i++)
                _level[i] = 50;
        }
        
        // Various important members here so the derived classes can access them
        // without accessors;
        short _gen;
        short _minPos;
        short _maxMana;
        short _minMana;
        short _manaChange;
        short _saveType;
        int _targets;
        int _flags;
        int _skillNum;
        bool _violent;
        bool _avatar;
        map<short, short>_level;

    private:
        // These guys should NEVER be called.  Hence, they are private
        // and have no implementation.
        SkillObject(const SkillObject &c);
        SkillObject &operator =(const SkillObject &c);
};

#endif
