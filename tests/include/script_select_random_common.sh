#!/usr/bin/env bash
#
################################################################################
# Common code for select_random_* tests
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

for test in select_random_points select_random_ranges
do
    ARGS="${SBTEST_SCRIPTDIR}/${test}.lua $DB_DRIVER_ARGS --tables=1"

    sysbench $ARGS prepare

    db_show_table sbtest1

    for i in $(seq 2 8)
    do
        db_show_table sbtest${i} || true # Error on non-existing table
    done

    sysbench $ARGS --events=100 run

    sysbench $ARGS cleanup

    for i in $(seq 1 8)
    do
        db_show_table sbtest${i} || true # Error on non-existing table
    done

    ARGS="${SBTEST_SCRIPTDIR}/select_random_points.lua $DB_DRIVER_ARGS --tables=8"
done
