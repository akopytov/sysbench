dnl ---------------------------------------------------------------------------
dnl Macro: AC_CHECK_MYSQLR
dnl First check if the MySQL root directory is specified with --with-mysql.
dnl Otherwise check for custom MySQL paths in --with-mysql-includes and
dnl --with-mysql-libs. If some paths are not specified explicitly, try to get
dnl them from mysql_config.
dnl ---------------------------------------------------------------------------

AC_DEFUN([AC_CHECK_MYSQLR],[
# Check for custom MySQL root directory
if test [ x$1 != xyes -a x$1 != xno ] 
then
    ac_cv_mysql_root=`echo $1 | sed -e 's+/$++'`
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
])
