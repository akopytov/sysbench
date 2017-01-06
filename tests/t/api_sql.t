########################################################################
SQL Lua API tests
########################################################################

  $ if [ -z "${SBTEST_MYSQL_ARGS:-}" ]
  > then
  >   exit 80
  > fi

  $ SB_ARGS="--verbosity=1 --test=$CRAMTMP/api_sql.lua --max-requests=1 --num-threads=1 --db-driver=mysql $SBTEST_MYSQL_ARGS"
  $ cat >$CRAMTMP/api_sql.lua <<EOF
  > function event(thread_id)
  >   db_query("CREATE TABLE t(a INT)")
  > 
  >   db_bulk_insert_init("INSERT INTO t VALUES")
  >   for i = 1,100 do
  >     db_bulk_insert_next(string.format("(%d)", i))
  >   end
  >   db_bulk_insert_done()
  > 
  >   db_connect()
  >   db_query("SELECT 1")
  >   db_disconnect()
  >   db_query("SELECT 1")
  >   db_connect()
  > 
  >   local stmt = db_prepare("UPDATE t SET a = a + ?")
  >   db_bind_param(stmt, {100})
  >   rs = db_execute(stmt)
  >   db_store_results(rs)
  >   db_free_results(rs)
  >   db_close(stmt)
  > 
  >   print("DB_ERROR_NONE = " .. DB_ERROR_NONE)
  >   print("DB_ERROR_RESTART_TRANSACTION = " .. DB_ERROR_RESTART_TRANSACTION)
  >   print("DB_ERROR_FAILED = " .. DB_ERROR_FAILED)
  > end
  > EOF

  $ mysql -uroot sbtest -Nse "DROP TABLE IF EXISTS t"

  $ sysbench $SB_ARGS run
  DB_ERROR_NONE = 0
  DB_ERROR_RESTART_TRANSACTION = 1
  DB_ERROR_FAILED = 3

  $ mysql -uroot sbtest -Nse "SHOW CREATE TABLE t\G"
  *************************** 1. row ***************************
  t
  CREATE TABLE `t` (
    `a` int(11) DEFAULT NULL
  ) * (glob)

  $ mysql -uroot sbtest -Nse "SELECT COUNT(DISTINCT a) FROM t"
  100

  $ mysql -uroot sbtest -Nse "SELECT MIN(a), MAX(a) FROM t\G"
  *************************** 1. row ***************************
  101
  200

  $ mysql -uroot sbtest -Nse "DROP TABLE t"
