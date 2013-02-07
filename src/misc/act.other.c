/* ************************************************************************
*   File: act.other.c                                   Part of CircleMUD *
*  Usage: Miscellaneous player-level commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: act.other.c                      -- Part of TempusMUD
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
#include <sys/stat.h>
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
#include "screen.h"
#include "house.h"
#include "clan.h"
#include "char_class.h"
#include "players.h"
#include "tmpstr.h"
#include "account.h"
#include "spells.h"
#include "vehicle.h"
#include "fight.h"
#include "obj_data.h"
#include "strutil.h"

/* extern variables */
extern struct descriptor_data *descriptor_list;
extern struct spell_info_type spell_info[];
extern struct obj_data *object_list;
extern struct obj_shared_data *null_obj_shared;
extern int log_cmds;
extern int jet_stream_state;

int check_mob_reaction(struct creature *ch, struct creature *vict);
void set_desc_state(enum cxn_state state, struct descriptor_data *d);
void look_at_target(struct creature *ch, char *arg, int cmd);
int find_door(struct creature *ch, char *type, char *dir, const char *cmdname);
void weather_change(void);
int drag_object(struct creature *ch, struct obj_data *obj, char *argument);
void ice_room(struct room_data *room, int amount);
ACMD(do_drag_char);

ACMD(do_show_more)
{
    if (!ch->desc)
        return;
    if (!ch->desc->showstr_head) {
        send_to_char(ch, "You weren't in the middle of reading anything.\r\n");
        return;
    }

    show_string(ch->desc);

}

ACMD(do_quit)
{
    extern int free_rent;
    struct descriptor_data *d, *next_d;
    int cost = 0;

    skip_spaces(&argument);

    if (IS_NPC(ch) || !ch->desc) {
        send_to_char(ch, "No!  You're trapped!  Muhahahahahah!\r\n");
        return;
    }

    if (subcmd != SCMD_QUIT && GET_LEVEL(ch) < LVL_AMBASSADOR)
        send_to_char(ch, "You have to type quit - no less, to quit!\r\n");
    else if (GET_POSITION(ch) == POS_FIGHTING)
        send_to_char(ch, "No way!  You're fighting for your life!\r\n");
    else if (GET_POSITION(ch) < POS_STUNNED) {
        send_to_char(ch, "You die before your time...\r\n");
        die(ch, NULL, 0);
    } else {

        /*
         * kill off all sockets connected to the same player as the one who is
         * trying to quit.  Helps to maintain sanity as well as prevent duping.
         */
        for (d = descriptor_list; d; d = next_d) {
            next_d = d->next;
            if (d == ch->desc)
                continue;
            if (d->creature && (GET_IDNUM(d->creature) == GET_IDNUM(ch)))
                close_socket(d);
        }

        if ((free_rent) || GET_LEVEL(ch) >= LVL_AMBASSADOR || is_tester(ch)) {
            if (GET_LEVEL(ch) >= LVL_AMBASSADOR) {
                mlog(ROLE_ADMINBASIC, GET_INVIS_LVL(ch), NRM, true,
                    "%s has departed from the known multiverse", GET_NAME(ch));
                act("$n steps out of the universe.", true, ch, NULL, NULL, TO_ROOM);
                send_to_char(ch,
                    "Goodbye.  We will be awaiting your return.\r\n");
            } else {
                send_to_char(ch, "\r\nYou flicker out of reality...\r\n");
                act("$n flickers out of reality.", true, ch, NULL, NULL, TO_ROOM);
                mlog(ROLE_ADMINBASIC, GET_INVIS_LVL(ch), NRM, true,
                    "%s has left the game%s", GET_NAME(ch),
                    is_tester(ch) ? " (tester)" : " naked");
            }
            creature_rent(ch);
        } else if ((ch->carrying || IS_WEARING_W(ch)) &&
            !ROOM_FLAGGED(ch->in_room, ROOM_HOUSE) &&
            strncasecmp(argument, "yes", 3)) {
            send_to_char(ch,
                "   If you quit without renting, you will lose all of your things.  Anything\r\n"
                "you have implanted will be destroyed.\r\n"
                "   If you would rather rent, type HELP INNS to find out where the nearest\r\n"
                "inn is located.  If you are SURE you want to QUIT, type 'quit yes'.\r\n");
            return;

        } else if (IS_CARRYING_N(ch) || IS_WEARING_W(ch)
            || ROOM_FLAGGED(ch->in_room, ROOM_HOUSE)) {
            if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
                act("You fear that your death will grieve $N.",
                    false, ch, NULL, ch->master, TO_CHAR);
                return;
            }
            if (ROOM_FLAGGED(ch->in_room, ROOM_HOUSE)) {
                cost = calc_daily_rent(ch, 1, NULL, false);

                if (display_unrentables(ch))
                    return;

                if (cost < 0) {
                    send_to_char(ch, "Unable to rent.\r\n");
                    return;
                }

                mlog(ROLE_ADMINBASIC, GET_INVIS_LVL(ch), NRM, true,
                    "%s has left the game from house, room %d",
                    GET_NAME(ch), ch->in_room->number);
                send_to_char(ch, "You smoothly slip out of existence.\r\n");
                act("$n smoothly slips out of existence and is gone.",
                    true, ch, NULL, NULL, TO_ROOM);
                creature_rent(ch);
            } else {
                send_to_char(ch,
                    "\r\nVery well %s.  You drop all your things and vanish!\r\n",
                    GET_NAME(ch));
                act("$n disappears, leaving all $s equipment behind!",
                    true, ch, NULL, NULL, TO_ROOM);
                mlog(ROLE_ADMINBASIC, GET_INVIS_LVL(ch), NRM, true,
                    "%s (%d) has quit the game, EQ drop at %d",
                    GET_NAME(ch), GET_LEVEL(ch), ch->in_room->number);
                creature_quit(ch);
            }
        } else {
            send_to_char(ch, "\r\nYou flicker out of reality...\r\n");
            act("$n flickers out of reality.", true, ch, NULL, NULL, TO_ROOM);
            mlog(ROLE_ADMINBASIC, GET_INVIS_LVL(ch), NRM, true,
                "%s has left the game naked", GET_NAME(ch));
            creature_quit(ch);
        }
    }
}

ACMD(do_save)
{
    if (IS_NPC(ch) || !ch->desc)
        return;

    if (cmd) {
        send_to_char(ch, "Saving %s.\r\n", GET_NAME(ch));
    }
    crashsave(ch);
    save_player_objects(ch);
    if (ROOM_FLAGGED(ch->in_room, ROOM_HOUSE)) {
        struct house *house = find_house_by_room(ch->in_room->number);
        if (house != NULL)
            save_house(house);
    }
}

/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
    send_to_char(ch, "Sorry, but you cannot do that here!\r\n");
}

ACMD(do_elude)
{
    struct affected_type af;

    init_affect(&af);

    if (affected_by_spell(ch, SKILL_ELUSION)) {
        affect_from_char(ch, SKILL_ELUSION);
        send_to_char(ch, "You are no longer being actively elusive.\r\n");
        return;
    } else {
        if (CHECK_SKILL(ch, SKILL_ELUSION) > number(0, 101)) {
            af.type = SKILL_ELUSION;
            af.duration = -1;
            af.aff_index = 1;
            af.bitvector = AFF_NOTRACK;
            af.level = GET_LEVEL(ch) + GET_REMORT_GEN(ch);

            affect_to_char(ch, &af);
        }
        send_to_char(ch, "You begin to elude any followers.\r\n");
    }
}

ACMD(do_practice)
{
    send_to_char(ch, "You can only practice skills in your guild.\r\n");
}

ACMD(do_improve)
{
    send_to_char(ch, "You need expert supervision in order to improve.\r\n");
}

ACMD(do_board)
{
    send_to_char(ch, "And just what do you plan on boarding?\r\n");
}

ACMD(do_palette)
{
    strcpy(buf, "Available Colors:\r\n");
    sprintf(buf,
        "%s%sRED %sBOLD%s %s%sUNDER%s %s%sBLINK %sBLINKBOLD%s %s%sREV %sREVBOLD%s\r\n",
        buf, KRED, KBLD, KNRM, KRED, KUND, KNRM, KRED, KBLK, KBLD, KNRM, KRED,
        KREV, KBLD, KNRM);
    sprintf(buf,
        "%s%sGRN %sBOLD%s %s%sUNDER%s %s%sBLINK %sBLINKBOLD%s %s%sREV %sREVBOLD%s\r\n",
        buf, KGRN, KBLD, KNRM, KGRN, KUND, KNRM, KGRN, KBLK, KBLD, KNRM, KGRN,
        KREV, KBLD, KNRM);
    sprintf(buf,
        "%s%sYEL %sBOLD%s %s%sUNDER%s %s%sBLINK %sBLINKBOLD%s %s%sREV %sREVBOLD%s\r\n",
        buf, KYEL, KBLD, KNRM, KYEL, KUND, KNRM, KYEL, KBLK, KBLD, KNRM, KYEL,
        KREV, KBLD, KNRM);
    sprintf(buf,
        "%s%sBLU %sBOLD%s %s%sUNDER%s %s%sBLINK %sBLINKBOLD%s %s%sREV %sREVBOLD%s\r\n",
        buf, KBLU, KBLD, KNRM, KBLU, KUND, KNRM, KBLU, KBLK, KBLD, KNRM, KBLU,
        KREV, KBLD, KNRM);
    sprintf(buf,
        "%s%sMAG %sBOLD%s %s%sUNDER%s %s%sBLINK %sBLINKBOLD%s %s%sREV %sREVBOLD%s\r\n",
        buf, KMAG, KBLD, KNRM, KMAG, KUND, KNRM, KMAG, KBLK, KBLD, KNRM, KMAG,
        KREV, KBLD, KNRM);
    sprintf(buf,
        "%s%sCYN %sBOLD%s %s%sUNDER%s %s%sBLINK %sBLINKBOLD%s %s%sREV %sREVBOLD%s\r\n",
        buf, KCYN, KBLD, KNRM, KCYN, KUND, KNRM, KCYN, KBLK, KBLD, KNRM, KCYN,
        KREV, KBLD, KNRM);
    sprintf(buf,
        "%s%sWHT %sBOLD%s %s%sUNDER%s %s%sBLINK %sBLINKBOLD%s %s%sREV %sREVBOLD%s\r\n",
        buf, KWHT, KBLD, KNRM, KWHT, KUND, KNRM, KWHT, KBLK, KBLD, KNRM, KWHT,
        KREV, KBLD, KNRM);
    send_to_char(ch, "%s", buf);
}

ACMD(do_disembark)
{
    send_to_char(ch, "Pray tell, whats the point of that?\r\n");
}

ACMD(do_drive)
{
    send_to_char(ch, "Drive WHAT?!\r\n");
}

ACMD(do_visible)
{
    int found = 0;

    if (affect_from_char(ch, SPELL_INVISIBLE)) {
        found = 1;
        send_to_char(ch, "You are no longer invisible.\r\n");
    }
    if (affect_from_char(ch, SPELL_GREATER_INVIS) && !found) {
        found = 1;
        send_to_char(ch, "You are no longer invisible.\r\n");
    }
    if (affect_from_char(ch, SPELL_TRANSMITTANCE)) {
        found = 1;
        send_to_char(ch, "You are no longer transparent.\r\n");
    }
    if (affect_from_char(ch, SPELL_INVIS_TO_UNDEAD)) {
        found = 1;
        send_to_char(ch, "You are no longer invisibile to undead.\r\n");
    }

    if (!found)
        send_to_char(ch, "You are already visible.\r\n");
    else
        act("$n fades into view.", true, ch, NULL, NULL, TO_ROOM);
}

ACMD(do_title)
{
    skip_spaces(&argument);

    if (IS_NPC(ch))
        send_to_char(ch, "Your title is fine... go away.\r\n");
    else if (PLR_FLAGGED(ch, PLR_NOTITLE))
        send_to_char(ch,
            "You can't title yourself -- you shouldn't have abused it!\r\n");
    else if (strstr(argument, "(") || strstr(argument, ")"))
        send_to_char(ch, "Titles can't contain the ( or ) characters.\r\n");
    else if (strlen(argument) > MAX_TITLE_LENGTH) {
        send_to_char(ch,
            "Sorry, titles can't be longer than %d characters.\r\n",
            MAX_TITLE_LENGTH);
    } else {
        set_title(ch, argument);
        send_to_char(ch, "Okay, you're now %s%s.\r\n", GET_NAME(ch),
            GET_TITLE(ch));
    }
}

int
perform_group(struct creature *ch, struct creature *vict)
{
    if (AFF_FLAGGED(vict, AFF_GROUP) || !can_see_creature(ch, vict))
        return 0;

    SET_BIT(AFF_FLAGS(vict), AFF_GROUP);
    if (ch != vict)
        act("$N is now a member of your group.", false, ch, NULL, vict, TO_CHAR);
    act("You are now a member of $n's group.", false, ch, NULL, vict, TO_VICT);
    act("$N is now a member of $n's group.", false, ch, NULL, vict, TO_NOTVICT);
    return 1;
}

const char *
desc_exp_tnl(struct creature *ch)
{
    long tnl;

    if (GET_LEVEL(ch) > 49 || IS_NPC(ch))
        return "";

    tnl = exp_scale[GET_LEVEL(ch) + 1] - GET_EXP(ch);
    if (tnl < 0)
        return "can remort";
    if (tnl > 1000000)
        return tmp_sprintf("%ldmil tnl", tnl / 1000000);
    if (tnl > 1000)
        return tmp_sprintf("%ldk tnl", tnl / 1000);

    return tmp_sprintf("%ld tnl", tnl);
}

void
print_group(struct creature *ch)
{
    struct creature *k;
    struct follow_type *f;

    if (!AFF_FLAGGED(ch, AFF_GROUP))
        send_to_char(ch, "But you are not the member of a group!\r\n");
    else {
        send_to_char(ch, "Your group consists of:\r\n");

        k = (ch->master ? ch->master : ch);

        if (AFF_FLAGGED(k, AFF_GROUP)) {
            sprintf(buf,
                "     %s[%s%4dH %4dM %4dV %4dA%s]%s %s[%s%10s%s]%s %s[%s%2d %s%s]%s $N %s(%sHead of group%s)%s",
                CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                GET_HIT(k), GET_MANA(k), GET_MOVE(k), GET_ALIGNMENT(k),
                CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                desc_exp_tnl(k),
                CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                CCRED(ch, C_NRM), CCNRM(ch, C_NRM),
                GET_LEVEL(k), CLASS_ABBR(k),
                CCRED(ch, C_NRM), CCNRM(ch, C_NRM), CCBLU(ch, C_NRM), CCCYN(ch,
                    C_NRM), CCBLU(ch, C_NRM), CCNRM(ch, C_NRM));
            act(buf, false, ch, NULL, k, TO_CHAR | TO_SLEEP);
        }

        for (f = k->followers; f; f = f->next) {
            if (!AFF_FLAGGED(f->follower, AFF_GROUP))
                continue;

            sprintf(buf,
                "     %s[%s%4dH %4dM %4dV %4dA%s]%s %s[%s%10s%s]%s %s[%s%2d %s%s]%s $N",
                CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_HIT(f->follower),
                GET_MANA(f->follower), GET_MOVE(f->follower),
                GET_ALIGNMENT(f->follower), CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), desc_exp_tnl(f->follower),
                CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), CCRED(ch, C_NRM), CCNRM(ch,
                    C_NRM), GET_LEVEL(f->follower), CLASS_ABBR(f->follower),
                CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
            act(buf, false, ch, NULL, f->follower, TO_CHAR | TO_SLEEP);
        }
    }
}

ACMD(do_group)
{
    struct creature *vict;
    struct follow_type *f;
    int found;

    one_argument(argument, buf);

    if (!*buf) {
        print_group(ch);
        return;
    }

    if (GET_POSITION(ch) < POS_RESTING) {
        send_to_char(ch, "You need to be awake to do that.\r\n");
        return;
    }

    if (ch->master) {
        act("You can not enroll group members without being head of a group.",
            false, ch, NULL, NULL, TO_CHAR);
        return;
    }

    if (!strcasecmp(buf, "all")) {
        perform_group(ch, ch);
        for (found = 0, f = ch->followers; f; f = f->next)
            found += perform_group(ch, f->follower);
        if (!found)
            send_to_char(ch,
                "Everyone following you is already in your group.\r\n");
        return;
    }

    if (!(vict = get_char_room_vis(ch, buf))) {
        send_to_char(ch, "%s", NOPERSON);
    } else if ((vict->master != ch) && (vict != ch))
        act("$N must follow you to enter your group.", false, ch, NULL, vict,
            TO_CHAR);
    else {
        if (!AFF_FLAGGED(vict, AFF_GROUP))
            perform_group(ch, vict);
        else {
            if (ch != vict)
                act("$N is no longer a member of your group.", false, ch, NULL,
                    vict, TO_CHAR);
            act("You have been kicked out of $n's group!", false, ch, NULL, vict,
                TO_VICT);
            act("$N has been kicked out of $n's group!", false, ch, NULL, vict,
                TO_NOTVICT);
            REMOVE_BIT(AFF_FLAGS(vict), AFF_GROUP);
        }
    }
}

ACMD(do_appoint)
{
    struct creature *tch;
    char *name = tmp_getword(&argument);

    if (ch->master || !AFF_FLAGGED(ch, AFF_GROUP)) {
        send_to_char(ch , "But you lead no group!\r\n");
        return;
    }

    if (!*name) {
        send_to_char(ch, "Whom do you wish to appoint as the leader of the group?\r\n");
        return;
    }

    if (!(tch = get_char_room_vis(ch, name))) {
        send_to_char(ch, "There is no such person!\r\n");
        return;
    }

    if (tch->master != ch) {
        send_to_char(ch, "That person is not following you!\r\n");
        return;
    }

    if (!AFF_FLAGGED(tch, AFF_GROUP)) {
        send_to_char(ch, "That person isn't in your group.\r\n");
        return;
    }

    remove_follower(tch);
    new_follower(ch, tch);

    act("&gYou appoint $N as the new group leader.", true,
        ch, NULL, tch, TO_CHAR);
    act("&g$n has appointed you as the group leader.", true,
        ch, NULL, tch, TO_VICT);
    struct follow_type *f, *next_fol;
    for (f = ch->followers; f; f = next_fol) {
        next_fol = f->next;
        struct creature *fch = f->follower;

        if (AFF_FLAGGED(fch, AFF_GROUP)) {
            remove_follower(fch);
            new_follower(fch, tch);
            perform_act("&g$n has appointed $N as the new group leader.",
                        ch, NULL, tch, fch, 0);
        }
    }
}

ACMD(do_ungroup)
{
    struct follow_type *f, *next_fol;
    struct creature *tch;
    void stop_follower(struct creature *ch);

    one_argument(argument, buf);

    if (!*buf) {
        if (ch->master || !(AFF_FLAGGED(ch, AFF_GROUP))) {
            send_to_char(ch, "But you lead no group!\r\n");
            return;
        }
        for (f = ch->followers; f; f = next_fol) {
            next_fol = f->next;
            if (AFF_FLAGGED(f->follower, AFF_GROUP)) {
                REMOVE_BIT(AFF_FLAGS(f->follower), AFF_GROUP);
                if (!AFF_FLAGGED(f->follower, AFF_CHARM)) {
                    send_to_char(f->follower,
                        "%s has disbanded the group.\r\n", PERS(ch,
                            f->follower));
                    stop_follower(f->follower);
                }
            }
        }

        REMOVE_BIT(AFF_FLAGS(ch), AFF_GROUP);
        send_to_char(ch, "You disband the group.\r\n");
        return;
    }
    if (!(tch = get_char_room_vis(ch, buf))) {
        send_to_char(ch, "There is no such person!\r\n");
        return;
    }
    if (tch->master != ch) {
        send_to_char(ch, "That person is not following you!\r\n");
        return;
    }

    if (!AFF_FLAGGED(tch, AFF_GROUP)) {
        send_to_char(ch, "That person isn't in your group.\r\n");
        return;
    }

    REMOVE_BIT(AFF_FLAGS(tch), AFF_GROUP);

    act("$N is no longer a member of your group.", false, ch, NULL, tch, TO_CHAR);
    act("You have been kicked out of $n's group!", false, ch, NULL, tch, TO_VICT);
    act("$N has been kicked out of $n's group!", false, ch, NULL, tch,
        TO_NOTVICT);

    if (!AFF_FLAGGED(tch, AFF_CHARM))
        stop_follower(tch);
}

ACMD(do_report)
{
    struct creature *k;
    struct follow_type *f;

    if (!AFF_FLAGGED(ch, AFF_GROUP)) {
        send_to_char(ch, "But you are not a member of any group!\r\n");
        return;
    }
    sprintf(buf, "%s reports: %d/%dH, %d/%dM, %d/%dV, %dA\r\n",
        GET_NAME(ch), GET_HIT(ch), GET_MAX_HIT(ch),
        GET_MANA(ch), GET_MAX_MANA(ch),
        GET_MOVE(ch), GET_MAX_MOVE(ch), GET_ALIGNMENT(ch));

    k = (ch->master ? ch->master : ch);

    for (f = k->followers; f; f = f->next)
        if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower != ch)
            send_to_char(f->follower, "%s", buf);
    if (k != ch)
        send_to_char(k, "%s", buf);

    send_to_char(ch, "You report: %d/%dH, %d/%dM, %d/%dV, %dA\r\n",
        GET_HIT(ch), GET_MAX_HIT(ch),
        GET_MANA(ch), GET_MAX_MANA(ch),
        GET_MOVE(ch), GET_MAX_MOVE(ch), GET_ALIGNMENT(ch));
}

ACMD(do_split)
{
    int amount, num, share, mode = 0;
    struct creature *k;
    struct follow_type *f;

    if (IS_NPC(ch))
        return;

    argument = one_argument(argument, buf);

    skip_spaces(&argument);

    if (*argument && is_abbrev(argument, "credits"))
        mode = 1;

    if (!is_number(buf)) {
        send_to_char(ch, "How much do you wish to split with your group?\r\n");
        return;
    }
    amount = atoi(buf);
    if (amount <= 0) {
        send_to_char(ch, "Sorry, you can't do that.\r\n");
        return;
    }
    if (mode && amount > GET_CASH(ch)) {
        send_to_char(ch, "You don't seem to have that many credits.\r\n");
        return;
    }
    if (!mode && amount > GET_GOLD(ch)) {
        send_to_char(ch, "You don't seem to have that much gold.\r\n");
        return;
    }
    k = (ch->master ? ch->master : ch);

    if (AFF_FLAGGED(k, AFF_GROUP) && (k->in_room == ch->in_room))
        num = 1;
    else
        num = 0;

    for (f = k->followers; f; f = f->next)
        if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
            (!IS_NPC(f->follower)) && (f->follower->in_room == ch->in_room))
            num++;

    if (num && AFF_FLAGGED(ch, AFF_GROUP))
        share = amount / num;
    else {
        send_to_char(ch, "With whom do you wish to share your loot?\r\n");
        return;
    }

    if (mode)
        GET_CASH(ch) -= share * (num - 1);
    else
        GET_GOLD(ch) -= share * (num - 1);

    if (AFF_FLAGGED(k, AFF_GROUP) && (k->in_room == ch->in_room)
        && !(IS_NPC(k)) && k != ch) {
        if (mode)
            GET_CASH(k) += share;
        else
            GET_GOLD(k) += share;
        send_to_char(k, "%s splits %'d %s; you receive %'d.\r\n", GET_NAME(ch),
            amount, mode ? "credits" : "coins", share);
    }
    for (f = k->followers; f; f = f->next) {
        if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
            (!IS_NPC(f->follower)) &&
            (f->follower->in_room == ch->in_room) && f->follower != ch) {
            if (mode)
                GET_CASH(f->follower) += share;
            else
                GET_GOLD(f->follower) += share;
            send_to_char(f->follower, "%s splits %'d %s; you receive %'d.\r\n",
                GET_NAME(ch), amount, mode ? "credits" : "coins", share);
        }
    }
    send_to_char(ch, "You split %'d %s among %d member%s -- %'d each.\r\n",
        amount, mode ? "credits" : "coins", num, (num > 1 ? "s" : ""), share);
}

ACMD(do_use)
{
    struct obj_data *mag_item;

    char *arg = tmp_getword(&argument);
    if (!*arg) {
        send_to_char(ch, "What do you want to %s?\r\n", CMD_NAME);
        return;
    }
    mag_item = GET_EQ(ch, WEAR_HOLD);

    if (!mag_item || !isname(arg, mag_item->aliases)) {
        switch (subcmd) {
        case SCMD_RECITE:
        case SCMD_QUAFF:
        case SCMD_SWALLOW:
            if (!(mag_item = get_obj_in_list_all(ch, arg, ch->carrying))) {
                send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg),
                    arg);
                return;
            }
            if (subcmd == SCMD_RECITE && !can_see_object(ch, mag_item)) {
                act("You can't see $p well enough to recite from it.",
                    false, ch, mag_item, NULL, TO_CHAR);
                return;
            }
            break;
        case SCMD_READ:
            if (!(mag_item = get_obj_in_list_all(ch, arg, ch->carrying))) {
                send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg),
                    arg);
                return;
            }
            if (!can_see_object(ch, mag_item)) {
                act("You can't see $p well enough to read it.",
                    false, ch, mag_item, NULL, TO_CHAR);
                return;
            }
            break;
        case SCMD_USE:
        case SCMD_INJECT:
            send_to_char(ch, "You don't seem to be holding %s %s.\r\n",
                AN(arg), arg);
            return;
        default:
            errlog("Unknown subcmd passed to do_use");
            return;
        }
    }
    switch (subcmd) {
    case SCMD_QUAFF:
        if (GET_OBJ_TYPE(mag_item) != ITEM_POTION) {
            send_to_char(ch, "You can only quaff potions.\r\n");
            return;
        }
        break;
    case SCMD_SWALLOW:
        if (!IS_OBJ_TYPE(mag_item, ITEM_PILL)) {
            if (IS_OBJ_TYPE(mag_item, ITEM_FOOD))
                send_to_char(ch, "You should really chew that first.\r\n");
            else
                send_to_char(ch, "You cannot swallow that.\r\n");
            return;
        }
        break;

    case SCMD_READ:
        if (is_fighting(ch)) {
            act("What, while fighting $N?!", false, ch, NULL, random_opponent(ch),
                TO_CHAR);
            return;
        }
        if (!IS_OBJ_TYPE(mag_item, ITEM_BOOK)) {
            look_at_target(ch, arg, SCMD_LOOK);
            return;
        }
        break;
    case SCMD_RECITE:
        if (is_fighting(ch)) {
            act("What, while fighting $N?!", false, ch, NULL, random_opponent(ch),
                TO_CHAR);
            return;
        } else if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
            send_to_char(ch, "You can only recite scrolls.\r\n");
            return;
        } else if (CHECK_SKILL(ch, SKILL_READ_SCROLLS) < 10) {
            send_to_char(ch,
                "You do not know how to recite the magical script.\r\n");
            return;
        }
        break;
    case SCMD_USE:
        if (IS_OBJ_TYPE(mag_item, ITEM_TRANSPORTER)) {
            send_to_char(ch,
                "You must use the command 'activate' to use this.\r\n");
            return;
        } else if (IS_OBJ_TYPE(mag_item, ITEM_SYRINGE)) {
            send_to_char(ch, "Use the 'inject' command to use this item.\r\n");
            return;
        } else if (IS_OBJ_TYPE(mag_item, ITEM_CIGARETTE) ||
            IS_OBJ_TYPE(mag_item, ITEM_TOBACCO) ||
            IS_OBJ_TYPE(mag_item, ITEM_PIPE)) {
            send_to_char(ch, "You need to SMOKE that.\r\n");
            return;
        } else if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) &&
            (GET_OBJ_TYPE(mag_item) != ITEM_STAFF)) {
            send_to_char(ch,
                "You can't seem to figure out how to use it.\r\n");
            return;
        } else if (CHECK_SKILL(ch, SKILL_USE_WANDS) < 10) {
            act("You do not know how to harness the magical power of $p.",
                false, ch, mag_item, NULL, TO_CHAR);
            return;
        }
        break;
    case SCMD_INJECT:
        if (GET_OBJ_TYPE(mag_item) != ITEM_SYRINGE) {
            send_to_char(ch, "You can only inject with syringes.\r\n");
            return;
        }
        break;
    default:
        errlog("Illegal subcmd passed to do_use.");
        return;
    }

    mag_objectmagic(ch, mag_item, argument);
}

ACMD(do_wimpy)
{
    int wimp_lev;

    char *arg = tmp_getword(&argument);

    if (!*arg) {
        if (GET_WIMP_LEV(ch)) {
            send_to_char(ch, "Your current wimp level is %d hit points.\r\n",
                GET_WIMP_LEV(ch));
            return;
        } else {
            send_to_char(ch,
                "At the moment, you're not a wimp.  (sure, sure...)\r\n");
            return;
        }
    }
    if (isdigit(*arg)) {
        if ((wimp_lev = atoi(arg))) {
            if (wimp_lev < 0)
                send_to_char(ch,
                    "Heh, heh, heh.. we are jolly funny today, eh?\r\n");
            else if (wimp_lev > GET_MAX_HIT(ch))
                send_to_char(ch,
                    "That doesn't make much sense, now does it?\r\n");
            else if (wimp_lev > (GET_MAX_HIT(ch) / 2))
                send_to_char(ch,
                    "You can't set your wimp level above half your hit points.\r\n");
            else {
                sprintf(buf,
                    "Okay, you'll wimp out if you drop below %d hit points.\r\n",
                    wimp_lev);
                send_to_char(ch, "%s", buf);
                GET_WIMP_LEV(ch) = wimp_lev;
            }
        } else {
            send_to_char(ch,
                "Okay, you'll now tough out fights to the bitter end.\r\n");
            GET_WIMP_LEV(ch) = 0;
        }
    } else
        send_to_char(ch,
            "Specify at how many hit points you want to wimp out at.  (0 to disable)\r\n");

    return;

}

ACMD(do_display)
{
    char *arg1;
    char *arg2;
    unsigned int i;

    arg1 = buf1;
    arg2 = buf2;

    if (IS_NPC(ch)) {
        send_to_char(ch, "Mosters don't need displays.  Go away.\r\n");
        return;
    }
    skip_spaces(&argument);
    argument = two_arguments(argument, arg1, arg2);

    if (!*arg1) {
        send_to_char(ch,
            "Usage: prompt { H | M | V | A | T | all | normal | none %s}\r\n",
            GET_LEVEL(ch) > LVL_IMMORT ? "| vnums" : "");
        return;
    }

    if (arg2 != NULL) {
        if (is_abbrev(arg1, "rows")) {

            if (is_number(arg2)) {
                GET_ROWS(ch) = atoi(arg2);
                return;
            }

            else {
                send_to_char(ch, "Usage: display rows <number>\r\n");
                return;
            }

        } else if (is_abbrev(arg1, "cols")) {

            if (is_number(arg2)) {
                GET_COLS(ch) = atoi(arg2);
                return;
            }

            else {
                send_to_char(ch, "Usage: display cols <number>\r\n");
                return;
            }

        }

    }

    if ((!strcasecmp(arg1, "on")) || (!strcasecmp(arg1, "normal"))) {
        REMOVE_BIT(PRF2_FLAGS(ch), PRF2_DISPALIGN | PRF2_DISPTIME);
        SET_BIT(PRF_FLAGS(ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE);
    }

    else if ((!strcasecmp(arg1, "all"))) {
        SET_BIT(PRF_FLAGS(ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE);
        SET_BIT(PRF2_FLAGS(ch), PRF2_DISPALIGN | PRF2_DISPTIME);
    }

    else if ((!strcasecmp(arg1, "vnums"))) {
        if (PRF2_FLAGS(ch) & PRF2_DISP_VNUMS)
            REMOVE_BIT(PRF2_FLAGS(ch), PRF2_DISP_VNUMS);
        else
            SET_BIT(PRF2_FLAGS(ch), PRF2_DISP_VNUMS);
    } else {
        REMOVE_BIT(PRF_FLAGS(ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE);
        REMOVE_BIT(PRF2_FLAGS(ch), PRF2_DISPALIGN | PRF2_DISPTIME);

        for (i = 0; i < strlen(arg1); i++) {
            switch (tolower(arg1[i])) {
            case 'h':
                SET_BIT(PRF_FLAGS(ch), PRF_DISPHP);
                break;
            case 'm':
                SET_BIT(PRF_FLAGS(ch), PRF_DISPMANA);
                break;
            case 'v':
                SET_BIT(PRF_FLAGS(ch), PRF_DISPMOVE);
                break;
            case 'a':
                SET_BIT(PRF2_FLAGS(ch), PRF2_DISPALIGN);
                break;
            case 't':
                SET_BIT(PRF2_FLAGS(ch), PRF2_DISPTIME);
                break;
            }
        }
    }

    send_to_char(ch, "%s", OK);
}

ACMD(do_gen_write)
{
    FILE *fl;
    char *tmp;
    const char *filename;
    struct stat fbuf;
    extern int max_filesize;
    time_t ct;

    switch (subcmd) {
    case SCMD_BUG:
        filename = BUG_FILE;
        break;
    case SCMD_TYPO:
        filename = TYPO_FILE;
        break;
    case SCMD_IDEA:
        filename = IDEA_FILE;
        break;
    case SCMD_BAD_IDEA:
        send_to_char(ch,
            "You have to type idea - no less, to submit an idea!\r\n");
        return;
    default:
        return;
    }

    ct = time(NULL);
    tmp = asctime(localtime(&ct));

    if (IS_NPC(ch)) {
        send_to_char(ch, "Monsters can't have ideas - Go away.\r\n");
        return;
    }

    skip_spaces(&argument);

    if (!*argument) {
        send_to_char(ch, "That must be a mistake...\r\n");
        return;
    }
    mudlog(MAX(LVL_AMBASSADOR, GET_INVIS_LVL(ch)), NRM, false,
        "%s %s [%d]: %s", GET_NAME(ch), CMD_NAME, ch->in_room->number,
        argument);

    if (stat(filename, &fbuf) < 0) {
        perror("Error statting file");
        return;
    }
    if (fbuf.st_size >= max_filesize) {
        send_to_char(ch,
            "Sorry, the file is full right now.. try again later.\r\n");
        return;
    }
    if (!(fl = fopen(filename, "a"))) {
        perror("do_gen_write");
        send_to_char(ch, "Could not open the file.  Sorry.\r\n");
        return;
    }
    fprintf(fl, "%-8s (%6.6s) [%5d] %s\n", GET_NAME(ch), (tmp + 4),
        ch->in_room->number, argument);
    fclose(fl);
    send_to_char(ch, "Okay.  Thanks!\r\n");
}

#define TOG_OFF 0
#define TOG_ON  1

bool LOG_DEATHS = false;
ACMD(do_gen_tog)
{
    long result = 0;
    extern int nameserver_is_slow;

    const char *tog_messages[][2] = {
        {"You are now safe from summoning by other players.\r\n",
         "You may now be summoned by other players.\r\n"},
        {"Nohassle disabled.\r\n",
         "Nohassle enabled.\r\n"},
        {"Brief mode off.\r\n",
         "Brief mode on.\r\n"},
        {"Compact mode off.\r\n",
         "Compact mode on.\r\n"},
        {"You can now hear tells.\r\n", // 5
         "You are now deaf to tells.\r\n"},
        {"You can now hear auctions.\r\n",
         "You are now deaf to auctions.\r\n"},
        {"You can now hear shouts.\r\n",
         "You are now deaf to shouts.\r\n"},
        {"You can now hear gossip.\r\n",
         "You are now deaf to gossip.\r\n"},
        {"You can now hear the congratulation messages.\r\n",
         "You are now deaf to the congratulation messages.\r\n"},
        {"You can now hear the Wiz-channel.\r\n",   // 10
         "You are now deaf to the Wiz-channel.\r\n"},
        {"You will no longer see the room flags.\r\n",
         "You will now see the room flags.\r\n"},
        {"HolyLight mode off.\r\n",
         "HolyLight mode on.\r\n"},
        {"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
         "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
        {"Autoexits disabled.\r\n",
         "Autoexits enabled.\r\n"},
        {"You have returned to the keyboard.\r\n",  // 15
         "You are now afk.  When you move again, you will no longer be.\r\n"},
        {"You will now hear the music of your peers.\r\n",
         "You will no longer hear the music of your peers.\r\n"},
        {"You will now be subjected to the spewings of your peers.\r\n",
         "You will no longer be subjected to the spewings of your peers.\r\n"},
        {"You will now see all misses in your fights.\r\n",
         "You will no longer see the misses in your fights.\r\n"},
        {"You will no longer be beeped if someone tells to you.\r\n",
         "You will now receive a bell tone if someone tells to you.\r\n"},
        {"You have now opened yourself to clan communication.\r\n", // 20
         "You have closed yourself to clan communication.\r\n"},
        {"You are now allowing others to identify you.\r\n",
         "You are now consciously resisting any attempts to identify you.\r\n"},
        {"You will now stop debugging fights.\r\n",
         "You will now be debugging fights.\r\n"},
        {"You are now ignoring the newbies.\r\n",
         "You are now monitoring the newbies.\r\n"},
        {"You have disabled auto-diagnose.\r\n",
         "You will now automatically be aware of your opponent's condition.\r\n"},
        {"You will now hear the dreams of other players while you sleep.\r\n",  // 25
         "You will no longer hear dreams.\r\n"},
        {"You will now be aware of the projections of other remorts.\r\n",
         "You will now ignore the projections of other remorts.\r\n"},
        {"You are no longer halted.\r\n",
         "You are now in halt mode.\r\n"},
        {"You resume your immortal status.\r\n",
         "Other gods may now kill you... if you don't watch it.\r\n"},
        {"You will now see all affections on the score page.\r\n",
         "You will no longer see your affections on the score page.\r\n"},
        {"You now hear the hollering.\r\n", // 30
         "You now ignore the hollering.\r\n"},
        {"You are now on the immchat channel.\r\n",
         "You are now closed off from the immchat channel.\r\n",},
        {"Your clan badge will now show up on the who list.\r\n",
         "Your clan badge will now be hidden on the who list.\r\n"},
        {"Your prompt will now be displayed only after carriage returns.\r\n",
         "Your prompt will now be redrawn after every message you receive.\r\n"},
        {"You will now be shown on the who list.\r\n",
         "You will no longer be shown on the who list.\r\n"},
        {"Your level will be shown on the who list.\r\n",   // 35
         "Your level will not be shown on the who list.\r\n"},
        {"You will now see affect trailers on characters.\r\n",
         "You will now ignore affect trailers on characters.\r\n"},
        {"Logall", "Logall"},
        {"Jetstream", "Jetstream"},
        {"Weather", "Weather"},
        {"Autosplit OFF.\r\n",  // 40
         "Autosplit ON.\r\n"},
        {"Autoloot OFF.\r\n",
         "Autoloot ON.\r\n"},
        {"Pkiller OFF.\r\n",
         "Pkiller ON.\r\n"},
        {"You open yourself to the echoing thoughts of the gods.\r\n",
         "You close your mind to the echoing thoughts of the gods.\r\n"},
        {"TEDII will now wrap your text at 76 columns.\r\n",
         "TEDII will not wrap your text at 76 colums next time.\r\n"},
        {"Your world writing ability is now deactivated.\r\n",  // 45
         "Your world writing ability is now activated.\r\n"},
        {"Deaths will not be logged.\r\n",
         "All deaths will now be logged.\r\n"},
        {"You can now hear the rumors within your guild.\r\n",
         "You are now deaf to the rumors of your guild members.\r\n"},
        {"You can now hear the petitions of mortals.\r\n",
         "You are now deaf to the petitions of mortals.\r\n"},
        {"You are now protected from the nasty words of your peers.\r\n",
         "You are now subjected to the nastiness of your peers.\r\n"},
        {"You can now hear the haggling of your peers.\r\n",
         "You are now deaf to the haggling of your peers.\r\n"},
        {"You can now hear criers plug tempus.\r\n",
         "You no longer hear criers plug tempus.\r\n"},
        {"You are now using the imperial system of measurement.\r\n",
         "You are now using the metric system of measurement.\r\n"},
        {"\n", "\n"}
    };

    if (IS_NPC(ch))
        return;

    switch (subcmd) {
    case SCMD_NOHASSLE:
        result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
        break;
    case SCMD_BRIEF:
        result = PRF_TOG_CHK(ch, PRF_BRIEF);
        break;
    case SCMD_NOTELL:
        result = PRF_TOG_CHK(ch, PRF_NOTELL);
        break;
    case SCMD_NOAUCTION:
        result = PRF_TOG_CHK(ch, PRF_NOAUCT);
        break;
    case SCMD_DEAF:
        result = PRF_TOG_CHK(ch, PRF_DEAF);
        break;
    case SCMD_NOGOSSIP:
        result = PRF_TOG_CHK(ch, PRF_NOGOSS);
        break;
    case SCMD_NOGRATZ:
        result = PRF_TOG_CHK(ch, PRF_NOGRATZ);
        break;
    case SCMD_NOWIZ:
        result = PRF_TOG_CHK(ch, PRF_NOWIZ);
        break;
    case SCMD_ROOMFLAGS:
        result = PRF_TOG_CHK(ch, PRF_ROOMFLAGS);
        break;
    case SCMD_HOLYLIGHT:
        result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
        break;
    case SCMD_SLOWNS:
        result = (nameserver_is_slow = !nameserver_is_slow);
        mudlog(MAX(LVL_IMMORT, GET_INVIS_LVL(ch)), NRM, true,
            "%s has turned name server %s", GET_NAME(ch),
            ONOFF(!nameserver_is_slow));
        break;
    case SCMD_AUTOEXIT:
        result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
        break;
    case SCMD_NOMUSIC:
        result = PRF_TOG_CHK(ch, PRF_NOMUSIC);
        break;
    case SCMD_NOSPEW:
        result = PRF_TOG_CHK(ch, PRF_NOSPEW);
        break;
    case SCMD_GAGMISS:
        result = PRF_TOG_CHK(ch, PRF_GAGMISS);
        break;
    case SCMD_AUTOPAGE:
        result = PRF2_TOG_CHK(ch, PRF2_AUTOPAGE);
        break;
    case SCMD_NOCLANSAY:
        result = PRF_TOG_CHK(ch, PRF_NOCLANSAY);
        break;
    case SCMD_DEBUG:
        result = PRF2_TOG_CHK(ch, PRF2_DEBUG);
        break;
    case SCMD_NEWBIE_HELP:
        result = PRF2_TOG_CHK(ch, PRF2_NEWBIE_HELPER);
        break;
    case SCMD_AUTO_DIAGNOSE:
        result = PRF2_TOG_CHK(ch, PRF2_AUTO_DIAGNOSE);
        break;
    case SCMD_NODREAM:
        result = PRF_TOG_CHK(ch, PRF_NODREAM);
        break;
    case SCMD_NOPROJECT:
        result = PRF_TOG_CHK(ch, PRF_NOPROJECT);
        break;
    case SCMD_HALT:
        result = PLR_TOG_CHK(ch, PLR_HALT);
        break;
    case SCMD_MORTALIZE:
        if (is_fighting(ch)) {
            send_to_char(ch, "You can't do this while fighting.\r\n");
            return;
        }
        result = PLR_TOG_CHK(ch, PLR_MORTALIZED);
        mudlog(GET_INVIS_LVL(ch), NRM, true,
            "(GC): %s has %smortalized at %d", GET_NAME(ch),
            PLR_FLAGGED(ch, PLR_MORTALIZED) ? "" : "im", ch->in_room->number);

        if (PLR_FLAGGED(ch, PLR_MORTALIZED)) {
            GET_MAX_HIT(ch) = (GET_LEVEL(ch) * 100) +
                dice(GET_LEVEL(ch), GET_CON(ch));
            GET_MAX_MOVE(ch) = (GET_LEVEL(ch) * 100);
            GET_MAX_MANA(ch) = (GET_LEVEL(ch) * 100) +
                dice(GET_LEVEL(ch), GET_WIS(ch));
            GET_HIT(ch) = GET_MAX_HIT(ch);
            GET_MANA(ch) = GET_MAX_MANA(ch);
            GET_MOVE(ch) = GET_MAX_MOVE(ch);
        }
        break;
    case SCMD_NOAFFECTS:
        result = PRF2_TOG_CHK(ch, PRF2_NOAFFECTS);
        break;

    case SCMD_NOHOLLER:
        result = PRF2_TOG_CHK(ch, PRF2_NOHOLLER);
        break;

    case SCMD_NOIMMCHAT:
        result = PRF2_TOG_CHK(ch, PRF2_NOIMMCHAT);
        break;

    case SCMD_CLAN_HIDE:
        result = PRF2_TOG_CHK(ch, PRF2_CLAN_HIDE);
        break;

    case SCMD_AUTOPROMPT:
        result = PRF2_TOG_CHK(ch, PRF2_AUTOPROMPT);
        break;

    case SCMD_NOWHO:
        result = PRF2_TOG_CHK(ch, PRF2_NOWHO);
        break;

    case SCMD_ANON:
        result = PRF2_TOG_CHK(ch, PRF2_ANONYMOUS);
        break;

    case SCMD_NOTRAILERS:
        result = PRF2_TOG_CHK(ch, PRF2_NOTRAILERS);
        break;

    case SCMD_LOGALL:
        TOGGLE_BIT(log_cmds, 1);
        sprintf(buf, "%s has toggled logall %s", GET_NAME(ch),
            ONOFF(log_cmds));
        mudlog(MAX(LVL_LOGALL, GET_INVIS_LVL(ch)), BRF, true, "%s", buf);
        send_to_char(ch, "%s\r\n", buf);
        return;
    case SCMD_JET_STREAM:
        TOGGLE_BIT(jet_stream_state, 1);
        sprintf(buf, "%s has toggled jet_stream_state %s", GET_NAME(ch),
            ONOFF(jet_stream_state));
        mudlog(GET_INVIS_LVL(ch), BRF, true, "%s", buf);
        send_to_char(ch, "%s\r\n", buf);
        return;

    case SCMD_WEATHER:
        weather_change();
        send_to_char(ch, "Weather change called.\r\n");
        return;

    case SCMD_AUTOSPLIT:
        result = PRF2_TOG_CHK(ch, PRF2_AUTOSPLIT);
        break;

    case SCMD_AUTOLOOT:
        result = PRF2_TOG_CHK(ch, PRF2_AUTOLOOT);
        break;

    case SCMD_PKILLER:
        if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
            send_to_char(ch, "Nothing happens.\r\n");
            return;
        }
        if (is_fighting(ch)) {
            send_to_char(ch,
                "You can't change your mind about playerkilling while you're fighting!\r\n");
            return;
        }
        result = PRF2_TOG_CHK(ch, PRF2_PKILLER);
        break;

    case SCMD_NOGECHO:
        result = PRF2_TOG_CHK(ch, PRF2_NOGECHO);
        break;
    case SCMD_AUTOWRAP:
        send_to_char(ch, "Disabled.\r\n");
        return;
    case SCMD_WORLDWRITE:
        if (!is_authorized(ch, WORLDWRITE, NULL)) {
            send_to_char(ch, "You do not have the worldwrite power!\n");
            return;
        }

        result = PRF2_TOG_CHK(ch, PRF2_WORLDWRITE);
        break;
    case SCMD_DEATHLOG:
        result = (LOG_DEATHS = !LOG_DEATHS);
        slog("%s has toggled jet_stream_state %s.", GET_NAME(ch),
            ONOFF(LOG_DEATHS));
        break;
    case SCMD_NOGUILDSAY:
        result = PRF2_TOG_CHK(ch, PRF2_NOGUILDSAY);
        break;
    case SCMD_NOPETITION:
        result = PRF_TOG_CHK(ch, PRF_NOPETITION);
        break;
    case SCMD_NASTY:
        result = PRF_TOG_CHK(ch, PRF_NASTY);
        break;
    case SCMD_NOHAGGLE:
        result = PRF_TOG_CHK(ch, PRF_NOHAGGLE);
        break;
    case SCMD_NOPLUG:
        result = PRF_TOG_CHK(ch, PRF_NOPLUG);
        break;
    case SCMD_METRIC:
        if (!ch->desc || !ch->desc->account) {
            send_to_char(ch, "You don't get to pick metric, mobile.\r\n");
            return;
        }
        result = !ch->desc->account->metric_units;
        account_set_metric(ch->desc->account, result);
        break;
    default:
        errlog("Unknown subcmd in do_gen_toggle");
        return;
    }

    if (result)
        send_to_char(ch, "%s", tog_messages[subcmd][TOG_ON]);
    else
        send_to_char(ch, "%s", tog_messages[subcmd][TOG_OFF]);

    return;
}

ACMD(do_afk)
{
    if (*argument) {
        // Afk with a reason always keeps afk
        skip_spaces(&argument);
        if (strlen(argument) > MAX_AFK_LENGTH) {
            send_to_char(ch,
                "Your afk reason must be limited to %d characters.\r\n",
                MAX_AFK_LENGTH);
            return;
        }

        free(AFK_REASON(ch));
        AFK_REASON(ch) = strdup(argument);
        g_list_free(AFK_NOTIFIES(ch));
        AFK_NOTIFIES(ch) = NULL;
        SET_BIT(PLR_FLAGS(ch), PLR_AFK);
        send_to_char(ch, "You are now afk: %s.\r\n", argument);
        act(tmp_sprintf("$n has gone away from the keyboard: %s", argument),
            false, ch, NULL, NULL, TO_ROOM);
    } else if (PLR_FLAGGED(ch, PLR_AFK)) {
        // Leaving afk
        free(AFK_REASON(ch));
        AFK_REASON(ch) = NULL;
        g_list_free(AFK_NOTIFIES(ch));
        AFK_NOTIFIES(ch) = NULL;
        REMOVE_BIT(PLR_FLAGS(ch), PLR_AFK);
        send_to_char(ch, "You have returned to the keyboard.\r\n");
        act("$n has returned to the keyboard", false, ch, NULL, NULL, TO_ROOM);
    } else {
        // Afk with no reason
        free(AFK_REASON(ch));
        AFK_REASON(ch) = NULL;
        g_list_free(AFK_NOTIFIES(ch));
        AFK_NOTIFIES(ch) = NULL;
        send_to_char(ch,
            "You are now afk.  When you move again, you will no longer be.\r\n");
        SET_BIT(PLR_FLAGS(ch), PLR_AFK);
        act("$n has gone away from the keyboard.", false, ch, NULL, NULL, TO_ROOM);
    }
}

ACMD(do_store)
{
    send_to_char(ch, "You can't do that here.\r\n");
    return;
}

ACMD(do_compare)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    struct obj_data *item1, *item2;
    int avg1, avg2;

    two_arguments(argument, arg1, arg2);

    if (!*arg1) {
        send_to_char(ch, "Usage: compare <item1> <item2>\r\n");
        return;
    }
    if (!(item1 = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
        send_to_char(ch, "You aren't carrying %s %s.\r\n", AN(arg1), arg1);
        return;
    }

    if (!*arg2) {
        send_to_char(ch, "Compare it to what?\r\n");
        return;
    }
    if (!(item2 = get_obj_in_list_vis(ch, arg2, ch->carrying))) {
        send_to_char(ch, "You aren't carrying %s %s.\r\n", AN(arg2), arg2);
        return;
    }
    if (GET_OBJ_COST(item2) == GET_OBJ_COST(item1))
        send_to_char(ch,
            "They appear to be of approximately equal value.\r\n");
    else {
        send_to_char(ch, "%s appears to be a bit more valuable.\r\n",
            GET_OBJ_COST(item1) > GET_OBJ_COST(item2) ?
            item1->name : item2->name);
    }
    if (GET_OBJ_MAX_DAM(item1) && GET_OBJ_MAX_DAM(item2)) {
        if (GET_OBJ_DAM(item1) * 100 / GET_OBJ_MAX_DAM(item1) >
            GET_OBJ_DAM(item2) * 100 / GET_OBJ_MAX_DAM(item2))
            act("$p appears to be in better condition than $P.",
                false, ch, item1, item2, TO_CHAR);
        if (GET_OBJ_DAM(item1) * 100 / GET_OBJ_MAX_DAM(item1) <
            GET_OBJ_DAM(item2) * 100 / GET_OBJ_MAX_DAM(item2))
            act("$P appears to be in better condition than $p.",
                false, ch, item1, item2, TO_CHAR);
        else if (GET_OBJ_MAX_DAM(item1) > GET_OBJ_MAX_DAM(item2))
            act("$p appears to be sturdier than $P.",
                false, ch, item1, item2, TO_CHAR);
        else
            act("$P appears to be sturdier than $p.",
                false, ch, item1, item2, TO_CHAR);
    }
    if (GET_OBJ_TYPE(item1) != GET_OBJ_TYPE(item2))
        return;

    switch (GET_OBJ_TYPE(item1)) {
    case ITEM_SCROLL:
    case ITEM_WAND:
    case ITEM_STAFF:
    case ITEM_POTION:
        if (!IS_MAGE(ch) || IS_DWARF(ch) || GET_INT(ch) < 30)
            break;
        send_to_char(ch, "%s seems to carry a greater aura of power.\r\n",
            GET_OBJ_VAL(item1, 0) >= GET_OBJ_VAL(item2, 0) ?
            item1->name : item2->name);
        break;
    case ITEM_WEAPON:
        avg1 =
            (int)(((GET_OBJ_VAL(item1, 2) + 1) / 2.0) * GET_OBJ_VAL(item1, 1));
        avg2 =
            (int)(((GET_OBJ_VAL(item2, 2) + 1) / 2.0) * GET_OBJ_VAL(item2, 1));
        if (avg1 == avg2)
            send_to_char(ch,
                "Both weapons appear to have similar damage potential.\r\n");
        else {
            send_to_char(ch, "%s appears to be a more formidable weapon.\r\n",
                avg1 >= avg2 ? item1->name : item2->name);
        }
        break;
    case ITEM_ARMOR:
        avg1 = GET_OBJ_VAL(item1, 0);
        avg2 = GET_OBJ_VAL(item2, 0);
        if (avg1 == avg2)
            send_to_char(ch,
                "They both seem to be of about equal quality.\r\n");
        else {
            send_to_char(ch, "%s appears to have more protective quality.\r\n",
                avg1 >= avg2 ? item1->name : item2->name);
        }
        break;
    case ITEM_CONTAINER:
    case ITEM_DRINKCON:
        avg1 = GET_OBJ_VAL(item1, 0);
        avg2 = GET_OBJ_VAL(item2, 0);
        if (avg1 == avg2)
            send_to_char(ch,
                "Looks like they will each hold about the same amount.\r\n");
        else {
            send_to_char(ch, "%s looks like it will hold more.\r\n",
                avg1 >= avg2 ? item1->name : item2->name);
        }
        break;
    case ITEM_FOOD:
        avg1 = GET_OBJ_VAL(item1, 0);
        avg2 = GET_OBJ_VAL(item2, 0);
        if (avg1 == avg2)
            send_to_char(ch, "They both look equally tasty.\r\n");
        else {
            send_to_char(ch, "%s looks more filling that the other.\r\n",
                avg1 >= avg2 ? item1->name : item2->name);
        }
        break;
    case ITEM_HOLY_SYMB:
        if (GET_OBJ_VAL(item1, 0) != GET_OBJ_VAL(item2, 0)) {
            send_to_char(ch,
                "They appear to be symbols of different deities.\r\n");
            break;
        }
        if (!IS_KNIGHT(ch) && !IS_CLERIC(ch))
            break;
        avg1 = GET_OBJ_VAL(item1, 2);
        avg2 = GET_OBJ_VAL(item2, 2);
        if (avg1 == avg2)
            send_to_char(ch,
                "They appear to have the same minimum level.\r\n");
        else {
            send_to_char(ch,
                "%s looks like it has a higher minimum level.\r\n",
                avg1 >= avg2 ? item1->name : item2->name);
        }
        avg1 = GET_OBJ_VAL(item1, 3);
        avg2 = GET_OBJ_VAL(item2, 3);
        if (avg1 == avg2)
            send_to_char(ch,
                "They appear to have the same maximum level.\r\n");
        else {
            send_to_char(ch, "%s seems to support greater power.\r\n",
                avg1 >= avg2 ? item1->name : item2->name);
        }
        break;
    case ITEM_BATTERY:
        avg1 = GET_OBJ_VAL(item1, 0);
        avg2 = GET_OBJ_VAL(item2, 0);
        if (avg1 == avg2)
            send_to_char(ch,
                "They appear to be able to store the same amount of energy.\r\n");
        else {
            send_to_char(ch, "%s looks like it will hold more energy.\r\n",
                avg1 >= avg2 ? item1->name : item2->name);
        }
        break;
    case ITEM_ENERGY_GUN:
        avg1 = GET_OBJ_VAL(item1, 0);
        avg2 = GET_OBJ_VAL(item2, 0);
        if (avg1 == avg2)
            send_to_char(ch,
                "They appear to be able to store the same amount of energy.\r\n");
        else {
            send_to_char(ch, "%s looks like it will hold more energy.\r\n",
                avg1 >= avg2 ? item1->name : item2->name);
        }
        avg1 = GET_OBJ_VAL(item1, 2);
        avg2 = GET_OBJ_VAL(item2, 2);
        if (avg1 == avg2)
            send_to_char(ch,
                "They can both discharge energy at comparable rates.\r\n");
        else {
            send_to_char(ch, "%s can release energy at a greater rate.\r\n",
                avg1 >= avg2 ? item1->name : item2->name);
        }
        break;
    default:
        send_to_char(ch,
            "You can see nothing else special about either one.\r\n");
        break;
    }
}

ACMD(do_screen)
{
    if (ch->desc == NULL || ch->desc->account == NULL) {
        send_to_char(ch,
            "You don't have an account to put the screen in.\r\n");
    }

    skip_spaces(&argument);
    if (!*argument) {
        // Handle the no argument option
        send_to_char(ch, "Your current screen length is: %d lines.\r\n",
            ch->desc->account->term_height);
        return;
    }

    char *height_str = tmp_getword(&argument);
    char *width_str = tmp_getword(&argument);

    // Validate the arguments
    if ((!is_number(height_str) && strcmp(height_str, "off"))
        || (*width_str && !is_number(width_str) && strcmp(width_str, "off"))) {
        send_to_char(ch,
            "Usage: screen (<terminal height>|off) [(<terminal width>|off)]\r\n");
        return;
    }

    int new_height = 0;
    if (strcmp(height_str, "off")) {
        new_height = atoi(height_str);
        if (new_height < 0 || new_height > 200)
            new_height = 0;
    }

    int new_width = 0;
    if (*width_str && strcmp(width_str, "off")) {
        new_width = atoi(width_str);
        if (new_width < 0 || new_width > 200)
            new_width = 0;
    }
    // Now change the validated values
    if (*height_str) {
        account_set_term_height(ch->account, new_height);
        if (GET_PAGE_LENGTH(ch)) {
            send_to_char(ch, "Your screen length will now be %d lines.\r\n",
                GET_PAGE_LENGTH(ch));
        } else {
            send_to_char(ch,
                "Long descriptions will no longer be paged for you.\r\n");
        }
    }
    if (*width_str) {
        account_set_term_width(ch->account, new_width);
        if (GET_PAGE_WIDTH(ch)) {
            send_to_char(ch,
                "Your screen width will now be %d characters.\r\n",
                GET_PAGE_WIDTH(ch));
        } else {
            send_to_char(ch,
                "Line overruns will no longer be calculated when paging large texts.\r\n");
        }
    }
}

ACMD(do_throw)
{
    struct obj_data *obj = NULL, *target_obj = NULL;
    struct creature *vict = NULL, *target_vict = NULL;
    char arg1[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int dir = 0, victim_ac, diceroll, calc_thaco;
    struct room_data *r_toroom = NULL;

    argument = one_argument(argument, arg1);
    if (!*arg1)
        send_to_char(ch, "Throw what at what?\r\n");
    else if (!(obj = get_object_in_equip_pos(ch, arg1, WEAR_WIELD)) &&
        !(obj = get_object_in_equip_pos(ch, arg1, WEAR_HOLD)) &&
        !(obj = get_obj_in_list_vis(ch, arg1, ch->carrying)) &&
        !(vict = get_char_room_vis(ch, arg1)))
        send_to_char(ch, "You can't find that to throw it.\r\n");
    else if (obj) {
        if (IS_OBJ_STAT(obj, ITEM_NODROP) && GET_LEVEL(ch) < LVL_ETERNAL) {
            act("Arrrgh!  $p won't come off of your hand!", false, ch, obj, NULL,
                TO_CHAR);
            act("$n struggles with $p for a moment.", false, ch, obj, NULL,
                TO_ROOM);
            return;
        }
        argument = one_argument(argument, arg1);
        if (!*arg1) {
            if (obj->worn_by)
                obj_to_char(unequip_char(ch, obj->worn_on, EQUIP_WORN), ch);

            act("$n throws $p across the room.", false, ch, obj, NULL, TO_ROOM);
            act("You throw $p across the room.", false, ch, obj, NULL, TO_CHAR);
            obj_from_char(obj);
            obj_to_room(obj, ch->in_room);
            return;
        }

        if (is_abbrev(arg1, "northward"))
            dir = 1;
        else if (is_abbrev(arg1, "eastward"))
            dir = 2;
        else if (is_abbrev(arg1, "southward"))
            dir = 3;
        else if (is_abbrev(arg1, "westward"))
            dir = 4;
        else if (is_abbrev(arg1, "upward"))
            dir = 5;
        else if (is_abbrev(arg1, "downward"))
            dir = 6;
        else if (is_abbrev(arg1, "futureward"))
            dir = 7;
        else if (is_abbrev(arg1, "pastward"))
            dir = 8;
        else if (!(target_vict = get_char_room_vis(ch, arg1)) &&
            !(target_obj =
                get_obj_in_list_vis(ch, arg1, ch->in_room->contents))) {
            send_to_char(ch, "Throw at what??\r\n");
            return;
        }

        if (dir) {
            if (!ch->in_room->dir_option[dir - 1] ||
                (r_toroom =
                    ch->in_room->dir_option[dir - 1]->to_room) == NULL) {
                send_to_char(ch, "You cannot throw anything to the %s.\r\n",
                    dirs[(int)(dir - 1)]);
                return;
            }
            if (IS_SET(EXIT(ch, dir - 1)->exit_info, EX_CLOSED)) {
                if (obj->worn_by)
                    obj_to_char(unequip_char(ch, obj->worn_on, EQUIP_WORN), ch);

                sprintf(buf, "$n throws $p %sward against the closed %s.",
                    dirs[(int)(dir - 1)], EXIT(ch, dir - 1)->keyword ?
                    fname(EXIT(ch, dir - 1)->keyword) : "door");
                act(buf, false, ch, obj, NULL, TO_ROOM);
                sprintf(buf, "You throw $p %sward against the closed %s.",
                    dirs[(int)(dir - 1)], EXIT(ch, dir - 1)->keyword ?
                    fname(EXIT(ch, dir - 1)->keyword) : "door");
                act(buf, false, ch, obj, NULL, TO_CHAR);
                obj_from_char(obj);
                obj_to_room(obj, ch->in_room);
                return;
            }
            argument = one_argument(argument, arg1);
            if (*arg1 &&
                !(target_vict =
                    get_char_in_remote_room_vis(ch, arg1, r_toroom))
                && !(target_obj =
                    get_obj_in_list_vis(ch, arg1, r_toroom->contents))) {
                send_to_char(ch, "No such target in that direction.\r\n");
                return;
            }
            if (obj->worn_by)
                obj_to_char(unequip_char(ch, obj->worn_on, EQUIP_WORN), ch);

            sprintf(buf, "$n throws $p %sward.", dirs[(int)(dir - 1)]);
            act(buf, false, ch, obj, NULL, TO_ROOM);
            sprintf(buf, "You throw $p %sward.", dirs[(int)(dir - 1)]);
            act(buf, false, ch, obj, NULL, TO_CHAR);
            obj_from_char(obj);
            obj_to_room(obj, r_toroom);
            if (target_vict) {
                sprintf(buf, "$p flies in from %s and hits you in the head!",
                    from_dirs[(int)(dir - 1)]);
                act(buf, false, NULL, obj, target_vict, TO_VICT);
                sprintf(buf, "$p flies in from %s and hits $N in the head!",
                    from_dirs[(int)(dir - 1)]);
                act(buf, false, NULL, obj, target_vict, TO_NOTVICT);
            } else if (target_obj) {
                sprintf(buf, "$p flies in from %s and slams into %s!",
                    from_dirs[(int)(dir - 1)], target_obj->name);
                act(buf, false, NULL, obj, target_vict, TO_ROOM);
            } else {
                sprintf(buf, "$p flies in from %s and lands by your feet.",
                    from_dirs[(int)(dir - 1)]);
                act(buf, false, NULL, obj, NULL, TO_ROOM);
            }
            return;
        }

        if (target_vict) {
            if (obj->worn_by)
                obj_to_char(unequip_char(ch, obj->worn_on, EQUIP_WORN), ch);

            if (target_vict == ch) {
                send_to_char(ch, "You can't throw something at yourself!\r\n");
                return;
            }
            act("$n throws $p at $N!", false, ch, obj, target_vict,
                TO_NOTVICT);
            act("$n throws $p at you!", false, ch, obj, target_vict, TO_VICT);
            act("You throw $p at $N.", false, ch, obj, target_vict, TO_CHAR);

            calc_thaco = calculate_thaco(ch, target_vict, NULL) + 10;
            calc_thaco -= (CHECK_SKILL(ch, SKILL_THROWING) / 10);
            if (!IS_OBJ_STAT2(obj, ITEM2_THROWN_WEAPON))
                calc_thaco += 2 * GET_OBJ_WEIGHT(obj);

            victim_ac = GET_AC(target_vict) / 10;
            victim_ac = MAX(-10, victim_ac);    /* -10 is lowest */
            if (AWAKE(target_vict))
                victim_ac += dexterity_defense_bonus(GET_DEX(target_vict));
            diceroll = number(1, 20);
            if ((diceroll < 20 && AWAKE(target_vict)) &&
                ((diceroll == 1) || ((calc_thaco - diceroll) > victim_ac))) {
                // Missed throw
                if (IS_POTION(obj)) {
                    act("$p narrowly misses you and shatters on the ground!",
                        false, ch, obj, target_vict, TO_VICT);
                    act("$p narrowly misses $M and shatters on the ground!",
                        false, ch, obj, target_vict, TO_NOTVICT);
                    act("$p narrowly misses $M and shatters on the ground!",
                        false, ch, obj, target_vict, TO_CHAR);
                    extract_obj(obj);
                } else {
                    act("$p narrowly misses you!", false, ch, obj,
                        target_vict, TO_VICT);
                    act("$p narrowly misses $M!", false, ch, obj,
                        target_vict, TO_NOTVICT);
                    act("$p narrowly misses $M!", false, ch, obj,
                        target_vict, TO_CHAR);
                    obj_from_char(obj);
                    obj_to_room(obj, ch->in_room);
                }
            } else if (IS_OBJ_TYPE(obj, ITEM_WEAPON)
                && ok_to_attack(ch, target_vict, false)) {
                // Hit throw with a weapon
                damage(ch, target_vict, obj,
                       dice(GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2)) +
                       strength_damage_bonus(GET_STR(ch)),
                       GET_OBJ_VAL(obj, 3) + TYPE_HIT,
                       number(0, NUM_WEARS - 1));
            } else if (IS_OBJ_TYPE(obj, ITEM_POTION)) {
                // Hit throw with a potion
                act("$p hits $M in the head and shatters!",
                    false, ch, obj, target_vict, TO_CHAR);
                act("$p hits $N in the head and shatters!",
                    false, ch, obj, target_vict, TO_NOTVICT);
                act("$p hits you in the head and shatters!",
                    false, ch, obj, target_vict, TO_VICT);
                // TODO: Copied from mag_objectmagic... Needs to be refactored
                if (!ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
                    for (int i = 1; i < 4; i++) {
                        call_magic(ch, target_vict,
                            NULL, NULL, GET_OBJ_VAL(obj, i),
                            MAX(1, GET_OBJ_VAL(obj, 0) / 4),
                            CAST_POTION);
                        if (is_dead(ch) || is_dead(target_vict))
                            break;
                    }
                }
                extract_obj(obj);
            } else {
                act("$p hits $M in the head!",
                    false, ch, obj, target_vict, TO_CHAR);
                act("$p hits $N in the head!",
                    false, ch, obj, target_vict, TO_NOTVICT);
                act("$p hits you in the head!",
                    false, ch, obj, target_vict, TO_VICT);
                obj_from_char(obj);
                obj_to_room(obj, ch->in_room);
            }

            gain_skill_prof(ch, SKILL_THROWING);
            WAIT_STATE(ch, PULSE_VIOLENCE * 1);
            if (CHECK_SKILL(ch, SKILL_THROWING) < 90)
                WAIT_STATE(ch, PULSE_VIOLENCE * 1);
            if (CHECK_SKILL(ch, SKILL_THROWING) < 60)
                WAIT_STATE(ch, PULSE_VIOLENCE * 1);

            return;
        } else if (target_obj) {
            if (obj->worn_by)
                obj_to_char(unequip_char(ch, obj->worn_on, EQUIP_WORN), ch);
            sprintf(buf, "$n hurls $p up against %s with brute force!",
                target_obj->name);
            act(buf, false, ch, obj, NULL, TO_ROOM);
            sprintf(buf, "You hurl $p up against %s with brute force!",
                target_obj->name);
            act(buf, false, ch, obj, NULL, TO_CHAR);
        }
        obj_from_char(obj);
        obj_to_room(obj, ch->in_room);
    } else if (vict) {
        send_to_char(ch, "You cannot throw people.\r\n");
    }
}

ACMD(do_feed)
{
    struct creature *vict = NULL;
    struct obj_data *food = NULL;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    half_chop(argument, arg1, arg2);

    if (!*arg1 || !*arg2)
        send_to_char(ch, "Feed who to what?\r\n");
    else if (!(food = get_obj_in_list_vis(ch, arg1, ch->carrying)))
        send_to_char(ch, "You don't have that food.\r\n");
    else if (!((vict = MOUNTED_BY(ch)) &&
            isname(arg2, vict->player.name)) &&
        !(vict = get_char_room_vis(ch, arg2)))
        send_to_char(ch, "No-one around by that name.\r\n");
    else if (GET_OBJ_TYPE(food) != ITEM_FOOD)
        act("I don't think anyone wants to eat $p.",
            false, ch, food, NULL, TO_CHAR);
    else if (!NPC2_FLAGGED(vict, NPC2_MOUNT))
        act("You cannot feed $p to $N.", false, ch, food, vict, TO_CHAR);
    else if (!AWAKE(vict))
        act("$N is in no position to be eating.", false, ch, food, vict,
            TO_CHAR);
    else if (GET_MOVE(vict) >= GET_MAX_MOVE(vict))
        act("$N must not be hungry -- $E refuses $p.", false, ch, food,
            vict, TO_CHAR);
    else {
        act("$n feeds $p to $N.", false, ch, food, vict, TO_NOTVICT);
        act("$N devours $p.", false, ch, food, vict, TO_CHAR);
        GET_MOVE(vict) = MIN(GET_MAX_MOVE(vict), GET_MOVE(vict) +
            GET_OBJ_VAL(food, 0));
        extract_obj(food);
        return;
    }
}

ACMD(do_weigh)
{
    struct obj_data *obj = NULL;

    skip_spaces(&argument);

    if (!*argument) {
        send_to_char(ch, "Weigh what?\r\n");
        return;
    }

    if (!(obj = get_obj_in_list_vis(ch, argument, ch->carrying))) {
        send_to_char(ch,
            "You can only weigh things which you are carrying.\r\n");
        return;
    }

    act("You carefully judge the weight of $p.", false, ch, obj, NULL, TO_CHAR);
    act("$n gauges the weight of $p by tossing it in one hand.",
        true, ch, obj, NULL, TO_ROOM);

    send_to_char(ch, "It seems to weigh about %s.\r\n",
                 format_weight(GET_OBJ_WEIGHT(obj) +
                               number(-(GET_OBJ_WEIGHT(obj) / GET_INT(ch)),
                                      GET_OBJ_WEIGHT(obj) / GET_INT(ch)),
                               USE_METRIC(ch)));
}

ACMD(do_knock)
{
    struct obj_data *obj = NULL;
    int dir;
    char dname[128], arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    struct room_data *other_room = NULL;
    struct creature *vict = NULL;

    // Do some basic parsing
    skip_spaces(&argument);
    half_chop(argument, arg1, arg2);
    if (!*arg1) {
        send_to_char(ch, "Knock on what?\r\n");
        return;
    }
    // Check to see if we should knock on someone's head first
    if ((vict = get_char_room_vis(ch, arg1))) {
        if (vict == ch) {
            send_to_char(ch, "You rap your knuckles on your head.\r\n");
            act("$n raps $s knuckles on $s head.", true, ch, NULL, ch, TO_ROOM);
            return;
        }
        act("You knock on $N's skull.  Anybody home?", false, ch, NULL, vict,
            TO_CHAR);
        act("$n knocks on your skull.  Anybody home?", false, ch, NULL, vict,
            TO_VICT | TO_SLEEP);
        act("$n knocks on $N's skull.  Anybody home?", false, ch, NULL, vict,
            TO_NOTVICT);
        if (GET_POSITION(vict) == POS_SLEEPING
            && !AFF_FLAGGED(vict, AFF_SLEEP))
            GET_POSITION(vict) = POS_RESTING;
        return;
    }
    // Ok, not someone's head.  Maybe an object?
    obj = get_obj_in_list_vis(ch, argument, ch->carrying);
    if (!obj)
        obj = get_obj_in_list_vis(ch, argument, ch->in_room->contents);

    if (obj) {
        act("$n knocks on $p.", false, ch, obj, NULL, TO_ROOM);
        act("You knock on $p.", false, ch, obj, NULL, TO_CHAR);

        if (IS_VEHICLE(obj)) {
            if ((other_room = real_room(ROOM_NUMBER(obj)))
                && other_room->people) {
                act("$N knocks on the outside of $p.", false,
                    other_room->people->data, obj, ch, TO_NOTVICT);
                act("$N knocks on the outside of $p.", false,
                    other_room->people->data, obj, ch, TO_CHAR);
            }
        }
        return;
    }
    // Ok... not an object.  Maybe a door?
    if ((dir = find_door(ch, arg1, arg2, "knock on")) >= 0) {
        if (EXIT(ch, dir)->keyword)
            strcpy(dname, fname(EXIT(ch, dir)->keyword));
        else
            strcpy(dname, "door");

        sprintf(buf, "$n knocks on the %s.", dname);
        act(buf, false, ch, NULL, NULL, TO_ROOM);
        send_to_char(ch, "You knock on the %s.\r\n", dname);

        if ((other_room = EXIT(ch, dir)->to_room) &&
            other_room->dir_option[rev_dir[dir]] &&
            other_room->dir_option[rev_dir[dir]]->to_room == ch->in_room &&
            other_room->people) {
            if (other_room->dir_option[rev_dir[dir]]->keyword)
                strcpy(dname,
                    fname(other_room->dir_option[rev_dir[dir]]->keyword));
            else
                strcpy(dname, "door");

            sprintf(buf, "Someone knocks on the %s from the other side.",
                dname);
            act(buf, false, other_room->people->data, NULL, NULL,
                TO_CHAR | TO_SLEEP);
            act(buf, false, other_room->people->data, NULL, NULL,
                TO_ROOM | TO_SLEEP);
        }
        return;
    }
    // They're hallucinating something to knock on, but find_door has already
    // told them that.
}

ACMD(do_loadroom)
{

    struct room_data *room = NULL;

    skip_spaces(&argument);

    if (!*argument) {

        if (GET_HOMEROOM(ch) && (room = real_room(GET_HOMEROOM(ch)))) {
            send_to_char(ch, "Your loadroom is currently set to: %s%s%s\r\n"
                "Type 'loadroom off' to remove it or 'loadroom set' to set it.\r\n",
                CCCYN(ch, C_NRM), room->name, CCNRM(ch, C_NRM));
        } else
            send_to_char(ch, "Your loadroom is currently default.\r\n");

        return;
    }

    if (is_abbrev(argument, "off")) {

        GET_HOMEROOM(ch) = 0;
        send_to_char(ch, "Loadroom disabled.\r\n");

    } else if (is_abbrev(argument, "set")) {

        if ((ROOM_FLAGGED(ch->in_room,
                    ROOM_HOUSE | ROOM_CLAN_HOUSE | ROOM_COMFORT)
                || GET_LEVEL(ch) >= LVL_AMBASSADOR)
            && can_enter_house(ch, ch->in_room->number)
            && clan_house_can_enter(ch, ch->in_room)) {
            GET_HOMEROOM(ch) = ch->in_room->number;
            send_to_char(ch, "Okay, you will now load in this room.\r\n");
        } else
            send_to_char(ch, "You cannot load here.\r\n");

    } else
        send_to_char(ch, "Usage: loadroom [off | set]\r\n");

}

ACMD(do_gasify)
{
    struct creature *gas = NULL;
    struct room_data *tank = real_room(13);

    if (!ch->desc)
        return;

    if (ch->desc->original) {
        send_to_char(ch, "You're already switched.\r\n");
        return;
    }

    if (!(gas = read_mobile(1518)) || !tank) {
        send_to_char(ch, "Nope.\r\n");
        return;
    }

    IS_CARRYING_N(gas) = 200;

    act("$n slowly fades away, leaving only a gaseous cloud behind.",
        true, ch, NULL, NULL, TO_ROOM);
    send_to_char(ch, "You become gaseous.\r\n");

    char_to_room(gas, ch->in_room, false);
    char_from_room(ch, false);
    char_to_room(ch, tank, false);

    ch->desc->creature = gas;
    ch->desc->original = ch;

    gas->desc = ch->desc;
    ch->desc = NULL;

}

ACMD(do_clean)
{
    struct obj_data *obj = NULL;
    struct creature *vict = NULL;
    int i, j, k, found, pos;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    argument = two_arguments(argument, arg1, arg2);

    if (!*arg1) {
        send_to_char(ch, "Clean who or what?\r\n");
        return;
    }

    if (!(vict = get_char_room_vis(ch, arg1)) &&
        !(obj = get_object_in_equip_vis(ch, arg1, ch->equipment, &i)) &&
        !(obj = get_obj_in_list_vis(ch, arg1, ch->carrying)) &&
        !(obj = get_obj_in_list_vis(ch, arg1, ch->in_room->contents))) {
        send_to_char(ch, "You can't find any '%s' here.\r\n", arg1);
        return;
    }

    if (vict) {

        if (!*arg2) {
            send_to_char(ch, "Clean what position?\r\n");
            return;
        }
        if ((pos = search_block(arg2, wear_eqpos, false)) < 0) {
            strcpy(buf, "Invalid position.  Valid positions are:\r\n");
            for (i = 0, j = 0; i < NUM_WEARS; i++) {
                if (ILLEGAL_SOILPOS(i))
                    continue;
                j++;
                strcat(buf, wear_eqpos[i]);
                if (i < WEAR_EAR_R)
                    strcat(buf, ", ");
                else
                    strcat(buf, ".\r\n");
            }
            send_to_char(ch, "%s", buf);
            return;
        }
        if (ILLEGAL_SOILPOS(pos)) {
            send_to_char(ch, "Hrm.  You can't clean anybody's '%s'!\r\n",
                wear_description[pos]);
            return;
        }
        if (!CHAR_SOILAGE(vict, pos)) {
            sprintf(buf, "%s not soiled there.",
                ch == vict ? "You are" : "$E is");
            act(buf, false, ch, NULL, vict, TO_CHAR);
            return;
        }

        if (vict == ch) {
            sprintf(buf, "$n carefully cleans $s %s.", wear_description[pos]);
            act(buf, true, ch, NULL, NULL, TO_ROOM);
            send_to_char(ch, "You carefully clean your %s.\r\n",
                wear_description[pos]);
        } else {
            sprintf(buf, "$n carefully cleans $N's %s.",
                wear_description[pos]);
            act(buf, true, ch, NULL, vict, TO_NOTVICT);
            sprintf(buf, "You carefully clean $N's %s.",
                wear_description[pos]);
            act(buf, false, ch, NULL, vict, TO_CHAR);
            sprintf(buf, "$n carefully cleans your %s.",
                wear_description[pos]);
            act(buf, false, ch, NULL, vict, TO_VICT);
        }
        found = 0;
        strcpy(buf, "no longer ");

        for (k = 0, j = 0; j < 16; j++)
            if (CHAR_SOILED(vict, pos, (1 << j)))
                k++;

        for (j = 0; j < 16; j++) {
            if (CHAR_SOILED(vict, pos, (1 << j))) {
                found++;
                if (found > 1) {
                    strcat(buf, ", ");
                    if (found == k)
                        strcat(buf, "or ");
                }
                strcat(buf, soilage_bits[j]);
            }
        }
        sprintf(buf2, "Your %s %s %s.", wear_description[pos],
            pos == WEAR_FEET ? "are" : ISARE(wear_description[pos]), buf);
        act(buf2, false, ch, obj, vict, TO_VICT);
        sprintf(buf2, "$N's %s %s %s.", wear_description[pos],
            pos == WEAR_FEET ? "are" : ISARE(wear_description[pos]), buf);
        act(buf2, true, ch, obj, vict, TO_NOTVICT);
        if (ch != vict)
            act(buf2, true, ch, obj, vict, TO_CHAR);

        CHAR_SOILAGE(vict, pos) = 0;
        return;
    }

    /* obj */

    if (!OBJ_SOILAGE(obj))
        act("$p is not soiled.", false, ch, obj, NULL, TO_CHAR);
    else {
        sprintf(buf, "$n carefully cleans $p. (%s)",
            obj->carried_by ? "carried" : obj->worn_by ? "worn" : "here");
        act(buf, true, ch, obj, NULL, TO_ROOM);
        sprintf(buf, "You carefully clean $p. (%s)",
            obj->carried_by ? "carried" : obj->worn_by ? "worn" : "here");
        act(buf, false, ch, obj, NULL, TO_CHAR);

        found = 0;
        strcpy(buf, "$p is no longer ");

        for (k = 0, j = 0; j < 17; j++)
            if (OBJ_SOILED(obj, (1 << j)))
                k++;

        for (j = 0; j < 17; j++) {
            if (OBJ_SOILED(obj, (1 << j))) {
                found++;
                if (found > 1) {
                    strcat(buf, ", ");
                    if (found == k)
                        strcat(buf, "or ");
                }
                strcat(buf, soilage_bits[j]);
            }
        }
        strcat(buf, ".");
        act(buf, false, ch, obj, NULL, TO_CHAR);
        act(buf, false, ch, obj, NULL, TO_ROOM);
        OBJ_SOILAGE(obj) = 0;
    }
}

ACMD(do_drag)
{
    struct creature *found_char;
    struct obj_data *found_obj;
    char *args = argument;
    char *target_str = tmp_getword(&args);

    int bits;

    bits = generic_find(target_str, FIND_OBJ_ROOM | FIND_CHAR_ROOM, ch,
        &found_char, &found_obj);

    //Target is a character
    if (!bits) {
        send_to_char(ch, "What do you want to drag?\r\n");
        return;
    } else if (found_char) {
        do_drag_char(ch, argument, 0, 0);
        return;
    } else if (found_obj) {
        drag_object(ch, found_obj, argument);
        return;
    }

}

void
ice_room(struct room_data *room, int amount)
{
    struct obj_data *ice;
    int new_ice = false;

    if ((!(room)) || amount <= 0) {
        return;
    }

    for (ice = room->contents; ice; ice = ice->next_content) {
        if (GET_OBJ_VNUM(ice) == ICE_VNUM) {
            break;
        }
    }

    if (!ice && (new_ice = true) && !(ice = read_object(ICE_VNUM))) {
        errlog("Unable to load ice.");
        return;
    }

    if (GET_OBJ_TIMER(ice) > 50) {
        return;
    }

    GET_OBJ_TIMER(ice) += amount;

    if (new_ice) {
        obj_to_room(ice, room);
    }
}

ACMD(do_trust)
{
    long idnum;

    skip_spaces(&argument);

    if (IS_NPC(ch)) {
        send_to_char(ch, "Mobiles don't trust anyone.\r\n");
        return;
    }

    if (!*argument) {
        if (!ch->account->trusted) {
            send_to_char(ch, "You don't trust anyone right now.\r\n");
            return;
        }
        send_to_char(ch, "Trusted friends:\r\n");
        account_display_trusted(ch->account, ch);
        return;
    }

    if (!player_name_exists(argument)) {
        send_to_char(ch, "That player doesn't exist.\r\n");
        return;
    }

    idnum = player_idnum_by_name(argument);

    if (idnum == GET_IDNUM(ch)) {
        send_to_char(ch, "Don't you already trust yourself?\r\n");
        return;
    }

    if (ch->account->id == player_account_by_idnum(idnum)) {
        send_to_char(ch,
            "You automatically trust characters in your own account.\r\n");
        return;
    }

    if (creature_trusts_idnum(ch, idnum)) {
        send_to_char(ch, "You already trust %s.\r\n",
            player_name_by_idnum(idnum));
        return;
    }

    account_trust(ch->account, idnum);
    send_to_char(ch, "%s is now a trusted friend to you.\r\n",
        player_name_by_idnum(idnum));
}

ACMD(do_distrust)
{
    long idnum;

    if (IS_NPC(ch)) {
        send_to_char(ch, "Mobiles distrust everyone already!\r\n");
        return;
    }

    skip_spaces(&argument);

    if (!*argument) {
        send_to_char(ch, "You must specify who to distrust.\r\n");
        return;
    }

    if (!player_name_exists(argument)) {
        send_to_char(ch, "That player doesn't exist.\r\n");
        return;
    }

    idnum = player_idnum_by_name(argument);
    if (idnum == GET_IDNUM(ch)) {
        send_to_char(ch,
            "If you can't trust yourself, who can you trust?\r\n");
        return;
    }

    if (ch->account->id == player_account_by_idnum(idnum)) {
        send_to_char(ch,
            "You can't distrust characters in your own account.\r\n");
        return;
    }

    if (creature_distrusts_idnum(ch, idnum)) {
        send_to_char(ch, "You already don't trust %s.\r\n",
            player_name_by_idnum(idnum));
        return;
    }

    account_distrust(ch->account, idnum);
    send_to_char(ch, "You are no longer trusting of %s.\r\n",
        player_name_by_idnum(idnum));
}
