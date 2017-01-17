########################################################################
# Common code for MySQL-specific tests
########################################################################
set -eu

if [ -z "${SBTEST_MYSQL_ARGS:-}" ]
then
  exit 80
fi

function db_show_table() {
  mysql -uroot sbtest -Nse "SHOW CREATE TABLE $1\G"
}

DB_DRIVER_ARGS="--db-driver=mysql $SBTEST_MYSQL_ARGS"
