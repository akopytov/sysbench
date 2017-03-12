#!/usr/bin/env bash

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

set -eu

testroot=$(cd $(dirname "$0"); echo $PWD)

# Find the sysbench binary to use
dirlist=( "$testroot/../src"       # source directory
          "$testroot/../bin"       # standalone install root directory
          "$testroot/../../../bin" # system-wide install (e.g. /usr/local/share/sysbench/tests)
          "$PWD/../src" )          # build directory by 'make distcheck'

for dir in ${dirlist[@]}
do
    if [ -x "$dir/sysbench" ]
    then
        sysbench_dir="$dir"
        break
    fi
done

if [ -z ${sysbench_dir+x} ]
then
    echo "Cannot find sysbench in the following list of directories: \
${dirlist[@]}"
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
export SBTEST_SCRIPTDIR="$testroot/../src/lua"
export SBTEST_SUITEDIR="$testroot/t"
export SBTEST_CONFIG
export SBTEST_INCDIR

# Add directories containing sysbench and cram to PATH
export PATH="${sysbench_dir}:${SBTEST_ROOTDIR}/../third_party/cram/scripts:$PATH"

export PYTHONPATH="${SBTEST_ROOTDIR}/../third_party/cram:${PYTHONPATH:-}"
export LUA_PATH="$SBTEST_SCRIPTDIR/?.lua"

. $SBTEST_CONFIG

cram --shell=/bin/bash --verbose $tests
