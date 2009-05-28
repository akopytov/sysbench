# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT(sysbench, 0.4.12, alexeyk@mysql.com)
AC_CONFIG_AUX_DIR([config])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE()
AC_CONFIG_SRCDIR([sysbench/sysbench.c])
AM_CONFIG_HEADER([config/config.h])

# Checks for user arguments

# Additional linker flags
AC_ARG_WITH([extra-ldflags],
              AC_HELP_STRING([--with-extra-ldflags], [additional linker flags, e.g. -all-static]),
              EXTRA_LDFLAGS=$withval
)
AC_SUBST(EXTRA_LDFLAGS)

# Check if we should compile with MySQL support
AC_ARG_WITH([mysql], 
            AC_HELP_STRING([--with-mysql], [compile with MySQL support (default is enabled)]),
            [ac_cv_use_mysql=$withval], [ac_cv_use_mysql=yes]
)
AC_CACHE_CHECK([whether to compile with MySQL support], [ac_cv_use_mysql], [ac_cv_use_mysql=no])

# Check if we should compile with Oracle support
AC_ARG_WITH([oracle],
            AC_HELP_STRING([--with-oracle], [compile with Oracle support (default is disabled)]),
            [ac_cv_use_oracle=$withval], [ac_cv_use_oracle=no]
)
AC_CACHE_CHECK([whether to compile with Oracle support], [ac_cv_use_oracle], [ac_cv_use_oracle=yes])

# Check if we should compile with PostgreSQL support
AC_ARG_WITH([pgsql],
            AC_HELP_STRING([--with-pgsql], [compile with PostgreSQL support (default is disabled)]),
            [ac_cv_use_pgsql=$withval], [ac_cv_use_pgsql=no]
)
AC_CACHE_CHECK([whether to compile with PostgreSQL support], [ac_cv_use_pgsql], [ac_cv_use_pgsql=yes])

# Check if we should enable large files support
AC_ARG_ENABLE(largefile,
    AC_HELP_STRING([--enable-largefile],[enable large files support (default is enabled)]), ,
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
   AC_HELP_STRING([--enable-aio],[enable Linux asynchronous I/O support (default is enabled)]), ,
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
AC_PROG_CC
AC_PROG_LIBTOOL

AX_CHECK_DOCBOOK

# Checks for libraries.

ACX_PTHREAD

AC_CHECK_LIB([m], [sqrt])

if test x$ac_cv_use_mysql != xno; then
    AC_CHECK_MYSQLR([$ac_cv_use_mysql])
    AC_DEFINE(USE_MYSQL,1,[Define to 1 if you want to compile with MySQL support])
    AC_SUBST([MYSQL_LIBS])
    AC_SUBST([MYSQL_CFLAGS])
fi
AM_CONDITIONAL(USE_MYSQL, test x$ac_cv_use_mysql != xno)

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

AC_CHECK_FUNCS([ \
alarm \
fdatasync \
gettimeofday \
lrand48 \
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

if test "$GCC" = "xyes"; then
    AM_CFLAGS="-W -Wall -Wpointer-arith -Wbad-function-cast \
-Wstrict-prototypes -Wnested-externs -Winline \
-funroll-loops"
fi

AM_CFLAGS="$AM_CFLAGS $PTHREAD_CFLAGS"

AM_CPPFLAGS='-D_REENTRANT -D_XOPEN_SOURCE=500 -D_GNU_SOURCE -I$(top_srcdir)/sysbench'
AM_LDFLAGS="$PTHREAD_LIBS"

AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CPPFLAGS)
AC_SUBST(AM_LDFLAGS)

AC_OUTPUT([
Makefile
doc/xsl/Makefile
doc/xsl/catalog.xml
doc/Makefile
sysbench/Makefile
sysbench/drivers/Makefile
sysbench/drivers/mysql/Makefile
sysbench/drivers/oracle/Makefile
sysbench/drivers/pgsql/Makefile
sysbench/tests/Makefile
sysbench/tests/cpu/Makefile
sysbench/tests/fileio/Makefile
sysbench/tests/memory/Makefile
sysbench/tests/threads/Makefile
sysbench/tests/mutex/Makefile
sysbench/tests/oltp/Makefile
])
