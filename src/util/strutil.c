#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <libxml/parser.h>

#include "utils.h"
#include "tmpstr.h"
#include "strutil.h"

const char *fill_words[] = {
    "in",
    "from",
    "with",
    "the",
    "on",
    "at",
    "to",
    "\n"
};

/**
 * snprintf_cat:
 * @param dest Destination buffer
 * @param size Size of destination buffer
 * @param fmt Format string for snprintf(2)
 *
 * Acts as a concatenating version of snprintf().  Returns the total
 * length of the string if it fits into the buffer size.  Otherwise
 * returns the length the string that would have been written.
 **/
int
snprintf_cat(char *dest, size_t size, const char *fmt, ...)
{
    va_list args;
    size_t len = strlen(dest);

    va_start(args, fmt);

    int result = vsnprintf(dest + len, size - len, fmt, args);

    va_end(args);

    return result + len;
}


/**
 * remove_from_cstring:
 * @param str String to modify
 * @param c Character to search for
 * @param c_to Replacement character
 *
 * Replaces all occurrences of a character in a string with another.
 **/
void
remove_from_cstring(char *str, char c, char c_to)
{
    if (!str) {
        return;
    }

    for (char *p = str; *p; ++p) {
        if (*p == c) {
            *p = c_to;
        }
    }
}

void
sprintbit(long vektor, const char *names[], char *result, size_t size)
{
    long nr;

    *result = '\0';

    if (vektor < 0) {
        strcpy_s(result, size, "SPRINTBIT ERROR!");
        return;
    }
    for (nr = 0; vektor; vektor /= 2) {
        if ((1 & vektor) != 0) {
            if (*names[nr] != '\n') {
                strcat_s(result, size, names[nr]);
                strcat_s(result, size, " ");
            } else {
                strcat_s(result, size, "UNDEFINED ");
            }
        }
        if (*names[nr] != '\n') {
            nr++;
        }
    }

    if (!*result) {
        strcat_s(result, size, "NOBITS ");
    }
}

const char *
strlist_aref(int idx, const char **names)
{
    int nr;

    if (idx < 0) {
        return tmp_sprintf("ILLEGAL(%d)", idx);
    }

    for (nr = 0; *names[nr] != '\n'; nr++) {
        if (idx == nr) {
            return names[idx];
        }
    }

    return tmp_sprintf("UNDEFINED(%d)", idx);
}

const char *
AN(const char *str)
{
    if (PLUR(str)) {
        return "some";
    }
    if (strchr("aeiouAEIOU", *str)) {
        return "an";
    }
    return "a";
}

const char *
YESNO(bool a)
{
    if (a) {
        return "YES";
    } else {
        return "NO";
    }
}

const char *
ONOFF(bool a)
{
    if (a) {
        return "ON";
    }
    return "OFF";
}

char *
fname(const char *namelist)
{
    static char holder[255];
    char *point;

    if (!namelist) {
        errlog("Null namelist passed to fname().");
        return tmp_strdup("");
    }
    for (point = holder; isalnum(*namelist); namelist++, point++) {
        *point = *namelist;
    }

    *point = '\0';

    return (holder);
}

int
isname(const char *str, const char *namelist)
{
    register const char *curname, *curstr;

    if (!*str) {
        return 0;
    }

    if (namelist == NULL || *namelist == '\0') {
        errlog(" NULL namelist given to isname()");
        return 0;
    }

    curname = namelist;
    while (*curname) {
        curstr = str;
        while (true) {
            if (!*curstr || isspace(*curstr)) {
                return true;
            }

            if (!*curname) {
                return false;
            }

            if (*curname == ' ') {
                break;
            }

            if (tolower(*curstr) != tolower(*curname)) {
                break;
            }

            curstr++;
            curname++;
        }

        /* skip to next name */

        while (isalnum(*curname)) {
            curname++;
        }
        if (!*curname) {
            return false;
        }
        curname++;              /* first char of new name */
    }

    return false;
}

int
isname_exact(const char *str, const char *namelist)
{
    register const char *curname, *curstr;

    if (!*str) {
        return 0;
    }

    if (namelist == NULL || *namelist == '\0') {
        errlog(" NULL namelist given to isname()");
        return 0;
    }

    curname = namelist;
    while (*curname) {
        curstr = str;
        while (true) {
            if ((!*curstr || isspace(*curstr)) && !isalnum(*curname)) {
                return true;
            }

            if (!*curname) {
                return false;
            }

            if (*curname == ' ') {
                break;
            }

            if (tolower(*curstr) != tolower(*curname)) {
                break;
            }

            curstr++;
            curname++;
        }

        /* skip to next name */

        while (isalnum(*curname)) {
            curname++;
        }
        if (!*curname) {
            return false;
        }
        curname++;              /* first char of new name */
    }

    return false;
}

// Returns true if all words in list_targ can be found in list_vict
// list targ and list_vict are both space delimited strings
bool
namelist_match(const char *sub_list, const char *super_list)
{
    const char *word_pt;

    word_pt = sub_list;
    if (!*word_pt) {
        return false;
    }
    while (*word_pt) {
        if (!isname(word_pt, super_list)) {
            return false;
        }
        while (isgraph(*word_pt)) {
            word_pt++;
        }
        while (isspace(*word_pt)) {
            word_pt++;
        }
    }
    return true;
}

// Given a designation '3.object', returns 3 and sets argument to 'object'
// Given '.object', returns 0 and sets argument to 'object'
int
get_number(char **name)
{
    char *read_pt, *write_pt;
    int i = 0;

    if ((read_pt = strchr(*name, '.'))) {
        *read_pt++ = '\0';

        if (**name) {
            i = atoi(*name);
        }

        write_pt = *name;
        while (*read_pt) {
            *write_pt++ = *read_pt++;
        }
        *write_pt = '\0';
        return i;
    }
    return 1;
}

/* a function to scan for "all" or "all.x" */
int
find_all_dots(char *arg)
{
    if (!strcmp(arg, "all")) {
        return FIND_ALL;
    } else if (!strncmp(arg, "all.", 4)) {
        memmove(arg, arg + 4, strlen(arg) - 3);
        return FIND_ALLDOT;
    } else {
        return FIND_INDIV;
    }
}


/* One_Word is like one_argument, execpt that words in quotes "" are */
/* regarded as ONE word                                              */

char *
one_word(char *argument, char *first_arg)
{
    int begin, look_at;

    begin = 0;

    do {
        while (isspace(*(argument + begin))) {
            begin++;
        }

        if (*(argument + begin) == '\"') {  /* is it a quote */

            begin++;

            for (look_at = 0; (*(argument + begin + look_at) >= ' ') &&
                 (*(argument + begin + look_at) != '\"'); look_at++) {
                *(first_arg + look_at) =
                    tolower(*(argument + begin + look_at));
            }

            if (*(argument + begin + look_at) == '\"') {
                begin++;
            }

        } else {

            for (look_at = 0; *(argument + begin + look_at) > ' '; look_at++) {
                *(first_arg + look_at) =
                    tolower(*(argument + begin + look_at));
            }

        }

        first_arg[look_at] = '\0';
        begin += look_at;
    } while (fill_word(first_arg));

    return (argument + begin);
}


/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 */
int
search_block(const char *arg, const char *const *list, bool exact)
{
    if (*arg == '\0') {
        return -1;
    }

    if (exact) {
        for (int i = 0; *(list[i]) != '\n'; i++) {
            if (*(list[i]) == '!') { // reserved
                continue;
            }
            if (strcasecmp(arg, list[i]) == 0) {
                return i;
            }
        }
    } else {
        int len = strlen(arg);
        for (int i = 0; *(list[i]) != '\n'; i++) {
            if (*(list[i]) == '!') { // reserved
                continue;
            }
            if (strncasecmp(arg, list[i], len) == 0) {
                return i;
            }
            if (i > 1000) {
                errlog
                    ("search_block in unterminated list. arg = %s [0] = '%s'.",
                    arg, list[0]);
                return -1;
            }
        }
    }
    return -1;
}

bool
is_number(const char *str)
{
    if (!*str) {
        return false;
    }

    if (str[0] == '-' || str[0] == '+') {
        str++;
    }

    while (*str) {
        if (!isdigit(*(str++))) {
            return false;
        }
    }

    return true;
}

bool
is_float_number(const char *str)
{
    int radix_count = 0;

    if (!*str) {
        return false;
    }

    if (str[0] == '-' || str[0] == '+') {
        str++;
    }

    while (*str) {
        if (*str == '.') {
            radix_count++;
            if (radix_count > 1) {
                return false;
            }
        } else if (!isdigit(*str)) {
            return false;
        }

        str++;
    }

    return true;
}

void
skip_spaces_const(const char **string)
{
    while (**string && isspace(**string)) {
        (*string)++;
    }
}

void
skip_spaces(char **string)
{
    while (**string && isspace(**string)) {
        (*string)++;
    }
}

int
fill_word(char *argument)
{
    return (search_block(argument, fill_words, true) >= 0);
}

/*
 * copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string.
 */
char *
one_argument(char *argument, char *first_arg)
{
    char *begin = first_arg;

    do {
        skip_spaces(&argument);

        first_arg = begin;
        while (*argument && !isspace(*argument)) {
            *(first_arg++) = tolower(*argument);
            argument++;
        }

        *first_arg = '\0';
    } while (fill_word(begin));

    return argument;
}

/* same as one_argument except that it doesn't ignore fill words */
char *
any_one_arg(char *argument, char *first_arg)
{
    skip_spaces(&argument);

    while (*argument && !isspace(*argument)) {
        *(first_arg++) = tolower(*argument);
        argument++;
    }

    *first_arg = '\0';

    return argument;
}

/*
 * Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words
 */
char *
two_arguments(char *argument, char *first_arg, char *second_arg)
{
    char *s;

    do {
        s = tmp_getword(&argument);
    } while (fill_word(s));

    strcpy(first_arg, s);

    do {
        s = tmp_getword(&argument);
    } while (fill_word(s));

    strcpy(second_arg, s);

    return argument;
}

int
is_abbrev(const char *needle, const char *haystack)
{
    if (!*needle) {
        return 0;
    }

    while (*needle && *haystack) {
        if (tolower(*needle++) != tolower(*haystack++)) {
            return 0;
        }
    }

    if (!*needle && !*haystack) {
        return 2;
    }
    if (!*needle) {
        return 1;
    }

    return 0;
}

/*
 * determine if a given string is an abbreviation of another
 * (now works symmetrically -- JE 7/25/94)
 *
 * that was dumb.  it shouldn't be symmetrical.  JE 5/1/95
 *
 * returns 1 if arg1 is an abbreviation of arg2
 * returns 2 if arg1 is an extact match to arg2
 * returns 0 otherwise
 *
 * added 3rd argument with default for minimum match length
 * LK -- 2/22/2004
 */
int
is_abbrevn(const char *needle, const char *haystack, int count)
{
    int matched = 0;

    if (!*needle) {
        return 0;
    }

    while (*needle && *haystack) {
        matched++;
        if (tolower(*needle++) != tolower(*haystack++)) {
            return 0;
        }
    }

    if (!*needle && !*haystack) {
        return 2;
    }
    if ((!*needle) && matched >= count) {
        return 1;
    }

    return 0;
}

/* return first space-delimited token in arg1; remainder of string in arg2 */
void
half_chop(char *string, char *arg1, char *arg2)
{
    char *temp;

    temp = any_one_arg(string, arg1);
    skip_spaces(&temp);
    memmove(arg2, temp, strlen(temp) + 1);
}

/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 */
int
search_block_no_lower(char *arg, const char **list, bool exact)
{
    register int i, l;

    l = strlen(arg);

    if (exact) {
        for (i = 0; **(list + i) != '\n'; i++) {
            if (!strcmp(arg, *(list + i))) {
                return (i);
            }
        }
    } else {
        if (!l) {
            l = 1;              /* Avoid "" to match the first available
                                 * string */
        }
        for (i = 0; **(list + i) != '\n'; i++) {
            if (!strncmp(arg, *(list + i), l)) {
                return (i);
            }
        }
    }

    return -1;
}

int
fill_word_no_lower(char *argument)
{
    return (search_block_no_lower(argument, fill_words, false) >= 0);
}

/*
 * copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string.
 */
char *
one_argument_no_lower(char *argument, char *first_arg)
{
    char *begin = first_arg;

    do {
        skip_spaces(&argument);

        first_arg = begin;
        while (*argument && !isspace(*argument)) {
            *(first_arg++) = *argument;
            argument++;
        }

        *first_arg = '\0';
    } while (fill_word_no_lower(begin));

    return argument;
}
