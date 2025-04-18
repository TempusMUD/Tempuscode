AC_PREREQ([2.71])
AC_INIT([tempusmud],[1.0],[azimuth@tempusmud.com])
AC_CONFIG_SRCDIR([src/clan/clan.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects])

# Enable or disable debugging
AC_ARG_ENABLE(debug,
         AS_HELP_STRING([--enable-debug],
                [enable verbose debugging @<:@default=yes@:>@]),
              [case "${enableval}" in
                 yes) debug=true ;;
                 no)  debug=false ;;
                 *)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
               esac],
              [debug=true])

AM_CONDITIONAL(DEBUG, test x"$debug" = "xtrue")

# Enable or disable ipv6
AC_ARG_ENABLE(ipv6,
         AS_HELP_STRING([--enable-ipv6],
                [enable ipv6 support @<:@default=yes@:>@]),
              [case "${enableval}" in
                 yes) ipv6_support=true ;;
                 no)  ipv6_support=false ;;
                 *)   AC_MSG_ERROR([bad value ${enableval} for --enable-ipv6]) ;;
               esac],
              [ipv6_support=true])

AM_CONDITIONAL(IPV6_SUPPORT, test x"$ipv6_support" = "xtrue")

# Set default CFLAGS to empty
: ${CFLAGS=""}

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AX_LIB_POSTGRESQL
PKG_CHECK_MODULES([GLIB], [glib-2.0])
PKG_CHECK_MODULES([XML], [libxml-2.0])
PKG_CHECK_MODULES([CHECK], [check])
PKG_CHECK_MODULES([ZLIB], [zlib])
AC_CHECK_LIB([rt], [clock_gettime])
AC_CHECK_LIB([crypt], [crypt])
AC_CHECK_LIB([m], [ceilf])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h limits.h malloc.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_CHECK_FUNCS([bzero gethostbyaddr gettimeofday isascii memmove memset mkdir select socket strcasecmp strchr strdup strerror strncasecmp strpbrk strspn strstr strtol vsnprintf])

AC_CONFIG_FILES([Makefile src/Makefile tests/Makefile])
AC_OUTPUT
