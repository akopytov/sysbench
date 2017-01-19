########################################################################
SQL Lua API + MySQL tests
########################################################################

  $ . ${SBTEST_INCDIR}/mysql_common.sh
  $ . ${SBTEST_INCDIR}/api_sql_common.sh
  drv:name() = mysql
  SQL types:
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
  --
  SQL error codes:
  NONE = 0
  FATAL = 2
  IGNORABLE = 1
  --
  FATAL: invalid database driver name: 'non-existing'
  failed to initialize the DB driver
  100
  --
  --
  nil bar 0.2
  nil nil 0.1
  1 foo 0.4
  2 nil 0.3
  --
  nil 2
  --
  <sql_bind>
  <sql_bind>
  Unsupported argument type: 8
  nil
  <sql_result>
  ALERT: attempt to fetch row from an empty result set
  <sql_result>
  ALERT: attempt to free an invalid result set
  db_free_results() failed
  db_free_results() failed
  --
  (last message repeated 1 times)
  ALERT: attempt to use an already closed connection
  [string "sysbench.sql.lua"]:*: Fatal SQL error, drv_errno = 0 (glob)
  ALERT: attempt to close an already closed connection
  --
  4
  301 400 0123456789 0123456789
  --
  FATAL: unable to connect to MySQL server on host 'non-existing', port 3306, aborting...
  FATAL: error 2005: Unknown MySQL server host 'non-existing' (0)
  connection creation failed
  1
  2
  3
  4
  5
  6
  7
  8
  9
  10
