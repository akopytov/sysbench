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

#include "sysbench.h"
#include "db_driver.h"
#include "sb_oltp.h"

#define GET_RANDOM_ID() ((*rnd_func)())

/* How many rows to insert in a single query (used for test DB creation) */
#define INSERT_ROWS 10000

/* How many rows to insert before COMMITs (used for test DB creation) */
#define ROWS_BEFORE_COMMIT 1000

/* Maximum query length */
#define MAX_QUERY_LEN 1024

/* Large prime number to generate unique set of random numbers in delete test */
#define LARGE_PRIME 2147483647

/* Command line arguments definition */
static sb_arg_t oltp_args[] =
{
  {"oltp-test-mode", "test type to use {simple,complex,nontrx,sp}", SB_ARG_TYPE_STRING, "complex"},
  {"oltp-reconnect-mode", "reconnect mode {session,transaction,query,random}", SB_ARG_TYPE_STRING,
   "session"},
  {"oltp-sp-name", "name of store procedure to call in SP test mode", SB_ARG_TYPE_STRING, ""},
  {"oltp-read-only", "generate only 'read' queries (do not modify database)", SB_ARG_TYPE_FLAG, "off"},
  {"oltp-skip-trx", "skip BEGIN/COMMIT statements", SB_ARG_TYPE_FLAG, "off"},
  {"oltp-range-size", "range size for range queries", SB_ARG_TYPE_INT, "100"},
  {"oltp-point-selects", "number of point selects", SB_ARG_TYPE_INT, "10"},
  {"oltp-simple-ranges", "number of simple ranges", SB_ARG_TYPE_INT, "1"},
  {"oltp-sum-ranges", "number of sum ranges", SB_ARG_TYPE_INT, "1"},
  {"oltp-order-ranges", "number of ordered ranges", SB_ARG_TYPE_INT, "1"},
  {"oltp-distinct-ranges", "number of distinct ranges", SB_ARG_TYPE_INT, "1"},
  {"oltp-index-updates", "number of index update", SB_ARG_TYPE_INT, "1"},
  {"oltp-non-index-updates", "number of non-index updates", SB_ARG_TYPE_INT, "1"},
  {"oltp-nontrx-mode",
   "mode for non-transactional test {select, update_key, update_nokey, insert, delete}",
   SB_ARG_TYPE_STRING, "select"},
  {"oltp-auto-inc", "whether AUTO_INCREMENT (or equivalent) should be used on id column",
   SB_ARG_TYPE_FLAG, "on"},
  {"oltp-connect-delay", "time in microseconds to sleep after connection to database", SB_ARG_TYPE_INT,
   "10000"},
  {"oltp-user-delay-min", "minimum time in microseconds to sleep after each request",
   SB_ARG_TYPE_INT, "0"},
  {"oltp-user-delay-max", "maximum time in microseconds to sleep after each request",
   SB_ARG_TYPE_INT, "0"},
  {"oltp-table-name", "name of test table", SB_ARG_TYPE_STRING, "sbtest"},
  {"oltp-table-size", "number of records in test table", SB_ARG_TYPE_INT, "10000"},

  {"oltp-dist-type", "random numbers distribution {uniform,gaussian,special}", SB_ARG_TYPE_STRING,
   "special"},
  {"oltp-dist-iter", "number of iterations used for numbers generation", SB_ARG_TYPE_INT, "12"},
  {"oltp-dist-pct", "percentage of values to be treated as 'special' (for special distribution)",
   SB_ARG_TYPE_INT, "1"},
  {"oltp-dist-res", "percentage of 'special' values to use (for special distribution)",
   SB_ARG_TYPE_INT, "75"},
  
  {NULL, NULL, SB_ARG_TYPE_NULL, NULL}
};

/* Test modes */
typedef enum
{
  TEST_MODE_SIMPLE,
  TEST_MODE_COMPLEX,
  TEST_MODE_NONTRX,
  TEST_MODE_SP
} oltp_mode_t;

/* Modes for 'non-transactional' test */
typedef enum
{
  NONTRX_MODE_SELECT,
  NONTRX_MODE_UPDATE_KEY,
  NONTRX_MODE_UPDATE_NOKEY,
  NONTRX_MODE_INSERT,
  NONTRX_MODE_DELETE
} nontrx_mode_t;

/* Random numbers distributions */
typedef enum
{
  DIST_TYPE_UNIFORM,
  DIST_TYPE_GAUSSIAN,
  DIST_TYPE_SPECIAL
} oltp_dist_t;

/*
  Some code in get_request_*() depends on the order in which the following
  constants are defined
*/
typedef enum {
  RECONNECT_SESSION,
  RECONNECT_QUERY,
  RECONNECT_TRANSACTION,
  RECONNECT_RANDOM,
  RECONNECT_LAST
} reconnect_mode_t;

typedef struct
{
  oltp_mode_t      test_mode;
  reconnect_mode_t reconnect_mode;
  unsigned int     read_only;
  unsigned int     skip_trx;
  unsigned int     auto_inc;
  unsigned int     range_size;
  unsigned int     point_selects;
  unsigned int     simple_ranges;
  unsigned int     sum_ranges;
  unsigned int     order_ranges;
  unsigned int     distinct_ranges;
  unsigned int     index_updates;
  unsigned int     non_index_updates;
  nontrx_mode_t    nontrx_mode;
  unsigned int     connect_delay;
  unsigned int     user_delay_min;
  unsigned int     user_delay_max;
  char             *table_name;
  char             *sp_name;
  unsigned int     table_size;
  oltp_dist_t      dist_type;
  unsigned int     dist_iter;
  unsigned int     dist_pct;
  unsigned int     dist_res;
} oltp_args_t;

/* Test statements structure */
typedef struct
{
  db_stmt_t *lock;
  db_stmt_t *unlock;
  db_stmt_t *point;
  db_stmt_t *call;
  db_stmt_t *range;
  db_stmt_t *range_sum;
  db_stmt_t *range_order;
  db_stmt_t *range_distinct;
  db_stmt_t *update_index;
  db_stmt_t *update_non_index;
  db_stmt_t *delete;
  db_stmt_t *insert;
} oltp_stmt_set_t;

/* Bind buffers for statements */
typedef struct
{
  sb_sql_query_point_t  point;
  sb_sql_query_range_t  range;
  sb_sql_query_range_t  range_sum;
  sb_sql_query_range_t  range_order;
  sb_sql_query_range_t  range_distinct;
  sb_sql_query_update_t update_index;
  sb_sql_query_update_t update_non_index;
  sb_sql_query_delete_t delete;
  sb_sql_query_insert_t insert;
  sb_sql_query_call_t   call;
  /* Buffer for the 'c' table field in update_non_index and insert queries */
  char                  c[120];
  unsigned long         c_len;
  /* Buffer for the 'pad' table field in insert query */
  char                  pad[60];
  unsigned long         pad_len;
} oltp_bind_set_t;

/* OLTP test commands */
static int oltp_cmd_help(void);
static int oltp_cmd_prepare(void);
static int oltp_cmd_cleanup(void);

/* OLTP test operations */
static int oltp_init(void);
static void oltp_print_mode(void);
static sb_request_t oltp_get_request(int);
static int oltp_execute_request(sb_request_t *, int);
static void oltp_print_stats(void);
static db_conn_t *oltp_connect(void);
static int oltp_disconnect(db_conn_t *);
static int oltp_reconnect(int thread_id);
static int oltp_done(void);

static sb_test_t oltp_test =
{
  "oltp",
  "OLTP test",
  {
    oltp_init,
    NULL,
    NULL,
    oltp_print_mode,
    oltp_get_request,
    oltp_execute_request,
    oltp_print_stats,
    NULL,
    NULL,
    oltp_done
  },
  {
    oltp_cmd_help,
    oltp_cmd_prepare,
    NULL,
    oltp_cmd_cleanup
  },
  oltp_args,
  {NULL, NULL}
};

/* Global variables */
static oltp_args_t args;                  /* test args */
static unsigned int (*rnd_func)(void);    /* pointer to random numbers generator */
static unsigned int req_performed;        /* number of requests done */
static db_conn_t **connections;           /* database connection pool */
static oltp_stmt_set_t *statements;       /* prepared statements pool */
static oltp_bind_set_t *bind_bufs;        /* bind buffers pool */
static reconnect_mode_t *reconnect_modes; /* per-thread reconnect modes */
static db_driver_t *driver;               /* current database driver */
static drv_caps_t driver_caps;            /* driver capabilities */

/* Statistic counters */
static int read_ops;
static int write_ops;
static int other_ops;
static int transactions;
static int deadlocks;

static sb_timer_t *exec_timers;
static sb_timer_t *fetch_timers;

/* Random seed used to generate unique random numbers */
static unsigned long long rnd_seed;
/* Mutex to protect random seed */
static pthread_mutex_t    rnd_mutex;

/* Variable to pass is_null flag to drivers */

static char oltp_is_null = 1;

/* Parse command line arguments */
static int parse_arguments(void);

/* Random number generators */
static unsigned int rnd_func_uniform(void);
static unsigned int rnd_func_gaussian(void);
static unsigned int rnd_func_special(void);
static unsigned int get_unique_random_id(void);

/* SQL request generators */
static sb_request_t get_request_simple(int);
static sb_request_t get_request_complex(int);
static sb_request_t get_request_nontrx(int);
static sb_request_t get_request_sp(int);

/* Adds a 'reconnect' request to the list of SQL queries */
static inline int add_reconnect_req(sb_list_t *list);

/* Get random 'user think' time */
static int get_think_time(void);

/* Generate SQL statement from query */
static db_stmt_t *get_sql_statement(sb_sql_query_t *, int);
static db_stmt_t *get_sql_statement_trx(sb_sql_query_t *, int);
static db_stmt_t *get_sql_statement_nontrx(sb_sql_query_t *, int);
static db_stmt_t *get_sql_statement_sp(sb_sql_query_t *, int);

/* Prepare a set of statements for test */
static int prepare_stmt_set(oltp_stmt_set_t *, oltp_bind_set_t *, db_conn_t *);
static int prepare_stmt_set_trx(oltp_stmt_set_t *, oltp_bind_set_t *, db_conn_t *);
static int prepare_stmt_set_nontrx(oltp_stmt_set_t *, oltp_bind_set_t *, db_conn_t *);
static int prepare_stmt_set_sp(oltp_stmt_set_t *, oltp_bind_set_t *, db_conn_t *);

/* Close a set of statements */
void close_stmt_set(oltp_stmt_set_t *set);

int register_test_oltp(sb_list_t *tests)
{
  /* Register database API */
  if (db_register())
    return 1;
  
  /* Register OLTP test */
  SB_LIST_ADD_TAIL(&oltp_test.listitem, tests);
  
  return 0;
}


int oltp_cmd_help(void)
{
  db_print_help();
  
  return 0;
}


int oltp_cmd_prepare(void)
{
  db_conn_t      *con;
  char           *query = NULL;
  unsigned int   query_len;
  unsigned int   i;
  unsigned int   j;
  unsigned int   n;
  unsigned long  nrows;
  unsigned long  commit_cntr = 0;
  char           insert_str[MAX_QUERY_LEN];
  char           *pos;
  char           *table_options_str;
  
  if (parse_arguments())
    return 1;

  /* Get database capabilites */
  if (db_describe(driver, &driver_caps, NULL))
  {
    log_text(LOG_FATAL, "failed to get database capabilities!");
    return 1;
  }
  
  /* Create database connection */
  con = oltp_connect();
  if (con == NULL)
    return 1;

  /* Determine if database supports multiple row inserts */
  if (driver_caps.multi_rows_insert)
    nrows = INSERT_ROWS;
  else
    nrows = 1;
  
  /* Prepare statement buffer */
  if (args.auto_inc)
    snprintf(insert_str, sizeof(insert_str),
             "(0,' ','qqqqqqqqqqwwwwwwwwwweeeeeeeeeerrrrrrrrrrtttttttttt')");
  else
    snprintf(insert_str, sizeof(insert_str),
             "(%d,0,' ','qqqqqqqqqqwwwwwwwwwweeeeeeeeeerrrrrrrrrrtttttttttt')",
             args.table_size);
  
  query_len = MAX_QUERY_LEN + nrows * (strlen(insert_str) + 1);
  query = (char *)malloc(query_len);
  if (query == NULL)
  {
    log_text(LOG_FATAL, "memory allocation failure!");
    goto error;
  }
  
  /* Create test table */
  log_text(LOG_NOTICE, "Creating table '%s'...", args.table_name);
  table_options_str = driver_caps.table_options_str;
  snprintf(query, query_len,
           "CREATE TABLE %s ("
           "id %s %s NOT NULL %s, "
           "k integer %s DEFAULT '0' NOT NULL, "
           "c char(120) DEFAULT '' NOT NULL, "
           "pad char(60) DEFAULT '' NOT NULL, "
           "PRIMARY KEY  (id) "
           ") %s",
           args.table_name,
           (args.auto_inc && driver_caps.serial) ? "SERIAL" : "INTEGER",
           driver_caps.unsigned_int ? "UNSIGNED" : "",
           (args.auto_inc && driver_caps.auto_increment) ? "AUTO_INCREMENT" : "",
           driver_caps.unsigned_int ? "UNSIGNED" : "",
           (table_options_str != NULL) ? table_options_str : ""
           );
  if (db_query(con, query) == NULL)
  {
    log_text(LOG_FATAL, "failed to create test table");
    goto error;
  }  

  if (args.auto_inc && !driver_caps.serial && !driver_caps.auto_increment)
  {
    if (db_query(con, "CREATE SEQUENCE sbtest_seq") == NULL ||
        db_query(con,
                 "CREATE TRIGGER sbtest_trig BEFORE INSERT ON sbtest "
                 "FOR EACH ROW "
                 "BEGIN SELECT sbtest_seq.nextval INTO :new.id FROM DUAL; "
                 "END;")
        == NULL)
    {
      log_text(LOG_FATAL, "failed to create test table");
      goto error;
    }
  }
  
  /* Create secondary index on 'k' */
  snprintf(query, query_len,
           "CREATE INDEX k on %s(k)",
           args.table_name);
  if (db_query(con, query) == NULL)
  {
    log_text(LOG_FATAL, "failed to create secondary index on table!");
    goto error;
  }
  /* Fill test table with data */
  log_text(LOG_NOTICE, "Creating %d records in table '%s'...", args.table_size,
         args.table_name);

  for (i = 0; i < args.table_size; i += nrows)
  {
    /* Build query */
    if (args.auto_inc)
      n = snprintf(query, query_len, "INSERT INTO %s(k, c, pad) VALUES ",
                   args.table_name);
    else
      n = snprintf(query, query_len, "INSERT INTO %s(id, k, c, pad) VALUES ",
                   args.table_name);
    if (n >= query_len)
    {
      log_text(LOG_FATAL, "query is too long!");
      goto error;
    }
    pos = query + n;
    for (j = 0; j < nrows; j++)
    {
      if ((unsigned)(pos - query) >= query_len)
      {
        log_text(LOG_FATAL, "query is too long!");
        goto error;
      }

      /* Form the values string when if are not using auto_inc */
      if (!args.auto_inc)
        snprintf(insert_str, sizeof(insert_str),
                 "(%d,0,' ','qqqqqqqqqqwwwwwwwwwweeeeeeeeeerrrrrrrrrrtttttttttt')",
                 i + j + 1);
      
      if (j == nrows - 1 || i+j == args.table_size -1)
        n = snprintf(pos, query_len - (pos - query), "%s", insert_str);
      else
        n = snprintf(pos, query_len - (pos - query), "%s,", insert_str);
      if (n >= query_len - (pos - query))
      {
        log_text(LOG_FATAL, "query is too long!");
        goto error;
      }
      if (i+j == args.table_size - 1)
        break;
      pos += n;
    }
    
    /* Execute query */
    if (db_query(con, query) == NULL)
    {
      log_text(LOG_FATAL, "failed to create test table!");
      goto error;
    }

    if (driver_caps.needs_commit)
    {
      commit_cntr += nrows;
      if (commit_cntr >= ROWS_BEFORE_COMMIT)
      {
        if (db_query(con, "COMMIT") == NULL)
        {
          log_text(LOG_FATAL, "failed to commit inserted rows!");
          goto error;
        }
        commit_cntr -= ROWS_BEFORE_COMMIT;
      }
    }
  }

  if (driver_caps.needs_commit && db_query(con, "COMMIT") == NULL)
  {
    if (db_query(con, "COMMIT") == NULL)
    {
      log_text(LOG_FATAL, "failed to commit inserted rows!");
      return 1;
    }
  }

  oltp_disconnect(con);
  
  return 0;

 error:
  oltp_disconnect(con);
  if (query != NULL)
    free(query);

  return 1;
}

int oltp_cmd_cleanup(void)
{
  db_conn_t *con;
  char      query[256];
  
  if (parse_arguments())
    return 1;

  /* Get database capabilites */
  if (db_describe(driver, &driver_caps, NULL))
  {
    log_text(LOG_FATAL, "failed to get database capabilities!");
    return 1;
  }

  /* Create database connection */
  con = oltp_connect();
  if (con == NULL)
    return 1;

  /* Drop the test table */
  log_text(LOG_NOTICE, "Dropping table '%s'...", args.table_name);
  snprintf(query, sizeof(query), "DROP TABLE %s", args.table_name);
  if (db_query(con, query) == NULL)
  {
    oltp_disconnect(con);
    return 1;
  }

  oltp_disconnect(con);
  log_text(LOG_INFO, "Done.");
  
  return 0;
}

int oltp_init(void)
{
  db_conn_t    *con;
  unsigned int thread_id;
  char         query[MAX_QUERY_LEN];

  if (parse_arguments())
    return 1;
  
  /* Get database capabilites */
  if (db_describe(driver, &driver_caps, args.table_name))
  {
    log_text(LOG_FATAL, "failed to get database capabilities!");
    return 1;
  }
  
  /* Truncate table in case of nontrx INSERT test */
  if (args.test_mode == TEST_MODE_NONTRX && args.nontrx_mode == NONTRX_MODE_INSERT)
  {
    con = oltp_connect();
    if (con == NULL)
      return 1;
    snprintf(query, sizeof(query), "TRUNCATE TABLE %s", args.table_name);
    if (db_query(con, query) == NULL)
      return 1;
    oltp_disconnect(con);
  }
  
  /* Allocate database connection pool */
  connections = (db_conn_t **)malloc(sb_globals.num_threads * sizeof(db_conn_t *));
  if (connections == NULL)
  {
    log_text(LOG_FATAL, "failed to allocate DB connection pool!");
    return 1;
  }

  /* Create database connections */
  for (thread_id = 0; thread_id < sb_globals.num_threads; thread_id++)
  {
    connections[thread_id] = oltp_connect();
    if (connections[thread_id] == NULL)
    {
      log_text(LOG_FATAL, "thread#%d: failed to connect to database server, aborting...",
             thread_id);
      return 1;
    }
  }

  /* Allocate statements pool */
  statements = (oltp_stmt_set_t *)calloc(sb_globals.num_threads,
                                          sizeof(oltp_stmt_set_t));
  if (statements == NULL)
  {
    log_text(LOG_FATAL, "failed to allocate statements pool!");
    return 1;
  }
  
  /* Allocate bind buffers for each thread */
  bind_bufs = (oltp_bind_set_t *)calloc(sb_globals.num_threads,
                                        sizeof(oltp_bind_set_t));
  /* Prepare statements for each thread */
  for (thread_id = 0; thread_id < sb_globals.num_threads; thread_id++)
  {
    if (prepare_stmt_set(statements + thread_id, bind_bufs + thread_id,
                         connections[thread_id]))
    {
      log_text(LOG_FATAL, "thread#%d: failed to prepare statements for test",
             thread_id);
      return 1;
    }
  }

  /* Per-thread reconnect modes */
  if (!(reconnect_modes = (reconnect_mode_t *)calloc(sb_globals.num_threads,
                                                     sizeof(reconnect_mode_t))))
    return 1;
  
  /* Initialize random seed for non-transactional delete test */
  if (args.test_mode == TEST_MODE_NONTRX)
  {
    rnd_seed = LARGE_PRIME;
    pthread_mutex_init(&rnd_mutex, NULL);
  }

  /* Initialize internal timers if we are in the debug mode */
  if (sb_globals.debug)
  {
    exec_timers = (sb_timer_t *)malloc(sb_globals.num_threads * sizeof(sb_timer_t));
    fetch_timers = (sb_timer_t *)malloc(sb_globals.num_threads * sizeof(sb_timer_t));
    for (thread_id = 0; thread_id < sb_globals.num_threads; thread_id++)
    {
      sb_timer_init(exec_timers + thread_id);
      sb_timer_init(fetch_timers + thread_id);
    }
  }
  
  return 0;
}


int oltp_done(void)
{
  unsigned int thread_id;

  if (args.test_mode == TEST_MODE_NONTRX)
    pthread_mutex_destroy(&rnd_mutex);

  /* Close statements and database connections */
  for (thread_id = 0; thread_id < sb_globals.num_threads; thread_id++)
  {
    close_stmt_set(statements + thread_id);
    oltp_disconnect(connections[thread_id]);
  }

  /* Deallocate connection pool */
  free(connections);

  free(bind_bufs);
  free(statements);
  
  return 0;
}


void oltp_print_mode(void)
{
  log_text(LOG_NOTICE, "Doing OLTP test.");
  
  switch (args.test_mode) {
    case TEST_MODE_SIMPLE:
      log_text(LOG_NOTICE, "Running simple OLTP test");
      break;
    case TEST_MODE_COMPLEX:
      log_text(LOG_NOTICE, "Running mixed OLTP test");
      break;
    case TEST_MODE_NONTRX:
      log_text(LOG_NOTICE, "Running non-transactional test");
      break;
    case TEST_MODE_SP:
      log_text(LOG_NOTICE, "Running stored procedure test");
      return;
      break;
    default:
      log_text(LOG_WARNING, "Unknown OLTP test mode!");
      break;
  }

  if (args.read_only)
    log_text(LOG_NOTICE, "Doing read-only test");
  
  switch (args.dist_type) {
    case DIST_TYPE_UNIFORM:
      log_text(LOG_NOTICE, "Using Uniform distribution");
      break;
    case DIST_TYPE_GAUSSIAN:
      log_text(LOG_NOTICE, "Using Normal distribution (%d iterations)",
               args.dist_iter);
      break;
    case DIST_TYPE_SPECIAL:
      log_text(LOG_NOTICE, "Using Special distribution (%d iterations,  "
               "%d pct of values are returned in %d pct cases)",
               args.dist_iter, args.dist_pct, args.dist_res);
      break;
    default:
      log_text(LOG_WARNING, "Unknown distribution!");
      break;
  }

  if (args.skip_trx)
    log_text(LOG_NOTICE, "Skipping BEGIN/COMMIT");
  else
    log_text(LOG_NOTICE, "Using \"%s%s\" for starting transactions",
             driver_caps.transactions ? "BEGIN" : "LOCK TABLES",
             (driver_caps.transactions) ? "" :
             ((args.read_only) ? " READ" : " WRITE"));

  if (args.auto_inc)
    log_text(LOG_NOTICE, "Using auto_inc on the id column");
  else
    log_text(LOG_NOTICE, "Not using auto_inc on the id column");
  
  if (sb_globals.max_requests > 0)
    log_text(LOG_NOTICE,
             "Maximum number of requests for OLTP test is limited to %d",
             sb_globals.max_requests);
  if (sb_globals.validate)
    log_text(LOG_NOTICE, "Validation mode enabled");
}


sb_request_t oltp_get_request(int tid)
{
  sb_request_t sb_req;

  if (sb_globals.max_requests > 0 && req_performed >= sb_globals.max_requests)
  {
    sb_req.type = SB_REQ_TYPE_NULL;
    return sb_req;
  }

  switch (args.test_mode) {
    case TEST_MODE_SIMPLE:
      return get_request_simple(tid);
    case TEST_MODE_COMPLEX:
      return get_request_complex(tid);
    case TEST_MODE_NONTRX:
      return get_request_nontrx(tid);
    case TEST_MODE_SP:
      return get_request_sp(tid);
    default:
      log_text(LOG_FATAL, "unknown test mode: %d!", args.test_mode);
      sb_req.type = SB_REQ_TYPE_NULL;
  }
  
  return sb_req;
}


inline int add_reconnect_req(sb_list_t *list)
{
  sb_sql_query_t *query;

  query = (sb_sql_query_t *)malloc(sizeof(sb_sql_query_t));
  query->num_times = 1;
  query->type = SB_SQL_QUERY_RECONNECT;
  query->nrows = 0;
  query->think_time = get_think_time();
  SB_LIST_ADD_TAIL(&query->listitem, list);
  return 0;
}

sb_request_t get_request_sp(int tid)
{
  sb_request_t     sb_req;
  sb_sql_request_t *sql_req = &sb_req.u.sql_request;
  sb_sql_query_t   *query;

  (void)tid; /* unused */
  
  sb_req.type = SB_REQ_TYPE_SQL;
  
  sql_req->queries = (sb_list_t *)malloc(sizeof(sb_list_t));
  query = (sb_sql_query_t *)malloc(sizeof(sb_sql_query_t));
  if (sql_req->queries == NULL || query == NULL)
  {
    log_text(LOG_FATAL, "cannot allocate SQL query!");
    sb_req.type = SB_REQ_TYPE_NULL;
    return sb_req;
  }
  
  SB_LIST_INIT(sql_req->queries);
  query->num_times = 1;
  query->think_time = get_think_time();
  query->type = SB_SQL_QUERY_CALL;
  query->nrows = 0;
  SB_LIST_ADD_TAIL(&query->listitem, sql_req->queries);

  if (args.reconnect_mode == RECONNECT_QUERY ||
      (args.reconnect_mode == RECONNECT_RANDOM && sb_rnd() % 2))
    add_reconnect_req(sql_req->queries);
  
  req_performed++;
 
  return sb_req;
}


sb_request_t get_request_simple(int tid)
{
  sb_request_t        sb_req;
  sb_sql_request_t    *sql_req = &sb_req.u.sql_request;
  sb_sql_query_t      *query;

  (void)tid; /* unused */
  
  sb_req.type = SB_REQ_TYPE_SQL;
  
  sql_req->queries = (sb_list_t *)malloc(sizeof(sb_list_t));
  query = (sb_sql_query_t *)malloc(sizeof(sb_sql_query_t));
  if (sql_req->queries == NULL || query == NULL)
  {
    log_text(LOG_FATAL, "cannot allocate SQL query!");
    sb_req.type = SB_REQ_TYPE_NULL;
    return sb_req;
  }

  SB_LIST_INIT(sql_req->queries);
  query->num_times = 1;
  query->think_time = get_think_time();
  query->type = SB_SQL_QUERY_POINT;
  query->u.point_query.id = GET_RANDOM_ID();
  query->nrows = 1;
  SB_LIST_ADD_TAIL(&query->listitem, sql_req->queries);
  
  if (args.reconnect_mode == RECONNECT_QUERY ||
      (args.reconnect_mode == RECONNECT_RANDOM && sb_rnd() % 2))
    add_reconnect_req(sql_req->queries);
  
  req_performed++;
  
  return sb_req;
}


sb_request_t get_request_complex(int tid)
{
  sb_request_t        sb_req;
  sb_sql_request_t    *sql_req = &sb_req.u.sql_request;
  sb_sql_query_t      *query;
  sb_list_item_t      *pos;
  sb_list_item_t      *tmp;
  unsigned int        i;
  unsigned int        range;
  reconnect_mode_t    rmode;
  
  sb_req.type = SB_REQ_TYPE_SQL;

  sql_req->queries = (sb_list_t *)malloc(sizeof(sb_list_t));
  if (sql_req->queries == NULL)
  {
    log_text(LOG_FATAL, "cannot allocate SQL query!");
    sb_req.type = SB_REQ_TYPE_NULL;
    return sb_req;
  }
  SB_LIST_INIT(sql_req->queries);

  if (args.reconnect_mode == RECONNECT_RANDOM)
  {
    rmode = reconnect_modes[tid];
    reconnect_modes[tid] = sb_rnd() % RECONNECT_RANDOM;
    if (rmode == RECONNECT_SESSION &&
        reconnect_modes[tid] != RECONNECT_SESSION)
      add_reconnect_req(sql_req->queries);
    rmode = reconnect_modes[tid];
  }
  else
    rmode = args.reconnect_mode;
  
  if (!args.skip_trx)
  {
    /* Generate BEGIN statement */
    query = (sb_sql_query_t *)malloc(sizeof(sb_sql_query_t));
    if (query == NULL)
      goto memfail;
    query->type = SB_SQL_QUERY_LOCK;
    query->num_times = 1;
    query->think_time = 0;
    SB_LIST_ADD_TAIL(&query->listitem, sql_req->queries);
  }
  
  /* Generate set of point selects */
  for(i = 0; i < args.point_selects; i++)
  {
    query = (sb_sql_query_t *)malloc(sizeof(sb_sql_query_t));
    if (query == NULL)
      goto memfail;
    query->num_times = 1;
    query->think_time = get_think_time();
    query->type = SB_SQL_QUERY_POINT;
    query->u.point_query.id = GET_RANDOM_ID();
    query->nrows = 1;
    SB_LIST_ADD_TAIL(&query->listitem, sql_req->queries);
    if (rmode == RECONNECT_QUERY)
      add_reconnect_req(sql_req->queries);
  }
  
  /* Generate range queries */
  for(i = 0; i < args.simple_ranges; i++)
  {
    query = (sb_sql_query_t *)malloc(sizeof(sb_sql_query_t));
    if (query == NULL)
      goto memfail;
    query->num_times = 1;
    query->think_time = get_think_time();
    query->type = SB_SQL_QUERY_RANGE;
    range = GET_RANDOM_ID();
    if (range + args.range_size > args.table_size)
      range = args.table_size - args.range_size;
    if (range < 1)
      range = 1;     
    query->u.range_query.from = range;
    query->u.range_query.to = range + args.range_size - 1;
    query->nrows = args.range_size;
    SB_LIST_ADD_TAIL(&query->listitem, sql_req->queries);
    if (rmode == RECONNECT_QUERY)
      add_reconnect_req(sql_req->queries);
  }
  
  /* Generate sum range queries */
  for(i = 0; i < args.sum_ranges; i++)
  {
    query = (sb_sql_query_t *)malloc(sizeof(sb_sql_query_t));
    if (query == NULL)
      goto memfail;
    query->num_times = 1;
    query->think_time = get_think_time();
    query->type = SB_SQL_QUERY_RANGE_SUM;
    range = GET_RANDOM_ID();
    if (range + args.range_size > args.table_size)
      range = args.table_size - args.range_size;
    if (range < 1)
      range = 1;
    query->u.range_query.from = range;
    query->u.range_query.to = range + args.range_size - 1;
    query->nrows = 1;
    SB_LIST_ADD_TAIL(&query->listitem, sql_req->queries);
    if (rmode == RECONNECT_QUERY)
      add_reconnect_req(sql_req->queries);
  }

  /* Generate ordered range queries */
  for(i = 0; i < args.order_ranges; i++)
  {
    query = (sb_sql_query_t *)malloc(sizeof(sb_sql_query_t));
    if (query == NULL)
      goto memfail;
    query->num_times = 1;
    query->think_time = get_think_time();
    query->type = SB_SQL_QUERY_RANGE_ORDER;
    range = GET_RANDOM_ID();
    if (range + args.range_size > args.table_size)
      range = args.table_size - args.range_size;
    if (range < 1)
      range = 1;
    query->u.range_query.from = range;
    query->u.range_query.to = range + args.range_size - 1;
    query->nrows = args.range_size;
    SB_LIST_ADD_TAIL(&query->listitem, sql_req->queries);
    if (rmode == RECONNECT_QUERY)
      add_reconnect_req(sql_req->queries);
  }

  /* Generate distinct range queries */
  for(i = 0; i < args.distinct_ranges; i++)
  {
    query = (sb_sql_query_t *)malloc(sizeof(sb_sql_query_t));
    if (query == NULL)
      goto memfail;
    query->num_times = 1;
    query->think_time = get_think_time();
    query->type = SB_SQL_QUERY_RANGE_DISTINCT;
    range = GET_RANDOM_ID();
    if (range + args.range_size > args.table_size)
      range = args.table_size - args.range_size;
    if (range < 1)
      range = 1;     
    query->u.range_query.from = range;
    query->u.range_query.to = range + args.range_size;
    query->nrows = 0;
    SB_LIST_ADD_TAIL(&query->listitem, sql_req->queries);
    if (rmode == RECONNECT_QUERY)
      add_reconnect_req(sql_req->queries);
  }

  /* Skip all write queries for read-only test mode */
  if (args.read_only)
    goto readonly;
  
  /* Generate index update */
  for (i = 0; i < args.index_updates; i++)
  {
    query = (sb_sql_query_t *)malloc(sizeof(sb_sql_query_t));
    if (query == NULL)
      goto memfail;
    query->num_times = 1;
    query->think_time = get_think_time();
    query->type = SB_SQL_QUERY_UPDATE_INDEX;
    query->u.update_query.id = GET_RANDOM_ID();
    SB_LIST_ADD_TAIL(&query->listitem, sql_req->queries);
    if (rmode == RECONNECT_QUERY)
      add_reconnect_req(sql_req->queries);
  }
  
  /* Generate non-index update */
  for (i = 0; i < args.non_index_updates; i++)
  {
    query = (sb_sql_query_t *)malloc(sizeof(sb_sql_query_t));
    if (query == NULL)
      goto memfail;
    query->num_times = 1;
    query->think_time = get_think_time();
    query->type = SB_SQL_QUERY_UPDATE_NON_INDEX;
    query->u.update_query.id = GET_RANDOM_ID();
    SB_LIST_ADD_TAIL(&query->listitem, sql_req->queries);
    if (rmode == RECONNECT_QUERY)
      add_reconnect_req(sql_req->queries);
  }
  
  /* FIXME: generate one more UPDATE with the same ID as DELETE/INSERT to make
     PostgreSQL work */
  query = (sb_sql_query_t *)malloc(sizeof(sb_sql_query_t));
  if (query == NULL)
    goto memfail;
  query->num_times = 1;
  query->think_time = get_think_time();
  query->type = SB_SQL_QUERY_UPDATE_INDEX;
  range = GET_RANDOM_ID();
  query->u.update_query.id = range;
  SB_LIST_ADD_TAIL(&query->listitem, sql_req->queries);
  
  /* Generate delete */
  query = (sb_sql_query_t *)malloc(sizeof(sb_sql_query_t));
  if (query == NULL)
    goto memfail;
  query->num_times = 1;
  query->think_time = get_think_time();
  query->type = SB_SQL_QUERY_DELETE;
  /* FIXME  range = GET_RANDOM_ID(); */
  query->u.delete_query.id = range;
  SB_LIST_ADD_TAIL(&query->listitem, sql_req->queries);

  /* Generate insert with same value */
  query = (sb_sql_query_t *)malloc(sizeof(sb_sql_query_t));
  if (query == NULL)
    goto memfail;
  query->num_times = 1;
  query->think_time = get_think_time();
  query->type = SB_SQL_QUERY_INSERT;
  query->u.insert_query.id = range;
  SB_LIST_ADD_TAIL(&query->listitem, sql_req->queries);

 readonly:
  
  if (!args.skip_trx)
  {
    /* Generate commit */
    query = (sb_sql_query_t *)malloc(sizeof(sb_sql_query_t));
    if (query == NULL)
      goto memfail;
    query->type = SB_SQL_QUERY_UNLOCK;
    query->num_times = 1;
    query->think_time = 0;
    SB_LIST_ADD_TAIL(&query->listitem, sql_req->queries);
    if (rmode == RECONNECT_TRANSACTION)
      add_reconnect_req(sql_req->queries);
  }
  
  /* return request */
  req_performed++;
  return sb_req;

  /* Handle memory allocation failures */
 memfail:
  log_text(LOG_FATAL, "cannot allocate SQL query!");
  SB_LIST_FOR_EACH_SAFE(pos, tmp, sql_req->queries)
  {
    query = SB_LIST_ENTRY(pos, sb_sql_query_t, listitem);
    free(query);
  }
  free(sql_req->queries);
  sb_req.type = SB_REQ_TYPE_NULL;
  return sb_req;
}


sb_request_t get_request_nontrx(int tid)
{
  sb_request_t        sb_req;
  sb_sql_request_t    *sql_req = &sb_req.u.sql_request;
  sb_sql_query_t      *query;

  (void)tid; /* unused */
  
  sb_req.type = SB_REQ_TYPE_SQL;

  sql_req->queries = (sb_list_t *)malloc(sizeof(sb_list_t));
  if (sql_req->queries == NULL)
  {
    log_text(LOG_FATAL, "cannot allocate SQL query!");
    sb_req.type = SB_REQ_TYPE_NULL;
    return sb_req;
  }
  SB_LIST_INIT(sql_req->queries);

  query = (sb_sql_query_t *)malloc(sizeof(sb_sql_query_t));
  if (query == NULL)
    goto memfail;
  query->num_times = 1;
  query->think_time = get_think_time();
  
  switch (args.nontrx_mode) {
    case NONTRX_MODE_SELECT:
      query->type = SB_SQL_QUERY_POINT;
      query->u.point_query.id = GET_RANDOM_ID();
      query->nrows = 1;
      break;
    case NONTRX_MODE_UPDATE_KEY:
      query->type = SB_SQL_QUERY_UPDATE_INDEX;
      query->u.update_query.id = GET_RANDOM_ID();
      break;
    case NONTRX_MODE_UPDATE_NOKEY:
      query->type = SB_SQL_QUERY_UPDATE_NON_INDEX;
      query->u.update_query.id = GET_RANDOM_ID();
      break;
    case NONTRX_MODE_INSERT:
      query->type = SB_SQL_QUERY_INSERT;
      query->u.update_query.id = GET_RANDOM_ID();
      break;
    case NONTRX_MODE_DELETE:
      query->type = SB_SQL_QUERY_DELETE;
      query->u.delete_query.id = get_unique_random_id();
      break;
    default:
      log_text(LOG_FATAL, "unknown mode for non-transactional test!");
      free(query);
      sb_req.type = SB_REQ_TYPE_NULL;
      break;
  }

  SB_LIST_ADD_TAIL(&query->listitem, sql_req->queries);

  if (args.reconnect_mode == RECONNECT_QUERY ||
      (args.reconnect_mode == RECONNECT_RANDOM && sb_rnd() % 2))
    add_reconnect_req(sql_req->queries);
  
  /* return request */
  req_performed++;
  return sb_req;

  /* Handle memory allocation failures */
 memfail:
  log_text(LOG_FATAL, "cannot allocate SQL query!");
  if (query)
    free(query);
  free(sql_req->queries);
  sb_req.type = SB_REQ_TYPE_NULL;
  return sb_req;
}


/*
 * We measure read operations, write operations and transactions
 * performance. The time is counted for atomic operations as user might sleep
 * before some of them.
 */


int oltp_execute_request(sb_request_t *sb_req, int thread_id)
{
  db_stmt_t           *stmt;
  sb_sql_request_t    *sql_req = &sb_req->u.sql_request;
  db_error_t          rc;
  db_result_set_t     *rs;
  sb_list_item_t      *pos;
  sb_list_item_t      *tmp;
  sb_sql_query_t      *query;
  unsigned int        i;
  unsigned int        local_read_ops=0;
  unsigned int        local_write_ops=0;
  unsigned int        local_other_ops=0;
  unsigned int        local_deadlocks=0;
  int                 retry;
  log_msg_t           msg;
  log_msg_oper_t      op_msg;
  unsigned long long  nrows;
  
  /* Prepare log message */
  msg.type = LOG_MSG_TYPE_OPER;
  msg.data = &op_msg;

  /* measure the time for transaction */
  LOG_EVENT_START(msg, thread_id);

  do  /* deadlock handling */
  {
    retry = 0;
    SB_LIST_FOR_EACH(pos, sql_req->queries)
    {
      query = SB_LIST_ENTRY(pos, sb_sql_query_t, listitem);

      for(i = 0; i < query->num_times; i++)
      {
        /* emulate user thinking */
        if (query->think_time > 0)
          usleep(query->think_time); 

        if (query->type == SB_SQL_QUERY_RECONNECT)
        {
          if (oltp_reconnect(thread_id))
            return 1;
          continue;
        }
        
        /* find prepared statement */
        stmt = get_sql_statement(query, thread_id);
        if (stmt == NULL)
        {
          log_text(LOG_FATAL, "unknown SQL query type: %d!", query->type);
          sb_globals.error = 1;
          return 1;
        }

        if (sb_globals.debug)
          sb_timer_start(exec_timers + thread_id);

        rs = db_execute(stmt);

        if (sb_globals.debug)
          sb_timer_stop(exec_timers + thread_id);
          
        if (rs == NULL)
        {
          rc = db_errno(connections[thread_id]);
          if (rc != SB_DB_ERROR_DEADLOCK)
          {
            log_text(LOG_FATAL, "database error, exiting...");
            /* exiting, forget about allocated memory */
            sb_globals.error = 1;
            return 1; 
          }  
          else
          {
            local_deadlocks++;
            retry = 1;
            /* exit for loop */
            break;  
          }
        }
        
        if (query->type >= SB_SQL_QUERY_POINT &&
          query->type <= SB_SQL_QUERY_RANGE_DISTINCT) /* select query */
        {
          if (sb_globals.debug)
            sb_timer_start(fetch_timers + thread_id);
          
          rc = db_store_results(rs);

          if (sb_globals.debug)
            sb_timer_stop(fetch_timers + thread_id);
          
          
          if (rc == SB_DB_ERROR_DEADLOCK)
          {
            db_free_results(rs);
            local_deadlocks++;
            retry = 1;
            break;  
          }
          else if (rc != SB_DB_ERROR_NONE)
          {
            log_text(LOG_FATAL, "Error fetching result: `%s`", stmt);
            /* exiting, forget about allocated memory */
            sb_globals.error = 1;
            return 1; 
          }

          /* Validate the result set if requested */
          if (sb_globals.validate && query->nrows > 0)
          {
            nrows = db_num_rows(rs);
            if (nrows != query->nrows)
              log_text(LOG_WARNING,
                       "Number of received rows mismatch, expected: %ld, actual: %ld",
                       (long )query->nrows, (long)nrows);
          }
          
        }
        db_free_results(rs);
      }
      
      /* count operation statistics */
      switch(query->type) {
        case SB_SQL_QUERY_POINT:
        case SB_SQL_QUERY_RANGE:
        case SB_SQL_QUERY_RANGE_SUM:
        case SB_SQL_QUERY_RANGE_ORDER:
        case SB_SQL_QUERY_RANGE_DISTINCT:
          local_read_ops += query->num_times;
          break;
        case SB_SQL_QUERY_UPDATE_INDEX:
        case SB_SQL_QUERY_UPDATE_NON_INDEX:
        case SB_SQL_QUERY_DELETE:
        case SB_SQL_QUERY_INSERT:
          local_write_ops += query->num_times;
          break;
        default: 
          local_other_ops += query->num_times;
      }   
      if (retry)
        break;  /* break transaction execution if deadlock */
    }
  } while(retry); /* retry transaction in case of deadlock */

  LOG_EVENT_STOP(msg, thread_id);
  
  SB_THREAD_MUTEX_LOCK();
  read_ops += local_read_ops;
  write_ops += local_write_ops;
  other_ops += local_other_ops;
  transactions++;
  deadlocks += local_deadlocks;
  SB_THREAD_MUTEX_UNLOCK();

  /* Free list of queries */
  SB_LIST_FOR_EACH_SAFE(pos, tmp, sql_req->queries)
  {
    query = SB_LIST_ENTRY(pos, sb_sql_query_t, listitem);
    free(query);
  }
  free(sql_req->queries);
  
  return 0;
}


void oltp_print_stats(void)
{
  double       total_time;
  unsigned int i;
  sb_timer_t   exec_timer;
  sb_timer_t   fetch_timer;

  total_time = NS2SEC(sb_timer_value(&sb_globals.exec_timer));
  
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
           " (%.2f per sec.)", transactions, transactions / total_time);
  log_text(LOG_NOTICE, "    deadlocks:                           %-6d"
           " (%.2f per sec.)", deadlocks, deadlocks / total_time);
  log_text(LOG_NOTICE, "    read/write requests:                 %-6d"
           " (%.2f per sec.)", read_ops + write_ops,
           (read_ops + write_ops) / total_time);  
  log_text(LOG_NOTICE, "    other operations:                    %-6d"
           " (%.2f per sec.)", other_ops, other_ops / total_time);

  if (sb_globals.debug)
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
}


db_conn_t *oltp_connect(void)
{
  db_conn_t *con;

  con = db_connect(driver);
  if (con == NULL)
  {
    log_text(LOG_FATAL, "failed to connect to database server!");
    return NULL;
  }
  
  if (args.connect_delay > 0)
    usleep(args.connect_delay);
  
  return con;
}


int oltp_disconnect(db_conn_t *con)
{
  return db_disconnect(con);
}


int oltp_reconnect(int thread_id)
{
  close_stmt_set(statements + thread_id);
  if (oltp_disconnect(connections[thread_id]))
    return 1;
  if (!(connections[thread_id] = oltp_connect()))
    return 1;
  if (prepare_stmt_set(statements + thread_id, bind_bufs + thread_id,
                       connections[thread_id]))
  {
    log_text(LOG_FATAL, "thread#%d: failed to prepare statements for test",
             thread_id);
    return 1;
  }
  
  return 0;
}


/* Parse command line arguments */


int parse_arguments(void)
{
  char           *s;
  
  s = sb_get_value_string("oltp-test-mode");
  if (!strcmp(s, "simple"))
    args.test_mode = TEST_MODE_SIMPLE;
  else if (!strcmp(s, "complex"))
    args.test_mode = TEST_MODE_COMPLEX;
  else if (!strcmp(s, "nontrx"))
    args.test_mode = TEST_MODE_NONTRX;
  else if (!strcmp(s, "sp"))
    args.test_mode = TEST_MODE_SP;
  else
  {
    log_text(LOG_FATAL, "Invalid OLTP test mode: %s.", s);
    return 1;
  }

  s = sb_get_value_string("oltp-reconnect-mode");
  if (!strcasecmp(s, "session"))
    args.reconnect_mode = RECONNECT_SESSION;
  else if (!strcasecmp(s, "query"))
    args.reconnect_mode = RECONNECT_QUERY;
  else if (!strcasecmp(s, "transaction"))
    args.reconnect_mode = RECONNECT_TRANSACTION;
  else if (!strcasecmp(s, "random"))
    args.reconnect_mode = RECONNECT_RANDOM;
  else
  {
    log_text(LOG_FATAL, "Invalid value for --oltp-reconnect-mode: '%s'", s);
    return 1;
  }
  
  args.sp_name = sb_get_value_string("oltp-sp-name");
  if (args.test_mode == TEST_MODE_SP && args.sp_name == NULL)
  {
    log_text(LOG_FATAL, "Name of stored procedure must be specified with --oltp-sp-name "
             "in SP test mode");
    return 1;
  }

  args.read_only = sb_get_value_flag("oltp-read-only");
  args.skip_trx = sb_get_value_flag("oltp-skip-trx");
  args.auto_inc = sb_get_value_flag("oltp-auto-inc");
  args.range_size = sb_get_value_int("oltp-range-size");
  args.point_selects = sb_get_value_int("oltp-point-selects");
  args.simple_ranges = sb_get_value_int("oltp-simple-ranges");
  args.sum_ranges = sb_get_value_int("oltp-sum-ranges");
  args.order_ranges = sb_get_value_int("oltp-order-ranges");
  args.distinct_ranges = sb_get_value_int("oltp-distinct-ranges");
  args.index_updates = sb_get_value_int("oltp-index-updates");
  args.non_index_updates = sb_get_value_int("oltp-non-index-updates");

  s = sb_get_value_string("oltp-nontrx-mode");
  if (!strcmp(s, "select"))
    args.nontrx_mode = NONTRX_MODE_SELECT;
  else if (!strcmp(s, "update_key"))
    args.nontrx_mode = NONTRX_MODE_UPDATE_KEY;
  else if (!strcmp(s, "update_nokey"))
    args.nontrx_mode = NONTRX_MODE_UPDATE_NOKEY;
  else if (!strcmp(s, "insert"))
    args.nontrx_mode = NONTRX_MODE_INSERT;
  else if (!strcmp(s, "delete"))
    args.nontrx_mode = NONTRX_MODE_DELETE;
  else
  {
    log_text(LOG_FATAL, "Invalid value of oltp-nontrx-mode: %s", s);
    return 1;
  }
  
  args.connect_delay = sb_get_value_int("oltp-connect-delay");
  args.user_delay_min = sb_get_value_int("oltp-user-delay-min");
  args.user_delay_max = sb_get_value_int("oltp-user-delay-max");
  args.table_name = sb_get_value_string("oltp-table-name");
  args.table_size = sb_get_value_int("oltp-table-size");

  s = sb_get_value_string("oltp-dist-type");
  if (!strcmp(s, "uniform"))
  {
    args.dist_type = DIST_TYPE_UNIFORM;
    rnd_func = &rnd_func_uniform;
  }
  else if (!strcmp(s, "gaussian"))
  {
    args.dist_type = DIST_TYPE_GAUSSIAN;
    rnd_func = &rnd_func_gaussian;
  }
  else if (!strcmp(s, "special"))
  {
    args.dist_type = DIST_TYPE_SPECIAL;
    rnd_func = &rnd_func_special;
  }
  else
  {
    log_text(LOG_FATAL, "Invalid random numbers distribution: %s.", s);
    return 1;
  }
  
  args.dist_iter = sb_get_value_int("oltp-dist-iter");
  args.dist_pct = sb_get_value_int("oltp-dist-pct");
  args.dist_res = sb_get_value_int("oltp-dist-res");

  /* Select driver according to command line arguments */
  driver = db_init(NULL);
  if (driver == NULL)
  {
    log_text(LOG_FATAL, "failed to initialize database driver!");
    return 1;
  }
  
  return 0;
}


/* Prepare a set of statements for the test */


int prepare_stmt_set(oltp_stmt_set_t *set, oltp_bind_set_t *bufs, db_conn_t *conn)
{
  if (args.test_mode == TEST_MODE_NONTRX)
    return prepare_stmt_set_nontrx(set, bufs, conn);
  else if (args.test_mode == TEST_MODE_COMPLEX ||
           args.test_mode == TEST_MODE_SIMPLE)
    return prepare_stmt_set_trx(set, bufs, conn);

  return prepare_stmt_set_sp(set, bufs, conn);
}


/* Close a set of statements for the test */

void close_stmt_set(oltp_stmt_set_t *set)
{
  db_close(set->lock);
  db_close(set->unlock);
  db_close(set->point);
  db_close(set->call);
  db_close(set->range);
  db_close(set->range_sum);
  db_close(set->range_order);
  db_close(set->range_distinct);
  db_close(set->update_index);
  db_close(set->update_non_index);
  db_close(set->delete);
  db_close(set->insert);
  memset(set, 0, sizeof(oltp_stmt_set_t));
}


/* Generate SQL statement from query */


db_stmt_t *get_sql_statement(sb_sql_query_t *query, int thread_id)
{
  if (args.test_mode == TEST_MODE_NONTRX)
    return get_sql_statement_nontrx(query, thread_id);
  else if (args.test_mode == TEST_MODE_COMPLEX ||
           args.test_mode == TEST_MODE_SIMPLE)
    return get_sql_statement_trx(query, thread_id);

  return get_sql_statement_sp(query, thread_id);
}


/* Prepare a set of statements for SP test */


int prepare_stmt_set_sp(oltp_stmt_set_t *set, oltp_bind_set_t *bufs, db_conn_t *conn)
{
  db_bind_t params[2];
  char      query[MAX_QUERY_LEN];
  
  /* Prepare CALL statement */
  snprintf(query, MAX_QUERY_LEN, "CALL %s(?,?)", args.sp_name);
  set->call = db_prepare(conn, query);
  if (set->call == NULL)
    return 1;
  params[0].type = DB_TYPE_INT;
  params[0].buffer = &bufs->call.thread_id;
  params[0].is_null = 0;
  params[0].data_len = 0;
  params[1].type = DB_TYPE_INT;
  params[1].buffer = &bufs->call.nthreads;
  params[1].is_null = 0;
  params[1].data_len = 0;
  if (db_bind_param(set->call, params, 2))
    return 1;
  return 0;
}

/* Prepare a set of statements for transactional test */


int prepare_stmt_set_trx(oltp_stmt_set_t *set, oltp_bind_set_t *bufs, db_conn_t *conn)
{
  db_bind_t binds[11];
  char      query[MAX_QUERY_LEN];

  /* Prepare the point statement */
  snprintf(query, MAX_QUERY_LEN, "SELECT c from %s where id=?",
           args.table_name);
  set->point = db_prepare(conn, query);
  if (set->point == NULL)
    return 1;
  binds[0].type = DB_TYPE_INT;
  binds[0].buffer = &bufs->point.id;
  binds[0].is_null = 0;
  binds[0].data_len = 0;
  if (db_bind_param(set->point, binds, 1))
    return 1;

  /* Prepare the range statement */
  snprintf(query, MAX_QUERY_LEN, "SELECT c from %s where id between ? and ?",
           args.table_name);
  set->range = db_prepare(conn, query);
  if (set->range == NULL)
    return 1;
  binds[0].type = DB_TYPE_INT;
  binds[0].buffer = &bufs->range.from;
  binds[0].is_null = 0;
  binds[0].data_len = 0;
  binds[1].type = DB_TYPE_INT;
  binds[1].buffer = &bufs->range.to;
  binds[1].is_null = 0;
  binds[1].data_len = 0;
  if (db_bind_param(set->range, binds, 2))
    return 1;

  /* Prepare the range_sum statement */
  snprintf(query, MAX_QUERY_LEN,
           "SELECT SUM(K) from %s where id between ? and ?", args.table_name);
  set->range_sum = db_prepare(conn, query);
  if (set->range_sum == NULL)
    return 1;
  binds[0].type = DB_TYPE_INT;
  binds[0].buffer = &bufs->range_sum.from;
  binds[0].is_null = 0;
  binds[0].data_len = 0;
  binds[1].type = DB_TYPE_INT;
  binds[1].buffer = &bufs->range_sum.to;
  binds[1].is_null = 0;
  binds[1].data_len = 0;
  if (db_bind_param(set->range_sum, binds, 2))
    return 1;

  /* Prepare the range_order statement */
  snprintf(query, MAX_QUERY_LEN,
           "SELECT c from %s where id between ? and ? order by c",
           args.table_name);
  set->range_order = db_prepare(conn, query);
  if (set->range_order == NULL)
    return 1;
  binds[0].type = DB_TYPE_INT;
  binds[0].buffer = &bufs->range_order.from;
  binds[0].is_null = 0;
  binds[0].data_len = 0;
  binds[1].type = DB_TYPE_INT;
  binds[1].buffer = &bufs->range_order.to;
  binds[1].is_null = 0;
  binds[1].data_len = 0;
  if (db_bind_param(set->range_order, binds, 2))
    return 1;

  /* Prepare the range_distinct statement */
  snprintf(query, MAX_QUERY_LEN,
           "SELECT DISTINCT c from %s where id between ? and ? order by c",
           args.table_name);
  set->range_distinct = db_prepare(conn, query);
  if (set->range_distinct == NULL)
    return 1;
  binds[0].type = DB_TYPE_INT;
  binds[0].buffer = &bufs->range_distinct.from;
  binds[0].is_null = 0;
  binds[0].data_len = 0;
  binds[1].type = DB_TYPE_INT;
  binds[1].buffer = &bufs->range_distinct.to;
  binds[1].is_null = 0;
  binds[1].data_len = 0;
  if (db_bind_param(set->range_distinct, binds, 2))
    return 1;

  /* Prepare the update_index statement */
  snprintf(query, MAX_QUERY_LEN, "UPDATE %s set k=k+1 where id=?",
           args.table_name);
  set->update_index = db_prepare(conn, query);
  if (set->update_index == NULL)
    return 1;
  binds[0].type = DB_TYPE_INT;
  binds[0].buffer = &bufs->update_index.id;
  binds[0].is_null = 0;
  binds[0].data_len = 0;
  if (db_bind_param(set->update_index, binds, 1))
    return 1;

  /* Prepare the update_non_index statement */
  snprintf(query, MAX_QUERY_LEN,
           "UPDATE %s set c=? where id=?",
           args.table_name);
  set->update_non_index = db_prepare(conn, query);
  if (set->update_non_index == NULL)
    return 1;
  /*
    Non-index update statement is re-bound each time because of the string
    parameter
  */
  
  /* Prepare the delete statement */
  snprintf(query, MAX_QUERY_LEN, "DELETE from %s where id=?",
           args.table_name);
  set->delete = db_prepare(conn, query);
  if (set->delete == NULL)
    return 1;
  binds[0].type = DB_TYPE_INT;
  binds[0].buffer = &bufs->delete.id;
  binds[0].is_null = 0;
  binds[0].data_len = 0;
  if (db_bind_param(set->delete, binds, 1))
    return 1;

  /* Prepare the insert statement */
  snprintf(query, MAX_QUERY_LEN, "INSERT INTO %s values(?,0,' ',"
           "'aaaaaaaaaaffffffffffrrrrrrrrrreeeeeeeeeeyyyyyyyyyy')",
           args.table_name);
  set->insert = db_prepare(conn, query);
  if (set->insert == NULL)
    return 1;
  binds[0].type = DB_TYPE_INT;
  binds[0].buffer = &bufs->insert.id;
  binds[0].is_null = 0;
  binds[0].data_len = 0;
  if (db_bind_param(set->insert, binds, 1))
    return 1;

  if (args.skip_trx)
    return 0;
  
  /* Prepare the lock statement */
  if (driver_caps.transactions)
    strncpy(query, "BEGIN", MAX_QUERY_LEN);
  else
  {
    if (args.read_only)
      snprintf(query, MAX_QUERY_LEN, "LOCK TABLES %s READ", args.table_name);
    else
      snprintf(query, MAX_QUERY_LEN, "LOCK TABLES %s WRITE", args.table_name);
  }
  set->lock = db_prepare(conn, query);
  if (set->lock == NULL)
    return 1;

  /* Prepare the unlock statement */
  if (driver_caps.transactions)
    strncpy(query, "COMMIT", MAX_QUERY_LEN);
  else
    strncpy(query, "UNLOCK TABLES", MAX_QUERY_LEN);
  set->unlock = db_prepare(conn, query);
  if (set->unlock == NULL)
    return 1;

  return 0;
}


/* Prepare a set of statements for non-transactional test */


int prepare_stmt_set_nontrx(oltp_stmt_set_t *set, oltp_bind_set_t *bufs, db_conn_t *conn)
{
  db_bind_t binds[11];
  char      query[MAX_QUERY_LEN];

  /* Prepare the point statement */
  snprintf(query, MAX_QUERY_LEN, "SELECT pad from %s where id=?",
           args.table_name);
  set->point = db_prepare(conn, query);
  if (set->point == NULL)
    return 1;
  binds[0].type = DB_TYPE_INT;
  binds[0].buffer = &bufs->point.id;
  binds[0].is_null = 0;
  binds[0].data_len = 0;
  if (db_bind_param(set->point, binds, 1))
    return 1;

  /* Prepare the update_index statement */
  snprintf(query, MAX_QUERY_LEN, "UPDATE %s set k=k+1 where id=?",
           args.table_name);
  set->update_index = db_prepare(conn, query);
  if (set->update_index == NULL)
    return 1;
  binds[0].type = DB_TYPE_INT;
  binds[0].buffer = &bufs->update_index.id;
  binds[0].is_null = 0;
  binds[0].data_len = 0;
  if (db_bind_param(set->update_index, binds, 1))
    return 1;

  /* Prepare the update_non_index statement */
  snprintf(query, MAX_QUERY_LEN,
           "UPDATE %s set c=? where id=?",
           args.table_name);
  set->update_non_index = db_prepare(conn, query);
  if (set->update_non_index == NULL)
    return 1;
  /*
    Non-index update statement is re-bound each time because of the string
    parameter
  */
  
  /* Prepare the delete statement */
  snprintf(query, MAX_QUERY_LEN, "DELETE from %s where id=?",
           args.table_name);
  set->delete = db_prepare(conn, query);
  if (set->delete == NULL)
    return 1;
  binds[0].type = DB_TYPE_INT;
  binds[0].buffer = &bufs->delete.id;
  binds[0].is_null = 0;
  binds[0].data_len = 0;
  if (db_bind_param(set->delete, binds, 1))
    return 1;

  /* Prepare the insert statement */
  snprintf(query, MAX_QUERY_LEN, "INSERT INTO %s values(?,?,?,?)",
           args.table_name);
  set->insert = db_prepare(conn, query);
  /*
    Insert statement is re-bound each time because of the string
    parameters
  */
  
  return 0;
}


/* Generate SQL statement from query for SP test */


db_stmt_t *get_sql_statement_sp(sb_sql_query_t *query, int thread_id)
{
  db_stmt_t       *stmt;
  oltp_bind_set_t  *buf = bind_bufs + thread_id;

  (void) query; /* unused */
  
  stmt = statements[thread_id].call;
  buf->call.thread_id = thread_id;
  buf->call.nthreads = sb_globals.num_threads;
  
  return stmt;
}


/* Generate SQL statement from query for transactional test */


db_stmt_t *get_sql_statement_trx(sb_sql_query_t *query, int thread_id)
{
  db_stmt_t       *stmt = NULL;
  db_bind_t       binds[2];
  oltp_bind_set_t *buf = bind_bufs + thread_id;
  
  switch (query->type) {
    case SB_SQL_QUERY_LOCK:
      stmt = statements[thread_id].lock;
      break;

    case SB_SQL_QUERY_UNLOCK:
      stmt = statements[thread_id].unlock;
      break;

    case SB_SQL_QUERY_POINT:
      stmt = statements[thread_id].point;
      buf->point.id = query->u.point_query.id;
      break;

    case SB_SQL_QUERY_RANGE:
      stmt = statements[thread_id].range;
      buf->range.from = query->u.range_query.from;
      buf->range.to = query->u.range_query.to;
      break;

    case SB_SQL_QUERY_RANGE_SUM:
      stmt = statements[thread_id].range_sum;
      buf->range_sum.from = query->u.range_query.from;
      buf->range_sum.to = query->u.range_query.to;
      break;

    case SB_SQL_QUERY_RANGE_ORDER:
      stmt = statements[thread_id].range_order;
      buf->range_order.from = query->u.range_query.from;
      buf->range_order.to = query->u.range_query.to;
      break;

    case SB_SQL_QUERY_RANGE_DISTINCT:
      stmt = statements[thread_id].range_distinct;
      buf->range_distinct.from = query->u.range_query.from;
      buf->range_distinct.to = query->u.range_query.to;
      break;

    case SB_SQL_QUERY_UPDATE_INDEX:
      stmt = statements[thread_id].update_index;
      buf->update_index.id = query->u.update_query.id;
      break;

    case SB_SQL_QUERY_UPDATE_NON_INDEX:
      stmt = statements[thread_id].update_non_index;
      /*
        We have to bind non-index update data each time
        because of string parameter
      */
      snprintf(buf->c, 120, "%lu-%lu-%lu-%lu-%lu-%lu-%lu-%lu-%lu-%lu",
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd());
      buf->update_non_index.id = query->u.update_query.id;
      buf->c_len = strlen(buf->c);
      binds[0].type = DB_TYPE_CHAR;
      binds[0].buffer = buf->c;
      binds[0].data_len = &buf->c_len;
      binds[0].is_null = 0;
      binds[0].max_len = 120;
      binds[1].type = DB_TYPE_INT;
      binds[1].buffer = &buf->update_non_index.id;
      binds[1].data_len = 0;
      binds[1].is_null = 0;
      if (db_bind_param(statements[thread_id].update_non_index, binds, 2))
        return NULL;
      break;

    case SB_SQL_QUERY_DELETE:
      stmt = statements[thread_id].delete;
      buf->delete.id = query->u.delete_query.id;
      break;

    case SB_SQL_QUERY_INSERT:
      stmt = statements[thread_id].insert;
      buf->insert.id = query->u.insert_query.id;
      break;

    default:
      return NULL;
  }

  return stmt;
}


/* Generate SQL statement from query for non-transactional test */


db_stmt_t *get_sql_statement_nontrx(sb_sql_query_t *query, int thread_id)
{
  db_stmt_t       *stmt = NULL;
  db_bind_t       binds[4];
  oltp_bind_set_t *buf = bind_bufs + thread_id;
  
  switch (query->type) {
    case SB_SQL_QUERY_POINT:
      stmt = statements[thread_id].point;
      buf->point.id = query->u.point_query.id;
      break;

    case SB_SQL_QUERY_UPDATE_INDEX:
      stmt = statements[thread_id].update_index;
      buf->update_index.id = query->u.update_query.id;
      break;

    case SB_SQL_QUERY_UPDATE_NON_INDEX:
      stmt = statements[thread_id].update_non_index;
      /*
        We have to bind non-index update data each time
        because of string parameter
      */
      snprintf(buf->c, 120, "%lu-%lu-%lu-%lu-%lu-%lu-%lu-%lu-%lu-%lu",
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd());
      buf->update_non_index.id = query->u.update_query.id;
      buf->c_len = strlen(buf->c);

      binds[0].type = DB_TYPE_CHAR;
      binds[0].buffer = buf->c;
      binds[0].data_len = &buf->c_len;
      binds[0].is_null = 0;
      binds[0].max_len = 120;

      binds[1].type = DB_TYPE_INT;
      binds[1].buffer = &buf->update_non_index.id;
      binds[1].data_len = 0;
      binds[1].is_null = 0;

      if (db_bind_param(statements[thread_id].update_non_index, binds, 2))
        return NULL;
      break;

    case SB_SQL_QUERY_DELETE:
      stmt = statements[thread_id].delete;
      buf->delete.id = query->u.delete_query.id;
      break;

    case SB_SQL_QUERY_INSERT:
      stmt = statements[thread_id].insert;
      /*
        We have to bind insert data each time
        because of string parameters
      */
      buf->range.to = query->u.insert_query.id;
      snprintf(buf->c, 120, "%lu-%lu-%lu-%lu-%lu-%lu-%lu-%lu-%lu-%lu",
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd());
      buf->c_len = strlen(buf->c);
      snprintf(buf->pad, 60, "%lu-%lu-%lu-%lu-%lu",
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd(),
               (unsigned long)sb_rnd());
      buf->pad_len = strlen(buf->pad);

      /* Use NULL is AUTO_INCREMENT is used, unique id otherwise */
      if (args.auto_inc)
      {
        binds[0].is_null = &oltp_is_null;
      }
      else
      {
        buf->range.from = get_unique_random_id();
        binds[0].buffer = &buf->range.from;
        binds[0].is_null = 0;
      }
      
      binds[0].type = DB_TYPE_INT;
      binds[0].data_len = 0;

      binds[1].type = DB_TYPE_INT;
      binds[1].buffer = &buf->range.to;
      binds[1].data_len = 0;
      binds[1].is_null = 0;

      binds[2].type = DB_TYPE_CHAR;
      binds[2].buffer = buf->c;
      binds[2].data_len = &buf->c_len;
      binds[2].is_null = 0;
      binds[2].max_len = 120;

      binds[3].type = DB_TYPE_CHAR;
      binds[3].buffer = buf->pad;
      binds[3].data_len = &buf->pad_len;
      binds[3].is_null = 0;
      binds[3].max_len = 60;

      if (db_bind_param(statements[thread_id].insert, binds, 4))
        return NULL;
      
      break;

    default:
      return NULL;
  }

  return stmt;
}


/* uniform distribution */


unsigned int rnd_func_uniform(void)
{
  return 1 + sb_rnd() % args.table_size;
}


/* gaussian distribution */


unsigned int rnd_func_gaussian(void)
{
  int          sum;
  unsigned int i;

  for(i=0, sum=0; i < args.dist_iter; i++)
    sum += (1 + sb_rnd() % args.table_size);
  
  return sum / args.dist_iter;
}


/* 'special' distribution */


unsigned int rnd_func_special(void)
{
  int          sum = 0;
  unsigned int i;
  unsigned int d;
  unsigned int res;
  unsigned int range_size;
  
  if (args.table_size == 0)
    return 0;
  
  /* Increase range size for special values. */
  range_size = args.table_size * (100 / (100 - args.dist_res));
  
  /* Generate evenly distributed one at this stage  */
  res = (1 + sb_rnd() % range_size);
  
  /* For first part use gaussian distribution */
  if (res <= args.table_size)
  {
    for(i = 0; i < args.dist_iter; i++)
    {
      sum += (1 + sb_rnd() % args.table_size);
    }
    return sum / args.dist_iter;  
  }

  /*
   * For second part use even distribution mapped to few items 
   * We shall distribute other values near by the center
   */
  d = args.table_size * args.dist_pct / 100;
  if (d < 1)
    d = 1;
  res %= d;
   
  /* Now we have res values in SPECIAL_PCT range of the data */
  res += (args.table_size / 2 - args.table_size * args.dist_pct / (100 * 2));
   
  return res;
}


/* Generate unique random id */


unsigned int get_unique_random_id(void)
{
  unsigned int res;

  pthread_mutex_lock(&rnd_mutex);
  res = (unsigned int) (rnd_seed % args.table_size) + 1;
  rnd_seed += LARGE_PRIME;
  pthread_mutex_unlock(&rnd_mutex);

  return res;
}


int get_think_time(void)
{
  int t = args.user_delay_min;

  if (args.user_delay_min < args.user_delay_max)
    t += sb_rnd() % (args.user_delay_max - args.user_delay_min);

  return t; 
}

