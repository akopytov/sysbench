# Copyright (C) 2016-2017 Alexey Kopytov <akopytov@gmail.com>
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
# Macro: SB_CONCURRENCY_KIT
# ---------------------------------------------------------------------------
AC_DEFUN([SB_CONCURRENCY_KIT], [

AC_ARG_WITH([system-ck],
  AC_HELP_STRING([--with-system-ck],
  [Use system-provided Concurrency Kit headers and library (requires pkg-config)]),
  [sb_use_ck="system"],
  [sb_use_ck="bundled"])

AC_CACHE_CHECK([whether to build with system or bundled Concurrency Kit],
  [sb_cv_lib_ck], [
    AS_IF([test "x$sb_use_ck" = "xsystem"],
    [
      sb_cv_lib_ck=[system]
    ], [
      sb_cv_lib_ck=[bundled]
    ])
  ])

AS_IF([test "x$sb_cv_lib_ck" = "xsystem"],
  # let PKG_CHECK_MODULES set CK_CFLAGS and CK_LIBS for system libck
  [PKG_CHECK_MODULES([CK], [ck])],
  # Set CK_CFLAGS and CK_LIBS manually for bundled libck
  [
    CK_CFLAGS="-I\$(abs_top_builddir)/third_party/concurrency_kit/include"
    CK_LIBS="\$(abs_top_builddir)/third_party/concurrency_kit/lib/libck.a"

    case $target_cpu in
      powerpc*|aarch64)
        # Assume 128-byte cache line on AArch64 and PowerPC
        CPPFLAGS="${CPPFLAGS} -DCK_MD_CACHELINE=128"
        ;;
        # Force --platform=i*86 for CK, otherwise its configure script
        # autodetects target based on 'uname -m' which doesn't work for
        # cross-compiliation
      i486*|i586*)
        CK_CONFIGURE_FLAGS="--platform=i586"
        ;;
      i686*)
        CK_CONFIGURE_FLAGS="--platform=i686"
        ;;
    esac
    # Add --enable-lse to CK build flags, if LSE instructions are supported by
    # the target architecture
    if test "$cross_compiling" = no -a "$host_cpu" = aarch64; then
      AC_MSG_CHECKING([whether LSE instructions are supported])
      AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM(,
        [[
          unsigned long long d = 1, a = 1;
          unsigned long long *t = &a;

          __asm__ __volatile__(
                               "stadd %0, [%1];"
                                  : "+&r" (d)
                                  : "r"   (t)
                                  : "memory");
        ]])],
        [
          CK_CONFIGURE_FLAGS="--enable-lse"
          AC_MSG_RESULT([yes])
        ],
        [
          CK_CONFIGURE_FLAGS=""
          AC_MSG_RESULT([no])
        ]
      )

      AC_SUBST([CK_CONFIGURE_FLAGS])
    fi
  ]
)

AC_DEFINE_UNQUOTED([SB_WITH_CK], ["$sb_use_ck"],
  [Whether system or bundled Concurrency Kit is used])

AM_CONDITIONAL([USE_BUNDLED_CK], [test "x$sb_use_ck" = xbundled])
])
