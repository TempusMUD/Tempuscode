/* ************************************************************************
*   File: gnetsys.cc                                    Part of CircleMUD *
*  Usage: handle functions dealing with the slobal network systems        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: gnetsys.cc                         -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 2001 by Daniel Lowe, all rights reserved.
//

#define __gnetsys_c__

#include <arpa/telnet.h>
#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "handler.h"
#include "mail.h"
#include "screen.h"
#include "help.h"
#include "char_class.h"
#include "clan.h"
#include "vehicle.h"
#include "house.h"
#include "login.h"
#include "matrix.h"
#include "bomb.h"

extern int skill_sort_info[MAX_SKILLS - MAX_SPELLS + 1];
extern struct descriptor_data *descriptor_list;
void set_desc_state(int state,struct descriptor_data *d );

void
perform_net_help(descriptor_data *d) {
	SEND_TO_Q("       System Help\r\n",d);
	SEND_TO_Q("------------------------------------------------------------------\r\n",d);
	SEND_TO_Q("            logout - disconnects from network\r\n", d);
	SEND_TO_Q("              exit - synonym for logout\r\n" , d);
	SEND_TO_Q("               who - lists users logged into network\r\n", d);
	SEND_TO_Q("     finger <user> - displays information about user\r\n", d);
	SEND_TO_Q("write <user> <msg> - sends a private message to a user\r\n", d);
	SEND_TO_Q("       wall <user> - sends a message to all users logged in\r\n", d);
	SEND_TO_Q("              help - displays this menu\r\n", d);
	SEND_TO_Q("                 ? - synonym for help\r\n", d);
	if ( IS_CYBORG(d->character) ) {
		SEND_TO_Q("              list - lists programs executing locally\r\n", d);
		SEND_TO_Q("            status - displays local system status\r\n", d);
		SEND_TO_Q("    load <program> - loads and executes programs into local memory\r\n", d);
	}
}

void
perform_net_write(descriptor_data *d,char *arg) {
	char targ[MAX_INPUT_LENGTH];
	char msg[MAX_INPUT_LENGTH];
	Creature *vict;

	half_chop(arg,targ,msg);

	if ( !*targ || !*msg) {
		SEND_TO_Q("Usage: write <user> <message>\r\n", d);
		return;
	}

	delete_doubledollar(msg);

	vict = get_player_vis(d->character, targ, 1);
	if ( !vict )
		vict = get_player_vis(d->character, targ, 0);
	
	if ( !vict || STATE(vict->desc) != CON_NETWORK) {
		SEND_TO_Q("Error: user not logged in\r\n", d);
		return;
	}
	
	sprintf(buf,"Message sent to %s: %s\r\n",GET_NAME(vict),msg);
	SEND_TO_Q(buf, d);
	sprintf(buf,"Message from %s: %s\r\n",GET_NAME(d->character),msg);
	SEND_TO_Q(buf, vict->desc);
}

void
perform_net_wall(descriptor_data *d,char *arg) {
	descriptor_data *r_d;

	if ( !*arg ) {
		SEND_TO_Q("Usage: wall <message>\r\n",d);
		return;
	}

	for (r_d = descriptor_list; r_d; r_d = r_d->next) {
		if (STATE(r_d) != CON_NETWORK)
			continue;
		sprintf(buf,"%s walls:%s\r\n",GET_NAME(d->character),
			arg);
		SEND_TO_Q(buf,r_d);
	}
}

void
perform_net_load(descriptor_data *d,char *arg) {
	int skill_num,percent;

	if ( !GET_PRACTICES(d->character)) {
		SEND_TO_Q("Error: No data storage units free\r\n",d);
		return;
	}

	skip_spaces(&arg);
	if ( !*arg ) {
		SEND_TO_Q("Usage: load <program>\r\n",d);
		return;
	}

	skill_num = find_skill_num(arg);
	if ( skill_num < 1 ) {
		SEND_TO_Q("Error: program '",d);
		SEND_TO_Q(arg,d);
		SEND_TO_Q("' not found\r\n",d );
		return;
	}

	if ( (SPELL_GEN(skill_num, CLASS_CYBORG ) > 0 && GET_CLASS(d->character) != CLASS_CYBORG) ||
		 (GET_REMORT_GEN(d->character) < SPELL_GEN(skill_num,CLASS_CYBORG)) ||
		 (GET_LEVEL(d->character) < SPELL_LEVEL(skill_num,CLASS_CYBORG))) {
		SEND_TO_Q("Error: resources unavailable to load '",d);
		SEND_TO_Q(arg,d);
		SEND_TO_Q("'\r\n",d);
		return;
	}

	if (GET_SKILL(d->character, skill_num) >= LEARNED( d->character)) {
		SEND_TO_Q("Program fully installed on local system.\r\n", d);
		return;
	}

	percent = MIN(MAXGAIN(d->character),
				  MAX(MINGAIN(d->character),
					  INT_APP(GET_INT(d->character))));
	percent = MIN(LEARNED(d->character) - 
				GET_SKILL(d->character,skill_num),percent);
	GET_PRACTICES(d->character)--;
	SET_SKILL(d->character, skill_num, GET_SKILL(d->character, skill_num) + percent);
	sprintf(buf, "Program download: %s terminating, %d percent transfer.\r\n", spell_to_str(skill_num), percent);
	SEND_TO_Q(buf, d);
	if (GET_SKILL(d->character, skill_num) >= LEARNED( d->character))
		strcpy(buf,"Program fully installed on local system.\r\n");
	else
		sprintf(buf, "Program %d%% installed on local system.\r\n",
			GET_SKILL(d->character,skill_num));
	SEND_TO_Q(buf, d);
}

void
perform_net_status(descriptor_data *d) {
	sprintf(buf, "You have %d data storage units free.\r\n", GET_PRACTICES(d->character));
	SEND_TO_Q(buf, d);
}

void 
perform_net_who(struct Creature *ch, char *arg)
{
    struct descriptor_data *d = NULL;
    int count = 0;

    strcpy(buf, "Visible users of the global net:\r\n");
    for (d = descriptor_list; d; d = d->next) {
        if (STATE(d) != CON_NETWORK)
            continue;
        if (!can_see_creature(ch, d->character))
            continue;
    
        count++;
        sprintf(buf, "%s   (%03d)     %s\r\n", buf, count, GET_NAME(d->character));
        continue;
    }
    sprintf(buf, "%s\r\n%d users detected.\r\n", buf, count);
    page_string(ch->desc, buf);
}

void perform_net_finger(struct Creature *ch, char *arg)
{
    struct Creature *vict = NULL;

	skip_spaces(&arg);
    if (!*arg) {
        send_to_char(ch, "Usage: finger <user>\r\n");
        return;
    }

    if (!(vict = get_char_vis(ch, arg)) || !vict->desc ||
        STATE(vict->desc) != CON_NETWORK || (GET_LEVEL(ch) < LVL_AMBASSADOR && GET_LEVEL(vict) >= LVL_AMBASSADOR)) {
        send_to_char(ch, "No such user detected.\r\n");
        return;
    }
    send_to_char(ch, "Finger results:\r\n"
            "Name:  %s, Level %d %s %s.\r\n"
            "Logged in at: %s.\r\n",
            GET_NAME(vict), GET_LEVEL(vict), 
            player_race[(int)GET_RACE(vict)], 
            pc_char_class_types[(int)GET_CLASS(vict)], 
            vict->in_room != NULL ?ch->in_room->name: "NOWHERE");
}

void 
perform_net_list(struct Creature * ch, int char_class) {
    int i, sortpos;

    strcpy(buf2, "Directory listing for local programs.\r\n\r\n");
  
    for (sortpos = 1; sortpos < MAX_SKILLS - MAX_SPELLS; sortpos++) {
        i = skill_sort_info[sortpos];
        if (strlen(buf2) >= MAX_STRING_LENGTH - 32) {
            strcat(buf2, "**OVERFLOW**\r\n");
            return;
        }
        if ((CHECK_SKILL(ch, i) || ABLE_TO_LEARN(ch, i)) &&
            SPELL_LEVEL(i, 0) <= LVL_GRIMP) {
            sprintf(buf, "%-30s [%3d] percent installed.\r\n", 
                    spell_to_str(i), GET_SKILL(ch, i));
            strcat(buf2, buf);
        } 
    }

    page_string(ch->desc, buf2);
}

void
handle_network(descriptor_data *d,char *arg) {
	char arg1[MAX_INPUT_LENGTH];

	if (!*arg)
		return;
	arg = one_argument(arg, arg1);
	if ( is_abbrev( arg1,"who" ) ) {
		perform_net_who(d->character,"");
	} else if ( *arg1 == '?' || is_abbrev( arg1,"help" ) ) {
		perform_net_help(d);
	} else if ( is_abbrev( arg1,"finger" ) ) {
		perform_net_finger(d->character, arg);
	} else if ( is_abbrev( arg1,"write" ) ) {
		perform_net_write(d, arg);
	} else if ( is_abbrev( arg1,"wall" ) ) {
		perform_net_wall(d, arg);
	} else if ( is_abbrev( arg1,"more" ) ) {
		if (d->showstr_head)
			show_string(d);
		else
			SEND_TO_Q("Error: Resource not available\r\n", d);
	} else if ( *arg1 == '@' || is_abbrev( arg1,"exit" ) || is_abbrev(arg1, "logout") ) {
		slog("User %s disconnecting from net.", GET_NAME(d->character));
		set_desc_state( CON_PLAYING,d );
		SEND_TO_Q("Connection closed.\r\n",d);
		act("$n disconnects from the network.", TRUE, d->character, 0, 0, TO_ROOM);
	} else if ( IS_CYBORG(d->character) && is_abbrev( arg1,"list" ) ) {
		perform_net_list(d->character, CLASS_CYBORG);
	} else if ( IS_CYBORG(d->character) && ( is_abbrev( arg1,"load" ) || is_abbrev(arg,"download"))) {
		perform_net_load(d, arg);
	} else if ( IS_CYBORG(d->character) && is_abbrev( arg1,"status" ) ) {
		perform_net_status(d);
	} else {
	   SEND_TO_Q(arg1, d);
	   SEND_TO_Q(": command not found\r\n", d);
	}
}

#undef __gnetsys_c__
