########################################################################
SQL Lua API + PostgreSQL tests
########################################################################

  $ . ${SBTEST_INCDIR}/pgsql_common.sh
  $ . ${SBTEST_INCDIR}/api_sql_common.sh
  drv:name() = pgsql
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
  
  NOTICE:  table "t" does not exist, skipping
  100
  --
  NOTICE:  table "t2" does not exist, skipping
  --
  1 foo 0.4
  2 nil 0.3
  nil bar 0.2
  nil nil 0.1
  --
  bar nil
  --
  4
  301 400 0123456789 0123456789
  --
  NOTICE:  table "t" does not exist, skipping
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
