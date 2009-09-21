dnl ---------------------------------------------------------------------------
dnl Macro: AC_CHECK_AIO
dnl Check for Linux AIO availability on the target system
dnl Also, check the version of libaio library (at the moment, there are two
dnl versions with incompatible interfaces).
dnl ---------------------------------------------------------------------------

AC_DEFUN([AC_CHECK_AIO],[
if test x$enable_aio = xyes; then
    AC_CHECK_HEADER([libaio.h], 
                    [AC_DEFINE(HAVE_LIBAIO_H,1,[Define to 1 if your system has <libaio.h> header file])],
                    [enable_aio=no])
fi
if test x$enable_aio = xyes; then 
    AC_CHECK_LIB([aio], [io_queue_init], , [enable_aio=no])
fi
if test x$enable_aio = xyes; then
    AC_MSG_CHECKING(if io_getevents() has an old interface)
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
                       [[
#ifdef HAVE_LIBAIO_H
# include <libaio.h>
#endif
struct io_event event; 
io_context_t ctxt;
                       ]],
                       [[
(void)io_getevents(ctxt, 1, &event, NULL);
                       ]] )
    ], [
        AC_DEFINE([HAVE_OLD_GETEVENTS], 1, [Define to 1 if libaio has older getevents() interface])
        AC_MSG_RESULT(yes)
       ],
    [AC_MSG_RESULT(no)]
    )
fi
])

