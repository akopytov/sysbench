/* Copyright (C) 2006 MySQL AB

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

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "sb_script.h"

#include "db_driver.h"

#define EVENT_FUNC "event"
#define PREPARE_FUNC "prepare"
#define CLEANUP_FUNC "cleanup"
#define THREAD_INIT_FUNC "thread_init"
#define THREAD_DONE_FUNC "thread_done"

/* Macros to call Lua functions */
#define CALL_ERROR(L, name)           \
  do {                                \
    log_text(LOG_FATAL, "failed to execute function `%s': %s", \
             name, lua_tostring(L, -1));                       \
  } while (0)

/* Interpreter context */

typedef struct {
  int       thread_id; /* SysBench thread ID */
  db_conn_t *con;      /* Database connection */
} sb_lua_ctxt_t;

typedef struct {
  int            id;
  db_bind_type_t type;
  void           *buf;
  unsigned long  *buflen;
  char           is_null;
} sb_lua_bind_t;

typedef struct {
  db_stmt_t     *ptr;
  sb_lua_bind_t *params;
  unsigned int  nparams;
  int           ref;
} sb_lua_db_stmt_t;

typedef struct {
  db_result_set_t *ptr;
} sb_lua_db_rs_t;

/* Lua interpreter states */

static lua_State **states;

/* Event counter */
static unsigned int nevents;

/* Lua test operations */

static int sb_lua_init(void);
static int sb_lua_done(void);
static sb_request_t sb_lua_get_request(void);
static int sb_lua_op_execute_request(sb_request_t *, int);
static int sb_lua_op_thread_init(int);
static int sb_lua_op_thread_done(int);
static void sb_lua_op_print_stats(void);

static sb_operations_t lua_ops = {
  .init = &sb_lua_init,
  .get_request = &sb_lua_get_request,
  .execute_request = &sb_lua_op_execute_request,
  .done = &sb_lua_done
};

/* Main (global) interpreter state */
static lua_State *gstate;

/* Database driver */
static db_driver_t *db_driver;

/* Variable with unique address to store per-state data */
static const char sb_lua_ctxt_key;

/* Lua test commands */
static int sb_lua_cmd_prepare(void);
static int sb_lua_cmd_cleanup(void);

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
static int sb_lua_db_execute(lua_State *);
static int sb_lua_db_close(lua_State *);
static int sb_lua_db_store_results(lua_State *);
static int sb_lua_db_free_results(lua_State *);
static int sb_lua_rand(lua_State *);
static int sb_lua_rand_uniq(lua_State *);
static int sb_lua_rnd(lua_State *);
static int sb_lua_rand_str(lua_State *);

/* Get a per-state interpreter context */
static sb_lua_ctxt_t *sb_lua_get_context(lua_State *);

/* Set a per-state interpreter context */
void sb_lua_set_context(lua_State *, sb_lua_ctxt_t *);

unsigned int sb_lua_table_size(lua_State *, int);

/* Load a specified Lua script */

int script_load_lua(const char *testname, sb_test_t *test)
{
  unsigned int i;

  /* Initialize global interpreter state */
  gstate = sb_lua_new_state(testname, -1);
  if (gstate == NULL)
    goto error;

  /* Test commands */
  lua_getglobal(gstate, PREPARE_FUNC);
  if (!lua_isnil(gstate, -1))
    test->cmds.prepare = &sb_lua_cmd_prepare;
  lua_pop(gstate, 1);

  lua_getglobal(gstate, CLEANUP_FUNC);
  if (!lua_isnil(gstate, -1))
    test->cmds.cleanup = &sb_lua_cmd_cleanup;

  /* Test operations */
  test->ops = lua_ops;

  lua_getglobal(gstate, THREAD_INIT_FUNC);
  if (!lua_isnil(gstate, -1))
    test->ops.thread_init = &sb_lua_op_thread_init;

  lua_getglobal(gstate, THREAD_DONE_FUNC);
  if (!lua_isnil(gstate, -1))
    test->ops.thread_done = &sb_lua_op_thread_done;

  test->ops.print_stats = &sb_lua_op_print_stats;
  
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
  
  return 0;

 error:

  sb_lua_close_state(gstate);
  if (states != NULL)
  {
    for (i = 0; i < sb_globals.num_threads; i++)
      sb_lua_close_state(states[i]);
    free(states);
  }
  
  return 1;
}

/* Initialize Lua script */

int sb_lua_init(void)
{
  return 0;
}

sb_request_t sb_lua_get_request(void)
{
  sb_request_t req;

  if (nevents >= sb_globals.max_requests)
  {
    req.type = SB_REQ_TYPE_NULL;
    return req;
  }

  req.type = SB_REQ_TYPE_SCRIPT;
  nevents++;
  
  return req;
}

int sb_lua_op_execute_request(sb_request_t *sb_req, int thread_id)
{
  double rc;
  log_msg_t           msg;
  log_msg_oper_t      op_msg;

  (void)sb_req; /* unused */
  
  /* Prepare log message */
  msg.type = LOG_MSG_TYPE_OPER;
  msg.data = &op_msg;
  
  lua_getglobal(states[thread_id], EVENT_FUNC);

  lua_pushnumber(states[thread_id], (double)thread_id);

  LOG_EVENT_START(msg, thread_id);

  if (lua_pcall(states[thread_id], 1, 1, 0))
  {
    CALL_ERROR(states[thread_id], EVENT_FUNC);
    sb_globals.error = 1;
    return 1;
  }

  LOG_EVENT_STOP(msg, thread_id);

  rc = lua_tonumber(states[thread_id], -1);
  lua_pop(states[thread_id], 1);
  
  return (int)rc;
}

int sb_lua_op_thread_init(int thread_id)
{
  sb_lua_ctxt_t *ctxt;

  ctxt = sb_lua_get_context(states[thread_id]);

  if (ctxt->con == NULL)
    sb_lua_db_connect(states[thread_id]);

  lua_getglobal(states[thread_id], THREAD_INIT_FUNC);
  lua_pushnumber(states[thread_id], (double)thread_id);
  
  if (lua_pcall(states[thread_id], 1, 1, 0))
  {
    CALL_ERROR(states[thread_id], THREAD_INIT_FUNC);
    return 1;
  }
  
  return 0;
}

int sb_lua_op_thread_done(int thread_id)
{
  lua_getglobal(states[thread_id], THREAD_DONE_FUNC);
  lua_pushnumber(states[thread_id], (double)thread_id);

  if (lua_pcall(states[thread_id], 1, 1, 0))
  {
    CALL_ERROR(states[thread_id], THREAD_DONE_FUNC);
    return 1;
  }
  
  return 0;
}

void sb_lua_op_print_stats(void)
{
  /* check if db driver has been initialized */
  if (db_driver != NULL)
    db_print_stats();
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
  lua_State      *state;
  sb_lua_ctxt_t  *ctxt;
  sb_list_item_t *pos;
  option_t       *opt;

  state = luaL_newstate();
  
  luaL_openlibs(state);
  luaopen_math(state);

  /* Export all global options */
  pos = sb_options_enum_start();
  while ((pos = sb_options_enum_next(pos, &opt)) != NULL)
  {
    switch (opt->type)
    {
      case SB_ARG_TYPE_FLAG:
        lua_pushboolean(state, sb_opt_to_flag(opt));
        break;
      case SB_ARG_TYPE_INT:
        lua_pushnumber(state, sb_opt_to_int(opt));
        break;
      case SB_ARG_TYPE_FLOAT:
        lua_pushnumber(state, sb_opt_to_float(opt));
        break;
      case SB_ARG_TYPE_SIZE:
        lua_pushnumber(state, sb_opt_to_size(opt));
        break;
      case SB_ARG_TYPE_STRING:
        lua_pushstring(state, sb_opt_to_string(opt));
        break;
      case SB_ARG_TYPE_LIST:
        /*FIXME: should be exported as tables */
        lua_pushnil(state);
        break;
      default:
        log_text(LOG_WARNING, "Global option '%s' will not be exported, because"
                 " the type is unknown", opt->name);
        break;
    }
    
    lua_setglobal(state, opt->name);
  }
  
  /* Export functions */
  lua_pushcfunction(state, sb_lua_rand);
  lua_setglobal(state, "sb_rand");

  lua_pushcfunction(state, sb_lua_rand_uniq);
  lua_setglobal(state, "sb_rand_uniq");

  lua_pushcfunction(state, sb_lua_rnd);
  lua_setglobal(state, "sb_rnd");

  lua_pushcfunction(state, sb_lua_rand_str);
  lua_setglobal(state, "sb_rand_str");
  
  lua_pushcfunction(state, sb_lua_db_connect);
  lua_setglobal(state, "db_connect");
  
  lua_pushcfunction(state, sb_lua_db_disconnect);
  lua_setglobal(state, "db_disconnect");

  lua_pushcfunction(state, sb_lua_db_query);
  lua_setglobal(state, "db_query");
  
  lua_pushcfunction(state, sb_lua_db_bulk_insert_init);
  lua_setglobal(state, "db_bulk_insert_init");
  
  lua_pushcfunction(state, sb_lua_db_bulk_insert_next);
  lua_setglobal(state, "db_bulk_insert_next");
  
  lua_pushcfunction(state, sb_lua_db_bulk_insert_done);
  lua_setglobal(state, "db_bulk_insert_done");

  lua_pushcfunction(state, sb_lua_db_prepare);
  lua_setglobal(state, "db_prepare");

  lua_pushcfunction(state, sb_lua_db_bind_param);
  lua_setglobal(state, "db_bind_param");

  lua_pushcfunction(state, sb_lua_db_execute);
  lua_setglobal(state, "db_execute");
  
  lua_pushcfunction(state, sb_lua_db_close);
  lua_setglobal(state, "db_close");

  lua_pushcfunction(state, sb_lua_db_store_results);
  lua_setglobal(state, "db_store_results");

  lua_pushcfunction(state, sb_lua_db_free_results);
  lua_setglobal(state, "db_free_results");
  
  luaL_newmetatable(state, "sysbench.stmt");
  luaL_newmetatable(state, "sysbench.rs");
  
  if (lua_dofile(state, scriptname))
  {
    lua_error(state);
    return NULL;
  }
    
  /* Create new state context */
  ctxt = (sb_lua_ctxt_t *)calloc(1, sizeof(sb_lua_ctxt_t));
  if (ctxt == NULL)
    return NULL;
  ctxt->thread_id = thread_id;
  sb_lua_set_context(state, ctxt);
  
  return state;
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

int sb_lua_db_connect(lua_State *L)
{
  sb_lua_ctxt_t *ctxt;
  
  ctxt = sb_lua_get_context(L);

  if (db_driver == NULL)
  {
    db_driver = db_init(NULL);
    if (db_driver == NULL)
      lua_error(L);
    lua_pushstring(L, db_driver->sname);
    lua_setglobal(L, "db_driver");
  }

  ctxt->con = db_connect(db_driver);
  if (ctxt->con == NULL)
    lua_error(L);
  db_set_thread(ctxt->con, ctxt->thread_id);
  
  return 0;
}

int sb_lua_db_disconnect(lua_State *L)
{
  sb_lua_ctxt_t *ctxt;
  
  ctxt = sb_lua_get_context(L);

  db_disconnect(ctxt->con);

  ctxt->con = NULL;
  
  return 0;
}

int sb_lua_db_query(lua_State *L)
{
  sb_lua_ctxt_t *ctxt;
  const char *query;

  ctxt = sb_lua_get_context(L);

  if (ctxt->con == NULL)
    sb_lua_db_connect(L);
  
  query = luaL_checkstring(L, 1);
  if (db_query(ctxt->con, query) == NULL)
    lua_error(L);
  
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
  {
    lua_pushstring(L, "db_bulk_insert_init() failed");
    lua_error(L);
  }
  
  return 0;
}

int sb_lua_db_bulk_insert_next(lua_State *L)
{
  sb_lua_ctxt_t *ctxt;
  const char *query;

  ctxt = sb_lua_get_context(L);

  if (ctxt->con == NULL)
    lua_error(L);
    
  query = luaL_checkstring(L, 1);
  if (db_bulk_insert_next(ctxt->con, query))
    lua_error(L);
  
  return 0;
}

int sb_lua_db_bulk_insert_done(lua_State *L)
{
  sb_lua_ctxt_t *ctxt;

  ctxt = sb_lua_get_context(L);

  if (ctxt->con == NULL)
    lua_error(L);
    
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
    lua_error(L);

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

  if (ctxt->con == NULL)
    lua_error(L);

  stmt = (sb_lua_db_stmt_t *)luaL_checkudata(L, 1, "sysbench.stmt");
  luaL_argcheck(L, stmt != NULL, 1, "prepared statement expected");

  if (!lua_istable(L, 2))
  {
    lua_pushstring(L, "table expected");
    lua_error(L);
  }
  /* Get table size */
  n = sb_lua_table_size(L, 2);
  if (!n)
  {
    lua_pushstring(L, "table is empty");
    lua_error(L);
  }
  binds = (db_bind_t *)calloc(n, sizeof(db_bind_t));
  stmt->params = (sb_lua_bind_t *)calloc(n, sizeof(sb_lua_bind_t));
  if (binds == NULL || stmt->params == NULL)
    lua_error(L);

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
        stmt->params[i].buflen = (unsigned long *)malloc(sizeof(unsigned long));
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
  stmt->ref = luaL_ref(L, LUA_REGISTRYINDEX);
  
  free(binds);
  
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
  sb_lua_db_rs_t   *rs;
  unsigned int     i;
  char             needs_rebind = 0;
  db_bind_t        *binds;

  ctxt = sb_lua_get_context(L);

  if (ctxt->con == NULL)
    lua_error(L);

  stmt = (sb_lua_db_stmt_t *)luaL_checkudata(L, 1, "sysbench.stmt");
  luaL_argcheck(L, stmt != NULL, 1, "prepared statement expected");

  /* Get params table */
  lua_rawgeti(L, LUA_REGISTRYINDEX, stmt->ref);
  if (!lua_isnil(L, -1) && !lua_istable(L, -1))
  {
    lua_pushstring(L, "table expected");
    lua_error(L);
  }

  for (i = 0; i < stmt->nparams; lua_pop(L, 1), i++)
  {
    lua_pushnumber(L, stmt->params[i].id);
    lua_gettable(L, -2);
    if (lua_isnil(L, -1))
    {
      stmt->params[i].is_null = 1;
      continue;
    }
    stmt->params[i].is_null = 0;
    switch (stmt->params[i].type)
    {
      case DB_TYPE_INT:
        *((int *)stmt->params[i].buf) = luaL_checknumber(L, -1);
        break;
      case DB_TYPE_CHAR:
        stmt->params[i].buf = strdup(luaL_checkstring(L, -1));
        needs_rebind = 1;
        break;
      default:
        lua_pushfstring(L, "Unsupported variable type: %s",
                        lua_typename(L, lua_type(L, -1)));
        lua_error(L);
    }
  }
  
  /* Rebind if needed */
  if (needs_rebind)
  {
    binds = (db_bind_t *)calloc(stmt->nparams, sizeof(db_bind_t));
    if (binds == NULL)
      lua_error(L);
    
    for (i = 0; i < stmt->nparams; i++)
    {
      binds[i].type = stmt->params[i].type;
      binds[i].is_null = &stmt->params[i].is_null;
      if (*binds[i].is_null != 0)
        continue;
      switch (stmt->params[i].type)
      {
        case DB_TYPE_INT:
          binds[i].buffer = stmt->params[i].buf;
          break;
        case DB_TYPE_CHAR:
          binds[i].buffer = stmt->params[i].buf;
          binds[i].data_len = stmt->params[i].buflen;
          *stmt->params[i].buflen = strlen(stmt->params[i].buf);
          binds[i].max_len = *stmt->params[i].buflen;
          binds[i].is_null = 0;
          break;
        default:
          lua_pushstring(L, "Unsupported variable type");
          lua_error(L);
      }
    }
    
    if (db_bind_param(stmt->ptr, binds, stmt->nparams))
      lua_error(L);
    free(binds);
  }
  
  rs = (sb_lua_db_rs_t *)lua_newuserdata(L, sizeof(sb_lua_db_rs_t));
  luaL_getmetatable(L, "sysbench.rs");
  lua_setmetatable(L, -2);

  rs->ptr = db_execute(stmt->ptr);
  if (rs->ptr == NULL)
    lua_error(L);

  return 1;
}

int sb_lua_db_close(lua_State *L)
{
  sb_lua_ctxt_t    *ctxt;
  sb_lua_db_stmt_t *stmt;
  unsigned int     i;
  
  ctxt = sb_lua_get_context(L);

  if (ctxt->con == NULL)
    lua_error(L);

  stmt = (sb_lua_db_stmt_t *)luaL_checkudata(L, 1, "sysbench.stmt");
  luaL_argcheck(L, stmt != NULL, 1, "prepared statement expected");

  for (i = 0; i < stmt->nparams; i++)
  {
    if (stmt->params[i].buf != NULL)
      free(stmt->params[i].buf);
    if (stmt->params[i].buflen != NULL)
      free(stmt->params[i].buflen);
  }
  free(stmt->params);
  stmt->params = NULL;
  
  luaL_unref(L, LUA_REGISTRYINDEX, stmt->ref);
  
  return 0;
}

int sb_lua_db_store_results(lua_State *L)
{
  sb_lua_ctxt_t    *ctxt;
  sb_lua_db_rs_t   *rs;

  ctxt = sb_lua_get_context(L);

  if (ctxt->con == NULL)
    lua_error(L);

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

  if (ctxt->con == NULL)
    lua_error(L);

  rs = (sb_lua_db_rs_t *)luaL_checkudata(L, 1, "sysbench.rs");
  luaL_argcheck(L, rs != NULL, 1, "result set expected");

  db_free_results(rs->ptr);
  
  return 0;
}

int sb_lua_rand(lua_State *L)
{
  int a, b;

  a = luaL_checknumber(L, 1);
  b = luaL_checknumber(L, 2);

  lua_pushnumber(L, sb_rand(a, b));

  return 1;
}

int sb_lua_rand_uniq(lua_State *L)
{
  int a, b;

  a = luaL_checknumber(L, 1);
  b = luaL_checknumber(L, 2);

  lua_pushnumber(L, sb_rand_uniq(a, b));

  return 1;
}

int sb_lua_rnd(lua_State *L)
{
  lua_pushnumber(L, sb_rnd());

  return 1;
}

int sb_lua_rand_str(lua_State *L)
{
  const char *fmt = luaL_checkstring(L, -1);
  char *buf = strdup(fmt);

  sb_rand_str(fmt, buf);
  lua_pushstring(L, buf);

  free(buf);

  return 1;
}

/* Get a per-state interpreter context */

sb_lua_ctxt_t *sb_lua_get_context(lua_State *L)
{
  lua_pushlightuserdata(L, (void *)&sb_lua_ctxt_key);
  lua_gettable(L, LUA_REGISTRYINDEX);

  return (sb_lua_ctxt_t *)lua_touserdata(L, -1);
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
