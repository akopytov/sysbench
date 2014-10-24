/* Copyright (C) 2009 Sun Microsystems, Inc.

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
#include <stdio.h>

#include <libdrizzle/drizzle_client.h>

#include "sb_options.h"

#include "db_driver.h"

#define DEBUG(format, ...) do { if (db_globals.debug) log_text(LOG_DEBUG, format, __VA_ARGS__); } while (0)

/* Drizzle driver arguments */

static sb_arg_t drizzle_drv_args[] =
{
  {"drizzle-host", "Drizzle server host", SB_ARG_TYPE_LIST, "localhost"},
  {"drizzle-port", "Drizzle server port", SB_ARG_TYPE_INT, "4427"},
  {"drizzle-socket", "Drizzle socket", SB_ARG_TYPE_STRING, NULL},
  {"drizzle-user", "Drizzle user", SB_ARG_TYPE_STRING, ""},
  {"drizzle-password", "Drizzle password", SB_ARG_TYPE_STRING, ""},
  {"drizzle-db", "Drizzle database name", SB_ARG_TYPE_STRING, "sbtest"},
  {"drizzle-buffer", "Level of library buffering (none, field, row, all)", SB_ARG_TYPE_STRING, "none"},
  {"drizzle-mysql", "Use MySQL Protocol", SB_ARG_TYPE_FLAG, "off"}, 
  
  {NULL, NULL, SB_ARG_TYPE_NULL, NULL}
};

typedef enum
{
  BUFFER_NONE,
  BUFFER_FIELD,
  BUFFER_ROW,
  BUFFER_ALL
} buffer_level;

typedef struct
{
  sb_list_t          *hosts;
  unsigned int       port;
  char               *socket;
  char               *user;
  char               *password;
  char               *db;
  buffer_level       buffer;
  int                mysql;
} drizzle_drv_args_t;

/* Drizzle driver capabilities */

static drv_caps_t drizzle_drv_caps =
{
  .multi_rows_insert = 1,
  .prepared_statements = 0,
  .auto_increment = 1,
  .serial = 0,
  .unsigned_int = 0,
};

static drizzle_drv_args_t args;          /* driver args */

static sb_list_item_t *hosts_pos;

static pthread_mutex_t hosts_mutex;

static void column_info(drizzle_column_st *column);

/* Drizzle driver operations */

static int drizzle_drv_init(void);
static int drizzle_drv_describe(drv_caps_t *);
static int drizzle_drv_connect(db_conn_t *);
static int drizzle_drv_disconnect(db_conn_t *);
static int drizzle_drv_prepare(db_stmt_t *, const char *);
static int drizzle_drv_bind_param(db_stmt_t *, db_bind_t *, unsigned int);
static int drizzle_drv_bind_result(db_stmt_t *, db_bind_t *, unsigned int);
static int drizzle_drv_execute(db_stmt_t *, db_result_set_t *);
static int drizzle_drv_fetch(db_result_set_t *);
static int drizzle_drv_fetch_row(db_result_set_t *, db_row_t *);
static unsigned long long drizzle_drv_num_rows(db_result_set_t *);
static int drizzle_drv_query(db_conn_t *, const char *, db_result_set_t *);
static int drizzle_drv_free_results(db_result_set_t *);
static int drizzle_drv_close(db_stmt_t *);
static int drizzle_drv_store_results(db_result_set_t *);
static int drizzle_drv_done(void);

/* Drizzle driver definition */

static db_driver_t drizzle_driver =
{
  .sname = "drizzle",
  .lname = "Drizzle driver",
  .args = drizzle_drv_args,
  .ops =
  {
    drizzle_drv_init,
    drizzle_drv_describe,
    drizzle_drv_connect,
    drizzle_drv_disconnect,
    drizzle_drv_prepare,
    drizzle_drv_bind_param,
    drizzle_drv_bind_result,
    drizzle_drv_execute,
    drizzle_drv_fetch,
    drizzle_drv_fetch_row,
    drizzle_drv_num_rows,
    drizzle_drv_free_results,
    drizzle_drv_close,
    drizzle_drv_query,
    drizzle_drv_store_results,
    drizzle_drv_done
  },
  .listitem = {NULL, NULL}
};


/* Local functions */

/* Register Drizzle driver */


int register_driver_drizzle(sb_list_t *drivers)
{
  SB_LIST_ADD_TAIL(&drizzle_driver.listitem, drivers);

  return 0;
}


/* Drizzle driver initialization */


int drizzle_drv_init(void)
{
  char *s;
  
  args.hosts = sb_get_value_list("drizzle-host");
  if (SB_LIST_IS_EMPTY(args.hosts))
  {
    log_text(LOG_FATAL, "No Drizzle hosts specified, aborting");
    return 1;
  }
  hosts_pos = args.hosts;
  pthread_mutex_init(&hosts_mutex, NULL);
  
  args.port = (unsigned int)sb_get_value_int("drizzle-port");
  args.socket = sb_get_value_string("drizzle-socket");
  args.user = sb_get_value_string("drizzle-user");
  args.password = sb_get_value_string("drizzle-password");
  args.db = sb_get_value_string("drizzle-db");
  s= sb_get_value_string("drizzle-buffer");
  if (!strcasecmp(s, "none"))
    args.buffer= BUFFER_NONE;
  else if (!strcasecmp(s, "field"))
    args.buffer= BUFFER_FIELD;
  else if (!strcasecmp(s, "row"))
    args.buffer= BUFFER_ROW;
  else if (!strcasecmp(s, "all"))
    args.buffer= BUFFER_ALL;

  args.mysql= sb_get_value_flag("drizzle-mysql");
  
  return 0;
}


/* Describe database capabilities (possibly depending on table type) */


int drizzle_drv_describe(drv_caps_t *caps )
{
  *caps = drizzle_drv_caps;
  
  return 0;
}


/* Connect to Drizzle database */


int drizzle_drv_connect(db_conn_t *sb_conn)
{
  drizzle_st              *drizzle_lib= NULL;
  drizzle_con_st          *con= NULL;
  const char              *host;
  drizzle_return_t        ret;

  drizzle_lib= drizzle_create(drizzle_lib);

  if (args.socket)
  {
    DEBUG("drizzle_con_add_uds(%p, %p \"%s\", \"%s\", \"%s\", \"%s\", %d)",
      drizzle_lib,
      con,
      args.socket,
      args.user,
      args.password,
      args.db,
      args.mysql ? DRIZZLE_CON_MYSQL : 0);
    con= drizzle_con_add_uds(drizzle_lib,
                             con,
                             args.socket,
                             args.user,
                             args.password,
                             args.db,
                             args.mysql ? DRIZZLE_CON_MYSQL : 0);
  } else {

    pthread_mutex_lock(&hosts_mutex);
    hosts_pos = SB_LIST_ITEM_NEXT(hosts_pos);
    if (hosts_pos == args.hosts)
      hosts_pos = SB_LIST_ITEM_NEXT(hosts_pos);
    host = SB_LIST_ENTRY(hosts_pos, value_t, listitem)->data;
    pthread_mutex_unlock(&hosts_mutex);

    DEBUG("drizzle_con_add_tcp(%p, %p \"%s\", %u, \"%s\", \"%s\", \"%s\", %d)",
          drizzle_lib,
          con,
          host,
          args.port,
          args.user,
          args.password,
          args.db,
          args.mysql ? DRIZZLE_CON_MYSQL : 0);
    con= drizzle_con_add_tcp(drizzle_lib,
                             con,
                             host,
                             args.port,
                             args.user,
                             args.password,
                             args.db,
                             args.mysql ? DRIZZLE_CON_MYSQL : 0);
  }

  if (con == NULL)
  {
    log_text(LOG_FATAL, "unable to Add Drizzle Connection, aborting...");
    log_text(LOG_FATAL, "error %d: %s", drizzle_errno(drizzle_lib),
             drizzle_error(drizzle_lib));
    return 1;
  }
  if ((ret= drizzle_con_connect(con)) != DRIZZLE_RETURN_OK)
  {
    log_text(LOG_FATAL, "unable to connect to Drizzle server: %d", ret);
    log_text(LOG_FATAL, "error %d: %s", drizzle_errno(drizzle_lib),
             drizzle_error(drizzle_lib));
    free(con);
    return 1;

  }
  sb_conn->ptr = con;

  return 0;
}


/* Disconnect from Drizzle database */


int drizzle_drv_disconnect(db_conn_t *sb_conn)
{
  drizzle_con_st *con = (drizzle_con_st *)sb_conn->ptr;

  if (con != NULL)
  {
    DEBUG("drizzle_close(%p)", con);
    drizzle_con_close(con);
    free(con);
  }
  return 0;
}


/* Prepare statement */


int drizzle_drv_prepare(db_stmt_t *stmt, const char *query)
{

  /* Use client-side PS */
  stmt->emulated = 1;
  stmt->query = strdup(query);

  return 0;
}


/* Bind parameters for prepared statement */
int drizzle_drv_bind_param(db_stmt_t *stmt, db_bind_t *params, unsigned int len)
{
  drizzle_con_st        *con = (drizzle_con_st *)stmt->connection->ptr;
  
  if (con == NULL)
    return 1;

  /* Use emulation */
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
int drizzle_drv_bind_result(db_stmt_t *stmt, db_bind_t *params, unsigned int len)
{
  (void)stmt;
  (void)params;
  (void)len;
  return 0;
}


/* Execute prepared statement */


int drizzle_drv_execute(db_stmt_t *stmt, db_result_set_t *rs)
{
  db_conn_t       *con = stmt->connection;
  char            *buf = NULL;
  unsigned int    buflen = 0;
  unsigned int    i, j, vcnt;
  char            need_realloc;
  int             n;

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
      {
        log_text(LOG_DEBUG, "ERROR: exiting drizzle_drv_execute(), memory allocation failure");
        return SB_DB_ERROR_FAILED;
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
  
  con->db_errno = drizzle_drv_query(con, buf, rs);
  free(buf);
  if (con->db_errno != SB_DB_ERROR_NONE)
  {
    log_text(LOG_DEBUG, "ERROR: exiting drizzle_drv_execute(), database error");
    return con->db_errno;
  }
  
  return SB_DB_ERROR_NONE;
}


/* Execute SQL query */


int drizzle_drv_query(db_conn_t *sb_conn, const char *query,
                      db_result_set_t *rs)
{
  drizzle_con_st *con = sb_conn->ptr;
  unsigned int rc;
  drizzle_return_t ret;
  drizzle_result_st *result= NULL;

  DEBUG("drizzle_query(%p, %p, \"%s\", %u, %p)",
        con,
        result,
        query,
        strlen(query),
        &ret);
  result= drizzle_query(con, NULL, query, strlen(query), &ret);
  DEBUG("drizzle_query(%p) == %d", con, ret);

  if (ret == DRIZZLE_RETURN_ERROR_CODE)
  {
    rc= drizzle_result_error_code(result);
    /* Error code constants haven't been added yet to libdrizzle
       ER_LOCK_DEADLOCK==1213
       ER_LOCK_WAIT_TIMEOUT==1205
       ER_CHECKREAD==1020
     */
    if (rc == 1213 || rc == 1205 ||
        rc == 1020)
      return SB_DB_ERROR_RESTART_TRANSACTION;
    log_text(LOG_ALERT, "Drizzle Query Failed: %u:%s",
             drizzle_result_error_code(result),
             drizzle_result_error(result));
    return SB_DB_ERROR_FAILED;
  }
  else if (ret != DRIZZLE_RETURN_OK)
  {
    rc = drizzle_con_errno(con);
    DEBUG("drizzle_errno(%p) = %u", drizzle_con_drizzle(con), rc);
    log_text(LOG_ALERT, "failed to execute Drizzle query: len==%d `%s`:",
             strlen(query), query);
    log_text(LOG_ALERT, "Error %d %s", drizzle_con_errno(con),
             drizzle_con_error(con));
    return SB_DB_ERROR_FAILED;
  }
  DEBUG("drizzle_query \"%s\" returned %d", query, ret);

  if (result == NULL)
  {
    DEBUG("drizzle_query(%p, \"%s\") == NULL",con,query);
    return SB_DB_ERROR_FAILED;
  }


  rs->ptr= result;
  rs->nrows= drizzle_result_row_count(result);
  DEBUG("drizzle_result_row_count(%p) == %d",result,rs->nrows);

  return SB_DB_ERROR_NONE;
}


/* Fetch row from result set of a prepared statement */


int drizzle_drv_fetch(db_result_set_t *rs)
{
  /* NYI */
  (void)rs;
  printf("in drizzle_drv_fetch_row!\n");

  return 1;
}


/* Fetch row from result set of a query */


int drizzle_drv_fetch_row(db_result_set_t *rs, db_row_t *row)
{
  /* NYI */
  printf("in drizzle_drv_fetch_row!\n");
  (void)rs;  /* unused */
  (void)row; /* unused */
  
  return 1;
}


/* Return the number of rows in a result set */


unsigned long long drizzle_drv_num_rows(db_result_set_t *rs)
{
  return rs->nrows;
}


/* Store results from the last query */


int drizzle_drv_store_results(db_result_set_t *rs)
{

  drizzle_con_st        *con = rs->connection->ptr;
  drizzle_result_st     *res = rs->ptr;
  drizzle_return_t      ret;
  drizzle_column_st     *column= NULL;
  unsigned int rc;


  if (con == NULL || res == NULL)
    return SB_DB_ERROR_FAILED;


  if (args.buffer == BUFFER_ALL)
  {

    ret= drizzle_result_buffer(res);
    DEBUG("drizzle_result_buffer(%p) = %d", res, ret);
    if (ret != DRIZZLE_RETURN_OK)
    {
      rc = drizzle_con_errno(con);
      DEBUG("drizzle_errno(%p) = %u", drizzle_con_drizzle(con), rc);
      log_text(LOG_ALERT, "drizzle_result_buffer failed: `%p`:", res);
      log_text(LOG_ALERT, "Error %d %s",
               drizzle_con_errno(con),
               drizzle_con_error(con));
      return SB_DB_ERROR_FAILED;
    }
    while ((column = drizzle_column_next(res)) != NULL)
      column_info(column);
  }
  else if (drizzle_result_column_count(res) > 0)
  {

    drizzle_row_t         row;
    drizzle_field_t       field;
    uint64_t              row_num;
    size_t offset= 0;
    size_t length;
    size_t total;

    /* Read column meta-info */
    while (1)
    {
        column= drizzle_column_read(res, column, &ret);
        DEBUG("drizzle_column_read(%p,%p,%p) == %d",res, column, &ret, ret);
        if (ret != DRIZZLE_RETURN_OK)
        {
          rc = drizzle_con_errno(con);
          DEBUG("drizzle_errno(%p) = %u", drizzle_con_drizzle(con), rc);
          log_text(LOG_ALERT, "drizzle_column_read failed: `%p`:", res);
          log_text(LOG_ALERT, "Error %d %s",
                   drizzle_con_errno(con),
                   drizzle_con_error(con));
          return SB_DB_ERROR_FAILED;
        }
        if (column == NULL)
          break;

        column_info(column);
        drizzle_column_free(column);
    }

    /* Actually fetch rows */
    while (1) /* Loop for rows */
    {
      if (args.buffer == BUFFER_ROW)
      {
        row= drizzle_row_buffer(res, &ret);
        DEBUG("drizzle_row_buffer(%p, %p) == %p, %d",res, &ret, row, ret);
        if (ret != DRIZZLE_RETURN_OK)
        {
          rc = drizzle_con_errno(con);
          DEBUG("drizzle_errno(%p) = %u", drizzle_con_drizzle(con), rc);
          log_text(LOG_ALERT, "drizzle_row_buffer failed: `%p`:", res);
          log_text(LOG_ALERT, "Error %d %s",
                   drizzle_con_errno(con),
                   drizzle_con_error(con));
          return SB_DB_ERROR_FAILED;
        }

        if (row == NULL)
          break;

        DEBUG("drizzle_row_free(%p, %p)",res,row);
        drizzle_row_free(res, row);
      }
      else if (args.buffer == BUFFER_NONE || args.buffer == BUFFER_FIELD)
      {
        row_num= drizzle_row_read(res, &ret);
        DEBUG("drizzle_row_read(%p, %p) == %"PRIu64", %d",
              res, &ret, row_num, ret);
        if (ret != DRIZZLE_RETURN_OK)
        {
          rc = drizzle_con_errno(con);
          DEBUG("drizzle_errno(%p) = %u", drizzle_con_drizzle(con), rc);
          log_text(LOG_ALERT, "drizzle_row_read failed: `%p`:", res);
          log_text(LOG_ALERT, "Error %d %s",
                   drizzle_con_errno(con),
                   drizzle_con_error(con));
          return SB_DB_ERROR_FAILED;
        }

        if (row_num == 0)
          break;

        while (1) /* Loop for fields */
        {

          if (args.buffer == BUFFER_FIELD)
          {
            /* Since an entire field is buffered, we don't need to worry about
               partial reads. */
            field= drizzle_field_buffer(res, &total, &ret);
            DEBUG("drizzle_field_buffer(%p, &p, %p) == %p, %x, %d",
                  res, &total, &ret, field, total, ret);
            length= total;
          }
          else
          {
            field= drizzle_field_read(res, &offset, &length, &total, &ret);
            DEBUG("drizzle_field_read(%p, %p, %p, %p, %p) == "
                  "%p, %x, %x, %x, %d",
                  res, &offset, &length, &total, &ret,
                  field, offset, length, total, ret);
          }

          if (ret == DRIZZLE_RETURN_ROW_END)
            break;
          else if (ret != DRIZZLE_RETURN_OK)
          {
            rc = drizzle_con_errno(con);
            DEBUG("drizzle_errno(%p) = %u", drizzle_con_drizzle(con), rc);
            log_text(LOG_ALERT, "drizzle_field_(buffer|read) failed: `%p`:",
                     res);
            log_text(LOG_ALERT, "Error %d %s",
                     drizzle_con_errno(con),
                     drizzle_con_error(con));
            return SB_DB_ERROR_FAILED;
          }

          if (args.buffer == BUFFER_FIELD)
            drizzle_field_free(field);

        } /* while (1) Loop for fields */

      } /* if (args.buffer) */

    } /* while (1) */
  }
  return SB_DB_ERROR_NONE;
}


/* Free result set */


int drizzle_drv_free_results(db_result_set_t *rs)
{

  if (rs->ptr != NULL)
  {
    DEBUG("drizzle_result_free(%p)", rs->ptr);
    drizzle_result_free(rs->ptr);
    return 0;
  }

  return 1;
}


/* Close prepared statement */


int drizzle_drv_close(db_stmt_t *stmt)
{
  (void)stmt;
  return 0;
}


/* Uninitialize driver */
int drizzle_drv_done(void)
{
  return 0;
}


void column_info(drizzle_column_st *column)
{
  DEBUG("Field:   catalog=%s\n"
         "              db=%s\n"
         "           table=%s\n"
         "       org_table=%s\n"
         "            name=%s\n"
         "        org_name=%s\n"
         "         charset=%u\n"
         "            size=%u\n"
         "            type=%u\n"
         "           flags=%u\n",
         drizzle_column_catalog(column), drizzle_column_db(column),
         drizzle_column_table(column), drizzle_column_orig_table(column),
         drizzle_column_name(column), drizzle_column_orig_name(column),
         drizzle_column_charset(column), drizzle_column_size(column),
         drizzle_column_type(column), drizzle_column_flags(column));
}

