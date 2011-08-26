/*
 * String utilities
 */
#ifndef _STRUTIL_H_
#define _STRUTIL_H_

#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r')
#define IF_STR(st) ((st) ? (st) : "\0")

// add special plural words that don't end in S to the SPECIAL_PLUR macro
#define SPECIAL_PLUR( buf )    ( !strcasecmp( buf, "teeth" ) || \
                                 !strcasecmp( buf, "cattle" ) ||  \
                                 !strcasecmp( buf, "data" ) )
#define SPECIAL_SING( buf )    ( !strcasecmp( buf, "portcullis" ) )
#define PLUR(buf)              ( !SPECIAL_SING( buf ) && ( SPECIAL_PLUR( buf ) || buf[strlen(buf) - 1] == 's' ) )

#define ISARE(buf)             (PLUR(buf) ? "are" : "is")
#define IT_THEY(buf)           (PLUR(buf) ? "they" : "it")
#define IT_THEM(buf)           (PLUR(buf) ? "them" : "it")

static inline char *CAP(char *st)
{
    *st = (char)toupper(*st);
    return st;
}

int remove_from_cstring(char *str, char c, char c_to);
void sprintbit(long vektor, const char *names[], char *result);
const char *strlist_aref(int idx, const char **names);
void sprinttype(int type, const char *names[], char *result);
const char *AN(const char *str);
const char *YESNO(bool a);
const char *ONOFF(bool a);
char *fname(const char *namelist);
int isname(const char *str, const char *namelist);
int isname_exact(const char *str, const char *namelist);
bool namelist_match(const char *sub_list, const char *super_list);
int get_number(char **name);
int find_all_dots(char *arg);

enum {
    FIND_INDIV,
    FIND_ALL,
    FIND_ALLDOT,
};

char *one_word(char *argument, char *first_arg);
int search_block(const char *arg, const char **list, bool exact);
bool is_number(const char *str);
void skip_spaces_const(const char **string);
void skip_spaces(char **string);
int fill_word(char *argument);
char *one_argument(char *argument, char *first_arg);
char *any_one_arg(char *argument, char *first_arg);
char *two_arguments(char *argument, char *first_arg, char *second_arg);
int is_abbrev(const char *needle, const char *haystack);
int is_abbrevn(const char *needle, const char *haystack, int count);
void half_chop(char *string, char *arg1, char *arg2);

char *one_argument_no_lower(char *argument, char *first_arg);
int search_block_no_lower(char *arg, const char **list, bool exact);
int fill_word_no_lower(char *argument);
void num2str(char *str, int num);

#endif
