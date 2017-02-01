#!/usr/bin/env bash
#
################################################################################
# Common code for select_bulk_insert* tests
#
# Expects the following variables and callback functions to be defined by the
# caller:
#
#   DB_DRIVER_ARGS -- extra driver-specific arguments to pass to sysbench
#
#   db_show_table() -- called with a single argument to dump a specified table
#                      schema
################################################################################

set -eu

ARGS="${SBTEST_SCRIPTDIR}/bulk_insert.lua $DB_DRIVER_ARGS --threads=2 --verbosity=1"

sysbench $ARGS prepare

for i in $(seq 1 3)
do
    db_show_table sbtest${i} || true # Error on non-existing table
done

sysbench $ARGS --events=100 --verbosity=3 run

sysbench $ARGS cleanup

for i in $(seq 1 3)
do
    db_show_table sbtest${i} || true # Error on non-existing table
done
