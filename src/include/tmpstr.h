#ifndef _TMPSTR_H_
#define _TMPSTR_H_

#include <time.h>
// vsprintf into a temp str
/*@shared@*/ char *tmp_vsprintf(const char *fmt, va_list args)
__attribute__ ((format_arg (1)));

// sprintf into a temp str
/*@shared@*/ char *tmp_sprintf(const char *fmt, ...)
__attribute__ ((format (printf, 1, 2)));

// returns a temp str of length n filled with c
/*@shared@*/ char *tmp_pad(int c, size_t n);

// get the next word, copied into a temp pool
/*@shared@*/ char *tmp_gettoken(char **src);
/*@shared@*/ char *tmp_gettoken_const(const char **src);

// like tmp_gettoken, except it downcases the result
/*@shared@*/ char *tmp_getword(char **src);
/*@shared@*/ char *tmp_getword_const(const char **src);

// like tmp_getword, except it pulls out an entire string, if delimited by
// quotation marks.  Otherwise, acts just like tmp_getword.
/*@shared@*/ char *tmp_getquoted(char **src);

// like tmp_getword, except it pulls out a single line, broken with a CR or a LF
/*@shared@*/ char *tmp_getline(char **src);
/*@shared@*/ char *tmp_getline_const(const char **src);

// strcat into a temp str.  You must terminate the arguments with a NULL,
// since the va_arg method is too stupid to give us the number of arguments.
/*@shared@*/ char *tmp_strcat(const char *src, ...)
__attribute__((sentinel));

// creates a copy of the given str as a temp str
/*@shared@*/ char *tmp_strdup(const char *src);
/*@shared@*/ char *tmp_strdupn(const char *src, ssize_t n);
/*@shared@*/ char *tmp_strdupt(const char *src, const char *term);

// returns a string, in which every needle in haystack is substituted with sub
/*@shared@*/ char *tmp_gsub(const char *haystack, const char *needle, const char *sub);

// case insensitive version of tmp_gsub
/*@shared@*/ char *tmp_gsubi(const char *haystack, const char *needle, const char *sub);

// returns a copy of str with all characters converted to lowercase
/*@shared@*/ char *tmp_tolower(const char *str);

// returns a copy of str with all character converted to uppercase
/*@shared@*/ char *tmp_toupper(const char *str);

// returns a copy of str with the first character capitalized
/*@shared@*/ char *tmp_capitalize(const char *str);

// returns an escaped string to be used in a sql query
/*@shared@*/ char *tmp_sqlescape(const char *str);

// returns the standard string representation of a unix timestamp
/*@shared@*/ char *tmp_ctime(time_t val);

// returns the formatted time string of a tm structure
/*@shared@*/ char *tmp_strftime(const char *fmt, const struct tm *tm);

// returns a string which contains a space-delimited list of each bit
// description whose index corresponds to a set bit in the value
/*@shared@*/ char *tmp_printbits(int val, const char *bit_descs[]);

// returns a string from the start_pos to the end_pos
/*@shared@*/ char *tmp_substr(const char *str, int start_pos, int end_pos);

// returns a string with leading and trailing spaces removed
/*@shared@*/ char *tmp_trim(const char *str);

// returns a formatted string word-wrapped to the given width, the
// first line indented by first_indent, lines preceded by a line break
// are indented by par_indent, and the rest of the lines indented by
// rest_indent.  Attempts to correct spacing between words and
// punctuation.  Intended for formatting user input.
/*@shared@*/ char *
tmp_format(const char *str, int width, int first_indent, int par_indent, int rest_indent);

// returns a string word-wrapped to the given width, the first line
// indented by first_indent, lines preceded by a line break are
// indented by par_indent, and the rest of the lines indented by
// rest_indent.  This function will correctly wrap ANSI escape
// seqences.  Intended for wrapping output for display.
/*@shared@*/ char *
tmp_wrap(const char *str, int width, int first_indent, int par_indent, int rest_indent);


// Initializes the structures used for the temporary string mechanism
void tmp_string_init(void);

// Frees all memory used by the tmpstr pool.  This is mostly used in
// testing to prevent memory leak false positives.
void tmp_string_cleanup(void);

// tmp_gc_strings will deallocate every temporary string, adjust the
// size of the string pool.  All previously allocated strings will be
// invalid after this is called.
void tmp_gc_strings(void);

extern size_t tmp_max_used;

#endif
