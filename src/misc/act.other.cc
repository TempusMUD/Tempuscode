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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "house.h"
#include "vehicle.h"
#include "materials.h"
#include "smokes.h"
#include "clan.h"
#include "constants.h"
#include "fight.h"
#include "char_class.h"

/* extern variables */
extern struct descriptor_data *descriptor_list;
extern struct spell_info_type spell_info[];
extern struct obj_data *object_list;
extern struct obj_shared_data *null_obj_shared;
extern int log_cmds;
extern int jet_stream_state;

extern int check_mob_reaction(struct char_data *ch, struct char_data *vict);
void gain_condition(struct char_data *ch, int condition, int value);
void look_at_target(struct char_data *ch, char *arg);
int find_door(struct char_data *ch, char *type, char *dir, const char *cmdname);
void enable_vt100(struct char_data *ch);
void disable_vt100(struct char_data *ch);
void weather_change(void);
void Crash_rentsave(struct char_data * ch, int cost, int rentcode);
int Crash_rentcost(struct char_data *ch, int display, int factor);
void Crash_cursesave( struct char_data * ch );
int drag_object(CHAR* ch, struct obj_data *obj, char* argument );
void ice_room(struct room_data *room, int amount);
ACMD(do_drag_char);

ACMD(do_quit)
{
    extern int free_rent;
    struct descriptor_data *d, *next_d;
    struct room_data * save_room = NULL;
    int cost = 0;
    skip_spaces(&argument);

    if (IS_NPC(ch) || !ch->desc) {
	send_to_char("No!  You're trapped!  Muhahahahahah!\r\n", ch);
	return;
    }

    if PLR_FLAGGED(ch, PLR_AFK)
		      REMOVE_BIT(PLR_FLAGS(ch), PLR_AFK);
  
    if (PLR_FLAGGED(ch, PLR_KILLER | PLR_THIEF)) {
	send_to_char("Outlaws cannot quit.\r\n", ch);
	return;
    }

    if (subcmd != SCMD_QUIT && GET_LEVEL(ch) < LVL_AMBASSADOR)
	send_to_char("You have to type quit - no less, to quit!\r\n", ch);
    else if (ch->getPosition() == POS_FIGHTING)
	send_to_char("No way!  You're fighting for your life!\r\n", ch);
    else if (ch->getPosition() < POS_STUNNED) {
	send_to_char("You die before your time...\r\n", ch);
	die(ch, 0, 0, 0);
    } else if (PLR_FLAGGED(ch, PLR_QUESTOR)) {
	send_to_char("Please remove your questor flag first.\r\n", ch);
    } else {

	/*
	 * kill off all sockets connected to the same player as the one who is
	 * trying to quit.  Helps to maintain sanity as well as prevent duping.
	 */
	for (d = descriptor_list; d; d = next_d) {
	    next_d = d->next;
	    if (d == ch->desc)
		continue;
	    if (d->character && (GET_IDNUM(d->character) == GET_IDNUM(ch)))
		close_socket(d);
	}

	if ((free_rent) || GET_LEVEL(ch) >= LVL_AMBASSADOR || 
	    PLR_FLAGGED(ch, PLR_TESTER)) {
	    if (GET_LEVEL(ch) >= LVL_AMBASSADOR) {
		sprintf(buf, "%s has departed from the known multiverse.", GET_NAME(ch));
		mudlog(buf, NRM, MAX(LVL_AMBASSADOR, GET_INVIS_LEV(ch)), TRUE);
		act("$n steps out of the universe.", TRUE, ch, 0, 0, TO_ROOM);
		send_to_char("Goodbye.  We will be awaiting your return.\r\n", ch);
	    } else {
		send_to_char("\r\nYou flicker out of reality...\r\n", ch);
		act("$n flickers out of reality.", TRUE, ch, 0, 0, TO_ROOM);
		sprintf(buf, "%s has left the game%s.", GET_NAME(ch),
			PLR_FLAGGED(ch, PLR_TESTER) ? " (tester)" : " naked");
		mudlog(buf, NRM, MAX(LVL_AMBASSADOR, GET_INVIS_LEV(ch)), TRUE);
	    }
	    Crash_rentsave(ch, 0, RENT_RENTED);
	} else if ((ch->carrying || IS_WEARING_W(ch)) && 
		   !ROOM_FLAGGED(ch->in_room, ROOM_HOUSE) &&
		   strncasecmp(argument, "yes", 3)) {
	    send_to_char("If you quit without renting, you will lose all your things.\r\n"
			 "If you would rather rent, type HELP INNS to find out where the nearest\r\n"
			 "inn is.  If you are SURE you want to QUIT, type 'quit yes'.\r\n", ch); 
	    return;

	} else if (IS_CARRYING_N(ch) || IS_WEARING_W(ch)) {
	    if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
		act("You fear that your death will grieve $N.",
		    FALSE, ch, 0, ch->master, TO_CHAR);
		return;
	    }
	    if (ROOM_FLAGGED(ch->in_room, ROOM_HOUSE)) {
		cost = Crash_rentcost(ch, TRUE, 1);

		if ( cost < 0 ) {
		    send_to_char("Unable to rent.\r\n", ch);
		    return;
		}
	  
		Crash_rentsave(ch, cost, RENT_RENTED);
		sprintf(buf, "%s has left the game from Houseroom %d.",
			GET_NAME(ch), ch->in_room->number);
		mudlog(buf, NRM, MAX(LVL_AMBASSADOR, GET_INVIS_LEV(ch)), TRUE);
		send_to_char("You smoothly slip out of existence.\r\n", ch);
		act("$n smoothly slips out of existence and is gone.", 
		    TRUE, ch, 0, 0, TO_ROOM);
	    } 
	    else {
		sprintf(buf, 
			"\r\nVery well %s.  You drop all your things and vanish!\r\n", 
			GET_NAME(ch));
		send_to_char(buf, ch);
		Crash_cursesave(ch);  // saves !remove items
		act("$n disappears, leaving all $s equipment behind!", 
		    TRUE, ch, 0, 0, TO_ROOM);
		sprintf(buf, "%s (%d) has quit the game, EQ drop at %d.", 
			GET_NAME(ch), 
			GET_LEVEL(ch), ch->in_room->number);
		mudlog(buf, NRM, MAX(LVL_AMBASSADOR, GET_INVIS_LEV(ch)), TRUE);
	    }
	} else {
	    Crash_cursesave( ch );
	    send_to_char("\r\nYou flicker out of reality...\r\n", ch);
	    act("$n flickers out of reality.", TRUE, ch, 0, 0, TO_ROOM);
	    sprintf(buf, "%s has left the game naked.", GET_NAME(ch));
	    mudlog(buf, NRM, MAX(LVL_AMBASSADOR, GET_INVIS_LEV(ch)), TRUE);
	}

	save_room = ch->in_room;
	extract_char(ch, FALSE);	 /* Char is saved in extract char */
	if (GET_ZONE(save_room) == 109 || ROOM_FLAGGED(save_room, ROOM_HOUSE)) {
	    save_char(ch, save_room);
	}
    }
}



ACMD(do_save)
{
    if (IS_NPC(ch) || !ch->desc)
	return;

    if (cmd) {
	sprintf(buf, "Saving %s.\r\n", GET_NAME(ch));
	send_to_char(buf, ch);
    }
    save_char(ch, NULL);
    Crash_crashsave(ch);
    if (ROOM_FLAGGED(ch->in_room, ROOM_HOUSE))
	House_crashsave(ch->in_room->number);
}


/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
    send_to_char("Sorry, but you cannot do that here!\r\n", ch);
}



ACMD(do_elude)
{
    struct affected_type af;
  
    if (affected_by_spell(ch, SKILL_ELUSION)) {
	affect_from_char(ch, SKILL_ELUSION);
	send_to_char("You are no longer being actively elusive.\r\n", ch);
	return;
    } else {
	if (CHECK_SKILL(ch, SKILL_ELUSION) > number(0, 101)) {
	    af.type = SKILL_ELUSION;
        af.is_instant = 0;
	    af.duration = -1;
	    af.location = APPLY_NONE;
	    af.modifier = 0;
	    af.aff_index = 1;
	    af.bitvector = AFF_NOTRACK;
	    af.level = GET_LEVEL(ch) + GET_REMORT_GEN(ch);

	    affect_to_char(ch, &af);
	}
	send_to_char("You begin to elude any followers.\r\n", ch);
    }
}


ACMD(do_practice)
{
    void list_skills(struct char_data * ch, int mode, int type);

    one_argument(argument, arg);

    if (*arg)
	send_to_char("You can only practice skills in your guild.\r\n", ch);
    else {
	list_skills(ch, 1, 3);
    }
}

ACMD(do_improve)
{
    send_to_char("You need expert supervision in order to improve.\r\n", ch);
}
ACMD(do_board)
{
    send_to_char("And just what do you plan on boarding?\r\n", ch);
}
ACMD(do_palette)
{
    strcpy(buf, "Available Colors:\r\n");
    sprintf(buf, "%s%sRED %sBOLD%s %s%sUNDER%s %s%sBLINK %sBLINKBOLD%s %s%sREV %sREVBOLD%s\r\n", buf,
	    KRED, KBLD, KNRM, KRED, KUND, KNRM, KRED, KBLK, KBLD, KNRM, KRED, KREV, KBLD, KNRM);
    sprintf(buf, "%s%sGRN %sBOLD%s %s%sUNDER%s %s%sBLINK %sBLINKBOLD%s %s%sREV %sREVBOLD%s\r\n", buf,
	    KGRN, KBLD, KNRM, KGRN, KUND, KNRM, KGRN, KBLK, KBLD, KNRM, KGRN, KREV, KBLD, KNRM);
    sprintf(buf, "%s%sYEL %sBOLD%s %s%sUNDER%s %s%sBLINK %sBLINKBOLD%s %s%sREV %sREVBOLD%s\r\n", buf,
	    KYEL, KBLD, KNRM, KYEL, KUND, KNRM, KYEL, KBLK, KBLD, KNRM, KYEL, KREV, KBLD, KNRM);
    sprintf(buf, "%s%sBLU %sBOLD%s %s%sUNDER%s %s%sBLINK %sBLINKBOLD%s %s%sREV %sREVBOLD%s\r\n", buf,
	    KBLU, KBLD, KNRM, KBLU, KUND, KNRM, KBLU, KBLK, KBLD, KNRM, KBLU, KREV, KBLD, KNRM);
    sprintf(buf, "%s%sMAG %sBOLD%s %s%sUNDER%s %s%sBLINK %sBLINKBOLD%s %s%sREV %sREVBOLD%s\r\n", buf,
	    KMAG, KBLD, KNRM, KMAG, KUND, KNRM, KMAG, KBLK, KBLD, KNRM, KMAG, KREV, KBLD, KNRM);
    sprintf(buf, "%s%sCYN %sBOLD%s %s%sUNDER%s %s%sBLINK %sBLINKBOLD%s %s%sREV %sREVBOLD%s\r\n", buf,
	    KCYN, KBLD, KNRM, KCYN, KUND, KNRM, KCYN, KBLK, KBLD, KNRM, KCYN, KREV, KBLD, KNRM);
    sprintf(buf, "%s%sWHT %sBOLD%s %s%sUNDER%s %s%sBLINK %sBLINKBOLD%s %s%sREV %sREVBOLD%s\r\n", buf,
	    KWHT, KBLD, KNRM, KWHT, KUND, KNRM, KWHT, KBLK, KBLD, KNRM, KWHT, KREV, KBLD, KNRM);
    send_to_char(buf, ch);
}

ACMD(do_disembark)
{
    send_to_char("Pray tell, whats the point of that?\r\n", ch);
}
ACMD(do_drive)
{
    send_to_char("Drive WHAT?!\r\n", ch);
}

ACMD(do_visible)
{
    int found = 0;

    if (affect_from_char(ch, SPELL_INVISIBLE)) {
	found = 1;
	send_to_char("You are no longer invisible.\r\n", ch);
    }
    if (affect_from_char(ch, SPELL_GREATER_INVIS) &&
	!found) {
	found = 1;
	send_to_char("You are no longer invisible.\r\n", ch);
    }
    if (affect_from_char(ch, SPELL_TRANSMITTANCE)) {
	found = 1;
	send_to_char("You are no longer transparent.\r\n", ch);
    }
    if (affect_from_char(ch, SPELL_INVIS_TO_ANIMALS)) {
	found = 1;
	send_to_char("You are no longer invisible to animals.\r\n", ch);
    }
    if (affect_from_char(ch, SPELL_INVIS_TO_UNDEAD)) {
	found = 1;
	send_to_char("You are no longer invisibile to undead.\r\n", ch);
    }
 
    if (!found)
	send_to_char("You are already visible.\r\n", ch);
    else
	act("$n fades into view.", TRUE, ch, 0, 0, TO_ROOM);
}



ACMD(do_title)
{
    skip_spaces(&argument);
    delete_doubledollar(argument);

    if (IS_NPC(ch))
	send_to_char("Your title is fine... go away.\r\n", ch);
    else if (PLR_FLAGGED(ch, PLR_NOTITLE))
	send_to_char("You can't title yourself -- you shouldn't have abused it!\r\n", ch);
    else if (strstr(argument, "(") || strstr(argument, ")"))
	send_to_char("Titles can't contain the ( or ) characters.\r\n", ch);
    else if (strlen(argument) > MAX_TITLE_LENGTH) {
	sprintf(buf, "Sorry, titles can't be longer than %d characters.\r\n",
		MAX_TITLE_LENGTH);
	send_to_char(buf, ch);
    } else {
	set_title(ch, argument);
	sprintf(buf, "Okay, you're now %s %s.\r\n", GET_NAME(ch), GET_TITLE(ch));
	send_to_char(buf, ch);
    }
}


int perform_group(struct char_data *ch, struct char_data *vict)
{
    if (IS_AFFECTED(vict, AFF_GROUP) || !CAN_SEE(ch, vict))
	return 0;

    SET_BIT(AFF_FLAGS(vict), AFF_GROUP);
    if (ch != vict)
	act("$N is now a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
    act("You are now a member of $n's group.", FALSE, ch, 0, vict, TO_VICT);
    act("$N is now a member of $n's group.", FALSE, ch, 0, vict, TO_NOTVICT);
    return 1;
}


void print_group(struct char_data *ch)
{
    struct char_data *k;
    struct follow_type *f;

    if (!IS_AFFECTED(ch, AFF_GROUP))
	send_to_char("But you are not the member of a group!\r\n", ch);
    else {
	send_to_char("Your group consists of:\r\n", ch);

	k = (ch->master ? ch->master : ch);

	if (IS_AFFECTED(k, AFF_GROUP)) {
	    sprintf(buf, "     %s[%s%4dH %4dM %4dV%s]%s %s[%s%2d %s%s]%s $N %s(%sHead of group%s)%s",
		    CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_HIT(k), GET_MANA(k), GET_MOVE(k),
		    CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), CCRED(ch, C_NRM), CCNRM(ch, C_NRM),
		    GET_LEVEL(k), CLASS_ABBR(k), CCRED(ch, C_NRM), CCNRM(ch, C_NRM), 
		    CCBLU(ch, C_NRM), CCCYN(ch, C_NRM), CCBLU(ch, C_NRM), CCNRM(ch, C_NRM));
	    act(buf, FALSE, ch, 0, k, TO_CHAR | TO_SLEEP);
	}

	for (f = k->followers; f; f = f->next) {
	    if (!IS_AFFECTED(f->follower, AFF_GROUP))
		continue;

	    sprintf(buf, "     %s[%s%4dH %4dM %4dV%s]%s %s[%s%2d %s%s]%s $N",
		    CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), 
		    GET_HIT(f->follower), GET_MANA(f->follower), GET_MOVE(f->follower),
		    CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), CCRED(ch, C_NRM), CCNRM(ch, C_NRM),
		    GET_LEVEL(f->follower), CLASS_ABBR(f->follower), CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
	    act(buf, FALSE, ch, 0, f->follower, TO_CHAR | TO_SLEEP);
	}
    }
}

ACMD(do_group)
{
    struct char_data *vict;
    struct follow_type *f;
    int found;

    one_argument(argument, buf);

    if (!*buf) {
	print_group(ch);
	return;
    } 

    if (ch->getPosition() < POS_RESTING) {
	send_to_char("You need to be awake to do that.\r\n", ch);
	return;
    }

    if (ch->master) {
	act("You can not enroll group members without being head of a group.",
	    FALSE, ch, 0, 0, TO_CHAR);
	return;
    }

    if (!str_cmp(buf, "all")) {
	perform_group(ch, ch);
	for (found = 0, f = ch->followers; f; f = f->next)
	    found += perform_group(ch, f->follower);
	if (!found)
	    send_to_char("Everyone following you is already in your group.\r\n", ch);
	return;
    }

    if (!(vict = get_char_room_vis(ch, buf))) {
	send_to_char(NOPERSON, ch);
    } else if ((vict->master != ch) && (vict != ch))
	act("$N must follow you to enter your group.", FALSE, ch, 0, vict, TO_CHAR);
    else {
	if (!IS_AFFECTED(vict, AFF_GROUP))
	    perform_group(ch, vict);
	else {
	    if (ch != vict)
		act("$N is no longer a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
	    act("You have been kicked out of $n's group!", FALSE, ch, 0, vict, TO_VICT);
	    act("$N has been kicked out of $n's group!", FALSE, ch, 0, vict, TO_NOTVICT);
	    REMOVE_BIT(AFF_FLAGS(vict), AFF_GROUP);
	}
    }
}



ACMD(do_ungroup)
{
    struct follow_type *f, *next_fol;
    struct char_data *tch;
    void stop_follower(struct char_data * ch);

    one_argument(argument, buf);

    if (!*buf) {
	if (ch->master || !(IS_AFFECTED(ch, AFF_GROUP))) {
	    send_to_char("But you lead no group!\r\n", ch);
	    return;
	}
	sprintf(buf2, "%s has disbanded the group.\r\n", GET_NAME(ch));
	for (f = ch->followers; f; f = next_fol) {
	    next_fol = f->next;
	    if (IS_AFFECTED(f->follower, AFF_GROUP)) {
		REMOVE_BIT(AFF_FLAGS(f->follower), AFF_GROUP);
		send_to_char(buf2, f->follower);
		if (!IS_AFFECTED(f->follower, AFF_CHARM))
		    stop_follower(f->follower);
	    }
	}

	REMOVE_BIT(AFF_FLAGS(ch), AFF_GROUP);
	send_to_char("You disband the group.\r\n", ch);
	return;
    }
    if (!(tch = get_char_room_vis(ch, buf))) {
	send_to_char("There is no such person!\r\n", ch);
	return;
    }
    if (tch->master != ch) {
	send_to_char("That person is not following you!\r\n", ch);
	return;
    }

    if (!IS_AFFECTED(tch, AFF_GROUP)) {
	send_to_char("That person isn't in your group.\r\n", ch);
	return;
    }

    REMOVE_BIT(AFF_FLAGS(tch), AFF_GROUP);

    act("$N is no longer a member of your group.", FALSE, ch, 0, tch, TO_CHAR);
    act("You have been kicked out of $n's group!", FALSE, ch, 0, tch, TO_VICT);
    act("$N has been kicked out of $n's group!", FALSE, ch, 0, tch, TO_NOTVICT);
 
    if (!IS_AFFECTED(tch, AFF_CHARM))
	stop_follower(tch);
}




ACMD(do_report)
{
    struct char_data *k;
    struct follow_type *f;

    if (!IS_AFFECTED(ch, AFF_GROUP)) {
	send_to_char("But you are not a member of any group!\r\n", ch);
	return;
    }
    sprintf(buf, "%s reports: %d/%dH, %d/%dM, %d/%dV\r\n",
	    GET_NAME(ch), GET_HIT(ch), GET_MAX_HIT(ch),
	    GET_MANA(ch), GET_MAX_MANA(ch),
	    GET_MOVE(ch), GET_MAX_MOVE(ch));

    CAP(buf);

    k = (ch->master ? ch->master : ch);

    for (f = k->followers; f; f = f->next)
	if (IS_AFFECTED(f->follower, AFF_GROUP) && f->follower != ch)
	    send_to_char(buf, f->follower);
    if (k != ch)
	send_to_char(buf, k);

    sprintf(buf, "You report: %d/%dH, %d/%dM, %d/%dV\r\n",
	    GET_HIT(ch), GET_MAX_HIT(ch),
	    GET_MANA(ch), GET_MAX_MANA(ch),
	    GET_MOVE(ch), GET_MAX_MOVE(ch));
    send_to_char(buf, ch);
}



ACMD(do_split)
{
    int amount, num, share, mode = 0;
    struct char_data *k;
    struct follow_type *f;

    if (IS_NPC(ch))
	return;

    argument = one_argument(argument, buf);

    skip_spaces(&argument);

    if (*argument && is_abbrev(argument, "credits"))
	mode = 1;

    if (!is_number(buf)) {
	send_to_char("How much do you wish to split with your group?\r\n", ch);
	return;
    }
    amount = atoi(buf);
    if (amount <= 0) {
	send_to_char("Sorry, you can't do that.\r\n", ch);
	return;
    }
    if (mode && amount > GET_CASH(ch)) {
	send_to_char("You don't seem to have that many credits.\r\n", ch);
	return;
    }
    if (!mode && amount > GET_GOLD(ch)) {
	send_to_char("You don't seem to have that much gold.\r\n", ch);
	return;
    }
    k = (ch->master ? ch->master : ch);
  
    if (IS_AFFECTED(k, AFF_GROUP) && (k->in_room == ch->in_room))
	num = 1;
    else
	num = 0;
  
    for (f = k->followers; f; f = f->next)
	if (IS_AFFECTED(f->follower, AFF_GROUP) &&
	    (!IS_NPC(f->follower)) &&
	    (f->follower->in_room == ch->in_room))
	    num++;
  
    if (num && IS_AFFECTED(ch, AFF_GROUP))
	share = amount / num;
    else {
	send_to_char("With whom do you wish to share your loot?\r\n", ch);
	return;
    }
  
    if (mode)
	GET_CASH(ch) -= share * (num - 1);
    else
	GET_GOLD(ch) -= share * (num - 1);
  
    if (IS_AFFECTED(k, AFF_GROUP) && (k->in_room == ch->in_room)
	&& !(IS_NPC(k)) && k != ch) {
	if (mode)
	    GET_CASH(k) += share;
	else
	    GET_GOLD(k) += share;
	sprintf(buf, "%s splits %d %s; you receive %d.\r\n", GET_NAME(ch),
		amount, mode ? "credits" : "coins", share);
	send_to_char(buf, k);
    }
    for (f = k->followers; f; f = f->next) {
	if (IS_AFFECTED(f->follower, AFF_GROUP) &&
	    (!IS_NPC(f->follower)) &&
	    (f->follower->in_room == ch->in_room) &&
	    f->follower != ch) {
	    if (mode)
		GET_CASH(f->follower) += share;
	    else
		GET_GOLD(f->follower) += share;
	    sprintf(buf, "%s splits %d %s; you receive %d.\r\n", GET_NAME(ch),
		    amount, mode ? "credits" : "coins", share);
	    send_to_char(buf, f->follower);
	}
    }
    sprintf(buf, "You split %d %s among %d member%s -- %d each.\r\n",
	    amount, mode ? "credits" : "coins", num, ( num > 1 ? "s" : "" ), share);
    send_to_char(buf, ch);
}

ACMD(do_use)
{
    struct obj_data *mag_item;
    int equipped = 1;
	char arg1[256];
	char_data *vict = NULL;

    half_chop(argument, arg, buf);
    if (!*arg) {
	sprintf(buf2, "What do you want to %s?\r\n", CMD_NAME);
	send_to_char(buf2, ch);
	return;
    }
    mag_item = GET_EQ(ch, WEAR_HOLD);

    if (!mag_item || !isname(arg, mag_item->name)) {
	switch (subcmd) {
	case SCMD_RECITE:
	case SCMD_QUAFF:
	case SCMD_SWALLOW:
	    equipped = 0;
	    if (!(mag_item = get_obj_in_list_all(ch, arg, ch->carrying))) {
		sprintf(buf2, "You don't seem to have %s %s.\r\n", AN(arg), arg);
		send_to_char(buf2, ch);
		return;
	    }
	    if (subcmd == SCMD_RECITE && !CAN_SEE_OBJ(ch, mag_item)) {
		act("You can't see $p well enough to recite from it.",
		    FALSE, ch, mag_item, 0, TO_CHAR);
		return;
	    }
	    break;
	case SCMD_USE:
	case SCMD_INJECT:
	    sprintf(buf2, "You don't seem to be holding %s %s.\r\n", AN(arg), arg);
	    send_to_char(buf2, ch);
	    return;
	default:
	    slog("SYSERR: Unknown subcmd passed to do_use");
	    return;
	}
    }
    switch (subcmd) {
    case SCMD_QUAFF:
	if (GET_OBJ_TYPE(mag_item) != ITEM_POTION) {
	    send_to_char("You can only quaff potions.\r\n", ch);
	    return;
	}
	break;
    case SCMD_SWALLOW:
	if (!IS_OBJ_TYPE(mag_item, ITEM_PILL)) {
	    if (IS_OBJ_TYPE(mag_item, ITEM_FOOD))
		send_to_char("You should really chew that first.\r\n", ch);
	    else
		send_to_char("You cannot swallow that.\r\n", ch);
	    return;
	}
	break;
      
    case SCMD_RECITE:
	if (FIGHTING(ch)) {
	    act("What, while fighting $N?!", FALSE, ch, 0, FIGHTING(ch), TO_CHAR);
	    return;
	} else if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
	    send_to_char("You can only recite scrolls.", ch);
	    return;
	} else if (CHECK_SKILL(ch, SKILL_READ_SCROLLS) < 10) {
	    send_to_char("You do not know how to recite the magical script.\r\n",ch);
	    return;
	}
	break;
    case SCMD_USE:
	if (GET_OBJ_TYPE(mag_item) == ITEM_TRANSPORTER) {
	    send_to_char("You must use the command 'activate' to use this.\r\n",ch);
	    return;
	} else if (GET_OBJ_TYPE(mag_item) == ITEM_SYRINGE) {
	    send_to_char("Use the 'inject' command to use this item.\r\n", ch);
	    return;
	} else if (GET_OBJ_TYPE(mag_item) == ITEM_CIGARETTE ||
		   GET_OBJ_TYPE(mag_item) == ITEM_TOBACCO ||
		   GET_OBJ_TYPE(mag_item) == ITEM_PIPE) {
	    send_to_char("You need to SMOKE that.\r\n", ch);
	    return;
	} else if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) &&
		   (GET_OBJ_TYPE(mag_item) != ITEM_STAFF)) {
	    send_to_char("You can't seem to figure out how to use it.\r\n", ch);
	    return;
	}
	else if (CHECK_SKILL(ch, SKILL_USE_WANDS) < 10) {
	    act("You do not know how to harness the magical power of $p.",
		FALSE, ch, mag_item, 0, TO_CHAR);
	    return;
	}
    
	break;
    case SCMD_INJECT:
	if (GET_OBJ_TYPE(mag_item) != ITEM_SYRINGE) {
	    send_to_char("You can only inject with syringes.\r\n", ch);
	    return;
	}
	one_argument(buf,arg1);
	if((vict = get_char_room(arg1,ch->in_room)) &&
		(
          affected_by_spell(vict, SPELL_STONESKIN) ||
		  (affected_by_spell(vict, SPELL_DERMAL_HARDENING) && random_binary()) ||
		  (affected_by_spell(vict, SPELL_BARKSKIN) && random_binary())
        )
       ){
		 act("$p breaks.", TRUE, ch, mag_item, vict, TO_CHAR);
		 act("$n breaks $p on your arm!", TRUE, ch,mag_item, vict, TO_VICT);
		 act("$n breaks $p on $N's arm!", TRUE, ch,mag_item, vict, TO_NOTVICT);
		 unequip_char(ch,mag_item->worn_on, MODE_EQ);
		 mag_item->obj_flags.damage = 0;
		 return;
	}
	break;
    default:
	slog("SYSERR: Illegal subcmd passed to do_use.");
	return;
    }

    mag_objectmagic(ch, mag_item, buf);
}



ACMD(do_wimpy)
{
    int wimp_lev;

    one_argument(argument, arg);

    if (!*arg) {
	if (GET_WIMP_LEV(ch)) {
	    sprintf(buf, "Your current wimp level is %d hit points.\r\n",
		    GET_WIMP_LEV(ch));
	    send_to_char(buf, ch);
	    return;
	} else {
	    send_to_char("At the moment, you're not a wimp.  (sure, sure...)\r\n", ch);
	    return;
	}
    }
    if (isdigit(*arg)) {
	if ((wimp_lev = atoi(arg))) {
	    if (wimp_lev < 0)
		send_to_char("Heh, heh, heh.. we are jolly funny today, eh?\r\n", ch);
	    else if (wimp_lev > GET_MAX_HIT(ch))
		send_to_char("That doesn't make much sense, now does it?\r\n", ch);
	    else if (wimp_lev > (GET_MAX_HIT(ch) >> 1))
		send_to_char("You can't set your wimp level above half your hit points.\r\n", ch);
	    else {
		sprintf(buf, "Okay, you'll wimp out if you drop below %d hit points.\r\n",
			wimp_lev);
		send_to_char(buf, ch);
		GET_WIMP_LEV(ch) = wimp_lev;
	    }
	} else {
	    send_to_char("Okay, you'll now tough out fights to the bitter end.\r\n", ch);
	    GET_WIMP_LEV(ch) = 0;
	}
    } else
	send_to_char("Specify at how many hit points you want to wimp out at.  (0 to disable)\r\n", ch);

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
	send_to_char("Mosters don't need displays.  Go away.\r\n", ch);
	return;
    }
    skip_spaces(&argument);
    argument = two_arguments(argument, arg1, arg2);
  
    if (!*arg1) {
	send_to_char("Usage: prompt { H | M | V | A | all | normal | none }\r\n",ch);
	return;
    }
    if (arg2 != NULL) {
	if (is_abbrev(arg1, "rows")) {

	    if (is_number(arg2)) { 
		GET_ROWS(ch) = atoi(arg2);
	        return;
	    }

	    else {
		send_to_char("Usage: display rows <number>\r\n", ch);
	        return;
	    }

	} 
	else if (is_abbrev(arg1, "cols")) {
	    
	    if (is_number(arg2)) {
		GET_COLS(ch) = atoi(arg2);
		return;
	    }
	    
	    else {
		send_to_char("Usage: display cols <number>\r\n", ch);
		return;
	    }

	}

	else if (is_abbrev(arg1, "vt100"))

	    if (is_abbrev(arg2, "enable")) {
		enable_vt100(ch);
		return;
	    }
	    
	    else if (is_abbrev(arg2, "disable")) {
		disable_vt100(ch);
		return;
	    }

	    else {
		send_to_char("Usage: display vt100 [on | off]\r\n", ch);
		return;
	    }
    }

    if ( ( ! str_cmp( arg1, "on" ) ) || ( ! str_cmp( arg1, "normal" ) ) ) {
	REMOVE_BIT( PRF2_FLAGS( ch ), PRF2_DISPALIGN );
        SET_BIT( PRF_FLAGS( ch ), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE );
    }

    else if ( ( ! str_cmp( arg1, "all" ) ) ) {
        SET_BIT( PRF_FLAGS( ch ), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE );
        SET_BIT( PRF2_FLAGS( ch ), PRF2_DISPALIGN );
    }

    else {
	REMOVE_BIT(PRF_FLAGS(ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE);
	REMOVE_BIT(PRF2_FLAGS(ch), PRF2_DISPALIGN );

	for (i = 0; i < strlen(arg1); i++) {
	    switch (LOWER(arg1[i])) {
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
	        SET_BIT( PRF2_FLAGS( ch ), PRF2_DISPALIGN );
	        break;	
	    }
	}
    }

    send_to_char(OK, ch);
}



ACMD(do_gen_write)
{
    FILE *fl;
    char *tmp, *filename;
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
    default:
	return;
    }

    ct = time(0);
    tmp = asctime(localtime(&ct));

    if (IS_NPC(ch)) {
	send_to_char("Monsters can't have ideas - Go away.\r\n", ch);
	return;
    }

    skip_spaces(&argument);
    delete_doubledollar(argument);

    if (!*argument) {
	send_to_char("That must be a mistake...\r\n", ch);
	return;
    }
    sprintf(buf, "%s %s: %s", GET_NAME(ch), CMD_NAME, argument);
    mudlog(buf, NRM, MAX(GET_INVIS_LEV(ch), LVL_AMBASSADOR), FALSE);

    if (stat(filename, &fbuf) < 0) {
	perror("Error statting file");
	return;
    }
    if (fbuf.st_size >= max_filesize) {
	send_to_char("Sorry, the file is full right now.. try again later.\r\n", ch);
	return;
    }
    if (!(fl = fopen(filename, "a"))) {
	perror("do_gen_write");
	send_to_char("Could not open the file.  Sorry.\r\n", ch);
	return;
    }
    fprintf(fl, "%-8s (%6.6s) [%5d] %s\n", GET_NAME(ch), (tmp + 4),
	    ch->in_room->number, argument);
    fclose(fl);
    send_to_char("Okay.  Thanks!\r\n", ch);
}



#define TOG_OFF 0
#define TOG_ON  1

#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))
#define PRF2_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF2_FLAGS(ch), (flag))) & (flag))

ACMD(do_gen_tog)
{
    long result;
    extern int nameserver_is_slow;

    char *tog_messages[][2] = {
	{"You are now safe from summoning by other players.\r\n",
	 "You may now be summoned by other players.\r\n"},
	{"Nohassle disabled.\r\n",
	 "Nohassle enabled.\r\n"},
	{"Brief mode off.\r\n",
	 "Brief mode on.\r\n"},
	{"Compact mode off.\r\n",
	 "Compact mode on.\r\n"},
	{"You can now hear tells.\r\n",
	 "You are now deaf to tells.\r\n"},
	{"You can now hear auctions.\r\n",
	 "You are now deaf to auctions.\r\n"},
	{"You can now hear shouts.\r\n",
	 "You are now deaf to shouts.\r\n"},
	{"You can now hear gossip.\r\n",
	 "You are now deaf to gossip.\r\n"},
	{"You can now hear the congratulation messages.\r\n",
	 "You are now deaf to the congratulation messages.\r\n"},
	{"You can now hear the Wiz-channel.\r\n",
	 "You are now deaf to the Wiz-channel.\r\n"},
	{"You are no longer part of the Quest.\r\n",
	 "Okay, you are part of the Quest!\r\n"},
	{"You will no longer see the room flags.\r\n",
	 "You will now see the room flags.\r\n"},
	{"You will now have your communication repeated.\r\n",
	 "You will no longer have your communication repeated.\r\n"},
	{"HolyLight mode off.\r\n",
	 "HolyLight mode on.\r\n"},
	{"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
	 "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
	{"Autoexits disabled.\r\n",
	 "Autoexits enabled.\r\n"},
	{"You have returned to the keyboard.\r\n",
	 "You are now afk.  When you move again, you will no longer be.\r\n"},
	{"You will now hear the music of your peers.\r\n", 
	 "You will no longer hear the music of your peers.\r\n"}, 
	{"You will now be subjected to the spewings of your peers.\r\n", 
	 "You will no longer be subjected to the spewings of your peers.\r\n"},
	{"You will now see all misses in your fights.\r\n",
	 "You will no longer see the misses in your fights.\r\n"},
	{"You will no longer be beeped if someone tells to you.\r\n",
	 "You will now receive a bell tone if someone tells to you.\r\n"},
	{"You are now open to intermud communication.\r\n",
	 "You are now closed to intermud communication.\r\n"},
	{"You have now opened yourself to clan communication.\r\n",
	 "You have closed yourself to clan communication.\r\n"},
	{"You are now allowing others to identify you.\r\n",
	 "You are now consciously resisting any attempts to identify you.\r\n"},
	{"You will now stop debugging fights.\r\n",
	 "You will now be debugging fights.\r\n"},
	{"You are now ignoring the newbies.\r\n",
	 "You are now monitoring the newbies.\r\n"},
	{"You have disabled auto-diagnose.\r\n",
	 "You will now automatically be aware of your opponent's condition.\r\n"},
	{"You will now hear the dreams of other players while you sleep.\r\n",
	 "You will no longer hear dreams.\r\n"},
	{"You will now be aware of the projections of other remorts.\r\n",
	 "You will now ignore the projections of other remorts.\r\n"},
	{"You are no longer halted.\r\n",
	 "You are now in halt mode.\r\n"},
	{"You resume your immortal status.\r\n",
	 "Other gods may now kill you... if you don't watch it.\r\n"},
	{"You will now see all affections on the score page.\r\n",
	 "You will no longer see your affections on the score page.\r\n"},
	{"You now hear the hollering.\r\n",
	 "You now ignore the hollering.\r\n"},
	{"You are now on the immchat channel.\r\n",
	 "You are now closed off from the immchat channel.\r\n",},
	{"Your title will no longer default to your clan title.\r\n",
	 "Your title will now default to your clan title.\r\n"},
	{"Your clan badge will now show up on the who list.\r\n",
	 "Your clan badge will now be hidden on the who list.\r\n"},
	{"You are no longer a light reader.\r\n",
	 "You are a light reader.\r\n"},
	{"Your prompt will now be displayed only after carriage returns.\r\n",
	 "Your prompt will now be redrawn after every message you receive.\r\n"},
	{"You will now be shown on the who list.\r\n",
	 "You will no longer be shown on the who list.\r\n"},
	{"You are no longer anonymous.\r\n",
	 "Your char_class and level will now be anonymous.\r\n"},
	{"You will now see affect trailers on characters.\r\n",
	 "You will now ignore affect trailers on characters.\r\n"},
	{"Logall","Logall"},
	{"Jetstream","Jetstream"},
	{"Weather", "Weather"},
	{"Autosplit OFF.\r\n",
	 "Autosplit ON.\r\n"},
	{"Autoloot OFF.\r\n",
	 "Autoloot ON.\r\n"},
	{"Pkiller OFF.\r\n",
	 "Pkiller ON.\r\n" },
	{"You open yourself to the echoing thoughts of the gods.\r\n",
	 "You close your mind to the echoing thoughts of the gods.\r\n",},
	{"\n","\n"}
    };

    if (IS_NPC(ch))
	return;

    switch (subcmd) {
    case SCMD_NOSUMMON:
	result = PRF_TOG_CHK(ch, PRF_SUMMONABLE);
	break;
    case SCMD_NOHASSLE:
	result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
	break;
    case SCMD_BRIEF:
	result = PRF_TOG_CHK(ch, PRF_BRIEF);
	break;
    case SCMD_COMPACT:
	result = PRF_TOG_CHK(ch, PRF_COMPACT);
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
    case SCMD_QUEST:
	result = PRF_TOG_CHK(ch, PRF_QUEST);
	if (!PRF_FLAGGED(ch, PRF_QUEST) && IS_SET(PLR_FLAGS(ch), PLR_QUESTOR))
	    send_to_char("Don't forget to remove your questor flag.\r\n", ch);
	break;
    case SCMD_ROOMFLAGS:
	result = PRF_TOG_CHK(ch, PRF_ROOMFLAGS);
	break;
    case SCMD_NOREPEAT:
	result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
	break;
    case SCMD_HOLYLIGHT:
	result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
	break;
    case SCMD_SLOWNS:
	result = (nameserver_is_slow = !nameserver_is_slow);
	sprintf(buf, "%s has turned name server %s.", GET_NAME(ch), 
		ONOFF(!nameserver_is_slow));
	mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
	break;
    case SCMD_AUTOEXIT:
	result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
	break;
    case SCMD_AFK:
	result = PLR_TOG_CHK(ch, PLR_AFK);
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
    case SCMD_NOINTWIZ:
	result = PRF_TOG_CHK(ch, PRF_NOINTWIZ);
	break;
    case SCMD_NOCLANSAY:
	result = PRF_TOG_CHK(ch, PRF_NOCLANSAY);
	break;
    case SCMD_NOIDENTIFY:
	result = PRF_TOG_CHK(ch, PRF_NOIDENTIFY);
	break;
    case SCMD_DEBUG:
	result = PRF2_TOG_CHK(ch, PRF2_FIGHT_DEBUG);
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
	if (FIGHTING(ch)) {
	    send_to_char("You can't do this while fighting.\r\n", ch);
	    return;
	}
	result = PLR_TOG_CHK(ch, PLR_MORTALIZED);
	sprintf(buf, "(GC): %s has %smortalized at %d.", GET_NAME(ch),
		PLR_FLAGGED(ch, PLR_MORTALIZED) ? "" : "im", 
		ch->in_room->number);
	mudlog(buf, NRM, GET_INVIS_LEV(ch), TRUE);

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

    case SCMD_CLAN_TITLE:
	result = PRF2_TOG_CHK(ch, PRF2_CLAN_TITLE);
	break;

    case SCMD_CLAN_HIDE:
	result = PRF2_TOG_CHK(ch, PRF2_CLAN_HIDE);
	break;

    case SCMD_LIGHT_READ:
	result = PRF2_TOG_CHK(ch, PRF2_LIGHT_READ);
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
	sprintf(buf, "%s has toggled logall %s.", GET_NAME(ch), ONOFF(log_cmds));
	mudlog(buf, BRF, MAX(LVL_LOGALL, GET_INVIS_LEV(ch)), TRUE);
	send_to_char(strcat(buf, "\r\n"), ch);
	/*
	  if (log_cmds && !cmd_log_fl && 
	  (!(cmd_log_fl = fopen(CMD_LOG_FILE, "w"))))
	  mudlog("SYSERR:  Unable to open CMD_LOG_FILE", 
	  BRF, MAX(LVL_LOGALL, GET_INVIS_LEV(ch)), TRUE);
	  else if (!log_cmds && cmd_log_fl)
	  fclose(cmd_log_fl);
	*/
	return;
    case SCMD_JET_STREAM:
	TOGGLE_BIT(jet_stream_state, 1);
	sprintf(buf, "%s has toggled jet_stream_state %s.",GET_NAME(ch),ONOFF(jet_stream_state));
	mudlog(buf, BRF, GET_INVIS_LEV(ch), TRUE);
	send_to_char(strcat(buf, "\r\n"), ch);
	return;
    
    case SCMD_WEATHER:
	weather_change();
	send_to_char("Weather change called.\r\n", ch);
	return;

    case SCMD_AUTOSPLIT:
	result = PRF2_TOG_CHK(ch, PRF2_AUTOSPLIT);
	break;
    
    case SCMD_AUTOLOOT:
	result = PRF2_TOG_CHK(ch, PRF2_AUTOLOOT);
	break;
    
    case SCMD_PKILLER:
	result = PRF2_TOG_CHK( ch, PRF2_PKILLER );
	break;

	case SCMD_NOGECHO:
	result = PRF2_TOG_CHK( ch, PRF2_NOGECHO );
	break;

    default:
	slog("SYSERR: Unknown subcmd in do_gen_toggle");
	return;
    }

    if (result)
	send_to_char(tog_messages[subcmd][TOG_ON], ch);
    else
	send_to_char(tog_messages[subcmd][TOG_OFF], ch);

    return;
}
ACMD(do_store)
{
    send_to_char("You can't do that here.\r\n", ch);
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
	send_to_char("Usage: compare <item1> <item2>\r\n", ch);
	return;
    }
    if (!(item1 = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
	sprintf(buf, "You aren't carrying %s %s.\r\n", AN(arg1), arg1);
	send_to_char(buf, ch);
	return;
    }

    if (!*arg2) {
	send_to_char("Compare it to what?\r\n", ch);
	return;
    }
    if (!(item2 = get_obj_in_list_vis(ch, arg2, ch->carrying))) {
	sprintf(buf, "You aren't carrying %s %s.\r\n", AN(arg2), arg2);
	send_to_char(buf, ch);
	return;
    }
    if (GET_OBJ_COST(item2) == GET_OBJ_COST(item1))
	send_to_char("They appear to be of approximately equal value.\r\n", ch);
    else {
	sprintf(buf, "%s ", GET_OBJ_COST(item1) > GET_OBJ_COST(item2) ? 
		item1->short_description : item2->short_description);
	strcat(buf, "appears to be a bit more valuable.\r\n");
	send_to_char(CAP(buf), ch);
    }
    if (GET_OBJ_MAX_DAM(item1) && GET_OBJ_MAX_DAM(item2)) {
	if (GET_OBJ_DAM(item1) * 100 / GET_OBJ_MAX_DAM(item1) >
	    GET_OBJ_DAM(item2) * 100 / GET_OBJ_MAX_DAM(item2))
	    act("$p appears to be in better condition than $P.",
		FALSE, ch, item1, item2, TO_CHAR);
	if (GET_OBJ_DAM(item1) * 100 / GET_OBJ_MAX_DAM(item1) <
	    GET_OBJ_DAM(item2) * 100 / GET_OBJ_MAX_DAM(item2))
	    act("$P appears to be in better condition than $p.",
		FALSE, ch, item1, item2, TO_CHAR);
	else if (GET_OBJ_MAX_DAM(item1) > GET_OBJ_MAX_DAM(item2))
	    act("$p appears to be sturdier than $P.",
		FALSE, ch, item1, item2, TO_CHAR);
	else
	    act("$P appears to be sturdier than $p.",
		FALSE, ch, item1, item2, TO_CHAR);
    }
    if (GET_OBJ_TYPE(item1) != GET_OBJ_TYPE(item2))
	return;
  
    switch (GET_OBJ_TYPE(item1)) {
    case ITEM_SCROLL:
    case ITEM_WAND:
    case ITEM_STAFF:
    case ITEM_POTION:
	if (!IS_MAGE(ch) || IS_DWARF(ch) || GET_INT(ch) < 15)
	    break;
	sprintf(buf, "%s ", GET_OBJ_VAL(item1, 0) >= GET_OBJ_VAL(item2, 0) ? 
		item1->short_description : item2->short_description);
	strcat(buf, "seems to carry a greater aura of power.\r\n");
	send_to_char(CAP(buf), ch);
	break;
    case ITEM_WEAPON:
	avg1 = (int) (((GET_OBJ_VAL(item1, 2) +1) / 2.0) * GET_OBJ_VAL(item1, 1)); 
	avg2 = (int) (((GET_OBJ_VAL(item2, 2) +1) / 2.0) * GET_OBJ_VAL(item2, 1)); 
	if (avg1 == avg2)
	    send_to_char("Both weapons appear to have similar damage potential.\r\n", ch);
	else {
	    sprintf(buf, "%s appears to be a more formidable weapon.\r\n", 
		    avg1 >= avg2 ? item1->short_description : item2->short_description);
	    send_to_char(CAP(buf), ch);
	}
	break;
    case ITEM_ARMOR:
	avg1 = GET_OBJ_VAL(item1, 0);
	avg2 = GET_OBJ_VAL(item2, 0);
	if (avg1 == avg2) 
	    send_to_char("They both seem to be of about equal quality.\r\n", ch);
	else {
	    sprintf(buf, "%s appears to have more protective quality.\r\n", 
		    avg1 >= avg2 ? item1->short_description : item2->short_description);
	    send_to_char(CAP(buf), ch);
	}
	break;
    case ITEM_CONTAINER:
    case ITEM_DRINKCON:
	avg1 = GET_OBJ_VAL(item1, 0);
	avg2 = GET_OBJ_VAL(item2, 0);
	if (avg1 == avg2) 
	    send_to_char("Looks like they will each hold about the same amount.\r\n", ch);
	else {
	    sprintf(buf, "%s looks like it will hold more.\r\n", 
		    avg1 >= avg2 ? item1->short_description : item2->short_description);
	    send_to_char(CAP(buf), ch);
	}
	break;
    case ITEM_FOOD:
	avg1 = GET_OBJ_VAL(item1, 0);
	avg2 = GET_OBJ_VAL(item2, 0);
	if (avg1 == avg2) 
	    send_to_char("They both look equally tasty.\r\n", ch);
	else {
	    sprintf(buf, "%s looks more filling that the other.\r\n", 
		    avg1 >= avg2 ? item1->short_description : item2->short_description);
	    send_to_char(CAP(buf), ch);
	}
	break;
    case ITEM_HOLY_SYMB:
	if (GET_OBJ_VAL(item1, 0) != GET_OBJ_VAL(item2, 0)) {
	    send_to_char("They appear to be symbols of different deities.\r\n", ch);
	    break;
	}
	if (!IS_KNIGHT(ch) && !IS_CLERIC(ch)) 
	    break;
	avg1 = GET_OBJ_VAL(item1, 2);
	avg2 = GET_OBJ_VAL(item2, 2);
	if (avg1 == avg2) 
	    send_to_char("They appear to have the same minimum level.\r\n", ch);
	else {
	    sprintf(buf, "%s looks like it has a higher minimum level.\r\n", 
		    avg1 >= avg2 ? item1->short_description : item2->short_description);
	    send_to_char(CAP(buf), ch);
	}
	avg1 = GET_OBJ_VAL(item1, 3);
	avg2 = GET_OBJ_VAL(item2, 3);
	if (avg1 == avg2) 
	    send_to_char("They appear to have the same maximum level.\r\n", ch);
	else {
	    sprintf(buf, "%s seems to support greater power.\r\n", 
		    avg1 >= avg2 ? item1->short_description : item2->short_description);
	    send_to_char(CAP(buf), ch);
	}
	break;
    case ITEM_BATTERY:
	avg1 = GET_OBJ_VAL(item1, 0);
	avg2 = GET_OBJ_VAL(item2, 0);
	if (avg1 == avg2) 
	    send_to_char("They appear to be able to store the same amount of energy.\r\n", ch);
	else {
	    sprintf(buf, "%s looks like it will hold more energy.\r\n", 
		    avg1 >= avg2 ? item1->short_description : item2->short_description);
	    send_to_char(CAP(buf), ch);
	}
	break;
    case ITEM_ENERGY_GUN:
	avg1 = GET_OBJ_VAL(item1, 0);
	avg2 = GET_OBJ_VAL(item2, 0);
	if (avg1 == avg2) 
	    send_to_char("They appear to be able to store the same amount of energy.\r\n", ch);
	else {
	    sprintf(buf, "%s looks like it will hold more energy.\r\n", 
		    avg1 >= avg2 ? item1->short_description : item2->short_description);
	    send_to_char(CAP(buf), ch);
	}
	avg1 = GET_OBJ_VAL(item1, 2);
	avg2 = GET_OBJ_VAL(item2, 2);
	if (avg1 == avg2) 
	    send_to_char("They can both discharge energy at comparable rates.\r\n", ch);
	else {
	    sprintf(buf, "%s can release energy at a greater rate.\r\n", 
		    avg1 >= avg2 ? item1->short_description : item2->short_description);
	    send_to_char(CAP(buf), ch);
	}
	break;
    default:
	send_to_char("You can see nothing else special about either one.\r\n", ch);
	break;
    }
}

ACMD(do_screen)
{
    int leng;
    if (IS_NPC(ch)) {
	send_to_char("Your SCREEN is default 22 as an NPC.  Sorry.\r\n", ch);
	return;
    }
    one_argument(argument, arg);
    if (!*arg) {
	sprintf(buf, "Your current screen length is: %d lines.\r\n", GET_PAGE_LENGTH(ch));
	send_to_char(buf, ch);
	return;
    } 
    if (isdigit(*arg)) {
	leng = atoi(arg);
	leng = MIN(leng, 200);
	leng = MAX(leng, 0);
	GET_PAGE_LENGTH(ch) = leng;
	sprintf(buf, "Your screen length will now be %d lines.\r\n", GET_PAGE_LENGTH(ch));
	send_to_char(buf, ch);
	return;
    } else
	send_to_char("Specify the length of your screen from top to bottom, in lines.\r\n", ch);
}
  
ACMD(do_throw)
{
    struct obj_data *obj=NULL, *target_obj=NULL;
    struct char_data *vict=NULL, *target_vict=NULL;
    char arg1[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int dir = 0, victim_ac, diceroll, calc_thaco;
    struct room_data *r_toroom = NULL;

    argument = one_argument(argument, arg1);
    if (!*arg1)
	send_to_char("Throw what at what?\r\n",ch);
    else if (!(obj = get_object_in_equip_pos(ch, arg1, WEAR_WIELD)) &&
	     !(obj = get_object_in_equip_pos(ch, arg1, WEAR_HOLD)) &&
	     !(obj = get_obj_in_list_vis(ch, arg1, ch->carrying)) &&
	     !(vict = get_char_room_vis(ch, arg1)))
	send_to_char("You can't find that to throw it.\r\n", ch);
    else if (obj) {
	if (IS_OBJ_STAT(obj, ITEM_NODROP) && GET_LEVEL(ch) < LVL_ETERNAL) {
	    act("Arrrgh!  $p won't come off of your hand!", FALSE, ch, obj, 0, TO_CHAR);
	    act("$n struggles with $p for a moment.", FALSE, ch, obj, 0, TO_ROOM);
	    return;
	}
	argument = one_argument(argument, arg1);
	if (!*arg1) {
	    if (obj->worn_on >= 0) {
		if (obj->worn_on == WEAR_WIELD && GET_EQ(ch, WEAR_WIELD_2))
		    obj_to_char(unequip_char(ch, WEAR_WIELD_2, MODE_EQ), ch);
		obj_to_char(unequip_char(ch, obj->worn_on, MODE_EQ), ch);
	    }

	    act("$n throws $p across the room.", FALSE, ch, obj, 0, TO_ROOM);
	    act("You throw $p across the room.", FALSE, ch, obj, 0, TO_CHAR);  
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
		 !(target_obj = get_obj_in_list_vis(ch, arg1, ch->in_room->contents))) {
	    send_to_char("Throw at what??\r\n", ch);
	    return;
	}

	if (dir) {
	    if (!ch->in_room->dir_option[dir-1] ||
		(r_toroom = ch->in_room->dir_option[dir-1]->to_room) == NULL) {
		sprintf(buf, "You cannot throw anything to the %s.\r\n", 
			dirs[(int)(dir-1)]);
		send_to_char(buf, ch);
		return;
	    }
	    if (IS_SET(EXIT(ch, dir-1)->exit_info, EX_CLOSED)) {
		if (obj->worn_on >= 0) {
		    if (obj->worn_on == WEAR_WIELD && GET_EQ(ch, WEAR_WIELD_2))
			obj_to_char(unequip_char(ch, WEAR_WIELD_2, MODE_EQ), ch);
		    obj_to_char(unequip_char(ch, obj->worn_on, MODE_EQ), ch);
		}

		sprintf(buf, "$n throws $p %sward against the closed %s.",
			dirs[(int)(dir-1)], EXIT(ch, dir-1)->keyword? 
			fname(EXIT(ch, dir-1)->keyword) : "door");
		act(buf, FALSE, ch, obj, 0, TO_ROOM);
		sprintf(buf, "You throw $p %sward against the closed %s.",
			dirs[(int)(dir-1)], EXIT(ch, dir-1)->keyword? 
			fname(EXIT(ch, dir-1)->keyword) : "door");
		act(buf, FALSE, ch, obj, 0, TO_CHAR);
		obj_from_char(obj);
		obj_to_room(obj, ch->in_room);
		return;
	    }
	    argument = one_argument(argument, arg1);
	    if (*arg1 &&
		!(target_vict = get_char_in_remote_room_vis(ch, arg1, r_toroom)) &&
		!(target_obj = get_obj_in_list_vis(ch, arg1, r_toroom->contents))) {
		send_to_char("No such target in that direction.\r\n", ch);
		return;
	    }
	    if (obj->worn_on >= 0) {
		if (obj->worn_on == WEAR_WIELD && GET_EQ(ch, WEAR_WIELD_2))
		    obj_to_char(unequip_char(ch, WEAR_WIELD_2, MODE_EQ), ch);
	
		obj_to_char(unequip_char(ch, obj->worn_on, MODE_EQ), ch);
	    }

	    sprintf(buf, "$n throws $p %sward.", dirs[(int)(dir-1)]);
	    act(buf, FALSE, ch, obj, 0, TO_ROOM);
	    sprintf(buf, "You throw $p %sward.", dirs[(int)(dir-1)]);
	    act(buf, FALSE, ch, obj, 0, TO_CHAR);
	    obj_from_char(obj);
	    obj_to_room(obj, r_toroom);
	    if (target_vict) {
		sprintf(buf, "$p flies in from %s and hits you in the head!", 
			from_dirs[(int)(dir-1)]);
		act(buf, FALSE, 0, obj, target_vict, TO_VICT);
		sprintf(buf, "$p flies in from %s and hits $N in the head!", 
			from_dirs[(int)(dir-1)]);
		act(buf, FALSE, 0, obj, target_vict, TO_NOTVICT);
	    } else if (target_obj) {
		sprintf(buf, "$p flies in from %s and slams into %s!", 
			from_dirs[(int)(dir-1)], target_obj->short_description);
		act(buf, FALSE, 0, obj, target_vict, TO_ROOM);
	    } else {
		sprintf(buf, "$p flies in from %s and lands by your feet.", 
			from_dirs[(int)(dir-1)]);
		act(buf, FALSE, 0, obj, 0, TO_ROOM);
	    }
	    return;
	}   
     
	if (target_vict) {
	    if (obj->worn_on >= 0) {
		if (obj->worn_on == WEAR_WIELD && GET_EQ(ch, WEAR_WIELD_2))
		    obj_to_char(unequip_char(ch, WEAR_WIELD_2, MODE_EQ), ch);
	
		obj_to_char(unequip_char(ch, obj->worn_on, MODE_EQ), ch);
	    }
      
	    if (target_vict != ch) {
		act("$n throws $p at $N!", FALSE, ch, obj, target_vict, TO_NOTVICT);
		act("$n throws $p at you!", FALSE, ch, obj, target_vict, TO_VICT);
		act("You throw $p at $N.", FALSE, ch, obj, target_vict, TO_CHAR);


		if (GET_OBJ_TYPE(obj) == ITEM_WEAPON && 
		    !ROOM_FLAGGED(target_vict->in_room, ROOM_PEACEFUL)) {
		    calc_thaco = calculate_thaco(ch, target_vict, NULL) + 10;
		    calc_thaco -= (CHECK_SKILL(ch, SKILL_THROWING)/10);
		    if (!IS_OBJ_STAT2(obj, ITEM2_THROWN_WEAPON))
			calc_thaco += 2*obj->getWeight();
          
		    victim_ac = GET_AC(target_vict) / 10;
		    victim_ac = MAX(-10, victim_ac);	/* -10 is lowest */
		    if (AWAKE(target_vict))
			victim_ac += dex_app[GET_DEX(target_vict)].defensive;
		    diceroll = number(1, 20);
		    if ((diceroll < 20 && AWAKE(target_vict)) && 
			((diceroll == 1) || ((calc_thaco - diceroll) > victim_ac))) {
			act("$p narrowly misses you!", 
			    FALSE, ch,obj, target_vict, TO_VICT);
			act("$p narrowly misses $M!", 
			    FALSE, ch,obj,target_vict,TO_NOTVICT);
			act("$p narrowly misses $M!", 
			    FALSE, ch,obj,target_vict,TO_CHAR);
		    } else {
			damage(ch, target_vict, 
			       dice(GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2)) +
			       str_app[STRENGTH_APPLY_INDEX(ch)].todam,
			       GET_OBJ_VAL(obj, 3) + TYPE_HIT, number(0, NUM_WEARS-1));
			gain_skill_prof(ch, SKILL_THROWING);
		    }
            
		    WAIT_STATE(ch, PULSE_VIOLENCE * 1);
		    if (CHECK_SKILL(ch, SKILL_THROWING) < 90)
			WAIT_STATE(ch, PULSE_VIOLENCE * 1);
		    if (CHECK_SKILL(ch, SKILL_THROWING) < 60)
			WAIT_STATE(ch, PULSE_VIOLENCE * 1);

		    if (IS_MOB(target_vict) && !FIGHTING(target_vict))
			hit(target_vict, ch, TYPE_UNDEFINED);

		    obj_from_char(obj);
		    obj_to_room(obj, ch->in_room);
		    return;
		}    
	    } else {
		act("$n throws $p at $mself!", FALSE, ch, obj, target_vict, TO_ROOM);
		act("You throw $p at yourself.", FALSE, ch, obj, target_vict, TO_CHAR);
	    }
	    if (ch != target_vict)
		act("$p hits $M in the head!", FALSE, ch, obj, target_vict, TO_CHAR);
	    act("$p hits $N in the head!", FALSE, ch, obj, target_vict, TO_NOTVICT);
	    act("$p hits you in the head!", FALSE, ch, obj, target_vict, TO_VICT);
	} else if (target_obj) {
	    if (obj->worn_on >= 0) {
		if (obj->worn_on == WEAR_WIELD && GET_EQ(ch, WEAR_WIELD_2))
		    obj_to_char(unequip_char(ch, WEAR_WIELD_2, MODE_EQ), ch);
	
		obj_to_char(unequip_char(ch, obj->worn_on, MODE_EQ), ch);
	    }
	    sprintf(buf, "$n hurls $p up against %s with brute force!", 
		    target_obj->short_description);
	    act(buf, FALSE, ch, obj, 0, TO_ROOM);
	    sprintf(buf, "You hurl $p up against %s with brute force!", 
		    target_obj->short_description);
	    act(buf, FALSE, ch, obj, 0, TO_CHAR);
	} 
	obj_from_char(obj);
	obj_to_room(obj, ch->in_room);     
    } // else if (obj) {
    else if (vict) {
	send_to_char("You cannot throw people.\r\n", ch);
    }
}

ACMD(do_feed)
{
    struct char_data *vict = NULL;
    struct obj_data *food = NULL;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int amount;

    half_chop(argument, arg1, arg2);

    if (IS_VAMPIRE(ch)) {
	if (FIGHTING(ch)) {
	    if (!IS_AFFECTED_3(ch, AFF3_FEEDING)) {
		send_to_char("You prepare to feed.\r\n", ch);
		SET_BIT(AFF3_FLAGS(ch), AFF3_FEEDING);
	    } else
		send_to_char("You are already prepared to feed.\r\n", ch);
	} else {                                 /*  !FIGHTING  */
	    if (!*arg1 || !*arg2)
		send_to_char("Feed upon who?\r\n", ch);
	    else if (strncmp(arg1, "on", 2))
		send_to_char("You must enter the command: 'feed on <victim>'.\r\n",ch);
	    else if (!(vict = get_char_room_vis(ch, arg2)))
		send_to_char("You don't see anyone around by that name.\r\n", ch);
	    else if (vict->getPosition() > POS_SLEEPING)
		act("$E is too alert for you to feed on $M.", FALSE, ch, 0, vict, TO_CHAR);
	    else if (IS_UNDEAD(vict))
		send_to_char("Ack!  You cannot feed from another undead!\r\n", ch);
	    else if (IS_ROBOT(vict))
		send_to_char("You cannot feed from a robot.\r\n", ch);
	    else if (GET_COND(ch, THIRST) >= 24 && GET_COND(ch, FULL) >= 20)
		send_to_char("Your belly is too full to feed now.\r\n", ch);
	    else if (CHECK_SKILL(ch, SKILL_FEED) < (number(10, 80)+GET_LEVEL(vict)))
		send_to_char("You are unable to feed.\r\n", ch);
	    else if (!peaceful_room_ok(ch, vict, true))
		send_to_char("You cannot feed in this place.\r\n", ch);
	    else {
		send_to_char("You begin to feed.\r\n", ch);
		act("$n begins to feed on $N.", FALSE, ch, 0, vict, TO_NOTVICT);
		send_to_char("You feel sharp teeth pierce your throat.\r\n", vict);
		amount = GET_LEVEL(vict) + GET_LEVEL(ch) + 
		    (number(0, CHECK_SKILL(ch, SKILL_FEED) / 5));
		GET_HIT(vict) -= amount;
		GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + amount);
		amount /= 10;
		gain_condition(ch, THIRST, amount);
	
		if (GET_COND(ch, THIRST) >= 24) 
		    send_to_char("Your thirst for blood is satiated... for now.\r\n", ch);
		update_pos(vict);

		if (GET_HIT(vict) < -10) {
		    act("You have drained $M completely of life!", FALSE, ch, 0, vict, TO_CHAR);
		    act("You feel a strange sensation as $n drains your life away.", 
			FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
		    raw_kill(vict, ch, SKILL_FEED);
		} else
		    gain_skill_prof(ch, SKILL_FEED);
	    }
	}
    } else {
	if (!*arg1 || !*arg2)
	    send_to_char("Feed who to what?\r\n", ch);
	else if (!(food = get_obj_in_list_vis(ch, arg1, ch->carrying)))
	    send_to_char("You don't have that food.\r\n", ch);
	else if (!((vict = MOUNTED(ch)) && 
		   isname(arg2, vict->player.name)) &&
		 !(vict = get_char_room_vis(ch, arg2)))
	    send_to_char("No-one around by that name.\r\n", ch);
	else if (GET_OBJ_TYPE(food) != ITEM_FOOD)
	    act("I don't think anyone wants to eat $p.", 
		FALSE, ch, food, 0, TO_CHAR);
	else if (!MOB2_FLAGGED(vict, MOB2_MOUNT))
	    act("You cannot feed $p to $N.", FALSE, ch, food, vict, TO_CHAR);
	else if (!AWAKE(vict))
	    act("$N is in no position to be eating.", FALSE,ch,food,vict, TO_CHAR);
	else if (GET_MOVE(vict) >= GET_MAX_MOVE(vict))
	    act("$N must not be hungry -- $E refuses $p.", FALSE, ch, food, vict, TO_CHAR);
	else {
	    act("$n feeds $p to $N.", FALSE, ch, food, vict, TO_NOTVICT);
	    act("$N devours $p.", FALSE, ch, food, vict, TO_CHAR);
	    GET_MOVE(vict) = MIN(GET_MAX_MOVE(vict), GET_MOVE(vict) + 
				 GET_OBJ_VAL(food, 0));
	    extract_obj(food);
	    return;
	}
    }
}
	

ACMD(do_weigh)
{
    struct obj_data *obj = NULL;

    skip_spaces(&argument);

    if (!*argument) {
	send_to_char("Weigh what?\r\n", ch);
	return;
    }

    if (!(obj = get_obj_in_list_vis(ch, argument, ch->carrying))) {
	send_to_char("You can only weigh things which you are carrying.\r\n", ch);
	return;
    }

    act("You carefully judge the weight of $p.",FALSE,ch,obj,0,TO_CHAR);
    act("$n gauges the weight of $p by tossing it in one hand.",
	TRUE, ch, obj, 0, TO_ROOM);

    sprintf(buf, "It seems to weight about %d pounds.\r\n", 
	    (obj->getWeight() + number(-( obj->getWeight() /GET_INT(ch)), 
					  obj->getWeight() / GET_INT(ch))));
    send_to_char(buf, ch);
}

ACMD(do_knock)
{

    struct obj_data *obj = NULL;
    int dir;
    char dname[128], arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    struct room_data *other_room = NULL;
    struct char_data *vict = NULL;
  
    skip_spaces(&argument);
  
    half_chop(argument, arg1, arg2);

    if (!*arg1) {
	send_to_char("Knock on what?\r\n", ch);
	return;
    }

    if ((dir = find_door(ch, arg1, arg2, "knock on")) >= 0) {

	if (EXIT(ch, dir)->keyword)
	    strcpy(dname, fname(EXIT(ch, dir)->keyword));
	else
	    strcpy(dname, "door");

	sprintf(buf, "$n knocks on the %s.", dname);
	act(buf, FALSE, ch, 0, 0, TO_ROOM);
	sprintf(buf, "You knock on the %s.\r\n", dname);
	send_to_char(buf, ch);

	if ((other_room = EXIT(ch, dir)->to_room) && 
	    other_room->dir_option[rev_dir[dir]] &&
	    other_room->dir_option[rev_dir[dir]]->to_room == ch->in_room &&
	    other_room->people) {
	    if (other_room->dir_option[rev_dir[dir]]->keyword)
		strcpy(dname, fname(other_room->dir_option[rev_dir[dir]]->keyword));
	    else
		strcpy(dname, "door");

	    sprintf(buf, "Someone knocks on the %s from the other side.", dname);
	    act(buf, FALSE, other_room->people, 0, 0, TO_CHAR | TO_SLEEP);
	    act(buf, FALSE, other_room->people, 0, 0, TO_ROOM | TO_SLEEP);
	}
    
	return;
    }
  
    if ((vict = get_char_room_vis(ch, arg1))) {
	if (vict == ch) {
	    send_to_char("You rap your knuckles on your head.\r\n", ch);
	    act("$n raps $s knuckles on $s head.", TRUE, ch, 0, 0, TO_CHAR);
	    return;
	}
	act("You knock on $N's skull.  Anybody home?", FALSE,ch,0,vict,TO_CHAR);
	act("$n knocks on your skull.  Anybody home?", 
	    FALSE,ch,0,vict,TO_VICT|TO_SLEEP);
	act("$n knocks on $N's skull.  Anybody home?", FALSE,ch,0,vict,TO_NOTVICT);
	if (vict->getPosition() == POS_SLEEPING && !AFF_FLAGGED(vict, AFF_SLEEP))
	    vict->setPosition( POS_RESTING );
	return;
    }
    
    if (!(obj = get_obj_in_list_vis(ch, argument, ch->carrying)) &&
	!(obj = get_obj_in_list_vis(ch, argument, ch->in_room->contents))) {
	send_to_char("Knock on what?\r\n", ch);
	return;
    }

    act("$n knocks on $p.", FALSE, ch, obj, 0, TO_ROOM);
    act("You knock on $p.", FALSE, ch, obj, 0, TO_CHAR);

    if (IS_VEHICLE(obj)) {
	if ((other_room = real_room(ROOM_NUMBER(obj))) && other_room->people) {
	    act("$N knocks on the outside of $p.", 
		FALSE, other_room->people, obj, ch, TO_NOTVICT);
	    act("$N knocks on the outside of $p.", 
		FALSE, other_room->people, obj, ch, TO_CHAR);
	}
    }
}


ACMD(do_loadroom)
{

    struct room_data *room = NULL;

    skip_spaces(&argument);

    if (!*argument) {

	if (PLR_FLAGGED(ch, PLR_LOADROOM) && 
	    (room = real_room(GET_LOADROOM(ch)))) {
	    sprintf(buf, "Your loadroom is currently set to: %s%s%s\r\n"
		    "Type 'loadroom off' to remove it or 'loadroom set' to set it.\r\n",
		    CCCYN(ch, C_NRM), room->name, CCNRM(ch, C_NRM));
	    send_to_char(buf, ch);
	} else
	    send_to_char("Your loadroom is currently default.\r\n", ch);

	return;
    }

    if (is_abbrev(argument, "off")) {

	REMOVE_BIT(PLR_FLAGS(ch), PLR_LOADROOM);
	GET_LOADROOM(ch) = -1;
	send_to_char("Loadroom disabled.\r\n", ch);

    } else if (is_abbrev(argument, "set")) {

	if ((ROOM_FLAGGED(ch->in_room, ROOM_HOUSE | ROOM_CLAN_HOUSE) ||
	     GET_LEVEL(ch) >= LVL_AMBASSADOR) &&
	    House_can_enter(ch, ch->in_room->number) &&
	    clan_house_can_enter(ch, ch->in_room)) {
	    GET_LOADROOM(ch) = ch->in_room->number;
	    SET_BIT(PLR_FLAGS(ch), PLR_LOADROOM);
	    send_to_char("Okay, you will now load in this room.\r\n", ch);
	} else
	    send_to_char("You cannot load here.\r\n", ch);

    } else
	send_to_char("Usage: loadroom [off | set]\r\n", ch);

}

ACMD(do_gasify)
{
    struct char_data *gas = NULL;
    struct room_data *tank = real_room(13);

    if ( !ch->desc )
        return;

    if (ch->desc->original) {
	send_to_char("You're already switched.\r\n", ch);
	return;
    }

    if (!(gas = read_mobile(1518)) || !tank) {
	send_to_char("Nope.\r\n", ch);
	return;
    }

    IS_CARRYING_N(gas) = 200;
  
    act("$n slowly fades away, leaving only a gaseous cloud behind.",
	TRUE, ch, 0, 0, TO_ROOM);
    send_to_char("You become gaseous.\r\n", ch);

    char_to_room(gas, ch->in_room);
    char_from_room(ch);
    char_to_room(ch, tank);

    ch->desc->character = gas;
    ch->desc->original = ch;
  
    gas->desc = ch->desc;
    ch->desc = NULL;

}

ACMD(do_clean)
{
    struct obj_data *obj=NULL;
    struct char_data *vict=NULL;
    int i, j, k, found, pos;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    argument = two_arguments(argument, arg1, arg2);

    if (!*arg1) {
	send_to_char("Clean who or what?\r\n", ch);
	return;
    }

    if (!(vict = get_char_room_vis(ch, arg1)) &&
	!(obj = get_object_in_equip_vis(ch, arg1, ch->equipment, &i)) &&
	!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying)) &&      
	!(obj = get_obj_in_list_vis(ch, arg1, ch->in_room->contents))) {
	sprintf(buf, "You can't find any '%s' here.\r\n", arg1);
	send_to_char(buf, ch);
	return;
    }

    if (vict) {

	if (!*arg2) {
	    send_to_char("Clean what position?\r\n", ch);
	    return;
	}
	if ((pos = search_block(arg2, wear_eqpos, FALSE)) < 0) {
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
	    send_to_char(buf, ch);
	    return;
	}
	if (ILLEGAL_SOILPOS(pos)) {
	    sprintf(buf, "Hrm.  You can't clean anybody's '%s'!\r\n", 
		    wear_description[pos]);
	    send_to_char(buf, ch);
	    return;
	}
	if (!CHAR_SOILAGE(vict, pos)) {
	    sprintf(buf, "%s not soiled there.", ch == vict ? "You are" : "$e is");
	    act(buf, FALSE, ch, 0, vict, TO_CHAR);
	    return;
	}

	if (vict == ch) {
	    sprintf(buf, "$n carefully cleans $s %s.", wear_description[pos]);
	    act(buf,TRUE,ch,0,0,TO_ROOM);
	    sprintf(buf, "You carefully clean your %s.\r\n",wear_description[pos]); 
	    send_to_char(buf, ch);
	} else {
	    sprintf(buf, "$n carefully cleans $N's %s.", wear_description[pos]);
	    act(buf,TRUE,ch,0,vict,TO_NOTVICT);
	    sprintf(buf, "You carefully clean $N's %s.",wear_description[pos]); 
	    act(buf, FALSE, ch, 0, vict, TO_CHAR);
	    sprintf(buf, "$n carefully cleans your %s.", wear_description[pos]);
	    act(buf, FALSE, ch, 0, vict, TO_VICT);
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
	act(buf2, FALSE, ch, obj, vict, TO_VICT);
	sprintf(buf2, "$N's %s %s %s.", wear_description[pos], 
		pos == WEAR_FEET ? "are" : ISARE(wear_description[pos]),buf);
	act(buf2, TRUE, ch, obj, vict, TO_NOTVICT);
	if (ch != vict)
	    act(buf, TRUE, ch, obj, vict, TO_CHAR);

	CHAR_SOILAGE(vict, pos) = 0;
	return;
    }
  
    /* obj */

    if (!OBJ_SOILAGE(obj))
	act("$p is not soiled.",FALSE,ch,obj,0,TO_CHAR);
    else {
	sprintf(buf, "$n carefully cleans $p. (%s)",
		obj->carried_by ? "carried" : obj->worn_by ? "worn" : "here");
	act(buf, TRUE, ch, obj, 0, TO_ROOM);
	sprintf(buf, "You carefully clean $p. (%s)",
		obj->carried_by ? "carried" : obj->worn_by ? "worn" : "here");
	act(buf, FALSE, ch, obj, 0, TO_CHAR);

	found = 0;
	strcpy(buf, "$p is no longer ");
      
	for (k = 0, j = 0; j < 16; j++)
	    if (OBJ_SOILED(obj, (1 << j)))
		k++;
    
	for (j = 0; j < 16; j++) {
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
	act(buf, FALSE, ch, obj, 0, TO_CHAR);
	act(buf, FALSE, ch, obj, 0, TO_ROOM);
	OBJ_SOILAGE(obj) = 0;
    }
}

ACMD(do_drag)
{
    struct char_data *found_char;
    struct obj_data  *found_obj;
    
    int bits;
 
    bits = generic_find(argument, FIND_OBJ_ROOM | FIND_CHAR_ROOM, ch, &found_char, &found_obj );


    //Target is a character
	if( !bits ) {
		send_to_char( "What do you want to drag?\r\n", ch );
		return;
	} else if( found_char ) {
		do_drag_char(ch, argument, 0, 0);
		return;
    } else if( found_obj ) {
		drag_object( ch, found_obj, argument );
		return;
    }
	
}


void
ice_room(struct room_data *room,int amount)
{
    struct obj_data *ice;
    int new_ice = FALSE;

    if ( (! ( room )) || amount <= 0 ) {
	return;
    }
    
    for ( ice = room->contents; ice; ice = ice->next_content ) {
        if ( GET_OBJ_VNUM( ice ) == ICE_VNUM ) {
            break;
	}
    }
   
    if ( !ice && ( new_ice = TRUE ) && !( ice = read_object( ICE_VNUM ))){
	slog( "SYSERR: Unable to load ice." );
	return;
    }


    if ( GET_OBJ_TIMER( ice ) > 50 ) {
	return;
    }

    GET_OBJ_TIMER( ice ) += amount;
   
    if ( new_ice ) {
	obj_to_room( ice, room );
    }
}
