#define MAX_WEAPON_SPEC 6


#define MAX_CHAR_DESC 1023
#define NEW_HOST_LENGTH 63

/* Char's points.  Used in char_file_u *DO*NOT*CHANGE* */
struct NEWchar_point_data {
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
 *  If you want to add new values to the playerfile, do it here.  DO NOT
 * ADD, DELETE OR MOVE ANY OF THE VARIABLES - doing so will change the
 * size of the structure and ruin the playerfile.  However, you can change
 * the names of the spares to something more meaningful, and then use them
 * in your new code.  They will automatically be transferred from the
 * playerfile into memory when players log in.
 */
struct NEWplayer_special_data_saved {
  byte skills[MAX_SKILLS+1];	/* array of skills plus skill 0		*/
  weapon_spec weap_spec[MAX_WEAPON_SPEC];
  int	wimp_level;		/* Below this # of hit points, flee!	*/
  byte freeze_level;		/* Level of god who froze char, if any	*/
  sh_int invis_level;		/* level of invisibility		*/
  room_num load_room;		/* Which room to place char in		*/
  long	pref;			/* preference flags for PC's.		*/
  long  pref2;                  /* 2nd pref flag                        */
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
  ubyte spare_c[4];
  int deity;
  int spells_to_learn;
  int life_points;
  int pkills;
  int mobkills;
  int deaths;
  int old_class;               /* Type of borg, or class before vamprism. */
  int page_length;
  int total_dam;
  int columns;
  int hold_load_room;
  int spare_i[4];
  long	mana_shield_low;
  long	mana_shield_pct;
  long	spare19;
  long	spare20;
  long	spare21;

};
/* An affect structure.  Used in char_file_u *DO*NOT*CHANGE* */
struct NEWaffected_type {
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
/* ==================== File Structure for Player ======================= */
/*             BEWARE: Changing it will ruin the playerfile		  */
struct NEWchar_file_u {
   /* char_player_data */
   char	name[MAX_NAME_LENGTH+1];
   char	description[MAX_CHAR_DESC+1];
   char	title[MAX_TITLE_LENGTH+1];
   char poofin[MAX_POOF_LENGTH];
   char poofout[MAX_POOF_LENGTH];
   sh_int char_class;
   sh_int remort_class;
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
   struct NEWplayer_special_data_saved player_specials_saved;
   struct char_ability_data abilities;
   struct NEWchar_point_data points;
   struct NEWaffected_type affected[MAX_AFFECT];

   time_t last_logon;		/* Time (in secs) of last logon */
   char host[NEW_HOST_LENGTH+1];	/* host of last logon */
};
/* ====================================================================== */

