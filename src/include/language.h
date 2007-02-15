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

#include <stdio.h>
#include <map>
#include "creature.h"

class Tongue {
    struct trans_pair {
        char _pattern[10];
        char _replacement[10];
    };
public:
    Tongue();
    Tongue(const Tongue &o);
    ~Tongue(void);
    Tongue &operator=(const Tongue &o);

    void clear(void);
    bool load(xmlNodePtr node);
    char *translate(const char *str, int amount);

    int _idnum;
    char *_name;
    trans_pair *_syllables;
    int _syllable_count;
    char _letters[256];
    char *_nospeak_msg;
private:
    char *translate_word(char *word);
};

extern std::map<int,Tongue> tongues;

#define GET_LANG_HEARD(ch) ((ch)->language_data->languages_heard)
#define GET_TONGUE(ch) ((ch)->language_data->current_language)

char *translate_tongue(Creature *speaker, Creature *listener, const char *message);
char *make_tongue_str(Creature *ch, Creature *to);
void set_initial_tongue(Creature * ch);
void write_tongue_xml(Creature *ch, FILE *ouf);
int racial_tongue(int race_idx);
const char *tongue_name(int tongue_idx);
const char *fluency_desc(Creature *ch, int tongue_idx);
void show_language_help(Creature *ch);
int find_tongue_idx_by_name(const char *tongue_name);

static const int TONGUE_NONE =                          -1;
static const int TONGUE_COMMON =                        0;

#endif
