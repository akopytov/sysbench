########################################################################
SQL Lua API + MySQL tests
########################################################################

  $ . ${SBTEST_INCDIR}/mysql_common.sh
  $ . ${SBTEST_INCDIR}/api_sql_common.sh
  drv:name() = mysql
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
  4
  301 400 0123456789 0123456789
  --
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
