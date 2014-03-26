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

static inline char *CAP(/*@returned@*/ char *st)
{
    *st = (char)toupper(*st);
    return st;
}

int remove_from_cstring(char *str, char c, char c_to)
    __attribute__ ((nonnull));
void sprintbit(long vektor, const char *names[], char *result);
const char *strlist_aref(int idx, const char **names);
void sprinttype(int type, const char *names[], char *result);
const char *AN(const char *str)
    __attribute__ ((nonnull));
const char *YESNO(bool a);
const char *ONOFF(bool a);
char *fname(const char *namelist)
    __attribute__ ((nonnull));
int isname(const char *str, const char *namelist)
    __attribute__ ((nonnull));
int isname_exact(const char *str, const char *namelist)
    __attribute__ ((nonnull));
bool namelist_match(const char *sub_list, const char *super_list)
    __attribute__ ((nonnull));
int get_number(char **name)
    __attribute__ ((nonnull));
int find_all_dots(char *arg)
    __attribute__ ((nonnull));

enum {
    FIND_INDIV,
    FIND_ALL,
    FIND_ALLDOT,
};

char *one_word(char *argument, char *first_arg)
    __attribute__ ((nonnull));
int search_block(const char *arg, const char * const *list, bool exact)
    __attribute__ ((nonnull));
bool is_number(const char *str)
    __attribute__ ((nonnull));
void skip_spaces_const(const char **string)
    __attribute__ ((nonnull));
void skip_spaces(char **string)
    __attribute__ ((nonnull));
int fill_word(char *argument)
    __attribute__ ((nonnull));
char *one_argument(char *argument, char *first_arg)
    __attribute__ ((nonnull));
char *any_one_arg(char *argument, char *first_arg)
    __attribute__ ((nonnull));
char *two_arguments(char *argument, char *first_arg, char *second_arg)
    __attribute__ ((nonnull));
int is_abbrev(const char *arg1, const char *arg2)
    __attribute__ ((nonnull));
int is_abbrevn(const char *arg1, const char *arg2, int count)
    __attribute__ ((nonnull));
void half_chop(char *string, char *arg1, char *arg2)
    __attribute__ ((nonnull));

char *one_argument_no_lower(char *argument, char *first_arg)
    __attribute__ ((nonnull));
int search_block_no_lower(char *arg, const char **list, bool exact)
    __attribute__ ((nonnull));
int fill_word_no_lower(char *argument)
    __attribute__ ((nonnull));
void num2str(char *str, int num)
    __attribute__ ((nonnull));

#endif
