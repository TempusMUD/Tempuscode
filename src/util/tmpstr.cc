#include <unistd.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "interpreter.h"
#include "utils.h"
#include "db.h"

struct tmp_str_pool {
	struct tmp_str_pool *next;		// Ptr to next in linked list
	size_t space;					// Amount of space allocated
	size_t used;					// Amount of space used in pool
	unsigned long underflow;		// Buffer underflow detection
	char data[0];					// The actual data
};

const size_t DEFAULT_POOL_SIZE = 65536;	// 64k to start with
size_t tmp_max_used = 0;				// Tracks maximum tmp str space used

static tmp_str_pool *tmp_list_head;	// Always points to the initial pool
static tmp_str_pool *tmp_list_tail;	// Points to the end of the linked	
									// list of pools

struct tmp_str_pool *tmp_alloc_pool(size_t size_req);
// Initializes the structures used for the temporary string mechanism
void
tmp_string_init(void)
{
	tmp_list_head = tmp_alloc_pool(DEFAULT_POOL_SIZE);
}

// tmp_gc_strings will deallocate every temporary string, adjust the
// size of the string pool.  All previously allocated strings will be
// invalid after this is called.
void
tmp_gc_strings(void)
{
	struct tmp_str_pool *cur_buf, *next_buf;
	size_t wanted = tmp_list_head->used;

	// Free the extra space (if any), adding up the amount we needed
	for(cur_buf = tmp_list_head->next;cur_buf;cur_buf = next_buf) {
		next_buf = cur_buf->next;

		if (cur_buf->underflow != 0xddccbbaa)
			slog("WARNING: buffer underflow detected in tmpstr module");
		if (*((unsigned long *)&cur_buf->data[cur_buf->space]) != 0xaabbccdd)
			slog("WARNING: buffer overflow detected in tmpstr module");

		wanted += cur_buf->used;
		free(cur_buf);
	}

	// Track the max used and resize the pool if necessary
	if (wanted > tmp_max_used) {
		tmp_max_used = wanted;
		
		if (tmp_max_used > tmp_list_head->space) {
			free(tmp_list_head);
			tmp_list_tail = NULL;
			tmp_list_head = tmp_alloc_pool(tmp_max_used);
			slog("NOTICE: tmpstr pool allocated increased to %d bytes", 
				tmp_max_used);
		}
	}

	// We don't deallocate the initial pool here.  We can just set its
	// 'used' to 0
	tmp_list_head->next = NULL;
	tmp_list_head->used = 0;
	tmp_list_tail = tmp_list_head;
}

// Allocate a new string pool
struct tmp_str_pool *
tmp_alloc_pool(size_t size_req)
{
	struct tmp_str_pool *new_buf;
	size_t size = MAX(size_req,DEFAULT_POOL_SIZE);

	new_buf = (struct tmp_str_pool *)malloc(sizeof(struct tmp_str_pool) + size + 4);
	new_buf->next = NULL;
	if (tmp_list_tail)
		tmp_list_tail->next = new_buf;
	tmp_list_tail = new_buf;
	new_buf->space = size;
	new_buf->used = 0;

	// Add buffer overflow detection
	new_buf->underflow = 0xddccbbaa;
	*((unsigned long *)&new_buf->data[new_buf->space]) = 0xaabbccdd;

	return new_buf;
}

// vsprintf into a temp str
char *
tmp_vsprintf(const char *fmt, va_list args)
{
	struct tmp_str_pool *cur_buf;
	size_t wanted;
	char *result;

	cur_buf = tmp_list_tail;

	result = &cur_buf->data[cur_buf->used];
	wanted = vsnprintf(result, cur_buf->space - cur_buf->used, fmt, args) + 1;

	// If there was enough space, our work is done here.  If there wasn't enough
	// space, we allocate another pool, and write into that.  The newer
	// pool is, of course, always big enough.

	if (cur_buf->space - cur_buf->used < wanted) {
		cur_buf = tmp_alloc_pool(wanted);
		result = &cur_buf->data[0];
		wanted = vsnprintf(result, cur_buf->space - cur_buf->used, fmt, args) + 1;
	}

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
tmp_strcat(const char *src, ...)
{
	struct tmp_str_pool *cur_buf;
	char *write_pt, *result;
	const char *read_pt;
	size_t len;
	va_list args;

	// Figure out how much space we'll need
	len = strlen(src);

	va_start(args, src);
	while ((read_pt = va_arg(args, const char *)) != NULL)
		len += strlen(read_pt);
	va_end(args);

	len += 1;


	// If we don't have the space, we allocate another pool
	if (len > tmp_list_tail->space - tmp_list_tail->used)
		cur_buf = tmp_alloc_pool(len);
	else
		cur_buf = tmp_list_tail;

	result = cur_buf->data + cur_buf->used;
	write_pt = result;
	cur_buf->used += len;

	// Copy in the first string
	strcpy(write_pt, src);
	while (*write_pt)
		write_pt++;

	// Then copy in the rest of the strings
	va_start(args, src);
	while ((read_pt = va_arg(args, const char *)) != NULL) {
		strcpy(write_pt, read_pt);
		while (*write_pt)
			write_pt++;
	}
	va_end(args);
		
	return result;
}

// get the next word, copied into a temp pool
char *
tmp_getword(const char **src)
{
	struct tmp_str_pool *cur_buf;
	const char *read_pt;
	char *result, *write_pt;
	size_t len = 0;

	skip_spaces(src);

	read_pt = *src;
	while (*read_pt && !isspace(*read_pt))
		read_pt++;
	len = (read_pt - *src) + 1;

	if (len > tmp_list_tail->space - tmp_list_tail->used)
		cur_buf = tmp_alloc_pool(len);
	else
		cur_buf = tmp_list_tail;

	result = cur_buf->data + cur_buf->used;
	read_pt = *src;
	write_pt = result;

	while (*read_pt && !isspace(*read_pt))
		*write_pt++ = tolower(*read_pt++);
	*write_pt = '\0';

	cur_buf->used += len;
	*src = read_pt;

	skip_spaces(src);
	return result;
}

char *
tmp_getquoted(const char **src)
{
	struct tmp_str_pool *cur_buf;
	const char *read_pt;
	char *result, *write_pt;
	size_t len = 0;
	int delim;

	skip_spaces(src);

	delim = **src;

	// No quote mark at the beginning
	if (delim != '\'' && delim != '"')
		return tmp_getword(src);

	read_pt = *src + 1;
	*src = read_pt;

	while (*read_pt && delim != *read_pt)
		read_pt++;

	// No terminating quote, so we act just like tmp_getword
	if (delim != *read_pt)
		return tmp_getword(src);

	len = (read_pt - *src) + 1;

	if (len > tmp_list_tail->space - tmp_list_tail->used)
		cur_buf = tmp_alloc_pool(len);
	else
		cur_buf = tmp_list_tail;

	result = cur_buf->data + cur_buf->used;
	read_pt = *src;
	write_pt = result;

	while (*read_pt && delim != *read_pt)
		*write_pt++ = tolower(*read_pt++);
	*write_pt = '\0';

	cur_buf->used += len;
	*src = read_pt + 1;
	return result;
}

char *
tmp_pad(int c, size_t len)
{
	struct tmp_str_pool *cur_buf;
	char *result;

	if (len + 1 > tmp_list_tail->space - tmp_list_tail->used)
		cur_buf = tmp_alloc_pool(len + 1);
	else
		cur_buf = tmp_list_tail;

	result = cur_buf->data + cur_buf->used;
	cur_buf->used += len + 1;
	if (len)
		memset(result, c, len);
	result[len] = '\0';

	return result;
}

// get the next line, copied into a temp pool
char *
tmp_getline(const char **src)
{
	struct tmp_str_pool *cur_buf;
	const char *read_pt;
	char *result, *write_pt;
	size_t len = 0;

	read_pt = *src;
	while (*read_pt && '\r' != *read_pt && '\n' != *read_pt)
		read_pt++;
	len = (read_pt - *src) + 1;

	if (len == 1 && !*read_pt)
		return NULL;

	if (len > tmp_list_tail->space - tmp_list_tail->used)
		cur_buf = tmp_alloc_pool(len);
	else
		cur_buf = tmp_list_tail;

	result = cur_buf->data + cur_buf->used;
	read_pt = *src;
	write_pt = result;

	while (*read_pt && '\r' != *read_pt && '\n' != *read_pt)
		*write_pt++ = *read_pt++;
	*write_pt = '\0';

	cur_buf->used += len;
	if (*read_pt == '\r')
		read_pt++;
	if (*read_pt == '\n')
		read_pt++;
	*src = read_pt;

	return result;
}

char *
tmp_gsub(const char *haystack, const char *needle, const char *sub)
{
	struct tmp_str_pool *cur_buf;
	char *write_pt, *result;
	const char *read_pt, *search_pt;
	size_t len;
	int matches;

	// An empty string matches nothing
	if (!*needle)
		return tmp_strdup(haystack);

	// Count number of occurences of needle in haystack
	matches = 0;
	read_pt = haystack;
	while ((read_pt = strstr(read_pt, needle)) != NULL) {
		matches++;
		read_pt += strlen(needle);
	}

	// Figure out how much space we'll need
		len = strlen(haystack) + matches * (strlen(sub) - strlen(needle)) + 1;

	// If we don't have the space, we allocate another pool
	if (len > tmp_list_tail->space - tmp_list_tail->used)
		cur_buf = tmp_alloc_pool(len);
	else
		cur_buf = tmp_list_tail;

	result = cur_buf->data + cur_buf->used;
	cur_buf->used += len;

	// Now copy up to matches
	read_pt = haystack;
	write_pt = result;
	while ((search_pt = strstr(read_pt, needle)) != NULL) {
		while (read_pt != search_pt)
			*write_pt++ = *read_pt++;
		read_pt = sub;
		while (*read_pt)
			*write_pt++ = *read_pt++;
		search_pt += strlen(needle);
		read_pt = search_pt;
	}

	// Copy the rest of the string
	strcpy(write_pt, read_pt);
	return result;
}

char *
tmp_gsubi(const char *haystack, const char *needle, const char *sub)
{
	struct tmp_str_pool *cur_buf;
	char *low_stack;
	char *write_pt, *result;
	const char *read_pt, *search_pt;
	size_t len;
	int matches;

	// Count number of occurences of needle in haystack
	matches = 0;
	low_stack = tmp_tolower(haystack);
	needle = tmp_tolower(needle);
	read_pt = low_stack;
	while ((read_pt = strstr(read_pt, needle)) != NULL) {
		matches++;
		read_pt += strlen(needle);
	}

	// Figure out how much space we'll need
	len = strlen(haystack) + matches * (strlen(sub) - strlen(needle)) + 1;

	// If we don't have the space, we allocate another pool
	if (len > tmp_list_tail->space - tmp_list_tail->used)
		cur_buf = tmp_alloc_pool(len);
	else
		cur_buf = tmp_list_tail;

	result = cur_buf->data + cur_buf->used;
	cur_buf->used += len;

	// Now copy up to matches
	read_pt = haystack;
	search_pt = low_stack;
	write_pt = result;
	while ((search_pt = strstr(search_pt, needle)) != NULL) {
		while (read_pt - haystack < search_pt - low_stack)
			*write_pt++ = *read_pt++;
		read_pt = sub;
		while (*read_pt)
			*write_pt++ = *read_pt++;
		search_pt += strlen(needle);
		read_pt = search_pt;
	}

	// Copy the rest of the string
	strcpy(write_pt, read_pt);
	return result;
}


char *
tmp_tolower(const char *str)
{
	char *result, *c;

	c = result = tmp_strcat(str, NULL);
	while (*c)
		*c++ = tolower(*c);
	return result;
}

char *
tmp_capitalize(const char *str)
{
	char *result;

	result = tmp_strcat(str, NULL);
	if (*result)
		*result = toupper(*result);
	return result;
}

char *
tmp_strdup(const char *src, const char *term_str)
{
	struct tmp_str_pool *cur_buf;
	char *write_pt, *result;
	const char *read_pt;
	size_t len;

	// Figure out how much space we'll need
	read_pt = term_str ? strstr(src, term_str):NULL;
	if (read_pt)
		len = read_pt - src + 1;
	else
		len = strlen(src) + 1;

	// If we don't have the space, we allocate another pool
	if (len > tmp_list_tail->space - tmp_list_tail->used)
		cur_buf = tmp_alloc_pool(len);
	else
		cur_buf = tmp_list_tail;

	result = cur_buf->data + cur_buf->used;
	write_pt = result;
	cur_buf->used += len;

	// Copy in the first string
	read_pt = src;
	write_pt = result;
	while (--len)
		*write_pt++ = *read_pt++;
	*write_pt = '\0';

	return result;
}

char *
tmp_sqlescape(const char *str)
{
	struct tmp_str_pool *cur_buf;
	char *result;
	size_t len;
	
	len = strlen(str) * 2 + 1;
	if (len + 1 > tmp_list_tail->space - tmp_list_tail->used)
		cur_buf = tmp_alloc_pool(len + 1);
	else
		cur_buf = tmp_list_tail;

	result = cur_buf->data + cur_buf->used;
	len = PQescapeString(result, str, len);
	cur_buf->used += len + 1;

	return result;
}

char *
tmp_ctime(time_t val)
{
	struct tmp_str_pool *cur_buf;
	char *result;
	size_t len;
	
	len = 26;
	if (len + 1 > tmp_list_tail->space - tmp_list_tail->used)
		cur_buf = tmp_alloc_pool(len + 1);
	else
		cur_buf = tmp_list_tail;

	result = cur_buf->data + cur_buf->used;
	ctime_r(&val, result);
	cur_buf->used += strlen(result) + 1;
	// last byte is, sadly, a newline.  we remove it here
	result[strlen(result) - 1] = '\0';

	return result;
}

char *
tmp_printbits(int val, const char *bit_descs[])
{
	struct tmp_str_pool *cur_buf;
	char *write_pt, *result;
	unsigned int idx;
	bool not_first = false;
	size_t len;

	// Figure out how much space we'll need
	len = 0;
	for (idx = 0;idx < sizeof(val) * 8;idx++)
		if ((val >> idx) & 1)
			len += strlen(bit_descs[idx]) + 1;
	len += 1;


	// If we don't have the space, we allocate another pool
	if (len > tmp_list_tail->space - tmp_list_tail->used)
		cur_buf = tmp_alloc_pool(len);
	else
		cur_buf = tmp_list_tail;

	result = cur_buf->data + cur_buf->used;
	cur_buf->used += len;
	write_pt = result;

	// Copy in the first string
	*write_pt = '\0';

	// Then copy in the rest of the strings
	for (idx = 0;idx < sizeof(val) * 8;idx++)
		if ((val >> idx) & 1) {
			if (not_first)
				*write_pt++ = ' ';
			else
				not_first = true;

			strcpy(write_pt, bit_descs[idx]);
			while (*write_pt)
				write_pt++;
		}

	*write_pt = '\0';

	return result;		
}

char *
tmp_substr(const char *str, int start_pos, int end_pos)
{
	struct tmp_str_pool *cur_buf;
	const char *read_pt;
	char *result, *write_pt;
	int len;
	size_t result_len;

	len = strlen(str);
	// Check bounds for end_pos
	if (end_pos < 0) {
		end_pos = len + end_pos;
		if (end_pos < 0)
			end_pos = 0;
	}
	if (end_pos > len)
		end_pos = len;

	// Check bounds for start_pos
	if (start_pos < 0) {
		start_pos = len + start_pos;
		if (start_pos < 0)
			start_pos = 0;
	}
	if (start_pos > len)
		start_pos = len;
	
	// This is the true result string length
	result_len = end_pos - start_pos + 2;

	// If we don't have the space, we allocate another pool
	if (result_len > tmp_list_tail->space - tmp_list_tail->used)
		cur_buf = tmp_alloc_pool(result_len);
	else
		cur_buf = tmp_list_tail;

	result = cur_buf->data + cur_buf->used;
	cur_buf->used += result_len;
	write_pt = result;
	
	read_pt = str + start_pos;
	result_len--;
	while (result_len--)
		*write_pt++ = *read_pt++;	
	*write_pt = '\0';

	return result;
}

const char *
tmp_string_test(void)
{
	const char *bit_descs[] = {
		"BIT_00", "BIT_01", "BIT_02", "BIT_03", "BIT_04",
		"BIT_05", "BIT_06", "BIT_07", "BIT_08", "BIT_09",
		"BIT_10", "BIT_11", "BIT_12", "BIT_13", "BIT_14",
		"BIT_15", "BIT_16", "BIT_17", "BIT_18", "BIT_19",
		"BIT_20", "BIT_21", "BIT_22", "BIT_23", "BIT_24",
		"BIT_25", "BIT_26", "BIT_27", "BIT_28", "BIT_29",
		"BIT_30", "BIT_31", "BIT_32", "\n"
	};

	// Testing tmp_tolower
	if (strcmp(tmp_tolower("abcdef"), "abcdef"))
		return "tmp_tolower #1 failed";
	if (strcmp(tmp_tolower("ABCDEF"), "abcdef"))
		return "tmp_tolower #2 failed";
	if (strcmp(tmp_tolower("AbCdEf"), "abcdef"))
		return "tmp_tolower #3 failed";
	if (strcmp(tmp_tolower("aBcDeF"), "abcdef"))
		return "tmp_tolower #4 failed";
	if (strcmp(tmp_tolower("123456"), "123456"))
		return "tmp_tolower #5 failed";
	if (strcmp(tmp_tolower(""), ""))
		return "tmp_tolower #6 failed";

	// Testing tmp_gsub
	if (strcmp(tmp_gsub("abcdef", "", ""), "abcdef"))
		return "tmp_gsub #1 failed";
	if (strcmp(tmp_gsub("abcdef", "ghi", ""), "abcdef"))
		return "tmp_gsub #2 failed";
	if (strcmp(tmp_gsub("abcdef", "c", "ghi"), "abghidef"))
		return "tmp_gsub #3 failed";
	if (strcmp(tmp_gsub("abcdef", "cde", "g"), "abgf"))
		return "tmp_gsub #4 failed";
	if (strcmp(tmp_gsub("abcdef", "abc", ""), "def"))
		return "tmp_gsub #5 failed";
	if (strcmp(tmp_gsub("abcdef", "abc", "g"), "gdef"))
		return "tmp_gsub #6 failed";
	if (strcmp(tmp_gsub("abcdef", "def", ""), "abc"))
		return "tmp_gsub #7 failed";
	if (strcmp(tmp_gsub("abcdef", "def", "g"), "abcg"))
		return "tmp_gsub #8 failed";
	if (strcmp(tmp_gsub("", "", ""), ""))
		return "tmp_gsub #9 failed";

	// Testing tmp_sqlescape
	if (strcmp(tmp_sqlescape("abcd'ef"), "abcd''ef"))
		return "tmp_sqlescape #1 failed";
	if (strcmp(tmp_sqlescape("abcd\\'ef"), "abcd\\\\''ef"))
		return "tmp_sqlescape #2 failed";
	if (strcmp(tmp_sqlescape("abcd\\ef"), "abcd\\\\ef"))
		return "tmp_sqlescape #3 failed";

	// Testing tmp_printbits
	if (strcmp(tmp_printbits(0x1, bit_descs), "BIT_00"))
		return "tmp_printbits #1 failed";
	if (strcmp(tmp_printbits(0x2, bit_descs), "BIT_01"))
		return "tmp_printbits #2 failed";
	if (strcmp(tmp_printbits(0x3, bit_descs), "BIT_00 BIT_01"))
		return "tmp_printbits #3 failed";
	if (strcmp(tmp_printbits(0x4, bit_descs), "BIT_02"))
		return "tmp_printbits #4 failed";
	if (strcmp(tmp_printbits(0x5, bit_descs), "BIT_00 BIT_02"))
		return "tmp_printbits #5 failed";
	if (strcmp(tmp_printbits(0x6, bit_descs), "BIT_01 BIT_02"))
		return "tmp_printbits #6 failed";
	if (strcmp(tmp_printbits(0x7, bit_descs), "BIT_00 BIT_01 BIT_02"))
		return "tmp_printbits #7 failed";

	// Testing tmp_substr
	if (strcmp(tmp_substr("abcdef", 0, -1), "abcdef"))
		return "tmp_substr #1 failed";
	if (strcmp(tmp_substr("abcdef", 0, 2), "abc"))
		return "tmp_substr #2 failed";
	if (strcmp(tmp_substr("abcdef", 1, 4), "bcde"))
		return "tmp_substr #3 failed";
	if (strcmp(tmp_substr("abcdef", 3, -1), "def"))
		return "tmp_substr #4 failed";
	if (strcmp(tmp_substr("abcdef", -1, -1), "f"))
		return "tmp_substr #5 failed";
	if (strcmp(tmp_substr("abcdef", 0, 0), "a"))
		return "tmp_substr #6 failed";
	if (strcmp(tmp_substr("abcdef", -5, -1), "bcdef"))
		return "tmp_substr #7 failed";

	return NULL;
}
