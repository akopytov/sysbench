########################################################################
SQL Lua API + MySQL tests
########################################################################

  $ . ${SBTEST_INCDIR}/mysql_common.sh
  $ . ${SBTEST_INCDIR}/api_sql_common.sh
  drv:name() = mysql
  SQL types:
  {
    BIGINT = 4,
    CHAR = 11,
    DATE = 8,
    DATETIME = 9,
    DOUBLE = 6,
    FLOAT = 5,
    INT = 3,
    NONE = 0,
    SMALLINT = 2,
    TIME = 7,
    TIMESTAMP = 10,
    TINYINT = 1,
    VARCHAR = 12
  }
  --
  SQL error codes:
  {
    FATAL = 2,
    IGNORABLE = 1,
    NONE = 0
  }
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
  FATAL: mysql_stmt_prepare() failed
  FATAL: MySQL error: 1146 "Table 'sbtest.nonexisting' doesn't exist"
  SQL API error
  --
  <sql_param>
  <sql_param>
  Unsupported argument type: 8
  nil
  <sql_result>
  <sql_result>
  ALERT: attempt to free an invalid result set
  db_free_results() failed
  db_free_results() failed
  --
  (last message repeated 1 times)
  ALERT: attempt to use an already closed connection
  */api_sql.lua:*: SQL API error (glob)
  ALERT: attempt to close an already closed connection
  --
  4
  301 400 0123456789 0123456789
  --
  1
  2
  --
  reconnects = 1
  FATAL: unable to connect to MySQL server on host 'non-existing', port 3306, aborting...
  FATAL: error 2005: Unknown MySQL server host 'non-existing' (0)
  connection creation failed
  --
  FATAL: mysql_drv_query() returned error 1048 (Column 'a' cannot be null) for query 'INSERT INTO t VALUES (NULL)'
  Got an error descriptor:
  {
    connection = <sql_connection>,
    query = "INSERT INTO t VALUES (NULL)",
    sql_errmsg = "Column 'a' cannot be null",
    sql_errno = 1048,
    sql_state = "23000"
  }
  */api_sql.lua:*: SQL error, errno = 1048, state = '23000': Column 'a' cannot be null (glob)
  FATAL: mysql_drv_query() returned error 1406 (Data too long for column 'a' at row 1) for query 'INSERT INTO t VALUES ('test')'
  Got an error descriptor:
  {
    connection = <sql_connection>,
    query = "INSERT INTO t VALUES ('test')",
    sql_errmsg = "Data too long for column 'a' at row 1",
    sql_errno = 1406,
    sql_state = "22001"
  }
  */api_sql.lua:*: SQL error, errno = 1406, state = '22001': Data too long for column 'a' at row 1 (glob)
  FATAL: mysql_drv_query() returned error 1051 (Unknown table '*t') for query 'DROP TABLE t' (glob)
  Got an error descriptor:
  {
    connection = <sql_connection>,
    query = "DROP TABLE t",
    sql_errmsg = "Unknown table 'sbtest.t'",
    sql_errno = 1051,
    sql_state = "42S02"
  }
  */api_sql.lua:*: SQL error, errno = 1051, state = '42S02': Unknown table '*t' (glob)
  --
  ########################################################################
  # Multiple connections test
  ########################################################################
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
  ########################################################################
  # Incorrect bulk API usage
  ########################################################################
  ALERT: attempt to call bulk_insert_next() before bulk_insert_init()
  */api_sql.lua:*: db_bulk_insert_next() failed (glob)
  ########################################################################
  # query_row() with an empty result set
  ########################################################################
  nil
  ########################################################################
  # GH-282: Mysql's fetch_row() is broken
  ########################################################################
  1
  2
