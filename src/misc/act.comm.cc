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
#include "creature_list.h"
#include "clan.h"
#include "security.h"
#include "tmpstr.h"

/* extern variables */
extern const char *pc_char_class_types[];
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct zone_data *zone_table;
int Nasty_Words(char *words);
extern int quest_status;

int parse_player_class(char *arg);
//extern struct command_info cmd_info[];

ACMD(do_say)
{
	struct Creature *vict = NULL;
	char name[MAX_INPUT_LENGTH], buf3[MAX_INPUT_LENGTH],
		buf[MAX_STRING_LENGTH];
	struct room_data *was_in = NULL;
	static byte recurs_say = 0;
	int j;
	struct obj_data *o = NULL;

	if PLR_FLAGGED
		(ch, PLR_AFK) {
		send_to_char(ch, "You are no longer afk.\r\n");
		REMOVE_BIT(PLR_FLAGS(ch), PLR_AFK);
		}

	if (!*argument) {
		switch (subcmd) {
		case SCMD_BELLOW:
			send_to_char(ch, "You bellow loudly.\r\n");
			act("$n begins to bellow loudly.", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case SCMD_EXPOSTULATE:
			send_to_char(ch, 
				"You expostulate at length about your latest theories.\r\n");
			act("$n expostulates at length about $s latest theories.", FALSE,
				ch, 0, 0, TO_ROOM);
			break;
		case SCMD_RAMBLE:
			send_to_char(ch, "You ramble endlessly, much to your own delight.\r\n");
			act("$n rambles endlessly, much to your chagrin.", FALSE, ch, 0, 0,
				TO_ROOM);
			break;
		case SCMD_BABBLE:
			send_to_char(ch, "You babble incoherently for quite some time.\r\n");
			act("$n babbles completely incoherently for a while.", FALSE, ch,
				0, 0, TO_ROOM);
			break;
		case SCMD_MURMUR:
			send_to_char(ch, "You murmur softly.\r\n");
			act("You are disturbed by a low murmur from $n's vicinity.",
				FALSE, ch, 0, 0, TO_ROOM);
			break;
		case SCMD_SAY_TO:
			send_to_char(ch, "Say what to who?\r\n");
			break;
		default:
			send_to_char(ch, "Yes, but WHAT do you want to say?\r\n");
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
					char_from_room(ch,false);
					char_to_room(ch, ABS_EXIT(was_in, j)->to_room,false);
					recurs_say = 1;
					do_say(ch, arg, cmd, subcmd, 0);
				}
			}

			char_from_room(ch,false);
			char_to_room(ch, was_in,false);

			recurs_say = 0;
		}
	}

	if (subcmd != SCMD_SAY_TO)
		skip_spaces(&argument);

	delete_doubledollar(argument);

	if (subcmd == SCMD_SAY_TO) {
		argument = one_argument(argument, name);
		skip_spaces(&argument);

		o = NULL;
		if (!*name) {
			send_to_char(ch, "Say what to who?\r\n");
			return;
		}

		if (!(vict = get_char_room_vis(ch, name))) {
			if (!o)
				o = get_object_in_equip_vis(ch, name, ch->equipment, &j);
			if (!o)
				o = get_obj_in_list_vis(ch, name, ch->carrying);
			if (!o)
				o = get_obj_in_list_vis(ch, name, ch->in_room->contents);
		}
		if (!vict && !o) {
			if (!recurs_say)
				send_to_char(ch, "No-one by that name here.\r\n");
		} else {

			CreatureList::iterator it = ch->in_room->people.begin();
			for (; it != ch->in_room->people.end(); ++it) {
				if (!AWAKE((*it)) || (*it) == ch ||
					PLR_FLAGGED((*it), PLR_OLC | PLR_WRITING | PLR_MAILING))
					continue;
				strcpy(buf, PERS(ch, (*it)));
				strcpy(buf2, CAP(buf));
				if (o)
					strcpy(buf3, o->short_description);
				else if ((*it) == vict)
					strcpy(buf3, "you");
				else if (vict == ch) {
					strcpy(buf3, HMHR(ch));
					strcat(buf3, "self");
				} else
					strcpy(buf3, PERS(vict, (*it)));
				send_to_char(*it, "%s%s%s%s says to %s,%s %s'%s'%s\r\n",
					recurs_say ? "(remote) " : "", CCBLD((*it), C_NRM),
					CCBLU((*it), C_SPR), buf2, buf3, CCNRM((*it), C_SPR),
					CCCYN((*it), C_NRM), argument, CCNRM((*it), C_NRM));
			}
			if (!recurs_say) {
				if (o)
					strcpy(buf3, o->short_description);
				else if (vict == ch)
					strcpy(buf3, "yourself");
				else
					strcpy(buf3, PERS(vict, ch));
				send_to_char(ch, "%s%sYou say to %s,%s %s'%s'%s\r\n", CCBLD(ch,
						C_NRM), CCBLU(ch, C_NRM), buf3,
						CCNRM(ch, C_NRM), CCCYN(ch, C_NRM),
					argument, CCNRM(ch, C_NRM));
			}
		}
		return;
	}

	/* NOT say_to stuff: ********************************************* */
	CreatureList::iterator it = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end(); ++it) {
		if (!AWAKE((*it)) || (*it) == ch ||
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
		send_to_char(*it, "%s", buf);
	}
	if (!recurs_say) {
		send_to_char(ch, "%s%sYou %s,%s %s'%s'%s\r\n", CCBLD(ch, C_NRM),
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
	}
}


ACMD(do_gsay)
{
	struct Creature *k;
	struct follow_type *f;

	skip_spaces(&argument);

	if (!IS_AFFECTED(ch, AFF_GROUP)) {
		send_to_char(ch, "But you are not the member of a group!\r\n");
		return;
	}
	if (!*argument)
		send_to_char(ch, "Yes, but WHAT do you want to group-say?\r\n");
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

		sprintf(buf, "%sYou tell the group,%s '%s'%s", CCGRN(ch, C_NRM),
			CCYEL(ch, C_NRM), argument, CCNRM(ch, C_NRM));
		act(buf, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
	}
}


void
perform_tell(struct Creature *ch, struct Creature *vict, char *arg)
{
	char *str;

	str = tmp_sprintf("%sYou tell $N,%s '%s'",
		CCRED(ch, C_NRM), CCNRM(ch, C_NRM), arg);
	act(str, FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);

	delete_doubledollar(arg);

	if (!IS_NPC(vict)) {
		str = tmp_strdup(PERS(ch, vict));
		str[0] = toupper(str[0]);
		send_to_char(vict, "%s%s tells you,%s '%s'\r\n",
			CCRED(vict, C_NRM),
			str,
			CCNRM(vict, C_NRM),
			arg);
	} else {
		str = tmp_sprintf("$n tells you, '%s'", arg);
		act(str, FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
	}
	if (PRF2_FLAGGED(vict, PRF2_AUTOPAGE) && !IS_MOB(ch))
		send_to_char(vict, "\007\007");

	if (!IS_NPC(ch))
		GET_LAST_TELL(vict) = GET_IDNUM(ch);
}

/*
 * Yes, do_tell probably could be combined with whisper and ask, but
 * called frequently, and should IMHO be kept as tight as possible.
 */
ACMD(do_tell)
{
	struct Creature *vict;

	char buf2[MAX_INPUT_LENGTH];
	half_chop(argument, buf, buf2);

	if (!*buf || !*buf2)
		send_to_char(ch, "Who do you wish to tell what??\r\n");
	else if (!(vict = get_player_vis(ch, buf, false))) {
		send_to_char(ch, NOPERSON);
	} else if (ch == vict)
		send_to_char(ch, "You try to tell yourself something.\r\n");
	else if (PRF_FLAGGED(ch, PRF_NOTELL) && GET_LEVEL(ch) < LVL_AMBASSADOR)
		send_to_char(ch, 
			"You can't tell other people while you have notell on.\r\n");
	else if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)
		&& GET_LEVEL(ch) < LVL_GRGOD && ch->in_room != vict->in_room)
		send_to_char(ch, "The walls seem to absorb your words.\r\n");
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
	else {
		if (COMM_NOTOK_ZONES(ch, vict)) {
			if (!(affected_by_spell(ch, SPELL_TELEPATHY) ||
					affected_by_spell(vict, SPELL_TELEPATHY))) {
				act("Your telepathic voice cannot reach $M.",
					FALSE, ch, 0, vict, TO_CHAR);
				return;
			}

			WAIT_STATE(ch, 1 RL_SEC);
		}

		perform_tell(ch, vict, buf2);
	}
}


ACMD(do_reply)
{
	CreatureList::iterator tch = characterList.begin();
	skip_spaces(&argument);

	if (GET_LAST_TELL(ch) == NOBODY) {
		send_to_char(ch, "You have no-one to reply to!\r\n");
		return;
	}
	if (!*argument) {
		send_to_char(ch, "What is your reply?\r\n");
		return;
	}
	/*
	 * Make sure the person you're replying to is still playing by searching
	 * for them.  Note, now last tell is stored as player IDnum instead of
	 * a pointer, which is much better because it's safer, plus will still
	 * work if someone logs out and back in again.
	 */

	while (tch != characterList.end() && GET_IDNUM(*tch) != GET_LAST_TELL(ch))
		++tch;

	if (tch == characterList.end())
		send_to_char(ch, "They are no longer playing.\r\n");
	else if (!IS_NPC(*tch) && (*tch)->desc == NULL)
		send_to_char(ch, "They are linkless at the moment.\r\n");
	else if (PLR_FLAGGED(*tch, PLR_WRITING | PLR_MAILING | PLR_OLC))
		send_to_char(ch, "They are writing at the moment.\r\n");
	else {
		if (COMM_NOTOK_ZONES(ch, (*tch)) && COMM_NOTOK_ZONES((*tch), ch)) {
			if (!(affected_by_spell(ch, SPELL_TELEPATHY) ||
					affected_by_spell((*tch), SPELL_TELEPATHY))) {
				act("Your telepathic voice cannot reach $M.",
					FALSE, ch, 0, (*tch), TO_CHAR);
				return;
			}

			WAIT_STATE(ch, 1 RL_SEC);
		}

		perform_tell(ch, (*tch), argument);
	}
}


ACMD(do_spec_comm)
{
	struct Creature *vict;
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
		send_to_char(ch, "Whom do you want to %s.. and what??\r\n", action_sing);
	} else if (!(vict = get_char_room_vis(ch, buf))) {
		send_to_char(ch, NOPERSON);
	} else if (vict == ch)
		send_to_char(ch, 
			"You can't get your mouth close enough to your ear...\r\n");
	else {
		sprintf(buf, "%s$n %s you,%s '%s'", CCYEL(vict, C_NRM), action_plur,
			CCNRM(vict, C_NRM), buf2);
		act(buf, FALSE, ch, 0, vict, TO_VICT);
		sprintf(buf, "%sYou %s %s,%s '%s'", CCYEL(ch, C_NRM), action_sing,
			GET_DISGUISED_NAME(ch, vict), CCNRM(ch, C_NRM), buf2);
		act(buf, FALSE, ch, 0, 0, TO_CHAR);
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
		send_to_char(ch, 
			"Write?  With what?  ON what?  What are you trying to do?!?\r\n");
		return;
	}
	if (*penname) {				/* there were two arguments */
		if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying))) {
			send_to_char(ch, "You have no %s.\r\n", papername);
			return;
		}
		if (!(pen = get_obj_in_list_vis(ch, penname, ch->carrying))) {
			send_to_char(ch, "You have no %s.\r\n", penname);
			return;
		}
	} else {					/* there was one arg.. let's see what we can find */
		if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying))) {
			send_to_char(ch, "There is no %s in your inventory.\r\n", papername);
			return;
		}
		if (GET_OBJ_TYPE(paper) == ITEM_PEN) {	/* oops, a pen.. */
			pen = paper;
			paper = 0;
		} else if (GET_OBJ_TYPE(paper) != ITEM_NOTE) {
			send_to_char(ch, "That thing has nothing to do with writing.\r\n");
			return;
		}
		/* One object was found.. now for the other one. */
		if (!GET_EQ(ch, WEAR_HOLD)) {
			send_to_char(ch, "You can't write with %s %s alone.\r\n",
				AN(papername), papername);
			return;
		}
		if (!can_see_object(ch, GET_EQ(ch, WEAR_HOLD))) {
			send_to_char(ch, "The stuff in your hand is invisible!  Yeech!!\r\n");
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
		send_to_char(ch, "There's something written on it already.\r\n");
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
	struct Creature *vict;

	half_chop(argument, arg, buf2);

	if (IS_NPC(ch))
		send_to_char(ch, "Monsters can't page.. go away.\r\n");
	else if (!*arg)
		send_to_char(ch, "Whom do you wish to page?\r\n");
	else {
		sprintf(buf, "\007\007*%s* %s\r\n", GET_NAME(ch), buf2);
		if (!str_cmp(arg, "all")) {
			if (GET_LEVEL(ch) > LVL_GOD) {
				for (d = descriptor_list; d; d = d->next)
					if (IS_PLAYING(d) && d->creature) {
						send_to_char(d->creature, CCYEL(d->creature, C_NRM));
						send_to_char(d->creature, CCBLD(d->creature, C_SPR));
						act(buf, FALSE, ch, 0, d->creature, TO_VICT);
						send_to_char(d->creature, CCNRM(d->creature, C_SPR));
					}
			} else
				send_to_char(ch, "You will never be godly enough to do that!\r\n");
			return;
		}
		if ((vict = get_char_vis(ch, arg)) != NULL) {
			send_to_char(vict, CCYEL_BLD(vict, C_SPR));
			act(buf, FALSE, ch, 0, vict, TO_VICT);
			send_to_char(vict, CCYEL_BLD(vict, C_SPR));
			send_to_char(ch, CCYEL(vict, C_NRM));
			send_to_char(ch, CCBLD(vict, C_SPR));
			act(buf, FALSE, ch, 0, vict, TO_CHAR);
			send_to_char(vict, CCNRM(vict, C_SPR));

			return;
		} else
			send_to_char(ch, "There is no such person in the game!\r\n");
	}
}


/**********************************************************************
 * generalized communication func, originally by Fred C. Merkel (Torg) *
  *********************************************************************/
const bool CURSE_BAD = true;
const bool CURSE_OK = false;
const bool INTERPLANAR = false;
const bool PLANAR = true;
const bool NOT_EMOTE = false;
const bool IS_EMOTE = true;

struct channel_info_t {
	char *name;
	int deaf_vector;
	int deaf_flag;
	bool check_curse;
	bool check_plane;
	bool is_emote;
	char *desc_color;
	char *text_color;
	char *msg_noton;
	char *msg_muted;
};

static const channel_info_t channels[] = {
	{ "holler", 2, PRF2_NOHOLLER, CURSE_BAD, INTERPLANAR, NOT_EMOTE,
		KYEL_BLD, KRED,
		"Ha!  You are noholler buddy.",
		"You find yourself unable to holler!" },
	{ "shout", 1, PRF_DEAF, CURSE_BAD, PLANAR, NOT_EMOTE,
		KYEL, KCYN,
		"Turn off your noshout flag first!",
		"You cannot shout!!" },
	{ "gossip", 1, PRF_NOGOSS, CURSE_BAD, PLANAR, NOT_EMOTE,
		KGRN, KNRM,
		"You aren't even on the channel!",
		"You cannot gossip!!" },
	{ "auction", 1, PRF_NOAUCT, CURSE_BAD, PLANAR, NOT_EMOTE,
		KMAG, KNRM,
		"You aren't even on the channel!",
		"You cannot auction!!" },
	{ "congrat", 1, PRF_NOGRATZ, CURSE_BAD, PLANAR, NOT_EMOTE,
		KGRN, KMAG,
		"You aren't even on the channel!",
		"You cannot congratulate!!" },
	{ "sing", 1, PRF_NOMUSIC, CURSE_BAD, PLANAR, NOT_EMOTE,
		KCYN, KYEL,
		"You aren't even on the channel!",
		"You cannot sing!!" },
	{ "spew", 1, PRF_NOSPEW, CURSE_OK, PLANAR, NOT_EMOTE,
		KRED, KYEL,
		"You aren't even on the channel!",
		"You cannot spew!!" },
	{ "dream", 1, PRF_NODREAM, CURSE_BAD, PLANAR, NOT_EMOTE,
		KCYN, KNRM_BLD,
		"You aren't even on the channel!",
		"You cannot dream!!" },
	{ "project", 1, PRF_NOPROJECT, CURSE_BAD, INTERPLANAR, NOT_EMOTE,
		KNRM_BLD, KCYN,
		"You are not open to projections yourself...",
		"You cannot project.  The gods have muted you." },
	{ "newbie", -2, PRF2_NEWBIE_HELPER, CURSE_BAD, PLANAR, NOT_EMOTE,
		KYEL, KNRM,
		"You aren't on the illustrious newbie channel.",
		"The gods have muted you for bad behavior!" },
	{ "clan-say", 1, PRF_NOCLANSAY, CURSE_OK, PLANAR, NOT_EMOTE,
		KCYN, KNRM,
		"You aren't listening to the words of your clan.",
		"The gods have muted you.  You may not clan say." },
	{ "guild-say", 2, PRF2_NOGUILDSAY, CURSE_BAD, PLANAR, NOT_EMOTE,
		KMAG, KYEL,
		"You aren't listening to the rumors of your guild.",
		"You may not guild-say, for the gods have muted you." },
	{ "clan-emote", 1, PRF_NOCLANSAY, CURSE_OK, PLANAR, IS_EMOTE,
		KCYN, KNRM,
		"You aren't listening to the words of your clan.",
		"The gods have muted you.  You may not clan emote." },
};

ACMD(do_gen_comm)
{
	extern int level_can_shout;
	extern int holler_move_cost;
	const channel_info_t *chan;
	struct descriptor_data *i;
	struct clan_data *clan;
	char *plain_emit, *color_emit;
	char *imm_plain_emit, *imm_color_emit;
	const char *str, *sub_channel_desc;
	int eff_is_neutral, eff_is_good, eff_is_evil, eff_class, eff_clan;

	chan = &channels[subcmd];

	// pets can't shout on interplanar channels
	if (!ch->desc && ch->master && !chan->check_plane)
		return;

	// Drunk people not allowed!
	if ((GET_COND(ch, DRUNK) > 5) && (number(0, 3) >= 2)) {
		send_to_char(ch,
			"You try to %s, but somehow it just doesn't come out right.\r\n",
			chan->name);
		return;
	}
	if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
		send_to_char(ch, chan->msg_noton);
		return;
	}

	// These are all restrictions, to which immortals and npcs are uncaring
	if (!IS_NPC(ch) && !IS_IMMORT(ch)) {
		/* level_can_shout defined in config.c */
		if (GET_LEVEL(ch) < level_can_shout
			&& subcmd != SCMD_NEWBIE && GET_REMORT_GEN(ch) <= 0 ) {
			send_to_char(ch, "You must be at least level %d before you can %s.\r\n",
				level_can_shout, chan->name);
			send_to_char(ch, "Try using the newbie channel instead.\r\n");
			return;
		}

		// Only remort players can project
		if (subcmd == SCMD_PROJECT && !IS_REMORT(ch) &&
				GET_LEVEL(ch) < LVL_AMBASSADOR) {
			send_to_char(ch, "You do not know how to project yourself that way.\r\n");
			return;
		}

		/* make sure the char is on the channel */
		if (chan->deaf_vector == 1 && PRF_FLAGGED(ch, chan->deaf_flag)) {
			send_to_char(ch, "%s\r\n", chan->msg_noton);
			return;
		}
		if (chan->deaf_vector == 2 && PRF2_FLAGGED(ch, chan->deaf_flag)) {
			send_to_char(ch, "%s\r\n", chan->msg_noton);
			return;
		}
		if (chan->deaf_vector == -2 && !PRF2_FLAGGED(ch, chan->deaf_flag)) {
			send_to_char(ch, "%s\r\n", chan->msg_noton);
			return;
		}

		if (subcmd == SCMD_DREAM && (ch->getPosition() != POS_SLEEPING)) {
			send_to_char(ch, 
				"You attempt to dream, but realize you need to sleep first.\r\n");
			return;
		}

		if (subcmd == SCMD_NEWBIE && !PRF2_FLAGGED(ch, PRF2_NEWBIE_HELPER)) {
			send_to_char(ch, "You aren't on the illustrious newbie channel.\r\n");
			return;
		}

		if (subcmd == SCMD_GUILDSAY) {
			if (IS_NEUTRAL(ch)) {
				// Clerics and knights cannot be neutral
				if (GET_CLASS(ch) == CLASS_CLERIC) {
					send_to_char(ch,
						"You have been cast out of the ranks of the blessed.\r\n");
					return;
				}
				if (GET_CLASS(ch) == CLASS_KNIGHT) {
					send_to_char(ch,
						"You have been cast out of the ranks of the honored.\r\n");
					return;
				}
					
			} else {
				// Monks must be neutral
				if (GET_CLASS(ch) == CLASS_MONK) {
					send_to_char(ch, "You have been cast out of the monks until your neutrality is regained.\r\n");
					return;
				}
			}
		}
	}

	// skip leading spaces
	skip_spaces(&argument);

	/* make sure that there is something there to say! */
	if (!*argument) {
		send_to_char(ch, "Yes, %s, fine, %s we must, but WHAT???\r\n",
			chan->name, chan->name);
		return;
	}

	if (subcmd == SCMD_HOLLER && GET_LEVEL(ch) < LVL_TIMEGOD) {
		if (GET_MOVE(ch) < holler_move_cost) {
			send_to_char(ch, "You're too exhausted to holler.\r\n");
			return;
		} else
			GET_MOVE(ch) -= holler_move_cost;
	}

	eff_is_neutral = IS_NEUTRAL(ch);
	eff_is_evil = IS_EVIL(ch);
	eff_is_good = IS_GOOD(ch);
	eff_class = GET_CLASS(ch);
	eff_clan = GET_CLAN(ch);

	if (subcmd == SCMD_GUILDSAY && Security::isMember(ch, "AdminBasic") && *argument == '>') {
		char *class_str, *tmp_arg;

		tmp_arg = argument + 1;
		class_str = tmp_getword(&tmp_arg);
		if (!strcmp(class_str, "e-cleric")) {
			eff_is_neutral = false;
			eff_is_evil = true;
			eff_is_good = false;
			eff_class = CLASS_CLERIC;
		} else if (!strcmp(class_str, "g-cleric")) {
			eff_is_neutral = false;
			eff_is_evil = false;
			eff_is_good = true;
			eff_class = CLASS_CLERIC;
		} else if (!strcmp(class_str, "e-knight")) {
			eff_is_neutral = false;
			eff_is_evil = true;
			eff_is_good = false;
			eff_class = CLASS_KNIGHT;
		} else if (!strcmp(class_str, "g-knight")) {
			eff_is_neutral = false;
			eff_is_evil = false;
			eff_is_good = true;
			eff_class = CLASS_KNIGHT;
		} else
			eff_class = parse_player_class(class_str);

		if (eff_class == CLASS_MONK) {
			eff_is_neutral = true;
			eff_is_evil = false;
			eff_is_good = false;
		}
		if (eff_class == CLASS_UNDEFINED) {
			send_to_char(ch, "That guild doesn't exist in this universe.\r\n");
			return;
		}

		argument = tmp_arg;
	}

	if (subcmd == SCMD_CLANSAY || subcmd == SCMD_CLANEMOTE) {
		if (Security::isMember(ch, "AdminBasic") && *argument == '>') {
			char *tmp_arg;

			tmp_arg = argument + 1;
			clan = clan_by_name(tmp_getword(&tmp_arg));
			if (!clan) {
				send_to_char(ch, "That clan does not exist.\r\n");
				return;
			}

			eff_clan = clan->number;
			argument = tmp_arg;
		}

		if (IS_NPC(ch) || !eff_clan) {
			send_to_char(ch, "You aren't a member of a clan!\r\n");
			return;
		}
	}

	/* see if it's dirty! */
	if (chan->check_curse && !IS_MOB(ch) && GET_LEVEL(ch) < LVL_GRGOD &&
		Nasty_Words(argument)) {
		switch (number(0, 2)) {
		case 0:
			send_to_char(ch, 
				"Unless you like being muted, use SPEW for profanity.\r\n");
			break;
		case 1:
			send_to_char(ch, 
				"Why don't you keep that smut on the spew channel, eh?\r\n");
			break;
		case 2:
			send_to_char(ch, 
				"I don't think everyone wants to hear that.  SPEW it!\r\n");
			break;
		}
		slog("%s warned for nasty public communication.", GET_NAME(ch));
	}

	if (subcmd == SCMD_GUILDSAY) {
		if (eff_class >= 0 && eff_class < TOP_CLASS)
			str = tmp_tolower(pc_char_class_types[eff_class]);
		else
			str = tmp_sprintf("#%d", eff_class);
		if (eff_class == CLASS_CLERIC || eff_class == CLASS_KNIGHT)
			str = tmp_sprintf("%s-%s", (eff_is_good ? "g":"e"), str);
		sub_channel_desc = tmp_strcat("[", str, "]", NULL);
	} else if (subcmd == SCMD_CLANSAY || subcmd == SCMD_CLANEMOTE) {
		clan = real_clan(eff_clan);

		if (eff_clan >= 0 && eff_clan < TOP_CLASS)
			str = tmp_tolower(clan->name);
		else
			str = tmp_sprintf("#%d", eff_clan);
		sub_channel_desc = tmp_strcat("[", str, "]", NULL);
	} else {
		sub_channel_desc = "";
	}

	// Eliminate double dollars, and double percent signs (for sprintf)
	argument = tmp_gsub(argument, "$$", "$");

	// Construct all the emits ahead of time.
	if (chan->is_emote) {
		if (*sub_channel_desc)
			sub_channel_desc = tmp_strcat(sub_channel_desc, " ");
		if (COLOR_LEV(ch) >= C_NRM)
			send_to_char(ch, "%s%s%s%s %s%s%s\r\n", chan->desc_color,
				(IS_IMMORT(ch) ? sub_channel_desc:""),
				GET_NAME(ch),
				KNRM, chan->text_color, argument, KNRM);
		else
			send_to_char(ch, "%s%s %s\r\n",
				(IS_IMMORT(ch) ? sub_channel_desc:""),
				GET_NAME(ch),
				argument);

		// The emits are passed directly as the format string to
		// send_to_char, so the argument must have its percent signs
		// doubled
		argument = tmp_gsub(argument, "%", "%%");

		plain_emit = tmp_sprintf("%%s %s\r\n", argument);
		color_emit = tmp_sprintf("%s%%s %s%s%s%s\r\n", chan->desc_color,
			KNRM, chan->text_color, argument, KNRM);
		imm_plain_emit = tmp_sprintf("%s%%s %s\r\n", sub_channel_desc,
			argument);
		imm_color_emit = tmp_sprintf("%s%s%%s%s%s %s%s\r\n",
			chan->desc_color, sub_channel_desc, KNRM, chan->text_color,
			argument, KNRM);
	} else {
		if (*sub_channel_desc)
			sub_channel_desc = tmp_strcat(" ", sub_channel_desc);
		if (COLOR_LEV(ch) >= C_NRM)
			send_to_char(ch, "%sYou %s%s,%s%s '%s'%s\r\n", chan->desc_color,
				chan->name,
				(IS_IMMORT(ch) ? sub_channel_desc:""),
				KNRM,
				chan->text_color, argument, KNRM);
		else
			send_to_char(ch, "You %s%s, '%s'\r\n", chan->name,
				(IS_IMMORT(ch) ? sub_channel_desc:""),
				argument);
		// The emits are passed directly as the format string to
		// send_to_char, so the argument must have its percent signs
		// doubled
		argument = tmp_gsub(argument, "%", "%%");

		plain_emit = tmp_sprintf("%%s %ss, '%s'\r\n", chan->name, argument);
		color_emit = tmp_sprintf("%s%%s %ss,%s%s '%s'%s\r\n", chan->desc_color,
			chan->name, KNRM, chan->text_color, argument, KNRM);
		imm_plain_emit = tmp_sprintf("%%s %ss%s, '%s'\r\n", chan->name,
			sub_channel_desc, argument);
		imm_color_emit = tmp_sprintf("%s%%s %ss%s,%s%s '%s'%s\r\n",
			chan->desc_color, chan->name, sub_channel_desc, KNRM,
			chan->text_color, argument, KNRM);
	}

	/* now send all the strings out */
	for (i = descriptor_list; i; i = i->next) {
		if (STATE(i) != CXN_PLAYING || i == ch->desc || !i->creature ||
				PLR_FLAGGED(i->creature, PLR_WRITING) ||
				PLR_FLAGGED(i->creature, PLR_OLC))
			continue;

		if (chan->deaf_vector == 1 &&
				PRF_FLAGGED(i->creature, chan->deaf_flag))
			continue;
		if (chan->deaf_vector == 2 &&
				PRF2_FLAGGED(i->creature, chan->deaf_flag))
			continue;
		if (chan->deaf_vector == -2 &&
				!PRF2_FLAGGED(i->creature, chan->deaf_flag))
			continue;

		// Must be in same clan or an admin to hear clansay
		if ((subcmd == SCMD_CLANSAY || subcmd == SCMD_CLANEMOTE) &&
				GET_CLAN(i->creature) != eff_clan &&
				!Security::isMember(i->creature, "AdminBasic"))
			continue;

		// Must be in same guild or an admin to hear guildsay
		if (subcmd == SCMD_GUILDSAY &&
				GET_CLASS(i->creature) != eff_class &&
				!Security::isMember(i->creature, "AdminBasic"))
			continue;

		// Evil and good clerics and knights have different guilds
		if (subcmd == SCMD_GUILDSAY &&
				(GET_CLASS(i->creature) == CLASS_CLERIC ||
				GET_CLASS(i->creature) == CLASS_KNIGHT) &&
				!Security::isMember(i->creature, "AdminBasic")) {
			if (eff_is_neutral)
				continue;
			if (eff_is_evil && !IS_EVIL(i->creature))
				continue;
			if (eff_is_good && !IS_GOOD(i->creature))
				continue;
		}

		// Outcast monks don't hear other monks
		if (subcmd == SCMD_GUILDSAY &&
				GET_CLASS(i->creature) == CLASS_MONK &&
				!IS_NEUTRAL(i->creature) &&
				!Security::isMember(i->creature, "AdminBasic"))
			continue;

		if (IS_NPC(ch) || !IS_IMMORT(i->creature)) {
			if (subcmd == SCMD_PROJECT && !IS_REMORT(i->creature))
				continue;

			if (subcmd == SCMD_DREAM &&
					i->creature->getPosition() != POS_SLEEPING)
				continue;

			if (subcmd == SCMD_SHOUT &&
				((ch->in_room->zone != i->creature->in_room->zone) ||
					i->creature->getPosition() < POS_RESTING))
				continue;

			if ((ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF) ||
					ROOM_FLAGGED(i->creature->in_room, ROOM_SOUNDPROOF)) &&
					!IS_IMMORT(ch) && i->creature->in_room != ch->in_room)
				continue;

			if (chan->check_plane && COMM_NOTOK_ZONES(ch, i->creature))
				continue;

		}

		if (IS_IMMORT(i->creature))
			send_to_char(i->creature,
				COLOR_LEV(i->creature) >= C_NRM ? imm_color_emit : imm_plain_emit,
				tmp_capitalize(PERS(ch, i->creature)));
		else
			send_to_char(i->creature,
				COLOR_LEV(i->creature) >= C_NRM ? color_emit : plain_emit,
				tmp_capitalize(PERS(ch, i->creature)));
	}

	if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF) && !IS_IMMORT(ch))
		send_to_char(ch, "The walls seem to absorb your words.\r\n");
}

ACMD(do_qcomm)
{
	struct descriptor_data *i;

	if (!IS_NPC(ch) && !GET_QUEST(ch)) {
		send_to_char(ch, "You aren't even part of the quest!\r\n");
		return;
	}

	if (GET_LEVEL(ch) < LVL_IMMORT && !quest_status) {
		send_to_char(ch, "There are no active quests at the moment.\r\n");
		return;
	}

	skip_spaces(&argument);

	if (!*argument) {
		send_to_char(ch, "%s?  Yes, fine, %s we must, but WHAT??\r\n", CMD_NAME,
			CMD_NAME);
		CAP(buf);
	} else if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
		send_to_char(ch, "You cannot quest say, sorry!\r\n");
	} else {
		if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
			send_to_char(ch, "The walls absorb the sound of your voice.\r\n");
		else {
			if (subcmd == SCMD_QSAY) {
				send_to_char(ch, "%s%sYou quest-say,%s '%s'\r\n", CCYEL(ch, C_NRM),
					CCBLD(ch, C_SPR), CCNRM(ch, C_SPR), argument);
				delete_doubledollar(buf);
			} else {
				send_to_char(ch, "%s%s%s\r\n",
					CCYEL_BLD(ch, C_NRM),
					argument,
					CCNRM(ch, C_NRM));
			}
		}

		if (subcmd == SCMD_QSAY) {
			for (i = descriptor_list; i; i = i->next)
				if (STATE(i) == CXN_PLAYING && i != ch->desc &&
					GET_QUEST(i->creature) == GET_QUEST(ch) &&
					i->creature->in_room != NULL &&
					!ROOM_FLAGGED(i->creature->in_room, ROOM_SOUNDPROOF) &&
					!PLR_FLAGGED(i->creature,
						PLR_MAILING | PLR_WRITING | PLR_OLC)) {
					send_to_char(i->creature, "%s%s quest-says,%s '%s'\r\n",
						CCYEL_BLD(i->creature, C_SPR),
						PERS(ch, i->creature),
						CCNRM(i->creature, C_SPR),
						argument);
				}
		} else {
			strcpy(buf, argument);

			for (i = descriptor_list; i; i = i->next)
				if (STATE(i) == CXN_PLAYING && i != ch->desc &&
					GET_QUEST(i->creature) != GET_QUEST(ch) &&
					i->creature->in_room != NULL &&
					!ROOM_FLAGGED(i->creature->in_room, ROOM_SOUNDPROOF) &&
					!PLR_FLAGGED(i->creature,
						PLR_MAILING | PLR_WRITING | PLR_OLC)) {
					send_to_char(i->creature, "%s", CCYEL_BLD(i->creature, C_NRM));
					act(buf, 0, ch, 0, i->creature, TO_VICT | TO_SLEEP);
					send_to_char(i->creature, "%s", CCNRM(i->creature, C_NRM));
				}
		}
	}
}

#undef __act_comm_cc__
