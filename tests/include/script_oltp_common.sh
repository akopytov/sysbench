#!/usr/bin/env bash
#
################################################################################
# Common code for OLTP tests
#
# Expects the following variables and callback functions to be defined by the
# caller:
#
#   DB_DRIVER_ARGS -- extra driver-specific arguments to pass to sysbench
#   OLTP_SCRIPT_PATH -- path to the script file to execute
#   SB_EXTRA_ARGS -- optional variable to specify extra sysbench arguments
#
#   db_show_table() -- called with a single argument to dump a specified table
#                      schema
################################################################################

set -eu

SB_EXTRA_ARGS=${SB_EXTRA_ARGS:-}

ARGS="${OLTP_SCRIPT_PATH} ${DB_DRIVER_ARGS} ${SB_EXTRA_ARGS} --tables=8"

sysbench $ARGS prepare

db_show_table sbtest1
db_show_table sbtest2
db_show_table sbtest3
db_show_table sbtest4
db_show_table sbtest5
db_show_table sbtest6
db_show_table sbtest7
db_show_table sbtest8
db_show_table sbtest9 || true # Error on non-existing table

sysbench $ARGS prewarm || true # MySQL only

sysbench --events=100 --threads=2 $ARGS run

sysbench $ARGS cleanup

db_show_table sbtest1 || true # Error on non-existing table
db_show_table sbtest2 || true # Error on non-existing table
db_show_table sbtest3 || true # Error on non-existing table
db_show_table sbtest4 || true # Error on non-existing table
db_show_table sbtest5 || true # Error on non-existing table
db_show_table sbtest6 || true # Error on non-existing table
db_show_table sbtest7 || true # Error on non-existing table
db_show_table sbtest8 || true # Error on non-existing table

# Test --create-secondary=off
ARGS="${OLTP_SCRIPT_PATH} ${DB_DRIVER_ARGS} ${SB_EXTRA_ARGS} --tables=1"

sysbench --create-secondary=off $ARGS prepare

db_show_table sbtest1

sysbench $ARGS cleanup
