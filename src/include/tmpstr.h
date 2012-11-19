#ifndef _TMPSTR_H_
#define _TMPSTR_H_

// vsprintf into a temp str
char *tmp_vsprintf(const char *fmt, va_list args)
	__attribute__ ((format_arg (1)));

// sprintf into a temp str
char *tmp_sprintf(const char *fmt, ...)
	__attribute__ ((format (printf, 1, 2)));

// returns a temp str of length n filled with c
char *tmp_pad(int c, size_t n);

// get the next word, copied into a temp pool
char *tmp_gettoken(char **src);
char *tmp_gettoken_const(const char **src);

// like tmp_gettoken, except it downcases the result
char *tmp_getword(char **src);
char *tmp_getword_const(const char **src);

// like tmp_getword, except it pulls out an entire string, if delimited by
// quotation marks.  Otherwise, acts just like tmp_getword.
char *tmp_getquoted(char **src);

// like tmp_getword, except it pulls out a single line, broken with a CR or a LF
char *tmp_getline(char **src);
char *tmp_getline_const(const char **src);

// strcat into a temp str.  You must terminate the arguments with a NULL,
// since the va_arg method is too stupid to give us the number of arguments.
char *tmp_strcat(const char *src, ...)
    __attribute__((sentinel));

// creates a copy of the given str as a temp str
char *tmp_strdup(const char *src);
char *tmp_strdupt(const char *src, const char *term);

// returns a string, in which every needle in haystack is substituted with sub
char *tmp_gsub(const char *haystack, const char *needle, const char *sub);

// case insensitive version of tmp_gsub
char *tmp_gsubi(const char *haystack, const char *needle, const char *sub);

// returns a copy of str with all characters converted to lowercase
char *tmp_tolower(const char *str);

// returns a copy of str with all character converted to uppercase
char *tmp_toupper(const char *str);

// returns a copy of str with the first character capitalized
char *tmp_capitalize(const char *str);

// returns an escaped string to be used in a sql query
char *tmp_sqlescape(const char *str);

// returns the standard string representation of a unix timestamp
char *tmp_ctime(time_t val);

// returns a string which contains a space-delimited list of each bit
// description whose index corresponds to a set bit in the value
char *tmp_printbits(int val, const char *bit_descs[]);

// returns a string from the start_pos to the end_pos
char *tmp_substr(const char *str, int start_pos, int end_pos);

// returns a string with leading and trailing spaces removed
char *tmp_trim(const char *str);

// returns a string word-wrapped to the given width, the first line
// indented by first_indent, lines preceded by a line break are
// indented by par_indent, and the rest of the lines indented
// by rest_indent
char *
tmp_format(const char *str, int width, int first_indent, int par_indent, int rest_indent);

// Initializes the structures used for the temporary string mechanism
void tmp_string_init(void);

// tmp_gc_strings will deallocate every temporary string, adjust the
// size of the string pool.  All previously allocated strings will be
// invalid after this is called.
void tmp_gc_strings(void);

extern size_t tmp_max_used;

#endif
