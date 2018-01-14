# ---------------------------------------------------------------------------
# Provide various compatibility macros for older Autoconf machines
# Definitions were copied from the Autoconf source code.
# ---------------------------------------------------------------------------
m4_ifdef([AS_VAR_IF],,m4_define([AS_VAR_IF],
[AS_LITERAL_WORD_IF([$1],
  [AS_IF(m4_ifval([$2], [[test "x$$1" = x[]$2]], [[${$1:+false} :]])],
  [AS_VAR_COPY([as_val], [$1])
   AS_IF(m4_ifval([$2], [[test "x$as_val" = x[]$2]], [[${as_val:+false} :]])],
  [AS_IF(m4_ifval([$2],
    [[eval test \"x\$"$1"\" = x"_AS_ESCAPE([$2], [`], [\"$])"]],
    [[eval \${$1:+false} :]])]),
[$3], [$4])]))dnl

# Define m4_ifblank and m4_ifnblank macros from introduced in
#  autotools 2.64 m4sugar.m4 if using an earlier autotools.
ifdef([m4_ifblank], [], [
m4_define([m4_ifblank],
[m4_if(m4_translit([[$1]],  [ ][	][
]), [], [$2], [$3])])
])

ifdef([m4_ifnblank], [], [
m4_define([m4_ifnblank],
[m4_if(m4_translit([[$1]],  [ ][	][
]), [], [$3], [$2])])
])
