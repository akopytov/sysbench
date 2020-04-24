# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.63)
AC_INIT([sysbench],[1.0.20],[https://github.com/akopytov/sysbench/issues],
        [sysbench], [https://github.com/akopytov/sysbench])
AC_CONFIG_AUX_DIR([config])

# Define m4_ifblank and m4_ifnblank macros from introduced in
#  autotools 2.64 m4sugar.m4 if using an earlier autotools.
m4_ifdef([m4_ifblank], [], [
m4_define([m4_ifblank],
[m4_if(m4_translit([[$1]],  [ ][	][
]), [], [$2], [$3])])
])

m4_ifdef([m4_ifnblank], [], [
m4_define([m4_ifnblank],
[m4_if(m4_translit([[$1]],  [ ][	][
]), [], [$3], [$2])])
])

# Setting CFLAGS here prevents AC_CANONICAL_TARGET from injecting them
SAVE_CFLAGS=${CFLAGS}
SAVE_CXXFLAGS=${CXXFLAGS}
CFLAGS=
CXXFLAGS=
AC_CANONICAL_TARGET

CFLAGS=${SAVE_CFLAGS}
CXXFLAGS=${SAVE_CXXFLAGS}
 
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_SRCDIR([src/sysbench.c])
AC_CONFIG_HEADER([config/config.h])
AC_CONFIG_MACRO_DIR([m4])

m4_pattern_forbid([^PKG_[A-Z_]+$],
        [pkg-config has to be installed to build sysbench])

ACX_USE_SYSTEM_EXTENSIONS

AC_PROG_CC
AC_PROG_CC_C99
AC_PROG_CPP
AM_PROG_CC_C_O

if test x"$ac_cv_prog_cc_c99" = xno; then
   AC_MSG_ERROR([a C99 compiler is required to build sysbench])
fi

# Try to guess the most optimal compiler architecture flag, unless it's already
# been specified by the user via CFLAGS (or --without-gcc-arch is passed to
# configure)
AS_CASE([$CFLAGS],
  [*-march=*],,
  [AX_GCC_ARCHFLAG([no])]
)

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

AC_PROG_LIBTOOL

AC_CHECK_PROG(sb_have_pkg_config, pkg-config, yes, no)
if test x"$sb_have_pkg_config" = xno; then
  AC_MSG_ERROR([the pkg-config package is required to build sysbench])
fi

AX_COMPILER_VENDOR

# Checks for user arguments

AC_LIB_PREFIX()
# Additional linker flags
AC_ARG_WITH([extra-ldflags],
              AS_HELP_STRING([--with-extra-ldflags],[additional linker flags, e.g. -all-static]),
              EXTRA_LDFLAGS=$withval
)
AC_SUBST(EXTRA_LDFLAGS)

CPPFLAGS="-D_GNU_SOURCE ${CPPFLAGS}"

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
  OPTIMIZE_CFLAGS="-O2 -funroll-loops"
  GCOV_CFLAGS="-O0 --coverage"
  GCOV_LDFLAGS="-coverage"
  ASAN_CFLAGS="-fsanitize=address"
  ASAN_LDFLAGS="${ASAN_CFLAGS}"
  MSAN_CFLAGS="-fsanitize=memory"
  MSAN_LDFLAGS="${MSAN_LDFLAGS}"
fi
if test "$ax_cv_c_compiler_vendor" = "sun"
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

# Set Concurrency Kit flags
SB_CONCURRENCY_KIT

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

SB_CHECK_MYSQL

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
    ORA_CFLAGS="-I${with_oracle}/include -I${with_oracle}/rdbms/demo -I${with_oracle}/rdbms/public"
    AC_SUBST([ORA_LIBS])
    AC_SUBST([ORA_CFLAGS])
])
AM_CONDITIONAL(USE_ORACLE, test x$with_oracle != xno)

AS_IF([test x$with_pgsql != xno], [
    AC_CHECK_PGSQL([$with_pgsql])
    USE_PGSQL=1
    AC_DEFINE(USE_PGSQL,1,[Define to 1 if you want to compile with PostgreSQL support])
    AC_SUBST([PGSQL_LIBS])
    AC_SUBST([PGSQL_CFLAGS])
])
AM_CONDITIONAL(USE_PGSQL, test x$with_pgsql != xno)
AC_SUBST([USE_PGSQL])

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
libgen.h \
])


# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_OFF_T
AC_HEADER_TIME
AX_TLS([],
  AC_MSG_ERROR([thread-local storage is not suppored by the target platform!])
)

# Define HAVE_FUNC_ATTRIBUTE_FORMAT if compiler supports the
# __attribute__((format...)) function attribute
AX_GCC_FUNC_ATTRIBUTE(format)

# Define HAVE_FUNC_ATTRIBUTE_UNUSED if compiler supports the
# __attribute__((unused)) function attribute
AX_GCC_FUNC_ATTRIBUTE(unused)

if test "$enable_largefile" = yes; then
    AC_SYS_LARGEFILE
fi

AC_CHECK_SIZEOF(size_t)
AC_CHECK_SIZEOF(bool,,
  [
    #include <stdbool.h>
  ])

# Checks for library functions.
AC_FUNC_MMAP
AC_FUNC_STRERROR_R

AC_SEARCH_LIBS([clock_gettime], [rt]) 

save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
save_LIBS="$LIBS"
LIBS="$PTHREAD_LIBS $LIBS"

AC_CHECK_FUNCS([ \
alarm \
clock_gettime \
directio \
fdatasync \
gettimeofday \
isatty \
memalign \
memset \
posix_memalign \
pthread_yield \
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

AC_ARG_ENABLE([coverage],
    [AS_HELP_STRING([--enable-coverage],
       [Toggle coverage @<:@default=no@:>@])],
    [ac_coverage="$enableval"],
    [ac_coverage="no"])

AC_ARG_ENABLE([asan],
  [AS_HELP_STRING([--enable-asan],
    [Enable AddressSanitizer @<:@default=no@:>@])],
  [ac_asan="$enableval"],
  [ac_asan="no"])

AC_ARG_ENABLE([msan],
  [AS_HELP_STRING([--enable-msan],
    [Enable MemorySanitizer @<:@default=no@:>@])],
  [ac_msan="$enableval"],
  [ac_msan="no"])

AC_ARG_ENABLE([fail],
    [AS_HELP_STRING([--disable-fail],
       [Turn warnings into failures @<:@default=no@:>@])],
    [ac_warn_fail="$enableval"],
    [ac_warn_fail="no"])

if test "$with_debug" = "yes"
then
  # Debugging. No optimization.
  CFLAGS="${DEBUG_CFLAGS} -DDEBUG ${CFLAGS}"
elif test "$ac_coverage" = "yes"
then
  # Gcov-enabled build. No optimization.
  CFLAGS="${GCOV_CFLAGS} ${CFLAGS}"
  LDFLAGS="${GCOV_LDFLAGS} ${LDFLAGS}"
else
  # Optimized version. No debug
  CFLAGS="${OPTIMIZE_CFLAGS} ${CFLAGS}"
fi

if test "$ac_asan" = "yes"
then
  # Add -fsanitize=address to CFLAGS/LDFLAGS if supported by the compiler
  AX_CHECK_COMPILE_FLAG([-fsanitize=address],
  [
    CFLAGS="${ASAN_CFLAGS} ${CFLAGS}"
    LDFLAGS="${ASAN_LDFLAGS} ${LDFLAGS}"
  ])
fi

if test "$ac_msan" = "yes"
then
  # Add -fsanitize=memory to CFLAGS/LDFLAGS if supported by the compiler
  AX_CHECK_COMPILE_FLAG([-fsanitize=memory],
  [
    CFLAGS="${MSAN_CFLAGS} ${CFLAGS}"
    LDFLAGS="${MSAN_CFLAGS} ${LDFLAGS}"
  ])
fi

if test "$GCC" = "yes"
then
  if test "$ac_warn_fail" = "yes"
  then
    W_FAIL="-Werror"
  fi

  CC_WARNINGS="-Wall -Wextra -Wpointer-arith -Wbad-function-cast \
-Wstrict-prototypes -Wnested-externs -Wno-format-zero-length \
-Wundef -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations \
-Wredundant-decls -Wcast-align -Wvla ${W_FAIL}"
fi

if test "$ax_cv_c_compiler_vendor" = "sun"
then
  CC_WARNINGS="-v -errtags=yes -errwarn=%all -erroff=E_INTEGER_OVERFLOW_DETECTED -erroff=E_STATEMENT_NOT_REACHED"
fi

AM_CFLAGS="${CC_WARNINGS} ${AM_CFLAGS} ${PTHREAD_CFLAGS}"

AM_CPPFLAGS="${AM_CPPFLAGS} -I\$(top_srcdir)/src ${LUAJIT_CFLAGS} ${CK_CFLAGS}"

AM_LDFLAGS="$PTHREAD_LIBS"

AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CPPFLAGS)
AC_SUBST(AM_LDFLAGS)

# Define SB_GIT_SHA
git=$(which git)
if test -n "$git"
then
        SB_GIT_SHA=$(git rev-parse --short HEAD 2>/dev/null)
        if test -n "$SB_GIT_SHA"
        then
          SB_GIT_SHA="-$SB_GIT_SHA"
        fi
fi
AC_DEFINE_UNQUOTED([SB_GIT_SHA], ["$SB_GIT_SHA"], [Git commit hash, if available.])
AC_SUBST([SB_GIT_SHA])

AC_CONFIG_FILES([
Makefile
doc/xsl/Makefile
doc/xsl/catalog.xml
doc/Makefile
third_party/luajit/Makefile
third_party/concurrency_kit/Makefile
src/Makefile
src/drivers/Makefile
src/drivers/mysql/Makefile
src/drivers/drizzle/Makefile
src/drivers/oracle/Makefile
src/drivers/pgsql/Makefile
src/drivers/attachsql/Makefile
src/tests/Makefile
src/tests/cpu/Makefile
src/tests/fileio/Makefile
src/tests/memory/Makefile
src/tests/threads/Makefile
src/tests/mutex/Makefile
src/lua/Makefile
src/lua/internal/Makefile
tests/Makefile
tests/include/config.sh
snap/snapcraft.yaml
])
AC_OUTPUT

AC_MSG_RESULT([===============================================================================])
AC_MSG_RESULT([sysbench version   : ${PACKAGE_VERSION}${SB_GIT_SHA}])
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
AC_MSG_RESULT([datadir            : $(eval echo ${datadir})])
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
AC_MSG_RESULT([])
AC_MSG_RESULT([Concurrency Kit    : ${sb_use_ck}])
if test "$sb_use_ck" = bundled; then
  AC_MSG_RESULT([CK_CFLAGS          : ${CK_CFLAGS}])
  AC_MSG_RESULT([CK_LIBS            : ${CK_LIBS}])
  AC_MSG_RESULT([configure flags    : ${CK_CONFIGURE_FLAGS}])
fi
AC_MSG_RESULT([===============================================================================])
