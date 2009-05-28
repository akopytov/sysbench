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

#ifndef DB_DRIVER_H
#define DB_DRIVER_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "sysbench.h"
#include "sb_list.h"


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

extern db_globals_t db_globals;

/* Driver capabilities definition */

typedef struct
{
  char     multi_rows_insert;   /* 1 if database supports multi-row inserts */
  char     transactions;        /* 1 if database supports transactions */
  char     prepared_statements; /* 1 if database supports prepared statements */
  char     auto_increment;      /* 1 if database supports AUTO_INCREMENT clause */
  char     needs_commit;        /* 1 if database needs explicit commit after INSERTs */
  char     serial;              /* 1 if database supports SERIAL clause */
  char     unsigned_int;        /* 1 if database supports UNSIGNED INTEGER types */
  
  char    *table_options_str;   /* additional table options provided by database driver */
} drv_caps_t;

/* Database errors definition */

typedef enum
{
  SB_DB_ERROR_NONE,
  SB_DB_ERROR_DEADLOCK,
  SB_DB_ERROR_FAILED
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
struct db_result_set;
struct db_row;

/* Driver operations definition */

typedef int drv_op_init(void);
typedef int drv_op_describe(drv_caps_t *, const char *);
typedef int drv_op_connect(struct db_conn *);
typedef int drv_op_disconnect(struct db_conn *);
typedef int drv_op_prepare(struct db_stmt *, const char *);
typedef int drv_op_bind_param(struct db_stmt *, db_bind_t *, unsigned int);
typedef int drv_op_bind_result(struct db_stmt *, db_bind_t *, unsigned int );
typedef int drv_op_execute(struct db_stmt *, struct db_result_set *);
typedef int drv_op_fetch(struct db_result_set *);
typedef int drv_op_fetch_row(struct db_result_set *, struct db_row *);
typedef unsigned long long drv_op_num_rows(struct db_result_set *);
typedef int drv_op_query(struct db_conn *, const char *,
                         struct db_result_set *);
typedef int drv_op_free_results(struct db_result_set *);
typedef int drv_op_close(struct db_stmt *);
typedef int drv_op_store_results(struct db_result_set *);
typedef int drv_op_done(void);

typedef struct
{
  drv_op_init            *init;           /* initializate driver */
  drv_op_describe        *describe;       /* describe database capabilities */
  drv_op_connect         *connect;        /* connect to database */
  drv_op_disconnect      *disconnect;     /* disconnect from database */
  drv_op_prepare         *prepare;        /* prepare statement */
  drv_op_bind_param      *bind_param;     /* bind params for prepared statement */
  drv_op_bind_result     *bind_result;    /* bind results for prepared statement */
  drv_op_execute         *execute;        /* execute prepared statement */
  drv_op_fetch           *fetch;          /* fetch row for prepared statement */
  drv_op_fetch_row       *fetch_row;      /* fetch row for queries */
  drv_op_num_rows        *num_rows;       /* return number of rows in result set */
  drv_op_free_results    *free_results;   /* free result set */
  drv_op_close           *close;          /* close prepared statement */
  drv_op_query           *query;          /* execute non-prepared statement */
  drv_op_store_results   *store_results;  /* store results from last query */
  drv_op_done            *done;           /* uninitialize driver */
} drv_ops_t;

/* Database driver definition */

typedef struct
{
  char            *sname;   /* short name */
  char            *lname;   /* long name */
  sb_arg_t        *args;    /* driver command line arguments */
  drv_ops_t       ops;      /* driver operations */

  sb_list_item_t  listitem; /* can be linked in a list */
} db_driver_t;

/* Connection types definition */

typedef enum {
  DB_CONN_TYPE_UNUSED,
  DB_CONN_TYPE_MYSQL
} db_conn_type_t;

/* Result set definition */

typedef struct db_result_set
{
  struct db_conn *connection; /* Connection which this result set belongs to */
  struct db_stmt *statement;  /* Statement for this result set (if any) */ 
  struct db_row  *row;        /* Last row fetched by db_fetch_row */
  void           *ptr;        /* Pointer to driver-specific data */
  unsigned long long nrows;   /* Number of rows in a result set */
} db_result_set_t;

/* Database connection structure */

typedef struct db_conn
{
  db_driver_t     *driver;    /* DB driver for this connection */
  db_conn_type_t  type;
  void            *ptr;
  db_error_t      db_errno;
  db_result_set_t rs;   /* Result set */
} db_conn_t;

/* Prepared statement definition */

typedef struct db_stmt
{
  db_conn_t    *connection;     /* Connection which this statement belongs to */
  char         *query;          /* Query string for emulated PS */
  db_bind_t    *bound_param;    /* Array of bound parameters for emulated PS */
  unsigned int bound_param_len; /* Length of the bound_param array */
  db_bind_t    *bound_res;      /* Array of bound results for emulated PS */ 
  db_bind_t    *bound_res_len;  /* Length of the bound_res array */
  void         *ptr;            /* Pointer to driver-specific data structure */
  char         emulated;        /* Should this statement be emulated? */
} db_stmt_t;

/* Result set row definition */

typedef struct db_row
{
  db_result_set_t *result_set; /* Result set which this row belongs to */
} db_row_t;

/* Database abstraction layer calls */

int db_register(void);

void db_print_help(void);

db_driver_t *db_init(const char *);

int db_describe(db_driver_t *, drv_caps_t *, const char *);

db_conn_t *db_connect(db_driver_t *);

int db_disconnect(db_conn_t *);

db_stmt_t *db_prepare(db_conn_t *, const char *);

int db_bind_param(db_stmt_t *, db_bind_t *, unsigned int);

int db_bind_result(db_stmt_t *, db_bind_t *, unsigned int);

db_result_set_t *db_execute(db_stmt_t *);

db_row_t *db_fetch_row(db_result_set_t *);

unsigned long long db_num_rows(db_result_set_t *);

db_result_set_t *db_query(db_conn_t *, const char *);

int db_free_results(db_result_set_t *);

int db_store_results(db_result_set_t *);

int db_close(db_stmt_t *);

int db_done(db_driver_t *);

db_error_t db_errno(db_conn_t *);

int db_print_value(db_bind_t *, char *, int);

#endif /* DB_DRIVER_H */
