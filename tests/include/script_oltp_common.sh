#!/usr/bin/env bash
#
#########################################################################
# Common code for OLTP tests
# Variables:
#   DB_DRIVER_ARGS -- extra driver-specific arguments to pass to sysbench
#########################################################################

set -eu

ARGS="--test=${SBTEST_SCRIPTDIR}/oltp.lua $DB_DRIVER_ARGS --oltp-tables-count=8"

sysbench $ARGS prepare

sysbench $ARGS --max-requests=100 --num-threads=1 run

sysbench $ARGS cleanup
