/*
   Copyright (C) 2004 MySQL AB
   Copyright (C) 2004-2017 Alexey Kopytov <akopytov@gmail.com>

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
#ifdef _WIN32
#include "sb_win.h"
#endif
#ifdef STDC_HEADERS
# include <ctype.h>
# include <inttypes.h>
# include <stdbool.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <pthread.h>

#include "db_driver.h"
#include "sb_list.h"
#include "sb_histogram.h"
#include "sb_ck_pr.h"

/* Query length limit for bulk insert queries */
#define BULK_PACKET_SIZE (512*1024)

/* How many rows to insert before COMMITs (used in bulk insert) */
#define ROWS_BEFORE_COMMIT 1000

/* Global variables */
db_globals_t db_globals CK_CC_CACHELINE;

static sb_list_t        drivers;          /* list of available DB drivers */

static uint8_t stats_enabled;

static bool db_global_initialized;
static pthread_once_t db_global_once = PTHREAD_ONCE_INIT;

/* Timers used in debug mode */
static sb_timer_t *exec_timers;
static sb_timer_t *fetch_timers;

/* Static functions */

static int db_parse_arguments(void);
#if 0
static void db_free_row(db_row_t *);
#endif
static int db_bulk_do_insert(db_conn_t *, int);
static void db_reset_stats(void);
static int db_free_results_int(db_conn_t *con);

/* DB layer arguments */

static sb_arg_t db_args[] =
{
  SB_OPT("db-driver", "specifies database driver to use "
         "('help' to get list of available drivers)",
#ifdef USE_MYSQL
         "mysql",
#else
         NULL,
#endif
         STRING),
  SB_OPT("db-ps-mode", "prepared statements usage mode {auto, disable}", "auto",
         STRING),
  SB_OPT("db-debug", "print database-specific debug information", "off", BOOL),

  SB_OPT_END
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
#ifdef USE_DRIZZLE
  register_driver_drizzle(&drivers);
#endif
#ifdef USE_ATTACHSQL
  register_driver_attachsql(&drivers);
#endif
#ifdef USE_DRIZZLECLIENT
  register_driver_drizzleclient(&drivers);
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
    drv->initialized = false;
    pthread_mutex_init(&drv->mutex, NULL);
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


static void enable_print_stats(void)
{
  ck_pr_fence_store();
  ck_pr_store_8(&stats_enabled, 1);
}

static void disable_print_stats(void)
{
  ck_pr_store_8(&stats_enabled, 0);
  ck_pr_fence_store();
}


static bool check_print_stats(void)
{
  bool rc = ck_pr_load_8(&stats_enabled) == 1;
  ck_pr_fence_load();

  return rc;
}

static void db_init(void)
{
  if (SB_LIST_IS_EMPTY(&drivers))
  {
    log_text(LOG_FATAL, "No DB drivers available");
    return;
  }

  if (db_parse_arguments())
    return;

  /* Initialize timers if in debug mode */
  if (db_globals.debug)
  {
    exec_timers = sb_alloc_per_thread_array(sizeof(sb_timer_t));
    fetch_timers = sb_alloc_per_thread_array(sizeof(sb_timer_t));
  }

  db_reset_stats();

  enable_print_stats();

  db_global_initialized = true;
}

/*
  Initialize a driver specified by 'name' and return a handle to it
  If NULL is passed as a name, then use the driver passed in --db-driver
  command line option
*/

db_driver_t *db_create(const char *name)
{
  db_driver_t    *drv = NULL;
  db_driver_t    *tmp;
  sb_list_item_t *pos;

  pthread_once(&db_global_once, db_init);

  if (!db_global_initialized)
    goto err;

  if (name == NULL && db_globals.driver == NULL)
  {
    drv = SB_LIST_ENTRY(SB_LIST_ITEM_NEXT(&drivers), db_driver_t, listitem);
    /* Is it the only driver available? */
    if (SB_LIST_ITEM_NEXT(&(drv->listitem)) ==
        SB_LIST_ITEM_PREV(&(drv->listitem)))
      log_text(LOG_INFO, "No DB drivers specified, using %s", drv->sname);
    else
    {
      log_text(LOG_FATAL, "Multiple DB drivers are available. "
               "Use --db-driver=name to specify which one to use");
      goto err;
    }
  }
  else
  {
    if (name == NULL)
      name = db_globals.driver;

    SB_LIST_FOR_EACH(pos, &drivers)
    {
      tmp = SB_LIST_ENTRY(pos, db_driver_t, listitem);
      if (!strcmp(tmp->sname, name))
      {
        drv = tmp;
        break;
      }
    }
  }

  if (drv == NULL)
  {
    log_text(LOG_FATAL, "invalid database driver name: '%s'", name);
    goto err;
  }

  /* Initialize database driver only once */
  pthread_mutex_lock(&drv->mutex);
  if (!drv->initialized)
  {
    if (drv->ops.init())
    {
      pthread_mutex_unlock(&drv->mutex);
      goto err;
    }
    drv->initialized = true;
  }
  pthread_mutex_unlock(&drv->mutex);

  if (drv->ops.thread_init != NULL && drv->ops.thread_init(sb_tls_thread_id))
  {
    log_text(LOG_FATAL, "thread-local driver initialization failed.");
    return NULL;
  }

  return drv;

err:
  return NULL;
}

/* Deinitialize a driver object */

int db_destroy(db_driver_t *drv)
{
  if (drv->ops.thread_done != NULL)
    return drv->ops.thread_done(sb_tls_thread_id);

  return 0;
}

/* Describe database capabilities */

int db_describe(db_driver_t *drv, drv_caps_t *caps)
{
  if (drv->ops.describe == NULL)
    return 1;

  return drv->ops.describe(caps);
}


/* Connect to database */


db_conn_t *db_connection_create(db_driver_t *drv)
{
  db_conn_t *con;

  SB_COMPILE_TIME_ASSERT(sizeof(db_conn_t) % CK_MD_CACHELINE == 0);

  con = (db_conn_t *)calloc(1, sizeof(db_conn_t));
  if (con == NULL)
    return NULL;

  con->driver = drv;
  con->state = DB_CONN_READY;

  con->thread_id =  sb_tls_thread_id;

  if (drv->ops.connect(con))
  {
    free(con);
    return NULL;
  }

  return con;
}


/* Disconnect from database */


int db_connection_close(db_conn_t *con)
{
  int         rc;
  db_driver_t *drv = con->driver;

  if (con->state == DB_CONN_INVALID)
  {
    log_text(LOG_ALERT, "attempt to close an already closed connection");
    return 0;
  }
  else if (con->state == DB_CONN_RESULT_SET)
  {
    db_free_results_int(con);
  }

  rc = drv->ops.disconnect(con);

  con->state = DB_CONN_INVALID;

  return rc;
}

/* Reconnect with the same connection parameters */

int db_connection_reconnect(db_conn_t *con)
{
  int         rc;
  db_driver_t *drv = con->driver;

  if (drv->ops.reconnect == NULL)
  {
    log_text(LOG_ALERT, "reconnect is not supported by the current driver");
    return 0;
  }

  if (con->state == DB_CONN_INVALID)
  {
    log_text(LOG_ALERT, "attempt to close an already closed connection");
    return 0;
  }
  else if (con->state == DB_CONN_RESULT_SET)
  {
    db_free_results_int(con);
  }

  rc = drv->ops.reconnect(con);

  if (rc == DB_ERROR_FATAL)
  {
    con->state = DB_CONN_INVALID;
    sb_counter_inc(con->thread_id, SB_CNT_ERROR);
  }
  else
  {
    con->state = DB_CONN_READY;
    sb_counter_inc(con->thread_id, SB_CNT_RECONNECT);

    /* Clear DB_ERROR_IGNORABLE */
    rc = DB_ERROR_NONE;
  }

  return rc;
}

/* Disconnect and release memory allocated by a connection object */

void db_connection_free(db_conn_t *con)
{
  if (con->state != DB_CONN_INVALID)
    db_connection_close(con);

  free(con);
}


/* Prepare statement */


db_stmt_t *db_prepare(db_conn_t *con, const char *query, size_t len)
{
  db_stmt_t *stmt;

  if (con->state == DB_CONN_INVALID)
  {
    log_text(LOG_ALERT, "attempt to use an already closed connection");
    return NULL;
  }

  stmt = (db_stmt_t *)calloc(1, sizeof(db_stmt_t));
  if (stmt == NULL)
    return NULL;

  stmt->connection = con;

  if (con->driver->ops.prepare(stmt, query, len))
  {
    con->error = DB_ERROR_FATAL;
    free(stmt);
    return NULL;
  }

  return stmt;
}


/* Bind parameters for prepared statement */


int db_bind_param(db_stmt_t *stmt, db_bind_t *params, size_t len)
{
  db_conn_t *con = stmt->connection;

  if (con->state == DB_CONN_INVALID)
  {
    log_text(LOG_ALERT, "attempt to use an already closed connection");
    return 1;
  }

  return con->driver->ops.bind_param(stmt, params, len);
}


/* Bind results for prepared statement */


int db_bind_result(db_stmt_t *stmt, db_bind_t *results, size_t len)
{
  db_conn_t *con = stmt->connection;

  if (con->state == DB_CONN_INVALID)
  {
    log_text(LOG_ALERT, "attempt to use an already closed connection");
    return 1;
  }

  return con->driver->ops.bind_result(stmt, results, len);
}


/* Execute prepared statement */


db_result_t *db_execute(db_stmt_t *stmt)
{
  db_conn_t       *con = stmt->connection;
  db_result_t     *rs = &con->rs;
  int             rc;

  if (con->state == DB_CONN_INVALID)
  {
    log_text(LOG_ALERT, "attempt to use an already closed connection");
    return NULL;
  }
  else if (con->state == DB_CONN_RESULT_SET &&
           (rc = db_free_results_int(con)) != 0)
  {
    return NULL;
  }

  rs->statement = stmt;

  con->error = con->driver->ops.execute(stmt, rs);

  sb_counter_inc(con->thread_id, rs->counter);

  if (SB_LIKELY(con->error == DB_ERROR_NONE))
  {
    if (rs->counter == SB_CNT_READ)
    {
      con->state = DB_CONN_RESULT_SET;
      return rs;
    }
    con->state = DB_CONN_READY;

    return NULL;
  }

  return NULL;
}


/* Fetch row from result set of a query */


db_row_t *db_fetch_row(db_result_t *rs)
{
  db_conn_t *con = SB_CONTAINER_OF(rs, db_conn_t, rs);

  if (con->state == DB_CONN_INVALID)
  {
    log_text(LOG_ALERT, "attempt to use an already closed connection");
    return NULL;
  }
  else if (con->state != DB_CONN_RESULT_SET)
  {
    log_text(LOG_ALERT, "attempt to fetch row from an invalid result set");
    return NULL;
  }

  if (con->driver->ops.fetch_row == NULL)
  {
    log_text(LOG_ALERT, "fetching rows is not supported by the driver");
  }

  if (rs->nrows == 0 || rs->nfields == 0)
  {
    log_text(LOG_ALERT, "attempt to fetch row from an empty result set");
    return NULL;
  }

  if (rs->row.values == NULL)
  {
    rs->row.values = malloc(rs->nfields * sizeof(db_value_t));
  }

  if (con->driver->ops.fetch_row(rs, &rs->row))
  {
    return NULL;
  }

  return &rs->row;
}


/* Execute non-prepared statement */


db_result_t *db_query(db_conn_t *con, const char *query, size_t len)
{
  db_result_t *rs = &con->rs;
  int         rc;

  if (con->state == DB_CONN_INVALID)
  {
    log_text(LOG_ALERT, "attempt to use an already closed connection");
    con->error = DB_ERROR_FATAL;
    return NULL;
  }
  else if (con->state == DB_CONN_RESULT_SET &&
           (rc = db_free_results_int(con)) != 0)
  {
    con->error = DB_ERROR_FATAL;
    return NULL;
  }

  con->error = con->driver->ops.query(con, query, len, rs);

  sb_counter_inc(con->thread_id, rs->counter);

  if (SB_LIKELY(con->error == DB_ERROR_NONE))
  {
    if (rs->counter == SB_CNT_READ)
    {
      con->state = DB_CONN_RESULT_SET;
      return rs;
    }
    con->state = DB_CONN_READY;

    return NULL;
  }

  return NULL;
}


/* Free result set */


static int db_free_results_int(db_conn_t *con)
{
  int       rc;

  rc = con->driver->ops.free_results(&con->rs);

  if (con->rs.row.values != NULL)
  {
    free(con->rs.row.values);
    con->rs.row.values = NULL;
  }

  con->rs.nrows = 0;
  con->rs.nfields = 0;

  con->rs.statement = NULL;

  con->state = DB_CONN_READY;

  return rc;
}

int db_free_results(db_result_t *rs)
{
  db_conn_t * const con = SB_CONTAINER_OF(rs, db_conn_t, rs);

  if (con->state == DB_CONN_INVALID)
  {
    log_text(LOG_ALERT, "attempt to use an already closed connection");
    return 1;
  }
  else if (con->state != DB_CONN_RESULT_SET)
  {
    log_text(LOG_ALERT, "attempt to free an invalid result set");
    return 1;
  }

  return db_free_results_int(con);
}

/* Close prepared statement */


int db_close(db_stmt_t *stmt)
{
  int       rc;
  db_conn_t *con = stmt->connection;

  if (con->state == DB_CONN_INVALID)
  {
    log_text(LOG_ALERT, "attempt to use an already closed connection");
    return 0;
  }
  else if (con->state == DB_CONN_RESULT_SET &&
           (rc = db_free_results_int(con)) != 0)
  {
    return DB_ERROR_FATAL;
  }

  if (con->state == DB_CONN_INVALID)
  {
    log_text(LOG_ALERT, "attempt to use an already closed connection");
    return 0;
  }
  else if (con->state == DB_CONN_RESULT_SET && con->rs.statement == stmt &&
           (rc = db_free_results_int(con)) != 0)
  {
    return 0;
  }

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

/* Uninitialize DB API */

void db_done(void)
{
  sb_list_item_t *pos;
  db_driver_t    *drv;

  if (!db_global_initialized)
    return;

  disable_print_stats();

  if (db_globals.debug)
  {
    free(exec_timers);
    free(fetch_timers);

    exec_timers = fetch_timers = NULL;
  }

  SB_LIST_FOR_EACH(pos, &drivers)
  {
    drv = SB_LIST_ENTRY(pos, db_driver_t, listitem);
    if (drv->initialized)
    {
      drv->ops.done();
      pthread_mutex_destroy(&drv->mutex);
    }
  }

  return;
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

  db_globals.debug = sb_get_value_flag("db-debug");
  
  return 0;
}


/* Produce character representation of a 'bind' variable */


int db_print_value(db_bind_t *var, char *buf, int buflen)
{
  int       n;
  db_time_t *tm;

  if (var->is_null != NULL && *var->is_null)
  {
    n = snprintf(buf, buflen, "NULL");
    return (n < buflen) ? n : -1;
  }
  
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


#if 0
/* Free row fetched by db_fetch_row() */


void db_free_row(db_row_t *row)
{
  free(row);
}
#endif

/* Initialize multi-row insert operation */


int db_bulk_insert_init(db_conn_t *con, const char *query, size_t query_len)
{
  drv_caps_t driver_caps;
  int        rc;

  if (con->state == DB_CONN_INVALID)
  {
    log_text(LOG_ALERT, "attempt to use an already closed connection");
    return 0;
  }
  else if (con->state == DB_CONN_RESULT_SET &&
           (rc = db_free_results_int(con)) != 0)
  {
    return 0;
  }

  /* Get database capabilites */
  if (db_describe(con->driver, &driver_caps))
  {
    log_text(LOG_FATAL, "failed to get database capabilities!");
    return 1;
  }

  /* Allocate query buffer */
  if (query_len + 1 > BULK_PACKET_SIZE)
  {
    log_text(LOG_FATAL,
             "Query length exceeds the maximum value (%u), aborting",
             BULK_PACKET_SIZE);
    return 1;
  }
  con->bulk_buflen = BULK_PACKET_SIZE;
  con->bulk_buffer = (char *)malloc(con->bulk_buflen);
  if (con->bulk_buffer == NULL)
    return 1;
  
  con->bulk_commit_max = driver_caps.needs_commit ? ROWS_BEFORE_COMMIT : 0;
  con->bulk_commit_cnt = 0;
  strcpy(con->bulk_buffer, query);
  con->bulk_ptr = query_len;
  con->bulk_values = query_len;
  con->bulk_cnt = 0;

  return 0;
}

/* Add row to multi-row insert operation */

int db_bulk_insert_next(db_conn_t *con, const char *query, size_t query_len)
{
  int rc;

  if (con->state == DB_CONN_INVALID)
  {
    log_text(LOG_ALERT, "attempt to use an already closed connection");
    return 0;
  }
  else if (con->state == DB_CONN_RESULT_SET &&
           (rc = db_free_results_int(con)) != 0)
  {
    return 0;
  }

  if (con->bulk_buffer == NULL)
  {
    log_text(LOG_ALERT, "attempt to call bulk_insert_next() before bulk_insert_init()");
    return 1;
  }

  /*
    Reserve space for '\0' and ',' (if not the first chunk in
    a bulk insert
  */
  if (con->bulk_ptr + query_len + 1 + (con->bulk_cnt>0) > con->bulk_buflen)
  {
    /* Is this a first row? */
    if (!con->bulk_cnt)
    {
      log_text(LOG_FATAL,
               "Query length exceeds the maximum value (%u), aborting",
               con->bulk_buflen);
      return 1;
    }
    if (db_bulk_do_insert(con, 0))
      return 1;
  }

  if (con->bulk_cnt > 0)
  {
    con->bulk_buffer[con->bulk_ptr] = ',';
    strcpy(con->bulk_buffer + con->bulk_ptr + 1, query);
  }
  else
    strcpy(con->bulk_buffer + con->bulk_ptr, query);
  con->bulk_ptr += query_len + (con->bulk_cnt > 0);

  con->bulk_cnt++;

  return 0;
}

/* Do the actual INSERT (and COMMIT, if necessary) */

static int db_bulk_do_insert(db_conn_t *con, int is_last)
{
  if (!con->bulk_cnt)
    return 0;

  if (db_query(con, con->bulk_buffer, con->bulk_ptr) == NULL &&
      con->error != DB_ERROR_NONE)
    return 1;


  if (con->bulk_commit_max != 0)
  {
    con->bulk_commit_cnt += con->bulk_cnt;

    if (is_last || con->bulk_commit_cnt >= con->bulk_commit_max)
    {
      if (db_query(con, "COMMIT", 6) == NULL &&
          con->error != DB_ERROR_NONE)
        return 1;
      con->bulk_commit_cnt = 0;
    }
  }

  con->bulk_ptr = con->bulk_values;
  con->bulk_cnt = 0;

  return 0;
}

/* Finish multi-row insert operation */

int db_bulk_insert_done(db_conn_t *con)
{
  /* Flush remaining data in buffer, if any */
  if (db_bulk_do_insert(con, 1))
    return 1;

  if (con->bulk_buffer != NULL)
  {
    free(con->bulk_buffer);
    con->bulk_buffer = NULL;
  }

  return 0;
}

void db_report_intermediate(sb_stat_t *stat)
{
  /* Use default stats handler if no drivers are used */
  if (!check_print_stats())
  {
    sb_report_intermediate(stat);
    return;
  }

  const double seconds = stat->time_interval;

  log_timestamp(LOG_NOTICE, stat->time_total,
                "thds: %u tps: %4.2f "
                "qps: %4.2f (r/w/o: %4.2f/%4.2f/%4.2f) "
                "lat (ms,%u%%): %4.2f err/s: %4.2f "
                "reconn/s: %4.2f",
                stat->threads_running,
                stat->events / seconds,
                (stat->reads + stat->writes + stat->other) / seconds,
                stat->reads / seconds,
                stat->writes / seconds,
                stat->other / seconds,
                sb_globals.percentile,
                SEC2MS(stat->latency_pct),
                stat->errors / seconds,
                stat->reconnects / seconds);

  if (sb_globals.tx_rate > 0)
  {
    log_timestamp(LOG_NOTICE, stat->time_total,
                  "queue length: %" PRIu64", concurrency: %" PRIu64,
                  stat->queue_length, stat->concurrency);
  }
}


void db_report_cumulative(sb_stat_t *stat)
{
  sb_timer_t    exec_timer;
  sb_timer_t    fetch_timer;

  /* Use default stats handler if no drivers are used */
  if (!check_print_stats())
  {
    sb_report_cumulative(stat);
    return;
  }

  const double seconds = stat->time_interval;
  const uint64_t queries = stat->reads + stat->writes + stat->other;

  log_text(LOG_NOTICE, "SQL statistics:");
  log_text(LOG_NOTICE, "    queries performed:");
  log_text(LOG_NOTICE, "        read:                            %" PRIu64,
           stat->reads);
  log_text(LOG_NOTICE, "        write:                           %" PRIu64,
           stat->writes);
  log_text(LOG_NOTICE, "        other:                           %" PRIu64,
           stat->other);
  log_text(LOG_NOTICE, "        total:                           %" PRIu64,
           queries);
  log_text(LOG_NOTICE, "    transactions:                        %-6" PRIu64
           " (%.2f per sec.)", stat->events, stat->events / seconds);
  log_text(LOG_NOTICE, "    queries:                             %-6" PRIu64
           " (%.2f per sec.)", queries,
           queries / seconds);
  log_text(LOG_NOTICE, "    ignored errors:                      %-6" PRIu64
           " (%.2f per sec.)", stat->errors, stat->errors / seconds);
  log_text(LOG_NOTICE, "    reconnects:                          %-6" PRIu64
           " (%.2f per sec.)", stat->reconnects, stat->reconnects / seconds);

  if (db_globals.debug)
  {
    sb_timer_init(&exec_timer);
    sb_timer_init(&fetch_timer);

    for (unsigned i = 0; i < sb_globals.threads; i++)
    {
      exec_timer = sb_timer_merge(&exec_timer, exec_timers + i);
      fetch_timer = sb_timer_merge(&fetch_timer, fetch_timers + i);
    }

    log_text(LOG_DEBUG, "");
    log_text(LOG_DEBUG, "Query execution statistics:");
    log_text(LOG_DEBUG, "    min:                                %.4fs",
             NS2SEC(sb_timer_min(&exec_timer)));
    log_text(LOG_DEBUG, "    avg:                                %.4fs",
             NS2SEC(sb_timer_avg(&exec_timer)));
    log_text(LOG_DEBUG, "    max:                                %.4fs",
             NS2SEC(sb_timer_max(&exec_timer)));
    log_text(LOG_DEBUG, "  total:                                %.4fs",
             NS2SEC(sb_timer_sum(&exec_timer)));

    log_text(LOG_DEBUG, "Results fetching statistics:");
    log_text(LOG_DEBUG, "    min:                                %.4fs",
             NS2SEC(sb_timer_min(&fetch_timer)));
    log_text(LOG_DEBUG, "    avg:                                %.4fs",
             NS2SEC(sb_timer_avg(&fetch_timer)));
    log_text(LOG_DEBUG, "    max:                                %.4fs",
             NS2SEC(sb_timer_max(&fetch_timer)));
    log_text(LOG_DEBUG, "  total:                                %.4fs",
             NS2SEC(sb_timer_sum(&fetch_timer)));
  }

  /* Report sysbench general stats */
  sb_report_cumulative(stat);
}


static void db_reset_stats(void)
{
  unsigned int i;

  /*
    So that intermediate stats are calculated from the current moment
    rather than from the previous intermediate report
  */
  sb_timer_current(&sb_intermediate_timer);

  if (db_globals.debug)
  {
    for (i = 0; i < sb_globals.threads; i++)
    {
      sb_timer_init(exec_timers + i);
      sb_timer_init(fetch_timers + i);
    }
  }
}
