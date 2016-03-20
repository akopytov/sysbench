#!/usr/bin/env bash
# Taken from lighthttpd server (BSD). Thanks Jan!
# Run this to generate all the initial makefiles, etc.

die() { echo "$@"; exit 1; }

ACLOCAL_FLAGS="-I m4"
LIBTOOLIZE_FLAGS="--copy --force"
AUTOMAKE_FLAGS="-c --foreign --add-missing"

ARGV0=$0
ARGS="$@"


run() {
	echo "$ARGV0: running \`$@' $ARGS"
	$@ $ARGS
}

## jump out if one of the programs returns 'false'
set -e


if test x$ACLOCAL = x; then
  if test \! "x`which aclocal-1.10 2> /dev/null | grep -v '^no'`" = x; then
    ACLOCAL=aclocal-1.10
  elif test \! "x`which aclocal110 2> /dev/null | grep -v '^no'`" = x; then
    ACLOCAL=aclocal110
  elif test \! "x`which aclocal 2> /dev/null | grep -v '^no'`" = x; then
    ACLOCAL=aclocal
  else 
    echo "automake 1.10.x (aclocal) wasn't found, exiting"; exit 1
  fi
fi

if test x$LIBTOOLIZE = x; then
  if test \! "x`which libtoolize15 2> /dev/null | grep -v '^no'`" = x; then
    LIBTOOLIZE=libtoolize15
  elif test \! "x`which libtoolize14 2> /dev/null | grep -v '^no'`" = x; then
    LIBTOOLIZE=libtoolize14
  elif test \! "x`which glibtoolize 2> /dev/null | grep -v '^no'`" = x; then
    LIBTOOLIZE=glibtoolize
  elif test \! "x`which libtoolize 2> /dev/null | grep -v '^no'`" = x; then
    LIBTOOLIZE=libtoolize
  else
    echo "libtoolize 1.4+ wasn't found, exiting"; exit 1
  fi
fi

if test x$AUTOMAKE = x; then
  if test \! "x`which automake-1.10 2> /dev/null | grep -v '^no'`" = x; then
    AUTOMAKE=automake-1.10
  elif test \! "x`which automake110 2> /dev/null | grep -v '^no'`" = x; then
    AUTOMAKE=automake110
  elif test \! "x`which automake 2> /dev/null | grep -v '^no'`" = x; then
    AUTOMAKE=automake
  else 
    echo "automake 1.10.x wasn't found, exiting"; exit 1
  fi
fi


## macosx has autoconf-2.59 and autoconf-2.60
if test x$AUTOCONF = x; then
  if test \! "x`which autoconf-2.59 2> /dev/null | grep -v '^no'`" = x; then
    AUTOCONF=autoconf-2.59
  elif test \! "x`which autoconf259 2> /dev/null | grep -v '^no'`" = x; then
    AUTOCONF=autoconf259
  elif test \! "x`which autoconf 2> /dev/null | grep -v '^no'`" = x; then
    AUTOCONF=autoconf
  else 
    echo "autoconf 2.59+ wasn't found, exiting"; exit 1
  fi
fi

if test x$AUTOHEADER = x; then
  if test \! "x`which autoheader-2.59 2> /dev/null | grep -v '^no'`" = x; then
    AUTOHEADER=autoheader-2.59
  elif test \! "x`which autoheader259 2> /dev/null | grep -v '^no'`" = x; then
    AUTOHEADER=autoheader259
  elif test \! "x`which autoheader 2> /dev/null | grep -v '^no'`" = x; then
    AUTOHEADER=autoheader
  else 
    echo "autoconf 2.59+ (autoheader) wasn't found, exiting"; exit 1
  fi
fi

run $LIBTOOLIZE $LIBTOOLIZE_FLAGS || die "Can't execute libtoolize"
run $ACLOCAL $ACLOCAL_FLAGS || die "Can't execute aclocal"
run $AUTOHEADER || die "Can't execute autoheader"
run $AUTOMAKE $AUTOMAKE_FLAGS  || die "Can't execute automake"
run $AUTOCONF || die "Can't execute autoconf"
echo -n "Libtoolized with: "
$LIBTOOLIZE --version | head -1
echo -n "Automade with: "
$AUTOMAKE --version | head -1
echo -n "Configured with: "
$AUTOCONF --version | head -1
