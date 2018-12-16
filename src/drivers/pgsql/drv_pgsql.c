/* Copyright (C) 2005 MySQL AB
   Copyright (C) 2005-2017 Alexey Kopytov <akopytov@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <libpq-fe.h>

#include "sb_options.h"
#include "db_driver.h"
#include "sb_rand.h"

#define xfree(ptr) ({ if (ptr) free((void *)ptr); ptr = NULL; })

/* Maximum length of text representation of bind parameters */
#define MAX_PARAM_LENGTH 256UL

/* PostgreSQL driver arguments */

static sb_arg_t pgsql_drv_args[] =
{
  SB_OPT("pgsql-host", "PostgreSQL server host", "localhost", STRING),
  SB_OPT("pgsql-port", "PostgreSQL server port", "5432", INT),
  SB_OPT("pgsql-user", "PostgreSQL user", "sbtest", STRING),
  SB_OPT("pgsql-password", "PostgreSQL password", "", STRING),
  SB_OPT("pgsql-db", "PostgreSQL database name", "sbtest", STRING),

  SB_OPT_END
};

typedef struct
{
  char               *host;
  char               *port;
  char               *user;
  char               *password;
  char               *db;
} pgsql_drv_args_t;

/* Structure used for DB-to-PgSQL bind types map */

typedef struct
{
  db_bind_type_t   db_type;
  int              pg_type;
} db_pgsql_bind_map_t;

/* DB-to-PgSQL bind types map */
db_pgsql_bind_map_t db_pgsql_bind_map[] =
{
  {DB_TYPE_TINYINT,   0},
  {DB_TYPE_SMALLINT,  21},
  {DB_TYPE_INT,       23},
  {DB_TYPE_BIGINT,    20},
  {DB_TYPE_FLOAT,     700},
  {DB_TYPE_DOUBLE,    701},
  {DB_TYPE_DATETIME,  0},
  {DB_TYPE_TIMESTAMP, 1114},
  {DB_TYPE_CHAR,      18},
  {DB_TYPE_VARCHAR,   1043},
  {DB_TYPE_NONE,      0}
};

/* PgSQL driver capabilities */

static drv_caps_t pgsql_drv_caps =
{
  1,    /* multi_rows_insert */
  1,    /* prepared_statements */
  0,    /* auto_increment */
  0,    /* needs_commit */
  1,    /* serial */
  0,    /* unsigned int */
};

/* Describes the PostgreSQL prepared statement */
typedef struct pg_stmt
{
  char     *name;
  int      prepared;
  int      nparams;
  Oid      *ptypes;
  char     **pvalues;
} pg_stmt_t;

static pgsql_drv_args_t args;          /* driver args */

static char use_ps; /* whether server-side prepared statemens should be used */

/* PgSQL driver operations */

static int pgsql_drv_init(void);
static int pgsql_drv_describe(drv_caps_t *);
static int pgsql_drv_connect(db_conn_t *);
static int pgsql_drv_disconnect(db_conn_t *);
static int pgsql_drv_prepare(db_stmt_t *, const char *, size_t);
static int pgsql_drv_bind_param(db_stmt_t *, db_bind_t *, size_t);
static int pgsql_drv_bind_result(db_stmt_t *, db_bind_t *, size_t);
static db_error_t pgsql_drv_execute(db_stmt_t *, db_result_t *);
static int pgsql_drv_fetch(db_result_t *);
static int pgsql_drv_fetch_row(db_result_t *, db_row_t *);
static db_error_t pgsql_drv_query(db_conn_t *, const char *, size_t,
                                  db_result_t *);
static int pgsql_drv_free_results(db_result_t *);
static int pgsql_drv_close(db_stmt_t *);
static int pgsql_drv_done(void);

/* PgSQL driver definition */

static db_driver_t pgsql_driver =
{
  .sname = "pgsql",
  .lname = "PostgreSQL driver",
  .args = pgsql_drv_args,
  .ops =
  {
    .init = pgsql_drv_init,
    .describe = pgsql_drv_describe,
    .connect = pgsql_drv_connect,
    .disconnect = pgsql_drv_disconnect,
    .prepare = pgsql_drv_prepare,
    .bind_param = pgsql_drv_bind_param,
    .bind_result = pgsql_drv_bind_result,
    .execute = pgsql_drv_execute,
    .fetch = pgsql_drv_fetch,
    .fetch_row = pgsql_drv_fetch_row,
    .free_results = pgsql_drv_free_results,
    .close = pgsql_drv_close,
    .query = pgsql_drv_query,
    .done = pgsql_drv_done
  }
};


/* Local functions */

static int get_pgsql_bind_type(db_bind_type_t);
static int get_unique_stmt_name(char *, int);

/* Register PgSQL driver */


int register_driver_pgsql(sb_list_t *drivers)
{
  SB_LIST_ADD_TAIL(&pgsql_driver.listitem, drivers);

  return 0;
}

/* PgSQL driver initialization */

int pgsql_drv_init(void)
{
  args.host = sb_get_value_string("pgsql-host");
  args.port = sb_get_value_string("pgsql-port");
  args.user = sb_get_value_string("pgsql-user");
  args.password = sb_get_value_string("pgsql-password");
  args.db = sb_get_value_string("pgsql-db");

  use_ps = 0;
  pgsql_drv_caps.prepared_statements = 1;
  if (db_globals.ps_mode != DB_PS_MODE_DISABLE)
    use_ps = 1;
  
  return 0;
}


/* Describe database capabilities */


int pgsql_drv_describe(drv_caps_t *caps)
{
  PGconn *con;
  
  *caps = pgsql_drv_caps;

  /* Determine the server version */
  con = PQsetdbLogin(args.host,
                     args.port,
                     NULL,
                     NULL,
                     args.db,
                     args.user,
                     args.password);
  if (PQstatus(con) != CONNECTION_OK)
  {
    log_text(LOG_FATAL, "Connection to database failed: %s",
             PQerrorMessage(con));
    PQfinish(con);
    return 1;
  }

  /* Support for multi-row INSERTs is not available before 8.2 */
  if (PQserverVersion(con) < 80200)
    caps->multi_rows_insert = 0;

  PQfinish(con);
  
  return 0;
}

static void empty_notice_processor(void *arg, const char *msg)
{
  (void) arg; /* unused */
  (void) msg; /* unused */
}

/* Connect to database */

int pgsql_drv_connect(db_conn_t *sb_conn)
{
  PGconn *con;

  con = PQsetdbLogin(args.host,
                     args.port,
                     NULL,
                     NULL,
                     args.db,
                     args.user,
                     args.password);
  if (PQstatus(con) != CONNECTION_OK)
  {
    log_text(LOG_FATAL, "Connection to database failed: %s",
             PQerrorMessage(con));
    PQfinish(con);
    return 1;
  }

  /* Silence the default notice receiver spitting NOTICE message to stderr */
  PQsetNoticeProcessor(con, empty_notice_processor, NULL);
  sb_conn->ptr = con;
  
  return 0;
}

/* Disconnect from database */

int pgsql_drv_disconnect(db_conn_t *sb_conn)
{
  PGconn *con = (PGconn *)sb_conn->ptr;

  /* These might be allocated in pgsql_check_status() */
  xfree(sb_conn->sql_state);
  xfree(sb_conn->sql_errmsg);

  if (con != NULL)
    PQfinish(con);

  return 0;
}


/* Prepare statement */


int pgsql_drv_prepare(db_stmt_t *stmt, const char *query, size_t len)
{
  PGconn       *con = (PGconn *)stmt->connection->ptr;
  PGresult     *pgres;
  pg_stmt_t    *pgstmt;
  char         *buf = NULL;
  unsigned int vcnt;
  unsigned int need_realloc;
  unsigned int i,j;
  unsigned int buflen;
  int          n;
  char         name[32];

  (void) len; /* unused */

  if (con == NULL)
    return 1;

  if (!use_ps)
  {
    /* Use client-side PS */
    stmt->emulated = 1;
    stmt->query = strdup(query);

    return 0;
  }

  /* Convert query to PgSQL-style named placeholders */
  need_realloc = 1;
  vcnt = 1;
  buflen = 0;
  for (i = 0, j = 0; query[i] != '\0'; i++)
  {
  again:
    if (j+1 >= buflen || need_realloc)
    {
      buflen = (buflen > 0) ? buflen * 2 : 256;
      buf = realloc(buf, buflen);
      if (buf == NULL)
        goto error;
      need_realloc = 0;
    }

    if (query[i] != '?')
    {
      buf[j++] = query[i];
      continue;
    }

    n = snprintf(buf + j, buflen - j, "$%d", vcnt);
    if (n < 0 || n >= (int)(buflen - j))
    {
      need_realloc = 1;
      goto again;
    }

    j += n;
    vcnt++;
  }
  buf[j] = '\0';

  /* Store the query to be prepared later on the first bind_param call */
  stmt->query = strdup(buf);
  free(buf);
  
  pgstmt = (pg_stmt_t *)calloc(1, sizeof(pg_stmt_t));
  if (pgstmt == NULL)
    goto error;
  /* Generate random statement name */
  get_unique_stmt_name(name, sizeof(name));
  pgstmt->name = strdup(name);
  pgstmt->nparams = vcnt - 1;

  /*
    Special keys for statements without parameters, since we don't need
    to know the types of arguments, and no calls to bind_param() will be made
  */
  if (pgstmt->nparams == 0)
  {
    /* Do prepare */
    pgres = PQprepare(con, pgstmt->name, stmt->query, pgstmt->nparams,
                      NULL);
    
    if (PQresultStatus(pgres) != PGRES_COMMAND_OK)
    {
      log_text(LOG_FATAL, "PQprepare() failed: %s", PQerrorMessage(con));

      PQclear(pgres);

      free(stmt->query);
      free(pgstmt->name);
      free(pgstmt);

      return 1;
    }
    PQclear(pgres);
    pgstmt->prepared = 1;
  }

  stmt->ptr = pgstmt;
  
  return 0;

 error:

  return 1;
}


/* Bind parameters for prepared statement */


int pgsql_drv_bind_param(db_stmt_t *stmt, db_bind_t *params, size_t len)
{
  PGconn       *con = (PGconn *)stmt->connection->ptr;
  PGresult     *pgres;
  pg_stmt_t    *pgstmt;
  unsigned int i;
  
  if (con == NULL)
    return 1;

  if (stmt->bound_param != NULL)
    free(stmt->bound_param);
  stmt->bound_param = (db_bind_t *)malloc(len * sizeof(db_bind_t));
  if (stmt->bound_param == NULL)
    return 1;
  memcpy(stmt->bound_param, params, len * sizeof(db_bind_t));
  stmt->bound_param_len = len;
 
  if (stmt->emulated)
    return 0;

  pgstmt = stmt->ptr;
  if (pgstmt->prepared)
    return 0;
  
  /* Prepare statement here, since we need to know types of parameters */
  /* Validate parameters count */
  if ((unsigned)pgstmt->nparams != len)
  {
    log_text(LOG_ALERT, "wrong number of parameters in prepared statement");
    log_text(LOG_DEBUG, "counted: %d, passed to bind_param(): %zd",
             pgstmt->nparams, len);
    return 1;
  }

  pgstmt->ptypes = (Oid *)malloc(len * sizeof(int));
  if (pgstmt->ptypes == NULL)
    return 1;

  /* Convert sysbench data types to PgSQL ones */
  for (i = 0; i < len; i++)
    pgstmt->ptypes[i] = get_pgsql_bind_type(params[i].type);

  /* Do prepare */
  pgres = PQprepare(con, pgstmt->name, stmt->query, pgstmt->nparams,
                    pgstmt->ptypes);

  if (PQresultStatus(pgres) != PGRES_COMMAND_OK)
  {
    log_text(LOG_FATAL, "PQprepare() failed: %s", PQerrorMessage(con));
    return 1;
  }

  PQclear(pgres);

  pgstmt->pvalues = (char **)calloc(len, sizeof(char *));
  if (pgstmt->pvalues == NULL)
    return 1;
      
  /* Allocate buffers for bind parameters */
  for (i = 0; i < len; i++)
  {
    if (pgstmt->pvalues[i] != NULL)
    {
      free(pgstmt->pvalues[i]);
    }

    pgstmt->pvalues[i] = (char *)malloc(MAX_PARAM_LENGTH);
    if (pgstmt->pvalues[i] == NULL)
      return 1;
  }
  pgstmt->prepared = 1;

  return 0;
}


/* Bind results for prepared statement */


int pgsql_drv_bind_result(db_stmt_t *stmt, db_bind_t *params, size_t len)
{
  /* unused */
  (void)stmt;
  (void)params;
  (void)len;
  
  return 0;
}


/* Check query execution status */


static db_error_t pgsql_check_status(db_conn_t *con, PGresult *pgres,
                                     const char *funcname, const char *query,
                                     db_result_t *rs)
{
  ExecStatusType status;
  db_error_t     rc;
  PGconn * const pgcon = con->ptr;

  status = PQresultStatus(pgres);
  switch(status) {
  case PGRES_TUPLES_OK:
    rs->nrows = PQntuples(pgres);
    rs->nfields = PQnfields(pgres);
    rs->counter = SB_CNT_READ;

    rc = DB_ERROR_NONE;

    break;

  case PGRES_COMMAND_OK:
    rs->nrows = strtoul(PQcmdTuples(pgres), NULL, 10);;
    rs->counter = (rs->nrows > 0) ? SB_CNT_WRITE : SB_CNT_OTHER;
    rc = DB_ERROR_NONE;

    /*
      Since we are not returning a result set, the SQL layer will never call
      pgsql_drv_free_results(). So we must call PQclear() here.
    */
    PQclear(pgres);

    break;

  case PGRES_FATAL_ERROR:
    rs->nrows = 0;
    rs->counter = SB_CNT_ERROR;

    /*
      Duplicate strings here, because PostgreSQL will deallocate them on
      PQclear() call below. They will be deallocated either on subsequent calls
      to pgsql_check_status() or in pgsql_drv_disconnect().
    */
    xfree(con->sql_state);
    xfree(con->sql_errmsg);

    con->sql_state = strdup(PQresultErrorField(pgres, PG_DIAG_SQLSTATE));
    con->sql_errmsg = strdup(PQresultErrorField(pgres, PG_DIAG_MESSAGE_PRIMARY));

    if (!strcmp(con->sql_state, "40P01") /* deadlock_detected */ ||
        !strcmp(con->sql_state, "23505") /* unique violation */ ||
        !strcmp(con->sql_state, "40001"))/* serialization_failure */
    {
      PGresult *tmp;
      tmp = PQexec(pgcon, "ROLLBACK");
      PQclear(tmp);
      rc = DB_ERROR_IGNORABLE;
    }
    else
    {
      log_text(LOG_FATAL, "%s() failed: %d %s", funcname, status,
               con->sql_errmsg);

      if (query != NULL)
        log_text(LOG_FATAL, "failed query was: %s", query);

      rc =  DB_ERROR_FATAL;
    }

    PQclear(pgres);

    break;

  default:
    rs->nrows = 0;
    rs->counter = SB_CNT_ERROR;
    rc = DB_ERROR_FATAL;
  }

  return rc;
}


/* Execute prepared statement */


db_error_t pgsql_drv_execute(db_stmt_t *stmt, db_result_t *rs)
{
  db_conn_t       *con = stmt->connection;
  PGconn          *pgcon = (PGconn *)con->ptr;
  PGresult        *pgres;
  pg_stmt_t       *pgstmt;
  char            *buf = NULL;
  unsigned int    buflen = 0;
  unsigned int    i, j, vcnt;
  char            need_realloc;
  int             n;
  db_error_t      rc;
  unsigned long   len;

  con->sql_errno = 0;
  xfree(con->sql_state);
  xfree(con->sql_errmsg);

  if (!stmt->emulated)
  {
    pgstmt = stmt->ptr;
    if (pgstmt == NULL)
    {
      log_text(LOG_DEBUG,
               "ERROR: exiting mysql_drv_execute(), uninitialized statement");
      return DB_ERROR_FATAL;
    }

    /* Convert sysbench bind structures to PgSQL data */
    for (i = 0; i < (unsigned)pgstmt->nparams; i++)
    {
      if (stmt->bound_param[i].is_null && *(stmt->bound_param[i].is_null))
        continue;

      switch (stmt->bound_param[i].type) {
        case DB_TYPE_CHAR:
        case DB_TYPE_VARCHAR:

          len = stmt->bound_param[i].data_len[0];

          memcpy(pgstmt->pvalues[i], stmt->bound_param[i].buffer,
                 SB_MIN(MAX_PARAM_LENGTH, len));
          /* PostgreSQL requires a zero-terminated string */
          pgstmt->pvalues[i][len] = '\0';

          break;
        default:
          db_print_value(stmt->bound_param + i, pgstmt->pvalues[i],
                         MAX_PARAM_LENGTH);
      }
    }

    pgres = PQexecPrepared(pgcon, pgstmt->name, pgstmt->nparams,
                           (const char **)pgstmt->pvalues, NULL, NULL, 1);

    rc = pgsql_check_status(con, pgres, "PQexecPrepared", NULL, rs);

    rs->ptr = (rs->counter == SB_CNT_READ) ? (void *) pgres : NULL;

    return rc;
  }

  /* Use emulation */
  /* Build the actual query string from parameters list */
  need_realloc = 1;
  vcnt = 0;
  for (i = 0, j = 0; stmt->query[i] != '\0'; i++)
  {
  again:
    if (j+1 >= buflen || need_realloc)
    {
      buflen = (buflen > 0) ? buflen * 2 : 256;
      buf = realloc(buf, buflen);
      if (buf == NULL)
        return DB_ERROR_FATAL;
      need_realloc = 0;
    }

    if (stmt->query[i] != '?')
    {
      buf[j++] = stmt->query[i];
      continue;
    }

    n = db_print_value(stmt->bound_param + vcnt, buf + j, buflen - j);
    if (n < 0)
    {
      need_realloc = 1;
      goto again;
    }
    j += n;
    vcnt++;
  }
  buf[j] = '\0';

  rc = pgsql_drv_query(con, buf, j, rs);

  free(buf);

  return rc;
}


/* Execute SQL query */


db_error_t pgsql_drv_query(db_conn_t *sb_conn, const char *query, size_t len,
                           db_result_t *rs)
{
  PGconn         *pgcon = sb_conn->ptr;
  PGresult       *pgres;
  db_error_t     rc;

  (void)len; /* unused */

  sb_conn->sql_errno = 0;
  xfree(sb_conn->sql_state);
  xfree(sb_conn->sql_errmsg);

  pgres = PQexec(pgcon, query);
  rc = pgsql_check_status(sb_conn, pgres, "PQexec", query, rs);

  rs->ptr = (rs->counter == SB_CNT_READ) ? (void *) pgres : NULL;

  return rc;
}


/* Fetch row from result set of a prepared statement */


int pgsql_drv_fetch(db_result_t *rs)
{
  /* NYI */
  (void)rs;

  return 1;
}


/* Fetch row from result set of a query */


int pgsql_drv_fetch_row(db_result_t *rs, db_row_t *row)
{
  intptr_t rownum;
  int      i;

  /*
    Use row->ptr as a row number, rather than a pointer to avoid dynamic
    memory management.
  */
  rownum = (intptr_t) row->ptr;
  if (rownum >= (int) rs->nrows)
    return DB_ERROR_IGNORABLE;

  for (i = 0; i < (int) rs->nfields; i++)
  {
    /*
      PQgetvalue() returns an empty string, not a NULL value for a NULL
      field. Callers of this function expect a NULL pointer in this case.
    */
    if (PQgetisnull(rs->ptr, rownum, i))
      row->values[i].ptr = NULL;
    else
    {
      row->values[i].len = PQgetlength(rs->ptr, rownum, i);
      row->values[i].ptr = PQgetvalue(rs->ptr, rownum, i);
    }
  }

  row->ptr = (void *) (rownum + 1);

  return DB_ERROR_NONE;
}


/* Free result set */


int pgsql_drv_free_results(db_result_t *rs)
{
  if (rs->ptr != NULL)
  {
    PQclear((PGresult *)rs->ptr);
    rs->ptr = NULL;

    rs->row.ptr = 0;
    return 0;
  }

  return 1;
}


/* Close prepared statement */


int pgsql_drv_close(db_stmt_t *stmt)
{
  pg_stmt_t *pgstmt = stmt->ptr;
  int       i;
  
  if (pgstmt == NULL)
    return 1;

  if (pgstmt->name != NULL)
    free(pgstmt->name);
  if (pgstmt->ptypes != NULL)
    free(pgstmt->ptypes);
  if (pgstmt->pvalues != NULL)
  {
    for (i = 0; i < pgstmt->nparams; i++)
      if (pgstmt->pvalues[i] != NULL)
        free(pgstmt->pvalues[i]);
    free(pgstmt->pvalues);
  }

  xfree(stmt->ptr);

  return 0;
}


/* Uninitialize driver */
int pgsql_drv_done(void)
{
  return 0;
}


/* Map SQL data type to bind_type value in MYSQL_BIND */


int get_pgsql_bind_type(db_bind_type_t type)
{
  unsigned int i;

  for (i = 0; db_pgsql_bind_map[i].db_type != DB_TYPE_NONE; i++)
    if (db_pgsql_bind_map[i].db_type == type)
      return db_pgsql_bind_map[i].pg_type;

  return -1;
}


int get_unique_stmt_name(char *name, int len)
{
  return snprintf(name, len, "sbstmt%d%d",
                  (int) sb_rand_uniform_uint64(),
                  (int) sb_rand_uniform_uint64());
}
