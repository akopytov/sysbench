/* Copyright (C) 2004 MySQL AB

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
#ifdef _WIN32
#include "sb_win.h"
#endif
#include "db_driver.h"
#include "sb_list.h"


/* Global variables */

db_globals_t db_globals;


/* Static variables */

static sb_list_t    drivers;          /* list of available DB drivers */

/* DB drivers registrars */

#ifdef USE_MYSQL
int register_driver_mysql(sb_list_t *);
#endif

#ifdef USE_ORACLE
int register_driver_oracle(sb_list_t *);
#endif

#ifdef USE_PGSQL
int register_driver_pgsql(sb_list_t *);
#endif

/* Static functions */

static int db_parse_arguments(void);
db_error_t db_do_query(db_conn_t *, const char *, db_result_set_t *);
static void db_free_row(db_row_t *);

/* DB layer arguments */

static sb_arg_t db_args[] =
{
  {
    "db-driver",
    "specifies database driver to use ('help' to get list of available drivers)",
   SB_ARG_TYPE_STRING, NULL
  },
  {
    "db-ps-mode", "prepared statements usage mode {auto, disable}",
    SB_ARG_TYPE_STRING, "auto"
  },
  
  {NULL, NULL, SB_ARG_TYPE_NULL, NULL}
};


/* Register available database drivers and command line arguments */


int db_register(void)
{
  sb_list_item_t *pos;
  db_driver_t    *drv;

  /* Register database drivers */
  SB_LIST_INIT(&drivers);
#ifdef USE_MYSQL
  register_driver_mysql(&drivers);
#endif
#ifdef USE_ORACLE
  register_driver_oracle(&drivers);
#endif
#ifdef USE_PGSQL
  register_driver_pgsql(&drivers);
#endif

  /* Register command line options for each driver */
  SB_LIST_FOR_EACH(pos, &drivers)
  {
    drv = SB_LIST_ENTRY(pos, db_driver_t, listitem);
    if (drv->args != NULL)
      sb_register_arg_set(drv->args);
  }
  /* Register general command line arguments for DB API */
  sb_register_arg_set(db_args);

  return 0;
}


/* Print list of available drivers and their options */


void db_print_help(void)
{
  sb_list_item_t *pos;
  db_driver_t    *drv;

  log_text(LOG_NOTICE, "General database options:\n");
  sb_print_options(db_args);
  log_text(LOG_NOTICE, "");
  
  log_text(LOG_NOTICE, "Compiled-in database drivers:");
  SB_LIST_FOR_EACH(pos, &drivers)
  {
    drv = SB_LIST_ENTRY(pos, db_driver_t, listitem);
    log_text(LOG_NOTICE, "  %s - %s", drv->sname, drv->lname);
  }
  log_text(LOG_NOTICE, "");
  SB_LIST_FOR_EACH(pos, &drivers)
  {
    drv = SB_LIST_ENTRY(pos, db_driver_t, listitem);
    log_text(LOG_NOTICE, "%s options:", drv->sname);
    sb_print_options(drv->args);
  }
}


/*
  Initialize a driver specified by 'name' and return a handle to it
  If NULL is passed as a name, then use the driver passed in --db-driver
  command line option
*/


db_driver_t *db_init(const char *name)
{
  db_driver_t    *drv = NULL;
  sb_list_item_t *pos;
  
  if (SB_LIST_IS_EMPTY(&drivers))
  {
    log_text(LOG_FATAL, "No DB drivers available");
    return NULL;
  }

  if (db_parse_arguments())
    return NULL;

  if (name == NULL && db_globals.driver == NULL)
  {
    drv = SB_LIST_ENTRY(SB_LIST_ITEM_NEXT(&drivers), db_driver_t, listitem);
    /* Is it the only driver available? */
    if (SB_LIST_ITEM_NEXT(&(drv->listitem)) ==
        SB_LIST_ITEM_PREV(&(drv->listitem)))
      log_text(LOG_INFO, "No DB drivers specified, using %s", drv->sname);
    else
    {
      log_text(LOG_FATAL, "no database driver specified");
      return NULL;
    }
  }
  else
  {
    if (name == NULL)
      name = db_globals.driver;

    SB_LIST_FOR_EACH(pos, &drivers)
    {
      drv = SB_LIST_ENTRY(pos, db_driver_t, listitem);
      if (!strcmp(drv->sname, name))
        break;
    }
  }

  if (drv == NULL)
  {
    log_text(LOG_FATAL, "invalid database driver name: '%s'", name);
    return NULL;
  }

  db_globals.debug = sb_globals.debug;
  
  /* Initialize database driver */
  if (drv->ops.init())
  {
    return NULL;
  }

  return drv;
}


/* Describe database capabilities */


int db_describe(db_driver_t *drv, drv_caps_t *caps, const char *table)
{
  if (drv->ops.describe == NULL)
    return 1;

  return drv->ops.describe(caps, table);
}


/* Connect to database */


db_conn_t *db_connect(db_driver_t *drv)
{
  db_conn_t *con;
  
  con = (db_conn_t *)calloc(1, sizeof(db_conn_t));
  if (con == NULL)
    return NULL;
  
  con->driver = drv;
  if (drv->ops.connect(con))
  {
    free(con);
    return NULL;
  }
  
  return con;
}


/* Disconnect from database */


int db_disconnect(db_conn_t *con)
{
  int         rc;
  db_driver_t *drv;

  drv = con->driver;
  if (drv == NULL)
    return 1;

  rc = drv->ops.disconnect(con);
  free(con);

  return 0;
}


/* Prepare statement */


db_stmt_t *db_prepare(db_conn_t *con, const char *query)
{
  db_stmt_t *stmt;

  stmt = (db_stmt_t *)calloc(1, sizeof(db_stmt_t));
  if (stmt == NULL)
    return NULL;

  stmt->connection = con;

  if (con->driver->ops.prepare(stmt, query))
  {
    free(stmt);
    return NULL;
  }
  
  return stmt;
}


/* Bind parameters for prepared statement */


int db_bind_param(db_stmt_t *stmt, db_bind_t *params, unsigned int len)
{
  db_conn_t *con = stmt->connection;

  if (con == NULL || con->driver == NULL)
    return 1;

  return con->driver->ops.bind_param(stmt, params, len);
}


/* Execute prepared statement */


db_result_set_t *db_execute(db_stmt_t *stmt)
{
  db_conn_t       *con = stmt->connection;
  db_result_set_t *rs = &con->rs;

  if (con == NULL || con->driver == NULL)
  {
    log_text(LOG_DEBUG, "ERROR: exiting db_execute(), uninitialized connection");
    return NULL;
  }

  
  memset(rs, 0, sizeof(db_result_set_t));
      
  rs->statement = stmt;
  rs->connection = con;

  con->db_errno = con->driver->ops.execute(stmt, rs);
  if (con->db_errno != SB_DB_ERROR_NONE)
  {
    log_text(LOG_DEBUG, "ERROR: exiting db_execute(), driver's execute method failed");
    return NULL;
  }

  return rs;
}


/* Fetch row into buffers bound by db_bind() */


int db_fetch(db_result_set_t *rs)
{
  db_conn_t *con;

  /* Is this a result set from a prepared statement? */
  if (rs->statement == NULL)
    return 1;

  con = rs->connection;
  if (con == NULL || con->driver == NULL)
    return 1;

  if (!rs->statement->emulated)
    return con->driver->ops.fetch(rs);

  /* NYI: Use emulation */
  return 1;
}


/* Return the number of rows in a result set */


unsigned long long db_num_rows(db_result_set_t *rs)
{
  db_conn_t *con = rs->connection;

  if (con == NULL || con->driver == NULL)
    return 0;

  return con->driver->ops.num_rows(rs);
}


/* Fetch row from result set of a query */


db_row_t *db_fetch_row(db_result_set_t *rs)
{
  db_conn_t *con = rs->connection;

  /* Is this a result set of a non-prepared statement? */
  if (rs->statement != NULL)
    return NULL;

  if (con == NULL || con->driver == NULL)
    return NULL;

  if (rs->row != NULL)
    db_free_row(rs->row);
  rs->row = (db_row_t *)calloc(1, sizeof(db_row_t));
  if (rs->row == NULL)
    return NULL;
  
  if (con->driver->ops.fetch_row(rs, rs->row))
  {
    db_free_row(rs->row);
    return NULL;
  }

  return rs->row;
}


/* Execute non-prepared statement */


db_result_set_t *db_query(db_conn_t *con, const char *query)
{
  db_result_set_t *rs = &con->rs;
  
  if (con->driver == NULL)
    return NULL;

  memset(rs, 0, sizeof(db_result_set_t));
  
  rs->connection = con;

  con->db_errno = con->driver->ops.query(con, query, rs);
  if (con->db_errno != SB_DB_ERROR_NONE)
    return NULL;

  return rs;
}


/* Free result set */


int db_free_results(db_result_set_t *rs)
{
  int       rc;
  db_conn_t *con = rs->connection;

  if (con == NULL || con->driver == NULL)
    return 1;

  rc = con->driver->ops.free_results(rs);

  if (rs->row != NULL)
    db_free_row(rs->row);

  return rc;
}


/* Close prepared statement */


int db_close(db_stmt_t *stmt)
{
  int       rc;
  db_conn_t *con;

  if (stmt == NULL)
    return 1;
  
  con = stmt->connection;

  if (con == NULL || con->driver == NULL)
    return 1;

  rc = con->driver->ops.close(stmt);

  if (stmt->query != NULL)
  {
    free(stmt->query);
    stmt->query = NULL;
  }
  if (stmt->bound_param != NULL)
  {
    free(stmt->bound_param);
    stmt->bound_param = NULL;
  }
  free(stmt);

  return rc;
}


/* Store result set from last query */


int db_store_results(db_result_set_t *rs)
{
  db_conn_t *con = rs->connection;

  if (con == NULL || con->driver == NULL)
    return SB_DB_ERROR_FAILED;

  return con->driver->ops.store_results(rs);
}


/* Uninitialize driver */


int db_done(db_driver_t *drv)
{
  return drv->ops.done();
}


/* Return the error code for the last function */


db_error_t db_errno(db_conn_t *con)
{
  return con->db_errno;
}


/* Parse command line arguments */


int db_parse_arguments(void)
{
  char *s;

  s = sb_get_value_string("db-ps-mode");

  if (!strcmp(s, "auto"))
    db_globals.ps_mode = DB_PS_MODE_AUTO;
  else if (!strcmp(s, "disable"))
    db_globals.ps_mode = DB_PS_MODE_DISABLE;
  else
  {
    log_text(LOG_FATAL, "Invalid value for db-ps-mode: %s", s);
    return 1;
  }

  db_globals.driver = sb_get_value_string("db-driver");
  
  return 0;
}


/* Produce character representation of a 'bind' variable */


int db_print_value(db_bind_t *var, char *buf, int buflen)
{
  int       n;
  db_time_t *tm;
  
  switch (var->type) {
    case DB_TYPE_TINYINT:
      n = snprintf(buf, buflen, "%hhd", *(char *)var->buffer);
      break;
    case DB_TYPE_SMALLINT:
      n = snprintf(buf, buflen, "%hd", *(short *)var->buffer);
      break;
    case DB_TYPE_INT:
      n = snprintf(buf, buflen, "%d", *(int *)var->buffer);
      break;
    case DB_TYPE_BIGINT:
      n = snprintf(buf, buflen, "%lld", *(long long *)var->buffer);
      break;
    case DB_TYPE_FLOAT:
      n = snprintf(buf, buflen, "%f", *(float *)var->buffer);
      break;
    case DB_TYPE_DOUBLE:
      n = snprintf(buf, buflen, "%f", *(double *)var->buffer);
      break;
    case DB_TYPE_CHAR:
    case DB_TYPE_VARCHAR:
      n = snprintf(buf, buflen, "'%s'", (char *)var->buffer);
      break;
    case DB_TYPE_DATE:
      tm = (db_time_t *)var->buffer;
      n = snprintf(buf, buflen, "'%d-%d-%d'", tm->year, tm->month, tm->day);
      break;
    case DB_TYPE_TIME:
      tm = (db_time_t *)var->buffer;
      n = snprintf(buf, buflen, "'%d:%d:%d'", tm->hour, tm->minute,
                   tm->second);
      break;
    case DB_TYPE_DATETIME:
    case DB_TYPE_TIMESTAMP:
      tm = (db_time_t *)var->buffer;
      n = snprintf(buf, buflen, "'%d-%d-%d %d:%d:%d'", tm->year, tm->month,
                   tm->day, tm->hour, tm->minute, tm->second);
      break;
    default:
      n = 0;
      break;
  }

  return (n < buflen) ? n : -1;
}


/* Free row fetched by db_fetch_row() */


void db_free_row(db_row_t *row)
{
  free(row);
}
