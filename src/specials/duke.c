/* ************************************************************************
*   File: castle.c                                      Part of CircleMUD *
*  Usage: Special procedures for King's Castle area                       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Special procedures for Kings Castle by Pjotr (d90-pem@nada.kth.se)     *
*  Coded by Sapowox (d90-jkr@nada.kth.se)                                 *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: duke.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#ifdef HAS_CONFIG_H
#endif

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <libpq-fe.h>
#include <libxml/parser.h>
#include <glib.h>

#include "interpreter.h"
#include "structs.h"
#include "utils.h"
#include "constants.h"
#include "comm.h"
#include "security.h"
#include "handler.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "race.h"
#include "creature.h"
#include "db.h"
#include "tmpstr.h"
#include "spells.h"
#include "fight.h"
#include "obj_data.h"

/*   external vars  */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
void set_local_time(struct zone_data *zone, struct time_info_data *local_time);

/* IMPORTANT!
   The below defined number is the zone number of the Kings Castle.
   Change it to apply to your chosen zone number. The default zone
   number (On Alex and Alfa) is 80 (That is rooms and mobs have numbers
   in the 8000 series... */

#define Z_DUKES_C 152

/**********************************************************************\
|* Special procedures for Kings Castle by Pjotr (d90-pem@nada.kth.se) *|
|* Coded by Sapowox (d90-jkr@nada.kth.se)                             *|
\**********************************************************************/

#define R_ROOM(zone, num) (real_room(((zone)*100)+(num)))

#define EXITN(room, door)     (room->dir_option[door])

#define CASTLE_ITEM(item) (Z_DUKES_C*100+(item))
/* note that castle_item() works necesarrily only for mobs and objs
   our castle covers rooms from 15200-15499                                */

/* Routine member_of_staff */
/* Used to see if a character is a member of the castle staff */
/* Used mainly by BANZAI:ng NPC:s */
int
member_of_staff(struct creature *chChar)
{
    int ch_num;

    if (!IS_NPC(chChar))
        return (false);

    ch_num = GET_NPC_VNUM(chChar);
    return (ch_num == CASTLE_ITEM(1) ||
        (ch_num > CASTLE_ITEM(2) && ch_num < CASTLE_ITEM(15)) ||
        (ch_num > CASTLE_ITEM(15) && ch_num < CASTLE_ITEM(18)) ||
        (ch_num > CASTLE_ITEM(18) && ch_num < CASTLE_ITEM(30)));
}

/* Function member_of_royal_guard */
/* Returns true if the character is a guard on duty, otherwise false */
/* Used by Peter the captain of the royal guard */
int
member_of_royal_guard(struct creature *chChar)
{
    int ch_num;

    if (!chChar || !IS_NPC(chChar))
        return false;

    ch_num = GET_NPC_VNUM(chChar);
    return (ch_num == CASTLE_ITEM(3) ||
        ch_num == CASTLE_ITEM(6) ||
        (ch_num > CASTLE_ITEM(7) && ch_num < CASTLE_ITEM(12)) ||
        (ch_num > CASTLE_ITEM(23) && ch_num < CASTLE_ITEM(26)));
}

/* Function find_npc_by_name */
/* Returns a pointer to an npc by the given name */
/* Used by Tim and Tom */
struct creature *
find_npc_by_name(struct creature *chAtChar, const char *pszName, int iLen)
{
    for (GList * cit = chAtChar->in_room->people; cit; cit = cit->next) {
        struct creature *tch = cit->data;
        if (IS_NPC(tch))
            if (!strncmp(pszName, tch->player.short_descr, iLen))
                return tch;
    }
    return NULL;
}

/* Function find_guard */
/* Returns the pointer to a guard on duty. */
/* Used by Peter the Captain of the Royal Guard */
struct creature *
find_guard(struct creature *chAtChar)
{

    for (GList * cit = chAtChar->in_room->people; cit; cit = cit->next) {
        struct creature *tch = cit->data;
        if (!is_fighting(tch) && member_of_royal_guard(tch))
            return tch;
    }
    return NULL;
}

/* Function get_victim */
/* Returns a pointer to a randomly chosen character in the same room, */
/* fighting someone in the castle staff... */
/* Used by BANZAII-ing characters and King Welmar... */
struct creature *
get_victim(struct creature *chAtChar)
{

    int iNum_bad_guys = 0, iVictim;
    for (GList * cit = chAtChar->in_room->people; cit; cit = cit->next) {
        struct creature *tch = cit->data;
        if (is_fighting(tch) && member_of_staff(random_opponent(tch)))
            iNum_bad_guys++;
    }
    if (!iNum_bad_guys)
        return NULL;

    iVictim = number(0, iNum_bad_guys); /* How nice, we give them a chance */
    if (!iVictim)
        return NULL;

    iNum_bad_guys = 0;
    for (GList * cit = chAtChar->in_room->people; cit; cit = cit->next) {
        struct creature *tch = cit->data;
        if (is_fighting(tch) &&
            member_of_staff(random_opponent(tch)) &&
            ++iNum_bad_guys == iVictim)
            return tch;
    }
    return NULL;
}

/* Function banzaii */
/* Makes a character banzaii on attackers of the castle staff */
/* Used by Guards, Tim, Tom, Peter, Master, King and Guards */
int
banzaii(struct creature *ch)
{

    struct creature *chOpponent = NULL;

    if (!AWAKE(ch) || GET_POSITION(ch) == POS_FIGHTING)
        return false;

    if ((chOpponent = get_victim(ch))) {
        act("$n roars: 'Protect the Duchy of the Great Duke Araken!  BANZAIIII!!!'", false, ch, NULL, NULL, TO_ROOM);
        hit(ch, chOpponent, TYPE_UNDEFINED);
        return true;
    }
    return false;
}

/* Function do_npc_rescue */
/* Makes ch_hero rescue ch_victim */
/* Used by Tim and Tom */
int
do_npc_rescue(struct creature *ch_hero, struct creature *ch_victim)
{

    struct creature *ch_bad_guy = NULL;
    for (GList * cit = ch_hero->in_room->people; cit; cit = cit->next) {
        struct creature *tch = cit->data;
        if (g_list_find(tch->fighting, ch_victim))
            ch_bad_guy = tch;
    }
    if (ch_bad_guy) {
        if (ch_bad_guy == ch_hero)
            return false;       /* NO WAY I'll rescue the one I'm fighting! */
        act("You bravely rescue $N.\r\n", false, ch_hero, NULL, ch_victim,
            TO_CHAR);
        act("You are rescued by $N, your loyal friend!\r\n", false, ch_victim,
            NULL, ch_hero, TO_CHAR);
        act("$n heroically rescues $N.", false, ch_hero, NULL, ch_victim,
            TO_NOTVICT);

        remove_combat(ch_bad_guy, ch_victim);
        remove_combat(ch_victim, ch_bad_guy);

        add_combat(ch_hero, ch_bad_guy, false);
        add_combat(ch_bad_guy, ch_hero, true);
        return true;
    }
    return false;
}

/* Procedure to block a person trying to enter a room. */
/* Used by Tim/Tom at Kings bedroom */
int
block_way(struct creature *ch, struct creature *guard, int cmd,
    int iIn_room, int iProhibited_direction)
{

    if (cmd != ++iProhibited_direction || (ch->player.short_descr &&
            !strncmp(ch->player.short_descr, "Duke Araken", 11)))
        return false;

    if (!can_see_creature(guard, ch) || !AWAKE(guard))
        return false;

    if ((ch->in_room == real_room(iIn_room)) && (cmd == iProhibited_direction)) {
        if (!member_of_staff(ch))
            act("$N roars at $n and pushes $m back.",
                false, ch, NULL, guard, TO_ROOM);
        act("$N roars: 'Entrance is Prohibited!', and pushes you back.", false,
            ch, NULL, guard, TO_CHAR);
        return (true);
    }
    return false;
}

/* Routine to check if an object is trash... */
/* Used by James the Butler and the Cleaning Lady */
int
is_trash(struct obj_data *i)
{
    // don't get sigilized items
    if (GET_OBJ_SIGIL_IDNUM(i))
        return false;
    else if (GET_OBJ_VNUM(i) == QUAD_VNUM)
        return false;

    else if (IS_SET(i->obj_flags.wear_flags, ITEM_WEAR_TAKE) &&
        ((IS_OBJ_TYPE(i, ITEM_DRINKCON)) || (GET_OBJ_COST(i) <= 50)))
        return true;
    else
        return false;
}

/* Function fry_victim */
/* Finds a suitabe victim, and cast some _NASTY_ spell on him */
/* Used by King Welmar */
void
fry_victim(struct creature *ch)
{

    struct creature *tch;

    if (ch->points.mana < 10)
        return;

    /* Find someone suitable to fry ! */

    if (!(tch = get_victim(ch)))
        return;

    switch (number(0, 8)) {
    case 1:
    case 2:
    case 3:
        act("You raise your hand in a dramatic gesture.", 1, ch, NULL, NULL,
            TO_CHAR);
        act("$n raises $s hand in a dramatic gesture.", 1, ch, NULL, NULL, TO_ROOM);
        cast_spell(ch, tch, NULL, NULL, SPELL_COLOR_SPRAY);
        break;
    case 4:
    case 5:
        act("You concentrate and mumble to yourself.", 1, ch, NULL, NULL, TO_CHAR);
        act("$n concentrates, and mumbles to $mself.", 1, ch, NULL, NULL, TO_ROOM);
        cast_spell(ch, tch, NULL, NULL, SPELL_HARM);
        break;
    case 6:
    case 7:
        act("You look deeply into the eyes of $N.", 1, ch, NULL, tch, TO_CHAR);
        act("$n looks deeply into the eyes of $N.", 1, ch, NULL, tch, TO_NOTVICT);
        act("You see an ill-boding flame in the eye of $n.", 1, ch, NULL, tch,
            TO_VICT);
        cast_spell(ch, tch, NULL, NULL, SPELL_FIREBALL);
        break;
    default:
        if (!number(0, 1))
            cast_spell(ch, ch, NULL, NULL, SPELL_HEAL);
        break;
    }

    ch->points.mana -= 10;

    return;
}

SPECIAL(duke_araken)
{
    struct time_info_data local_time;
    ACMD(do_gen_door);

    static const char *monolog[] = {
        "$n proclaims 'Primus in regnis Geticis coronam'.",
        "$n proclaims 'regiam gessi, subiique regis'.",
        "$n proclaims 'munus et mores colui sereno'.",
        "$n proclaims 'principe dignos'."
    };

    static char bedroom_path[] = "s33004o1c1S.";

    static char throne_path[] = "W3o3cG52211rg.";

    static char monolog_path[] = "ABCDPPPP.";

    static char *path;
    static int index;
    static bool move = false;

    if (spec_mode != SPECIAL_TICK)
        return 0;
    set_local_time(ch->in_room->zone, &local_time);

    if (!move) {
        if (local_time.hours == 8 && ch->in_room == R_ROOM(Z_DUKES_C, 51)) {
            move = true;
            path = throne_path;
            index = 0;
        } else if (local_time.hours == 21
            && ch->in_room == R_ROOM(Z_DUKES_C, 17)) {
            move = true;
            path = bedroom_path;
            index = 0;
        } else if (local_time.hours == 12
            && ch->in_room == R_ROOM(Z_DUKES_C, 17)) {
            move = true;
            path = monolog_path;
            index = 0;
        }
    }
    if (cmd || (GET_POSITION(ch) < POS_SLEEPING) ||
        (GET_POSITION(ch) == POS_SLEEPING && !move))
        return false;

    if (GET_POSITION(ch) == POS_FIGHTING) {
        fry_victim(ch);
        return false;
    } else if (banzaii(ch))
        return false;

    if (!move)
        return false;

    switch (path[index]) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
        perform_move(ch, path[index] - 0, MOVE_NORM, 1);
        break;
    case 'A':
    case 'B':
    case 'C':
    case 'D':
        act(monolog[path[index] - 'A'], false, ch, NULL, NULL, TO_ROOM);
        break;
    case 'P':
        break;
    case 'W':
        GET_POSITION(ch) = POS_STANDING;
        act("$n awakens and stands up.", false, ch, NULL, NULL, TO_ROOM);
        break;

    case 'S':
        GET_POSITION(ch) = POS_SLEEPING;
        act("$n lies down on $s beautiful bed and instantly falls asleep.",
            false, ch, NULL, NULL, TO_ROOM);
        break;

    case 'r':
        GET_POSITION(ch) = POS_SITTING;
        act("$n sits down on $s great throne.", false, ch, NULL, NULL, TO_ROOM);
        break;

    case 's':
        GET_POSITION(ch) = POS_STANDING;
        act("$n stands up.", false, ch, NULL, NULL, TO_ROOM);
        break;

    case 'G':
        act("$n says 'Good morning, trusted friends.'", false, ch, NULL, NULL,
            TO_ROOM);
        break;

    case 'g':
        act("$n says 'Good morning, dear subjects.'", false, ch, NULL, NULL,
            TO_ROOM);
        break;

    case 'o':
        do_gen_door(ch, tmp_strdup("door"), 0, SCMD_UNLOCK);
        do_gen_door(ch, tmp_strdup("door"), 0, SCMD_OPEN);
        break;

    case 'c':
        do_gen_door(ch, tmp_strdup("door"), 0, SCMD_CLOSE);
        do_gen_door(ch, tmp_strdup("door"), 0, SCMD_LOCK);
        break;

    case '.':
        move = false;
        break;
    }

    index++;
    return false;
}

/* Function training_master */
/* Acts actions to the training room, if his students are present */
/* Also allowes warrior-char_class to practice */
/* Used by the Training Master */
SPECIAL(training_master)
{

    struct creature *pupil1, *pupil2, *tch;

    if (spec_mode == SPECIAL_TICK)
        return 0;

    if (!AWAKE(ch) || (GET_POSITION(ch) == POS_FIGHTING))
        return false;

    if (cmd)
        return false;

    if (!banzaii(ch) && !number(0, 2)) {
        if ((pupil1 = find_npc_by_name(ch, "Brian", 5)) &&
            (pupil2 = find_npc_by_name(ch, "Mick", 4)) &&
            (!is_fighting(pupil1) && !is_fighting(pupil2))) {
            if (number(0, 1)) {
                tch = pupil1;
                pupil1 = pupil2;
                pupil2 = tch;
            }
            switch (number(0, 7)) {
            case 0:
                act("$n hits $N on $s head with a powerful blow.",
                    false, pupil1, NULL, pupil2, TO_NOTVICT);
                act("You hit $N on $s head with a powerful blow.",
                    false, pupil1, NULL, pupil2, TO_CHAR);
                act("$n hits you on your head with a powerful blow.",
                    false, pupil1, NULL, pupil2, TO_VICT);
                break;
            case 1:
                act("$n hits $N in $s chest with a thrust.",
                    false, pupil1, NULL, pupil2, TO_NOTVICT);
                act("You manage to thrust $N in the chest.",
                    false, pupil1, NULL, pupil2, TO_CHAR);
                act("$n manages to thrust you in your chest.",
                    false, pupil1, NULL, pupil2, TO_VICT);
                break;
            case 2:
                send_to_char(ch, "You command your pupils to bow\r\n.");
                act("$n commands $s pupils to bow.", false, ch, NULL, NULL,
                    TO_ROOM);
                act("$n bows before $N.", false, pupil1, NULL, pupil2,
                    TO_NOTVICT);
                act("$N bows before $n.", false, pupil1, NULL, pupil2,
                    TO_NOTVICT);
                act("You bow before $N, who returns your gesture.", false,
                    pupil1, NULL, pupil2, TO_CHAR);
                act("You bow before $n, who returns your gesture.", false,
                    pupil1, NULL, pupil2, TO_VICT);
                break;
            case 3:
                act("$N yells at $n, as $e fumbles and drops $s sword.",
                    false, pupil1, NULL, ch, TO_NOTVICT);
                act("$n quickly picks up $s weapon.", false, pupil1, NULL, NULL,
                    TO_ROOM);
                act("$N yells at you, as you fumble, losing your weapon.",
                    false, pupil1, NULL, ch, TO_CHAR);
                send_to_char(pupil1, "You quickly pick up your weapon again.");
                act("You yell at $n, as $e fumbles, losing $s weapon.",
                    false, pupil1, NULL, ch, TO_VICT);
                break;
            case 4:
                act("$N tricks $n, and slashes him across the back.",
                    false, pupil1, NULL, pupil2, TO_NOTVICT);
                act("$N tricks you, and slashes you across your back.",
                    false, pupil1, NULL, pupil2, TO_CHAR);
                act("You trick $n, and quickly slash him across $s back.",
                    false, pupil1, NULL, pupil2, TO_VICT);
                break;
            case 5:
                act("$n lunges a blow at $N but $N parries skillfully.",
                    false, pupil1, NULL, pupil2, TO_NOTVICT);
                act("You lunge a blow at $N but $E parries skillfully.",
                    false, pupil1, NULL, pupil2, TO_CHAR);
                act("$n lunges a blow at you, but you skillfully parry it.",
                    false, pupil1, NULL, pupil2, TO_VICT);
                break;
            case 6:
                act("$n clumsily tries to kick $N, but misses.",
                    false, pupil1, NULL, pupil2, TO_NOTVICT);
                act("You clumsily miss $N with your poor excuse for a kick.",
                    false, pupil1, NULL, pupil2, TO_CHAR);
                act("$n fails an unusually clumsy attempt at kicking you.",
                    false, pupil1, NULL, pupil2, TO_VICT);
                break;
            default:
                send_to_char(ch,
                    "You show your pupils an advanced technique.");
                act("$n shows $s pupils an advanced technique.", false, ch, NULL,
                    NULL, TO_ROOM);
                break;
            }
        }
    }
    return false;
}

SPECIAL(tom)
{

    struct creature *tom = (struct creature *)me;
    struct creature *king, *tim;

    if (spec_mode != SPECIAL_TICK)
        return 0;
    ACMD(do_follow);

    if (!AWAKE(ch))
        return false;

    if ((!cmd) && (king = find_npc_by_name(ch, "Duke Araken", 11))) {
        if (!ch->master)
            do_follow(ch, tmp_strdup("Duke Araken"), 0, 0);
        if (is_fighting(king))
            do_npc_rescue(ch, king);
    }
    if (!cmd)
        if ((tim = find_npc_by_name(ch, "Tim", 3)))
            if (is_fighting(tim) && 2 * GET_HIT(tim) < GET_HIT(ch))
                do_npc_rescue(ch, tim);

    if (!cmd && GET_POSITION(ch) != POS_FIGHTING)
        banzaii(ch);

    return block_way(ch, tom, cmd, CASTLE_ITEM(49), 1);
}

SPECIAL(tim)
{

    struct creature *tim = (struct creature *)me;
    struct creature *king, *tom;

    if (spec_mode != SPECIAL_TICK)
        return 0;
    ACMD(do_follow);

    if (!AWAKE(ch))
        return false;

    if ((!cmd) && (king = find_npc_by_name(ch, "Duke Araken", 11))) {
        if (!ch->master)
            do_follow(ch, tmp_strdup("Duke Araken"), 0, 0);
        if (is_fighting(king))
            do_npc_rescue(ch, king);
    }
    if (!cmd)
        if ((tom = find_npc_by_name(ch, "Tom", 3)))
            if (is_fighting(tom) && 2 * GET_HIT(tom) < GET_HIT(ch))
                do_npc_rescue(ch, tom);

    if (!cmd && GET_POSITION(ch) != POS_FIGHTING)
        banzaii(ch);

    return block_way(ch, tim, cmd, CASTLE_ITEM(49), 1);
}

/* Routine for James the Butler */
/* Complains if he finds any trash... */
SPECIAL(James)
{

    struct obj_data *i;

    if (spec_mode != SPECIAL_TICK)
        return 0;
    if (cmd || !AWAKE(ch) || (GET_POSITION(ch) == POS_FIGHTING))
        return (false);

    for (i = ch->in_room->contents; i; i = i->next_content)
        if (is_trash(i)) {
            act("$n says: 'My oh my!  I ought to fire that lazy cleaning woman!'", false, ch, NULL, NULL, TO_ROOM);
            act("$n picks up a piece of trash.", false, ch, NULL, NULL, TO_ROOM);
            obj_from_room(i);
            obj_to_char(i, ch);
            return true;
        } else
            return false;

    return false;
}

/* Routine for the Cleaning Woman */
/* Picks up any trash she finds... */
SPECIAL(cleaning)
{
    struct obj_data *i, *next;

    if (spec_mode != SPECIAL_TICK)
        return 0;

    if (cmd || !AWAKE(ch))
        return 0;

    for (i = ch->in_room->contents; i; i = next) {
        next = i->next_content;
        if (is_trash(i)) {
            act("$n picks up some trash.", false, ch, NULL, NULL, TO_ROOM);
            obj_from_room(i);
            obj_to_char(i, ch);
            return true;
        }
    }
    return false;
}

/* Routine CastleGuard */
/* Standard routine for ordinary castle guards */
SPECIAL(CastleGuard)
{

    if (spec_mode != SPECIAL_TICK)
        return 0;
    if (cmd || !AWAKE(ch) || (GET_POSITION(ch) == POS_FIGHTING))
        return false;

    return (banzaii(ch));
}

SPECIAL(sleeping_soldier)
{
    if (spec_mode != SPECIAL_TICK)
        return 0;
    if (cmd || (GET_POSITION(ch) == POS_FIGHTING))
        return false;
    if (!AWAKE(ch))
        switch (number(0, 60)) {
        case 0:
            act("$n farts in $s sleep.", false, ch, NULL, NULL, TO_ROOM);
            break;
        case 1:
            act("$n rolls over fitfully.", false, ch, NULL, NULL, TO_ROOM);
            break;
        case 2:
            act("$n moans loudly.", false, ch, NULL, NULL, TO_ROOM);
            break;
        case 3:
            act("$n begins to exhibit Rapid Eye Movement.", true, ch, NULL, NULL,
                TO_ROOM);
            break;
        case 4:
            act("$n drools all over the place.", true, ch, NULL, NULL, TO_ROOM);
            break;
        }
    if (!AWAKE(ch))
        return 0;

    return (banzaii(ch));
}

SPECIAL(lounge_soldier)
{
    if (spec_mode != SPECIAL_TICK)
        return 0;
    if (cmd || !AWAKE(ch) || (GET_POSITION(ch) == POS_FIGHTING))
        return false;

    switch (number(0, 60)) {
    case 0:
        act("$n grabs a dirty magazine and starts flipping through the pages.",
            true, ch, NULL, NULL, TO_ROOM);
        send_to_char(ch, "You grab a dirty mag.\r\n");
        break;
    case 1:
        act("$n counts up this month's pay and scowls.", true, ch, NULL, NULL,
            TO_ROOM);
        send_to_char(ch, "You count your pay and scowl.\r\n");
        break;
    case 2:
        act("$n wonders when $s next leave will be.", true, ch, NULL, NULL, TO_ROOM);
        send_to_char(ch, "You wonder about leave.\r\n");
        break;
    case 3:
        act("$n starts biting $s fingernails.", true, ch, NULL, NULL, TO_ROOM);
        send_to_char(ch, "You bite your fingernails.\r\n");
        break;
    case 4:
        act("$n starts biting $s toenails.", true, ch, NULL, NULL, TO_ROOM);
        send_to_char(ch, "You bite your toenails.\r\n");
        break;
    case 5:
        act("$n flexes $s muscles.", true, ch, NULL, NULL, TO_ROOM);
        send_to_char(ch, "You flex.\r\n");
        break;
    case 6:
        act("$n rolls up a blunt.", true, ch, NULL, NULL, TO_ROOM);
        send_to_char(ch, "You roll a blunt.\r\n");
        break;
    case 7:
        act("$n starts shadowboxing.", false, ch, NULL, NULL, TO_ROOM);
        send_to_char(ch, "You shadowbox.\r\n");
        break;
    case 8:
        act("$n scratches $s butt.", true, ch, NULL, NULL, TO_ROOM);
        send_to_char(ch, "You scratch your butt.\r\n");
        break;
    }
    return (banzaii(ch));
}

SPECIAL(armory_person)
{

    struct creature *guard = (struct creature *)me;
    if (spec_mode != SPECIAL_TICK)
        return 0;
    if (!cmd || IS_NPC(ch))
        return false;

    if (!can_see_creature(guard, ch) || is_fighting(guard))
        return false;

    act("$n screams, 'This is a RESTRICTED AREA!!!'", false, guard, NULL, NULL,
        TO_ROOM);
    hit(guard, ch, TYPE_UNDEFINED);
    return true;

}

/* Routine peter */
/* Routine for Captain of the Guards. */
SPECIAL(peter)
{

    struct creature *ch_guard;

    if (spec_mode != SPECIAL_TICK)
        return 0;
    if (cmd || !AWAKE(ch) || GET_POSITION(ch) == POS_FIGHTING)
        return (false);

    if (banzaii(ch))
        return false;

    if (!(number(0, 3)) && (ch_guard = find_guard(ch)))
        switch (number(0, 5)) {
        case 0:
            act("$N comes sharply into attention as $n inspects $M.",
                false, ch, NULL, ch_guard, TO_NOTVICT);
            act("$N comes sharply into attention as you inspect $M.",
                false, ch, NULL, ch_guard, TO_CHAR);
            act("You go sharply into attention as $n inspects you.",
                false, ch, NULL, ch_guard, TO_VICT);
            break;
        case 1:
            act("$N looks very small, as $n roars at $M.",
                false, ch, NULL, ch_guard, TO_NOTVICT);
            act("$N looks very small as you roar at $M.",
                false, ch, NULL, ch_guard, TO_CHAR);
            act("You feel very small as $N roars at you.",
                false, ch, NULL, ch_guard, TO_VICT);
            break;
        case 2:
            act("$n gives $N some Royal directions.",
                false, ch, NULL, ch_guard, TO_NOTVICT);
            act("You give $N some Royal directions.",
                false, ch, NULL, ch_guard, TO_CHAR);
            act("$n gives you some Royal directions.",
                false, ch, NULL, ch_guard, TO_VICT);
            break;
        case 3:
            act("$n looks at you.", false, ch, NULL, ch_guard, TO_VICT);
            act("$n looks at $N.", false, ch, NULL, ch_guard, TO_NOTVICT);
            act("$n growls: 'Those boots need polishing!'",
                false, ch, NULL, ch_guard, TO_ROOM);
            act("You growl at $N.", false, ch, NULL, ch_guard, TO_CHAR);
            break;
        case 4:
            act("$n looks at you.", false, ch, NULL, ch_guard, TO_VICT);
            act("$n looks at $N.", false, ch, NULL, ch_guard, TO_NOTVICT);
            act("$n growls: 'Straighten that collar!'",
                false, ch, NULL, ch_guard, TO_ROOM);
            act("You growl at $N.", false, ch, NULL, ch_guard, TO_CHAR);
            break;
        default:
            act("$n looks at you.", false, ch, NULL, ch_guard, TO_VICT);
            act("$n looks at $N.", false, ch, NULL, ch_guard, TO_NOTVICT);
            act("$n growls: 'That chain mail looks rusty!  CLEAN IT !!!'",
                false, ch, NULL, ch_guard, TO_ROOM);
            act("You growl at $N.", false, ch, NULL, ch_guard, TO_CHAR);
            break;
        }

    return false;
}

/* Procedure for Jerry and Michael in x08 of King's Castle.      */
/* Code by Sapowox modified by Pjotr.(Original code from Master) */

SPECIAL(jerry)
{
    struct creature *gambler1, *gambler2, *tch;

    if (spec_mode != SPECIAL_TICK)
        return 0;
    if (!AWAKE(ch) || (GET_POSITION(ch) == POS_FIGHTING))
        return false;

    if (cmd)
        return false;

    if (!banzaii(ch) && !number(0, 2)) {
        if ((gambler1 = ch) &&
            (gambler2 = find_npc_by_name(ch, "Michael", 7)) &&
            (!is_fighting(gambler1) && !is_fighting(gambler2))) {
            if (number(0, 1)) {
                tch = gambler1;
                gambler1 = gambler2;
                gambler2 = tch;
            }
            switch (number(0, 5)) {
            case 0:
                act("$n rolls the dice and cheers loudly at the result.",
                    false, gambler1, NULL, gambler2, TO_NOTVICT);
                act("You roll the dice and cheer. GREAT!",
                    false, gambler1, NULL, gambler2, TO_CHAR);
                act("$n cheers loudly as $e rolls the dice.",
                    false, gambler1, NULL, gambler2, TO_VICT);
                break;
            case 1:
                act("$n curses the Goddess of Luck roundly as $e sees $N's roll.", false, gambler1, NULL, gambler2, TO_NOTVICT);
                act("You curse the Goddess of Luck as $N rolls.",
                    false, gambler1, NULL, gambler2, TO_CHAR);
                act("$n swears angrily. You are in luck!",
                    false, gambler1, NULL, gambler2, TO_VICT);
                break;
            case 2:
                act("$n sighs loudly and gives $N some gold.",
                    false, gambler1, NULL, gambler2, TO_NOTVICT);
                act("You sigh loudly at the pain of having to give $N some gold.", false, gambler1, NULL, gambler2, TO_CHAR);
                act("$n sighs loudly as $e gives you your rightful win.",
                    false, gambler1, NULL, gambler2, TO_VICT);
                break;
            case 3:
                act("$n smiles remorsefully as $N's roll tops his.",
                    false, gambler1, NULL, gambler2, TO_NOTVICT);
                act("You smile sadly as you see that $N beats you. Again.",
                    false, gambler1, NULL, gambler2, TO_CHAR);
                act("$n smiles remorsefully as your roll tops his.",
                    false, gambler1, NULL, gambler2, TO_VICT);
                break;
            case 4:
                act("$n excitedly follows the dice with $s eyes.",
                    false, gambler1, NULL, gambler2, TO_NOTVICT);
                act("You excitedly follow the dice with your eyes.",
                    false, gambler1, NULL, gambler2, TO_CHAR);
                act("$n excitedly follows the dice with $s eyes.",
                    false, gambler1, NULL, gambler2, TO_VICT);
                break;
            default:
                act("$n says 'Well, my luck has to change soon', as $e shakes the dice.", false, gambler1, NULL, gambler2, TO_NOTVICT);
                act("You say 'Well, my luck has to change soon' and shake the dice.", false, gambler1, NULL, gambler2, TO_CHAR);
                act("$n says 'Well, my luck has to change soon', as $e shakes the dice.", false, gambler1, NULL, gambler2, TO_VICT);
                break;
            }
        }
    }
    return false;
}

SPECIAL(dukes_chamber)
{
    struct room_data *other_room;
    struct room_direction_data *back = NULL;

    if (!CMD_IS("pull") && !CMD_IS("push") && !CMD_IS("get") && !CMD_IS("take")
        && !CMD_IS("open"))
        return 0;
    skip_spaces(&argument);

    if (strncasecmp(argument, "book", 4) &&
        strncasecmp(argument, "shelf", 5) &&
        strncasecmp(argument, "lever", 5) &&
        strncasecmp(argument, "on lever", 8) &&
        strncasecmp(argument, "on shelf", 8) &&
        strncasecmp(argument, "on book", 7))
        return 0;

    if (CMD_IS("open") && !strncasecmp(argument, "book", 4)) {
        send_to_char(ch, "It's securely lodged in the bookshelf.\r\n");
        return 1;
    }
    if ((other_room = EXIT(ch, WEST)->to_room) != NULL)
        if ((back = other_room->dir_option[rev_dir[WEST]]))
            if (back->to_room != ch->in_room) {
                back = NULL;
                send_to_char(ch, " back= 0");
            }

    send_to_char(ch, "You pull firmly on the protruding book...\r\n");
    act("$n pulls on a book in the bookshelf.", true, ch, NULL, NULL, TO_ROOM);

    if (IS_SET(EXIT(ch, WEST)->exit_info, EX_CLOSED)) {
        send_to_room
            ("A small section of the bookshelf rotates open, revealing a small room\r\n"
            "to the west.\r\n", ch->in_room);

        TOGGLE_BIT(EXITN(ch->in_room, WEST)->exit_info, EX_CLOSED);
        if (back && IS_SET(back->exit_info, EX_CLOSED)) {
            TOGGLE_BIT(EXITN(other_room, rev_dir[WEST])->exit_info, EX_CLOSED);
            send_to_room
                ("The east wall rotates open, revealing a large chamber.\r\n",
                other_room);
        }
        return 1;
    } else {
        send_to_room("A small section of the bookshelf rotates closed.\r\n",
            ch->in_room);

        TOGGLE_BIT(EXITN(ch->in_room, WEST)->exit_info, EX_CLOSED);
        if (back && !IS_SET(back->exit_info, EX_CLOSED)) {
            TOGGLE_BIT(EXITN(other_room, rev_dir[WEST])->exit_info, EX_CLOSED);
            send_to_room("The east wall rotates closed.\r\n", other_room);
        }
        return 1;
    }
}
