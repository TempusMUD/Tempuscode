#ifndef _TMPSTR_H_
#define _TMPSTR_H_

#include <unistd.h>
#include <stdarg.h>

void tmp_string_init(void);
void tmp_gc_strings(void);
size_t tmp_storage_space(void);
size_t tmp_storage_used(void);
char *tmp_vsprintf(const char *fmt, va_list args); 
char *tmp_sprintf(const char *fmt, ...);
char *tmp_strcat(char *src, ...);
char *tmp_getword(char **src);

inline char *tmp_strdup(char *src)
{
	return tmp_strcat(src, NULL);
}

#endif
