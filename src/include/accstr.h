#ifndef _ACCSTR_H_
#define _ACCSTR_H_

// Initialize the string accumulator
void acc_string_init(void);

// Clears the string accumulator for use.
void acc_string_clear(void);

// vsprintf into a string accumulator
void acc_vsprintf(const char *fmt, va_list args);

// sprintf into the string accumulator
void acc_sprintf(const char *fmt, ...)
	__attribute__ ((format (printf, 1, 2))); 

// strcat into a string accumulator.  You must terminate the arguments with
// a NULL, since the va_arg method is too stupid to give us the number of
// arguments.
void acc_strcat(const char *str, ...);

// Get the current size of the accumulator
size_t acc_get_length(void);

// Grab the string after we've gotten everything out of it
char *acc_get_string(void);

extern size_t acc_str_space;

#endif
