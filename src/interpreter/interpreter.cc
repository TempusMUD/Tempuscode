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

#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "handler.h"
#include "mail.h"
#include "screen.h"
#include "char_class.h"
#include "clan.h"
#include "vehicle.h"
#include "house.h"
#include "login.h"
#include "matrix.h"
#include "bomb.h"

extern char *motd;
extern char *ansi_motd;
extern char *imotd;
extern char *ansi_imotd;
extern char *background;
extern char *MENU;
extern char *WELC_MESSG;
extern char *START_MESSG;
extern struct char_data *character_list;
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern int restrict;
extern int num_of_houses;
extern int mini_mud;
extern int log_cmds;
extern struct spell_info_type spell_info[];
extern struct house_control_rec house_control[];
extern int shutdown_count;


/* external functions */
void echo_on(struct descriptor_data * d);
void echo_off(struct descriptor_data * d);
void do_start(struct char_data * ch, int mode);
void init_char(struct char_data * ch);
void show_mud_date_to_char(struct char_data *ch);
void show_programs_to_char(struct char_data *ch, int char_class);
void perform_net_who(struct char_data *ch, char *arg);
void perform_net_finger(struct char_data *ch, char *arg);
int general_search(struct char_data *ch, struct special_search_data *srch, int mode);
void roll_real_abils(struct char_data * ch);
void print_attributes_to_buf(struct char_data *ch, char *buff);
void polc_input(struct descriptor_data * d, char *str);

int create_entry(char *name);
int special(struct char_data * ch, int cmd, int subcmd, char *arg);
int isbanned(char *hostname, char *blocking_hostname);
int Valid_Name(char *newname);
int House_can_enter(struct char_data *ch, room_num house);

void notify_cleric_moon(struct char_data *ch);

ACMD(do_zcom);
ACMD(do_objupdate);

/* prototypes for all do_x functions. */
ACMD(do_action);
ACMD(do_activate);
ACMD(do_addname);
ACMD(do_addpos);
ACMD(do_advance);
ACMD(do_affects);
ACMD(do_alias);
ACMD(do_alter);
ACMD(do_analyze);
ACMD(do_approve);
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
ACMD(do_demote);
ACMD(do_de_energize);
ACMD(do_defuse);
ACMD(do_disembark);
ACMD(do_disguise);
ACMD(do_disarm);
ACMD(do_discharge);
ACMD(do_dismiss);
ACMD(do_distance);
ACMD(do_drain);
ACMD(do_dynedit);
ACMD(do_dyntext_show);
ACMD(do_beserk);
ACMD(do_board);
ACMD(do_bomb);
ACMD(do_bandage);
ACMD(do_cast);
ACMD(do_ceasefire);
ACMD(do_conceal);
ACMD(do_cedit);
ACMD(do_circle);
ACMD(do_cinfo);
ACMD(do_clan_comm);
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
ACMD(do_tune);
ACMD(do_color);
ACMD(do_combo);
ACMD(do_commands);
ACMD(do_compare);
ACMD(do_computer);
ACMD(do_consider);
ACMD(do_convert);
ACMD(do_credits);
ACMD(do_date);
ACMD(do_dc);
ACMD(do_diagnose);
ACMD(do_dismount);
ACMD(do_display);
ACMD(do_drag);
ACMD(do_drink);
ACMD(do_drive);
ACMD(do_drop);
ACMD(do_eat);
ACMD(do_echo);
ACMD(do_econvert);
ACMD(do_elude);
ACMD(do_empower);
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
ACMD(do_get);
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
ACMD(do_help);
ACMD(do_hide);
ACMD(do_hit);
ACMD(do_house);
ACMD(do_holytouch);
ACMD(do_hotwire);
ACMD(do_ignite);
ACMD(do_impale);
ACMD(do_improve);
ACMD(do_info);
ACMD(do_intermud);
ACMD(do_interpage);
ACMD(do_intertel);
ACMD(do_interwho);
ACMD(do_interwiz);
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
ACMD(do_menu);
ACMD(do_mload);
ACMD(do_mount);
ACMD(do_move);
ACMD(do_mshield);
ACMD(do_mudlist);
ACMD(do_mudinfo);
ACMD(do_not_here);
ACMD(do_oecho);
ACMD(do_offer);
ACMD(do_offensive_skill);
ACMD(do_olc);
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
ACMD(do_teleport);
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
ACMD(do_wield);
ACMD(do_wimpy);
ACMD(do_wizlock);
ACMD(do_wiznet);
ACMD(do_wizutil);
ACMD(do_mudwipe);
ACMD(do_write);
ACMD(do_xlag);
ACMD(do_zreset);
ACMD(do_zonepurge);
ACMD(do_rlist);
ACMD(do_olist);
ACMD(do_mlist);
ACMD(do_xlist);


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



extern const struct command_info cmd_info[] = {
    { "RESERVED", 0, 0, 0, 0 },	/* this must be first -- for specprocs */

    /* directions must come before other commands but after RESERVED */
    { "north"    , POS_STANDING, do_move     , 0, SCMD_NORTH },
    { "east"     , POS_STANDING, do_move     , 0, SCMD_EAST },
    { "south"    , POS_STANDING, do_move     , 0, SCMD_SOUTH },
    { "west"     , POS_STANDING, do_move     , 0, SCMD_WEST },
    { "up"       , POS_STANDING, do_move     , 0, SCMD_UP },
    { "down"     , POS_STANDING, do_move     , 0, SCMD_DOWN },
    { "future"   , POS_STANDING, do_move     , 0, SCMD_FUTURE },
    { "past"     , POS_STANDING, do_move     , 0, SCMD_PAST },

    /* now, the main list */
    { "areas"    , POS_DEAD    , do_gen_ps   , 0, SCMD_AREAS},
    { "at"       , POS_DEAD    , do_at       , LVL_DEMI, 0},
    { "attributes",POS_SLEEPING , do_attributes, 0, 0 },
    { "attack"   , POS_RESTING , do_kill     , 0, 0 },
    { "attach"   , POS_RESTING  , do_attach  , 0, SCMD_ATTACH },
    { "addname"  , POS_SLEEPING, do_addname  , LVL_SPIRIT, 0 },
    { "addpos"   , POS_SLEEPING, do_addpos   , LVL_GOD, 0 },
    { "advance"  , POS_DEAD    , do_advance  , LVL_GRGOD, 0 },
    { "ahem"     , POS_RESTING , do_action   , 0, 0 },
    { "alias"    , POS_DEAD    , do_alias    , 0, 0 },
    { "alter"    , POS_SITTING , do_alter    , 0, 0 },
    { "alterations", POS_SLEEPING, do_skills   , 0, SCMD_SPELLS_ONLY },
    { "activate" , POS_SITTING , do_activate , 0, SCMD_ON },
    { "accept"   , POS_RESTING , do_action   , 0, 0 },
    { "accuse"   , POS_RESTING , do_action   , 0, 0 },
    { "ack"      , POS_RESTING , do_action   , 0, 0 },
    { "addict"   , POS_RESTING , do_action   , 0, 0 },
    { "adjust"   , POS_RESTING , do_action   , 0, 0 },
    { "afw"      , POS_RESTING , do_action   , 0, 0 },
    { "agree"    , POS_RESTING , do_action   , 0, 0 },
    { "analyze"  , POS_SITTING , do_analyze  , 0, 0 },
    { "anonymous", POS_DEAD    , do_gen_tog  , 1, SCMD_ANON},
    { "anticipate",POS_RESTING , do_action   , 0, 0 },
    { "apologize", POS_RESTING , do_action   , 0, 0 },
    { "applaud"  , POS_RESTING , do_action   , 0, 0 },
    { "approve"  , POS_DEAD,     do_approve  , LVL_GOD, 0 },
    { "assist"   , POS_FIGHTING, do_assist   , 1, 0 },
    { "assimilate",POS_RESTING , do_assimilate, 0, 0 },
    { "ask"      , POS_RESTING , do_spec_comm, 0, SCMD_ASK },
    { "auction"  , POS_SLEEPING, do_gen_comm , 0, SCMD_AUCTION },
    { "autodiagnose",POS_DEAD  , do_gen_tog  , 1, SCMD_AUTO_DIAGNOSE },
    { "autoexits" , POS_DEAD   , do_gen_tog  , 1, SCMD_AUTOEXIT },
    { "autoloot"  , POS_DEAD   , do_gen_tog  , 1, SCMD_AUTOLOOT },
    { "autopage" , POS_SLEEPING, do_gen_tog  , 0, SCMD_AUTOPAGE },
    { "autoprompt", POS_SLEEPING, do_gen_tog  , 0, SCMD_AUTOPROMPT },
    { "autopsy"  , POS_RESTING , do_autopsy  , 0, 0 },
    { "autosplit"  , POS_DEAD   , do_gen_tog  , 1, SCMD_AUTOSPLIT },
    { "affects"  , POS_SLEEPING, do_affects  , 0, 0 },
    { "afk"      , POS_SLEEPING, do_gen_tog  , 0, SCMD_AFK },

    { "babble"   , POS_RESTING , do_say   , 0, SCMD_BABBLE },
    { "backstab" , POS_STANDING, do_backstab , 1, 0 },
    { "ban"      , POS_DEAD    , do_ban      , LVL_ETERNAL, 0 },
    { "bandage"  , POS_STANDING, do_bandage  , 0, 0 },
    { "balance"  , POS_STANDING, do_not_here , 1, 0 },
    { "banzai"   , POS_RESTING , do_action   , 0, 0 },
    { "bark"     , POS_SITTING , do_action   , 0, 0 },  
    { "bash"     , POS_FIGHTING, do_bash     , 1, 0 },
    { "bat"      , POS_SITTING , do_action   , 0, 0 },
    { "battlecry", POS_FIGHTING, do_battlecry, 0, SCMD_BATTLE_CRY },
    { "bawl"     , POS_RESTING , do_action   , 0, 0 },
    { "bcatch"   , POS_RESTING , do_action   , 0, 0 },  
    { "beam"     , POS_RESTING , do_action   , 0, 0 },
    { "beckon"   , POS_SITTING , do_action   , 0, 0 },
    { "beef"     , POS_RESTING , do_action   , 0, 0 },
    { "beer"     , POS_RESTING , do_action   , 0, 0 },
    { "beg"      , POS_RESTING , do_action   , 0, 0 },
    { "beguile"  , POS_STANDING, do_beguile  , 0, 0 },
    { "beserk"   , POS_FIGHTING, do_beserk   , 0, 0 },
    { "bellow"   , POS_RESTING , do_say      , 0, SCMD_BELLOW },
    { "belly"    , POS_RESTING , do_action   , 0, 0},
    { "bearhug"  , POS_FIGHTING, do_offensive_skill, 0, SKILL_BEARHUG},
    { "behead"   , POS_FIGHTING, do_offensive_skill, 0, SKILL_BEHEAD },
    { "beeper"   , POS_SLEEPING, do_gen_tog  , 0, SCMD_AUTOPAGE },
    { "bell"     , POS_SLEEPING, do_gen_tog  , 0, SCMD_AUTOPAGE },
    { "belch"    , POS_SLEEPING, do_action   , 0, 0 },
    { "bioscan"  , POS_SITTING , do_bioscan  , 0 ,0 },
    { "bird"     , POS_RESTING , do_action   , 0, 0 },
    { "bite"     , POS_FIGHTING, do_offensive_skill, 0, SKILL_BITE },
    { "bkiss"    , POS_RESTING , do_action   , 0, 0 },
    { "blame"      , POS_RESTING , do_action   , 0, 0 },
    { "bleed"    , POS_RESTING , do_action   , 0, 0 },
    { "blush"    , POS_RESTING , do_action   , 0, 0 },
    { "blink"    , POS_RESTING , do_action   , 0, 0 },
    { "blow"     , POS_RESTING , do_action   , 0, 0 },
    { "board"    , POS_STANDING, do_board    , 0, 0 },
    { "bodyslam" , POS_FIGHTING, do_offensive_skill , 0, SKILL_BODYSLAM },
    { "bow"      , POS_STANDING, do_action   , 0, 0 },
    { "boggle"   , POS_RESTING , do_action   , 0, 0 },
    { "bomb"     , POS_SLEEPING, do_bomb     , LVL_IMMORT, 0},
    { "bonk"     , POS_RESTING , do_action   , 0, 0 },
    { "bones"    , POS_RESTING , do_action   , 0, 0 },
    { "booger"   , POS_RESTING , do_action   , 0, 0 },
    { "bottle"   , POS_RESTING , do_action   , 0, 0 },
    { "bounce"   , POS_SITTING , do_action   , 0, 0 },
    { "brag"     , POS_RESTING , do_action   , 0, 0 },
    { "brb"      , POS_RESTING , do_action   , 0, 0 },
    { "brief"    , POS_DEAD    , do_gen_tog  , 0, SCMD_BRIEF },
    { "buff"     , POS_DEAD    , do_action   , 0, 0 },
    { "burn"     , POS_RESTING , do_action   , 0, 0 },
    { "burp"     , POS_RESTING , do_action   , 0, 0 },
    { "buy"      , POS_STANDING, do_not_here , 0, 0 },
    { "bug"      , POS_DEAD    , do_gen_write, 0, SCMD_BUG },

    { "cast"     , POS_SITTING , do_cast     , 1, 0 },
    { "cash"     , POS_DEAD    , do_cash     , 0, 0 },
    { "cackle"   , POS_RESTING , do_action   , 0, 0 },
    { "camel"    , POS_RESTING , do_action   , 0, 0 },
    { "catnap"   , POS_RESTING , do_action   , 0, 0 },
    { "chat"     , POS_SLEEPING, do_gen_comm , 0, SCMD_GOSSIP },
    { "charge"   , POS_STANDING, do_action   , 0, 0 },
    { "check"    , POS_STANDING, do_not_here , 1, 0 },
    { "cheer"    , POS_RESTING,  do_action   , 0, 0 },
    { "choke"    , POS_FIGHTING, do_offensive_skill, 0, SKILL_CHOKE },
    { "clansay"  , POS_MORTALLYW, do_clan_comm, LVL_CAN_CLAN, SCMD_CLAN_SAY },
    { "csay"     , POS_MORTALLYW,  do_clan_comm, LVL_CAN_CLAN, SCMD_CLAN_SAY },
    { "clanpasswd", POS_RESTING, do_clanpasswd, LVL_CAN_CLAN, 0 },
    { "clanemote", POS_STUNNED,  do_clan_comm,  LVL_CAN_CLAN, SCMD_CLAN_ECHO },
    { "clantitle", POS_STUNNED,  do_gen_tog,   LVL_CAN_CLAN, SCMD_CLAN_TITLE },
    { "clanhide" , POS_STUNNED,  do_gen_tog,   LVL_CAN_CLAN, SCMD_CLAN_HIDE },
    { "clue"      , POS_RESTING , do_action   , 0, 0 },
    { "clueless"  , POS_RESTING , do_action   , 0, 0 },
    { "ceasefire", POS_RESTING,  do_ceasefire,  0, 0 },
    { "cemote"   , POS_STUNNED,  do_clan_comm,  LVL_CAN_CLAN, SCMD_CLAN_ECHO },
    { "claninfo" , POS_SLEEPING, do_cinfo    ,  LVL_CAN_CLAN, 0 },
    { "cinfo"    , POS_SLEEPING, do_cinfo    ,  LVL_CAN_CLAN, 0 },
    { "cedit"    , POS_DEAD,     do_cedit,      LVL_GRGOD,  0 },
    { "circle"   , POS_FIGHTING, do_circle   , 0, 0 },
    { "cities"   , POS_SLEEPING, do_help     , 0, SCMD_CITIES },
    { "clanlist" , POS_MORTALLYW, do_clanlist, LVL_CAN_CLAN, 0 },
    { "claw"     , POS_FIGHTING, do_offensive_skill, 0, SKILL_CLAW },
    { "cling"    , POS_RESTING , do_action   , 0, 0 },
    { "clothesline", POS_FIGHTING, do_offensive_skill, 0, SKILL_CLOTHESLINE },
    { "chuckle"  , POS_RESTING , do_action   , 0, 0 },
    { "clap"     , POS_RESTING , do_action   , 0, 0 },
    { "clean"    , POS_RESTING , do_clean   , 0, 0 },
    { "clear"    , POS_DEAD    , do_gen_ps   , 0, SCMD_CLEAR },
    { "close"    , POS_SITTING , do_gen_door , 0, SCMD_CLOSE },
    { "cls"      , POS_DEAD    , do_gen_ps   , 0, SCMD_CLEAR },
    { "color"    , POS_DEAD    , do_color    , 0, 0 },
    { "collapse" , POS_RESTING , do_action   , 0, 0 },
    { "comfort"  , POS_RESTING , do_action   , 0, 0 },
    { "comb"     , POS_RESTING , do_action   , 0, 0 },
    { "combo"    , POS_FIGHTING, do_combo    , 0, 0 },
    { "commands" , POS_DEAD    , do_commands , 0, SCMD_COMMANDS },
    { "compact"  , POS_DEAD    , do_gen_tog  , 0, SCMD_COMPACT },
    { "compare"  , POS_RESTING , do_compare  , 0, 0 },
    { "consider" , POS_RESTING , do_consider , 0, 0 },
    { "conceal"  , POS_RESTING , do_conceal  , 0, 0 },
    { "concerned", POS_RESTING , do_action   , 0, 0 },
    { "confused", POS_RESTING , do_action   , 0, 0 },
    { "conspire" , POS_RESTING , do_action   , 0, 0 },
    { "contemplate", POS_RESTING, do_action  , 0, 0 },
    { "convert"  , POS_RESTING , do_convert  , 0, 0 },
    { "cough"    , POS_RESTING , do_action   , 0, 0 },
    { "council"  , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_COUNCIL },
    { "cover"    , POS_RESTING , do_action   , 0, 0 },
    { "cower"    , POS_RESTING , do_action   , 0, 0 },
    { "crack"    , POS_RESTING , do_action   , 0, 0 },
    { "cramp"    , POS_RESTING , do_action   , 0, 0 },
    { "cranekick", POS_FIGHTING, do_offensive_skill, 0, SKILL_CRANE_KICK, },
    { "crank"    , POS_RESTING , do_action   , 0, 0 },
    { "credits"  , POS_DEAD    , do_gen_ps   , 0, SCMD_CREDITS },
    { "cringe"   , POS_RESTING , do_action   , 0, 0 },
    { "criticize", POS_RESTING , do_action   , 0, 0 },
    { "crush"    , POS_RESTING , do_action   , 0, 0 },
    { "cry"      , POS_RESTING , do_action   , 0, 0 },
    { "cry_from_beyond", POS_FIGHTING, do_battlecry, 1, SCMD_CRY_FROM_BEYOND },
    { "cuddle"   , POS_RESTING , do_action   , 0, 0 },
    { "curious"  , POS_RESTING , do_action   , 0, 0 },
    { "curse"    , POS_RESTING , do_action   , 0, 0 },
    { "curtsey"  , POS_STANDING, do_action   , 0, 0 },
    { "cyberscan", POS_STANDING, do_cyberscan, 0, 0 },

    { "dance"    , POS_STANDING, do_action   , 0, 0 },
    { "dare"      , POS_RESTING , do_action   , 0, 0 },
    { "date"     , POS_DEAD    , do_date     , LVL_AMBASSADOR, SCMD_DATE },
    { "daydream" , POS_SLEEPING, do_action   , 0, 0 },
    { "dc"       , POS_DEAD    , do_dc       , LVL_GRGOD, 0 },
    { "deactivate",POS_SITTING , do_activate  , 0, SCMD_OFF },
    { "deassimilate", POS_RESTING, do_deassimilate, 0, 0 },
    { "deathtouch",POS_FIGHTING, do_offensive_skill, 0, SKILL_DEATH_TOUCH },
    { "defuse"   , POS_SITTING , do_defuse   , 0, 0 },
    { "demote"   , POS_RESTING ,  do_demote  , LVL_CAN_CLAN, 0 },
    { "deposit"  , POS_STANDING, do_not_here , 1, 0 },
    { "detach"   , POS_RESTING  , do_attach  , 0, SCMD_DETACH },
    { "debug"    , POS_DEAD    , do_gen_tog  , LVL_IMMORT, SCMD_DEBUG },
    { "de-energize", POS_FIGHTING, do_de_energize, 0, 0 },
    { "diagnose" , POS_RESTING , do_diagnose , 0, 0 },
    { "disrespect", POS_RESTING, do_action   , 0, 0 },
    { "disguise" , POS_RESTING , do_disguise, 0, 0 },
    { "disbelieve", POS_RESTING , do_action   , 0, 0 },
    { "disgusted", POS_RESTING , do_action   , 0, 0 },
    { "distance" , POS_DEAD,     do_distance , LVL_IMMORT, 0 },
    { "discharge", POS_FIGHTING, do_discharge, 0, 0 },
    { "disco"    , POS_STANDING, do_action   , 0, 0 },
    { "dismount" , POS_MOUNTED , do_dismount , 0, 0 },
    { "dismiss"  , POS_RESTING,  do_dismiss  , 0, 0 },
    { "disarm"   , POS_FIGHTING, do_disarm   , 0, 0 },
    { "display"  , POS_DEAD    , do_display  , 0, 0 },
    { "disembark", POS_STANDING, do_disembark, 0, 0 },
    { "dizzy"    , POS_RESTING , do_action   , 0, 0 },
    { "doctor"   , POS_RESTING , do_action   , 0, 0 },
    { "dodge"    , POS_FIGHTING, do_evade    , 0, 0 },
    { "doh"      , POS_RESTING , do_action   , 0, 0 },
    { "donate"   , POS_RESTING , do_drop     , 0, SCMD_DONATE },
    { "drag"     , POS_STANDING, do_drag     , 0, SKILL_DRAG },
    { "dump"     , POS_RESTING , do_action   , 0, 0 },
    { "guildonate",POS_RESTING , do_drop     , 0, SCMD_GUILD_DONATE },
    { "drive"    , POS_SITTING , do_drive    , 0, 0 },
    { "drink"    , POS_RESTING , do_drink    , 0, SCMD_DRINK },
    { "dream"    , POS_DEAD    , do_gen_comm , 0, SCMD_DREAM },
    { "drop"     , POS_RESTING , do_drop     , 0, SCMD_DROP },
    { "drool"    , POS_RESTING , do_action   , 0, 0 },
    { "drain"    , POS_FIGHTING, do_drain    , 10, 0 },
    { "duck"     , POS_RESTING , do_action   , 0, 0 },
    { "dynedit"  , POS_DEAD ,    do_dynedit  , LVL_DEMI, 0 },


    { "eat"      , POS_RESTING , do_eat      , 0, SCMD_EAT },
    { "echo"     , POS_SLEEPING, do_echo     , LVL_DEMI, SCMD_ECHO },
    { "econvert" , POS_RESTING,  do_econvert , 0, 0 },
    { "elbow"    , POS_FIGHTING, do_offensive_skill, 0, SKILL_ELBOW },
    { "elude"    , POS_STANDING, do_elude    , 0, 0 },
    { "emote"    , POS_SLEEPING, do_echo     , 1, SCMD_EMOTE },
    { ":"        , POS_SLEEPING, do_echo      , 1, SCMD_EMOTE },
    { "embrace"  , POS_STANDING, do_action   , 0, 0 },
    { "empower"  , POS_STANDING, do_empower  , 0, 0 },
    { "enter"    , POS_STANDING, do_enter    , 0, 0 },
    { "encumbrance", POS_SLEEPING, do_encumbrance, 0, 0 },
    { "enroll"   , POS_RESTING,  do_enroll   , 0, 0 },
    { "envy"     , POS_RESTING , do_action   , 0, 0 },
    { "equipment", POS_SLEEPING, do_equipment, 0, SCMD_EQ },
    { "evade"    , POS_FIGHTING, do_evade    , 0, 0 },
    { "evaluate" , POS_RESTING , do_evaluate , 0, 0 },
    { "exit"     , POS_RESTING , do_exits    , 0, 0 },
    { "exits"    , POS_RESTING , do_exits    , 0, 0 },
    { "examine"  , POS_RESTING , do_examine  , 0, 0 },
    { "exchange",  POS_SITTING , do_not_here , 0, 0 },
    { "experience",POS_DEAD,     do_experience,0, 0},
    { "explain"  , POS_RESTING,  do_action   , 0, 0 },
    { "extinguish", POS_RESTING, do_extinguish, 0, 0 },
    { "expostulate", POS_RESTING ,do_say   , 0, SCMD_EXPOSTULATE },
    { "explode"  , POS_RESTING , do_action   , 0, 0 },
    { "extract"  , POS_SITTING , do_extract , 5, 0 },
    { "force"    , POS_SLEEPING, do_force    , LVL_TIMEGOD, 0 },
    { "fart"     , POS_RESTING , do_action   , 0, 0 },
    { "faint"    , POS_RESTING,  do_action   , 0, 0 },
    { "fakerep"  , POS_RESTING , do_action   , 0, 0 },
    { "fatality" , POS_RESTING,  do_action   , 0, 0 },
    { "fill"     , POS_STANDING, do_pour     , 0, SCMD_FILL },
    { "firstaid" , POS_STANDING, do_firstaid , 0, 0 },
    { "five"     , POS_STANDING, do_action   , 0, 0 },
    { "feed"     , POS_RESTING , do_feed     , 0, 0 },
    { "feign"    , POS_FIGHTING, do_feign    , 0, 0 },
    { "flare"    , POS_RESTING,  do_action   , 0, 0 },
    { "flash"    , POS_STANDING, do_action   , 0, 0 },
    { "flee"     , POS_FIGHTING, do_flee     , 0, 0 },
    { "fly"      , POS_STANDING, do_fly      , 0, 0 },
    { "flex"     , POS_RESTING, do_action    , 0, 0 },
    { "flinch"   , POS_RESTING,  do_action   , 0, 0 },
    { "flip"     , POS_STANDING, do_action   , 0, 0 },
    { "flirt"    , POS_RESTING , do_action   , 0, 0 },
    { "flowers"  , POS_RESTING , do_action   , 0, 0 },
    { "flutter"  , POS_RESTING , do_action   , 0, 0 },
    { "follow"   , POS_RESTING , do_follow   , 0, 0 },
    { "foam"     , POS_SLEEPING, do_action   , 0, 0 },
    { "fondle"   , POS_RESTING , do_action   , 0, 0 },
    { "freak"    , POS_RESTING , do_action   , 0, 0 },
    { "freeze"   , POS_DEAD    , do_wizutil  , LVL_FREEZE, SCMD_FREEZE },
    { "french"   , POS_RESTING , do_action   , 0, 0 },
    { "frown"    , POS_RESTING , do_action   , 0, 0 },
    { "frustrated", POS_RESTING , do_action   , 0, 0 },
    { "fume"     , POS_RESTING , do_action   , 0, 0 },

    { "get"      , POS_RESTING , do_get      , 0, 0 },
    { "gagmiss"  , POS_SLEEPING, do_gen_tog  , 0, SCMD_GAGMISS },
    { "gack"     , POS_RESTING , do_action   , 0, 0 },
    { "gain"     , POS_STANDING, do_not_here , 0, 0 },
    { "gasp"     , POS_RESTING , do_action   , 0, 0 },
    { "gasify"   , POS_RESTING , do_gasify   , 50, 0 },
    { "gecho"    , POS_DEAD    , do_gecho    , LVL_GRGOD, 0 },
    { "ghug"     , POS_RESTING , do_action   , 0, 0 },
    { "give"     , POS_RESTING , do_give     , 0, 0 },
    { "giggle"   , POS_RESTING , do_action   , 0, 0 },
    { "glance"   , POS_RESTING , do_glance   , 0, 0 },
    { "glare"    , POS_RESTING , do_action   , 0, 0 },
    { "glide"    , POS_RESTING , do_action   , 0, 0 },
    { "goto"     , POS_SLEEPING, do_goto     , LVL_AMBASSADOR, 0 },
    { "gold"     , POS_SLEEPING , do_gold     , 0, 0 },
    { "gossip"   , POS_SLEEPING, do_gen_comm , 0, SCMD_GOSSIP },
    { "goose"    , POS_RESTING , do_action   , 0, 0 },
    { "gouge"    , POS_FIGHTING, do_offensive_skill, 0, SKILL_GOUGE },
    { "group"    , POS_SLEEPING, do_group    , 1, 0 },
    { "grab"     , POS_RESTING , do_grab     , 0, 0 },
    { "grats"    , POS_SLEEPING, do_gen_comm , 0, SCMD_GRATZ },
    { "greet"    , POS_RESTING , do_action   , 0, 0 },
    { "grimace"  , POS_RESTING , do_action   , 0, 0 },
    { "grin"     , POS_RESTING , do_action   , 0, 0 },
    { "groan"    , POS_RESTING , do_action   , 0, 0 },
    { "groinkick", POS_FIGHTING, do_offensive_skill, 0, SKILL_GROINKICK },
    { "grope"    , POS_RESTING , do_action   , 0, 0 },
    { "grovel"   , POS_RESTING , do_action   , 0, 0 },
    { "growl"    , POS_RESTING , do_action   , 0, 0 },
    { "grumble"  , POS_RESTING , do_action   , 0, 0 },
    { "gsay"     , POS_SLEEPING, do_gsay     , 0, 0 },
    { "gtell"    , POS_SLEEPING, do_gsay     , 0, 0 },
    { "gunset"   , POS_RESTING , do_gunset   , 0, 0 },

    { "help"     , POS_DEAD    , do_help     , 0, 0 },
    { "?"        , POS_DEAD    , do_help     , 0, 0 },
    { "hedit"    , POS_RESTING , do_hedit    , 20, 0 },
    { "hack"     , POS_STANDING, do_gen_door , 1, SCMD_HACK },
    { "halt"     , POS_DEAD    , do_gen_tog  , LVL_AMBASSADOR, SCMD_HALT },
    { "handbook" , POS_DEAD    , do_gen_ps   , LVL_AMBASSADOR, SCMD_HANDBOOK },
    { "hangover" , POS_RESTING , do_action   , 0, 0 },
    { "happy"    , POS_RESTING , do_action   , 0, 0 },
    { "hcontrol" , POS_DEAD    , do_hcontrol , LVL_DEMI, 0 },
    { "head"     , POS_RESTING , do_action   , 0, 0 },
    { "headbutt" , POS_FIGHTING, do_offensive_skill , 0, SKILL_HEADBUTT },
    { "headlights",POS_RESTING , do_not_here , 0, 0 },
    { "heineken" , POS_RESTING , do_action   , 0, 0 },
    { "hiccup"   , POS_RESTING , do_action   , 0, 0 },
    { "hide"     , POS_RESTING , do_hide     , 1, 0 },
    { "hit"      , POS_FIGHTING, do_hit      , 0, SCMD_HIT },
    { "hiptoss"  , POS_FIGHTING, do_offensive_skill , 0, SKILL_HIP_TOSS },
    { "hamstring", POS_STANDING, do_hamstring, 0, SKILL_HAMSTRING },
    { "hmmm"     , POS_RESTING , do_action   , 0, 0 },
    { "hold"     , POS_RESTING , do_grab     , 1, 0 },
    { "holler"   , POS_RESTING , do_gen_comm , 1, SCMD_HOLLER },
    { "holylight", POS_DEAD    , do_gen_tog  , LVL_AMBASSADOR, SCMD_HOLYLIGHT },
    { "honk"     , POS_RESTING , do_not_here , 0, 0 },
    { "hook"     , POS_FIGHTING, do_offensive_skill, 0, SKILL_HOOK },
    { "hop"      , POS_RESTING , do_action   , 0, 0 },
    { "howl"     , POS_RESTING , do_action   , 0, 0 },
    { "house"    , POS_RESTING , do_house    , 0, 0 },
    { "holytouch", POS_STANDING, do_holytouch, 0, 0 },
    { "hotwire"  , POS_SITTING , do_hotwire  , 0, 0 },
    { "hug"      , POS_RESTING , do_action   , 0, 0 },
    { "hump"     , POS_SITTING , do_action   , 0, 0 }, 
    { "hungry"   , POS_RESTING , do_action   , 0, 0 },
    { "hurl"     , POS_RESTING,  do_action   , 0, 0 },
    { "hush"     , POS_RESTING , do_action   , 0, 0 },

    { "inventory", POS_DEAD    , do_inventory, 0, 0 },
    { "inews"    , POS_SLEEPING, do_dyntext_show, LVL_AMBASSADOR, SCMD_DYNTEXT_INEWS },
    { "increase" , POS_STANDING, do_not_here , 0, 0 },
    { "inject"   , POS_RESTING , do_use      , 0, SCMD_INJECT },
    { "ignite"   , POS_RESTING , do_ignite   , 0, 0 },
    { "install"  , POS_DEAD    , do_install  , LVL_ETERNAL, 0},
    { "interpage", POS_DEAD    , do_interpage, LVL_GRGOD, 0 },
    { "interwho",  POS_DEAD    , do_interwho,  LVL_AMBASSADOR, 0 },
    { "intertell",  POS_DEAD    , do_intertel, LVL_AMBASSADOR, 0 },
    { "interwiz",  POS_DEAD    , do_interwiz,  LVL_AMBASSADOR, 0 },
    { "intermud",  POS_DEAD    , do_intermud,  LVL_TIMEGOD, 0 },
    { "intimidate", POS_STANDING, do_intimidate, 0, SKILL_INTIMIDATE },
    { "intone"   , POS_RESTING , do_say   , 0, SCMD_INTONE },
    { "introduce", POS_RESTING , do_action   , 0, 0 },
    { "id"       , POS_DEAD    , do_action   , 0, 0 }, 
    { "idea"     , POS_DEAD    , do_gen_write, 0, SCMD_IDEA },
    { "imbibe"   , POS_RESTING , do_drink    , 0, SCMD_DRINK },
    { "immchat"  , POS_DEAD    , do_wiznet   , LVL_AMBASSADOR, SCMD_IMMCHAT },
    { "imotd"    , POS_DEAD    , do_gen_ps   , LVL_AMBASSADOR, SCMD_IMOTD },
    { "immlist"  , POS_DEAD    , do_gen_ps   , 0, SCMD_IMMLIST },
    { "immortals", POS_DEAD    , do_gen_ps   , 0, SCMD_IMMLIST },
    { "impale"   , POS_FIGHTING, do_impale   , 0, 0 },
    { "implants" , POS_SLEEPING, do_equipment, 0, SCMD_IMPLANTS },
    { "improve"  , POS_STANDING, do_improve  , 0, 0 },
    { "info"     , POS_SLEEPING, do_gen_ps   , 0, SCMD_INFO },
    { "innocent" , POS_RESTING , do_action   , 0, 0 },
    { "insane"   , POS_RESTING , do_action   , 0, 0 },
    { "insert"   , POS_RESTING , do_insert   , 5, 0 },
    { "insult"   , POS_RESTING , do_insult   , 0, 0 },
    { "invis"    , POS_DEAD    , do_invis    , 0, 0 },

    { "jab"      , POS_FIGHTING, do_offensive_skill, 0, SKILL_JAB },
    { "jet_stream",POS_DEAD,     do_gen_tog  , LVL_CREATOR, SCMD_JET_STREAM },
    { "joint"    , POS_RESTING , do_action   , 0, 0 },
    { "judge"    , POS_RESTING , do_action   , 0, 0 },
    { "jump"     , POS_STANDING, do_move     , 0, SCMD_JUMP },
    { "junk"     , POS_RESTING , do_drop     , 0, SCMD_JUNK },

    { "kill"     , POS_STANDING, do_kill     , 0, 0 },
    { "kia"      , POS_FIGHTING, do_battlecry, 1, SCMD_KIA },
    { "kata"     , POS_STANDING, do_kata     , 0, 0 },
    { "kiss"     , POS_RESTING , do_action   , 0, 0 },
    { "kick"     , POS_FIGHTING, do_offensive_skill, 1, SKILL_KICK },
    { "kneethrust", POS_FIGHTING, do_offensive_skill, 0, SKILL_KNEE },
    { "kneel"     , POS_SITTING, do_action    , 0, 0 },
    { "knock"    , POS_SITTING,  do_knock    , 0, 0 },

    { "look"     , POS_RESTING , do_look     , 0, SCMD_LOOK },
    { "load"     , POS_RESTING , do_load     , 0, SCMD_LOAD },
    { "loadroom" , POS_SLEEPING, do_loadroom , 0, 0 },
    { "logall"    , POS_RESTING , do_gen_tog , LVL_LOGALL, SCMD_LOGALL },  
    { "login"    , POS_RESTING , do_not_here , 0, 0 },
    { "laces"    , POS_RESTING , do_action   , 0, 0 },
    { "lag"      , POS_RESTING , do_action   , 0, 0 },
    { "laugh"    , POS_RESTING , do_action   , 0, 0 },
    { "last"     , POS_DEAD    , do_last     , LVL_IMMORT, 0 },
    { "leave"    , POS_STANDING, do_leave    , 0, 0 },
    { "learn"    , POS_STANDING, do_practice , 1, 0 },
    { "lecture"  , POS_STANDING, do_lecture  , 0, 0 },
    { "leer"     , POS_RESTING,  do_action   , 0, 0 },
    { "levels"   , POS_DEAD    , do_levels   , 0, 0 },
    { "levitate" , POS_RESTING , do_fly      , 0, 0 },
    { "light"    , POS_RESTING , do_light    , 0, 0 },
    { "lightreader", POS_DEAD , do_gen_tog  , 0, SCMD_LIGHT_READ },
    { "list"     , POS_STANDING, do_not_here , 0, 0 },
    { "listen"   , POS_RESTING,  do_listen   , 0, 0 },
    { "lick"     , POS_RESTING , do_action   , 0, 0 },
    { "life"     , POS_RESTING , do_action   , 0, 0 },
    { "lightbulb", POS_RESTING , do_action   , 0, 0 },
    { "liver"    , POS_RESTING , do_action   , 0, 0 },
    { "lock"     , POS_SITTING , do_gen_door , 0, SCMD_LOCK },
    { "love"     , POS_RESTING , do_action   , 0, 0 },
    { "lungepunch",POS_FIGHTING, do_offensive_skill, 0, SKILL_LUNGE_PUNCH },
    { "lust"     , POS_RESTING , do_action   , 0, 0 }, 

    { "makeout"  , POS_RESTING , do_action   , 0, 0 },
    { "makemount", POS_STANDING, do_makemount, LVL_DEMI, 0},
    { "mload"    , POS_SLEEPING, do_mload    , LVL_TIMEGOD, 0 },
    { "mail"     , POS_STANDING, do_not_here , 1, 0 },
    { "marvel"   , POS_RESTING , do_action   , 0, 0 },
    { "marvelous", POS_RESTING , do_action   , 0, 0 },
    { "massage"  , POS_RESTING , do_action   , 0, 0 },
    { "medic"    , POS_STANDING, do_medic    , 0, 0 },
    { "meditate" , POS_SITTING, do_meditate , 0, 0 },
    { "menu"     , POS_DEAD    , do_menu     , LVL_DEMI, 0 },
    { "mischievous", POS_RESTING, do_action  , 0, 0 },
    { "miss"     , POS_RESTING ,  do_action  , 0, 0 },
    { "mount"    , POS_STANDING, do_mount    , 0, 0 },
    { "moan"     , POS_RESTING , do_action   , 0, 0 },
    { "modrian"  , POS_DEAD    , do_help     , 0, SCMD_MODRIAN },
    { "mosh"     , POS_FIGHTING, do_action   , 0 ,0 },
    { "motd"     , POS_DEAD    , do_gen_ps   , 0, SCMD_MOTD },
    { "moo"      , POS_RESTING , do_action   , 0, 0 },
    { "moon"     , POS_SITTING , do_action   , 0, 0 },
    { "mortalize", POS_SLEEPING, do_gen_tog  , LVL_AMBASSADOR, SCMD_MORTALIZE },
    { "move"     , POS_STANDING, do_move     , 0, SCMD_MOVE },
    { "mshield"  , POS_DEAD    , do_mshield  , 0, 0 },
    { "mudinfo"  , POS_DEAD    , do_mudinfo  , LVL_AMBASSADOR, 0 },
    { "mudlist"  , POS_DEAD    , do_mudlist  , LVL_AMBASSADOR, 0 },
    { "mudwipe"  , POS_DEAD    , do_mudwipe  , LVL_ENTITY, 0 },
    { "muhah"    , POS_RESTING,  do_action   , 0, 0 },
    { "mull"     , POS_RESTING , do_action   , 0, 0 },
    { "mumble"   , POS_RESTING,  do_action   , 0, 0 },
    { "murmur"   , POS_RESTING , do_say      , 0, SCMD_MURMUR },
    { "music"    , POS_DEAD    , do_gen_comm , 0, SCMD_MUSIC },
    { "mute"     , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_SQUELCH },
    { "mutter"   , POS_RESTING,  do_action   , 0, 0 },
    { "murder"   , POS_FIGHTING, do_hit      , 0, SCMD_MURDER },

    { "news"     , POS_SLEEPING, do_dyntext_show, 0, SCMD_DYNTEXT_NEWS },
    { "nervepinch",POS_FIGHTING, do_pinch    , 0, 0 },
    { "newbie"   , POS_SLEEPING, do_gen_comm , 0, SCMD_NEWBIE },
    { "nibble"   , POS_RESTING , do_action   , 0, 0 },
    { "nod"      , POS_RESTING , do_action   , 0, 0 },
    { "nodream"  , POS_DEAD    , do_gen_tog  , 0, SCMD_NODREAM },
    { "noauction", POS_DEAD    , do_gen_tog  , 0, SCMD_NOAUCTION },
    { "noaffects", POS_DEAD    , do_gen_tog  , 0, SCMD_NOAFFECTS },
    { "noclansay", POS_DEAD    , do_gen_tog  , 0, SCMD_NOCLANSAY },
    { "nogossip" , POS_DEAD    , do_gen_tog  , 0, SCMD_NOGOSSIP },
    { "nograts"  , POS_DEAD    , do_gen_tog  , 0, SCMD_NOGRATZ },
    { "nohassle" , POS_DEAD    , do_gen_tog  , LVL_AMBASSADOR, SCMD_NOHASSLE },
    { "noholler" , POS_DEAD    , do_gen_tog  , LVL_IMMORT, SCMD_NOHOLLER },
    { "noimmchat", POS_DEAD    , do_gen_tog  , LVL_AMBASSADOR, SCMD_NOIMMCHAT },
    { "nointwiz" , POS_DEAD    , do_gen_tog  , LVL_AMBASSADOR, SCMD_NOINTWIZ },
    { "noidentify",POS_DEAD    , do_gen_tog  , 1, SCMD_NOIDENTIFY },
    { "nomusic"  , POS_DEAD    , do_gen_tog  , 0, SCMD_NOMUSIC },
    { "nonewbie",POS_DEAD  , do_gen_tog  , 0, SCMD_NEWBIE_HELP },
    { "noogie"   , POS_RESTING , do_action   , 0, 0 },
    { "noproject", POS_DEAD    , do_gen_tog  , 1, SCMD_NOPROJECT },
    { "norepeat" , POS_DEAD    , do_gen_tog  , 0, SCMD_NOREPEAT },
    { "noshout"  , POS_SLEEPING, do_gen_tog  , 1, SCMD_DEAF },
    { "nosing"   , POS_DEAD    , do_gen_tog  , 0, SCMD_NOMUSIC },
    { "nospew"   , POS_DEAD    , do_gen_tog  , 1, SCMD_NOSPEW },
    { "nosummon" , POS_DEAD    , do_gen_tog  , 1, SCMD_NOSUMMON },
    { "notell"   , POS_DEAD    , do_gen_tog  , 1, SCMD_NOTELL },
    { "notitle"  , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_NOTITLE },
    { "nopost"   , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_NOPOST },
    { "notrailers", POS_DEAD   , do_gen_tog  , 1, SCMD_NOTRAILERS},
    { "nowho"    , POS_DEAD    , do_gen_tog  , LVL_CAN_CLAN, SCMD_NOWHO },
    { "nowiz"    , POS_DEAD    , do_gen_tog  , LVL_DEMI, SCMD_NOWIZ },
    { "nudge"    , POS_RESTING , do_action   , 0, 0 },
    { "nuzzle"   , POS_RESTING , do_action   , 0, 0 },

    { "order"    , POS_RESTING , do_order    , 1, 0 },
    { "oif"      , POS_RESTING , do_action   , 0, 0 },
    { "ogg"      , POS_RESTING , do_action   , 0, 0 },
    { "olc"      , POS_DEAD    , do_olc      , LVL_IMMORT, 0 },
    { "oload"    , POS_SLEEPING, do_oload    , LVL_GRGOD, 0 },
    { "offer"    , POS_STANDING, do_not_here , 1, 0 },
    { "open"     , POS_SITTING , do_gen_door , 0, SCMD_OPEN },
    { "oecho"    , POS_DEAD    , do_oecho    , LVL_GOD, 0 },
    { "oset"     , POS_DEAD    , do_oset     , LVL_CREATOR, 0 },
    { "overhaul",  POS_STANDING, do_overhaul , 1, 0 },
    { "overdrain", POS_STANDING, do_overdrain , 1, 0 },
    { "objupdate", POS_DEAD,     do_objupdate, LVL_GRIMP, 0 },

    { "pace"     , POS_STANDING, do_action   , 0, 0 },
    { "pack"     , POS_RESTING , do_put      , 0, 0 },
    { "palette"  , POS_DEAD    , do_palette  , LVL_IMMORT, 0 },
    { "palmstrike",POS_FIGHTING, do_offensive_skill, 0, SKILL_PALM_STRIKE },
    { "pant"     , POS_RESTING , do_action   , 0, 0 }, 
    { "pants"    , POS_SITTING , do_action   , 0, 0 },
    { "pat"      , POS_RESTING , do_action   , 0, 0 },
    { "page"     , POS_DEAD    , do_page     , LVL_TIMEGOD, 0 },
    { "pardon"   , POS_DEAD    , do_wizutil  , LVL_TIMEGOD, SCMD_PARDON },
    { "park"     , POS_SITTING , do_action   , 0, 0 },
    { "passout"  , POS_RESTING , do_action   , 0, 0 },
    { "peace"    , POS_SLEEPING, do_peace    , LVL_ELEMENT, 0 },
    { "peck"     , POS_RESTING , do_action   , 0, 0 },
    { "pelekick" , POS_FIGHTING, do_offensive_skill, 0, SKILL_PELE_KICK },
    { "pie"      , POS_RESTING , do_action   , 0, 0 },
    { "pkiller"  , POS_DEAD    , do_gen_tog, 0, SCMD_PKILLER },
    { "place"    , POS_RESTING , do_put      , 0, 0 },
    { "put"      , POS_RESTING , do_put      , 0, 0 },
    { "pull"     , POS_RESTING , do_action   , 0, 0 },
    { "puff"     , POS_RESTING , do_action   , 0, 0 },
    { "push"     , POS_RESTING , do_action   , 0, 0 },
    { "purchase" , POS_STANDING, do_not_here , 0, 0 },
    { "peer"     , POS_RESTING , do_action   , 0, 0 },
    { "pick"     , POS_STANDING, do_gen_door , 1, SCMD_PICK },
    { "piledrive", POS_FIGHTING, do_offensive_skill, 0, SKILL_PILEDRIVE },
    { "pimp"      , POS_RESTING , do_action   , 0, 0 },
    { "pinch"    , POS_RESTING , do_pinch   , 0, 0 }, 
    { "piss"     , POS_STANDING, do_action   , 0, 0 },
    { "pissed"   , POS_RESTING , do_action   , 0, 0 },
    { "pistolwhip", POS_FIGHTING, do_pistolwhip, 1, 0 },
    { "plant"    , POS_STANDING, do_plant    , 0, 0 },
    { "plead"    , POS_RESTING , do_action   , 0, 0 },
    { "pload"    , POS_RESTING , do_pload    , LVL_GRGOD, 0},
    { "point"    , POS_RESTING , do_point    , 0, 0 },
    { "poke"     , POS_RESTING , do_action   , 0, 0 },
    { "policy"   , POS_DEAD    , do_gen_ps   , 0, SCMD_POLICIES },
    { "ponder"   , POS_RESTING , do_action   , 0, 0 },
    { "poofin"   , POS_DEAD    , do_poofset  , LVL_AMBASSADOR, SCMD_POOFIN },
    { "poofout"  , POS_DEAD    , do_poofset  , LVL_AMBASSADOR, SCMD_POOFOUT },
    { "pose"     , POS_RESTING , do_echo     , 1, SCMD_EMOTE },
    { "pour"     , POS_STANDING, do_pour     , 0, SCMD_POUR },
    { "pout"     , POS_RESTING , do_action   , 0, 0 },
    { "poupon"   , POS_RESTING , do_action   , 0, 0 },
    { "powertrip", POS_RESTING , do_action   , 0, 0 },
    { "project"  , POS_DEAD    , do_gen_comm , 0, SCMD_PROJECT },
    { "pretend"  , POS_RESTING , do_action   , 0, 0 },
    { "promote"  , POS_RESTING , do_promote  , LVL_CAN_CLAN, 0 },
    { "prompt"   , POS_DEAD    , do_display  , 0, 0 },
    { "propose"  , POS_RESTING , do_action   , 0, 0 },
    { "practice" , POS_SLEEPING , do_practice , 1, 0 },
    { "pray"     , POS_SITTING , do_action   , 0, 0 },
    { "psiblast" , POS_FIGHTING, do_offensive_skill, 0, SKILL_PSIBLAST },
    { "psidrain" , POS_FIGHTING, do_psidrain, 0, 0 },
    { "psilocate", POS_STANDING, do_psilocate, 0, 0 },
    { "puke"     , POS_RESTING , do_action   , 0, 0 },
    { "punch"    , POS_FIGHTING, do_offensive_skill, 1, SKILL_PUNCH },
    { "punish"   , POS_RESTING , do_action   , 0, 0 },
    { "purr"     , POS_RESTING , do_action   , 0, 0 },
    { "purge"    , POS_DEAD    , do_purge    , LVL_TIMEGOD, 0 },

    { "quaff"    , POS_RESTING , do_use      , 0, SCMD_QUAFF },
    { "qcontrol" , POS_DEAD    , do_qcontrol , LVL_IMMORT, 0 },
    //  { "qecho"    , POS_DEAD    , do_qcomm    , LVL_IMMORT, SCMD_QECHO },
    { "qecho"    , POS_DEAD    , do_qecho    , LVL_IMMORT, 0 },
    { "qlog"     , POS_DEAD    , do_qlog     , LVL_IMMORT, 0 },
    //  { "quest"    , POS_DEAD    , do_gen_tog  , 0, SCMD_QUEST },
    { "quest"    , POS_DEAD    , do_quest    , 0, 0 },
    { "quake"    , POS_RESTING , do_action   , 0, 0 },
    { "qui"      , POS_DEAD    , do_quit     , 0, 0 },
    { "quit"     , POS_DEAD    , do_quit     , 0, SCMD_QUIT },
    { "qpoints"  , POS_DEAD    , do_qpoints  , 0, 0 },
    { "qpreload" , POS_DEAD    , do_qpreload , LVL_GRGOD, 0 },
    //  { "qsay"     , POS_SLEEPING , do_qcomm   , 0, SCMD_QSAY },
    { "qsay"     , POS_SLEEPING , do_qsay   , 0, 0 },
    { "qswitch"  , POS_DEAD    , do_switch   , LVL_IMMORT, SCMD_QSWITCH },

    { "raise"    , POS_RESTING,  do_action   , 0, 0 },
    { "ramble"   , POS_RESTING , do_say   , 0, SCMD_RAMBLE },
    { "reply"    , POS_SLEEPING, do_reply    , 0, 0 },
    { "repair"   , POS_SITTING , do_repair   , 0, 0 },
    { "respond"  , POS_RESTING , do_spec_comm, 0, SCMD_RESPOND },
    { "rest"     , POS_RESTING , do_rest     , 0, 0 },
    { "resign"   , POS_SLEEPING, do_resign   , 0, 0 },
    { "read"     , POS_RESTING , do_look     , 0, SCMD_READ },
    { "reboot"   , POS_SLEEPING, do_cyborg_reboot , 0, 0},
    { "rabbitpunch",POS_FIGHTING,do_offensive_skill,0,SKILL_RABBITPUNCH },
    { "reload"   , POS_DEAD    , do_reboot   , LVL_CREATOR, 0 },
    { "refill"   , POS_DEAD    , do_refill   , 1, 0 },
    { "recite"   , POS_RESTING , do_use      , 0, SCMD_RECITE },
    { "receive"  , POS_STANDING, do_not_here , 1, 0 },
    { "recharge" , POS_SITTING,  do_recharge , 0, 0 },
    { "remove"   , POS_RESTING , do_remove   , 0, 0 },
    { "remort"   , POS_STANDING, do_not_here , LVL_IMMORT, 0 },
    { "remote"   , POS_RESTING , do_action   , 0, 0 },
    { "register" , POS_SITTING,  do_not_here , 0, 0 },
    { "regurgitate", POS_RESTING , do_action   , 0, 0 },
    { "rename"   , POS_SLEEPING, do_rename   , LVL_SPIRIT, 0 },
    { "rent"     , POS_STANDING, do_not_here , 1, 0 },
    { "report"   , POS_SLEEPING, do_report   , 0, 0 },
    { "reroll"   , POS_DEAD    , do_wizutil  , LVL_GRGOD, SCMD_REROLL },
    { "rescue"   , POS_FIGHTING, do_rescue   , 1, 0 },
    { "restore"  , POS_DEAD    , do_restore  , LVL_TIMEGOD, 0 },
    { "retreat"  , POS_FIGHTING, do_retreat  , 0, 0 },
    { "return"   , POS_DEAD    , do_return   , 0, SCMD_RETURN },
    { "recorporealize"   , POS_DEAD    , do_return   , 0, SCMD_RECORP },
    { "retrieve" , POS_RESTING , do_not_here , 0, 0 },
    { "rev"      , POS_SITTING , do_action   , 0, 0 },
    { "ridgehand", POS_FIGHTING, do_offensive_skill, 0, SKILL_RIDGEHAND },
    { "roar"     , POS_RESTING , do_action   , 0, 0 },
    { "rofl"     , POS_RESTING, do_action    , 0, 0 },
    { "roll"     , POS_RESTING , do_roll     , 0, 0 },
    { "roomflags", POS_DEAD    , do_gen_tog  , LVL_AMBASSADOR, SCMD_ROOMFLAGS },
    { "rose"     , POS_RESTING , do_action   , 0, 0 },
    { "roundhouse",POS_FIGHTING, do_offensive_skill, 0,SKILL_ROUNDHOUSE },
    { "rstat"    , POS_DEAD    , do_rstat    , LVL_IMMORT, 0 },
    { "rswitch"  , POS_DEAD    , do_rswitch  , LVL_GRGOD, 0 },
    { "rub"      , POS_RESTING , do_action   , 0, 0 },
    { "ruffle"   , POS_STANDING, do_action   , 0, 0 },
    { "rules"   , POS_DEAD    , do_gen_ps   , 0, SCMD_POLICIES },
    { "run"     , POS_STANDING, do_action    , 0, 0 },

    { "say"      , POS_RESTING , do_say      , 0, SCMD_SAY },
    { "'"        , POS_RESTING , do_say      , 0, SCMD_SAY },
    { ">"        , POS_RESTING , do_say      , 0, SCMD_SAY_TO },
    { "sacrifice", POS_RESTING , do_sacrifice, 0, 0 },
    { "salute"   , POS_SITTING , do_action   , 0, 0 },
    { "save"     , POS_SLEEPING, do_save     , 0, 0 },
    { "score"    , POS_DEAD    , do_score    , 0, 0 },
    { "scan"     , POS_RESTING , do_scan     , 0, 0 },
    { "scissorkick",POS_FIGHTING,do_offensive_skill, 0, SKILL_SCISSOR_KICK },
    { "scratch"  , POS_RESTING , do_action   , 0, 0 },
    { "scream"   , POS_RESTING , do_action   , 0, 0 },
    { "screen"   , POS_DEAD    , do_screen   , 0, 0 },
    { "scuff"    , POS_RESTING , do_action   , 0, 0 },
    { "search"   , POS_DEAD    , do_examine  , 0, 0 },
    { "searchfor", POS_DEAD    , do_searchfor, LVL_DEMI, 0 },
    { "seduce"   , POS_RESTING , do_action   , 0, 0 },
    { "sell"     , POS_STANDING, do_not_here , 0, 0 },
    { "selfdestruct", POS_RESTING, do_self_destruct, 0, 0},
    { "send"     , POS_SLEEPING, do_send     , LVL_TIMEGOD, 0 },
    { "serenade" , POS_RESTING , do_action   , 0, 0 },
    { "set"      , POS_DEAD    , do_set      , LVL_GRGOD, 0 },
    { "severtell", POS_DEAD    , do_severtell, LVL_IMMORT, 0 },
    { "shout"    , POS_RESTING , do_gen_comm , 0, SCMD_SHOUT },
    { "shake"    , POS_RESTING , do_action   , 0, 0 },
    { "shin"     , POS_RESTING , do_action   , 0, 0 },
    { "shiver"   , POS_RESTING , do_action   , 0, 0 },
    { "shudder"   , POS_RESTING , do_action  , 0, 0 },
    { "show"     , POS_DEAD    , do_show     , LVL_AMBASSADOR, 0 },
    { "shower"   , POS_RESTING,  do_action   , 0, 0 },
    { "shoot"    , POS_SITTING , do_shoot    , 0, 0 },
    { "shrug"    , POS_RESTING , do_action   , 0, 0 },
    { "shutdow"  , POS_DEAD    , do_shutdown , LVL_IMPL, 0 },
    { "shutdown" , POS_DEAD    , do_shutdown , LVL_IMPL, SCMD_SHUTDOWN },
    { "shutoff"  , POS_SITTING , do_activate  , 0, SCMD_OFF },
    { "sidekick" , POS_FIGHTING, do_offensive_skill , 0, SKILL_SIDEKICK },
    { "sigh"     , POS_RESTING , do_action   , 0, 0 },
    { "silly"    , POS_RESTING , do_action   , 0, 0 },
    { "simon"    , POS_RESTING , do_action   , 0, 0 },
    { "sing"     , POS_RESTING , do_gen_comm , 0, SCMD_MUSIC },
    { "sip"      , POS_RESTING , do_drink    , 0, SCMD_SIP },
    { "sit"      , POS_RESTING , do_sit      , 0, 0 },
    { "skank"    , POS_FIGHTING, do_action   , 0, 0 },
    { "skills"   , POS_SLEEPING, do_skills   , 0, SCMD_SKILLS_ONLY },
    { "skset"    , POS_SLEEPING, do_skillset , LVL_TIMEGOD, 0 },
    { "skillset" , POS_SLEEPING, do_skillset , LVL_TIMEGOD, 0 },
    { "sleep"    , POS_SLEEPING, do_sleep    , 0, 0 },
    { "sleeper"  , POS_FIGHTING, do_sleeper  , 0, 0 },
    { "slap"     , POS_RESTING , do_action   , 0, 0 },
    { "slist"    , POS_SLEEPING, do_help     , 0, SCMD_SKILLS },
    { "sling"    , POS_FIGHTING, do_not_here , 0, 0 },
    { "slowns"   , POS_DEAD    , do_gen_tog  , LVL_GRGOD, SCMD_SLOWNS },
    { "slay"     , POS_RESTING , do_kill     , 0, SCMD_SLAY },
    { "smack"    , POS_SLEEPING , do_action   , 0, 0 },  
    { "smell"    , POS_RESTING , do_action   , 0, 0 },
    { "smile"    , POS_RESTING , do_action   , 0, 0 },
    { "smirk"    , POS_RESTING , do_action   , 0, 0 },
    { "smoke"    , POS_RESTING , do_smoke    , 0, 0 },
    { "smush"    , POS_RESTING , do_action   , 0, 0 },
    { "snicker"  , POS_RESTING , do_action   , 0, 0 },
    { "snap"     , POS_RESTING , do_action   , 0, 0 },
    { "snarl"    , POS_RESTING , do_action   , 0, 0 },
    { "snatch"   , POS_FIGHTING, do_snatch, 0, SKILL_SNATCH},
    { "sneer"    , POS_RESTING,  do_action    , 0, 0 },
    { "sneeze"   , POS_RESTING , do_action   , 0, 0 },
    { "sneak"    , POS_STANDING, do_sneak    , 1, 0 },
    { "sniff"    , POS_RESTING , do_action   , 0, 0 },
    { "snore"    , POS_SLEEPING, do_action   , 0, 0 },
    { "snort"    , POS_RESTING , do_action   , 0, 0 },
    { "snowball" , POS_STANDING, do_action   , LVL_IMMORT, 0 },
    { "snoop"    , POS_DEAD    , do_snoop    , LVL_GOD, 0 },
    { "snuggle"  , POS_RESTING , do_action   , 0, 0 },
    { "sob"      , POS_RESTING , do_action   , 0, 0 },
    { "socials"  , POS_DEAD    , do_commands , 0, SCMD_SOCIALS },
    { "soilage"  , POS_SLEEPING, do_soilage  , 0, 0 },
    { "specials" , POS_DEAD,     do_special  , LVL_IMPL, 0},
    { "specializations", POS_DEAD, do_specializations, 0, 0 },
    { "spells"   , POS_SLEEPING, do_skills   , 0, SCMD_SPELLS_ONLY },
    { "spinfist" , POS_FIGHTING, do_offensive_skill , 0, SKILL_SPINFIST },
    { "spinkick" , POS_FIGHTING, do_offensive_skill , 0, SKILL_SPINKICK },
    { "split"    , POS_SITTING , do_split    , 1, 0 },
    { "spam"     , POS_RESTING , do_action   , 0, 0 },
    { "spasm"    , POS_RESTING , do_action   , 0, 0 },
    { "spank"    , POS_RESTING , do_action   , 0, 0 },
    { "spew"     , POS_SLEEPING, do_gen_comm , 0, SCMD_SPEW },
    { "spinout"  , POS_SITTING ,  do_action   , 0, 0 },
    { "spit"     , POS_RESTING , do_action   , 0, 0 },
    { "squeal"   , POS_RESTING , do_action   , 0, 0 },
    { "squeeze"  , POS_RESTING , do_action   , 0, 0 },
    { "squirm"   , POS_RESTING , do_action   , 0, 0 },
    { "stand"    , POS_RESTING , do_stand    , 0, 0 },
    { "stagger"  , POS_STANDING, do_action   , 0, 0 },
    { "stare"    , POS_RESTING , do_action   , 0, 0 },
    { "starve"   , POS_RESTING , do_action   , 0, 0 },
    { "stat"     , POS_DEAD    , do_stat     , LVL_AMBASSADOR, 0 },
    { "status"   , POS_RESTING , do_status   , 0, 0 },
    { "stalk"    , POS_STANDING, do_stalk    , 1, 0 },
    { "steal"    , POS_SITTING , do_steal    , 1, 0 },
    { "steam"    , POS_RESTING , do_action   , 0, 0 },
    { "stretch"  , POS_SITTING , do_action   , 0, 0 },
    { "strip"    , POS_RESTING , do_action   , 0, 0 },
    { "stroke"   , POS_RESTING , do_action   , 0, 0 },
    { "strut"    , POS_STANDING, do_action   , 0, 0 },
    { "stomp"    , POS_FIGHTING, do_offensive_skill, 0, SKILL_STOMP },
    { "store"    , POS_SITTING,  do_store    , 0, 0 },
    { "stun"     , POS_FIGHTING, do_stun     , 0, 0 },
    { "sulk"     , POS_RESTING , do_action   , 0, 0 },
    { "support"  , POS_RESTING , do_action   , 0, 0 },
    { "swallow"  , POS_RESTING , do_use      , 0, SCMD_SWALLOW },
    { "switch"   , POS_DEAD    , do_switch   , LVL_ELEMENT, 0 },
    { "sweep"    , POS_RESTING , do_action   , 0, 0 },
    { "sweepkick", POS_FIGHTING, do_offensive_skill, 0, SKILL_SWEEPKICK },
    { "sweat"    , POS_RESTING , do_action   , 0, 0 },
    { "swoon"    , POS_RESTING , do_action   , 0, 0 },
    { "syslog"   , POS_DEAD    , do_syslog   , LVL_AMBASSADOR, 0 },

    { "tell"     , POS_DEAD    , do_tell     , 0, 0 },
    { "tempus"   , POS_DEAD    , do_help     , 0, 0 },
    { "terrorize", POS_FIGHTING, do_intimidate, 5, SKILL_TERRORIZE },
    { "tackle"   , POS_RESTING , do_action   , 0, 0 },
    { "take"     , POS_RESTING , do_get      , 0, 0 },
    { "talk"     , POS_RESTING , do_say      , 0, 0 },
    { "tango"    , POS_STANDING, do_action   , 0, 0 },
    { "tank"     , POS_RESTING,  do_action   , 0, 0 },
    { "taunt"    , POS_RESTING , do_action   , 0, 0 },
    { "taste"    , POS_RESTING , do_eat      , 0, SCMD_TASTE },
    { "tag"      , POS_FIGHTING, do_tag      , 0, 0 },
    { "teleport" , POS_DEAD    , do_teleport , LVL_GRGOD, 0 },
    { "thank"    , POS_RESTING , do_action   , 0, 0 },
    { "think"    , POS_RESTING , do_action   , 0, 0 },
    { "thaw"     , POS_DEAD    , do_wizutil  , LVL_FREEZE, SCMD_THAW },
    { "thrash"   , POS_RESTING , do_action   , 0, 0 },
    { "threaten" , POS_RESTING , do_action   , 0, 0 },
    { "throatstrike",POS_FIGHTING, do_offensive_skill, 0, SKILL_THROAT_STRIKE },
    { "throw"    , POS_FIGHTING, do_throw    , 0, 0 },
    { "tie"      , POS_RESTING , do_action   , 0, 0 },
    { "tight"    , POS_RESTING , do_action   , 0, 0 }, 
    { "title"    , POS_DEAD    , do_title    , 0, 0 },
    { "tickle"   , POS_RESTING , do_action   , 0, 0 },
    { "time"     , POS_DEAD    , do_time     , 0, 0 },
    { "timeout"  , POS_RESTING , do_action   , 0, 0 },
    { "tip"      , POS_RESTING , do_action   , 0, 0 },
    { "toast"    , POS_RESTING , do_action   , 0, 0 },
    { "touch"    , POS_RESTING , do_action   , 0, 0 },
    { "toggle"   , POS_DEAD    , do_toggle   , 0, 0 },
    { "toke"     , POS_RESTING , do_smoke    , 0, 0 },
    { "tornado"  , POS_FIGHTING, do_tornado_kick, 0, 0 },
    { "track"    , POS_STANDING, do_track    , 0, 0 },
    { "train"    , POS_STANDING, do_practice , 1, 0 },
    { "transfer" , POS_SLEEPING, do_trans    , LVL_DEMI, 0 },
    { "translocate", POS_RESTING, do_translocate, 20, ZEN_TRANSLOCATION },
    { "transmit" , POS_SLEEPING, do_transmit , 0, 0 },
    { "trigger"  , POS_SITTING , do_trigger  , 1, 0 },
    { "triggers" , POS_SLEEPING, do_skills   , 0, SCMD_SPELLS_ONLY },
    { "trip"     , POS_FIGHTING, do_offensive_skill, 0, SKILL_TRIP },
    { "tweak"    , POS_RESTING , do_action   , 0, 0 },
    { "twiddle"  , POS_RESTING , do_action   , 0, 0 },
    { "twitch"   , POS_SLEEPING, do_action   , 0, 0 },
    { "twirl"    , POS_SITTING , do_action   , 0, 0 },
    { "type"     , POS_RESTING , do_action   , 0, 0 },
    { "typo"     , POS_DEAD    , do_gen_write, 0, SCMD_TYPO },
    { "tongue"   , POS_RESTING , do_action   , 0, 0 },
    { "turn"     , POS_FIGHTING, do_turn     , 0, 0 },
    { "tune"     , POS_RESTING , do_tune     , 0, 0 },

    { "unlock"   , POS_SITTING , do_gen_door , 0, SCMD_UNLOCK },
    { "unload"   , POS_RESTING , do_load     , 0, SCMD_UNLOAD },
    { "ungroup"  , POS_DEAD    , do_ungroup  , 0, 0 },
    { "unban"    , POS_DEAD    , do_unban    , LVL_GRGOD, 0 },
    { "unaffect" , POS_DEAD    , do_wizutil  , LVL_TIMEGOD, SCMD_UNAFFECT },
    { "undisguise",POS_RESTING , do_disguise , 0, 1 },
    { "undress"  , POS_RESTING , do_action   , 0, 0 },
    { "uninstall", POS_DEAD    , do_uninstall, LVL_ETERNAL, 0},
    { "unalias"  , POS_DEAD    , do_unalias  , 0, 0 },
    { "uppercut" , POS_FIGHTING, do_offensive_skill , 0, SKILL_UPPERCUT },
    { "uptime"   , POS_DEAD    , do_date     , LVL_AMBASSADOR, SCMD_UPTIME },
    { "use"      , POS_SITTING , do_use      , 1, SCMD_USE },
    { "users"    , POS_DEAD    , do_users    , LVL_GOD, 0 },
    { "utter"    , POS_RESTING , do_say   , 0, SCMD_UTTER },

    { "value"    , POS_STANDING, do_not_here , 0, 0 },
    { "version"  , POS_DEAD    , do_gen_ps   , 0, SCMD_VERSION },
    { "view"     , POS_RESTING , do_look     , 0, SCMD_LOOK },
    { "visible"  , POS_RESTING , do_visible  , 1, 0 },
    { "vnum"     , POS_DEAD    , do_vnum     , LVL_IMMORT, 0 },
    { "vomit"   , POS_RESTING , do_action   , 0, 0 },  
    { "voodoo"   , POS_RESTING , do_action   , 0, 0 },
    { "vstat"    , POS_DEAD    , do_vstat    , LVL_IMMORT, 0 },

    { "wake"     , POS_SLEEPING, do_wake     , 0, 0 },
    { "wait"     , POS_RESTING , do_action   , 0, 0 },
    { "wave"     , POS_RESTING , do_action   , 0, 0 },
    { "warcry"   , POS_RESTING , do_action   , 0, 0 },
    { "wear"     , POS_RESTING , do_wear     , 0, 0 },
    { "weather"  , POS_RESTING , do_weather  , 0, 0 },
    { "change_weather",POS_DEAD, do_gen_tog  , LVL_CREATOR, SCMD_WEATHER },
    { "weigh"    , POS_RESTING , do_weigh    , 0, 0 },
    { "wedgie"   , POS_RESTING , do_action   , 0, 0 },
    { "whap"     , POS_RESTING , do_action   , 0, 0 },
    { "who"      , POS_DEAD    , do_who      , 0, 0 },
    { "whoami"   , POS_DEAD    , do_gen_ps   , 0, SCMD_WHOAMI },
    { "where"    , POS_SLEEPING, do_where    , 0, 0 },
    { "whisper"  , POS_RESTING , do_spec_comm, 0, SCMD_WHISPER },
    { "whine"    , POS_RESTING , do_action   , 0, 0 },
    { "whirlwind", POS_FIGHTING, do_whirlwind, 0, 0 },
    { "wheeze"   , POS_SLEEPING, do_action   , 0, 0 },
    { "whistle"  , POS_RESTING , do_action   , 0, 0 },
    { "wield"    , POS_RESTING , do_wield    , 0, 0 },
    { "wiggle"   , POS_STANDING, do_action   , 0, 0 },
    { "wimpy"    , POS_DEAD    , do_wimpy    , 0, 0 },
    { "wince"    , POS_RESTING , do_action   , 0, 0 },
    { "wind"     , POS_RESTING , do_not_here , 0, 0 },
    { "wink"     , POS_RESTING , do_action   , 0, 0 },
    { "wipe"     , POS_RESTING , do_action   , 0, 0 },
    { "withdraw" , POS_STANDING, do_not_here , 1, 0 },
    { "wiznet"   , POS_DEAD    , do_wiznet   , LVL_DEMI, SCMD_WIZNET },
    { ";"        , POS_DEAD    , do_wiznet   , LVL_DEMI, SCMD_WIZNET },
    { "wizhelp"  , POS_SLEEPING, do_commands , LVL_AMBASSADOR, SCMD_WIZHELP },
    { "wizlick"  , POS_RESTING , do_action   , LVL_GOD, 0 },
    { "wizlist"  , POS_DEAD    , do_gen_ps   , 0, SCMD_WIZLIST },
    { "wizards"  , POS_DEAD    , do_gen_ps   , 0, SCMD_WIZLIST },
    { "wizlock"  , POS_DEAD    , do_wizlock  , LVL_CREATOR, 0 },
    { "wizflex"  , POS_RESTING , do_action   , LVL_IMMORT, 0 },
    { "wizpiss"  , POS_RESTING , do_action   , LVL_GRGOD, 0 },
    { "wizpants" , POS_RESTING , do_action   , LVL_TIMEGOD, 0 },
    { "wonder"   , POS_RESTING , do_action   , 0, 0 },
    { "woowoo"   , POS_RESTING , do_action   , 0, 0 },
    { "wormhole" , POS_STANDING, do_translocate, 0, SKILL_WORMHOLE },
    { "worry"    , POS_RESTING , do_action   , 0, 0 },
    { "worship"  , POS_RESTING , do_action   , 0, 0 },
    { "wrestle"  , POS_RESTING , do_action   , 0, 0 },
    { "write"    , POS_RESTING , do_write    , 1, 0 },

    { "xlag"     , POS_DEAD    , do_xlag     , LVL_GOD, 0 },
    { "yae"      , POS_RESTING , do_action   , 0, 0 },
    { "yawn"     , POS_RESTING , do_action   , 0, 0 },
    { "yabba"    , POS_RESTING , do_action   , 0, 0 },
    { "yeehaw"   , POS_RESTING , do_action   , 0, 0 },
    { "yell"     , POS_RESTING , do_say , 0, SCMD_YELL },
    { "yodel"    , POS_RESTING , do_action   , 0, 0 },
    { "yowl"     , POS_RESTING , do_action   , 0, 0 },

    { "zerbert"  , POS_RESTING , do_action   , 0, 0 },
    { "zreset"   , POS_DEAD    , do_zreset   , LVL_GRGOD, 0 },
    { "zonepurge", POS_DEAD    , do_zonepurge, LVL_IMPL,  0 },
    { "rlist"    , POS_DEAD    , do_rlist    , LVL_IMMORT, 0 },
    { "olist"    , POS_DEAD    , do_olist    , LVL_IMMORT, 0 },
    { "mlist"    , POS_DEAD    , do_mlist    , LVL_IMMORT, 0 },
    { "xlist"    , POS_DEAD    , do_xlist    , LVL_IMMORT, 0 },

    { "\n", 0, 0, 0, 0 }
};	/* this must be last */


const char *fill[] =
{
    "in",
    "from",
    "with",
    "the",
    "on",
    "at",
    "to",
    "\n"
};

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

/*
 * This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function.
 */
void 
command_interpreter(struct char_data * ch, char *argument)
{
    int cmd, length;
    extern int no_specials;
    char *line;

    REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);
    REMOVE_BIT(AFF2_FLAGS(ch), AFF2_MEDITATE);
    REMOVE_BIT(AFF2_FLAGS(ch), AFF2_EVADE);
    if (GET_POS(ch) > POS_SLEEPING)
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
	line = argument+1;
    } else
	line = any_one_arg(argument, arg);

    /* otherwise, find the command */
    for (length = strlen(arg), cmd = 0; *cmd_info[cmd].command != '\n'; cmd++) {
	if (!strncmp(cmd_info[cmd].command, arg, length)) {
	    if (GET_LEVEL(ch) >= cmd_info[cmd].minimum_level || GET_IDNUM(ch) == 1) {
		break;
	    } 
	} 
    }
      
    if (*cmd_info[cmd].command == '\n') {
	switch (number(0, 10)) {
	case 0:
	    send_to_char("Huh?!?\r\n", ch);
	    break;
	case 1:
	    send_to_char("What's that?\r\n", ch);
	    break;
	case 2:
	    sprintf(buf, "%cQue?!?\r\n", 191);
	    send_to_char(buf, ch);
	    break;
	case 3:
	    send_to_char("You'll have to speak in a language I understand...\r\n", ch);
	    break;
	case 4:
	    send_to_char("I don't understand that.\r\n", ch);
	    break;
	case 5:
	    send_to_char("Wie bitte?\r\n", ch);
	    break;
	case 6:
	    send_to_char("You're talking nonsense to me.\r\n", ch);
	    break;
	case 7:
	case 8:
	case 9:
	    sprintf(buf, "Hmm, I don't understand the command '%s'.\r\n", arg);
	    send_to_char(buf, ch);
	    break;
	default:
	    send_to_char("Come again?\r\n", ch);
	    break;
	}
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
	    sprintf(buf, "CMD: %s ::%s '%s'", GET_NAME(ch), cmd_info[cmd].command, line);
	    slog(buf);
      
	}
	/* end log cmds */
    }
  
    if (PLR_FLAGGED(ch, PLR_FROZEN) && GET_LEVEL(ch) < LVL_GRIMP)
	send_to_char("You try, but the mind-numbing cold prevents you...\r\n", ch);
    else if (IS_AFFECTED_2(ch, AFF2_PETRIFIED) && GET_LEVEL(ch)<LVL_ELEMENT &&
	     (!ch->desc || !ch->desc->original)) {
	if (!number(0, 3))
	    send_to_char("You have been turned to stone!\r\n", ch);
	else if (!number(0, 2))
	    send_to_char("You are petrified!  You cannot move an inch!\r\n",ch);
	else if (!number(0, 1))
	    send_to_char("WTF??!!  Your body has been turned to solid stone!",ch);
	else
	    send_to_char("You have been turned to stone, and cannot move.\r\n",ch);
    }
    else if (cmd_info[cmd].command_pointer == NULL)
	send_to_char("Sorry, that command hasn't been implemented yet.\r\n", ch);
    else if (IS_NPC(ch) && cmd_info[cmd].minimum_level >= LVL_IMMORT)
	send_to_char("You can't use immortal commands while switched.\r\n", ch);
    else if (GET_POS(ch) < cmd_info[cmd].minimum_position && 
	     GET_LEVEL(ch) < LVL_AMBASSADOR)
	switch (GET_POS(ch)) {
	case POS_DEAD:
	    send_to_char("Lie still; you are DEAD!!! :-(\r\n", ch);
	    break;
	case POS_INCAP:
	case POS_MORTALLYW:
	    send_to_char("You are in a pretty bad shape, unable to do anything!\r\n", ch);
	    break;
	case POS_STUNNED:
	    send_to_char("All you can do right now is think about the stars!\r\n", ch);
	    break;
	case POS_SLEEPING:
	    send_to_char("In your dreams, or what?\r\n", ch);
	    break;
	case POS_RESTING:
	    send_to_char("Nah... You're resting.  Why don't you sit up first?\r\n", ch);
	    break;
	case POS_SITTING:
	    send_to_char("Maybe you should get on your feet first?\r\n", ch);
	    break;
	case POS_FIGHTING:
	    send_to_char("No way!  You're fighting for your life!\r\n", ch);
	    if (!FIGHTING(ch))
		slog("SYSERR: Char !FIGHTING(ch) while pos fighting.");
	    break;
	}
    else if (no_specials || !special(ch, cmd, cmd_info[cmd].subcmd, line)) {
	((*cmd_info[cmd].command_pointer) (ch, line, cmd, cmd_info[cmd].subcmd));
    }
    
}

/**************************************************************************
 * Routines to handle aliasing                                             *
  **************************************************************************/


struct alias *
find_alias(struct alias * alias_list, char *str)
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
free_alias(struct alias * a)
{
    if (a->alias)
	free(a->alias);
    if (a->replacement)
	free(a->replacement);
    free(a);
}

void 
add_alias(struct char_data *ch, struct alias *a)
{
    struct alias *this_alias = GET_ALIASES(ch);

    if ((!this_alias) || (strcmp(a->alias, this_alias->alias) < 0))  {
	a->next = this_alias;
	GET_ALIASES(ch) = a;
	return;
    }

    while (this_alias->next != NULL)  {
	if (strcmp(a->alias, this_alias->next->alias) < 0)  {
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
    char *repl;
    struct alias *a, *temp;

    if (IS_NPC(ch))
	return;

    repl = any_one_arg(argument, arg);

    if (!*arg) {  /* no argument specified -- list currently defined aliases */
	strcpy(buf, "Currently defined aliases:\r\n");
	if ((a = GET_ALIASES(ch)) == NULL)
	    strcat(buf, " None.\r\n");
	else {
	    while (a != NULL) {
		sprintf(buf, "%s%s%-15s%s %s\r\n", buf, CCCYN(ch, C_NRM), a->alias, 
			CCNRM(ch, C_NRM), a->replacement);
		a = a->next;
	    }
	}
	page_string(ch->desc, buf, 1);
    } 
    else { /* otherwise, add or display aliases */
	if (!*repl) {
	    if ((a = find_alias(GET_ALIASES(ch), arg)) == NULL) {
		send_to_char("No such alias.\r\n", ch);
	    }
	    else {
		sprintf(buf, "%s%-15s%s %s\r\n", CCCYN(ch, C_NRM), a->alias, 
			CCNRM(ch, C_NRM), a->replacement);
		send_to_char(buf,ch);
	    }
	}
	else {  /* otherwise, either add or redefine an alias */
	    if (!str_cmp(arg, "alias")) {
		send_to_char("You can't alias 'alias'.\r\n", ch);
		return;
	    }

	    /* is this an alias we've already defined? */
	    if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL) {
		REMOVE_FROM_LIST(a, GET_ALIASES(ch), next);
#ifdef DMALLOC
		dmalloc_verify(0);
#endif
		free_alias(a);
#ifdef DMALLOC
		dmalloc_verify(0);
#endif
	    }

	    CREATE(a, struct alias, 1);
	    a->alias = str_dup(arg);
	    delete_doubledollar(repl);
	    a->replacement = str_dup(repl);
	    if (strchr(repl, ALIAS_SEP_CHAR) || strchr(repl, ALIAS_VAR_CHAR))
		a->type = ALIAS_COMPLEX;
	    else
		a->type = ALIAS_SIMPLE;
	    add_alias(ch, a);
	    send_to_char("Alias added.\r\n", ch);
	}
    }
}

/* The interface to the outside world: do_unalias */
ACMD(do_unalias)
{
    char *repl;
    struct alias *a, *temp;

    if (IS_NPC(ch))
	return;

    repl = any_one_arg(argument, arg); /* vintage */

    if (!*arg) {  /* no argument specified -- what a dumbass */
	send_to_char ("Unalias what?\r\n", ch);
	return;
    }
    else {
	if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL) {
	    REMOVE_FROM_LIST(a, GET_ALIASES(ch), next);
#ifdef DMALLOC
	    dmalloc_verify(0);
#endif
	    free_alias(a);
#ifdef DMALLOC
	    dmalloc_verify(0);
#endif
	    send_to_char("Alias removed.\r\n", ch);
	}
	else {
	    send_to_char("No such alias Mr. Tard.\r\n", ch);
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
perform_complex_alias(struct txt_q *input_q, char *orig, struct alias *a)
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
	    buf[MAX_INPUT_LENGTH-1] = '\0';
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
	    } else
		if ((*(write_point++) = *temp) == '$') /* redouble $ for act safety */
		    *(write_point++) = '$';
	} else
	    *(write_point++) = *temp;
    }

    *write_point = '\0';
    buf[MAX_INPUT_LENGTH-1] = '\0';
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
perform_alias(struct descriptor_data * d, char *orig)
{
    char first_arg[MAX_INPUT_LENGTH], *ptr;
    struct alias *a, *tmp;

    /* bail out immediately if the guy doesn't have any aliases */
    if ((tmp = GET_ALIASES(d->character)) == NULL)
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
search_block(char *arg, const char **list, bool exact)
{
    register int i, l;
    int count = 0;

    /* Make into lower case, and get length of string */
    for (l = 0; *(arg + l); l++)
	*(arg + l) = LOWER(*(arg + l));

    if (exact) {
	for (i = 0; **(list + i) != '\n'; i++)
	    if (!strcmp(arg, *(list + i)))
		return (i);
    } else {
	if (!l)
	    l = 1;			/* Avoid "" to match the first available
					 * string */
	for (i = 0; **(list + i) != '\n' ; i++) {
	    count++;
	    if (!strncasecmp(arg, *(list + i), l))
		return (i);
	    if (count > 1000) {
		sprintf(buf,"SYSERR: search_block in unterminated list. [0] = '%s'.",
			list[0]);
		slog(buf);
		return (-1);
	    }
	}

    }

    return -1;
}


int 
is_number(char *str)
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


int fill_word(char *argument)
{
    return (search_block(argument, fill, TRUE) >= 0);
}


int reserved_word(char *argument)
{
    return (search_block(argument, reserved, TRUE) >= 0);
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
	    *(first_arg++) = LOWER(*argument);
	    argument++;
	}

	*first_arg = '\0';
    } while (fill_word(begin));

    return argument;
}


/* same as one_argument except that it doesn't ignore fill words */
char *
any_one_arg(char *argument, char *first_arg)
{
    skip_spaces(&argument);

    while (*argument && !isspace(*argument)) {
	*(first_arg++) = LOWER(*argument);
	argument++;
    }

    *first_arg = '\0';

    return argument;
}


/*
 * Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words
 */
char *two_arguments(char *argument, char *first_arg, char *second_arg)
{
    return one_argument(one_argument(argument, first_arg), second_arg);	/* :-) */
}


/*
 * determine if a given string is an abbreviation of another
 * (now works symmetrically -- JE 7/25/94)
 *
 * that was dumb.  it shouldn't be symmetrical.  JE 5/1/95
 * 
 * returnss 1 if arg1 is an abbreviation of arg2
 */
int 
is_abbrev(const char *arg1, const char *arg2)
{
    if (!*arg1)
	return 0;

    for (; *arg1 && *arg2; arg1++, arg2++)
	if (LOWER(*arg1) != LOWER(*arg2))
	    return 0;

    if (!*arg1)
	return 1;
    else
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
find_command(char *command)
{
    int cmd;

    for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
	if (!strcmp(cmd_info[cmd].command, command))
	    return cmd;

    return -1;
}


int 
special(struct char_data * ch, int cmd, int subcmd,  char *arg)
{
    register struct obj_data *i;
    register struct char_data *k;
    register struct special_search_data *srch = NULL;
    char tmp_arg[MAX_INPUT_LENGTH];
    int j, tmp_cmd = 0;
    int found = 0, result = 0;

    /* special in room? */
    if (GET_ROOM_SPEC(ch->in_room) != NULL)
	if (GET_ROOM_SPEC(ch->in_room) (ch, ch->in_room, cmd, arg, 0))
	    return 1;

    /* search special in room */
    strcpy(tmp_arg, arg); /* don't mess up the arg, in case of special */
    tmp_cmd = cmd;

    /* translate cmd for jumping, etc... */
    if (cmd_info[cmd].command_pointer == do_move &&
	subcmd > SCMD_MOVE && *tmp_arg) {
	if ((j = search_block(tmp_arg, dirs, FALSE)) >= 0) {
	    tmp_cmd = j+1;
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

    /* special in equipment list? */
    for (j = 0; j < NUM_WEARS; j++) {
	if ((i = GET_EQ(ch, j))) {
	    if (GET_OBJ_SPEC(i) && (GET_OBJ_SPEC(i) (ch, i, cmd, arg, 0)))
		return 1;
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
    }

    /* special in inventory? */
    for (i = ch->carrying; i; i = i->next_content) {
	if (GET_OBJ_SPEC(i) && (GET_OBJ_SPEC(i) (ch, i, cmd, arg, 0)))
	    return 1;
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
    for (k = ch->in_room->people; k; k = k->next_in_room)
	if (GET_MOB_SPEC(k) != NULL) {
	    if (GET_MOB_SPEC(k) (ch, k, cmd, arg, 0)) {
		return 1;
	    }
	}

    /* special in object present? */
    for (i = ch->in_room->contents; i; i = i->next_content) {
	if (GET_OBJ_SPEC(i) != NULL)
	    if (GET_OBJ_SPEC(i) (ch, i, cmd, arg, 0))
		return 1;
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



/* *************************************************************************
*  Stuff for controlling the non-playing sockets (get name, pwd etc)       *
************************************************************************* */


/* locate entry in p_table with entry->name == name. -1 mrks failed search */
int 
find_name(char *name)
{
    int i;

    for (i = 0; i <= top_of_p_table; i++) {
	if (!str_cmp((player_table + i)->name, name))
	    return i;
    }

    return -1;
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



/* deal with newcomers and other non-playing sockets */
void 
nanny(struct descriptor_data * d, char *arg)
{
    char buf[128];
    char arg1[MAX_INPUT_LENGTH];
    int player_i, load_result=0;
    char tmp_name[MAX_INPUT_LENGTH];
    struct char_file_u tmp_store;
    struct char_data *tmp_ch;
    struct descriptor_data *k, *next;
    extern struct descriptor_data *descriptor_list;
    extern struct room_data * r_mortal_start_room;
    extern struct room_data * r_electro_start_room;
    extern struct room_data * r_new_thalos_start_room;
    extern struct room_data * r_istan_start_room;
    extern struct room_data * r_tower_modrian_start_room;
    extern struct room_data * r_immort_start_room;
    extern struct room_data * r_frozen_start_room;
    extern struct room_data * r_elven_start_room;
    extern struct room_data * r_arena_start_room;
    extern struct room_data * r_doom_start_room;
    extern struct room_data * r_city_start_room;
    extern struct room_data * r_monk_start_room;
    extern struct room_data * r_solace_start_room;
    extern struct room_data * r_mavernal_start_room;
    extern struct room_data * r_dwarven_caverns_start_room;
    extern struct room_data * r_human_square_start_room;
    extern struct room_data * r_skullport_start_room;
    extern struct room_data * r_drow_isle_start_room;
    extern struct room_data * r_astral_manse_start_room;
    extern struct room_data * r_zul_dane_start_room;
    extern struct room_data * r_zul_dane_newbie_start_room;
    extern int max_bad_pws;
    struct room_data *load_room = NULL, *h_rm = NULL, *rm = NULL;
    struct clan_data *clan = NULL;
    struct clanmember_data *member = NULL;
    struct obj_data *obj = NULL, *next_obj;
    int skill_num, percent, i, j, cur, rooms;
    int polc_char = 0;

    int load_char(char *name, struct char_file_u * char_element);
    int parse_char_class_future(char *arg);
    int parse_char_class_past(char *arg);
    int parse_time_frame(char *arg);
    sh_int char_class_state = CLASS_UNDEFINED;

    skip_spaces(&arg);

    switch (STATE(d)) {
    case CON_GET_NAME:		/* wait for input of name */
	if (d->character == NULL) {
	    CREATE(d->character, struct char_data, 1);
	    clear_char(d->character);
	    CREATE(d->character->player_specials, struct player_special_data, 1);
	    GET_LOADROOM(d->character) = -1;
	    GET_HOLD_LOADROOM(d->character) = -1;
	    d->character->desc = d;
	}
	if (!*arg)
	    close_socket(d);
	else if (strlen(tmp_name) > 7 && !strncasecmp(tmp_name,"polc-",5)) {
	    strcpy(tmp_name,tmp_name+5);
	    if ((player_i = load_char(tmp_name, &tmp_store)) == -1) {
		SEND_TO_Q("Invalid name, please try another.\r\nName:  ", d);
		return;
	    }
	    sprintf(tmp_store.name,"polc-%s",tmp_store.name);
	    store_to_char(&tmp_store, d->character);
	    GET_PFILEPOS(d->character) = player_i;

	    if (!PLR_FLAGGED(d->character, PLR_POLC)) {
		free_char (d->character);
		if ((player_i = load_char(tmp_name, &tmp_store)) == -1) {
		    SEND_TO_Q("Invalid name, please try another.\r\nName:  ", d);
		    return;
		}
	    }

	    REMOVE_BIT(PLR_FLAGS(d->character), PLR_WRITING | PLR_MAILING |
		       PLR_CRYO | PLR_OLC | PLR_QUESTOR);

	    SEND_TO_Q("Password: ", d);
	    echo_off(d);

	    STATE(d) = CON_PASSWORD;
	}
	else {
	    if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 ||
		strlen(tmp_name) > MAX_NAME_LENGTH ||
		fill_word(strcpy(buf, tmp_name)) || reserved_word(buf)) {
		SEND_TO_Q("Invalid name, please try another.\r\n"
			  "Name: ", d);
		return;
	    }
	    if ((player_i = load_char(tmp_name, &tmp_store)) > -1) {

		store_to_char(&tmp_store, d->character);
		// set this up for the dyntext check
		d->old_login_time = tmp_store.last_logon;

		GET_PFILEPOS(d->character) = player_i;

		if (PLR_FLAGGED(d->character, PLR_DELETED)) {
#ifdef DMALLOC
		    dmalloc_verify(0);
#endif
		    free_char(d->character);
		    CREATE(d->character, struct char_data, 1);
		    clear_char(d->character);
		    CREATE(d->character->player_specials, struct player_special_data, 1);
		    d->character->desc = d;
		    CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
		    strcpy(d->character->player.name, CAP(tmp_name));
		    GET_PFILEPOS(d->character) = player_i;
		    GET_WAS_IN(d->character) = NULL;
		    Crash_delete_file(GET_NAME(d->character), CRASH_FILE);
		    Crash_delete_file(GET_NAME(d->character), IMPLANT_FILE);
		    sprintf(buf, "Did I get that right, %s (Y/N)? ", tmp_name);
		    SEND_TO_Q(buf, d);
		    STATE(d) = CON_NAME_CNFRM;
#ifdef DMALLOC
		    dmalloc_verify(0);
#endif
		} else {
		    /* undo it just in case they are set */
		    REMOVE_BIT(PLR_FLAGS(d->character),
			       PLR_WRITING | PLR_MAILING | PLR_CRYO | PLR_OLC | 
			       PLR_QUESTOR);
	  
		    /* make sure clan is valid */
		    if ((clan = real_clan(GET_CLAN(d->character)))) {
			if (!(member = real_clanmember(GET_IDNUM(d->character), clan)))
			    GET_CLAN(d->character) = 0;
		    } else if (GET_CLAN(d->character))
			GET_CLAN(d->character) = 0;

		    SEND_TO_Q("Password: ", d);
		    echo_off(d);

		    STATE(d) = CON_PASSWORD;
		}
	    } else {
		/* player unknown -- make new character */

		if (!Valid_Name(tmp_name)) {
		    SEND_TO_Q("Invalid name, please try another.\r\n", d);
		    SEND_TO_Q("Name: ", d);
		    return;
		}
		CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
		strcpy(d->character->player.name, CAP(tmp_name));

		sprintf(buf, "Did I get that right, %s (Y/N)? ", tmp_name);
		SEND_TO_Q(buf, d);
		STATE(d) = CON_NAME_CNFRM;
	    }
	}
	break;
    case CON_NAME_CNFRM:		/* wait for conf. of new name	 */
	if (UPPER(*arg) == 'Y') {
	    if (isbanned(d->host, buf2) >= BAN_NEW) {
		sprintf(buf, "Request for new char %s denied from [%s] (siteban)",
			GET_NAME(d->character), d->host);
		mudlog(buf, NRM, LVL_GOD, TRUE);
		SEND_TO_Q("Sorry, new characters are not allowed from your site!\r\n", d);
		STATE(d) = CON_CLOSE;
		return;
	    }
	    if (restrict) {
		SEND_TO_Q("Sorry, new players can't be created at the moment.\r\n", d);
		sprintf(buf, "Request for new char %s denied from %s (wizlock)",
			GET_NAME(d->character), d->host);
		mudlog(buf, NRM, LVL_GOD, TRUE);
		STATE(d) = CON_CLOSE;
		return;
	    }
	    sprintf(buf, "New player [%s] connect from %s.", GET_NAME(d->character),
		    d->host);
	    mudlog(buf, CMP, LVL_GOD, TRUE);

	    SEND_TO_Q("\033[H\033[J", d);
	    sprintf(buf,"Creating new character '%s'.\r\n\r\n",
		    GET_NAME(d->character));
	    SEND_TO_Q(buf, d);
	    SEND_TO_Q("What shall be your password: ", d);
	    echo_off(d);
	    STATE(d) = CON_NEWPASSWD;
	} else if (*arg == 'n' || *arg == 'N') {
	    SEND_TO_Q("Okay, what IS it, then? ", d);
	    free(d->character->player.name);
	    d->character->player.name = NULL;
	    STATE(d) = CON_GET_NAME;
	} else {
	    SEND_TO_Q("Please type Yes or No: ", d);
	}
	break;
    case CON_PASSWORD:		/* get pwd for known player	 */
	/* turn echo back on */
	echo_on(d);

	if (!*arg)
	    close_socket(d);
	else {
	    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), 
			GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
		sprintf(buf, "Bad PW: %s [%s]", GET_NAME(d->character), d->host);
		mudlog(buf, CMP, 
		       MAX(LVL_GOD, PLR_FLAGGED(d->character, PLR_INVSTART) ? 
			   GET_LEVEL(d->character) :
			   GET_INVIS_LEV(d->character)), TRUE);
		GET_BAD_PWS(d->character)++;
		save_char(d->character, real_room(GET_LOADROOM(d->character)));
		if (++(d->bad_pws) >= max_bad_pws) {	/* 3 strikes and you're out. */
		    SEND_TO_Q("Wrong password... disconnecting.\r\n", d);
		    STATE(d) = CON_CLOSE;
		} else {
		    SEND_TO_Q("Wrong password.\r\nPassword: ", d);
		    echo_off(d);
		}
		return;
	    }
	    load_result = GET_BAD_PWS(d->character);
	    GET_BAD_PWS(d->character) = 0;

	    if (!strncasecmp(GET_NAME(d->character),"polc-",5))
		polc_char = TRUE;

	    if (!polc_char)
		save_char(d->character, real_room(GET_LOADROOM(d->character)));

	    if (isbanned(d->host, buf2) == BAN_SELECT &&
		!PLR_FLAGGED(d->character, PLR_SITEOK)) {
		SEND_TO_Q("Sorry, this char has not been cleared for login from your site!\r\n", d);
		STATE(d) = CON_CLOSE;
		sprintf(buf, "Connection attempt for %s denied from %s",
			GET_NAME(d->character), d->host);
		mudlog(buf, NRM, LVL_GOD, TRUE);
		return;
	    }
	    if (GET_LEVEL(d->character) < restrict && 
		(!PLR_FLAGGED(d->character, PLR_TESTER) || restrict > LVL_ETERNAL)) {
		SEND_TO_Q("The game is temporarily restricted.. try again later.\r\n", d);
		STATE(d) = CON_CLOSE;
		sprintf(buf, "Request for login denied for %s [%s] (wizlock)",
			GET_NAME(d->character), d->host);
		mudlog(buf, NRM, LVL_GOD, TRUE);
		return;
	    }
	    /* first, check for other logins */
	    for (k = descriptor_list; k; k = next) {
		next = k->next;
		if (k != d && k->character && !k->character->in_room &&
		    GET_IDNUM(k->character) == GET_IDNUM(d->character) &&
		    !strcmp(GET_NAME(k->character),GET_NAME(d->character))) {
		    sprintf(buf, "socket close on multi login (%s)", 
			    GET_NAME(d->character));
		    slog(buf);
		    close_socket(k);
		}
	    }
	    /* check to see if this is a port olc descriptor */
      
	    /*
	     * next, check to see if this person is already logged in, but
	     * switched.  If so, disconnect the switched persona.
	     */
	    for (k = descriptor_list; k; k = k->next) {
		if (k->original && 
		    (GET_IDNUM(k->original) == GET_IDNUM(d->character))) {
		    SEND_TO_Q("Disconnecting for return to unswitched char.\r\n", k);
		    STATE(k) = CON_CLOSE;
#ifdef DMALLOC
		    dmalloc_verify(0);
#endif
		    free_char(d->character);
#ifdef DMALLOC
		    dmalloc_verify(0);
#endif
		    d->character = k->original;
		    d->character->desc = d;
		    d->original = NULL;
		    d->character->char_specials.timer = 0;
		    if (k->character)
			k->character->desc = NULL;
		    k->character = NULL;
		    k->original = NULL;
		    SEND_TO_Q("Reconnecting to unswitched char.\r\n", d);
		    REMOVE_BIT(PLR_FLAGS(d->character), PLR_MAILING | PLR_WRITING | PLR_OLC);
		    STATE(d) = CON_PLAYING;

		    if (shutdown_count > 0) {
			sprintf(buf, 
				"\r\n\007\007:: NOTICE :: Tempus will be rebooting in [%d] second%s ::\r\n",
				shutdown_count, shutdown_count == 1 ? "" : "s");
			send_to_char(buf, d->character);
		    }

		    if (has_mail(GET_IDNUM(d->character)))
			SEND_TO_Q("You have mail waiting.\r\n", d);

		    notify_cleric_moon(d->character);

		    sprintf(buf, "%s [%s] has reconnected.",
			    GET_NAME(d->character), d->host);
		    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE);
		    return;
		}
	    }

	    /* now check for linkless and usurpable */
	    for (tmp_ch = character_list; tmp_ch; tmp_ch = tmp_ch->next)
		if (!IS_NPC(tmp_ch) &&
		    GET_IDNUM(d->character) == GET_IDNUM(tmp_ch) &&
		    !strcmp(GET_NAME(tmp_ch),GET_NAME(d->character))) {
		    if (!tmp_ch->desc) {
			SEND_TO_Q("Reconnecting.\r\n", d);

			if (shutdown_count > 0) {
			    sprintf(buf, 
				    "\r\n\007\007:: NOTICE :: Tempus will be rebooting in [%d] second%s ::\r\n",
				    shutdown_count, shutdown_count == 1 ? "" : "s");
			    send_to_char(buf, d->character);
			}
	    
			sprintf(buf, "%s [%s] has reconnected.", GET_NAME(d->character),
				d->host);
			mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(d->character)),
			       TRUE);
			if (!polc_char) {
			    act("$n has reconnected.", TRUE, tmp_ch, 0, 0, TO_ROOM);
			    if (has_mail(GET_IDNUM(d->character)))
				SEND_TO_Q("You have mail waiting.\r\n", d);

			    notify_cleric_moon(d->character);

			}

			// check for dynamic text updates (news, inews, etc...)
			check_dyntext_updates(d->character, CHECKDYN_RECONNECT);


		    } else {
			sprintf(buf, "%s has re-logged:[%s]. disconnecting old socket.",
				GET_NAME(tmp_ch), d->host);
			mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(tmp_ch)), TRUE);
			SEND_TO_Q("This body has been usurped!\r\n", tmp_ch->desc);
			STATE(tmp_ch->desc) = CON_CLOSE;
			tmp_ch->desc->character = NULL;
			tmp_ch->desc = NULL;
			SEND_TO_Q("You take over your own body, already in use!\r\n", d);
			if (shutdown_count > 0) {
			    sprintf(buf, 
				    "\r\n\007\007:: NOTICE :: Tempus will be rebooting in [%d] second%s ::\r\n",
				    shutdown_count, shutdown_count == 1 ? "" : "s");
			    send_to_char(buf, d->character);
			}
	    
			if (!polc_char) {
			    if (AWAKE(tmp_ch))
				act("$n suddenly keels over in pain, surrounded by a white "
				    "aura...", TRUE, tmp_ch, 0, 0, TO_ROOM);
			    else 
				act("$n's body goes into convulsions!",TRUE,tmp_ch,0,0,TO_ROOM);
			    act("$n's body has been taken over by a new spirit!",
				TRUE, tmp_ch, 0, 0, TO_ROOM);
			}
		    }
#ifdef DMALLOC
		    dmalloc_verify(0);
#endif
		    free_char(d->character);
#ifdef DMALLOC
		    dmalloc_verify(0);
#endif
		    tmp_ch->desc = d;
		    d->character = tmp_ch;
		    tmp_ch->char_specials.timer = 0;
		    d->character->player_specials->olc_obj = NULL;
		    REMOVE_BIT(PLR_FLAGS(d->character), PLR_MAILING | PLR_WRITING |
			       PLR_OLC);
		    if (polc_char)
			STATE(d) = CON_PORT_OLC;
		    else
			STATE(d) = CON_PLAYING;
		    return;
		}
	    if (!polc_char) {
		if (!mini_mud) {
		    SEND_TO_Q("\033[H\033[J", d);
		    if (GET_LEVEL(d->character) >= LVL_IMMORT) {
			if (clr(d->character, C_NRM))
			    SEND_TO_Q(ansi_imotd, d);
			else
			    SEND_TO_Q(imotd, d);
		    } else {
			if (clr(d->character, C_NRM))     
			    SEND_TO_Q(ansi_motd, d);
			else
			    SEND_TO_Q(motd, d);
		    }
		}
	    }

	    sprintf(buf, "%s [%s] has connected.", GET_NAME(d->character), d->host);
	    mudlog(buf, CMP, MAX(LVL_GOD,
				 PLR_FLAGGED(d->character, PLR_INVSTART) ? 
				 GET_LEVEL(d->character) :
				 GET_INVIS_LEV(d->character)), TRUE);

	    if (polc_char) {
		STATE(d) = CON_PORT_OLC;
	    }
	    else {
		if (load_result) {
		    sprintf(buf, "\r\n\r\n\007\007\007"
			    "%s%d LOGIN FAILURE%s SINCE LAST SUCCESSFUL LOGIN.%s\r\n",
			    CCRED(d->character, C_SPR), load_result,
			    (load_result > 1) ? "S" : "", CCNRM(d->character, C_SPR));
		    SEND_TO_Q(buf, d);
		}
		SEND_TO_Q("\r\n*** PRESS RETURN: ", d);
		STATE(d) = CON_RMOTD;
	    }
	}
	break;

    case CON_NEWPASSWD:
    case CON_CHPWD_GETNEW:
	if (!*arg || strlen(arg) > MAX_PWD_LENGTH || strlen(arg) < 3 ||
	    !str_cmp(arg, GET_NAME(d->character))) {
	    SEND_TO_Q("\r\nIllegal password.\r\n", d);
	    SEND_TO_Q("Password: ", d);
	    return;
	}
	strncpy(GET_PASSWD(d->character), CRYPT(arg, GET_NAME(d->character)), MAX_PWD_LENGTH);
	*(GET_PASSWD(d->character) + MAX_PWD_LENGTH) = '\0';

	SEND_TO_Q("\r\nPlease retype password: ", d);
	if (STATE(d) == CON_NEWPASSWD)
	    STATE(d) = CON_CNFPASSWD;
	else
	    STATE(d) = CON_CHPWD_VRFY;

	break;

    case CON_CNFPASSWD:
    case CON_CHPWD_VRFY:
	if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character),
		    MAX_PWD_LENGTH)) {
	    SEND_TO_Q("\r\nPasswords don't match... start over.\r\n", d);
	    SEND_TO_Q("Password: ", d);
	    if (STATE(d) == CON_CNFPASSWD)
		STATE(d) = CON_NEWPASSWD;
	    else
		STATE(d) = CON_CHPWD_GETNEW;
	    return;
	}
	echo_on(d);

	if (STATE(d) == CON_CNFPASSWD) {
	    SEND_TO_Q("This game supports ANSI color standards.\r\n"
		      "Is your terminal compatible to receive colors? [y/n]  ", d);
	    STATE(d) = CON_QCOLOR;
	} else {
	    save_char(d->character, real_room(GET_LOADROOM(d->character)));
	    echo_on(d);
	    SEND_TO_Q("\033[H\033[J", d);
	    SEND_TO_Q("\r\nPassword changed.\r\n", d);
	    show_menu(d);
	    STATE(d) = CON_MENU;
	}

	break;

    case CON_QCOLOR:
	switch (*arg) {
	case 'y':
	case 'Y':
	    SET_BIT(PRF_FLAGS(d->character), PRF_COLOR_2);
	    sprintf(buf, "\r\nYou will receive colors in the %sNORMAL%s format.\r\n",
	            CCCYN(d->character, C_NRM), CCNRM(d->character, C_NRM));
	    SEND_TO_Q(buf, d);
	    SEND_TO_Q(CCYEL(d->character, C_NRM), d);
	    SEND_TO_Q("If this is not acceptable, you may change modes at any time.\r\n\r\n", d);
	    SEND_TO_Q(CCNRM(d->character, C_NRM), d);
	    // default settings
	    SET_BIT(PRF_FLAGS(d->character), PRF_COMPACT);
	    SET_BIT(PRF_FLAGS(d->character), PRF_NOSPEW);
	    break;
	case 'n':
	case 'N':
	    SEND_TO_Q("\r\nVery well.  You will not receive colored text.\r\n", d);
	    break;
	default:
	    SEND_TO_Q("\r\nYou must specify either 'Y' or 'N'.\r\n", d);
	    return;
	}   

	SET_BIT(PRF2_FLAGS(d->character), PRF2_LIGHT_READ | 
		PRF2_AUTOPROMPT | PRF2_NEWBIE_HELPER);
	sprintf(buf, "%sWhat do you choose as your sex %s(M/F)%s?%s ", 
		CCCYN(d->character, C_NRM), CCGRN(d->character, C_NRM), 
		CCCYN(d->character, C_NRM), CCNRM(d->character, C_NRM));
	SEND_TO_Q(buf, d);
	STATE(d) = CON_QSEX;
	return;

    case CON_QSEX:		/* query sex of new user	 */

	if (is_abbrev(arg, "male"))
	    d->character->player.sex = SEX_MALE;
	else if (is_abbrev(arg, "female"))
	    d->character->player.sex = SEX_FEMALE;
	else {
	    SEND_TO_Q("That is not a sex...\r\n", d);
	    sprintf(buf, "%sWhat IS your sex?%s ", CCCYN(d->character, C_NRM),
		    CCNRM(d->character, C_NRM));
	    SEND_TO_Q(buf, d);
	    return;
	    break;
	}

	SEND_TO_Q("\033[H\033[J", d);    
	show_time_menu(d);
	STATE(d) = CON_QTIME_FRAME;
	break;
    
	/*    GET_HOME(d->character) = HOME_MODRIAN;   assigns past to char  */
	/*    SEND_TO_Q("\033[H\033[J", d);    
	      show_race_menu_past(d);
	      SEND_TO_Q(CCCYN(d->character, C_NRM), d);
	      SEND_TO_Q("\r\nYour Choice: ", d);
	      SEND_TO_Q(CCNRM(d->character, C_NRM), d);
	      STATE(d) = CON_RACE;
	      break;    */

    case CON_QTIME_FRAME:
    
	if ((GET_HOME(d->character) = parse_time_frame(arg)) == HOME_UNDEFINED) {
	    SEND_TO_Q("\033[H\033[J", d);
	    SEND_TO_Q("\r\nThat's not a choice.\r\n\r\n", d);
	    show_time_menu(d);
	    return;
	}

	SEND_TO_Q("\033[H\033[J", d);
	if (GET_HOME(d->character) == HOME_ELECTRO) {
	    show_race_menu_future(d);
	    STATE(d) = CON_RACE_FUTURE;
	} else {
	    show_race_menu_past(d);
	    STATE(d) = CON_RACE_PAST;
	}
	break;

    case CON_RACEHELP_P:
	SEND_TO_Q("\033[H\033[J", d);
	show_race_menu_past(d);
	STATE(d) = CON_RACE_PAST;
	break;

    case CON_RACEHELP_F:
	SEND_TO_Q("\033[H\033[J", d);
	show_race_menu_future(d);
	STATE(d) = CON_RACE_FUTURE;
	break;

    case CON_RACE_PAST:
	if ((GET_RACE(d->character) = parse_race_past(d, arg)) == -1) {
	    SEND_TO_Q("\033[H\033[J", d);    
	    SEND_TO_Q(CCRED(d->character, C_NRM), d);
	    SEND_TO_Q("\r\nThat's not a choice.\r\n", d);
	    SEND_TO_Q(CCNRM(d->character, C_NRM), d);
	    show_race_menu_past(d);
	    break;
	} else if (GET_RACE(d->character) == -2) {
	    STATE(d) = CON_RACEHELP_P;
	    break;
	}

	SEND_TO_Q("\033[H\033[J", d);    
	show_char_class_menu_past(d);
	SEND_TO_Q(CCCYN(d->character, C_NRM), d);
	SEND_TO_Q("\r\nClass: ", d);
	SEND_TO_Q(CCNRM(d->character, C_NRM), d);
	STATE(d) = CON_QCLASS;
	break;

    case CON_RACE_FUTURE:
	if ((GET_RACE(d->character) = parse_race_future(d, arg)) == -1) {
	    SEND_TO_Q("\033[H\033[J", d);    
	    SEND_TO_Q(CCRED(d->character, C_NRM), d);
	    SEND_TO_Q("\r\nThat's not a choice.\r\n", d);
	    SEND_TO_Q(CCNRM(d->character, C_NRM), d);
	    show_race_menu_future(d);
	    break;
	} else if (GET_RACE(d->character) == -2) {
	    STATE(d) = CON_RACEHELP_F;
	    break;
	}

	SEND_TO_Q("\033[H\033[J", d);    
	show_char_class_menu_future(d);
	SEND_TO_Q(CCCYN(d->character, C_NRM), d);
	SEND_TO_Q("\r\nClass: ", d);
	SEND_TO_Q(CCNRM(d->character, C_NRM), d);
	STATE(d) = CON_QCLASS;
	break;

    case CON_CLASSHELP_P:
	show_char_class_menu_past(d);
	STATE(d) = CON_QCLASS;
	break;

    case CON_CLASSHELP_F:
	show_char_class_menu_future(d);
	STATE(d) = CON_QCLASS;
	break;

    case CON_QCLASS:
	if (GET_HOME(d->character) == HOME_ELECTRO) {
	    if ((GET_CLASS(d->character)=parse_char_class_future(arg))==CLASS_UNDEFINED){
		SEND_TO_Q("\033[H\033[J", d);    
		SEND_TO_Q(CCRED(d->character, C_NRM), d);
		SEND_TO_Q("\r\nThat's not a char_class.\r\n", d);
		SEND_TO_Q(CCNRM(d->character, C_NRM), d);
		show_char_class_menu_future(d);
		SEND_TO_Q(CCCYN(d->character, C_NRM), d);
		SEND_TO_Q("Class:  \r\n", d);
		SEND_TO_Q(CCNRM(d->character, C_NRM), d);
		return;
	    } else if (IS_HALF_ORC(d->character) && IS_PSIONIC(d->character)) {
		SEND_TO_Q(CCGRN(d->character, C_NRM), d);
		SEND_TO_Q("\r\nThat char_class is not allowed to your race!\r\n", d);
		show_race_restrict_past(d);
		show_race_menu_past(d);
		STATE(d) = CON_RACE_FUTURE;
		break;
	    } 
	}
    
	else if (GET_HOME(d->character) == HOME_MODRIAN) {
	    if ((GET_CLASS(d->character) = parse_char_class_past(arg))==CLASS_UNDEFINED) 
	    {
		SEND_TO_Q("\033[H\033[J", d);    
		SEND_TO_Q(CCRED(d->character, C_NRM), d);
		SEND_TO_Q("\r\nThat's not a char_class.\r\n", d);
		SEND_TO_Q(CCNRM(d->character, C_NRM), d);
		show_char_class_menu_past(d);
		SEND_TO_Q(CCCYN(d->character, C_NRM), d);
		SEND_TO_Q("Class:  \r\n", d);
		SEND_TO_Q(CCNRM(d->character, C_NRM), d);
		return;
	    } else if ((IS_DWARF(d->character) &&
			(IS_MAGE(d->character) ||
			 IS_MONK(d->character) || IS_RANGER(d->character))) ||
		       (IS_HALF_ORC(d->character) &&
			(IS_MAGE(d->character) || IS_CLERIC(d->character) ||
			 IS_MONK(d->character) || IS_KNIGHT(d->character))) ||
		       (IS_ELF(d->character) && IS_BARB(d->character)) ||
		       (IS_DROW(d->character) && (IS_BARB(d->character) ||
						  IS_MONK(d->character))) ||
		       (IS_TABAXI(d->character) && IS_KNIGHT(d->character)) ||
		       (IS_MINOTAUR(d->character) && 
			(IS_KNIGHT(d->character) || IS_THIEF(d->character) ||
			 IS_MONK(d->character)))) {
		SEND_TO_Q(CCGRN(d->character, C_NRM), d);
		SEND_TO_Q("\r\nThat char_class is not allowed to your race!\r\n", d);
		show_race_restrict_past(d);
		show_race_menu_past(d);
		STATE(d) = CON_RACE_PAST;
		break;
	    } 
	} else
	    GET_CLASS(d->character) = load_result;
    

	SEND_TO_Q("\r\nALIGNMENT is a measure of your philosophies and morals.\r\n\r\n", d);

	if (GET_CLASS(d->character) == CLASS_MONK) {
	    SEND_TO_Q("The monastic ideology requires that you remain neutral in alignment.\r\nTherefore you begin your life with a perfect neutrality.\r\n", d);
	    GET_ALIGNMENT(d->character) = 0;
	    if (GET_HOME(d->character) != HOME_ELECTRO) {
		GET_HOME(d->character) = HOME_MONK;
		STATE(d) = CON_QHOME_PAST;
	    } else {
		SEND_TO_Q("\033[H\033[J", d);
		STATE(d) = CON_QHOME_FUTURE;
		show_future_home_menu(d);
	    }
	    break;
	} else if (IS_DROW(d->character)) {
	    SEND_TO_Q("The Drow race is inherently evil.  Thus you begin your life as evil.\r\n", d);
	    GET_ALIGNMENT(d->character) = -666;
	    GET_HOME(d->character) = HOME_SKULLPORT;
	    STATE(d) = CON_QHOME_PAST;
	    break;
	}

	sprintf(buf, "%sDo you wish to be %sGood%s, %sNeutral%s, or %sEvil%s?%s ",
		CCCYN(d->character, C_NRM), CCGRN(d->character, C_NRM),
		CCCYN(d->character, C_NRM), CCGRN(d->character, C_NRM),
		CCCYN(d->character, C_NRM), CCGRN(d->character, C_NRM),
		CCCYN(d->character, C_NRM), CCNRM(d->character, C_NRM));
	SEND_TO_Q(buf, d);
    
	STATE(d) = CON_QALIGN;
	break;
    
    
    case CON_QALIGN:
	if (GET_CLASS(d->character) != CLASS_MONK) {
	    if (is_abbrev(arg, "evil"))
		d->character->char_specials.saved.alignment = -666;
	    else if (is_abbrev(arg, "good"))
		d->character->char_specials.saved.alignment = 777;
	    else if (is_abbrev(arg, "neutral")) {
		if (GET_CLASS(d->character) == CLASS_KNIGHT ||
		    GET_CLASS(d->character) == CLASS_CLERIC) {
		    SEND_TO_Q(CCGRN(d->character, C_NRM), d);
		    SEND_TO_Q("Characters of your char_class must be either Good, or Evil.\r\n", d);
		    SEND_TO_Q(CCCYN(d->character, C_NRM), d);
		    SEND_TO_Q("Please choose one of these. \r\n", d);
		    SEND_TO_Q(CCNRM(d->character, C_NRM), d);
		    return;
		}
		d->character->char_specials.saved.alignment = 0;
	    } else {
		SEND_TO_Q(CCRED(d->character, C_NRM), d);
		SEND_TO_Q("Thats not a choice.\r\n", d); 
		SEND_TO_Q(CCNRM(d->character, C_NRM), d);
		sprintf(buf,
			"%sDo you wish to be %sGood%s, %sNeutral%s, or %sEvil%s?%s ",
			CCCYN(d->character, C_NRM), CCGRN(d->character, C_NRM),
			CCCYN(d->character, C_NRM), CCGRN(d->character, C_NRM),
			CCCYN(d->character, C_NRM), CCGRN(d->character, C_NRM),
			CCCYN(d->character, C_NRM), CCNRM(d->character, C_NRM));
		SEND_TO_Q(buf, d);
		break;
	    }
	}

	if (GET_CLASS(d->character) == CLASS_CLERIC)
	    GET_DEITY(d->character) = DEITY_GUIHARIA;
	else if (GET_CLASS(d->character) == CLASS_KNIGHT)
	    if (IS_GOOD(d->character))
		GET_DEITY(d->character) = DEITY_JUSTICE;
	    else
		GET_DEITY(d->character) = DEITY_ARES;
	else 
	    GET_DEITY(d->character) = DEITY_GUIHARIA;

	SEND_TO_Q("\033[H\033[J", d);    

	if (GET_HOME(d->character) == HOME_ELECTRO) {
	    show_future_home_menu(d);
	    STATE(d) = CON_QHOME_FUTURE;
	} else {
	    show_past_home_menu(d);
	    STATE(d) = CON_QHOME_PAST;
	}
	break;

    case CON_HOMEHELP_P:
	SEND_TO_Q("\033[H\033[J", d);
	show_past_home_menu(d);
	STATE(d) = CON_QHOME_PAST;
	break;

    case CON_HOMEHELP_F:
	SEND_TO_Q("\033[H\033[J", d);
	show_future_home_menu(d);
	STATE(d) = CON_QHOME_FUTURE;
	break;

    case CON_QHOME_PAST:
    case CON_QHOME_FUTURE:

	if (STATE(d) == CON_QHOME_FUTURE) {
	    if ((GET_HOME(d->character) = parse_future_home(d, arg)) == -1) {
		SEND_TO_Q("\033[H\033[J", d);    
		SEND_TO_Q("That is not a valid choice.", d);
		show_future_home_menu(d);
		break;
	    } else if (GET_HOME(d->character) == -2) {
		STATE(d) = CON_HOMEHELP_F;
		break;
	    }
	} else {
	    if (GET_CLASS(d->character) != CLASS_MONK && !IS_DROW(d->character)) {
		if ((GET_HOME(d->character) = parse_past_home(d, arg)) == -1) {
		    SEND_TO_Q("\033[H\033[J", d);    
		    SEND_TO_Q("That is not a valid choice.", d);
		    show_past_home_menu(d);
		    break;
		} else if (GET_HOME(d->character) == HOME_ELVEN_VILLAGE && 
			   (!IS_ELF(d->character) || IS_EVIL(d->character))) {
		    SEND_TO_Q("\033[H\033[J", d);    
		    SEND_TO_Q("Only good elves are allowed to reside in this place.\r\n",d);
		    show_past_home_menu(d);
		    break;
		} else if (GET_HOME(d->character) == -2) {
		    STATE(d) = CON_HOMEHELP_P;
		    break;
		}
	    }
	}

	population_record[GET_HOME(d->character)]++;
    
	if (GET_PFILEPOS(d->character) < 0)
	    GET_PFILEPOS(d->character) = create_entry(GET_NAME(d->character));
	init_char(d->character);
	roll_real_abils(d->character);
	save_char(d->character, NULL);

	SEND_TO_Q("\033[H\033[J", d);
	SEND_TO_Q("The following is a description of your character's attributes:\r\n\r\n", d);
	print_attributes_to_buf(d->character, buf2);
	SEND_TO_Q(buf2, d);
	SEND_TO_Q("\r\n", d);
	SEND_TO_Q("Would you like to REROLL or KEEP these attributes?\r\n", d);
	STATE(d) = CON_QREROLL;
	break;

    case CON_QREROLL:

	if (is_abbrev(arg, "reroll")) {
	    roll_real_abils(d->character);
	    print_attributes_to_buf(d->character, buf2);
	    SEND_TO_Q("\033[H\033[J", d);
	    SEND_TO_Q("Character attributes:\r\n\r\n", d);
	    SEND_TO_Q(buf2, d);
	    SEND_TO_Q("\r\n", d);
	    SEND_TO_Q("Would you like to REROLL or KEEP these attributes?\r\n", d);
	    d->wait = 4;
	    break;
	} else if (is_abbrev(arg, "keep")) {
	    save_char(d->character, NULL);

	    SEND_TO_Q("\033[H\033[J", d);
	    if (clr(d->character, C_NRM))     
		SEND_TO_Q(ansi_motd, d);
	    else
		SEND_TO_Q(motd, d);
      
	    SEND_TO_Q(CCYEL(d->character, C_NRM), d);
	    SEND_TO_Q("\r\n\n*** PRESS RETURN: ", d);
	    SEND_TO_Q(CCNRM(d->character, C_NRM), d);
	    STATE(d) = CON_RMOTD;

	    sprintf(buf, "%s [%s] new player.", GET_NAME(d->character), d->host);
	    mudlog(buf, NRM, LVL_GOD, TRUE);

	    break;
	} else
	    SEND_TO_Q("You must type 'reroll' or 'keep'.\r\n", d);
	break;

    case CON_RMOTD:		/* read CR after printing motd	 */
	if (!mini_mud)	SEND_TO_Q("\033[H\033[J", d);
	show_menu(d);
	STATE(d) = CON_MENU;
	break;

    case CON_MENU:		/* get selection from main menu	 */
	switch (*arg) {
	case '0':
	    SEND_TO_Q("\033[H\033[J", d);
	    SEND_TO_Q("Connection terminated.\r\n", d);
	    close_socket(d);
	    break;

	case '1':
	    /* this code is to prevent people from multiply logging in */
	    for (k = descriptor_list; k; k = next) {
		next = k->next;
		if (!k->connected && k->character &&
		    !str_cmp(GET_NAME(k->character), GET_NAME(d->character))) {
		    SEND_TO_Q("Your character has been deleted.\r\n", d);
		    STATE(d) = CON_CLOSE;
		    return;
		}
	    }
	    if (!mini_mud)	SEND_TO_Q("\033[H\033[J", d);
	    if (!d->character->player.description && !GET_LEVEL(d->character)) {
		SEND_TO_Q("Other players will usually be able to determine your general\r\n"
			  "size, as well as your race and gender, by looking at you.  What\r\n"
			  "else is noticable about your character?\r\n", d);
		SEND_TO_Q("Terminate with a '@' on a new line.\r\n", d);
		SEND_TO_Q("Enter a * on a new line to enter TED\r\n", d);
		SEND_TO_Q(" [+--------+---------+---------+--------"
			  "-+---------+---------+---------+------+]\r\n", d);
		d->str = &d->character->player.description;
		d->max_str = MAX_CHAR_DESC-1;
		STATE(d) = CON_EXDESC;
		break;
	    }

	    reset_char(d->character);

	    // if we're not a new char, check loadroom and rent
	    if (GET_LEVEL(d->character)) {

		d->character->in_room = real_room(GET_LOADROOM(d->character));

		if (PLR_FLAGGED(d->character, PLR_INVSTART))
		    GET_INVIS_LEV(d->character) = (GET_LEVEL(d->character) > LVL_LUCIFER ?
						   LVL_LUCIFER : GET_LEVEL(d->character));
	  
		if ((load_result = Crash_load(d->character)) && 
		    d->character->in_room == NULL) {
		    d->character->in_room = NULL;
		}
	    }

	    // otherwise null the loadroom
	    else {

		d->character->in_room = NULL;

	    }

	    save_char(d->character, NULL);
	    send_to_char(CCRED(d->character, C_NRM), d->character);
	    send_to_char(CCBLD(d->character, C_NRM), d->character);
	    send_to_char(WELC_MESSG, d->character);
	    send_to_char(CCNRM(d->character, C_NRM), d->character);
	    d->character->next = character_list;
	    character_list = d->character;

	    load_room = NULL;
	    if (PLR_FLAGGED(d->character, PLR_FROZEN))
		load_room = r_frozen_start_room;
	    else if (PLR_FLAGGED(d->character, PLR_LOADROOM)) {
		if ((load_room = real_room(GET_LOADROOM(d->character))) == NULL)
		    load_room = NULL;
		else if (!House_can_enter(d->character, load_room->number) ||
			 !clan_house_can_enter(d->character, load_room))
		    load_room = NULL;
	    } 
	    if (load_room == NULL)  {
		if (GET_LEVEL(d->character) >= LVL_IMMORT) {
		    if (d->character->in_room == NULL)
			load_room = r_immort_start_room;
		    else if ((load_room = real_room(d->character->in_room->number)) == 
			     NULL || !House_can_enter(d->character, load_room->number) ||
			     !clan_house_can_enter(d->character, load_room))
			load_room = r_immort_start_room;
		} else {
		    if (d->character->in_room == NULL || 
			(load_room = real_room(d->character->in_room->number)) == NULL) {
			if (GET_HOME(d->character) == HOME_ELECTRO)
			    load_room = r_electro_start_room;
			else if (GET_HOME(d->character) == HOME_NEWBIE_TOWER) {
			    if (GET_LEVEL(d->character) > 5) {
				population_record[HOME_NEWBIE_TOWER]--;
				GET_HOME(d->character) = HOME_MODRIAN;
				population_record[HOME_MODRIAN]--;
				load_room = r_mortal_start_room;
			    } else
				load_room = r_tower_modrian_start_room;
			}
			else if (GET_HOME(d->character) == HOME_NEW_THALOS)
			    load_room = r_new_thalos_start_room;
			else if (GET_HOME(d->character) == HOME_ELVEN_VILLAGE)
			    load_room = r_elven_start_room;
			else if (GET_HOME(d->character) == HOME_ISTAN)
			    load_room = r_istan_start_room;
			else if (GET_HOME(d->character) == HOME_ARENA)
			    load_room = r_arena_start_room;
			else if (GET_HOME(d->character) == HOME_DOOM)
			    load_room = r_doom_start_room;
			else if (GET_HOME(d->character) == HOME_CITY)
			    load_room = r_city_start_room;
			else if (GET_HOME(d->character) == HOME_MONK)
			    load_room = r_monk_start_room;
			else if (GET_HOME(d->character) == HOME_SOLACE_COVE)
			    load_room = r_solace_start_room;
			else if (GET_HOME(d->character) == HOME_MAVERNAL)
			    load_room = r_mavernal_start_room;
			else if (GET_HOME(d->character) == HOME_DWARVEN_CAVERNS)
			    load_room = r_dwarven_caverns_start_room;
			else if (GET_HOME(d->character) == HOME_HUMAN_SQUARE)
			    load_room = r_human_square_start_room;
			else if (GET_HOME(d->character) == HOME_SKULLPORT)
			    load_room = r_skullport_start_room;
			else if (GET_HOME(d->character) == HOME_DROW_ISLE)
			    load_room = r_drow_isle_start_room;
			else if (GET_HOME(d->character) == HOME_ASTRAL_MANSE)
			    load_room = r_astral_manse_start_room;
			// zul dane
			else if (GET_HOME(d->character) == HOME_ZUL_DANE) {
			    // newbie start room for zul dane
			    if (GET_LEVEL(d->character) > 5)
				load_room = r_zul_dane_newbie_start_room;
			    else
				load_room = r_zul_dane_start_room;
			}
			else
			    load_room = r_mortal_start_room;
		    }
		}
	    }
	    char_to_room(d->character, load_room);
	    load_room->zone->enter_count++;

	    if (!PLR_FLAGGED(d->character, PLR_LOADROOM) &&
		GET_HOLD_LOADROOM(d->character) > 0 &&
		real_room(GET_HOLD_LOADROOM(d->character))) {
		GET_LOADROOM(d->character) = GET_HOLD_LOADROOM(d->character);
		SET_BIT(PLR_FLAGS(d->character), PLR_LOADROOM);
		GET_HOLD_LOADROOM(d->character) = NOWHERE;
	    }
	    show_mud_date_to_char(d->character);
	    send_to_char("\r\n", d->character);

	    STATE(d) = CON_PLAYING;
	    if (!GET_LEVEL(d->character)) {
		send_to_char(START_MESSG, d->character);
		sprintf(buf, 
			" ***> New adventurer %s has entered the realm. <***\r\n", 
			GET_NAME(d->character));
		send_to_newbie_helpers(buf);
		do_start(d->character, 0);

		d->old_login_time = time(0);  // clear login time so we dont get news updates
    
	    } 
	    else if (load_result == 2) {	/* rented items lost */
		send_to_char("\r\n\007You could not afford your rent!\r\n"
			     "Some of your possesions have been repossesed to cover your bill!\r\n",
			     d->character);
	    }
      
	    act("$n has entered the game.", TRUE, d->character, 0, 0, TO_ROOM);
	    look_at_room(d->character, d->character->in_room, 0);

	    // temporary fix to quest flag
	    REMOVE_BIT(PRF_FLAGS(d->character), PRF_QUEST);

	    if (has_mail(GET_IDNUM(d->character)))
		send_to_char("You have mail waiting.\r\n", d->character);

	    notify_cleric_moon(d->character);

	    // check for dynamic text updates (news, inews, etc...)
	    check_dyntext_updates(d->character, CHECKDYN_UNRENT);
      
	    if (shutdown_count > 0) {
		sprintf(buf, 
			"\r\n\007\007:: NOTICE :: Tempus will be rebooting in [%d] second%s ::\r\n",
			shutdown_count, shutdown_count == 1 ? "" : "s");
		send_to_char(buf, d->character);
	    }

	    for (i = 0; i < num_of_houses; i++) {
		if (house_control[i].mode != HOUSE_PUBLIC &&
		    house_control[i].owner1 == GET_IDNUM(d->character) &&
		    house_control[i].rent_sum && 
		    (h_rm = real_room(house_control[i].house_rooms[0]))) {
		    for (j=0, percent=1, rooms=0; j < house_control[i].num_of_rooms; j++) {
			if ((rm = real_room(house_control[i].house_rooms[j]))) {
			    for (cur=0, obj = rm->contents; obj; obj = obj->next_content)
				cur += recurs_obj_contents(obj, NULL);
			    if (cur > MAX_HOUSE_ITEMS) {
				sprintf(buf, "WARNING:  House room [%s%s%s] contains %d items.\r\n", 
					CCCYN(d->character, C_NRM), rm->name, CCNRM(d->character, C_NRM), cur);
				SEND_TO_Q(buf, d);

				// add one factor for every MAX_HOUSE_ITEMS over
				percent += (cur / MAX_HOUSE_ITEMS);
				rooms++;

			    }
			}
		    }
		    if (percent > 1) {
			sprintf(buf, "You exceeded the house limit in %d room%s.\n"
				"Your house rent is multiplied by a factor of %d.\n", 
				rooms, percent > 1 ? "s" : "", percent);
			SEND_TO_Q(buf, d);
			sprintf(buf, "%s exceeded house limit in %d rooms, %d multiplier charged.",
				GET_NAME(d->character), rooms, percent);
			slog(buf);

			// actually multiply it
			house_control[i].rent_sum *= percent;
		    }
		    sprintf(buf, 
			    "You have accrued costs of %d %s on property: %s.\r\n", 
			    house_control[i].rent_sum, 
			    (cur = (h_rm->zone->time_frame == TIME_ELECTRO)) ?
			    "credits" : "coins", h_rm->name);
		    SEND_TO_Q(buf, d);
		    if (cur) {        /* credits */
			if ((GET_ECONET(d->character) + GET_CASH(d->character))
			    < house_control[i].rent_sum) {
			    house_control[i].rent_sum -= GET_ECONET(d->character);
			    house_control[i].rent_sum -= GET_CASH(d->character);
			    GET_ECONET(d->character) = GET_CASH(d->character) = 0;
			} else {
			    GET_CASH(d->character) -= house_control[i].rent_sum;
			    if (GET_CASH(d->character) < 0) {
				GET_ECONET(d->character) += GET_CASH(d->character);
				GET_CASH(d->character) = 0;
			    }
			    house_control[i].rent_sum = 0;
			}
		    } else {          /** gold economy **/
			if (GET_GOLD(d->character) < house_control[i].rent_sum) {
			    house_control[i].rent_sum -= GET_GOLD(d->character);
			    GET_GOLD(d->character) = 0;
			} else {
			    GET_GOLD(d->character) -= house_control[i].rent_sum;
			    house_control[i].rent_sum = 0;
			}
		    }

		    if (house_control[i].rent_sum > 0) {     /* bank is universal */

			if (GET_BANK_GOLD(d->character) < house_control[i].rent_sum) {
			    house_control[i].rent_sum -= GET_BANK_GOLD(d->character);
			    GET_BANK_GOLD(d->character) = 0;
			    SEND_TO_Q("You could not afford the rent.\r\n"
				      "Some of your items have been repossessed.\r\n", d);
			    sprintf(buf, "House-rent (%d) - equipment lost.",
				    house_control[i].house_rooms[0]);
			    slog(buf);
			    for (j = 0; 
				 j < house_control[i].num_of_rooms &&
				     house_control[i].rent_sum > 0; j++) {
				if ((h_rm = real_room(house_control[i].house_rooms[j]))) {
				    for (obj = h_rm->contents; 
					 obj && house_control[i].rent_sum > 0; 
					 obj = next_obj) {
					next_obj = obj->next_content;
					house_control[i].rent_sum -=
					    recurs_obj_cost(obj, true, NULL);
					extract_obj(obj);
				    }
				}
			    }
			} else
			    GET_BANK_GOLD(d->character) -= house_control[i].rent_sum;
		    }
	  
		    house_control[i].rent_sum = 0;
		    house_control[i].hourly_rent_sum = 0;
		    house_control[i].rent_time = 0;
		    House_crashsave(h_rm->number);
		}
	    }
	    d->prompt_mode = 1;
	    break;

	case '2':
	    SEND_TO_Q("\033[H\033[J", d);
	    SEND_TO_Q("Other players will be usually be able to determine your general\r\n"
		      "size, as well as your race and gender, by looking at you.  What\r\n"
		      "else is noticable about your character?\r\n", d);
	    SEND_TO_Q("Terminate with a '@' on a new line.\r\n", d);
	    if (d->character->player.description) {
		SEND_TO_Q("Old description:\r\n", d);
		SEND_TO_Q(d->character->player.description, d);
#ifdef DMALLOC
		dmalloc_verify(0);
#endif
		free(d->character->player.description);
#ifdef DMALLOC
		dmalloc_verify(0);
#endif
		d->character->player.description = NULL;
	    }
	    d->str = &d->character->player.description;
	    d->max_str = MAX_CHAR_DESC-1;
	    STATE(d) = CON_EXDESC;
	    break;

	case '3':
	    SEND_TO_Q("\033[H\033[J", d);
	    page_string(d, background, 0);
	    STATE(d) = CON_RMOTD;
	    break;

	case '4':
	    SEND_TO_Q("\033[H\033[J", d);
	    sprintf(buf, "Changing password for %s.\r\n", GET_NAME(d->character));
	    SEND_TO_Q(buf, d);
	    SEND_TO_Q("\r\nEnter your old password: ", d);
	    echo_off(d);
	    STATE(d) = CON_CHPWD_GETOLD;
	    break;

	case '5':
	    SEND_TO_Q("\033[H\033[J", d);
	    sprintf(buf, "\r\nDeleting character '%s', level %d.\r\n",
		    GET_NAME(d->character), GET_LEVEL(d->character));
	    SEND_TO_Q(buf, d);
	    SEND_TO_Q("\r\nFor verification enter your password: ", d);
	    echo_off(d);
	    STATE(d) = CON_DELCNF1;
	    break;

	default:
	    if (!mini_mud)
		SEND_TO_Q("\033[H\033[J", d);
	    SEND_TO_Q("\r\nThat's not a menu choice!\r\n", d);
	    show_menu(d);
	    break;
	}

	break;

    case CON_CHPWD_GETOLD:
	if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
	    echo_on(d);
	    SEND_TO_Q("\033[H\033[J", d);
	    SEND_TO_Q("\r\nIncorrect password.  ---  Password unchanged\r\n", d);
	    show_menu(d);
	    STATE(d) = CON_MENU;
	    return;
	} else {
	    SEND_TO_Q("\r\nEnter a new password: ", d);
	    STATE(d) = CON_CHPWD_GETNEW;
	    return;
	}
	break;

    case CON_DELCNF1:
	echo_on(d);
	if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
	    SEND_TO_Q("\033[H\033[J", d);
	    SEND_TO_Q("\r\nIncorrect password. -- Deletion aborted.\r\n", d);
	    show_menu(d);
	    STATE(d) = CON_MENU;
	} else {
	    SEND_TO_Q(CCRED(d->character, C_SPR), d);
	    SEND_TO_Q("\r\nYOU ARE ABOUT TO DELETE THIS CHARACTER PERMANENTLY.\r\n"
		      "ARE YOU ABSOLUTELY SURE?\r\n\r\n", d);
	    SEND_TO_Q(CCNRM(d->character, C_SPR), d);
	    SEND_TO_Q("Please type \"yes\" to confirm: ", d);
	    STATE(d) = CON_DELCNF2;
	}
	break;

    case CON_DELCNF2:
	if (!strcmp(arg, "yes") || !strcmp(arg, "YES")) {
	    if (PLR_FLAGGED(d->character, PLR_FROZEN)) {
		SEND_TO_Q(CCCYN(d->character, C_NRM), d);
		SEND_TO_Q(CCBLD(d->character, C_NRM), d);
		SEND_TO_Q("You try to kill yourself, but the ice stops you.\r\n", d);
		SEND_TO_Q(CCNRM(d->character, C_NRM), d);
		SEND_TO_Q("Character not deleted.\r\n\r\n", d);
		STATE(d) = CON_CLOSE;
		return;
	    }
	    if (GET_LEVEL(d->character) < LVL_GRGOD)
		SET_BIT(PLR_FLAGS(d->character), PLR_DELETED);
	    save_char(d->character, real_room(GET_LOADROOM(d->character)));
	    Crash_delete_file(GET_NAME(d->character), CRASH_FILE);
	    Crash_delete_file(GET_NAME(d->character), IMPLANT_FILE);
	    population_record[GET_HOME(d->character)]--;

	    if ((clan = real_clan(GET_CLAN(d->character))) &&
		(member = real_clanmember(GET_IDNUM(d->character), clan))) {
		REMOVE_MEMBER_FROM_CLAN(member, clan);

#ifdef DMALLOC
		dmalloc_verify(0);
#endif
		free(member);
#ifdef DMALLOC
		dmalloc_verify(0);
#endif
		save_clans();
	    }

	    sprintf(buf, "Character '%s' deleted!\r\n"
		    "Goodbye.\r\n", GET_NAME(d->character));
	    SEND_TO_Q(buf, d);
	    sprintf(buf, "%s (lev %d) has self-deleted.", GET_NAME(d->character),
		    GET_LEVEL(d->character));
	    mudlog(buf, NRM, LVL_GOD, TRUE);
	    STATE(d) = CON_CLOSE;
	    return;
	} else {
	    SEND_TO_Q("\033[H\033[J", d);      
	    SEND_TO_Q("\r\nCharacter not deleted.\r\n", d);
	    show_menu(d);
	    STATE(d) = CON_MENU;
	}
	break;

    case CON_AFTERLIFE:
	SEND_TO_Q("\033[H\033[J", d);
	show_menu(d);
	STATE(d) = CON_MENU;
	break;

    case CON_CLOSE:
	close_socket(d);
	break;

    case CON_NET_MENU1:
	switch (*arg) {
	case '0':
	case '@':
	case '+':
	    sprintf(buf, "User %s disconnecting from net.", GET_NAME(d->character));
	    slog(buf);
	    strcat(buf, "\r\n");
	    SEND_TO_Q(buf, d);
	    STATE(d) = CON_PLAYING;
	    act("$n disconnects from the network.", TRUE, d->character, 0, 0, TO_ROOM);
	    break;
	case '1':
	    show_net_progmenu1_to_descriptor(d);
	    STATE(d) = CON_NET_PROGMENU1;
	    break;
	case '2':
	    perform_net_who(d->character, "");
	    SEND_TO_Q("  >", d);
	    break;
	default:
	    sprintf(buf, "%s : ILLEGAL OPTION.\r\n", arg);
	    show_net_menu1_to_descriptor(d);
	    break;
	}
	break;
  
    case CON_NET_PROGMENU1:
	switch (*arg) {
	case '@':
	case '+':
	    sprintf(buf, "User %s disconnecting from net.", GET_NAME(d->character));
	    slog(buf);
	    strcat(buf, "\r\n");
	    SEND_TO_Q(buf, d);
	    STATE(d) = CON_PLAYING;
	    act("$n disconnects from the network.", TRUE, d->character, 0, 0, TO_ROOM);
	    break;
	case '0':
	    show_net_menu1_to_descriptor(d);
	    STATE(d) = CON_NET_MENU1;
	    break;
	case '1':
	    SEND_TO_Q("Entering Cyborg database.\r\n"
		      "Use the list command to view programs.\r\n"
		      "  >", d);
	    STATE(d) = CON_NET_PROG_CYB;
	    break;
	default:
	    sprintf(buf, "%s : ILLEGAL OPTION.\r\n", arg);
	    show_net_progmenu1_to_descriptor(d);
	    break;
	}
	break;
    case CON_NET_PROG_CYB:
	if (!*arg) {
	    SEND_TO_Q("  >", d);
	    return;
	}
	char_class_state = CLASS_CYBORG;

	arg = one_argument(arg, arg1);
	if (!strcmp(arg1, "@") || !strcmp(arg1, "+")) {
	    sprintf(buf, "User %s disconnecting from net.", GET_NAME(d->character));
	    slog(buf);
	    strcat(buf, "\r\n");
	    SEND_TO_Q(buf, d);
	    STATE(d) = CON_PLAYING;
	    act("$n disconnects from the network.", TRUE, d->character, 0, 0, TO_ROOM);
	    return;
	} else if (is_abbrev(arg1, "download")) {
	    if (!IS_CYBORG(d->character))
		SEND_TO_Q("You are unable to download to your neural network.\r\n", d);
	    else if (!GET_PRACTICES(d->character))
		SEND_TO_Q("You have no data storage units free.\r\n", d);
	    else {
		skip_spaces(&arg);
		if (!*arg) {
		    SEND_TO_Q("Download which program?\r\n", d);
		    return;
		} 
		skill_num = find_skill_num(arg);
        
		if (skill_num < 1) {
		    SEND_TO_Q("Unknown program.\r\n", d);
		    return;
		}
		if (GET_LEVEL(d->character) < spell_info[skill_num].min_level[char_class_state]) {
		    SEND_TO_Q("That program is unavailable.\r\n", d);
		    return;
		}
		if (GET_SKILL(d->character, skill_num) >= LEARNED(d->character)) {
		    SEND_TO_Q("That program already fully installed.\r\n", d);
		    return;
		}
		percent = MIN(MAXGAIN(d->character), 
			      MAX(MINGAIN(d->character), 
				  INT_APP(GET_INT(d->character))));
		percent = MIN(LEARNED(d->character) - 
                              GET_SKILL(d->character, skill_num), percent); 
		GET_PRACTICES(d->character)--;
		SET_SKILL(d->character, skill_num, GET_SKILL(d->character, skill_num) + percent); 

		sprintf(buf, "Program download: %s terminating, %d percent transfer.\r\n",
			spells[skill_num], percent); 
		SEND_TO_Q(buf, d);
		if (GET_SKILL(d->character, skill_num) >= LEARNED(d->character))
		    strcpy(buf, "Program fully installed on home system.\r\n");
		else
		    sprintf(buf, "Program %d percent installed on home system.\r\n", 
			    GET_SKILL(d->character, skill_num));
		SEND_TO_Q(buf, d);
		return;
	    }
	    SEND_TO_Q("  >", d);
	} else if (is_abbrev(arg1, "exit") || is_abbrev(arg1, "back") || 
		   is_abbrev(arg1, "return")) {
	    sprintf(buf, "Leaving directory of %s programs.\r\n",
		    char_class_state == CLASS_CYBORG ? "Cyborg" :
		    char_class_state == CLASS_MONK ? "Monk" : 
		    char_class_state == CLASS_HOOD ? "Hoodlum" : "UNDEFINED");
	    SEND_TO_Q(buf, d);
	    show_net_progmenu1_to_descriptor(d);
	    STATE(d) = CON_NET_PROGMENU1;
	    break;
	} else if (is_abbrev(arg1, "list")) {
	    show_programs_to_char(d->character, char_class_state);
	} else if (is_abbrev(arg1, "help") || is_abbrev(arg1, "?")) {
	    skip_spaces(&arg);
	    if (!*arg) {
		SEND_TO_Q("Valid commands are:\r\n"
			  "list         help        ?\r\n"
			  "finger       who         status\r\n"
			  "exit         back        \r\n"
			  "Escape character: @ or +\r\n", d);
	    } else
		do_help(d->character, arg, 0, 0);
	} else if (is_abbrev(arg1, "who")) {
	    perform_net_who(d->character, "");
	} else if (is_abbrev(arg1, "finger")) {
	    skip_spaces(&arg);
	    if (!*arg)
		perform_net_who(d->character, "");
	    else
		perform_net_finger(d->character, arg);
	} else if (is_abbrev(arg1, "status") || is_abbrev(arg1, "free")) {
	    sprintf(buf, "You have %d data storage units free.\r\n", GET_PRACTICES(d->character));
	    SEND_TO_Q(buf, d);
	} else {
	    SEND_TO_Q("Invalid command.\r\n", d);
	}
	SEND_TO_Q("  >", d);
	break;

    case CON_PORT_OLC:
	polc_input (d, arg);
	break;
        
    default:
	slog("SYSERR: Nanny: illegal state of con'ness; closing connection");
	close_socket(d);
	break;
    }
}

#undef __interpreter_c__
