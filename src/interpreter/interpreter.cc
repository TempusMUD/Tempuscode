/* ************************************************************************
*   File: interpreter.c                                 Part of CircleMUD *
*  Usage: parse user commands, search for specials, call ACMD functions   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: interpreter.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#define __interpreter_c__
#include <iostream>
#include <fstream>
using namespace std;

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
#include "security.h"

const char *fill_words[] = {
	"in",
	"from",
	"with",
	"the",
	"on",
	"at",
	"to",
	"\n"
};

extern int log_cmds;
struct sort_struct *cmd_sort_info = NULL;
int num_of_cmds = 0;

int general_search(struct Creature *ch, struct special_search_data *srch,
	int mode);
long special(struct Creature *ch, int cmd, int subcmd, char *arg, special_mode spec_mode);


/* writes a string to the command log */
void
cmdlog(char *str)
{
	static char *log;
	static ofstream commandLog("log/command.log", ios::app);
	time_t ct;
	char *tmstr;

	ct = time(0);
	tmstr = asctime(localtime(&ct));
	*(tmstr + strlen(tmstr) - 1) = '\0';
	log = tmp_sprintf("%-19.19s :: %s", tmstr, str);
	commandLog << log << endl;
	commandLog.flush();
}


ACMD(do_zcom);
ACMD(do_objupdate);

/* prototypes for all do_x functions. */
ACMD(do_action);
ACMD(do_activate);
ACMD(do_account);
ACMD(do_addname);
ACMD(do_addpos);
ACMD(do_advance);
ACMD(do_affects);
ACMD(do_alias);
ACMD(do_alignment);
ACMD(do_alter);
ACMD(do_ambush);
ACMD(do_analyze);
ACMD(do_approve);
ACMD(do_arm);
ACMD(do_access);
ACMD(do_coderutil);
ACMD(do_unapprove);
ACMD(do_assist);
ACMD(do_assimilate);
ACMD(do_deassimilate);
ACMD(do_at);
ACMD(do_attach);
ACMD(do_attributes);
ACMD(do_autopsy);
ACMD(do_backstab);
ACMD(do_bash);
ACMD(do_ban);
ACMD(do_battlecry);
ACMD(do_beguile);
ACMD(do_breathe);
ACMD(do_crossface);
ACMD(do_demote);
ACMD(do_de_energize);
ACMD(do_defuse);
ACMD(do_disembark);
ACMD(do_disguise);
ACCMD(do_disarm);
ACMD(do_discharge);
ACMD(do_dismiss);
ACMD(do_distance);
ACMD(do_drain);
ACMD(do_dynedit);
ACMD(do_dyntext_show);
ACMD(do_berserk);
ACMD(do_board);
ACMD(do_bomb);
ACMD(do_bandage);
ACMD(do_cast);
ACMD(do_ceasefire);
ACMD(do_conceal);
ACMD(do_cedit);
ACMD(do_charge);
ACMD(do_circle);
ACMD(do_cinfo);
ACMD(do_cleave);
ACMD(do_clanpasswd);
ACMD(do_clanlist);
ACMD(do_clean);
ACMD(do_cyborg_reboot);
ACMD(do_cyberscan);
ACMD(do_bioscan);
ACMD(do_self_destruct);
ACMD(do_soilage);
ACMD(do_throat_strike);
ACMD(do_throw);
ACMD(do_trigger);
ACMD(do_transmit);
ACMD(do_translocate);
ACMD(do_tornado_kick);
ACMD(do_toss_mail);
ACMD(do_tune);
ACMD(do_color);
ACMD(do_combo);
ACMD(do_commands);
ACMD(do_compare);
ACMD(do_computer);
ACMD(do_consider);
ACMD(do_convert);
ACMD(do_corner);
ACMD(do_credits);
ACMD(do_date);
ACMD(do_dc);
ACMD(do_delete);
ACMD(do_diagnose);
ACMD(do_dismount);
ACMD(do_display);
ACMD(do_drag);
ACMD(do_drink);
ACMD(do_drive);
ACCMD(do_drop);
ACMD(do_eat);
ACMD(do_echo);
ACMD(do_econvert);
ACMD(do_elude);
ACMD(do_empower);
ACMD(do_empty);
ACMD(do_encumbrance);
ACMD(do_enter);
ACMD(do_enroll);
ACMD(do_equipment);
ACMD(do_examine);
ACMD(do_extinguish);
ACMD(do_experience);
ACMD(do_exit);
ACMD(do_exits);
ACMD(do_evade);
ACMD(do_evaluate);
ACMD(do_extract);
ACMD(do_feed);
ACMD(do_feign);
ACMD(do_firstaid);
ACMD(do_flee);
ACMD(do_fly);
ACMD(do_follow);
ACMD(do_force);
ACMD(do_gasify);
ACMD(do_gecho);
ACMD(do_gen_comm);
ACMD(do_gen_door);
ACMD(do_gen_ps);
ACMD(do_gen_tog);
ACMD(do_gen_write);
ACCMD(do_get);
ACMD(do_give);
ACMD(do_glance);
ACMD(do_gold);
ACMD(do_cash);
ACMD(do_goto);
ACMD(do_grab);
ACMD(do_group);
ACMD(do_gsay);
ACMD(do_gunset);
ACMD(do_hamstring);
ACMD(do_hcontrol);
ACMD(do_hedit);
ACMD(do_headbutt);
ACMD(do_hcollect_help);
ACMD(do_hide);
ACMD(do_hit);
ACMD(do_house);
ACMD(do_holytouch);
ACMD(do_hotwire);
ACMD(do_ignite);
ACMD(do_impale);
ACMD(do_improve);
ACMD(do_infiltrate);
ACMD(do_info);
ACMD(do_install);
ACMD(do_insert);
ACMD(do_intimidate);
ACMD(do_insult);
ACMD(do_inventory);
ACMD(do_invis);
ACMD(do_kata);
ACMD(do_kill);
ACMD(do_knock);
ACMD(do_last);
ACMD(do_leave);
ACMD(do_lecture);
ACMD(do_levels);
ACMD(do_listen);
ACMD(do_light);
ACMD(do_look);
ACMD(do_load);
ACMD(do_loadroom);
ACMD(do_makemount);
ACMD(do_medic);
ACMD(do_meditate);
ACMD(do_mload);
ACMD(do_mood);
ACMD(do_mount);
ACMD(do_show_more);
ACMD(do_move);
ACMD(do_mshield);
ACMD(do_not_here);
ACMD(do_nolocate);
ACMD(do_oecho);
ACMD(do_offer);
ACCMD(do_offensive_skill);
ACMD(do_olc);
ACMD(do_olchelp);
ACMD(do_oload);
ACMD(do_order);
ACMD(do_oset);
ACMD(do_overhaul);
ACMD(do_overdrain);
ACMD(do_page);
ACMD(do_palette);
ACMD(do_peace);
ACMD(do_pinch);
ACMD(do_pistolwhip);
ACMD(do_pkiller);
ACMD(do_plant);
ACMD(do_pload);
ACMD(do_poofset);
ACMD(do_point);
ACMD(do_pour);
ACMD(do_practice);
ACMD(do_promote);
ACMD(do_purge);
ACMD(do_put);
ACMD(do_psidrain);
ACMD(do_psilocate);
ACMD(do_qlog);
ACMD(do_qcomm);
ACMD(do_qpoints);
ACMD(do_qpreload);
ACMD(do_qcontrol);
ACMD(do_quit);
ACMD(do_quest);
ACMD(do_qsay);
ACMD(do_qecho);
ACMD(do_reboot);
ACMD(do_refill);
ACMD(do_recharge);
ACMD(do_remove);
ACMD(do_rename);
ACMD(do_rent);
ACMD(do_repair);
ACMD(do_reply);
ACMD(do_report);
ACMD(do_rescue);
ACMD(do_resign);
ACMD(do_rest);
ACMD(do_restore);
ACMD(do_return);
ACMD(do_retreat);
ACMD(do_roll);
ACMD(do_rswitch);
ACMD(do_save);
ACMD(do_sacrifice);
ACMD(do_say);
ACMD(do_scan);
ACMD(do_score);
ACMD(do_screen);
ACMD(do_search);
ACMD(do_searchfor);
ACMD(do_send);
ACMD(do_set);
ACMD(do_severtell);
ACMD(do_show);
ACMD(do_shoot);
ACMD(do_shutdown);
ACMD(do_shutoff);
ACMD(do_sit);
ACMD(do_skillset);
ACMD(do_skills);
ACMD(do_sleep);
ACMD(do_sleeper);
ACMD(do_smoke);
ACMD(do_snatch);
ACMD(do_sneak);
ACMD(do_snipe);
ACMD(do_snoop);
ACMD(do_spec_comm);
ACMD(do_specializations);
ACMD(do_special);
ACMD(do_split);
ACMD(do_stand);
ACMD(do_stat);
ACMD(do_status);
ACMD(do_stalk);
ACMD(do_steal);
ACMD(do_store);
ACMD(do_stun);
ACMD(do_switch);
ACMD(do_syslog);
ACMD(do_tag);
ACMD(do_taunt);
ACMD(do_teleport);
ACMD(do_tester);
ACMD(do_tell);
ACMD(do_time);
ACMD(do_title);
ACMD(do_toggle);
ACMD(do_track);
ACMD(do_trans);
ACMD(do_turn);
ACMD(do_unban);
ACMD(do_ungroup);
ACMD(do_uninstall);
ACMD(do_unalias);
ACMD(do_use);
ACMD(do_users);
ACMD(do_visible);
ACMD(do_vnum);
ACMD(do_vstat);
ACMD(do_rstat);
ACMD(do_wake);
ACMD(do_wear);
ACMD(do_weather);
ACMD(do_weigh);
ACMD(do_where);
ACMD(do_whirlwind);
ACMD(do_who);
ACCMD(do_wield);
ACMD(do_wimpy);
ACMD(do_wizcut);
ACMD(do_wizlock);
ACMD(do_wiznet);
ACMD(do_wizutil);
ACMD(do_mudwipe);
ACMD(do_wrench);
ACMD(do_write);
ACMD(do_xlag);
ACMD(do_zreset);
ACMD(do_zonepurge);
ACMD(do_zecho);
ACMD(do_rlist);
ACMD(do_olist);
ACMD(do_mlist);
ACMD(do_xlist);
ACMD(do_help_collection_command);
ACMD(do_wizlist);
ACMD(do_immhelp);
ACMD(do_map);


/* This is the Master Command List(tm).
 *
 * You can put new commands in, take commands out, change the order
 * they appear in, etc.  You can adjust the "priority" of commands
 * simply by changing the order they appear in the command list.
 * (For example, if you want "as" to mean "assist" instead of "ask",
 * just put "assist" above "ask" in the Master Command List(tm).
 *
 * In general, utility commands such as "at" should have high priority;
 * infrequently used and dangerously destructive commands should have low
 * priority.
 */



struct command_info cmd_info[] = {
	{"RESERVED", 0, 0, 0, 0, 0, 0},	/* this must be first -- for specprocs */

	/* directions must come before other commands but after RESERVED */
	{"north", POS_STANDING, do_move, 0, SCMD_NORTH, 0, 0},
	{"east", POS_STANDING, do_move, 0, SCMD_EAST, 0, 0},
	{"south", POS_STANDING, do_move, 0, SCMD_SOUTH, 0, 0},
	{"west", POS_STANDING, do_move, 0, SCMD_WEST, 0, 0},
	{"up", POS_STANDING, do_move, 0, SCMD_UP, 0, 0},
	{"down", POS_STANDING, do_move, 0, SCMD_DOWN, 0, 0},
	{"future", POS_STANDING, do_move, 0, SCMD_FUTURE, 0, 0},
	{"past", POS_STANDING, do_move, 0, SCMD_PAST, 0, 0},

	/* now, the main list */
	{"areas", POS_DEAD, do_areas, 0, 0, 0, 0},
	{"at", POS_DEAD, do_at, LVL_AMBASSADOR, 0, 0, 0},
	{"attributes", POS_SLEEPING, do_attributes, 0, 0, 0, 0},
	{"attack", POS_RESTING, do_kill, 0, 0, 0, 0},
	{"attach", POS_RESTING, do_attach, 0, SCMD_ATTACH, 0, 0},
	{"addname", POS_SLEEPING, do_addname, LVL_IMMORT, 0, 0, 0},
	{"addpos", POS_SLEEPING, do_addpos, LVL_IMMORT, 0, 0, 0},
	{"advance", POS_DEAD, do_advance, LVL_IMMORT, 0, 0, 0},
	{"ahem", POS_RESTING, do_action, 0, 0, 0, 0},
	{"alias", POS_DEAD, do_alias, 0, 0, 0, 0},
	{"alignment", POS_DEAD, do_alignment, 0, 0, 0, 0},
	{"alter", POS_SITTING, do_alter, 0, 0, 0, 0},
	{"alterations", POS_SLEEPING, do_skills, 0, SCMD_SPELLS_ONLY, 0, 0},
	{"activate", POS_SITTING, do_activate, 0, SCMD_ON, 0, 0},
	{"accept", POS_RESTING, do_action, 0, 0, 0, 0},
	{"accuse", POS_RESTING, do_action, 0, 0, 0, 0},
	{"ack", POS_RESTING, do_action, 0, 0, 0, 0},
	{"account", POS_RESTING, do_account, LVL_IMMORT, 0, 0, 0},
	{"addict", POS_RESTING, do_action, 0, 0, 0, 0},
	{"adjust", POS_RESTING, do_action, 0, 0, 0, 0},
	{"afw", POS_RESTING, do_action, 0, 0, 0, 0},
	{"agree", POS_RESTING, do_action, 0, 0, 0, 0},
//	{"ambush", POS_STANDING, do_ambush, 0, 0, 0, 0},
	{"analyze", POS_SITTING, do_analyze, 0, 0, 0, 0},
	{"anonymous", POS_DEAD, do_gen_tog, 1, SCMD_ANON, 0, 0},
	{"anticipate", POS_RESTING, do_action, 0, 0, 0, 0},
	{"apologize", POS_RESTING, do_action, 0, 0, 0, 0},
	{"applaud", POS_RESTING, do_action, 0, 0, 0, 0},
	{"approve", POS_DEAD, do_approve, LVL_IMMORT, 0, 0, 0},
	{"unapprove", POS_DEAD, do_unapprove, LVL_IMMORT, 0, 0, 0},
	{"arm", POS_SITTING, do_arm, 1, 0, 0, 0},
	{"assist", POS_FIGHTING, do_assist, 1, 0, 0, 0},
	{"assimilate", POS_RESTING, do_assimilate, 0, 0, 0, 0},
	{"ask", POS_RESTING, do_spec_comm, 0, SCMD_ASK, 0, 0},
	{"auction", POS_SLEEPING, do_gen_comm, 0, SCMD_AUCTION, 0, 0},
	{"autodiagnose", POS_DEAD, do_gen_tog, 1, SCMD_AUTO_DIAGNOSE, 0, 0},
	{"autoexits", POS_DEAD, do_gen_tog, 1, SCMD_AUTOEXIT, 0, 0},
	{"autoloot", POS_DEAD, do_gen_tog, 1, SCMD_AUTOLOOT, 0, 0},
	{"autopage", POS_SLEEPING, do_gen_tog, 0, SCMD_AUTOPAGE, 0, 0},
	{"autoprompt", POS_SLEEPING, do_gen_tog, 0, SCMD_AUTOPROMPT, 0, 0},
	{"autopsy", POS_RESTING, do_autopsy, 0, 0, 0, 0},
	{"autosplit", POS_DEAD, do_gen_tog, 1, SCMD_AUTOSPLIT, 0, 0},
	{"autowrap", POS_DEAD, do_gen_tog, LVL_IMMORT, SCMD_AUTOWRAP, 0, 0},
	{"affects", POS_SLEEPING, do_affects, 0, 0, 0, 0},
	{"afk", POS_SLEEPING, do_gen_tog, 0, SCMD_AFK, 0, 0},

	{"babble", POS_RESTING, do_say, 0, SCMD_BABBLE, 0, 0},
	{"backstab", POS_STANDING, do_backstab, 1, 0, 0, 0},
	{"ban", POS_DEAD, do_ban, LVL_IMMORT, 0, 0, 0},
	{"bandage", POS_STANDING, do_bandage, 0, 0, 0, 0},
	{"balance", POS_STANDING, do_not_here, 1, 0, 0, 0},
	{"banzai", POS_RESTING, do_action, 0, 0, 0, 0},
	{"bark", POS_SITTING, do_action, 0, 0, 0, 0},
	{"bash", POS_FIGHTING, do_bash, 1, 0, 0, 0},
	{"bat", POS_SITTING, do_action, 0, 0, 0, 0},
	{"battlecry", POS_FIGHTING, do_battlecry, 0, SCMD_BATTLE_CRY, 0, 0},
	{"bawl", POS_RESTING, do_action, 0, 0, 0, 0},
	{"bcatch", POS_RESTING, do_action, 0, 0, 0, 0},
	{"beam", POS_RESTING, do_action, 0, 0, 0, 0},
	{"beckon", POS_SITTING, do_action, 0, 0, 0, 0},
	{"beef", POS_RESTING, do_action, 0, 0, 0, 0},
	{"beer", POS_RESTING, do_action, 0, 0, 0, 0},
	{"beg", POS_RESTING, do_action, 0, 0, 0, 0},
	{"beguile", POS_STANDING, do_beguile, 0, 0, 0, 0},
	{"berserk", POS_FIGHTING, do_berserk, 0, 0, 0, 0},
	{"bellow", POS_RESTING, do_say, 0, SCMD_BELLOW, 0, 0},
	{"belly", POS_RESTING, do_action, 0, 0, 0, 0},
	{"bearhug", POS_FIGHTING, do_offensive_skill, 0, SKILL_BEARHUG, 0, 0},
	{"behead", POS_FIGHTING, do_offensive_skill, 0, SKILL_BEHEAD, 0, 0},
	{"beeper", POS_SLEEPING, do_gen_tog, 0, SCMD_AUTOPAGE, 0, 0},
	{"bell", POS_SLEEPING, do_gen_tog, 0, SCMD_AUTOPAGE, 0, 0},
	{"belch", POS_SLEEPING, do_action, 0, 0, 0, 0},
	{"bioscan", POS_SITTING, do_bioscan, 0, 0, 0, 0},
	{"bird", POS_RESTING, do_action, 0, 0, 0, 0},
	{"bite", POS_FIGHTING, do_offensive_skill, 0, SKILL_BITE, 0, 0},
	{"bkiss", POS_RESTING, do_action, 0, 0, 0, 0},
	{"blame", POS_RESTING, do_action, 0, 0, 0, 0},
	{"bleed", POS_RESTING, do_action, 0, 0, 0, 0},
	{"blush", POS_RESTING, do_action, 0, 0, 0, 0},
	{"blink", POS_RESTING, do_action, 0, 0, 0, 0},
	{"blow", POS_RESTING, do_action, 0, 0, 0, 0},
	{"board", POS_STANDING, do_board, 0, 0, 0, 0},
	{"bodyslam", POS_FIGHTING, do_offensive_skill, 0, SKILL_BODYSLAM, 0, 0},
	{"bored", POS_RESTING, do_action, 0, 0, 0, 0},
	{"bow", POS_STANDING, do_action, 0, 0, 0, 0},
	{"boggle", POS_RESTING, do_action, 0, 0, 0, 0},
	{"bomb", POS_SLEEPING, do_bomb, LVL_IMMORT, 0, 0, 0},
	{"bonk", POS_RESTING, do_action, 0, 0, 0, 0},
	{"bones", POS_RESTING, do_action, 0, 0, 0, 0},
	{"booger", POS_RESTING, do_action, 0, 0, 0, 0},
	{"bottle", POS_RESTING, do_action, 0, 0, 0, 0},
	{"bounce", POS_SITTING, do_action, 0, 0, 0, 0},
	{"brag", POS_RESTING, do_action, 0, 0, 0, 0},
	{"brb", POS_RESTING, do_action, 0, 0, 0, 0},
	{"breathe", POS_SITTING, do_breathe, 0, 0, 0, 0},
	{"brief", POS_DEAD, do_gen_tog, 0, SCMD_BRIEF, 0, 0},
	{"buff", POS_DEAD, do_action, 0, 0, 0, 0},
	{"burp", POS_RESTING, do_action, 0, 0, 0, 0},
	{"burn", POS_RESTING, do_action, 0, 0, 0, 0},
	{"buy", POS_STANDING, do_not_here, 0, 0, 0, 0},
	{"bug", POS_DEAD, do_gen_write, 0, SCMD_BUG, 0, 0},

	{"cast", POS_SITTING, do_cast, 1, 0, 0, 0},
	{"cash", POS_DEAD, do_cash, 0, 0, 0, 0},
	{"cackle", POS_RESTING, do_action, 0, 0, 0, 0},
	{"camel", POS_RESTING, do_action, 0, 0, 0, 0},
	{"catnap", POS_RESTING, do_action, 0, 0, 0, 0},
	{"chat", POS_SLEEPING, do_gen_comm, 0, SCMD_GOSSIP, 0, 0},
	{"charge", POS_STANDING, do_charge, 0, 0, 0, 0},
	{"check", POS_STANDING, do_not_here, 1, 0, 0, 0},
	{"cheer", POS_RESTING, do_action, 0, 0, 0, 0},
	{"choke", POS_FIGHTING, do_offensive_skill, 0, SKILL_CHOKE, 0, 0},
	{"clansay", POS_MORTALLYW, do_gen_comm, LVL_CAN_CLAN, SCMD_CLANSAY, 0, 0},
	{"csay", POS_MORTALLYW, do_gen_comm, LVL_CAN_CLAN, SCMD_CLANSAY, 0, 0},
	{"crossface", POS_FIGHTING, do_crossface, 1, 0, 0, 0},
	{"clanpasswd", POS_RESTING, do_clanpasswd, LVL_CAN_CLAN, 0, 0, 0},
	{"clanemote", POS_STUNNED, do_gen_comm, LVL_CAN_CLAN, SCMD_CLANEMOTE, 0, 0},
	{"clanhide", POS_STUNNED, do_gen_tog, LVL_CAN_CLAN, SCMD_CLAN_HIDE, 0, 0},
	{"cleave", POS_FIGHTING, do_cleave, 1, 0, 0, 0},
	{"clue", POS_RESTING, do_action, 0, 0, 0, 0},
	{"clueless", POS_RESTING, do_action, 0, 0, 0, 0},
	{"ceasefire", POS_RESTING, do_ceasefire, 0, 0, 0, 0},
	{"cemote", POS_STUNNED, do_gen_comm, LVL_CAN_CLAN, SCMD_CLANEMOTE, 0, 0},
	{"claninfo", POS_SLEEPING, do_cinfo, LVL_CAN_CLAN, 0, 0, 0},
	{"cinfo", POS_SLEEPING, do_cinfo, LVL_CAN_CLAN, 0, 0, 0},
	{"cedit", POS_DEAD, do_cedit, LVL_IMMORT, 0, 0, 0},
	{"circle", POS_FIGHTING, do_circle, 0, 0, 0, 0},
	{"cities", POS_SLEEPING, do_hcollect_help, 0, SCMD_CITIES, 0, 0},
	{"clanlist", POS_MORTALLYW, do_clanlist, LVL_CAN_CLAN, 0, 0, 0},
	{"claw", POS_FIGHTING, do_offensive_skill, 0, SKILL_CLAW, 0, 0},
	{"climb", POS_RESTING, do_action, 0, 0, 0, 0},
	{"cling", POS_RESTING, do_action, 0, 0, 0, 0},
	{"clothesline", POS_FIGHTING, do_offensive_skill, 0, SKILL_CLOTHESLINE, 0, 0},
	{"chuckle", POS_RESTING, do_action, 0, 0, 0, 0},
	{"clap", POS_RESTING, do_action, 0, 0, 0, 0},
	{"clean", POS_RESTING, do_clean, 0, 0, 0, 0},
	{"clear", POS_DEAD, do_gen_ps, 0, SCMD_CLEAR, 0, 0},
	{"close", POS_SITTING, do_gen_door, 0, SCMD_CLOSE, 0, 0},
	{"cls", POS_DEAD, do_gen_ps, 0, SCMD_CLEAR, 0, 0},
	{"color", POS_DEAD, do_color, 0, 0, 0, 0},
	{"collapse", POS_RESTING, do_action, 0, 0, 0, 0},
	{"comfort", POS_RESTING, do_action, 0, 0, 0, 0},
	{"comb", POS_RESTING, do_action, 0, 0, 0, 0},
	{"combo", POS_FIGHTING, do_combo, 0, 0, 0, 0},
	{"commands", POS_DEAD, do_commands, 0, SCMD_COMMANDS, 0, 0},
	{"compact", POS_DEAD, do_gen_tog, 0, SCMD_COMPACT, 0, 0},
	{"compare", POS_RESTING, do_compare, 0, 0, 0, 0},
	{"consider", POS_RESTING, do_consider, 0, 0, 0, 0},
	{"conceal", POS_RESTING, do_conceal, 0, 0, 0, 0},
	{"concerned", POS_RESTING, do_action, 0, 0, 0, 0},
	{"confused", POS_RESTING, do_action, 0, 0, 0, 0},
	{"conspire", POS_RESTING, do_action, 0, 0, 0, 0},
	{"contemplate", POS_RESTING, do_action, 0, 0, 0, 0},
	{"convert", POS_RESTING, do_convert, 0, 0, 0, 0},
	{"corner", POS_FIGHTING, do_corner, 0, 0, 0, 0},
	{"cough", POS_RESTING, do_action, 0, 0, 0, 0},
	{"council", POS_DEAD, do_wizutil, LVL_IMMORT, SCMD_COUNCIL, 0, 0},
	{"cover", POS_RESTING, do_action, 0, 0, 0, 0},
	{"cower", POS_RESTING, do_action, 0, 0, 0, 0},
	{"crack", POS_RESTING, do_action, 0, 0, 0, 0},
	{"cramp", POS_RESTING, do_action, 0, 0, 0, 0},
	{"crawl", POS_RESTING, do_move, 0, SCMD_CRAWL, 0, 0},
	{"cranekick", POS_FIGHTING, do_offensive_skill, 0, SKILL_CRANE_KICK, 0, 0},
	{"crank", POS_RESTING, do_action, 0, 0, 0, 0},
	{"credits", POS_DEAD, do_gen_ps, 0, SCMD_CREDITS, 0, 0},
	{"cringe", POS_RESTING, do_action, 0, 0, 0, 0},
	{"criticize", POS_RESTING, do_action, 0, 0, 0, 0},
	{"crush", POS_RESTING, do_action, 0, 0, 0, 0},
	{"cry", POS_RESTING, do_action, 0, 0, 0, 0},
	{"cry_from_beyond", POS_FIGHTING, do_battlecry, 1, SCMD_CRY_FROM_BEYOND,
			0, 0},
	{"cuddle", POS_RESTING, do_action, 0, 0, 0, 0},
	{"curious", POS_RESTING, do_action, 0, 0, 0, 0},
	{"curse", POS_RESTING, do_action, 0, 0, 0, 0},
	{"curtsey", POS_STANDING, do_action, 0, 0, 0, 0},
	{"cyberscan", POS_STANDING, do_cyberscan, 0, 0, 0, 0},

	{"dance", POS_STANDING, do_action, 0, 0, 0, 0},
	{"dare", POS_RESTING, do_action, 0, 0, 0, 0},
	{"date", POS_DEAD, do_date, LVL_AMBASSADOR, SCMD_DATE, 0, 0},
	{"daydream", POS_SLEEPING, do_action, 0, 0, 0, 0},
	{"dc", POS_DEAD, do_dc, LVL_IMMORT, 0, 0, 0},
	{"deactivate", POS_SITTING, do_activate, 0, SCMD_OFF, 0, 0},
	{"deassimilate", POS_RESTING, do_deassimilate, 0, 0, 0, 0},
	{"deathtouch", POS_FIGHTING, do_offensive_skill, 0, SKILL_DEATH_TOUCH, 0, 0},
	{"defuse", POS_SITTING, do_defuse, 0, 0, 0, 0},
	{"delete", POS_DEAD, do_delete, LVL_IMMORT, 0, 0, 0},
	{"demote", POS_RESTING, do_demote, LVL_CAN_CLAN, 0, 0, 0},
	{"deposit", POS_STANDING, do_not_here, 1, 0, 0, 0},
	{"detach", POS_RESTING, do_attach, 0, SCMD_DETACH, 0, 0},
	{"debug", POS_DEAD, do_gen_tog, LVL_IMMORT, SCMD_DEBUG, 0, 0},
	{"de-energize", POS_FIGHTING, do_de_energize, 0, 0, 0, 0},
	{"diagnose", POS_RESTING, do_diagnose, 0, 0, 0, 0},
	{"disrespect", POS_RESTING, do_action, 0, 0, 0, 0},
	{"disguise", POS_RESTING, do_disguise, 0, 0, 0, 0},
	{"disbelieve", POS_RESTING, do_action, 0, 0, 0, 0},
	{"disgusted", POS_RESTING, do_action, 0, 0, 0, 0},
	{"distance", POS_DEAD, do_distance, LVL_IMMORT, 0, 0, 0},
	{"discharge", POS_FIGHTING, do_discharge, 0, 0, 0, 0},
	{"disco", POS_STANDING, do_action, 0, 0, 0, 0},
	{"dismount", POS_MOUNTED, do_dismount, 0, 0, 0, 0},
	{"dismiss", POS_RESTING, do_dismiss, 0, 0, 0, 0},
	{"disarm", POS_FIGHTING, do_disarm, 0, 0, 0, 0},
	{"display", POS_DEAD, do_display, 0, 0, 0, 0},
	{"disembark", POS_STANDING, do_disembark, 0, 0, 0, 0},
	{"dizzy", POS_RESTING, do_action, 0, 0, 0, 0},
	{"doctor", POS_RESTING, do_action, 0, 0, 0, 0},
	{"dodge", POS_FIGHTING, do_evade, 0, 0, 0, 0},
	{"doh", POS_RESTING, do_action, 0, 0, 0, 0},
	{"donate", POS_RESTING, do_drop, 0, SCMD_DONATE, 0, 0},
	{"drag", POS_STANDING, do_drag, 0, SKILL_DRAG, 0, 0},
	{"dump", POS_RESTING, do_action, 0, 0, 0, 0},
	{"drive", POS_SITTING, do_drive, 0, 0, 0, 0},
	{"drink", POS_RESTING, do_drink, 0, SCMD_DRINK, 0, 0},
	{"dream", POS_DEAD, do_gen_comm, 0, SCMD_DREAM, 0, 0},
	{"drop", POS_RESTING, do_drop, 0, SCMD_DROP, 0, 0},
	{"drool", POS_RESTING, do_action, 0, 0, 0, 0},
	{"drain", POS_FIGHTING, do_drain, 10, 0, 0, 0},
	{"duck", POS_RESTING, do_action, 0, 0, 0, 0},
	{"dynedit", POS_DEAD, do_dynedit, LVL_IMMORT, 0, 0, 0},


	{"eat", POS_RESTING, do_eat, 0, SCMD_EAT, 0, 0},
	{"echo", POS_SLEEPING, do_echo, LVL_IMMORT, SCMD_ECHO, 0, 0},
	{"econvert", POS_RESTING, do_econvert, 0, 0, 0, 0},
	{"elbow", POS_FIGHTING, do_offensive_skill, 0, SKILL_ELBOW, 0, 0},
	{"elude", POS_STANDING, do_elude, 0, 0, 0, 0},
	{"emote", POS_SLEEPING, do_echo, 1, SCMD_EMOTE, 0, 0},
	{":", POS_SLEEPING, do_echo, 1, SCMD_EMOTE, 0, 0},
	{"embrace", POS_STANDING, do_action, 0, 0, 0, 0},
	{"empower", POS_STANDING, do_empower, 0, 0, 0, 0},
	{"empty", POS_STANDING, do_empty, 0, 0, 0, 0},
	{"enter", POS_STANDING, do_enter, 0, 0, 0, 0},
	{"encumbrance", POS_SLEEPING, do_encumbrance, 0, 0, 0, 0},
	{"enroll", POS_RESTING, do_enroll, 0, 0, 0, 0},
	{"envy", POS_RESTING, do_action, 0, 0, 0, 0},
	{"equipment", POS_SLEEPING, do_equipment, 0, SCMD_EQ, 0, 0},
	{"evade", POS_FIGHTING, do_evade, 0, 0, 0, 0},
	{"evaluate", POS_RESTING, do_evaluate, 0, 0, 0, 0},
	{"exit", POS_RESTING, do_exits, 0, 0, 0, 0},
	{"exits", POS_RESTING, do_exits, 0, 0, 0, 0},
	{"examine", POS_RESTING, do_examine, 0, 0, 0, 0},
	{"exchange", POS_SITTING, do_not_here, 0, 0, 0, 0},
	{"experience", POS_DEAD, do_experience, 0, 0, 0, 0},
	{"explain", POS_RESTING, do_action, 0, 0, 0, 0},
	{"extinguish", POS_RESTING, do_extinguish, 0, 0, 0, 0},
	{"expostulate", POS_RESTING, do_say, 0, SCMD_EXPOSTULATE, 0, 0},
	{"explode", POS_RESTING, do_action, 0, 0, 0, 0},
	{"extract", POS_SITTING, do_extract, 5, 0, 0, 0},
	{"force", POS_SLEEPING, do_force, LVL_IMMORT, 0, 0, 0},
	{"fart", POS_RESTING, do_action, 0, 0, 0, 0},
	{"faint", POS_RESTING, do_action, 0, 0, 0, 0},
	{"fakerep", POS_RESTING, do_action, 0, 0, 0, 0},
	{"fatality", POS_RESTING, do_action, 0, 0, 0, 0},
	{"fidget", POS_RESTING, do_action, 0, 0, 0, 0},
	{"fill", POS_STANDING, do_pour, 0, SCMD_FILL, 0, 0},
	{"firstaid", POS_STANDING, do_firstaid, 0, 0, 0, 0},
	{"five", POS_STANDING, do_action, 0, 0, 0, 0},
	{"feed", POS_RESTING, do_feed, 0, 0, 0, 0},
	{"feign", POS_FIGHTING, do_feign, 0, 0, 0, 0},
	{"flare", POS_RESTING, do_action, 0, 0, 0, 0},
	{"flash", POS_STANDING, do_action, 0, 0, 0, 0},
	{"flee", POS_FIGHTING, do_flee, 0, 0, 0, 0},
	{"fly", POS_STANDING, do_fly, 0, 0, 0, 0},
	{"flex", POS_RESTING, do_action, 0, 0, 0, 0},
	{"flinch", POS_RESTING, do_action, 0, 0, 0, 0},
	{"flip", POS_STANDING, do_action, 0, 0, 0, 0},
	{"flirt", POS_RESTING, do_action, 0, 0, 0, 0},
	{"flowers", POS_RESTING, do_action, 0, 0, 0, 0},
	{"flutter", POS_RESTING, do_action, 0, 0, 0, 0},
	{"follow", POS_RESTING, do_follow, 0, 0, 0, 0},
	{"foam", POS_SLEEPING, do_action, 0, 0, 0, 0},
	{"fondle", POS_RESTING, do_action, 0, 0, 0, 0},
	{"freak", POS_RESTING, do_action, 0, 0, 0, 0},
	{"freeze", POS_DEAD, do_wizutil, LVL_IMMORT, SCMD_FREEZE, 0, 0},
	{"french", POS_RESTING, do_action, 0, 0, 0, 0},
	{"frown", POS_RESTING, do_action, 0, 0, 0, 0},
	{"frustrated", POS_RESTING, do_action, 0, 0, 0, 0},
	{"fume", POS_RESTING, do_action, 0, 0, 0, 0},

	{"get", POS_RESTING, do_get, 0, 0, 0, 0},
	{"gagmiss", POS_SLEEPING, do_gen_tog, 0, SCMD_GAGMISS, 0, 0},
	{"gack", POS_RESTING, do_action, 0, 0, 0, 0},
	{"gain", POS_STANDING, do_not_here, 0, 0, 0, 0},
	{"gasp", POS_RESTING, do_action, 0, 0, 0, 0},
	{"gasify", POS_RESTING, do_gasify, 50, 0, 0, 0},
	{"gecho", POS_DEAD, do_gecho, LVL_IMMORT, 0, 0, 0},
	{"ghug", POS_RESTING, do_action, 0, 0, 0, 0},
	{"give", POS_RESTING, do_give, 0, 0, 0, 0},
	{"giggle", POS_RESTING, do_action, 0, 0, 0, 0},
	{"glance", POS_RESTING, do_glance, 0, 0, 0, 0},
	{"glare", POS_RESTING, do_action, 0, 0, 0, 0},
	{"glide", POS_RESTING, do_action, 0, 0, 0, 0},
	{"gloat", POS_RESTING, do_action, 0, 0, 0, 0},
	{"goto", POS_SLEEPING, do_goto, LVL_AMBASSADOR, 0, 0, 0},
	{"gold", POS_SLEEPING, do_gold, 0, 0, 0, 0},
	{"gore", POS_STANDING, do_action, 0, 0, 0, 0},
	{"gossip", POS_SLEEPING, do_gen_comm, 0, SCMD_GOSSIP, 0, 0},
	{"goose", POS_RESTING, do_action, 0, 0, 0, 0},
	{"gouge", POS_FIGHTING, do_offensive_skill, 0, SKILL_GOUGE, 0, 0},
	{"group", POS_SLEEPING, do_group, 1, 0, 0, 0},
	{"grunt", POS_RESTING, do_action, 0, 0, 0, 0},
	{"grab", POS_RESTING, do_grab, 0, 0, 0, 0},
	{"grats", POS_SLEEPING, do_gen_comm, 0, SCMD_GRATZ, 0, 0},
	{"greet", POS_RESTING, do_action, 0, 0, 0, 0},
	{"grimace", POS_RESTING, do_action, 0, 0, 0, 0},
	{"grin", POS_RESTING, do_action, 0, 0, 0, 0},
	{"groan", POS_RESTING, do_action, 0, 0, 0, 0},
	{"groinkick", POS_FIGHTING, do_offensive_skill, 0, SKILL_GROINKICK, 0, 0},
	{"grope", POS_RESTING, do_action, 0, 0, 0, 0},
	{"grovel", POS_RESTING, do_action, 0, 0, 0, 0},
	{"growl", POS_RESTING, do_action, 0, 0, 0, 0},
	{"grumble", POS_RESTING, do_action, 0, 0, 0, 0},
	{"gsay", POS_SLEEPING, do_gsay, 0, 0, 0, 0},
	{"gtell", POS_SLEEPING, do_gsay, 0, 0, 0, 0},
	{"guildsay", POS_MORTALLYW, do_gen_comm, 0, SCMD_GUILDSAY, 0, 0},
	{"guilddonate", POS_RESTING, do_drop, 0, SCMD_GUILD_DONATE, 0, 0},
	{"gunset", POS_RESTING, do_gunset, 0, 0, 0, 0},

	//    { "help"     , POS_DEAD    , do_help     , 0, 0 , 0 , 0},
	{"help", POS_DEAD, do_hcollect_help, 0, 0, 0, 0},
	//    { "?"        , POS_DEAD    , do_help     , 0, 0 , 0 , 0},
	{"?", POS_DEAD, do_hcollect_help, 0, 0, 0, 0},
	{"hedit", POS_RESTING, do_hedit, 1, 0, 0, 0},
	{"hack", POS_STANDING, do_gen_door, 1, SCMD_HACK, 0, 0},
	{"halt", POS_DEAD, do_gen_tog, LVL_AMBASSADOR, SCMD_HALT, 0, 0},
	{"handbook", POS_DEAD, do_hcollect_help, LVL_AMBASSADOR, SCMD_HANDBOOK, 0, 0},
	{"handshake", POS_RESTING, do_action, 0, 0, 0, 0},
	{"hangover", POS_RESTING, do_action, 0, 0, 0, 0},
	{"happy", POS_RESTING, do_action, 0, 0, 0, 0},
	{"hcontrol", POS_DEAD, do_hcontrol, LVL_IMMORT, 0, 0, 0},
	{"head", POS_RESTING, do_action, 0, 0, 0, 0},
	{"headbutt", POS_FIGHTING, do_offensive_skill, 0, SKILL_HEADBUTT, 0, 0},
	{"headlights", POS_RESTING, do_not_here, 0, 0, 0, 0},
	{"heineken", POS_RESTING, do_action, 0, 0, 0, 0},
	{"hiccup", POS_RESTING, do_action, 0, 0, 0, 0},
	{"hide", POS_RESTING, do_hide, 1, 0, 0, 0},
	{"hiss", POS_RESTING, do_action, 0, 0, 0, 0},
	{"hit", POS_FIGHTING, do_hit, 0, SCMD_HIT, 0, 0},
	{"hiptoss", POS_FIGHTING, do_offensive_skill, 0, SKILL_HIP_TOSS, 0, 0},
	{"hamstring", POS_STANDING, do_hamstring, 0, SKILL_HAMSTRING, 0, 0},
	{"hmmm", POS_RESTING, do_action, 0, 0, 0, 0},
	{"hold", POS_RESTING, do_grab, 1, 0, 0, 0},
	{"holler", POS_RESTING, do_gen_comm, 1, SCMD_HOLLER, 0, 0},
	{"holylight", POS_DEAD, do_gen_tog, LVL_AMBASSADOR, SCMD_HOLYLIGHT, 0, 0},
	{"honk", POS_RESTING, do_not_here, 0, 0, 0, 0},
	{"hook", POS_FIGHTING, do_offensive_skill, 0, SKILL_HOOK, 0, 0},
	{"hop", POS_RESTING, do_action, 0, 0, 0, 0},
	{"howl", POS_RESTING, do_action, 0, 0, 0, 0},
	{"house", POS_RESTING, do_house, 0, 0, 0, 0},
	{"holytouch", POS_STANDING, do_holytouch, 0, 0, 0, 0},
	{"hotwire", POS_SITTING, do_hotwire, 0, 0, 0, 0},
	{"hug", POS_RESTING, do_action, 0, 0, 0, 0},
	{"hum", POS_RESTING, do_action, 0, 0, 0, 0},
	{"hump", POS_SITTING, do_action, 0, 0, 0, 0},
	{"hungry", POS_RESTING, do_action, 0, 0, 0, 0},
	{"hurl", POS_RESTING, do_action, 0, 0, 0, 0},
	{"hush", POS_RESTING, do_action, 0, 0, 0, 0},

	{"inventory", POS_DEAD, do_inventory, 0, 0, 0, 0},
	{"inews", POS_SLEEPING, do_dyntext_show, LVL_AMBASSADOR,
			SCMD_DYNTEXT_INEWS, 0, 0},
	{"increase", POS_STANDING, do_not_here, 0, 0, 0, 0},
	{"infiltrate", POS_STANDING, do_infiltrate, 1, 0, 0, 0},
	{"inject", POS_RESTING, do_use, 0, SCMD_INJECT, 0, 0},
	{"ignite", POS_RESTING, do_ignite, 0, 0, 0, 0},
	{"install", POS_DEAD, do_install, LVL_IMMORT, 0, 0, 0},
	{"intimidate", POS_STANDING, do_intimidate, 0, SKILL_INTIMIDATE, 0, 0},
	{"intone", POS_RESTING, do_say, 0, SCMD_INTONE, 0, 0},
	{"introduce", POS_RESTING, do_action, 0, 0, 0, 0},
	{"id", POS_DEAD, do_action, 0, 0, 0, 0},
	{"idea", POS_DEAD, do_gen_write, 0, SCMD_IDEA, 0, 0},
	{"imbibe", POS_RESTING, do_drink, 0, SCMD_DRINK, 0, 0},
	{"immchat", POS_DEAD, do_wiznet, LVL_AMBASSADOR, SCMD_IMMCHAT, 0, 0},
	{"imotd", POS_DEAD, do_gen_ps, LVL_AMBASSADOR, SCMD_IMOTD, 0, 0},
	{"immlist", POS_DEAD, do_wizlist, 0, SCMD_IMMLIST, 0, 0},
	{"immortals", POS_DEAD, do_gen_ps, 0, SCMD_IMMLIST, 0, 0},
	{"immhelp", POS_DEAD, do_immhelp, LVL_IMMORT, 0, 0, 0},
	{"impale", POS_FIGHTING, do_impale, 0, 0, 0, 0},
	{"implants", POS_SLEEPING, do_equipment, 0, SCMD_IMPLANTS, 0, 0},
	{"improve", POS_STANDING, do_improve, 0, 0, 0, 0},
	{"info", POS_SLEEPING, do_gen_ps, 0, SCMD_INFO, 0, 0},
	{"innocent", POS_RESTING, do_action, 0, 0, 0, 0},
	{"insane", POS_RESTING, do_action, 0, 0, 0, 0},
	{"insert", POS_RESTING, do_insert, 5, 0, 0, 0},
	{"insult", POS_RESTING, do_insult, 0, 0, 0, 0},
	{"invis", POS_DEAD, do_invis, 0, 0, 0, 0},
	{"irritate", POS_RESTING, do_action, 0, 0, 0, 0},

	{"jab", POS_FIGHTING, do_offensive_skill, 0, SKILL_JAB, 0, 0},
	{"jet_stream", POS_DEAD, do_gen_tog, LVL_CREATOR, SCMD_JET_STREAM, 0, 0},
	{"joint", POS_RESTING, do_action, 0, 0, 0, 0},
	{"judge", POS_RESTING, do_action, 0, 0, 0, 0},
	{"jump", POS_STANDING, do_move, 0, SCMD_JUMP, 0, 0},
	{"junk", POS_RESTING, do_drop, 0, SCMD_JUNK, 0, 0},

	{"kill", POS_FIGHTING, do_kill, 0, 0, 0, 0},
	{"kia", POS_FIGHTING, do_battlecry, 1, SCMD_KIA, 0, 0},
	{"kata", POS_STANDING, do_kata, 0, 0, 0, 0},
	{"kiss", POS_RESTING, do_action, 0, 0, 0, 0},
	{"kick", POS_FIGHTING, do_offensive_skill, 1, SKILL_KICK, 0, 0},
	{"kneethrust", POS_FIGHTING, do_offensive_skill, 0, SKILL_KNEE, 0, 0},
	{"kneel", POS_SITTING, do_action, 0, 0, 0, 0},
	{"knock", POS_SITTING, do_knock, 0, 0, 0, 0},

	{"look", POS_RESTING, do_look, 0, SCMD_LOOK, 0, 0},
	{"load", POS_RESTING, do_load, 0, SCMD_LOAD, 0, 0},
	{"loadroom", POS_SLEEPING, do_loadroom, 0, 0, 0, 0},
	{"logall", POS_RESTING, do_gen_tog, LVL_IMMORT, SCMD_LOGALL, 0, 0},
	{"login", POS_RESTING, do_not_here, 0, 0, 0, 0},
	{"logdeaths", POS_RESTING, do_gen_tog, LVL_IMMORT, SCMD_DEATHLOG, 0, 0},
	{"laces", POS_RESTING, do_action, 0, 0, 0, 0},
	{"lag", POS_RESTING, do_action, 0, 0, 0, 0},
	{"laugh", POS_RESTING, do_action, 0, 0, 0, 0},
	{"lash", POS_STANDING, do_action, 0, 0, 0, 0},
	{"last", POS_DEAD, do_last, LVL_IMMORT, 0, 0, 0},
	{"leave", POS_STANDING, do_leave, 0, 0, 0, 0},
	{"learn", POS_STANDING, do_practice, 1, 0, 0, 0},
	{"lecture", POS_STANDING, do_lecture, 0, 0, 0, 0},
	{"leer", POS_RESTING, do_action, 0, 0, 0, 0},
	{"levels", POS_DEAD, do_levels, 0, 0, 0, 0},
	{"levitate", POS_RESTING, do_fly, 0, 0, 0, 0},
	{"light", POS_RESTING, do_light, 0, 0, 0, 0},
	{"list", POS_STANDING, do_not_here, 0, 0, 0, 0},
	{"listen", POS_RESTING, do_listen, 0, 0, 0, 0},
	{"lick", POS_RESTING, do_action, 0, 0, 0, 0},
	{"life", POS_RESTING, do_action, 0, 0, 0, 0},
	{"lightbulb", POS_RESTING, do_action, 0, 0, 0, 0},
	{"liver", POS_RESTING, do_action, 0, 0, 0, 0},
	{"lock", POS_SITTING, do_gen_door, 0, SCMD_LOCK, 0, 0},
	{"love", POS_RESTING, do_action, 0, 0, 0, 0},
	{"lungepunch", POS_FIGHTING, do_offensive_skill, 0, SKILL_LUNGE_PUNCH, 0, 0},
	{"lust", POS_RESTING, do_action, 0, 0, 0, 0},

	{"more", POS_DEAD, do_show_more, 0, 0, 0, 0},
	{"makeout", POS_RESTING, do_action, 0, 0, 0, 0},
	{"makemount", POS_STANDING, do_makemount, LVL_IMMORT, 0, 0, 0},
	{"map", POS_SLEEPING, do_map, LVL_IMMORT, 0, 0, 0},
	{"mload", POS_SLEEPING, do_mload, LVL_IMMORT, 0, 0, 0},
	{"mail", POS_STANDING, do_not_here, 1, 0, 0, 0},
	{"marvel", POS_RESTING, do_action, 0, 0, 0, 0},
	{"marvelous", POS_RESTING, do_action, 0, 0, 0, 0},
	{"massage", POS_RESTING, do_action, 0, 0, 0, 0},
	{"medic", POS_STANDING, do_medic, 0, 0, 0, 0},
	{"meditate", POS_SITTING, do_meditate, 0, 0, 0, 0},
	{"mischievous", POS_RESTING, do_action, 0, 0, 0, 0},
	{"miss", POS_RESTING, do_action, 0, 0, 0, 0},
	{"mount", POS_STANDING, do_mount, 0, 0, 0, 0},
	{"moan", POS_RESTING, do_action, 0, 0, 0, 0},
	{"modrian", POS_DEAD, do_hcollect_help, 0, SCMD_MODRIAN, 0, 0},
	{"mosh", POS_FIGHTING, do_action, 0, 0, 0, 0},
	{"motd", POS_DEAD, do_gen_ps, 0, SCMD_MOTD, 0, 0},
	{"moo", POS_RESTING, do_action, 0, 0, 0, 0},
	{"moods", POS_SITTING, do_commands, 0, SCMD_MOODS, 0, 0},
	{"moon", POS_SITTING, do_action, 0, 0, 0, 0},
	{"mortalize", POS_SLEEPING, do_gen_tog, LVL_AMBASSADOR, SCMD_MORTALIZE, 0, 0},
	{"move", POS_STANDING, do_move, 0, SCMD_MOVE, 0, 0},
	{"mshield", POS_DEAD, do_mshield, 0, 0, 0, 0},
	{"mudwipe", POS_DEAD, do_mudwipe, LVL_ENTITY, 0, 0, 0},
	{"muhah", POS_RESTING, do_action, 0, 0, 0, 0},
	{"mull", POS_RESTING, do_action, 0, 0, 0, 0},
	{"mumble", POS_RESTING, do_action, 0, 0, 0, 0},
	{"murmur", POS_RESTING, do_say, 0, SCMD_MURMUR, 0, 0},
	{"music", POS_DEAD, do_gen_comm, 0, SCMD_MUSIC, 0, 0},
	{"mute", POS_DEAD, do_wizutil, LVL_IMMORT, SCMD_SQUELCH, 0, 0},
	{"mutter", POS_RESTING, do_action, 0, 0, 0, 0},
	{"murder", POS_FIGHTING, do_hit, 0, SCMD_MURDER, 0, 0},
	{"news", POS_SLEEPING, do_dyntext_show, 0, SCMD_DYNTEXT_NEWS, 0, 0},
	{"nervepinch", POS_FIGHTING, do_pinch, 0, 0, 0, 0},
	{"newbie", POS_SLEEPING, do_gen_comm, 0, SCMD_NEWBIE, 0, 0},
	{"nibble", POS_RESTING, do_action, 0, 0, 0, 0},
	{"nod", POS_RESTING, do_action, 0, 0, 0, 0},
	{"nodream", POS_DEAD, do_gen_tog, 0, SCMD_NODREAM, 0, 0},
	{"noauction", POS_DEAD, do_gen_tog, 0, SCMD_NOAUCTION, 0, 0},
	{"noaffects", POS_DEAD, do_gen_tog, 0, SCMD_NOAFFECTS, 0, 0},
	{"noclansay", POS_DEAD, do_gen_tog, 0, SCMD_NOCLANSAY, 0, 0},
	{"nogecho", POS_DEAD, do_gen_tog, LVL_DEMI, SCMD_NOGECHO, 0, 0},
	{"nogossip", POS_DEAD, do_gen_tog, 0, SCMD_NOGOSSIP, 0, 0},
	{"nograts", POS_DEAD, do_gen_tog, 0, SCMD_NOGRATZ, 0, 0},
	{"noguildsay", POS_DEAD, do_gen_tog, 0, SCMD_NOGUILDSAY, 0, 0},
	{"nohassle", POS_DEAD, do_gen_tog, LVL_AMBASSADOR, SCMD_NOHASSLE, 0, 0},
	{"noholler", POS_DEAD, do_gen_tog, LVL_IMMORT, SCMD_NOHOLLER, 0, 0},
	{"noimmchat", POS_DEAD, do_gen_tog, LVL_AMBASSADOR, SCMD_NOIMMCHAT, 0, 0},
	{"noidentify", POS_DEAD, do_gen_tog, 1, SCMD_NOIDENTIFY, 0, 0},
	{"nomusic", POS_DEAD, do_gen_tog, 0, SCMD_NOMUSIC, 0, 0},
	{"nonewbie", POS_DEAD, do_gen_tog, 0, SCMD_NEWBIE_HELP, 0, 0},
	{"noogie", POS_RESTING, do_action, 0, 0, 0, 0},
	{"nolocate", POS_SLEEPING, do_nolocate, LVL_IMMORT, 0, 0, 0},
	{"noproject", POS_DEAD, do_gen_tog, 1, SCMD_NOPROJECT, 0, 0},
	{"noshout", POS_SLEEPING, do_gen_tog, 1, SCMD_DEAF, 0, 0},
	{"nosing", POS_DEAD, do_gen_tog, 0, SCMD_NOMUSIC, 0, 0},
	{"nospew", POS_DEAD, do_gen_tog, 1, SCMD_NOSPEW, 0, 0},
	{"nosummon", POS_DEAD, do_gen_tog, 1, SCMD_NOSUMMON, 0, 0},
	{"notell", POS_DEAD, do_gen_tog, 1, SCMD_NOTELL, 0, 0},
	{"notitle", POS_DEAD, do_wizutil, LVL_IMMORT, SCMD_NOTITLE, 0, 0},
	{"nopost", POS_DEAD, do_wizutil, LVL_IMMORT, SCMD_NOPOST, 0, 0},
	{"notrailers", POS_DEAD, do_gen_tog, 1, SCMD_NOTRAILERS, 0, 0},
	{"nowho", POS_DEAD, do_gen_tog, LVL_IMMORT, SCMD_NOWHO, 0, 0},
	{"nowiz", POS_DEAD, do_gen_tog, LVL_IMMORT, SCMD_NOWIZ, 0, 0},
	{"nudge", POS_RESTING, do_action, 0, 0, 0, 0},
	{"nuzzle", POS_RESTING, do_action, 0, 0, 0, 0},

	{"order", POS_RESTING, do_order, 1, 0, 0, 0},
	{"oif", POS_RESTING, do_action, 0, 0, 0, 0},
	{"ogg", POS_RESTING, do_action, 0, 0, 0, 0},
	{"olc", POS_DEAD, do_olc, LVL_AMBASSADOR, 0, 0, 0},
	{"olchelp", POS_DEAD, do_olchelp, LVL_AMBASSADOR, 0, 0, 0},
	{"oload", POS_SLEEPING, do_oload, LVL_IMMORT, 0, 0, 0},
	{"offer", POS_STANDING, do_not_here, 1, 0, 0, 0},
	{"open", POS_SITTING, do_gen_door, 0, SCMD_OPEN, 0, 0},
	{"oecho", POS_DEAD, do_oecho, LVL_IMMORT, 0, 0, 0},
	{"oset", POS_DEAD, do_oset, LVL_IMMORT, 0, 0, 0},
	{"overhaul", POS_STANDING, do_overhaul, 1, 0, 0, 0},
	{"overdrain", POS_STANDING, do_overdrain, 1, 0, 0, 0},
	{"objupdate", POS_DEAD, do_objupdate, LVL_GRIMP, 0, 0, 0},

	{"pace", POS_STANDING, do_action, 0, 0, 0, 0},
	{"pack", POS_RESTING, do_put, 0, 0, 0, 0},
	{"palette", POS_DEAD, do_palette, LVL_IMMORT, 0, 0, 0},
	{"palmstrike", POS_FIGHTING, do_offensive_skill, 0, SKILL_PALM_STRIKE, 0, 0},
	{"pant", POS_RESTING, do_action, 0, 0, 0, 0},
	{"pants", POS_SITTING, do_action, 0, 0, 0, 0},
	{"pat", POS_RESTING, do_action, 0, 0, 0, 0},
	{"page", POS_DEAD, do_page, LVL_IMMORT, 0, 0, 0},
	{"pardon", POS_DEAD, do_wizutil, LVL_IMMORT, SCMD_PARDON, 0, 0},
	{"park", POS_SITTING, do_action, 0, 0, 0, 0},
	{"passout", POS_RESTING, do_action, 0, 0, 0, 0},
	{"peace", POS_SLEEPING, do_peace, LVL_IMMORT, 0, 0, 0},
	{"peck", POS_RESTING, do_action, 0, 0, 0, 0},
	{"pelekick", POS_FIGHTING, do_offensive_skill, 0, SKILL_PELE_KICK, 0, 0},
	{"pie", POS_RESTING, do_action, 0, 0, 0, 0},
	{"pkiller", POS_DEAD, do_pkiller, 0, 0, 0, 0},
	{"place", POS_RESTING, do_put, 0, 0, 0, 0},
	{"put", POS_RESTING, do_put, 0, 0, 0, 0},
	{"pull", POS_RESTING, do_action, 0, 0, 0, 0},
	{"puff", POS_RESTING, do_action, 0, 0, 0, 0},
	{"push", POS_RESTING, do_action, 0, 0, 0, 0},
	{"purchase", POS_STANDING, do_not_here, 0, 0, 0, 0},
	{"peer", POS_RESTING, do_action, 0, 0, 0, 0},
	{"pick", POS_STANDING, do_gen_door, 1, SCMD_PICK, 0, 0},
	{"pickup", POS_RESTING, do_action, 0, 0, 0, 0},
	{"piledrive", POS_FIGHTING, do_offensive_skill, 0, SKILL_PILEDRIVE, 0, 0},
	{"pimp", POS_RESTING, do_action, 0, 0, 0, 0},
	{"pimpslap", POS_RESTING, do_action, 0, 0, 0, 0},
	{"pinch", POS_RESTING, do_pinch, 0, 0, 0, 0},
	{"piss", POS_STANDING, do_action, 0, 0, 0, 0},
	{"pissed", POS_RESTING, do_action, 0, 0, 0, 0},
	{"pistolwhip", POS_FIGHTING, do_pistolwhip, 1, 0, 0, 0},
	{"plant", POS_STANDING, do_plant, 0, 0, 0, 0},
	{"plead", POS_RESTING, do_action, 0, 0, 0, 0},
	{"pload", POS_RESTING, do_pload, LVL_IMMORT, 0, 0, 0},
	{"point", POS_RESTING, do_point, 0, 0, 0, 0},
	{"poke", POS_RESTING, do_action, 0, 0, 0, 0},
	{"policy", POS_DEAD, do_hcollect_help, 0, SCMD_POLICIES, 0, 0},
	{"ponder", POS_RESTING, do_action, 0, 0, 0, 0},
	{"poofin", POS_DEAD, do_poofset, LVL_AMBASSADOR, SCMD_POOFIN, 0, 0},
	{"poofout", POS_DEAD, do_poofset, LVL_AMBASSADOR, SCMD_POOFOUT, 0, 0},
	{"pose", POS_RESTING, do_echo, 1, SCMD_EMOTE, 0, 0},
	{"pounce", POS_STANDING, do_action, 0, 0, 0, 0},
	{"pour", POS_STANDING, do_pour, 0, SCMD_POUR, 0, 0},
	{"pout", POS_RESTING, do_action, 0, 0, 0, 0},
	{"poupon", POS_RESTING, do_action, 0, 0, 0, 0},
	{"powertrip", POS_RESTING, do_action, 0, 0, 0, 0},
	{"project", POS_DEAD, do_gen_comm, 0, SCMD_PROJECT, 0, 0},
	{"pretend", POS_RESTING, do_action, 0, 0, 0, 0},
	{"promote", POS_RESTING, do_promote, LVL_CAN_CLAN, 0, 0, 0},
	{"prompt", POS_DEAD, do_display, 0, 0, 0, 0},
	{"propose", POS_RESTING, do_action, 0, 0, 0, 0},
	{"practice", POS_SLEEPING, do_practice, 1, 0, 0, 0},
	{"pray", POS_SITTING, do_action, 0, 0, 0, 0},
	{"psiblast", POS_FIGHTING, do_offensive_skill, 0, SKILL_PSIBLAST, 0, 0},
	{"psidrain", POS_FIGHTING, do_psidrain, 0, 0, 0, 0},
	{"psilocate", POS_STANDING, do_psilocate, 0, 0, 0, 0},
	{"puke", POS_RESTING, do_action, 0, 0, 0, 0},
	{"punch", POS_FIGHTING, do_offensive_skill, 1, SKILL_PUNCH, 0, 0},
	{"punish", POS_RESTING, do_action, 0, 0, 0, 0},
	{"purr", POS_RESTING, do_action, 0, 0, 0, 0},
	{"purge", POS_DEAD, do_purge, LVL_IMMORT, 0, 0, 0},

	{"quaff", POS_RESTING, do_use, 0, SCMD_QUAFF, 0, 0},
	{"qcontrol", POS_DEAD, do_qcontrol, LVL_IMMORT, 0, 0, 0},
	//  { "qecho"    , POS_DEAD    , do_qcomm    , LVL_IMMORT, SCMD_QECHO , 0 , 0},
	{"qecho", POS_DEAD, do_qecho, LVL_AMBASSADOR, 0, 0, 0},
	{"qlog", POS_DEAD, do_qlog, LVL_AMBASSADOR, 0, 0, 0},
	//  { "quest"    , POS_DEAD    , do_gen_tog  , 0, SCMD_QUEST , 0 , 0},
	{"quest", POS_DEAD, do_quest, 0, 0, 0, 0},
	{"quake", POS_RESTING, do_action, 0, 0, 0, 0},
	{"qui", POS_DEAD, do_quit, 0, 0, 0, 0},
	{"quit", POS_DEAD, do_quit, 0, SCMD_QUIT, 0, 0},
	{"qpoints", POS_DEAD, do_qpoints, 0, 0, 0, 0},
	{"qpreload", POS_DEAD, do_qpreload, LVL_IMMORT, 0, 0, 0},
	//  { "qsay"     , POS_SLEEPING , do_qcomm   , 0, SCMD_QSAY , 0 , 0},
	{"qsay", POS_SLEEPING, do_qsay, 0, 0, 0, 0},

	{"raise", POS_RESTING, do_action, 0, 0, 0, 0},
	{"ramble", POS_RESTING, do_say, 0, SCMD_RAMBLE, 0, 0},
	{"reply", POS_SLEEPING, do_reply, 0, 0, 0, 0},
	{"redeem", POS_RESTING, do_not_here, 0, 0, 0, 0},
	{"repair", POS_SITTING, do_repair, 0, 0, 0, 0},
	{"respond", POS_RESTING, do_spec_comm, 0, SCMD_RESPOND, 0, 0},
	{"rest", POS_RESTING, do_rest, 0, 0, 0, 0},
	{"resign", POS_SLEEPING, do_resign, 0, 0, 0, 0},
	{"read", POS_RESTING, do_look, 0, SCMD_READ, 0, 0},
	{"reboot", POS_SLEEPING, do_cyborg_reboot, 0, 0, 0, 0},
	{"rabbitpunch", POS_FIGHTING, do_offensive_skill, 0, SKILL_RABBITPUNCH, 0, 0},
	{"reload", POS_DEAD, do_reboot, LVL_IMMORT, 0, 0, 0},
	{"refill", POS_DEAD, do_refill, 1, 0, 0, 0},
	{"recite", POS_RESTING, do_use, 0, SCMD_RECITE, 0, 0},
	{"receive", POS_STANDING, do_not_here, 1, 0, 0, 0},
	{"recharge", POS_SITTING, do_recharge, 0, 0, 0, 0},
	{"remove", POS_RESTING, do_remove, 0, 0, 0, 0},
	{"remort", POS_STANDING, do_not_here, LVL_IMMORT, 0, 0, 0},
	{"remote", POS_RESTING, do_action, 0, 0, 0, 0},
	{"register", POS_SITTING, do_not_here, 0, 0, 0, 0},
	{"regurgitate", POS_RESTING, do_action, 0, 0, 0, 0},
	{"rename", POS_SLEEPING, do_rename, LVL_IMMORT, 0, 0, 0},
	{"rent", POS_STANDING, do_not_here, 1, 0, 0, 0},
	{"report", POS_SLEEPING, do_report, 0, 0, 0, 0},
	{"reroll", POS_DEAD, do_wizutil, LVL_IMMORT, SCMD_REROLL, 0, 0},
	{"rescue", POS_FIGHTING, do_rescue, 1, 0, 0, 0},
	{"restore", POS_DEAD, do_restore, LVL_IMMORT, 0, 0, 0},
	{"retreat", POS_FIGHTING, do_retreat, 0, 0, 0, 0},
	{"return", POS_DEAD, do_return, 0, SCMD_RETURN, 0, 0},
	{"recorporealize", POS_DEAD, do_return, 0, SCMD_RECORP, 0, 0},
	{"retrieve", POS_RESTING, do_not_here, 0, 0, 0, 0},
	{"rev", POS_SITTING, do_action, 0, 0, 0, 0},
	{"ridgehand", POS_FIGHTING, do_offensive_skill, 0, SKILL_RIDGEHAND, 0, 0},
	{"roar", POS_RESTING, do_action, 0, 0, 0, 0},
	{"rofl", POS_RESTING, do_action, 0, 0, 0, 0},
	{"roll", POS_RESTING, do_roll, 0, 0, 0, 0},
	{"roomflags", POS_DEAD, do_gen_tog, LVL_AMBASSADOR, SCMD_ROOMFLAGS, 0, 0},
	{"rose", POS_RESTING, do_action, 0, 0, 0, 0},
	{"roundhouse", POS_FIGHTING, do_offensive_skill, 0, SKILL_ROUNDHOUSE, 0, 0},
	{"rstat", POS_DEAD, do_rstat, LVL_IMMORT, 0, 0, 0},
	{"rswitch", POS_DEAD, do_rswitch, LVL_IMMORT, 0, 0, 0},
	{"rub", POS_RESTING, do_action, 0, 0, 0, 0},
	{"ruffle", POS_STANDING, do_action, 0, 0, 0, 0},
	{"rules", POS_DEAD, do_hcollect_help, 0, SCMD_POLICIES, 0, 0},
	{"run", POS_STANDING, do_action, 0, 0, 0, 0},

	{"say", POS_RESTING, do_say, 0, SCMD_SAY, 0, 0},
	{"'", POS_RESTING, do_say, 0, SCMD_SAY, 0, 0},
	{">", POS_RESTING, do_say, 0, SCMD_SAY_TO, 0, 0},
	{"sacrifice", POS_RESTING, do_sacrifice, 0, 0, 0, 0},
	{"salute", POS_SITTING, do_action, 0, 0, 0, 0},
	{"save", POS_SLEEPING, do_save, 0, 0, 0, 0},
	{"score", POS_DEAD, do_score, 0, 0, 0, 0},
	{"scan", POS_RESTING, do_scan, 0, 0, 0, 0},
	{"scissorkick", POS_FIGHTING, do_offensive_skill, 0, SKILL_SCISSOR_KICK,
			0, 0},
	{"scold", POS_RESTING, do_action, 0, 0, 0, 0},
	{"scratch", POS_RESTING, do_action, 0, 0, 0, 0},
	{"scream", POS_RESTING, do_action, 0, 0, 0, 0},
	{"screen", POS_DEAD, do_screen, 0, 0, 0, 0},
	{"scuff", POS_RESTING, do_action, 0, 0, 0, 0},
	{"search", POS_DEAD, do_examine, 0, 0, 0, 0},
	{"searchfor", POS_DEAD, do_searchfor, LVL_DEMI, 0, 0, 0},
	{"seduce", POS_RESTING, do_action, 0, 0, 0, 0},
	{"sell", POS_STANDING, do_not_here, 0, 0, 0, 0},
	{"selfdestruct", POS_RESTING, do_self_destruct, 0, 0, 0, 0},
	{"send", POS_SLEEPING, do_send, LVL_IMMORT, 0, 0, 0},
	{"serenade", POS_RESTING, do_action, 0, 0, 0, 0},
	{"set", POS_DEAD, do_set, LVL_IMMORT, 0, 0, 0},
	{"severtell", POS_DEAD, do_severtell, LVL_IMMORT, 0, 0, 0},
	{"shout", POS_RESTING, do_gen_comm, 0, SCMD_SHOUT, 0, 0},
	{"shake", POS_RESTING, do_action, 0, 0, 0, 0},
	{"shin", POS_RESTING, do_action, 0, 0, 0, 0},
	{"shiver", POS_RESTING, do_action, 0, 0, 0, 0},
	{"shudder", POS_RESTING, do_action, 0, 0, 0, 0},
	{"shoulderthrow", POS_FIGHTING, do_offensive_skill, 0,
			SKILL_SHOULDER_THROW, 0, 0},
	{"show", POS_DEAD, do_show, LVL_AMBASSADOR, 0, 0, 0},
	{"shower", POS_RESTING, do_action, 0, 0, 0, 0},
	{"shoot", POS_SITTING, do_shoot, 0, 0, 0, 0},
	{"shrug", POS_RESTING, do_action, 0, 0, 0, 0},
	{"shutdow", POS_DEAD, do_shutdown, LVL_CREATOR, 0, 0, 0},
	{"shutdown", POS_DEAD, do_shutdown, LVL_CREATOR, SCMD_SHUTDOWN, 0, 0},
	{"shutoff", POS_SITTING, do_activate, 0, SCMD_OFF, 0, 0},
	{"sidekick", POS_FIGHTING, do_offensive_skill, 0, SKILL_SIDEKICK, 0, 0},
	{"sigh", POS_RESTING, do_action, 0, 0, 0, 0},
	{"silly", POS_RESTING, do_action, 0, 0, 0, 0},
	{"simon", POS_RESTING, do_action, 0, 0, 0, 0},
	{"sing", POS_RESTING, do_gen_comm, 0, SCMD_MUSIC, 0, 0},
	{"sip", POS_RESTING, do_drink, 0, SCMD_SIP, 0, 0},
	{"sit", POS_RESTING, do_sit, 0, 0, 0, 0},
	{"skank", POS_FIGHTING, do_action, 0, 0, 0, 0},
	{"skills", POS_SLEEPING, do_skills, 0, SCMD_SKILLS_ONLY, 0, 0},
	{"skset", POS_SLEEPING, do_skillset, LVL_IMMORT, 0, 0, 0},
	{"skillset", POS_SLEEPING, do_skillset, LVL_IMMORT, 0, 0, 0},
	{"sleep", POS_SLEEPING, do_sleep, 0, 0, 0, 0},
	{"sleeper", POS_FIGHTING, do_sleeper, 0, 0, 0, 0},
	{"slap", POS_RESTING, do_action, 0, 0, 0, 0},
	{"slist", POS_SLEEPING, do_hcollect_help, 0, SCMD_SKILLS, 0, 0},
	{"sling", POS_FIGHTING, do_not_here, 0, 0, 0, 0},
	{"slowns", POS_DEAD, do_gen_tog, LVL_GRGOD, SCMD_SLOWNS, 0, 0},
	{"slay", POS_RESTING, do_kill, 0, SCMD_SLAY, 0, 0},
	{"smack", POS_SLEEPING, do_action, 0, 0, 0, 0},
	{"smell", POS_RESTING, do_action, 0, 0, 0, 0},
	{"smile", POS_RESTING, do_action, 0, 0, 0, 0},
	{"smirk", POS_RESTING, do_action, 0, 0, 0, 0},
	{"smoke", POS_RESTING, do_smoke, 0, 0, 0, 0},
	{"smush", POS_RESTING, do_action, 0, 0, 0, 0},
	{"snicker", POS_RESTING, do_action, 0, 0, 0, 0},
	{"snap", POS_RESTING, do_action, 0, 0, 0, 0},
	{"snarl", POS_RESTING, do_action, 0, 0, 0, 0},
	{"snatch", POS_FIGHTING, do_snatch, 0, SKILL_SNATCH, 0, 0},
	{"sneer", POS_RESTING, do_action, 0, 0, 0, 0},
	{"sneeze", POS_RESTING, do_action, 0, 0, 0, 0},
	{"sneak", POS_STANDING, do_sneak, 1, 0, 0, 0},
	{"sniff", POS_RESTING, do_action, 0, 0, 0, 0},
	{"snipe", POS_STANDING, do_snipe, 0, SKILL_SNIPE, 0, 0},
	{"snore", POS_SLEEPING, do_action, 0, 0, 0, 0},
	{"snort", POS_RESTING, do_action, 0, 0, 0, 0},
	{"snowball", POS_STANDING, do_action, LVL_IMMORT, 0, 0, 0},
	{"snoop", POS_DEAD, do_snoop, LVL_IMMORT, 0, 0, 0},
	{"snuggle", POS_RESTING, do_action, 0, 0, 0, 0},
	{"sob", POS_RESTING, do_action, 0, 0, 0, 0},
	{"socials", POS_DEAD, do_commands, 0, SCMD_SOCIALS, 0, 0},
	{"soilage", POS_SLEEPING, do_soilage, 0, 0, 0, 0},
	{"specials", POS_DEAD, do_special, LVL_IMPL, 0, 0, 0},
	{"specializations", POS_DEAD, do_specializations, 0, 0, 0, 0},
	{"spells", POS_SLEEPING, do_skills, 0, SCMD_SPELLS_ONLY, 0, 0},
	{"spinfist", POS_FIGHTING, do_offensive_skill, 0, SKILL_SPINFIST, 0, 0},
	{"spinkick", POS_FIGHTING, do_offensive_skill, 0, SKILL_SPINKICK, 0, 0},
	{"split", POS_SITTING, do_split, 1, 0, 0, 0},
	{"spam", POS_RESTING, do_action, 0, 0, 0, 0},
	{"spasm", POS_RESTING, do_action, 0, 0, 0, 0},
	{"spank", POS_RESTING, do_action, 0, 0, 0, 0},
	{"spew", POS_SLEEPING, do_gen_comm, 0, SCMD_SPEW, 0, 0},
	{"spinout", POS_SITTING, do_action, 0, 0, 0, 0},
	{"spit", POS_RESTING, do_action, 0, 0, 0, 0},
	{"squeal", POS_RESTING, do_action, 0, 0, 0, 0},
	{"squeeze", POS_RESTING, do_action, 0, 0, 0, 0},
	{"squirm", POS_RESTING, do_action, 0, 0, 0, 0},
	{"stand", POS_RESTING, do_stand, 0, 0, 0, 0},
	{"stagger", POS_STANDING, do_action, 0, 0, 0, 0},
	{"stare", POS_RESTING, do_action, 0, 0, 0, 0},
	{"starve", POS_RESTING, do_action, 0, 0, 0, 0},
	{"stat", POS_DEAD, do_stat, LVL_AMBASSADOR, 0, 0, 0},
	{"status", POS_RESTING, do_status, 0, 0, 0, 0},
	{"stalk", POS_STANDING, do_stalk, 1, 0, 0, 0},
	{"steal", POS_SITTING, do_steal, 1, 0, 0, 0},
	{"steam", POS_RESTING, do_action, 0, 0, 0, 0},
	{"stretch", POS_SITTING, do_action, 0, 0, 0, 0},
	{"strike", POS_FIGHTING, do_offensive_skill, 0, SKILL_STRIKE, 0, 0},
	{"strip", POS_RESTING, do_action, 0, 0, 0, 0},
	{"stroke", POS_RESTING, do_action, 0, 0, 0, 0},
	{"strut", POS_STANDING, do_action, 0, 0, 0, 0},
	{"stomp", POS_FIGHTING, do_offensive_skill, 0, SKILL_STOMP, 0, 0},
	{"store", POS_SITTING, do_store, 0, 0, 0, 0},
	{"stun", POS_FIGHTING, do_stun, 0, 0, 0, 0},
	{"sulk", POS_RESTING, do_action, 0, 0, 0, 0},
	{"support", POS_RESTING, do_action, 0, 0, 0, 0},
	{"swallow", POS_RESTING, do_use, 0, SCMD_SWALLOW, 0, 0},
	{"switch", POS_DEAD, do_switch, LVL_IMMORT, 0, 0, 0},
	{"sweep", POS_RESTING, do_action, 0, 0, 0, 0},
	{"sweepkick", POS_FIGHTING, do_offensive_skill, 0, SKILL_SWEEPKICK, 0, 0},
	{"sweat", POS_RESTING, do_action, 0, 0, 0, 0},
	{"swoon", POS_RESTING, do_action, 0, 0, 0, 0},
	{"syslog", POS_DEAD, do_syslog, LVL_AMBASSADOR, 0, 0, 0},

	{"tell", POS_DEAD, do_tell, 0, 0, 0, 0},
	//    { "tempus"   , POS_DEAD    , do_help     , 0, 0 , 0 , 0},
	{"tempus", POS_DEAD, do_hcollect_help, 0, 0, 0, 0},
	{"terrorize", POS_FIGHTING, do_intimidate, 5, SKILL_TERRORIZE, 0, 0},
	{"tester", POS_FIGHTING, do_tester, 0, 0, 0, 0},
	{"tackle", POS_RESTING, do_action, 0, 0, 0, 0},
	{"take", POS_RESTING, do_get, 0, 0, 0, 0},
	{"talk", POS_RESTING, do_say, 0, 0, 0, 0},
	{"tango", POS_STANDING, do_action, 0, 0, 0, 0},
	{"tank", POS_RESTING, do_action, 0, 0, 0, 0},
	{"tantrum", POS_RESTING, do_action, 0, 0, 0, 0},
	{"taunt", POS_RESTING, do_action, 0, 0, 0, 0},
	{"taste", POS_RESTING, do_eat, 0, SCMD_TASTE, 0, 0},
	{"tag", POS_FIGHTING, do_tag, 0, 0, 0, 0},
	{"teleport", POS_DEAD, do_teleport, LVL_IMMORT, 0, 0, 0},
	{"thank", POS_RESTING, do_action, 0, 0, 0, 0},
	{"think", POS_RESTING, do_action, 0, 0, 0, 0},
	{"thaw", POS_DEAD, do_wizutil, LVL_IMMORT, SCMD_THAW, 0, 0},
	{"thumb", POS_RESTING, do_action, 0, 0, 0, 0},
	{"thrash", POS_RESTING, do_action, 0, 0, 0, 0},
	{"threaten", POS_RESTING, do_action, 0, 0, 0, 0},
	{"throatstrike", POS_FIGHTING, do_offensive_skill, 0, SKILL_THROAT_STRIKE,
			0, 0},
	{"throw", POS_FIGHTING, do_throw, 0, 0, 0, 0},
	{"tie", POS_RESTING, do_action, 0, 0, 0, 0},
	{"tight", POS_RESTING, do_action, 0, 0, 0, 0},
	{"title", POS_DEAD, do_title, 0, 0, 0, 0},
	{"tickle", POS_RESTING, do_action, 0, 0, 0, 0},
	{"time", POS_DEAD, do_time, 0, 0, 0, 0},
	{"timeout", POS_RESTING, do_action, 0, 0, 0, 0},
	{"tip", POS_RESTING, do_action, 0, 0, 0, 0},
	{"toast", POS_RESTING, do_action, 0, 0, 0, 0},
	{"touch", POS_RESTING, do_action, 0, 0, 0, 0},
	{"toggle", POS_DEAD, do_toggle, 0, 0, 0, 0},
	{"toke", POS_RESTING, do_smoke, 0, 0, 0, 0},
	{"tornado", POS_FIGHTING, do_tornado_kick, 0, 0, 0, 0},
	//{ "toss_mail", POS_DEAD       , do_toss_mail, LVL_GRIMP, 0 , 0 , 0},
	{"track", POS_STANDING, do_track, 0, 0, 0, 0},
	{"train", POS_STANDING, do_practice, 1, 0, 0, 0},
	{"trample", POS_STANDING, do_action, 0, 0, 0, 0},
	{"transfer", POS_SLEEPING, do_trans, LVL_IMMORT, 0, 0, 0},
	{"translocate", POS_RESTING, do_translocate, 20, ZEN_TRANSLOCATION, 0, 0},
	{"transmit", POS_SLEEPING, do_transmit, 0, 0, 0, 0},
	{"taunt", POS_STANDING, do_taunt, 0, 0, 0, 0},
	{"trigger", POS_SITTING, do_trigger, 1, 0, 0, 0},
	{"triggers", POS_SLEEPING, do_skills, 0, SCMD_SPELLS_ONLY, 0, 0},
	{"trip", POS_FIGHTING, do_offensive_skill, 0, SKILL_TRIP, 0, 0},
	{"tweak", POS_RESTING, do_action, 0, 0, 0, 0},
	{"twiddle", POS_RESTING, do_action, 0, 0, 0, 0},
	{"twitch", POS_SLEEPING, do_action, 0, 0, 0, 0},
	{"twirl", POS_SITTING, do_action, 0, 0, 0, 0},
	{"type", POS_RESTING, do_action, 0, 0, 0, 0},
	{"typo", POS_DEAD, do_gen_write, 0, SCMD_TYPO, 0, 0},
	{"tongue", POS_RESTING, do_action, 0, 0, 0, 0},
	{"turn", POS_FIGHTING, do_turn, 0, 0, 0, 0},
	{"tune", POS_RESTING, do_tune, 0, 0, 0, 0},

	{"unlock", POS_SITTING, do_gen_door, 0, SCMD_UNLOCK, 0, 0},
	{"unload", POS_RESTING, do_load, 0, SCMD_UNLOAD, 0, 0},
	{"ungroup", POS_DEAD, do_ungroup, 0, 0, 0, 0},
	{"unban", POS_DEAD, do_unban, LVL_IMMORT, 0, 0, 0},
	{"unaffect", POS_DEAD, do_wizutil, LVL_IMMORT, SCMD_UNAFFECT, 0, 0},
	{"undisguise", POS_RESTING, do_disguise, 0, 1, 0, 0},
	{"undress", POS_RESTING, do_action, 0, 0, 0, 0},
	{"uninstall", POS_DEAD, do_uninstall, LVL_IMMORT, 0, 0, 0},
	{"unalias", POS_DEAD, do_unalias, 0, 0, 0, 0},
	{"uppercut", POS_FIGHTING, do_offensive_skill, 0, SKILL_UPPERCUT, 0, 0},
	{"uptime", POS_DEAD, do_date, LVL_AMBASSADOR, SCMD_UPTIME, 0, 0},
	{"use", POS_SITTING, do_use, 1, SCMD_USE, 0, 0},
	{"users", POS_DEAD, do_users, LVL_IMMORT, 0, 0, 0},
	{"utter", POS_RESTING, do_say, 0, SCMD_UTTER, 0, 0},

	{"value", POS_STANDING, do_not_here, 0, 0, 0, 0},
	{"version", POS_DEAD, do_gen_ps, 0, SCMD_VERSION, 0, 0},
	{"view", POS_RESTING, do_look, 0, SCMD_LOOK, 0, 0},
	{"visible", POS_RESTING, do_visible, 1, 0, 0, 0},
	{"vnum", POS_DEAD, do_vnum, LVL_IMMORT, 0, 0, 0},
	{"vomit", POS_RESTING, do_action, 0, 0, 0, 0},
	{"voodoo", POS_RESTING, do_action, 0, 0, 0, 0},
	{"vote", POS_RESTING, do_action, 0, 0, 0, 0},
	{"vstat", POS_DEAD, do_vstat, LVL_IMMORT, 0, 0, 0},

	{"wake", POS_SLEEPING, do_wake, 0, 0, 0, 0},
	{"wait", POS_RESTING, do_action, 0, 0, 0, 0},
	{"waiting", POS_RESTING, do_action, 0, 0, 0, 0},
	{"wave", POS_RESTING, do_action, 0, 0, 0, 0},
	{"warcry", POS_RESTING, do_action, 0, 0, 0, 0},
	{"wear", POS_RESTING, do_wear, 0, 0, 0, 0},
	{"weather", POS_RESTING, do_weather, 0, 0, 0, 0},
	{"change_weather", POS_DEAD, do_gen_tog, LVL_CREATOR, SCMD_WEATHER, 0, 0},
	{"weigh", POS_RESTING, do_weigh, 0, 0, 0, 0},
	{"wedgie", POS_RESTING, do_action, 0, 0, 0, 0},
	{"whap", POS_RESTING, do_action, 0, 0, 0, 0},
	{"who", POS_DEAD, do_who, 0, 0, 0, 0},
	{"whoami", POS_DEAD, do_gen_ps, 0, SCMD_WHOAMI, 0, 0},
	{"where", POS_SLEEPING, do_where, 0, 0, 0, 0},
	{"whimper", POS_RESTING, do_action, 0, 0, 0, 0},
	{"whisper", POS_RESTING, do_spec_comm, 0, SCMD_WHISPER, 0, 0},
	{"whine", POS_RESTING, do_action, 0, 0, 0, 0},
	{"whirlwind", POS_FIGHTING, do_whirlwind, 0, 0, 0, 0},
	{"wheeze", POS_SLEEPING, do_action, 0, 0, 0, 0},
	{"whistle", POS_RESTING, do_action, 0, 0, 0, 0},
	{"wield", POS_RESTING, do_wield, 0, 0, 0, 0},
	{"wiggle", POS_STANDING, do_action, 0, 0, 0, 0},
	{"wimpy", POS_DEAD, do_wimpy, 0, 0, 0, 0},
	{"wince", POS_RESTING, do_action, 0, 0, 0, 0},
	{"wind", POS_RESTING, do_not_here, 0, 0, 0, 0},
	{"wink", POS_RESTING, do_action, 0, 0, 0, 0},
	{"wipe", POS_RESTING, do_action, 0, 0, 0, 0},
	{"withdraw", POS_STANDING, do_not_here, 1, 0, 0, 0},
	{"wiznet", POS_DEAD, do_wiznet, LVL_IMMORT, SCMD_WIZNET, 0, 0},
	{";", POS_DEAD, do_wiznet, LVL_IMMORT, SCMD_WIZNET, 0, 0},
	{"wizhelp", POS_SLEEPING, do_commands, LVL_AMBASSADOR, SCMD_WIZHELP, 0, 0},
	{"wizlick", POS_RESTING, do_action, LVL_IMMORT, 0, 0, 0},
	{"wizlist", POS_DEAD, do_wizlist, 0, SCMD_WIZLIST, 0, 0},
	{"wizards", POS_DEAD, do_gen_ps, 0, SCMD_WIZLIST, 0, 0},
	{"wizlock", POS_DEAD, do_wizlock, LVL_CREATOR, 0, 0, 0},
	{"wizflex", POS_RESTING, do_action, LVL_IMMORT, 0, 0, 0},
	{"wizpiss", POS_RESTING, do_action, LVL_GRGOD, 0, 0, 0},
	{"wizpants", POS_RESTING, do_action, LVL_TIMEGOD, 0, 0, 0},
	{"wizcut", POS_DEAD, do_wizcut, LVL_CREATOR, 0, 0, 0},
	{"wonder", POS_RESTING, do_action, 0, 0, 0, 0},
	{"woowoo", POS_RESTING, do_action, 0, 0, 0, 0},
	{"wormhole", POS_STANDING, do_translocate, 0, SKILL_WORMHOLE, 0, 0},
	{"worry", POS_RESTING, do_action, 0, 0, 0, 0},
	{"worship", POS_RESTING, do_action, 0, 0, 0, 0},
	{"worldwrite", POS_DEAD, do_gen_tog, LVL_IMMORT, SCMD_WORLDWRITE, 0, 0},
	{"wrench", POS_FIGHTING, do_wrench, 0, 0, 0, 0},
	{"wrestle", POS_RESTING, do_action, 0, 0, 0, 0},
	{"write", POS_RESTING, do_write, 1, 0, 0, 0},

	{"xlag", POS_DEAD, do_xlag, LVL_IMMORT, 0, 0, 0},
	{"yae", POS_RESTING, do_action, 0, 0, 0, 0},
	{"yay", POS_RESTING, do_action, 0, 0, 0, 0},
	{"yawn", POS_RESTING, do_action, 0, 0, 0, 0},
	{"yabba", POS_RESTING, do_action, 0, 0, 0, 0},
	{"yeehaw", POS_RESTING, do_action, 0, 0, 0, 0},
	{"yell", POS_RESTING, do_say, 0, SCMD_YELL, 0, 0},
	{"yodel", POS_RESTING, do_action, 0, 0, 0, 0},
	{"yowl", POS_RESTING, do_action, 0, 0, 0, 0},

	{"zerbert", POS_RESTING, do_action, 0, 0, 0, 0},
	{"zreset", POS_DEAD, do_zreset, LVL_IMMORT, 0, 0, 0},
	{"zoneecho", POS_DEAD, do_zecho, LVL_AMBASSADOR, 0, 0, 0},
	{"zonepurge", POS_DEAD, do_zonepurge, LVL_IMPL, 0, 0, 0},
	{"rlist", POS_DEAD, do_rlist, LVL_IMMORT, 0, 0, 0},
	{"olist", POS_DEAD, do_olist, LVL_IMMORT, 0, 0, 0},
	{"mlist", POS_DEAD, do_mlist, LVL_IMMORT, 0, 0, 0},
	{"xlist", POS_DEAD, do_xlist, LVL_IMMORT, 0, 0, 0},
	{"hcollection", POS_DEAD, do_help_collection_command, 1, 0, 0, 0},
	{"access", POS_DEAD, do_access, LVL_IMMORT, 0, 0, 0},
	{"coderutil", POS_DEAD, do_coderutil, LVL_DEMI, 0, 0, 0},
	// Moods!
	{"accusingly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"angrily", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"boldly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"calmly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"carefully", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"cautiously", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"contentedly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"crazily", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"curiously", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"desperately", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"divinely", POS_DEAD, do_mood, LVL_IMMORT, 0, 0, 0},
	{"dreamily", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"evilly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"fearfully", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"fiercely", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"fondly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"gallantly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"gently", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"graciously", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"happily", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"hatefully", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"icily", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"impatiently", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"innocently", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"kindly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"knowingly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"lazily", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"loudly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"lovingly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"lustily", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"maliciously", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"merrily", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"mockingly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"nervously", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"nicely", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"outrageously", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"patiently", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"playfully", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"politely", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"proudly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"quickly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"quietly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"rudely", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"ruthlessly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"sadly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"savagely", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"seductively", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"sheepishly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"shyly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"skeptically", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"slowly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"slyly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"softly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"solemnly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"stoically", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"sweetly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"viciously", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"warily", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"warmly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"wearily", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"wildly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"wisely", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"wishfully", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"valiantly", POS_DEAD, do_mood, 0, 0, 0, 0},
	{"\n", 0, 0, 0, 0, 0, 0}
};								/* this must be last */

void
send_unknown_cmd(Creature *ch)
{
	switch (number(0, 11)) {
	case 0:
		send_to_char(ch, "Huh?!?\r\n"); break;
	case 1:
		send_to_char(ch, "What's that?\r\n"); break;
	case 2:
		send_to_char(ch, "%cQue?!?\r\n", 191); break;
	case 3:
		send_to_char(ch, 
			"You'll have to speak in a language I understand...\r\n"); break;
	case 4:
		send_to_char(ch, "I don't understand that.\r\n"); break;
	case 5:
		send_to_char(ch, "Wie bitte?\r\n"); break;
	case 6:
		send_to_char(ch, "You're talking nonsense to me.\r\n"); break;
	case 7:
	case 8:
	case 9:
		send_to_char(ch, "Hmm, I don't understand that command.\r\n");
		break;
	case 10:
		send_to_char(ch, "Beg pardon?\r\n"); break;
	default:
		send_to_char(ch, "Come again?\r\n"); break;
	}
	return;
}

/*
 * This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function.
 */
void
command_interpreter(struct Creature *ch, char *argument)
{
	int cmd, length;
	extern int no_specials;
	char *line;

	REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);
	if (IS_AFFECTED_2(ch, AFF2_MEDITATE)) {
		send_to_char(ch, "You stop meditating.\r\n");
		REMOVE_BIT(AFF2_FLAGS(ch), AFF2_MEDITATE);
	}
	REMOVE_BIT(AFF2_FLAGS(ch), AFF2_EVADE);
	if (ch->getPosition() > POS_SLEEPING)
		REMOVE_BIT(AFF3_FLAGS(ch), AFF3_STASIS);

	if (MOUNTED(ch) && ch->in_room != MOUNTED(ch)->in_room) {
		REMOVE_BIT(AFF2_FLAGS(MOUNTED(ch)), AFF2_MOUNTED);
		MOUNTED(ch) = NULL;
	}

	/* just drop to next line for hitting CR */
	skip_spaces(&argument);
	if (!*argument)
		return;

	/*
	 * special case to handle one-character, non-alphanumeric commands;
	 * requested by many people so "'hi" or ";godnet test" is possible.
	 * Patch sent by Eric Green and Stefan Wasilewski.
	 */
	if (!isalpha(*argument)) {
		arg[0] = argument[0];
		arg[1] = '\0';
		line = argument + 1;
	} else
		line = any_one_arg(argument, arg);

	/* otherwise, find the command */
	for (length = strlen(arg), cmd = 0; *cmd_info[cmd].command != '\n'; cmd++) {
		if (!strncmp(cmd_info[cmd].command, arg, length)) {
			if (Security::canAccess(ch, &cmd_info[cmd])) {
				break;
			}
		}
	}

	if (*cmd_info[cmd].command == '\n') {
		send_unknown_cmd(ch);
		return;
	}
	if (ch->desc) {
		if (cmd == ch->desc->last_cmd &&
			!strcasecmp(line, ch->desc->last_argument))
			ch->desc->repeat_cmd_count++;
		else
			ch->desc->repeat_cmd_count = 0;
		strcpy(ch->desc->last_argument, line);
		ch->desc->last_cmd = cmd;

		/* log cmds */
		if (log_cmds || PLR_FLAGGED(ch, PLR_LOG)) {
			// Don't log movement, that's just silly.
			if(strcmp(cmd_info[cmd].command,"north")
			 && strcmp(cmd_info[cmd].command,"south")
			 && strcmp(cmd_info[cmd].command,"east")
			 && strcmp(cmd_info[cmd].command,"west") 
			 && strcmp(cmd_info[cmd].command,"up") 
			 && strcmp(cmd_info[cmd].command,"down") 
			 && strcmp(cmd_info[cmd].command,"exits")) {
				cmdlog(tmp_sprintf("CMD: %s ::%s '%s'", GET_NAME(ch),
					cmd_info[cmd].command, line));
			}
		}
		/* end log cmds */

		// Now we gather statistics on which commands are being used
		cmd_info[cmd].usage += 1;
	}

	if (PLR_FLAGGED(ch, PLR_FROZEN) && GET_LEVEL(ch) < LVL_GRIMP)
		send_to_char(ch, "You try, but the mind-numbing cold prevents you...\r\n");
	else if (IS_AFFECTED_2(ch, AFF2_PETRIFIED) && !IS_IMMORT(ch)
		&& (!ch->desc || !ch->desc->original)) {
		if (!number(0, 3))
			send_to_char(ch, "You have been turned to stone!\r\n");
		else if (!number(0, 2))
			send_to_char(ch, "You are petrified!  You cannot move an inch!\r\n");
		else if (!number(0, 1))
			send_to_char(ch, "WTF?!!  Your body has been turned to solid stone!");
		else
			send_to_char(ch, "You have been turned to stone, and cannot move.\r\n");
	} else if (cmd_info[cmd].command_pointer == NULL)
		send_to_char(ch, "Sorry, that command hasn't been implemented yet.\r\n");
	else if (IS_NPC(ch) && cmd_info[cmd].minimum_level >= LVL_IMMORT)
		send_to_char(ch, "You can't use immortal commands while switched.\r\n");
	else if (ch->getPosition() < cmd_info[cmd].minimum_position
		&& GET_LEVEL(ch) < LVL_AMBASSADOR)
		switch (ch->getPosition()) {
		case POS_DEAD:
			send_to_char(ch, "Lie still; you are DEAD!!! :-(\r\n");
			break;
		case POS_INCAP:
		case POS_MORTALLYW:
			send_to_char(ch, 
				"You are in a pretty bad shape, unable to do anything!\r\n");
			break;
		case POS_STUNNED:
			send_to_char(ch, 
				"All you can do right now is think about the stars!\r\n");
			break;
		case POS_SLEEPING:
			send_to_char(ch, "In your dreams, or what?\r\n");
			break;
		case POS_RESTING:
			send_to_char(ch, 
				"Nah... You're resting.  Why don't you sit up first?\r\n");
			break;
		case POS_SITTING:
			send_to_char(ch, "Maybe you should get on your feet first?\r\n");
			break;
		case POS_FIGHTING:
			send_to_char(ch, "No way!  You're fighting for your life!\r\n");
			if (!FIGHTING(ch))
				slog("SYSERR: Char !FIGHTING(ch) while pos fighting.");
			break;
	} else if (no_specials ||
			!special(ch, cmd, cmd_info[cmd].subcmd, line, SPECIAL_CMD)) {
		cmd_info[cmd].command_pointer(ch, line, cmd, cmd_info[cmd].subcmd, 0);
	}

}

/**************************************************************************
 * Routines to handle aliasing                                             *
  **************************************************************************/


struct alias_data *
find_alias(struct alias_data *alias_list, char *str)
{
	while (alias_list != NULL) {
		if (*str == *alias_list->alias)	/* hey, every little bit counts :-) */
			if (!strcmp(str, alias_list->alias))
				return alias_list;

		alias_list = alias_list->next;
	}

	return NULL;
}


void
free_alias(struct alias_data *a)
{
	if (a->alias)
		free(a->alias);
	if (a->replacement)
		free(a->replacement);
	free(a);
}

void
add_alias(struct Creature *ch, struct alias_data *a)
{
	struct alias_data *this_alias = GET_ALIASES(ch);

	if ((!this_alias) || (strcmp(a->alias, this_alias->alias) < 0)) {
		a->next = this_alias;
		GET_ALIASES(ch) = a;
		return;
	}

	while (this_alias->next != NULL) {
		if (strcmp(a->alias, this_alias->next->alias) < 0) {
			a->next = this_alias->next;
			this_alias->next = a;
			return;
		}
		this_alias = this_alias->next;
	}
	a->next = NULL;
	this_alias->next = a;
	return;
}

/* The interface to the outside world: do_alias */
ACMD(do_alias)
{
	char *repl, *arg;
	struct alias_data *cur_alias, *temp;
	int alias_cnt = 0;

	if (IS_NPC(ch))
		return;

	arg = tmp_getword(&argument);
	repl = argument;

	if (!*arg || !*repl) {				/* no argument specified -- list currently defined aliases */
		strcpy(buf, "Currently defined aliases:\r\n");
		cur_alias = GET_ALIASES(ch);
		if (!cur_alias) {
			strcat(buf, " None.\r\n");
		} else {
			while (cur_alias != NULL) {
				if (!*arg || is_abbrev(arg, cur_alias->alias)) {
					sprintf(buf, "%s%s%-15s%s %s\r\n", buf, CCCYN(ch, C_NRM),
						cur_alias->alias, CCNRM(ch, C_NRM),
						cur_alias->replacement);
					alias_cnt++;
				}
				cur_alias = cur_alias->next;
			}
			if (!alias_cnt)
				strcat(buf, " None matching.\r\n");
		}
		page_string(ch->desc, buf);
	} else {					/* otherwise, add or display aliases */
		if (!str_cmp(arg, "alias")) {
			send_to_char(ch, "You can't alias 'alias'.\r\n");
			return;
		}

		/* is this an alias we've already defined? */
		cur_alias = find_alias(GET_ALIASES(ch), arg);
		if (cur_alias) {
			REMOVE_FROM_LIST(cur_alias, GET_ALIASES(ch), next);
#ifdef DMALLOC
			dmalloc_verify(0);
#endif
			free_alias(cur_alias);
#ifdef DMALLOC
			dmalloc_verify(0);
#endif
		}

		CREATE(cur_alias, struct alias_data, 1);
		cur_alias->alias = str_dup(arg);
		delete_doubledollar(repl);
		cur_alias->replacement = str_dup(repl);
		if (strchr(repl, ALIAS_SEP_CHAR) || strchr(repl, ALIAS_VAR_CHAR))
			cur_alias->type = ALIAS_COMPLEX;
		else
			cur_alias->type = ALIAS_SIMPLE;
		add_alias(ch, cur_alias);
		send_to_char(ch, "Alias added.\r\n");
	}
}

/* The interface to the outside world: do_unalias */
ACMD(do_unalias)
{
	char *repl;
	struct alias_data *a, *temp;

	if (IS_NPC(ch))
		return;

	repl = any_one_arg(argument, arg);	/* vintage */

	if (!*arg) {				/* no argument specified -- what a dumbass */
		send_to_char(ch, "You must specify something to unalias.\r\n");
		return;
	} else {
		if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL) {
			REMOVE_FROM_LIST(a, GET_ALIASES(ch), next);
#ifdef DMALLOC
			dmalloc_verify(0);
#endif
			free_alias(a);
#ifdef DMALLOC
			dmalloc_verify(0);
#endif
			send_to_char(ch, "Alias removed.\r\n");
		} else {
			send_to_char(ch, "No such alias Mr. Tard.\r\n");
		}
	}
}

/*
 * Valid numeric replacements are only &1 .. &9 (makes parsing a little
 * easier, and it's not that much of a limitation anyway.)  Also valid
 * is "&*", which stands for the entire original line after the alias.
 * ";" is used to delimit commands.
 */
#define NUM_TOKENS       9

void
perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a)
{
	struct txt_q temp_queue;
	char *tokens[NUM_TOKENS], *temp, *write_point;
	int num_of_tokens = 0, num;

	/* First, parse the original string */
	temp = strtok(strcpy(buf2, orig), " ");
	while (temp != NULL && num_of_tokens < NUM_TOKENS) {
		tokens[num_of_tokens++] = temp;
		temp = strtok(NULL, " ");
	}

	/* initialize */
	write_point = buf;
	temp_queue.head = temp_queue.tail = NULL;

	/* now parse the alias */
	for (temp = a->replacement; *temp; temp++) {
		if (*temp == ALIAS_SEP_CHAR) {
			*write_point = '\0';
			buf[MAX_INPUT_LENGTH - 1] = '\0';
			write_to_q(buf, &temp_queue, 1);
			write_point = buf;
		} else if (*temp == ALIAS_VAR_CHAR) {
			temp++;
			if ((num = *temp - '1') < num_of_tokens && num >= 0) {
				strcpy(write_point, tokens[num]);
				write_point += strlen(tokens[num]);
			} else if (*temp == ALIAS_GLOB_CHAR) {
				strcpy(write_point, orig);
				write_point += strlen(orig);
			} else if ((*(write_point++) = *temp) == '$')	/* redouble $ for act safety */
				*(write_point++) = '$';
		} else
			*(write_point++) = *temp;
	}

	*write_point = '\0';
	buf[MAX_INPUT_LENGTH - 1] = '\0';
	write_to_q(buf, &temp_queue, 1);

	/* push our temp_queue on to the _front_ of the input queue */
	if (input_q->head == NULL)
		*input_q = temp_queue;
	else {
		temp_queue.tail->next = input_q->head;
		input_q->head = temp_queue.head;
	}
}


/*
 * Given a character and a string, perform alias replacement on it.
 *
 * Return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue.
 */
int
perform_alias(struct descriptor_data *d, char *orig)
{
	char first_arg[MAX_INPUT_LENGTH], *ptr;
	struct alias_data *a, *tmp;

	/* bail out immediately if the guy doesn't have any aliases */
	if ((tmp = GET_ALIASES(d->creature)) == NULL)
		return 0;

	/* find the alias we're supposed to match */
	ptr = any_one_arg(orig, first_arg);

	/* bail out if it's null */
	if (!*first_arg)
		return 0;

	/* if the first arg is not an alias, return without doing anything */
	if ((a = find_alias(tmp, first_arg)) == NULL)
		return 0;

	if (a->type == ALIAS_SIMPLE) {
		strcpy(orig, a->replacement);
		return 0;
	} else {
		perform_complex_alias(&d->input, ptr, a);
		return 1;
	}
}



/***************************************************************************
 * Various other parsing utilities                                         *
 **************************************************************************/

/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 */
int
search_block(const char *arg, const char **list, bool exact)
{
	if( *arg == '\0' )
		return -1;

	if (exact) {
		for( int i = 0; *(list[i]) != '\n'; i++ ) {
			if( *(list[i]) == '!' ) // reserved
				continue;
			if( strcasecmp( arg, list[i] ) == 0 )
				return i;
		}
	} else {
		int len = strlen( arg );
		for (int i = 0; *(list[i]) != '\n'; i++) {
			if( *(list[i]) == '!' ) // reserved
				continue;
			if (strncasecmp(arg, list[i], len) == 0)
				return i;
			if (i > 1000) {
				mudlog(LVL_AMBASSADOR, BRF, true,
					"SYSERR: search_block in unterminated list. arg = %s [0] = '%s'.",
				arg,list[0]);
				return -1;
			}
		}
	}
	return -1;
}


int
is_number(const char *str)
{

	if (str[0] == '-' || str[0] == '+')
		str++;

	while (*str)
		if (!isdigit(*(str++)))
			return 0;

	return 1;
}


void
skip_spaces(char **string)
{
	for (; **string && isspace(**string); (*string)++);
}

void
skip_spaces(const char **string)
{
	for (; **string && isspace(**string); (*string)++);
}


char *
delete_doubledollar(char *string)
{
	char *read, *write;

	if ((write = strchr(string, '$')) == NULL)
		return string;

	read = write;

	while (*read)
		if ((*(write++) = *(read++)) == '$')
			if (*read == '$')
				read++;

	*write = '\0';

	return string;
}


int
fill_word(char *argument)
{
	return (search_block(argument, fill_words, TRUE) >= 0);
}


/*
 * copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string.
 */
char *
one_argument(char *argument, char *first_arg)
{
	char *begin = first_arg;

	do {
		skip_spaces(&argument);

		first_arg = begin;
		while (*argument && !isspace(*argument)) {
			*(first_arg++) = tolower(*argument);
			argument++;
		}

		*first_arg = '\0';
	} while (fill_word(begin));

	return argument;
}

// A non-destructive version of the previous that returns nothing.
// DO NOT set this equal to anything, you'll get nada.
void
one_argument(const char *argument, char *first_arg)
{
	char *begin = first_arg;
	const char *s = argument;
	do {
		while (isspace(*s))
			s++;
		//skip_spaces(&s);

		first_arg = begin;
		while (*s && !isspace(*s)) {
			*(first_arg++) = tolower(*s);
			s++;
		}

		*first_arg = '\0';
	} while (fill_word(begin));
}


/* same as one_argument except that it doesn't ignore fill words */
char *
any_one_arg(char *argument, char *first_arg)
{
	skip_spaces(&argument);

	while (*argument && !isspace(*argument)) {
		*(first_arg++) = tolower(*argument);
		argument++;
	}

	*first_arg = '\0';

	return argument;
}


/*
 * Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words
 */
char *
two_arguments(char *argument, char *first_arg, char *second_arg)
{
	return one_argument(one_argument(argument, first_arg), second_arg);
	/* :-) */
}

void
two_arguments(const char *argument, char *first_arg, char *second_arg)
{
	char *begin = first_arg;
	const char *s = argument;
	do {
		while (isspace(*s))
			s++;
		// Yank out the first arg
		first_arg = begin;
		while (*s && !isspace(*s)) {
			*(first_arg++) = tolower(*s);
			s++;
		}
		*first_arg = '\0';
	} while (fill_word(begin));

	// Now the second arg
	begin = second_arg;
	do {
		while (isspace(*s))
			s++;
		second_arg = begin;
		while (*s && !isspace(*s)) {
			*(second_arg++) = tolower(*s);
			s++;
		}
		*second_arg = '\0';
	} while (fill_word(begin));
}


/*
 * determine if a given string is an abbreviation of another
 * (now works symmetrically -- JE 7/25/94)
 *
 * that was dumb.  it shouldn't be symmetrical.  JE 5/1/95
 * 
 * returns 1 if arg1 is an abbreviation of arg2
 * returns 2 if arg1 is an extact match to arg2
 * returns 0 otherwise
 */
int
is_abbrev(const char *needle, const char *haystack)
{
	if (!*needle)
		return 0;

	while (*needle && *haystack)
		if (tolower(*needle++) != tolower(*haystack++))
			return 0;
	
	if (!*needle && !*haystack)
		return 2;
	if (!*needle)
		return 1;

	return 0;
}



/* return first space-delimited token in arg1; remainder of string in arg2 */
void
half_chop(char *string, char *arg1, char *arg2)
{
	char *temp;

	temp = any_one_arg(string, arg1);
	skip_spaces(&temp);
	strcpy(arg2, temp);
}



/* Used in specprocs, mostly.  (Exactly) matches "command" to cmd number */
int
find_command(char *command, bool abbrev)
{
	int cmd;

	if (abbrev) {
		int len = strlen(command);

		for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
			if (!strncmp(cmd_info[cmd].command, command, len))
				return cmd;
	} else {
		for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
			if (!strcmp(cmd_info[cmd].command, command))
				return cmd;
	}

	return -1;
}


long
special(struct Creature *ch, int cmd, int subcmd, char *arg, special_mode spec_mode)
{
	struct obj_data *i;
	struct special_search_data *srch = NULL;
	char tmp_arg[MAX_INPUT_LENGTH];
	int j, tmp_cmd = 0;
	int found = 0, result = 0;
	long specAddress = 0;

	/* special in room? */
	if (GET_ROOM_SPEC(ch->in_room) != NULL) {
		specAddress = (long)GET_ROOM_SPEC(ch->in_room);
		if (GET_ROOM_SPEC(ch->in_room) (ch, ch->in_room, cmd, arg, spec_mode)) {
            //if( spec_mode == SPECIAL_LEAVE )
            //    raise(SIGINT);
			return 1;
        }
    }

	/* search special in room */
	strcpy(tmp_arg, arg);		/* don't mess up the arg, in case of special */
	tmp_cmd = cmd;

	/* translate cmd for jumping, etc... */
	if (cmd_info[cmd].command_pointer == do_move &&
		subcmd > SCMD_MOVE && *tmp_arg) {
		if ((j = search_block(tmp_arg, dirs, FALSE)) >= 0) {
			tmp_cmd = j + 1;
		}
	}
	for (srch = ch->in_room->search; srch; srch = srch->next) {
		if (triggers_search(ch, cmd, tmp_arg, srch)) {
			if ((result = general_search(ch, srch, found)) == 2)
				return 1;
			if (!found)
				found = result;
		}
	}

	if (found)
		return 1;

	// Special in self?  (for activating special abilities in switched mobs)
	if (IS_NPC(ch) && GET_MOB_SPEC(ch)) {
		specAddress = (long)GET_MOB_SPEC(ch);
		if (GET_MOB_SPEC(ch) (ch, ch, cmd, arg, spec_mode))
			return specAddress;
	}

	/* special in equipment list? */
	for (j = 0; j < NUM_WEARS; j++) {
		if ((i = GET_EQ(ch, j))) {
			specAddress = (long)GET_OBJ_SPEC(i);
			if (GET_OBJ_SPEC(i) && (GET_OBJ_SPEC(i) (ch, i, cmd, arg, spec_mode))) {
                //if( spec_mode == SPECIAL_LEAVE )
                //    raise(SIGINT);
				return specAddress;
            }
			if (IS_BOMB(i) && i->contains && IS_FUSE(i->contains) &&
				(FUSE_IS_MOTION(i->contains) || FUSE_IS_CONTACT(i->contains))
				&& FUSE_STATE(i->contains)) {
				FUSE_TIMER(i->contains)--;
				if (FUSE_TIMER(i->contains) <= 0) {
					detonate_bomb(i);
					return TRUE;
				}
			}
		}
	}

	/* special in inventory? */
	for (i = ch->carrying; i; i = i->next_content) {
		specAddress = (long)GET_OBJ_SPEC(i);
		if (GET_OBJ_SPEC(i) && (GET_OBJ_SPEC(i) (ch, i, cmd, arg, spec_mode))) {
            //if( spec_mode == SPECIAL_LEAVE )
            //    raise(SIGINT);
			return specAddress;
        }
		if (IS_BOMB(i) && i->contains && IS_FUSE(i->contains) &&
			(FUSE_IS_MOTION(i->contains) || FUSE_IS_CONTACT(i->contains)) &&
			FUSE_STATE(i->contains)) {
			FUSE_TIMER(i->contains)--;
			if (FUSE_TIMER(i->contains) <= 0) {
				detonate_bomb(i);
				return TRUE;
			}
		}
	}

	/* special in mobile present? */
	room_data *theRoom = ch->in_room;
	CreatureList::iterator it = theRoom->people.begin();
	for (; it != theRoom->people.end(); ++it)
		if (GET_MOB_SPEC((*it)) != NULL) {
			specAddress = (long)GET_MOB_SPEC((*it));
			if (GET_MOB_SPEC((*it)) (ch, (*it), cmd, arg, spec_mode)) {
                //if( spec_mode == SPECIAL_LEAVE )
                //    raise(SIGINT);
				return specAddress;
			}
		}

	/* special in object present? */
	for (i = ch->in_room->contents; i; i = i->next_content) {
		if (GET_OBJ_SPEC(i) != NULL) {
			specAddress = (long)GET_OBJ_SPEC(i);
			if (GET_OBJ_SPEC(i) (ch, i, cmd, arg, spec_mode)) {
                //if( spec_mode == SPECIAL_LEAVE )
                //    raise(SIGINT);
				return specAddress;
            }
        }
		if (IS_BOMB(i) && i->contains && IS_FUSE(i->contains) &&
			FUSE_IS_MOTION(i->contains) && FUSE_STATE(i->contains)) {
			FUSE_TIMER(i->contains)--;
			if (FUSE_TIMER(i->contains) <= 0) {
				detonate_bomb(i);
				return TRUE;
			}
		}
	}

	return 0;
}

void
sort_commands(void)
{
	int a, b, tmp;

	ACMD(do_action);

	num_of_cmds = 0;

	/*
	 * first, count commands (num_of_commands is actually one greater than the
	 * number of commands; it inclues the '\n'.
	 */
	while (*cmd_info[num_of_cmds].command != '\n')
		num_of_cmds++;

	/* create data array */
	CREATE(cmd_sort_info, struct sort_struct, num_of_cmds);

	/* initialize it */
	for (a = 1; a < num_of_cmds; a++) {
		cmd_sort_info[a].sort_pos = a;
		cmd_sort_info[a].is_social =
			(cmd_info[a].command_pointer == do_action);
		cmd_sort_info[a].is_mood =
			(cmd_info[a].command_pointer == do_mood);
	}

	/* the infernal special case */
	cmd_sort_info[find_command("insult")].is_social = TRUE;

	/* Sort.  'a' starts at 1, not 0, to remove 'RESERVED' */
	for (a = 1; a < num_of_cmds - 1; a++)
		for (b = a + 1; b < num_of_cmds; b++)
			if (strcmp(cmd_info[cmd_sort_info[a].sort_pos].command,
					cmd_info[cmd_sort_info[b].sort_pos].command) > 0) {
				tmp = cmd_sort_info[a].sort_pos;
				cmd_sort_info[a].sort_pos = cmd_sort_info[b].sort_pos;
				cmd_sort_info[b].sort_pos = tmp;
			}
}



#undef __interpreter_c__
