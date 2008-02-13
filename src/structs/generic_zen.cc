#include "generic_skill.h"

bool
GenericZen::perform(ExecutableVector &ev, Creature *target) {
    gainSkillProficiency();
    return GenericSkill::perform(ev, target);
}

bool
GenericZen::perform(ExecutableVector &ev, obj_data *obj) {
    gainSkillProficiency();
    return GenericSkill::perform(ev, obj);
}

bool
GenericZen::perform(ExecutableVector &ev, char *targets) {
    gainSkillProficiency();
    return GenericSkill::perform(ev);
}


