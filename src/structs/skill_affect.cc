#include "skill_affect.h"
#include "spells.h"
#include "handler.h"
#include "utils.h"
#include "creature.h"

extern char* NOEFFECT;

// Constructor & destructor
SkillAffect::SkillAffect() : ExecutableObject()
{
    _skillNum = 0;
    _accumDuration = false;
    _accumAffect = false;
}

SkillAffect::~SkillAffect()
{
    for (unsigned int x = 0; x < _affArray.size(); x++)
        delete _affArray[x];
}

void 
SkillAffect::addAffect(struct affected_type *aff)
{
    _affArray.push_back(aff);
}

void 
SkillAffect::removeAffect(struct affected_type *aff)
{
    vector<struct affected_type *>::iterator vi = _affArray.begin();
    for (int i = 0; vi != _affArray.end(); ++i, ++vi) {
        struct affected_type *tempAff = _affArray[i];
        if (*tempAff == *aff) {
            _affArray.erase(vi);
            break;
        }
    }
}

void 
SkillAffect::setAccumDuration(bool accum)
{
    _accumDuration = accum;
}

void 
SkillAffect::setAccumAffect(bool accum)
{
    _accumAffect = accum;
}

void 
SkillAffect::setSkillNum(int snum)
{
    _skillNum = snum;
}

bool 
SkillAffect::getAccumDuration()
{
    return _accumDuration;
}

bool 
SkillAffect::getAccumAffect()
{
    return _accumAffect;
}

int 
SkillAffect::getSkillNum()
{
    return _skillNum;
}

// Normal Members
void 
SkillAffect::execute()
{
    for (unsigned int i=0; i<_affArray.size(); i++) {
        struct affected_type *afp = _affArray[i];
        
        /*
        * If this is a mob that has this affect set in its mob file, do not
        * perform the affect.  This prevents people from un-sancting mobs
        * by sancting them and waiting for it to fade, for example.
        */
        // Nathan, there's no need to use the scope operator on the call 
        // to addToCharMessage() below.  The scope operator is only needed
        // when a derived class reimplents a method from a base class and
        // you want to specifically call the base class's version.
        // Also, I believe we should clear the messages before adding
        // NOEFFECT. ;-)
        // On a side note, the reason using the scope operator when you
        // done need it is bad is that in a function like this, when you
        // use it, it actually creates an instance of the class you're
        // refering to.  Which is inefficient if you don't need it.
        if (IS_NPC(_victim)) {
            if (afp->aff_index == 0) {
                if (IS_AFFECTED(_victim, afp->bitvector) &&
                    !affected_by_spell(_victim, _skillNum)) {
                    getCharMessages().clear();
                    addToCharMessage(NOEFFECT);
                    continue;
                }
            } else if (afp->aff_index == 2) {
                if (IS_AFFECTED_2(_victim, afp->bitvector) &&
                    !affected_by_spell(_victim, _skillNum)) {
                    getVictMessages().clear();
                    addToVictMessage(NOEFFECT);
                    continue;
                }
            } else if (afp->aff_index == 3) {
                if (IS_AFFECTED_3(_victim, afp->bitvector) &&
                    !affected_by_spell(_victim, _skillNum)) {
                    getRoomMessages().clear();
                    addToRoomMessage(NOEFFECT);
                    continue;
                }
            }
        }
        
        /* If the victim is already affected by this spell, and the spell does
        * not have an accumulative effect, then fail the spell.
        */
        if (affected_by_spell(_victim, _skillNum) && !(_accumDuration || _accumAffect))
            break;
        
        affect_join(_victim, afp, _accumDuration, false, _accumAffect, false);
    }

    ExecutableObject::execute(); 
}
