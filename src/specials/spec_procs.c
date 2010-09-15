/* ************************************************************************
*   File: spec_procs.c                                  Part of CircleMUD *
*  Usage: implementation of special procedures for mobiles/objects/rooms  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: spec_procs.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include "structs.h"
#include "actions.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "char_class.h"
#include "screen.h"
#include "clan.h"
#include "vehicle.h"
#include "materials.h"
#include "paths.h"
#include "security.h"
#include "olc.h"
#include "specs.h"
#include "login.h"
#include "house.h"
#include "fight.h"
#include "tmpstr.h"
#include "accstr.h"
#include "players.h"
#include "weather.h"

/*   external vars  */
extern struct descriptor_data *descriptor_list;
extern struct time_info_data time_info;
extern struct spell_info_type spell_info[];

/* extern functions */
void add_follower(struct creature *ch, struct creature *leader);
void do_auto_exits(struct creature *ch, room_num room);
int get_check_money(struct creature *ch, struct obj_data **obj, int display);

ACMD(do_echo);
ACMD(do_gen_comm);
ACMD(do_rescue);
ACMD(do_steal);
ACCMD(do_get);

/* ********************************************************************
*  Special procedures for mobiles                                     *
******************************************************************** */
// skill_gain: mode==true means to return a skill gain value
//             mode==false means to return an average value
int
skill_gain(struct creature *ch, int mode)
{
    if (mode)
        return (number(MINGAIN(ch), MAXGAIN(ch)));
    else                        // average (for stat)
        return ((MAXGAIN(ch) + MINGAIN(ch)) / 2);
}

extern int spell_sort_info[MAX_SPELLS + 1];
extern int skill_sort_info[MAX_SKILLS - MAX_SPELLS + 1];

void
sort_spells(void)
{
    int a, b, tmp;

    /* initialize array */
    for (a = 1; a < MAX_SPELLS; a++)
        spell_sort_info[a] = a;

    /* Sort.  'a' starts at 1, not 0, to remove 'RESERVED' */
    for (a = 1; a < MAX_SPELLS - 1; a++)
        for (b = a + 1; b < MAX_SPELLS; b++)
            if (strcmp(spell_to_str(spell_sort_info[a]),
                    spell_to_str(spell_sort_info[b])) > 0) {
                tmp = spell_sort_info[a];
                spell_sort_info[a] = spell_sort_info[b];
                spell_sort_info[b] = tmp;
            }
}

void
sort_skills(void)
{
    int a, b, tmp;

    /* initialize array */
    for (a = 1; a < MAX_SKILLS - MAX_SPELLS; a++)
        skill_sort_info[a] = a + MAX_SPELLS;

    /* Sort.  'a' starts at 1, not 0, to remove 'RESERVED' */
    for (a = 1; a < MAX_SKILLS - MAX_SPELLS - 1; a++)
        for (b = a + 1; b < MAX_SKILLS - MAX_SPELLS; b++)
            if (strcmp(spell_to_str(skill_sort_info[a]),
                    spell_to_str(skill_sort_info[b])) > 0) {
                tmp = skill_sort_info[a];
                skill_sort_info[a] = skill_sort_info[b];
                skill_sort_info[b] = tmp;
            }
}

char *
how_good(int percent)
{
    static char buf[256];

    if (percent < 0)
        strcpy(buf, " (terrible)");
    else if (percent == 0)
        strcpy(buf, " (not learned)");
    else if (percent <= 10)
        strcpy(buf, " (awful)");
    else if (percent <= 20)
        strcpy(buf, " (bad)");
    else if (percent <= 40)
        strcpy(buf, " (poor)");
    else if (percent <= 55)
        strcpy(buf, " (average)");
    else if (percent <= 70)
        strcpy(buf, " (fair)");
    else if (percent <= 80)
        strcpy(buf, " (good)");
    else if (percent <= 85)
        strcpy(buf, " (very good)");
    else if (percent <= 100)
        strcpy(buf, " (superb)");
    else if (percent <= 150)
        strcpy(buf, " (extraordinary)");
    else
        strcpy(buf, " (superhuman)");

    return (buf);
}

const char *prac_types[] = {
    "spell",
    "skill",
    "trigger",
    "alteration",
    "program",
    "zen art",
    "song",
    "\n",
};

#define PRAC_TYPE        3      /* should it say 'spell' or 'skill'?         */

/* actual prac_params are in char_class.c */

void
list_skills(struct creature *ch, int mode, int type)
{
    int i, sortpos;

    acc_string_clear();
    if ((type == 1 || type == 3) &&
        ((prac_params[PRAC_TYPE][(int)GET_CLASS(ch)] != 1 &&
                prac_params[PRAC_TYPE][(int)GET_CLASS(ch)] != 4) ||
            (GET_REMORT_CLASS(ch) >= 0 &&
                (prac_params[PRAC_TYPE][(int)GET_REMORT_CLASS(ch)] != 1 &&
                    prac_params[PRAC_TYPE][(int)GET_REMORT_CLASS(ch)] !=
                    1)))) {
        acc_sprintf("%s%sYou know %sthe following %ss:%s\r\n",
            CCYEL(ch, C_CMP), CCBLD(ch, C_SPR), mode ? "of " : "", SPLSKL(ch),
            CCNRM(ch, C_SPR));

        for (sortpos = 1; sortpos < MAX_SPELLS; sortpos++) {
            i = spell_sort_info[sortpos];
            if ((CHECK_SKILL(ch, i) || is_able_to_learn(ch, i)) &&
                SPELL_LEVEL(i, 0) <= LVL_GRIMP) {
                if (!mode && !CHECK_SKILL(ch, i))   // !mode => list only learned
                    continue;
                if (IS_IMMORT(ch))
                    acc_sprintf("%s%s%-30s %s%-17s%s %s(%3d mana)%s\r\n",
                        CCGRN(ch, C_NRM),
                        tmp_sprintf("%3d. ", i), spell_to_str(i),
                        CCBLD(ch, C_SPR), how_good(CHECK_SKILL(ch, i)),
                        tmp_sprintf("%s[%3d]", CCYEL(ch, C_NRM),
                            CHECK_SKILL(ch, i)), CCRED(ch, C_SPR),
                        mag_manacost(ch, i), CCNRM(ch, C_SPR));
                else
                    acc_sprintf("%s%-30s %s%-17s %s(%3d mana)%s\r\n",
                        CCGRN(ch, C_NRM), spell_to_str(i),
                        CCBLD(ch, C_SPR), how_good(CHECK_SKILL(ch, i)),
                        CCRED(ch, C_SPR), mag_manacost(ch, i),
                        CCNRM(ch, C_SPR));
            }
        }

        if (type != 2 && type != 3) {
            page_string(ch->desc, acc_get_string());
            return;
        }

        acc_sprintf("\r\n%s%sYou know %sthe following %s:%s\r\n",
            CCYEL(ch, C_CMP), CCBLD(ch, C_SPR),
            mode ? "of " : "", IS_CYBORG(ch) ? "programs" :
            "skills", CCNRM(ch, C_SPR));
    } else {
        acc_sprintf("%s%sYou know %sthe following %s:%s\r\n",
            CCYEL(ch, C_CMP), CCBLD(ch, C_SPR),
            mode ? "of " : "", IS_CYBORG(ch) ? "programs"
            : "skills", CCNRM(ch, C_SPR));
    }

    for (sortpos = 1; sortpos < MAX_SKILLS - MAX_SPELLS; sortpos++) {
        i = skill_sort_info[sortpos];
        if ((CHECK_SKILL(ch, i) || is_able_to_learn(ch, i)) &&
            SPELL_LEVEL(i, 0) <= LVL_GRIMP) {
            if (!mode && !CHECK_SKILL(ch, i))   // !mode => list only learned
                continue;

            if (IS_IMMORT(ch)) {
                acc_sprintf("%s%s%-30s %s%-17s%s%s\r\n",
                    CCGRN(ch, C_NRM), tmp_sprintf("%3d. ", i),
                    spell_to_str(i), CCBLD(ch, C_SPR),
                    how_good(GET_SKILL(ch, i)),
                    tmp_sprintf("%s[%3d]%s", CCYEL(ch, C_NRM),
                        CHECK_SKILL(ch, i), CCNRM(ch, C_NRM)),
                    CCNRM(ch, C_SPR));
            } else {
                acc_sprintf("%s%-30s %s%s%s\r\n",
                    CCGRN(ch, C_NRM), spell_to_str(i), CCBLD(ch, C_SPR),
                    how_good(GET_SKILL(ch, i)), CCNRM(ch, C_SPR));
            }
        }
    }
    page_string(ch->desc, acc_get_string());
}

SPECIAL(guild)
{
    int skill_num, percent;
    struct creature *master = (struct creature *)me;
    long int cost;

    if (spec_mode != SPECIAL_CMD)
        return 0;
    if (!AWAKE(ch) || !AWAKE(master))
        return 0;

    if (CMD_IS("list")) {
        list_skills(ch, 1, 3);
        return 1;
    }

    if (!(CMD_IS("practice") ||
            CMD_IS("train") || CMD_IS("learn") || CMD_IS("offer")))
        return 0;

    skip_spaces(&argument);

    if (!*argument) {
        if (CMD_IS("offer"))
            perform_tell(master, ch,
                "For what skill would you like to know the price of training?");
        else
            list_skills(ch, 1, 3);
        return 1;
    }

    if ((GET_CLASS(master) != CLASS_NORMAL
            || GET_LEVEL(ch) > LVL_CAN_RETURN
            || GET_REMORT_GEN(ch) > 0) &&
        GET_CLASS(ch) != GET_CLASS(master) &&
        GET_CLASS(master) != CHECK_REMORT_CLASS(ch) &&
        (!IS_REMORT(master) ||
            CHECK_REMORT_CLASS(ch) != GET_CLASS(ch)) &&
        (!IS_REMORT(master) ||
            CHECK_REMORT_CLASS(ch) != CHECK_REMORT_CLASS(ch))) {
        perform_tell(master, ch, "Go to your own guild to practice!");
        return 1;
    }

    skill_num = find_skill_num(argument);

    if (skill_num < 1) {
        perform_tell(master, ch,
            tmp_sprintf("You do not know of that %s!", SPLSKL(ch)));
        return 1;
    }
    if (!is_able_to_learn(ch, skill_num)) {
        if (CHECK_SKILL(ch, skill_num))
            perform_tell(master, ch,
                tmp_sprintf("I cannot teach you %s yet.",
                    spell_to_str(skill_num)));
        else
            perform_tell(master, ch,
                tmp_sprintf("You are not yet ready to practice %s.",
                    spell_to_str(skill_num)));
        return 1;
    }

    if (skill_num < 1 ||
        (GET_CLASS(master) != CLASS_NORMAL &&
            GET_LEVEL(master) < SPELL_LEVEL(skill_num, GET_CLASS(master)) &&
            (!IS_REMORT(master) ||
                GET_LEVEL(master) <
                SPELL_LEVEL(skill_num, GET_REMORT_CLASS(master))))) {
        perform_tell(master, ch, "I am not able to teach you that skill.");
        return 1;
    }

    if (GET_CLASS(master) < NUM_CLASSES &&
        (SPELL_GEN(skill_num, GET_CLASS(master)) && !IS_REMORT(master))) {
        perform_tell(master, ch,
            "You must go elsewhere to learn that remort skill.");
        return 1;
    }

    if ((skill_num == SKILL_READ_SCROLLS || skill_num == SKILL_USE_WANDS) &&
        CHECK_SKILL(ch, skill_num) > 10) {
        perform_tell(master, ch,
            tmp_sprintf("You cannot practice %s any further.",
                spell_to_str(skill_num)));
        return 1;
    }

    if (GET_SKILL(ch, skill_num) >= LEARNED(ch)) {
        perform_tell(master, ch, "You are already learned in that area.");
        return 1;
    }

    if ((SPELL_IS_GOOD(skill_num) && !IS_GOOD(ch)) ||
        (SPELL_IS_EVIL(skill_num) && !IS_EVIL(ch))) {
        perform_tell(master, ch,
            "You have no business dealing with such magic.");
        return 1;
    }

    cost = GET_SKILL_COST(ch, skill_num);
    cost += (cost * cost_modifier(ch, master)) / 100;

    if (ch->in_room->zone->time_frame == TIME_ELECTRO) {
        if (CMD_IS("offer")) {
            perform_tell(master, ch,
                tmp_sprintf("It will cost you %ld creds to train %s.", cost,
                    spell_to_str(skill_num)));
            return 1;
        }

        if (GET_CASH(ch) < cost) {
            perform_tell(master, ch,
                tmp_sprintf
                ("You haven't got the %ld creds I require to train %s.", cost,
                    spell_to_str(skill_num)));
            return 1;
        }

        send_to_char(ch,
            "You buy training for %ld creds and practice for a while...\r\n",
            cost);
        GET_CASH(ch) -= cost;
    } else {
        if (CMD_IS("offer")) {
            perform_tell(master, ch,
                tmp_sprintf("It will cost you %ld gold to train %s.", cost,
                    spell_to_str(skill_num)));
            return 1;
        }

        if (GET_GOLD(ch) < cost) {
            perform_tell(master, ch,
                tmp_sprintf
                ("You haven't got the %ld gold I require to train %s.", cost,
                    spell_to_str(skill_num)));
            return 1;
        }

        send_to_char(ch,
            "You buy training for %ld gold and practice for a while...\r\n",
            cost);
        GET_GOLD(ch) -= cost;
    }

    act("$n practices for a while.", true, ch, 0, 0, TO_ROOM);
    WAIT_STATE(ch, 2 RL_SEC);

    percent = GET_SKILL(ch, skill_num);
    percent += skill_gain(ch, true);
    if (percent > LEARNED(ch))
        percent -= (percent - LEARNED(ch)) >> 1;

    SET_SKILL(ch, skill_num, percent);

    if (GET_SKILL(ch, skill_num) >= LEARNED(ch))
        send_to_char(ch, "You are now well learned in that area.\r\n");

    return 1;
}

SPECIAL(dump)
{
    struct obj_data *k, *next_obj;
    int value = 0;

    ACCMD(do_drop);
    if (spec_mode != SPECIAL_TICK && spec_mode != SPECIAL_CMD)
        return 0;

    for (k = ch->in_room->contents; k; k = next_obj) {
        next_obj = k->next_content;
        act("$p vanishes in a puff of smoke!", false, 0, k, 0, TO_ROOM);
        extract_obj(k);
    }

    if (!CMD_IS("drop"))
        return 0;

    do_drop(ch, argument, cmd, 0, 0);

    for (k = ch->in_room->contents; k; k = next_obj) {
        next_obj = k->next_content;
        act("$p vanishes in a puff of smoke!", false, 0, k, 0, TO_ROOM);
        value += MAX(1, MIN(50, GET_OBJ_COST(k) / 10));
        extract_obj(k);
    }

    if (value) {
        act("You are awarded for outstanding performance.", false, ch, 0, 0,
            TO_CHAR);
        act("$n has been awarded for being a good citizen.", true, ch, 0, 0,
            TO_ROOM);

        if (GET_LEVEL(ch) < 3)
            gain_exp(ch, value);
        else
            GET_GOLD(ch) += value;
    }
    return 1;
}

/* ********************************************************************
*  General special procedures for mobiles                             *
******************************************************************** */

void
npc_steal(struct creature *ch, struct creature *victim)
{
    struct obj_data *obj = NULL;

    if (GET_POSITION(ch) < POS_STANDING)
        return;

    if (GET_LEVEL(victim) >= LVL_AMBASSADOR)
        return;

    if ((GET_LEVEL(ch) + 20 - GET_LEVEL(victim) + (AWAKE(victim) ? 0 : 20)) >
        number(10, 70)) {

        for (obj = victim->carrying; obj; obj = obj->next_content)
            if (can_see_object(ch, obj) && !IS_OBJ_STAT(obj, ITEM_NODROP) &&
                GET_OBJ_COST(obj) > number(10, GET_LEVEL(ch) * 10) &&
                GET_OBJ_WEIGHT(obj) < GET_LEVEL(ch) * 5)
                break;

        if (obj) {
            sprintf(buf, "%s %s", fname(obj->aliases), victim->player.name);
            do_steal(ch, buf, 0, 0, 0);
            return;
        }

    }

    sprintf(buf, "gold %s", victim->player.name);
    do_steal(ch, buf, 0, 0, 0);

}

SPECIAL(venom_attack)
{
    const char *act_toroom = "$n bites $N!";
    const char *act_tovict = "$n bites you!";
    char *str;
    const char *err = NULL;
    int lineno, perc_damaged;
    char *line, *param_key;

    if (spec_mode != SPECIAL_TICK)
        return 0;

    if (GET_POSITION(ch) != POS_FIGHTING)
        return false;

    if (GET_NPC_PARAM(ch)) {
        str = GET_NPC_PARAM(ch);
        for (line = tmp_getline(&str), lineno = 1; line;
            line = tmp_getline(&str), lineno++) {
            param_key = tmp_getword(&line);
            if (!strcmp(param_key, "toroom"))
                act_toroom = line;
            else if (!strcmp(param_key, "tovict"))
                act_tovict = line;
            else
                err = "first word in param must be toroom or tovict";
        }
        if (err) {
            mudlog(LVL_IMMORT, NRM, true,
                "ERR: Mobile %d has %s in line %d of specparam",
                GET_NPC_VNUM(ch), err, lineno);

            return 1;
        }
    }
    // As with real creatures, become more likely to use poison
    // as threat to survival increases
    if (GET_MAX_HIT(ch) && GET_HIT(ch) > 0)
        perc_damaged = 100 - GET_HIT(ch) * 100 / GET_MAX_HIT(ch);
    else
        return false;

    struct creature *target = random_opponent(ch);
    if (target && (target->in_room == ch->in_room) &&
        number(0, 75) < perc_damaged) {
        act(act_tovict, 1, ch, 0, target, TO_VICT);
        act(act_toroom, 1, ch, 0, target, TO_NOTVICT);
        call_magic(ch, target, 0, NULL, SPELL_POISON, GET_LEVEL(ch),
            CAST_SPELL, NULL);
        return true;
    }
    return false;
}

SPECIAL(thief)
{
    if (spec_mode != SPECIAL_TICK && spec_mode != SPECIAL_ENTER)
        return 0;
    if (cmd)
        return false;

    if (GET_POSITION(ch) != POS_STANDING)
        return false;

    for (GList * cit = ch->in_room->people; cit; cit = cit->next) {
        struct creature *tch = cit->data;
        if ((GET_LEVEL(tch) < LVL_AMBASSADOR) && !number(0, 3) &&
            (GET_LEVEL(ch) + 10 + AWAKE(tch) ? 0 : 20 - GET_LEVEL(ch)) >
            number(0, 40)) {
            npc_steal(ch, tch);
            return true;
        }
    }
    return false;
}

SPECIAL(magic_user)
{
    struct creature *vict = NULL;

    if (spec_mode != SPECIAL_TICK)
        return 0;
    if (cmd || GET_POSITION(ch) != POS_FIGHTING)
        return false;

    /* pseudo-randomly choose someone in the room who is fighting me */
    vict = random_opponent(ch);

    if (vict == NULL)
        return 0;

    bool found = true;

    if ((GET_LEVEL(ch) > 5) && (number(0, 8) == 0) &&
        !affected_by_spell(ch, SPELL_ARMOR)) {
        cast_spell(ch, ch, NULL, NULL, SPELL_ARMOR, NULL);
    } else if ((GET_LEVEL(ch) > 14) && (number(0, 8) == 0)
        && !AFF_FLAGGED(ch, AFF_BLUR)) {
        cast_spell(ch, ch, NULL, NULL, SPELL_BLUR, NULL);
    } else if ((GET_LEVEL(ch) > 18) && (number(0, 8) == 0) &&
        !AFF2_FLAGGED(ch, AFF2_FIRE_SHIELD)) {
        cast_spell(ch, ch, NULL, NULL, SPELL_FIRE_SHIELD, NULL);
    } else if ((GET_LEVEL(ch) > 12) && (number(0, 12) == 0)) {
        if (IS_EVIL(ch))
            cast_spell(ch, vict, NULL, NULL, SPELL_ENERGY_DRAIN, NULL);
        else if (IS_GOOD(ch) && IS_EVIL(vict))
            cast_spell(ch, vict, NULL, NULL, SPELL_DISPEL_EVIL, NULL);
    } else if (number(0, 4)) {
        // do nothing
    } else {
        found = false;
    }
    if (found)
        return true;

    switch (GET_LEVEL(ch)) {
    case 4:
    case 5:
        cast_spell(ch, vict, NULL, NULL, SPELL_MAGIC_MISSILE, NULL);
        break;
    case 6:
    case 7:
        cast_spell(ch, vict, NULL, NULL, SPELL_CHILL_TOUCH, NULL);
        break;
    case 8:
    case 9:
    case 10:
    case 11:
        cast_spell(ch, vict, NULL, NULL, SPELL_SHOCKING_GRASP, NULL);
        break;
    case 12:
    case 13:
    case 14:
        cast_spell(ch, vict, NULL, NULL, SPELL_BURNING_HANDS, NULL);
        break;
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
        cast_spell(ch, vict, NULL, NULL, SPELL_LIGHTNING_BOLT, NULL);
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
    case 32:
        cast_spell(ch, vict, NULL, NULL, SPELL_COLOR_SPRAY, NULL);
        break;
    case 33:
    case 34:
    case 35:
    case 36:
    case 37:
    case 38:
    case 39:
    case 40:
    case 41:
    case 42:
        cast_spell(ch, vict, NULL, NULL, SPELL_FIREBALL, NULL);
        break;
    default:
        cast_spell(ch, vict, NULL, NULL, SPELL_PRISMATIC_SPRAY, NULL);
        break;
    }
    return true;

}

SPECIAL(battle_cleric)
{

    if (spec_mode != SPECIAL_TICK)
        return 0;
    if (cmd || GET_POSITION(ch) != POS_FIGHTING)
        return false;

    struct creature *vict = NULL;

    /* pseudo-randomly choose someone in the room who is fighting me */
    vict = random_opponent(ch);

    if (vict == NULL)
        return 0;

    bool found = true;
    if ((GET_LEVEL(ch) > 2) && (number(0, 8) == 0) &&
        !affected_by_spell(ch, SPELL_ARMOR)) {
        cast_spell(ch, ch, NULL, NULL, SPELL_ARMOR, NULL);

    } else if ((GET_HIT(ch) / GET_MAX_HIT(ch)) < (GET_MAX_HIT(ch) >> 1)) {
        if ((GET_LEVEL(ch) < 12) && (number(0, 4) == 0))
            cast_spell(ch, ch, NULL, NULL, SPELL_CURE_LIGHT, NULL);

        else if ((GET_LEVEL(ch) < 24) && (number(0, 4) == 0))
            cast_spell(ch, ch, NULL, NULL, SPELL_CURE_CRITIC, NULL);

        else if ((GET_LEVEL(ch) < 34) && (number(0, 4) == 0))
            cast_spell(ch, ch, NULL, NULL, SPELL_HEAL, NULL);

        else if ((GET_LEVEL(ch) > 34) && (number(0, 4) == 0))
            cast_spell(ch, ch, NULL, NULL, SPELL_GREATER_HEAL, NULL);
    } else if ((GET_LEVEL(ch) > 12) && (number(0, 12) == 0)) {
        if (IS_EVIL(ch))
            cast_spell(ch, vict, NULL, NULL, SPELL_DISPEL_GOOD, NULL);
        else if (IS_GOOD(ch) && IS_EVIL(vict))
            cast_spell(ch, vict, NULL, NULL, SPELL_DISPEL_EVIL, NULL);
    } else if (number(0, 4)) {
        // do nothing this round
    } else {
        found = false;
    }

    if (found) {
        return true;
    }

    switch (GET_LEVEL(ch)) {
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
        cast_spell(ch, vict, NULL, NULL, SPELL_SPIRIT_HAMMER, NULL);
        break;
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
        cast_spell(ch, vict, NULL, NULL, SPELL_CALL_LIGHTNING, NULL);
        break;
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
        cast_spell(ch, vict, NULL, NULL, SPELL_HARM, NULL);
        break;
    default:
        cast_spell(ch, vict, NULL, NULL, SPELL_FLAME_STRIKE, NULL);
        break;
    }
    return true;

}

SPECIAL(barbarian)
{

    if (spec_mode != SPECIAL_TICK)
        return 0;
    if (cmd || GET_POSITION(ch) != POS_FIGHTING)
        return false;

    struct creature *vict = NULL;

    /* pseudo-randomly choose someone in the room who is fighting me */
    vict = random_opponent(ch);

    /* if I didn't pick any of those, then just slam the guy I'm fighting */
    if (vict == NULL)
        return 0;

    bool found = true;
    if ((GET_LEVEL(ch) > 2) && (number(0, 12) == 0)) {
        damage(ch, vict, number(0, GET_LEVEL(ch)), SKILL_PUNCH, -1);
    } else if ((GET_LEVEL(ch) > 5) && (number(0, 8) == 0)) {
        damage(ch, vict, number(0, GET_LEVEL(ch)), SKILL_STOMP, WEAR_FEET);
    } else if ((GET_LEVEL(ch) > 16) && (number(0, 18) == 0)) {
        damage(ch, vict, number(0, GET_LEVEL(ch)), SKILL_CHOKE, WEAR_NECK_1);
    } else if ((GET_LEVEL(ch) > 9) && (number(0, 8) == 0)) {
        damage(ch, vict, number(0, GET_LEVEL(ch)), SKILL_ELBOW, -1);
    } else if ((GET_LEVEL(ch) > 20) && (number(0, 8) == 0)) {
        damage(ch, vict, number(0, GET_LEVEL(ch)), SKILL_CLOTHESLINE,
            WEAR_NECK_1);
    } else if ((GET_LEVEL(ch) > 27) && (number(0, 14) == 0)) {
        damage(ch, vict, number(0, GET_LEVEL(ch)), SKILL_SLEEPER, -1);
    } else if ((GET_LEVEL(ch) > 22) && (number(0, 6) == 0)) {
        damage(ch, vict, number(0, GET_LEVEL(ch)), SKILL_BODYSLAM, WEAR_BODY);
    } else if ((GET_LEVEL(ch) > 32) && (number(0, 8) == 0)) {
        damage(ch, vict, number(0, GET_LEVEL(ch)), SKILL_PILEDRIVE, WEAR_ASS);
    } else if (number(0, 4)) {
        // do nothing this round
    } else {
        found = false;
    }

    if (found) {
        return true;
    }

    switch (GET_LEVEL(ch)) {
    case 1:
    case 2:
    case 3:
    case 4:
        damage(ch, vict, number(0, GET_LEVEL(ch)), SKILL_PUNCH, -1);
        break;
    case 5:
    case 6:
        damage(ch, vict, number(0, GET_LEVEL(ch)), SKILL_STOMP, WEAR_FEET);
        break;
    case 7:
    case 8:
        damage(ch, vict, number(0, GET_LEVEL(ch)), SKILL_KNEE, -1);
        break;
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
        damage(ch, vict, number(0, GET_LEVEL(ch)), SKILL_ELBOW, -1);
        break;
    case 16:
    case 17:
    case 18:
        damage(ch, vict, number(0, GET_LEVEL(ch)), SKILL_CHOKE, WEAR_NECK_1);
        break;
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
        damage(ch, vict, number(0, GET_LEVEL(ch)), SKILL_CLOTHESLINE,
            WEAR_NECK_1);
        break;
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
        damage(ch, vict, number(0, GET_LEVEL(ch)), SKILL_BODYSLAM, -1);
        break;
    default:
        damage(ch, vict, number(0, GET_LEVEL(ch)), SKILL_PILEDRIVE, -1);
        break;
    }
    return true;
}

/* *******************************************************************/

/* ********************************************************************
*  Special procedures for mobiles                                      *
******************************************************************** */

int parse_player_class(char *arg, int timeframe);

SPECIAL(fido)
{
    struct creature *vict;

    if (spec_mode != SPECIAL_TICK)
        return 0;
    if (cmd || !AWAKE(ch) || ch->fighting)
        return (false);

    vict = get_char_random_vis(ch, ch->in_room);
    if (!vict || vict == ch)
        return 0;

    switch (number(0, 70)) {
    case 0:
        act("$n scratches at some fleas.", true, ch, 0, 0, TO_ROOM);
        break;
    case 1:
        act("$n starts howling mournfully.", false, ch, 0, 0, TO_ROOM);
        break;
    case 2:
        act("$n licks $s chops.", true, ch, 0, 0, TO_ROOM);
        break;
    case 3:
        act("$n pukes all over the place.", false, ch, 0, 0, TO_ROOM);
        break;
    case 4:
        if (!IS_NPC(vict) || GET_NPC_SPEC(vict) != fido) {
            act("$n takes a leak on $N's shoes.", false, ch, 0, vict,
                TO_NOTVICT);
            act("$n takes a leak on your shoes.", false, ch, 0, vict, TO_VICT);
        }
        break;
    case 5:
        if (IS_MALE(ch) && !number(0, 1))
            act("$n licks $s balls.", true, ch, 0, 0, TO_ROOM);
        else
            act("$n licks $s ass.", true, ch, 0, 0, TO_ROOM);
        break;
    case 6:
        act("$n sniffs your crotch.", true, ch, 0, vict, TO_VICT);
        act("$n sniffs $N's crotch.", true, ch, 0, vict, TO_NOTVICT);
        break;
    case 7:
        if (!IS_NPC(vict) || GET_NPC_SPEC(vict) != fido) {
            act("$n slobbers on your hand.", true, ch, 0, vict, TO_VICT);
            act("$n slobbers on $N's hand.", true, ch, 0, vict, TO_NOTVICT);
        }
        break;
    case 8:
        act("$n starts biting on $s leg.", true, ch, 0, 0, TO_ROOM);
        break;
    default:
        return false;
    }
    return true;
}

SPECIAL(buzzard)
{
    struct obj_data *i, *temp, *next_obj;
    struct creature *vict = NULL;

    if (spec_mode != SPECIAL_TICK)
        return 0;
    if (cmd || !AWAKE(ch) || random_opponent(ch))
        return (false);

    for (i = ch->in_room->contents; i; i = i->next_content) {
        if (GET_OBJ_TYPE(i) == ITEM_CONTAINER && GET_OBJ_VAL(i, 3)) {
            act("$n descends on $p and devours it.", false, ch, i, 0, TO_ROOM);
            for (temp = i->contains; temp; temp = next_obj) {
                next_obj = temp->next_content;
                if (IS_IMPLANT(temp)) {
                    SET_BIT(GET_OBJ_WEAR(temp), ITEM_WEAR_TAKE);
                    if (GET_OBJ_DAM(temp) > 0)
                        GET_OBJ_DAM(temp) >>= 1;
                }
                obj_from_obj(temp);
                obj_to_room(temp, ch->in_room);
            }
            extract_obj(i);
            return true;
        }
    }
    vict = random_opponent(ch);

    if (!vict || vict == ch)
        return 0;

    switch (number(0, 70)) {
    case 0:
        act("$n emits a terrible croaking noise.", false, ch, 0, 0, TO_ROOM);
        break;
    case 1:
        act("You begin to notice a foul stench coming from $n.", false, ch, 0,
            0, TO_ROOM);
        break;
    case 2:
        act("$n regurgitates some half-digested carrion.", false, ch, 0, 0,
            TO_ROOM);
        break;
    case 3:
        act("$n glares at you hungrily.", true, ch, 0, vict, TO_VICT);
        act("$n glares at $N hungrily.", true, ch, 0, vict, TO_NOTVICT);
        break;
    case 4:
        act("You notice that $n really is quite putrid.", false, ch, 0, 0,
            TO_ROOM);
        break;
    case 5:
        act("A hideous stench wafts across your nostrils.", false, ch, 0, 0,
            TO_ROOM);
        break;
    default:
        return false;
    }
    return true;
}

SPECIAL(garbage_pile)
{

    struct obj_data *i, *temp, *next_obj;

    if (spec_mode != SPECIAL_TICK)
        return 0;
    if (cmd || !AWAKE(ch))
        return (false);

    for (i = ch->in_room->contents; i; i = i->next_content) {
        if (GET_OBJ_VNUM(i) == QUAD_VNUM)
            continue;

        // don't get sigilized items
        if (GET_OBJ_SIGIL_IDNUM(i))
            continue;

        if (GET_OBJ_TYPE(i) == ITEM_CONTAINER && GET_OBJ_VAL(i, 3)) {
            act("$n devours $p.", false, ch, i, 0, TO_ROOM);
            for (temp = i->contains; temp; temp = next_obj) {
                next_obj = temp->next_content;
                if (IS_IMPLANT(temp)) {
                    SET_BIT(GET_OBJ_WEAR(temp), ITEM_WEAR_TAKE);
                    if (GET_OBJ_DAM(temp) > 0)
                        GET_OBJ_DAM(temp) >>= 1;
                }
                obj_from_obj(temp);
                obj_to_room(temp, ch->in_room);
            }
            extract_obj(i);
            return true;
        } else if (CAN_WEAR(i, ITEM_WEAR_TAKE) && GET_OBJ_WEIGHT(i) < 5) {
            act("$n assimilates $p.", false, ch, i, 0, TO_ROOM);
            if (GET_OBJ_VNUM(i) == 3365)
                extract_obj(i);
            else {
                obj_from_room(i);
                obj_to_char(i, ch);
                get_check_money(ch, &i, 0);
            }
            return 1;
        }
    }
    switch (number(0, 80)) {
    case 0:
        act("Some trash falls off of $n.", false, ch, 0, 0, TO_ROOM);
        i = read_object(3365);
        if (i)
            obj_to_room(i, ch->in_room);
        break;
    default:
        return false;
    }
    return true;
}

SPECIAL(janitor)
{
    struct obj_data *i;
    int ahole = 0;

    if (spec_mode != SPECIAL_TICK)
        return 0;

    if (cmd || !AWAKE(ch))
        return (false);

    for (i = ch->in_room->contents; i; i = i->next_content) {
        if (GET_OBJ_VNUM(i) == QUAD_VNUM ||
            !CAN_WEAR(i, ITEM_WEAR_TAKE) || !can_see_object(ch, i) ||
            (GET_OBJ_WEIGHT(i) + IS_CARRYING_W(ch)) > CAN_CARRY_W(ch) ||
            (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 150))
            continue;

        // don't get sigilized items
        if (GET_OBJ_SIGIL_IDNUM(i))
            continue;

        if (!number(0, 5)) {
            ahole = 1;
            perform_say(ch, "bellow", "You assholes must like LAG.");
        } else if (!number(0, 5))
            perform_say(ch, "say", "Why don't you guys junk this crap?");

        do_get(ch, fname(i->aliases), 0, 0, 0);

        if (ahole && IS_MALE(ch)) {
            for (GList * cit = ch->in_room->people; cit; cit = cit->next) {
                struct creature *tch = cit->data;
                if (tch != ch && IS_FEMALE(tch) && can_see_creature(ch, tch)) {
                    perform_say_to(ch, tch, "Excuse me, ma'am.");
                }
            }
        }

        return true;
    }

    return false;
}

SPECIAL(elven_janitor)
{
    struct obj_data *i;

    if (spec_mode != SPECIAL_TICK)
        return 0;

    if (cmd || !AWAKE(ch))
        return (false);

    for (i = ch->in_room->contents; i; i = i->next_content) {
        if (GET_OBJ_VNUM(i) == QUAD_VNUM ||
            !CAN_WEAR(i, ITEM_WEAR_TAKE) || !can_see_object(ch, i) ||
            (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 50) ||
            IS_OBJ_STAT(i, ITEM_NODROP))
            continue;

        // don't get sigilized items
        if (GET_OBJ_SIGIL_IDNUM(i))
            continue;

        act("$n grumbles as $e picks up $p.", false, ch, i, 0, TO_ROOM);
        do_get(ch, fname(i->aliases), 0, 0, 0);
        return true;
    }

    return false;
}

SPECIAL(gelatinous_blob)
{
    struct obj_data *i;

    if (spec_mode != SPECIAL_TICK)
        return 0;

    if (cmd || !AWAKE(ch))
        return (false);

    for (i = ch->in_room->contents; i; i = i->next_content) {
        if (GET_OBJ_VNUM(i) == QUAD_VNUM ||
            !CAN_WEAR(i, ITEM_WEAR_TAKE) || !can_see_object(ch, i) ||
            (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 50))
            continue;

        // don't get sigilized items
        if (GET_OBJ_SIGIL_IDNUM(i))
            continue;

        if (GET_NPC_VNUM(ch) == 30068)
            act("$n sucks $p into a holding tank with a WHOOSH!", false, ch, i,
                0, TO_ROOM);
        else
            act("$n absorbs $p into $s body.", false, ch, i, 0, TO_ROOM);
        obj_from_room(i);
        obj_to_char(i, ch);
        get_check_money(ch, &i, 0);
        return true;
    }

    return false;
}

SPECIAL(pet_shops)
{
    struct creature *pet;
    struct room_data *pet_room;
    char *pet_name, *pet_kind;
    int cost;

    if (SPECIAL_CMD != spec_mode)
        return false;

    pet_room = real_room(ch->in_room->number + 1);

    if (CMD_IS("list")) {
        send_to_char(ch, "Available pets are:\r\n");
        for (GList * cit = pet_room->people; cit; cit = cit->next) {
            struct creature *tch = cit->data;
            cost = (IS_NPC(tch) ? GET_EXP(tch) * 3 : (GET_EXP(ch) >> 2));
            send_to_char(ch, "%8d - %s\r\n", cost, GET_NAME(tch));
        }
        return 1;
    }

    if (CMD_IS("buy")) {
        pet_kind = tmp_getword(&argument);
        pet_name = tmp_getword(&argument);

        if (!(pet = get_char_room(pet_kind, pet_room))) {
            send_to_char(ch, "There is no such pet!\r\n");
            return true;
        }

        if (pet == ch) {
            send_to_char(ch, "You buy yourself. Yay. Are you happy now?\r\n");
            return true;
        }

        if (IS_NPC(ch))
            cost = GET_EXP(pet) * 3;
        else
            cost = GET_EXP(pet) >> 4;

        //we have no shop keeper so compare charisma with the pet
        cost += (cost * cost_modifier(ch, pet)) / 100;

        if (GET_GOLD(ch) < cost) {
            send_to_char(ch, "You don't have enough gold!\r\n");
            return true;
        }

        if (ch->followers) {
            struct follow_type *f;

            for (f = ch->followers; f; f = f->next) {
                if (AFF_FLAGGED(f->follower, AFF_CHARM)) {
                    send_to_char(ch, "You seem to already have a pet.\r\n");
                    return 1;
                }
            }
        }

        GET_GOLD(ch) -= cost;

        if (IS_NPC(pet)) {
            pet = read_mobile(GET_NPC_VNUM(pet));
            GET_EXP(pet) = 0;

            if (*pet_name) {
                char *tmp;

                tmp = tmp_strcat(pet->player.name, " ", pet_name, NULL);
                pet->player.name = strdup(tmp);

                tmp =
                    tmp_sprintf
                    ("A small sign on a chain around the neck says 'My name is %s\r\n'",
                    pet_name);
                if (pet->player.description != NULL)
                    tmp = tmp_strcat(pet->player.description, tmp, NULL);
                pet->player.description = strdup(tmp);
            }
            char_to_room(pet, ch->in_room, false);

            if (IS_ANIMAL(pet)) {
                /* Be certain that pets can't get/carry/use/wield/wear items */
                IS_CARRYING_W(pet) = 1000;
                IS_CARRYING_N(pet) = 100;
            }
        } else {                /* player characters */
            char_from_room(pet, false);
            char_to_room(pet, ch->in_room, false);
        }

        SET_BIT(AFF_FLAGS(pet), AFF_CHARM);
        add_follower(pet, ch);
        if (IS_NPC(pet)) {
            send_to_char(ch, "May you enjoy your pet.\r\n");
            act("$n buys $N as a pet.", false, ch, 0, pet, TO_ROOM);
            SET_BIT(NPC_FLAGS(pet), NPC_PET);
        } else {
            send_to_char(ch, "May you enjoy your slave.\r\n");
            act("$n buys $N as a slave.", false, ch, 0, pet, TO_ROOM);
        }
        return true;
    }

    return false;
}

/* ********************************************************************
*  Special procedures for objects                                     *
******************************************************************** */

#define PLURAL(num) (num == 1 ? "" : "s")

SPECIAL(bank)
{
    struct clan_data *clan = NULL;
    struct clanmember_data *member = NULL;
    const char *vict_name;
    struct account *acct;
    char *arg;
    int amount = 0;

    if (spec_mode != SPECIAL_CMD)
        return 0;

    if (!CMD_IS("balance") && !CMD_IS("withdraw") && !CMD_IS("deposit")
        && !CMD_IS("transfer")) {
        return 0;
    }

    arg = tmp_getword(&argument);
    if (!strcasecmp(arg, "clan")) {
        clan = real_clan(GET_CLAN(ch));
        member = (clan) ? real_clanmember(GET_IDNUM(ch), clan) : NULL;

        if (!member || (CMD_IS("withdraw")
                && !PLR_FLAGGED(ch, PLR_CLAN_LEADER))) {
            send_to_char(ch, "You can't do that.\r\n");
            return 1;
        }

        arg = tmp_getword(&argument);
    }

    if ((!CMD_IS("transfer") && GET_LEVEL(ch) >= LVL_AMBASSADOR) || IS_NPC(ch)) {
        send_to_char(ch, "Why would you need a bank?\r\n");
        return 1;
    }

    if (!CMD_IS("balance")) {
        // Do checks for deposit, withdraw, and transfer
        if (strcasecmp(arg, "all")) {
            if (!is_number(arg)) {
                send_to_char(ch, "You must specify an amount!\r\n");
                return 1;
            }

            amount = atoi(arg);
            if (amount < 0) {
                send_to_char(ch, "Ha ha.  Very funny.\r\n");
                return 1;
            } else if (amount == 0) {
                send_to_char(ch, "You should specify more than zero!\r\n");
                return 1;
            }
        }
    }

    if (CMD_IS("deposit")) {
        if (!strcasecmp(arg, "all")) {
            if (!CASH_MONEY(ch)) {
                send_to_char(ch, "You don't have any %ss to deposit!\r\n",
                    CURRENCY(ch));
                return 1;
            }
            amount = CASH_MONEY(ch);
        }

        if (CASH_MONEY(ch) < amount) {
            send_to_char(ch, "You don't have that many %ss!\r\n",
                CURRENCY(ch));
            return 1;
        }

        if (ch->in_room->zone->time_frame == TIME_ELECTRO)
            GET_CASH(ch) -= amount;
        else
            GET_GOLD(ch) -= amount;
        if (clan) {
            clan->bank_account += amount;
            send_to_char(ch, "You deposit %d %s%s in the clan account.\r\n",
                amount, CURRENCY(ch), PLURAL(amount));
            sql_exec("update clans set bank=%lld where idnum=%d",
                clan->bank_account, clan->number);
            slog("CLAN: %s clandep (%s) %d.", GET_NAME(ch),
                clan->name, amount);
        } else {
            if (ch->in_room->zone->time_frame == TIME_ELECTRO)
                deposit_future_bank(ch->account, amount);
            else
                deposit_past_bank(ch->account, amount);
            send_to_char(ch, "You deposit %d %s%s.\r\n", amount, CURRENCY(ch),
                PLURAL(amount));
            if (amount > 50000000) {
                mudlog(LVL_IMMORT, NRM, true,
                    "%s deposited %d into the bank", GET_NAME(ch), amount);
            }
        }

        act("$n makes a bank transaction.", true, ch, 0, false, TO_ROOM);

    } else if (CMD_IS("withdraw")) {
        if (!strcasecmp(arg, "all")) {
            amount = (clan) ? clan->bank_account : BANK_MONEY(ch);
            if (!amount) {
                send_to_char(ch,
                    "There's nothing there for you to withdraw!\r\n");
                return 1;
            }
        }

        if (AFF_FLAGGED(ch, AFF_CHARM)) {
            send_to_char(ch, "You can't do that while charmed!\r\n");
            send_to_char(ch->master,
                "You can't force %s to do that, even while charmed!\r\n",
                ch->player.name);
            return 1;
        }

        if (clan) {
            if (clan->bank_account < amount) {
                send_to_char(ch,
                    "The clan doesn't have that much deposited.\r\n");
                return 1;
            }
            clan->bank_account -= amount;
            sql_exec("update clans set bank=%lld where idnum=%d",
                clan->bank_account, clan->number);
        } else {
            if (BANK_MONEY(ch) < amount) {
                send_to_char(ch, "You don't have that many %ss deposited!\r\n",
                    CURRENCY(ch));
                return 1;
            }
            if (ch->in_room->zone->time_frame == TIME_ELECTRO)
                withdraw_future_bank(ch->account, amount);
            else
                withdraw_past_bank(ch->account, amount);
        }

        if (ch->in_room->zone->time_frame == TIME_ELECTRO)
            GET_CASH(ch) += amount;
        else
            GET_GOLD(ch) += amount;
        send_to_char(ch, "You withdraw %d %s%s.\r\n", amount, CURRENCY(ch),
            PLURAL(amount));
        act("$n makes a bank transaction.", true, ch, 0, false, TO_ROOM);

        if (amount > 50000000) {
            mudlog(LVL_IMMORT, NRM, true,
                "%s withdrew %d from %s",
                GET_NAME(ch), amount, (clan) ? "clan account" : "bank");
        }

    } else if (CMD_IS("transfer")) {
        if (!strcasecmp(arg, "all")) {
            amount = (clan) ? clan->bank_account : BANK_MONEY(ch);
            if (!amount) {
                send_to_char(ch,
                    "There's nothing there for you to transfer!\r\n");
                return 1;
            }
        }

        arg = tmp_getword(&argument);
        if (!*arg) {
            send_to_char(ch, "Who do you want to transfer money to?\r\n");
            return 1;
        }

        if (!player_name_exists(arg)) {
            send_to_char(ch,
                "You can't transfer money to someone who doesn't exist!\r\n");
            return 1;
        }

        vict_name = player_name_by_idnum(player_idnum_by_name(arg));
        acct = account_by_idnum(player_account_by_name(vict_name));

        if (!clan && acct == ch->account) {
            send_to_char(ch,
                "Transferring money to your own account?  Odd...\r\n");
            return 1;
        }

        if (AFF_FLAGGED(ch, AFF_CHARM)) {
            send_to_char(ch, "You can't do that while charmed!\r\n");
            if (ch->master)
                send_to_char(ch->master,
                    "You can't force %s to do that, even while charmed!\r\n",
                    GET_NAME(ch));
            return 1;
        }

        if (clan) {
            if (clan->bank_account < amount) {
                send_to_char(ch,
                    "The clan doesn't have that much deposited.\r\n");
                return 1;
            }
            clan->bank_account -= amount;
            sql_exec("update clans set bank=%lld where idnum=%d",
                clan->bank_account, clan->number);
        } else {
            if (BANK_MONEY(ch) < amount) {
                send_to_char(ch, "You don't have that many %ss deposited!\r\n",
                    CURRENCY(ch));
                return 1;
            }
            if (ch->in_room->zone->time_frame == TIME_ELECTRO)
                withdraw_future_bank(ch->account, amount);
            else
                withdraw_past_bank(ch->account, amount);
        }

        if (ch->in_room->zone->time_frame == TIME_ELECTRO)
            deposit_future_bank(acct, amount);
        else
            deposit_past_bank(acct, amount);

        send_to_char(ch, "You transfer %d %s%s to %s's account.\r\n",
            amount, CURRENCY(ch), PLURAL(amount), vict_name);
        act("$n makes a bank transaction.", true, ch, 0, false, TO_ROOM);

        if (amount > 50000000) {
            mudlog(LVL_IMMORT, NRM, true,
                "%s transferred %d from %s to %s's account",
                GET_NAME(ch), amount, (clan) ? "clan account" : "bank",
                vict_name);
        }
    }

    save_player_to_xml(ch);
    if (clan) {
        if (clan->bank_account > 0)
            send_to_char(ch, "The current clan balance is %lld %s%s.\r\n",
                clan->bank_account, CURRENCY(ch), PLURAL(clan->bank_account));
        else
            send_to_char(ch, "The clan currently has no money deposited.\r\n");
    } else {
        if (BANK_MONEY(ch) > 0)
            send_to_char(ch, "Your current balance is %lld %s%s.\r\n",
                BANK_MONEY(ch), CURRENCY(ch), PLURAL(BANK_MONEY(ch)));
        else
            send_to_char(ch, "You currently have no money deposited.\r\n");
    }
    return 1;
}

#undef PLURAL

SPECIAL(cave_bear)
{

    if (spec_mode != SPECIAL_TICK)
        return 0;
    if (cmd || !ch->fighting)
        return false;

    if (!number(0, 12)) {
        damage(ch, random_opponent(ch),
            number(0, 1 + GET_LEVEL(ch)), SKILL_BEARHUG, WEAR_BODY);
        return true;
    }
    return false;
}

/************************************************************************
  Included special procedures
*************************************************************************/
/* MODRIAN SPECIALS */
#include "Specs/modrian_specs/shimmering_portal.c"
#include "Specs/modrian_specs/wagon_obj.c"
#include "Specs/modrian_specs/wagon_room.c"
#include "Specs/modrian_specs/wagon_driver.c"
#include "Specs/modrian_specs/temple_healer.c"
#include "Specs/modrian_specs/rat_mama.c"
#include "Specs/modrian_specs/ornery_goat.c"
#include "Specs/modrian_specs/entrance_to_cocks.c"
#include "Specs/modrian_specs/entrance_to_brawling.c"
#include "Specs/modrian_specs/cock_fight.c"
#include "Specs/modrian_specs/circus_clown.c"
#include "Specs/modrian_specs/chest_mimic.c"
#include "Specs/modrian_specs/cemetary_ghoul.c"
#include "Specs/modrian_specs/modrian_fountain_obj.c"
#include "Specs/modrian_specs/modrian_fountain_rm.c"
#include "Specs/modrian_specs/mystical_enclave_rm.c"
#include "Specs/modrian_specs/safiir.c"
#include "Specs/modrian_specs/unholy_square.c"

/* NEWBIE SPECS */
#include "Specs/newbie_specs/newbie_improve.c"
#include "Specs/newbie_specs/newbie_tower_rm.c"
#include "Specs/newbie_specs/newbie_cafe_rm.c"
#include "Specs/newbie_specs/newbie_healer.c"
#include "Specs/newbie_specs/newbie_portal_rm.c"
#include "Specs/newbie_specs/newbie_stairs_rm.c"
#include "Specs/newbie_specs/newbie_fly.c"
#include "Specs/newbie_specs/newbie_gold_coupler.c"
#include "Specs/newbie_specs/newbie_fodder.c"
#include "Specs/newbie_specs/guardian_angel.c"

/* GENERAL UTIL SPECS */
#include "Specs/utility_specs/gen_locker.c"
#include "Specs/utility_specs/gen_shower_rm.c"
#include "Specs/utility_specs/improve_stats.c"
#include "Specs/utility_specs/registry.c"
#include "Specs/utility_specs/corpse_retrieval.c"
#include "Specs/utility_specs/dt_cleaner.c"
#include "Specs/utility_specs/killer_hunter.c"
#include "Specs/utility_specs/nohunger_dude.c"
#include "Specs/utility_specs/portal_home.c"
#include "Specs/utility_specs/stable_room.c"
#include "Specs/utility_specs/increaser.c"
#include "Specs/utility_specs/donation_room.c"
#include "Specs/utility_specs/tester_util.c"
#include "Specs/utility_specs/typo_util.c"
#include "Specs/utility_specs/newspaper.c"
#include "Specs/utility_specs/prac_manual.c"
#include "Specs/utility_specs/life_point_potion.c"
#include "Specs/utility_specs/jail_locker.c"
#include "Specs/utility_specs/repairer.c"
#include "Specs/utility_specs/telescope.c"
#include "Specs/utility_specs/fate_cauldron.c"
#include "Specs/utility_specs/fate_portal.c"
#include "Specs/utility_specs/quantum_rift.c"
#include "Specs/utility_specs/roaming_portal.c"
#include "Specs/utility_specs/astral_portal.c"
#include "Specs/utility_specs/reinforcer.c"
#include "Specs/utility_specs/enhancer.c"
#include "Specs/utility_specs/communicator.c"
#include "Specs/utility_specs/shade_zone.c"
#include "Specs/utility_specs/bounty_clerk.c"

/* SPECS BY SARFLIN */
#include "Specs/sarflin_specs/maze_cleaner.c"
#include "Specs/sarflin_specs/maze_switcher.c"
#include "Specs/sarflin_specs/darom.c"
#include "Specs/sarflin_specs/puppet.c"
#include "Specs/sarflin_specs/oracle.c"
#include "Specs/sarflin_specs/fountain_heal.c"
#include "Specs/sarflin_specs/fountain_restore.c"
#include "Specs/sarflin_specs/library.c"
#include "Specs/sarflin_specs/arena.c"
#include "Specs/sarflin_specs/gingwatzim_rm.c"
#include "Specs/sarflin_specs/dwarven_hermit.c"
#include "Specs/sarflin_specs/vault_door.c"
#include "Specs/sarflin_specs/vein.c"
#include "Specs/sarflin_specs/gunnery_device.c"
#include "Specs/sarflin_specs/ramp_leaver.c"

/* OBJECT SPECIALS */
#include "Specs/object_specs/loudspeaker.c"
#include "Specs/object_specs/voting_booth.c"
#include "Specs/object_specs/ancient_artifact.c"
#include "Specs/object_specs/finger_of_death.c"
#include "Specs/object_specs/quest_sphere.c"
#include "Specs/object_specs/wand_of_wonder.c"

/* HILL GIANT STEADING */
#include "Specs/giants_specs/javelin_of_lightning.c"
#include "Specs/giants_specs/carrion_crawler.c"
#include "Specs/giants_specs/ogre1.c"
#include "Specs/giants_specs/kata.c"
#include "Specs/giants_specs/insane_merchant.c"

/* HELL */
#include "Specs/hell_specs/cloak_of_deception.c"
#include "Specs/hell_specs/horn_of_geryon.c"
#include "Specs/hell_specs/unholy_compact.c"
#include "Specs/hell_specs/stygian_lightning_rm.c"
#include "Specs/hell_specs/tiamat.c"
#include "Specs/hell_specs/hell_hound.c"
#include "Specs/hell_specs/geryon.c"
#include "Specs/hell_specs/moloch.c"
#include "Specs/hell_specs/malbolge_bridge.c"
#include "Specs/hell_specs/abandoned_cavern.c"
#include "Specs/hell_specs/dangerous_climb.c"
#include "Specs/hell_specs/bearded_devil.c"
#include "Specs/hell_specs/cyberfiend.c"
#include "Specs/hell_specs/maladomini_jailer.c"
#include "Specs/hell_specs/regulator.c"
#include "Specs/hell_specs/ressurector.c"
#include "Specs/hell_specs/killzone_room.c"
#include "Specs/hell_specs/hell_domed_chamber.c"
#include "Specs/hell_specs/malagard_lightning_room.c"
#include "Specs/hell_specs/pit_keeper.c"
#include "Specs/hell_specs/cremator.c"

/* SMITTY'S SPECS */
#include "Specs/smitty_specs/aziz_canon.c"
#include "Specs/smitty_specs/battlefield_ghost.c"
#include "Specs/smitty_specs/nude_guard.c"
#include "Specs/smitty_specs/djinn_lamp.c"

/* C.J's SPECS */
#include "Specs/cj_specs/beer_tree.c"
#include "Specs/cj_specs/black_rose.c"

/* ELECTRO CENTRALIS SPECS */
#include "Specs/electro_specs/sunstation.c"
#include "Specs/electro_specs/lawyer.c"
#include "Specs/electro_specs/electronics_school.c"
#include "Specs/electro_specs/vr_arcade_game.c"
#include "Specs/electro_specs/cyber_cock.c"
#include "Specs/electro_specs/cyborg_overhaul.c"
#include "Specs/electro_specs/credit_exchange.c"
#include "Specs/electro_specs/implanter.c"
#include "Specs/electro_specs/electrician.c"
#include "Specs/electro_specs/paramedic.c"
#include "Specs/electro_specs/unspecializer.c"
#include "Specs/electro_specs/mugger.c"

/* HEAVENLY SPECS */
#include "Specs/heaven/archon.c"
#include "Specs/heaven/high_priestess.c"

/* Araken Specs */
#include "Specs/araken_specs/rust_monster.c"

/* Mavernal Specs */
#include "Specs/falling_tower_dt.c"
#include "Specs/stepping_stone.c"

/* GENERAL MOBILE SPECS */
#include "Specs/mobile_specs/healing_ranger.c"
#include "Specs/mobile_specs/mob_helper.c"
#include "Specs/mobile_specs/underwater_predator.c"
#include "Specs/mobile_specs/watchdog.c"
#include "Specs/mobile_specs/medusa.c"
#include "Specs/mobile_specs/grandmaster.c"
#include "Specs/mobile_specs/fire_breather.c"
#include "Specs/mobile_specs/energy_drainer.c"
#include "Specs/mobile_specs/basher.c"
#include "Specs/mobile_specs/aziz.c"
#include "Specs/mobile_specs/juju_zombie.c"
#include "Specs/mobile_specs/phantasmic_sword.c"
#include "Specs/mobile_specs/spinal.c"
#include "Specs/mobile_specs/fates.c"
#include "Specs/mobile_specs/astral_deva.c"
#include "Specs/mobile_specs/junker.c"
#include "Specs/mobile_specs/duke_nukem.c"
#include "Specs/mobile_specs/tarrasque.c"
#include "Specs/mobile_specs/weaponsmaster.c"
#include "Specs/mobile_specs/unholy_stalker.c"
#include "Specs/mobile_specs/oedit_reloader.c"
#include "Specs/mobile_specs/town_crier.c"
#include "Specs/mobile_specs/mage_teleporter.c"
#include "Specs/mobile_specs/languagemaster.c"
#include "Specs/mobile_specs/auctioneer.c"
#include "Specs/mobile_specs/courier_imp.c"

/* RADFORD's SPECS */
#include "Specs/rad_specs/labyrinth.c"

/* LABYRINTH SPECS */
#include "Specs/labyrinth/laby_portal.c"
#include "Specs/labyrinth/laby_carousel.c"
#include "Specs/labyrinth/laby_altar.c"

/* Bugs' SPECS */
#include "Specs/bugs_specs/monastery_eating.c"
#include "Specs/bugs_specs/red_highlord.c"

/* skullport SPECS */
#include "Specs/skullport_specs/corpse_griller.c"
#include "Specs/skullport_specs/head_shrinker.c"
#include "Specs/skullport_specs/multi_healer.c"
#include "Specs/skullport_specs/slaver.c"
#include "Specs/skullport_specs/fountain_youth.c"

/* out specs */
#include "Specs/out_specs/spirit_priestess.c"
#include "Specs/out_specs/taunting_frenchman.c"

/* altas specs */
#include "Specs/atlas_specs/align_fountains.c"
/** OTHERS **/

#include "Specs/disaster_specs/boulder_thrower.c"
#include "Specs/disaster_specs/windy_room.c"
#include "Specs/javelin_specs/clone_lab.c"

SPECIAL(weapon_lister)
{
    int dam, i, found = 0;
    char buf3[MAX_STRING_LENGTH];
    unsigned int avg_dam[60];

    if (!CMD_IS("list"))
        return 0;

    for (i = 0; i < 60; i++)
        avg_dam[i] = 0;

    strcpy(buf3, "");
    GHashTableIter iter;
    gpointer key, val;

    g_hash_table_iter_init(&iter, obj_prototypes);
    while (g_hash_table_iter_next(&iter, &key, &val)) {
        struct obj_data *obj = val;

        if (GET_OBJ_TYPE(obj) != ITEM_WEAPON)
            continue;

        if (!OBJ_APPROVED(obj))
            continue;

        for (i = 0, dam = 0; i < MAX_OBJ_AFFECT; i++)
            if (obj->affected[i].location == APPLY_DAMROLL)
                dam += obj->affected[i].modifier;

        sprintf(buf, "[%5d] %-30s %2dd%-2d", GET_OBJ_VNUM(obj),
            obj->name, GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));

        if (dam > 0)
            sprintf(buf, "%s+%-2d", buf, dam);
        else if (dam > 0)
            sprintf(buf, "%s-%-2d", buf, -dam);
        else
            strcat(buf, "   ");

        sprintf(buf, "%s (%2d) %3d lb ", buf,
            (GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2) + 1) / 2) + dam,
            GET_OBJ_WEIGHT(obj));

        if (((GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2) + 1) / 2) + dam > 0 &&
                GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj,
                        2) + 1) / 2) + dam < 60)
            avg_dam[(int)(GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj,
                            2) + 1) / 2) + dam]++;

        if (IS_TWO_HAND(obj))
            strcat(buf, "2-H ");

        if (GET_OBJ_VAL(obj, 0))
            sprintf(buf, "%sCast:%s ", buf, spell_to_str(GET_OBJ_VAL(obj, 0)));

        for (i = 0, found = 0; i < 3; i++)
            if (obj->obj_flags.bitvector[i]) {
                if (!found)
                    strcat(buf, "Set: ");
                found = 1;
                if (i == 0)
                    sprintbit(obj->obj_flags.bitvector[i], affected_bits,
                        buf2);
                else if (i == 1)
                    sprintbit(obj->obj_flags.bitvector[i], affected2_bits,
                        buf2);
                else
                    sprintbit(obj->obj_flags.bitvector[i], affected3_bits,
                        buf2);
                strcat(buf, buf2);
                strcat(buf, " ");
            }

        for (i = 0; i < MAX_OBJ_AFFECT; i++)
            if (obj->affected[i].location && obj->affected[i].modifier &&
                obj->affected[i].location != APPLY_DAMROLL)
                sprintf(buf, "%s%s%s%d ", buf,
                    apply_types[(int)obj->affected[i].location],
                    obj->affected[i].modifier > 0 ? "+" : "",
                    obj->affected[i].modifier);

        strcat(buf, "\r\n");
        strcat(buf3, buf);
    }

    strcat(buf3, "\r\n\r\n");

    for (i = 0; i < 60; i++)
        sprintf(buf3, "%s%2d -- [ %2d] weapons\r\n", buf3, i, avg_dam[i]);

    page_string(ch->desc, buf3);
    return 1;
}
