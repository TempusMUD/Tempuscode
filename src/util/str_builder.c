#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "tmpstr.h"
#include "str_builder.h"

const size_t SB_CHUNK_SIZE = 4096;
const size_t SB_MAX_SIZE = 20 * 1024 * 1024;

const struct str_builder str_builder_default = {0};
char *tmp_alloc(size_t size_req);

static void
sb_adjust(struct str_builder *sb, size_t wanted)
{
    if (sb->size > wanted) {
        return;
    }

    if (sb->size > SB_MAX_SIZE) {
        raise(SIGSEGV);
    }

    size_t new_size = wanted + (SB_CHUNK_SIZE - (wanted % SB_CHUNK_SIZE));
    char *new_str = tmp_alloc(new_size);

    if (sb->len > 0) {
        memcpy(new_str, sb->str, sb->len+1);
    }
    sb->size = new_size;
    sb->str = new_str;
}

void
sb_vsprintf(struct str_builder *sb, const char *fmt, va_list args)
{
    size_t wanted;
    char *result;
    va_list args_copy;

    va_copy(args_copy, args);

    result = sb->str + sb->len;
    wanted = vsnprintf(result, sb->size - sb->len, fmt, args);

    // If there was enough space, our work is done here.  If there wasn't enough
    // space, we expand the accumulator, and write into that.

    if (sb->size - sb->len <= wanted) {
        sb_adjust(sb, sb->size + wanted + 1);
        result = sb->str + sb->len;
        wanted = vsnprintf(result, sb->size - sb->len, fmt, args_copy);
    }
    va_end(args_copy);

    sb->len += wanted;

    // NUL-terminate finished string
    sb->str[sb->len] = '\0';
}

void
sb_sprintf(struct str_builder *sb, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    sb_vsprintf(sb, fmt, args);
    va_end(args);
}

void
sb_strcat(struct str_builder *sb, const char *str, ...)
{
    char *write_pt;
    const char *read_pt;
    size_t len;
    va_list args;

    // Figure out how much space we'll need
    len = strlen(str);

    va_start(args, str);
    while ((read_pt = va_arg(args, const char *)) != NULL) {
        len += strlen(read_pt);
    }
    va_end(args);

    // If we don't have the space, we allocate more
    if (len > sb->size - sb->len) {
        sb_adjust(sb, sb->size + len);
    }

    write_pt = sb->str + sb->len;
    sb->len += len;

    // Copy in the first string
    strcpy(write_pt, str);
    while (*write_pt) {
        write_pt++;
    }

    // Then copy in the rest of the strings
    va_start(args, str);
    while ((read_pt = va_arg(args, const char *)) != NULL) {
        strcpy(write_pt, read_pt);
        while (*write_pt) {
            write_pt++;
        }
    }
    va_end(args);

    // NUL-terminate finished string
    sb->str[sb->len] = '\0';
}
