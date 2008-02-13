#include "generic_skill.h"
#include "utils.h"

static const int LEVEL_NEEDS_INSTRUMENT = 10;

extern bool check_instrument(Creature *ch, int songnum);
extern char *get_instrument_type(int songnum);
extern void send_to_char(struct Creature *ch, const char *str, ...);
extern void sing_song(struct Creature *ch, struct Creature *tch,
          struct obj_data *tobj, int spellnum);

bool
SongSpell::perform(ExecutableVector &ev, Creature *target) {
    if (!GenericSpell::perform(ev, target))
        return false;
    
    if (!canPerform())
        return false;
    
    sing_song(_owner, target, NULL, getSkillNumber());
    return true;
}

bool
SongSpell::perform(ExecutableVector &ev, obj_data *obj) {
    if (!GenericSpell::perform(ev, obj))
        return false;

    if (!canPerform())
        return false;

    sing_song(_owner, NULL, obj, getSkillNumber());
    return true;
}

bool
SongSpell::perform(ExecutableVector &ev, char *targets) {
    if (!GenericSpell::perform(ev))
        return false;

    if (!canPerform())
        return false;

    sing_song(_owner, NULL, NULL, getSkillNumber());
    return true;
}

bool
SongSpell::canPerform() {
    if (_objectMagic)
        return true;

    if (!_owner->isImmortal()) {
        if (canBeLearned(_owner) == PracticeAbility::CannotLearn) {
            send_to_char(_owner, "Sing what?!?\r\n");
            return false;
        }
        
        if (TOTAL_ENCUM(_owner) > (CAN_CARRY_W(_owner) * 0.90)) {
            send_to_char(_owner, "Your equipment is too heavy and bulky "
            "for you to perform!\r\n");
            return false;
        }
        
        if ((_owner->getLevel() > LEVEL_NEEDS_INSTRUMENT) &&
        !check_instrument(_owner, _skillNum)) {
            send_to_char(_owner, "But you're not using a %s instrument!\r\n",
            get_instrument_type(_skillNum));
            return false;
        }
        
        if ((SECT_TYPE(_owner->in_room) == SECT_UNDERWATER
        || SECT_TYPE(_owner->in_room) == SECT_DEEP_OCEAN)) {
            send_to_char(_owner, "You can't sing or play underwater!.\r\n");
            return false;
        }
    }

    gainSkillProficiency();

    return true;
}
