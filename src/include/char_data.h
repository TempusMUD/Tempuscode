//
// File: char_data.h                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#ifndef __char_data_h__
#define __char_data_h__


/* char and mob-related defines *****************************************/


/* PC char_classes */
#define CLASS_UNDEFINED	      -1
#define CLASS_MAGIC_USER       0
#define CLASS_MAGE             CLASS_MAGIC_USER
#define CLASS_CLERIC           1
#define CLASS_THIEF            2
#define CLASS_WARRIOR          3
#define CLASS_BARB	       4
#define CLASS_PSIONIC          5    /* F */
#define CLASS_PHYSIC           6    /* F */
#define CLASS_CYBORG           7    /* F */
#define CLASS_KNIGHT           8
#define CLASS_RANGER           9
#define CLASS_HOOD            10    /* F */
#define CLASS_MONK            11   
#define CLASS_VAMPIRE         12
#define CLASS_MERCENARY       13
#define CLASS_SPARE1          14
#define CLASS_SPARE2          15
#define CLASS_SPARE3          16

#define NUM_CLASSES     17   /* This must be the number of char_classes!! */
#define CLASS_NORMAL    50
#define CLASS_BIRD      51
#define CLASS_PREDATOR  52
#define CLASS_SNAKE     53
#define CLASS_HORSE     54
#define CLASS_SMALL     55
#define CLASS_MEDIUM    56
#define CLASS_LARGE     57
#define CLASS_SCIENTIST 58
#define CLASS_SKELETON  60
#define CLASS_GHOUL	61
#define CLASS_SHADOW    62
#define CLASS_WIGHT     63
#define CLASS_WRAITH    64
#define CLASS_MUMMY     65
#define CLASS_SPECTRE   66
#define CLASS_NPC_VAMPIRE   67
#define CLASS_GHOST     68
#define CLASS_LICH      69
#define CLASS_ZOMBIE    70

#define CLASS_EARTH     81		/* Elementals */
#define CLASS_FIRE      82
#define CLASS_WATER     83
#define CLASS_AIR	84
#define CLASS_LIGHTNING 85
#define CLASS_GREEN	91	  	/* Dragons */
#define CLASS_WHITE     92
#define CLASS_BLACK     93
#define CLASS_BLUE      94
#define CLASS_RED       95
#define CLASS_SILVER    96
#define CLASS_SHADOW_D  97
#define CLASS_DEEP      98
#define CLASS_TURTLE    99
#define CLASS_LEAST    101		/* Devils  */
#define CLASS_LESSER   102		
#define CLASS_GREATER  103
#define CLASS_DUKE     104
#define CLASS_ARCH     105
#define CLASS_HILL     111
#define CLASS_STONE    112
#define CLASS_FROST    113
#define CLASS_FIRE_G   114
#define CLASS_CLOUD    115
#define CLASS_STORM    116
#define CLASS_SLAAD_RED    120		/* Slaad */
#define CLASS_SLAAD_BLUE   121
#define CLASS_SLAAD_GREEN  122
#define CLASS_SLAAD_GREY   123
#define CLASS_SLAAD_DEATH  124
#define CLASS_SLAAD_LORD   125
#define CLASS_DEMON_I	 	130	/* Demons of the Abyss */
#define CLASS_DEMON_II    	131
#define CLASS_DEMON_III    	132
#define CLASS_DEMON_IV    	133
#define CLASS_DEMON_V     	134
#define CLASS_DEMON_VI     	135
#define CLASS_DEMON_SEMI     	136
#define CLASS_DEMON_MINOR    	137
#define CLASS_DEMON_MAJOR    	138
#define CLASS_DEMON_LORD	139
#define CLASS_DEMON_PRINCE	140
#define CLASS_DEVA_ASTRAL       150
#define CLASS_DEVA_MONADIC      151
#define CLASS_DEVA_MOVANIC      152
#define CLASS_MEPHIT_FIRE       160
#define CLASS_MEPHIT_LAVA       161
#define CLASS_MEPHIT_SMOKE      162
#define CLASS_MEPHIT_STEAM      163
#define CLASS_DAEMON_ARCANA     170   // daemons
#define CLASS_DAEMON_CHARONA    171
#define CLASS_DAEMON_DERGHO     172
#define CLASS_DAEMON_HYDRO      173
#define CLASS_DAEMON_PISCO      174
#define CLASS_DAEMON_ULTRO      175
#define CLASS_DAEMON_YAGNO      176
#define CLASS_DAEMON_PYRO       177

#define TOP_CLASS               177

#define RACE_UNDEFINED  -1
#define RACE_HUMAN    	 0
#define RACE_ELF   	 1
#define RACE_DWARF       2
#define RACE_HALF_ORC    3
#define RACE_KLINGON     4
#define RACE_HAFLING     5
#define RACE_TABAXI      6
#define RACE_DROW        7
#define RACE_MOBILE      10
#define RACE_UNDEAD      11
#define RACE_HUMANOID    12
#define RACE_ANIMAL      13
#define RACE_DRAGON      14
#define RACE_GIANT       15
#define RACE_ORC	 16
#define RACE_GOBLIN	 17
#define RACE_HALFLING    18
#define RACE_MINOTAUR    19
#define RACE_TROLL       20
#define RACE_GOLEM       21
#define RACE_ELEMENTAL   22
#define RACE_OGRE        23
#define RACE_DEVIL       24
#define RACE_TROGLODYTE  25
#define RACE_MANTICORE   26
#define RACE_BUGBEAR     27
#define RACE_DRACONIAN   28
#define RACE_DUERGAR     29
#define RACE_SLAAD       30
#define RACE_ROBOT       31
#define RACE_DEMON       32
#define RACE_DEVA        33
#define RACE_PLANT       34
#define RACE_ARCHON      35
#define RACE_PUDDING     36
#define RACE_ALIEN_1     37
#define RACE_PRED_ALIEN  38
#define RACE_SLIME       39
#define RACE_ILLITHID    40
#define RACE_FISH        41
#define RACE_BEHOLDER    42
#define RACE_GASEOUS     43
#define RACE_GITHYANKI   44
#define RACE_INSECT      45
#define RACE_DAEMON      46
#define RACE_MEPHIT      47
#define RACE_KOBOLD      48
#define RACE_UMBER_HULK  49
#define RACE_WEMIC       50
#define NUM_RACES        51

/* Hometown defines                            */
#define HOME_UNDEFINED   -1
#define HOME_MODRIAN      0        
#define HOME_NEW_THALOS   1
#define HOME_ELECTRO      2
#define HOME_ELVEN_VILLAGE 3
#define HOME_ISTAN        4
#define HOME_MONK         5
#define HOME_NEWBIE_TOWER 6
#define HOME_SOLACE_COVE  7
#define HOME_MAVERNAL     8
#define HOME_ZUL_DANE     9
#define HOME_ARENA        10
#define HOME_CITY         11
#define HOME_DOOM         12
#define HOME_SKULLPORT       15
#define HOME_DWARVEN_CAVERNS 16
#define HOME_HUMAN_SQUARE    17
#define HOME_DROW_ISLE       18
#define HOME_ASTRAL_MANSE    19
#define NUM_HOMETOWNS        20

/* Deity Defines				*/
#define DEITY_NONE        0
#define DEITY_GUIHARIA    1
#define DEITY_PAN	  2
#define DEITY_JUSTICE     3
#define DEITY_ARES	  4
#define DEITY_KALAR       5


/* Sex */
#define SEX_NEUTRAL   0
#define SEX_MALE      1
#define SEX_FEMALE    2


/* Positions */
#define POS_DEAD       0	/* dead			*/
#define POS_MORTALLYW  1	/* mortally wounded	*/
#define POS_INCAP      2	/* incapacitated	*/
#define POS_STUNNED    3	/* stunned		*/
#define POS_SLEEPING   4	/* sleeping		*/
#define POS_RESTING    5	/* resting		*/
#define POS_SITTING    6	/* sitting		*/
#define POS_FIGHTING   7	/* fighting		*/
#define POS_STANDING   8	/* standing		*/
#define POS_FLYING     9        /* flying around        */
#define POS_MOUNTED    10
#define POS_SWIMMING   11


/* Player flags: used by char_data.char_specials.act */
#define PLR_KILLER	(1 << 0)   /* Player is a player-killer		*/
#define PLR_THIEF	(1 << 1)   /* Player is a player-thief		*/
#define PLR_FROZEN	(1 << 2)   /* Player is frozen			*/
#define PLR_DONTSET     (1 << 3)   /* Don't EVER set (ISNPC bit)	*/
#define PLR_WRITING	(1 << 4)   /* Player writing (board/mail/olc)	*/
#define PLR_MAILING	(1 << 5)   /* Player is writing mail		*/
#define PLR_CRASH	(1 << 6)   /* Player needs to be crash-saved	*/
#define PLR_SITEOK	(1 << 7)   /* Player has been site-cleared	*/
#define PLR_NOSHOUT	(1 << 8)   /* Player not allowed to shout/goss	*/
#define PLR_NOTITLE	(1 << 9)   /* Player not allowed to set title	*/
#define PLR_DELETED	(1 << 10)  /* Player deleted - space reusable	*/
#define PLR_LOADROOM	(1 << 11)  /* Player uses nonstandard loadroom	*/
#define PLR_NOWIZLIST	(1 << 12)  /* Player shouldn't be on wizlist	*/
#define PLR_NODELETE	(1 << 13)  /* Player shouldn't be deleted	*/
#define PLR_INVSTART	(1 << 14)  /* Player should enter game wizinvis	*/
#define PLR_CRYO	(1 << 15)  /* Player is cryo-saved (purge prog)	*/
#define PLR_AFK         (1 << 16)  /* Player is away from keyboard      */
#define PLR_CLAN_LEADER (1 << 17)  /* The head of the respective clan   */
#define PLR_TOUGHGUY    (1 << 18)  /* Player is open to pk and psteal   */
#define PLR_OLC         (1 << 19)  /* Player is descripting olc         */
#define PLR_HALT        (1 << 20)  /* Player is halted                  */
#define PLR_OLCGOD      (1 << 21)  /* Player can edit at will           */
#define PLR_TESTER      (1 << 22)  /* Player is a tester                */
#define PLR_QUESTOR     (1 << 23)  /* Quest god                         */
#define PLR_MORTALIZED  (1 << 24)  /* God can be killed                 */
#define PLR_REMORT_TOUGHGUY (1 << 25) /* open to pk by remortz          */
#define PLR_COUNCIL     (1 << 26)
#define PLR_NOPOST      (1 << 27)
#define PLR_LOG         (1 << 28)  /* log all cmds */
#define PLR_POLC        (1 << 29)  /* player approved for port olc      */
#define PLR_NOPK        (1 << 30)  /* player cannot pk */

/* Mobile flags: used by char_data.char_specials.act */
#define MOB_SPEC         (1 << 0)  /* Mob has a callable spec-proc	*/
#define MOB_SENTINEL     (1 << 1)  /* Mob should not move		*/
#define MOB_SCAVENGER    (1 << 2)  /* Mob picks up stuff on the ground	*/
#define MOB_ISNPC        (1 << 3)  /* (R) Automatically set on all Mobs	*/
#define MOB_AWARE	 (1 << 4)  /* Mob can't be backstabbed		*/
#define MOB_AGGRESSIVE   (1 << 5)  /* Mob hits players in the room	*/
#define MOB_STAY_ZONE    (1 << 6)  /* Mob shouldn't wander out of zone	*/
#define MOB_WIMPY        (1 << 7)  /* Mob flees if severely injured	*/
#define MOB_AGGR_EVIL	 (1 << 8)  /* auto attack evil PC's		*/
#define MOB_AGGR_GOOD	 (1 << 9)  /* auto attack good PC's		*/
#define MOB_AGGR_NEUTRAL (1 << 10) /* auto attack neutral PC's		*/
#define MOB_MEMORY	 (1 << 11) /* remember attackers if attacked	*/
#define MOB_HELPER	 (1 << 12) /* attack PCs fighting other NPCs	*/
#define MOB_NOCHARM	 (1 << 13) /* Mob can't be charmed		*/
#define MOB_NOSUMMON	 (1 << 14) /* Mob can't be summoned		*/
#define MOB_NOSLEEP	 (1 << 15) // Mob can't be slept
#define MOB_NOBASH	 (1 << 16) // Mob can't be bashed (e.g. trees)
#define MOB_NOBLIND	 (1 << 17) // Mob can't be blinded
#define MOB_NOTURN       (1 << 18) // Hard to turn
#define MOB_NOPETRI      (1 << 19) // Cannot be petrified
#define MOB_NONE		(1 << 20)	// A Useless, No-do-nothing flag.
#define MOB_PET			(1 << 21)	// Mob is a conjured pet and shouldn't
									// get nor give any xp in any way.
#define NUM_MOB_FLAGS    22

#define MOB2_SCRIPT     (1 << 0)
#define MOB2_MOUNT      (1 << 1)
#define MOB2_STAY_SECT  (1 << 2)  /* Can't leave SECT_type.   */
#define MOB2_ATK_MOBS   (1 << 3)  /* Aggro Mobs will attack other mobs */
#define MOB2_HUNT	(1 << 4)  /* Mob will hunt attacker    */
#define MOB2_LOOTER	(1 << 5)  /* Loots corpses     */
#define MOB2_NOSTUN     (1 << 6)
#define MOB2_SELLER     (1 << 7)  /* If shopkeeper, sells anywhere. */
#define MOB2_WONT_WEAR	(1 << 8)  /* Wont wear shit it picks up (SHPKPER) */
#define MOB2_SILENT_HUNTER (1 << 9)
#define MOB2_FAMILIAR   (1 << 10)  /* mages familiar */
#define MOB2_NO_FLOW    (1 << 11) /* Mob doesnt flow */
#define MOB2_UNAPPROVED (1 << 12) /* Mobile not approved for game play */
#define MOB2_RENAMED    (1 << 13) /* Mobile renamed */
#define MOB2_NOAGGRO_RACE (1 << 14) /* wont attack members of own race */
#define MOB2_MUGGER     (1 << 15)
#define NUM_MOB2_FLAGS  15

/* Preference flags: used by char_data.player_specials.pref */
#define PRF_BRIEF       (1 << 0)  /* Room descs won't normally be shown	*/
#define PRF_COMPACT     (1 << 1)  /* No extra CRLF pair before prompts	*/
#define PRF_DEAF	(1 << 2)  /* Can't hear shouts			*/
#define PRF_NOTELL	(1 << 3)  /* Can't receive tells		*/
#define PRF_DISPHP	(1 << 4)  /* Display hit points in prompt	*/
#define PRF_DISPMANA	(1 << 5)  /* Display mana points in prompt	*/
#define PRF_DISPMOVE	(1 << 6)  /* Display move points in prompt	*/
#define PRF_AUTOEXIT	(1 << 7)  /* Display exits in a room		*/
#define PRF_NOHASSLE	(1 << 8)  /* Aggr mobs won't attack		*/
#define PRF_QUEST	(1 << 9)  /* On quest				*/
#define PRF_SUMMONABLE	(1 << 10) /* Can be summoned			*/
#define PRF_NOREPEAT	(1 << 11) /* No repetition of comm commands	*/
#define PRF_HOLYLIGHT	(1 << 12) /* Can see in dark			*/
#define PRF_COLOR_1	(1 << 13) /* Color (low bit)			*/
#define PRF_COLOR_2	(1 << 14) /* Color (high bit)			*/
#define PRF_NOWIZ	(1 << 15) /* Can't hear wizline			*/
#define PRF_LOG1	(1 << 16) /* On-line System Log (low bit)	*/
#define PRF_LOG2	(1 << 17) /* On-line System Log (high bit)	*/
#define PRF_NOAUCT	(1 << 18) /* Can't hear auction channel		*/
#define PRF_NOGOSS	(1 << 19) /* Can't hear gossip channel		*/
#define PRF_NOGRATZ	(1 << 20) /* Can't hear grats channel		*/
#define PRF_ROOMFLAGS	(1 << 21) /* Can see room flags (ROOM_x)	*/
#define PRF_NOSNOOP     (1 << 22) /* Can not be snooped by immortals    */
#define PRF_NOMUSIC     (1 << 23) /* Can't hear music channel	        */
#define PRF_NOSPEW	(1 << 24) /* Can't hear spews			*/
#define PRF_GAGMISS     (1 << 25) /* Doesn't see misses during fight    */
#define PRF_NOPROJECT   (1 << 26) /* Cannot hear the remort channel     */
#define PRF_NOINTWIZ    (1 << 27)
#define PRF_NOCLANSAY   (1 << 28) /* Doesnt hear clan says and such     */
#define PRF_NOIDENTIFY  (1 << 29) /* Saving throw is made when id'd     */
#define PRF_NODREAM     (1 << 30)

#define PRF2_FIGHT_DEBUG   (1 << 0) /* Sees info on fight.              */
#define PRF2_NEWBIE_HELPER (1 << 1) /* sees newbie arrivals             */
#define PRF2_AUTO_DIAGNOSE (1 << 2) /* automatically see condition of enemy */
#define PRF2_AUTOPAGE      (1 << 3) /* Beeps when ch receives a tell    */
#define PRF2_NOAFFECTS     (1 << 4) /* Affects are not shown in score   */
#define PRF2_NOHOLLER      (1 << 5) /* Gods only                        */
#define PRF2_NOIMMCHAT     (1 << 6) /* Gods only                        */
#define PRF2_CLAN_TITLE    (1 << 7) /* auto-sets title to clan stuff */
#define PRF2_CLAN_HIDE     (1 << 8) /* don't show badge in who list */
#define PRF2_LIGHT_READ    (1 << 9) /* interrupts while d->showstr_point */
#define PRF2_AUTOPROMPT    (1 << 10) /* always draw new prompt */
#define PRF2_NOWHO         (1 << 11) /* don't show in who */
#define PRF2_ANONYMOUS     (1 << 12) /* dont show char_class, level */
#define PRF2_NOTRAILERS    (1 << 13) /* don't show trailer affects */
#define PRF2_VT100         (1 << 14) /* Players uses VT100 inferface */
#define PRF2_AUTOSPLIT     (1 << 15)
#define PRF2_AUTOLOOT      (1 << 16)
#define PRF2_PKILLER       (1 << 17) // player can attack other players

/* Affect bits: used in char_data.char_specials.saved.affected_by */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define AFF_BLIND             (1 << 0)	   /* (R) Char is blind		*/
#define AFF_INVISIBLE         (1 << 1)	   /* Char is invisible		*/
#define AFF_DETECT_ALIGN      (1 << 2)	   /* Char is sensitive to align*/
#define AFF_DETECT_INVIS      (1 << 3)	   /* Char can see invis chars  */
#define AFF_DETECT_MAGIC      (1 << 4)	   /* Char is sensitive to magic*/
#define AFF_SENSE_LIFE        (1 << 5)	   /* Char can sense hidden life*/
#define AFF_WATERWALK	      (1 << 6)	   /* Char can walk on water	*/
#define AFF_SANCTUARY         (1 << 7)	   /* Char protected by sanct.	*/
#define AFF_GROUP             (1 << 8)	   /* (R) Char is grouped	*/
#define AFF_CURSE             (1 << 9)	   /* Char is cursed		*/
#define AFF_INFRAVISION       (1 << 10)	   /* Char can see in dark	*/
#define AFF_POISON            (1 << 11)	   /* (R) Char is poisoned	*/
#define AFF_PROTECT_EVIL      (1 << 12)	   /* Char protected from evil  */
#define AFF_PROTECT_GOOD      (1 << 13)	   /* Char protected from good  */
#define AFF_SLEEP             (1 << 14)	   /* (R) Char magically asleep	*/
#define AFF_NOTRACK	      (1 << 15)	   /* Char can't be tracked	*/
#define AFF_INFLIGHT	      (1 << 16)	   /* Room for future expansion	*/
#define AFF_TIME_WARP         (1 << 17)	   /* Room for future expansion	*/
#define AFF_SNEAK             (1 << 18)	   /* Char can move quietly	*/
#define AFF_HIDE              (1 << 19)	   /* Char is hidden		*/
#define AFF_WATERBREATH       (1 << 20)	   /* Room for future expansion	*/
#define AFF_CHARM             (1 << 21)	   /* Char is charmed		*/
#define AFF_CONFUSION         (1 << 22)    /* Char is confused     	*/
#define AFF_NOPAIN            (1 << 23)    /* Char feels no pain	*/
#define AFF_RETINA            (1 << 24)    /* Char's retina is stimulated*/
#define AFF_ADRENALINE        (1 << 25)    /* Char's adrenaline is pumping*/
#define AFF_CONFIDENCE        (1 << 26)    /* Char is confident       	*/
#define AFF_REJUV             (1 << 27)    /* Char is rejuvenating */
#define AFF_REGEN             (1 << 28)    /* Body is regenerating */
#define AFF_GLOWLIGHT         (1 << 29)    /* Light spell is operating   */
#define AFF_BLUR              (1 << 30)    /* Blurry image */
#define NUM_AFF_FLAGS         31

#define AFF2_FLUORESCENT	(1 << 0)
#define AFF2_TRANSPARENT	(1 << 1)
#define AFF2_SLOW               (1 << 2)
#define AFF2_HASTE		(1 << 3)
#define AFF2_MOUNTED		(1 << 4)   /*DO NOT SET THIS IN MOB FILE*/
#define AFF2_FIRE_SHIELD        (1 << 5)    /* affected by Fire Shield  */
#define AFF2_BESERK		(1 << 6)
#define AFF2_INTIMIDATED	(1 << 7)
#define AFF2_TRUE_SEEING        (1 << 8)
#define AFF2_DIVINE_ILLUMINATION		(1 << 9)
#define AFF2_PROTECT_UNDEAD	(1 << 10)
#define AFF2_INVIS_TO_UNDEAD	(1 << 11)
#define AFF2_INVIS_TO_ANIMALS   (1 << 12)
#define AFF2_ENDURE_COLD        (1 << 13)
#define AFF2_PARALYZED          (1 << 14)
#define AFF2_PROT_LIGHTNING     (1 << 15)
#define AFF2_PROT_FIRE          (1 << 16)
#define AFF2_TELEKINESIS        (1 << 17)  /* Char can carry more stuff */
#define AFF2_PROT_RAD		(1 << 18)  /* Enables Autoexits ! :)    */
#define AFF2_ABLAZE             (1 << 19)
#define AFF2_NECK_PROTECTED     (1 << 20)  /* Can't be beheaded         */
#define AFF2_DISPLACEMENT       (1 << 21)
#define AFF2_PROT_DEVILS        (1 << 22)
#define AFF2_MEDITATE           (1 << 23)
#define AFF2_EVADE              (1 << 24)
#define AFF2_BLADE_BARRIER      (1 << 25)
#define AFF2_OBLIVITY           (1 << 26)
#define AFF2_ENERGY_FIELD       (1 << 27)
#define AFF2_PETRIFIED          (1 << 28)
#define AFF2_VERTIGO            (1 << 29)
#define AFF2_PROT_DEMONS        (1 << 30)
#define NUM_AFF2_FLAGS          31

#define AFF3_ATTRACTION_FIELD   (1 << 0)
#define AFF3_FEEDING            (1 << 1)
#define AFF3_POISON_2           (1 << 2)
#define AFF3_POISON_3           (1 << 3)
#define AFF3_SICKNESS           (1 << 4)
#define AFF3_SELF_DESTRUCT      (1 << 5) /* Self-destruct sequence init */
#define AFF3_DAMAGE_CONTROL     (1 << 6) /* Damage control for cyborgs  */
#define AFF3_STASIS             (1 << 7) /* Borg is in static state     */
#define AFF3_PRISMATIC_SPHERE   (1 << 8) /* Defensive */
#define AFF3_RADIOACTIVE        (1 << 9)
#define AFF3_UNUSED_RAD_SICKNESS       (1 << 10) // unused
#define AFF3_MANA_TAP           (1 << 11)
#define AFF3_ENERGY_TAP         (1 << 12)
#define AFF3_SONIC_IMAGERY      (1 << 13)
#define AFF3_SHROUD_OBSCUREMENT (1 << 14)
#define AFF3_NOBREATHE          (1 << 15)
#define AFF3_PROT_HEAT          (1 << 16)
#define AFF3_PSISHIELD          (1 << 17)
#define AFF3_PSYCHIC_CRUSH      (1 << 18)
#define AFF3_DOUBLE_DAMAGE      (1 << 19)
#define AFF3_ACIDITY            (1 << 20)
#define AFF3_HAMSTRUNG		    (1 << 21)    /* Bleeding badly from the leg */
#define NUM_AFF3_FLAGS          22

#define ARRAY_AFF_1        1
#define ARRAY_AFF_2        2
#define ARRAY_AFF_3	   3

/* Character equipment positions: used as index for char_data.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
   which control the valid places you can wear a piece of equipment */
#define WEAR_LIGHT      0
#define WEAR_FINGER_R   1
#define WEAR_FINGER_L   2
#define WEAR_NECK_1     3
#define WEAR_NECK_2     4
#define WEAR_BODY       5
#define WEAR_HEAD       6
#define WEAR_LEGS       7
#define WEAR_FEET       8
#define WEAR_HANDS      9
#define WEAR_ARMS      10
#define WEAR_SHIELD    11
#define WEAR_ABOUT     12
#define WEAR_WAIST     13
#define WEAR_WRIST_R   14
#define WEAR_WRIST_L   15
#define WEAR_WIELD     16
#define WEAR_HOLD      17
#define WEAR_CROTCH    18
#define WEAR_EYES      19
#define WEAR_BACK      20
#define WEAR_BELT      21
#define WEAR_FACE      22
#define WEAR_EAR_L     23
#define WEAR_EAR_R     24
#define WEAR_WIELD_2   25
#define WEAR_ASS       26
#define NUM_WEARS      27	/* This must be the # of eq positions!! */
#define WEAR_RANDOM    28


/* Modifier constants used with obj affects ('A' fields) */
#define APPLY_NONE              0	/* No effect			*/
#define APPLY_STR               1	/* Apply to strength		*/
#define APPLY_DEX               2	/* Apply to dexterity		*/
#define APPLY_INT               3	/* Apply to intellegence	*/
#define APPLY_WIS               4	/* Apply to wisdom		*/
#define APPLY_CON               5	/* Apply to constitution	*/
#define APPLY_CHA		6	/* Apply to charisma		*/
#define APPLY_CLASS             7	/* Reserved			*/
#define APPLY_LEVEL             8	/* Reserved			*/
#define APPLY_AGE               9	/* Apply to age			*/
#define APPLY_CHAR_WEIGHT      10	/* Apply to weight		*/
#define APPLY_CHAR_HEIGHT      11	/* Apply to height		*/
#define APPLY_MANA             12	/* Apply to max mana		*/
#define APPLY_HIT              13	/* Apply to max hit points	*/
#define APPLY_MOVE             14	/* Apply to max move points	*/
#define APPLY_GOLD             15	/* Reserved			*/
#define APPLY_EXP              16	/* Reserved			*/
#define APPLY_AC               17	/* Apply to Armor Class		*/
#define APPLY_HITROLL          18	/* Apply to hitroll		*/
#define APPLY_DAMROLL          19	/* Apply to damage roll		*/
#define APPLY_SAVING_PARA      20	/* Apply to save throw: paralz	*/
#define APPLY_SAVING_ROD       21	/* Apply to save throw: rods	*/
#define APPLY_SAVING_PETRI     22	/* Apply to save throw: petrif	*/
#define APPLY_SAVING_BREATH    23	/* Apply to save throw: breath	*/
#define APPLY_SAVING_SPELL     24	/* Apply to save throw: spells	*/
#define APPLY_SNEAK            25
#define APPLY_HIDE             26
#define APPLY_RACE	       27
#define APPLY_SEX	       28
#define APPLY_BACKSTAB	       29
#define APPLY_PICK_LOCK        30
#define APPLY_PUNCH	       31
#define APPLY_SHOOT	       32
#define APPLY_KICK	       33
#define APPLY_TRACK            34
#define APPLY_IMPALE           35
#define APPLY_BEHEAD           36
#define APPLY_THROWING         37   
#define APPLY_RIDING           38
#define APPLY_TURN             39
#define APPLY_SAVING_CHEM      40
#define APPLY_SAVING_PSI       41
#define APPLY_ALIGN            42
#define APPLY_SAVING_PHY       43
#define APPLY_CASTER           44   // special usage
#define APPLY_WEAPONSPEED      45
#define APPLY_DISGUISE         46
#define APPLY_NOTHIRST         47
#define APPLY_NOHUNGER         48
#define APPLY_NODRUNK          49
#define APPLY_SPEED            50
#define NUM_APPLIES            51


/* other miscellaneous defines *******************************************/


/* Player conditions */
#define DRUNK        0
#define FULL         1
#define THIRST       2



/* other #defined constants **********************************************/

#define LVL_GRIMP      72
#define LVL_FISHBONE   71
#define LVL_LUCIFER    70
#define LVL_IMPL       69
#define LVL_ENTITY     68
#define LVL_ANCIENT    LVL_ENTITY
#define LVL_CREATOR    67
#define LVL_GRGOD      66
#define LVL_TIMEGOD    65
#define LVL_DEITY      64
#define LVL_GOD	       63	/* Lesser God */
#define LVL_ENERGY     62
#define LVL_FORCE      61
#define LVL_POWER      60
#define LVL_BEING      59
#define LVL_SPIRIT     58
#define LVL_ELEMENT    57
#define LVL_DEMI       56
#define LVL_ETERNAL    55
#define LVL_ETHEREAL   54
#define LVL_LUMINARY   53
#define LVL_BUILDER    52
#define LVL_IMMORT     51
#define LVL_AMBASSADOR 50

#define LVL_FREEZE	LVL_GOD
#define LVL_CAN_BAN     LVL_GOD
#define LVL_VIOLENCE    LVL_POWER
#define LVL_LOGALL      LVL_ENTITY
#define LVL_PROTECTED   5


/* char-related structures ************************************************/


/* memory structure for characters */
struct memory_rec_struct {
    long	id;
    struct memory_rec_struct *next;
};

typedef struct memory_rec_struct memory_rec;


/* These data contain information about a players time data */
struct time_data {
    time_t birth;    /* This represents the characters age                */
    time_t death;    // when did we die
    time_t logon;    /* Time of the last logon (used to calculate played) */
    int	played;     /* This is the total accumulated time played in secs */
};

#define MAX_WEAPON_SPEC 6
typedef struct weapon_spec {
    int vnum;
    ubyte level;
} weapon_spec;

/* general player-related info, usually PC's and NPC's */
struct char_player_data {
    char	passwd[MAX_PWD_LENGTH+1]; /* character's password      */
    char	*name;	       /* PC / NPC s name (kill ...  )         */
    char	*short_descr;  /* for NPC 'actions'                    */
    char	*long_descr;   /* for 'look'			       */
    char	*description;  /* Extra descriptions                   */
    char	*title;        /* PC / NPC's title                     */
    sh_int char_class;       /* PC / NPC's char_class		       */
    sh_int remort_char_class; /* PC / NPC REMORT CLASS (-1 for none) */
    sh_int weight;      /* PC / NPC's weight                    */
    inline short getWeight() { return weight; }
    inline short setWeight( const short new_weight ) { return ( weight = new_weight ); }
    short modifyWeight( const short mod_weight );

    sh_int height;      /* PC / NPC's height                    */
    sh_int hometown;    /* PC s Hometown (zone)                 */
    byte sex;           /* PC / NPC's sex                       */
    byte race;          /* PC / NPC's race   		       */
    byte level;         /* PC / NPC's level                     */
    byte age_adjust;    /* PC age adjust to maintain sanity     */
    struct time_data time;  /* PC's AGE in days                 */
};


/* Char's abilities.  Used in char_file_u *DO*NOT*CHANGE* */
struct char_ability_data {
    sbyte str;
    sbyte str_add;      /* 000 - 100 if strength 18             */
    sbyte intel;
    sbyte wis;
    sbyte dex;
    sbyte con;
    sbyte cha;
};


/* Char's points.  Used in char_file_u *DO*NOT*CHANGE* */
struct char_point_data {
    sh_int mana;
    sh_int max_mana;     /* Max move for PC/NPC			   */
    sh_int hit;
    sh_int max_hit;      /* Max hit for PC/NPC                      */
    sh_int move;
    sh_int max_move;     /* Max move for PC/NPC                     */

    sh_int armor;        /* Internal -100..100, external -10..10 AC */
    int	gold;           /* Money carried                           */
    int	bank_gold;	/* Gold the char has in a bank account	   */
    int  cash;         // cash on hand
    int  credits;	/* Internal net credits			*/
    int	exp;            /* The experience of the player            */

    sbyte hitroll;       /* Any bonus or penalty to the hit roll    */
    sbyte damroll;       /* Any bonus or penalty to the damage roll */
};


/* 
 * char_special_data_saved: specials which both a PC and an NPC have in
 * common, but which must be saved to the playerfile for PC's.
 *
 * WARNING:  Do not change this structure.  Doing so will ruin the
 * playerfile.  If you want to add to the playerfile, use the spares
 * in player_special_data.
 */
struct char_special_data_saved {
    int	alignment;		/* +-1000 for alignments                */
    long	idnum;			/* player's idnum; -1 for mobiles	*/
    long	act;			/* act flag for NPC's; player flag for PC's */
    long act2;

    long	affected_by;		/* Bitvector for spells/skills affected by */
    long affected2_by;
    long affected3_by;

    sh_int apply_saving_throw[10]; /* Saving throw (Bonuses)		*/
};


/* Special playing constants shared by PCs and NPCs which aren't in pfile */
struct char_special_data {

    int setCarriedWeight( int new_weight ) {
	return ( carry_weight = new_weight );
    }
    
    inline int setWornWeight( int new_weight ) {
	return ( worn_weight = new_weight );
    }
    
    inline int getCarriedWeight( void ) { return carry_weight; }
    inline int getWornWeight( void ) { return worn_weight; }

    struct char_data *fighting;	/* Opponent				*/
    struct char_data *hunting;	/* Char hunted by this char		*/
    struct char_data *mounted;	/* MOB mounted by this char		*/
  
    int  carry_weight;	      /* Carried weight		         	*/
    int  worn_weight;	      /* Total weight equipped		        */
    int  timer;		      /* Timer for update			*/
    int  meditate_timer;        /* How long has been meditating           */
    int  cur_flow_pulse;        /* Keeps track of whether char has flowed */

    byte breath_count;          /* for breathing and falling              */
    byte fall_count;
    byte position;	      /* Standing, fighting, sleeping, etc.	*/
    byte carry_items;	      /* Number of items carried		*/
    byte weapon_proficiency;    /* Scale of learnedness of weapon prof.   */
  
    struct char_special_data_saved saved; /* constants saved in plrfile	*/
};


/*
 *  If you want to add new values to the playerfile, do it here.  DO NOT
 * ADD, DELETE OR MOVE ANY OF THE VARIABLES - doing so will change the
 * size of the structure and ruin the playerfile.  However, you can change
 * the names of the spares to something more meaningful, and then use them
 * in your new code.  They will automatically be transferred from the
 * playerfile into memory when players log in.
 */
struct player_special_data_saved {
    byte skills[MAX_SKILLS+1];	/* array of skills plus skill 0		*/
    weapon_spec weap_spec[MAX_WEAPON_SPEC];
    int	wimp_level;		/* Below this # of hit points, flee!	*/
    byte freeze_level;		/* Level of god who froze char, if any	*/
    sh_int invis_level;		/* level of invisibility		*/
    room_num load_room;		/* Which room to place char in		*/
    long	pref;			/* preference flags for PC's.		*/
    long pref2;                  /* 2nd pref flag                        */
    ubyte bad_pws;		/* number of bad password attemps	*/
    sbyte conditions[3];         /* Drunk, full, thirsty			*/

    /* spares below for future expansion.  You can change the names from
       'sparen' to something meaningful, but don't change the order.  */

    ubyte clan;
    ubyte hold_home;
    ubyte remort_invis_level;
    ubyte broken_component;
    ubyte remort_generation;
    ubyte quest_points;
    ubyte qlog_level;           // what level of awareness we have to qlog
    ubyte speed;                // percentage of speedup
    ubyte qp_allowance;         // Quest point allowance 
    ubyte spare_c[1];
    int deity;
    int spells_to_learn;
    int life_points;
    int pkills;
    int mobkills;
    int deaths;
    int old_char_class;               /* Type of borg, or char_class before vamprism. */
    int page_length;
    int total_dam;
    int columns;
    int hold_load_room;
    int quest_id;
    int spare_i[3];
    long	mana_shield_low;
    long	mana_shield_pct;
    long	spare19;
    long	spare20;
    long	spare21;

};

/*
 * Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs and the portion
 * of it labelled 'saved' is saved in the playerfile.  This structure can
 * be changed freely; beware, though, that changing the contents of
 * player_special_data_saved will corrupt the playerfile.
 */
#define MAX_IMPRINT_ROOMS 6

struct player_special_data {
    
    struct player_special_data_saved saved;
    char	*poofin;		/* Description on arrival of a god.     */
    char	*poofout;		/* Description upon a god's exit.       */
    struct alias *aliases;	/* Character's aliases			*/
    long last_tell;		/* idnum of last tell from		*/
    int imprint_rooms[MAX_IMPRINT_ROOMS];
    unsigned int soilage[NUM_WEARS];     
    struct obj_data *olc_obj;     /* which obj being edited               */
    struct char_data *olc_mob;    /* which mob being edited               */
    struct shop_data *olc_shop;   /* which shop being edited              */
    struct olc_help_r *olc_help;  /* which help record being edited       */
    struct special_search_data *olc_srch;      /* which srch being edited */
    struct ticl_data *olc_ticl;   /* which ticl being edited              */
    struct room_data *was_in_room;/* location for linkdead people         */
};

struct mob_shared_data {
    int vnum;
    int number;
    int  attack_type;           /* The Attack Type integer for NPC's     */
    int lair;                /* room the mob always returns to */
    int leader;              // mob vnum the mob always helps and follows
    int kills;
    int loaded;
    byte default_pos;           /* Default position for NPC              */
    byte damnodice;             /* The number of damage dice's	       */
    byte damsizedice;           /* The size of the damage dice's         */
    byte morale;
    char *move_buf;             /* custom move buf */
    struct char_data *proto;    /* pointer to prototype */
    struct ticl_data *ticl_ptr; /* Pointer to TICL procedure */
    SPECIAL(*func);
};


struct mob_mugger_data {
    int idnum;   /* idnum of player on shit list */
    int  vnum;    /* vnum of object desired */
    byte timer;   /* how long has the mob been waiting */
};

/* Specials used by NPCs, not PCs */
struct mob_special_data {
    memory_rec *memory;	    /* List of attackers to remember	       */
    struct extra_descr_data *response;  /* for response processing */
    struct mob_mugger_data *mug;
    struct mob_shared_data *shared;
    int wait_state;	    /* Wait state for bashed mobs	       */
    byte last_direction;     /* The last direction the monster went     */
    unsigned int mob_idnum;  /* mobile's unique idnum */
};


/* An affect structure.  Used in char_file_u *DO*NOT*CHANGE* */
struct affected_type {
    sh_int type;          /* The type of spell that caused this      */
    sh_int duration;      /* For how long its effects will last      */
    sh_int modifier;       /* This is added to apropriate ability     */
    sh_int location;      /* Tells which ability to change(APPLY_XXX)*/
    ubyte level;
    ubyte spare_c;
    long	bitvector;       /* Tells which bits to set (AFF_XXX)       */
    int aff_index;
    struct affected_type *next;
};


/* Structure used for chars following other chars */
struct follow_type {
    struct char_data *follower;
    struct follow_type *next;
};


/* ================== Structure for player/non-player ===================== */
struct char_data {

    // carried weight
    inline int getCarriedWeight( void ) { return char_specials.getCarriedWeight(); }
    inline int setCarriedWeight( int new_weight ) {
	return char_specials.setCarriedWeight( new_weight );
    }
    int modifyCarriedWeight( int mod_weight );

    // worn weight
    inline int getWornWeight( void ) { return char_specials.getWornWeight(); }
    inline int setWornWeight( int new_weight ) { 
	return char_specials.setWornWeight( new_weight );
    }
    int modifyWornWeight( int mod_weight );

    // char weight
    inline short getWeight( void ) { return player.getWeight(); }
    inline short setWeight( short new_weight ) {
	return player.setWeight( new_weight );
    }
    inline short modifyWeight( short mod_weight ) { return player.modifyWeight( mod_weight ); }

    // breath count
    inline int getBreathCount( void ) { return char_specials.breath_count; }
    inline int getBreathThreshold( void ) { return ( getLevel() >> 5 ) + 2; }
    inline int setBreathCount( int new_count ) {
	return ( char_specials.breath_count = new_count );
    }
    inline int modifyBreathCount( int mod_count ) {
	return setBreathCount( getBreathCount() + mod_count );
    }
    
    inline int getLevel( void ) { return player.level; }

    int pfilepos;			 /* playerfile pos		  */
    struct room_data *in_room;            /* Location (real room number)	  */

    struct char_player_data player;       /* Normal data                   */
    struct char_ability_data real_abils;	 /* Abilities without modifiers   */
    struct char_ability_data aff_abils;	 /* Abils with spells/stones/etc  */
    struct char_point_data points;        /* Points                        */
    struct char_special_data char_specials;	/* PC/NPC specials	  */
    struct player_special_data *player_specials; /* PC specials		  */
    struct mob_special_data mob_specials;	/* NPC specials		  */
  
    struct affected_type *affected;       /* affected by what spells       */
    struct obj_data *equipment[NUM_WEARS];/* Equipment array               */
    struct obj_data *implants[NUM_WEARS];

    struct obj_data *carrying;            /* Head of list                  */
    struct descriptor_data *desc;         /* NULL for mobiles              */

    struct char_data *next_in_room;     /* For room->people - list         */
    struct char_data *next;             /* For either monster or ppl-list  */
    struct char_data *next_fighting;    /* For fighting list               */
  
    struct follow_type *followers;        /* List of chars followers       */
    struct char_data *master;             /* Who is char following?        */
};
typedef struct char_data CHAR;

/* ====================================================================== */


/* ==================== File Structure for Player ======================= */
/*             BEWARE: Changing it will ruin the playerfile		  */
struct char_file_u {
    /* char_player_data */
    char	name[MAX_NAME_LENGTH+1];
    char	description[MAX_CHAR_DESC+1];
    char	title[MAX_TITLE_LENGTH+1];
    char poofin[MAX_POOF_LENGTH];
    char poofout[MAX_POOF_LENGTH];
    sh_int char_class;
    sh_int remort_char_class;
    sh_int weight;
    sh_int height;
    sh_int hometown;
    byte sex;
    byte race;
    byte level;
    time_t birth;   /* Time of birth of character     */
    time_t death;   // time of death of the character (undead)
    int	played;    /* Number of secs played in total */

    char	pwd[MAX_PWD_LENGTH+1];    /* character's password */

    struct char_special_data_saved char_specials_saved;
    struct player_special_data_saved player_specials_saved;
    struct char_ability_data abilities;
    struct char_point_data points;
    struct affected_type affected[MAX_AFFECT];

    time_t last_logon;		/* Time (in secs) of last logon */
    char host[HOST_LENGTH+1];	/* host of last logon */
};
/* ====================================================================== */


struct dex_skill_type {
    sh_int p_pocket;
    sh_int p_locks;
    sh_int traps;
    sh_int sneak;
    sh_int hide;
};


struct dex_app_type {
    sh_int reaction;
    sh_int miss_att;
    sh_int defensive;
};


struct str_app_type {
    sh_int tohit;    /* To Hit (THAC0) Bonus/Penalty        */
    sh_int todam;    /* Damage Bonus/Penalty                */
    sh_int carry_w;  /* Maximum weight that can be carrried */
    sh_int wield_w;  /* Maximum weight that can be wielded  */
};


struct wis_app_type {
    byte bonus;       /* how many practices player gains per lev */
};


struct int_app_type {
    byte learn;       /* how many % a player learns a spell/skill */
};


struct con_app_type {
    sh_int hitp;
    sh_int shock;
};

struct title_type {
    char	*title_m;
    char	*title_f;
};



#endif __char_data_h__
