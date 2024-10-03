#ifdef STDC_HEADERS
# include <stdio.h>
#endif
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <sql.h>
#include <sqlext.h>

#include "sb_options.h"
#include "db_driver.h"

#define xfree(ptr) ({ if (ptr) free((void *)ptr); ptr = NULL; })

#define DRIVER_NAME "ODBC Driver 18 for SQL Server"

#define DRV_BEGIN 1
#define DRV_COMMIT 2
#define DRV_ROLLBACK 3

/* SQL Server driver arguments */

static sb_arg_t sqlserver_drv_args[] =
{
  SB_OPT("sqlserver-host", "SQL Server server host", "localhost", STRING),
  SB_OPT("sqlserver-port", "SQL Server server port", "1433", INT),
  SB_OPT("sqlserver-user", "SQL Server user", "sbtest", STRING),
  SB_OPT("sqlserver-password", "SQL Server password", "", STRING),
  SB_OPT("sqlserver-db", "SQL Server database name", "sbtest", STRING),
  SB_OPT("sqlserver-ignore-errors", "list of errors to ignore, or \"all\"",
          "1205,2627", LIST), // 1205: deadlock, 2627: unique primary key
  SB_OPT("sqlserver-dry-run", "Dry run, pretend all calls are successful" 
         "without executing them", "off", BOOL),
  SB_OPT_END
};

typedef struct
{
  char               *host;
  char               *port;
  char               *user;
  char               *password;
  char               *db;
  sb_list_t          *ignored_errors;
  bool                dry_run;
} sqlserver_drv_args_t;

/* Structure used for DB-to-SQLServer bind types map */
typedef struct
{
  db_bind_type_t   db_type;
  int              my_type;
  int              c_type;
} db_sqlserver_bind_map_t;

/* DB-to-SQLServer bind types map */
db_sqlserver_bind_map_t db_sqlserver_bind_map[] =
{
  {DB_TYPE_TINYINT,   SQL_TINYINT,    SQL_C_TINYINT},
  {DB_TYPE_SMALLINT,  SQL_SMALLINT,   SQL_C_SSHORT},
  {DB_TYPE_INT,       SQL_INTEGER,    SQL_C_SLONG},
  {DB_TYPE_BIGINT,    SQL_BIGINT,     SQL_C_SBIGINT},
  {DB_TYPE_FLOAT,     SQL_FLOAT,      SQL_C_FLOAT},
  {DB_TYPE_DOUBLE,    SQL_DOUBLE,     SQL_C_DOUBLE},
  {DB_TYPE_DATETIME,  SQL_DATETIME,   SQL_C_TYPE_TIME},
  {DB_TYPE_TIMESTAMP, SQL_TIMESTAMP,  SQL_C_TYPE_TIMESTAMP},
  {DB_TYPE_CHAR,      SQL_CHAR,       SQL_C_CHAR},
  {DB_TYPE_VARCHAR,   SQL_VARCHAR,    SQL_C_CHAR},
  {DB_TYPE_NONE,      0,              0}
};

typedef struct
{
  int sql_type;
  int c_type;
  unsigned long length;
  void *buffer;
  unsigned long buffer_len;
  int ind_ptr;
} db_sqlserver_bind_t;

/* SQL Server driver capabilities */
static drv_caps_t sqlserver_drv_caps =
{
  1,    /* multi_rows_insert */
  1,    /* prepared_statements */
  0,    /* auto_increment */
  0,    /* needs_commit */
  0,    /* serial */
  0,    /* unsigned int */
};

static char use_ps; /* whether server-side prepared statemens should be used */

typedef struct
{
  SQLHENV   henv;
  SQLHDBC   hdbc;
} db_sqlserv_conn_t;

static sqlserver_drv_args_t args;

static int sqlserver_drv_init(void);
static int sqlserver_drv_describe(drv_caps_t *);
static int sqlserver_drv_connect(db_conn_t *);
static int sqlserver_drv_disconnect(db_conn_t *);
static int sqlserver_drv_reconnect(db_conn_t *);
static int sqlserver_drv_prepare(db_stmt_t *, const char *, size_t);
static int sqlserver_drv_bind_param(db_stmt_t *, db_bind_t *, size_t);
static int sqlserver_drv_bind_result(db_stmt_t *, db_bind_t *, size_t);
static db_error_t sqlserver_drv_execute(db_stmt_t *, db_result_t *);
static db_error_t sqlserver_drv_query(db_conn_t *, const char *, size_t, 
                                      db_result_t *);
static int sqlserver_drv_free_results(db_result_t *);
static int sqlserver_drv_close(db_stmt_t *);
static int sqlserver_drv_done(void);

static int check_transaction(const char *);
static SQLRETURN drv_transact(SQLHDBC, int);
static int convert_to_sqlserver_bind(db_sqlserver_bind_t *, db_bind_t *);

static db_driver_t sqlserver_driver =
{
  .sname = "sqlserver",
  .lname = "SQL Server driver",
  .args = sqlserver_drv_args,
  .ops =
  {
    .init = sqlserver_drv_init,
    .describe = sqlserver_drv_describe,
    .connect = sqlserver_drv_connect,
    .disconnect = sqlserver_drv_disconnect,
    .reconnect = sqlserver_drv_reconnect,
    .prepare = sqlserver_drv_prepare,
    .bind_param = sqlserver_drv_bind_param,
    .bind_result = sqlserver_drv_bind_result,
    .execute = sqlserver_drv_execute,
    .free_results = sqlserver_drv_free_results,
    .close = sqlserver_drv_close,
    .query = sqlserver_drv_query,
    .done = sqlserver_drv_done,
  }
};

/*
 * Extracts and prints driver error for non-connection ODBC API calls.
 * 
 * This function assumes there IS an error 
 * and will always return DB_ERROR_FATAL.
 */
static db_error_t check_error(char *fn, SQLHANDLE handle, SQLSMALLINT type)
{
  SQLINTEGER i = 0;
  SQLINTEGER NativeError;
  SQLCHAR SQLState[7];
  SQLCHAR MessageText[256];
  SQLSMALLINT TextLength;
  SQLRETURN ret;

  log_text(LOG_DEBUG, "The driver ran into an error at %s\n", fn);
  do {
    ret = SQLGetDiagRec(type, handle, ++i, SQLState, &NativeError,
                        MessageText, sizeof(MessageText), &TextLength);
    if (SQL_SUCCEEDED(ret)) {
      log_text(LOG_DEBUG, "%s:%ld:%ld:%s\n",
              SQLState, (long) i, (long) NativeError, MessageText);
    }
  } while (ret == SQL_SUCCESS);
  
  return DB_ERROR_FATAL;
}

/* 
 * Extracts and prints driver error for connections.
 * 
 * Since this is only for connections, SQLHANDLE should 
 * always be a SQL_HANDLE_STMT. Checks if the connection error
 * should be ignored based on args.ignored_errors
 */
static db_error_t check_conn_error(db_conn_t *sb_con, char *fn, 
                                   SQLHANDLE handle, 
                                   sb_counter_type_t *counter)
{
  SQLINTEGER i = 0;
  SQLINTEGER NativeError;
  SQLCHAR SQLState[7];
  SQLCHAR MessageText[256];
  SQLSMALLINT TextLength;
  SQLSMALLINT type = SQL_HANDLE_STMT;
  SQLRETURN ret;

  log_text(LOG_DEBUG, "The driver ran into an error at %s\n", fn);
  
  ret = SQLGetDiagRec(type, handle, ++i, SQLState, &NativeError,
                      MessageText, sizeof(MessageText), &TextLength);
  if (SQL_SUCCEEDED(ret)) {
    log_text(LOG_DEBUG, "%s:%ld:%ld:%s\n",
                SQLState, (long) i, (long) NativeError, MessageText);

    xfree(sb_con->sql_state);
    xfree(sb_con->sql_errmsg);

    sb_con->sql_errno = NativeError;
    sb_con->sql_state = strdup((char *)SQLState);
    sb_con->sql_errmsg = strdup((char *)MessageText);

    // Check if we can ignore the error
    sb_list_item_t *pos;
    int tmp;

    SB_LIST_FOR_EACH(pos, args.ignored_errors) {
      const char *val = SB_LIST_ENTRY(pos, value_t, listitem)->data;

      tmp = atoi(val);

      if (NativeError == tmp || !strcmp(val, "all")) {
        log_text(LOG_DEBUG, "Ignoring %u\n", NativeError);

        // Reconnect for communication failures
        if (!strcmp("08S01", (char *)SQLState) 
              || !strcmp("08007", (char *)SQLState)) {
          *counter = SB_CNT_RECONNECT;
          return sqlserver_drv_reconnect(sb_con);
        }
        
        *counter = SB_CNT_ERROR;
        return DB_ERROR_IGNORABLE;
      }
    }

    *counter = SB_CNT_ERROR;
    return DB_ERROR_FATAL;
  }

  return DB_ERROR_FATAL;
}

/*
 * Returns transaction type given a query
 */
int check_transaction(const char *query)
{
    if (!strncmp(query, "BEGIN T", 7)) {
      // SQL Server ODBC should not use BEGIN TRAN
    // Completely ignore this query, but we must turn off
    // auto commit to support batch statements
    return DRV_BEGIN;
  } else if (!strcmp(query, "COMMIT")) {
    return DRV_COMMIT;
  } else if (!strcmp(query, "ROLLBACK")) {
    return DRV_ROLLBACK;
  }
  return 0;
}

/*
 * BEGIN TRANSACTION, COMMIT TRANSACTION, and ROLLBACK shouldn't be used
 * with SQL Server ODBC. Instead, we should turn off autocommits, and use
 * SQLEndTran to Commit a block of statements.
 */
SQLRETURN drv_transact(SQLHDBC hdbc, int type)
{
  SQLRETURN ret;

  switch (type) {
  case DRV_BEGIN:
    // Do nothing
    
    ret = SQL_SUCCESS;
    break;
  case DRV_COMMIT:
    ret = SQLEndTran(SQL_HANDLE_DBC, hdbc, SQL_COMMIT);
    break;
  case DRV_ROLLBACK:
    ret = SQLEndTran(SQL_HANDLE_DBC, hdbc, SQL_ROLLBACK);
    break;
  default:
    ret = 0;
    break;
  }

  return ret;
}

/* Register SQLServer driver */
int register_driver_sqlserver(sb_list_t *drivers)
{
  SB_LIST_ADD_TAIL(&sqlserver_driver.listitem, drivers);
  return 0;
}

int sqlserver_drv_init(void) 
{
  args.host           = sb_get_value_string("sqlserver-host");
  args.port           = sb_get_value_string("sqlserver-port");
  args.user           = sb_get_value_string("sqlserver-user");
  args.password       = sb_get_value_string("sqlserver-password");
  args.db             = sb_get_value_string("sqlserver-db");
  args.ignored_errors = sb_get_value_list("sqlserver-ignore-errors");
  args.dry_run        = sb_get_value_flag("sqlserver-dry-run");

  use_ps = 0;
  sqlserver_drv_caps.prepared_statements = 1;
  if (db_globals.ps_mode != DB_PS_MODE_DISABLE)
    use_ps = 1;
   
  return 0;
}

/*
 * Define this driver's capabilities
 */
int sqlserver_drv_describe(drv_caps_t *caps) 
{
  *caps = sqlserver_drv_caps;
  // We may need to connect to the server and get the capabilities
  // but we don't do that yet.
  return 0;
}

/*
 * Connect to the SQL database and store a driver specific pointer
 * in sb_conn.
 */
int sqlserver_drv_connect(db_conn_t *sb_conn)
{
  if (args.dry_run)
   return 0;

  db_sqlserv_conn_t *conn = malloc(sizeof(db_sqlserv_conn_t));
  if (conn == NULL)
    return 1;
  
  conn->henv = SQL_NULL_HENV;
  conn->hdbc = SQL_NULL_HDBC;
  SQLRETURN ret;

  // Allocate environment handle
  ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &conn->henv);
 
  if (!SQL_SUCCEEDED(ret)) {
    log_text(LOG_FATAL, "Unable to allocate SQL Server Env Handle.");
    return 1;
  }

  // Set the ODBC version env attribute
  ret = SQLSetEnvAttr(conn->henv, SQL_ATTR_ODBC_VERSION, 
    (SQLPOINTER *)SQL_OV_ODBC3, 0);

  if (!SQL_SUCCEEDED(ret)) {
    log_text(LOG_FATAL, "Unable to set env attr.");
    return 1;
  }

  // Allocate connection handle
  ret = SQLAllocHandle(SQL_HANDLE_DBC, conn->henv, &conn->hdbc);

  if (!SQL_SUCCEEDED(ret)) {
    log_text(LOG_FATAL, "Unable to allocate SQL Server Conn handle.");
    return 1;
  }

  // Turn off autocommits, since most workloads will be prepared and ODBC
  // will need explicit SQLEndTran's.
  ret = SQLSetConnectAttr(conn->hdbc, SQL_ATTR_AUTOCOMMIT, 
    (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);

  if (!SQL_SUCCEEDED(ret)) {
    log_text(LOG_FATAL, "Error in turning off autocommit.");
    return 1;
  }

  // Connect!
  char conn_str[256];
  // Updated the connection string to use Managed Identity
  if (strcmp(args.user, "ms_entra") == 0) {
    snprintf(conn_str, sizeof(conn_str), 
        "Driver=" DRIVER_NAME ";"
        "Server=%s,%s;Authentication=ActiveDirectoryMsi;UID=%s;Database=%s;Encrypt=no;TrustServerCertificate=yes;", 
        args.host, args.port, args.password, args.db);  // Using password as UID for ms_entra
  } else {
    snprintf(conn_str, sizeof(conn_str), 
        "Driver=" DRIVER_NAME ";"
        "Server=%s,%s;Uid=%s;Pwd=%s;database=%s;", 
        args.host, args.port, args.user, args.password, args.db);
  }
   ret = SQLDriverConnect(conn->hdbc, NULL, (SQLCHAR *)conn_str, SQL_NTS, 
    NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

  if (!SQL_SUCCEEDED(ret)) {
    log_text(LOG_FATAL, "Connection String: %s", conn_str);
    log_text(LOG_FATAL, "Connection to database failed!");
    SQLFreeHandle(SQL_HANDLE_DBC, conn->hdbc);
    SQLFreeHandle(SQL_HANDLE_ENV, conn->henv);
    xfree(conn);
    return 1;
  }
  
  sb_conn->ptr = conn;
  sb_conn->sql_errmsg = NULL;
  sb_conn->sql_state = NULL;
  //log_text(LOG_DEBUG, "Connected!");
    return 0;
}

/*
 * Disconnect from SQL server and free resources
 */
int sqlserver_drv_disconnect(db_conn_t *sb_conn)
{
  db_sqlserv_conn_t *conn = sb_conn->ptr;

  xfree(sb_conn->sql_state);
  xfree(sb_conn->sql_errmsg);

  if (args.dry_run)
    return 0;

  if (conn != NULL) {
    SQLDisconnect(conn->hdbc);
    SQLFreeHandle(SQL_HANDLE_DBC, conn->hdbc);
    SQLFreeHandle(SQL_HANDLE_ENV, conn->henv);
    xfree(conn);
  }

  log_text(LOG_DEBUG, "Disconnected.");
  return 0;
}

int sqlserver_drv_reconnect(db_conn_t *sb_conn)
{
  log_text(LOG_DEBUG, "Reconnecting");

  if (sqlserver_drv_disconnect(sb_conn))
    return DB_ERROR_FATAL;

  while (sqlserver_drv_connect(sb_conn))
  {
    if (sb_globals.error)
      return DB_ERROR_FATAL;

    usleep(1000);
  }

  //log_text(LOG_DEBUG, "Reconnected");

  return DB_ERROR_IGNORABLE;
}

/*
 * Set up a SQL Prepared Statement (PS)
 */
int sqlserver_drv_prepare(db_stmt_t *stmt, const char *query, size_t len)
{
  if (args.dry_run)
    return 0;
  db_sqlserv_conn_t *conn = (db_sqlserv_conn_t *)stmt->connection->ptr;
  SQLHSTMT hstmt = SQL_NULL_HSTMT;
  SQLRETURN ret;
    if (use_ps)
  {
     // Ignore BEGIN TRAN, COMMIT, and ROLLBACK queries
    if (check_transaction(query)) {
      stmt->query = strdup(query);
      stmt->counter = SB_CNT_OTHER;
      return 0;
    }
  
    SQLAllocHandle(SQL_HANDLE_STMT, conn->hdbc, &hstmt);
    ret = SQLPrepare(hstmt, (SQLCHAR *)query, len);
    if (!SQL_SUCCEEDED(ret))
      return check_error("sqlserver_drv_prepare", hstmt, SQL_HANDLE_STMT);
 
    SQLSMALLINT cols;
    SQLNumResultCols(hstmt, &cols);
    
    stmt->counter = (cols > 0) ? SB_CNT_READ : SB_CNT_WRITE;
    stmt->ptr = hstmt;
    stmt->query = strdup(query);
    return 0;
  }
  // Use client-side PS
  stmt->emulated = 1;
  stmt->query = strdup(query);
  return 0;
}

/*
 * Bind parameters to ready a prepared statement.
 * 
 * stmt:    Relevant prepared statement
 * params:  List of parameters to bind
 * len:     Number of parameters to bind
 */
int sqlserver_drv_bind_param(db_stmt_t *stmt, db_bind_t *params, size_t len)
{
  if (args.dry_run)
    return 0;

  if (stmt->emulated)
  {
    // Use emulation
    if (stmt->bound_param != NULL)
    free(stmt->bound_param);
    stmt->bound_param = (db_bind_t *)malloc(len * sizeof(db_bind_t));
    if (stmt->bound_param == NULL)
      return 1;
    memcpy(stmt->bound_param, params, len * sizeof(db_bind_t));
    stmt->bound_param_len = len;
    return 0;
  }

  if (stmt->ptr == NULL)
    return 1;

  db_sqlserver_bind_t *sqlserver_types = calloc(len, 
    sizeof(db_sqlserver_bind_t));

  SQLRETURN ret;

  /* Convert sysbench data types to SQL Server ones */
  for (size_t i = 0; i < len; i++) {
    if (convert_to_sqlserver_bind(&sqlserver_types[i], &params[i])) {
      xfree(sqlserver_types);
      return 1;
    }
  }

  /* Bind Params */
  for (size_t i = 0; i < len; i++) {
    ret = SQLBindParameter(stmt->ptr, i+1, SQL_PARAM_INPUT, 
            sqlserver_types[i].c_type, 
            sqlserver_types[i].sql_type,
            sqlserver_types[i].length,
            0,
            params[i].buffer, 
            params[i].max_len,
            0);
    if (!SQL_SUCCEEDED(ret)) {
      return check_error("sqlserver_drv_bind_param", stmt->ptr, 
        SQL_HANDLE_STMT);
    }
  }

  xfree(sqlserver_types);
  return 0;
}

int sqlserver_drv_bind_result(db_stmt_t *stmt, db_bind_t *params, size_t len)
{
  if (args.dry_run)
    return 0;
  // Unused
  (void)stmt;
  (void)params;
  (void)len;
  return 0;
}

/*
 * Execute a prepared statement
 */
db_error_t sqlserver_drv_execute(db_stmt_t *stmt, db_result_t *rs)
{
  if (args.dry_run)
    return DB_ERROR_NONE;

  stmt->connection->sql_errno = 0;
  xfree(stmt->connection->sql_state);
  xfree(stmt->connection->sql_errmsg);

  int tran_type;
  SQLRETURN ret;
  SQLHSTMT  hstmt;
  SQLLEN    num_rows;
  db_sqlserv_conn_t *db_conn = stmt->connection->ptr;

  if (!stmt->emulated) {
    hstmt = stmt->ptr;
    char *query = stmt->query;

    //log_text(LOG_DEBUG, "ExecuteFirstQuery: %s\n", query);
    
    if ((tran_type = check_transaction(query))) {
      ret = drv_transact(db_conn->hdbc, tran_type);
      if (!SQL_SUCCEEDED(ret)) {
        return check_error("drv_transact", db_conn->hdbc, SQL_HANDLE_DBC);
      }
      rs->counter = SB_CNT_OTHER;
      return DB_ERROR_NONE;
    }
    
    ret = SQLExecute(hstmt);
    // If SQLExecute executes a searched update, insert, or delete statement
    // that does not affect any rows at the data source, the call to 
    // SQLExecute returns SQL_NO_DATA. We will consider this a success.
    if (ret == SQL_NO_DATA) ret = SQL_SUCCESS;
    else if (!SQL_SUCCEEDED(ret)) {
      return check_conn_error(stmt->connection, "sqlserver_drv_execute", 
        hstmt, &rs->counter);
    }

    if (stmt->counter != SB_CNT_READ) {
      SQLRowCount(hstmt, &num_rows);
      rs->counter = (num_rows > 0) ? SB_CNT_WRITE : SB_CNT_OTHER;
      rs->nrows = num_rows;
    } else {
      rs->counter = stmt->counter;
     //rs->counter = NULL;

      // TODO: The below three lines are responsible for fetching data. For perf reasons,
      // I'm leaving this in. But in the future I need to store the results somewhere.
      do {
        while (SQLFetch(hstmt) != SQL_NO_DATA) {}
      } while (SQLMoreResults(hstmt) != SQL_NO_DATA);
    }

    return DB_ERROR_NONE;
  }
  
  /* Use emulation */
  /* Build the actual query string from parameters list */
  char           *buf = NULL;
  unsigned int    buflen = 0;
  char            need_realloc = 1;
  unsigned int    i, j, vcnt;
  int             n;

  vcnt = 0; // Count of var/params. Used to keep track of bound_param index

  // 'i' as an index for query characters
  // 'j' as an index for the buffer we're filling
  for (i = 0, j = 0; stmt->query[i] != '\0'; i++)
  {
  again:
    if (j+1 >= buflen || need_realloc)
    {
      buflen = (buflen > 0) ? buflen * 2 : 256;
      buf = realloc(buf, buflen);
      if (buf == NULL)
      {
        return DB_ERROR_FATAL;
      }
      need_realloc = 0;
    }

    if (stmt->query[i] != '?')
    {
      buf[j++] = stmt->query[i];
      continue;
    }

    n = db_print_value(stmt->bound_param + vcnt, buf + j, (int)(buflen - j));
    if (n < 0)
    {
      need_realloc = 1;
      goto again;
    }
    j += (unsigned int)n;
    vcnt++;
  }
  buf[j] = '\0';

    db_error_t rc = sqlserver_drv_query(stmt->connection, buf, j, rs);

  xfree(buf);

  return rc;
}

int sqlserver_drv_free_results(db_result_t *result)
{
  if (args.dry_run)
    return 0;

  if (result->ptr != NULL) {
    SQLHSTMT hstmt = result->ptr;
    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    result->ptr = NULL;
  }

  return 0;
}

/*
 * Execute a non-prepared statement
 */
db_error_t sqlserver_drv_query(db_conn_t *sb_conn, const char *query,
                          size_t len, db_result_t *rs)
{
  if (args.dry_run)
    return DB_ERROR_NONE;

  sb_conn->sql_errno = 0;
  xfree(sb_conn->sql_state);
  xfree(sb_conn->sql_errmsg);

  int tran_type;
  db_sqlserv_conn_t *conn = sb_conn->ptr;
  SQLHSTMT statement = SQL_NULL_HSTMT;
  SQLRETURN ret;
  (void)len; // Unused
  
    if ((tran_type = check_transaction(query))) {
    ret = drv_transact(conn->hdbc, tran_type);
    if (!SQL_SUCCEEDED(ret)) {
      return check_error("drv_transact", conn->hdbc, SQL_HANDLE_DBC);
   }
   rs->counter = SB_CNT_OTHER;
   return DB_ERROR_NONE;
  }

  SQLAllocHandle(SQL_HANDLE_STMT, conn->hdbc, &statement);
  ret = SQLExecDirect(statement, (SQLCHAR *)query, SQL_NTS);
  
  // Ignore SQL_NO_DATA
  if (ret == SQL_NO_DATA) ret = SQL_SUCCESS;

  else if (!SQL_SUCCEEDED(ret)) {
    return check_conn_error(sb_conn, "sqlserver_drv_query", 
                        statement, &rs->counter);
  }

  SQLSMALLINT cols;
  SQLLEN rows;
  SQLNumResultCols(statement, &cols);
  SQLRowCount(statement, &rows);

  rs->nrows = rows;
  rs->nfields = cols;

  if (cols == 0) { // cols == 0 means no result set
    rs->counter = (rows > 0) ? SB_CNT_WRITE : SB_CNT_OTHER;
  } else {
    rs->counter = SB_CNT_READ;
  }
  
  SQLFreeHandle(SQL_HANDLE_STMT, statement);

  // Sysbench expects this drv_query to auto commit, so let's commit now.
  // Note: Although we should increment rs counter with SB_CNT_OTHER,
  // we can't because it will conflict with the actual query.
  ret = SQLEndTran(SQL_HANDLE_DBC, conn->hdbc, SQL_COMMIT);

  if (!SQL_SUCCEEDED(ret)) {
    return check_conn_error(sb_conn, "sqlserver_drv_query", 
                        statement, &rs->counter);
  }

  return DB_ERROR_NONE;
}

/* 
 * Close prepared statement and free all resources associated with it 
 */
int sqlserver_drv_close(db_stmt_t *stmt)
{
  if (args.dry_run)
    return 0;

  xfree(stmt->query);

  if (stmt->ptr == NULL) {
    // We're trying to close an invalid statement!
    return 1;
  }

  SQLFreeHandle(SQL_HANDLE_STMT, stmt->ptr);
  stmt->ptr = NULL;

  return 0;
}

/*
 * Uninitialize the driver
 */
int sqlserver_drv_done(void)
{
  if (args.dry_run)
    return 0;

  return 0;
}

/*
 * Create a sqlserver bind (my_bind) from a sysbench bind (bind)
 */
static int convert_to_sqlserver_bind(db_sqlserver_bind_t *my_bind, 
                                      db_bind_t *bind)
{
  // Iterate through type map to set SQL and C types needed for queries
  for (int i = 0; db_sqlserver_bind_map[i].db_type != DB_TYPE_NONE; i++) {
    if (db_sqlserver_bind_map[i].db_type == bind->type) {
      my_bind->sql_type = db_sqlserver_bind_map[i].my_type;
      my_bind->c_type   = db_sqlserver_bind_map[i].c_type;
      goto success;
    }
  }

  // Didn't find a mapping. Return error
  return -1;

success:
  my_bind->buffer     = bind->buffer;
  my_bind->buffer_len = bind->max_len;
  my_bind->length     = *bind->data_len;

  return 0;
}