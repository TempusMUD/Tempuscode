#include "generic_skill.h"
#include "screen.h"
#include "generic_affect.h"
#include "creature.h"
#include "utils.h"
#include "comm.h"

bool
GenericProgram::perform(ExecutableVector &ev, char *targets) {
    if (!GenericSkill::perform(ev))
        return false;

    CreatureAffect *aff = (CreatureAffect *)_owner->affectedBy(_skillNum);
    if (aff) {
        const char *name = GenericSkill::getNameByNumber(_skillNum).c_str();
        send_to_char(_owner, "%sERROR:%s %s %s already activated.\r\n",
            CCCYN(_owner, C_NRM), CCNRM(_owner, C_NRM),
            name, ISARE(name));
        return false;
    }
        
    gainSkillProficiency();

    return true;
}

bool
GenericProgram::deactivate() {
    CreatureAffect *aff = (CreatureAffect *)_owner->affectedBy(_skillNum);
    if (!aff) {
        const char *name = GenericSkill::getNameByNumber(_skillNum).c_str();
        send_to_char(_owner, "%sERROR:%s %s %s not currently activated.\r\n",
            CCCYN(_owner, C_NRM), CCNRM(_owner, C_NRM),
            name, ISARE(name));

        return false;
    }

    while ((aff = (CreatureAffect *)_owner->affectedBy(_skillNum))) {
        aff->setIsPermenant(false);
        aff->setDuration(0);
        aff->handleTick();
    }

    return true;
}

