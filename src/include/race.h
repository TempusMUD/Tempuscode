#ifndef _RACES_H_
#define _RACES_H_

/*
  #include <stdint.h>
  #include <stdbool.h>
  #include <glib.h>
  #include "utils.h"
*/

struct creature;

enum race_num {
    RACE_UNDEFINED = -1,
    RACE_HUMAN = 0,
    RACE_ELF = 1,
    RACE_DWARF = 2,
    RACE_HALF_ORC = 3,
    RACE_KLINGON = 4,
    RACE_HALFLING = 5,
    RACE_TABAXI = 6,
    RACE_DROW = 7,
    RACE_MOBILE = 10,
    RACE_UNDEAD = 11,
    RACE_HUMANOID = 12,
    RACE_ANIMAL = 13,
    RACE_DRAGON = 14,
    RACE_GIANT = 15,
    RACE_ORC = 16,
    RACE_GOBLIN = 17,
    RACE_HAFLING = 18,
    RACE_MINOTAUR = 19,
    RACE_TROLL = 20,
    RACE_GOLEM = 21,
    RACE_ELEMENTAL = 22,
    RACE_OGRE = 23,
    RACE_DEVIL = 24,
    RACE_TROGLODYTE = 25,
    RACE_MANTICORE = 26,
    RACE_BUGBEAR = 27,
    RACE_DRACONIAN = 28,
    RACE_DUERGAR = 29,
    RACE_SLAAD = 30,
    RACE_ROBOT = 31,
    RACE_DEMON = 32,
    RACE_DEVA = 33,
    RACE_PLANT = 34,
    RACE_ARCHON = 35,
    RACE_PUDDING = 36,
    RACE_ALIEN_1 = 37,
    RACE_PRED_ALIEN = 38,
    RACE_SLIME = 39,
    RACE_ILLITHID = 40,
    RACE_FISH = 41,
    RACE_BEHOLDER = 42,
    RACE_GASEOUS = 43,
    RACE_GITHYANKI = 44,
    RACE_INSECT = 45,
    RACE_DAEMON = 46,
    RACE_MEPHIT = 47,
    RACE_KOBOLD = 48,
    RACE_UMBER_HULK = 49,
    RACE_WEMIC = 50,
    RACE_RAKSHASA = 51,
    RACE_SPIDER = 52,
    RACE_GRIFFIN = 53,
    RACE_ROTARIAN = 54,
    RACE_HALF_ELF = 55,
    RACE_CELESTIAL = 56,
    RACE_GUARDINAL = 57,
    RACE_OLYMPIAN = 58,
    RACE_YUGOLOTH = 59,
    RACE_ROWLAHR = 60,
    RACE_GITHZERAI = 61,
    RACE_FAERIE = 62,
    NUM_RACES = 63,
    NUM_PC_RACES = 9,
};

struct race {
    int idnum;
    char *name;
    int8_t str_mod;
    int8_t int_mod;
    int8_t wis_mod;
    int8_t dex_mod;
    int8_t con_mod;
    int8_t cha_mod;
    int age_adjust;
    int lifespan;

    /* Intrinsic affects */
    int aff1;
    int aff2;
    int aff3;

    /* Weight min and max for each sex */
    int weight_min[3];
    int weight_max[3];
    int height_min[3];
    int height_max[3];
    int weight_add[3];          /* Portion of weight adding to height */
};

extern GHashTable *races;

struct race *race_by_idnum(int idnum);
struct race *race_by_name(char *name, bool exact);
const char *race_name_by_idnum(int idnum);

void boot_races(const char *path);
int max_creature_str(struct creature *ch);
int max_creature_int(struct creature *ch);
int max_creature_wis(struct creature *ch);
int max_creature_dex(struct creature *ch);
int max_creature_con(struct creature *ch);
int max_creature_cha(struct creature *ch);

#endif
