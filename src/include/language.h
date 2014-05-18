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

struct trans_pair {
    char pattern[15];
    char replacement[15];
};
struct tongue {
    int idnum;
    char *name;
    struct trans_pair *syllables;
    int syllable_count;
    char letters[256];
    char *nospeak_msg;
};

extern GHashTable *tongues;

#define GET_LANG_HEARD(ch) ((ch)->language_data.languages_heard)
#define GET_TONGUE(ch) ((ch)->language_data.current_language)

char *translate_with_tongue(struct tongue *tongue,
                            const char *phrase,
                            int amount);
char *translate_tongue(struct creature *speaker,
                       struct creature *listener,
                       const char *message);
char *translate_string(int tongue, const char *str, int accuracy);
char *make_tongue_str(struct creature *ch, struct creature *to);
void set_initial_tongue(struct creature * ch);
void write_tongue_xml(struct creature *ch, FILE *ouf);
int racial_tongue(int race_idx);
const char *tongue_name(int tongue_idx);
const char *fluency_desc(struct creature *ch, int tongue_idx);
void show_language_help(struct creature *ch);
int find_tongue_idx_by_name(const char *tongue_name);
struct tongue *find_tongue_by_idnum(int tongue_id);

enum {
    TONGUE_NONE = -1,
    TONGUE_COMMON = 0,
    TONGUE_ARCANUM = 1
};

#endif
