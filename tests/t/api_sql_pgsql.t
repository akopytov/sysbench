########################################################################
SQL Lua API + PostgreSQL tests
########################################################################

  $ . ${SBTEST_INCDIR}/pgsql_common.sh
  $ . ${SBTEST_INCDIR}/api_sql_common.sh
  drv:name() = pgsql
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
  1 foo 0.4
  2 nil 0.3
  nil bar 0.2
  nil nil 0.1
  --
  bar nil
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
  FATAL: Connection to database failed: could not translate host name "non-existing" to address: nodename nor servname provided, or not known
  
  connection creation failed
  --
  FATAL: PQexec() failed: 7 ERROR:  null value in column "a" violates not-null constraint
  DETAIL:  Failing row contains (null).
  
  FATAL: failed query was: INSERT INTO t VALUES (NULL)
  Got an error descriptor:
    sql_errno = \t0 (esc)
    connection = \t<sql_connection> (esc)
    query = \tINSERT INTO t VALUES (NULL) (esc)
    sql_state = \t23502 (esc)
    sql_errmsg = \tERROR:  null value in column "a" violates not-null constraint (esc)
  DETAIL:  Failing row contains (null).
  
  */api_sql.lua:*: SQL error, errno = 0, state = '23502': ERROR:  null value in column "a" violates not-null constraint (glob)
  DETAIL:  Failing row contains (null).
  
  FATAL: PQexec() failed: 7 ERROR:  value too long for type character(1)
  
  FATAL: failed query was: INSERT INTO t VALUES ('test')
  Got an error descriptor:
    sql_errno = \t0 (esc)
    connection = \t<sql_connection> (esc)
    query = \tINSERT INTO t VALUES ('test') (esc)
    sql_state = \t22001 (esc)
    sql_errmsg = \tERROR:  value too long for type character(1) (esc)
  
  */api_sql.lua:*: SQL error, errno = 0, state = '22001': ERROR:  value too long for type character(1) (glob)
  
  FATAL: PQexec() failed: 7 ERROR:  table "t" does not exist
  
  FATAL: failed query was: DROP TABLE t
  Got an error descriptor:
    sql_errno = \t0 (esc)
    connection = \t<sql_connection> (esc)
    query = \tDROP TABLE t (esc)
    sql_state = \t42P01 (esc)
    sql_errmsg = \tERROR:  table "t" does not exist (esc)
  
  */api_sql.lua:*: SQL error, errno = 0, state = '42P01': ERROR:  table "t" does not exist (glob)
  
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
