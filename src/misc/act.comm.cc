/* ************************************************************************
*   File: act.comm.c                                    Part of CircleMUD *
*  Usage: Player-level communication commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: act.comm.c                       -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#define __act_comm_cc__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "structs.h"
#include "spells.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "rpl_resp.h"
#include "character_list.h"

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct zone_data *zone_table;
int Nasty_Words(char *words);
extern int quest_status;

int get_line_count(char *buffer);

//extern struct command_info cmd_info[];

ACMD(do_say)
{
	struct char_data *vict = NULL;
	char name[MAX_INPUT_LENGTH], buf3[MAX_INPUT_LENGTH],
		buf[MAX_STRING_LENGTH];
	struct room_data *was_in = NULL;
	static byte recurs_say = 0;
	int j;
	struct obj_data *o = NULL;

	if PLR_FLAGGED
		(ch, PLR_AFK) {
		send_to_char("You are no longer afk.\r\n", ch);
		REMOVE_BIT(PLR_FLAGS(ch), PLR_AFK);
		}

	if (!*argument) {
		switch (subcmd) {
		case SCMD_BELLOW:
			send_to_char("You bellow loudly.\r\n", ch);
			act("$n begins to bellow loudly.", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case SCMD_EXPOSTULATE:
			send_to_char
				("You expostulate at length about your latest theories.\r\n",
				ch);
			act("$n expostulates at length about $s latest theories.", FALSE,
				ch, 0, 0, TO_ROOM);
			break;
		case SCMD_RAMBLE:
			send_to_char("You ramble endlessly, much to your own delight.\r\n",
				ch);
			act("$n rambles endlessly, much to your chagrin.", FALSE, ch, 0, 0,
				TO_ROOM);
			break;
		case SCMD_BABBLE:
			send_to_char("You babble incoherently for quite some time.\r\n",
				ch);
			act("$n babbles completely incoherently for a while.", FALSE, ch,
				0, 0, TO_ROOM);
			break;
		case SCMD_MURMUR:
			send_to_char("You murmur softly.\r\n", ch);
			act("You are disturbed by a low murmur from $n's vicinity.",
				FALSE, ch, 0, 0, TO_ROOM);
			break;
		case SCMD_SAY_TO:
			send_to_char("Say what to who?\r\n", ch);
			break;
		default:
			send_to_char("Yes, but WHAT do you want to say?\r\n", ch);
		}
		return;
	}

	if (!recurs_say) {
		strcpy(arg, argument);

		for (o = ch->in_room->contents; o; o = o->next_content)
			if (GET_OBJ_TYPE(o) == ITEM_PODIUM)
				break;

		if (o) {

			was_in = ch->in_room;

			for (j = 0; j < NUM_OF_DIRS; j++) {
				if (ABS_EXIT(was_in, j) && ABS_EXIT(was_in, j)->to_room &&
					was_in != ABS_EXIT(was_in, j)->to_room &&
					!IS_SET(ABS_EXIT(was_in, j)->exit_info,
						EX_ISDOOR | EX_CLOSED)) {
					char_from_room(ch);
					char_to_room(ch, ABS_EXIT(was_in, j)->to_room);
					recurs_say = 1;
					do_say(ch, arg, cmd, subcmd);
				}
			}

			char_from_room(ch);
			char_to_room(ch, was_in);

			recurs_say = 0;
		}
	}

	if (subcmd != SCMD_SAY_TO)
		skip_spaces(&argument);

	delete_doubledollar(argument);

	if (subcmd == SCMD_SAY_TO) {
		argument = one_argument(argument, name);
		skip_spaces(&argument);

		if (!*name)
			send_to_char("Say what to who?\r\n", ch);
		else if (!(vict = get_char_room_vis(ch, name))) {
			if (!recurs_say)
				send_to_char("No-one by that name here.\r\n", ch);
		} else {

			CharacterList::iterator it = ch->in_room->people.begin();
			for (; it != ch->in_room->people.end(); ++it) {
				if (!AWAKE((*it)) || (*it) == ch ||
					((*it)->desc && (*it)->desc->showstr_point &&
						!PRF2_FLAGGED((*it), PRF2_LIGHT_READ)) ||
					PLR_FLAGGED((*it), PLR_OLC | PLR_WRITING | PLR_MAILING))
					continue;
				strcpy(buf, PERS(ch, (*it)));
				strcpy(buf2, CAP(buf));
				if ((*it) == vict)
					strcpy(buf3, "you");
				else if (vict == ch) {
					strcpy(buf3, HMHR(ch));
					strcat(buf3, "self");
				} else
					strcpy(buf3, PERS(vict, (*it)));
				sprintf(buf, "%s%s%s%s says to %s,%s %s'%s'%s\r\n",
					recurs_say ? "(remote) " : "", CCBLD((*it), C_NRM),
					CCBLU((*it), C_SPR), buf2, buf3, CCNRM((*it), C_SPR),
					CCCYN((*it), C_NRM), argument, CCNRM((*it), C_NRM));
				send_to_char(buf, (*it));
			}
			if (!recurs_say) {
				if (PRF_FLAGGED(ch, PRF_NOREPEAT))
					send_to_char(OK, ch);
				else {
					sprintf(buf, "%s%sYou say to %s,%s %s'%s'%s\r\n", CCBLD(ch,
							C_NRM), CCBLU(ch, C_NRM), GET_DISGUISED_NAME(ch,
							vict), CCNRM(ch, C_NRM), CCCYN(ch, C_NRM),
						argument, CCNRM(ch, C_NRM));
					send_to_char(buf, ch);
				}
			}
		}
		return;
	}

	/* NOT say_to stuff: ********************************************* */
	CharacterList::iterator it = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end(); ++it) {
		if (!AWAKE((*it)) || (*it) == ch ||
			((*it)->desc && (*it)->desc->showstr_point &&
				!PRF2_FLAGGED((*it), PRF2_LIGHT_READ)) ||
			PLR_FLAGGED((*it), PLR_OLC | PLR_MAILING | PLR_WRITING))
			continue;
		strcpy(buf, PERS(ch, (*it)));
		strcpy(buf2, CAP(buf));
		sprintf(buf, "%s%s%s%s %s,%s %s'%s'%s\r\n",
			recurs_say ? "(remote) " : "", CCBLD((*it), C_NRM), CCBLU((*it),
				C_SPR), buf2,
			(subcmd == SCMD_UTTER ? "utters" : subcmd ==
				SCMD_EXPOSTULATE ? "expostulates" : subcmd ==
				SCMD_RAMBLE ? "rambles" : subcmd ==
				SCMD_BELLOW ? "bellows" : subcmd ==
				SCMD_MURMUR ? "murmurs" : subcmd ==
				SCMD_INTONE ? "intones" : subcmd ==
				SCMD_YELL ? "yells" : subcmd ==
				SCMD_BABBLE ? "babbles" : "says"), CCNRM((*it), C_SPR),
			CCCYN((*it), C_NRM), argument, CCNRM((*it), C_NRM));
		send_to_char(buf, (*it));
	}
	if (!recurs_say) {
		if (PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(OK, ch);
		else {
			sprintf(buf, "%s%sYou %s,%s %s'%s'%s\r\n", CCBLD(ch, C_NRM),
				CCBLU(ch, C_SPR),
				(subcmd == SCMD_UTTER ? "utter" :
					subcmd == SCMD_EXPOSTULATE ? "expostulate" :
					subcmd == SCMD_RAMBLE ? "ramble" :
					subcmd == SCMD_BELLOW ? "bellow" :
					subcmd == SCMD_MURMUR ? "murmur" :
					subcmd == SCMD_INTONE ? "intone" :
					subcmd == SCMD_YELL ? "yell" :
					subcmd == SCMD_BABBLE ? "babble" : "say"),
				CCNRM(ch, C_SPR), CCCYN(ch, C_NRM),
				argument, CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
		}
	}
}


ACMD(do_gsay)
{
	struct char_data *k;
	struct follow_type *f;

	skip_spaces(&argument);

	if (!IS_AFFECTED(ch, AFF_GROUP)) {
		send_to_char("But you are not the member of a group!\r\n", ch);
		return;
	}
	if (!*argument)
		send_to_char("Yes, but WHAT do you want to group-say?\r\n", ch);
	else {
		if (ch->master)
			k = ch->master;
		else
			k = ch;


		if (IS_AFFECTED(k, AFF_GROUP) && (k != ch) && !COMM_NOTOK_ZONES(ch, k)) {
			sprintf(buf, "%s$n tells the group,%s '%s'%s", CCGRN(k, C_NRM),
				CCYEL(k, C_NRM), argument, CCNRM(k, C_NRM));
			act(buf, FALSE, ch, 0, k, TO_VICT | TO_SLEEP);
		}
		for (f = k->followers; f; f = f->next)
			if (IS_AFFECTED(f->follower, AFF_GROUP) && (f->follower != ch) &&
				!COMM_NOTOK_ZONES(ch, f->follower)) {
				sprintf(buf, "%s$n tells the group,%s '%s'%s",
					CCGRN(f->follower, C_NRM), CCYEL(f->follower, C_NRM),
					argument, CCNRM(f->follower, C_NRM));
				act(buf, FALSE, ch, 0, f->follower, TO_VICT | TO_SLEEP);
			}

		if (PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(OK, ch);
		else {
			sprintf(buf, "%sYou tell the group,%s '%s'%s", CCGRN(ch, C_NRM),
				CCYEL(ch, C_NRM), argument, CCNRM(ch, C_NRM));
			act(buf, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
		}
	}
}


void
perform_tell(struct char_data *ch, struct char_data *vict, char *arg)
{
	char buf2[1024];

	if (PRF_FLAGGED(ch, PRF_NOREPEAT))
		send_to_char(OK, ch);
	else {
		sprintf(buf, "%sYou tell $N,%s '%s'",
			CCRED(ch, C_NRM), CCNRM(ch, C_NRM), arg);
		act(buf, FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	}

	delete_doubledollar(arg);

	if (!IS_NPC(vict)) {
		strcpy(buf, CCRED(vict, C_NRM));
		sprintf(buf2, "%s tells you,%s '%s'\r\n",
			PERS(ch, vict), CCNRM(vict, C_NRM), arg);
		strcat(buf, CAP(buf2));
		send_to_char(buf, vict);
	} else {
		sprintf(buf, "$n tells you, '%s'", arg);
		act(buf, FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
	}
	if (PRF2_FLAGGED(vict, PRF2_AUTOPAGE) && !IS_MOB(ch))
		send_to_char("\007\007", vict);

	GET_LAST_TELL(vict) = GET_IDNUM(ch);
}

/*
 * Yes, do_tell probably could be combined with whisper and ask, but
 * called frequently, and should IMHO be kept as tight as possible.
 */
ACMD(do_tell)
{
	struct char_data *vict;

	char buf2[MAX_INPUT_LENGTH];
	half_chop(argument, buf, buf2);

	if (!*buf || !*buf2)
		send_to_char("Who do you wish to tell what??\r\n", ch);
	else if (!(vict = get_player_vis(ch, buf, 1)) &&
		!(vict = get_player_vis(ch, buf, 0))) {
		send_to_char(NOPERSON, ch);
	} else if (ch == vict)
		send_to_char("You try to tell yourself something.\r\n", ch);
	else if (PRF_FLAGGED(ch, PRF_NOTELL) && GET_LEVEL(ch) < LVL_AMBASSADOR)
		send_to_char
			("You can't tell other people while you have notell on.\r\n", ch);
	else if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)
		&& GET_LEVEL(ch) < LVL_GRGOD && ch->in_room != vict->in_room)
		send_to_char("The walls seem to absorb your words.\r\n", ch);
	else if (!IS_NPC(vict) && !vict->desc)	/* linkless */
		act("$E's linkless at the moment.", FALSE, ch, 0, vict,
			TO_CHAR | TO_SLEEP);
	else if (PLR_FLAGGED(vict, PLR_WRITING | PLR_MAILING))
		act("$E's writing a message right now; try again later.",
			FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else if ((PRF_FLAGGED(vict, PRF_NOTELL) ||
			PLR_FLAGGED(vict, PLR_OLC) ||
			(ROOM_FLAGGED(vict->in_room, ROOM_SOUNDPROOF) &&
				ch->in_room != vict->in_room)) &&
		!(GET_LEVEL(ch) >= LVL_GRGOD && GET_LEVEL(ch) > GET_LEVEL(vict)))
		act("$E can't hear you.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else if (COMM_NOTOK_ZONES(ch, vict))
		act("Your telepathic voice cannot reach $M.",
			FALSE, ch, 0, vict, TO_CHAR);
	else
		perform_tell(ch, vict, buf2);
}


ACMD(do_reply)
{
	//struct char_data *tch = character_list;
	CharacterList::iterator tch = characterList.begin();
	skip_spaces(&argument);

	if (GET_LAST_TELL(ch) == NOBODY)
		send_to_char("You have no-one to reply to!\r\n", ch);
	else if (!*argument)
		send_to_char("What is your reply?\r\n", ch);
	else {
		/*
		 * Make sure the person you're replying to is still playing by searching
		 * for them.  Note, now last tell is stored as player IDnum instead of
		 * a pointer, which is much better because it's safer, plus will still
		 * work if someone logs out and back in again.
		 */

		while (tch != characterList.end()
			&& GET_IDNUM(*tch) != GET_LAST_TELL(ch))
			++tch;

		if (tch == characterList.end())
			send_to_char("They are no longer playing.\r\n", ch);
		else if (!IS_NPC(*tch) && (*tch)->desc == NULL)
			send_to_char("They are linkless at the moment.\r\n", ch);
		else if (PLR_FLAGGED(*tch, PLR_WRITING | PLR_MAILING | PLR_OLC))
			send_to_char("They are writing at the moment.\r\n", ch);
		else if (COMM_NOTOK_ZONES(ch, (*tch)) && COMM_NOTOK_ZONES((*tch), ch))
			act("Your telepathic voice cannot reach $M.",
				FALSE, ch, 0, (*tch), TO_CHAR);
		else
			perform_tell(ch, (*tch), argument);
	}
}


ACMD(do_spec_comm)
{
	struct char_data *vict;
	struct extra_descr_data *rpl_ptr;
	char *action_sing, *action_plur, *action_others, *word;
	short found;


	if (subcmd == SCMD_WHISPER) {
		action_sing = "whisper to";
		action_plur = "whispers to";
		action_others = "$n whispers something to $N.";
	} else if (subcmd == SCMD_RESPOND) {
		action_sing = "respond to";
		action_plur = "responds to";
		action_others = "$n responds to $N.";
	} else {
		action_sing = "ask";
		action_plur = "asks";
		action_others = "$n asks $N a question.";
	}

	half_chop(argument, buf, buf2);

	if (!*buf || !*buf2) {
		sprintf(buf, "Whom do you want to %s.. and what??\r\n", action_sing);
		send_to_char(buf, ch);
	} else if (!(vict = get_char_room_vis(ch, buf))) {
		send_to_char(NOPERSON, ch);
	} else if (vict == ch)
		send_to_char
			("You can't get your mouth close enough to your ear...\r\n", ch);
	else {
		sprintf(buf, "%s$n %s you,%s '%s'", CCYEL(vict, C_NRM), action_plur,
			CCNRM(vict, C_NRM), buf2);
		act(buf, FALSE, ch, 0, vict, TO_VICT);
		if (PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(OK, ch);
		else {
			sprintf(buf, "%sYou %s %s,%s '%s'", CCYEL(ch, C_NRM), action_sing,
				GET_DISGUISED_NAME(ch, vict), CCNRM(ch, C_NRM), buf2);
			act(buf, FALSE, ch, 0, 0, TO_CHAR);
		}
		act(action_others, FALSE, ch, 0, vict, TO_NOTVICT);

		/* add the ask mob <txt> and the mob will reply. -Sarflin 7-19-95 */

		if (subcmd != SCMD_WHISPER && (vict != 0) && IS_NPC(vict) &&
			vict->mob_specials.response != NULL) {
			rpl_ptr = vict->mob_specials.response;
			word = NULL;
			found = 0;
			word = strtok(buf2, " ");
			if (word != NULL) {
				do {
					rpl_ptr = vict->mob_specials.response;
					do {
						if (strstr(rpl_ptr->keyword, word) != NULL) {
							found = 1;
							break;
						}
						rpl_ptr = rpl_ptr->next;
					} while (rpl_ptr != NULL);

					if (found)
						break;
					word = strtok(NULL, " ");
				} while (word != NULL);
			}
			if (found) {
				reply_respond(ch, vict, rpl_ptr->description);
			} else {
				sprintf(buf,
					"%s%s tells you:%s\r\n   I don't know anything about that!",
					CCRED(ch, C_NRM), GET_NAME(vict), CCNRM(ch, C_NRM));
				act(buf, FALSE, ch, 0, 0, TO_CHAR);
			}
		}
	}
}


#define MAX_NOTE_LENGTH 16384	/* arbitrary */

ACMD(do_write)
{
	struct obj_data *paper = 0, *pen = 0;
	char *papername, *penname;

	papername = buf1;
	penname = buf2;

	two_arguments(argument, papername, penname);

	if (!ch->desc)
		return;

	if (!*papername) {			/* nothing was delivered */
		send_to_char
			("Write?  With what?  ON what?  What are you trying to do?!?\r\n",
			ch);
		return;
	}
	if (*penname) {				/* there were two arguments */
		if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying))) {
			sprintf(buf, "You have no %s.\r\n", papername);
			send_to_char(buf, ch);
			return;
		}
		if (!(pen = get_obj_in_list_vis(ch, penname, ch->carrying))) {
			sprintf(buf, "You have no %s.\r\n", penname);
			send_to_char(buf, ch);
			return;
		}
	} else {					/* there was one arg.. let's see what we can find */
		if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying))) {
			sprintf(buf, "There is no %s in your inventory.\r\n", papername);
			send_to_char(buf, ch);
			return;
		}
		if (GET_OBJ_TYPE(paper) == ITEM_PEN) {	/* oops, a pen.. */
			pen = paper;
			paper = 0;
		} else if (GET_OBJ_TYPE(paper) != ITEM_NOTE) {
			send_to_char("That thing has nothing to do with writing.\r\n", ch);
			return;
		}
		/* One object was found.. now for the other one. */
		if (!GET_EQ(ch, WEAR_HOLD)) {
			sprintf(buf, "You can't write with %s %s alone.\r\n",
				AN(papername), papername);
			send_to_char(buf, ch);
			return;
		}
		if (!CAN_SEE_OBJ(ch, GET_EQ(ch, WEAR_HOLD))) {
			send_to_char("The stuff in your hand is invisible!  Yeech!!\r\n",
				ch);
			return;
		}
		if (pen)
			paper = GET_EQ(ch, WEAR_HOLD);
		else
			pen = GET_EQ(ch, WEAR_HOLD);
	}


	/* ok.. now let's see what kind of stuff we've found */
	if (GET_OBJ_TYPE(pen) != ITEM_PEN) {
		act("$p is no good for writing with.", FALSE, ch, pen, 0, TO_CHAR);
	} else if (GET_OBJ_TYPE(paper) != ITEM_NOTE) {
		act("You can't write on $p.", FALSE, ch, paper, 0, TO_CHAR);
	} else if (paper->action_description) {
		send_to_char("There's something written on it already.\r\n", ch);
	} else {
		if (paper->action_description == NULL)
			CREATE(paper->action_description, char, MAX_NOTE_LENGTH);
		start_text_editor(ch->desc, &paper->action_description, true,
			MAX_NOTE_LENGTH);
		SET_BIT(PLR_FLAGS(ch), PLR_WRITING);
		act("$n begins to jot down a note..", TRUE, ch, 0, 0, TO_ROOM);
	}
}



ACMD(do_page)
{
	struct descriptor_data *d;
	struct char_data *vict;

	half_chop(argument, arg, buf2);

	if (IS_NPC(ch))
		send_to_char("Monsters can't page.. go away.\r\n", ch);
	else if (!*arg)
		send_to_char("Whom do you wish to page?\r\n", ch);
	else {
		sprintf(buf, "\007\007*%s* %s\r\n", GET_NAME(ch), buf2);
		if (!str_cmp(arg, "all")) {
			if (GET_LEVEL(ch) > LVL_GOD) {
				for (d = descriptor_list; d; d = d->next)
					if (IS_PLAYING(d) && d->character) {
						send_to_char(CCYEL(d->character, C_NRM), d->character);
						send_to_char(CCBLD(d->character, C_SPR), d->character);
						act(buf, FALSE, ch, 0, d->character, TO_VICT);
						send_to_char(CCNRM(d->character, C_SPR), d->character);
					}
			} else
				send_to_char("You will never be godly enough to do that!\r\n",
					ch);
			return;
		}
		if ((vict = get_char_vis(ch, arg)) != NULL) {
			send_to_char(CCYEL_BLD(vict, C_SPR), vict);
			act(buf, FALSE, ch, 0, vict, TO_VICT);
			send_to_char(CCYEL_BLD(vict, C_SPR), vict);
			if (PRF_FLAGGED(ch, PRF_NOREPEAT))
				send_to_char(OK, ch);
			else {
				send_to_char(CCYEL(vict, C_NRM), ch);
				send_to_char(CCBLD(vict, C_SPR), ch);
				act(buf, FALSE, ch, 0, vict, TO_CHAR);
				send_to_char(CCNRM(vict, C_SPR), vict);
			}

			return;
		} else
			send_to_char("There is no such person in the game!\r\n", ch);
	}
}


/**********************************************************************
 * generalized communication func, originally by Fred C. Merkel (Torg) *
  *********************************************************************/

ACMD(do_gen_comm)
{
	extern int level_can_shout;
	extern int holler_move_cost;
	struct descriptor_data *i;
	char color_on1[24], color_on2[24], buf2[MAX_STRING_LENGTH],
		buf[MAX_STRING_LENGTH];

	/* Array of flags which must _not_ be set in order for comm to be heard */
	static int channels[] = {
		0,
		PRF_DEAF,
		PRF_NOGOSS,
		PRF_NOAUCT,
		PRF_NOGRATZ,
		PRF_NOMUSIC,
		PRF_NOSPEW,
		PRF_NODREAM,
		PRF_NOPROJECT,
		0
	};

	/*
	 * com_msgs: [0] Message if you can't perform the action because of noshout
	 *           [1] name of the action
	 *           [2] message if you're not on the channel
	 *           [3] a color string.
	 */
	static char *com_msgs[][5] = {
		{"You cannot holler!!\r\n",
				"holler",
				"You aren't on the holler channel.\r\n",
			KYEL_BLD, KRED},

		{"You cannot shout!!\r\n",
				"shout",
				"Turn off your noshout flag first!\r\n",
			KYEL, KCYN},

		{"You cannot gossip!!\r\n",
				"gossip",
				"You aren't even on the channel!\r\n",
			KGRN, KNUL},

		{"You cannot auction!!\r\n",
				"auction",
				"You aren't even on the channel!\r\n",
			KMAG, KNRM},

		{"You cannot congratulate!\r\n",
				"congrat",
				"You aren't even on the channel!\r\n",
			KGRN, KMAG},

		{"You cannot sing!!\r\n",
				"sing",
				"You aren't even on the channel!\r\n",
			KCYN, KYEL},

		{"You cannot spew!!\r\n",
				"spew",
				"You aren't even on the channel!\r\n",
			KRED, KYEL},

		{"You cannot dream.  You are muted.\r\n",
				"dream",
				"You aren't even on the dream channel!\r\n",
			KCYN, KNRM_BLD},

		{"You cannot project.  The gods have muted you.\r\n",
				"project",
				"You are not open to projections yourself...\r\n",
			KNRM_BLD, KCYN},

		{"You are muted.\r\n",
				"newbie",
				"",
			KYEL, KNRM},

	};

	/* to keep pets, etc from being ordered to shout  */

	if (!ch->desc && ch->master && (subcmd == SCMD_PROJECT ||
			subcmd == SCMD_HOLLER))
		return;

	if ((GET_COND(ch, DRUNK) > 5) && (number(0, 3) >= 2)) {
		sprintf(buf1,
			"You try to %s, but somehow it just doesn't come out right.\r\n",
			subcmd == SCMD_GOSSIP ? "gossip" : subcmd ==
			SCMD_AUCTION ? "auction" : subcmd ==
			SCMD_GRATZ ? "congratulate" : subcmd ==
			SCMD_MUSIC ? "sing" : subcmd == SCMD_SPEW ? "spew" : subcmd ==
			SCMD_DREAM ? "dream" : subcmd ==
			SCMD_PROJECT ? "project" : subcmd ==
			SCMD_NEWBIE ? "newbie" : "speak");
		send_to_char(buf1, ch);
		return;
	}
	if (subcmd == SCMD_PROJECT && !IS_NPC(ch) && CHECK_REMORT_CLASS(ch) < 0 &&
		GET_LEVEL(ch) < LVL_AMBASSADOR) {
		send_to_char("You do not know how to project yourself that way.\r\n",
			ch);
		return;
	}
	if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
		send_to_char(com_msgs[subcmd][0], ch);
		return;
	}
	if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)
		&& GET_LEVEL(ch) < LVL_GRGOD) {
		send_to_char("The walls seem to absorb your words.\r\n", ch);
		return;
	}
	/* level_can_shout defined in config.c */
	if (!IS_NPC(ch) && GET_LEVEL(ch) < level_can_shout
		&& subcmd != SCMD_NEWBIE) {
		sprintf(buf1, "You must be at least level %d before you can %s.\r\n",
			level_can_shout, com_msgs[subcmd][1]);
		send_to_char(buf1, ch);
		send_to_char("Try using the newbie channel instead.\r\n", ch);
		return;
	}
	if (!IS_NPC(ch)) {
		/* make sure the char is on the channel */
		if (PRF_FLAGGED(ch, channels[subcmd])) {
			send_to_char(com_msgs[subcmd][2], ch);
			return;
		}
		if (subcmd == SCMD_HOLLER && PRF2_FLAGGED(ch, PRF2_NOHOLLER)) {
			send_to_char("Ha!  You are noholler buddy.\r\n", ch);
			return;
		}

		if (subcmd == SCMD_DREAM &&
			(ch->getPosition() != POS_SLEEPING)
			&& GET_LEVEL(ch) < LVL_IMMORT) {
			send_to_char
				("You attempt to dream, but realize you need to sleep first.\r\n",
				ch);
			return;
		}

		if (subcmd == SCMD_NEWBIE && !PRF2_FLAGGED(ch, PRF2_NEWBIE_HELPER)) {
			send_to_char("You aren't on the illustrious newbie channel.\r\n",
				ch);
			return;
		}
	}
	/* skip leading spaces */
	skip_spaces(&argument);

	/* make sure that there is something there to say! */
	if (!*argument) {
		sprintf(buf1, "Yes, %s, fine, %s we must, but WHAT???\r\n",
			com_msgs[subcmd][1], com_msgs[subcmd][1]);
		send_to_char(buf1, ch);
		return;
	}

	if (subcmd == SCMD_HOLLER && GET_LEVEL(ch) < LVL_TIMEGOD) {
		if (GET_MOVE(ch) < holler_move_cost) {
			send_to_char("You're too exhausted to holler.\r\n", ch);
			return;
		} else
			GET_MOVE(ch) -= holler_move_cost;
	}
	/* set up the color on code */
	strcpy(color_on1, com_msgs[subcmd][3]);
	if (com_msgs[subcmd][4])
		strcpy(color_on2, com_msgs[subcmd][4]);

	/* first, set up strings to be given to the communicator */
	if (PRF_FLAGGED(ch, PRF_NOREPEAT))
		send_to_char(OK, ch);
	else {
		if (COLOR_LEV(ch) >= C_NRM)
			sprintf(buf1, "%sYou %s,%s%s '%s'%s", color_on1,
				com_msgs[subcmd][1], KNRM, color_on2, argument, KNRM);
		else
			sprintf(buf1, "You %s, '%s'", com_msgs[subcmd][1], argument);
		act(buf1, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);

		/* see if it's dirty! */
		if (!IS_MOB(ch) && GET_LEVEL(ch) < LVL_GRGOD &&
			Nasty_Words(argument) && subcmd != SCMD_SPEW) {
			switch (number(0, 2)) {
			case 0:
				send_to_char
					("Unless you like being muted, use SPEW for profanity.\r\n",
					ch);
				break;
			case 1:
				send_to_char
					("Why don't you keep that smut on the spew channel, eh?\r\n",
					ch);
				break;
			case 2:
				send_to_char
					("I don't think everyone wants to hear that.  SPEW it!\r\n",
					ch);
				break;
			}
			sprintf(buf, "%s warned for nasty public communication.",
				GET_NAME(ch));
			slog(buf);
		}
	}

	sprintf(buf, "$n %ss, '%s'", com_msgs[subcmd][1], argument);
	sprintf(buf2, "$n %ss,%s%s '%s'", com_msgs[subcmd][1], KNRM, color_on2,
		argument);

	/* now send all the strings out */
	for (i = descriptor_list; i; i = i->next) {
		if (STATE(i) == CON_PLAYING && i != ch->desc && i->character &&
			!PRF_FLAGGED(i->character, channels[subcmd]) &&
			!PLR_FLAGGED(i->character, PLR_WRITING) &&
			!PLR_FLAGGED(i->character, PLR_OLC) &&
			(!i->showstr_point || PRF2_FLAGGED(ch, PRF2_LIGHT_READ)) &&
			!ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF)) {

			if (subcmd == SCMD_NEWBIE &&
				!PRF2_FLAGGED(i->character, PRF2_NEWBIE_HELPER))
				continue;

			if (subcmd == SCMD_HOLLER &&
				PRF2_FLAGGED(i->character, PRF2_NOHOLLER))
				continue;

			if (IS_NPC(ch) || GET_LEVEL(i->character) < LVL_AMBASSADOR) {
				if (subcmd == SCMD_SHOUT &&
					((ch->in_room->zone != i->character->in_room->zone) ||
						i->character->getPosition() < POS_RESTING))
					continue;

				if (subcmd == SCMD_PROJECT &&
					CHECK_REMORT_CLASS(i->character) < 0 &&
					GET_LEVEL(i->character) < LVL_AMBASSADOR)
					continue;

				if (subcmd == SCMD_DREAM &&
					i->character->getPosition() != POS_SLEEPING)
					continue;

				if ((subcmd == SCMD_GOSSIP || subcmd == SCMD_AUCTION ||
						subcmd == SCMD_GRATZ || subcmd == SCMD_DREAM ||
						subcmd == SCMD_MUSIC || subcmd == SCMD_SPEW ||
						subcmd == SCMD_NEWBIE) &&
					COMM_NOTOK_ZONES(ch, i->character))
					continue;

			}

			if (COLOR_LEV(i->character) >= C_NRM) {
				send_to_char(color_on1, i->character);
				act(buf2, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
				send_to_char(KNRM, i->character);
			} else
				act(buf, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
		}
	}
}

ACMD(do_auction)
{
	extern int level_can_shout;

	// Is this a mob?
	if (IS_NPC(ch))
		return;

	// Is the player muted?
	if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
		send_to_char("You cannot auction!!\r\n", ch);
		return;
	}
	// Is the room soundproof?  Not sure how valid this check is now, but I don't
	// see any good reason not to use it.
	if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)
		&& GET_LEVEL(ch) < LVL_GRGOD) {
		send_to_char("The walls seem to absorb your words.\r\n", ch);
		return;
	}
	// level_can_shout defined in config.c
	// Is ch over level_can_shout
	if (!IS_NPC(ch) && GET_LEVEL(ch) < level_can_shout
		&& subcmd != SCMD_NEWBIE) {
		sprintf(buf1,
			"You must be at least level %d before you can auction.\r\n",
			level_can_shout);
		send_to_char(buf1, ch);
		return;
	}

	skip_spaces(&argument);

	if (!*argument) {
		sprintf(buf, "Auction?  Yes, fine, auction we must, but WHAT??\r\n");
		CAP(buf);
		send_to_char(buf, ch);
	}
	return;
}

ACMD(do_qcomm)
{
	struct descriptor_data *i;

	if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_QUEST)) {
		send_to_char("You aren't even part of the quest!\r\n", ch);
		return;
	}

	if (GET_LEVEL(ch) < LVL_IMMORT && !quest_status) {
		send_to_char("There are no active quests at the moment.\r\n", ch);
		return;
	}

	skip_spaces(&argument);

	if (!*argument) {
		sprintf(buf, "%s?  Yes, fine, %s we must, but WHAT??\r\n", CMD_NAME,
			CMD_NAME);
		CAP(buf);
		send_to_char(buf, ch);
	} else if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
		send_to_char("You cannot quest say, sorry!\r\n", ch);
	} else {
		if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
			send_to_char("The walls absorb the sound of your voice.\r\n", ch);
		else if (PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(OK, ch);
		else {
			if (subcmd == SCMD_QSAY) {
				sprintf(buf, "%s%sYou quest-say,%s '%s'\r\n", CCYEL(ch, C_NRM),
					CCBLD(ch, C_SPR), CCNRM(ch, C_SPR), argument);
				delete_doubledollar(buf);
				send_to_char(buf, ch);
			} else {
				strcpy(buf, argument);
				strcat(buf, CCNRM(ch, C_NRM));
				strcat(buf, "\r\n");
				send_to_char(CCYEL_BLD(ch, C_NRM), ch);
				send_to_char(buf, ch);
			}
		}

		if (subcmd == SCMD_QSAY) {
			for (i = descriptor_list; i; i = i->next)
				if (STATE(i) == CON_PLAYING && i != ch->desc &&
					PRF_FLAGGED(i->character, PRF_QUEST) &&
					i->character->in_room != NULL &&
					!ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF) &&
					(!i->showstr_point || PRF2_FLAGGED(ch, PRF2_LIGHT_READ)) &&
					!PLR_FLAGGED(i->character,
						PLR_MAILING | PLR_WRITING | PLR_OLC)) {
					strcpy(buf, CCYEL_BLD(i->character, C_SPR));
					sprintf(buf2, "%s quest-says,%s '%s'\r\n", PERS(ch,
							i->character), CCNRM(i->character, C_SPR),
						argument);
					strcat(buf, CAP(buf2));
					send_to_char(buf, i->character);
				}
		} else {
			strcpy(buf, argument);

			for (i = descriptor_list; i; i = i->next)
				if (STATE(i) == CON_PLAYING && i != ch->desc &&
					PRF_FLAGGED(i->character, PRF_QUEST) &&
					i->character->in_room != NULL &&
					!ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF) &&
					(!i->showstr_point || PRF2_FLAGGED(ch, PRF2_LIGHT_READ)) &&
					!PLR_FLAGGED(i->character,
						PLR_MAILING | PLR_WRITING | PLR_OLC)) {
					send_to_char(CCYEL_BLD(i->character, C_NRM), i->character);
					act(buf, 0, ch, 0, i->character, TO_VICT | TO_SLEEP);
					send_to_char(CCNRM(i->character, C_NRM), i->character);
				}
		}
	}
}

ACMD(do_clan_comm)
{
	struct descriptor_data *i;

	if (!GET_CLAN(ch)) {
		send_to_char("You aren't even a member of a clan!\r\n", ch);
		return;
	}
	skip_spaces(&argument);

	if (!*argument) {
		sprintf(buf, "%s?  Yes, fine, %s we must, but WHAT??\r\n", CMD_NAME,
			CMD_NAME);
		CAP(buf);
		send_to_char(buf, ch);
	} else {
		if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF) &&
			GET_LEVEL(ch) < LVL_AMBASSADOR)
			send_to_char("The walls absorb the sound of your voice.\r\n", ch);
		else if (PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(OK, ch);
		else {
			if (subcmd == SCMD_CLAN_SAY) {
				delete_doubledollar(buf);
				sprintf(buf, "%sYou clan-say, %s'%s'\r\n", CCCYN(ch, C_NRM),
					CCNRM(ch, C_NRM), argument);
			} else
				sprintf(buf, "%s%s %s%s\r\n", CCCYN(ch, C_NRM), GET_NAME(ch),
					CCNRM(ch, C_NRM), argument);
			send_to_char(buf, ch);
		}

		if (subcmd == SCMD_CLAN_SAY) {
			for (i = descriptor_list; i; i = i->next)
				if (STATE(i) == CON_PLAYING && i != ch->desc && i->character &&
					GET_CLAN(i->character) &&
					GET_CLAN(i->character) == GET_CLAN(ch) &&
					!PRF_FLAGGED(i->character, PRF_NOCLANSAY) &&
					i->character->in_room != NULL &&
					!COMM_NOTOK_ZONES(ch, i->character) &&
					(!i->showstr_point || PRF2_FLAGGED(ch, PRF2_LIGHT_READ)) &&
					!PLR_FLAGGED(i->character,
						PLR_MAILING | PLR_WRITING | PLR_OLC)) {
					strcpy(buf, CCCYN(i->character, C_NRM));
					sprintf(buf2, "%s clan-says, %s'%s'\r\n", PERS(ch,
							i->character), CCNRM(i->character, C_NRM),
						argument);
					strcat(buf, CAP(buf2));
					send_to_char(buf, i->character);
				}
		} else {
			for (i = descriptor_list; i; i = i->next)
				if (STATE(i) == CON_PLAYING && i != ch->desc && i->character &&
					GET_CLAN(i->character) &&
					GET_CLAN(i->character) == GET_CLAN(ch) &&
					!PRF_FLAGGED(i->character, PRF_NOCLANSAY) &&
					i->character->in_room != NULL &&
					!COMM_NOTOK_ZONES(ch, i->character) &&
					(!i->showstr_point || PRF2_FLAGGED(ch, PRF2_LIGHT_READ)) &&
					!PLR_FLAGGED(i->character,
						PLR_MAILING | PLR_WRITING | PLR_OLC)) {
					sprintf(buf, "$n%s %s", CCNRM(i->character, C_NRM),
						argument);
					send_to_char(CCCYN(i->character, C_NRM), i->character);
					act(buf, 0, ch, 0, i->character, TO_VICT | TO_SLEEP);
				}
		}
	}
}

#undef __act_comm_cc__
