/* ************************************************************************
*   File: config.c                                      Part of CircleMUD *
*  Usage: Configuration of various aspects of CircleMUD operation         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: config.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#define __CONFIG_C__

#include "structs.h"
#include "spells.h"
#include "char_class.h"

#define TRUE	1
#define YES	1
#define FALSE	0
#define NO	0

/*
 * Below are several constants which you can change to alter certain aspects
 * of the way CircleMUD acts.  Since this is a .c file, all you have to do
 * to change one of the constants (assuming you keep your object files around)
 * is change the constant in this file and type 'make'.  Make will recompile
 * this file and relink; you don't have to wait for the whole thing to
 * recompile as you do if you change a header file.
 *
 * I realize that it would be slightly more efficient to have lots of
 * #defines strewn about, so that, for example, the autowiz code isn't
 * compiled at all if you don't want to use autowiz.  However, the actual
 * code for the various options is quite small, as is the computational time
 * in checking the option you've selected at run-time, so I've decided the
 * convenience of having all your options in this one file outweighs the
 * efficency of doing it the other way.
 *
 */

/****************************************************************************/
/****************************************************************************/


/* GAME PLAY OPTIONS */

/* minimum level a player must be to shout/holler/gossip/auction */
int level_can_shout = 6;

/* number of movement points it costs to holler */
int holler_move_cost = 20;

/* exp change limits */
int max_exp_gain = 10000000;	/* max gainable per kill */
int max_exp_loss = 1000000;		/* max losable per death */

/* number of tics (usually 75 seconds) before PC/NPC corpses decompose */
int max_npc_corpse_time = 5;
int max_pc_corpse_time = 10;

/* should items in death traps automatically be junked? */
int dts_are_dumps = NO;

/* "okay" etc. */
char *OK = "You got it.\r\n";
char *NOPERSON = "No-one by that name here.\r\n";
char *NOEFFECT = "Nothing seems to happen.\r\n";

/****************************************************************************/
/****************************************************************************/


/* RENT/CRASHSAVE OPTIONS */

/*
 * Should the MUD allow you to 'rent' for free?  (i.e. if you just quit,
 * your objects are saved at no cost, as in Merc-type MUDs.
 */
int free_rent = NO;

/* maximum number of items players are allowed to rent */
int max_obj_save = 25;

/* receptionist's surcharge on top of item costs */
int min_rent_cost = 5;

/*
 * Should the game automatically save people?  (i.e., save player data
 * every 4 kills (on average), and Crash-save as defined below.
 */
int auto_save = YES;

/*
 * if auto_save (above) is yes, how often (in minutes) should the MUD
 * Crash-save people's objects?   Also, this number indicates how often
 * the MUD will Crash-save players' houses.
 */
int autosave_time = 4;

/* Lifetime of crashfiles and forced-rent (idlesave) files in days */
int crash_file_timeout = 15;

/* Lifetime of normal rent files in days */
int rent_file_timeout = 90;


/****************************************************************************/
/****************************************************************************/


/* ROOM NUMBERS */

/* vnum number of room that mortals should enter at */
room_num mortal_start_room = 3001;
room_num new_thalos_start_room = 5505;
room_num kromguard_start_room = 39188;
room_num electro_start_room = 30001;
room_num newbie_start_room = 2301;
room_num elven_start_room = 19024;
room_num istan_start_room = 20444;
room_num arena_start_room = 40000;
room_num tower_modrian_start_room = 2301;
room_num monk_start_room = 21007;
room_num solace_start_room = 63000;
room_num mavernal_start_room = 59125;
room_num dwarven_caverns_start_room = 22809;
room_num human_square_start_room = 22898;
room_num skullport_start_room = 22873;
room_num drow_isle_start_room = 22727;
room_num skullport_newbie_start_room = 23100;
room_num zul_dane_newbie_start_room = 53306;
room_num zul_dane_start_room = 53172;
room_num newbie_school_start_room = 200;

room_num astral_manse_start_room = 42500;

/* vnum number of room that immorts should enter at by default */
room_num immort_start_room = 1204;

/* vnum number of room that frozen players should enter at */
room_num frozen_start_room = 1202;

/*
 * vnum numbers of donation rooms.  note: you must change code in
 * do_drop of act.obj1.c if you change the number of non-NOWHERE
 * donation rooms.
 */
room_num donation_room_1 = 3032;
room_num donation_room_2 = 30032;
room_num donation_room_3 = 5510;
room_num donation_room_istan = 20470;
room_num donation_room_solace = 63102;
room_num donation_room_skullport_common = 22942;
room_num donation_room_skullport_dwarven = 22807;

int guild_donation_info[][4] = {

	{CLASS_THIEF, ALL, HOME_SKULLPORT, 22998},
	{CLASS_THIEF, ALL, HOME_DWARVEN_CAVERNS, 22998},
	{CLASS_THIEF, ALL, HOME_HUMAN_SQUARE, 22998},
	{CLASS_THIEF, ALL, HOME_DROW_ISLE, 22998},

	{CLASS_CLERIC, EVIL, HOME_SKULLPORT, 22929},
	{CLASS_CLERIC, EVIL, HOME_DWARVEN_CAVERNS, 22929},
	{CLASS_CLERIC, EVIL, HOME_HUMAN_SQUARE, 22929},
	{CLASS_CLERIC, EVIL, HOME_DROW_ISLE, 22929},

	{CLASS_VAMPIRE, EVIL, HOME_SKULLPORT, 22995},
	{CLASS_VAMPIRE, EVIL, HOME_DWARVEN_CAVERNS, 22995},
	{CLASS_VAMPIRE, EVIL, HOME_HUMAN_SQUARE, 22995},
	{CLASS_VAMPIRE, EVIL, HOME_DROW_ISLE, 22995},

	{CLASS_BARB, ALL, HOME_SKULLPORT, 22993},
	{CLASS_BARB, ALL, HOME_DWARVEN_CAVERNS, 22993},
	{CLASS_BARB, ALL, HOME_HUMAN_SQUARE, 22993},
	{CLASS_BARB, ALL, HOME_DROW_ISLE, 22993},

	{CLASS_MAGIC_USER, ALL, HOME_SKULLPORT, 22712},
	{CLASS_MAGIC_USER, ALL, HOME_DWARVEN_CAVERNS, 22712},
	{CLASS_MAGIC_USER, ALL, HOME_HUMAN_SQUARE, 22712},
	{CLASS_MAGIC_USER, ALL, HOME_DROW_ISLE, 22712},

	{CLASS_KNIGHT, EVIL, HOME_SKULLPORT, 22992},
	{CLASS_KNIGHT, EVIL, HOME_DWARVEN_CAVERNS, 22992},
	{CLASS_KNIGHT, EVIL, HOME_HUMAN_SQUARE, 22992},
	{CLASS_KNIGHT, EVIL, HOME_DROW_ISLE, 22992},

	{CLASS_RANGER, ALL, HOME_MODRIAN, 2701},
	{CLASS_MAGIC_USER, ALL, HOME_MODRIAN, 2702},
	{CLASS_THIEF, ALL, HOME_MODRIAN, 2703},
	{CLASS_BARB, ALL, HOME_MODRIAN, 2704},
	{CLASS_CLERIC, EVIL, HOME_MODRIAN, 2705},
	{CLASS_KNIGHT, EVIL, HOME_MODRIAN, 2706},
	{CLASS_KNIGHT, GOOD, HOME_MODRIAN, 2707},
	{CLASS_CLERIC, GOOD, HOME_MODRIAN, 2708},

	{CLASS_MONK, ALL, HOME_MONK, 21033},

	{CLASS_CYBORG, ALL, HOME_ELECTRO, 30270},
	{CLASS_PHYSIC, ALL, HOME_ELECTRO, 30271},
	{CLASS_MONK, ALL, HOME_ELECTRO, 30272},
	{CLASS_PSIONIC, ALL, HOME_ELECTRO, 30273},
	{CLASS_MERCENARY, ALL, HOME_ELECTRO, 30274},
	{CLASS_CLERIC, EVIL, HOME_ELECTRO, 30276},
	{CLASS_THIEF, ALL, HOME_ELECTRO, 30277},
	{CLASS_RANGER, ALL, HOME_ELECTRO, 30278},

	{CLASS_RANGER, ALL, HOME_SOLACE_COVE, 63134},
	{CLASS_MAGIC_USER, ALL, HOME_SOLACE_COVE, 63137},
	{CLASS_BARB, ALL, HOME_SOLACE_COVE, 63140},
	{CLASS_KNIGHT, EVIL, HOME_SOLACE_COVE, 63143},
	{CLASS_KNIGHT, GOOD, HOME_SOLACE_COVE, 63146},
	{CLASS_CLERIC, GOOD, HOME_SOLACE_COVE, 63149},
	{CLASS_THIEF, ALL, HOME_SOLACE_COVE, 63152},

	{-1, -1, -1, -1}
};

/****************************************************************************/
/****************************************************************************/


/* GAME OPERATION OPTIONS */

/* default port the game should run on if no port given on command-line */
int DFLT_PORT = 8888;

/* default directory to use as data directory */
char *DFLT_DIR = "lib";

/* maximum number of players allowed before game starts to turn people away */
unsigned int MAX_PLAYERS = 300;

/* maximum size of bug, typo and idea files (to prevent bombing) */
int max_filesize = 50000;

/* maximum number of password attempts before disconnection */
int max_bad_pws = 2;

/*
 * Some nameservers are very slow and cause the game to lag terribly every 
 * time someone logs in.  The lag is caused by the gethostbyaddr() function
 * which is responsible for resolving numeric IP addresses to alphabetic names.
 * Sometimes, nameservers can be so slow that the incredible lag caused by
 * gethostbyaddr() isn't worth the luxury of having names instead of numbers
 * for players' sitenames.
 *
 * If your nameserver is fast, set the variable below to NO.  If your
 * nameserver is slow, of it you would simply prefer to have numbers
 * instead of names for some other reason, set the variable to YES.
 *
 * You can experiment with the setting of nameserver_is_slow on-line using
 * the SLOWNS command from within the MUD.
 */

int nameserver_is_slow = YES;


char *GREETINGS =
	".   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   . \r\n"
	"\r\n"
	".   .   .  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ .   .   . \r\n"
	"          @@@****************************************************@@@\r\n"
	".   .   . @@* ************************************************** *@@.   .   . \r\n"
	"          @@* *                     TEMPUS                     * *@@\r\n"
	".   .   . @@* *                                                * *@@.   .   . \r\n"
	"          @@* *               the anachronistic                * *@@\r\n"
	".   .   . @@* *             Multiple User Domain               * *@@.   .   . \r\n"
	"          @@* *                                                * *@@\r\n"
	".   .   . @@* *                                                * *@@.   .   . \r\n"
	"          @@* *     FOUNDATION:  Circle 3.00, Jeremy Elson     * *@@\r\n"
	".   .   . @@* *       A derivative of DikuMUD (GAMMA 0.0)      * *@@.   .   . \r\n"
	"          @@* *                                                * *@@\r\n"
	".   .   . @@* *               Powered by Linux                 * *@@.   .   . \r\n"
	"          @@* ************************************************** *@@\r\n"
	".   .   . @@@****************************************************@@@.   .   . \r\n"
	"           @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n"
	".   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   . \r\n"
	"\r\n"
	".   .   .   .   .   .   . Welcome to the Mothership .   .   .   .   .   .   . \r\n"
	"\r\n"
	".   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   . \r\n"
	"\r\n";

char *MUD_MOVED_MSG =
	".   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   . \r\n"
	"\r\n"
	".   .   .  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ .   .   . \r\n"
	"          @@@****************************************************@@@\r\n"
	".   .   . @@* ************************************************** *@@.   .   . \r\n"
	"          @@* *                     TEMPUS                     * *@@\r\n"
	".   .   . @@* *                                                * *@@.   .   . \r\n"
	"          @@* *               the anachronistic                * *@@\r\n"
	".   .   . @@* *             Multiple User Domain               * *@@.   .   . \r\n"
	"          @@* *                                                * *@@\r\n"
	".   .   . @@* *                                                * *@@.   .   . \r\n"
	"          @@* *               TEMPUS HAS MOVED!                * *@@\r\n"
	".   .   . @@* *                                                * *@@.   .   . \r\n"
	"          @@* *         mud.tempusmud.com port 2020            * *@@\r\n"
	".   .   . @@* *              ( 206.41.250.2 )                  * *@@.   .   . \r\n"
	"          @@* *                                                * *@@\r\n"
	".   .   . @@* *                                                * *@@.   .   . \r\n"
	"          @@* ************************************************** *@@\r\n"
	".   .   . @@@****************************************************@@@.   .   . \r\n"
	"           @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n"
	".   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   . \r\n"
	"\r\n"
	".   .   .   .   .   .   . The Mothership awaits you .   .   .   .   .   .   . \r\n"
	"\r\n"
	".   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   . \r\n"
	"\r\n";

char *WELC_MESSG = "\r\n" "Welcome to the realms of Tempus, adventurer.\r\n";

char *START_MESSG =
	"Welcome.  This is your new character in the world of Tempus! \r\n"
	"You must be strong to survive, but in time you may become powerful\r\n"
	"beyond your wildest dreams...\r\n\r\n";

/****************************************************************************/
/****************************************************************************/


/* AUTOWIZ OPTIONS */

/* Should the game automatically create a new wizlist/immlist every time
   someone immorts, or is promoted to a higher (or lower) god level? */
int use_autowiz = NO;

/* If yes, what is the lowest level which should be on the wizlist?  (All
   immort levels below the level you specify will go on the immlist instead.) */
int min_wizlist_lev = LVL_GOD;
