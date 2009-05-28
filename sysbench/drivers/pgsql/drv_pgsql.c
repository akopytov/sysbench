/* Copyright (C) 2005 MySQL AB

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

/* Maximum length of text representation of bind parameters */
#define MAX_PARAM_LENGTH 256

/* PostgreSQL driver arguments */

static sb_arg_t pgsql_drv_args[] =
{
  {"pgsql-host", "PostgreSQL server host", SB_ARG_TYPE_STRING, "localhost"},
  {"pgsql-port", "PostgreSQL server port", SB_ARG_TYPE_INT, "5432"},
  {"pgsql-user", "PostgreSQL user", SB_ARG_TYPE_STRING, "sbtest"},
  {"pgsql-password", "PostgreSQL password", SB_ARG_TYPE_STRING, ""},
  {"pgsql-db", "PostgreSQL database name", SB_ARG_TYPE_STRING, "sbtest"},
  
  {NULL, NULL, SB_ARG_TYPE_NULL, NULL}
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
  0,
  1,
  1,
  0,
  0,
  1,
  0,
  
  NULL
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
static int pgsql_drv_describe(drv_caps_t *, const char *);
static int pgsql_drv_connect(db_conn_t *);
static int pgsql_drv_disconnect(db_conn_t *);
static int pgsql_drv_prepare(db_stmt_t *, const char *);
static int pgsql_drv_bind_param(db_stmt_t *, db_bind_t *, unsigned int);
static int pgsql_drv_bind_result(db_stmt_t *, db_bind_t *, unsigned int);
static int pgsql_drv_execute(db_stmt_t *, db_result_set_t *);
static int pgsql_drv_fetch(db_result_set_t *);
static int pgsql_drv_fetch_row(db_result_set_t *, db_row_t *);
static unsigned long long pgsql_drv_num_rows(db_result_set_t *);
static int pgsql_drv_query(db_conn_t *, const char *, db_result_set_t *);
static int pgsql_drv_free_results(db_result_set_t *);
static int pgsql_drv_close(db_stmt_t *);
static int pgsql_drv_store_results(db_result_set_t *);
static int pgsql_drv_done(void);

/* PgSQL driver definition */

static db_driver_t pgsql_driver =
{
  "pgsql",
  "PostgreSQL driver",
  pgsql_drv_args,
  {
    pgsql_drv_init,
    pgsql_drv_describe,
    pgsql_drv_connect,
    pgsql_drv_disconnect,
    pgsql_drv_prepare,
    pgsql_drv_bind_param,
    pgsql_drv_bind_result,
    pgsql_drv_execute,
    pgsql_drv_fetch,
    pgsql_drv_fetch_row,
    pgsql_drv_num_rows,
    pgsql_drv_free_results,
    pgsql_drv_close,
    pgsql_drv_query,
    pgsql_drv_store_results,
    pgsql_drv_done
  },
  {NULL, NULL}
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


int pgsql_drv_describe(drv_caps_t *caps, const char * table_name)
{
  (void)table_name; /* unused */
  
  *caps = pgsql_drv_caps;
  
  return 0;
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
  
  sb_conn->ptr = con;
  
  return 0;
}


/* Disconnect from database */


int pgsql_drv_disconnect(db_conn_t *sb_conn)
{
  PGconn *con = (PGconn *)sb_conn->ptr;

  if (con != NULL)
    PQfinish(con);
  
  return 0;
}


/* Prepare statement */


int pgsql_drv_prepare(db_stmt_t *stmt, const char *query)
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
      return 1;
    }
    pgstmt->prepared = 1;
  }

  stmt->ptr = pgstmt;
  
  return 0;

 error:

  return 1;
}


/* Bind parameters for prepared statement */


int pgsql_drv_bind_param(db_stmt_t *stmt, db_bind_t *params, unsigned int len)
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
    log_text(LOG_DEBUG, "counted: %d, passed to bind_param(): %d",
             pgstmt->nparams, len);
    return 1;
  }

  pgstmt->ptypes = (int *)malloc(len * sizeof(int));
  if (pgstmt->ptypes == NULL)
    return 1;

  /* Convert SysBench data types to PgSQL ones */
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
    
  pgstmt->pvalues = (char **)calloc(len, sizeof(char *));
  if (pgstmt->pvalues == NULL)
    return 1;
      
  /* Allocate buffers for bind parameters */
  for (i = 0; i < len; i++)
  {
    if (pgstmt->pvalues[i] != NULL)
    {
      free(pgstmt->pvalues[i]);
      pgstmt->pvalues[i] = NULL;
    }
      
    pgstmt->pvalues[i] = (char *)malloc(MAX_PARAM_LENGTH);
    if (pgstmt->pvalues[i] == NULL)
      return 1;
  }
  pgstmt->prepared = 1;

  return 0;
}


/* Bind results for prepared statement */


int pgsql_drv_bind_result(db_stmt_t *stmt, db_bind_t *params, unsigned int len)
{
  /* unused */
  (void)stmt;
  (void)params;
  (void)len;
  
  return 0;
}


/* Execute prepared statement */


int pgsql_drv_execute(db_stmt_t *stmt, db_result_set_t *rs)
{
  db_conn_t       *con = stmt->connection;
  PGconn          *pgcon = (PGconn *)con->ptr;
  PGresult        *pgres;
  ExecStatusType  status;
  pg_stmt_t       *pgstmt;
  char            *buf = NULL;
  unsigned int    buflen = 0;
  unsigned int    i, j, vcnt;
  char            need_realloc;
  int             n;
 
  if (!stmt->emulated)
  {
    pgstmt = stmt->ptr;
    if (pgstmt == NULL)
      return SB_DB_ERROR_FAILED;

  
    /* Convert SysBench bind structures to PgSQL data */
    for (i = 0; i < (unsigned)pgstmt->nparams; i++)
    {
      if (stmt->bound_param[i].is_null)
        continue;

      switch (stmt->bound_param[i].type) {
        case DB_TYPE_CHAR:
        case DB_TYPE_VARCHAR:
          strncpy(pgstmt->pvalues[i], stmt->bound_param[i].buffer,
                  MAX_PARAM_LENGTH);
          break;
        default:
          db_print_value(stmt->bound_param + i, pgstmt->pvalues[i],
                         MAX_PARAM_LENGTH);
      }
    }

    pgres = PQexecPrepared(pgcon, pgstmt->name, pgstmt->nparams,
                           (const char **)pgstmt->pvalues, NULL, NULL, 1);
    status = PQresultStatus(pgres);
    if (status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK)
    {
      log_text(LOG_FATAL, "query execution failed: %d", PQerrorMessage(pgcon));
      log_text(LOG_DEBUG, "status = %d", status);
      return SB_DB_ERROR_FAILED;
    }
    rs->ptr = (void *)pgres;
    
    return SB_DB_ERROR_NONE;
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
        return SB_DB_ERROR_FAILED;
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
  
  con->db_errno = pgsql_drv_query(con, buf, rs);
  free(buf);
  if (con->db_errno != SB_DB_ERROR_NONE)
  {
    return con->db_errno;
  }

  return SB_DB_ERROR_NONE;
}


/* Execute SQL query */


int pgsql_drv_query(db_conn_t *sb_conn, const char *query,
                      db_result_set_t *rs)
{
  PGconn         *con = sb_conn->ptr;
  PGresult       *pgres;
  ExecStatusType status;

  (void)rs; /* unused */

  pgres = PQexec(con, query);
  status = PQresultStatus(pgres);
  if (status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK)
  {
    log_text(LOG_ALERT, "failed to execute PgSQL query: `%s`:", query);
    log_text(LOG_ALERT, "error: %s", PQerrorMessage(con));
    return SB_DB_ERROR_FAILED;
  }

  rs->ptr = pgres;
  
  return SB_DB_ERROR_NONE;
}


/* Fetch row from result set of a prepared statement */


int pgsql_drv_fetch(db_result_set_t *rs)
{
  /* NYI */
  (void)rs;

  return 1;
}


/* Fetch row from result set of a query */


int pgsql_drv_fetch_row(db_result_set_t *rs, db_row_t *row)
{
  /* NYI */
  (void)rs;  /* unused */
  (void)row; /* unused */
  
  return 1;
}


/* Return the number of rows in a result set */


unsigned long long pgsql_drv_num_rows(db_result_set_t *rs)
{
  return rs->nrows;
}


/* Store results from the last query */


int pgsql_drv_store_results(db_result_set_t *rs)
{
  PGresult           *pgres;
  unsigned long long i,j, ncolumns;
  
  pgres = (PGresult *)rs->ptr;
  if (pgres == NULL)
    return 1;

  rs->nrows = PQntuples(pgres);
  ncolumns = PQnfields(pgres);

  for (i = 0; i < rs->nrows; i++)
    for (j = 0; j < ncolumns; j++)
      PQgetvalue(pgres, i, j);
  
  return 0;
}


/* Free result set */


int pgsql_drv_free_results(db_result_set_t *rs)
{
  if (rs->ptr != NULL)
  {
    PQclear((PGresult *)rs->ptr);
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
  return snprintf(name, len, "sbstmt%d%d", sb_rnd(), sb_rnd());
}

