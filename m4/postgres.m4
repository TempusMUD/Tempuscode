AC_DEFUN([AC_CHECK_POSTGRES], [

AC_ARG_WITH(pgsql,
        [  --with-pgsql=PREFIX          Prefix of your PostgreSQL installation],
        [pg_prefix=$withval], [pg_prefix=])
AC_ARG_WITH(pgsql-inc,
        [  --with-pgsql-inc=PATH                Path to the include directory of PostgreSQL],
        [pg_inc=$withval], [pg_inc=])
AC_ARG_WITH(pgsql-lib,
        [  --with-pgsql-lib=PATH                Path to the libraries of PostgreSQL],
        [pg_lib=$withval], [pg_lib=])


AC_SUBST(PQINCPATH)
AC_SUBST(PQLIBPATH)

AC_MSG_CHECKING([for PostgreSQL includes in $pg_inc])
if test "$pg_prefix" != ""; then
    pg_inc="$pg_prefix/include"
    pg_lib="$pg_prefix/lib"
	LDFLAGS="$LDFLAGS -L$pg_lib"
else
    for dir in /usr/include/postgresql \
               /usr/local/pgsql/include \
               /usr/local/include/; do
        if test -f "$dir/libpq-fe.h"; then
            pg_inc="$dir"
        fi
    done
fi
if test "$pg_inc" != "" ; then
    PQINCPATH="-I$pg_inc"
    AC_MSG_RESULT([yes])
else
    AC_MSG_ERROR(libpq-fe.h not found)
fi

AC_CHECK_LIB(pq, PQconnectdb)

])
