# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT([sysbench],[0.5],[sysbench-developers@lists.launchpad.net])
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
  GCOV_CFLAGS="-O0 --coverage"
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
# TODO: Set flags for Gcov-enabled builds, if supported by Sun Studio
fi


# Check if we should compile with MySQL support
AC_ARG_WITH([mysql], 
            AS_HELP_STRING([--with-mysql],[compile with MySQL support (default is enabled)]),
            [ac_cv_use_mysql=$withval], [ac_cv_use_mysql=yes]
)
AC_CACHE_CHECK([whether to compile with MySQL support], [ac_cv_use_mysql], [ac_cv_use_mysql=no])

# Check if we should compile with Drizzle support
AC_ARG_WITH([drizzle], 
            AS_HELP_STRING([--with-drizzle],[compile with Drizzle support (default is enabled)]),
            [ac_cv_use_drizzle=$withval], [ac_cv_use_drizzle=yes]
)
AC_CACHE_CHECK([whether to compile with Drizzle support], [ac_cv_use_drizzle], [ac_cv_use_drizzle=no])

# Check if we should compile with libattachsql support
AC_ARG_WITH([attachsql], 
            AS_HELP_STRING([--with-attachsql],[compile with libattachsql support (default is enabled)]),
            [ac_cv_use_attachsql=$withval], [ac_cv_use_attachsql=yes]
)
AC_CACHE_CHECK([whether to compile with libattachsql support], [ac_cv_use_attachsql], [ac_cv_use_attachsql=no])


# Check if we should compile with Oracle support
AC_ARG_WITH([oracle],
            AS_HELP_STRING([--with-oracle],[compile with Oracle support (default is disabled)]),
            [ac_cv_use_oracle=$withval], [ac_cv_use_oracle=no]
)
AC_CACHE_CHECK([whether to compile with Oracle support], [ac_cv_use_oracle], [ac_cv_use_oracle=yes])

# Check if we should compile with PostgreSQL support
AC_ARG_WITH([pgsql],
            AS_HELP_STRING([--with-pgsql],[compile with PostgreSQL support (default is disabled)]),
            [ac_cv_use_pgsql=$withval], [ac_cv_use_pgsql=no]
)
AC_CACHE_CHECK([whether to compile with PostgreSQL support], [ac_cv_use_pgsql], [ac_cv_use_pgsql=yes])

# Check if we should compile with Lua interpreter support
AC_LUA_DEVEL

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

if test x$ac_cv_use_mysql != xno; then
    AC_CHECK_MYSQLR([$ac_cv_use_mysql])
    AC_DEFINE(USE_MYSQL,1,[Define to 1 if you want to compile with MySQL support])
    AC_SUBST([MYSQL_LIBS])
    AC_SUBST([MYSQL_CFLAGS])
fi
AM_CONDITIONAL(USE_MYSQL, test x$ac_cv_use_mysql != xno)

if test x$ac_cv_use_drizzle != xno; then
  AC_LIB_HAVE_LINKFLAGS(drizzle,,
    [#include <libdrizzle/drizzle.h>],
    [
     int x= DRIZZLE_RETURN_ERROR_CODE;
     const char *version= drizzle_version();
    ])
fi
if test "x$ac_cv_libdrizzle" = "xyes"
then
    AC_DEFINE(USE_DRIZZLE,1,[Define to 1 if you want to compile with Drizzle support])
fi
AM_CONDITIONAL(USE_DRIZZLE, test x$ac_cv_libdrizzle = xyes)

if test x$ac_cv_use_attachsql != xno; then
  AC_LIB_HAVE_LINKFLAGS(attachsql,,
    [#include <libattachsql-1.0/attachsql.h>],
    [
     const char *version= attachsql_get_library_version();
    ])
fi
if test "x$ac_cv_libattachsql" = "xyes"
then
    AC_DEFINE(USE_ATTACHSQL,1,[Define to 1 if you want to compile with libattachsql support])
fi
AM_CONDITIONAL(USE_ATTACHSQL, test x$ac_cv_libattachsql = xyes)


AM_CONDITIONAL(USE_ORACLE, test x$ac_cv_use_oracle != xno)
if test x$ac_cv_use_oracle != xno; then
    AC_DEFINE(USE_ORACLE,1,[Define to 1 if you want to compile with Oracle support])
    ORA_LIBS="-L${ac_cv_use_oracle}/lib -lclntsh"
    ORA_CFLAGS="-I${ac_cv_use_oracle}/rdbms/demo -I${ac_cv_use_oracle}/rdbms/public"
    AC_SUBST([ORA_LIBS])
    AC_SUBST([ORA_CFLAGS])
fi

if test x$ac_cv_use_pgsql != xno; then
    AC_CHECK_PGSQL([$ac_cv_use_pgsql])
    AC_DEFINE(USE_PGSQL,1,[Define to 1 if you want to compile with PostgreSQL support])
    AC_SUBST([PGSQL_LIBS])
    AC_SUBST([PGSQL_CFLAGS])
fi
AM_CONDITIONAL(USE_PGSQL, test x$ac_cv_use_pgsql != xno)

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
lrand48 \
drand48 \
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

if test "$with_debug" = "yes"
then
  # Debugging. No optimization.
  CFLAGS="${DEBUG_CFLAGS} -DDEBUG ${CFLAGS}"
elif test "$ac_coverage" = "yes"
then
  # Gcov-enabled build. No optimization.
  CFLAGS="${GCOV_CFLAGS} ${CFLAGS}"
else
  # Optimized version. No debug
  CFLAGS="${OPTIMIZE_CFLAGS} ${CFLAGS}"
fi

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
    GPROF_COVERAGE="--coverage"
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

  CC_WARNINGS="${BASE_WARNINGS} ${W_PEDANTIC} ${W_UNREACHABLE} ${W_CRAZY}"

  NO_REDUNDANT_DECLS="-Wno-redundant-decls"
  CPPFLAGS="${CPPFLAGS}"
fi
if test "$SUNCC" = "yes"
then
  CC_WARNINGS="-v -errtags=yes -errwarn=%all -erroff=E_INTEGER_OVERFLOW_DETECTED"
fi

AM_CFLAGS="${CC_WARNINGS} ${GPROF_PROFILING} ${GPROF_COVERAGE} ${AM_CFLAGS} ${PTHREAD_CFLAGS}"

AM_CPPFLAGS="${AM_CPPFLAGS} -I\$(top_srcdir)/sysbench"

AM_LDFLAGS="$PTHREAD_LIBS"

AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CPPFLAGS)
AC_SUBST(AM_LDFLAGS)

AC_CONFIG_FILES([
Makefile
doc/xsl/Makefile
doc/xsl/catalog.xml
doc/Makefile
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
sysbench/scripting/lua/Makefile
sysbench/scripting/lua/src/Makefile
tests/Makefile
tests/include/config.sh
])
AC_OUTPUT
