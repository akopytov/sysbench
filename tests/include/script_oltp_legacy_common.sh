#!/usr/bin/env bash
#
################################################################################
# Common code for legacy OLTP tests
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

ARGS="--test=${SBTEST_INCDIR}/oltp_legacy/oltp.lua $DB_DRIVER_ARGS --oltp-tables-count=8"

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

sysbench $ARGS --max-requests=100 --num-threads=2 run

sysbench $ARGS cleanup

db_show_table sbtest1 || true # Error on non-existing table
db_show_table sbtest2 || true # Error on non-existing table
db_show_table sbtest3 || true # Error on non-existing table
db_show_table sbtest4 || true # Error on non-existing table
db_show_table sbtest5 || true # Error on non-existing table
db_show_table sbtest6 || true # Error on non-existing table
db_show_table sbtest7 || true # Error on non-existing table
db_show_table sbtest8 || true # Error on non-existing table

# Test --oltp-create-secondary=off
ARGS="--test=${SBTEST_INCDIR}/oltp_legacy/oltp.lua $DB_DRIVER_ARGS --oltp-tables-count=1"

sysbench $ARGS --oltp-create-secondary=off prepare

db_show_table sbtest1

sysbench $ARGS cleanup
