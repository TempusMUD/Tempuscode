#include <unistd.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "interpreter.h"

struct tmp_str_pool {
	struct tmp_str_pool *next;		// Ptr to next in linked list
	size_t space;					// Amount of space allocated
	size_t used;					// Amount of space used in pool
	char data[0];					// The actual data
};

const size_t INITIAL_POOL_SIZE = 32767;	// 32k to start with
unsigned long tmp_max_used = 0;			// Tracks maximum tmp str space used
unsigned long tmp_overruns = 0;			// Tracks number of times the initial
										// pool has been insufficient

static tmp_str_pool *tmp_list_head;	// Always points to the initial pool
static tmp_str_pool *tmp_list_tail;	// Points to the end of the linked	
									// list of pools

// Initializes the structures used for the temporary string mechanism
void
tmp_string_init(void)
{
	tmp_list_head = (struct tmp_str_pool *)malloc(sizeof(struct tmp_str_pool) + INITIAL_POOL_SIZE);
	tmp_list_tail = tmp_list_head;
	tmp_list_head->next = NULL;
	tmp_list_head->space = INITIAL_POOL_SIZE;
	tmp_list_head->used = 0;
}

// tmp_gc_strings will deallocate every temporary string, adjust the
// size of the string pool.  All previously allocated strings will be
// invalid after this is called.
void
tmp_gc_strings(void)
{
	struct tmp_str_pool *cur_buf, *next_buf;
	size_t wanted = tmp_list_head->used;

	// If we needed more than the initial pool, count it as an overrun
	if (tmp_list_head->next)
		tmp_overruns++;

	// Free the extra space (if any), adding up the amount we needed
	for(cur_buf = tmp_list_head->next;cur_buf;cur_buf = next_buf) {
		next_buf = cur_buf->next;

		wanted += cur_buf->used;
		free(cur_buf);
	}

	// Track the max used
	if (wanted > tmp_max_used)
		tmp_max_used = wanted;

	// We don't deallocate the initial pool here.  We can just set its
	// 'used' to 0
	tmp_list_head->next = NULL;
	tmp_list_head->used = 0;
	tmp_list_tail = tmp_list_head;
}

// Allocate a new string pool
struct tmp_str_pool *
tmp_alloc_pool(int size)
{
	struct tmp_str_pool *new_buf;

	new_buf = (struct tmp_str_pool *)malloc(sizeof(struct tmp_str_pool) + size);
	new_buf->next = NULL;
	tmp_list_tail->next = new_buf;
	tmp_list_tail = new_buf;
	new_buf->space = size;
	new_buf->used = 0;

	return new_buf;
}

// vsprintf into a temp str
char *
tmp_vsprintf(const char *fmt, va_list args)
{
	struct tmp_str_pool *cur_buf = tmp_list_head;
	size_t wanted;
	char *result;

	cur_buf = tmp_list_head;

	result = &cur_buf->data[cur_buf->used];
	wanted = vsnprintf(result, cur_buf->space - cur_buf->used, fmt, args) + 1;

	// If there was enough space, our work is done here.  If there wasn't enough
	// space, we allocate another pool, and write into that.  The newer
	// pool is, of course, always big enough.

	if (cur_buf->space - cur_buf->used < wanted) {
		cur_buf = tmp_alloc_pool(wanted);
		result = &cur_buf->data[0];
		vsnprintf(result, cur_buf->space - cur_buf->used, fmt, args);
	} else
		cur_buf->used += wanted;
	
	return result;
}

// sprintf into a temp str
char *
tmp_sprintf(const char *fmt, ...)
{
	char *result;
	va_list args;

	va_start(args, fmt);
	result = tmp_vsprintf(fmt, args);
	va_end(args);

	return result;
}

// strcat into a temp str.  You must terminate the arguments with a NULL,
// since the va_arg method is too stupid to give us the number of arguments.
char *
tmp_strcat(char *src, ...)
{
	struct tmp_str_pool *cur_buf;
	char *write_pt, *read_pt, *result;
	size_t len;
	va_list args;

	// Figure out how much space we'll need
	len = strlen(src);

	va_start(args, src);
	while ((read_pt = va_arg(args, char *)) != NULL)
		len += strlen(read_pt);
	va_end(args);

	len += 1;


	// If we don't have the space, we allocate another pool
	if (len > tmp_list_head->space - tmp_list_head->used)
		cur_buf = tmp_alloc_pool(len);
	else
		cur_buf = tmp_list_head;

	result = cur_buf->data + cur_buf->used;
	write_pt = result;
	cur_buf->used += len;

	// Copy in the first string
	strcpy(write_pt, src);
	while (*write_pt)
		write_pt++;

	// Then copy in the rest of the strings
	va_start(args, src);
	while ((read_pt = va_arg(args, char *)) != NULL) {
		strcpy(write_pt, read_pt);
		while (*write_pt)
			write_pt++;
	}
	va_end(args);
		
	return result;
}

// get the next word, copied into a temp pool
char *
tmp_getword(char **src)
{
	struct tmp_str_pool *cur_buf;
	char *result, *read_pt, *write_pt;
	size_t len = 0;

	skip_spaces(src);

	read_pt = *src;
	while (*read_pt && !isspace(*read_pt))
		read_pt++;
	len = (read_pt - *src) + 1;

	if (len > tmp_list_head->space - tmp_list_head->used)
		cur_buf = tmp_alloc_pool(len);
	else
		cur_buf = tmp_list_head;

	result = tmp_list_head->data + tmp_list_head->used;
	read_pt = *src;
	write_pt = result;

	while (*read_pt && !isspace(*read_pt))
		*write_pt++ = tolower(*read_pt++);
	*write_pt = '\0';

	cur_buf->used += len;
	*src = read_pt;
	return result;
}
