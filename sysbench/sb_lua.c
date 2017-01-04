/* Copyright (C) 2006 MySQL AB
   Copyright (C) 2006-2016 Alexey Kopytov <akopytov@gmail.com>

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
/* Auto-generated headers */
#include "lua/internal/sysbench.lua.h"

#define EVENT_FUNC "event"
#define PREPARE_FUNC "prepare"
#define CLEANUP_FUNC "cleanup"
#define HELP_FUNC "help"
#define THREAD_INIT_FUNC "thread_init"
#define THREAD_DONE_FUNC "thread_done"
#define THREAD_RUN_FUNC "thread_run"

/* Macros to call Lua functions */
#define CALL_ERROR(L, name)           \
  do { \
    const char *err = lua_tostring(L, -1); \
    log_text(LOG_FATAL, "failed to execute function `%s': %s",   \
             name, err ? err : "(null)");                               \
  } while (0)

#define CHECK_CONNECTION(L, ctxt) \
  do { \
    if (ctxt->con == NULL) \
      luaL_error(L, "Uninitialized database connection"); \
  } while(0);

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
  db_result_set_t *ptr;
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

/* Lua interpreter states */

static lua_State **states;

/* Event counter */
static unsigned int nevents;

static sb_test_t sbtest;

/* Lua test operations */

static int sb_lua_init(void);
static int sb_lua_done(void);
static sb_event_t sb_lua_next_event(int);
static int sb_lua_op_execute_event(sb_event_t *, int);
static int sb_lua_op_thread_init(int);
static int sb_lua_op_thread_run(int);
static int sb_lua_op_thread_done(int);
static void sb_lua_op_print_stats(sb_stat_t type);

static sb_operations_t lua_ops = {
   .init = sb_lua_init,
   .next_event = &sb_lua_next_event,
   .execute_event = sb_lua_op_execute_event,
   .done = sb_lua_done
};

/* Main (global) interpreter state */
static lua_State *gstate;

/* Database driver */
static db_driver_t *db_driver;

/* Variable with unique address to store per-state data */
static const char sb_lua_ctxt_key = 0;

/* Control variable for sb_lua_db_init_once() */
static pthread_once_t db_init_control = PTHREAD_ONCE_INIT;

/* Lua test commands */
static int sb_lua_cmd_prepare(void);
static int sb_lua_cmd_cleanup(void);
static int sb_lua_cmd_help(void);

/* Initialize interpreter state */
static lua_State *sb_lua_new_state(const char *, int);

/* Close interpretet state */
static int sb_lua_close_state(lua_State *);

/* Exported C functions */
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

/* Get a per-state interpreter context */
static sb_lua_ctxt_t *sb_lua_get_context(lua_State *);

/* Set a per-state interpreter context */
void sb_lua_set_context(lua_State *, sb_lua_ctxt_t *);

unsigned int sb_lua_table_size(lua_State *, int);

/* Load a specified Lua script */

sb_test_t *sb_load_lua(const char *testname)
{
  unsigned int i;

#ifdef DATA_PATH
  setenv("LUA_PATH", DATA_PATH LUA_DIRSEP "?.lua", 0);
#endif

  /* Initialize global interpreter state */
  gstate = sb_lua_new_state(testname, -1);
  if (gstate == NULL)
    goto error;

  /* Test commands */
  lua_getglobal(gstate, PREPARE_FUNC);
  if (!lua_isnil(gstate, -1))
    sbtest.cmds.prepare = &sb_lua_cmd_prepare;
  lua_pop(gstate, 1);

  lua_getglobal(gstate, CLEANUP_FUNC);
  if (!lua_isnil(gstate, -1))
    sbtest.cmds.cleanup = &sb_lua_cmd_cleanup;

  lua_getglobal(gstate, HELP_FUNC);
  if (!lua_isnil(gstate, -1) && lua_isfunction(gstate, -1))
    sbtest.cmds.help = &sb_lua_cmd_help;

  /* Test operations */
  sbtest.ops = lua_ops;

  lua_getglobal(gstate, THREAD_INIT_FUNC);
  if (!lua_isnil(gstate, -1))
    sbtest.ops.thread_init = &sb_lua_op_thread_init;

  lua_getglobal(gstate, THREAD_DONE_FUNC);
  if (!lua_isnil(gstate, -1))
    sbtest.ops.thread_done = &sb_lua_op_thread_done;

  lua_getglobal(gstate, THREAD_RUN_FUNC);
  if (!lua_isnil(gstate, -1))
    sbtest.ops.thread_run = &sb_lua_op_thread_run;


  sbtest.ops.print_stats = &sb_lua_op_print_stats;
  
  /* Initialize per-thread interpreters */
  states = (lua_State **)calloc(sb_globals.num_threads, sizeof(lua_State *));
  if (states == NULL)
    goto error;
  for (i = 0; i < sb_globals.num_threads; i++)
  {
    states[i] = sb_lua_new_state(testname, i);
    if (states[i] == NULL)
      goto error;
  }
  
  return &sbtest;

 error:

  sb_lua_close_state(gstate);
  if (states != NULL)
  {
    for (i = 0; i < sb_globals.num_threads; i++)
      sb_lua_close_state(states[i]);
    free(states);
  }
  
  return NULL;
}

/* Initialize Lua script */

int sb_lua_init(void)
{
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

int sb_lua_op_execute_event(sb_event_t *sb_req, int thread_id)
{
  unsigned int        restart;
  lua_State           *L = states[thread_id];

  (void)sb_req; /* unused */
  (void)thread_id; /* unused */

  do
  {
    restart = 0;

    lua_getglobal(L, EVENT_FUNC);

    if (lua_pcall(L, 0, 1, 0))
    {
      if (lua_gettop(L) && lua_isnumber(L, -1) &&
          lua_tonumber(L, -1) == SB_DB_ERROR_RESTART_TRANSACTION)
      {
        log_text(LOG_DEBUG,
                 "Ignored error encountered, restarting transaction");
        restart = 1;
      }
      else
      {
        CALL_ERROR(L, EVENT_FUNC);
        sb_globals.error = 1;
        return 1;
        
      }
    }
    lua_pop(L, 1);
  } while (restart);

  return 0;
}

int sb_lua_op_thread_init(int thread_id)
{
  sb_lua_ctxt_t     *ctxt;
  lua_State * const L = states[thread_id];

  ctxt = sb_lua_get_context(L);

  if (ctxt->con == NULL)
    sb_lua_db_connect(L);

  lua_getglobal(L, THREAD_INIT_FUNC);

  if (lua_pcall(L, 0, 1, 0))
  {
    CALL_ERROR(L, THREAD_INIT_FUNC);
    return 1;
  }

  return 0;
}

int sb_lua_op_thread_run(int thread_id)
{
  lua_State * const L = states[thread_id];

  lua_getglobal(L, THREAD_RUN_FUNC);

  if (lua_pcall(L, 0, 1, 0))
  {
    CALL_ERROR(L, THREAD_RUN_FUNC);
    return 1;
  }

  return 0;
}

int sb_lua_op_thread_done(int thread_id)
{
  lua_State * const L = states[thread_id];

  lua_getglobal(L, THREAD_DONE_FUNC);

  if (lua_pcall(L, 0, 1, 0))
  {
    CALL_ERROR(L, THREAD_DONE_FUNC);
    return 1;
  }
  
  return 0;
}

void sb_lua_op_print_stats(sb_stat_t type)
{
  /* check if db driver has been initialized */
  if (db_driver != NULL)
    db_print_stats(type);
}

int sb_lua_done(void)
{
  unsigned int i;

  for (i = 0; i < sb_globals.num_threads; i++)
    lua_close(states[i]);

  free(states);
  
  return 0;
}

/* Allocate and initialize new interpreter state */

lua_State *sb_lua_new_state(const char *scriptname, int thread_id)
{
  lua_State      *L;
  sb_lua_ctxt_t  *ctxt;
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

#define SB_LUA_VAR(luaname, cname)               \
  lua_pushstring(L, luaname);                \
  lua_pushnumber(L, cname);                  \
  lua_settable(L, -3)
#define SB_LUA_FUNC(luaname, cname)              \
  lua_pushstring(L, luaname);                \
  lua_pushcfunction(L, cname);               \
  lua_settable(L, -3)

  /* Export variables into per-L 'sysbench' table */

  lua_newtable(L);

  /* sysbench.tid */
  SB_LUA_VAR("tid", thread_id);

  /* Export functions into per-L 'sysbench' table  */

  SB_LUA_FUNC("more_events", sb_lua_more_events);
  SB_LUA_FUNC("event_start", sb_lua_event_start);
  SB_LUA_FUNC("event_stop", sb_lua_event_stop);

  /* Export functions into per-L 'sysbench.db' table  */

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

  SB_LUA_VAR("DB_ERROR_NONE", SB_DB_ERROR_NONE);
  SB_LUA_VAR("DB_ERROR_RESTART_TRANSACTION", SB_DB_ERROR_RESTART_TRANSACTION);
  SB_LUA_VAR("DB_ERROR_FAILED", SB_DB_ERROR_FAILED);

#undef SB_LUA_VAR
#undef SB_LUA_FUNC

  lua_settable(L, -3); /* sysbench.db */

  lua_setglobal(L, "sysbench");

  luaL_newmetatable(L, "sysbench.stmt");
  luaL_newmetatable(L, "sysbench.rs");

  /* Pre-load internal modules */
  if (luaL_loadbuffer(L, (const char *) sysbench_lua, sysbench_lua_len,
                      "sysbench.lua"))
  {
    log_text(LOG_FATAL, "failed to load internal module sysbench.lua: %s",
             lua_tostring(L, -1));
  }
  lua_pushstring(L, "sysbench");
  lua_call(L, 1, 0);

  if (luaL_loadfile(L, scriptname))
  {
#ifdef DATA_PATH
    /* first location failed - look in DATA_PATH */
    char p[PATH_MAX + 1];
    strncpy(p, DATA_PATH LUA_DIRSEP, sizeof(p));
    strncat(p, scriptname, sizeof(p)-strlen(p)-1);
    if (!strrchr(scriptname, '.'))
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
    return NULL;
#endif
  }

  if (lua_pcall(L, 0, 0, 0))
  {
    lua_error(L);
    return NULL;
  }
    
  /* Create new L context */
  ctxt = (sb_lua_ctxt_t *)calloc(1, sizeof(sb_lua_ctxt_t));
  if (ctxt == NULL)
    return NULL;

  ctxt->thread_id = thread_id;

  sb_lua_set_context(L, ctxt);
  
  return L;
}

/* Close interpreter state */

int sb_lua_close_state(lua_State *state)
{
  if (state != NULL)
    lua_close(state);

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


/* init_routine for pthread_once() */


static void sb_lua_db_init_once(void)
{
  if (db_driver == NULL)
    db_driver = db_init(NULL);
}

int sb_lua_db_connect(lua_State *L)
{
  sb_lua_ctxt_t *ctxt;
  
  ctxt = sb_lua_get_context(L);

  /* Initialize the DB driver once for all threads */
  pthread_once(&db_init_control, sb_lua_db_init_once);
  if (db_driver == NULL)
    luaL_error(L, "DB initialization failed");
  lua_pushstring(L, db_driver->sname);
  lua_setglobal(L, "db_driver");
  
  ctxt->con = db_connect(db_driver);
  if (ctxt->con == NULL)
    luaL_error(L, "Failed to connect to the database");
  db_set_thread(ctxt->con, ctxt->thread_id);

  return 0;
}

int sb_lua_db_disconnect(lua_State *L)
{
  sb_lua_ctxt_t *ctxt;
  
  ctxt = sb_lua_get_context(L);

  if (ctxt->con)
    db_disconnect(ctxt->con);

  ctxt->con = NULL;
  
  return 0;
}

int sb_lua_db_query(lua_State *L)
{
  sb_lua_ctxt_t *ctxt;
  const char *query;
  db_result_set_t *rs;

  ctxt = sb_lua_get_context(L);

  if (ctxt->con == NULL)
    sb_lua_db_connect(L);
  
  query = luaL_checkstring(L, 1);
  rs = db_query(ctxt->con, query);
  if (rs == NULL)
  {
    lua_pushnumber(L, ctxt->con->db_errno);
    lua_error(L);
  }

  db_store_results(rs);
  db_free_results(rs);
  
  return 0;
}

int sb_lua_db_bulk_insert_init(lua_State *L)
{
  sb_lua_ctxt_t *ctxt;
  const char *query;

  ctxt = sb_lua_get_context(L);

  if (ctxt->con == NULL)
    sb_lua_db_connect(L);
    
  query = luaL_checkstring(L, 1);
  if (db_bulk_insert_init(ctxt->con, query))
    luaL_error(L, "db_bulk_insert_init() failed");
  
  return 0;
}

int sb_lua_db_bulk_insert_next(lua_State *L)
{
  sb_lua_ctxt_t *ctxt;
  const char *query;

  ctxt = sb_lua_get_context(L);

  CHECK_CONNECTION(L, ctxt);
    
  query = luaL_checkstring(L, 1);
  if (db_bulk_insert_next(ctxt->con, query))
    luaL_error(L, "db_bulk_insert_next() failed");
  
  return 0;
}

int sb_lua_db_bulk_insert_done(lua_State *L)
{
  sb_lua_ctxt_t *ctxt;

  ctxt = sb_lua_get_context(L);

  CHECK_CONNECTION(L, ctxt);
    
  db_bulk_insert_done(ctxt->con);
  
  return 0;
}

int sb_lua_db_prepare(lua_State *L)
{
  sb_lua_ctxt_t *ctxt;
  sb_lua_db_stmt_t *stmt;
  const char *query;

  ctxt = sb_lua_get_context(L);

  if (ctxt->con == NULL)
    sb_lua_db_connect(L);

  query = luaL_checkstring(L, 1);
  
  stmt = (sb_lua_db_stmt_t *)lua_newuserdata(L, sizeof(sb_lua_db_stmt_t));
  luaL_getmetatable(L, "sysbench.stmt");
  lua_setmetatable(L, -2);
  memset(stmt, 0, sizeof(sb_lua_db_stmt_t));
  
  stmt->ptr = db_prepare(ctxt->con, query);
  if (stmt->ptr == NULL)
    luaL_error(L, "db_prepare() failed");

  stmt->param_ref = LUA_REFNIL;
  
  return 1;
}

int sb_lua_db_bind_param(lua_State *L)
{
  sb_lua_ctxt_t    *ctxt;
  sb_lua_db_stmt_t *stmt;
  unsigned int     i, n;
  db_bind_t        *binds;
  char             needs_rebind = 0; 

  ctxt = sb_lua_get_context(L);

  CHECK_CONNECTION(L, ctxt);

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
  sb_lua_ctxt_t    *ctxt;
  sb_lua_db_stmt_t *stmt;
  unsigned int     i, n;
  db_bind_t        *binds;
  char             needs_rebind = 0; 

  ctxt = sb_lua_get_context(L);

  CHECK_CONNECTION(L, ctxt);

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
  sb_lua_ctxt_t    *ctxt;
  sb_lua_db_stmt_t *stmt;
  db_result_set_t  *ptr;
  sb_lua_db_rs_t   *rs;
  unsigned int     i;
  char             needs_rebind = 0;
  db_bind_t        *binds;
  size_t           length;
  const char       *str;
  sb_lua_bind_t    *param;

  ctxt = sb_lua_get_context(L);

  CHECK_CONNECTION(L, ctxt);

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
  if (ptr == NULL)
  {
    stmt->rs = NULL;
    lua_pushnumber(L, ctxt->con->db_errno);
    lua_error(L);
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
  sb_lua_ctxt_t    *ctxt;
  sb_lua_db_stmt_t *stmt;
  unsigned int     i;
  
  ctxt = sb_lua_get_context(L);

  CHECK_CONNECTION(L, ctxt);

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
  sb_lua_ctxt_t    *ctxt;
  sb_lua_db_rs_t   *rs;

  ctxt = sb_lua_get_context(L);

  CHECK_CONNECTION(L, ctxt);

  rs = (sb_lua_db_rs_t *)luaL_checkudata(L, 1, "sysbench.rs");
  luaL_argcheck(L, rs != NULL, 1, "result set expected");

  db_store_results(rs->ptr);
  
  return 0;
}

int sb_lua_db_free_results(lua_State *L)
{
  sb_lua_ctxt_t    *ctxt;
  sb_lua_db_rs_t   *rs;

  ctxt = sb_lua_get_context(L);

  CHECK_CONNECTION(L, ctxt);

  rs = (sb_lua_db_rs_t *)luaL_checkudata(L, 1, "sysbench.rs");
  luaL_argcheck(L, rs != NULL, 1, "result set expected");

  db_free_results(rs->ptr);
  rs->ptr = NULL;
  
  return 0;
}

/* Get a per-state interpreter context */

sb_lua_ctxt_t *sb_lua_get_context(lua_State *L)
{
  sb_lua_ctxt_t *ctxt;

  lua_pushlightuserdata(L, (void *)&sb_lua_ctxt_key);
  lua_gettable(L, LUA_REGISTRYINDEX);

  ctxt = (sb_lua_ctxt_t *)lua_touserdata(L, -1);

  if (ctxt == NULL)
    luaL_error(L, "Attempt to access database driver before it is initialized. "
               "Check your script for syntax errors");

  return ctxt;
}

/* Set a per-state interpreter context */

void sb_lua_set_context(lua_State *L, sb_lua_ctxt_t *ctxt)
{
  lua_pushlightuserdata(L, (void *)&sb_lua_ctxt_key);
  lua_pushlightuserdata(L, (void *)ctxt);
  lua_settable(L, LUA_REGISTRYINDEX);
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
  sb_lua_ctxt_t *ctxt;

  ctxt = sb_lua_get_context(L);

  e = sb_next_event(&sbtest, ctxt->thread_id);
  lua_pushboolean(L, e.type == SB_REQ_TYPE_SCRIPT);

  return 1;
}

/* Exported wrapper for sb_event_start() */

int sb_lua_event_start(lua_State *L)
{
  sb_lua_ctxt_t *ctxt;

  ctxt = sb_lua_get_context(L);

  sb_event_start(ctxt->thread_id);

  return 0;
}

/* Exported wrapper for sb_event_stop() */

int sb_lua_event_stop(lua_State *L)
{
  sb_lua_ctxt_t *ctxt;

  ctxt = sb_lua_get_context(L);

  sb_event_stop(ctxt->thread_id);

  return 0;
}
