/* Copyright (C) 2006 MySQL AB
   Copyright (C) 2006-2017 Alexey Kopytov <akopytov@gmail.com>

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

#include "sb_lua.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "db_driver.h"
#include "sb_rand.h"
#include "sb_thread.h"

#include "ck_pr.h"

/*
  Auto-generated headers for internal scripts. If you add a new header here,
  make sure it is also added to the internal_sources array below.
*/
#include "lua/internal/sysbench.lua.h"
#include "lua/internal/sysbench.rand.lua.h"
#include "lua/internal/sysbench.sql.lua.h"

#define EVENT_FUNC "event"
#define PREPARE_FUNC "prepare"
#define CLEANUP_FUNC "cleanup"
#define HELP_FUNC "help"
#define THREAD_INIT_FUNC "thread_init"
#define THREAD_DONE_FUNC "thread_done"
#define THREAD_RUN_FUNC "thread_run"
#define INIT_FUNC "init"
#define DONE_FUNC "done"

/* Interpreter context */

typedef struct {
  int       thread_id; /* sysbench thread id */
  db_conn_t *con;      /* Database connection */
} sb_lua_ctxt_t;

typedef struct {
  int            id;
  db_bind_type_t type;
  void           *buf;
  unsigned long  buflen;
  char           is_null;
} sb_lua_bind_t;

typedef struct {
  db_result_t *ptr;
} sb_lua_db_rs_t;

typedef struct {
  db_stmt_t      *ptr;
  sb_lua_bind_t  *params;
  unsigned int   nparams;
  sb_lua_bind_t  *results;
  unsigned int   nresults;
  int            param_ref;
  int            result_ref;
  sb_lua_db_rs_t *rs;
} sb_lua_db_stmt_t;

typedef struct {
  const char *name;
  const unsigned char *source;
  /* Use a pointer, since _len variables are not compile-time constants */
  unsigned int *source_len;
} internal_script_t;

typedef enum {
  SB_LUA_ERROR_NONE,
  SB_LUA_ERROR_RESTART_EVENT
} sb_lua_error_t;

/* Lua interpreter states */

static lua_State **states CK_CC_CACHELINE;

/* Event counter */
static unsigned int nevents CK_CC_CACHELINE;

static sb_test_t sbtest CK_CC_CACHELINE;

static const char *sb_lua_script_path CK_CC_CACHELINE;

static TLS sb_lua_ctxt_t *tls_lua_ctxt CK_CC_CACHELINE;

/* Database driver */
static TLS db_driver_t *db_driver;

/* List of pre-loaded internal scripts */
static internal_script_t internal_scripts[] = {
  {"sysbench.rand.lua", sysbench_rand_lua, &sysbench_rand_lua_len},
  {"sysbench.lua", sysbench_lua, &sysbench_lua_len},
  {"sysbench.sql.lua", sysbench_sql_lua, &sysbench_sql_lua_len},
  {NULL, NULL, 0}
};

/* Main (global) interpreter state */
static lua_State *gstate;

/* Lua test operations */

static int sb_lua_op_init(void);
static int sb_lua_op_done(void);
static sb_event_t sb_lua_next_event(int);
static int sb_lua_op_thread_init(int);
static int sb_lua_op_thread_run(int);
static int sb_lua_op_thread_done(int);
static void sb_lua_op_print_stats(sb_stat_t type);

static sb_operations_t lua_ops = {
   .init = sb_lua_op_init,
   .next_event = &sb_lua_next_event,
   .done = sb_lua_op_done
};

/* Lua test commands */
static int sb_lua_cmd_prepare(void);
static int sb_lua_cmd_cleanup(void);
static int sb_lua_cmd_help(void);

/* Initialize interpreter state */
static lua_State *sb_lua_new_state(int);

/* Close interpretet state */
static int sb_lua_close_state(lua_State *);

/* Exported C functions for legacy Lua API */
static int sb_lua_db_connect(lua_State *);
static int sb_lua_db_disconnect(lua_State *);
static int sb_lua_db_query(lua_State *);
static int sb_lua_db_bulk_insert_init(lua_State *);
static int sb_lua_db_bulk_insert_next(lua_State *);
static int sb_lua_db_bulk_insert_done(lua_State *);
static int sb_lua_db_prepare(lua_State *);
static int sb_lua_db_bind_param(lua_State *);
static int sb_lua_db_bind_result(lua_State *);
static int sb_lua_db_execute(lua_State *);
static int sb_lua_db_close(lua_State *);
static int sb_lua_db_store_results(lua_State *);
static int sb_lua_db_free_results(lua_State *);
static int sb_lua_more_events(lua_State *);
static int sb_lua_event_start(lua_State *);
static int sb_lua_event_stop(lua_State *);

static unsigned int sb_lua_table_size(lua_State *, int);

static void call_error(lua_State *L, const char *name)
{
  const char * const err = lua_tostring(L, -1);
  log_text(LOG_FATAL, "`%s' function failed: %s", name,
           err ? err : "(not a string)");
  lua_pop(L, 1);
}

static void check_connection(lua_State *L, sb_lua_ctxt_t *ctxt)
{
  if (ctxt->con == NULL)
    luaL_error(L, "Uninitialized database connection");
}

static bool func_available(lua_State *L, const char *func)
{
  lua_getglobal(L, func);
  bool rc = !lua_isnil(L, -1) && lua_type(L, -1) == LUA_TFUNCTION;
  lua_pop(L, 1);

  return rc;
}

/* Load a specified Lua script */

sb_test_t *sb_load_lua(const char *testname)
{
#ifdef DATA_PATH
  setenv("LUA_PATH", DATA_PATH LUA_DIRSEP "?.lua", 0);
#endif

  sb_lua_script_path = testname;

  /* Initialize global interpreter state */
  gstate = sb_lua_new_state(-1);
  if (gstate == NULL)
    goto error;

  /* Test commands */
  if (func_available(gstate, PREPARE_FUNC))
    sbtest.cmds.prepare = &sb_lua_cmd_prepare;

  if (func_available(gstate, CLEANUP_FUNC))
    sbtest.cmds.cleanup = &sb_lua_cmd_cleanup;

  if (func_available(gstate, HELP_FUNC))
    sbtest.cmds.help = &sb_lua_cmd_help;

  /* Test operations */
  sbtest.ops = lua_ops;

  sbtest.ops.thread_init = &sb_lua_op_thread_init;

  sbtest.ops.thread_done = &sb_lua_op_thread_done;

  if (func_available(gstate, THREAD_RUN_FUNC))
    sbtest.ops.thread_run = &sb_lua_op_thread_run;

  sbtest.ops.print_stats = &sb_lua_op_print_stats;

  /* Allocate per-thread interpreters array */
  states = (lua_State **)calloc(sb_globals.num_threads, sizeof(lua_State *));
  if (states == NULL)
    goto error;

  return &sbtest;

 error:

  sb_lua_close_state(gstate);
  if (states != NULL)
    free(states);

  return NULL;
}

/* Initialize Lua script */

int sb_lua_op_init(void)
{
  lua_getglobal(gstate, INIT_FUNC);
  if (!lua_isnil(gstate, -1))
  {
    if (lua_pcall(gstate, 0, 0, 0))
    {
      call_error(gstate, INIT_FUNC);
      return 1;
    }
  }

  if (!func_available(gstate, EVENT_FUNC))
  {
    log_text(LOG_FATAL, "cannot find the event() function in %s",
             sb_lua_script_path);
    return 1;
  }

  return 0;
}

sb_event_t sb_lua_next_event(int thread_id)
{
  sb_event_t req;

  (void) thread_id; /* unused */

  if (sb_globals.max_requests > 0 &&
      ck_pr_faa_uint(&nevents, 1) >= sb_globals.max_requests)
  {
    req.type = SB_REQ_TYPE_NULL;
    return req;
  }

  req.type = SB_REQ_TYPE_SCRIPT;

  return req;
}

int sb_lua_op_thread_init(int thread_id)
{
  lua_State * L;

  L = sb_lua_new_state(thread_id);
  if (L == NULL)
    return 1;

  states[thread_id] = L;

  lua_getglobal(L, THREAD_INIT_FUNC);
  if (!lua_isnil(L, -1))
  {
    lua_pushnumber(L, thread_id);

    if (lua_pcall(L, 1, 1, 0))
    {
      call_error(L, THREAD_INIT_FUNC);
      return 1;
    }
  }

  return 0;
}

int sb_lua_op_thread_run(int thread_id)
{
  lua_State * const L = states[thread_id];

  lua_getglobal(L, THREAD_RUN_FUNC);
  lua_pushnumber(L, thread_id);

  if (lua_pcall(L, 1, 1, 0))
  {
    call_error(L, THREAD_RUN_FUNC);
    return 1;
  }

  return 0;
}

int sb_lua_op_thread_done(int thread_id)
{
  lua_State * const L = states[thread_id];
  int rc = 0;

  lua_getglobal(L, THREAD_DONE_FUNC);
  if (!lua_isnil(L, -1))
  {
    lua_pushnumber(L, thread_id);

    if (lua_pcall(L, 1, 1, 0))
    {
      call_error(L, THREAD_DONE_FUNC);
      rc = 1;
    }
  }

  sb_lua_close_state(L);

  return rc;
}

void sb_lua_op_print_stats(sb_stat_t type)
{
  db_print_stats(type);
}

int sb_lua_op_done(void)
{
  if (states != NULL)
    free(states);

  lua_getglobal(gstate, DONE_FUNC);
  if (!lua_isnil(gstate, -1))
  {
    if (lua_pcall(gstate, 0, 0, 0))
    {
      call_error(gstate, DONE_FUNC);
      return 1;
    }
  }

  return 0;
}

/* Pre-load internal scripts */

static int load_internal_scripts(lua_State *L)
{
  for (internal_script_t *s = internal_scripts; s->name != NULL; s++)
  {
    if (luaL_loadbuffer(L, (const char *) s->source, s->source_len[0], s->name))
    {
      log_text(LOG_FATAL, "failed to load internal module '%s': %s",
               s->name, lua_tostring(L, -1));
      lua_pop(L, 1);
      return 1;
    }

    lua_call(L, 0, 0);
  }

  return 0;
}

/* Allocate and initialize new interpreter state */

lua_State *sb_lua_new_state(int thread_id)
{
  lua_State      *L;
  sb_list_item_t *pos;
  option_t       *opt;
  char           *tmp;

  L = luaL_newstate();
  
  luaL_openlibs(L);

  /* Export all global options */
  pos = sb_options_enum_start();
  while ((pos = sb_options_enum_next(pos, &opt)) != NULL)
  {
    switch (opt->type)
    {
      case SB_ARG_TYPE_FLAG:
        lua_pushboolean(L, sb_opt_to_flag(opt));
        break;
      case SB_ARG_TYPE_INT:
        lua_pushnumber(L, sb_opt_to_int(opt));
        break;
      case SB_ARG_TYPE_FLOAT:
        lua_pushnumber(L, sb_opt_to_float(opt));
        break;
      case SB_ARG_TYPE_SIZE:
        lua_pushnumber(L, sb_opt_to_size(opt));
        break;
      case SB_ARG_TYPE_STRING:
        tmp = sb_opt_to_string(opt);
        lua_pushstring(L, tmp ? tmp : "");
        break;
      case SB_ARG_TYPE_LIST:
        /*FIXME: should be exported as tables */
        lua_pushnil(L);
        break;
      case SB_ARG_TYPE_FILE:
        /* FIXME: no need to export anything */
        lua_pushnil(L);
        break;
      default:
        log_text(LOG_WARNING, "Global option '%s' will not be exported, because"
                 " the type is unknown", opt->name);
        lua_pushnil(L);
        break;
    }
    
    lua_setglobal(L, opt->name);
  }

#define SB_LUA_VAR(luaname, cname)              \
  do {                                          \
    lua_pushstring(L, luaname);                 \
    lua_pushnumber(L, cname);                   \
    lua_settable(L, -3);                        \
  } while (0)

#define SB_LUA_FUNC(luaname, cname)             \
  do {                                          \
    lua_pushstring(L, luaname);                 \
    lua_pushcfunction(L, cname);                \
    lua_settable(L, -3);                        \
  } while (0)

  /* Export variables into per-state 'sysbench' table */

  lua_newtable(L);

  /* sysbench.tid */
  if (thread_id >= 0)
    SB_LUA_VAR("tid", thread_id);

  /* Export functions into per-state 'sysbench' table  */

  SB_LUA_FUNC("more_events", sb_lua_more_events);
  SB_LUA_FUNC("event_start", sb_lua_event_start);
  SB_LUA_FUNC("event_stop", sb_lua_event_stop);

  /* Export functions into per-state 'sysbench.db' table  */

  lua_pushstring(L, "db");
  lua_newtable(L);

  SB_LUA_FUNC("connect", sb_lua_db_connect);
  SB_LUA_FUNC("disconnect", sb_lua_db_disconnect);
  SB_LUA_FUNC("query", sb_lua_db_query);
  SB_LUA_FUNC("bulk_insert_init", sb_lua_db_bulk_insert_init);
  SB_LUA_FUNC("bulk_insert_next", sb_lua_db_bulk_insert_next);
  SB_LUA_FUNC("bulk_insert_done", sb_lua_db_bulk_insert_done);
  SB_LUA_FUNC("prepare", sb_lua_db_prepare);
  SB_LUA_FUNC("bind_param", sb_lua_db_bind_param);
  SB_LUA_FUNC("bind_result", sb_lua_db_bind_result);
  SB_LUA_FUNC("execute", sb_lua_db_execute);
  SB_LUA_FUNC("close", sb_lua_db_close);
  SB_LUA_FUNC("store_results", sb_lua_db_store_results);
  SB_LUA_FUNC("free_results", sb_lua_db_free_results);

  SB_LUA_VAR("DB_ERROR_NONE", DB_ERROR_NONE);
  SB_LUA_VAR("DB_ERROR_RESTART_TRANSACTION", DB_ERROR_IGNORABLE);
  SB_LUA_VAR("DB_ERROR_FAILED", DB_ERROR_FATAL);

  lua_settable(L, -3); /* sysbench.db */

  lua_pushstring(L, "error");
  lua_newtable(L);

  SB_LUA_VAR("NONE", SB_LUA_ERROR_NONE);
  SB_LUA_VAR("RESTART_EVENT", SB_LUA_ERROR_RESTART_EVENT);

  lua_settable(L, -3); /* sysbench.error */

#undef SB_LUA_VAR
#undef SB_LUA_FUNC

  lua_setglobal(L, "sysbench");

  luaL_newmetatable(L, "sysbench.stmt");
  luaL_newmetatable(L, "sysbench.rs");

  if (load_internal_scripts(L))
    return NULL;

  int rc;

  if ((rc = luaL_loadfile(L, sb_lua_script_path)))
  {
    if (rc != LUA_ERRFILE)
    {
      lua_error(L);
      return NULL;
    }
#ifdef DATA_PATH
    /* first location failed - look in DATA_PATH */
    char p[PATH_MAX + 1];
    strncpy(p, DATA_PATH LUA_DIRSEP, sizeof(p));
    strncat(p, sb_lua_script_path, sizeof(p)-strlen(p)-1);
    if (!strrchr(sb_lua_script_path, '.'))
    {
      /* add .lua extension if there isn't one */
      strncat(p, ".lua", sizeof(p)-strlen(p)-1);
    }

    if (luaL_loadfile(L, p))
    {
      lua_error(L);
      return NULL;
    }
#else
    lua_error(L);
    return NULL;
#endif
  }

  if (lua_pcall(L, 0, 0, 0))
  {
    lua_error(L);
    return NULL;
  }

  /* Create new L context */
  tls_lua_ctxt = calloc(1, sizeof(sb_lua_ctxt_t));
  if (tls_lua_ctxt == NULL)
    return NULL;

  tls_lua_ctxt->thread_id = thread_id;

  return L;
}

/* Close interpreter state */

int sb_lua_close_state(lua_State *state)
{
  if (state != NULL)
    lua_close(state);

  if (db_driver != NULL)
  {
    db_destroy(db_driver);
    db_driver = NULL;
  }

  return 0;
}

/* Prepare command */

int sb_lua_cmd_prepare(void)
{
  lua_getglobal(gstate, PREPARE_FUNC);

  if (lua_pcall(gstate, 0, 1, 0) != 0)
  {
    log_text(LOG_FATAL, "failed to execute function `"PREPARE_FUNC"': %s",
             lua_tostring(gstate, -1));
    return 1;
  }

  return 0;
}

/* Cleanup command */

int sb_lua_cmd_cleanup(void)
{
  lua_getglobal(gstate, CLEANUP_FUNC);

  if (lua_pcall(gstate, 0, 1, 0) != 0)
  {
    log_text(LOG_FATAL, "failed to execute function `"CLEANUP_FUNC"': %s",
             lua_tostring(gstate, -1));
    return 1;
  }

  return 0;
}

/* Help command */

int sb_lua_cmd_help(void)
{
  lua_getglobal(gstate, HELP_FUNC);

  if (lua_pcall(gstate, 0, 1, 0) != 0)
  {
    log_text(LOG_FATAL, "failed to execute function `"HELP_FUNC"': %s",
             lua_tostring(gstate, -1));
    return 1;
  }

  return 0;
}


int sb_lua_db_connect(lua_State *L)
{
  db_driver = db_create(NULL);
  if (db_driver == NULL)
    luaL_error(L, "DB initialization failed");
  lua_pushstring(L, db_driver->sname);
  lua_setglobal(L, "db_driver");

  tls_lua_ctxt->con = db_connection_create(db_driver);
  if (tls_lua_ctxt->con == NULL)
    luaL_error(L, "Failed to connect to the database");
  db_set_thread(tls_lua_ctxt->con, tls_lua_ctxt->thread_id);

  return 0;
}

int sb_lua_db_disconnect(lua_State *L)
{
  (void) L; /* unused */

  if (tls_lua_ctxt->con)
  {
    db_connection_close(tls_lua_ctxt->con);
    db_connection_free(tls_lua_ctxt->con);
  }

  tls_lua_ctxt->con = NULL;

  return 0;
}

/*
  Throw an error with the { errcode = RESTART_EVENT } table. This will make
  thread_run() restart the event.
*/

static void throw_restart_event(lua_State *L)
{
  log_text(LOG_DEBUG, "Ignored error encountered, restarting transaction");

  lua_newtable(L);
  lua_pushstring(L, "errcode");
  lua_pushnumber(L, SB_LUA_ERROR_RESTART_EVENT);
  lua_settable(L, -3);

  lua_error(L); /* this call never returns */
}

int sb_lua_db_query(lua_State *L)
{
  const char        *query;
  db_result_t       *rs;
  size_t            len;

  if (tls_lua_ctxt->con == NULL)
    sb_lua_db_connect(L);

  db_conn_t * const con = tls_lua_ctxt->con;

  query = luaL_checklstring(L, 1, &len);
  rs = db_query(con, query, len);
  if (rs == NULL && con->error == DB_ERROR_IGNORABLE)
    throw_restart_event(L);

  if (rs != NULL)
    db_free_results(rs);

  return 0;
}

int sb_lua_db_bulk_insert_init(lua_State *L)
{
  const char *query;
  size_t len;

  if (tls_lua_ctxt->con == NULL)
    sb_lua_db_connect(L);

  query = luaL_checklstring(L, 1, &len);
  if (db_bulk_insert_init(tls_lua_ctxt->con, query, len))
    luaL_error(L, "db_bulk_insert_init() failed");

  return 0;
}

int sb_lua_db_bulk_insert_next(lua_State *L)
{
  const char *query;
  size_t len;

  check_connection(L, tls_lua_ctxt);

  query = luaL_checklstring(L, 1, &len);
  if (db_bulk_insert_next(tls_lua_ctxt->con, query, len))
    luaL_error(L, "db_bulk_insert_next() failed");

  return 0;
}

int sb_lua_db_bulk_insert_done(lua_State *L)
{
  check_connection(L, tls_lua_ctxt);

  db_bulk_insert_done(tls_lua_ctxt->con);

  return 0;
}

int sb_lua_db_prepare(lua_State *L)
{
  sb_lua_db_stmt_t *stmt;
  const char       *query;
  size_t           len;

  if (tls_lua_ctxt->con == NULL)
    sb_lua_db_connect(L);

  query = luaL_checklstring(L, 1, &len);

  stmt = (sb_lua_db_stmt_t *)lua_newuserdata(L, sizeof(sb_lua_db_stmt_t));
  luaL_getmetatable(L, "sysbench.stmt");
  lua_setmetatable(L, -2);
  memset(stmt, 0, sizeof(sb_lua_db_stmt_t));

  stmt->ptr = db_prepare(tls_lua_ctxt->con, query, len);
  if (stmt->ptr == NULL)
    luaL_error(L, "db_prepare() failed");

  stmt->param_ref = LUA_REFNIL;
  
  return 1;
}

int sb_lua_db_bind_param(lua_State *L)
{
  sb_lua_db_stmt_t *stmt;
  unsigned int     i, n;
  db_bind_t        *binds;
  char             needs_rebind = 0; 

  check_connection(L, tls_lua_ctxt);

  stmt = (sb_lua_db_stmt_t *)luaL_checkudata(L, 1, "sysbench.stmt");
  luaL_argcheck(L, stmt != NULL, 1, "prepared statement expected");

  if (!lua_istable(L, 2))
    luaL_error(L, "table was expected");
  /* Get table size */
  n = sb_lua_table_size(L, 2);
  if (!n)
    luaL_error(L, "table is empty");
  binds = (db_bind_t *)calloc(n, sizeof(db_bind_t));
  stmt->params = (sb_lua_bind_t *)calloc(n, sizeof(sb_lua_bind_t));
  if (binds == NULL || stmt->params == NULL)
    luaL_error(L, "memory allocation failure");

  lua_pushnil(L);
  for (i = 0; i < n; i++)
  {
    lua_next(L, 2);
    switch(lua_type(L, -1))
    {
      case LUA_TNUMBER:
        stmt->params[i].buf = malloc(sizeof(int));
        stmt->params[i].id = luaL_checknumber(L, -2);
        binds[i].type = DB_TYPE_INT;
        binds[i].buffer = stmt->params[i].buf;
        break;
      case LUA_TSTRING:
        stmt->params[i].id = luaL_checknumber(L, -2);
        stmt->params[i].buflen = 0;
        binds[i].type = DB_TYPE_CHAR;
        needs_rebind = 1;
        break;
      default:
        lua_pushfstring(L, "Unsupported variable type: %s",
                        lua_typename(L, lua_type(L, -1)));
        goto error;
    }
    binds[i].is_null = &stmt->params[i].is_null;
    stmt->params[i].type = binds[i].type;
    lua_pop(L, 1);
  }

  if (!needs_rebind && db_bind_param(stmt->ptr, binds, n))
    goto error;

  stmt->nparams = n;
  
  /* Create reference for the params table */
  lua_pushvalue(L, 2);
  stmt->param_ref = luaL_ref(L, LUA_REGISTRYINDEX);
  
  free(binds);
  
  return 0;

 error:

  free(binds);
  lua_error(L);
  
  return 0;
}

int sb_lua_db_bind_result(lua_State *L)
{
  sb_lua_db_stmt_t *stmt;
  unsigned int     i, n;
  db_bind_t        *binds;
  char             needs_rebind = 0; 

  check_connection(L, tls_lua_ctxt);

  stmt = (sb_lua_db_stmt_t *)luaL_checkudata(L, 1, "sysbench.stmt");
  luaL_argcheck(L, stmt != NULL, 1, "prepared statement expected");

  if (!lua_istable(L, 2))
    luaL_error(L, "table was expected");
  /* Get table size */
  n = sb_lua_table_size(L, 2);
  if (!n)
    luaL_error(L, "table is empty");
  binds = (db_bind_t *)calloc(n, sizeof(db_bind_t));
  stmt->results = (sb_lua_bind_t *)calloc(n, sizeof(sb_lua_bind_t));
  if (binds == NULL || stmt->results == NULL)
    luaL_error(L, "memory allocation failure");

  lua_pushnil(L);
  for (i = 0; i < n; i++)
  {
    lua_next(L, 2);
    switch(lua_type(L, -1))
    {
      case LUA_TNUMBER:
        stmt->results[i].buf = malloc(sizeof(int));
        stmt->results[i].id = luaL_checknumber(L, -2);
        binds[i].type = DB_TYPE_BIGINT;
        binds[i].buffer = stmt->results[i].buf;
        break;
      case LUA_TSTRING:
        stmt->results[i].id = luaL_checknumber(L, -2);
        binds[i].type = DB_TYPE_CHAR;
        needs_rebind = 1;
        break;
      default:
        lua_pushfstring(L, "Unsupported variable type: %s",
                        lua_typename(L, lua_type(L, -1)));
        goto error;
    }
    binds[i].is_null = &stmt->results[i].is_null;
    stmt->results[i].type = binds[i].type;
    lua_pop(L, 1);
  }

  if (!needs_rebind && db_bind_result(stmt->ptr, binds, n))
    goto error;

  stmt->nresults = n;
  
  /* Create reference for the params table */
  lua_pushvalue(L, 2);
  stmt->result_ref = luaL_ref(L, LUA_REGISTRYINDEX);
  
  //  free(binds);
  
  return 0;

 error:

  free(binds);
  lua_error(L);
  
  return 0;
}

int sb_lua_db_execute(lua_State *L)
{
  sb_lua_db_stmt_t *stmt;
  db_result_t      *ptr;
  sb_lua_db_rs_t   *rs;
  unsigned int     i;
  char             needs_rebind = 0;
  db_bind_t        *binds;
  size_t           length;
  const char       *str;
  sb_lua_bind_t    *param;

  check_connection(L, tls_lua_ctxt);

  stmt = (sb_lua_db_stmt_t *)luaL_checkudata(L, 1, "sysbench.stmt");
  luaL_argcheck(L, stmt != NULL, 1, "prepared statement expected");

  /* Get params table */
  lua_rawgeti(L, LUA_REGISTRYINDEX, stmt->param_ref);
  if (!lua_isnil(L, -1) && !lua_istable(L, -1))
    luaL_error(L, "table expected");

  for (i = 0; i < stmt->nparams; lua_pop(L, 1), i++)
  {
    param = stmt->params + i;
    lua_pushnumber(L, param->id);
    lua_gettable(L, -2);
    if (lua_isnil(L, -1))
    {
      param->is_null = 1;
      continue;
    }
    param->is_null = 0;
    switch (param->type)
    {
      case DB_TYPE_INT:
        *((int *)param->buf) = luaL_checknumber(L, -1);
        break;
      case DB_TYPE_CHAR:
        str = luaL_checkstring(L, -1);
        length = lua_objlen(L, -1);
        if (length > param->buflen)
        {
          param->buf = realloc(param->buf, length);
          needs_rebind = 1;
        }
        strncpy(param->buf, str, length);
        param->buflen = length;
        break;
      default:
        luaL_error(L, "Unsupported variable type: %s",
                   lua_typename(L, lua_type(L, -1)));
    }
  }
  
  /* Rebind if needed */
  if (needs_rebind)
  {
    binds = (db_bind_t *)calloc(stmt->nparams, sizeof(db_bind_t));
    if (binds == NULL)
      luaL_error(L, "Memory allocation failure");
    
    for (i = 0; i < stmt->nparams; i++)
    {
      param = stmt->params + i;
      binds[i].type = param->type;
      binds[i].is_null = &param->is_null;
      if (*binds[i].is_null != 0)
        continue;
      switch (param->type)
      {
        case DB_TYPE_INT:
          binds[i].buffer = param->buf;
          break;
        case DB_TYPE_CHAR:
          binds[i].buffer = param->buf;
          binds[i].data_len = &stmt->params[i].buflen;
          binds[i].is_null = 0;
          break;
        default:
          luaL_error(L, "Unsupported variable type");
      }
    }
    
    if (db_bind_param(stmt->ptr, binds, stmt->nparams))
      luaL_error(L, "db_bind_param() failed");
    free(binds);
  }

  ptr = db_execute(stmt->ptr);
  if (ptr == NULL && tls_lua_ctxt->con->error == DB_ERROR_IGNORABLE)
  {
    stmt->rs = NULL;
    throw_restart_event(L);
  }
  else
  {
    rs = (sb_lua_db_rs_t *)lua_newuserdata(L, sizeof(sb_lua_db_rs_t));
    rs->ptr = ptr;
    luaL_getmetatable(L, "sysbench.rs");
    lua_setmetatable(L, -2);
    stmt->rs = rs;
  }

  return 1;
}

int sb_lua_db_close(lua_State *L)
{
  sb_lua_db_stmt_t *stmt;
  unsigned int     i;

  check_connection(L, tls_lua_ctxt);

  stmt = (sb_lua_db_stmt_t *)luaL_checkudata(L, 1, "sysbench.stmt");
  luaL_argcheck(L, stmt != NULL, 1, "prepared statement expected");

  for (i = 0; i < stmt->nparams; i++)
  {
    if (stmt->params[i].buf != NULL)
      free(stmt->params[i].buf);
  }
  free(stmt->params);
  stmt->params = NULL;
  
  luaL_unref(L, LUA_REGISTRYINDEX, stmt->param_ref);
  luaL_unref(L, LUA_REGISTRYINDEX, stmt->result_ref);

  db_close(stmt->ptr);
  
  return 0;
}

int sb_lua_db_store_results(lua_State *L)
{
  sb_lua_db_rs_t   *rs;

  check_connection(L, tls_lua_ctxt);

  rs = (sb_lua_db_rs_t *)luaL_checkudata(L, 1, "sysbench.rs");
  luaL_argcheck(L, rs != NULL, 1, "result set expected");

  /* noop as db_store_results() is now performed automatically by db_query() */

  return 0;
}

int sb_lua_db_free_results(lua_State *L)
{
  sb_lua_db_rs_t   *rs;

  check_connection(L, tls_lua_ctxt);

  rs = (sb_lua_db_rs_t *)luaL_checkudata(L, 1, "sysbench.rs");
  luaL_argcheck(L, rs != NULL, 1, "result set expected");

  db_free_results(rs->ptr);
  rs->ptr = NULL;
  
  return 0;
}

unsigned int sb_lua_table_size(lua_State *L, int index)
{
  unsigned int i;

  lua_pushnil(L);
  for (i = 0; lua_next(L, index); i++)
  {
    lua_pop(L, 1);
  }

  return i;
}

/* Check if there are more events to execute */

int sb_lua_more_events(lua_State *L)
{
  sb_event_t    e;

  e = sb_next_event(&sbtest, tls_lua_ctxt->thread_id);
  lua_pushboolean(L, e.type == SB_REQ_TYPE_SCRIPT);

  return 1;
}

/* Exported wrapper for sb_event_start() */

int sb_lua_event_start(lua_State *L)
{
  (void) L; /* unused */

  sb_event_start(tls_lua_ctxt->thread_id);

  return 0;
}

/* Exported wrapper for sb_event_stop() */

int sb_lua_event_stop(lua_State *L)
{
  (void) L; /* unused */

  sb_event_stop(tls_lua_ctxt->thread_id);

  return 0;
}
