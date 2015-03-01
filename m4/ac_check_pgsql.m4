dnl ---------------------------------------------------------------------------
dnl Macro: AC_CHECK_PGSQL
dnl First check for custom PostgreSQL paths in --with-pgsql-* options.
dnl If some paths are missing,  check if pg_config exists. 
dnl ---------------------------------------------------------------------------

AC_DEFUN([AC_CHECK_PGSQL],[

# Check for custom includes path
if test [ -z "$ac_cv_pgsql_includes" ] 
then 
    AC_ARG_WITH([pgsql-includes], 
                AC_HELP_STRING([--with-pgsql-includes], [path to PostgreSQL header files]),
                [ac_cv_pgsql_includes=$withval])
fi
if test [ -n "$ac_cv_pgsql_includes" ]
then
    AC_CACHE_CHECK([PostgreSQL includes], [ac_cv_pgsql_includes], [ac_cv_pgsql_includes=""])
    PGSQL_CFLAGS="-I$ac_cv_pgsql_includes"
fi

# Check for custom library path

if test [ -z "$ac_cv_pgsql_libs" ]
then
    AC_ARG_WITH([pgsql-libs], 
                AC_HELP_STRING([--with-pgsql-libs], [path to PostgreSQL libraries]),
                [ac_cv_pgsql_libs=$withval])
fi

if test [ -n "$ac_cv_pgsql_libs" ]
then
     AC_CACHE_CHECK([PostgreSQL libraries], [ac_cv_pgsql_libs], [ac_cv_pgsql_libs=""])
     PGSQL_LIBS="-L$ac_cv_pgsql_libs -lpq"
fi

# If some path is missing, try to autodetermine with pgsql_config
if test [ -z "$ac_cv_pgsql_includes" -o -z "$ac_cv_pgsql_libs" ]
then
    if test [ -z "$pgconfig" ]
    then 
        AC_PATH_PROG(pgconfig,pg_config)
    fi
    if test [ -z "$pgconfig" ]
    then
        AC_MSG_ERROR([pg_config executable not found
********************************************************************************
ERROR: cannot find PostgreSQL libraries. If you want to compile with PosgregSQL support,
       you must either specify file locations explicitly using 
       --with-pgsql-includes and --with-pgsql-libs options, or make sure path to 
       pg_config is listed in your PATH environment variable. If you want to 
       disable PostgreSQL support, use --without-pgsql option.
********************************************************************************
])
    else
        if test [ -z "$ac_cv_pgsql_includes" ]
        then
            AC_MSG_CHECKING(PostgreSQL C flags)
            PGSQL_CFLAGS="-I`${pgconfig} --includedir`"
            AC_MSG_RESULT($PGSQL_CFLAGS)
        fi
        if test [ -z "$ac_cv_pgsql_libs" ]
        then
            AC_MSG_CHECKING(PostgreSQL linker flags)
            PGSQL_LIBS="-L`${pgconfig} --libdir` -lpq"
            AC_MSG_RESULT($PGSQL_LIBS)
        fi
    fi
fi
])

