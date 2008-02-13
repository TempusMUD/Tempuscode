#include "creature.h"
#include "utils.h"
#include "spells.h"
#include "generic_skill.h"

extern void send_to_char(struct Creature *ch, const char *str, ...);

extern struct syllable syls[];

// This destructor is virtual!  DO NOT PUT ANY CODE HERE!
GenericSpell::~GenericSpell() {};

bool
GenericSpell::perform(ExecutableVector &ev, Creature *target) {
    if (!GenericSkill::perform(ev, target))
        return false;

    if (!canCast())
        return false;

    return true;
}

bool
GenericSpell::perform(ExecutableVector &ev, obj_data *obj) {
    if (!GenericSkill::perform(ev, obj))
        return false;

    if (!canCast())
        return false;

    return true;
}

bool
GenericSpell::perform(ExecutableVector &ev, char *targets) {
    if (!GenericSkill::perform(ev))
        return false;
    
    if (!canCast())
        return false;
    
    return true;
}

bool
GenericSpell::canCast() {
    if (_objectMagic)
        return true;

    if (!_owner->isImmortal()) {
        if ((SECT_TYPE(_owner->in_room) == SECT_UNDERWATER ||
            SECT_TYPE(_owner->in_room) == SECT_DEEP_OCEAN)
        && flagIsSet(MAG_NOWATER)) {
            send_to_char(_owner, "This %s does not function underwater.\r\n",
            getTypeDescription().c_str());
            return false;
        }
        
        if (!OUTSIDE(_owner) && flagIsSet(MAG_OUTDOORS)) {
            send_to_char(_owner, "This %s can only be used outdoors.\r\n",
            getTypeDescription().c_str());
            return false;
        }
        
        if (flagIsSet(MAG_NOSUN) && OUTSIDE(_owner)
        && !room_is_dark(_owner->in_room)) {
            send_to_char(_owner, "This %s cannot be used in sunlight.\r\n",
            getTypeDescription().c_str());
            return false;
        }
    }
    
    return true;
}

ExecutableObject *
GenericSpell::generateSkillMessage(Creature *victim, obj_data *obj)
{
    char *lbuf;
    char *xlated = "";
    int j, ofs = 0;
    char *toChar = NULL, *toRoom = NULL, *toVict = NULL;

    if (_objectMagic)
        return NULL;

    lbuf = tmp_strdup(_skillName.c_str());

    while (*(lbuf + ofs)) {
        for (j = 0; *(syls[j].org); j++) {
            if (!strncasecmp(syls[j].org, lbuf + ofs, strlen(syls[j].org))) {
                xlated = tmp_strcat(xlated, syls[j].new_syl, NULL);
                ofs += strlen(syls[j].org);
            }
        }
    }

    ExecutableObject *eo = new ExecutableObject(getOwner());
    if (victim && victim->in_room == _owner->in_room) {
        eo->setVictim(victim);
        if (victim == _owner) {
            toRoom = tmp_sprintf("$n closes $s eyes and utters, '%s'.", xlated);
            toChar = tmp_sprintf("You close your eyes and utter, '%s'.", xlated);
        }
        else {
            toRoom = tmp_sprintf("$n stares at $N and utters, '%s'.", xlated);
            toChar = tmp_sprintf("You stare at $N and utter, '%s'.", xlated);
            toVict = tmp_sprintf("$n stares at you and utters, '%s'.", xlated);
        }
    }
    else if (obj &&
        ((obj->in_room == _owner->in_room) || (obj->carried_by == _owner))) {
        eo->setObject(obj);
        toRoom = tmp_sprintf("$n stares at $p and utters, '%s'.", xlated);
        toChar = tmp_sprintf("You stare at $p and utter, '%s'.", xlated);
    }
    else {
        toRoom = tmp_sprintf("$n utters the words, '%s'.", xlated);
        toChar = tmp_sprintf("You utter the words, '%s'.", xlated);
    }

    if (toChar)
        eo->addToCharMessage(string(toChar));
    if (toVict)
        eo->addToVictMessage(string(toVict));
    if (toRoom)
        eo->addToRoomMessage(string(toRoom));

    return eo;
}
