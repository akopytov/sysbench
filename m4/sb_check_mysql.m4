dnl ---------------------------------------------------------------------------
dnl Macro: SB_CHECK_MYSQL
dnl First check if the MySQL root directory is specified with --with-mysql.
dnl Otherwise check for custom MySQL paths in --with-mysql-includes and
dnl --with-mysql-libs. If some paths are not specified explicitly, try to get
dnl them from mysql_config.
dnl ---------------------------------------------------------------------------

AC_DEFUN([SB_CHECK_MYSQL],[

AS_IF([test "x$with_mysql" != xno], [

# Check for custom MySQL root directory
if test [ "x$with_mysql" != xyes -a "x$with_mysql" != xno ] 
then
    ac_cv_mysql_root=`echo "$with_mysql" | sed -e 's+/$++'`
    if test [ -d "$ac_cv_mysql_root/include" -a \
              -d "$ac_cv_mysql_root/libmysql_r" ]
    then
        ac_cv_mysql_includes="$ac_cv_mysql_root/include"
        ac_cv_mysql_libs="$ac_cv_mysql_root/libmysql_r"
    elif test [ -x "$ac_cv_mysql_root/bin/mysql_config" ]
    then
        mysqlconfig="$ac_cv_mysql_root/bin/mysql_config"
    else 
        AC_MSG_ERROR([invalid MySQL root directory: $ac_cv_mysql_root])
    fi
fi

# Check for custom includes path
if test [ -z "$ac_cv_mysql_includes" ] 
then 
    AC_ARG_WITH([mysql-includes], 
                AC_HELP_STRING([--with-mysql-includes], [path to MySQL header files]),
                [ac_cv_mysql_includes=$withval])
fi
if test [ -n "$ac_cv_mysql_includes" ]
then
    AC_CACHE_CHECK([MySQL includes], [ac_cv_mysql_includes], [ac_cv_mysql_includes=""])
    MYSQL_CFLAGS="-I$ac_cv_mysql_includes"
fi

# Check for custom library path

if test [ -z "$ac_cv_mysql_libs" ]
then
    AC_ARG_WITH([mysql-libs], 
                AC_HELP_STRING([--with-mysql-libs], [path to MySQL libraries]),
                [ac_cv_mysql_libs=$withval])
fi
if test [ -n "$ac_cv_mysql_libs" ]
then
    # Trim trailing '.libs' if user passed it in --with-mysql-libs option
    ac_cv_mysql_libs=`echo ${ac_cv_mysql_libs} | sed -e 's/.libs$//' \
                      -e 's+.libs/$++'`
    AC_CACHE_CHECK([MySQL libraries], [ac_cv_mysql_libs], [ac_cv_mysql_libs=""])
    save_LDFLAGS="$LDFLAGS"
    save_LIBS="$LIBS"
    LDFLAGS="-L$ac_cv_mysql_libs"
    LIBS=""

    # libmysqlclient_r has been removed in MySQL 5.7
    AC_SEARCH_LIBS([mysql_real_connect],
      [mysqlclient_r mysqlclient],
      [],
      AC_MSG_ERROR([cannot find MySQL client libraries in $ac_cv_mysql_libs]))

    MYSQL_LIBS="$LDFLAGS $LIBS"
    LIBS="$save_LIBS"
    LDFLAGS="$save_LDFLAGS"
fi

# If some path is missing, try to autodetermine with mysql_config
if test [ -z "$ac_cv_mysql_includes" -o -z "$ac_cv_mysql_libs" ]
then
    if test [ -z "$mysqlconfig" ]
    then 
        AC_PATH_PROG(mysqlconfig,mysql_config)
    fi
    if test [ -z "$mysqlconfig" ]
    then
        AC_MSG_ERROR([mysql_config executable not found
********************************************************************************
ERROR: cannot find MySQL libraries. If you want to compile with MySQL support,
       please install the package containing MySQL client libraries and headers.
       On Debian-based systems the package name is libmysqlclient-dev.
       On RedHat-based systems, it is mysql-devel.
       If you have those libraries installed in non-standard locations,
       you must either specify file locations explicitly using
       --with-mysql-includes and --with-mysql-libs options, or make sure path to
       mysql_config is listed in your PATH environment variable. If you want to
       disable MySQL support, use --without-mysql option.
********************************************************************************
])
    else
        if test [ -z "$ac_cv_mysql_includes" ]
        then
            AC_MSG_CHECKING(MySQL C flags)
            MYSQL_CFLAGS=`${mysqlconfig} --cflags`
            AC_MSG_RESULT($MYSQL_CFLAGS)
        fi
        if test [ -z "$ac_cv_mysql_libs" ]
        then
            AC_MSG_CHECKING(MySQL linker flags)
            MYSQL_LIBS=`${mysqlconfig} --libs_r`
            AC_MSG_RESULT($MYSQL_LIBS)
        fi
    fi
fi

AC_DEFINE([USE_MYSQL], 1,
          [Define to 1 if you want to compile with MySQL support])

USE_MYSQL=1
AC_SUBST([MYSQL_LIBS])
AC_SUBST([MYSQL_CFLAGS])

AC_MSG_CHECKING([if mysql.h defines MYSQL_OPT_SSL_MODE])

SAVE_CFLAGS="${CFLAGS}"
CFLAGS="${CFLAGS} ${MYSQL_CFLAGS}"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
        [[
#include <mysql.h>

enum mysql_option opt = MYSQL_OPT_SSL_MODE;
        ]])], [
          AC_DEFINE([HAVE_MYSQL_OPT_SSL_MODE], 1,
                    [Define to 1 if mysql.h defines MYSQL_OPT_SSL_MODE])
          AC_MSG_RESULT([yes])
          ], [AC_MSG_RESULT([no])])
])
CFLAGS="${SAVE_CFLAGS}"


AM_CONDITIONAL([USE_MYSQL], test "x$with_mysql" != xno)
AC_SUBST([USE_MYSQL])
])
