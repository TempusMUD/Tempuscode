#ifndef _LANGUAGE_H_
#define _LANGUAGE_H_

/*************************************************************************
 *  File: language.h                                                     *
 *  Usage: Source file for multi-language support                        *
 *                                                                       *
 *  All rights reserved.  See license.doc for complete information.      *
 *                                                                       *
 *********************************************************************** */

//
// File: language.h                            -- Part of TempusMUD
//

#include "creature.h"

#define GET_LANGUAGE(ch) ((ch)->language_data.current_language)
#define KNOWN_LANGUAGES(ch) ((ch)->language_data.known_languages)

int can_speak_language(Creature *ch, char language_idx);
int find_language_idx(long long language);
int find_language_idx_by_name(char *language_name);
int find_language_idx_by_race(const char *race_name);
int known_languages(Creature *ch);
char *translate_string(char *phrase, char language_idx); 
void translate_word(char **word, char language_idx);
void set_initial_language(Creature *ch);
void learn_language(Creature *ch, char language_idx);
void forget_language(Creature *ch, char language_idx);

// Num languages should always be the number of languages defined
// below NOT INCLUDING COMMON!!!!!!!!
static const int NUM_LANGUAGES = 33;

// This is the number of syllable entries per language
static const int NUM_SYLLABLES = 58;

static const long long LANGUAGE_NONE =                          -2;
static const long long LANGUAGE_COMMON =                        -1;
static const long long LANGUAGE_MODRIATIC =     ((long long)1 << 0);
static const long long LANGUAGE_ELVISH =        ((long long)1 << 1);
static const long long LANGUAGE_DWARVEN =       ((long long)1 << 2);
static const long long LANGUAGE_ORCISH =        ((long long)1 << 3);
static const long long LANGUAGE_KLINGON =       ((long long)1 << 4);
static const long long LANGUAGE_HOBBISH =       ((long long)1 << 5);
static const long long LANGUAGE_SYLVAN =        ((long long)1 << 6);
static const long long LANGUAGE_UNDERDARK =     ((long long)1 << 7);
static const long long LANGUAGE_MORDORIAN =     ((long long)1 << 8);
static const long long LANGUAGE_DAEDALUS =      ((long long)1 << 9);
static const long long LANGUAGE_TROLLISH =      ((long long)1 << 10);
static const long long LANGUAGE_OGRISH =        ((long long)1 << 11);
static const long long LANGUAGE_INFERNUM =      ((long long)1 << 12);
static const long long LANGUAGE_TROGISH =       ((long long)1 << 13);
static const long long LANGUAGE_MANTICORA =     ((long long)1 << 14);
static const long long LANGUAGE_GHENNISH =      ((long long)1 << 15);
static const long long LANGUAGE_DRACONIAN =     ((long long)1 << 16);
static const long long LANGUAGE_INCONNU =       ((long long)1 << 17);
static const long long LANGUAGE_ABYSSAL =       ((long long)1 << 18);
static const long long LANGUAGE_ASTRAL =        ((long long)1 << 19);
static const long long LANGUAGE_SIBILANT =      ((long long)1 << 20);
static const long long LANGUAGE_GISH =          ((long long)1 << 21);
static const long long LANGUAGE_CENTRALIAN =    ((long long)1 << 22);
static const long long LANGUAGE_KOBOLAS =       ((long long)1 << 23);
static const long long LANGUAGE_KALERRIAN =     ((long long)1 << 24);
static const long long LANGUAGE_RAKSHASIAN =    ((long long)1 << 25);
static const long long LANGUAGE_GRYPHUS =       ((long long)1 << 26);
static const long long LANGUAGE_ROTARIAL =      ((long long)1 << 27);
static const long long LANGUAGE_CELESTIAL =     ((long long)1 << 28);
static const long long LANGUAGE_ELYSIAN =       ((long long)1 << 29);
static const long long LANGUAGE_GREEK =         ((long long)1 << 30);
static const long long LANGUAGE_DEADITE =       ((long long)1 << 31);
static const long long LANGUAGE_ELEMENTAL =     ((long long)1 << 32);

struct syl_struct {
    char *org;
    char *rep;
};

extern const char *language_names[];

#endif
