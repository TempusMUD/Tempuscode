/* ************************************************************************
*   File: nanny.cc                                      Part of CircleMUD *
*  Usage: parse user commands, search for specials, call ACMD functions   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: nanny.cc                           -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include <arpa/telnet.h>
#include "structs.h"
#include "ban.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "editor.h"
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
#include "bomb.h"
#include "security.h"
#include "quest.h"
#include "player_table.h"
#include "language.h"

extern char *motd;
extern char *ansi_motd;
extern char *imotd;
extern char *ansi_imotd;
extern char *background;
extern char *MENU;
extern char *WELC_MESSG;
extern char *START_MESSG;
extern struct descriptor_data *descriptor_list;
extern int top_of_p_table;
extern int restrict;
extern int num_of_houses;
extern int mini_mud;
extern int mud_moved;
extern int log_cmds;
extern struct spell_info_type spell_info[];
extern struct house_control_rec house_control[];
extern int shutdown_count;
extern const char *language_names[];


// external functions
ACMD(do_hcollect_help);
void do_start(struct Creature * ch, int mode);
void show_mud_date_to_char(struct Creature *ch);
void handle_network(struct descriptor_data *d,char *arg);
int general_search(struct Creature *ch, struct special_search_data *srch, int mode);
void roll_real_abils(struct Creature * ch);
void print_attributes_to_buf(struct Creature *ch, char *buff);
void show_character_detail(descriptor_data *d);
void push_command_onto_list(Creature *ch, char *comm);
void flush_q(struct txt_q *queue);
int _parse_name(char *arg, char *name);
int reserved_word(char *argument);
char *diag_conditions(struct Creature *ch);
int perform_alias(struct descriptor_data *d, char *orig);
int get_from_q(struct txt_q *queue, char *dest, int *aliased, int length = MAX_INPUT_LENGTH );
int parse_player_class(char *arg);
void save_all_players(void);

// internal functions
void set_desc_state(cxn_state state,struct descriptor_data *d );
void echo_on(struct descriptor_data * d);
void echo_off(struct descriptor_data * d);
void char_to_game(descriptor_data *d);

void notify_cleric_moon(struct Creature *ch);
void send_menu(descriptor_data *d);

int check_newbie_ban(struct descriptor_data *desc);
void
handle_input(struct descriptor_data *d)
{
    extern bool production_mode;
	char arg[MAX_INPUT_LENGTH * 10];
	int aliased;
	int char_id;
	int i;

	if (!get_from_q(&d->input, arg, &aliased))
		return;

	// we need a prompt here
	d->need_prompt = true;
	d->wait = 1;
	d->idle = 0;

	if (d->text_editor) {
		d->text_editor->Process(arg);
		return;
	}

	switch(d->input_mode) {
	case CXN_UNKNOWN:
	case CXN_DISCONNECT:
		break;
	case CXN_PLAYING:
		// Push input onto command list
		d->creature->char_specials.timer = 0;
		push_command_onto_list(d->creature, arg);
		// Drag them back from limbo
		if (GET_WAS_IN(d->creature)) {
			if (d->creature->in_room)
				char_from_room(d->creature, false);
			char_to_room(d->creature, GET_WAS_IN(d->creature), false);
			GET_WAS_IN(d->creature) = NULL;
			act("$n has returned.", true, d->creature, 0, 0, TO_ROOM);
		}
		// run it through aliasing system
		if (!aliased && perform_alias(d, arg))
			get_from_q(&d->input, arg, &aliased);
		// send it to interpreter
		command_interpreter(d->creature, arg);
		break;
	case CXN_ACCOUNT_LOGIN:
		if (!*arg) {
			set_desc_state(CXN_DISCONNECT, d);
			break;
		}
		if (strcasecmp(arg, "new")) {
			d->account = Account::retrieve(arg);
			if (d->account) {
				if (d->account->has_password())
					set_desc_state(CXN_ACCOUNT_PW, d);
				else
					set_desc_state(CXN_PW_PROMPT, d);
			} else
				send_to_desc(d, "That account does not exist.\r\n");
		} else
			set_desc_state(CXN_ACCOUNT_PROMPT, d);
		break;
	case CXN_ACCOUNT_PW:
		if (!d->account->authenticate(arg)) {
			slog("PASSWORD: account %s[%d] failed to authenticate. [%s]",
				d->account->get_name(),
				d->account->get_idnum(),
				d->host);
			send_to_desc(d, "Invalid password.\r\n");
			set_desc_state(CXN_ACCOUNT_LOGIN, d);
        } else if (d->account->is_banned()) {
            slog("Autobanning IP address of account %s[%d]",
                 d->account->get_name(),
                 d->account->get_idnum());
            perform_ban(BAN_ALL, d->host, "autoban",
                        tmp_sprintf("account %s", d->account->get_name()));
            d->account->login(d);
        } else {
			d->account->login(d);
		}
		break;
	case CXN_ACCOUNT_PROMPT:
		if (Valid_Name(arg)) {
			d->account = Account::retrieve(arg);

			if (!d->account) {
				set_desc_state(CXN_ACCOUNT_VERIFY, d);
				d->mode_data = strdup(arg);
			} else {
				d->account = NULL;
				send_to_desc(d, "That account name has already been taken.  Try another.\r\n");
			}
		}
		break;
	case CXN_ACCOUNT_VERIFY:
		switch (tolower(arg[0])) {
		case 'y':
    		d->account = Account::create(d->mode_data, d);
			set_desc_state(CXN_ANSI_PROMPT, d);
			break;
		case 'n':
			set_desc_state(CXN_ACCOUNT_PROMPT, d); break;
		default:
			send_to_desc(d, "Please enter Y or N\r\n"); break;
		}
		break;
	case CXN_ANSI_PROMPT:
		
		i = search_block(arg, ansi_levels, FALSE);
		if (i == -1) {
			send_to_desc(d, "\r\nPlease enter one of the selections.\r\n\r\n");
			return;
		}

		d->account->set_ansi_level(i);
		set_desc_state(CXN_COMPACT_PROMPT, d);
		break;
	case CXN_COMPACT_PROMPT:
		i = search_block(arg, compact_levels, FALSE);
		if (i == -1) {
			send_to_desc(d, "\r\nPlease enter one of the selections.\r\n\r\n");
			return;
		}

		d->account->set_compact_level(i);
		set_desc_state(CXN_EMAIL_PROMPT, d);
		break;
	case CXN_EMAIL_PROMPT:
		d->account->set_email_addr(arg);
		set_desc_state(CXN_PW_PROMPT, d);
		break;
	case CXN_PW_PROMPT:
		if (arg[0]) {
			d->account->set_password(arg);
			set_desc_state(CXN_PW_VERIFY, d);
		} else
			send_to_desc(d, "Sorry.  You MUST enter a password.\r\n");
		break;
	case CXN_PW_VERIFY:
		if (!d->account->authenticate(arg)) {
			send_to_desc(d, "Passwords did not match.  Please try again.\r\n");
			set_desc_state(CXN_PW_PROMPT, d);
		} else {
			set_desc_state(CXN_NAME_PROMPT, d);
		}
		break;
	case CXN_MENU:
		switch (tolower(arg[0])) {
		case 'l':
		case '0':
			send_to_desc(d, "Goodbye.  Return soon!\r\n");
			set_desc_state(CXN_DISCONNECT, d);
			break;
		case 'c':
            if (d->account->chars_available() <= 0) {
                send_to_desc(d, "\r\nNo more characters can be "
                                "created on this account.  Please keep "
                                "\r\nin mind that it is a violation of "
                                "policy to start multiple "
                                "accounts.\r\n\r\n");
                set_desc_state(CXN_WAIT_MENU, d);
            } else {
				set_desc_state(CXN_NAME_PROMPT, d);
			}
			break;
		case 'd':
			if (!d->account->invalid_char_index(2)) {
				set_desc_state(CXN_DELETE_PROMPT, d);
			} else if (!d->account->invalid_char_index(1)) {
				char_id = d->account->get_char_by_index(1);
				d->creature = new Creature(true);
				d->creature->desc = d;
				if (d->creature->loadFromXML(char_id)) {
					set_desc_state(CXN_DELETE_PW, d);
				} else {
                    errlog("loading character %d to delete.", char_id);
					send_to_desc(d, "\r\nThere was an error loading the character.\r\n\r\n");
					delete d->creature;
					d->creature = NULL;
				}
			} else
				send_to_desc(d, "\r\nThat isn't a valid command.\r\n\r\n");
			break;
		case 'e':
			if (!d->account->invalid_char_index(2)) {
				set_desc_state(CXN_EDIT_PROMPT, d);
			} else if (!d->account->invalid_char_index(1)) {
				char_id = d->account->get_char_by_index(1);
				d->creature = new Creature(true);
				d->creature->desc = d;
				if (d->creature->loadFromXML(char_id)) {
					set_desc_state(CXN_EDIT_DESC, d);
				} else {
                    errlog("loading character %d to edit it's description.", char_id);
					send_to_desc(d, "\r\nThere was an error loading the character.\r\n\r\n");
					delete d->creature;
					d->creature = NULL;
				}
			} else
				send_to_desc(d, "\r\nThat isn't a valid command.\r\n\r\n");
			break;
        case 's':
			if (!d->account->invalid_char_index(2)) {
				set_desc_state(CXN_DETAILS_PROMPT, d);
			} else if (!d->account->invalid_char_index(1)) {
				char_id = d->account->get_char_by_index(1);
				d->creature = new Creature(true);
				d->creature->desc = d;
				if (d->creature->loadFromXML(char_id)) {
					d->creature->desc = d;
					show_character_detail(d);
					delete d->creature;
					d->creature = NULL;
					set_desc_state(CXN_WAIT_MENU, d);
				} else {
                    errlog("loading character %d to show statistics.", char_id);
					send_to_desc(d, "\r\nThere was an error loading the character.\r\n\r\n");
					delete d->creature;
					d->creature = NULL;
				}
			} else
				send_to_desc(d, "\r\nThat isn't a valid command.\r\n\r\n");
			break;
		case 'p':
			set_desc_state(CXN_OLDPW_PROMPT, d); break;
		case 'v':
			set_desc_state(CXN_VIEW_BG, d); break;
		case '?':
		case '\0':
			send_menu(d);
			break;
		default:
			if (!is_number(arg)) {
				send_to_desc(d, "That isn't a valid command.\r\n\r\n");
				break;
			}

			if (d->account->invalid_char_index(atoi(arg))) {
				send_to_desc(d, "\r\nThat character selection doesn't exist.\r\n\r\n");
				return;
			}

			// Try to reconnect to an existing creature first
			char_id = d->account->get_char_by_index(atoi(arg));
			d->creature = get_char_in_world_by_idnum(char_id);

			if (d->creature) {
				REMOVE_BIT(PLR_FLAGS(d->creature), PLR_WRITING | PLR_OLC |
					PLR_MAILING | PLR_AFK);
				if (d->creature->desc) {
					descriptor_data *other_desc = d->creature->desc;

					send_to_desc(other_desc, "You have logged on from another location!\r\n");
                    // This descriptor should be closed immediately to prevent
                    // a race condition
					d->creature->desc = d;
                    other_desc->input_mode = CXN_DISCONNECT;
					other_desc->creature = NULL;
					send_to_desc(d, "\r\n\r\nYou take over your own body, already in use!\r\n");
					mlog(Security::ADMINBASIC, GET_INVIS_LVL(d->creature),
						NRM, true,
						"%s has reconnected", GET_NAME(d->creature));
				} else {
					d->creature->desc = d;
					mlog(Security::ADMINBASIC, GET_INVIS_LVL(d->creature),
						NRM, true,
						"%s has reconnected from linkless",
						GET_NAME(d->creature));
					send_to_desc(d, "\r\n\r\nYou take over your own body!\r\n");
				}

				set_desc_state(CXN_PLAYING, d);
				return;
			}

			d->creature = new Creature(true);
			d->creature->desc = d;
			d->creature->account = d->account;

			if (!d->creature->loadFromXML(char_id)) {
				mlog(Security::ADMINBASIC, LVL_IMMORT, CMP, true,
					"Character %d didn't load from account '%s'",
					char_id, d->account->get_name());

				send_to_desc(d, "Sorry.  There was an error processing your request.\r\n");
				send_to_desc(d, "The gods are not ignorant of your plight.\r\n\r\n");
				delete d->creature;
				d->creature = NULL;
				return;
			}

			// If they were in the middle of something important
			if (d->creature->player_specials->desc_mode != CXN_UNKNOWN) {
				set_desc_state(d->creature->player_specials->desc_mode, d);
				return;
			}

			if (production_mode && d->account->deny_char_entry(d->creature)) {
				send_to_desc(d, "You can't have another character in the game right now.\r\n");
				delete d->creature;
				d->creature = NULL;
				return;
			}
            
            if (GET_LEVEL(d->creature) >= LVL_AMBASSADOR && GET_LEVEL(d->creature) < LVL_POWER) {
                Creature *tmp_ch = new Creature(true);
                for (int idx=1; !d->account->invalid_char_index(idx); idx++) {
                    tmp_ch->clear();
                    tmp_ch->loadFromXML(d->account->get_char_by_index(idx));
                    if (GET_LEVEL(tmp_ch) < LVL_POWER && GET_QUEST(tmp_ch) &&
                       GET_IDNUM(d->creature) != GET_IDNUM(tmp_ch)) {
                        send_to_desc(d, "You can't log on an immortal while %s is in a quest.\r\n", GET_NAME(tmp_ch));
                        delete d->creature;
                        d->creature = NULL;
                        delete tmp_ch;
                        tmp_ch = NULL;
                        return;
                    }
                }
                delete tmp_ch;
                tmp_ch = NULL;
            }
            
			char_to_game(d);

			break;
		}
		break;
	case CXN_NAME_PROMPT:
		if (!arg[0]) {
			set_desc_state(CXN_MENU, d);
			return;
		}

		if (playerIndex.exists(arg)) {
			send_to_desc(d, "\r\nThat character name is already taken.\r\n\r\n");
			return;
		}

		if (d->creature) {
			errlog("Non-NULL creature during name prompt");
			send_to_desc(d, "\r\nSorry, there was an error creating your character.\r\n\r\n");
			set_desc_state(CXN_WAIT_MENU, d);
			return;
		}

		if (!Valid_Name(arg)) {
            send_to_desc(d, "\r\nThat character name is invalid.\r\n\r\n");
			return;
		}

		d->creature = d->account->create_char(arg);
		if (!d->creature) {
			errlog("Expected creature, got NULL during char creation");
			send_to_desc(d, "\r\nSorry, there was an error creating your character.\r\n\r\n");
			set_desc_state(CXN_WAIT_MENU, d);
		}
		d->creature->desc = d;
		d->creature->player_specials->rentcode = RENT_CREATING;
		set_desc_state(CXN_SEX_PROMPT, d);
		break;
	case CXN_SEX_PROMPT:
		switch (tolower(arg[0])) {
		case 'm':
			GET_SEX(d->creature) = 1;
			set_desc_state(CXN_CLASS_PROMPT, d);
			break;
		case 'f':
			GET_SEX(d->creature) = 2;
			set_desc_state(CXN_CLASS_PROMPT, d);
			break;
		default:
			send_to_desc(d, "\r\nPlease enter male or female.\r\n\r\n"); break;
		}
		break;
	case CXN_CLASS_PROMPT:
		if (is_abbrev("help", arg)) {
			show_pc_class_help(d, arg);
			return;
		}
		GET_CLASS(d->creature) = parse_player_class(arg);
		if (GET_CLASS(d->creature) == CLASS_UNDEFINED) {
			SEND_TO_Q(CCRED(d->creature, C_NRM), d);
			SEND_TO_Q("\r\nThat's not a character class.\r\n\r\n", d);
			SEND_TO_Q(CCNRM(d->creature, C_NRM), d);
			return;
		}
		set_desc_state(CXN_RACE_PROMPT, d);
		break;
	case CXN_RACE_PROMPT:
		if (is_abbrev("help", arg)) {
			show_pc_race_help(d, arg);
			return;
		}
		GET_RACE(d->creature) = parse_pc_race(d, arg);
		if (GET_RACE(d->creature) == RACE_UNDEFINED) {
			send_to_desc(d, "&gThat's not an allowable race!&n\r\n");
			return;
		}

		for (i=0;i < NUM_PC_RACES;i++)
			if (race_restr[i][0] == GET_RACE(d->creature))
				break;

		if (race_restr[i][GET_CLASS(d->creature)+1] != 2) {
			SEND_TO_Q(CCGRN(d->creature, C_NRM), d);
			SEND_TO_Q("\r\nThat race is not allowed to your character class!\r\n", d);
			SEND_TO_Q(CCNRM(d->creature, C_NRM), d);

			GET_RACE(d->creature) = RACE_UNDEFINED;
			break;
		}

		if (GET_RACE(d->creature) == RACE_UNDEFINED) {
			SEND_TO_Q(CCRED(d->creature, C_NRM), d);
			SEND_TO_Q("\r\nThat's not a choice.\r\n\r\n", d);
			SEND_TO_Q(CCNRM(d->creature, C_NRM), d);
			break;
		}

		set_desc_state(CXN_ALIGN_PROMPT, d);
		break;
	case CXN_CLASS_REMORT:
		if (is_abbrev(arg, "help")) {
			show_pc_class_help(d, arg);
			return;
		}
		GET_REMORT_CLASS(d->creature) = parse_player_class(arg);
		if (GET_REMORT_CLASS(d->creature) == CLASS_UNDEFINED) {
			SEND_TO_Q(CCRED(d->creature, C_NRM), d);
			SEND_TO_Q("\r\nThat's not a character class.\r\n\r\n", d);
			SEND_TO_Q(CCNRM(d->creature, C_NRM), d);
			return;
		}

		for (i=0;i < NUM_PC_RACES;i++)
			if (race_restr[i][0] == GET_RACE(d->creature))
				break;
		if (!race_restr[i][GET_REMORT_CLASS(d->creature)+1]) {
			SEND_TO_Q(CCGRN(d->creature, C_NRM), d);
			SEND_TO_Q("\r\nThat character class is not allowed to your race!\r\n\r\n", d);
		} else if (GET_REMORT_CLASS(d->creature) == GET_CLASS(d->creature)) {
			SEND_TO_Q(CCGRN(d->creature, C_NRM), d);
			SEND_TO_Q("\r\nYou can't remort to your primary class!\r\n\r\n", d);
			
		} else if ((GET_CLASS(d->creature) == CLASS_MONK &&
				(GET_REMORT_CLASS(d->creature) == CLASS_KNIGHT ||
				GET_REMORT_CLASS(d->creature) == CLASS_CLERIC)) ||
				((GET_CLASS(d->creature) == CLASS_CLERIC ||
				GET_CLASS(d->creature) == CLASS_KNIGHT) &&
				GET_REMORT_CLASS(d->creature) == CLASS_MONK)) {
			// No being a monk and a knight or cleric
			SEND_TO_Q(CCGRN(d->creature, C_NRM), d);
			SEND_TO_Q("\r\nYour religious beliefs are in conflict with that class!\r\n\r\n", d);
			
		} else {
			if (GET_CLASS(d->creature) == CLASS_VAMPIRE)
				GET_CLASS(d->creature) = GET_OLD_CLASS(d->creature);
			mudlog(LVL_IMMORT, BRF, true,
				   "%s has remorted to gen %d as a %s/%s",
				   GET_NAME(d->creature), GET_REMORT_GEN(d->creature),
				   class_names[(int)GET_CLASS(d->creature)],
				   class_names[(int)GET_REMORT_CLASS(d->creature)]);
				d->creature->saveToXML();
				set_desc_state( CXN_MENU,d );
		}
		break;
	case CXN_ALIGN_PROMPT:
		if ( IS_DROW(d->creature) ) {
			GET_ALIGNMENT(d->creature) = -666;
			set_desc_state( CXN_STATISTICS_ROLL,d );
			break;
		} else if ( IS_MONK( d->creature ) ) {
			GET_ALIGNMENT(d->creature) = 0;
			set_desc_state( CXN_STATISTICS_ROLL,d );
			break;
		}

		if (is_abbrev(arg, "evil")) {
            if (GET_CLASS(d->creature) == CLASS_BARD) {
                SEND_TO_Q("The bard class must be either Good or Neutral.\r\n\r\n", d);
                break;
            }
			d->creature->char_specials.saved.alignment = -666;
        }
		else if (is_abbrev(arg, "good"))
			d->creature->char_specials.saved.alignment = 777;
		else if (is_abbrev(arg, "neutral")) {
			if (GET_CLASS(d->creature) == CLASS_KNIGHT ||
				GET_CLASS(d->creature) == CLASS_CLERIC) {
				SEND_TO_Q(CCGRN(d->creature, C_NRM), d);
				SEND_TO_Q("Characters of your character class must be either Good or Evil.\r\n\r\n", d);
				break;
			}
			d->creature->char_specials.saved.alignment = 0;
		} else {
			send_to_desc(d, "\r\nThat's not a choice.\r\n\r\n");
			break;
		}

		if (GET_CLASS(d->creature) == CLASS_CLERIC)
			GET_DEITY(d->creature) = DEITY_GUIHARIA;
		else if (GET_CLASS(d->creature) == CLASS_KNIGHT)
			if (IS_GOOD(d->creature))
				GET_DEITY(d->creature) = DEITY_JUSTICE;
			else
				GET_DEITY(d->creature) = DEITY_ARES;
		else
			GET_DEITY(d->creature) = DEITY_GUIHARIA;

		set_desc_state( CXN_STATISTICS_ROLL,d );
		break;
	case CXN_STATISTICS_ROLL:
		if (is_abbrev(arg, "reroll")) {
			d->wait = 4;
			break;
		} else if (is_abbrev(arg, "keep")) {
			set_desc_state( CXN_EDIT_DESC,d );
			mlog(Security::ADMINBASIC, LVL_IMMORT, NRM, true,
				"%s[%d] has created new character %s[%ld]",
					d->account->get_name(), d->account->get_idnum(),
                    GET_NAME(d->creature), GET_IDNUM(d->creature) );
			d->creature->player_specials->rentcode = RENT_NEW_CHAR;
            calculate_height_weight( d->creature );
			d->creature->saveToXML();
		} else
			SEND_TO_Q("\r\nYou must type 'reroll' or 'keep'.\r\n\r\n", d);
		break;
	case CXN_EDIT_DESC:
		break;
	case CXN_DELETE_PROMPT:
		if (d->account->invalid_char_index(atoi(arg))) {
			send_to_desc(d, "\r\nThat character selection doesn't exist.\r\n\r\n");
			set_desc_state(CXN_WAIT_MENU, d);
			return;
		}

		char_id = d->account->get_char_by_index(atoi(arg));
		d->creature = get_char_in_world_by_idnum(char_id);
		if (!d->creature) {
			d->creature = new Creature(true);
			if (!d->creature->loadFromXML(char_id)) {
				send_to_desc(d, "Sorry.  That character could not be loaded.\r\n");
				set_desc_state(CXN_WAIT_MENU, d);
				return;
			}
		}

		set_desc_state(CXN_DELETE_PW, d);
		break;
	case CXN_EDIT_PROMPT:
		if (d->account->invalid_char_index(atoi(arg))) {
			send_to_desc(d, "\r\nThat character selection doesn't exist.\r\n\r\n");
			set_desc_state(CXN_WAIT_MENU, d);
			return;
		}

		char_id = d->account->get_char_by_index(atoi(arg));
		d->creature = new Creature(true);
		d->creature->desc = d;
		if (!d->creature->loadFromXML(char_id)) {
			send_to_desc(d, "Sorry.  That character could not be loaded.\r\n");
			set_desc_state(CXN_WAIT_MENU, d);
			return;
		}

		d->creature->desc = d;
		set_desc_state(CXN_EDIT_DESC, d);
		break;
	case CXN_DELETE_PW:
		if (d->account->authenticate(arg)) {
			set_desc_state(CXN_DELETE_VERIFY, d);
			return;
		}

		send_to_desc(d, "\r\n\r\n              &yWrong password!  %s will not be deleted.\r\n",
			GET_NAME(d->creature));
		if (!d->creature->in_room)
			delete d->creature;

		d->creature = NULL;
		set_desc_state(CXN_WAIT_MENU, d);
		break;
	case CXN_DELETE_VERIFY:
		if (strcmp(arg, "yes")) {
			send_to_desc(d, "\r\n              &yDelete cancelled.  %s will not be deleted.\r\n\r\n",
				GET_NAME(d->creature));
			if (!d->creature->in_room)
				delete d->creature;
			
			d->creature = NULL;
			set_desc_state(CXN_WAIT_MENU, d);
			break;
		}
		
		send_to_desc(d, "\r\n              &y%s has been deleted.&n\r\n\r\n", GET_NAME(d->creature));
		slog("%s[%d] has deleted character %s[%ld]",
				d->account->get_name(), d->account->get_idnum(),
				GET_NAME(d->creature), GET_IDNUM(d->creature) );
		if (d->creature->in_room) {
			// if the character is already in the game, delete_char will
			// handle it
			d->account->delete_char(d->creature);
		} else {
			// if the character was loaded, we need to delete it "manually"
			d->account->delete_char(d->creature);
			delete d->creature;
		}
		d->creature = NULL;
        d->account->reload();
		set_desc_state(CXN_WAIT_MENU, d);
		break;
	case CXN_AFTERLIFE:
		char_to_game(d); break;
	case CXN_WAIT_MENU:
		set_desc_state(CXN_MENU, d); break;
	case CXN_REMORT_AFTERLIFE:
		set_desc_state(CXN_CLASS_REMORT, d); break;
	case CXN_NETWORK:
		handle_network(d, arg); break;
	case CXN_OLDPW_PROMPT:
		if (d->account->authenticate(arg)) {
			set_desc_state(CXN_NEWPW_PROMPT, d);
			return;
		}

		send_to_desc(d, "\r\nWrong password!  Password change cancelled.\r\n\r\n");
		set_desc_state(CXN_WAIT_MENU, d);
		break;
	case CXN_NEWPW_PROMPT:
		if (arg[0]) {
			d->account->set_password(arg);
			set_desc_state(CXN_NEWPW_VERIFY, d);
		} else {
			send_to_desc(d, "\r\nPassword change cancelled!\r\n\r\n");
			set_desc_state(CXN_WAIT_MENU, d);
		}
		break;
	case CXN_NEWPW_VERIFY:
		if (!arg[0]) {
			send_to_desc(d, "\r\nPassword change cancelled!\r\n\r\n");
			set_desc_state(CXN_WAIT_MENU, d);
		} if (!d->account->authenticate(arg)) {
			send_to_desc(d, "\r\nPasswords did not match.  Please try again.\r\n\r\n");
			set_desc_state(CXN_PW_PROMPT, d);
		} else {
			send_to_desc(d, "\r\n\r\nPassword changed!\r\n\r\n");
			set_desc_state(CXN_WAIT_MENU, d);
		}
		break;
	case CXN_VIEW_BG:
		if (tolower(*arg) != 'q') {
			show_string(d);
			if (!d->showstr_head)
				set_desc_state(CXN_WAIT_MENU, d);
		} else
			set_desc_state(CXN_MENU, d);
		break;
    case CXN_DETAILS_PROMPT:
		if (d->account->invalid_char_index(atoi(arg))) {
			send_to_desc(d, "\r\nThat character selection doesn't exist.\r\n\r\n");
			set_desc_state(CXN_WAIT_MENU, d);
			return;
		}

		char_id = d->account->get_char_by_index(atoi(arg));
		d->creature = new Creature(true);
		d->creature->desc = d;
		if (!d->creature->loadFromXML(char_id)) {
			send_to_desc(d, "Sorry.  That character could not be loaded.\r\n");
			set_desc_state(CXN_WAIT_MENU, d);
			return;
		}

		d->creature->desc = d;
		show_character_detail(d);
		delete d->creature;
		d->creature = NULL;
		set_desc_state(CXN_WAIT_MENU, d);
        break;
	case CXN_CLASS_HELP:
		if (d->showstr_head && tolower(*arg) != 'q') {
			show_string(d);
			if (!d->showstr_head)
				send_to_desc(d,
					"&r**** &nPress return to go back to class selection &r****&n\r\n");
		} else
			set_desc_state(CXN_CLASS_PROMPT, d);
		break;
	case CXN_RACE_HELP:
		if (d->showstr_head && tolower(*arg) != 'q') {
			show_string(d);
			if (!d->showstr_head)
				send_to_desc(d,
					"&r**** &nPress return to go back to race selection &r****&n\r\n");
		} else
			set_desc_state(CXN_RACE_PROMPT, d);
		break;
	}
}


void
send_prompt(descriptor_data *d)
{
	char prompt[MAX_INPUT_LENGTH];
    char colorbuf[100];

	// No prompt for the wicked
	if (d->input_mode == CXN_DISCONNECT )
		return;

	// Check for the text editor being used
	if (d->creature && d->text_editor) {
        d->text_editor->SendPrompt();
		d->need_prompt = false;
		return;
	}

	// Handle all other states
	switch ( d->input_mode ) {
	case CXN_UNKNOWN:
	case CXN_DISCONNECT:
		break;
	case CXN_PLAYING:			// Playing - Nominal state
		*prompt = '\0';

		if (GET_INVIS_LVL(d->creature))
			sprintf(prompt,"%s%s(%si%d%s)%s ",prompt,CCMAG(d->creature, C_NRM),
				CCRED(d->creature, C_NRM), GET_INVIS_LVL(d->creature),
				CCMAG(d->creature, C_NRM), CCNRM(d->creature, C_NRM));
		else if (IS_MOB(d->creature))
			sprintf(prompt, "%s%s[NPC]%s ", prompt,
				CCCYN(d->creature, C_NRM), CCNRM(d->creature, C_NRM));

		if (PRF_FLAGGED(d->creature, PRF_DISPHP))
			sprintf(prompt, "%s%s%s< %s%d%s%sH%s ", prompt,
					CCWHT(d->creature, C_SPR), CCBLD(d->creature, C_CMP),
					CCGRN(d->creature, C_SPR), GET_HIT(d->creature),
					CCNRM(d->creature, C_SPR),
					CCYEL_BLD(d->creature, C_CMP), CCNRM(d->creature, C_SPR));

		if (PRF_FLAGGED(d->creature, PRF_DISPMANA))
			sprintf(prompt, "%s%s%s%d%s%sM%s ", prompt,
					CCBLD(d->creature, C_CMP), CCMAG(d->creature, C_SPR),
					GET_MANA(d->creature), CCNRM(d->creature, C_SPR),
					CCYEL_BLD(d->creature, C_CMP), CCNRM(d->creature, C_SPR));
	
		if (PRF_FLAGGED(d->creature, PRF_DISPMOVE))
			sprintf(prompt, "%s%s%s%d%s%sV%s ", prompt,
					CCCYN(d->creature, C_SPR), CCBLD(d->creature, C_CMP),
					GET_MOVE(d->creature), CCNRM(d->creature, C_SPR),
					CCYEL_BLD(d->creature, C_CMP), CCNRM(d->creature, C_SPR));

		if ( PRF2_FLAGGED( d->creature, PRF2_DISPALIGN ) ) {
			
			if( IS_GOOD( d->creature ) ) {
			sprintf( colorbuf, "%s", CCCYN( d->creature, C_SPR ) );
		} else if ( IS_EVIL( d->creature ) ) {
			sprintf( colorbuf, "%s", CCRED( d->creature, C_SPR ) );
			} else {
			sprintf( colorbuf, "%s", CCWHT(d->creature, C_SPR ) );
			}

			sprintf( prompt, "%s%s%s%d%s%sA%s ", prompt,
					colorbuf, CCBLD( d->creature,C_CMP ),
					GET_ALIGNMENT( d->creature ), CCNRM( d->creature, C_SPR ),
					CCYEL_BLD( d->creature, C_CMP ), CCNRM( d->creature,C_SPR ) );
		}
        
        if ( PRF2_FLAGGED( d->creature, PRF2_DISPTIME ) ) {
            if (d->creature->in_room->zone->time_frame == TIME_TIMELESS) {
                sprintf(prompt, "%s%s%s%s", prompt, CCYEL_BLD(d->creature, C_CMP),
                "!TIME ", CCNRM(d->creature, C_SPR));
            } else {
                struct time_info_data local_time;
                set_local_time(d->creature->in_room->zone, &local_time);
                sprintf(colorbuf, "%s%s", CCNRM(d->creature, C_SPR), CCYEL( d->creature, C_NRM));
                if (local_time.hours > 8 && local_time.hours < 18) { //day
                    sprintf(colorbuf, "%s%s%s", colorbuf, CCWHT(d->creature, C_CMP), CCBLD(d->creature, C_CMP));
                } else if (local_time.hours >= 6 && local_time.hours <= 20) { //dawn/dusk
                    sprintf(colorbuf, "%s%s", colorbuf, CCCYN_BLD(d->creature, C_CMP));
                } else { //night
                    sprintf(colorbuf, "%s%s", colorbuf, CCBLU_BLD(d->creature, C_CMP));
                }
                
                sprintf(prompt, "%s%s%d%s%s%s%s ", prompt, colorbuf,
                ((local_time.hours % 12 == 0) ? 12 : ((local_time.hours) % 12)), 
                CCNRM(d->creature, C_SPR), CCYEL_BLD(d->creature, C_CMP),
                ((local_time.hours >= 12) ? "PM" : "AM"), CCNRM(d->creature, C_SPR));
            }
        }

		if (d->creature->numCombatants() &&
			PRF2_FLAGGED(d->creature, PRF2_AUTO_DIAGNOSE))
			sprintf(prompt, "%s%s(%s)%s ", prompt, CCRED(d->creature, C_NRM),
					diag_conditions(d->creature->findRandomCombat()),
					CCNRM(d->creature, C_NRM));
		
		sprintf(prompt, "%s%s%s>%s ", prompt, CCWHT(d->creature, C_NRM),
				CCBLD(d->creature, C_CMP), CCNRM(d->creature, C_NRM));
		SEND_TO_Q(prompt,d);
		d->output_broken = FALSE;
		break;
	case CXN_ACCOUNT_LOGIN:
		send_to_desc(d, "Login with your account name, or 'new' for a new account: ");
		break;
	case CXN_ACCOUNT_PW:
		send_to_desc(d, "Password: "); break;
	case CXN_PW_PROMPT:
		send_to_desc(d, "        Enter your desired password: "); break;
	case CXN_PW_VERIFY:
	case CXN_NEWPW_VERIFY:
		// this awkward wording due to lame "assword:" search in tintin instead
		// of actually implementing one facet of telnet protocol
		send_to_desc(d, "\r\n\r\n        Enter it again to verify your password: "); break;
	case CXN_ACCOUNT_PROMPT:
		send_to_desc(d, "\r\n\r\nWhat would you like the name of your account to be? ");
		break;
	case CXN_ACCOUNT_VERIFY:
		send_to_desc(d, "Are you sure you want your account name to be '%s' (Y/N)? ",
			d->last_input);
		break;
	case CXN_ANSI_PROMPT:
		send_to_desc(d, "Enter the level of color you prefer: "); break;
	case CXN_COMPACT_PROMPT:
		send_to_desc(d, "Enter the level of compactness you prefer: "); break;
	case CXN_EMAIL_PROMPT:
		send_to_desc(d, "Please enter your email address: "); break;
	case CXN_OLDPW_PROMPT:
		send_to_desc(d, "For security purposes, please enter your old password: "); break;
	case CXN_NEWPW_PROMPT:
		send_to_desc(d, "Enter your new password: "); break;
	case CXN_NAME_PROMPT:
		send_to_desc(d, "Enter the name you wish for this character: "); break;
	case CXN_SEX_PROMPT:
		send_to_desc(d, "What do you choose as your sex (M/F)? "); break;
	case CXN_RACE_PROMPT:
		send_to_desc(d, "\r\n\r\n                   Of which race are you a member? "); break;
	case CXN_CLASS_PROMPT:
		send_to_desc(d, "             Choose your profession from the above list: "); break;
	case CXN_CLASS_REMORT:
		send_to_desc(d, "             Choose your secondary class from the above list: "); break;
	case CXN_ALIGN_PROMPT:
		if (IS_DROW(d->creature)) {
			send_to_desc(d, "The Drow race is inherently evil.  Thus you begin your life as evil.\r\n\r\nPress return to continue.\r\n");
		} else if (IS_MONK(d->creature)) {
			send_to_desc(d, "The monastic ideology requires that you remain neutral in alignment.\r\nTherefore you begin your life with a perfect neutrality.\r\n\r\nPress return to continue.\r\n");
		} else if (IS_KNIGHT(d->creature) || IS_CLERIC(d->creature)) {
			send_to_desc(d, "Do you wish to be good or evil? ");
		} else {
			send_to_desc(d, "Do you wish to be good, neutral, or evil? ");
		}
		break;
	case CXN_STATISTICS_ROLL:
			roll_real_abils(d->creature);
			print_attributes_to_buf( d->creature,buf2 );
			send_to_desc(d, "\e[H\e[J");
			send_to_desc(d, "%s\r\n                              CHARACTER ATTRIBUTES\r\n*******************************************************************************\r\n\r\n\r\n",CCCYN(d->creature,C_NRM));
			send_to_desc(d, "%s\r\n", buf2);
			send_to_desc(d, "%sWould you like to %sREROLL%s or %sKEEP%s these attributes?%s ",
				CCCYN(d->creature,C_NRM),CCGRN(d->creature,C_NRM),
				CCCYN(d->creature,C_NRM),CCGRN(d->creature,C_NRM),
				CCCYN(d->creature,C_NRM),CCNRM(d->creature,C_NRM));
			break;
	case CXN_EDIT_DESC:
		break;
	case CXN_MENU:
		send_to_desc(d, "\r\n&c                             Choose your selection:&n ");
		break;
	case CXN_DELETE_PROMPT:
		send_to_desc(d, "              &yWhich character would you like to delete:&n "); break;
	case CXN_EDIT_PROMPT:
		send_to_desc(d, "               &cWhich character's description do you want to edit:&n "); break;
	case CXN_DELETE_PW:
		send_to_desc(d, "              &yTo confirm deletion of %s, enter your account password: &n",	GET_NAME(d->creature)); break;
	case CXN_DELETE_VERIFY:
		send_to_desc(d, "              &yType 'yes' for final confirmation: &n"); break;
	case CXN_WAIT_MENU:
		send_to_desc(d, "Press return to go back to the main menu."); break;
	case CXN_AFTERLIFE:
	case CXN_REMORT_AFTERLIFE:
		send_to_desc(d, "Press return to continue into the afterlife.\r\n"); break;
	case CXN_VIEW_BG:
	case CXN_CLASS_HELP:
	case CXN_RACE_HELP:
		// Prompt sent by page_string
		break;
    case CXN_DETAILS_PROMPT:
        send_to_desc(d, "                      &cWhich character do you want to view:&n "); break;
	case CXN_NETWORK:
		send_to_desc(d, "> "); break;
		break;
	}
	d->need_prompt = false;
}

void
send_menu(descriptor_data *d)
{
	Creature *tmp_ch;
	int idx;

	switch (d->input_mode) {
	case CXN_DISCONNECT:
	case CXN_PLAYING:
	case CXN_ACCOUNT_LOGIN:
	case CXN_ACCOUNT_PW:
	case CXN_ACCOUNT_VERIFY:
	case CXN_PW_VERIFY:
	case CXN_STATISTICS_ROLL:
	case CXN_DELETE_VERIFY:
	case CXN_AFTERLIFE:
	case CXN_REMORT_AFTERLIFE:
	case CXN_DELETE_PW:
		// These states don't have menus
		break;
	case CXN_OLDPW_PROMPT:
		send_to_desc(d, "\e[H\e[J");
		send_to_desc(d,"&c\r\n                                 CHANGE PASSWORD\r\n*******************************************************************************\r\n\r\n&n");
		break;
	case CXN_PW_PROMPT:
		send_to_desc(d, "\e[H\e[J");
		send_to_desc(d,"&c\r\n                                  SET PASSWORD\r\n*******************************************************************************\r\n\r\n&n");
		send_to_desc(d, "    In order to protect your character against intrusion, you must\r\nchoose a password to use on this system.\r\n\r\n");
		break;
	case CXN_ACCOUNT_PROMPT:
        if (check_newbie_ban(d)) {
			set_desc_state(CXN_DISCONNECT, d);
            break;
        }
		send_to_desc(d, "\e[H\e[J");
		send_to_desc(d,"&n\r\n                                ACCOUNT CREATION\r\n*******************************************************************************&n\r\n");
		send_to_desc(d, "\r\n\r\n    On TempusMUD, you have an account, which is a handy way of keeping\r\ntrack of all your characters here.  All your characters share a bank\r\naccount, and you can see at a single glance which of your character have\r\nreceived mail.  Quest points are also shared by all your characters.\r\n\r\n");
		break;
	case CXN_ANSI_PROMPT:
		send_to_desc(d, "\e[H\e[J");
		send_to_desc(d,"\r\n                                   ANSI COLOR\r\n*******************************************************************************&n\r\n");
		send_to_desc(d,
"\r\n\r\n"
"    This game supports ANSI color standards.  If you have a color capable\r\n"
"terminal and wish to see useful color-coding of text, select the amount of\r\n"
"coloring you desire.  You may experiment within the game with the 'color'\r\n"
"command to see which level suits your personal taste.\r\n"
"\r\n"
"    If your terminal does not support color, you will want to select\r\n"
"'none', as the color codes may mess up your display.  Use of at least\r\n"
"some color is HIGHLY recommended, as it improves gameplay dramatically.\r\n"
"\r\n"
"                None - No color will be used\r\n"
"              Sparse - Minimal amounts of color will be used.\r\n"
"              Normal - Color will be used a medium amount.\r\n"
"            Complete - Use the maximum amount of color available.\r\n\r\n");
		break;
	case CXN_COMPACT_PROMPT:
		send_to_desc(d, "\e[H\e[J");
		send_to_desc(d,"\r\n                              TEXT COMPACTNESS\r\n*******************************************************************************&n\r\n");
		send_to_desc(d,
"\r\n\r\n"
"    Many players have differing tastes as to the vertical spacing of their\r\n"
"display.  A less compact view is often easier to read, while a more\r\n"
"compact view allows for more lines to fit on the screen.\r\n\r\n"
"Compact off:                         Compact minimal:\r\n"
"<---H ---M ---V ---A>                <---H ---M ---V ---A>\r\n"
"A goblin spits in your face!         A goblin tries to steal from you!\r\n"
"\r\n"
"<---H ---M ---V ---A>                <---H ---M ---V ---A> kill goblin\r\n"
"kill goblin\r\n\r\n"
"Compact partial:                     Compact full:\r\n"
"<---H ---M ---V ---A>                <---H ---M ---V ---A>\r\n"
"A goblin gives you a wedgie!         A goblin laughs insultingly at you!\r\n"
"<---H ---M ---V ---A>                <---H ---M ---V ---A> kill goblin\r\n"
"kill goblin\r\n");
		break;
	case CXN_EMAIL_PROMPT:
		send_to_desc(d, "\e[H\e[J");
		send_to_desc(d,"&c\r\n                                 EMAIL ADDRESS\r\n*******************************************************************************&n\r\n");
		send_to_desc(d, "\r\n\r\n    You may elect to associate an email address with this account.  This\r\nis entirely optional, and will not be sold to anyone.  Its primary use is\r\npassword reminders but may soon be used for Realm board login.\r\n\r\n");
		break;
	case CXN_NAME_PROMPT:
		send_to_desc(d, "\e[H\e[J");
		send_to_desc(d, "\r\n&c                                 CHARACTER CREATION\r\n*******************************************************************************&n\r\n");
		send_to_desc(d, "\r\n    Now that you have created your account, you probably want to create a\r\ncharacter to play on the mud.  This character will be your persona on the\r\nmud, allowing you to interact with other people and things.  You may press\r\nreturn at any time to cancel the creation of your character.\r\n\r\n");
		if (d->account->get_char_count())
			send_to_desc(d, "You have %d character%s in your account, you may create up to %d more.\r\n\r\n", 
				d->account->get_char_count(),
				d->account->get_char_count()==1 ? "" : "s",
				d->account->chars_available());
		break;
	case CXN_SEX_PROMPT:
		send_to_desc(d, "\e[H\e[J");
		send_to_desc(d, "&c\r\n                                     GENDER\r\n*******************************************************************************\r\n&n");
		send_to_desc(d, "\r\n    Is your character a male or a female?\r\n\r\n");
		break;
	case CXN_RACE_PROMPT:			// Racial Query
		send_to_desc(d, "\e[H\e[J");
		send_to_desc(d,
"&c\r\n                                      RACE\r\n"
"*******************************************************************************&n\r\n"
"    Races on Tempus have nothing to do with coloration.  Your character's\r\n"
"race refers to the intelligent species that your character can be.  Each\r\n"
"has its own advantages and disadvantages, and some have special abilities!\r\n"
"You may type 'help <race>' for information on any of the available races.\r\n"
"\r\n"
		);
		show_pc_race_menu(d);
		break;
	case CXN_CLASS_PROMPT:
		send_to_desc(d, "\e[H\e[J");
		send_to_desc(d,
"&c\r\n                                   PROFESSION\r\n"
"*******************************************************************************&n\r\n"
"    Your character class is the special training your character has had\r\n"
"before embarking on the life of an adventurer.  Your class determines\r\n"
"most of your character's capabilities within the game.  You may type\r\n"
"'help <class>' for an overview of a particular class.\r\n"
"\r\n"
		);
		show_char_class_menu(d, false);
		break;
	case CXN_CLASS_REMORT:
		send_to_desc(d, "\e[H\e[J");
		send_to_desc(d, "&c\r\n                                   PROFESSION\r\n*******************************************************************************&n\r\n\r\n\r\n");
		show_char_class_menu(d, true);
		break;
	case CXN_ALIGN_PROMPT:
		send_to_desc(d, "\e[H\e[J");
		send_to_desc(d, "&c\r\n                                   ALIGNMENT\r\n*******************************************************************************&n\r\n");
		send_to_desc(d, "\r\n\r\n    ALIGNMENT is a measure of your philosophies and morals.\r\n\r\n");
		break;
	case CXN_EDIT_DESC:
		send_to_desc(d, "\e[H\e[J");
		send_to_desc(d, "&c\r\n                                  DESCRIPTION\r\n*******************************************************************************&n\r\n");
		send_to_desc(d, "\r\n\r\n    Other players will usually be able to determine your general\r\n"
				  "size, as well as your race and gender, by looking at you.  What\r\n"
				  "else is noticable about your character?\r\n\r\n");
        if( d->text_editor ) {
            d->text_editor->SendStartupMessage();
            send_to_desc(d, "\r\n");
        }
            
		break;
	case CXN_MENU:
		// If we have a creature, save and offload
		if (d->creature) {
			d->creature->saveToXML();
			delete d->creature;
			d->creature = NULL;
		}

		send_to_desc(d, "\e[H\e[J");
		send_to_desc(d,
			"&c*&n&b-----------------------------------------------------------------------------&c*\r\n"
			"&n&b|                                 &YT E M P U S&n                                 &b|\r\n"
			"&c*&b-----------------------------------------------------------------------------&c*&n\r\n\r\n");
		
		if (d->account->get_char_count() > 0) {
			show_account_chars(d,
				d->account,
				false,
				(d->account->get_char_count() > 5));
			send_to_desc(d, "You have %d character%s in your account, you may create up to %d more.\r\n\r\n", 
				d->account->get_char_count(),
				d->account->get_char_count()==1 ? "" : "s",
				d->account->chars_available());
		}
        send_to_desc(d, "\r\n             Past bank: %-12lld      Future Bank: %-12lld\r\n\r\n",
			d->account->get_past_bank(), d->account->get_future_bank());
        
		send_to_desc(d, "    &b[&yP&b] &cChange your account password     &b[&yV&b] &cView the background story\r\n");
	    send_to_desc(d, "    &b[&yC&b] &cCreate a new character");
        if (d->account->get_char_count() > 0) {
			send_to_desc(d, "           &b[&yS&b] &cShow character details\r\n");
			send_to_desc(d, "    &b[&yE&b] &cEdit a character's description   &b[&yD&b] &cDelete an existing character\r\n");
		} else {
			send_to_desc(d, "\r\n");
		}
		send_to_desc(d, "\r\n                            &b[&yL&b] &cLog out of the game&n\r\n");

		// Helpful items for those people with only one character
		if (d->account->get_char_count() == 0) {
			send_to_desc(d,
"\r\n      This menu is your account menu, where you can manage your account,\r\n"
"      and enter the game.  You currently have no characters associated with\r\n"
"      your account.  To create a character, type 'c' and press return.\r\n");

		} else if (d->account->get_char_count() == 1) {
			send_to_desc(d, 
"\r\n      This menu is your account menu, where you can manage your account,\r\n"
"      and enter the game.  Your characters are listed at the top.  To\r\n"
"      enter the game, you may type the number of the character you wish to\r\n"
"      play.  To select one of the other options, type the adjacent letter.\r\n"
			);
		}
		break;
	case CXN_DELETE_PROMPT:
		send_to_desc(d, "\e[H\e[J");
		send_to_desc(d, "&r\r\n                                DELETE CHARACTER\r\n*******************************************************************************&n\r\n\r\n");

		idx = 1;
		tmp_ch = new Creature(true);
		while (!d->account->invalid_char_index(idx)) {
			tmp_ch->clear();
			tmp_ch->loadFromXML(d->account->get_char_by_index(idx));
			send_to_desc(d, "    &r[&y%2d&r] &y%-20s %10s %10s %6s %s\r\n",
				idx, GET_NAME(tmp_ch),
				player_race[(int)GET_RACE(tmp_ch)],
				class_names[GET_CLASS(tmp_ch)],
				genders[(int)GET_SEX(tmp_ch)],
				GET_LEVEL(tmp_ch) ? tmp_sprintf("lvl %d", GET_LEVEL(tmp_ch)):"&m new");
			idx++;
		}
		delete tmp_ch;
		send_to_desc(d, "&n\r\n");
		break;
	case CXN_EDIT_PROMPT:
		send_to_desc(d, "\e[H\e[J");
		send_to_desc(d, "&c\r\n                         EDIT CHARACTER DESCRIPTION\r\n*******************************************************************************&n\r\n\r\n");

		idx = 1;
		tmp_ch = new Creature(true);
		while (!d->account->invalid_char_index(idx)) {
			tmp_ch->clear();
			tmp_ch->loadFromXML(d->account->get_char_by_index(idx));
			send_to_desc(d, "    &c[&n%2d&c] &c%-20s &n%10s %10s %6s %s\r\n",
				idx, GET_NAME(tmp_ch),
				player_race[(int)GET_RACE(tmp_ch)],
				class_names[GET_CLASS(tmp_ch)],
				genders[(int)GET_SEX(tmp_ch)],
				GET_LEVEL(tmp_ch) ? tmp_sprintf("lvl %d", GET_LEVEL(tmp_ch)):"&m new");
			idx++;
		}
		delete tmp_ch;
		send_to_desc(d, "&n\r\n");
		break;
	case CXN_DETAILS_PROMPT:
		send_to_desc(d, "\e[H\e[J");
		send_to_desc(d, "&c\r\n                            VIEW CHARACTER DETAILS\r\n*******************************************************************************&n\r\n\r\n");

		idx = 1;
		tmp_ch = new Creature(true);
		while (!d->account->invalid_char_index(idx)) {
			tmp_ch->clear();
			tmp_ch->loadFromXML(d->account->get_char_by_index(idx));
			send_to_desc(d, "    &c[&n%2d&c] &c%-20s &n%10s %10s %6s %s\r\n",
				idx, GET_NAME(tmp_ch),
				player_race[(int)GET_RACE(tmp_ch)],
				class_names[GET_CLASS(tmp_ch)],
				genders[(int)GET_SEX(tmp_ch)],
				GET_LEVEL(tmp_ch) ? tmp_sprintf("lvl %d", GET_LEVEL(tmp_ch)):"&m new");
			idx++;
		}
		delete tmp_ch;
		send_to_desc(d, "&n\r\n");
		break;
	case CXN_NETWORK:
		SEND_TO_Q("\e[H\e[J",d );
		SEND_TO_Q("GLOBAL NETWORK SYSTEMS CLI\r\n",d);
		SEND_TO_Q("-------------------------------------------------------------------------------\r\n",d);
		SEND_TO_Q("Enter commands at prompt.  Use '@' to escape.  Use '?' for help.\r\n",d);
		break;
	case CXN_VIEW_BG:
		send_to_desc(d, "\e[H\e[J");
		page_string(d, tmp_sprintf("%s\r\n                                   BACKGROUND\r\n*******************************************************************************%s\r\n%s",
				(d->account->get_ansi_level() >= C_NRM) ? KCYN:"",
				(d->account->get_ansi_level() >= C_NRM) ? KNRM:"",
				background));
		// If there's no showstr_point, they finished already
		if (!d->showstr_point)
			set_desc_state(CXN_WAIT_MENU, d);
	case CXN_CLASS_HELP:
		if (!d->showstr_head)
			send_to_desc(d,
				"&r**** &nPress return to go back to class selection &r****&n\r\n");
		break;
	case CXN_RACE_HELP:
		if (!d->showstr_head)
			send_to_desc(d,
				"&r**** &nPress return to go back to race selection &r****&n\r\n");
		break;
	default:
		break;
	}
	return;
}

void
set_desc_state(cxn_state state,struct descriptor_data *d)
	{
	if (d->mode_data)
		free(d->mode_data);
	d->mode_data = NULL;
	if (d->input_mode == CXN_ACCOUNT_PW ||
			d->input_mode == CXN_PW_PROMPT ||
			d->input_mode == CXN_PW_VERIFY ||
			d->input_mode == CXN_OLDPW_PROMPT ||
			d->input_mode == CXN_DELETE_PW)
		echo_on(d);
	d->input_mode = state;
	if (d->input_mode == CXN_ACCOUNT_PW ||
			d->input_mode == CXN_PW_PROMPT ||
			d->input_mode == CXN_PW_VERIFY ||
			d->input_mode == CXN_OLDPW_PROMPT ||
			d->input_mode == CXN_DELETE_PW)
		echo_off(d);
	if (CXN_AFTERLIFE == state) {
		d->inbuf[0] = '\0';
		d->wait = 5 RL_SEC;
        if (d->input.head) {
            flush_q(&d->input);
        }
	}
	if (d->creature)
		d->creature->saveToXML();

    send_menu(d);

	if (CXN_EDIT_DESC == state) {
		if (!d->creature) {
			errlog("set_desc_state called with CXN_EDIT_DESC with no creature.");
			send_to_desc(d, "You can't edit yer desc right now.\r\n");
			set_desc_state(CXN_WAIT_MENU, d);
			return;
		}
		start_editing_text(d,&d->creature->player.description, MAX_CHAR_DESC-1);
	}

	d->need_prompt = true;
}

/*
 * Turn off echoing (specific to telnet client)
 */
void 
echo_off(struct descriptor_data *d)
{
    char off_string[] =
    {
        (char) IAC,
        (char) WILL,
        (char) TELOPT_ECHO,
        (char) 0,
    };

    SEND_TO_Q(off_string, d);
}


/*
 * Turn on echoing (specific to telnet client)
 */
void 
echo_on(struct descriptor_data *d)
{
    char on_string[] =
    {
        (char) IAC,
        (char) WONT,
        (char) TELOPT_ECHO,
        (char) TELOPT_NAOCRD,
        (char) TELOPT_NAOFFD,
        (char) 0,
    };

    SEND_TO_Q(on_string, d);
}

/* clear some of the the working variables of a char */
void
reset_char(struct Creature *ch)
{
	int i;

	for (i = 0; i < NUM_WEARS; i++) {
		ch->equipment[i] = NULL;
		ch->implants[i] = NULL;
	}

	ch->followers = NULL;
	ch->master = NULL;
	/* ch->in_room = NOWHERE; Used for start in room */
	ch->carrying = NULL;
	ch->removeAllCombat();
	ch->char_specials.position = POS_STANDING;
	if (ch->mob_specials.shared)
		ch->mob_specials.shared->default_pos = POS_STANDING;
	ch->char_specials.carry_weight = 0;
	ch->char_specials.carry_items = 0;
	ch->player_specials->olc_obj = NULL;
	ch->player_specials->olc_mob = NULL;

	if (GET_HIT(ch) <= 0)
		GET_HIT(ch) = 1;
	if (GET_MOVE(ch) <= 0)
		GET_MOVE(ch) = 1;
	if (GET_MANA(ch) <= 0)
		GET_MANA(ch) = 1;

	GET_LAST_TELL_FROM(ch) = NOBODY;
	GET_LAST_TELL_TO(ch) = NOBODY;
}

void
char_to_game(descriptor_data *d)
{
	struct clan_data* clan_by_owner( int idnum );
	struct descriptor_data *k, *next;
	struct room_data *load_room = NULL;
	time_t now = time(0);
	char *notes = "";
    Quest *quest;

	// this code is to prevent people from multiply logging in
	for (k = descriptor_list; k; k = next) {
		next = k->next;
		if (!k->input_mode && k->creature &&
			!str_cmp(GET_NAME(k->creature), GET_NAME(d->creature))) {
			SEND_TO_Q("Your character has been deleted.\r\n", d);
			set_desc_state( CXN_DISCONNECT,d );
			return;
		}
	}

	if (!mini_mud)
		SEND_TO_Q("\e[H\e[J", d);

	reset_char(d->creature);

	// Report and go back to menu if buried
	if (PLR2_FLAGGED(d->creature, PLR2_BURIED)) {
		SEND_TO_Q(tmp_sprintf(
			"You lay fresh flowers on the grave of %s.\r\n",
			GET_NAME(d->creature)), d);
		mudlog(LVL_GOD, NRM, true,
			"Denying entry to buried character %s",
			GET_NAME(d->creature));
		set_desc_state(CXN_WAIT_MENU, d);
		return;
	}

	// if we're not a new char, check loadroom and rent
	if (GET_LEVEL(d->creature)) {
		// Figure out the room the player is gonna start in
        load_room = NULL;
        if ((GET_LEVEL(d->creature) < LVL_AMBASSADOR) && 
            (quest = quest_by_vnum(GET_QUEST(d->creature)))) {
            if (quest->loadroom > -1)
                load_room = real_room(quest->loadroom);
        }
		if (!load_room && GET_LOADROOM(d->creature))
			load_room = real_room(GET_LOADROOM(d->creature));
		if (!load_room && GET_HOMEROOM(d->creature))
			load_room = real_room(GET_HOMEROOM(d->creature));
		if (!load_room)
			load_room = d->creature->getLoadroom();

		if (load_room && !House_can_enter(d->creature, load_room->number)) {
			mudlog(LVL_DEMI, NRM, true,
				"%s unable to load in house room %d",
				GET_NAME(d->creature),load_room->number);
			load_room = NULL;
		}

		if (load_room && !clan_house_can_enter(d->creature, load_room)) {
			mudlog(LVL_DEMI, NRM, true,
				"%s unable to load in clanhouse room %d; loadroom unset",
				GET_NAME(d->creature),load_room->number);
			load_room = NULL;
		}

		
		if (PLR_FLAGGED(d->creature, PLR_INVSTART))
			GET_INVIS_LVL(d->creature) = (GET_LEVEL(d->creature) > LVL_LUCIFER ?
										   LVL_LUCIFER : GET_LEVEL(d->creature));

		// Now load objects onto character
		switch (d->creature->unrent()) {
			case -1:
				notes = tmp_strcat(notes, "Your equipment could not be loaded.\r\n\r\n");
				mudlog(LVL_IMMORT, CMP, true, "%s's equipment could not be loaded.",
					GET_NAME(d->creature));
				break;
			case 0:
				// Everything ok
				break;
			case 1:
                // no eq file or file empty. no worries.
				break;
			case 2:
				notes = tmp_strcat(notes, "\r\n\007You could not afford your rent!\r\n"
					 "Some of your possessions have been sold to cover your bill!\r\n");
				break;
			case 3:
				load_room = real_room(number(10919, 10921));
				if (!load_room) {
					errlog("Can't send %s to jail - jail doesn't exist!",
						GET_NAME(d->creature));
				}
				notes = tmp_strcat(notes, "\r\nYou were unable to pay your rent and have been put in JAIL!\r\n");
				break;
			default:
				errlog("Can't happen at %s:%d", __FILE__, __LINE__);
		}

        // Loadroom is only good for one go
		GET_LOADROOM(d->creature) = 0;

        // Do we need to load up his corpse?
        if (d->creature->checkLoadCorpse()) {
            d->creature->loadCorpse();
        }

	} else { // otherwise null the loadroom
		load_room = NULL;
	}

	// Automatically reload their quest points if they haven't been on
	// since the last reload.
	if (GET_LEVEL(d->creature) >= LVL_AMBASSADOR && IS_PC(d->creature) &&
			GET_QUEST_ALLOWANCE(d->creature) > 0 &&
			GET_IMMORT_QP(d->creature) != GET_QUEST_ALLOWANCE(d->creature) &&
			d->creature->player.time.logon < last_sunday_time) {
		slog("Reset %s to %d QPs from %d (login)",
			GET_NAME(d->creature), GET_QUEST_ALLOWANCE(d->creature),
			GET_IMMORT_QP(d->creature));
		GET_IMMORT_QP(d->creature) = GET_QUEST_ALLOWANCE(d->creature);
		notes = tmp_strcat(notes, "Your quest points have been restored!\r\n");
	}
    
    // if their rep is 0 and they are >= gen 5 they gain 5 rep
	if (GET_REMORT_GEN(d->creature) >= 1 && d->creature->get_reputation() == 0) {
        d->creature->gain_reputation(5);
        notes = tmp_strcat(notes, "You are no longer innocent because you have reached your first generation.\r\n");
    }
    
	d->creature->player.time.logon = now;
	d->creature->saveToXML();
	send_to_char(d->creature, "%s%s%s%s",
		CCRED(d->creature, C_NRM), CCBLD(d->creature, C_NRM), WELC_MESSG,
		CCNRM(d->creature, C_NRM));

	characterList.add(d->creature);

	if(!load_room)
		load_room = d->creature->getLoadroom();

	char_to_room(d->creature, load_room);
	load_room->zone->enter_count++;
	show_mud_date_to_char(d->creature);
	SEND_TO_Q("\r\n", d);

	set_desc_state( CXN_PLAYING,d );

	if( GET_LEVEL(d->creature) < LVL_IMMORT )
		REMOVE_BIT(PRF2_FLAGS(d->creature), PRF2_NOWHO);

	if (!GET_LEVEL(d->creature)) {
		send_to_char(d->creature, START_MESSG);
		send_to_newbie_helpers(tmp_sprintf(
				" ***> New adventurer %s has entered the realm. <***\r\n",
				GET_NAME(d->creature)));
		do_start(d->creature, 0);

		// New characters shouldn't get old mail.
		if(has_mail(GET_IDNUM(d->creature))) {
		   if(purge_mail(GET_IDNUM(d->creature))>0) {
			   errlog("Purging pre-existing mailfile for new character.(%s)",
					GET_NAME(d->creature));
		   }
		}

	} else {
		mlog(Security::ADMINBASIC, GET_INVIS_LVL(d->creature), NRM, true,
			"%s has entered the game in room #%d%s",
             GET_NAME(d->creature),
             d->creature->in_room->number,
             d->account->is_banned() ? " [BANNED]":"");
		act("$n has entered the game.", true, d->creature, 0, 0, TO_ROOM);
	}

    // Wait 5-15 seconds before kicking them off for being banned
    if (d->account->is_banned() && !d->ban_dc_counter)
        d->ban_dc_counter = number(5 RL_SEC, 15 RL_SEC);

	look_at_room(d->creature, d->creature->in_room, 0);

	
	// Remove the quest prf flag (for who list) if they're
	// not in an active quest.
	if (GET_QUEST(d->creature)) {
		Quest *quest = quest_by_vnum( GET_QUEST(d->creature) );
		if (GET_QUEST(d->creature) == 0 ||
				quest == NULL ||
				quest->getEnded() != 0 ||
				!quest->isPlaying(GET_IDNUM(d->creature))) {
			slog("%s removed from quest %d", 
				  GET_NAME(d->creature), GET_QUEST(d->creature) );
			GET_QUEST(d->creature) = 0;
		}
	}

	REMOVE_BIT(PLR_FLAGS(d->creature), PLR_CRYO | PLR_WRITING | PLR_OLC |
		PLR_MAILING | PLR_AFK);
	REMOVE_BIT(PRF2_FLAGS(d->creature), PRF2_WORLDWRITE);

	if (*notes)
		send_to_char(d->creature, "%s", notes);

	if (has_mail(GET_IDNUM(d->creature)))
		send_to_char(d->creature, "You have mail waiting.\r\n");
	
	if (GET_CLAN(d->creature) && !real_clan(GET_CLAN(d->creature))) {
		send_to_char(d->creature, "Your clan has been disbanded.\r\n");
		GET_CLAN(d->creature) = 0;
	}

	notify_cleric_moon(d->creature);

	// check for dynamic text updates (news, inews, etc...)
	check_dyntext_updates(d->creature, CHECKDYN_UNRENT);
	
	// Check for house reposessions
	House *house = Housing.findHouseByOwner( d->creature->getAccountID() );
	if( house != NULL && house->getRepoNoteCount() > 0 )
		house->notifyReposession( d->creature );

	if( GET_CLAN(d->creature) != 0 ) {
		clan_data *clan = clan_by_owner( GET_IDNUM(d->creature) );
		if( clan != NULL ) {
			house = Housing.findHouseByOwner( clan->number, false );
			if( house != NULL && house->getRepoNoteCount() > 0 ) {
				house->notifyReposession( d->creature );
			}
		}
	}
    
    // Set thier languages here to make sure they speak their race language
    set_initial_tongue(d->creature);
	if (shutdown_count > 0)
		SEND_TO_Q(tmp_sprintf(
				"\r\n\007\007:: NOTICE :: Tempus will be rebooting in [%d] second%s ::\r\n",
				shutdown_count, shutdown_count == 1 ? "" : "s"), d);

	d->account->update_last_entry();
}

int 
_parse_name(char *arg, char *name)
{
    int i;

    /* skip whitespaces */
    for (; isspace(*arg); arg++);

    for (i = 0; (*name = *arg); arg++, i++, name++)
        if (!isalpha(*arg))
            return 1;

    if (!i)
        return 1;

    return 0;
}


/* *************************************************************************
*  Stuff for controlling the non-playing sockets (get name, pwd etc)       *
************************************************************************* */


const char *reserved[] =
{
    "self",
    "me",
    "all",
    "room",
    "someone",
    "something",
    "\n"
};

int reserved_word(char *argument)
{
    return (search_block(argument, reserved, TRUE) >= 0);
}

void
show_character_detail(descriptor_data *d)
{
	Creature *ch = d->creature;
	struct time_info_data playing_time;
	struct time_info_data real_time_passed(time_t t2, time_t t1);
	char *str;
	char time_buf[30];

	if (!ch) {
		errlog("show_player_detail() called without creature");
		return;
	}

	send_to_desc(d, "\e[H\e[J");
	send_to_desc(d, "&c\r\n                            VIEW CHARACTER DETAILS\r\n*******************************************************************************&n\r\n\r\n");

	if (IS_REMORT(ch)) {
		str = tmp_sprintf("%s%4s&n/%s%4s&n",
			get_char_class_color(ch, GET_CLASS(ch)),
			char_class_abbrevs[GET_CLASS(ch)],
			get_char_class_color(ch, GET_REMORT_CLASS(ch)),
			char_class_abbrevs[GET_REMORT_CLASS(ch)]);
	} else {
		str = tmp_sprintf("%s%9s&n",
			get_char_class_color(ch, GET_CLASS(ch)),
			class_names[GET_CLASS(ch)]);
	}
	send_to_desc(d, "&BName:&n %-20s &BLvl:&n %2d &BGen:&n %2d     &BClass:&n %s\r\n",
		GET_NAME(ch), GET_LEVEL(ch), GET_REMORT_GEN(ch), str);
	strftime(time_buf, 29, "%a %b %d, %Y %H:%M:%S",
		localtime(&ch->player.time.birth));
	send_to_desc(d, "\r\n&BCreated on:&n %s\r\n", time_buf);
	strftime(time_buf, 29, "%a %b %d, %Y %H:%M:%S",
		localtime(&ch->player.time.logon));
	send_to_desc(d, "&BLast logon:&n %s\r\n", time_buf);
	playing_time = real_time_passed(ch->player.time.played, 0);
	send_to_desc(d, "&BTime played:&n %d days, %d hours\r\n\r\n",
		playing_time.day, playing_time.hours);

	str = tmp_sprintf("%d/%d", GET_HIT(ch), GET_MAX_HIT(ch));
	send_to_desc(d, "&BHit Points:&n  %-25s &BAlignment:&n  %d\r\n",
		str, GET_ALIGNMENT(ch));
	str = tmp_sprintf("%d/%d", GET_MANA(ch), GET_MAX_MANA(ch));
	send_to_desc(d, "&BMana Points:&n %-25s &BReputation:&n %s\r\n",
		str, reputation_msg[GET_REPUTATION_RANK(ch)]);
	str = tmp_sprintf("%d/%d", GET_MOVE(ch), GET_MAX_MOVE(ch));
	send_to_desc(d, "&BMove Points:&n %-25s &BExperience:&n %d\r\n",
		str, GET_EXP(ch));

	send_to_desc(d, "\r\n");
}

void
show_account_chars(descriptor_data *d, Account *acct, bool immort, bool brief)
{
	const char *class_str, *status_str, *mail_str;
	char *sex_color = "";
	char *sex_str, *name_str;
	Creature *tmp_ch, *real_ch;
	char laston_str[40];
	int idx;

	if (!immort) {
		if (brief)
			send_to_desc(d,
"  # Name         Last on   Status  Mail   # Name         Last on   Status  Mail\r\n"
" -- --------- ---------- --------- ----  -- --------- ---------- --------- ----\r\n");
		else
			send_to_desc(d,
	"&y  # Name           Lvl Gen Sex     Race     Class      Last on    Status  Mail\r\n"
	"&b -- -------------- --- --- --- -------- --------- ------------- --------- ----\r\n");

	} 

	idx = 1;
	tmp_ch = new Creature(true);
	while (!acct->invalid_char_index(idx)) {

		tmp_ch->clear();

		if (!tmp_ch->loadFromXML(acct->get_char_by_index(idx))) {
			send_to_desc(d, "&R------ BAD PROBLEMS ------  PLEASE REPORT ------[%ld]&n\r\n",
                         acct->get_char_by_index(idx));
			idx++;
			continue;
		}

		// Deleted and buried characters don't show on player menus
		if(!immort && (PLR_FLAGGED(tmp_ch, PLR_DELETED) ||
				PLR2_FLAGGED(tmp_ch, PLR2_BURIED))) {
			idx++;
			continue;
		}

		switch( GET_SEX(tmp_ch) ) {
			case SEX_MALE:
				sex_color = "&b";
				break;
			case SEX_FEMALE:
				sex_color = "&m";
				break;
		}
		sex_str = tmp_sprintf("%s%c&n",
				sex_color, toupper(genders[(int)GET_SEX(tmp_ch)][0]) );
				
		name_str = tmp_strdup(GET_NAME(tmp_ch));
		if (strlen(name_str) > ((brief) ? 8:13))
			name_str[(brief) ? 8:13] = '\0';

		// Construct compact menu entry for each character
		if (IS_REMORT(tmp_ch)) {
			class_str = tmp_sprintf( "%s%4s&n/%s%4s&n",
				get_char_class_color( tmp_ch, GET_CLASS(tmp_ch) ),
				char_class_abbrevs[GET_CLASS(tmp_ch)],
				get_char_class_color( tmp_ch, GET_REMORT_CLASS(tmp_ch) ),
				char_class_abbrevs[GET_REMORT_CLASS(tmp_ch)] );
		} else {
			class_str = tmp_sprintf( "%s%9s&n",
				get_char_class_color( tmp_ch, GET_CLASS(tmp_ch) ),
				class_names[GET_CLASS(tmp_ch)] );
		}
		if (immort)
			strftime(laston_str, sizeof(laston_str), "%a, %d %b %Y %H:%M:%S",
				localtime(&tmp_ch->player.time.logon));
		else if (brief)
			strftime(laston_str, sizeof(laston_str), "%Y/%m/%d",
				localtime(&tmp_ch->player.time.logon));
		else
			strftime(laston_str, sizeof(laston_str), "%b %d, %Y",
				localtime(&tmp_ch->player.time.logon));
		if (PLR_FLAGGED(tmp_ch, PLR_FROZEN))
			status_str = "&CFROZEN!";
		else if (PLR2_FLAGGED(tmp_ch, PLR2_BURIED))
			status_str = "&GBURIED!";
		else if ((real_ch = get_char_in_world_by_idnum(GET_IDNUM(tmp_ch))) != NULL) {
			if (real_ch->desc)
				status_str = "&g  Playing";
			else
				status_str = "&c Linkless";
		} else if (tmp_ch->player_specials->desc_mode == CXN_AFTERLIFE) {
			status_str =     "&R     Died";
		} else switch (tmp_ch->player_specials->rentcode) {
			case RENT_CREATING:
				status_str = "&Y Creating"; break;
			case RENT_NEW_CHAR:
				status_str = "&Y      New"; break;
			case RENT_UNDEF:
				status_str = "&r    UNDEF"; break;
			case RENT_CRYO:
				status_str = "&c   Cryoed"; break;
			case RENT_CRASH:
				status_str = "&yCrashsave"; break;
			case RENT_RENTED:
				status_str = "&m   Rented"; break;
			case RENT_FORCED:
				status_str = "&yForcerent"; break;
			case RENT_QUIT:
				status_str = "&m     Quit"; break;
			case RENT_REMORTING:
				status_str = "&YRemorting"; break;
			default:
				status_str = "&R REPORTME"; break;
		}
		if (has_mail(GET_IDNUM(tmp_ch)))
			mail_str = "&Y Yes";
		else
			mail_str = "&n No ";
		if (immort) {
			send_to_desc(d,
				"&y%5ld &n%-13s %s&n %s\r\n",
				GET_IDNUM(tmp_ch), name_str, status_str, laston_str);
		} else if (tmp_ch->player_specials->rentcode == RENT_CREATING) {
			if (brief)
				send_to_desc(d,
					"&b[&y%2d&b] &n%-8s     &yNever  Creating&n  -- ",
					idx, name_str);
			else
				send_to_desc(d,
					"&b[&y%2d&b] &n%-13s   &y-   -  -         -         -         Never  Creating&n  --\r\n",
					idx, name_str);
		} else {
			if (brief)
				send_to_desc(d,
					"&b[&y%2d&b] &n%-8s %10s %s %s&n",
					idx, name_str,
					laston_str, status_str, mail_str);
			else
				send_to_desc(d,
					"&b[&y%2d&b] &n%-13s %3d %3d  %s  %8s %s %13s %s %s&n\r\n",
					idx, name_str,
					GET_LEVEL(tmp_ch), GET_REMORT_GEN(tmp_ch),
					sex_str,
					player_race[(int)GET_RACE(tmp_ch)],
					class_str, laston_str, status_str, mail_str);
		}
		idx++;
		if (brief) {
			if (idx & 1)
				send_to_desc(d, "\r\n");
			else
				send_to_desc(d, " ");
		}

	}
	if (brief) {
		if (idx & 1)
			send_to_desc(d, "\r\n");
		else
			send_to_desc(d, " ");
	}
	delete tmp_ch;
}

int check_newbie_ban(struct descriptor_data *desc)
{
    int bantype = isbanned(desc->host, buf2);
    if (bantype == BAN_NEW) {
        send_to_desc(desc, "**************************************************"
                           "******************************\r\n");
        send_to_desc(desc, "                               T E M P U S  M U D\r\n");
        send_to_desc(desc, "**************************************************"
                           "******************************\r\n");
        send_to_desc(desc, "\t\tWe're sorry, we have been forced to ban your "
                           "IP address.\r\n\tIf you have never played here "
                           "before, or you feel we have made\r\n\ta mistake, or "
                           "perhaps you just got caught in the wake of\r\n\tsomeone "
                           "else's trouble making, please mail "
                           "unban@tempusmud.com.\r\n\tPlease include your account "
                           "name and your character name(s)\r\n\tso we can siteok "
                           "your IP.  We apologize for the inconvenience,\r\n\tand "
                           "we hope to see you soon!");
        mlog(Security::ADMINBASIC, LVL_GOD, CMP, true,
             "Account creation denied from [%s]", desc->host);
        return BAN_NEW;
    }

    return 0;
}
#undef __interpreter_c__
