/*
   Copyright (C) 2004 MySQL AB
   Copyright (C) 2004-2015 Alexey Kopytov <akopytov@gmail.com>

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
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include "db_driver.h"
#include "sb_list.h"
#include "sb_percentile.h"

/* Query length limit for bulk insert queries */
#define BULK_PACKET_SIZE (512*1024)

/* How many rows to insert before COMMITs (used in bulk insert) */
#define ROWS_BEFORE_COMMIT 1000

typedef struct {
  unsigned long   read_ops;
  unsigned long   write_ops;
  unsigned long   other_ops;
  unsigned long   transactions;
  unsigned long   errors;
  unsigned long   reconnects;
  pthread_mutex_t stat_mutex;
} db_thread_stat_t;

/* Global variables */
db_globals_t db_globals;

sb_percentile_t local_percentile;

/* Used in intermediate reports */
static unsigned long last_transactions;
static unsigned long last_read_ops;
static unsigned long last_write_ops;
static unsigned long last_errors;
static unsigned long last_reconnects;

/* Static variables */
static sb_list_t        drivers;          /* list of available DB drivers */
static db_thread_stat_t *thread_stats; /* per-thread stats */

/* Timers used in debug mode */
static sb_timer_t *exec_timers;
static sb_timer_t *fetch_timers;

/* Static functions */

static int db_parse_arguments(void);
static void db_free_row(db_row_t *);
static int db_bulk_do_insert(db_conn_t *, int);
static db_query_type_t db_get_query_type(const char *);
static void db_update_thread_stats(int, db_query_type_t);
static void db_reset_stats(void);

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
  {
    "db-debug", "print database-specific debug information",
    SB_ARG_TYPE_FLAG, "off"
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
  unsigned int   i;
  
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

  /* Initialize database driver */
  if (drv->ops.init())
    return NULL;

  /* Initialize per-thread stats */
  thread_stats = (db_thread_stat_t *)malloc(sb_globals.num_threads *
                                            sizeof(db_thread_stat_t));
  if (thread_stats == NULL)
    return NULL;

  for (i = 0; i < sb_globals.num_threads; i++)
    pthread_mutex_init(&thread_stats[i].stat_mutex, NULL);

  /* Initialize timers if in debug mode */
  if (db_globals.debug)
  {
    exec_timers = (sb_timer_t *) malloc(sb_globals.num_threads *
                                        sizeof(sb_timer_t));
    fetch_timers = (sb_timer_t *) malloc(sb_globals.num_threads *
                                         sizeof(sb_timer_t));
  }

  db_reset_stats();

  if (sb_percentile_init(&local_percentile, 100000, 1.0, 1e13))
    return NULL;

  return drv;
}


/* Describe database capabilities */


int db_describe(db_driver_t *drv, drv_caps_t *caps)
{
  if (drv->ops.describe == NULL)
    return 1;

  return drv->ops.describe(caps);
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


/* Associate connection with a thread (required only for statistics */

void db_set_thread(db_conn_t *con, int thread_id)
{
  con->thread_id = thread_id;
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

  return rc;
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

  stmt->type = db_get_query_type(query);
  
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


/* Bind results for prepared statement */


int db_bind_result(db_stmt_t *stmt, db_bind_t *results, unsigned int len)
{
  db_conn_t *con = stmt->connection;

  if (con == NULL || con->driver == NULL)
    return 1;

  return con->driver->ops.bind_result(stmt, results, len);
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

    if (con->db_errno == SB_DB_ERROR_RECONNECTED)
    {
      thread_stats[con->thread_id].reconnects++;
      con->db_errno = SB_DB_ERROR_RESTART_TRANSACTION;
    }
    else if (con->db_errno == SB_DB_ERROR_RESTART_TRANSACTION)
      thread_stats[con->thread_id].errors++;

    return NULL;
  }

  db_update_thread_stats(con->thread_id, stmt->type);
  
  return rs;
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

  if (con->db_errno == SB_DB_ERROR_NONE)
    db_update_thread_stats(con->thread_id, db_get_query_type(query));
  else
  {
    if (con->db_errno == SB_DB_ERROR_RECONNECTED)
    {
      thread_stats[con->thread_id].reconnects++;
      con->db_errno = SB_DB_ERROR_RESTART_TRANSACTION;
    }
    else if (con->db_errno == SB_DB_ERROR_RESTART_TRANSACTION)
      thread_stats[con->thread_id].errors++;

    return NULL;
  }

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
  if (db_globals.debug)
  {
    free(exec_timers);
    free(fetch_timers);
  }
  
  if (thread_stats != NULL)
  {
    unsigned int i;
    for (i = 0; i < sb_globals.num_threads; i++)
      pthread_mutex_destroy(&thread_stats[i].stat_mutex);
    free(thread_stats);
  }

  sb_percentile_done(&local_percentile);

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


/* Free row fetched by db_fetch_row() */


void db_free_row(db_row_t *row)
{
  free(row);
}


/* Initialize multi-row insert operation */


int db_bulk_insert_init(db_conn_t *con, const char *query)
{
  drv_caps_t driver_caps;
  size_t query_len;

  if (con->driver == NULL)
    return 1;
  
  /* Get database capabilites */
  if (db_describe(con->driver, &driver_caps))
  {
    log_text(LOG_FATAL, "failed to get database capabilities!");
    return 1;
  }

  /* Allocate query buffer */
  query_len = strlen(query);
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
  
  con->bulk_supported = driver_caps.multi_rows_insert;
  con->bulk_commit_max = driver_caps.needs_commit ? ROWS_BEFORE_COMMIT : 0;
  con->bulk_commit_cnt = 0;
  strcpy(con->bulk_buffer, query);
  con->bulk_ptr = query_len;
  con->bulk_ptr_orig = query_len;
  con->bulk_cnt = 0;

  return 0;
}

/* Add row to multi-row insert operation */

int db_bulk_insert_next(db_conn_t *con, const char *query)
{
  unsigned int query_len = strlen(query);

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

int db_bulk_do_insert(db_conn_t *con, int is_last)
{
  if (!con->bulk_cnt)
    return 0;
      
  if (db_query(con, con->bulk_buffer) == NULL)
    return 1;
  

  if (con->bulk_commit_max != 0)
  {
    con->bulk_commit_cnt += con->bulk_cnt;

    if (is_last || con->bulk_commit_cnt >= con->bulk_commit_max)
    {
      if (db_query(con, "COMMIT") == NULL)
        return 1;
      con->bulk_commit_cnt = 0;
    }
  }

  con->bulk_ptr = con->bulk_ptr_orig;
  con->bulk_cnt = 0;

  return 0;
}

/* Finish multi-row insert operation */

void db_bulk_insert_done(db_conn_t *con)
{
  /* Flush remaining data in buffer, if any */
  db_bulk_do_insert(con, 1);
  
  if (con->bulk_buffer != NULL)
  {
    free(con->bulk_buffer);
    con->bulk_buffer = NULL;
  }
}

/* Print database-specific test stats */

void db_print_stats(sb_stat_t type)
{
  double        seconds;
  unsigned int  i;
  sb_timer_t    exec_timer;
  sb_timer_t    fetch_timer;
  unsigned long read_ops;
  unsigned long write_ops;
  unsigned long other_ops;
  unsigned long transactions;
  unsigned long errors;
  unsigned long reconnects;

  /* Summarize per-thread counters */
  read_ops = write_ops = other_ops = transactions = errors = reconnects = 0;
  for (i = 0; i < sb_globals.num_threads; i++)
  {
    pthread_mutex_lock(&thread_stats[i].stat_mutex);
    read_ops += thread_stats[i].read_ops;
    write_ops += thread_stats[i].write_ops;
    other_ops += thread_stats[i].other_ops;
    transactions += thread_stats[i].transactions;
    errors += thread_stats[i].errors;
    reconnects += thread_stats[i].reconnects;
    pthread_mutex_unlock(&thread_stats[i].stat_mutex);
  }

  if (type == SB_STAT_INTERMEDIATE)
  {
    seconds = NS2SEC(sb_timer_split(&sb_globals.exec_timer));

    log_timestamp(LOG_NOTICE, &sb_globals.exec_timer,
                  "threads: %d, tps: %4.2f, reads: %4.2f, writes: %4.2f, "
                  "response time: %4.2fms (%u%%), errors: %4.2f, "
                  "reconnects: %5.2f",
                  sb_globals.num_running,
                  (transactions - last_transactions) / seconds,
                  (read_ops - last_read_ops) / seconds,
                  (write_ops - last_write_ops) / seconds,
                  NS2MS(sb_percentile_calculate(&local_percentile,
                                                sb_globals.percentile_rank)),
                  sb_globals.percentile_rank,
                  (errors - last_errors) / seconds,
                  (reconnects - last_reconnects) / seconds);
    if (sb_globals.tx_rate > 0)
    {
      pthread_mutex_lock(&event_queue_mutex);

      log_timestamp(LOG_NOTICE, &sb_globals.exec_timer,
                    "queue length: %d, concurrency: %d",
                    sb_globals.event_queue_length, sb_globals.concurrency);

      pthread_mutex_unlock(&event_queue_mutex);
    }

    SB_THREAD_MUTEX_LOCK();
    last_transactions = transactions;
    last_read_ops = read_ops;
    last_write_ops = write_ops;
    last_errors = errors;
    last_reconnects = reconnects;
    SB_THREAD_MUTEX_UNLOCK();

    sb_percentile_reset(&local_percentile);

    return;
  }
  else if (type != SB_STAT_CUMULATIVE)
    return;

  seconds = NS2SEC(sb_timer_split(&sb_globals.cumulative_timer1));

  log_text(LOG_NOTICE, "OLTP test statistics:");
  log_text(LOG_NOTICE, "    queries performed:");
  log_text(LOG_NOTICE, "        read:                            %d",
           read_ops);
  log_text(LOG_NOTICE, "        write:                           %d",
           write_ops);
  log_text(LOG_NOTICE, "        other:                           %d",
           other_ops);
  log_text(LOG_NOTICE, "        total:                           %d",
           read_ops + write_ops + other_ops);
  log_text(LOG_NOTICE, "    transactions:                        %-6d"
           " (%.2f per sec.)", transactions, transactions / seconds);
  log_text(LOG_NOTICE, "    read/write requests:                 %-6d"
           " (%.2f per sec.)", read_ops + write_ops,
           (read_ops + write_ops) / seconds);  
  log_text(LOG_NOTICE, "    other operations:                    %-6d"
           " (%.2f per sec.)", other_ops, other_ops / seconds);
  log_text(LOG_NOTICE, "    ignored errors:                      %-6d"
           " (%.2f per sec.)", errors, errors / seconds);
  log_text(LOG_NOTICE, "    reconnects:                          %-6d"
           " (%.2f per sec.)", reconnects, reconnects / seconds);

  if (db_globals.debug)
  {
    sb_timer_init(&exec_timer);
    sb_timer_init(&fetch_timer);

    for (i = 0; i < sb_globals.num_threads; i++)
    {
      exec_timer = merge_timers(&exec_timer, exec_timers + i);
      fetch_timer = merge_timers(&fetch_timer, fetch_timers + i);
    }

    log_text(LOG_DEBUG, "");
    log_text(LOG_DEBUG, "Query execution statistics:");
    log_text(LOG_DEBUG, "    min:                                %.4fs",
             NS2SEC(get_min_time(&exec_timer)));
    log_text(LOG_DEBUG, "    avg:                                %.4fs",
             NS2SEC(get_avg_time(&exec_timer)));
    log_text(LOG_DEBUG, "    max:                                %.4fs",
             NS2SEC(get_max_time(&exec_timer)));
    log_text(LOG_DEBUG, "  total:                                %.4fs",
             NS2SEC(get_sum_time(&exec_timer)));

    log_text(LOG_DEBUG, "Results fetching statistics:");
    log_text(LOG_DEBUG, "    min:                                %.4fs",
             NS2SEC(get_min_time(&fetch_timer)));
    log_text(LOG_DEBUG, "    avg:                                %.4fs",
             NS2SEC(get_avg_time(&fetch_timer)));
    log_text(LOG_DEBUG, "    max:                                %.4fs",
             NS2SEC(get_max_time(&fetch_timer)));
    log_text(LOG_DEBUG, "  total:                                %.4fs",
             NS2SEC(get_sum_time(&fetch_timer)));
  }

  db_reset_stats();
}

/* Get query type */

db_query_type_t db_get_query_type(const char *query)
{
  while (isspace(*query))
    query++;

  if (!strncasecmp(query, "select", 6))
    return DB_QUERY_TYPE_READ;

  if (!strncasecmp(query, "insert", 6) ||
      !strncasecmp(query, "update", 6) ||
      !strncasecmp(query, "delete", 6))
    return DB_QUERY_TYPE_WRITE;

  if (!strncasecmp(query, "commit", 6) ||
      !strncasecmp(query, "unlock tables", 13))
    return DB_QUERY_TYPE_COMMIT;
    
  
  return DB_QUERY_TYPE_OTHER;
}

/* Update stats according to type */

void db_update_thread_stats(int id, db_query_type_t type)
{
  if (id < 0)
    return;

  pthread_mutex_lock(&thread_stats[id].stat_mutex);

  switch (type)
  {
    case DB_QUERY_TYPE_READ:
      thread_stats[id].read_ops++;
      break;
    case DB_QUERY_TYPE_WRITE:
      thread_stats[id].write_ops++;
      break;
    case DB_QUERY_TYPE_COMMIT:
      thread_stats[id].other_ops++;
      thread_stats[id].transactions++;
      break;
    case DB_QUERY_TYPE_OTHER:
      thread_stats[id].other_ops++;
      break;
    default:
      log_text(LOG_WARNING, "Unknown query type: %d", type);
  }

  pthread_mutex_unlock(&thread_stats[id].stat_mutex);
}

static void db_reset_stats(void)
{
  unsigned int i;

  for(i = 0; i < sb_globals.num_threads; i++)
  {
    thread_stats[i].read_ops = 0;
    thread_stats[i].write_ops = 0;
    thread_stats[i].other_ops = 0;
    thread_stats[i].transactions = 0;
    thread_stats[i].errors = 0;
    thread_stats[i].reconnects = 0;
  }

  last_transactions = 0;
  last_read_ops = 0;
  last_write_ops = 0;
  last_errors = 0;

  /*
    So that intermediate stats are calculated from the current moment
    rather than from the previous intermediate report
  */
  if (sb_timer_running(&sb_globals.exec_timer))
    sb_timer_split(&sb_globals.exec_timer);

  if (db_globals.debug)
  {
    for (i = 0; i < sb_globals.num_threads; i++)
    {
      sb_timer_init(exec_timers + i);
      sb_timer_init(fetch_timers + i);
    }
  }
}
