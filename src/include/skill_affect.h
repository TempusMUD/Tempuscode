#ifndef _SKILL_AFFECT_H_
#define _SKILL_AFFECT_H_

#include <vector>

#include "executable_object.h"

struct affected_type;

using namespace std;

class SkillAffect : public ExecutableObject {
    public:
        // Constructor & destructor
        SkillAffect();
        ~SkillAffect();

        void addAffect(struct affected_type *aff);
        void removeAffect(struct affected_type *aff);
        void setAccumDuration(bool accum = true);
        void setAccumAffect(bool accum = true);
        void setSkillNum(int snum);
        bool getAccumDuration();
        bool getAccumAffect();
        int getSkillNum();

        // Normal Members
        void execute();

    private:
        int _skillNum;
        bool _accumDuration;
        bool _accumAffect;
        vector<struct affected_type *> _affArray;
};


#endif
