#ifndef _TMPSTR_H_
#define _TMPSTR_H_

#include <unistd.h>
#include <stdarg.h>

// vsprintf into a temp str
char *tmp_vsprintf(const char *fmt, va_list args)
	__attribute__ ((format_arg (1))); 

// sprintf into a temp str
char *tmp_sprintf(const char *fmt, ...)
	__attribute__ ((format (printf, 1, 2))); 

// returns a temp str of length n filled with c
char *tmp_pad(int c, size_t n);

// get the next word, copied into a temp pool
char *tmp_getword(const char **src);
inline char *tmp_getword(char **src)
	{ return tmp_getword((const char **)src); }

// like tmp_getword, except it pulls out an entire string, if delimited by
// quotation marks.  Otherwise, acts just like tmp_getword.
char *tmp_getquoted(const char **src);
inline char *tmp_getquoted(char **src)
	{ return tmp_getquoted((const char **)src); }

// like tmp_getword, except it pulls out a single line, broken with a CR or a LF
char *tmp_getline(const char **src);
inline char *tmp_getline(char **src)
	{ return tmp_getline((const char **)src); }

// strcat into a temp str.  You must terminate the arguments with a NULL,
// since the va_arg method is too stupid to give us the number of arguments.
char *tmp_strcat(const char *src, ...);

// strcat into a temp str.  
inline char *tmp_strcat(const char *src_a, const char *src_b)
{
	return tmp_strcat(src_a, src_b, NULL);
}

// creates a copy of the given str as a temp str
char *tmp_strdup(const char *src, const char *term = NULL);

// returns a string, in which every needle in haystack is substituted with sub
char *tmp_gsub(const char *haystack, const char *needle, const char *sub);

// returns a copy of str with all characters converted to lowercase
char *tmp_tolower(const char *str);

// returns a copy of str with the first character capitalized
char *tmp_capitalize(const char *str);

// Initializes the structures used for the temporary string mechanism
void tmp_string_init(void);

// tmp_gc_strings will deallocate every temporary string, adjust the
// size of the string pool.  All previously allocated strings will be
// invalid after this is called.
void tmp_gc_strings(void);

extern unsigned long tmp_max_used;
extern unsigned long tmp_overruns;

#endif
