########################################################################
# Common code for MySQL-specific tests
########################################################################
set -eu

if [ -z "${SBTEST_MYSQL_ARGS:-}" ]
then
  exit 80
fi

MYSQL_ARGS=`echo -n $SBTEST_MYSQL_ARGS | sed -- 's/--mysql-db=//;s/--mysql-/--/g'`

if echo -n $SBTEST_MYSQL_ARGS | grep -q -- --mysql-db; then
  MYSQL_DB=""
else
  MYSQL_DB=sbtest
fi

if echo -n $SBTEST_MYSQL_ARGS | grep -q -- --mysql-user; then
  MYSQL_USER=
else
  MYSQL_USER=-usbtest
fi

function db_show_table() {
  mysql $MYSQL_ARGS $MYSQL_USER $MYSQL_DB -Nse "SHOW CREATE TABLE $1\G"
}

DB_DRIVER_ARGS="--db-driver=mysql $SBTEST_MYSQL_ARGS"

DB_NON_EXISTING="--mysql-host=non-existing --mysql-socket="
