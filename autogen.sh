#!/bin/sh

aclocal || exit 1

LIBTOOLIZE=""
for f in libtoolize glibtoolize ; do
  if $f --version >/dev/null 2>&1 ; then
    LIBTOOLIZE=$f
    break
  fi
done

if test -z "$LIBTOOLIZE"; then
  echo "libtoolize not found, you may want to install libtool on your system"
  exit 1
fi
$LIBTOOLIZE --copy --force || exit 1
automake -c --foreign --add-missing && autoheader && autoconf
