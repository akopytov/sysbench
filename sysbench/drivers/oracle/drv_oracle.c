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
#ifdef _WIN32
#include <winsock2.h>
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <oci.h>

#include "sb_options.h"
#include "db_driver.h"


/* Number of rows to prefetch for result sets */
#define ORA_DRV_PREFETCH_COUNT 1000

#define CHECKERR(stmt)                          \
  do { \
    if (rc != OCI_SUCCESS) \
    { \
      log_text(LOG_FATAL, "%s failed in %s:%d", stmt, __FILE__, __LINE__); \
      checkerr(ora_con->errhp, rc); \
      goto error; \
    } \
  } while(0);

static sb_arg_t ora_drv_args[] =
{
  {"oracle-user", "Oracle user", SB_ARG_TYPE_STRING, "sbtest"},
  {"oracle-password", "Oracle password", SB_ARG_TYPE_STRING, ""},
  {"oracle-db", "Oracle database name", SB_ARG_TYPE_STRING, "sbtest"},
  
  {NULL, NULL, SB_ARG_TYPE_NULL, NULL}
};

typedef struct
{
  OCISvcCtx  *svchp;
  OCIServer  *srvhp;
  OCIError   *errhp;
  OCITrans   *transhp;
  OCISession *usrhp;
} ora_conn_t;

typedef struct
{
  sb2 ind;
} ora_bind_t;

typedef enum
{
  STMT_TYPE_BEGIN,
  STMT_TYPE_COMMIT,
  STMT_TYPE_SELECT,
  STMT_TYPE_UPDATE
} ora_stmt_type_t;

typedef struct
{
  OCIStmt         *ptr;
  ora_stmt_type_t type;
  ora_bind_t      *params;
  ora_bind_t      *results;
} ora_stmt_t;

typedef struct
{
  ub2       type;
  text      *name;
  ub2       len;
  OCIDefine *defhp;
  void      *value;
  sb2       ind;
  
  sb_list_item_t listitem;
} ora_column_t;

typedef struct
{
  void *value;
  sb2  ind;
} ora_data_t;

typedef struct
{
  ora_data_t *data;
  sb_list_item_t listitem;
} ora_row_t;

typedef struct
{
  ub4       ncolumns;
  ub4       nrows;
  sb_list_t columns;
  sb_list_t rows;
} ora_result_set_t;

typedef struct
{
  char               *user;
  char               *password;
  char               *db;
} ora_drv_args_t;

/* Structure used for DB-to-Oracle bind types map */

typedef struct
{
  db_bind_type_t   db_type;
  ub2              ora_type;
  sb4              ora_len;
} db_oracle_bind_map_t;

/* DB-to-Oracle bind types map */
db_oracle_bind_map_t db_oracle_bind_map[] =
{

  {DB_TYPE_TINYINT,   SQLT_INT,       sizeof(char)},
  {DB_TYPE_SMALLINT,  SQLT_INT,       sizeof(short)},
  {DB_TYPE_INT,       SQLT_INT,       sizeof(int)} ,
  {DB_TYPE_BIGINT,    SQLT_INT,       sizeof(long long)},
  {DB_TYPE_FLOAT,     SQLT_FLT,       sizeof(float)},
  {DB_TYPE_DOUBLE,    SQLT_FLT,       sizeof(double)},
  {DB_TYPE_DATETIME,  SQLT_DATE,      sizeof(void *)},
  {DB_TYPE_TIMESTAMP, SQLT_TIMESTAMP, sizeof(void *)},
  {DB_TYPE_CHAR,      SQLT_AFC,       0},
  {DB_TYPE_VARCHAR,   SQLT_VCS,       0},
  {DB_TYPE_NONE,      0,              0}
};

/* Oracle driver capabilities */

static drv_caps_t ora_drv_caps =
{
  0,
  1,
  1,
  0,
  1,
  0,
  0,
  NULL
};


static OCIEnv *ora_env; /* OCI environmental handle */

static ora_drv_args_t args;          /* driver args */

/* Oracle driver operations */

static int ora_drv_init(void);
static int ora_drv_describe(drv_caps_t *, const char *);
static int ora_drv_connect(db_conn_t *);
static int ora_drv_disconnect(db_conn_t *);
static int ora_drv_prepare(db_stmt_t *, const char *);
static int ora_drv_bind_param(db_stmt_t *, db_bind_t *, unsigned int);
static int ora_drv_bind_result(db_stmt_t *, db_bind_t *, unsigned int);
static int ora_drv_execute(db_stmt_t *, db_result_set_t *);
static int ora_drv_fetch(db_result_set_t *);
static int ora_drv_fetch_row(db_result_set_t *, db_row_t *);
static unsigned long long ora_drv_num_rows(db_result_set_t *);
static int ora_drv_query(db_conn_t *, const char *, db_result_set_t *);
static int ora_drv_free_results(db_result_set_t *);
static int ora_drv_close(db_stmt_t *);
static int ora_drv_store_results(db_result_set_t *);
static int ora_drv_done(void);

/* Oracle driver definition */

static db_driver_t oracle_driver =
{
  "oracle",
  "Oracle driver",
  ora_drv_args,
  {
    ora_drv_init,
    ora_drv_describe,
    ora_drv_connect,
    ora_drv_disconnect,
    ora_drv_prepare,
    ora_drv_bind_param,
    ora_drv_bind_result,
    ora_drv_execute,
    ora_drv_fetch,
    ora_drv_fetch_row,
    ora_drv_num_rows,
    ora_drv_free_results,
    ora_drv_close,
    ora_drv_query,
    ora_drv_store_results,
    ora_drv_done
  },
  {NULL, NULL}
};


/* Local functions */

static sword get_oracle_bind_type(db_bind_t *, ub2 *, sb4 *, sb2 *);
static sb4 get_oracle_type_size(sword);
static ora_stmt_type_t get_stmt_type(const char *);
static void checkerr(OCIError *, sword);

/* Register Oracle driver */


int register_driver_oracle(sb_list_t *drivers)
{
  SB_LIST_ADD_TAIL(&oracle_driver.listitem, drivers);

  return 0;
}


/* Oracle driver initialization */


int ora_drv_init(void)
{
  sword   rc;

  args.user = sb_get_value_string("oracle-user");
  args.password = sb_get_value_string("oracle-password");
  args.db = sb_get_value_string("oracle-db");

  /* Initialize the environment */
  rc = OCIEnvCreate(&ora_env, OCI_THREADED | OCI_OBJECT, NULL, NULL, NULL, NULL,
                    0, NULL);
  if (rc != OCI_SUCCESS || ora_env == NULL)
  {
    log_text(LOG_FATAL, "OCIEnvCreate failed!");
    return 1;
  }
  
  return 0;
}


/* Describe database capabilities  */


int ora_drv_describe(drv_caps_t *caps, const char * table_name)
{
  (void)table_name;
  *caps = ora_drv_caps;
  
  return 0;
}


/* Connect to the database */


int ora_drv_connect(db_conn_t *sb_conn)
{
  sword       rc;
  ora_conn_t *ora_con = NULL;

  ora_con = (ora_conn_t *)malloc(sizeof(ora_conn_t));
  if (ora_con == NULL)
    goto error;
  
  /* Allocate a service handle */
  rc = OCIHandleAlloc(ora_env, (dvoid **)&(ora_con->svchp), OCI_HTYPE_SVCCTX, 0,
                      (dvoid **)NULL);
  if (rc != OCI_SUCCESS)
  {
    log_text(LOG_FATAL, "OCIHandleAlloc (OCI_HTYPE_SVCCTX) failed");
    goto error;
  }

  /* Allocate an error handle */
  rc = OCIHandleAlloc(ora_env, (dvoid **)&(ora_con->errhp), OCI_HTYPE_ERROR, 0,
                      (dvoid **)NULL);
  if (rc != OCI_SUCCESS)
  {
    log_text(LOG_FATAL, "OCIHandleAlloc (OCI_HTYPE_ERROR) failed");
    goto error;
  }

  /* Allocate an server handle */
  rc = OCIHandleAlloc(ora_env, (dvoid **)&(ora_con->srvhp), OCI_HTYPE_SERVER, 0,
                      (dvoid **)NULL);
  CHECKERR("OCIHandleAlloc");
  
  /* Allocate a user session handle */
  rc = OCIHandleAlloc(ora_env, (dvoid **)&(ora_con->usrhp), OCI_HTYPE_SESSION, 0,
                      (dvoid **)NULL);
  CHECKERR("OCIHandleAlloc");

  /* Attach to the server */
  rc = OCIServerAttach(ora_con->srvhp, ora_con->errhp, args.db, strlen(args.db),
                       OCI_DEFAULT);
  CHECKERR("OCIServerAttach");

  /* Set the server attribute in the service context handler */
  rc = OCIAttrSet(ora_con->svchp, OCI_HTYPE_SVCCTX, ora_con->srvhp, 0,
                  OCI_ATTR_SERVER, ora_con->errhp);
  CHECKERR("OCIAttrSet");

  /* Set the user name attribute in the user session handler */
  rc = OCIAttrSet(ora_con->usrhp, OCI_HTYPE_SESSION, args.user,
                  strlen(args.user), OCI_ATTR_USERNAME, ora_con->errhp);
  CHECKERR("OCIAttrSet");

  /* Set the password attribute in the user session handler */
  rc = OCIAttrSet(ora_con->usrhp, OCI_HTYPE_SESSION, args.password,
                  strlen(args.password), OCI_ATTR_PASSWORD, ora_con->errhp);
  CHECKERR("OCIAttrSet");

  /* Allocate the transaction handle and set it to service context */
  rc = OCIHandleAlloc(ora_env, (dvoid **)&(ora_con->transhp), OCI_HTYPE_TRANS, 0,
                      (dvoid **)NULL);
  CHECKERR("OCIHandleAlloc");
  rc = OCIAttrSet(ora_con->svchp, OCI_HTYPE_SVCCTX, ora_con->transhp, 0,
                  OCI_ATTR_TRANS, ora_con->errhp);
  CHECKERR("OCIAttrSet");
    
  /* Start the session */
  rc = OCISessionBegin (ora_con->svchp, ora_con->errhp, ora_con->usrhp,
                        OCI_CRED_RDBMS, OCI_DEFAULT);
  CHECKERR("OCISessionBegin");
  
  /* Set the user session attribute in the service context handler */
  rc = OCIAttrSet(ora_con->svchp, OCI_HTYPE_SVCCTX, ora_con->usrhp, 0,
                  OCI_ATTR_SESSION, ora_con->errhp);
  CHECKERR("OCIAttrSet");
  
  sb_conn->ptr = ora_con;
  
  return 0;

 error:
  if (ora_con != NULL)
    free(ora_con);
  
  return 1;
}


/* Disconnect from database */


int ora_drv_disconnect(db_conn_t *sb_conn)
{
  ora_conn_t *con = sb_conn->ptr;
  sword       rc;
  int         res = 0;
  
  if (con == NULL)
    return 1;

  rc = OCISessionEnd(con->svchp, con->errhp, con->usrhp, 0);
  if (rc != OCI_SUCCESS)
  {
    log_text(LOG_FATAL, "OCISessionEnd failed");
    res = 1;
  }

  rc = OCIServerDetach(con->srvhp, con->errhp, OCI_DEFAULT);
  if (rc != OCI_SUCCESS)
  {
    log_text(LOG_FATAL, "OCIServerDetach failed");
    res = 1;
  }

  /* Free handles */
  
  if (OCIHandleFree(con->usrhp, OCI_HTYPE_SESSION) != OCI_SUCCESS)
    res = 1;
  if (OCIHandleFree(con->srvhp, OCI_HTYPE_SERVER) != OCI_SUCCESS)
    res = 1;
  if (OCIHandleFree(con->svchp, OCI_HTYPE_SVCCTX) != OCI_SUCCESS)
    res = 1;
  if (OCIHandleFree(con->errhp, OCI_HTYPE_ERROR) != OCI_SUCCESS)
    res = 1;
  
  free(con);
  
  return res;
}


/* Prepare statement */


int ora_drv_prepare(db_stmt_t *stmt, const char *query)
{
  ora_conn_t   *ora_con = (ora_conn_t *)stmt->connection->ptr;
  sword        rc;
  ora_stmt_t   *ora_stmt = NULL;
  char         *buf = NULL;
  unsigned int vcnt;
  unsigned int need_realloc;
  unsigned int i,j;
  unsigned int buflen;
  int          n;
  ub4          prefetch_cnt = ORA_DRV_PREFETCH_COUNT;
  
  if (ora_con == NULL)
    return 1;

  if (db_globals.ps_mode != DB_PS_MODE_DISABLE)
  {
    ora_stmt = (ora_stmt_t *)calloc(1, sizeof(ora_stmt_t));
    if (ora_stmt == NULL)
      goto error;
    
    rc = OCIHandleAlloc(ora_env, (dvoid **)&(ora_stmt->ptr), OCI_HTYPE_STMT, 0,
                        NULL);
    if (rc != OCI_SUCCESS)
      goto error;

    /* Convert query to Oracle-style named placeholders */
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

      n = snprintf(buf + j, buflen - j, ":%d", vcnt);
      if (n < 0 || n >= (int)(buflen - j))
      {
        need_realloc = 1;
        goto again;
      }

      j += n;
      vcnt++;
    }
    buf[j] = '\0';
    
    ora_stmt->type = get_stmt_type(buf);
    
    if (ora_stmt->type != STMT_TYPE_BEGIN &&
        ora_stmt->type != STMT_TYPE_COMMIT)
    {
      rc = OCIStmtPrepare(ora_stmt->ptr, ora_con->errhp, buf, strlen(buf),
                          OCI_NTV_SYNTAX, OCI_DEFAULT);
      CHECKERR("OCIStmtPrepare");

      rc = OCIAttrSet(ora_stmt->ptr, OCI_HTYPE_STMT, &prefetch_cnt, 0,
                      OCI_ATTR_PREFETCH_ROWS, ora_con->errhp);
      CHECKERR("OCIAttrSet");
    }
    
    free(buf);
    
    stmt->ptr = (void *)ora_stmt;
  }
  else
  {
    /* Use client-side PS */
    stmt->emulated = 1;
  }
  stmt->query = strdup(query);
  
  return 0;
  
 error:
  if (ora_stmt != NULL)
  {
    if (ora_stmt->ptr != NULL)
      OCIHandleFree(ora_stmt->ptr, OCI_HTYPE_STMT);

    free(ora_stmt);
  }
  log_text(LOG_FATAL, "Failed to prepare statement: '%s'", query);
  
  return 1;
}


/* Bind parameters for prepared statement */


int ora_drv_bind_param(db_stmt_t *stmt, db_bind_t *params, unsigned int len)
{
  ora_conn_t  *con = (ora_conn_t *)stmt->connection->ptr;
  ora_stmt_t  *ora_stmt = (ora_stmt_t *)stmt->ptr;
  OCIBind *   bindp;
  unsigned int i;
  sword       rc;
  ub2         dtype;
  sb4         dlen;
  
  if (con == NULL)
    return 1;

  if (!stmt->emulated)
  {
    if (ora_stmt == NULL || ora_stmt->ptr == NULL)
      return 1;

    if (ora_stmt->params != NULL)
      free(ora_stmt->params);
    ora_stmt->params = (ora_bind_t *)malloc(len * sizeof(ora_bind_t));
    if (ora_stmt->params == NULL)
      return 1;
    
    /* Convert SysBench bind structures to Oracle ones */
    bindp = NULL;
    for (i = 0; i < len; i++)
    {
      if (get_oracle_bind_type(params+i, &dtype, &dlen,
                               &ora_stmt->params[i].ind))
      {
        free(ora_stmt->params);
        ora_stmt->params = NULL;
        return 1;
      }
      
      rc = OCIBindByPos(ora_stmt->ptr, &bindp, con->errhp, i+1, params[i].buffer,
                        dlen, dtype, (dvoid *)&ora_stmt->params[i].ind, NULL,
                        NULL, 0, NULL, OCI_DEFAULT);
      if (rc != OCI_SUCCESS)
      {
        log_text(LOG_FATAL, "OCIBindByPos failed");
        free(ora_stmt->params);
        ora_stmt->params = NULL;
        return 1;
      }
    }

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


int ora_drv_bind_result(db_stmt_t *stmt, db_bind_t *params, unsigned int len)
{
  /* NYI */

  (void)stmt;
  (void)params;
  (void)len;
  
  return 1;
}


/* Execute prepared statement */


int ora_drv_execute(db_stmt_t *stmt, db_result_set_t *rs)
{
  db_conn_t       *db_con = stmt->connection;
  ora_stmt_t      *ora_stmt = stmt->ptr;
  ora_conn_t      *ora_con;
  ub4             iters;
  char            *buf = NULL;
  unsigned int    buflen = 0;
  unsigned int    i, j, vcnt;
  char            need_realloc;
  int             n;
  sword           rc;
  
  (void)rs; /* unused */

  if (db_con == NULL)
    return SB_DB_ERROR_FAILED;
  ora_con = db_con->ptr;
  if (ora_con == NULL)
    return SB_DB_ERROR_FAILED;
  
  if (!stmt->emulated)
  {
    if (stmt->ptr == NULL)
      return SB_DB_ERROR_FAILED;

    if (ora_stmt->type == STMT_TYPE_BEGIN)
    {
      rc = OCITransStart(ora_con->svchp, ora_con->errhp, 3600, OCI_TRANS_NEW);
      CHECKERR("OCITransStart");

      return SB_DB_ERROR_NONE;
    }
    else if (ora_stmt->type == STMT_TYPE_COMMIT)
    {
      rc = OCITransCommit(ora_con->svchp, ora_con->errhp, OCI_DEFAULT);
      CHECKERR("OCITransCommit");

      return SB_DB_ERROR_NONE;
    }
    else if (ora_stmt->type == STMT_TYPE_SELECT)
      iters = 0;
    else
      iters = 1;
  
    rc = OCIStmtExecute(ora_con->svchp, ora_stmt->ptr, ora_con->errhp, iters, 0,
                        NULL, NULL, OCI_DEFAULT);
    CHECKERR("OCIStmtExecute");
    
    return SB_DB_ERROR_NONE;
  }

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
        return SB_DB_ERROR_FAILED;
      }
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
  
  db_con->db_errno = ora_drv_query(db_con, buf, rs);
  free(buf);

  return SB_DB_ERROR_NONE;

 error:
  log_text(LOG_FATAL, "failed query was: '%s'", stmt->query);
  
  return SB_DB_ERROR_FAILED;
}


/* Execute SQL query */


int ora_drv_query(db_conn_t *sb_conn, const char *query,
                      db_result_set_t *rs)
{
  ora_conn_t      *ora_con = sb_conn->ptr;
  sword           rc = 0;
  void            *tmp = NULL;
  ub4             iters;
  ora_stmt_type_t type;
  OCIStmt         *stmt = NULL;

  (void)rs; /* unused */

  type = get_stmt_type(query);

  if (type == STMT_TYPE_BEGIN)
  {
    rc = OCITransStart(ora_con->svchp, ora_con->errhp, 3600, OCI_TRANS_NEW);
    CHECKERR("OCITransStart");

    return SB_DB_ERROR_NONE;
  }
  else if (type == STMT_TYPE_COMMIT)
  {
    rc = OCITransCommit(ora_con->svchp, ora_con->errhp, OCI_DEFAULT);
    CHECKERR("OCITransCommit");

    return SB_DB_ERROR_NONE;
  }
  else if (type == STMT_TYPE_SELECT)
    iters = 0;
  else
    iters = 1;
  
  rc = OCIHandleAlloc(ora_env, (dvoid **)&tmp, OCI_HTYPE_STMT, 0, (dvoid **)NULL);
  CHECKERR("OCIHandleAlloc");

  stmt = (OCIStmt *)tmp;
  
  rc = OCIStmtPrepare(stmt, ora_con->errhp, (OraText *)query, strlen(query),
                      OCI_NTV_SYNTAX, OCI_DEFAULT);
  CHECKERR("OCIStmtPrepare");

  rc = OCIStmtExecute(ora_con->svchp, stmt, ora_con->errhp, iters, 0, NULL, NULL,
                      OCI_DEFAULT);
  CHECKERR("OCIStmtExecute");

  OCIHandleFree(stmt, OCI_HTYPE_STMT);

  return SB_DB_ERROR_NONE;

 error:
  log_text(LOG_FATAL, "failed query was: '%s'", query);
  if (stmt != NULL)
    OCIHandleFree(stmt, OCI_HTYPE_STMT);
  
  return SB_DB_ERROR_FAILED;
}


/* Fetch row from result set of a prepared statement */


int ora_drv_fetch(db_result_set_t *rs)
{
  /* NYI */
  (void)rs;

  return 1;
}


/* Fetch row from result set of a query */


int ora_drv_fetch_row(db_result_set_t *rs, db_row_t *row)
{
  /* NYI */
  (void)rs;  /* unused */
  (void)row; /* unused */
  
  return 1;
}


/* Return the number of rows in a result set */


unsigned long long ora_drv_num_rows(db_result_set_t *rs)
{
  ora_result_set_t *ora_rs = (ora_result_set_t *)rs->ptr;
  
  /* Check if the results are already fetched */
  if (ora_rs != NULL)
    return ora_rs->nrows;

  return 0;
}


/* Store results from the last query */


int ora_drv_store_results(db_result_set_t *rs)
{
  unsigned int     i;
  sword            rc;
  db_stmt_t        *db_stmt = rs->statement;
  db_conn_t        *db_conn = rs->connection;
  ora_stmt_t       *ora_stmt;
  ora_conn_t       *ora_con;
  ora_result_set_t *ora_rs;
  ora_column_t     *column;
  ora_row_t        *row;
  OCIParam         *parm;
  void             *tmp = NULL;
  unsigned int     col_len;
  ub4              semantics;
  sb_list_item_t   *pos;
  text             *fnamep;
  
  if (db_stmt == NULL || db_conn == NULL)
    return 1;
  
  ora_stmt = (ora_stmt_t *)db_stmt->ptr;
  ora_con = (ora_conn_t *)db_conn->ptr;
  if (ora_stmt == NULL || ora_con == NULL)
    return 1;

  if (rs->ptr != NULL)
    return 1;
  ora_rs = (ora_result_set_t *)calloc(1, sizeof(ora_result_set_t));
  if (ora_rs == NULL)
    return 1;
  rs->ptr = ora_rs;
  SB_LIST_INIT(&ora_rs->columns);
  SB_LIST_INIT(&ora_rs->rows);

  i = 1;
  rc = OCIParamGet((dvoid *)ora_stmt->ptr, OCI_HTYPE_STMT, ora_con->errhp, 
                   (dvoid **)&tmp, i);
  parm = (OCIParam *)tmp;

  /* Loop to get description of all columns */
  while (rc == OCI_SUCCESS)
  {
    column = (ora_column_t *)calloc(1, sizeof(ora_column_t));
    if (column == NULL)
      goto error;
    SB_LIST_ADD_TAIL(&column->listitem, &ora_rs->columns);

    /* Get the column type attribute */
    rc = OCIAttrGet((dvoid *)parm, OCI_DTYPE_PARAM, (dvoid *)&column->type,
                    NULL, OCI_ATTR_DATA_TYPE, ora_con->errhp);
    CHECKERR("OCIAttrGet");

    /* Get the column name attribute */
    rc = OCIAttrGet((dvoid *)parm, OCI_DTYPE_PARAM, &fnamep,
                    (ub4 *)&col_len, OCI_ATTR_NAME, ora_con->errhp);
    CHECKERR("OCIAttrGet");
    column->name = (char *)malloc(col_len + 1);
    if (column->name == NULL)
      goto error;
    strncpy(column->name, fnamep, col_len + 1);
    

    /* Get the length semantics */
    rc = OCIAttrGet((dvoid *)parm, OCI_DTYPE_PARAM, (dvoid *)&semantics,
                    NULL, OCI_ATTR_CHAR_USED, ora_con->errhp);
    CHECKERR("OCIAttrGet");

    if (semantics)
    {
      /* Get the column width in characters */
      rc = OCIAttrGet((dvoid *)parm, OCI_DTYPE_PARAM, (dvoid *)&column->len,
                      NULL, OCI_ATTR_CHAR_SIZE, ora_con->errhp);
      if (column->len == 0)
        column->len = get_oracle_type_size(column->type);
    }
    else
    {
      /* Get the column width in bytes */
      rc = OCIAttrGet((dvoid *)parm, OCI_DTYPE_PARAM, (dvoid *)&column->len,
                      NULL, OCI_ATTR_DATA_SIZE, ora_con->errhp);
      if (column->len == 0)
        column->len = get_oracle_type_size(column->type);
    }
    CHECKERR("OCIAttrGet");

    OCIDescriptorFree(parm, OCI_DTYPE_PARAM);
    
    /* Describe the column */
    column->value = malloc(column->len);
    if (column->value == NULL)
      goto error;
    rc = OCIDefineByPos(ora_stmt->ptr, &column->defhp, ora_con->errhp, i,
                        column->value, column->len, column->type, &column->ind,
                        NULL, NULL, OCI_DEFAULT);
    CHECKERR("OCIDefineByPos");
                        
    i++;
    rc = OCIParamGet(ora_stmt->ptr, OCI_HTYPE_STMT, ora_con->errhp,
                     (dvoid **)&tmp, i);
    parm = (OCIParam *)tmp;
  }
  ora_rs->ncolumns = i-1;
  
  /* Now fetch the actual data */
  while(1)
  {
    rc = OCIStmtFetch2(ora_stmt->ptr, ora_con->errhp, 1, OCI_FETCH_NEXT, 0,
                      OCI_DEFAULT);
    if (rc == OCI_NO_DATA)
      break;
    CHECKERR("OCIStmtFetch");
    
    row = (ora_row_t *)calloc(1, sizeof(ora_row_t));
    if (row == NULL)
      goto error;
    row->data = (ora_data_t *)calloc(ora_rs->ncolumns, sizeof(ora_data_t));
    i = 0;
    SB_LIST_FOR_EACH(pos, &ora_rs->columns)
    {
      column = SB_LIST_ENTRY(pos, ora_column_t, listitem);
      row->data[i].value = (void *)malloc(column->len);
      if (row->data[i].value == NULL)
        goto error;
      memcpy(row->data[i].value, column->value, column->len);
      row->data[i].ind = column->ind;
      i++;
    }
    SB_LIST_ADD_TAIL(&row->listitem, &ora_rs->rows);
    ora_rs->nrows++;
  }
  
  return 0;
  
 error:
  
  return 1;
}


/* Free result set */


int ora_drv_free_results(db_result_set_t *rs)
{
  ora_result_set_t *ora_rs = (ora_result_set_t *)rs->ptr;
  ora_row_t        *row;
  ora_column_t     *column;
  sb_list_item_t   *cur;
  sb_list_item_t   *next;
  unsigned int     i;
  
  if (ora_rs == NULL)
    return 1;
  
  SB_LIST_FOR_EACH_SAFE(cur, next, &ora_rs->rows)
  {
    row = SB_LIST_ENTRY(cur, ora_row_t, listitem);

    if (row->data != NULL)
    {
      for (i = 0; i < ora_rs->ncolumns; i++)
      {
        if (row->data[i].value != NULL)
          free(row->data[i].value);
      }
      free(row->data);
    }

    SB_LIST_DELETE(cur);
    free(row);
  }
  
  SB_LIST_FOR_EACH_SAFE(cur, next, &ora_rs->columns)
  {
    column = SB_LIST_ENTRY(cur, ora_column_t, listitem);

    if (column->name != NULL)
      free(column->name);
    if (column->value != NULL)
      free(column->value);
      
    SB_LIST_DELETE(cur);
    free(column);
  }

  free(ora_rs);
  
  return 0;
}


/* Close prepared statement */


int ora_drv_close(db_stmt_t *stmt)
{
  ora_stmt_t *ora_stmt = stmt->ptr;

  if (ora_stmt == NULL)
    return 1;
  OCIHandleFree(stmt, OCI_HTYPE_STMT);

  return 0;
}


/* Uninitialize driver */


int ora_drv_done(void)
{
  sword rc;

  if (ora_env == NULL)
    return 1;

  rc = OCIHandleFree(ora_env, OCI_HTYPE_ENV);
  if (rc != OCI_SUCCESS)
  {
    log_text(LOG_FATAL, "OCIHandleFree failed");
    return 1;
  }

  return 0;
}


/* Get Oracle type, type length and indicator values from SysBench parameter */

sword get_oracle_bind_type(db_bind_t *param, ub2 *type, sb4 *len,
                           sb2 *ind)
{
  unsigned int i;

  for (i = 0; db_oracle_bind_map[i].db_type != DB_TYPE_NONE; i++)
    if (db_oracle_bind_map[i].db_type == param->type)
    {
      *type = db_oracle_bind_map[i].ora_type;
      *len = db_oracle_bind_map[i].ora_len;
      if (param->type == DB_TYPE_CHAR || param->type == DB_TYPE_VARCHAR)
        *len = strlen(param->buffer);
      *ind = (param->is_null) ? -1 : 0;
      
      return 0;
    }

  return 1;
}


/* Get Oracle type size in bytes */


sb4 get_oracle_type_size(sword type)
{
  unsigned int i;
  sb4          size = 0;

  if (type == SQLT_NUM)
    return 21;
  
  for (i = 0; db_oracle_bind_map[i].db_type != DB_TYPE_NONE; i++)
    if (db_oracle_bind_map[i].ora_type == type &&
        size < db_oracle_bind_map[i].ora_len)
      size = db_oracle_bind_map[i].ora_len;

  return size;
}

ora_stmt_type_t get_stmt_type(const char *query)
{
    if (!strncmp(query, "BEGIN", 5))
      return STMT_TYPE_BEGIN;
    else if (!strncmp(query, "COMMIT", 6))
      return STMT_TYPE_COMMIT;
    else if (!strncmp(query, "SELECT", 6))
      return STMT_TYPE_SELECT;

    return STMT_TYPE_UPDATE;
}

db_bind_type_t get_db_bind_type(sword type)
{
  unsigned int i;

  for (i = 0; db_oracle_bind_map[i].db_type != DB_TYPE_NONE; i++)
    if (db_oracle_bind_map[i].ora_type == type)
      return db_oracle_bind_map[i].db_type;

  return DB_TYPE_NONE;
}


/* Check and display Oracle error */


void checkerr(OCIError *errhp, sword status)
{
  text errbuf[512];
  sword errcode;

  switch (status)
  {
    case OCI_SUCCESS:
      break;
    case OCI_SUCCESS_WITH_INFO:
      log_text(LOG_ALERT, "Error - OCI_SUCCESS_WITH_INFO");
      break;
    case OCI_NEED_DATA:
      log_text(LOG_ALERT, "Error - OCI_NEED_DATA");
      break;
    case OCI_NO_DATA:
      log_text(LOG_ALERT, "Error - OCI_NO_DATA");
      break;
    case OCI_ERROR:
      OCIErrorGet((dvoid *) errhp, (ub4) 1,
                  (text *) NULL, (sb4 *) &errcode,
                  errbuf, (ub4) sizeof(errbuf),
                  (ub4) OCI_HTYPE_ERROR);
      log_text(LOG_ALERT, "Error - %s", errbuf);
      break;
    case OCI_INVALID_HANDLE:
      log_text(LOG_ALERT, "Error - OCI_INVALID_HANDLE");
      break;
    case OCI_STILL_EXECUTING:
      log_text(LOG_ALERT, "Error - OCI_STILL_EXECUTE");
      break;
    case OCI_CONTINUE:
      log_text(LOG_ALERT, "Error - OCI_CONTINUE");
      break;
    default:
      break;
  }
}
