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

testroot=$(cd $(dirname "$0"); echo $PWD)

# Find the sysbench binary to use
if [ -x "$testroot/../sysbench/sysbench" ]
then
    # Invoked from a source directory?
    PATH="$testroot/../sysbench:$PATH"
elif  [ -x "$testroot/../bin" ]
then
    # Invoked from a standalone install root directory?
    PATH="$testroot/../bin:$PATH"
elif [ -x "$testroot/../../../bin/sysbench" ]
then
    # Invoked from a system-wide install (e.g. /usr/local/share/sysbench/tests)?
    PATH="$testroot/../../../bin:$PATH"
elif [ -x "$PWD/../sysbench/sysbench" ]
then
    # Invoked from the build directory by 'make distcheck'?
    PATH="$PWD/../sysbench:$PATH"
fi

if ! which sysbench >/dev/null 2>&1
then
    echo "Cannot find sysbench in PATH=$PATH"
    echo "testroot=$testroot"
    echo "PWD=$PWD"
    ls -l $PWD
    ls -l $PWD/../sysbench
    ls -l $testroot
    ls -l $testroot/..
    ls -l $testroot/../sysbench
    exit 1
fi

if [ -z ${srcdir+x} ]
then
    SBTEST_INCDIR="$PWD/include"
    SBTEST_CONFIG="$SBTEST_INCDIR/config.sh"
    if [ $# -lt 1 ]
    then
        tests="t/*.t"
    fi
else
    # SBTEST_INCDIR must be an absolute path, because cram changes CWD to a
    # temporary directory when executing tests. That's why we can just use
    # $srcdir here
    SBTEST_INCDIR="$(cd $srcdir; echo $PWD)/include"
    SBTEST_CONFIG="$PWD/include/config.sh"
    if [ $# -lt 1 ]
    then
        tests="$srcdir/t/*.t"
    fi
fi
if [ -z ${tests+x} ]
then
   tests="$*"
fi

export SBTEST_ROOTDIR="$testroot"
export SBTEST_SCRIPTDIR="$testroot/../sysbench/lua"
export SBTEST_SUITEDIR="$testroot/t"
export SBTEST_CONFIG
export SBTEST_INCDIR

. $SBTEST_CONFIG

cram --shell=/bin/bash --verbose $tests
