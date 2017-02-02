/* Copyright (C) 2004 MySQL AB
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

#ifndef DB_DRIVER_H
#define DB_DRIVER_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "sysbench.h"
#include "sb_list.h"
#include "sb_histogram.h"
#include "sb_counter.h"

/* Prepared statements usage modes */

typedef enum
{
  DB_PS_MODE_AUTO,
  DB_PS_MODE_DISABLE,
} db_ps_mode_t;

/* Global DB API options */

typedef struct
{
  db_ps_mode_t  ps_mode;   /* Requested prepared statements usage mode */
  char          *driver;   /* Requested database driver */
  unsigned char debug;     /* debug flag */
} db_globals_t;

/* Driver capabilities definition */

typedef struct
{
  char     multi_rows_insert;   /* 1 if database supports multi-row inserts */
  char     prepared_statements; /* 1 if database supports prepared statements */
  char     auto_increment;      /* 1 if database supports AUTO_INCREMENT clause */
  char     needs_commit;        /* 1 if database needs explicit commit after INSERTs */
  char     serial;              /* 1 if database supports SERIAL clause */
  char     unsigned_int;        /* 1 if database supports UNSIGNED INTEGER types */
} drv_caps_t;

/* Database errors definition */

typedef enum
{
  DB_ERROR_NONE,                /* no error(s) */
  DB_ERROR_IGNORABLE,           /* error should be ignored as defined by command
                                line arguments or a custom error handler */
  DB_ERROR_FATAL                /* non-ignorable error */
} db_error_t;


/* Available buffer types (for parameters binding) */

typedef enum
{
  DB_TYPE_NONE,
  DB_TYPE_TINYINT,
  DB_TYPE_SMALLINT,
  DB_TYPE_INT,
  DB_TYPE_BIGINT,
  DB_TYPE_FLOAT,
  DB_TYPE_DOUBLE,
  DB_TYPE_TIME,
  DB_TYPE_DATE,
  DB_TYPE_DATETIME,
  DB_TYPE_TIMESTAMP,
  DB_TYPE_CHAR,
  DB_TYPE_VARCHAR
} db_bind_type_t;


/* Structure used to represent DATE, TIME, DATETIME and TIMESTAMP values */

typedef struct
{
  unsigned int year;
  unsigned int month;
  unsigned int day;
  unsigned int hour;
  unsigned int minute;
  unsigned int second;
} db_time_t;


/* Structure used to bind data for prepared statements */

typedef struct
{
  db_bind_type_t   type;
  void             *buffer;
  unsigned long    *data_len;
  unsigned long    max_len;
  char             *is_null;
} db_bind_t;

/* Forward declarations */

struct db_conn;
struct db_stmt;
struct db_result;
struct db_row;

/* Driver operations definition */

typedef int drv_op_init(void);
typedef int drv_op_thread_init(int);
typedef int drv_op_describe(drv_caps_t *);
typedef int drv_op_connect(struct db_conn *);
typedef int drv_op_reconnect(struct db_conn *);
typedef int drv_op_disconnect(struct db_conn *);
typedef int drv_op_prepare(struct db_stmt *, const char *, size_t);
typedef int drv_op_bind_param(struct db_stmt *, db_bind_t *, size_t);
typedef int drv_op_bind_result(struct db_stmt *, db_bind_t *, size_t);
typedef db_error_t drv_op_execute(struct db_stmt *, struct db_result *);
typedef int drv_op_fetch(struct db_result *);
typedef int drv_op_fetch_row(struct db_result *, struct db_row *);
typedef db_error_t drv_op_query(struct db_conn *, const char *, size_t,
                                struct db_result *);
typedef int drv_op_free_results(struct db_result *);
typedef int drv_op_close(struct db_stmt *);
typedef int drv_op_thread_done(int);
typedef int drv_op_done(void);

typedef struct
{
  drv_op_init            *init;           /* initializate driver */
  drv_op_thread_init     *thread_init;    /* thread-local driver initialization */
  drv_op_describe        *describe;       /* describe database capabilities */
  drv_op_connect         *connect;        /* connect to database */
  drv_op_disconnect      *disconnect;     /* disconnect from database */
  drv_op_reconnect       *reconnect;      /* reconnect with the same parameters */
  drv_op_prepare         *prepare;        /* prepare statement */
  drv_op_bind_param      *bind_param;     /* bind params for prepared statement */
  drv_op_bind_result     *bind_result;    /* bind results for prepared statement */
  drv_op_execute         *execute;        /* execute prepared statement */
  drv_op_fetch           *fetch;          /* fetch row for prepared statement */
  drv_op_fetch_row       *fetch_row;      /* fetch row for queries */
  drv_op_free_results    *free_results;   /* free result set */
  drv_op_close           *close;          /* close prepared statement */
  drv_op_query           *query;          /* execute non-prepared statement */
  drv_op_thread_done     *thread_done;    /* thread-local driver deinitialization */
  drv_op_done            *done;           /* uninitialize driver */
} drv_ops_t;

/* Database driver definition */

typedef struct
{
  const char      *sname;   /* short name */
  const char      *lname;   /* long name */
  sb_arg_t        *args;    /* driver command line arguments */
  drv_ops_t       ops;      /* driver operations */

  sb_list_item_t  listitem; /* can be linked in a list */
  bool            initialized;
  pthread_mutex_t mutex;
} db_driver_t;

/* Row value definition */

typedef struct {
  uint32_t        len;         /* Value length */
  const char      *ptr;        /* Value string */
} db_value_t;

/* Result set row definition */

typedef struct db_row
{
  void            *ptr;        /* Driver-specific row data */
  db_value_t      *values;     /* Array of column values */
} db_row_t;

/* Result set definition */

typedef struct db_result
{
  sb_counter_type_t counter;     /* Statistical counter type */
  uint32_t       nrows;         /* Number of affected rows */
  uint32_t       nfields;       /* Number of fields */
  struct db_stmt *statement;    /* Pointer to prepared statement (if used) */
  void           *ptr;          /* Pointer to driver-specific data */
  db_row_t       row;           /* Last fetched row */
} db_result_t;

typedef enum {
  DB_CONN_READY,
  DB_CONN_RESULT_SET,
  DB_CONN_INVALID
} db_conn_state_t;

/* Database connection structure */

typedef struct db_conn
{
  db_error_t      error;             /* Database-independent error code */
  int             sql_errno;         /* Database-specific error code */
  const char      *sql_state;        /* Database-specific SQL state */
  const char      *sql_errmsg;       /* Database-specific error message */
  db_driver_t     *driver;           /* DB driver for this connection */
  void            *ptr;              /* Driver-specific data */
  db_result_t     rs;                /* Result set */
  db_conn_state_t state;             /* Connection state */
  int             thread_id;         /* Thread this connection belongs to */

  unsigned int    bulk_cnt;          /* Current number of rows in bulk insert buffer */
  unsigned int    bulk_buflen;       /* Current length of bulk_buffer */
  char            *bulk_buffer;      /* Bulk insert query buffer */
  unsigned int    bulk_ptr;          /* Current position in bulk_buffer */
  unsigned int    bulk_values;       /* Save value of bulk_ptr */
  unsigned int    bulk_commit_cnt;   /* Current value of uncommitted rows */
  unsigned int    bulk_commit_max;   /* Maximum value of uncommitted rows */

  char            pad[SB_CACHELINE_PAD(sizeof(db_error_t) +
                                       sizeof(int) +
                                       sizeof(void *) +
                                       sizeof(void *) +
                                       sizeof(void *) +
                                       sizeof(void *) +
                                       sizeof(db_result_t) +
                                       sizeof(db_conn_state_t) +
                                       sizeof(int) +
                                       sizeof(int) * 2 +
                                       sizeof(void *) +
                                       sizeof(int) * 4
                                       )];
} db_conn_t;

/* Prepared statement definition */

typedef struct db_stmt
{
  db_conn_t       *connection;     /* Connection which this statement belongs to */
  char            *query;          /* Query string for emulated PS */
  db_bind_t       *bound_param;    /* Array of bound parameters for emulated PS */
  unsigned int    bound_param_len; /* Length of the bound_param array */
  db_bind_t       *bound_res;      /* Array of bound results for emulated PS */ 
  db_bind_t       *bound_res_len;  /* Length of the bound_res array */
  char            emulated;        /* Should this statement be emulated? */
  sb_counter_type_t  counter;       /* Query type */
  void            *ptr;            /* Pointer to driver-specific data structure */
} db_stmt_t;

extern db_globals_t db_globals;

/* Database abstraction layer calls */

int db_register(void);

void db_print_help(void);

db_driver_t *db_create(const char *);

int db_destroy(db_driver_t *);

int db_describe(db_driver_t *, drv_caps_t *);

db_conn_t *db_connection_create(db_driver_t *);

int db_connection_close(db_conn_t *);

int db_connection_reconnect(db_conn_t *con);

void db_connection_free(db_conn_t *con);

db_stmt_t *db_prepare(db_conn_t *, const char *, size_t);

int db_bind_param(db_stmt_t *, db_bind_t *, size_t);

int db_bind_result(db_stmt_t *, db_bind_t *, size_t);

db_result_t *db_execute(db_stmt_t *);

db_row_t *db_fetch_row(db_result_t *);

db_result_t *db_query(db_conn_t *, const char *, size_t len);

int db_free_results(db_result_t *);

int db_store_results(db_result_t *);

int db_close(db_stmt_t *);

void db_done(void);

int db_print_value(db_bind_t *, char *, int);

/* Initialize multi-row insert operation */
int db_bulk_insert_init(db_conn_t *, const char *, size_t);

/* Add row to multi-row insert operation */
int db_bulk_insert_next(db_conn_t *, const char *, size_t);

/* Finish multi-row insert operation */
int db_bulk_insert_done(db_conn_t *);

/* Print database-specific test stats */
void db_report_intermediate(sb_stat_t *);
void db_report_cumulative(sb_stat_t *);

/* DB drivers registrars */

#ifdef USE_MYSQL
int register_driver_mysql(sb_list_t *);
#endif

#ifdef USE_DRIZZLE
int register_driver_drizzle(sb_list_t *);
#endif

#ifdef USE_ATTACHSQL
int register_driver_attachsql(sb_list_t *);
#endif

#ifdef USE_DRIZZLECLIENT
int register_driver_drizzleclient(sb_list_t *);
#endif

#ifdef USE_ORACLE
int register_driver_oracle(sb_list_t *);
#endif

#ifdef USE_PGSQL
int register_driver_pgsql(sb_list_t *);
#endif

#endif /* DB_DRIVER_H */
