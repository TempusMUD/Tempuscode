#include <unistd.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "interpreter.h"
#include "utils.h"
#include "db.h"

const size_t DEFAULT_ACCUM_SIZE = 65536;	// 64k to start with

static size_t acc_str_space = 0;
static size_t acc_str_len = 0;
static char *acc_str_data = NULL;

// Initializes the structures used for the accumulator string mechanism
void
acc_string_init(void)
{
	acc_str_space = DEFAULT_ACCUM_SIZE;
	acc_str_data = (char *)malloc(acc_str_space);
}

// Clears the accumulator for use.
void
acc_string_clear(void)
{
	acc_str_len = 0;
	*acc_str_data = '\0';
}

// When the accumulator overflows, we increase its size here
void
acc_string_adjust(size_t wanted)
{
	if (acc_str_space > wanted)
		return;

	// if they want more than we've got, chances are they'll want
	// even more, so we increase it in chunks
	while (acc_str_space < wanted)
		acc_str_space += DEFAULT_ACCUM_SIZE;

	acc_str_data = (char *)realloc(acc_str_data, acc_str_space);
}

// vsprintf into a accum str
void
acc_vsprintf(const char *fmt, va_list args)
{
	size_t wanted;
	char *result;

	result = acc_str_data + acc_str_len;
	wanted = vsnprintf(result, acc_str_space - acc_str_len, fmt, args);

	// If there was enough space, our work is done here.  If there wasn't enough
	// space, we expand the accumulator, and write into that.

	if (acc_str_space - acc_str_len < wanted) {
		acc_string_adjust(acc_str_space + wanted);
		wanted = vsnprintf(result, acc_str_space - acc_str_len, fmt, args);
	}

	acc_str_len += wanted;
}

// sprintf into the string accumulator
void
acc_sprintf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	acc_vsprintf(fmt, args);
	va_end(args);
}

// strcat into a string accumulator.  You must terminate the arguments with
// a NULL, since the va_arg method is too stupid to give us the number of
// arguments.
void
acc_strcat(const char *str, ...)
{
	char *write_pt, *result;
	const char *read_pt;
	size_t len;
	va_list args;

	// Figure out how much space we'll need
	len = acc_str_len + strlen(str);

	va_start(args, str);
	while ((read_pt = va_arg(args, const char *)) != NULL)
		len += strlen(read_pt);
	va_end(args);

	len += 1;


	// If we don't have the space, we allocate another pool
	if (len > acc_str_space - acc_str_len)
		acc_string_adjust(acc_str_space + len);

	result = acc_str_data + acc_str_len;
	write_pt = result;
	acc_str_len += len;

	// Copy in the first string
	strcpy(write_pt, str);
	while (*write_pt)
		write_pt++;

	// Then copy in the rest of the strings
	va_start(args, str);
	while ((read_pt = va_arg(args, const char *)) != NULL) {
		strcpy(write_pt, read_pt);
		while (*write_pt)
			write_pt++;
	}
	va_end(args);
}

// Get the current size of the accumulator
size_t
acc_get_length(void)
{
	return acc_str_len;
}

char *
acc_get_string(void)
{
	return acc_str_data;
}
