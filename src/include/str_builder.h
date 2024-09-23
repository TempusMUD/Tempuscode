#ifndef _STR_BUILDER_H_
#define _STR_BUILDER_H_

struct str_builder {
    size_t size;
    size_t len;
    char *str;
};

extern const struct str_builder str_builder_default;

void sb_vsprintf(struct str_builder *sb, const char *fmt, va_list args)
    __attribute__((nonnull (1, 2)));

void sb_sprintf(struct str_builder *sb, const char *fmt, ...)
    __attribute__((nonnull (1, 2)))
    __attribute__((format (printf, 2, 3)));

void sb_strcat(struct str_builder *sb, const char *str, ...)
    __attribute__((nonnull (1)))
    __attribute__((sentinel));;

#endif
