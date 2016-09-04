#!/usr/bin/env bash

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

set -eu

if [ -x "$PWD/../sysbench/sysbench" ]
then
    # Invoked from a source directory?
    export PATH=$PWD/../sysbench:$PATH
elif  [ -x "$PWD/../bin" ]
then
    # Invoked from a standalone install root directory?
    export PATH=$PWD/../bin:$PATH
elif [ -x "$PWD/../../../bin/sysbench" ]
then
    # Invoked from a system-wide install (e.g. /usr/local/share/sysbench/tests)?
    export PATH=$PWD/../../../bin:$PATH
fi

if ! which sysbench >/dev/null 2>&1
then
    echo "Cannot find sysbench in PATH=$PATH"
    exit 1
fi

if [ $# -lt 1 ]
then
    # Automake defines $srcdir where test datafiles are located on 'make check'
    if [ -z ${srcdir+x} ]
    then
        testroot="."
    else
        testroot="$srcdir"
    fi
    tests="${testroot}/*.t"
else
    tests="$*"
fi

cram --shell=/bin/bash --verbose $tests
