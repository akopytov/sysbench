/* Copyright (C) 2004 MySQL AB
   Copyright (C) 2004-2018 Alexey Kopytov <akopytov@gmail.com>

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

#ifdef STDC_HEADERS
# include <stdio.h>
#endif
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

#include <mysql.h>
#include <mysqld_error.h>
#include <errmsg.h>

#include "sb_options.h"
#include "db_driver.h"

#define DEBUG(format, ...)                      \
  do {                                          \
    if (SB_UNLIKELY(args.debug != 0))           \
      log_text(LOG_DEBUG, format, __VA_ARGS__); \
  } while (0)

static inline const char *SAFESTR(const char *s)
{
  return s ? s : "(null)";
}

#if !defined(MARIADB_BASE_VERSION) && !defined(MARIADB_VERSION_ID) && \
  MYSQL_VERSION_ID >= 80001 && MYSQL_VERSION_ID != 80002 /* see https://bugs.mysql.com/?id=87337 */
typedef bool my_bool;
#endif

/* MySQL driver arguments */

static sb_arg_t mysql_drv_args[] =
{
  SB_OPT("mysql-host", "MySQL server host", "localhost", LIST),
  SB_OPT("mysql-port", "MySQL server port", "3306", LIST),
  SB_OPT("mysql-socket", "MySQL socket", NULL, LIST),
  SB_OPT("mysql-user", "MySQL user", "sbtest", STRING),
  SB_OPT("mysql-password", "MySQL password", "", STRING),
  SB_OPT("mysql-db", "MySQL database name", "sbtest", STRING),
#ifdef HAVE_MYSQL_OPT_SSL_MODE
  SB_OPT("mysql-ssl", "SSL mode. This accepts the same values as the "
         "--ssl-mode option in the MySQL client utilities. Disabled by default",
         "disabled", STRING),
#else
  SB_OPT("mysql-ssl", "use SSL connections, if available in the client "
         "library", "off", BOOL),
#endif
  SB_OPT("mysql-ssl-key", "path name of the client private key file", NULL,
         STRING),
  SB_OPT("mysql-ssl-ca", "path name of the CA file", NULL, STRING),
  SB_OPT("mysql-ssl-cert",
         "path name of the client public key certificate file", NULL, STRING),
  SB_OPT("mysql-ssl-cipher", "use specific cipher for SSL connections", "",
         STRING),
  SB_OPT("mysql-compression", "use compression, if available in the "
         "client library", "off", BOOL),
  SB_OPT("mysql-compression-algorithms", "compression algorithms to use",
	 "zlib", STRING),
  SB_OPT("mysql-debug", "trace all client library calls", "off", BOOL),
  SB_OPT("mysql-ignore-errors", "list of errors to ignore, or \"all\"",
         "1213,1020,1205", LIST),
  SB_OPT("mysql-dry-run", "Dry run, pretend that all MySQL client API "
         "calls are successful without executing them", "off", BOOL),

  SB_OPT_END
};

typedef struct
{
  sb_list_t          *hosts;
  sb_list_t          *ports;
  sb_list_t          *sockets;
  const char         *user;
  const char         *password;
  const char         *db;
#ifdef HAVE_MYSQL_OPT_SSL_MODE
  unsigned int       ssl_mode;
#endif
  bool               use_ssl;
  const char         *ssl_key;
  const char         *ssl_cert;
  const char         *ssl_ca;
  const char         *ssl_cipher;
  unsigned char      use_compression;
#ifdef MYSQL_OPT_COMPRESSION_ALGORITHMS
  const char         *compression_alg;
#endif
  unsigned char      debug;
  sb_list_t          *ignored_errors;
  unsigned int       dry_run;
} mysql_drv_args_t;

typedef struct
{
  MYSQL        *mysql;
  const char   *host;
  const char   *user;
  const char   *password;
  const char   *db;
  unsigned int port;
  char         *socket;
} db_mysql_conn_t;

#ifdef HAVE_MYSQL_OPT_SSL_MODE
typedef struct {
  const char *name;
  enum mysql_ssl_mode mode;
} ssl_mode_map_t;
#endif


/* Structure used for DB-to-MySQL bind types map */

typedef struct
{
  db_bind_type_t   db_type;
  int              my_type;
} db_mysql_bind_map_t;

/* DB-to-MySQL bind types map */
db_mysql_bind_map_t db_mysql_bind_map[] =
{
  {DB_TYPE_TINYINT,   MYSQL_TYPE_TINY},
  {DB_TYPE_SMALLINT,  MYSQL_TYPE_SHORT},
  {DB_TYPE_INT,       MYSQL_TYPE_LONG},
  {DB_TYPE_BIGINT,    MYSQL_TYPE_LONGLONG},
  {DB_TYPE_FLOAT,     MYSQL_TYPE_FLOAT},
  {DB_TYPE_DOUBLE,    MYSQL_TYPE_DOUBLE},
  {DB_TYPE_DATETIME,  MYSQL_TYPE_DATETIME},
  {DB_TYPE_TIMESTAMP, MYSQL_TYPE_TIMESTAMP},
  {DB_TYPE_CHAR,      MYSQL_TYPE_STRING},
  {DB_TYPE_VARCHAR,   MYSQL_TYPE_VAR_STRING},
  {DB_TYPE_NONE,      0}
};

/* MySQL driver capabilities */

static drv_caps_t mysql_drv_caps =
{
  1,
  0,
  1,
  0,
  0,
  1
};



static mysql_drv_args_t args;          /* driver args */

static char use_ps; /* whether server-side prepared statemens should be used */

/* Positions in the list of hosts/ports/sockets. Protected by pos_mutex */
static sb_list_item_t *hosts_pos;
static sb_list_item_t *ports_pos;
static sb_list_item_t *sockets_pos;

static pthread_mutex_t pos_mutex;

#ifdef HAVE_MYSQL_OPT_SSL_MODE

#if MYSQL_VERSION_ID < 50711
/*
  In MySQL 5.6 the only valid SSL mode is SSL_MODE_REQUIRED. Define
  SSL_MODE_DISABLED to enable the 'disabled' default value for --mysql-ssl
*/
#define SSL_MODE_DISABLED 1
#endif

static ssl_mode_map_t ssl_mode_names[] = {
  {"DISABLED", SSL_MODE_DISABLED},
#if MYSQL_VERSION_ID >= 50711
  {"PREFERRED", SSL_MODE_PREFERRED},
#endif
  {"REQUIRED", SSL_MODE_REQUIRED},
#if MYSQL_VERSION_ID >= 50711
  {"VERIFY_CA", SSL_MODE_VERIFY_CA},
  {"VERIFY_IDENTITY", SSL_MODE_VERIFY_IDENTITY},
#endif
  {NULL, 0}
};
#endif

/* MySQL driver operations */

static int mysql_drv_init(void);
static int mysql_drv_thread_init(int);
static int mysql_drv_describe(drv_caps_t *);
static int mysql_drv_connect(db_conn_t *);
static int mysql_drv_reconnect(db_conn_t *);
static int mysql_drv_disconnect(db_conn_t *);
static int mysql_drv_prepare(db_stmt_t *, const char *, size_t);
static int mysql_drv_bind_param(db_stmt_t *, db_bind_t *, size_t);
static int mysql_drv_bind_result(db_stmt_t *, db_bind_t *, size_t);
static db_error_t mysql_drv_execute(db_stmt_t *, db_result_t *);
static db_error_t mysql_drv_stmt_next_result(db_stmt_t *, db_result_t *);
static int mysql_drv_fetch(db_result_t *);
static int mysql_drv_fetch_row(db_result_t *, db_row_t *);
static db_error_t mysql_drv_query(db_conn_t *, const char *, size_t,
                           db_result_t *);
static bool mysql_drv_more_results(db_conn_t *);
static db_error_t mysql_drv_next_result(db_conn_t *, db_result_t *);
static int mysql_drv_free_results(db_result_t *);
static int mysql_drv_close(db_stmt_t *);
static int mysql_drv_thread_done(int);
static int mysql_drv_done(void);

/* MySQL driver definition */

static db_driver_t mysql_driver =
{
  .sname = "mysql",
  .lname = "MySQL driver",
  .args = mysql_drv_args,
  .ops = {
    .init = mysql_drv_init,
    .thread_init = mysql_drv_thread_init,
    .describe = mysql_drv_describe,
    .connect = mysql_drv_connect,
    .disconnect = mysql_drv_disconnect,
    .reconnect = mysql_drv_reconnect,
    .prepare = mysql_drv_prepare,
    .bind_param = mysql_drv_bind_param,
    .bind_result = mysql_drv_bind_result,
    .execute = mysql_drv_execute,
    .stmt_next_result = mysql_drv_stmt_next_result,
    .fetch = mysql_drv_fetch,
    .fetch_row = mysql_drv_fetch_row,
    .more_results = mysql_drv_more_results,
    .next_result = mysql_drv_next_result,
    .free_results = mysql_drv_free_results,
    .close = mysql_drv_close,
    .query = mysql_drv_query,
    .thread_done = mysql_drv_thread_done,
    .done = mysql_drv_done
  }
};


/* Local functions */

static int get_mysql_bind_type(db_bind_type_t);

/* Register MySQL driver */


int register_driver_mysql(sb_list_t *drivers)
{
  SB_LIST_ADD_TAIL(&mysql_driver.listitem, drivers);

  return 0;
}


/* MySQL driver initialization */


int mysql_drv_init(void)
{
  pthread_mutex_init(&pos_mutex, NULL);

  args.hosts = sb_get_value_list("mysql-host");
  if (SB_LIST_IS_EMPTY(args.hosts))
  {
    log_text(LOG_FATAL, "No MySQL hosts specified, aborting");
    return 1;
  }
  hosts_pos = SB_LIST_ITEM_NEXT(args.hosts);

  args.ports = sb_get_value_list("mysql-port");
  if (SB_LIST_IS_EMPTY(args.ports))
  {
    log_text(LOG_FATAL, "No MySQL ports specified, aborting");
    return 1;
  }
  ports_pos = SB_LIST_ITEM_NEXT(args.ports);

  args.sockets = sb_get_value_list("mysql-socket");
  sockets_pos = args.sockets;

  args.user = sb_get_value_string("mysql-user");
  args.password = sb_get_value_string("mysql-password");
  args.db = sb_get_value_string("mysql-db");

  args.ssl_cipher = sb_get_value_string("mysql-ssl-cipher");
  args.ssl_key = sb_get_value_string("mysql-ssl-key");
  args.ssl_cert = sb_get_value_string("mysql-ssl-cert");
  args.ssl_ca = sb_get_value_string("mysql-ssl-ca");

#ifdef HAVE_MYSQL_OPT_SSL_MODE
  const char * const ssl_mode_string = sb_get_value_string("mysql-ssl");

  args.ssl_mode = 0;

  for (int i = 0; ssl_mode_names[i].name != NULL; i++) {
    if (!strcasecmp(ssl_mode_string, ssl_mode_names[i].name)) {
      args.ssl_mode = ssl_mode_names[i].mode;
      break;
    }
  }

  if (args.ssl_mode == 0)
  {
    log_text(LOG_FATAL, "Invalid value for --mysql-ssl: '%s'", ssl_mode_string);
    return 1;
  }

  args.use_ssl = (args.ssl_mode != SSL_MODE_DISABLED);
#else
  args.use_ssl = sb_get_value_flag("mysql-ssl");
#endif

  args.use_compression = sb_get_value_flag("mysql-compression");
#ifdef MYSQL_OPT_COMPRESSION_ALGORITHMS
  args.compression_alg = sb_get_value_string("mysql-compression-algorithms");
#endif

  args.debug = sb_get_value_flag("mysql-debug");
  if (args.debug)
    sb_globals.verbosity = LOG_DEBUG;

  args.ignored_errors = sb_get_value_list("mysql-ignore-errors");

  args.dry_run = sb_get_value_flag("mysql-dry-run");

  use_ps = 0;
  mysql_drv_caps.prepared_statements = 1;
  if (db_globals.ps_mode != DB_PS_MODE_DISABLE)
    use_ps = 1;

  DEBUG("mysql_library_init(%d, %p, %p)", 0, NULL, NULL);
  mysql_library_init(0, NULL, NULL);

  return 0;
}

/* Thread-local driver initialization */

int mysql_drv_thread_init(int thread_id)
{
  (void) thread_id; /* unused */

  const my_bool rc = mysql_thread_init();
  DEBUG("mysql_thread_init() = %d", (int) rc);

  return rc != 0;
}

/* Thread-local driver deinitialization */

int mysql_drv_thread_done(int thread_id)
{
  (void) thread_id; /* unused */

  DEBUG("mysql_thread_end(%s)", "");
  mysql_thread_end();

  return 0;
}

/* Describe database capabilities */

int mysql_drv_describe(drv_caps_t *caps)
{
  *caps = mysql_drv_caps;

  return 0;
}


static int mysql_drv_real_connect(db_mysql_conn_t *db_mysql_con)
{
  MYSQL          *con = db_mysql_con->mysql;

#ifdef HAVE_MYSQL_OPT_SSL_MODE
  DEBUG("mysql_options(%p,%s,%d)", con, "MYSQL_OPT_SSL_MODE", args.ssl_mode);
  mysql_options(con, MYSQL_OPT_SSL_MODE, &args.ssl_mode);
#endif

  if (args.use_ssl)
  {
    DEBUG("mysql_ssl_set(%p, \"%s\", \"%s\", \"%s\", NULL, \"%s\")", con,
          SAFESTR(args.ssl_key), SAFESTR(args.ssl_cert), SAFESTR(args.ssl_ca),
          SAFESTR(args.ssl_cipher));

    mysql_ssl_set(con, args.ssl_key, args.ssl_cert, args.ssl_ca, NULL,
                  args.ssl_cipher);
  }

  if (args.use_compression)
  {
    DEBUG("mysql_options(%p, %s, %s)",con, "MYSQL_OPT_COMPRESS", "NULL");
    mysql_options(con, MYSQL_OPT_COMPRESS, NULL);

#ifdef MYSQL_OPT_COMPRESSION_ALGORITHMS
    DEBUG("mysql_options(%p, %s, %s)",con, "MYSQL_OPT_COMPRESSION_ALGORITHMS", args.compression_alg);
    mysql_options(con, MYSQL_OPT_COMPRESSION_ALGORITHMS, args.compression_alg);
#endif
  }

  DEBUG("mysql_real_connect(%p, \"%s\", \"%s\", \"%s\", \"%s\", %u, \"%s\", %s)",
        con,
        SAFESTR(db_mysql_con->host),
        SAFESTR(db_mysql_con->user),
        SAFESTR(db_mysql_con->password),
        SAFESTR(db_mysql_con->db),
        db_mysql_con->port,
        SAFESTR(db_mysql_con->socket),
        (MYSQL_VERSION_ID >= 50000) ? "CLIENT_MULTI_STATEMENTS" : "0"
        );

  return mysql_real_connect(con,
                            db_mysql_con->host,
                            db_mysql_con->user,
                            db_mysql_con->password,
                            db_mysql_con->db,
                            db_mysql_con->port,
                            db_mysql_con->socket,
#if MYSQL_VERSION_ID >= 50000
                            CLIENT_MULTI_STATEMENTS
#else
                            0
#endif
                            ) == NULL;
}


/* Connect to MySQL database */


int mysql_drv_connect(db_conn_t *sb_conn)
{
  MYSQL           *con;
  db_mysql_conn_t *db_mysql_con;

  if (args.dry_run)
    return 0;

  db_mysql_con = (db_mysql_conn_t *) calloc(1, sizeof(db_mysql_conn_t));

  if (db_mysql_con == NULL)
    return 1;

  con = (MYSQL *) malloc(sizeof(MYSQL));
  if (con == NULL)
    return 1;

  db_mysql_con->mysql = con;

  DEBUG("mysql_init(%p)", con);
  mysql_init(con);

  pthread_mutex_lock(&pos_mutex);

  if (SB_LIST_IS_EMPTY(args.sockets))
  {
    db_mysql_con->socket = NULL;
    db_mysql_con->host = SB_LIST_ENTRY(hosts_pos, value_t, listitem)->data;
    db_mysql_con->port =
      atoi(SB_LIST_ENTRY(ports_pos, value_t, listitem)->data);

    /*
       Pick the next port in args.ports. If there are no more ports in the list,
       move to the next available host and get the first port again.
    */
    ports_pos = SB_LIST_ITEM_NEXT(ports_pos);
    if (ports_pos == args.ports) {
      hosts_pos = SB_LIST_ITEM_NEXT(hosts_pos);
      if (hosts_pos == args.hosts)
        hosts_pos = SB_LIST_ITEM_NEXT(hosts_pos);

      ports_pos = SB_LIST_ITEM_NEXT(ports_pos);
    }
  }
  else
  {
    db_mysql_con->host = "localhost";

    /*
       The sockets list may be empty. So unlike hosts/ports the loop invariant
       here is that sockets_pos points to the previous one and should be
       advanced before using it, not after.
     */
    sockets_pos = SB_LIST_ITEM_NEXT(sockets_pos);
    if (sockets_pos == args.sockets)
      sockets_pos = SB_LIST_ITEM_NEXT(sockets_pos);

    db_mysql_con->socket = SB_LIST_ENTRY(sockets_pos, value_t, listitem)->data;
  }
  pthread_mutex_unlock(&pos_mutex);

  db_mysql_con->user = args.user;
  db_mysql_con->password = args.password;
  db_mysql_con->db = args.db;

  if (mysql_drv_real_connect(db_mysql_con))
  {
    if (!SB_LIST_IS_EMPTY(args.sockets))
      log_text(LOG_FATAL, "unable to connect to MySQL server on socket '%s', "
               "aborting...", db_mysql_con->socket);
    else
      log_text(LOG_FATAL, "unable to connect to MySQL server on host '%s', "
               "port %u, aborting...",
               db_mysql_con->host, db_mysql_con->port);
    log_text(LOG_FATAL, "error %d: %s", mysql_errno(con),
             mysql_error(con));
    free(db_mysql_con);
    free(con);
    return 1;
  }

  if (args.use_ssl)
  {
    DEBUG("mysql_get_ssl_cipher(con): \"%s\"",
          SAFESTR(mysql_get_ssl_cipher(con)));
  }

  sb_conn->ptr = db_mysql_con;

  return 0;
}


/* Disconnect from MySQL database */


int mysql_drv_disconnect(db_conn_t *sb_conn)
{
  db_mysql_conn_t *db_mysql_con = sb_conn->ptr;

  if (args.dry_run)
    return 0;
  if (db_mysql_con != NULL && db_mysql_con->mysql != NULL)
  {
    DEBUG("mysql_close(%p)", db_mysql_con->mysql);
    mysql_close(db_mysql_con->mysql);
    free(db_mysql_con->mysql);
    free(db_mysql_con);
  }

  return 0;
}


/* Prepare statement */


int mysql_drv_prepare(db_stmt_t *stmt, const char *query, size_t len)
{
  MYSQL_STMT *mystmt;
  unsigned int rc;

  if (args.dry_run)
    return 0;

  db_mysql_conn_t *db_mysql_con = (db_mysql_conn_t *) stmt->connection->ptr;
  MYSQL      *con = db_mysql_con->mysql;

  if (con == NULL)
    return 1;

  if (use_ps)
  {
    mystmt = mysql_stmt_init(con);
    DEBUG("mysql_stmt_init(%p) = %p", con, mystmt);
    if (mystmt == NULL)
    {
      log_text(LOG_FATAL, "mysql_stmt_init() failed");
      return 1;
    }
    stmt->ptr = (void *)mystmt;
    DEBUG("mysql_stmt_prepare(%p, \"%s\", %u) = %p", mystmt, query,
          (unsigned int) len, stmt->ptr);
    if (mysql_stmt_prepare(mystmt, query, len))
    {
      /* Check if this statement in not supported */
      rc = mysql_errno(con);
      DEBUG("mysql_errno(%p) = %u", con, rc);
      if (rc == ER_UNSUPPORTED_PS)
      {
        log_text(LOG_INFO,
                 "Failed to prepare query \"%s\" (%d: %s), using emulation",
                 query, rc, mysql_error(con));
        goto emulate;
      }
      else
      {
        log_text(LOG_FATAL, "mysql_stmt_prepare() failed");
        log_text(LOG_FATAL, "MySQL error: %d \"%s\"", rc,
                 mysql_error(con));
        DEBUG("mysql_stmt_close(%p)", mystmt);
        mysql_stmt_close(mystmt);
        return 1;
      }
    }

    stmt->query = strdup(query);

    return 0;
  }

 emulate:

  /* Use client-side PS */
  stmt->emulated = 1;
  stmt->query = strdup(query);

  return 0;
}


static void convert_to_mysql_bind(MYSQL_BIND *mybind, db_bind_t *bind)
{
  mybind->buffer_type = get_mysql_bind_type(bind->type);
  mybind->buffer = bind->buffer;
  mybind->buffer_length = bind->max_len;
  mybind->length = bind->data_len;
  /*
    Reuse the buffer passed by the caller to avoid conversions. This is only
    valid if sizeof(char) == sizeof(mybind->is_null[0]). Depending on the
    version of the MySQL client library, the type of MYSQL_BIND::is_null[0] can
    be either my_bool or bool, but sizeof(bool) is not defined by the C
    standard. We assume it to be 1 on most platforms to simplify code and Lua
    API.
  */
#if SIZEOF_BOOL > 1
# error This code assumes sizeof(bool) == 1!
#endif
  mybind->is_null = (my_bool *) bind->is_null;
}


/* Bind parameters for prepared statement */


int mysql_drv_bind_param(db_stmt_t *stmt, db_bind_t *params, size_t len)
{
  MYSQL_BIND   *bind;
  unsigned int i;
  my_bool rc;
  unsigned long param_count;

  if (args.dry_run)
    return 0;

  db_mysql_conn_t *db_mysql_con = (db_mysql_conn_t *) stmt->connection->ptr;
  MYSQL        *con = db_mysql_con->mysql;

  if (con == NULL)
    return 1;

  if (!stmt->emulated)
  {
    if (stmt->ptr == NULL)
      return 1;
    /* Validate parameters count */
    param_count = mysql_stmt_param_count(stmt->ptr);
    DEBUG("mysql_stmt_param_count(%p) = %lu", stmt->ptr, param_count);
    if (param_count != len)
    {
      log_text(LOG_FATAL, "Wrong number of parameters to mysql_stmt_bind_param");
      return 1;
    }
    /* Convert sysbench bind structures to MySQL ones */
    bind = (MYSQL_BIND *)calloc(len, sizeof(MYSQL_BIND));
    if (bind == NULL)
      return 1;
    for (i = 0; i < len; i++)
      convert_to_mysql_bind(&bind[i], &params[i]);

    rc = mysql_stmt_bind_param(stmt->ptr, bind);
    DEBUG("mysql_stmt_bind_param(%p, %p) = %d", stmt->ptr, bind, rc);
    if (rc)
    {
      log_text(LOG_FATAL, "mysql_stmt_bind_param() failed");
      log_text(LOG_FATAL, "MySQL error: %d \"%s\"", mysql_errno(con),
                 mysql_error(con));
      free(bind);
      return 1;
    }
    free(bind);

    return 0;
  }

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


int mysql_drv_bind_result(db_stmt_t *stmt, db_bind_t *params, size_t len)
{
  MYSQL_BIND   *bind;
  unsigned int i;
  my_bool rc;

  if (args.dry_run)
    return 0;

  db_mysql_conn_t *db_mysql_con =(db_mysql_conn_t *) stmt->connection->ptr;
  MYSQL        *con = db_mysql_con->mysql;

  if (con == NULL || stmt->ptr == NULL)
    return 1;

  /* Convert sysbench bind structures to MySQL ones */
  bind = (MYSQL_BIND *)calloc(len, sizeof(MYSQL_BIND));
  if (bind == NULL)
    return 1;
  for (i = 0; i < len; i++)
    convert_to_mysql_bind(&bind[i], &params[i]);

  rc = mysql_stmt_bind_result(stmt->ptr, bind);
  DEBUG("mysql_stmt_bind_result(%p, %p) = %d", stmt->ptr, bind, rc);
  if (rc)
  {
    free(bind);
    return 1;
  }
  free(bind);

  return 0;
}

/* Reset connection to the server by reconnecting with the same parameters. */

static int mysql_drv_reconnect(db_conn_t *sb_con)
{
  db_mysql_conn_t *db_mysql_con = (db_mysql_conn_t *) sb_con->ptr;
  MYSQL *con = db_mysql_con->mysql;

  log_text(LOG_DEBUG, "Reconnecting");

  DEBUG("mysql_close(%p)", con);
  mysql_close(con);

  while (mysql_drv_real_connect(db_mysql_con))
  {
    if (sb_globals.error)
      return DB_ERROR_FATAL;

    usleep(1000);
  }

  log_text(LOG_DEBUG, "Reconnected");

  return DB_ERROR_IGNORABLE;
}


/*
  Check if the error in a given connection should be fatal or ignored according
  to the list of errors in --mysql-ignore-errors.
*/


static db_error_t check_error(db_conn_t *sb_con, const char *func,
                              const char *query, sb_counter_type_t *counter)
{
  sb_list_item_t *pos;
  unsigned int   tmp;
  db_mysql_conn_t *db_mysql_con = (db_mysql_conn_t *) sb_con->ptr;
  MYSQL          *con = db_mysql_con->mysql;

  const unsigned int error = mysql_errno(con);
  DEBUG("mysql_errno(%p) = %u", con, sb_con->sql_errno);

  sb_con->sql_errno = (int) error;

  sb_con->sql_state = mysql_sqlstate(con);
  DEBUG("mysql_state(%p) = %s", con, SAFESTR(sb_con->sql_state));

  sb_con->sql_errmsg = mysql_error(con);
  DEBUG("mysql_error(%p) = %s", con, SAFESTR(sb_con->sql_errmsg));

  /*
    Check if the error code is specified in --mysql-ignore-errors, and return
    DB_ERROR_IGNORABLE if so, or DB_ERROR_FATAL otherwise
  */
  SB_LIST_FOR_EACH(pos, args.ignored_errors)
  {
    const char *val = SB_LIST_ENTRY(pos, value_t, listitem)->data;

    tmp = (unsigned int) atoi(val);

    if (error == tmp || !strcmp(val, "all"))
    {
      log_text(LOG_DEBUG, "Ignoring error %u %s, ", error, sb_con->sql_errmsg);

      /* Check if we should reconnect */
      switch (error)
      {
      case CR_SERVER_LOST:
      case CR_SERVER_GONE_ERROR:
      case CR_TCP_CONNECTION:
      case CR_SERVER_LOST_EXTENDED:

        *counter = SB_CNT_RECONNECT;

        return mysql_drv_reconnect(sb_con);

      default:

        break;
      }

      *counter = SB_CNT_ERROR;

      return DB_ERROR_IGNORABLE;
    }
  }

  if (query)
    log_text(LOG_FATAL, "%s returned error %u (%s) for query '%s'",
             func, error, sb_con->sql_errmsg, query);
  else
    log_text(LOG_FATAL, "%s returned error %u (%s)",
             func, error, sb_con->sql_errmsg);

  *counter = SB_CNT_ERROR;

  return DB_ERROR_FATAL;
}

/* Execute prepared statement */


db_error_t mysql_drv_execute(db_stmt_t *stmt, db_result_t *rs)
{
  db_conn_t       *con = stmt->connection;
  char            *buf = NULL;
  unsigned int    buflen = 0;
  unsigned int    i, j, vcnt;
  char            need_realloc;
  int             n;

  if (args.dry_run)
    return DB_ERROR_NONE;

  con->sql_errno = 0;
  con->sql_state = NULL;
  con->sql_errmsg = NULL;

  if (!stmt->emulated)
  {
    if (stmt->ptr == NULL)
    {
      log_text(LOG_DEBUG,
               "ERROR: exiting mysql_drv_execute(), uninitialized statement");
      return DB_ERROR_FATAL;
    }

    int err = mysql_stmt_execute(stmt->ptr);
    DEBUG("mysql_stmt_execute(%p) = %d", stmt->ptr, err);

    if (err)
      return check_error(con, "mysql_stmt_execute()", stmt->query,
                         &rs->counter);

    err = mysql_stmt_store_result(stmt->ptr);
    DEBUG("mysql_stmt_store_result(%p) = %d", stmt->ptr, err);

    if (err)
      return check_error(con, "mysql_stmt_store_result()", NULL, &rs->counter);

    if (mysql_stmt_errno(stmt->ptr) == 0 &&
        mysql_stmt_field_count(stmt->ptr) == 0)
    {
      rs->nrows = (uint32_t) mysql_stmt_affected_rows(stmt->ptr);
      DEBUG("mysql_stmt_affected_rows(%p) = %u", stmt->ptr,
            (unsigned) rs->nrows);

      rs->counter = (rs->nrows > 0) ? SB_CNT_WRITE : SB_CNT_OTHER;

      return DB_ERROR_NONE;
    }

    rs->counter = SB_CNT_READ;

    rs->nrows = (uint32_t) mysql_stmt_num_rows(stmt->ptr);
    DEBUG("mysql_stmt_num_rows(%p) = %u", rs->statement->ptr,
          (unsigned) (rs->nrows));

    rs->nfields = (uint32_t) mysql_stmt_field_count(stmt->ptr);
    DEBUG("mysql_stmt_field_count(%p) = %u", rs->statement->ptr,
          (unsigned) (rs->nfields));

    return DB_ERROR_NONE;
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
      {
        log_text(LOG_DEBUG, "ERROR: exiting mysql_drv_execute(), memory allocation failure");
        return DB_ERROR_FATAL;
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

  db_error_t rc = mysql_drv_query(con, buf, j, rs);

  free(buf);

  return rc;
}

/* Retrieve the next result of a prepared statement */

db_error_t mysql_drv_stmt_next_result(db_stmt_t *stmt, db_result_t *rs)
{
  db_conn_t       *con = stmt->connection;

  if (args.dry_run)
    return DB_ERROR_NONE;

  con->sql_errno = 0;
  con->sql_state = NULL;
  con->sql_errmsg = NULL;

  if (stmt->emulated)
    return mysql_drv_next_result(con, rs);

  if (stmt->ptr == NULL)
    {
      log_text(LOG_DEBUG,
               "ERROR: exiting mysql_drv_stmt_next_result(), "
               "uninitialized statement");
      return DB_ERROR_FATAL;
    }

  int err = mysql_stmt_next_result(stmt->ptr);
  DEBUG("mysql_stmt_next_result(%p) = %d", stmt->ptr, err);

  if (SB_UNLIKELY(err > 0))
    return check_error(con, "mysql_drv_stmt_next_result()", stmt->query,
                       &rs->counter);

  if (err == -1)
  {
    rs->counter = SB_CNT_OTHER;
    return DB_ERROR_NONE;
  }

  err = mysql_stmt_store_result(stmt->ptr);
  DEBUG("mysql_stmt_store_result(%p) = %d", stmt->ptr, err);

  if (err)
    return check_error(con, "mysql_stmt_store_result()", NULL, &rs->counter);

  if (mysql_stmt_errno(stmt->ptr) == 0 &&
      mysql_stmt_field_count(stmt->ptr) == 0)
  {
    rs->nrows = (uint32_t) mysql_stmt_affected_rows(stmt->ptr);
    DEBUG("mysql_stmt_affected_rows(%p) = %u", stmt->ptr,
          (unsigned) rs->nrows);

    rs->counter = (rs->nrows > 0) ? SB_CNT_WRITE : SB_CNT_OTHER;

    return DB_ERROR_NONE;
  }

  rs->counter = SB_CNT_READ;

  rs->nrows = (uint32_t) mysql_stmt_num_rows(stmt->ptr);
  DEBUG("mysql_stmt_num_rows(%p) = %u", rs->statement->ptr,
        (unsigned) (rs->nrows));

  rs->nfields = (uint32_t) mysql_stmt_field_count(stmt->ptr);
  DEBUG("mysql_stmt_field_count(%p) = %u", rs->statement->ptr,
        (unsigned) (rs->nfields));

  return DB_ERROR_NONE;
}


/* Execute SQL query */


db_error_t mysql_drv_query(db_conn_t *sb_conn, const char *query, size_t len,
                           db_result_t *rs)
{
  db_mysql_conn_t *db_mysql_con;
  MYSQL *con;

  if (args.dry_run)
    return DB_ERROR_NONE;

  sb_conn->sql_errno = 0;
  sb_conn->sql_state = NULL;
  sb_conn->sql_errmsg = NULL;

  db_mysql_con = (db_mysql_conn_t *)sb_conn->ptr;
  con = db_mysql_con->mysql;

  int err = mysql_real_query(con, query, len);
  DEBUG("mysql_real_query(%p, \"%s\", %zd) = %d", con, query, len, err);

  if (SB_UNLIKELY(err != 0))
    return check_error(sb_conn, "mysql_drv_query()", query, &rs->counter);

  /* Store results and get query type */
  MYSQL_RES *res = mysql_store_result(con);
  DEBUG("mysql_store_result(%p) = %p", con, res);

  if (res == NULL)
  {
    if (mysql_errno(con) == 0 && mysql_field_count(con) == 0)
    {
      /* Not a select. Check if it was a DML */
      uint32_t nrows = (uint32_t) mysql_affected_rows(con);
      if (nrows > 0)
      {
        rs->counter = SB_CNT_WRITE;
        rs->nrows = nrows;
      }
      else
        rs->counter = SB_CNT_OTHER;

      return DB_ERROR_NONE;
    }

    return check_error(sb_conn, "mysql_store_result()", NULL, &rs->counter);
  }

  rs->counter = SB_CNT_READ;
  rs->ptr = (void *)res;

  rs->nrows = mysql_num_rows(res);
  DEBUG("mysql_num_rows(%p) = %u", res, (unsigned int) rs->nrows);

  rs->nfields = mysql_num_fields(res);
  DEBUG("mysql_num_fields(%p) = %u", res, (unsigned int) rs->nfields);

  return DB_ERROR_NONE;
}


/* Fetch row from result set of a prepared statement */


int mysql_drv_fetch(db_result_t *rs)
{
  /* NYI */
  (void)rs;  /* unused */

  if (args.dry_run)
    return DB_ERROR_NONE;

  return 1;
}

/* Fetch row from result set of a query */

int mysql_drv_fetch_row(db_result_t *rs, db_row_t *row)
{
  MYSQL_ROW my_row;

  if (args.dry_run)
    return DB_ERROR_NONE;

  my_row = mysql_fetch_row(rs->ptr);
  DEBUG("mysql_fetch_row(%p) = %p", rs->ptr, my_row);

  unsigned long *lengths = mysql_fetch_lengths(rs->ptr);
  DEBUG("mysql_fetch_lengths(%p) = %p", rs->ptr, lengths);

  if (lengths == NULL)
    return DB_ERROR_IGNORABLE;

  for (size_t i = 0; i < rs->nfields; i++)
  {
    row->values[i].len = lengths[i];
    row->values[i].ptr = my_row[i];
  }

  return DB_ERROR_NONE;
}

/* Check if more result sets are available */

bool mysql_drv_more_results(db_conn_t *sb_conn)
{
  db_mysql_conn_t *db_mysql_con;
  MYSQL *con;

  if (args.dry_run)
    return false;

  db_mysql_con = (db_mysql_conn_t *)sb_conn->ptr;
  con = db_mysql_con->mysql;

  bool res = mysql_more_results(con);
  DEBUG("mysql_more_results(%p) = %d", con, res);

  return res;
}

/* Retrieve the next result set */

db_error_t mysql_drv_next_result(db_conn_t *sb_conn, db_result_t *rs)
{
  db_mysql_conn_t *db_mysql_con;
  MYSQL *con;

  if (args.dry_run)
    return DB_ERROR_NONE;

  sb_conn->sql_errno = 0;
  sb_conn->sql_state = NULL;
  sb_conn->sql_errmsg = NULL;

  db_mysql_con = (db_mysql_conn_t *)sb_conn->ptr;
  con = db_mysql_con->mysql;

  int err = mysql_next_result(con);
  DEBUG("mysql_next_result(%p) = %d", con, err);

  if (SB_UNLIKELY(err > 0))
    return check_error(sb_conn, "mysql_drv_next_result()", NULL, &rs->counter);

  if (err == -1)
  {
    rs->counter = SB_CNT_OTHER;
    return DB_ERROR_NONE;
  }

  /* Store results and get query type */
  MYSQL_RES *res = mysql_store_result(con);
  DEBUG("mysql_store_result(%p) = %p", con, res);

  if (res == NULL)
  {
    if (mysql_errno(con) == 0 && mysql_field_count(con) == 0)
    {
      /* Not a select. Check if it was a DML */
      uint32_t nrows = (uint32_t) mysql_affected_rows(con);
      if (nrows > 0)
      {
        rs->counter = SB_CNT_WRITE;
        rs->nrows = nrows;
      }
      else
        rs->counter = SB_CNT_OTHER;

      return DB_ERROR_NONE;
    }

    return check_error(sb_conn, "mysql_store_result()", NULL, &rs->counter);
  }

  rs->counter = SB_CNT_READ;
  rs->ptr = (void *)res;

  rs->nrows = mysql_num_rows(res);
  DEBUG("mysql_num_rows(%p) = %u", res, (unsigned int) rs->nrows);

  rs->nfields = mysql_num_fields(res);
  DEBUG("mysql_num_fields(%p) = %u", res, (unsigned int) rs->nfields);

  return DB_ERROR_NONE;
}

/* Free result set */

int mysql_drv_free_results(db_result_t *rs)
{
  if (args.dry_run)
    return 0;

  /* Is this a result set of a prepared statement? */
  if (rs->statement != NULL && rs->statement->emulated == 0)
  {
    DEBUG("mysql_stmt_free_result(%p)", rs->statement->ptr);
    mysql_stmt_free_result(rs->statement->ptr);
    rs->ptr = NULL;
  }

  if (rs->ptr != NULL)
  {
    DEBUG("mysql_free_result(%p)", rs->ptr);
    mysql_free_result((MYSQL_RES *)rs->ptr);
    rs->ptr = NULL;
  }

  return 0;
}


/* Close prepared statement */


int mysql_drv_close(db_stmt_t *stmt)
{
  if (args.dry_run)
    return 0;

  if (stmt->query)
  {
    free(stmt->query);
    stmt->query = NULL;
  }

  if (stmt->ptr == NULL)
    return 1;

  int rc = mysql_stmt_close(stmt->ptr);
  DEBUG("mysql_stmt_close(%p) = %d", stmt->ptr, rc);

  stmt->ptr = NULL;

  return rc;
}


/* Uninitialize driver */
int mysql_drv_done(void)
{
  if (args.dry_run)
    return 0;

  mysql_library_end();

  return 0;
}

/* Map SQL data type to bind_type value in MYSQL_BIND */

int get_mysql_bind_type(db_bind_type_t type)
{
  unsigned int i;

  for (i = 0; db_mysql_bind_map[i].db_type != DB_TYPE_NONE; i++)
    if (db_mysql_bind_map[i].db_type == type)
      return db_mysql_bind_map[i].my_type;

  return -1;
}
