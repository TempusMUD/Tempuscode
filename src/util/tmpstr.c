#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <libpq-fe.h>
#include <glib.h>

#include "interpreter.h"
#include "tmpstr.h"
#include "utils.h"
#include "strutil.h"

struct tmp_str_pool {
    struct tmp_str_pool *next;  // Ptr to next in linked list
    size_t space;               // Amount of space allocated
    size_t used;                // Amount of space used in pool
    char data[0];               // The actual data
};

const size_t DEFAULT_POOL_SIZE = 65536; // 64k to start with
size_t tmp_max_used = 0;        // Tracks maximum tmp str space used

// list of pools
static struct tmp_str_pool *tmp_list_head = NULL;  // Always points to the initial pool

struct tmp_str_pool *tmp_alloc_pool(size_t size_req);

// Initializes the structures used for the temporary string mechanism
void
tmp_string_init(void)
{
    if (tmp_list_head != NULL) {
        errlog("Temp strings already initialized!");
    }
    tmp_list_head = tmp_alloc_pool(DEFAULT_POOL_SIZE);
}

// Frees all memory used by the tmpstr pool.  This is mostly used in
// testing to prevent memory leak false positives.
void
tmp_string_cleanup(void)
{
    while (tmp_list_head) {
        struct tmp_str_pool *next_buf = tmp_list_head->next;
        free(tmp_list_head);
        tmp_list_head = next_buf;
    }
}

// tmp_gc_strings will deallocate every temporary string, adjust the
// size of the string pool.  All previously allocated strings will be
// invalid after this is called.
void
tmp_gc_strings(void)
{
    size_t wanted = 0;

    // Free the extra space (if any), adding up the amount we needed
    while (tmp_list_head->next) {
        struct tmp_str_pool *next_buf = tmp_list_head->next;
        wanted += tmp_list_head->used;
        free(tmp_list_head);
        tmp_list_head = next_buf;
    }
    wanted += tmp_list_head->used;

    // Track the max used and resize the pool if necessary
    if (wanted > tmp_max_used) {
        tmp_max_used = wanted;

        if (tmp_max_used > tmp_list_head->space) {
            free(tmp_list_head);
            tmp_list_head = NULL;
            tmp_list_head = tmp_alloc_pool(tmp_max_used);
        }
    }
    // We don't deallocate the initial pool here.  We can just set its
    // 'used' to 0
    tmp_list_head->used = 0;
}

// Allocate a new string pool
struct tmp_str_pool *
tmp_alloc_pool(size_t size_req)
{
    struct tmp_str_pool *new_buf;
    size_t size = MAX(size_req, DEFAULT_POOL_SIZE);

    new_buf =
        (struct tmp_str_pool *)malloc(sizeof(struct tmp_str_pool) + size);
    new_buf->next = tmp_list_head;
    tmp_list_head = new_buf;
    new_buf->space = size;
    new_buf->used = 0;

    memset(new_buf->data, 0, size);

    return new_buf;
}

// Allocate space for a string, creating a new pool if necessary
char *
tmp_alloc(size_t size_req)
{
    struct tmp_str_pool *cur_buf = tmp_list_head;

    if (size_req > cur_buf->space - cur_buf->used) {
        cur_buf = tmp_alloc_pool(size_req);
    }

    char *result = cur_buf->data + cur_buf->used;

    cur_buf->used += size_req;

    return result;
}

// vsprintf into a temp str
char *
tmp_vsprintf(const char *fmt, va_list args)
{
    struct tmp_str_pool *cur_buf;
    size_t wanted;
    char *result;
    va_list args_copy;

    va_copy(args_copy, args);

    cur_buf = tmp_list_head;

    result = &cur_buf->data[cur_buf->used];
    wanted = vsnprintf(result, cur_buf->space - cur_buf->used, fmt, args) + 1;

    // If there was enough space, our work is done here.  If there wasn't enough
    // space, we allocate another pool, and write into that.  The newer
    // pool is, of course, always big enough.

    if (cur_buf->space - cur_buf->used < wanted) {
        cur_buf = tmp_alloc_pool(wanted);
        result = &cur_buf->data[0];
        wanted = vsnprintf(result, cur_buf->space, fmt, args_copy) + 1;
    }

    cur_buf->used += wanted;

    va_end(args_copy);

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
    char *write_pt, *result;
    const char *read_pt;
    size_t len;
    va_list args;

    // Figure out how much space we'll need
    len = strlen(src);

    va_start(args, src);
    while ((read_pt = va_arg(args, const char *)) != NULL) {
        len += strlen(read_pt);
    }
    va_end(args);

    len += 1;

    write_pt = result = tmp_alloc(len);

    // Copy in the first string
    strcpy(write_pt, src);
    while (*write_pt) {
        write_pt++;
    }

    // Then copy in the rest of the strings
    va_start(args, src);
    while ((read_pt = va_arg(args, const char *)) != NULL) {
        strcpy(write_pt, read_pt);
        while (*write_pt) {
            write_pt++;
        }
    }
    va_end(args);

    *write_pt = '\0';

    return result;
}

// get the next token, copied into a temp pool
char *
tmp_gettoken(char **src)
{
    char *read_pt;
    char *result, *write_pt;
    size_t len = 0;

    skip_spaces(src);

    read_pt = *src;
    while (*read_pt && !isspace(*read_pt)) {
        read_pt++;
    }
    len = (read_pt - *src) + 1;

    write_pt = result = tmp_alloc(len);
    read_pt = *src;

    while (*read_pt && !isspace(*read_pt)) {
        *write_pt++ = *read_pt++;
    }

    *write_pt = '\0';

    *src = read_pt;

    skip_spaces(src);
    return result;
}

char *
tmp_gettoken_const(const char **src)
{
    char *copy = tmp_strdup(*src);
    char *s = copy;
    char *result = tmp_gettoken(&copy);

    *src += copy - s;
    return result;
}

// like tmp_gettoken, but downcases the string before returning it
char *
tmp_getword(char **src)
{
    char *result = tmp_gettoken(src);
    char *c;

    for (c = result; *c; c++) {
        *c = tolower(*c);
    }
    return result;
}

char *
tmp_getword_const(const char **src)
{
    char *copy = tmp_strdup(*src);
    char *s = copy;
    char *result = tmp_getword(&copy);

    *src += copy - s;
    return result;
}

char *
tmp_getquoted(char **src)
{
    char *read_pt;
    char *result, *write_pt;
    size_t len = 0;
    int delim;

    skip_spaces(src);

    delim = **src;

    // No quote mark at the beginning
    if (delim != '\'' && delim != '"') {
        return tmp_getword(src);
    }

    read_pt = *src + 1;
    *src = read_pt;

    while (*read_pt && delim != *read_pt) {
        read_pt++;
    }

    // No terminating quote, so we act just like tmp_getword
    if (delim != *read_pt) {
        return tmp_getword(src);
    }

    len = (read_pt - *src) + 1;

    write_pt = result = tmp_alloc(len);
    read_pt = *src;

    while (*read_pt && delim != *read_pt) {
        *write_pt++ = tolower(*read_pt++);
    }

    *write_pt = '\0';

    *src = read_pt + 1;
    return result;
}

char *
tmp_pad(int c, size_t len)
{
    char *result;

    result = tmp_alloc(len + 1);
    if (len) {
        memset(result, c, len);
    }

    result[len] = '\0';

    return result;
}

// get the next line, copied into a temp pool
char *
tmp_getline(char **src)
{
    char *read_pt;
    char *result, *write_pt;
    size_t len = 0;

    if (!*src) {
        return NULL;
    }

    read_pt = *src;
    while (*read_pt && '\r' != *read_pt && '\n' != *read_pt) {
        read_pt++;
    }
    len = (read_pt - *src) + 1;

    if (len == 1 && !*read_pt) {
        return NULL;
    }

    write_pt = result = tmp_alloc(len);
    read_pt = *src;

    while (*read_pt && '\r' != *read_pt && '\n' != *read_pt) {
        *write_pt++ = *read_pt++;
    }

    *write_pt = '\0';

    if (*read_pt == '\r') {
        read_pt++;
    }
    if (*read_pt == '\n') {
        read_pt++;
    }
    *src = read_pt;

    return result;
}

char *
tmp_getline_const(const char **src)
{
    char *copy = tmp_strdup(*src);
    char *s = copy;
    char *result = tmp_getline(&copy);

    *src += copy - s;
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
    if (!*needle) {
        return tmp_strdup(haystack);
    }

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
    if (len > tmp_list_head->space - tmp_list_head->used) {
        cur_buf = tmp_alloc_pool(len);
    } else {
        cur_buf = tmp_list_head;
    }

    result = cur_buf->data + cur_buf->used;
    cur_buf->used += len;

    // Now copy up to matches
    read_pt = haystack;
    write_pt = result;
    while ((search_pt = strstr(read_pt, needle)) != NULL) {
        while (read_pt != search_pt) {
            *write_pt++ = *read_pt++;
        }
        read_pt = sub;
        while (*read_pt) {
            *write_pt++ = *read_pt++;
        }
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
    char *low_stack;
    char *write_pt, *result;
    const char *read_pt, *search_pt;
    size_t len, needle_len;
    int matches;

    // Head off an infinite loop at the pass
    if (!*needle) {
        return tmp_strdup(haystack);
    }

    // Count number of occurences of needle in haystack
    matches = 0;
    low_stack = tmp_tolower(haystack);
    needle = tmp_tolower(needle);
    needle_len = strlen(needle);
    read_pt = low_stack;
    while ((read_pt = strstr(read_pt, needle)) != NULL) {
        matches++;
        read_pt += needle_len;
    }

    // Figure out how much space we'll need
    len = strlen(haystack) + matches * (strlen(sub) - needle_len) + 1;

    result = tmp_alloc(len);

    // Now copy up to matches
    read_pt = haystack;
    search_pt = low_stack;
    write_pt = result;
    while ((search_pt = strstr(search_pt, needle)) != NULL) {
        while (read_pt - haystack < search_pt - low_stack) {
            *write_pt++ = *read_pt++;
        }
        read_pt = sub;
        while (*read_pt) {
            *write_pt++ = *read_pt++;
        }
        search_pt += needle_len;
        read_pt = haystack + (search_pt - low_stack);
    }

    // Copy the rest of the string
    strcpy(write_pt, read_pt);
    return result;
}

char *
tmp_tolower(const char *str)
{
    char *result;

    result = tmp_strcat(str, NULL);
    for (char *c = result; *c; c++) {
        *c = tolower(*c);
    }

    return result;
}

char *
tmp_toupper(const char *str)
{
    char *result;

    result = tmp_strcat(str, NULL);
    for (char *c = result; *c; c++) {
        *c = toupper(*c);
    }
    return result;
}

char *
tmp_capitalize(const char *str)
{
    char *result;

    result = tmp_strcat(str, NULL);
    if (*result) {
        *result = toupper(*result);
    }
    return result;
}

char *
tmp_strdup(const char *src)
{
    char *result = tmp_alloc(strlen(src) + 1);

    strcpy(result, src);

    return result;
}

char *
tmp_strdupn(const char *src, ssize_t n)
{
    char *result = tmp_alloc(n + 1);

    memcpy(result, src, n);
    result[n] = '\0';
    return result;
}

char *
tmp_strdupt(const char *src, const char *term_str)
{
    char *result;
    const char *read_pt;
    size_t len;

    // Figure out how much space we'll need
    read_pt = term_str ? strstr(src, term_str) : NULL;
    if (read_pt) {
        len = read_pt - src;
    } else {
        len = strlen(src);
    }

    result = tmp_alloc(len + 1);
    strncpy(result, src, len);
    result[len] = '\0';

    return result;
}

char *
tmp_sqlescape(const char *str)
{
    char *result;
    size_t len;

    len = strlen(str) * 2 + 1;
    result = tmp_alloc(len + 1);
    (void)PQescapeString(result, str, len);

    return result;
}

char *
tmp_ctime(time_t val)
{
    char *result;

    result = tmp_alloc(27);
    ctime_r(&val, result);
    // last byte is, sadly, a newline.  we remove it here
    result[strlen(result) - 1] = '\0';

    return result;
}

char *
tmp_strftime(const char *fmt, const struct tm *tm)
{
    char *result;
    size_t alloc_size = 16;

    // strftime() gives us nothing useful if the result string overruns the
    // maximum bytes, so we are forced to use trial-and-error.
    while (true) {
        result = tmp_alloc(alloc_size);
        size_t used = strftime(result, 32, fmt, tm);
        if (used != 0) {
            break;
        }
        alloc_size *= 2;
    }

    return result;
}

char *
tmp_printbits(int val, const char *bit_descs[])
{
    char *write_pt, *result;
    unsigned int idx;
    bool not_first = false;
    size_t len;

    // Figure out how much space we'll need
    len = 0;
    for (idx = 0;
         idx < sizeof(val) * 8 && bit_descs[idx] && bit_descs[idx][0] != '\n';
         idx++) {
        if ((val >> idx) & 1) {
            len += strlen(bit_descs[idx]) + 1;
        }
    }
    len += 1;

    write_pt = result = tmp_alloc(len);

    for (idx = 0;
         idx < sizeof(val) * 8 && bit_descs[idx] && bit_descs[idx][0] != '\n';
         idx++) {
        if ((val >> idx) & 1) {
            if (not_first) {
                *write_pt++ = ' ';
            } else {
                not_first = true;
            }

            strcpy(write_pt, bit_descs[idx]);
            while (*write_pt) {
                write_pt++;
            }
        }
    }

    *write_pt = '\0';

    return result;
}

char *
tmp_substr(const char *str, int start_pos, int end_pos)
{
    char *result;
    int len;
    size_t result_len;

    len = strlen(str);
    // Check bounds for end_pos
    if (end_pos < 0) {
        end_pos = len + end_pos;
        if (end_pos < 0) {
            end_pos = 0;
        }
    }
    if (end_pos > len) {
        end_pos = len;
    }

    // Check bounds for start_pos
    if (start_pos < 0) {
        start_pos = len + start_pos;
        if (start_pos < 0) {
            start_pos = 0;
        }
    }
    if (start_pos > len) {
        start_pos = len;
    }

    // This is the true result string length
    result_len = end_pos - start_pos + 1;

    result = tmp_alloc(result_len + 1);
    memmove(result, str + start_pos, result_len);
    result[result_len] = '\0';

    return result;
}

char *
tmp_trim(const char *str)
{
    const char *start, *end;
    char *result;
    size_t result_len;

    // Find beginning of trimmed string
    start = str;
    while (*start && isspace(*start)) {
        start++;
    }

    // Find end of trimmed string
    end = str + strlen(str) - 1;
    while (end > start && isspace(*end)) {
        end--;
    }

    // This is the true result string length
    result_len = end - start + 1;

    result = tmp_alloc(result_len + 1);
    memmove(result, start, result_len);
    result[result_len] = '\0';

    return result;
}

// Performs the formatting of a string, given a buffer.  Returns the
// end size of the result.  If buffer is NULL, performs no writing.
static size_t
format_buffer(char *buf, size_t buf_size, const char *str, int width, int first_indent, int par_indent, int rest_indent)
{
    const char *abbreviations[] = {"Dr.", "Mr." "Ms.", "Mrs.", "Miss.", "Esq.", "Dept.", NULL};
    const char *read_pt = str;
    size_t output_size = 0;
    int line_width = 0;
    char *write_pt = buf;
    char *buf_end = buf + buf_size;
    int padding = first_indent;
    bool par_end = false;

    // Skip initial spaces in input
    while (*read_pt && isspace(*read_pt)) {
        read_pt++;
    }

    // Outer loop here loops once per word
    while (*read_pt) {
        // Skip spaces to beginning of word
        while (*read_pt && isspace(*read_pt)) {
            if (*read_pt == '\n') {
                par_end = true;
            }
            read_pt++;
        }
        if (!*read_pt) {
            par_end = true;
        }

        // Find end of word.  A word ends before white space or an open
        // parenthesis, and after any other punctuation.
        const char *word = read_pt;
        bool sentence_end = false;

        // Find end of number
        if (isdigit(*read_pt)) {
            while (*read_pt) {
                while (*read_pt
                       && !isspace(*read_pt)
                       && (isdigit(*read_pt) || !ispunct(*read_pt))) {
                    read_pt++;
                }
                if (*read_pt != ',' && *read_pt != '.') {
                    break;
                }
                if (!isdigit(*(read_pt + 1))) {
                    break;
                }
                read_pt++;
            }
        } else {
            // Find end of text word
            if (*read_pt == '(') {
                read_pt++;
            }
            while (*read_pt
                   && !isspace(*read_pt)
                   && (*read_pt == '\'' || !ispunct(*read_pt))) {
                read_pt++;
            }
        }

        // Find end of punctuation
        while (*read_pt && ispunct(*read_pt) && *read_pt != '(') {
            if (*read_pt == '.') {
                // Check for common abbreviations
                for (int i = 0; abbreviations[i]; i++) {
                    if (strncasecmp(word, abbreviations[i], read_pt - word)) {
                        sentence_end = true;
                    }
                }
            } else if (*read_pt == '?' || *read_pt == '!') {
                sentence_end = true;
            }
            read_pt++;
        }

        // Check for fit in current line
        line_width += read_pt - word + padding;

        // Render the line breaks, padding, and word
        if (par_end || line_width > width) {
            output_size += 2;
            if (write_pt != buf_end) {
                *write_pt++ = '\r';
            }
            if (write_pt != buf_end) {
                *write_pt++ = '\n';
            }
            padding = (par_end) ? par_indent : rest_indent;
            line_width = read_pt - word + padding;
        }

        if (word != read_pt) {
            output_size += padding;

            if (buf) {
                while (padding--) {
                    if (write_pt != buf_end) {
                        *write_pt++ = ' ';
                    }
                }
            }
        }

        while (word != read_pt) {
            output_size += read_pt - word;
            if (write_pt != buf_end) {
                *write_pt++ = *word;
            }
            word++;
        }

        // Prepare for next word
        padding = (sentence_end) ? 2 : 1;
        par_end = false;
    }

    *write_pt = '\0';

    return output_size;
}

// returns a formatted string word-wrapped to the given width, the
// first line indented by first_indent, lines preceded by a line break
// are indented by par_indent, and the rest of the lines indented by
// rest_indent.  Attempts to correct spacing between punctuation and
// words.
char *
tmp_format(const char *str, int width, int first_indent, int par_indent, int rest_indent)
{
    struct tmp_str_pool *cur_buf;
    size_t wanted;
    char *result;

    cur_buf = tmp_list_head;

    result = &cur_buf->data[cur_buf->used];
    wanted = format_buffer(result, cur_buf->space - cur_buf->used, str, width, first_indent, par_indent, rest_indent) + 1;

    // If there was enough space, our work is done here.  If there wasn't enough
    // space, we allocate another pool, and write into that.  The newer
    // pool is, of course, always big enough.

    if (cur_buf->space - cur_buf->used < wanted) {
        cur_buf = tmp_alloc_pool(wanted);
        result = &cur_buf->data[0];
        wanted = format_buffer(result, cur_buf->space - cur_buf->used, str, width, first_indent, par_indent, rest_indent) + 1;
    }

    cur_buf->used += wanted;

    return result;
}

// Word-wraps a string, given a buffer.  Returns the end size of the
// result.  If buffer is NULL, performs no writing.
static size_t
wrap_buffer(char *buf, size_t buf_size, const char *str, int width, int first_indent, int par_indent, int rest_indent)
{
    const char *read_pt = str;
    size_t output_size = 0;
    int line_width = 0;
    char *write_pt = buf;
    char *buf_end = buf + buf_size;
    int padding = first_indent;
    bool par_end = false;

    // Skip initial spaces in input
    while (*read_pt && isspace(*read_pt)) {
        read_pt++;
    }

    // Outer loop here loops once per word
    while (*read_pt) {
        // Skip spaces to beginning of word
        while (*read_pt && isspace(*read_pt)) {
            if (*read_pt == '\n') {
                par_end = true;
            }
            padding++;
            read_pt++;
        }
        if (!*read_pt) {
            par_end = true;
        }

        // Find end of word.  A word ends before white space
        const char *word = read_pt;
        int nonprinting = 0;

        while (*read_pt && !isspace(*read_pt)) {
            read_pt++;
            if (*read_pt == '\e') {
                nonprinting++;
                while (*read_pt < '@' || *read_pt > '~') {
                    nonprinting++;
                    read_pt++;
                }
            }
        }

        // Check for fit in current line
        line_width += read_pt - word - nonprinting + padding;

        // Render the line breaks, padding, and word
        if (par_end || line_width > width) {
            output_size += 2;
            if (write_pt != buf_end) {
                *write_pt++ = '\r';
            }
            if (write_pt != buf_end) {
                *write_pt++ = '\n';
            }
            padding = (par_end) ? par_indent : rest_indent;
            line_width = read_pt - word + padding;
        }

        if (word != read_pt) {
            output_size += padding;

            if (buf) {
                while (padding--) {
                    if (write_pt != buf_end) {
                        *write_pt++ = ' ';
                    }
                }
            }
        }

        while (word != read_pt) {
            output_size += read_pt - word;
            if (write_pt != buf_end) {
                *write_pt++ = *word;
            }
            word++;
        }

        // Prepare for next word
        padding = 0;
        par_end = false;
    }

    *write_pt = '\0';

    return output_size;
}

// returns a string word-wrapped to the given width, the first line
// indented by first_indent, lines preceded by a line break are
// indented by par_indent, and the rest of the lines indented
// by rest_indent.
char *
tmp_wrap(const char *str, int width, int first_indent, int par_indent, int rest_indent)
{
    struct tmp_str_pool *cur_buf;
    size_t wanted;
    char *result;

    cur_buf = tmp_list_head;

    result = &cur_buf->data[cur_buf->used];
    wanted = wrap_buffer(result, cur_buf->space - cur_buf->used, str, width, first_indent, par_indent, rest_indent) + 1;

    // If there was enough space, our work is done here.  If there wasn't enough
    // space, we allocate another pool, and write into that.  The newer
    // pool is, of course, always big enough.

    if (cur_buf->space - cur_buf->used < wanted) {
        cur_buf = tmp_alloc_pool(wanted);
        result = &cur_buf->data[0];
        wanted = wrap_buffer(result, cur_buf->space - cur_buf->used, str, width, first_indent, par_indent, rest_indent) + 1;
    }

    cur_buf->used += wanted;

    return result;
}
