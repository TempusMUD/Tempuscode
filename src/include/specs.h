//
// File: specs.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//
//#define FATE_TEST 1
#define QUANTUM_RIFT_VNUM 1217
#define FATE_PORTAL_VNUM 1216
#ifndef FATE_TEST
#define FATE_VNUM_LOW 1526
#define FATE_VNUM_MID 1527
#define FATE_VNUM_HIGH 1528
#endif
#ifdef FATE_TEST
#define FATE_VNUM_LOW 1205
#define FATE_VNUM_MID 1206
#define FATE_VNUM_HIGH 1207
#endif

#ifndef __specs_h__
#define __specs_h__

SPECIAL(postmaster);
SPECIAL(cityguard);
SPECIAL(receptionist);
SPECIAL(cryogenicist);
SPECIAL(guild_guard);
SPECIAL(guild);
SPECIAL(puff);
SPECIAL(fido);
SPECIAL(buzzard);
SPECIAL(garbage_pile);
SPECIAL(janitor);
SPECIAL(elven_janitor);
SPECIAL(gelatinous_blob);
SPECIAL(snake);
SPECIAL(thief);
SPECIAL(magic_user);
SPECIAL(asp);
SPECIAL(medusa);
SPECIAL(temple_healer);
SPECIAL(barbarian);
SPECIAL(thieves_guard_south);
SPECIAL(thieves_guard_west);
SPECIAL(chest_mimic);
SPECIAL(rat_mama);
SPECIAL(improve_dex);
SPECIAL(improve_int);
SPECIAL(improve_con);
SPECIAL(improve_str);
SPECIAL(improve_cha);
SPECIAL(improve_wis);
SPECIAL(wagon_driver);
SPECIAL(fire_breather);
SPECIAL(energy_drainer);
SPECIAL(basher);
SPECIAL(ornery_goat);
SPECIAL(Aziz);
SPECIAL(circus_clown);
SPECIAL(grandmaster);
SPECIAL(cock_fight);
SPECIAL(cyber_cock);
SPECIAL(cave_bear);
SPECIAL(juju_zombie);
SPECIAL(cemetary_ghoul);
SPECIAL(phantasmic_sword);
SPECIAL(newbie_healer);
SPECIAL(newbie_improve);
SPECIAL(newbie_fly);
SPECIAL(gen_locker);
SPECIAL(gen_guard);
SPECIAL(newbie_gold_coupler);
SPECIAL(maze_switcher);
SPECIAL(maze_cleaner);
SPECIAL(darom);
SPECIAL(puppet);
SPECIAL(oracle);
SPECIAL(dt_cleaner);
SPECIAL(sleeping_soldier);
SPECIAL(registry);
SPECIAL(corpse_retrieval);
SPECIAL(carrion_crawler);
SPECIAL(ogre1);
SPECIAL(arena_locker);
SPECIAL(killer_hunter);
SPECIAL(remorter);
SPECIAL(nohunger_dude);
SPECIAL(dwarven_hermit);
SPECIAL(gunnery_device);
SPECIAL(ramp_leaver);
SPECIAL(aziz_canon);
SPECIAL(guard_east);
SPECIAL(guard_up);
SPECIAL(guard_north);
SPECIAL(safiir);
SPECIAL(spinal);
SPECIAL(fate);
SPECIAL(red_highlord);
SPECIAL(tiamat);
SPECIAL(beer_tree);
SPECIAL(kata);
SPECIAL(insane_merchant);
SPECIAL(astral_deva);
SPECIAL(hell_hound);
SPECIAL(geryon);
SPECIAL(moloch);
SPECIAL(bearded_devil);
SPECIAL(cyberfiend);
SPECIAL(jail_locker);
SPECIAL(mob_helper);
SPECIAL(healing_ranger);
SPECIAL(battlefield_ghost);
SPECIAL(battle_cleric);
SPECIAL(electronics_school);
SPECIAL(lawyer);
SPECIAL(electrician);
SPECIAL(paramedic);
SPECIAL(underwater_predator);
SPECIAL(high_priestess);
SPECIAL(gollum);
SPECIAL(cuckoo);
SPECIAL(pendulum_timer_mob);
SPECIAL(parrot);
SPECIAL(watchdog);
SPECIAL(cyborg_overhaul);
SPECIAL(credit_exchange);
SPECIAL(gold_exchange);
SPECIAL(repairer);
SPECIAL(junker);
SPECIAL(bank);
SPECIAL(reinforcer);
SPECIAL(enhancer);
/*SPECIAL(life_bldg_guard); */
SPECIAL(archon);
SPECIAL(taunting_frenchman);
SPECIAL(CastleGuard);
SPECIAL(James);
SPECIAL(cleaning);
SPECIAL(DicknDavid);
SPECIAL(tim);
SPECIAL(tom);
SPECIAL(duke_araken);
SPECIAL(training_master);
SPECIAL(peter);
SPECIAL(jerry);
SPECIAL(lounge_soldier);
SPECIAL(chess_guard_no_east);
SPECIAL(chess_guard_no_west);
SPECIAL(armory_guard);
SPECIAL(armory_person);
SPECIAL(increaser);
SPECIAL(quasimodo);
SPECIAL(hell_hunter);
SPECIAL(hell_hunter_brain);
SPECIAL(arioch);
SPECIAL(maladomini_jailer);
SPECIAL(hell_regulator);
SPECIAL(hell_ressurector);
SPECIAL(hell_domed_chamber);
SPECIAL(killzone_room);
SPECIAL(malagard_lightning_room);
SPECIAL(pit_keeper);
SPECIAL(cremator);
SPECIAL(underworld_goddess);
SPECIAL(duke_nukem);
SPECIAL(implanter);
SPECIAL(nude_guard);
SPECIAL(spirit_priestess);
SPECIAL(corpse_griller);
SPECIAL(head_shrinker);
SPECIAL(multi_healer);
SPECIAL(slaver);
SPECIAL(tarrasque);

SPECIAL(cheeky_monkey);
SPECIAL(hobbes_speaker);

SPECIAL(rust_monster);
SPECIAL(weaponsmaster);
SPECIAL(morkoth);
SPECIAL(unspecializer);

// lovely_specs
SPECIAL(mavernal_citizen);
SPECIAL(new_mavernal_talker);

// disaster_specs
SPECIAL(boulder_thrower);
SPECIAL(windy_room);

// utility specs
SPECIAL(gen_board);
SPECIAL(wagon_obj);
SPECIAL(fountain_heal);
SPECIAL(loud_speaker);
SPECIAL(library);
SPECIAL(arena_object);
SPECIAL(javelin_of_lightning);
SPECIAL(modrian_fountain_obj);
SPECIAL(stepping_stone);
SPECIAL(portal_out);
SPECIAL(vault_door);
SPECIAL(vein);
SPECIAL(ramp_leaver);
SPECIAL(portal_home);
SPECIAL(cloak_of_deception);
SPECIAL(newspaper);
SPECIAL(sunstation);
SPECIAL(horn_of_geryon);
SPECIAL(unholy_compact);
SPECIAL(telescope);
SPECIAL(fate_cauldron);
SPECIAL(fate_portal);
SPECIAL(quantum_rift);
SPECIAL(roaming_portal);
SPECIAL(tester_util);
SPECIAL(typo_util);
SPECIAL(questor_util);
SPECIAL(labyrinth_clock);
SPECIAL(drink_me_bottle);
SPECIAL(rabbit_hole);
SPECIAL(astral_portal);
SPECIAL(vr_arcade_game);
SPECIAL(astrolabe);
SPECIAL(book_int);
SPECIAL(black_rose);
SPECIAL(vehicle_console);
SPECIAL(vehicle_door);
SPECIAL(djinn_lamp);
SPECIAL(improve_stat_book);
SPECIAL(improve_prac_book);
SPECIAL(life_point_potion);
SPECIAL(shade_zone);

SPECIAL(chastity_belt);
SPECIAL(anti_spank_device);

SPECIAL(dump);
SPECIAL(pet_shops);
SPECIAL(wagon_room);
SPECIAL(entrance_to_cocks);
SPECIAL(entrance_to_brawling);
SPECIAL(shimmering_portal);
SPECIAL(physic_guildguard);
SPECIAL(newbie_tower_rm);
SPECIAL(newbie_cafe_rm);
SPECIAL(newbie_portal_rm);
SPECIAL(newbie_stairs_rm);
SPECIAL(gen_shower_rm);
SPECIAL(gingwatzim);
SPECIAL(falling_tower_dt);
SPECIAL(stable_room);
SPECIAL(mystical_enclave);
SPECIAL(stygian_lightning_rm);
SPECIAL(donation_room);
SPECIAL(pendulum_room);
SPECIAL(monastery_eating);
SPECIAL(modrian_fountain_rm);
/*SPECIAL(borg_guild_entrance); */
SPECIAL(malbolge_bridge);
SPECIAL(abandoned_cavern);
SPECIAL(dangerous_climb);

SPECIAL(dukes_chamber);
SPECIAL(wiz_library);

SPECIAL(master_communicator);
SPECIAL(fountain_good);
SPECIAL(fountain_evil);
SPECIAL(life_point_potion);

SPECIAL(elevator_panel);

SPECIAL(unholy_square);
SPECIAL(unholy_stalker);
SPECIAL(voting_booth);
SPECIAL(fountain_youth);
SPECIAL(clone_lab);

#define SPEC_MOB 1
#define SPEC_OBJ 2
#define SPEC_RM  4
#define SPEC_RES 8

struct spec_func_data {
	char *tag;
//  void *func;
	 SPECIAL(*func);
	byte flags;
};

int
 find_spec_index_arg(char *arg);
void
 do_show_specials(struct char_data *ch, char *arg);
int
 do_specassign_save(struct char_data *ch, int mode);
int
 find_spec_index_ptr(SPECIAL(*func));
int
 mob_read_script(struct char_data *ch);

#define SPEC_FILE_MOB "etc/spec_ass_mob"
#define SPEC_FILE_OBJ "etc/spec_ass_obj"
#define SPEC_FILE_RM  "etc/spec_ass_wld"

#ifndef __spec_ass_c__

extern const struct spec_func_data spec_list[];

#endif

#endif
