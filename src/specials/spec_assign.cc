/* ************************************************************************
*   File: spec_assign.c                                 Part of CircleMUD *
*  Usage: Functions to assign function pointers to objs/mobs/rooms        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: spec_assign.c -- assign specs to obj/mob/rooms -- Part of TempusMUD
//
// Although this file is based on the Circle spec_assign.c, please note
// that the current contents of this file are considerably different.
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//
#include <errno.h>

#include "structs.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "specs.h"
#include "screen.h"
#include "comm.h"
#include "vendor.h"
#include "tmpstr.h"
#include "mobile_map.h"
#include "object_map.h"

extern int mini_mud;
//extern struct obj_data *obj_proto;
extern struct zone_data *zone_table;
extern struct Creature *mob_proto;
extern struct shop_data *shop_index;

// weapon_lister is a transient
SPECIAL(weapon_lister);

//
// The list of special procedures
//

const struct spec_func_data spec_list[] = {
	{"null", NULL, SPEC_OBJ | SPEC_MOB | SPEC_RM},
	{"postmaster", postmaster, SPEC_MOB},
	{"cityguard", cityguard, SPEC_MOB},
	{"receptionist", receptionist, SPEC_MOB},
	{"cryogenicist", cryogenicist, SPEC_MOB},
	{"guildmaster", guild, SPEC_MOB},
	{"puff", puff, SPEC_MOB},
	{"fido", fido, SPEC_MOB},
	{"buzzard", buzzard, SPEC_MOB},
	{"garbage_pile", garbage_pile, SPEC_MOB},
	{"janitor", janitor, SPEC_MOB},
	{"elven_janitor", elven_janitor, SPEC_MOB},
	{"gelatinous_blob", gelatinous_blob, SPEC_MOB},
	{"venom_attack", venom_attack, SPEC_MOB},
	{"thief", thief, SPEC_MOB},
	{"magic_user", magic_user, SPEC_MOB},
	{"medusa", medusa, SPEC_MOB},
	{"temple_healer", temple_healer, SPEC_MOB},
	{"barbarian", barbarian, SPEC_MOB},
	{"chest_mimic", chest_mimic, SPEC_MOB},
	{"rat_mama", rat_mama, SPEC_MOB | SPEC_RES},
	{"improve_dex", improve_dex, SPEC_MOB},
	{"improve_int", improve_int, SPEC_MOB},
	{"improve_con", improve_con, SPEC_MOB},
	{"improve_str", improve_str, SPEC_MOB},
	{"improve_cha", improve_cha, SPEC_MOB},
	{"improve_wis", improve_wis, SPEC_MOB},
	{"wagon_driver", wagon_driver, SPEC_MOB},
	{"fire_breather", fire_breather, SPEC_MOB},
	{"energy_drainer", energy_drainer, SPEC_MOB},
	{"basher", basher, SPEC_MOB},
	{"ornery_goat", ornery_goat, SPEC_MOB},
	{"aziz", Aziz, SPEC_MOB | SPEC_RES},
	{"circus_clown", circus_clown, SPEC_MOB},
	{"grandmaster", grandmaster, SPEC_MOB | SPEC_RES},
	{"cock_fight", cock_fight, SPEC_MOB},
	{"cyber_cock", cyber_cock, SPEC_MOB},
	{"cave_bear", cave_bear, SPEC_MOB},
	{"juju_zombie", juju_zombie, SPEC_MOB},
	{"cemetary_ghoul", cemetary_ghoul, SPEC_MOB | SPEC_RES},
	{"phantasmic_sword", phantasmic_sword, SPEC_MOB | SPEC_RES},
	{"newbie_healer", newbie_healer, SPEC_MOB | SPEC_RES},
	{"newbie_improve", newbie_improve, SPEC_MOB | SPEC_RES},
	{"newbie_fly", newbie_fly, SPEC_MOB | SPEC_RES},
	{"newbie_fodder", newbie_fodder, SPEC_MOB | SPEC_RES},
	{"gen_locker", gen_locker, SPEC_MOB},
	{"guard", guard, SPEC_MOB},
	{"newbie_gold_coupler", newbie_gold_coupler, SPEC_MOB | SPEC_RES},
	{"maze_switcher", maze_switcher, SPEC_MOB | SPEC_RES},
	{"maze_cleaner", maze_cleaner, SPEC_MOB | SPEC_RES},
	{"darom", darom, SPEC_MOB | SPEC_RES},
	{"puppet", puppet, SPEC_MOB | SPEC_RES},
	{"oracle", oracle, SPEC_MOB | SPEC_RES},
	{"dt_cleaner", dt_cleaner, SPEC_MOB | SPEC_RES},
	{"sleeping_soldier", sleeping_soldier, SPEC_MOB},
	{"registry", registry, SPEC_MOB},
	{"corpse_retrieval", corpse_retrieval, SPEC_MOB},
	{"carrion_crawler", carrion_crawler, SPEC_MOB},
	{"ogre1", ogre1, SPEC_MOB},
	{"arena_locker", arena_locker, SPEC_MOB | SPEC_RES},
	{"killer_hunter", killer_hunter, SPEC_MOB},
	{"remorter", remorter, SPEC_MOB | SPEC_RES},
	{"nohunger_dude", nohunger_dude, SPEC_MOB},
	{"dwarven_hermit", dwarven_hermit, SPEC_MOB | SPEC_RES},
	{"gunnery_device", gunnery_device, SPEC_MOB | SPEC_RES},
	{"aziz_canon", aziz_canon, SPEC_MOB | SPEC_RES},
	{"safiir", safiir, SPEC_MOB},
	{"spinal", spinal, SPEC_MOB | SPEC_RES},
	{"fate", fate, SPEC_MOB | SPEC_RES},
	{"red_highlord", red_highlord, SPEC_MOB | SPEC_RES},
	{"tiamat", tiamat, SPEC_MOB | SPEC_RES},
	{"beer_tree", beer_tree, SPEC_MOB | SPEC_RES},
	{"kata", kata, SPEC_MOB},
	{"insane_merchant", insane_merchant, SPEC_MOB | SPEC_RES},
	{"astral_deva", astral_deva, SPEC_MOB},
	{"hell_hound", hell_hound, SPEC_MOB},
	{"geryon", geryon, SPEC_MOB},
	{"moloch", moloch, SPEC_MOB | SPEC_RES},
	{"bearded_devil", bearded_devil, SPEC_MOB | SPEC_RES},
	{"cyberfiend", cyberfiend, SPEC_MOB | SPEC_RES},
	{"jail_locker", jail_locker, SPEC_MOB | SPEC_RES},
	{"mob_helper", mob_helper, SPEC_MOB},
	{"healing_ranger", healing_ranger, SPEC_MOB},
	{"battlefield_ghost", battlefield_ghost, SPEC_MOB | SPEC_RES},
	{"battle_cleric", battle_cleric, SPEC_MOB},
	{"electronics_school", electronics_school, SPEC_MOB},
	{"electrician", electrician, SPEC_MOB},
	{"paramedic", paramedic, SPEC_MOB},
	{"lawyer", lawyer, SPEC_MOB | SPEC_RES},
	{"underwater_predator", underwater_predator, SPEC_MOB},
	{"high_priestess", high_priestess, SPEC_MOB | SPEC_RES},
	{"archon", archon, SPEC_MOB | SPEC_RES},
	{"gollum", gollum, SPEC_MOB | SPEC_RES},
	{"cuckoo", cuckoo, SPEC_MOB | SPEC_RES},
	{"pendulum_timer_mob", pendulum_timer_mob, SPEC_MOB | SPEC_RES},
	{"parrot", parrot, SPEC_MOB | SPEC_RES},
	{"watchdog", watchdog, SPEC_MOB},
	{"cyborg_overhaul", cyborg_overhaul, SPEC_MOB},
	{"credit_exchange", credit_exchange, SPEC_MOB},
	{"gold_exchange", gold_exchange, SPEC_MOB},
	{"repairer", repairer, SPEC_MOB},
	{"reinforcer", reinforcer, SPEC_MOB},
	{"enhancer", enhancer, SPEC_MOB},
	{"junker", junker, SPEC_MOB},
	{"bank", bank, SPEC_MOB | SPEC_OBJ},
	{"castleguard", CastleGuard, SPEC_MOB | SPEC_RES},
	{"james", James, SPEC_MOB | SPEC_RES},
	{"cleaning", cleaning, SPEC_MOB | SPEC_RES},
	{"tim", tim, SPEC_MOB | SPEC_RES},
	{"tom", tom, SPEC_MOB | SPEC_RES},
	{"duke_araken", duke_araken, SPEC_MOB | SPEC_RES},
	{"training_master", training_master, SPEC_MOB | SPEC_RES},
	{"peter", peter, SPEC_MOB | SPEC_RES},
	{"jerry", jerry, SPEC_MOB | SPEC_RES},
	{"lounge_soldier", lounge_soldier, SPEC_MOB | SPEC_RES},
	{"armory_person", armory_person, SPEC_MOB | SPEC_RES},
	{"underworld_goddess", underworld_goddess, SPEC_MOB | SPEC_RES},
	{"duke_nukem", duke_nukem, SPEC_MOB | SPEC_RES},
	{"implanter", implanter, SPEC_MOB},
	{"taunting_frenchman", taunting_frenchman, SPEC_MOB},
	{"nude_guard", nude_guard, SPEC_MOB},
	{"increaser", increaser, SPEC_MOB | SPEC_RES},
	{"quasimodo", quasimodo, SPEC_MOB | SPEC_RES},
	{"spirit_priestess", spirit_priestess, SPEC_MOB | SPEC_RES},
	{"corpse_griller", corpse_griller, SPEC_MOB | SPEC_RES},
	{"head_shrinker", head_shrinker, SPEC_MOB | SPEC_RES},
	{"multi_healer", multi_healer, SPEC_MOB | SPEC_RES},
	{"slaver", slaver, SPEC_MOB | SPEC_RES},
	{"rust_monster", rust_monster, SPEC_MOB},
	{"tarrasque", tarrasque, SPEC_MOB | SPEC_RES},
	{"hell_hunter", hell_hunter, SPEC_MOB | SPEC_RES},
	{"hell_hunter_brain", hell_hunter_brain, SPEC_MOB | SPEC_RES},
	{"arioch", arioch, SPEC_MOB | SPEC_RES},
	{"maladomini_jailer", maladomini_jailer, SPEC_MOB | SPEC_RES},
	{"weaponsmaster", weaponsmaster, SPEC_MOB},
	{"boulder_thrower", boulder_thrower, SPEC_MOB},
	{"unholy_stalker", unholy_stalker, SPEC_MOB | SPEC_RES},
	{"oedit_reloader", oedit_reloader, SPEC_MOB | SPEC_RES},
	{"town_crier", town_crier, SPEC_MOB | SPEC_RES},
	{"hell_regulator", hell_regulator, SPEC_MOB | SPEC_RES},
	{"hell_ressurector", hell_ressurector, SPEC_MOB | SPEC_RES},
	{"unspecializer", unspecializer, SPEC_MOB | SPEC_RES},
	{"pit_keeper", pit_keeper, SPEC_MOB | SPEC_RES},
	{"cremator", cremator, SPEC_MOB | SPEC_RES},
	{"mugger", mugger, SPEC_MOB | SPEC_RES},

	//  {"lord_vader",      lord_vader,            SPEC_MOB | SPEC_RES },
	/** objects **/
	{"gen_board", gen_board, SPEC_OBJ},
	{"wagon_obj", wagon_obj, SPEC_OBJ | SPEC_RES},
	{"loud_speaker", loud_speaker, SPEC_OBJ | SPEC_RES},
	{"fountain_heal", fountain_heal, SPEC_OBJ},
	{"fountain_restore", fountain_restore, SPEC_OBJ | SPEC_RES},
	{"library", library, SPEC_OBJ},
	{"arena_object", arena_object, SPEC_OBJ | SPEC_RES},
	{"javelin_of_lightning", javelin_of_lightning, SPEC_OBJ},
	{"modrian_fountain_obj", modrian_fountain_obj, SPEC_OBJ | SPEC_RES},
	{"stepping_stone", stepping_stone, SPEC_OBJ | SPEC_RES},
	{"portal_out", portal_out, SPEC_OBJ | SPEC_RES},
	{"vault_door", vault_door, SPEC_OBJ | SPEC_RES},
	{"vein", vein, SPEC_OBJ | SPEC_RES},
	{"ramp_leaver", ramp_leaver, SPEC_OBJ | SPEC_RES},
	{"portal_home", portal_home, SPEC_OBJ},
	{"cloak_of_deception", cloak_of_deception, SPEC_OBJ},
	{"newspaper", newspaper, SPEC_OBJ},
	{"sunstation", sunstation, SPEC_OBJ},
	{"horn_of_geryon", horn_of_geryon, SPEC_OBJ | SPEC_RES},
	{"unholy_compact", unholy_compact, SPEC_MOB | SPEC_RES},
	{"telescope", telescope, SPEC_OBJ},
	{"fate_cauldron", fate_cauldron, SPEC_OBJ | SPEC_RES},
	{"fate_portal", fate_portal, SPEC_OBJ | SPEC_RES},
	{"quantum_rift", quantum_rift, SPEC_OBJ | SPEC_RES},
	{"roaming_portal", roaming_portal, SPEC_OBJ | SPEC_RES},
	{"tester_util", tester_util, SPEC_OBJ | SPEC_RES},
	{"typo_util", typo_util, SPEC_OBJ | SPEC_RES},
	{"labyrinth_clock", labyrinth_clock, SPEC_OBJ | SPEC_RES},
	{"drink_me_bottle", drink_me_bottle, SPEC_OBJ | SPEC_RES},
	{"rabbit_hole", rabbit_hole, SPEC_OBJ | SPEC_RES},
	{"astral_portal", astral_portal, SPEC_OBJ},
	{"vr_arcade_game", vr_arcade_game, SPEC_OBJ},
	{"astrolabe", astrolabe, SPEC_OBJ | SPEC_RES},
	{"black_rose", black_rose, SPEC_OBJ},
	{"vehicle_console", vehicle_console, SPEC_OBJ},
	{"vehicle_door", vehicle_door, SPEC_OBJ},
	{"djinn_lamp", djinn_lamp, SPEC_OBJ | SPEC_RES},
	{"improve_stat_book", improve_stat_book, SPEC_OBJ | SPEC_RES},
	{"improve_prac_book", improve_prac_book, SPEC_OBJ | SPEC_RES},
	{"life_point_potion", life_point_potion, SPEC_OBJ | SPEC_RES},
	{"master_communicator", master_communicator, SPEC_OBJ | SPEC_RES},
	{"fountain_good", fountain_good, SPEC_OBJ},
	{"fountain_evil", fountain_evil, SPEC_OBJ},
	{"unholy_square", unholy_square, SPEC_OBJ | SPEC_RES},
	{"weapon_lister", weapon_lister, SPEC_OBJ | SPEC_RES},
	{"ancient_artifact", ancient_artifact, SPEC_OBJ | SPEC_RES},
	{"finger_of_death", finger_of_death, SPEC_OBJ | SPEC_RES},

	{"dukes_chamber", dukes_chamber, SPEC_RM | SPEC_RES},
	{"dump", dump, SPEC_RM | SPEC_RES},
	{"pet_shops", pet_shops, SPEC_RM},
	{"wagon_room", wagon_room, SPEC_RM | SPEC_RES},
	{"entrance_to_cocks", entrance_to_cocks, SPEC_RM},
	{"entrance_to_brawling", entrance_to_brawling, SPEC_RM},
	{"shimmering_portal", shimmering_portal, SPEC_RM | SPEC_RES},
	{"newbie_tower_rm", newbie_tower_rm, SPEC_RM | SPEC_RES},
	{"newbie_stairs_rm", newbie_stairs_rm, SPEC_RM | SPEC_RES},
	{"newbie_cafe_rm", newbie_cafe_rm, SPEC_RM | SPEC_RES},
	{"newbie_portal_rm", newbie_portal_rm, SPEC_RM | SPEC_RES},
	{"gen_shower_rm", gen_shower_rm, SPEC_RM},
	{"gingwatzim", gingwatzim, SPEC_RM | SPEC_RES},
	{"falling_tower_dt", falling_tower_dt, SPEC_RM | SPEC_RES},
	{"stable_room", stable_room, SPEC_RM},
	{"mystical_enclave", mystical_enclave, SPEC_RM | SPEC_RES},
	{"stygian_lightning_rm", stygian_lightning_rm, SPEC_RM},
	{"donation_room", donation_room, SPEC_RM},
	{"pendulum_room", pendulum_room, SPEC_RM | SPEC_RES},
	{"monastery_eating", monastery_eating, SPEC_RM | SPEC_RES},
	{"modrian_fountain_rm", modrian_fountain_rm, SPEC_RM | SPEC_RES},
	{"malbolge_bridge", malbolge_bridge, SPEC_RM | SPEC_RES},
	{"abandoned_cavern", abandoned_cavern, SPEC_RM | SPEC_RES},
	{"dangerous_climb", dangerous_climb, SPEC_RM},
	{"windy_room", windy_room, SPEC_RM},
	{"killzone_room", killzone_room, SPEC_RM | SPEC_RES},
	{"hell_domed_chamber", hell_domed_chamber, SPEC_RM | SPEC_RES},
	{"malagard_lightning_room", malagard_lightning_room, SPEC_RM | SPEC_RES},
	{"vendor", vendor, SPEC_MOB},
	{"voting_booth", voting_booth, SPEC_OBJ},
	{"fountain_youth", fountain_youth, SPEC_OBJ},
	{"clone_lab", clone_lab, SPEC_RM | SPEC_RES},
	{"quest_sphere", quest_sphere, SPEC_OBJ | SPEC_RES},
	{"demonic_overmind", demonic_overmind, SPEC_MOB | SPEC_RES},
	{"demonic_guard", demonic_guard, SPEC_MOB | SPEC_RES},
	{"guardian_angel", guardian_angel, SPEC_MOB | SPEC_RES},
	{"mage_teleporter", mage_teleporter, SPEC_MOB | SPEC_RES},
	{"languagemaster", languagemaster, SPEC_MOB},
	{"bounty_clerk", bounty_clerk, SPEC_MOB},
	{NULL, NULL, 0}				//terminator
};

const char *spec_flags[] = { "MOB", "OBJ", "ROOM", "RESERVED" };

//
// find_spec_index_ptr - given a spec pointer, return the index
//

int
find_spec_index_ptr(SPECIAL(*func))
{
	int i;

	for (i = 0; spec_list[i].tag != NULL && i < 300; i++)
		if (func == spec_list[i].func)
			return (i);

	return (-1);
}

//
// find_spec_index_arg - given a spec name, return the index
//

int
find_spec_index_arg(char *arg)
{
	int i;

	for (i = 0; spec_list[i].tag != NULL && i < 300; i++) {
		if (!strncmp(spec_list[i].tag, arg, strlen(arg))) {
			return (i);
		}
	}


	return (-1);
}

//
// do_specasiign_save - make a snapshot of mob,obj, or room spec assignments
//                      to the files etc/spec_ass_{mob,obj,room}
//

int
do_specassign_save(struct Creature *ch, int mode)
{

	FILE *file;
	int index;
	struct obj_data *obj = NULL;
	struct Creature *mob = NULL;
	struct room_data *room = NULL;
	struct zone_data *zone = NULL;

	if (!mode || IS_SET(mode, SPEC_MOB)) {
		if (!(file = fopen(SPEC_FILE_MOB, "w"))) {
			errlog("Error opening mob spec file for write.");
			return 1;
		}
		MobileMap::iterator mit = mobilePrototypes.begin();
		for (; mit != mobilePrototypes.end(); ++mit) {
			mob = mit->second;
			if (mob->mob_specials.shared->func) {
				if ((index = find_spec_index_ptr(mob->mob_specials.shared->
							func)) < 0)
					continue;
				fprintf(file, "%-6d %-20s ## %s\n",
					GET_MOB_VNUM(mob), spec_list[index].tag,
					GET_NAME(mob));
			}
		}
		fclose(file);
	}

	if (!mode || IS_SET(mode, SPEC_OBJ)) {
		if (!(file = fopen(SPEC_FILE_OBJ, "w"))) {
			errlog("Error opening obj spec file for write.");
			return 1;
		}
//		for (obj = obj_proto; obj; obj = obj->next) {
    ObjectMap::iterator oi = objectPrototypes.begin();
    for (; oi != objectPrototypes.end(); ++oi) {
        obj = oi->second;
			if (obj->shared->func) {
				if ((index = find_spec_index_ptr(obj->shared->func)) < 0)
					continue;
				fprintf(file, "%-6d %-20s ## %s\n",
					GET_OBJ_VNUM(obj), spec_list[index].tag,
					obj->name);
			}
		}
		fclose(file);
	}

	if (!mode || IS_SET(mode, SPEC_RM)) {
		if (!(file = fopen(SPEC_FILE_RM, "w"))) {
			errlog("Error opening room spec file for write.");
			return 1;
		}
		for (zone = zone_table; zone; zone = zone->next)
			for (room = zone->world; room; room = room->next) {
				if (room->func) {
					if ((index = find_spec_index_ptr(room->func)) < 0)
						continue;
					fprintf(file, "%-6d %-20s ## %s\n",
						room->number, spec_list[index].tag,
						room->name);
				}
			}
		fclose(file);
	}
	if (!mode)
		slog("%s saved all spec assign files.", GET_NAME(ch));
	return 0;
}

//
// do_show_specials
//

void
do_show_specials(struct Creature *ch, char *arg)
{

	int mode_all = 0, mode_mob = 0, mode_obj = 0, mode_room = 0;
	char arg1[MAX_INPUT_LENGTH], outbuf[MAX_STRING_LENGTH];
	int i;

	if (!*arg)
		mode_all = 1;
	else {
		arg = one_argument(arg, arg1);
		while (*arg1) {
			if (is_abbrev(arg1, "mobiles"))
				mode_mob = 1;
			else if (is_abbrev(arg1, "objects"))
				mode_obj = 1;
			else if (is_abbrev(arg1, "rooms"))
				mode_room = 1;
			else if (is_abbrev(arg1, "all"))
				mode_all = 1;
			else {
				send_to_char(ch, "Unknown show special option: '%s'\r\n", arg1);
			}
			arg = one_argument(arg, arg1);
		}
	}

	strcpy(outbuf, "SPECIAL PROCEDURES::                FLAGS::\r\n");
	for (i = 0; spec_list[i].tag && i < 300; i++) {
		if (!mode_all &&
			(!mode_mob || !IS_SET(spec_list[i].flags, SPEC_MOB)) &&
			(!mode_obj || !IS_SET(spec_list[i].flags, SPEC_OBJ)) &&
			(!mode_room || !IS_SET(spec_list[i].flags, SPEC_RM)))
			continue;
		if (spec_list[i].flags)
			sprintbit(spec_list[i].flags, spec_flags, buf2);
		else
			strcpy(buf2, "NONE");

		sprintf(buf, "  %s%-30s%s   (%s%s%s)\r\n",
			CCYEL(ch, C_NRM), spec_list[i].tag, CCNRM(ch, C_NRM),
			CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
		if (strlen(buf) + strlen(outbuf) > MAX_STRING_LENGTH - 128) {
			strcat(outbuf, "**OVERFLOW**\r\n");
			break;
		} else
			strcat(outbuf, buf);
	}
	page_string(ch->desc, outbuf);
}

//
// do_special -- a utility command
//  

ACMD(do_special)
{

	const char *special_cmds[] = {
		"show",
		"save",
		"\n"
	};
	int spec_cmd;

	skip_spaces(&argument);
	argument = one_argument(argument, buf);
	if ((spec_cmd = search_block(buf, special_cmds, 0)) < 0) {
		send_to_char(ch, "Invalid special command: show or save.\r\n");
		return;
	}

	switch (spec_cmd) {
	case 0:  /** show **/
		do_show_specials(ch, argument);
		break;

	case 1:
		if (do_specassign_save(ch, 0))
			send_to_char(ch, "There was an error saving the file.\r\n");
		else
			send_to_char(ch, "Special assignments saved.\r\n");
		break;
	default:
		errlog("Invalid command reached in do_special.");
		break;
	}
	return;
}

//
// assign_mobiles
//

void
assign_mobiles(void)
{

	FILE *file;
	int vnum, index;
	char *ptr_name, *str;
	struct Creature *mob = NULL;


	file = fopen(SPEC_FILE_MOB, "r");
	if (!file) {
		slog("Fatal error opening mob spec file: %s", strerror(errno));
		safe_exit(1);
	}

	while (!feof(file) && !ferror(file)) {
		if (!get_line(file, buf))
			break;
		
		// eliminate comments
		str = strstr(buf, "##");
		if (str) {
			*str-- = '\0';
			while (str != buf && isspace(*str))
				*str-- = '\0';
		}

		str = buf;
		vnum = atoi(tmp_getword(&str));
		ptr_name = tmp_getword(&str);
		if (!ptr_name) {
			slog("Format error in mob spec file.");
			safe_exit(1);
		}

		// Get the mobile
		mob = real_mobile_proto(vnum);
		if (!mob) {
			if (!mini_mud)
				slog("Error in mob spec file: mobile <%d> not exist.",
					vnum);
			continue;
		}

		// Find the spec
		index = find_spec_index_arg(ptr_name);
		if (index < 0)
			slog("Error in mob spec file: ptr <%s> not exist.", ptr_name);
		else if (!IS_SET(spec_list[index].flags, SPEC_MOB))
			slog("Attempt to assign ptr <%s> to a mobile.", ptr_name);
		else
			mob->mob_specials.shared->func = spec_list[index].func;
		
	}
	fclose(file);
}

//
// assign_objects
//

void
assign_objects(void)
{

	FILE *file;
	int vnum, index;
	char *str, *ptr_name;
	struct obj_data *obj = NULL;


	if (!(file = fopen(SPEC_FILE_OBJ, "r"))) {
		slog("Fatal error opening obj spec file.");
		safe_exit(1);
	}

	while (!feof(file) && !ferror(file)) {
		if (!get_line(file, buf))
			break;
		// eliminate comments
		str = strstr(buf, "##");
		if (str) {
			*str-- = '\0';
			while (str != buf && isspace(*str))
				*str-- = '\0';
		}

		str = buf;
		vnum = atoi(tmp_getword(&str));
		ptr_name = tmp_getword(&str);
		if (!ptr_name) {
			slog("Format error in mob spec file.");
			safe_exit(1);
		}

		if (!(obj = real_object_proto(vnum))) {
			if (!mini_mud) {
				slog("Error in obj spec file: object <%d> not exist.",
					vnum);
			}
		} else if ((index = find_spec_index_arg(ptr_name)) < 0) {
			slog("Error in obj spec file: ptr <%s> not exist.",
				ptr_name);
		} else if (!IS_SET(spec_list[index].flags, SPEC_OBJ)) {
			slog("Attempt to assign ptr <%s> to a object.", ptr_name);
		} else {
			obj->shared->func = spec_list[index].func;
		}
	}
	fclose(file);
}


//
// assign_rooms
//

void
assign_rooms(void)
{

	FILE *file;
	int vnum, index;
	char *str, *ptr_name;
	struct room_data *rm = NULL;

	if (!(file = fopen(SPEC_FILE_RM, "r"))) {
		slog("Fatal error opening room spec file.");
		safe_exit(1);
	}

	while (!feof(file) && !ferror(file)) {
		if (!get_line(file, buf))
			break;

		// eliminate comments
		str = strstr(buf, "##");
		if (str) {
			*str-- = '\0';
			while (str != buf && isspace(*str))
				*str-- = '\0';
		}

		str = buf;
		vnum = atoi(tmp_getword(&str));
		ptr_name = tmp_getword(&str);
		if (!ptr_name) {
			slog("Format error in mob spec file.");
			safe_exit(1);
		}

		if (!(rm = real_room(vnum))) {
			if (!mini_mud) {
				slog("Error in room spec file: room <%d> not exist.",
					vnum);
			}
		} else if ((index = find_spec_index_arg(ptr_name)) < 0) {
			slog("Error in room spec file: ptr <%s> not exist.",
				ptr_name);
		} else if (!IS_SET(spec_list[index].flags, SPEC_RM)) {
			slog("Attempt to assign ptr <%s> to a room.", ptr_name);
		} else {
			rm->func = spec_list[index].func;
		}
	}
	fclose(file);
}
