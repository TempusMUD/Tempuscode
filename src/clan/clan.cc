//
// File: clan.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

/* ************************************************************************
*   File: clan.c                                        Part of CircleMUD *
************************************************************************ */

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
#include "security.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "house.h"
#include "clan.h"
#include "char_class.h"
#include "tmpstr.h"
#include "player_table.h"

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;
extern struct spell_info_type spell_info[];
int check_mob_reaction(struct Creature *ch, struct Creature *vict);
char *save_wld(struct Creature *ch, zone_data *zone);

struct clan_data *clan_list;
extern FILE *player_fl;
int player_i = 0;


#define NAME(x) ((temp = playerIndex.getName(x)) == NULL ? "<UNDEF>" : temp)

void
REMOVE_ROOM_FROM_CLAN(struct room_list_elem *rm_list, struct clan_data *clan)
{
	struct room_list_elem *temp;
	REMOVE_FROM_LIST(rm_list, clan->room_list, next);
}

void
REMOVE_MEMBER_FROM_CLAN(struct clanmember_data *member, struct clan_data *clan)
{
	struct clanmember_data *temp;
	REMOVE_FROM_LIST(member, clan->member_list, next);
}

ACMD(do_enroll)
{
	struct Creature *vict = 0;
	struct clan_data *clan = real_clan(GET_CLAN(ch));
	struct clanmember_data *member = NULL;
	int count = 0;
	char *msg, *member_str;

	member_str = tmp_getword(&argument);

	if (Security::isMember(ch, "Clan")) {
		char *clan_str = tmp_getword(&argument);

		if (!*clan_str) {
			send_to_char(ch, "Enroll them into which clan?\r\n");
			return;
		} else {
			clan = clan_by_name(clan_str);
			if (!clan) {
				send_to_char(ch, "That clan doesn't exist.\r\n");
				return;
			}
		}
	}

	if (!clan)
		send_to_char(ch, "Hmm... You need to be in a clan yourself, first.\r\n");
	else if (!*member_str)
		send_to_char(ch, "You must specify the player to enroll.\r\n");
	else if (!(vict = get_char_room_vis(ch, member_str)))
		send_to_char(ch, "You don't see that person.\r\n");
	else if( IS_NPC(vict) )
		send_to_char(ch, "You can only enroll player characters.\r\n");
	else if (vict == ch && !Security::isMember(ch, "Clan"))
		send_to_char(ch, "Yeah, yeah, yeah... enroll yourself, huh?\r\n");
	else if (!IS_IMMORT(ch) && !PLR_FLAGGED(ch, PLR_CLAN_LEADER))
		send_to_char(ch, "You are not the leader of the clan!\r\n");
	else if (IS_MOB(vict) && GET_LEVEL(ch) < LVL_CREATOR)
		send_to_char(ch, "What's the point of that?\r\n");
	else if (real_clan(GET_CLAN(vict))) {
		if (GET_CLAN(vict) == GET_CLAN(ch)) {
			if (!(member = real_clanmember(GET_IDNUM(vict), clan))) {
				send_to_char(ch, "Clan flag purged on vict.  Re-enroll.\r\n");
				GET_CLAN(vict) = 0;
			} else
				send_to_char(ch, "That person is already in your clan.\r\n");
		} else
			send_to_char(ch, 
				"You cannot while they are a member of another clan.\r\n");
	} else if (PLR_FLAGGED(vict, PLR_FROZEN))
		send_to_char(ch, 
			"They are frozen right now.  Wait until a god has mercy.\r\n");
	else if (GET_LEVEL(vict) < LVL_CAN_CLAN)
		send_to_char(ch, 
			"Players must be level 10 before being inducted into the clan.\r\n");
	else if ((member = real_clanmember(GET_IDNUM(vict), clan))) {
		send_to_char(ch, "Something wierd just happened... try again.\r\n");
		REMOVE_MEMBER_FROM_CLAN(member, clan);
		free(member);
		save_clans();
	} else {

		for (count = 0, member = clan->member_list; member;
			count++, member = member->next);
		if (count >= MAX_CLAN_MEMBERS) {
			send_to_char(ch, 
				"The max number of members has been reached for this clan.\r\n");
			return;
		}
		REMOVE_BIT(PLR_FLAGS(vict), PLR_CLAN_LEADER);
		send_to_char(vict, "You have been inducted into clan %s by %s!\r\n",
			clan->name, GET_NAME(ch));
		msg = tmp_sprintf("%s has been inducted into clan %s by %s!",
			GET_NAME(vict), clan->name, GET_NAME(ch));
		mudlog(GET_INVIS_LVL(ch), NRM, true, "%s", msg);
		msg = tmp_strcat(msg, "\r\n",NULL);
		send_to_clan(msg, GET_CLAN(ch));
		GET_CLAN(vict) = clan->number;
		CREATE(member, struct clanmember_data, 1);
		member->idnum = GET_IDNUM(vict);
		member->rank = 0;
		member->next = clan->member_list;
		clan->member_list = member;
		sort_clanmembers(clan);
		save_clans();
	}
}

ACMD(do_dismiss)
{
	struct Creature *vict;
	struct clan_data *clan = real_clan(GET_CLAN(ch));
	struct clanmember_data *member = NULL, *member2 = NULL;
	bool in_file = false;
	long idnum = -1;
	char *arg, *msg;


	arg = tmp_getword(&argument);
	skip_spaces(&argument);

	if (!clan && !Security::isMember(ch, "Clan")) {
		send_to_char(ch, "Try joining a clan first.\r\n");
		return;
	} else if (!*arg) {
		send_to_char(ch, "You must specify the clan member to dismiss.\r\n");
		return;
	}
	/* Find the player. */
	if ((idnum = playerIndex.getID(arg)) < 0) {
		send_to_char(ch, "There is no character named '%s'\r\n", arg);
		return;
	}

	if (!(vict = get_char_in_world_by_idnum(idnum))) {
		// load the char from file
		vict = new Creature(true);
		in_file = true;

		if (!vict->loadFromXML(idnum)) {
			delete vict;
			send_to_char(ch, "Error loading char from file.\r\n");
			return;
		}
	}

	/* Was the player found? */
	if (!vict) {
		send_to_char(ch, "That person isn't in your clan.\r\n");
		if (in_file)
			delete vict;
		return;
	}

	clan = real_clan(GET_CLAN(vict));
	if (vict == ch)
		send_to_char(ch, "Try resigning if you want to leave the clan.\r\n");
	else if (!PLR_FLAGGED(ch, PLR_CLAN_LEADER) && !IS_IMMORT(ch))
		send_to_char(ch, "You are not the leader of the clan!\r\n");
	else if (GET_CLAN(vict) != GET_CLAN(ch) && !Security::isMember(ch, "Clan"))
		send_to_char(ch, "Umm, why dont you check the clan list, okay?\r\n");
	else if (!Security::isMember(ch, "Clan") &&
			(member = real_clanmember(GET_IDNUM(ch), clan)) &&
			(member2 = real_clanmember(GET_IDNUM(vict), clan)) &&
			(member->rank <= member2->rank && member->rank < clan->top_rank))
		send_to_char(ch, "You don't have the rank for that.\r\n");
	else if (PLR_FLAGGED(ch, PLR_FROZEN))
		send_to_char(ch, "You are frozen solid, and can't lift a finger!\r\n");
	else if (!Security::isMember(ch, "Clan") &&
			PLR_FLAGGED(vict, PLR_CLAN_LEADER) &&
			(GET_IDNUM(ch) != clan->owner && GET_LEVEL(ch) < LVL_IMMORT))
		send_to_char(ch, "You cannot dismiss co-leaders.\r\n");
	else {
		send_to_char(vict, "You have been dismissed from clan %s by %s!\r\n",
			clan->name, GET_NAME(ch));
		GET_CLAN(vict) = 0;
		msg = tmp_sprintf("%s has been dismissed from clan %s by %s!",
			GET_NAME(vict), clan->name, GET_NAME(ch));
		mudlog(GET_INVIS_LVL(ch), NRM, true, "%s", msg);
		msg = tmp_strcat(msg, "\r\n",NULL);
		send_to_clan(msg, GET_CLAN(ch));
		if ((member = real_clanmember(GET_IDNUM(vict), clan))) {
			REMOVE_MEMBER_FROM_CLAN(member, clan);
			free(member);
		}
		REMOVE_BIT(PLR_FLAGS(vict), PLR_CLAN_LEADER);

		if (in_file) {
			vict->saveToXML();
			delete vict;
			send_to_char(ch, "Player dismissed.\r\n");
		}
	}
}

ACMD(do_resign)
{

	struct clan_data *clan = real_clan(GET_CLAN(ch));
	struct clanmember_data *member = NULL;
	char *msg;

	skip_spaces(&argument);

	if (IS_NPC(ch))
		send_to_char(ch, "NPC's cannot resign...\r\n");
	else if (!clan)
		send_to_char(ch, "You need to be in a clan before you resign from it.\r\n");
	else if (strcmp(argument, "yes"))
		send_to_char(ch,
			"You must type 'resign yes' to leave your clan.\r\n");
	else {
		send_to_char(ch, "You have resigned from clan %s.\r\n", clan->name);
		msg = tmp_sprintf("%s has resigned from clan %s.", GET_NAME(ch),
			clan->name);
		mudlog(GET_INVIS_LVL(ch), NRM, true, "%s", msg);
		msg = tmp_strcat(msg, "\r\n",NULL);
		send_to_clan(msg, GET_CLAN(ch));
		GET_CLAN(ch) = 0;
		REMOVE_BIT(PLR_FLAGS(ch), PLR_CLAN_LEADER);
		if (clan->owner == GET_IDNUM(ch))
			clan->owner = 0;
		if ((member = real_clanmember(GET_IDNUM(ch), clan))) {
			REMOVE_MEMBER_FROM_CLAN(member, clan);
			free(member);
		}
	}
}

ACMD(do_clanlist)
{
	struct Creature *i;
	struct clan_data *clan = real_clan(GET_CLAN(ch));
	struct clanmember_data *member = NULL, *ch_member = NULL;
	struct descriptor_data *d;
	int min_lev = 0;
	bool complete = 0;
	int visible = 1;
	int found = 0;
	char *name, *line, *msg = "";

	if (!clan) {
		send_to_char(ch, "You are not a member of any clan.\r\n");
		return;
	}

	ch_member = real_clanmember(GET_IDNUM(ch), clan);

	skip_spaces(&argument);
	argument = one_argument(argument, arg);
	while (*arg) {

		if (is_number(arg)) {
			min_lev = atoi(arg);
			if (min_lev < 0 || min_lev > LVL_GRIMP) {
				send_to_char(ch, "Try a number between 0 and 60, please.\r\n");
				return;
			}
		} else if (is_abbrev(arg, "all") || is_abbrev(arg, "complete")) {
			complete = true;
		} else {
			send_to_char(ch, "Clanlist usage: clanlist [minlevel] ['all']\r\n");
			return;
		}
		argument = one_argument(argument, arg);
	}

	msg = tmp_strcat("Members of clan ", clan->name, " :\r\n", NULL);
	for (member = clan->member_list; member; member = member->next, found = 0) {
		for (d = descriptor_list; d && !found; d = d->next) {
			if (IS_PLAYING(d)) {
				i = ((d->original && GET_LEVEL(ch) > GET_LEVEL(d->original)) ?
					d->original : d->creature);
				if (i && GET_CLAN(i) == GET_CLAN(ch) &&
					GET_IDNUM(i) == member->idnum && (visible = can_see_creature(ch, i))
					&& GET_LEVEL(i) >= min_lev && (i->in_room != NULL)) {
					name = tmp_strcat(GET_NAME(i), " ",
						clan->ranknames[(int)member->rank] ? clan->
						ranknames[(int)member->rank] : "the member",
						" (online)", NULL);
					if (d->original)
						line = tmp_sprintf(
							"%s[%s%2d %s%s]%s %s%-40s%s - %s%s%s %s(in %s)%s\r\n",
							CCGRN(ch, C_NRM),
							CCNRM(ch, C_NRM),
							GET_LEVEL(i),
							char_class_abbrevs[(int)GET_CLASS(i)],
							CCGRN(ch, C_NRM),
							CCNRM(ch, C_NRM),
							(GET_LEVEL(i) >= LVL_AMBASSADOR ? CCGRN(ch,
									C_NRM) : (PLR_FLAGGED(i,
										PLR_CLAN_LEADER) ? CCCYN(ch,
										C_NRM) : "")),
							name,
							((GET_LEVEL(i) >= LVL_AMBASSADOR
									|| PLR_FLAGGED(i,
										PLR_CLAN_LEADER)) ? CCNRM(ch, C_NRM) : ""),
							CCCYN(ch, C_NRM),
							(d->creature->in_room->zone == ch->in_room->zone) ?
								(check_sight_room(ch, d->creature->in_room)) ?
									d->creature->in_room->name
									: "You cannot tell..."
								: d->creature->in_room->zone->name,
							CCNRM(ch, C_NRM),
							CCRED(ch, C_CMP),
							GET_NAME(d->creature),
							CCNRM(ch, C_CMP));
					else if (GET_LEVEL(i) >= LVL_AMBASSADOR)
						line = tmp_sprintf("%s[%s%s%s]%s %-40s%s - %s%s%s\r\n",
							CCYEL_BLD(ch, C_NRM),
							CCNRM_GRN(ch, C_SPR),
							level_abbrevs[(int)(GET_LEVEL(i) - LVL_AMBASSADOR)],
							CCYEL_BLD(ch, C_NRM),
							CCNRM_GRN(ch, C_SPR),
							name,
							CCNRM(ch, C_SPR),
							CCCYN(ch, C_NRM),
							(i->in_room->zone == ch->in_room->zone) ?
								((check_sight_room(ch, i->in_room)) ?
									i->in_room->name
									: "You cannot tell...")
								: i->in_room->zone->name,
							CCNRM(ch, C_NRM));
					else
						line = tmp_sprintf("%s[%s%2d %s%s]%s %s%-40s%s - %s%s%s\r\n",
							CCGRN(ch, C_NRM),
							CCNRM(ch, C_NRM),
							GET_LEVEL(i),
							char_class_abbrevs[(int)GET_CLASS(i)],
							CCGRN(ch, C_NRM),
							CCNRM(ch, C_NRM),
							(PLR_FLAGGED(i, PLR_CLAN_LEADER) ?
								CCCYN(ch, C_NRM) : ""),
							name,
							(PLR_FLAGGED(i, PLR_CLAN_LEADER) ?
								CCNRM(ch, C_NRM) : ""),
							CCCYN(ch, C_NRM),
							(i->in_room->zone == ch->in_room->zone) ?
								((check_sight_room(ch, i->in_room)) ?
									i->in_room->name
									: "You cannot tell...")
								: i->in_room->zone->name,
							CCNRM(ch, C_NRM));

					++found;
					msg = tmp_strcat(msg, line,NULL);
				}
			}
		}
		if (complete && !found) {
			if (!ch_member || member->rank > ch_member->rank ||
				!playerIndex.getName(member->idnum)) {
				continue;
			}
			i = new Creature(true);
			if (i->loadFromXML(member->idnum)) {
				name = tmp_strcat(GET_NAME(i), " ",
					clan->ranknames[(int)member->rank] ?
					clan->ranknames[(int)member->rank] : "the member", NULL);

				if (GET_LEVEL(i) >= LVL_AMBASSADOR)
					line = tmp_sprintf("%s[%s%s%s]%s %-40s%s\r\n",
						CCYEL_BLD(ch, C_NRM), CCNRM_GRN(ch, C_SPR),
						level_abbrevs[(int)(GET_LEVEL(i) - LVL_AMBASSADOR)],
						CCYEL_BLD(ch, C_NRM), CCNRM_GRN(ch, C_SPR),
						name, CCNRM(ch, C_SPR));
				else
					line = tmp_sprintf("%s[%s%2d %s%s]%s %s%-40s%s\r\n",
						CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_LEVEL(i),
						char_class_abbrevs[(int)GET_CLASS(i)], CCGRN(ch,
							C_NRM), CCNRM(ch, C_NRM), (PLR_FLAGGED(i,
								PLR_CLAN_LEADER) ? CCCYN(ch, C_NRM) : ""),
						name, CCNRM(ch, C_NRM));

				msg = tmp_strcat(msg, line,NULL);
			}
			delete i;
		}
	}
	page_string(ch->desc, msg);
}

ACMD(do_cinfo)
{

	struct clan_data *clan = real_clan(GET_CLAN(ch));
	struct clanmember_data *member;
	struct room_list_elem *rm_list = NULL;
	int found = 0;
	int i;
	char *msg = "";

	if (!clan)
		send_to_char(ch, "You are not a member of any clan.\r\n");
	else {
		for (i = 0, member = clan->member_list; member; member = member->next)
			i++;
		msg = tmp_sprintf("Information on clan %s%s%s:\r\n\r\n"
			"Clan badge: '%s%s%s', Clan headcount: %d, "
			"Clan bank account: %d\r\nClan ranks:\r\n",
			CCCYN(ch, C_NRM), clan->name, CCNRM(ch, C_NRM),
			CCCYN(ch, C_NRM), clan->badge, CCNRM(ch, C_NRM),
			i, clan->bank_account);
		for (i = clan->top_rank; i >= 0; i--) {
			msg = tmp_sprintf("%s (%2d)  %s%s%s\r\n", msg, i, CCYEL(ch, C_NRM),
				clan->ranknames[i] ? clan->ranknames[i] : "the Member",
				CCNRM(ch, C_NRM));
		}

		msg = tmp_strcat(msg, "Clan rooms:\r\n",NULL);
		for (rm_list = clan->room_list; rm_list; rm_list = rm_list->next) {
			if (rm_list->room
				&& ROOM_FLAGGED(rm_list->room, ROOM_CLAN_HOUSE)) {
				msg = tmp_strcat(msg,
					CCCYN(ch, C_NRM),
					rm_list->room->name,
					CCNRM(ch, C_NRM), "\r\n", NULL);
				++found;
			}
		}
		if (!found)
			msg = tmp_strcat(msg, "None.\r\n",NULL);

		page_string(ch->desc, msg);
	}
}

ACMD(do_demote)
{
	struct clan_data *clan = real_clan(GET_CLAN(ch));
	struct clanmember_data *member1, *member2;
	struct Creature *vict = NULL;
	char *msg;

	skip_spaces(&argument);

	if (!clan)
		send_to_char(ch, "You are not even in a clan.\r\n");
	else if (!*argument)
		send_to_char(ch, "You must specify the person to demote.\r\n");
	else if (!(vict = get_char_room_vis(ch, argument)))
		send_to_char(ch, "No-one around by that name.\r\n");
	else if (!(member1 = real_clanmember(GET_IDNUM(ch), clan)))
		send_to_char(ch, "You are not properly installed in the clan.\r\n");
	else if (ch == vict) {
		if (!member1->rank)
			send_to_char(ch, "You already at the bottom of the totem pole.\r\n");
		else {
			member1->rank--;
			msg = tmp_sprintf("%s has demoted self to clan rank %s (%d)",
				GET_NAME(ch), clan->ranknames[(int)member1->rank],
				member1->rank);
			slog("%s", msg);
			msg = tmp_strcat(msg, "\r\n",NULL);
			send_to_clan(msg, clan->number);
			sort_clanmembers(clan);
			save_clans();
		}
	} else if (real_clan(GET_CLAN(vict)) != clan) {
		act("$N is not a member of your clan.", FALSE, ch, 0, vict, TO_CHAR);
	} else if (!(member2 = real_clanmember(GET_IDNUM(vict), clan))) {
		act("$N is not properly installed in the clan.\r\n",
			FALSE, ch, 0, vict, TO_CHAR);
	} else if (!PLR_FLAGGED(ch, PLR_CLAN_LEADER)
		&& !Security::isMember(ch, "Clan")) {
		send_to_char(ch, "You are unable to demote.\r\n");
	} else if (member2->rank >= member1->rank
		|| PLR_FLAGGED(vict, PLR_CLAN_LEADER)) {
		act("You are not in a position to demote $M.", FALSE, ch, 0, vict,
			TO_CHAR);
	} else if (member2->rank <= 0) {
		send_to_char(ch, "They are already as low as they can go.\r\n");
		if (member2->rank < 0) {
			slog("SYSERR: clan member with rank < 0");
			member2->rank = 0;
			save_clans();
		}
	} else {
		member2->rank--;
		msg = tmp_sprintf("%s has demoted %s to clan rank %s (%d)",
			GET_NAME(ch), GET_NAME(vict),
			clan->ranknames[(int)member2->rank], member2->rank);
		slog("%s", msg);
		msg = tmp_strcat(msg, "\r\n",NULL);
		send_to_clan(msg, clan->number);
		sort_clanmembers(clan);
		save_clans();
	}
}

ACMD(do_promote)
{
	struct clan_data *clan = real_clan(GET_CLAN(ch));
	struct clanmember_data *member1, *member2;
	struct Creature *vict = NULL;
	char *msg;

	skip_spaces(&argument);

	if (!clan)
		send_to_char(ch, "You are not even in a clan.\r\n");
	else if (!*argument)
		send_to_char(ch, "You must specify the person to promote.\r\n");
	else if (!(vict = get_char_room_vis(ch, argument)))
		send_to_char(ch, "No-one around by that name.\r\n");
	else if (ch == vict && clan->owner != GET_IDNUM(ch))
		send_to_char(ch, "Very funny.  Really.\r\n");
	else if (real_clan(GET_CLAN(vict)) != clan)
		act("$N is not a member of your clan.", FALSE, ch, 0, vict, TO_CHAR);
	else if (!(member1 = real_clanmember(GET_IDNUM(ch), clan)))
		send_to_char(ch, "You are not properly installed in the clan.\r\n");
	else if (!(member2 = real_clanmember(GET_IDNUM(vict), clan)))
		act("$N is not properly installed in the clan.\r\n",
			FALSE, ch, 0, vict, TO_CHAR);
	else if (!PLR_FLAGGED(ch, PLR_CLAN_LEADER)
		&& !Security::isMember(ch, "Clan")) {
		if (member2->rank >= member1->rank - 1 && GET_IDNUM(ch) != clan->owner) {
			act("You are not in a position to promote $M.",
				FALSE, ch, 0, vict, TO_CHAR);
		} else {
			if (member2->rank >= clan->top_rank) {
				act("$E is already fully advanced.", FALSE, ch, 0, vict,
					TO_CHAR);
				return;
			}
			member2->rank++;
			msg = tmp_sprintf("%s has promoted %s to clan rank %s (%d)",
				GET_NAME(ch), GET_NAME(vict),
				clan->ranknames[(int)member2->rank] ?
				clan->ranknames[(int)member2->rank] : "member", member2->rank);
			slog("%s", msg);
			msg = tmp_strcat(msg, "\r\n",NULL);
			send_to_clan(msg, clan->number);
			sort_clanmembers(clan);
			save_clans();
		}
	} else if (PLR_FLAGGED(ch, PLR_CLAN_LEADER) ||
		member1->rank >= clan->top_rank) {
		if (member2->rank >= clan->top_rank
			&& PLR_FLAGGED(ch, PLR_CLAN_LEADER)) {
			if (PLR_FLAGGED(vict, PLR_CLAN_LEADER))
				act("$E is already fully advanced.", FALSE, ch, 0, vict,
					TO_CHAR);
			else {
				SET_BIT(PLR_FLAGS(vict), PLR_CLAN_LEADER);
				msg = tmp_sprintf("%s has promoted %s to clan leader status.",
					GET_NAME(ch), GET_NAME(vict));
				slog("%s", msg);
				msg = tmp_strcat(msg, "\r\n",NULL);
				send_to_clan(msg, clan->number);
				vict->saveToXML();
			}
		} else {
			if (member2->rank >= member1->rank && GET_IDNUM(ch) != clan->owner)
				act("You cannot promote $M further.", FALSE, ch, 0, vict,
					TO_CHAR);
			else {
				member2->rank++;
				msg = tmp_sprintf("%s has promoted %s to clan rank %s (%d)",
					GET_NAME(ch), GET_NAME(vict),
					clan->ranknames[(int)member2->rank] ?
					clan->ranknames[(int)member2->rank] : "member",
					member2->rank);
				slog("%s", msg);
				msg = tmp_strcat(msg, "\r\n",NULL);
				send_to_clan(msg, clan->number);
				sort_clanmembers(clan);
				save_clans();
			}
		}
	}
}

ACMD(do_clanpasswd)
{
	struct special_search_data *srch = NULL;
	struct clan_data *clan = NULL;

	if (!GET_CLAN(ch) || !PLR_FLAGGED(ch, PLR_CLAN_LEADER)) {
		send_to_char(ch, "Only clan leaders can do this.\r\n");
		return;
	}
	if (!(clan = real_clan(GET_CLAN(ch)))) {
		send_to_char(ch, "Something about your clan is screwed up.\r\n");
		return;
	}
	if (!ROOM_FLAGGED(ch->in_room, ROOM_CLAN_HOUSE)) {
		send_to_char(ch, "You can't set any clan passwds in this room.\r\n");
		return;
	}
	if (!clan_house_can_enter(ch, ch->in_room)) {
		send_to_char(ch, "You can't set passwds in other people's clan house.\r\n");
		return;
	}

	skip_spaces(&argument);
	if (!*argument) {
		send_to_char(ch, "Set the clanpasswd to what?\r\n");
		return;
	}

	for (srch = ch->in_room->search; srch; srch = srch->next) {
		if (srch->command == SEARCH_COM_TRANSPORT &&
			SRCH_FLAGGED(srch, SRCH_CLANPASSWD))
			break;
	}

	if (!srch) {
		send_to_char(ch, "There is no clanpasswd search in this room.\r\n");
		return;
	}

	free(srch->keywords);
	srch->keywords = str_dup(argument);
	if (!save_wld(ch, ch->in_room->zone))
		send_to_char(ch, "New clan password set here to '%s'.\r\n", argument);
	else
		send_to_char(ch, "There was a problem saving your clan password.\r\n");
}

struct clan_data *
real_clan(int vnum)
{
	struct clan_data *clan = NULL;

	if (!(vnum))
		return NULL;

	for (clan = clan_list; clan; clan = clan->next)
		if (clan->number == vnum)
			return (clan);

	return (NULL);
}

struct clan_data*
clan_by_owner( int idnum )
{
	for (clan_data *clan = clan_list; clan; clan = clan->next)
		if( clan->owner == idnum )
			return clan;
	return NULL;
}

struct clan_data *
clan_by_name(char *arg)
{
	struct clan_data *clan = NULL;
	int clan_num = -1;

	if (is_number(arg))
		clan_num = atoi(arg);

	skip_spaces(&arg);
	if (!*arg)
		return NULL;

	for (clan = clan_list; clan; clan = clan->next)
		if (is_abbrev(arg, clan->name) || clan->number == clan_num)
			return (clan);

	return (NULL);
}

struct clanmember_data *
real_clanmember(long idnum, struct clan_data *clan)
{

	struct clanmember_data *member;
	for (member = clan->member_list; member; member = member->next)
		if (member->idnum == idnum)
			return (member);

	return (NULL);
};

typedef struct cedit_command_data {
	char *keyword;
	int level;
} cedit_command_data;

cedit_command_data cedit_keys[] = {
	{"save", 0},
	{"create", 1},
	{"delete", 1},
	{"set", 0},
	{"show", 0},
	{"add", 0},
	{"remove", 0},
	{"sort", 1},
	{NULL, 0}
};

ACMD(do_cedit)
{

	struct clan_data *clan = NULL;
	struct clanmember_data *member = NULL;
	struct room_list_elem *rm_list = NULL;
	struct room_data *room = NULL;
	int cedit_command, i, j;
	char *arg1, *arg2, *arg3;

	skip_spaces(&argument);

	arg1 = tmp_getword(&argument);
	for (cedit_command = 0; cedit_keys[cedit_command].keyword; cedit_command++)
		if (!strcasecmp(arg1, cedit_keys[cedit_command].keyword))
			break;

	if (!cedit_keys[cedit_command].keyword) {
		send_to_char(ch, 
			"Valid cedit options:  save, create, delete(*), set, show, add, remove.\r\n");
		return;
	}

	if (!Security::isMember(ch, "Clan")) {
		send_to_char(ch, "Sorry, you can't use the cedit command.\r\n");
	}

	if (cedit_keys[cedit_command].level == 1
		&& !Security::isMember(ch, "WizardFull")) {
		send_to_char(ch, "Sorry, you can't use that cedit command.\r\n");
		return;
	}

	arg1 = tmp_getword(&argument);
	if (*arg1) {
		if (is_number(arg1))
			clan = real_clan(atoi(arg1));
		else
			clan = clan_by_name(arg1);

		// protect quaker and null
		if (clan && GET_LEVEL(ch) < LVL_CREATOR && (clan->number == 0
				|| clan->number == 6)) {
			send_to_char(ch, "Sorry, you cannot edit this clan.\r\n");
			return;
		}

	}

	switch (cedit_command) {

	case 0:			 /*** save ***/
		if (save_clans()) {
			send_to_char(ch, "Clans saved successfully.\r\n");
		} else {
			send_to_char(ch, "Clans failed to save!\r\n");
		}
		break;

	case 1:			/*** create ***/
		{
			int clan_number = 0;
			if (!*arg1) {
				send_to_char(ch, "Create clan with what vnum?\r\n");
				break;
			}
			if (!is_number(arg1)) {
				send_to_char(ch, "You must specify the clan numerically.");
				break;
			}
			clan_number = atoi(arg1);
			if (clan_number <= 0) {
				send_to_char(ch, "You must specify the clan numerically.");
				break;
			} else if (real_clan(atoi(arg1))) {
				send_to_char(ch, "A clan already exists with that vnum.\r\n");
				break;
			} else if ((clan = create_clan(atoi(arg1)))) {
				send_to_char(ch, "Clan created.\r\n");
				slog("(cedit) %s created clan %d.", GET_NAME(ch),
					clan->number);
			} else {
				send_to_char(ch, "There was an error creating the clan.\r\n");
			}
			break;
		}
	case 2:			  /*** delete ***/
		if (!clan) {
			if (!*arg1)
				send_to_char(ch, "Delete what clan?\r\n");
			else {
				send_to_char(ch, "Clan '%s' does not exist.\r\n", arg1);
			}
			return;
		} else {
			i = clan->number;
			if (!delete_clan(clan)) {
				send_to_char(ch, "Clan deleted.  Sucked anyway.\r\n");
				slog("(cedit) %s deleted clan %d.", GET_NAME(ch), i);
			} else
				send_to_char(ch, "ERROR occured while deleting clan.\r\n");
		}
		break;

	case 3:			  /*** set    ***/
		if (!*arg1) {
			send_to_char(ch, 
				"Usage: cedit set <vnum> <name|badge|rank|bank|member|owner>['top']<value>\r\n");
			return;
		}

		arg2 = tmp_getword(&argument);
		skip_spaces(&argument);

		if (!*argument || !*arg2) {
			send_to_char(ch, 
				"Usage: cedit set <clan> <name|badge|rank|bank|member|owner>['top']<value>\r\n");
			return;
		}
		if (!clan) {
			send_to_char(ch, "Clan '%s' does not exist.\r\n", arg1);
			return;
		}
		// cedit set name
		if (is_abbrev(arg2, "name")) {
			if (strlen(argument) > MAX_CLAN_NAME) {
				send_to_char(ch, "Name too long.  Maximum %d characters.\r\n",
					MAX_CLAN_NAME);
				return;
			}
			if (clan->name) {
				free(clan->name);
			}
			clan->name = str_dup(argument);
			slog("(cedit) %s set clan %d name to '%s'.", GET_NAME(ch),
				clan->number, clan->name);

		}
		// cedit set badge
		else if (is_abbrev(arg2, "badge")) {
			if (strlen(argument) > MAX_CLAN_BADGE - 1) {
				send_to_char(ch, "Badge too long.  Maximum %d characters.\r\n",
					MAX_CLAN_BADGE - 1);
				return;
			}
			if (clan->badge) {
				free(clan->badge);
			}
			clan->badge = str_dup(argument);
			slog("(cedit) %s set clan %d badge to '%s'.", GET_NAME(ch),
				clan->number, clan->badge);

		}
		// cedit set rank
		else if (is_abbrev(arg2, "rank")) {

			arg3 = tmp_getword(&argument);
			skip_spaces(&argument);

			if (is_abbrev(arg3, "top")) {
				if (!*argument) {
					send_to_char(ch, "Set top rank of clan to what?\r\n");
					return;
				}
				if (!is_number(argument) || (i = atoi(argument)) < 0 ||
					i >= NUM_CLAN_RANKS) {
					send_to_char(ch,
						"top rank must be a number between 0 and %d.\r\n",
						NUM_CLAN_RANKS - 1);
					return;
				}
				clan->top_rank = i;
				send_to_char(ch, "Top rank of clan set.\r\n");

				slog("(cedit) %s set clan %d top to %d.", GET_NAME(ch),
					clan->number, clan->top_rank);

				return;
			}

			if (!is_number(arg3) || (i = atoi(arg3)) < 0 || i > clan->top_rank) {
				send_to_char(ch, "[rank] must be a number between 0 and %d.\r\n",
					clan->top_rank);
				return;
			}
			if (!*argument) {
				send_to_char(ch, "Set that rank to what name?\r\n");
				return;
			}
			if (strlen(argument) >= MAX_CLAN_RANKNAME) {
				send_to_char(ch, "Rank names may not exceed %d characters.\r\n",
					MAX_CLAN_RANKNAME - 1);
				return;
			}
			if (clan->ranknames[i]) {
				free(clan->ranknames[i]);
			}
			clan->ranknames[i] = str_dup(argument);

			send_to_char(ch, "Rank title set.\r\n");
			slog("(cedit) %s set clan %d rank %d to '%s'.",
				GET_NAME(ch), clan->number, i, clan->ranknames[i]);

			return;

		}
		// cedit set bank
		else if (is_abbrev(arg2, "bank")) {
			if (!*argument) {
				send_to_char(ch, "Set the bank account of the clan to what?\r\n");
				return;
			}
			if( !is_number(argument) ) {
				send_to_char(ch, 
					"Try setting the bank account to an appropriate number asswipe.\r\n");
				return;
			} 
			i = atoi(argument);
			if( i < 0 ) {
				send_to_char(ch, 
					"This clan has no overdraft protection. Negative value invalid.\r\n");
				return;
			}
			slog("(cedit) %s set clan %d bank from %d to %d.", GET_NAME(ch),
				clan->number, clan->bank_account, i );
			send_to_char(ch, "Clan bank account set from %d to %d\r\n",
					clan->bank_account, i );
			clan->bank_account = i;

			return;

		}
		// cedit set owner
		else if (is_abbrev(arg2, "owner")) {
			if (!*argument) {
				send_to_char(ch, "Set the owner of the clan to what?\r\n");
				return;
			}
			if ((i = playerIndex.getID(argument)) < 0) {
				send_to_char(ch, "No such person.\r\n");
				return;
			}
			clan->owner = i;
			send_to_char(ch, "Clan owner set.\r\n");
			slog("(cedit) %s set clan %d owner to %s.", GET_NAME(ch),
				clan->number, argument);
			return;
		}
		// cedit set member
		else if (is_abbrev(arg2, "member")) {
			if (!*argument) {
				send_to_char(ch, 
					"Usage: cedit set <clan> member <member> <rank>\r\n");
				return;
			}
			arg3 = tmp_getword(&argument);
			arg1 = tmp_getword(&argument);

			if (!is_number(arg3)) {
				if ((i = playerIndex.getID(arg3)) < 0) {
					send_to_char(ch, "There is no such player in existence...\r\n");
					return;
				}
			} else if ((i = atoi(arg3)) < 0) {
				send_to_char(ch, "Such a comedian.\r\n");
				return;
			}

			if (!*arg1) {
				send_to_char(ch, 
					"Usage: cedit set <clan> member <member> <rank>\r\n");
				return;
			}

			j = atoi(arg1);

			for (member = clan->member_list; member; member = member->next)
				if (member->idnum == i) {
					member->rank = j;
					send_to_char(ch, "Member rank set.\r\n");
					slog("(cedit) %s set clan %d member %d rank to %d.",
						GET_NAME(ch), clan->number, i, member->rank);
					break;
				}

			if (!member)
				send_to_char(ch, "Unable to find that member.\r\n");
			return;

		} else {
			send_to_char(ch, "Unknown option '%s'.\r\n", arg2);
			return;
		}

		send_to_char(ch, "Successful.\r\n");
		break;

	case 4:			  /*** show   ***/

		if (!clan && *arg1) {
			send_to_char(ch, "Clan '%s' does not exist.\r\n", arg1);
			return;
		}
		do_show_clan(ch, clan);
		break;

	case 5:			  /*** add    ***/
		if (!*arg1) {
			send_to_char(ch, "Usage: cedit add <clan> <room|member> <number>\r\n");
			return;
		}
		arg2 = tmp_getword(&argument);
		arg3 = tmp_getword(&argument);

		if (!*arg3 || !*arg2) {
			send_to_char(ch, "Usage: cedit add <clan> <room|member> <number>\r\n");
			return;
		}
		if (!clan) {
			send_to_char(ch, "Clan '%s' does not exist.\r\n", arg1);
			return;
		}
		// cedit add room
		if (is_abbrev(arg2, "room")) {
			if (!(room = real_room(atoi(arg3)))) {
				send_to_char(ch, "No room with number %s exists.\r\n", arg3);
				return;
			}
			for (rm_list = clan->room_list; rm_list; rm_list = rm_list->next)
				if (rm_list->room == room) {
					send_to_char(ch, 
						"This room is already a part of the clan, dumbass.\r\n");
					return;
				}


			CREATE(rm_list, struct room_list_elem, 1);
			rm_list->room = room;
			rm_list->next = clan->room_list;
			clan->room_list = rm_list;
			send_to_char(ch, "Room added.\r\n");

			slog("(cedit) %s added room %d to clan %d.", GET_NAME(ch),
				room->number, clan->number);

			return;
		}
		// cedit add member
		else if (is_abbrev(arg2, "member")) {
			if (!is_number(arg3)) {
				if ((i = playerIndex.getID(arg3)) < 0) {
					send_to_char(ch, "There exists no player with that name.\r\n");
					return;
				}
			} else if ((i = atoi(arg3)) < 0) {
				send_to_char(ch, "Real funny... reeeeeaaal funny.\r\n");
				return;
			}
			for (member = clan->member_list; member; member = member->next) {
				if (member->idnum == i) {
					send_to_char(ch, 
						"That player is already on the member list.\r\n");
					return;
				}
			}
			CREATE(member, struct clanmember_data, 1);

			member->idnum = i;
			member->rank = 0;
			member->next = clan->member_list;
			clan->member_list = member;

			send_to_char(ch, "Clan member added to list.\r\n");;

			slog("(cedit) %s added member %d to clan %d.",
				GET_NAME(ch), (int)member->idnum, clan->number);

			return;
		} else {
			send_to_char(ch, "Invalid command, punk.\r\n");
		}
		break;
	case 6:			  /** remove ***/

		if (!*arg1) {
			send_to_char(ch, 
				"Usage: cedit remove <clan> <room|member> <number>\r\n");
			return;
		}
		arg2 = tmp_getword(&argument);
		arg3 = tmp_getword(&argument);

		if (!*arg3 || !*arg2) {
			send_to_char(ch, 
				"Usage: cedit remove <clan> <room|member> <number>\r\n");
			return;
		}
		if (!clan) {
			send_to_char(ch, "Clan '%s' does not exist.\r\n", arg1);
			return;
		}

		if (is_abbrev(arg2, "room")) {
			if (!(room = real_room(atoi(arg3)))) {
				send_to_char(ch, "No room with number %s exists.\r\n", arg3);
			}
			for (rm_list = clan->room_list; rm_list; rm_list = rm_list->next)
				if (rm_list->room == room)
					break;

			if (!rm_list) {
				send_to_char(ch, "That room is not part of the list, dorkus.\r\n");
				return;
			}

			REMOVE_ROOM_FROM_CLAN(rm_list, clan);
			free(rm_list);
			send_to_char(ch, 
				"Room removed and memory freed.  Thank you.. Call again.\r\n");

			slog("(cedit) %s removed room %d from clan %d.",
				GET_NAME(ch), room->number, clan->number);

			return;

		}
		// cedit remove member
		else if (is_abbrev(arg2, "member")) {
			if (!is_number(arg3)) {
				if ((i = playerIndex.getID(arg3)) < 0) {
					send_to_char(ch, "There exists no player with that name.\r\n");
					return;
				}
			} else if ((i = atoi(arg3)) < 0) {
				send_to_char(ch, "Real funny... reeeeeaaal funny.\r\n");
				return;
			}
			for (member = clan->member_list; member; member = member->next)
				if (member->idnum == i)
					break;

			if (!member) {
				send_to_char(ch, "That player is not a part of this clan.\r\n");
				return;
			}

			REMOVE_MEMBER_FROM_CLAN(member, clan);
			free(member);
			send_to_char(ch, "Member removed from the sacred list.\r\n");

			slog("(cedit) %s removed member %d from clan %d.",
				GET_NAME(ch), i, clan->number);

			return;
		} else
			send_to_char(ch, "YO!  Remove members, or rooms... geez!\r\n");

		break;

	case 7: /** sort **/
		if (!clan) {
			if (!*arg1)
				send_to_char(ch, "Sort what clan?\r\n");
			else {
				send_to_char(ch, "Clan '%s' does not exist.\r\n", arg1);
			}
			return;
		}
		sort_clanmembers(clan);
		send_to_char(ch, "Clanmembers sorted.\r\n");
		break;

	default:
		send_to_char(ch, "Unknown cedit option.\r\n");
		break;
	}
}


void
load_clan( xmlNodePtr clanNode )
{
	clan_data *clan;
	Creature clan_member(true);

	CREATE(clan, struct clan_data, 1);

	clan->member_list = NULL;
	clan->room_list = NULL;
	clan->next = NULL;

	clan->number = xmlGetIntProp( clanNode, "id", -1 );
	clan->name = xmlGetProp( clanNode, "name"  );
	clan->badge = xmlGetProp( clanNode, "badge" );

	//rentOverflow = xmlGetLongProp( clanNode, "rentOverflow", 0 );
	
	int curRank = 0;
	for ( xmlNodePtr node = clanNode->xmlChildrenNode; node; node = node->next ) 
	{
        if ( xmlMatches(node->name, "data") ) {
			clan->owner = xmlGetLongProp( node, "owner", 0 );
			clan->top_rank = xmlGetIntProp( node, "top_rank", -1 );
			clan->bank_account = xmlGetIntProp( node, "bank", -1 );
			clan->flags = xmlGetIntProp( node, "flags", -1 );
		} else if (xmlMatches(node->name, "member")) {
			long id = xmlGetLongProp( node, "id", -1 );
			int rank = xmlGetIntProp( node, "rank", -1 );
			clan_member.clear();

			if( !playerIndex.exists(id) ) {
				slog("SYSERR: Clan %d had invalid member: %ld.", clan->number, id);
			} else if( clan_member.loadFromXML(id)) {
				if (clan_member.player_specials->saved.clan != clan->number) {
					slog("Clan(%d) member (%ld) no longer a member.",
						clan->number, id);
					continue;
				}
			}
			clanmember_data *member;
			CREATE(member, struct clanmember_data, 1);
			member->idnum = id;
			member->rank = rank;
			member->next = NULL;
			if (!clan->member_list)
				clan->member_list = member;
			else {
				for (clanmember_data *tmp_member = clan->member_list; tmp_member;
					tmp_member = tmp_member->next) {
					if (!tmp_member->next) {
						tmp_member->next = member;
						break;
					}
				}
			}
		} else if( xmlMatches(node->name, "rank")) {
			char *name = xmlGetProp( node, "name" );
			if( name == NULL ) {
				clan->ranknames[curRank++] = strdup("the member");
			} else {
				clan->ranknames[curRank++] = name;
			}
		} else if( xmlMatches(node->name, "room")) {
			int number = xmlGetIntProp( node, "number", -1 );
			room_data *room = real_room(number);
			if( room != NULL ) {
				room_list_elem *rm_elem;
				CREATE(rm_elem, struct room_list_elem, 1);
				rm_elem->room = room;
				rm_elem->next = clan->room_list;
				clan->room_list = rm_elem;
			}
		}
	}

	if (!clan_list) {
		clan_list = clan;
	} else {
		for (clan_data *tmp_clan = clan_list; tmp_clan; tmp_clan = tmp_clan->next) {
			if (!tmp_clan->next) {
				tmp_clan->next = clan;
				break;
			}
		}
	}
}

#define CLAN_FILE "etc/clans"

void
boot_old_clans()
{

	FILE *file = NULL;
	int virt = -1, last = -1, i = 0;
	long member_id;
	byte member_rank;
	struct clan_file_elem_hdr clan_hdr;
	struct clan_data *clan = NULL, *tmp_clan = NULL;
	struct clanmember_data *member = NULL, *tmp_member = NULL;
	struct room_data *room = NULL;
	struct room_list_elem *rm_list = NULL;
	Creature clan_member(true);

	clan_list = NULL;

	if (!(file = fopen(CLAN_FILE, "r"))) {
		slog("Unable to open clan file '%s' for read.\n", CLAN_FILE);
		return;
	}

	while (fread(&clan_hdr, sizeof(struct clan_file_elem_hdr), 1, file)) {

		if (clan_hdr.number < last) {
			slog("Format error in clan file.  Clan %d after clan %d.\n",
				clan_hdr.number, last);
			return;
		}

		last = clan_hdr.number;

		CREATE(clan, struct clan_data, 1);

		clan->number = clan_hdr.number;
		clan->bank_account = clan_hdr.bank_account;
		clan->top_rank = clan_hdr.top_rank;
		clan->owner = clan_hdr.owner;
		clan->flags = clan_hdr.flags;
		clan->name = str_dup(clan_hdr.name);
		clan->badge = str_dup(clan_hdr.badge);
		clan->member_list = NULL;
		clan->room_list = NULL;
		clan->next = NULL;
		for (i = 0; i < NUM_CLAN_RANKS; i++) {
			if (clan_hdr.ranknames[i][0])
				clan->ranknames[i] = str_dup(clan_hdr.ranknames[i]);
			else {
				if (!(i))
					clan->ranknames[i] = str_dup("the recruit");
				else if (i == clan->top_rank)
					clan->ranknames[i] = str_dup("the leader");
				else
					clan->ranknames[i] = str_dup("the member");
			}
		}

		for (i = 0; i < clan_hdr.num_members; i++) {

			fread(&member_id, sizeof(long), 1, file);
			fread(&member_rank, sizeof(byte), 1, file);

			if (!playerIndex.getName(member_id)) {
				slog("Clan(%d) member (%ld) does not exist",
					clan->number, member_id);
				continue;
			}
			clan_member.clear();

			if (clan_member.loadFromXML(member_id)) {
				if (clan_member.player_specials->saved.clan != clan->number) {
					slog("Clan(%d) member (%ld) no longer a member.",
						clan->number, member_id);
					continue;
				}
			} else
				slog("ERROR");

			CREATE(member, struct clanmember_data, 1);
			member->idnum = member_id;
			member->rank = member_rank;
			member->next = NULL;
			if (!clan->member_list)
				clan->member_list = member;
			else {
				for (tmp_member = clan->member_list; tmp_member;
					tmp_member = tmp_member->next) {
					if (!tmp_member->next) {
						tmp_member->next = member;
						break;
					}
				}
			}
		}

		for (i = 0; i < clan_hdr.num_rooms; i++) {
			fread(&virt, sizeof(int), 1, file);

			if ((room = real_room(virt))) {
				CREATE(rm_list, struct room_list_elem, 1);
				rm_list->room = room;
				rm_list->next = clan->room_list;
				clan->room_list = rm_list;
			}
		}

		if (!clan_list)
			clan_list = clan;
		else {
			for (tmp_clan = clan_list; tmp_clan; tmp_clan = tmp_clan->next)
				if (!tmp_clan->next) {
					tmp_clan->next = clan;
					break;
				}
		}
	}
	fclose(file);
	slog("CLAN: Clans booted from binary files.");
}


bool
boot_clans()
{
	const char* filename = "etc/clans.xml";
	int axs = access(filename, R_OK);

	if( axs != 0 ) {
		if( errno == ENOENT ) {
			slog("SYSERR: XML clan file not found. Trying binary file.");
			boot_old_clans();
			return false;
		} else {
			slog("SYSERR: Unable to open xml clan file '%s': %s", 
				 filename, strerror(errno) );
			return false;
		}
	}
    xmlDocPtr doc = xmlParseFile(filename);
    if (!doc) {
        slog("SYSERR: XML parse error while loading %s", filename);
        return false;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root) {
        xmlFreeDoc(doc);
        slog("SYSERR: XML file %s is empty", filename);
        return false;
    }

	int clanCount = 0;
	xmlNodePtr clanNode;
	for ( clanNode = root->xmlChildrenNode; clanNode; clanNode = clanNode->next ) {
		if( xmlMatches( clanNode->name, "clan" ) ) {
			load_clan( clanNode );
			clanCount += 1;
		}
	}
	xmlFreeDoc(doc);
	slog("Clan boot successful.");
	return true;
}

bool
save_clans()
{
	const char* path = "etc/clans.xml";
	FILE* ouf = fopen(path, "w");
	if(!ouf) {
		slog("Unable to open clan file for save.[%s] (%s)\n",
						path, strerror(errno) );
		return false;
	}
	fprintf( ouf, "<clanfile>\n");
	for( clan_data *clan = clan_list; clan; clan = clan->next) {
		char *name = xmlEncodeTmp(clan->name);
		char *badge = xmlEncodeTmp(clan->badge);
		fprintf( ouf, "    <clan id=\"%d\" name=\"%s\" badge=\"%s\">\n", 
					clan->number, name, badge);
		fprintf( ouf, "        <data owner=\"%ld\" top_rank=\"%d\" bank=\"%d\" flags=\"%d\" />\n",
					clan->owner, clan->top_rank, clan->bank_account, clan->flags );

		for( int i = 0; i <= clan->top_rank; i++ ) {
			if (clan->ranknames[i]) {
				char *rank = xmlEncodeTmp(clan->ranknames[i]);
				fprintf( ouf, "        <rank name=\"%s\"/>\n", rank );
			} else {
				fprintf( ouf, "        <rank name=\"\"/>\n" );
			}
		}

		for( clanmember_data *member = clan->member_list; member; member = member->next) {
			fprintf( ouf, "        <member id=\"%ld\" rank=\"%d\" />\n", 
					member->idnum, member->rank );
		}

		for( room_list_elem *rm_list = clan->room_list; rm_list; rm_list = rm_list->next) {
			fprintf( ouf, "        <room number=\"%d\" />\n", 
					rm_list->room->number );
		}
		fprintf( ouf, "    </clan>\n" );
	}
	fprintf( ouf, "</clanfile>\n");
	fclose(ouf);

	return true;
}

struct clan_data *
create_clan(int vnum)
{

	struct clan_data *newclan = NULL, *clan = NULL;
	int i;

	CREATE(newclan, struct clan_data, 1);

	newclan->number = vnum;
	newclan->bank_account = 0;
	newclan->top_rank = 0;
	newclan->name = str_dup("New");
	newclan->badge = str_dup("(//NEW\\\\)");
	for (i = 0; i < NUM_CLAN_RANKS; i++)
		newclan->ranknames[i] = NULL;

	newclan->member_list = NULL;
	newclan->room_list = NULL;
	newclan->next = NULL;

	if (!clan_list || newclan->number < clan_list->number) {
		newclan->next = clan_list;
		clan_list = newclan;
	} else {
		for (clan = clan_list; clan; clan = clan->next)
			if (!clan->next ||
				(clan->number < vnum && clan->next->number > vnum)) {
				newclan->next = clan->next;
				clan->next = newclan;
				break;
			}
	}

	return (newclan);
}

int
delete_clan(struct clan_data *clan)
{

	struct clan_data *tmp_clan = NULL;
	struct room_list_elem *rm_list = NULL;
	struct clanmember_data *member = NULL;
	int i;

	if (clan_list == clan) {
		clan_list = clan->next;
	} else {
		for (tmp_clan = clan_list; tmp_clan; tmp_clan = tmp_clan->next)
			if (tmp_clan->next && tmp_clan->next == clan) {
				tmp_clan->next = clan->next;
				break;
			}

		if (!tmp_clan)
			return 1;
	}

	if (clan->name) {
		free(clan->name);
	}
	if (clan->badge) {
		free(clan->badge);
	}
	for (i = 0; i < NUM_CLAN_RANKS; i++)
		if (clan->ranknames[i]) {
			free(clan->ranknames[i]);
		}

	for (member = clan->member_list; member; member = clan->member_list) {
		clan->member_list = member->next;
		free(member);
	}

	for (rm_list = clan->room_list; rm_list; rm_list = clan->room_list) {
		clan->room_list = rm_list->next;
		free(rm_list);
	}
	free(clan);
	return 0;
}



void
do_show_clan(struct Creature *ch, struct clan_data *clan)
{
	struct clanmember_data *member = NULL;
	struct room_list_elem *rm_list = NULL;
	int i, num_rooms = 0, num_members = 0;
	char *msg;

	if (clan) {
		msg = tmp_sprintf(
			"CLAN %d - Name: %s%s%s, Badge: %s%s%s, Top Rank: %d, Bank: %d\r\n",
			clan->number, CCCYN(ch, C_NRM), clan->name, CCNRM(ch, C_NRM),
			CCCYN(ch, C_NRM), clan->badge, CCNRM(ch, C_NRM), clan->top_rank,
			clan->bank_account);

		if (GET_LEVEL(ch) > LVL_AMBASSADOR)
			msg = tmp_sprintf("%sOwner: %ld (%s), Flags: %d\r\n",
				msg, clan->owner, playerIndex.getName(clan->owner), clan->flags);

		for (i = clan->top_rank; i >= 0; i--) {
			msg = tmp_sprintf("%sRank %2d: %s%s%s\r\n", msg, i,
				CCYEL(ch, C_NRM),
				clan->ranknames[i] ? clan->ranknames[i] : !(i) ? "recruit" :
				"member", CCNRM(ch, C_NRM));
		}
		
		msg = tmp_strcat(msg, "ROOMS:\r\n",NULL);

		for (rm_list = clan->room_list, num_rooms = 0;
			rm_list; rm_list = rm_list->next) {
			num_rooms++;
			msg = tmp_sprintf("%s%3d) %5d.  %s%s%s\r\n", msg, num_rooms,
				rm_list->room->number,
				CCCYN(ch, C_NRM), rm_list->room->name, CCNRM(ch, C_NRM));
		}
		if (!num_rooms)
			msg = tmp_strcat(msg, "None.\r\n",NULL);

		msg = tmp_strcat(msg, "MEMBERS:\r\n",NULL);

			for (member = clan->member_list, num_members = 0;
				member; member = member->next) {
				num_members++;
				msg = tmp_sprintf("%s%3d) %5d -  %s%20s%s  Rank: %d\r\n", msg,
					num_members, (int)member->idnum,
					CCYEL(ch, C_NRM), playerIndex.getName(member->idnum),
					CCNRM(ch, C_NRM), member->rank);
			}
			msg = tmp_strcat(msg, "None.\r\n",NULL);

	} else {
		msg = tmp_strdup("CLANS:\r\n");
		for (clan = clan_list; clan; clan = clan->next) {
			for (member = clan->member_list, num_members = 0; member;
				num_members++, member = member->next);

			msg = tmp_sprintf("%s %3d - %s%20s%s  %s%20s%s  (%3d members)\r\n",
				msg,
				clan->number,
				CCCYN(ch, C_NRM), clan->name, CCNRM(ch, C_NRM),
				CCCYN(ch, C_NRM), clan->badge, CCNRM(ch, C_NRM), num_members);
		}
	}
	page_string(ch->desc, msg);
}

int
clan_house_can_enter(struct Creature *ch, struct room_data *room)
{
	struct clan_data *clan = NULL, *ch_clan = NULL;
	struct room_list_elem *rm_list = NULL;

	if (!ROOM_FLAGGED(room, ROOM_CLAN_HOUSE))
		return 1;
	if (GET_LEVEL(ch) >= LVL_DEMI)
		return 1;
	if (Security::isMember(ch, "Clan"))
		return 1;
	if (!(ch_clan = real_clan(GET_CLAN(ch))))
		return 0;

	for (clan = clan_list; clan; clan = clan->next) {
		if (clan == ch_clan)
			continue;
		for (rm_list = clan->room_list; rm_list; rm_list = rm_list->next)
			if (rm_list->room == room)
				return 0;
	}

	return 1;
}

void
sort_clanmembers(struct clan_data *clan)
{

	struct clanmember_data *new_list = NULL, *i = NULL, *j = NULL, *k = NULL;

	if (!clan->member_list)
		return;

	for (i = clan->member_list; i; i = j) {
		j = i->next;
		if (!new_list || (new_list->rank < i->rank)) {
			i->next = new_list;
			new_list = i;
		} else {
			k = new_list;
			while (k->next)
				if (k->next->rank < i->rank)
					break;
				else
					k = k->next;
			i->next = k->next;
			k->next = i;
		}
	}
	clan->member_list = new_list;
}
