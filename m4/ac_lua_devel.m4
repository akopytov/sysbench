dnl ---------------------------------------------------------------------------
dnl Macro: AC_LUA_DEVEL
dnl ---------------------------------------------------------------------------
AC_DEFUN([AC_LUA_DEVEL],[

AC_ARG_WITH(lua, 
    AC_HELP_STRING([--with-lua],[Compile with Lua scripting support (default is enabled)]), 
                   [ac_cv_use_lua="$with_lua"], [ac_cv_use_lua="yes"])
AC_CACHE_CHECK([whether to compile with Lua support], [ac_cv_use_lua], [ac_cv_use_lua=no])

if test "x$ac_cv_use_lua" != "xno"; then 

AC_DEFINE(HAVE_LUA, 1, [Define to 1 if you have Lua headers and libraries])

fi

AM_CONDITIONAL(USE_LUA, test "x$ac_cv_use_lua" != "xno")
])

