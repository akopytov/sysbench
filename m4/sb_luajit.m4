# Copyright (C) 2016 Alexey Kopytov <akopytov@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

# ---------------------------------------------------------------------------
# Macro: SB_LUAJIT
# ---------------------------------------------------------------------------
AC_DEFUN([SB_LUAJIT], [

AC_ARG_WITH([system-luajit],
  AC_HELP_STRING([--with-system-luajit],
  [Use system-provided LuaJIT headers and library (requires pkg-config)]),
  [sb_use_luajit="system"],
  [sb_use_luajit="bundled"])

AC_CACHE_CHECK([whether to build with system or bundled LuaJIT],
  [sb_cv_lib_luajit], [
    AS_IF([test "x$sb_use_luajit" = "xsystem"],
    [
      AC_MSG_RESULT([system])
      PKG_CHECK_MODULES([LUAJIT], [luajit])
    ], [
      AC_MSG_RESULT([bundled])
      LUAJIT_CFLAGS="-I\$(abs_top_builddir)/third_party/luajit/inc"
      LUAJIT_LIBS="\$(abs_top_builddir)/third_party/luajit/lib/libluajit-5.1.a"
    ])
  ])

AC_DEFINE_UNQUOTED([SB_WITH_LUAJIT], ["$sb_use_luajit"],
  [Whether system or bundled LuaJIT is used])

# Add extra flags when building a 64-bit application on OS X,
# http://luajit.org/install.html
AS_CASE([$host_os:$host_cpu],
        [*darwin*:x86_64],
        [LUAJIT_LDFLAGS="-pagezero_size 10000 -image_base 100000000"])
AC_SUBST([LUAJIT_LDFLAGS])

])
