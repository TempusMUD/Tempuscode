#include <unistd.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "interpreter.h"

struct tmp_str_buffer {
	struct tmp_str_buffer *next;
	size_t space;
	size_t used;
	char data[0];
};

const size_t INITIAL_POOL_SIZE = 32767;	// 32k to start with
unsigned long tmp_max_used = 0;
unsigned long tmp_overruns = 0;

static tmp_str_buffer *tmp_list_head;
static tmp_str_buffer *tmp_list_tail;

void
tmp_string_init(void)
{
	tmp_list_head = (struct tmp_str_buffer *)malloc(sizeof(struct tmp_str_buffer) + INITIAL_POOL_SIZE);
	tmp_list_tail = tmp_list_head;
	tmp_list_head->next = NULL;
	tmp_list_head->space = INITIAL_POOL_SIZE;
	tmp_list_head->used = 0;
}

// tmp_gc_strings will deallocate every temporary string, adjust the
// size of the string pool
void
tmp_gc_strings(void)
{
	struct tmp_str_buffer *cur_buf, *next_buf;
	size_t wanted = tmp_list_head->used;

	if (tmp_list_head->next)
		tmp_overruns++;

	for(cur_buf = tmp_list_head->next;cur_buf;cur_buf = next_buf) {
		next_buf = cur_buf->next;

		wanted += cur_buf->used;
		free(cur_buf);
	}

	if (wanted > tmp_max_used)
		tmp_max_used = wanted;

	tmp_list_head->next = NULL;
	tmp_list_head->used = 0;
}

struct tmp_str_buffer *
tmp_alloc_buf(int size)
{
	struct tmp_str_buffer *new_buf;

	new_buf = (struct tmp_str_buffer *)malloc(sizeof(struct tmp_str_buffer) + size);
	new_buf->next = NULL;
	tmp_list_tail->next = new_buf;
	tmp_list_tail = new_buf;
	new_buf->space = size;
	new_buf->used = 0;

	return new_buf;
}

// sprintf into a temp str
char *
tmp_vsprintf(const char *fmt, va_list args)
{
	struct tmp_str_buffer *cur_buf = tmp_list_head;
	size_t wanted;
	char *result;

	cur_buf = tmp_list_head;

	result = &cur_buf->data[cur_buf->used];
	wanted = vsnprintf(result, cur_buf->space - cur_buf->used, fmt, args) + 1;
	if (cur_buf->space - cur_buf->used < wanted) {
		cur_buf = tmp_alloc_buf(wanted);
		result = &cur_buf->data[0];
		vsnprintf(result, cur_buf->space - cur_buf->used, fmt, args);
	} else
		cur_buf->used += wanted;
	
	return result;
}

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

// strcat into a temp str
char *
tmp_strcat(char *src, ...)
{
	struct tmp_str_buffer *cur_buf;
	char *write_pt, *read_pt, *result;
	size_t len;
	va_list args;

	len = strlen(src);

	va_start(args, src);
	while ((read_pt = va_arg(args, char *)) != NULL)
		len += strlen(read_pt);
	va_end(args);

	len += 1;

	if (len > tmp_list_head->space - tmp_list_head->used)
		cur_buf = tmp_alloc_buf(len);
	else
		cur_buf = tmp_list_head;

	result = cur_buf->data + cur_buf->used;
	write_pt = result;
	cur_buf->used += len;

	strcpy(write_pt, src);
	while (*write_pt)
		write_pt++;

	va_start(args, src);
	while ((read_pt = va_arg(args, char *)) != NULL) {
		strcpy(write_pt, read_pt);
		while (*write_pt)
			write_pt++;
	}
	va_end(args);
		
	return result;
}

// get a word, placed into a temp str
char *
tmp_getword(char **src)
{
	struct tmp_str_buffer *cur_buf;
	char *result, *read_pt, *write_pt;
	size_t len = 0;

	skip_spaces(src);

	read_pt = *src;
	while (*read_pt && !isspace(*read_pt))
		read_pt++;
	len = (read_pt - *src) + 1;

	if (len > tmp_list_head->space - tmp_list_head->used)
		cur_buf = tmp_alloc_buf(len);
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
