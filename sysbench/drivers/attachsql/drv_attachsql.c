/* Copyright 2014 Hewlett-Packard Development Company, L.P.
   based on the Drizzle driver:
   Copyright (C) 2009 Sun Microsystems, Inc.

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
#include <stdio.h>

#include <stdint.h>
#include <libattachsql-1.0/attachsql.h>

#include "sb_options.h"

#include "db_driver.h"

#define DEBUG(format, ...) do { if (db_globals.debug) log_text(LOG_DEBUG, format, __VA_ARGS__); } while (0)

/* Drizzle driver arguments */

static sb_arg_t attachsql_drv_args[] =
{
  {"attachsql-host", "libAttachSQL server host", SB_ARG_TYPE_LIST, "localhost"},
  {"attachsql-port", "libAttachSQL server port", SB_ARG_TYPE_INT, "4427"},
  {"attachsql-socket", "libAttachSQL socket", SB_ARG_TYPE_STRING, NULL},
  {"attachsql-user", "libAttachSQL user", SB_ARG_TYPE_STRING, ""},
  {"attachsql-password", "libAttachSQL password", SB_ARG_TYPE_STRING, ""},
  {"attachsql-db", "libAttachSQL database name", SB_ARG_TYPE_STRING, "sbtest"},
  {NULL, NULL, SB_ARG_TYPE_NULL, NULL}
};

typedef struct
{
  sb_list_t          *hosts;
  unsigned int       port;
  char               *socket;
  char               *user;
  char               *password;
  char               *db;
} attachsql_drv_args_t;

/* AttachSQL driver capabilities
 * At a later date we will add prepared statements to this
 */

static drv_caps_t attachsql_drv_caps =
{
  .multi_rows_insert = 1,
  .prepared_statements = 1,
  .auto_increment = 1,
  .serial = 0,
  .unsigned_int = 0,
};

static attachsql_drv_args_t args;          /* driver args */

static sb_list_item_t *hosts_pos;

static pthread_mutex_t hosts_mutex;

/* libAttachSQL driver operations */

static int attachsql_drv_init(void);
static int attachsql_drv_describe(drv_caps_t *);
static int attachsql_drv_connect(db_conn_t *);
static int attachsql_drv_disconnect(db_conn_t *);
static int attachsql_drv_prepare(db_stmt_t *, const char *);
static int attachsql_drv_bind_param(db_stmt_t *, db_bind_t *, unsigned int);
static int attachsql_drv_bind_result(db_stmt_t *, db_bind_t *, unsigned int);
static int attachsql_drv_execute(db_stmt_t *, db_result_set_t *);
static int attachsql_drv_fetch(db_result_set_t *);
static int attachsql_drv_fetch_row(db_result_set_t *, db_row_t *);
static unsigned long long attachsql_drv_num_rows(db_result_set_t *);
static int attachsql_drv_query(db_conn_t *, const char *, db_result_set_t *);
static int attachsql_drv_free_results(db_result_set_t *);
static int attachsql_drv_close(db_stmt_t *);
static int attachsql_drv_store_results(db_result_set_t *);
static int attachsql_drv_done(void);

/* libAttachSQL driver definition */

static db_driver_t attachsql_driver =
{
  .sname = "attachsql",
  .lname = "libAttachSQL driver",
  .args = attachsql_drv_args,
  .ops =
  {
    attachsql_drv_init,
    attachsql_drv_describe,
    attachsql_drv_connect,
    attachsql_drv_disconnect,
    attachsql_drv_prepare,
    attachsql_drv_bind_param,
    attachsql_drv_bind_result,
    attachsql_drv_execute,
    attachsql_drv_fetch,
    attachsql_drv_fetch_row,
    attachsql_drv_num_rows,
    attachsql_drv_free_results,
    attachsql_drv_close,
    attachsql_drv_query,
    attachsql_drv_store_results,
    attachsql_drv_done
  },
  .listitem = {NULL, NULL}
};


/* Local functions */

/* Register libAttachSQL driver */


int register_driver_attachsql(sb_list_t *drivers)
{
  SB_LIST_ADD_TAIL(&attachsql_driver.listitem, drivers);

  return 0;
}


/* libAttachSQL driver initialization */


int attachsql_drv_init(void)
{
  args.hosts = sb_get_value_list("attachsql-host");
  if (SB_LIST_IS_EMPTY(args.hosts))
  {
    log_text(LOG_FATAL, "No libAttachSQL hosts specified, aborting");
    return 1;
  }
  hosts_pos = args.hosts;
  pthread_mutex_init(&hosts_mutex, NULL);
  
  args.port = (unsigned int)sb_get_value_int("attachsql-port");
  args.socket = sb_get_value_string("attachsql-socket");
  args.user = sb_get_value_string("attachsql-user");
  args.password = sb_get_value_string("attachsql-password");
  args.db = sb_get_value_string("attachsql-db");
  attachsql_library_init();
  return 0;
}


/* Describe database capabilities (possibly depending on table type) */


int attachsql_drv_describe(drv_caps_t *caps )
{
  *caps = attachsql_drv_caps;
  
  return 0;
}


/* Connect to libAttachSQL database */


int attachsql_drv_connect(db_conn_t *sb_conn)
{
  attachsql_connect_t     *con= NULL;
  const char              *host;
  attachsql_error_t      *error= NULL;
  attachsql_return_t aret= ATTACHSQL_RETURN_NONE;

  if (args.socket)
  {
    DEBUG("attachsql_connect_create(\"%s\", \"%s\", \"%s\", \"%s\")",
      args.socket,
      args.user,
      args.password,
      args.db);
    con= attachsql_connect_create(args.socket,
                             0,
                             args.user,
                             args.password,
                             args.db,
                             &error);
  } else {

    pthread_mutex_lock(&hosts_mutex);
    hosts_pos = SB_LIST_ITEM_NEXT(hosts_pos);
    if (hosts_pos == args.hosts)
      hosts_pos = SB_LIST_ITEM_NEXT(hosts_pos);
    host = SB_LIST_ENTRY(hosts_pos, value_t, listitem)->data;
    pthread_mutex_unlock(&hosts_mutex);

    DEBUG("attachsql_connect_create(\"%s\", %u, \"%s\", \"%s\", \"%s\")",
          host,
          args.port,
          args.user,
          args.password,
          args.db);
    con= attachsql_connect_create(host,
                             args.port,
                             args.user,
                             args.password,
                             args.db,
                             &error);
  }
  if (con == NULL)
  {
    log_text(LOG_FATAL, "unable to Add libAttachSQL Connection, aborting...");
    log_text(LOG_FATAL, "error %d: %s", attachsql_error_code(error), attachsql_error_message(error));
    attachsql_error_free(error);
    return 1;
  }
  attachsql_connect_set_option(con, ATTACHSQL_OPTION_SEMI_BLOCKING, NULL);

  if (!attachsql_connect(con, &error))
  {
    log_text(LOG_FATAL, "unable to connect to libAttachSQL server");
    log_text(LOG_FATAL, "error %d: %s", attachsql_error_code(error), attachsql_error_message(error));
    attachsql_error_free(error);
    attachsql_connect_destroy(con);
    return 1;

  }

  while (aret != ATTACHSQL_RETURN_IDLE)
  {
    aret = attachsql_connect_poll(con, &error);

    if (error)
    {
      log_text(LOG_FATAL, "unable to connect to libAttachSQL server");
      log_text(LOG_FATAL, "error %d: %s", attachsql_error_code(error), attachsql_error_message(error));
      attachsql_error_free(error);
      attachsql_connect_destroy(con);
      return 1;
    }
  }

  sb_conn->ptr = con;

  return 0;
}


/* Disconnect from libAttachSQL database */


int attachsql_drv_disconnect(db_conn_t *sb_conn)
{
  attachsql_connect_t *con = (attachsql_connect_t *)sb_conn->ptr;

  if (con != NULL)
  {
    DEBUG("attachsql_connect_destroy(%p)", con);
    attachsql_connect_destroy(con);
  }
  return 0;
}


/* Prepare statement */


int attachsql_drv_prepare(db_stmt_t *stmt, const char *query)
{
  attachsql_connect_t *con= (attachsql_connect_t *)stmt->connection->ptr;
  attachsql_error_t *error= NULL;
  attachsql_return_t aret= ATTACHSQL_RETURN_NONE;
  attachsql_statement_prepare(con, strlen(query), query, &error);
  while(aret != ATTACHSQL_RETURN_EOF)
  {
    aret= attachsql_connect_poll(con, &error);
    if (error)
    {
      log_text(LOG_ALERT, "libAttachSQL Prepare Failed: %u:%s", attachsql_error_code(error), attachsql_error_message(error));
      attachsql_error_free(error);
      return SB_DB_ERROR_FAILED;
    }
  }

  return 0;
}


/* Bind parameters for prepared statement */
int attachsql_drv_bind_param(db_stmt_t *stmt, db_bind_t *params, unsigned int len)
{
  /* libAttachSQL doesn't do this, you do this during execute
   * this is because sysbench doesn't set the values until that time
   */

  if (stmt->bound_param != NULL)
    free(stmt->bound_param);
  stmt->bound_param = (db_bind_t *)malloc(len * sizeof(db_bind_t));
  if (stmt->bound_param == NULL)
    return 1;
  memcpy(stmt->bound_param, params, len * sizeof(db_bind_t));
  stmt->bound_param_len = len;

  return 0;

}


/* Bind results for prepared statement */
int attachsql_drv_bind_result(db_stmt_t *stmt, db_bind_t *params, unsigned int len)
{
  (void)stmt;
  (void)params;
  (void)len;
  /* libAttachSQL doesn't do this, you get after execute */
  return 0;
}


/* Execute prepared statement */


int attachsql_drv_execute(db_stmt_t *stmt, db_result_set_t *rs)
{
  (void) rs;
  attachsql_connect_t *con= (attachsql_connect_t *)stmt->connection->ptr;
  attachsql_return_t aret= ATTACHSQL_RETURN_NONE;
  attachsql_error_t *error= NULL;

  uint16_t i;
  int8_t tinyint;
  int16_t smallint;
  int32_t normalint;
  int64_t bigint;
  float float_type;
  double double_type;
  db_time_t *time_data;
  if (con == NULL)
    return 1;

  for (i= 0; i < stmt->bound_param_len; i++)
  {
    db_bind_t *param= &stmt->bound_param[i];
    switch(param->type)
    {
      case DB_TYPE_TINYINT:
        tinyint= *(int8_t*)param->buffer;
        attachsql_statement_set_int(con, i, tinyint, NULL);
        break;
      case DB_TYPE_SMALLINT:
        smallint= *(int16_t*)param->buffer;
        attachsql_statement_set_int(con, i, smallint, NULL);
        break;
      case DB_TYPE_INT:
        normalint= *(int32_t*)param->buffer;
        attachsql_statement_set_int(con, i, normalint, NULL);
        break;
      case DB_TYPE_BIGINT:
        bigint= *(int64_t*)param->buffer;
        attachsql_statement_set_bigint(con, i, bigint, NULL);
        break;
      case DB_TYPE_FLOAT:
        float_type= *(float*)param->buffer;
        attachsql_statement_set_float(con, i, float_type, NULL);
        break;
      case DB_TYPE_DOUBLE:
        double_type= *(double*)param->buffer;
        attachsql_statement_set_double(con, i, double_type, NULL);
        break;
      case DB_TYPE_TIME:
        time_data= (db_time_t*)param->buffer;
        attachsql_statement_set_time(con, i, time_data->hour, time_data->minute, time_data->second, 0, false, NULL);
        break;
      case DB_TYPE_DATE:
      case DB_TYPE_DATETIME:
      case DB_TYPE_TIMESTAMP:
        time_data= (db_time_t*)param->buffer;
        attachsql_statement_set_datetime(con, i, time_data->year, time_data->month, time_data->day, time_data->hour, time_data->minute, time_data->second, 0, NULL);
        break;
      case DB_TYPE_CHAR:
      case DB_TYPE_VARCHAR:
        attachsql_statement_set_string(con, i, param->max_len, param->buffer, NULL);
      case DB_TYPE_NONE:
      default:
        attachsql_statement_set_null(con, i, NULL);
        /* Not supported */
    }
  }

  attachsql_statement_execute(con, &error);

  while(aret != ATTACHSQL_RETURN_EOF)
  {
    aret= attachsql_connect_poll(con, &error);
    if (aret == ATTACHSQL_RETURN_ROW_READY)
    {
      return 0;
    }
    if (error)
    {
      log_text(LOG_ALERT, "libAttachSQL Execute Failed: %u:%s", attachsql_error_code(error), attachsql_error_message(error));
      attachsql_error_free(error);
      return SB_DB_ERROR_FAILED;
    }
  }

  return SB_DB_ERROR_NONE;
}


/* Execute SQL query */


int attachsql_drv_query(db_conn_t *sb_conn, const char *query,
                      db_result_set_t *rs)
{
  (void) rs;
  attachsql_connect_t *con = sb_conn->ptr;
  unsigned int rc;
  attachsql_error_t *error= NULL;
  attachsql_return_t aret= ATTACHSQL_RETURN_NONE;

  /* Close any previous query */
  attachsql_query_close(con);

  DEBUG("attachsql_query(%p, \"%s\", %u)",
        con,
        query,
        strlen(query));
  attachsql_query(con, strlen(query), query, 0, NULL, &error);

  while((aret != ATTACHSQL_RETURN_EOF) && (aret != ATTACHSQL_RETURN_ROW_READY))
  {
    aret= attachsql_connect_poll(con, &error);

    if (error)
    {
      rc= attachsql_error_code(error);
      if (rc == 1213 || rc == 1205 || rc == 1020)
      {
        attachsql_error_free(error);
        return SB_DB_ERROR_RESTART_TRANSACTION;
      }
      log_text(LOG_ALERT, "libAttachSQL Query Failed: %u:%s", attachsql_error_code(error), attachsql_error_message(error));
      attachsql_error_free(error);
      return SB_DB_ERROR_FAILED;
    }
  }
  //rs->connection->ptr= con;
  DEBUG("attachsql_query \"%s\" returned %d", query, aret);

  return SB_DB_ERROR_NONE;
}


/* Fetch row from result set of a prepared statement */


int attachsql_drv_fetch(db_result_set_t *rs)
{
  /* NYI */
  attachsql_connect_t *con = rs->connection->ptr;
  size_t tmp_len;
  uint16_t columns, col;
  attachsql_return_t aret= ATTACHSQL_RETURN_NONE;
  attachsql_error_t *error= NULL;

  while((aret != ATTACHSQL_RETURN_EOF) && (aret != ATTACHSQL_RETURN_ROW_READY))
  {
    aret= attachsql_connect_poll(con, &error);

    if (error)
    {
      log_text(LOG_ALERT, "libAttachSQL Query Failed: %u:%s", attachsql_error_code(error), attachsql_error_message(error));
      attachsql_error_free(error);
      return 1;
    }
  }
  if (aret == ATTACHSQL_RETURN_EOF)
  {
    return 1;
  }
  attachsql_statement_row_get(con, NULL);
  columns= attachsql_query_column_count(con);
  for (col= 0; col < columns; col++)
  {
    switch (attachsql_statement_get_column_type(con, col))
    {
      case ATTACHSQL_COLUMN_TYPE_TINY:
      case ATTACHSQL_COLUMN_TYPE_SHORT:
      case ATTACHSQL_COLUMN_TYPE_LONG:
      case ATTACHSQL_COLUMN_TYPE_YEAR:
      case ATTACHSQL_COLUMN_TYPE_INT24:
        attachsql_statement_get_int(con, col, &error);
        break;
      case ATTACHSQL_COLUMN_TYPE_LONGLONG:
        attachsql_statement_get_bigint(con, col, &error);
        break;
      case ATTACHSQL_COLUMN_TYPE_FLOAT:
        attachsql_statement_get_float(con, col, &error);
        break;
      case ATTACHSQL_COLUMN_TYPE_DOUBLE:
        attachsql_statement_get_double(con, col, &error);
        break;
      default:
        attachsql_statement_get_char(con, col, &tmp_len, &error);
        break;
    }
  }
  attachsql_query_row_next(con);

  return 0;
}


/* Fetch row from result set of a query */


int attachsql_drv_fetch_row(db_result_set_t *rs, db_row_t *row)
{
  attachsql_error_t *error= NULL;
  attachsql_return_t aret= ATTACHSQL_RETURN_NONE;

  /* NYI */

  attachsql_connect_t *con = rs->connection->ptr;

  while((aret != ATTACHSQL_RETURN_EOF) && (aret != ATTACHSQL_RETURN_ROW_READY))
  {
    aret= attachsql_connect_poll(con, &error);

    if (error)
    {
      log_text(LOG_ALERT, "libAttachSQL Query Failed: %u:%s", attachsql_error_code(error), attachsql_error_message(error));
      attachsql_error_free(error);
      return 1;
    }
  }
  if (aret == ATTACHSQL_RETURN_EOF)
  {
    return 1;
  }
  row->ptr= attachsql_query_row_get(con, NULL);
  attachsql_query_row_next(con);

  return 0;
}


/* Return the number of rows in a result set */


unsigned long long attachsql_drv_num_rows(db_result_set_t *rs)
{
  return rs->nrows;
}


/* Store results from the last query */


int attachsql_drv_store_results(db_result_set_t *rs)
{
  int ret= 0;
  db_row_t row;
  /* libAttachSQL can't do things in this order */
  while (ret == 0)
  {
    if (rs->statement != NULL)
    {
      ret= attachsql_drv_fetch(rs);
    }
    else
    {
      ret= attachsql_drv_fetch_row(rs, &row);
    }
  }

  return SB_DB_ERROR_NONE;
}


/* Free result set */


int attachsql_drv_free_results(db_result_set_t *rs)
{

  if (rs->connection->ptr != NULL)
  {
    DEBUG("attachsql_query_close(%p)", rs->connection->ptr);
    attachsql_query_close(rs->connection->ptr);
    return 0;
  }

  return 1;
}


/* Close prepared statement */


int attachsql_drv_close(db_stmt_t *stmt)
{
  attachsql_connect_t *con= (attachsql_connect_t *)stmt->connection->ptr;
  attachsql_statement_close(con);

  return 0;
}


/* Uninitialize driver */
int attachsql_drv_done(void)
{
  return 0;
}

