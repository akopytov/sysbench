########################################################################
SQL Lua API tests
########################################################################

  $ if [ -z "${SBTEST_MYSQL_ARGS:-}" ]
  > then
  >   exit 80
  > fi

  $ SB_ARGS="--verbosity=1 --test=$CRAMTMP/api_sql.lua --max-requests=1 --num-threads=1 --db-driver=mysql $SBTEST_MYSQL_ARGS"
  $ cat >$CRAMTMP/api_sql.lua <<EOF
  > function thread_init()
  >   drv = sysbench.sql.driver()
  >   con = drv:connect()
  > end
  > 
  > function event()
  >   for k,v in pairs(sysbench.sql.type) do print(k .. " = " .. v) end
  >   print()
  >   
  >   con:query("DROP TABLE IF EXISTS t")
  >   con:query("CREATE TABLE t(a INT)")
  >   con:bulk_insert_init("INSERT INTO t VALUES")
  >   for i = 1,100 do
  >     con:bulk_insert_next(string.format("(%d)", i))
  >   end
  >   con:bulk_insert_done()
  > 
  >   con2 = drv:connect()
  >   con:query("SELECT CONNECTION_ID()")
  >   con2:query("SELECT CONNECTION_ID()")
  >   con2:disconnect()
  > 
  >   con:query("SELECT 1")
  >   con:disconnect()
  >   con:query("SELECT 1")
  >   
  >   con = drv:connect()
  >   con:query[[UPDATE t SET a = a + 100]]
  >   
  >   -- local stmt = con:prepare("UPDATE t SET a = a + ?")
  >   -- stmt:bind_param({{sysbench.sql.type.INT}})
  >   -- rs = stmt:execute()
  >   -- rs:free_results()
  >   -- stmt:close()
  > end
  > EOF

  $ mysql -uroot sbtest -Nse "DROP TABLE IF EXISTS t"

  $ sysbench $SB_ARGS run
  NONE = 0
  INT = 3
  CHAR = 11
  VARCHAR = 12
  TIMESTAMP = 10
  TIME = 7
  FLOAT = 5
  TINYINT = 1
  BIGINT = 4
  SMALLINT = 2
  DATE = 8
  DATETIME = 9
  DOUBLE = 6
  
  ALERT: attempt to use an already closed connection

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
