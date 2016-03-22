# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT([sysbench],[0.9],[akopytov@gmail.com])
AC_CONFIG_AUX_DIR([config])
# Setting CFLAGS here prevents AC_CANONICAL_TARGET from injecting them
SAVE_CFLAGS=${CFLAGS}
SAVE_CXXFLAGS=${CXXFLAGS}
CFLAGS=
CXXFLAGS=
AC_CANONICAL_TARGET

CFLAGS=${SAVE_CFLAGS}
CXXFLAGS=${SAVE_CXXFLAGS}
 
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_SRCDIR([sysbench/sysbench.c])
AC_CONFIG_HEADER([config/config.h])
AC_CONFIG_MACRO_DIR([m4])

ACX_USE_SYSTEM_EXTENSIONS

AC_PROG_CC
AC_PROG_CPP
AM_PROG_CC_C_O

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

#LT_INIT([dlopen])
#LT_LIB_DLLOAD
AC_PROG_LIBTOOL

AC_CHECK_DECL([__SUNPRO_C], [SUNCC="yes"], [SUNCC="no"])

AC_MSG_CHECKING("C Compiler version")
if test "$GCC" = "yes"
then
  CC_VERSION=`$CC --version | sed 1q`
elif test "$SUNCC" = "yes"
then
  CC_VERSION=`$CC -V 2>&1 | sed 1q`
else
  CC_VERSION=""
fi
AC_MSG_RESULT("$CC_VERSION")
AC_SUBST(CC_VERSION)

# Checks for user arguments

AC_LIB_PREFIX()
# Additional linker flags
AC_ARG_WITH([extra-ldflags],
              AS_HELP_STRING([--with-extra-ldflags],[additional linker flags, e.g. -all-static]),
              EXTRA_LDFLAGS=$withval
)
AC_SUBST(EXTRA_LDFLAGS)

CPPFLAGS="-D_XOPEN_SOURCE=500 -D_GNU_SOURCE ${CPPFLAGS}"

# Mac OS X requires _DARWIN_C_SOURCE for valloc(3) to be visible
case "${host_cpu}-${host_os}" in
	*-darwin*) CPPFLAGS="-D_DARWIN_C_SOURCE ${CPPFLAGS}";;
esac

# Build optimized or debug version ?
# First check for gcc and g++
if test "$GCC" = "yes"
then
  CFLAGS="-ggdb3 ${CFLAGS}"
  DEBUG_CFLAGS="-O0"
  OPTIMIZE_CFLAGS="-O2"
fi
if test "$SUNCC" = "yes"
then
  isainfo_k=`isainfo -k`
  if test "$target_cpu" = "sparc"
  then
    MEMALIGN_FLAGS="-xmemalign=8s"
    IS_64="-m64"
    LDFLAGS="${LDFLAGS} -L/usr/lib/${isainfo_k} -L/usr/local/lib/${isainfo_k}"
  else
    if test "$isainfo_k" = "amd64"
    then
      IS_64="-m64"
      LDFLAGS="${LDFLAGS} -L/usr/lib/${isainfo_k} -L/usr/local/lib/${isainfo_k}"
    fi
  fi
  CPPFLAGS="${CPPFLAGS} -I/usr/local/include"

  CFLAGS="-g -mt ${IS_64} ${MEMALIGN_FLAGS} ${CFLAGS}"
  DEBUG_CFLAGS="-xO0"
  OPTIMIZE_CFLAGS="-xO2 -xlibmil -xdepend -Xa -mt -xstrconst"
fi


# Check if we should compile with MySQL support
AC_ARG_WITH([mysql],
            AS_HELP_STRING([--with-mysql],
                           [compile with MySQL support (default is enabled)]),
            [], [with_mysql=yes])
AC_MSG_CHECKING([whether to compile with MySQL support])
AS_IF([test "x$with_mysql" != "xno"],
   [mysql_support=yes],
   [mysql_support=no])
AC_MSG_RESULT([$mysql_support])

# Check if we should compile with Drizzle support
AC_ARG_WITH([drizzle],
            AS_HELP_STRING([--with-drizzle],
                           [compile with Drizzle support (default is disabled)]),
            [], [with_drizzle=no])
AC_MSG_CHECKING([whether to compile with Drizzle support])
AS_IF([test "x$with_drizzle" != "xno"],
   [drizzle_support=yes],
   [drizzle_support=no])
AC_MSG_RESULT([$drizzle_support])

# Check if we should compile with libattachsql support
AC_ARG_WITH([attachsql],
            AS_HELP_STRING([--with-attachsql],
                           [compile with libattachsql support (default is disabled)]),
            [], [with_attachsql=no])
AC_MSG_CHECKING([whether to compile with libattachsql support])
AS_IF([test "x$with_attachsql" != "xno"],
   [attachsql_support=yes],
   [attachsql_support=no])
AC_MSG_RESULT([$attachsql_support])


# Check if we should compile with Oracle support
AC_ARG_WITH([oracle],
            AS_HELP_STRING([--with-oracle],
                           [compile with Oracle support (default is disabled)]),
            [], [with_oracle=no])
AC_MSG_CHECKING([whether to compile with Oracle support])
AS_IF([test "x$with_oracle" != "xno"],
   [oracle_support=yes],
   [oracle_support=no])
AC_MSG_RESULT([$oracle_support])

# Check if we should compile with PostgreSQL support
AC_ARG_WITH([pgsql],
            AS_HELP_STRING([--with-pgsql],
                           [compile with PostgreSQL support (default is disabled)]),
            [], [with_pgsql=no])
AC_MSG_CHECKING([whether to compile with PostgreSQL support])
AS_IF([test "x$with_pgsql" != "xno"],
   [pgsql_support=yes],
   [pgsql_support=no])
AC_MSG_RESULT([$pgsql_support])

# Set LuaJIT flags
SB_LUAJIT

# Check if we should enable large files support
AC_ARG_ENABLE(largefile,
    AS_HELP_STRING([--enable-largefile],[enable large files support (default is enabled)]), ,
    enable_largefile=yes
)

# For SHM_HUGETLB on Linux
AC_CHECK_DECLS(SHM_HUGETLB, 
    AC_DEFINE([HAVE_LARGE_PAGES], [1], 
              [Define if you have large pages support])
    AC_DEFINE([HUGETLB_USE_PROC_MEMINFO], [1],
              [Define if /proc/meminfo shows the huge page size (Linux only)])
    , ,
    [
#include <sys/shm.h>
    ]
)

# Check if we should enable Linux AIO support
AC_ARG_ENABLE(aio,
   AS_HELP_STRING([--enable-aio],[enable Linux asynchronous I/O support (default is enabled)]), ,
   enable_aio=yes
)

AC_CHECK_DECLS(O_SYNC, ,
   AC_DEFINE([O_SYNC], [O_FSYNC],
             [Define to the appropriate value for O_SYNC on your platform]),
   [
#include <fcntl.h>
   ]
)


# Checks for programs.
AX_CHECK_DOCBOOK

# Checks for libraries.

ACX_PTHREAD

AC_CHECK_LIB(m, sqrt)

AS_IF([test "x$with_mysql" != xno], [
    AC_CHECK_MYSQLR([$with_mysql])
    AC_DEFINE([USE_MYSQL], 1,
              [Define to 1 if you want to compile with MySQL support])
    AC_SUBST([MYSQL_LIBS])
    AC_SUBST([MYSQL_CFLAGS])
])
AM_CONDITIONAL(USE_MYSQL, test x$with_mysql != xno)

AS_IF([test "x$with_drizzle" != xno], [
  AC_LIB_HAVE_LINKFLAGS(drizzle,,
    [#include <libdrizzle/drizzle.h>],
    [
     int x= DRIZZLE_RETURN_ERROR_CODE;
     const char *version= drizzle_version();
    ])
    AS_IF([test "x$ac_cv_libdrizzle" = xyes], [
      AC_DEFINE([USE_DRIZZLE],1,
                [Define to 1 if you want to compile with Drizzle support])
    ])
])
AM_CONDITIONAL(USE_DRIZZLE, test x$ac_cv_libdrizzle = xyes)

AS_IF([test "x$with_attachsql" != xno], [
  AC_LIB_HAVE_LINKFLAGS(attachsql,,
    [#include <libattachsql-1.0/attachsql.h>],
    [
     const char *version= attachsql_get_library_version();
    ])
    AS_IF([test "x$ac_cv_libattachsql" = xyes], [
      AC_DEFINE(USE_ATTACHSQL,1,
                [Define to 1 if you want to compile with libattachsql support])
    ])
])
AM_CONDITIONAL(USE_ATTACHSQL, test x$ac_cv_libattachsql = xyes)


AS_IF([test x$with_oracle != xno], [
    AC_DEFINE(USE_ORACLE,1,[Define to 1 if you want to compile with Oracle support])
    ORA_LIBS="-L${sb_with_oracle}/lib -lclntsh"
    ORA_CFLAGS="-I${with_oracle}/rdbms/demo -I${with_oracle}/rdbms/public"
    AC_SUBST([ORA_LIBS])
    AC_SUBST([ORA_CFLAGS])
])
AM_CONDITIONAL(USE_ORACLE, test x$with_oracle != xno)

AS_IF([test x$with_pgsql != xno], [
    AC_CHECK_PGSQL([$with_pgsql])
    AC_DEFINE(USE_PGSQL,1,[Define to 1 if you want to compile with PostgreSQL support])
    AC_SUBST([PGSQL_LIBS])
    AC_SUBST([PGSQL_CFLAGS])
])
AM_CONDITIONAL(USE_PGSQL, test x$with_pgsql != xno)

# Check for libaio
AC_CHECK_AIO
AM_CONDITIONAL(USE_AIO, test x$enable_aio = xyes)

# Check for advanced memory allocation libraries 
AC_CHECK_LIB([umem], [malloc], [EXTRA_LDFLAGS="$EXTRA_LDFLAGS -lumem"], 
 AC_CHECK_LIB([mtmalloc], [malloc], [EXTRA_LDFLAGS="$EXTRA_LDFLAGS -lmtmalloc"]) 
) 

# Checks for header files.
AC_HEADER_STDC

AC_CHECK_HEADERS([ \
errno.h \
fcntl.h \
math.h \
pthread.h \
sched.h \
signal.h \
stdlib.h \
string.h \
sys/aio.h \
sys/ipc.h \
sys/time.h \
sys/mman.h \
sys/shm.h \
thread.h \
unistd.h \
limits.h \
])


# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AX_TLS([],
  AC_MSG_ERROR([Thread-local storage is not suppored by the target platform!])
)

if test "$enable_largefile" = yes; then
    AC_SYS_LARGEFILE
fi

AC_CHECK_SIZEOF(size_t)

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_STRERROR_R

AC_CHECK_DECL([clock_gettime], 
    AC_DEFINE([HAVE_CLOCK_GETTIME], 1, [Define if you have clock_gettime() declared in <time.h>]),,
    [
#define _XOPEN_SOURCE 500
#include <time.h>
    ]
)
AC_SEARCH_LIBS([clock_gettime], [rt]) 

save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
save_LIBS="$LIBS"
LIBS="$PTHREAD_LIBS $LIBS"

AC_CHECK_FUNCS([ \
alarm \
directio \
fdatasync \
gettimeofday \
lrand48_r \
rand_r \
memalign \
memset \
mkstemp \
popen \
posix_memalign \
pthread_yield \
_setjmp \
setvbuf \
sqrt \
strdup \
thr_setconcurrency \
valloc \
])

AC_CHECK_FUNC(pthread_once, , 
              AC_MSG_ERROR([*** pthread_once() is not available on this platform ***])
)

LIBS="$save_LIBS"
CFLAGS="$save_CFLAGS"

AC_ARG_WITH([debug],
    [AS_HELP_STRING([--with-debug],
       [Add debug code/turns off optimizations (yes|no) @<:@default=no@:>@])],
    [with_debug=$withval],
    [with_debug=no])
if test "$with_debug" = "yes"
then
  # Debugging. No optimization.
  CFLAGS="${DEBUG_CFLAGS} -DDEBUG ${CFLAGS}"
else
  # Optimized version. No debug
  CFLAGS="${OPTIMIZE_CFLAGS} ${CFLAGS}"
fi

AC_ARG_ENABLE([profiling],
    [AS_HELP_STRING([--enable-profiling],
       [Toggle profiling @<:@default=off@:>@])],
    [ac_profiling="$enableval"],
    [ac_profiling="no"])
 
AC_ARG_ENABLE([coverage],
    [AS_HELP_STRING([--enable-coverage],
       [Toggle coverage @<:@default=off@:>@])],
    [ac_coverage="$enableval"],
    [ac_coverage="no"])

AC_ARG_ENABLE([pedantic-warnings],
    [AS_HELP_STRING([--disable-pedantic-warnings],
       [Toggle pedanticness @<:@default=on@:>@])],
    [ac_warn_pedantic="$enableval"],
    [ac_warn_pedantic="yes"])

AC_ARG_ENABLE([iso-pedantic-warnings],
    [AS_HELP_STRING([--enable-iso-pedantic-warnings],
       [Toggle iso pedanticness @<:@default=off@:>@])],
    [ac_warn_iso_pedantic="$enableval"],
    [ac_warn_iso_pedantic="no"])

AC_ARG_ENABLE([unreachable],
  [AS_HELP_STRING([--enable-unreachable],
    [Enable warnings about unreachable code @<:@default=no@:>@])],
  [ac_warn_unreachable="$enableval"],
  [ac_warn_unreachable="no"])


AC_ARG_ENABLE([fail],
    [AS_HELP_STRING([--disable-fail],
       [Turn warnings into failures @<:@default=no@:>@])],
    [ac_warn_fail="$enableval"],
    [ac_warn_fail="no"])

AC_ARG_ENABLE([go-crazy],
    [AS_HELP_STRING([--disable-go-crazy],
       [Enables extra little warnings that might be too much @<:@default=on@:>@])],
    [ac_warn_go_crazy="$enableval"],
    [ac_warn_go_crazy="no"])

if test "$GCC" = "yes"
then
  if test "$ac_warn_fail" = "yes"
  then
    W_FAIL="-Werror"
  fi
  BASE_WARNINGS="-W -Wall -Wextra -Wpointer-arith -Wbad-function-cast \
  -Wstrict-prototypes -Wnested-externs -Winline \
  -funroll-loops ${W_FAIL}"

  if test "$ac_profiling" = "yes"
  then
    GPROF_PROFILING="-pg"
  else
    GPROF_PROFILING=" "
  fi

  if test "$ac_coverage" = "yes"
  then
    GPROF_COVERAGE="-fprofile-arcs -ftest-coverage"
  else
    GPROF_COVERAGE=" "
  fi

  if test "$ac_warn_pedantic" = "yes"
  then
    W_PEDANTIC="-Wundef -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Wcast-align"
  fi

  if test "$ac_warn_iso_pedantic" = "yes"
  then
    W_PEDANTIC="-pedantic ${W_PEDANTIC}"
  fi

  if test "$ac_warn_unreachable" = "yes"
  then
    W_UNREACHABLE="-Wunreachable-code"
  fi

  if test "$ac_warn_go_crazy" = "yes"
  then
    W_CRAZY="-Wshadow -Wconversion"
  fi

  CC_WARNINGS="${BASE_WARNINGS} ${W_PEDANTIC} ${W_UNREACHABLE} ${GPROF_PROFILING} ${GPROF_COVERAGE} ${W_CRAZY}"

  NO_REDUNDANT_DECLS="-Wno-redundant-decls"
  CPPFLAGS="${CPPFLAGS}"
fi
if test "$SUNCC" = "yes"
then
  CC_WARNINGS="-v -errtags=yes -errwarn=%all -erroff=E_INTEGER_OVERFLOW_DETECTED"
fi

AM_CFLAGS="${CC_WARNINGS} ${AM_CFLAGS} ${PTHREAD_CFLAGS}"

AM_CPPFLAGS="${AM_CPPFLAGS} -I\$(top_srcdir)/sysbench ${LUAJIT_CFLAGS}"

AM_LDFLAGS="$PTHREAD_LIBS"

AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CPPFLAGS)
AC_SUBST(AM_LDFLAGS)

AC_CONFIG_FILES([
Makefile
doc/xsl/Makefile
doc/xsl/catalog.xml
doc/Makefile
third_party/luajit/Makefile
sysbench/Makefile
sysbench/drivers/Makefile
sysbench/drivers/mysql/Makefile
sysbench/drivers/drizzle/Makefile
sysbench/drivers/oracle/Makefile
sysbench/drivers/pgsql/Makefile
sysbench/drivers/attachsql/Makefile
sysbench/tests/Makefile
sysbench/tests/cpu/Makefile
sysbench/tests/fileio/Makefile
sysbench/tests/memory/Makefile
sysbench/tests/threads/Makefile
sysbench/tests/mutex/Makefile
sysbench/tests/db/Makefile
sysbench/scripting/Makefile
])
AC_OUTPUT

AC_MSG_RESULT([===============================================================================])
AC_MSG_RESULT([sysbench version   : ${PACKAGE_VERSION}])
AC_MSG_RESULT([CC                 : ${CC}])
AC_MSG_RESULT([CFLAGS             : ${CFLAGS} ${AM_CFLAGS}])
AC_MSG_RESULT([CPPFLAGS           : ${CPPFLAGS} ${AM_CPPFLAGS}])
AC_MSG_RESULT([LDFLAGS            : ${LDFLAGS} ${AM_LDFLAGS}])
AC_MSG_RESULT([LIBS               : ${LIBS}])
AC_MSG_RESULT([EXTRA_LDFLAGS      : ${EXTRA_LDFLAGS}])
AC_MSG_RESULT([])
AC_MSG_RESULT([prefix             : $(eval echo ${prefix})])
AC_MSG_RESULT([bindir             : $(eval echo ${bindir})])
AC_MSG_RESULT([libexecdir         : $(eval echo ${libexecdir})])
AC_MSG_RESULT([mandir             : $(eval echo ${mandir})])
AC_MSG_RESULT([])
AC_MSG_RESULT([MySQL support      : ${mysql_support}])
AC_MSG_RESULT([Drizzle support    : ${drizzle_support}])
AC_MSG_RESULT([AttachSQL support  : ${attachsql_support}])
AC_MSG_RESULT([Oracle support     : ${oracle_support}])
AC_MSG_RESULT([PostgreSQL support : ${pgsql_support}])
AC_MSG_RESULT([])
AC_MSG_RESULT([LuaJIT             : ${sb_use_luajit}])
AC_MSG_RESULT([LUAJIT_CFLAGS      : ${LUAJIT_CFLAGS}])
AC_MSG_RESULT([LUAJIT_LIBS        : ${LUAJIT_LIBS}])
AC_MSG_RESULT([LUAJIT_LDFLAGS     : ${LUAJIT_LDFLAGS}])
AC_MSG_RESULT([===============================================================================])
