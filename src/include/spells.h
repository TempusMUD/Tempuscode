/* ************************************************************************
*   File: spells.h                                      Part of CircleMUD *
*  Usage: header file: constants and fn prototypes for spell system       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: spells.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#ifndef __spells_h__
#define __spells_h__

#ifndef __spell_parser_c__

extern const char *spells[];

#endif

#define DEFAULT_STAFF_LVL	12
#define DEFAULT_WAND_LVL	12

#define CAST_UNDEFINED	-1
#define CAST_SPELL	0
#define CAST_POTION	1
#define CAST_WAND	2
#define CAST_STAFF	3
#define CAST_SCROLL	4
#define CAST_PARA       5
#define CAST_PETRI      6
#define CAST_ROD        7
#define CAST_BREATH     8
#define CAST_CHEM       9
#define CAST_PSIONIC    10
#define CAST_PHYSIC     11
#define CAST_INTERNAL   12

#define MAG_DAMAGE	(1 << 0)
#define MAG_AFFECTS	(1 << 1)
#define MAG_UNAFFECTS	(1 << 2)
#define MAG_POINTS	(1 << 3)
#define MAG_ALTER_OBJS	(1 << 4)
#define MAG_GROUPS	(1 << 5)
#define MAG_MASSES	(1 << 6)
#define MAG_AREAS	(1 << 7)
#define MAG_SUMMONS	(1 << 8)
#define MAG_CREATIONS	(1 << 9)
#define MAG_MANUAL	(1 << 10)
#define MAG_OBJECTS	(1 << 11)
#define MAG_TOUCH       (1 << 12)
#define MAG_MAGIC       (1 << 13)
#define MAG_DIVINE      (1 << 14)
#define MAG_PHYSICS     (1 << 15)
#define MAG_PSIONIC     (1 << 16)
#define MAG_BIOLOGIC    (1 << 17)
#define CYB_ACTIVATE    (1 << 18)
#define MAG_EVIL        (1 << 19)
#define MAG_GOOD        (1 << 20)
#define MAG_EXITS       (1 << 21)
#define MAG_OUTDOORS    (1 << 22)
#define MAG_NOWATER     (1 << 23)
#define MAG_WATERZAP    (1 << 24)
#define MAG_NOSUN       (1 << 25)

#define SPELL_IS_MAGIC(splnm)   IS_SET(spell_info[splnm].routines, MAG_MAGIC)
#define SPELL_IS_DIVINE(splnm)  IS_SET(spell_info[splnm].routines, MAG_DIVINE)
#define SPELL_IS_PHYSICS(splnm) IS_SET(spell_info[splnm].routines, MAG_PHYSICS)
#define SPELL_IS_PSIONIC(splnm) IS_SET(spell_info[splnm].routines, MAG_PSIONIC)
#define SPELL_IS_BIO(splnm)    IS_SET(spell_info[splnm].routines, MAG_BIOLOGIC)
#define SPELL_IS_EVIL(splnm)    IS_SET(spell_info[splnm].routines, MAG_EVIL)
#define SPELL_IS_GOOD(splnm)    IS_SET(spell_info[splnm].routines, MAG_GOOD)

#define SPELL_FLAGS(splnm)     (spell_info[splnm].routines)
#define SPELL_FLAGGED(splnm, flag) (IS_SET(SPELL_FLAGS(splnm), flag))
 
#define TYPE_UNDEFINED               -1
#define SPELL_RESERVED_DBC            0  /* SKILL NUMBER ZERO -- RESERVED */
#define SPELL_LEVEL(spl, char_class)        (spell_info[spl].min_level[char_class])
#define SPELL_GEN(spl, char_class)          (spell_info[spl].gen[char_class])
#define ABLE_TO_LEARN(ch, spl) \
((GET_REMORT_GEN(ch) >= SPELL_GEN(spl, GET_CLASS(ch)) && \
  GET_LEVEL(ch) >= SPELL_LEVEL(spl, GET_CLASS(ch))) || \
 (IS_REMORT(ch) && GET_LEVEL(ch) >= SPELL_LEVEL(spl, GET_REMORT_CLASS(ch)) && \
  !SPELL_GEN(spl, GET_REMORT_CLASS(ch))))

#define UNHOLY_STALKER_VNUM 1513
#define ZOMBIE_VNUM         1512
  
  /* PLAYER SPELLS -- Numbered from 1 to MAX_SPELLS */

#define SPELL_ARMOR                   1 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_TELEPORT                2 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BLESS                   3 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BLINDNESS               4 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BURNING_HANDS           5 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CALL_LIGHTNING          6 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CHARM                   7 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CHILL_TOUCH             8 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CLONE                   9 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_COLOR_SPRAY            10 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CONTROL_WEATHER        11 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CREATE_FOOD            12 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CREATE_WATER           13 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_BLIND             14 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_CRITIC            15 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_LIGHT             16 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURSE                  17 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_ALIGN           18 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_INVIS           19 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_MAGIC           20 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_POISON          21 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DISPEL_EVIL            22 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_EARTHQUAKE             23 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ENCHANT_WEAPON         24 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ENERGY_DRAIN           25 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_FIREBALL               26 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_HARM                   27 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_HEAL                   28 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_INVISIBLE              29 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_LIGHTNING_BOLT         30 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_LOCATE_OBJECT          31 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_MAGIC_MISSILE          32 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_POISON                 33 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_PROT_FROM_EVIL         34 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_REMOVE_CURSE           35 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SANCTUARY              36 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SHOCKING_GRASP         37 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SLEEP                  38 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_STRENGTH               39 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SUMMON                 40 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_VENTRILOQUATE          41 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_WORD_OF_RECALL         42 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_REMOVE_POISON          43 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SENSE_LIFE             44 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ANIMATE_DEAD	     45 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DISPEL_GOOD	     46 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GROUP_ARMOR	     47 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GROUP_HEAL	     48 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GROUP_RECALL	     49 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_INFRAVISION	     50 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_WATERWALK		     51 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_MANA_SHIELD            52
#define SPELL_IDENTIFY               53	
#define SPELL_GLOWLIGHT              54
#define SPELL_BLUR                   55
#define SPELL_KNOCK                  56
#define SPELL_DISPEL_MAGIC           57
#define SPELL_WARDING_SIGIL          58
#define SPELL_DIMENSION_DOOR         59
#define SPELL_MINOR_CREATION         60
#define SPELL_TELEKINESIS            61
#define SPELL_SWORD                  62
#define SPELL_WORD_STUN              63
#define SPELL_PRISMATIC_SPRAY        64
#define SPELL_FIRE_SHIELD            65
#define SPELL_DETECT_SCRYING         66
#define SPELL_CLAIRVOYANCE           67
#define SPELL_SLOW                   68
#define SPELL_GREATER_ENCHANT        69
#define SPELL_ENCHANT_ARMOR          70
#define SPELL_MINOR_IDENTIFY         71
#define SPELL_FLY                    72
#define SPELL_GREATER_HEAL	     73
#define SPELL_CONE_COLD			74
#define SPELL_TRUE_SEEING		75
#define SPELL_PROT_FROM_GOOD		76
#define SPELL_MAGICAL_PROT		77
#define SPELL_UNDEAD_PROT		78
#define SPELL_SPIRIT_HAMMER		79
#define SPELL_PRAY			80
#define SPELL_FLAME_STRIKE		81
#define SPELL_ENDURE_COLD		82
#define SPELL_MAGICAL_VESTMENT		83
#define SPELL_REJUVENATE		84
#define SPELL_REGENERATE		85
#define SPELL_COMMAND			86
#define SPELL_AIR_WALK			87
#define SPELL_DIVINE_ILLUMINATION	88
#define SPELL_GOODBERRY			89
#define SPELL_BARKSKIN			90
#define SPELL_INVIS_TO_UNDEAD		91
#define SPELL_HASTE			92
#define SPELL_INVIS_TO_ANIMALS          93
#define SPELL_CHARM_ANIMAL		94
#define SPELL_REFRESH			95
#define SPELL_BREATHE_WATER	  	96
#define SPELL_CONJURE_ELEMENTAL	        97
#define SPELL_GREATER_INVIS		98
#define SPELL_PROT_FROM_LIGHTNING       99
#define SPELL_PROT_FROM_FIRE		100
#define SPELL_RESTORATION               101
#define SPELL_WORD_OF_INTELLECT         102
#define SPELL_GUST_OF_WIND              103
#define SPELL_RETRIEVE_CORPSE           104
#define SPELL_LOCAL_TELEPORT            105
#define SPELL_DISPLACEMENT              106
#define SPELL_PEER                      107
#define SPELL_METEOR_STORM              108
#define SPELL_PROTECT_FROM_DEVILS       109
#define SPELL_SYMBOL_OF_PAIN            110
#define SPELL_ICY_BLAST                 111
#define SPELL_ASTRAL_SPELL		112
#define SPELL_VESTIGIAL_RUNE            113
#define SPELL_DISRUPTION                114
#define SPELL_STONE_TO_FLESH            115
#define SPELL_REMOVE_SICKNESS           116
#define SPELL_SHROUD_OBSCUREMENT        117
#define SPELL_PRISMATIC_SPHERE          118
#define SPELL_WALL_OF_THORNS            119
#define SPELL_WALL_OF_STONE             120
#define SPELL_WALL_OF_ICE               121
#define SPELL_WALL_OF_FIRE              122
#define SPELL_WALL_OF_IRON              123
#define SPELL_THORN_TRAP                124
#define SPELL_FIERY_SHEET               125
#define SPELL_CHAIN_LIGHTNING           126
#define SPELL_HAILSTORM                 127
#define SPELL_ICE_STORM                 128
#define SPELL_SHIELD_OF_RIGHTEOUSNESS   129  // group protection
#define SPELL_BLACKMANTLE               130  // blocks healing spells
#define SPELL_SANCTIFICATION            131  // 2x dam vs. evil
#define SPELL_STIGMATA                  132  // causes a bleeding wound
#define SPELL_SUMMON_LEGION             133  // knights summon devils
#define SPELL_ENTANGLE                  134  // rangers entangle in veg.
#define SPELL_ANTI_MAGIC_SHELL          135    
#define SPELL_DIVINE_INTERVENTION       136
#define SPELL_SPHERE_OF_DESECRATION     137
#define SPELL_MALEFIC_VIOLATION         138  // cuts thru good sanct
#define SPELL_RIGHTEOUS_PENETRATION     139  // cuts thru evil sanct
#define SPELL_UNHOLY_STALKER            140  // evil cleric hunter mob
#define SPELL_INFERNO                   141  // evil cleric room affect
#define SPELL_VAMPIRIC_REGENERATION     142  // evil cleric vamp. regen
#define SPELL_BANISHMENT                143  // evil cleric sends devils away
#define SPELL_CONTROL_UNDEAD            144  // evil clerics charm undead
#define SPELL_STONESKIN					145  // remort rangers stone skin
  /************************** Psionic Triggers ***************/
#define SPELL_POWER                201 /* Strength                */
#define SPELL_INTELLECT            202
#define SPELL_CONFUSION		   203
#define SPELL_FEAR	           204
#define SPELL_SATIATION            205 /* fills hunger */
#define SPELL_QUENCH               206 /* fills thirst */
#define SPELL_CONFIDENCE           207 /* sets nopain */
#define SPELL_NOPAIN               208
#define SPELL_DERMAL_HARDENING     209
#define SPELL_WOUND_CLOSURE        210
#define SPELL_ANTIBODY             211
#define SPELL_RETINA               212
#define SPELL_ADRENALINE           213
#define SPELL_BREATHING_STASIS     214
#define SPELL_VERTIGO              215
#define SPELL_METABOLISM           216 /* Increased healing, hunger, thirst */
#define SPELL_EGO_WHIP             217
#define SPELL_PSYCHIC_CRUSH        218
#define SPELL_RELAXATION           219 /* speeds mana regen, weakens char */
#define SPELL_WEAKNESS             220 /* minus str */
#define SPELL_FORTRESS_OF_WILL     221
#define SPELL_CELL_REGEN           222
#define SPELL_PSISHIELD            223
#define SPELL_PSYCHIC_SURGE        224
#define SPELL_PSYCHIC_CONDUIT      225
#define SPELL_PSIONIC_SHATTER      226
#define SPELL_ID_INSINUATION       227
#define SPELL_MELATONIC_FLOOD      228
#define SPELL_MOTOR_SPASM          229
#define SPELL_PSYCHIC_RESISTANCE   230
#define SPELL_MASS_HYSTERIA        231
#define SPELL_GROUP_CONFIDENCE     232
#define SPELL_CLUMSINESS           233
#define SPELL_ENDURANCE            234
#define SPELL_AMNESIA              235  // psi remorts
#define SPELL_NULLPSI              236  // remove psi affects

  /*************************** Physic Alterations *****************/
#define SPELL_ACIDITY              301
#define SPELL_ATTRACTION_FIELD     302
#define SPELL_NUCLEAR_WASTELAND    303
#define SPELL_FLUORESCE	           304
#define SPELL_GAMMA_RAY            305
#define SPELL_HALFLIFE             306
#define SPELL_MICROWAVE            307
#define SPELL_OXIDIZE              308
#define SPELL_RANDOM_COORDINATES   309  // random teleport
#define SPELL_REPULSION_FIELD      310  
#define SPELL_TRANSMITTANCE        311  // transparency
#define SPELL_SPACETIME_IMPRINT    312  // sets room as teleport spot
#define SPELL_SPACETIME_RECALL     313  // teleports to imprint telep spot
#define SPELL_TIME_WARP            314  // random teleport into other time
#define SPELL_TIDAL_SPACEWARP      315  // fly
#define SPELL_FISSION_BLAST        316  // full-room damage
#define SPELL_REFRACTION           317  // like displacement
#define SPELL_ELECTROSHIELD        318  // prot_lightning
#define SPELL_VACUUM_SHROUD        319  // eliminates breathing and fire
#define SPELL_DENSIFY              320  // increase weight of obj & char
#define SPELL_CHEMICAL_STABILITY   321  // prevent/stop acidity
#define SPELL_ENTROPY_FIELD        322  // drains move on victim (time effect)
#define SPELL_GRAV_TRAP            323  // time effect crushing damage
#define SPELL_CAPACITANCE_BOOST    324  // increase maxmv
#define SPELL_ELECTRIC_ARC         325  // lightning bolt
#define SPELL_SONIC_BOOM           326  // area damage + wait state
#define SPELL_LATICE_HARDENING     327  // dermal hard or increase obj maxdam
#define SPELL_NULLIFY              328  // like dispel magic
#define SPELL_FORCE_WALL           329  // sets up an exit blocker
#define SPELL_UNUSED_330           330
#define SPELL_PHASING              331  // invuln.
#define SPELL_ABSORPTION_SHIELD    332  // works like mana shield
#define SPELL_TEMPORAL_COMPRESSION 333  // works like haste
#define SPELL_TEMPORAL_DILATION    334  // works like slow
#define SPELL_GAUSS_SHIELD         335  // half damage from metal
#define SPELL_ALBEDO_SHIELD        336  // reflects e/m attacks
#define SPELL_THERMOSTATIC_FIELD   337  // sets prot_heat + end_cold
#define SPELL_RADIOIMMUNITY        338  // sets prot_rad
#define SPELL_TRANSDIMENSIONALITY  339  // randomly teleport to another plane
#define SPELL_AREA_STASIS          340  // sets !phy room flag
#define SPELL_ELECTROSTATIC_FIELD  341  // protective static field does damage to attackers

  /*********************  MONK ZENS  *******************/
#define ZEN_HEALING                401
#define ZEN_AWARENESS              402
#define ZEN_OBLIVITY               403
#define ZEN_MOTION                 404
#define ZEN_TRANSLOCATION          405
#define ZEN_CELERITY               406

#define MAX_SPELLS		    500

/* PLAYER SKILLS - Numbered from MAX_SPELLS+1 to MAX_SKILLS */
#define SKILL_BACKSTAB              501 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_BASH                  502 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_HIDE                  503 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_KICK                  504 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_PICK_LOCK             505 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_PUNCH                 506 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_RESCUE                507 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_SNEAK                 508 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_STEAL                 509 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_TRACK		    510 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_PILEDRIVE		    511
#define SKILL_SLEEPER	            512
#define SKILL_ELBOW		    513
#define SKILL_KNEE                  514
#define SKILL_BESERK		    515
#define SKILL_STOMP		    516
#define SKILL_BODYSLAM	  	    517
#define SKILL_CHOKE		    518
#define SKILL_CLOTHESLINE	    519
#define SKILL_TAG		    520
#define SKILL_INTIMIDATE	    521
#define SKILL_SPEED_HEALING	    522
#define SKILL_STALK		    523
#define SKILL_HEARING		    524
#define SKILL_BEARHUG		    525
#define SKILL_BITE		    526
#define SKILL_DBL_ATTACK	    527
#define SKILL_BANDAGE		    528
#define SKILL_FIRSTAID	    	    529
#define SKILL_MEDIC		    530
#define SKILL_CONSIDER		    531
#define SKILL_GLANCE		    532
#define SKILL_HEADBUTT              533
#define SKILL_GOUGE                 534
#define SKILL_STUN		    535
#define SKILL_FEIGN		    536
#define SKILL_CONCEAL		    537
#define SKILL_PLANT		    538
#define SKILL_HOTWIRE		    539
#define SKILL_SHOOT		    540
#define SKILL_BEHEAD                541
#define SKILL_DISARM		    542
#define SKILL_SPINKICK	            543
#define SKILL_ROUNDHOUSE	    544
#define SKILL_SIDEKICK		    545
#define SKILL_SPINFIST              546
#define SKILL_JAB                   547
#define SKILL_HOOK                  548
#define SKILL_SWEEPKICK             549
#define SKILL_TRIP                  550
#define SKILL_UPPERCUT              551
#define SKILL_GROINKICK             552
#define SKILL_CLAW                  553
#define SKILL_RABBITPUNCH           554
#define SKILL_IMPALE                555
#define SKILL_PELE_KICK             556
#define SKILL_LUNGE_PUNCH           557
#define SKILL_TORNADO_KICK          558
#define SKILL_CIRCLE                559
#define SKILL_TRIPLE_ATTACK         560

  /*******************  MONK SKILLS  *******************/
#define SKILL_PALM_STRIKE           561
#define SKILL_THROAT_STRIKE         562
#define SKILL_WHIRLWIND        	    563
#define SKILL_HIP_TOSS              564
#define SKILL_COMBO                 565
#define SKILL_DEATH_TOUCH           566
#define SKILL_CRANE_KICK            567
#define SKILL_RIDGEHAND             568
#define SKILL_SCISSOR_KICK          569
#define SKILL_PINCH_ALPHA           570
#define SKILL_PINCH_BETA            571
#define SKILL_PINCH_GAMMA           572
#define SKILL_PINCH_DELTA           573
#define SKILL_PINCH_EPSILON         574
#define SKILL_PINCH_OMEGA           575
#define SKILL_PINCH_ZETA            576
#define SKILL_MEDITATE              577
#define SKILL_KATA                  578
#define SKILL_EVASION               579

#define SKILL_SECOND_WEAPON         580
#define SKILL_SCANNING              581
#define SKILL_CURE_DISEASE          582
#define SKILL_BATTLE_CRY            583
#define SKILL_AUTOPSY               584
#define SKILL_TRANSMUTE             585 /* physic transmute objs */
#define SKILL_METALWORKING          586
#define SKILL_LEATHERWORKING        587
#define SKILL_DEMOLITIONS           588
#define SKILL_PSIBLAST              589
#define SKILL_PSILOCATE             590
#define SKILL_PSIDRAIN              591  /* drain mana from vict */
#define SKILL_GUNSMITHING           592  /* repair gunz */
#define SKILL_ELUSION               593  /* !track */
#define SKILL_PISTOLWHIP            594  
#define SKILL_CROSSFACE             595  /* rifle whip */
#define SKILL_WRENCH                596  /* break neck */
#define SKILL_CRY_FROM_BEYOND       597
#define SKILL_KIA                   598
#define SKILL_WORMHOLE              599 // physic's wormhole
#define SKILL_LECTURE               600 // physic's boring-ass lecture

#define SKILL_TURN                  601 /* Cleric's turn               */
#define SKILL_ANALYZE               602 /* Physic's analysis           */
#define SKILL_EVALUATE              603 /* Physic's evaluation      */
#define SKILL_HOLY_TOUCH	    604 /* Knight's skill              */
#define SKILL_NIGHT_VISION	    605
#define SKILL_EMPOWER	            606
#define SKILL_SWIMMING		    607
#define SKILL_THROWING              608
#define SKILL_RIDING                609
#define SKILL_PIPEMAKING            610 /* Make a pipe!                */


  /*****************  CYBORG SKILLS  ********************/
#define SKILL_RECONFIGURE           613 /* Re-allocate stats */
#define SKILL_REBOOT                614 /* Start over from scratch */
#define SKILL_MOTION_SENSOR         615 /* Detect Entries into room */
#define SKILL_STASIS                616 /* State of rapid healing */
#define SKILL_ENERGY_FIELD          617 /* Protective field */
#define SKILL_OVERDRIVE             618 /* Speeds up processes */
#define SKILL_POWER_BOOST           619 /* Increases Strength */
#define SKILL_UNUSED_1              620 /**/
#define SKILL_FASTBOOT              621 /* Reboots are faster */
#define SKILL_SELF_DESTRUCT         622 /* Effective self destructs */
#define SKILL_UNUSED_2              623 /**/
#define SKILL_BIOSCAN               624 /* Sense Life scan */
#define SKILL_DISCHARGE             625 /* Discharge attack */
#define SKILL_SELFREPAIR            626 /* Repair hit points */
#define SKILL_CYBOREPAIR            627 /* Repair other borgs */
#define SKILL_OVERHAUL              628 /* Overhaul other borgs */
#define SKILL_DAMAGE_CONTROL        629 /* Damage Control System */
#define SKILL_ELECTRONICS           630 /* Operation of Electronics */
#define SKILL_HACKING               631 /* hack electronic systems */
#define SKILL_CYBERSCAN             632 /* scan others for implants */
#define SKILL_CYBO_SURGERY          633 /* implant objects */
#define SKILL_ENERGY_WEAPONS        634 /* energy weapon use */
#define SKILL_PROJ_WEAPONS          635 /* projectile weapon use */
#define SKILL_SPEED_LOADING         636 /* speed load weapons */
#define SKILL_HYPERSCAN             637 /* aware of hidden objs and traps */
#define SKILL_OVERDRAIN             638 /* overdrain batteries */
#define SKILL_DE_ENERGIZE           639 // drain energy from chars
#define SKILL_ASSIMILATE            640 // assimilate objects
#define SKILL_RADIONEGATION         641 // immunity to radiation

#define SKILL_RETREAT               648 // controlled flee
#define SKILL_DISGUISE              649 // look like a mob
#define SKILL_AMBUSH                650 /* surprise victim */

  /****************** VAMPIRE SKILLS  *******************/
#define SKILL_FLYING		    651
#define SKILL_SUMMON                652
#define SKILL_FEED                  653
#define SKILL_DRAIN                 654
#define SKILL_BEGUILE               655
#define SKILL_CREATE_VAMPIRE	    656
#define SKILL_CONTROL_UNDEAD        657
#define SKILL_TERRORIZE             658

  /***************  Hood Skill... yeah. Just one ********/
  /***************   Ok. now they have 2.  *****/
#define SKILL_HAMSTRING				666
#define SKILL_SNATCH				667

#define SKILL_ENERGY_CONVERSION     679 // physic's energy conversion


  /******************  PROFICENCIES  *******************/
#define SKILL_PROF_POUND            681
#define SKILL_PROF_WHIP             682
#define SKILL_PROF_PIERCE           683
#define SKILL_PROF_SLASH            684
#define SKILL_PROF_CRUSH	    685
#define SKILL_PROF_BLAST	    686
#define SKILL_BREAK_DOOR            687
#define SKILL_ARCHERY               688
#define SKILL_BOW_FLETCH            689
#define SKILL_READ_SCROLLS          690
#define SKILL_USE_WANDS             691


/* New skills may be added here up to MAX_SKILLS (700) */


/*
 *  NON-PLAYER AND OBJECT SPELLS AND SKILLS
 *  The practice levels for the spells and skills below are _not_ recorded
 *  in the playerfile; therefore, the intended use is for spells and skills
 *  associated with objects (such as SPELL_IDENTIFY used with scrolls of
 *  identify) or non-players (such as NPC-only spells).
 */

#define SPELL_FIRE_BREATH            702
#define SPELL_GAS_BREATH             703
#define SPELL_FROST_BREATH           704
#define SPELL_ACID_BREATH            705
#define SPELL_LIGHTNING_BREATH       706
#define TYPE_LAVA_BREATH             707
#define SPELL_EARTH_ELEMENTAL	     711
#define SPELL_FIRE_ELEMENTAL	     712
#define SPELL_WATER_ELEMENTAL        713
#define SPELL_AIR_ELEMENTAL	     714
#define SPELL_HELL_FIRE		     717
#define JAVELIN_OF_LIGHTNING         718
#define SPELL_TROG_STENCH            719
#define SPELL_MANTICORE_SPIKES       720
#define TYPE_CATAPULT                730
#define TYPE_BALISTA                 731
#define TYPE_BOILING_OIL             732
#define TYPE_HOTSANDS                733
#define SPELL_MANA_RESTORE           734
#define SPELL_BLADE_BARRIER          735
#define SPELL_SUMMON_MINOTAUR        736
#define TYPE_STYGIAN_LIGHTNING       737
#define SPELL_SKUNK_STENCH           738
#define SPELL_PETRIFY                739
#define SPELL_SICKNESS               740
#define SPELL_ESSENCE_OF_EVIL        741
#define SPELL_ESSENCE_OF_GOOD        742
#define SPELL_SHRINKING              743
#define SPELL_HELL_FROST             744
#define TYPE_ALIEN_BLOOD             745
#define TYPE_SURGERY                 746
#define SMOKE_EFFECTS                747
#define SPELL_HELL_FIRE_STORM        748
#define SPELL_HELL_FROST_STORM       749
#define SPELL_SHADOW_BREATH          750
#define SPELL_STEAM_BREATH           751
#define TYPE_TRAMPLING               752
#define TYPE_GORE_HORNS              753
#define TYPE_TAIL_LASH               754
#define TYPE_BOILING_PITCH           755
#define TYPE_FALLING                 756
#define TYPE_HOLYOCEAN               757
#define TYPE_FREEZING                758
#define TYPE_DROWNING		     759
#define TYPE_ABLAZE                  760
#define SPELL_QUAD_DAMAGE            761
#define TYPE_VADER_CHOKE             762
#define TYPE_ACID_BURN               763
#define SPELL_ZHENGIS_FIST_OF_ANNIHILATION 764
#define TYPE_RAD_SICKNESS            765
#define TYPE_FLAMETHROWER            766
#define TOP_NPC_SPELL                766

#define TOP_SPELL_DEFINE	     799
/* NEW NPC/OBJECT SPELLS can be inserted here up to 799 */


/* WEAPON ATTACK TYPES */

#define TYPE_HIT                     800
#define TYPE_STING                   801
#define TYPE_WHIP                    802
#define TYPE_SLASH                   803
#define TYPE_BITE                    804
#define TYPE_BLUDGEON                805
#define TYPE_CRUSH                   806
#define TYPE_POUND                   807
#define TYPE_CLAW                    808
#define TYPE_MAUL                    809
#define TYPE_THRASH                  810
#define TYPE_PIERCE                  811
#define TYPE_BLAST		     812
#define TYPE_PUNCH		     813
#define TYPE_STAB		     814
#define TYPE_ENERGY_GUN		     815
#define TYPE_RIP                     816
#define TYPE_CHOP                    817
#define TYPE_SHOOT                   818
#define TOP_ATTACKTYPE               819
/* new attack types can be added here - up to TYPE_SUFFERING */
#define TYPE_BLEED					 897 // Open wound
#define TYPE_OVERLOAD				 898 // cyborg overloading systems.
#define TYPE_SUFFERING		     899


#define SAVING_PARA    0
#define SAVING_ROD     1
#define SAVING_PETRI   2
#define SAVING_BREATH  3
#define SAVING_SPELL   4
#define SAVING_CHEM    5
#define SAVING_PSI     6
#define SAVING_PHY     7
#define SAVING_NONE    50

#define TAR_IGNORE        1
#define TAR_CHAR_ROOM     2
#define TAR_CHAR_WORLD    4
#define TAR_FIGHT_SELF    8
#define TAR_FIGHT_VICT   16
#define TAR_SELF_ONLY    32 /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_NOT_SELF     64 /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_OBJ_INV     128
#define TAR_OBJ_ROOM    256
#define TAR_OBJ_WORLD   512
#define TAR_OBJ_EQUIP  1024
#define TAR_DOOR       2048

struct spell_info_type {
  char min_position;	/* Position for caster	 */
  sh_int mana_min;	/* Min amount of mana used by a spell (highest lev) */
  sh_int mana_max;	/* Max amount of mana used by a spell (lowest lev) */
  char mana_change;	/* Change in mana used by spell from lev to lev */
  
  char min_level[NUM_CLASSES];
  char gen[NUM_CLASSES];
  int routines;
  char violent;
  sh_int targets;         /* See below for use with TAR_XXX  */
};

/* Possible Targets:

   bit 0 : IGNORE TARGET
   bit 1 : PC/NPC in room
   bit 2 : PC/NPC in world
   bit 3 : Object held
   bit 4 : Object in inventory
   bit 5 : Object in room
   bit 6 : Object in world
   bit 7 : If fighting, and no argument, select tar_char as self
   bit 8 : If fighting, and no argument, select tar_char as victim (fighting)
   bit 9 : If no argument, select self, if argument check that it IS self.

*/

#define SPELL_TYPE_SPELL   0
#define SPELL_TYPE_POTION  1
#define SPELL_TYPE_WAND    2
#define SPELL_TYPE_STAFF   3
#define SPELL_TYPE_SCROLL  4


/* Attacktypes with grammar */

struct attack_hit_type {
   char	*singular;
   char	*plural;
};


#define ASPELL(spellname) \
void	spellname(byte level, struct char_data *ch, \
		  struct char_data *victim, struct obj_data *obj)

#define MANUAL_SPELL(spellname)	spellname(level, caster, cvict, ovict);

ASPELL(spell_astral_spell);
ASPELL(spell_create_water);
ASPELL(spell_recall);
ASPELL(spell_teleport);
ASPELL(spell_summon);
ASPELL(spell_locate_object);
ASPELL(spell_charm);
ASPELL(spell_charm_animal);
ASPELL(spell_information);
ASPELL(spell_identify);
ASPELL(spell_minor_identify);
ASPELL(spell_enchant_weapon);
ASPELL(spell_enchant_armor);
ASPELL(spell_greater_enchant);
ASPELL(spell_clairvoyance);
ASPELL(spell_coordinates);
ASPELL(spell_detect_poison);
ASPELL(spell_magical_vestment);
ASPELL(spell_conjure_elemental);
ASPELL(spell_knock);
ASPELL(spell_sword);
ASPELL(spell_animate_dead);
ASPELL(spell_control_weather);
ASPELL(spell_gust_of_wind);
ASPELL(spell_retrieve_corpse);
ASPELL(spell_local_teleport);
ASPELL(spell_peer);
ASPELL(spell_vestigial_rune);
ASPELL(spell_id_insinuation);
ASPELL(spell_shadow_breath);
ASPELL(spell_summon_legion);
ASPELL(spell_unholy_stalker);
ASPELL(spell_control_undead);
ASPELL(spell_inferno);
ASPELL(spell_banishment);

/* basic magic calling functions */

int find_skill_num(char *name);

int mag_damage(int level, struct char_data *ch, struct char_data *victim,
	       int spellnum, int savetype);

int mag_exits(int level, struct char_data *caster, struct room_data *room, 
	      int spellnum);

void mag_affects(int level, struct char_data *ch, struct char_data *victim,
		 int spellnum, int savetype);

void mag_group_switch(int level, struct char_data *ch, struct char_data *tch, 
		      int spellnum, int savetype);

void mag_groups(int level, struct char_data *ch, int spellnum, int savetype);

void mag_masses(byte level, struct char_data *ch, int spellnum, int savetype);

void mag_areas(byte level, struct char_data *ch, int spellnum, int savetype);

void mag_summons(int level, struct char_data *ch, struct obj_data *obj,
		 int spellnum, int savetype);

void mag_points(int level, struct char_data *ch, struct char_data *victim,
		int spellnum, int savetype);

void mag_unaffects(int level, struct char_data *ch, struct char_data *victim,
		   int spellnum, int type);

void mag_alter_objs(int level, struct char_data *ch, struct obj_data *obj,
		    int spellnum, int type);

void mag_creations(int level, struct char_data *ch, int spellnum);

int call_magic(struct char_data *caster, struct char_data *cvict,
	       struct obj_data *ovict, int spellnum, int level, int casttype);

int mag_objectmagic(struct char_data *ch, struct obj_data *obj,
		    char *argument);

void mag_objects(int level, struct char_data *ch, struct obj_data *obj,
		 int spellnum);

int cast_spell(struct char_data *ch, struct char_data *tch,
	       struct obj_data *tobj, int spellnum);

int trig_psych(struct char_data *ch, struct char_data *tch,
	       struct obj_data *tobj, int spellnum);

int alter_reality(struct char_data *ch, struct char_data *tch,
		  struct obj_data *tobj, int spellnum);

int mag_savingthrow(struct char_data *ch, int level, int type);

int mag_manacost(struct char_data * ch, int spellnum);


#endif

