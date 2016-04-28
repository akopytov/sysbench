dnl ---------------------------------------------------------------------------
dnl Macro: AC_LUA_DEVEL
dnl ---------------------------------------------------------------------------
AC_DEFUN([AC_LUA_DEVEL],[

AC_ARG_WITH(lua, 
    AC_HELP_STRING([--with-lua],[Compile with Lua scripting support (default is enabled)]), 
                   [ac_cv_use_lua="$with_lua"], [ac_cv_use_lua="yes"])
AC_CACHE_CHECK([whether to compile with Lua support], [ac_cv_use_lua], [ac_cv_use_lua=no])

if test "x$ac_cv_use_lua" != "xno"; then 

PKG_CHECK_MODULES([LUA], [lua53 >= 5.1], ,
  [
    AC_MSG_WARN(['Lua53 not found, looking for lua5.2 (>=5.1)'])
    PKG_CHECK_MODULES([LUA], [lua5.2 >= 5.1], ,
      [
        AC_MSG_WARN(['Lua52 not found, looking for lua (>=5.1)'])
        PKG_CHECK_MODULES([LUA], [lua >= 5.1])
      ]
    )
  ]
)

AC_DEFINE(HAVE_LUA, 1, [Define to 1 if you have Lua headers and libraries])

fi

AM_CONDITIONAL(USE_LUA, test "x$ac_cv_use_lua" != "xno")
])

