#!/usr/bin/env bash
#
#########################################################################
# Common code for DB driver tests
# Variables:
#   DB_DRIVER_ARGS -- extra driver-specific arguments to pass to sysbench
#########################################################################

set -eu

cat >test.lua <<EOF
function event()
  db_query("SELECT 1")
end
EOF

sysbench test.lua \
         --events=10 \
         --threads=2 \
         $DB_DRIVER_ARGS \
         run
