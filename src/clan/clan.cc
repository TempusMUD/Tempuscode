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
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "house.h"
#include "clan.h"
#include "char_class.h"

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;
extern struct spell_info_type spell_info[];
int check_mob_reaction(struct char_data *ch, struct char_data *vict);
int save_wld (struct char_data *ch);

struct clan_data *clan_list;
extern FILE *player_fl;
int player_i=0;


#define NAME(x) ((temp = get_name_by_id(x)) == NULL ? "<UNDEF>" : temp)

#define LOG_CLANSAVE(ch, string)
/*
{sprintf(buf, "%s clan-saved from %s", ch ? GET_NAME(ch) : "NULL", string);\
 slog(buf);}
*/

void
REMOVE_ROOM_FROM_CLAN(struct room_list_elem *rm_list,
		      struct clan_data *clan)
{
    struct room_list_elem *temp;
    REMOVE_FROM_LIST(rm_list, clan->room_list, next);
}
void
REMOVE_MEMBER_FROM_CLAN(struct clanmember_data *member, 
			struct clan_data *clan)
{
    struct clanmember_data *temp;
    REMOVE_FROM_LIST(member, clan->member_list, next);
}

ACMD(do_enroll)
{
    struct char_data *vict = 0;
    struct clan_data *clan = real_clan(GET_CLAN(ch));
    struct clanmember_data *member = NULL;
    int count = 0;

    skip_spaces(&argument);

    if (!clan)
	send_to_char("Hmm... You need to be in a clan yourself, first.\r\n", ch);
    else if (!*argument) 
	send_to_char("Ummm... enroll who?\r\n", ch);
    else if (!(vict = get_char_room_vis(ch, argument)))
	send_to_char("Enroll who?\r\n", ch);
    else if (vict == ch) 
	send_to_char("Yeah, yeah, yeah... enroll yourself, huh?\r\n", ch);
    else if (!PLR_FLAGGED(ch, PLR_CLAN_LEADER) && GET_LEVEL(ch) < LVL_IMMORT &&
	     (!(member = real_clanmember(GET_IDNUM(ch), clan)) ||
	      member->rank < clan->top_rank))
	send_to_char("You are not the leader of the clan!\r\n", ch);
    else if (IS_MOB(vict) && GET_LEVEL(ch) < LVL_CREATOR)
	send_to_char("What's the point of that?\r\n", ch);
    else if (real_clan(GET_CLAN(vict))) {
	if (GET_CLAN(vict) == GET_CLAN(ch)) {
	    if (!(member = real_clanmember(GET_IDNUM(vict), clan))) {
		send_to_char("Clan flag purged on vict.  Re-enroll.\r\n", ch);
		GET_CLAN(vict) = 0;
	    } else
		send_to_char("Duhhh...\r\n", ch);
	} else
	    send_to_char("You cannot while they are a member of another clan.\r\n", ch);
    } else if (PLR_FLAGGED(vict, PLR_FROZEN))
	send_to_char("They are frozen right now.  Wait until a god has mercy.\r\n"
		     "Don't hold your breath.\r\n", ch);
    else if (GET_LEVEL(vict) < LVL_CAN_CLAN)
	send_to_char("Players must be level 10 before being inducted into the clan.\r\n", ch);
    else if ((member = real_clanmember(GET_IDNUM(vict), clan))) {
	send_to_char("Something wierd just happened... try again.\r\n", ch);
	REMOVE_MEMBER_FROM_CLAN(member, clan);
	free(member);
	LOG_CLANSAVE(ch, "do_enroll");
	save_clans();
    } else {

	for (count = 0,member = clan->member_list; member; 
	     count++, member = member->next);
	if (count >= MAX_CLAN_MEMBERS) {
	    send_to_char("The max number of members has been reached for this clan.\r\n", ch);
	    return;
	}
    REMOVE_BIT(PLR_FLAGS(vict),PLR_CLAN_LEADER);
	sprintf(buf, "You have been inducted into clan %s by %s!\r\n", 
		clan->name, GET_NAME(ch));
	send_to_char(buf, vict);
	sprintf(buf, "%s has been inducted into clan %s by %s!", GET_NAME(vict), 
		clan->name, GET_NAME(ch));
	mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), 1);
	strcat(buf, "\r\n");
	send_to_clan(buf, GET_CLAN(ch));
	GET_CLAN(vict) = clan->number;
	CREATE(member, struct clanmember_data, 1);
	member->idnum = GET_IDNUM(vict);
	member->rank  = 0;
	member->next  = clan->member_list;
	clan->member_list = member;
	sort_clanmembers(clan);
	LOG_CLANSAVE(ch, "do_induct");
	save_clans();
    }
}
    
ACMD(do_dismiss)
{
    struct char_data *vict;
    struct clan_data *clan = real_clan(GET_CLAN(ch));
    struct clanmember_data *member = NULL, *member2 = NULL;
    char arg[MAX_INPUT_LENGTH];
    bool in_file = false;
    long idnum = -1;
    struct char_file_u tmp_store;


    one_argument(argument, arg);
    skip_spaces(&argument);
  
    if (!clan) {
        send_to_char("Try joining a clan first.\r\n", ch);
        return;
    } else if (!*argument) {
        send_to_char("Ummm... dismiss who?\r\n", ch);
        return;
    }
    /* Find the player. */
    if ((idnum = get_id_by_name(arg)) < 0) {
        send_to_char("There is no character named '%s'\r\n", ch);
        return;
    }

    if (!(vict = get_char_in_world_by_idnum(idnum))) {
        // load the char from file
        CREATE(vict, CHAR, 1);
        clear_char(vict);
        in_file = true;

        if ((player_i = load_char(arg, &tmp_store)) > -1) {
            store_to_char(&tmp_store, vict);
        } else {
            free(vict);
            send_to_char("Error loading char from file.\r\n", ch);
            return;
        }
    } 

    /* Was the player found? */
    if (!vict) {
        send_to_char("Dismiss who?\r\n", ch);
        if(in_file)
            free(vict);
        return;
    }
    if (vict == ch) 
        send_to_char("Thats real damn funny.\r\n", ch);
    else if (!PLR_FLAGGED(ch, PLR_CLAN_LEADER) && GET_LEVEL(ch) < LVL_IMMORT)
        send_to_char("You are not the leader of the clan!\r\n", ch);
    else if (GET_LEVEL(vict) >= LVL_GRGOD && GET_LEVEL(ch) < LVL_GRGOD)
        send_to_char("I don't think you should do that.\r\n", ch);
    else if (GET_CLAN(vict) != GET_CLAN(ch))
        send_to_char("Umm, why dont you check the clan list, okay?\r\n", ch);
    else if ((member = real_clanmember(GET_IDNUM(ch), clan)) &&
	     (member2 = real_clanmember(GET_IDNUM(vict), clan)) &&
	     (member->rank <= member2->rank && member->rank < clan->top_rank))
        send_to_char("You don't have the rank for that.\r\n", ch);
    else if (PLR_FLAGGED(ch, PLR_FROZEN))
        send_to_char("You are frozen solid, and can't lift a finger!\r\n", ch);
    else if (PLR_FLAGGED(vict, PLR_CLAN_LEADER) && 
    (GET_IDNUM(ch) != clan->owner && GET_LEVEL(ch) < LVL_IMMORT ))
        send_to_char("You cannot dismiss co-leaders.\r\n", ch);
    else {
        sprintf(buf, "You have been dismissed from clan %s by %s!\r\n", 
            clan->name, GET_NAME(ch));
        send_to_char(buf, vict);
        GET_CLAN(vict) = 0;
        sprintf(buf, "%s has been dismissed from clan %s by %s!", GET_NAME(vict), 
            clan->name, GET_NAME(ch));
        mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), 1);
        strcat(buf, "\r\n");
        send_to_clan(buf, GET_CLAN(ch));
        if ((member = real_clanmember(GET_IDNUM(vict), clan))) {
            REMOVE_MEMBER_FROM_CLAN(member, clan);
            free(member);
        }
        REMOVE_BIT(PLR_FLAGS(vict), PLR_CLAN_LEADER);
    }
    if (in_file) {
        char_to_store(vict, &tmp_store);
        fseek(player_fl, (player_i) * sizeof(struct char_file_u), SEEK_SET);
        fwrite(&tmp_store, sizeof(struct char_file_u), 1, player_fl);
        free_char(vict);
        send_to_char("Player dismissed.\r\n", ch);
    }

}

ACMD(do_resign)
{
  
    struct clan_data *clan = real_clan(GET_CLAN(ch));
    struct clanmember_data *member = NULL;
  
    if (IS_NPC(ch))
	send_to_char("NPC's cannot resign...\r\n", ch);
    else if (!clan)
	send_to_char("You need to be in a clan before you resign from it.\r\n",ch);
    else {
	sprintf(buf, "You have resigned from clan %s.\r\n", clan->name);
	send_to_char(buf, ch);
	sprintf(buf, "%s has resigned from clan %s.", GET_NAME(ch), 
		clan->name);
	mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), 1);
	strcat(buf, "\r\n");
	send_to_clan(buf, GET_CLAN(ch));
	GET_CLAN(ch) = 0;
	REMOVE_BIT(PLR_FLAGS(ch), PLR_CLAN_LEADER);
    if(clan->owner == GET_IDNUM(ch))
        clan->owner = 0;
	if ((member = real_clanmember(GET_IDNUM(ch), clan))) {
	    REMOVE_MEMBER_FROM_CLAN(member, clan);
	    free(member);
	}
    }
}

ACMD(do_clanlist)
{
    struct char_data *i;
    struct clan_data *clan = real_clan(GET_CLAN(ch));
    struct clanmember_data *member = NULL, *ch_member = NULL;
    struct descriptor_data *d;
    struct char_file_u tmp_store;
    int min_lev = 0;
    bool complete = 0;
    int visible = 1;
    int found = 0;
    char namebuf[128], outbuf[MAX_STRING_LENGTH];

    if (!clan) {
	send_to_char("You are not a member of any clan.\r\n", ch);
	return;
    }

    ch_member = real_clanmember(GET_IDNUM(ch), clan);

    skip_spaces(&argument);
    argument = one_argument(argument, arg);
    while (*arg) {
    
	if (is_number(arg)) {
	    min_lev = atoi(arg);
	    if (min_lev < 0 || min_lev > LVL_GRIMP) {
		send_to_char("Try a number between 0 and 60, please.\r\n", ch);
		return;
	    }
	} else if (is_abbrev(arg, "all") || is_abbrev(arg, "complete")) {
	    complete = true;
	} else {
	    send_to_char("Clanlist usage: clanlist [minlevel] ['all']\r\n", ch);
	    return;
	}
	argument = one_argument(argument, arg);
    }
  
    sprintf(outbuf, "Members of clan %s :\r\n", clan->name);
    for (member = clan->member_list; member; member = member->next, found = 0) {
	for (d = descriptor_list; d && !found; d = d->next) {
	    if (!d->connected) {
		i = ((d->original && GET_LEVEL(ch) > GET_LEVEL(d->original)) ? 
		     d->original : d->character);
		if (i && GET_CLAN(i) == GET_CLAN(ch) && 
		    GET_IDNUM(i) == member->idnum && (visible = CAN_SEE(ch, i)) && 
		    GET_LEVEL(i) >= min_lev && (i->in_room != NULL)) {
		    sprintf(namebuf, "%s %s (online)", GET_NAME(i), 
			    clan->ranknames[(int)member->rank] ?
			    clan->ranknames[(int)member->rank] : "the member");
		    if (d->original)
			sprintf(buf, "%s[%s%2d %s%s]%s %s%-40s%s - %s%s%s %s(in %s)%s\r\n",
				CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_LEVEL(i),
				char_class_abbrevs[(int)GET_CLASS(i)], CCGRN(ch, C_NRM),
				CCNRM(ch, C_NRM),
				(GET_LEVEL(i) >= LVL_AMBASSADOR ? CCGRN(ch, C_NRM) : 
				 (PLR_FLAGGED(i, PLR_CLAN_LEADER)?CCCYN(ch, C_NRM) : "")),
				namebuf, ((GET_LEVEL(i) >= LVL_AMBASSADOR || 
					   PLR_FLAGGED(i, PLR_CLAN_LEADER)) ? 
					  CCNRM(ch, C_NRM) : ""),
				CCCYN(ch, C_NRM), 
				(d->character->in_room->zone == ch->in_room->zone) ?
				((LIGHT_OK(ch) && LIGHT_OK(d->character) &&
				  (IS_LIGHT(d->character->in_room) ||
				   CAN_SEE_IN_DARK(ch))) ?
				 d->character->in_room->name :
				 "You cannot tell...") :
				d->character->in_room->zone->name, CCNRM(ch, C_NRM), 
				CCRED(ch, C_CMP),GET_NAME(d->character),CCNRM(ch, C_CMP));
		    else if (GET_LEVEL(i) >= LVL_AMBASSADOR)
			sprintf(buf, "%s[%s%s%s]%s %-40s%s - %s%s%s\r\n",
				CCYEL_BLD(ch, C_NRM), CCNRM_GRN(ch, C_SPR), 
				level_abbrevs[(int)(GET_LEVEL(i)-LVL_AMBASSADOR)],
				CCYEL_BLD(ch, C_NRM),  CCNRM_GRN(ch, C_SPR),
				namebuf, CCNRM(ch, C_SPR), CCCYN(ch, C_NRM), 
				(i->in_room->zone == ch->in_room->zone) ?
				((LIGHT_OK(ch)&&LIGHT_OK(i) &&
				  (IS_LIGHT(i->in_room) ||
				   CAN_SEE_IN_DARK(ch))) ?
				 i->in_room->name :
				 "You cannot tell...") :
				i->in_room->zone->name, CCNRM(ch, C_NRM));
		    else
			sprintf(buf, "%s[%s%2d %s%s]%s %s%-40s%s - %s%s%s\r\n",
				CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_LEVEL(i),
				char_class_abbrevs[(int)GET_CLASS(i)], CCGRN(ch, C_NRM),
				CCNRM(ch, C_NRM),
				(PLR_FLAGGED(i, PLR_CLAN_LEADER)?CCCYN(ch, C_NRM) : ""),
				namebuf, (PLR_FLAGGED(i, PLR_CLAN_LEADER) ?
					  CCNRM(ch, C_NRM) : ""), CCCYN(ch, C_NRM), 
				(i->in_room->zone == ch->in_room->zone) ?
				((LIGHT_OK(ch) && LIGHT_OK(i) &&
				  (IS_LIGHT(i->in_room) ||
				   CAN_SEE_IN_DARK(ch))) ?
				 i->in_room->name :
				 "You cannot tell...") :
				i->in_room->zone->name, CCNRM(ch, C_NRM));

		    ++found;
		    if (strlen(buf) + strlen(outbuf) > MAX_STRING_LENGTH - 128) {
			strcat(outbuf, "**OVERFLOW**\r\n");
			break;
		    } else
			strcat(outbuf, buf);
		}
	    }
	}
	if (complete && !found) {
	    if (!ch_member || member->rank > ch_member->rank || 
		!get_name_by_id(member->idnum)) {
		continue;
	    }
	    if (load_char(get_name_by_id(member->idnum), &tmp_store) >= 0) {
		CREATE(i, struct char_data, 1);
		clear_char(i);
		store_to_char(&tmp_store, i);
		sprintf(namebuf, "%s %s", GET_NAME(i), 
			clan->ranknames[(int)member->rank] ?
			clan->ranknames[(int)member->rank] : "the member");
	
		if (GET_LEVEL(i) >= LVL_AMBASSADOR)
		    sprintf(buf, "%s[%s%s%s]%s %-40s%s\r\n",
			    CCYEL_BLD(ch, C_NRM), CCNRM_GRN(ch, C_SPR), 
			    level_abbrevs[(int)(GET_LEVEL(i)-LVL_AMBASSADOR)],
			    CCYEL_BLD(ch, C_NRM),  CCNRM_GRN(ch, C_SPR),
			    namebuf, CCNRM(ch, C_SPR));
		else
		    sprintf(buf, "%s[%s%2d %s%s]%s %s%-40s%s\r\n",
			    CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_LEVEL(i),
			    char_class_abbrevs[(int)GET_CLASS(i)], CCGRN(ch, C_NRM),
			    CCNRM(ch, C_NRM),
			    (PLR_FLAGGED(i, PLR_CLAN_LEADER)?CCCYN(ch, C_NRM) : ""),
			    namebuf, CCNRM(ch, C_NRM));
	
		if (strlen(buf) + strlen(outbuf) > MAX_STRING_LENGTH - 128) {
		    strcat(outbuf, "**OVERFLOW**\r\n");
		    break;
		} else
		    strcat(outbuf, buf);
		free_char(i);
	    }
	}
    }
    page_string(ch->desc, outbuf, 1);
}

ACMD(do_cinfo){

    struct clan_data *clan = real_clan(GET_CLAN(ch));
    struct room_list_elem *rm_list = NULL;
    char outbuf[MAX_STRING_LENGTH];
    int found = 0;
    bool overflow = false;
    int i;
  
    if (!clan)
	send_to_char("You are not a member of any clan.\r\n", ch);
    else {
	sprintf(outbuf, 
		"Information on clan %s%s%s:\r\n\r\n"
		"Clan badge: '%s%s%s', Clan bank account: %d\r\n"
		"Clan ranks:\r\n",
		CCCYN(ch, C_NRM), clan->name, CCNRM(ch, C_NRM),
		CCCYN(ch, C_NRM), clan->badge, CCNRM(ch, C_NRM),
		clan->bank_account);
	for (i = clan->top_rank; i >= 0; i--) {
	    sprintf(buf, " (%2d)  %s%s%s\r\n", i, CCYEL(ch, C_NRM),
		    clan->ranknames[i] ? clan->ranknames[i] : "the Member",
		    CCNRM(ch, C_NRM));
	    if (strlen(buf) + strlen(outbuf) > MAX_STRING_LENGTH - 128) {
		overflow = true;
		strcat(outbuf, "**OVERFLOW**\r\n");
		break;
	    } else
		strcat(outbuf, buf);
	}
      
	if (!overflow) {
	    strcat(outbuf, "Clan rooms:\r\n");
	    for (rm_list = clan->room_list; rm_list; rm_list = rm_list->next) {
		if (rm_list->room && ROOM_FLAGGED(rm_list->room, ROOM_CLAN_HOUSE)) {
		    sprintf(buf, "     %s%s%s\r\n",CCCYN(ch, C_NRM),rm_list->room->name,
			    CCNRM(ch, C_NRM));
		    ++found;
		    if (strlen(buf) + strlen(outbuf) > MAX_STRING_LENGTH - 128) {
			strcat(outbuf, "**OVERFLOW**\r\n");
			break;
		    } else
			strcat(outbuf, buf);
		}
	    }
	    if (!found)
		strcat(outbuf, "None.\r\n");
	}
	page_string(ch->desc, outbuf, 1);
    }
}

ACMD(do_demote)
{
    struct clan_data *clan = real_clan(GET_CLAN(ch));
    struct clanmember_data *member1, *member2;
    struct char_data *vict = NULL;

    skip_spaces(&argument);

    if (!clan)
	send_to_char("You are not even in a clan.\r\n", ch);
    else if (!*argument)
	send_to_char("Demote who?\r\n", ch);
    else if (!(vict = get_char_room_vis(ch, argument)))
	send_to_char("No-one around by that name.\r\n", ch);
    else if (!(member1 = real_clanmember(GET_IDNUM(ch), clan)))
	send_to_char("You are not properly installed in the clan.\r\n", ch);
    else if (ch == vict) {
	if (!member1->rank)
	    send_to_char("You already at the bottom of the totem pole.\r\n", ch);
	else {
	    member1->rank--;
	    sprintf(buf, "%s has demoted self to clan rank %s (%d)",
		    GET_NAME(ch), clan->ranknames[(int)member1->rank], member1->rank);
	    slog(buf);
	    strcat(buf, "\r\n");
	    send_to_clan(buf, clan->number);
	    sort_clanmembers(clan);
	    LOG_CLANSAVE(ch, "do_demote (self)");
	    save_clans();
	}
    } else if (real_clan(GET_CLAN(vict)) != clan)
	act("$N is not a member of your clan.", FALSE, ch, 0, vict, TO_CHAR);
    else if (!(member2 = real_clanmember(GET_IDNUM(vict), clan)))
	act("$N is not properly installed in the clan.\r\n",
	    FALSE, ch, 0, vict, TO_CHAR);
    else if (!PLR_FLAGGED(ch, PLR_CLAN_LEADER) && GET_LEVEL(ch) < LVL_GRGOD)
	send_to_char("You are unable to demote.\r\n", ch);
    else if (member2->rank >= member1->rank || PLR_FLAGGED(vict, PLR_CLAN_LEADER))
	act("You are not in a position to demote $M.",
	    FALSE, ch, 0, vict, TO_CHAR);
    else if (member2->rank <= 0) {
	send_to_char("They are already as low as they can go.\r\n", ch);
	if (member2->rank < 0) {
	    slog("SYSERR: clan member with rank < 0");
	    member2->rank = 0;
	    LOG_CLANSAVE(NULL, "do_demote (error)");
	    save_clans();
	}
    }
    else {
	member2->rank--;
	sprintf(buf, "%s has demoted %s to clan rank %s (%d)",
		GET_NAME(ch), GET_NAME(vict),
		clan->ranknames[(int)member2->rank], member2->rank);
	slog(buf);
	strcat(buf, "\r\n");
	send_to_clan(buf, clan->number);
	sort_clanmembers(clan);
	LOG_CLANSAVE(ch, "do_demote (normal)");
	save_clans();
    }
}

ACMD(do_promote)
{
    struct clan_data *clan = real_clan(GET_CLAN(ch));
    struct clanmember_data *member1, *member2;
    struct char_data *vict = NULL;

    skip_spaces(&argument);

    if (!clan)
	send_to_char("You are not even in a clan.\r\n", ch);
    else if (!*argument)
	send_to_char("Promote who?\r\n", ch);
    else if (!(vict = get_char_room_vis(ch, argument)))
	send_to_char("No-one around by that name.\r\n", ch);
    else if (ch == vict && clan->owner != GET_IDNUM(ch))
	send_to_char("Very funny.  Really.\r\n", ch);
    else if (real_clan(GET_CLAN(vict)) != clan)
	act("$N is not a member of your clan.",FALSE,ch,0,vict,TO_CHAR);
    else if (!(member1 = real_clanmember(GET_IDNUM(ch), clan)))
	send_to_char("You are not properly installed in the clan.\r\n", ch);
    else if (!(member2 = real_clanmember(GET_IDNUM(vict), clan)))
	act("$N is not properly installed in the clan.\r\n",
	    FALSE, ch, 0, vict, TO_CHAR);
    else if (!PLR_FLAGGED(ch, PLR_CLAN_LEADER) && GET_LEVEL(ch) < LVL_GRGOD) {
        if (member2->rank >= member1->rank - 1 && GET_IDNUM(ch) != clan->owner) {
            act("You are not in a position to promote $M.",
            FALSE, ch, 0, vict, TO_CHAR);
        } else {
            if (member2->rank >= clan->top_rank ) {
                act("$E is already fully advanced.",FALSE,ch,0,vict,TO_CHAR);
                return;
            }
            member2->rank++;
            sprintf(buf, "%s has promoted %s to clan rank %s (%d)",
                GET_NAME(ch), GET_NAME(vict), 
                clan->ranknames[(int)member2->rank] ?
                clan->ranknames[(int)member2->rank] : "member", member2->rank);
            slog(buf);
            strcat(buf, "\r\n");
            send_to_clan(buf, clan->number);
            sort_clanmembers(clan);      
            LOG_CLANSAVE(ch, "do_promote (nonleader)");
            save_clans();
        }
    } else if (PLR_FLAGGED(ch, PLR_CLAN_LEADER) || 
	       member1->rank >= clan->top_rank) {
	if (member2->rank >= clan->top_rank && PLR_FLAGGED(ch, PLR_CLAN_LEADER)) {
	    if (PLR_FLAGGED(vict, PLR_CLAN_LEADER))
		act("$E is already fully advanced.",FALSE,ch,0,vict,TO_CHAR);
	    else {
		SET_BIT(PLR_FLAGS(vict), PLR_CLAN_LEADER);
		sprintf(buf, "%s has promoted %s to clan leader status.",
			GET_NAME(ch), GET_NAME(vict));
		slog(buf);
		strcat(buf, "\r\n");
		send_to_clan(buf, clan->number);
		save_char(vict, NULL);
	    }
	} else {
	    if (member2->rank >= member1->rank && GET_IDNUM(ch) != clan->owner)
		act("You cannot promote $M further.",FALSE,ch,0,vict,TO_CHAR);
	    else {
		member2->rank++;
		sprintf(buf, "%s has promoted %s to clan rank %s (%d)",
			GET_NAME(ch), GET_NAME(vict), 
			clan->ranknames[(int)member2->rank] ?
			clan->ranknames[(int)member2->rank] : "member", member2->rank);
		slog(buf);
		strcat(buf, "\r\n");
		send_to_clan(buf, clan->number);
		sort_clanmembers(clan);
		LOG_CLANSAVE(ch, "do_promote (leader)");
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
	send_to_char("Only clan leaders can do this.\r\n", ch);
	return;
    }
    if (!(clan = real_clan(GET_CLAN(ch)))) {
	send_to_char("Something about your clan is screwed up.\r\n", ch);
	return;
    }
    if (!ROOM_FLAGGED(ch->in_room, ROOM_CLAN_HOUSE)) {
	send_to_char("You can't set any clan passwds in this room.\r\n", ch);
	return;
    }
    if (!clan_house_can_enter(ch, ch->in_room)) {
	send_to_char("You can't set passwds in other people's clan house.\r\n",ch);
	return;
    }

    skip_spaces(&argument);
    if (!*argument) {
	send_to_char("Set the clanpasswd to what?\r\n", ch);
	return;
    }

    for (srch = ch->in_room->search; srch; srch = srch->next) {
	if (srch->command == SEARCH_COM_TRANSPORT &&
	    SRCH_FLAGGED(srch, SRCH_CLANPASSWD))
	    break;
    }

    if (!srch) {
	send_to_char("There is no clanpasswd search in this room.\r\n", ch);
	return;
    }

    free(srch->keywords);
    srch->keywords = str_dup(argument);
    save_wld(ch);
  
    sprintf(buf, "New clanpasswd set here to '%s'.\r\n", argument);
    send_to_char(buf, ch);

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

struct clan_data *
clan_by_name(char *arg)
{
    struct clan_data *clan = NULL;
    int    clan_num = -1;

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
    int   level;
} cedit_command_data;

cedit_command_data cedit_keys[] = {
    {"save",    LVL_GOD},
    {"create",  LVL_GRGOD},
    {"delete",  LVL_GRGOD},
    {"set",     LVL_GOD},
    {"show",    LVL_GOD},
    {"add",     LVL_GOD},
    {"remove",  LVL_GRGOD},
    {"sort",    LVL_GRGOD},
    {NULL,      0}
};

ACMD(do_cedit)
{

    struct clan_data *clan = NULL;
    struct clanmember_data *member = NULL;
    struct room_list_elem *rm_list = NULL;
    struct room_data *room = NULL;
    int cedit_command, i, j;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH],arg3[MAX_INPUT_LENGTH];

    skip_spaces(&argument);
  
    argument = one_argument(argument, arg1);
    for (cedit_command = 0; cedit_keys[cedit_command].keyword; cedit_command++)
	if (!strcasecmp(arg1, cedit_keys[cedit_command].keyword))
	    break;
  
    if (!cedit_keys[cedit_command].keyword) {
	send_to_char("Valid cedit options:  save, create, delete(*), set, show, add, remove.\r\n", ch);
	return;
    }

    if (GET_LEVEL(ch) < cedit_keys[cedit_command].level) {
	send_to_char("Sorry, you can't use that cedit command.\r\n", ch);
	return;
    }

    argument = one_argument(argument, arg1);
    if (*arg1) {
	if (is_number(arg1))
	    clan = real_clan(atoi(arg1));
	else
	    clan = clan_by_name(arg1);

	// protect quaker and null
	if (clan && GET_LEVEL(ch) < LVL_CREATOR && (clan->number == 0 || clan->number == 6)) {
	    send_to_char("Sorry, you cannot edit this clan.\r\n", ch);
	    return;
	}

    }

    switch (cedit_command) {

    case 0:              /*** save ***/
	if (!save_clans()) {
	    send_to_char("Clans saved successfully.\r\n", ch);
	    LOG_CLANSAVE(ch, "cedit save");
	}
	else
	    send_to_char("ERROR in clan save.\r\n", ch);
	break;
    
    case 1:             /*** create ***/ 
    {
        int clan_number = 0;
        if (!*arg1) {
            send_to_char("Create clan with what vnum?\r\n", ch);
            break;
        }
        if (!is_number(arg1)) {
            send_to_char("You must specify the clan numerically.", ch);
            break;
        }
        clan_number = atoi(arg1);
        if(clan_number <= 0) {
            send_to_char("You must specify the clan numerically.", ch);
            break;
        } else if (real_clan(atoi(arg1))) {
            send_to_char("A clan already exists with that vnum.\r\n", ch);
            break;
        } else if ((clan = create_clan(atoi(arg1)))) {
            send_to_char("Clan created.\r\n", ch);
            sprintf(buf, "(cedit) %s created clan %d.", GET_NAME(ch), clan->number);
            slog(buf);
        } else {
            send_to_char("There was an error creating the clan.\r\n", ch);
        }
        break;
    }
    case 2:               /*** delete ***/
	if (!clan) {
	    if (!*arg1)
		send_to_char("Delete what clan?\r\n", ch);
	    else {
		sprintf(buf, "Clan '%s' does not exist.\r\n", arg1);
		send_to_char(buf, ch);
	    }
	    return;
	} else {
	    i = clan->number;
	    if (!delete_clan(clan)) {
		send_to_char("Clan deleted.  Sucked anyway.\r\n", ch);
		sprintf(buf, "(cedit) %s deleted clan %d.", GET_NAME(ch), i);
		slog(buf);
	    }
	    else
		send_to_char("ERROR occured while deleting clan.\r\n", ch);
	}
	break;

    case 3:               /*** set    ***/
	if (!*arg1) {
	    send_to_char("Usage: cedit set <vnum> <name|badge|rank|bank|member|owner>['top']<value>\r\n", ch);
	    return;
	}

	half_chop(argument, arg2, argument);
	skip_spaces(&argument);

	if (!*argument || !*arg2) {
	    send_to_char("Usage: cedit set <clan> <name|badge|rank|bank|member|owner>['top']<value>\r\n", ch);
	    return;
	}
	if (!clan) {
	    sprintf(buf, "Clan '%s' does not exist.\r\n", arg1);
	    send_to_char(buf, ch);
	    return;
	}

	// cedit set name
	if (is_abbrev(arg2, "name")) {
	    if (strlen(argument) > MAX_CLAN_NAME) {
		sprintf(buf, "Name too long.  Maximum %d characters.\r\n", 
			MAX_CLAN_NAME);
		send_to_char(buf, ch);
		return;
	    }
	    if (clan->name) {
		free(clan->name);
	    }
	    clan->name = str_dup(argument);
	    sprintf(buf, "(cedit) %s set clan %d name to '%s'.", GET_NAME(ch), clan->number, clan->name);
	    slog(buf);

	} 
	// cedit set badge
	else if (is_abbrev(arg2, "badge")) {
	    if (strlen(argument) > MAX_CLAN_BADGE-1) {
		sprintf(buf, "Badge too long.  Maximum %d characters.\r\n", 
			MAX_CLAN_BADGE-1);
		send_to_char(buf, ch);
		return;
	    }
	    if (clan->badge) {
		free(clan->badge);
	    }
	    clan->badge = str_dup(argument);
	    sprintf(buf, "(cedit) %s set clan %d name to '%s'.", GET_NAME(ch), clan->number, clan->name);
	    slog(buf);

	} 
	// cedit set rank
	else if (is_abbrev(arg2, "rank")) {

	    argument = one_argument(argument, arg3);
	    skip_spaces(&argument);

	    if (is_abbrev(arg3, "top")) {
		if (!*argument) {
		    send_to_char("Set top rank of clan to what?\r\n", ch);
		    return;
		}
		if (!is_number(argument) || (i = atoi(argument)) < 0 || 
		    i >= NUM_CLAN_RANKS) {
		    sprintf(buf, "top rank must be a number between 0 and %d.\r\n",
			    NUM_CLAN_RANKS-1);
		    send_to_char(buf, ch);
		    return;
		}
		clan->top_rank = i;
		send_to_char("Top rank of clan set.\r\n", ch);

		sprintf(buf, "(cedit) %s set clan %d top to %d.", GET_NAME(ch), clan->number, clan->top_rank);
		slog(buf);

		return;
	    }

	    if (!is_number(arg3) || (i = atoi(arg3)) < 0 || 
		i > clan->top_rank) {
		sprintf(buf, "[rank] must be a number between 0 and %d.\r\n",
			clan->top_rank);
		send_to_char(buf, ch);
		return;
	    }
	    if (!*argument) {
		send_to_char("Set that rank to what name?\r\n", ch);
		return;
	    }
	    if (strlen(argument) >= MAX_CLAN_RANKNAME) {
		sprintf(buf, "Rank names may not exceed %d characters.\r\n",
			MAX_CLAN_RANKNAME-1);
		send_to_char(buf, ch);
		return;
	    }
	    if (clan->ranknames[i]) {
		free(clan->ranknames[i]);
	    }
	    clan->ranknames[i] = str_dup(argument);
      
	    send_to_char("Rank title set.\r\n", ch);
	    sprintf(buf, "(cedit) %s set clan %d rank %d to '%s'.", GET_NAME(ch), clan->number, i, clan->ranknames[i]);
	    slog(buf);
      
	    return;
      
	} 

	// cedit set bank
	else if (is_abbrev(arg2, "bank")) {
	    if (!*argument) {
            send_to_char("Set the bank account of the clan to what?\r\n", ch);
            return;
	    }
	    if (!is_number(argument) || (i = atoi(argument)) < 0) {
            send_to_char("Try setting the bank account to an appropriate number asswipe.\r\n", ch);
            return;
	    }
	    clan->bank_account = i;
	    send_to_char("Clan bank account set.\r\n", ch);
	    sprintf(buf, "(cedit) %s set clan %d bank to %d.", GET_NAME(ch), clan->number, clan->bank_account);
	    slog(buf);
      
	    return;

	} 

	// cedit set owner
	else if (is_abbrev(arg2, "owner")) {
	    if (!*argument) {
            send_to_char("Set the owner of the clan to what?\r\n", ch);
            return;
	    }
	    if ((i = get_id_by_name(argument)) < 0) {
            send_to_char("No such person.\r\n", ch);
            return;
	    }
	    clan->owner = i;
	    send_to_char("Clan owner set.\r\n", ch);
	    sprintf(buf, "(cedit) %s set clan %d owner to %s.", GET_NAME(ch), clan->number, argument);
	    slog(buf);
	    return;
	}

	// cedit set member
	else if (is_abbrev(arg2, "member")) {
	    if (!*argument) {
		send_to_char("Usage: cedit set <clan> member <member> <rank>\r\n",ch);
		return;
	    }
	    argument = two_arguments(argument, arg3, arg1);

	    if (!is_number(arg3)) {
		if ((i = get_id_by_name(arg3)) < 0) {
		    send_to_char("There is no such player in existance...\r\n", ch);
		    return;
		}
	    } else if ((i = atoi(arg3)) < 0) {
		send_to_char("Such a comedian.\r\n", ch);
		return;
	    }
      
	    if (!*arg1) {
		send_to_char("Usage: cedit set <clan> member <member> <rank>\r\n",ch);
		return;
	    }
      
	    j = atoi(arg1);

	    for (member = clan->member_list; member; member = member->next)
		if (member->idnum == i) {
		    member->rank = j;
		    send_to_char("Member rank set.\r\n", ch);
		    sprintf(buf, "(cedit) %s set clan %d member %d rank to %d.", GET_NAME(ch), clan->number, i, member->rank);
		    slog(buf);
		    break;
		}
      
	    if (!member)
		send_to_char("Unable to find that member.\r\n", ch);
	    return;

	} else {
	    sprintf(buf, "Unknown option '%s'.\r\n", arg2);
	    send_to_char(buf, ch);
	    return;
	}

	send_to_char("Successful.\r\n", ch);
	break;

    case 4:               /*** show   ***/

	if (!clan && *arg1) {
	    sprintf(buf, "Clan '%s' does not exist.\r\n", arg1);
	    send_to_char(buf, ch);
	    return;
	}
	do_show_clan(ch, clan);
	break;
      
    case 5:               /*** add    ***/
	if (!*arg1) {
	    send_to_char("Usage: cedit add <clan> <room|member> <number>\r\n",ch);
	    return;
	}
	argument = two_arguments(argument, arg2, arg3);

	if (!*arg3 || !*arg2) {
	    send_to_char("Usage: cedit add <clan> <room|member> <number>\r\n",ch);
	    return;
	}
	if (!clan) {
	    sprintf(buf, "Clan '%s' does not exist.\r\n", arg1);
	    send_to_char(buf, ch);
	    return;
	}

	// cedit add room
	if (is_abbrev(arg2, "room")) {
	    if (!(room = real_room(atoi(arg3)))) {
		sprintf(buf, "No room with number %s exists.\r\n", arg3);
		send_to_char(buf, ch);
		return;              
	    }
	    for (rm_list = clan->room_list; rm_list; rm_list = rm_list->next)
		if (rm_list->room == room) {
		    send_to_char("This room is already a part of the clan, dumbass.\r\n",ch);
		    return;
		}


		CREATE(rm_list, struct room_list_elem, 1);
		rm_list->room = room;
		rm_list->next = clan->room_list;
		clan->room_list = rm_list;
		send_to_char("Room added.\r\n", ch);
		
		sprintf(buf, "(cedit) %s added room %d to clan %d.", GET_NAME(ch), room->number, clan->number);
		slog(buf);

	    return;
	} 
    
	// cedit add member
	else if (is_abbrev(arg2, "member")) {
	    if (!is_number(arg3)) {
    		if ((i = get_id_by_name(arg3)) < 0) {
    		    send_to_char("There exists no player with that name.\r\n", ch);
    		    return;
    		}
	    } else if ((i = atoi(arg3)) < 0) {
	    	send_to_char("Real funny... reeeeeaaal funny.\r\n", ch);
	    	return;
	    }
	    for (member = clan->member_list; member; member = member->next) {
    		if (member->idnum == i) {
    		    send_to_char("That player is already on the member list.\r\n", ch);
    		    return;
    		}
        }
	    CREATE(member, struct clanmember_data, 1);
      
	    member->idnum = i;
	    member->rank  = 0;
	    member->next  = clan->member_list;
	    clan->member_list = member;
      
	    send_to_char("Clan member added to list.\r\n", ch);;

	    sprintf(buf, "(cedit) %s added member %d to clan %d.", GET_NAME(ch), (int)member->idnum, clan->number);
	    slog(buf);

	    return;
	} else { 
	    send_to_char("Invalid command, punk.\r\n", ch);
    }
	break;
    case 6:               /** remove ***/

	if (!*arg1) {
	    send_to_char("Usage: cedit remove <clan> <room|member> <number>\r\n",ch);
	    return;
	}
	argument = two_arguments(argument, arg2, arg3);

	if (!*arg3 || !*arg2) {
	    send_to_char("Usage: cedit remove <clan> <room|member> <number>\r\n",ch);
	    return;
	}
	if (!clan) {
	    sprintf(buf, "Clan '%s' does not exist.\r\n", arg1);
	    send_to_char(buf, ch);
	    return;
	}

	if (is_abbrev(arg2, "room")) {
	    if (!(room = real_room(atoi(arg3)))) {
		sprintf(buf, "No room with number %s exists.\r\n", arg3);
		send_to_char(buf, ch);
	    }
	    for (rm_list = clan->room_list; rm_list; rm_list = rm_list->next)
		if (rm_list->room == room)
		    break;

	    if (!rm_list) {
		send_to_char("That room is not part of the list, dorkus.\r\n",ch);
		return;
	    }

	    REMOVE_ROOM_FROM_CLAN(rm_list, clan);
	    free(rm_list);
	    send_to_char("Room removed and memory freed.  Thank you.. Call again.\r\n", ch);

	    sprintf(buf, "(cedit) %s removed room %d from clan %d.", GET_NAME(ch), room->number, clan->number);
	    slog(buf);

	    return;

	} 

	// cedit remove member
	else if (is_abbrev(arg2, "member")) {
	    if (!is_number(arg3)) {
		if ((i = get_id_by_name(arg3)) < 0) {
		    send_to_char("There exists no player with that name.\r\n", ch);
		    return;
		}
	    } else if ((i = atoi(arg3)) < 0) {
		send_to_char("Real funny... reeeeeaaal funny.\r\n", ch);
		return;
	    }
	    for (member = clan->member_list; member; member = member->next)
		if (member->idnum == i) 
		    break;

	    if (!member) {
		send_to_char("That player is not a part of this clan.\r\n", ch);
		return;
	    }

	    REMOVE_MEMBER_FROM_CLAN(member, clan);
	    free(member);
	    send_to_char("Member removed from the sacred list.\r\n", ch);

	    sprintf(buf, "(cedit) %s removed member %d from clan %d.", GET_NAME(ch), i, clan->number);
	    slog(buf);
      
	    return;
	} else
	    send_to_char("YO!  Remove members, or rooms... geez!\r\n", ch);

	break;

    case 7:  /** sort **/
	if (!clan) {
	    if (!*arg1)
		send_to_char("Sort what clan?\r\n", ch);
	    else {
		sprintf(buf, "Clan '%s' does not exist.\r\n", arg1);
		send_to_char(buf, ch);
	    }
	    return;
	}
	sort_clanmembers(clan);
	send_to_char("Clanmembers sorted.\r\n", ch);
	break;
    
    default:
	send_to_char("Unknown cedit option.\r\n", ch);
	break;
    }
}


#define CLAN_FILE "etc/clans"

void 
boot_clans()
{
  
    FILE *file = NULL;
    int virt = -1, last = -1,i = 0;
    long member_id;
    byte member_rank;
    struct clan_file_elem_hdr clan_hdr;
    struct clan_data *clan = NULL, *tmp_clan = NULL;
    struct clanmember_data *member = NULL, *tmp_member = NULL;
    struct room_data *room = NULL;
    struct room_list_elem *rm_list = NULL;
    struct char_file_u tmp_store;

    clan_list = NULL;
  
    if (!(file = fopen(CLAN_FILE, "r"))) {
	sprintf(buf, "Unable to open clan file '%s' for read.\n", CLAN_FILE);
	slog(buf);
	return;
    }

    while (fread(&clan_hdr, sizeof(struct clan_file_elem_hdr), 1, file)) {

	if (clan_hdr.number < last) {
	    sprintf(buf, "Format error in clan file.  Clan %d after clan %d.\n",
		    clan_hdr.number, last);
	    slog(buf);
	    return;
	}

	last = clan_hdr.number;

	CREATE(clan, struct clan_data, 1);
    
	clan->number       = clan_hdr.number;
	clan->bank_account = clan_hdr.bank_account;
	clan->top_rank     = clan_hdr.top_rank;
	clan->owner        = clan_hdr.owner;
	clan->flags        = clan_hdr.flags;
	clan->name         = str_dup(clan_hdr.name);
	clan->badge        = str_dup(clan_hdr.badge);
	clan->member_list  = NULL;
	clan->room_list    = NULL;
	clan->next         = NULL;
	for (i = 0; i < NUM_CLAN_RANKS; i++) {
	    if (clan_hdr.ranknames[i][0])
		clan->ranknames[i]  = str_dup(clan_hdr.ranknames[i]);
	    else {
		if (!(i))
		    clan->ranknames[i]  = str_dup("the recruit");
		else if (i == clan->top_rank)
		    clan->ranknames[i]  = str_dup("the leader");
		else
		    clan->ranknames[i]  = str_dup("the member");
	    }
	}
    
	for (i = 0; i < clan_hdr.num_members; i++) {
	    fread(&member_id, sizeof(long), 1, file);
	    fread(&member_rank, sizeof(byte), 1, file);

	    if (!get_name_by_id(member_id)) {
		sprintf(buf, "Clan(%2d) member (%4ld) nonex.", 
			clan->number, member_id);
		slog(buf);
		continue;
	    }
	    if (load_char(get_name_by_id(member_id), &tmp_store) > -1) {
		if (tmp_store.player_specials_saved.clan != clan->number) {
		    sprintf(buf, "Clan(%2d) member (%4ld) nolonger.", 
			    clan->number, member_id);
		    continue;
		}
	    } else
		slog("ERROR");
  
	    CREATE(member, struct clanmember_data, 1);
	    member->idnum   = member_id;
	    member->rank    = member_rank;
	    member->next    = NULL;
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
    slog("Clan boot successful.");
}

int 
save_clans(void)
{

    struct clan_file_elem_hdr hdr;
    struct room_list_elem *rm_list = NULL;
    struct clanmember_data *member = NULL;
    struct clan_data *clan         = NULL;
    int i, j;
    FILE *file = NULL;

    if (!(file = fopen(CLAN_FILE, "w"))) {
	    slog("Error opening clan file for write.");
	    return 1;
    }

    for (clan = clan_list; clan; clan = clan->next) {
	strncpy(hdr.name, clan->name, MAX_CLAN_NAME-1);
	hdr.name[MAX_CLAN_NAME-1] = '\0';
	strncpy(hdr.badge, clan->badge, MAX_CLAN_BADGE-1);
	hdr.badge[MAX_CLAN_BADGE-1] = '\0';

	for (i = 0; i < NUM_CLAN_RANKS; i++)
	    if (clan->ranknames[i]) {
		    strncpy(hdr.ranknames[i], clan->ranknames[i], MAX_CLAN_RANKNAME);
	    } else {
		    hdr.ranknames[i][0] = '\0';
        }
	for (j = 0, rm_list = clan->room_list; rm_list; 
	     j++, rm_list = rm_list->next);
	hdr.num_rooms = (ubyte) j;

	for (j = 0, member = clan->member_list; member; 
	     j++, member = member->next);
	hdr.num_members  = (ubyte) j;
	hdr.number       = clan->number;
	hdr.bank_account = clan->bank_account;
	hdr.top_rank     = clan->top_rank;
	hdr.owner        = clan->owner;
	hdr.flags        = clan->flags;

	for (i = 0; i < 20; i++)
	    hdr.cSpares[i] = 0;

	fwrite(&hdr, sizeof(struct clan_file_elem_hdr), 1, file);
    
	for (member = clan->member_list; member; member = member->next) {
	    fwrite(&member->idnum, sizeof(long), 1, file);
	    fwrite(&member->rank,  sizeof(byte), 1, file);
	}

	for (rm_list = clan->room_list; rm_list; rm_list = rm_list->next)
	    fwrite(&rm_list->room->number, sizeof(int), 1, file);
    }
  
    fclose(file);
    slog("Clan save successful.");
    return 0;
}

struct clan_data *
create_clan(int vnum)
{

    struct clan_data *newclan = NULL, *clan = NULL;
    int i;

    CREATE(newclan, struct clan_data, 1);
  
    newclan->number       = vnum;
    newclan->bank_account = 0;
    newclan->top_rank     = 0;
    newclan->name         = str_dup("New");
    newclan->badge        = str_dup("(//NEW\\\\)");
    for (i = 0; i < NUM_CLAN_RANKS; i++)
	newclan->ranknames[i] = NULL;

    newclan->member_list = NULL;
    newclan->room_list   = NULL;
    newclan->next        = NULL;

    if (!clan_list || newclan->number < clan_list->number) {
	newclan->next     = clan_list;
	clan_list         = newclan;
    } else {
	for (clan = clan_list; clan; clan = clan->next)
	    if (!clan->next ||
		(clan->number < vnum && clan->next->number > vnum)) {
		newclan->next = clan->next;
		clan->next    = newclan;
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
do_show_clan(struct char_data *ch, struct clan_data *clan)
{
    char                    outbuf[MAX_STRING_LENGTH];
    bool                    overflow = false;
    struct clanmember_data *member = NULL;
    struct room_list_elem  *rm_list = NULL;
    int                     i, num_rooms = 0, num_members = 0;

    if (clan) {
	sprintf(outbuf, "CLAN %d - Name: %s%s%s, Badge: %s%s%s, Top Rank: %d, Bank: %d\r\n",
		clan->number, CCCYN(ch, C_NRM), clan->name, CCNRM(ch, C_NRM),
		CCCYN(ch, C_NRM), clan->badge, CCNRM(ch, C_NRM), 
		clan->top_rank, clan->bank_account);

	if (GET_LEVEL(ch) > LVL_AMBASSADOR)
	    sprintf(outbuf, "%sOwner: %ld (%s), Flags: %d\r\n",
		    outbuf, clan->owner, get_name_by_id(clan->owner), clan->flags);
	      
	for (i = clan->top_rank; i >= 0; i--) {
	    sprintf(buf, "Rank %2d: %s%s%s\r\n", i, CCYEL(ch, C_NRM),
		    clan->ranknames[i] ? clan->ranknames[i] : !(i) ? "recruit" :
		    "member",	 CCNRM(ch, C_NRM));
	    if (strlen(buf) + strlen(outbuf) < MAX_STRING_LENGTH - 128)
		strcat(outbuf, buf);
	    else {
		overflow = true;
		break;
	    }
	}
	if (!overflow) {
	    strcat(outbuf, "ROOMS:\r\n");
	
	    for (rm_list = clan->room_list, num_rooms = 0; 
		 rm_list; rm_list = rm_list->next) {
		num_rooms++;
		sprintf(buf, "%3d) %5d.  %s%s%s\r\n", num_rooms,
			rm_list->room->number,
			CCCYN(ch, C_NRM), rm_list->room->name, CCNRM(ch, C_NRM));
		if (strlen(buf) + strlen(outbuf) < MAX_STRING_LENGTH - 128)
		    strcat(outbuf, buf);
		else {
		    overflow = true;
		    break;
		}
	    }
	    if (!num_rooms)
		strcat(outbuf, "None.\r\n");
	}
	if (!overflow) {
	    strcat(outbuf, "MEMBERS:\r\n");

	    for (member = clan->member_list, num_members = 0;
		 member; member = member->next) {
		num_members++;
		sprintf(buf, "%3d) %5d -  %s%20s%s  Rank: %d\r\n", num_members,
			(int)member->idnum,
			CCYEL(ch, C_NRM), get_name_by_id(member->idnum), 
			CCNRM(ch, C_NRM), member->rank);
		if (strlen(buf) + strlen(outbuf) < MAX_STRING_LENGTH - 128)
		    strcat(outbuf, buf);
		else {
		    overflow = true;
		    break;
		}
	    }
	    if (!num_members)
		strcat(outbuf, "None.\r\n");
	}

	if (overflow)
	    strcat(outbuf, "**OVERFLOW**\r\n");

    } else {

	strcpy(outbuf, "CLANS:\r\n");
	for (clan = clan_list; clan; clan = clan->next) {
	    for (member = clan->member_list, num_members = 0; member;
		 num_members++, member = member->next);

	    sprintf(buf, " %3d - %s%20s%s  %s%20s%s  (%3d members)\r\n",
		    clan->number, 
		    CCCYN(ch, C_NRM), clan->name, CCNRM(ch, C_NRM),
		    CCCYN(ch, C_NRM), clan->badge, CCNRM(ch, C_NRM), num_members);
	    if (strlen(buf) + strlen(outbuf) < MAX_STRING_LENGTH - 128)
		strcat(outbuf, buf);
	    else {
		strcat(outbuf, "**OVERFLOW**\r\n");
		break;
	    }
	}
    }
    page_string(ch->desc, outbuf, 1);
}

int 
clan_house_can_enter(struct char_data *ch, struct room_data *room)
{
    struct clan_data *clan = NULL, *ch_clan = NULL;
    struct room_list_elem *rm_list = NULL;

    if (!ROOM_FLAGGED(room, ROOM_CLAN_HOUSE))
	return 1;
    if (GET_LEVEL(ch) >= LVL_DEMI)
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

    for (i = clan->member_list; i; i=j) {
	j = i->next;    if (!new_list || (new_list->rank < i->rank))  {
	    i->next = new_list;
	    new_list = i;
	}
	else  {
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
