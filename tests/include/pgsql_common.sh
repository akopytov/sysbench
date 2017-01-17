########################################################################
# Common code for PostgreSQL-specific tests
########################################################################
set -eu

if [ -z "${SBTEST_PGSQL_ARGS:-}" ]
then
  exit 80
fi

function db_show_table() {
  psql -c "\d+ $1" sbtest
}

DB_DRIVER_ARGS="--db-driver=pgsql $SBTEST_PGSQL_ARGS"
