#!/bin/sh

aclocal && automake -c --foreign --add-missing && autoheader && autoconf

