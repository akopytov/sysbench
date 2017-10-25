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

#ifdef HAVE_LIBGEN_H
# include <libgen.h>
#endif

#include "sb_lua.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "db_driver.h"
#include "sb_rand.h"
#include "sb_thread.h"

#include "sb_ck_pr.h"

/*
  Auto-generated headers for internal scripts. If you add a new header here,
  make sure it is also added to the internal_scripts array below.
*/
#include "lua/internal/sysbench.lua.h"
#include "lua/internal/sysbench.cmdline.lua.h"
#include "lua/internal/sysbench.compat.lua.h"
#include "lua/internal/sysbench.rand.lua.h"
#include "lua/internal/sysbench.sql.lua.h"
#include "lua/internal/sysbench.histogram.lua.h"

#define EVENT_FUNC "event"
#define PREPARE_FUNC "prepare"
#define CLEANUP_FUNC "cleanup"
#define HELP_FUNC "help"
#define THREAD_INIT_FUNC "thread_init"
#define THREAD_DONE_FUNC "thread_done"
#define THREAD_RUN_FUNC "thread_run"
#define INIT_FUNC "init"
#define DONE_FUNC "done"
#define REPORT_INTERMEDIATE_HOOK "report_intermediate"
#define REPORT_CUMULATIVE_HOOK "report_cumulative"

#define xfree(ptr) ({ if ((ptr) != NULL) free((void *) ptr); ptr = NULL; })

/* Interpreter context */

typedef struct {
  db_conn_t *con;      /* Database connection */
  db_driver_t *driver;
  lua_State *L;
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

bool sb_lua_more_events(int);
int sb_lua_set_test_args(sb_arg_t *, size_t);

/* Lua interpreter states */

static lua_State **states CK_CC_CACHELINE;

static sb_test_t sbtest CK_CC_CACHELINE;

static TLS sb_lua_ctxt_t tls_lua_ctxt CK_CC_CACHELINE;

/* List of pre-loaded internal scripts */
static internal_script_t internal_scripts[] = {
  {"sysbench.rand.lua", sysbench_rand_lua, &sysbench_rand_lua_len},
  {"sysbench.lua", sysbench_lua, &sysbench_lua_len},
  {"sysbench.compat.lua", sysbench_compat_lua, &sysbench_compat_lua_len},
  {"sysbench.cmdline.lua", sysbench_cmdline_lua, &sysbench_cmdline_lua_len},
  {"sysbench.sql.lua", sysbench_sql_lua, &sysbench_sql_lua_len},
  {"sysbench.histogram.lua", sysbench_histogram_lua,
   &sysbench_histogram_lua_len},
  {NULL, NULL, 0}
};

/* Main (global) interpreter state */
static lua_State *gstate;

/* Custom command name */
static const char * sb_lua_custom_command;

/* Lua test operations */

static int sb_lua_op_init(void);
static int sb_lua_op_done(void);
static int sb_lua_op_thread_init(int);
static int sb_lua_op_thread_run(int);
static int sb_lua_op_thread_done(int);

static sb_operations_t lua_ops = {
   .init = sb_lua_op_init,
   .thread_init = sb_lua_op_thread_init,
   .thread_done = sb_lua_op_thread_done,
   .report_intermediate = db_report_intermediate,
   .report_cumulative = db_report_cumulative,
   .done = sb_lua_op_done
};

/* Lua test commands */
static int sb_lua_cmd_prepare(void);
static int sb_lua_cmd_cleanup(void);
static int sb_lua_cmd_help(void);

/* Initialize interpreter state */
static lua_State *sb_lua_new_state(void);

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

static unsigned int sb_lua_table_size(lua_State *, int);

static int read_cmdline_options(lua_State *L);
static bool sb_lua_hook_defined(lua_State *, const char *);
static bool sb_lua_hook_push(lua_State *, const char *);
static void sb_lua_report_intermediate(sb_stat_t *);
static void sb_lua_report_cumulative(sb_stat_t *);

static int sb_lua_do_jitcmd(lua_State *L, const char *cmd);

static void call_error(lua_State *L, const char *name)
{
  const char * const err = lua_tostring(L, -1);
  log_text(LOG_FATAL, "`%s' function failed: %s", name,
           err ? err : "(not a string)");
  lua_pop(L, 1);
}

static void report_error(lua_State *L)
{
  const char * const err = lua_tostring(L, -1);
  log_text(LOG_FATAL, "%s", err ? err : "(not a string)");
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
  bool rc = lua_isfunction(L, -1);
  lua_pop(L, 1);

  return rc;
}

/* Export command line options */

static int do_export_options(lua_State *L, bool global)
{
  sb_list_item_t *pos;
  option_t       *opt;
  char           *tmp;

  if (!global)
  {
    lua_getglobal(L, "sysbench");
    lua_pushliteral(L, "opt");
    lua_newtable(L);
  }

  pos = sb_options_enum_start();
  while ((pos = sb_options_enum_next(pos, &opt)) != NULL)
  {
    /*
      The only purpose of the following check if to keep compatibility with
      legacy scripts where options were exported to the global namespace. In
      which case name collisions with user-defined functions and variables might
      occur. For example, the --help option might redefine the help() function.
    */
    if (global)
    {
      lua_getglobal(L, opt->name);
      if (lua_isfunction(L, -1))
      {
        lua_pop(L, 1);
        continue;
      }
      lua_pop(L, 1);
    }
    else
    {
      lua_pushstring(L, opt->name);
    }

    switch (opt->type)
    {
      case SB_ARG_TYPE_BOOL:
        lua_pushboolean(L, sb_opt_to_flag(opt));
        break;
      case SB_ARG_TYPE_INT:
        lua_pushnumber(L, sb_opt_to_int(opt));
        break;
      case SB_ARG_TYPE_DOUBLE:
        lua_pushnumber(L, sb_opt_to_double(opt));
        break;
      case SB_ARG_TYPE_SIZE:
        lua_pushnumber(L, sb_opt_to_size(opt));
        break;
      case SB_ARG_TYPE_STRING:
        tmp = sb_opt_to_string(opt);
        lua_pushstring(L, tmp ? tmp : "");
        break;
      case SB_ARG_TYPE_LIST:
        lua_newtable(L);

        sb_list_item_t *val;
        int count = 1;

        SB_LIST_FOR_EACH(val, sb_opt_to_list(opt))
        {
          lua_pushstring(L, SB_LIST_ENTRY(val, value_t, listitem)->data);
          lua_rawseti(L, -2, count++);
        }

        break;
      case SB_ARG_TYPE_FILE:
        /* no need to export anything */
        lua_pushnil(L);
        break;
      default:
        log_text(LOG_WARNING, "Global option '%s' will not be exported, because"
                 " the type is unknown", opt->name);
        lua_pushnil(L);
        break;
    }

    /* set var = value */
    if (global)
      lua_setglobal(L, opt->name);
    else
      lua_settable(L, -3);
  }

  if (!global)
    lua_settable(L, -3); /* set sysbench.opt */

  return 0;
}

/*
  Export option values to the 'sysbench.opt' table. If the script does not
  declare supported options with sysbench.cmdline.options also export to the
  global namespace for compatibility with the legacy API.
*/

static int export_options(lua_State *L)
{
  if (do_export_options(L, false))
    return 1;

  if (sbtest.args == NULL && do_export_options(L, true))
    return 1;

  return 0;
}

/* Load a specified Lua script */

sb_test_t *sb_load_lua(const char *testname)
{
  if (testname != NULL)
  {
    char *tmp = strdup(testname);
    sbtest.sname = strdup(basename(tmp));
    sbtest.lname = tmp;
  }
  else
  {
    sbtest.sname = strdup("<stdin>");
    sbtest.lname = NULL;
  }

  /* Initialize global interpreter state */
  gstate = sb_lua_new_state();
  if (gstate == NULL)
    goto error;

  if (read_cmdline_options(gstate))
    goto error;

  /* Test commands */
  if (func_available(gstate, PREPARE_FUNC))
    sbtest.builtin_cmds.prepare = &sb_lua_cmd_prepare;

  if (func_available(gstate, CLEANUP_FUNC))
    sbtest.builtin_cmds.cleanup = &sb_lua_cmd_cleanup;

  if (func_available(gstate, HELP_FUNC))
    sbtest.builtin_cmds.help = &sb_lua_cmd_help;

  /* Test operations */
  sbtest.ops = lua_ops;

  if (func_available(gstate, THREAD_RUN_FUNC))
    sbtest.ops.thread_run = &sb_lua_op_thread_run;

  if (sb_lua_hook_defined(gstate, REPORT_INTERMEDIATE_HOOK))
    sbtest.ops.report_intermediate = sb_lua_report_intermediate;

  if (sb_lua_hook_defined(gstate, REPORT_CUMULATIVE_HOOK))
    sbtest.ops.report_cumulative = sb_lua_report_cumulative;

  /* Allocate per-thread interpreters array */
  states = (lua_State **)calloc(sb_globals.threads, sizeof(lua_State *));
  if (states == NULL)
    goto error;

  return &sbtest;

 error:

  sb_lua_done();

  return NULL;
}


void sb_lua_done(void)
{
  sb_lua_close_state(gstate);
  gstate = NULL;

  xfree(states);

  if (sbtest.args != NULL)
  {
    for (size_t i = 0; sbtest.args[i].name != NULL; i++)
    {
      xfree(sbtest.args[i].name);
      xfree(sbtest.args[i].desc);
      xfree(sbtest.args[i].value);
    }

    xfree(sbtest.args);
  }

  xfree(sbtest.sname);
  xfree(sbtest.lname);
}


/* Initialize Lua script */

int sb_lua_op_init(void)
{
  if (export_options(gstate))
      return 1;

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
             sbtest.sname);
    return 1;
  }

  return 0;
}

int sb_lua_op_thread_init(int thread_id)
{
  lua_State * L;

  L = sb_lua_new_state();
  if (L == NULL)
    return 1;

  states[thread_id] = L;

  if (export_options(L))
    return 1;

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

int sb_lua_op_done(void)
{
  lua_getglobal(gstate, DONE_FUNC);
  if (!lua_isnil(gstate, -1))
  {
    if (lua_pcall(gstate, 0, 0, 0))
    {
      call_error(gstate, DONE_FUNC);
      return 1;
    }
  }

  sb_lua_done();

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

static void sb_lua_var_number(lua_State *L, const char *name, lua_Number n)
{
    lua_pushstring(L, name);
    lua_pushnumber(L, n);
    lua_settable(L, -3);
}

static void sb_lua_var_func(lua_State *L, const char *name, lua_CFunction f)
{
    lua_pushstring(L, name);
    lua_pushcfunction(L, f);
    lua_settable(L, -3);
}

static void sb_lua_var_string(lua_State *L, const char *name, const char *s)
{
    lua_pushstring(L, name);
    lua_pushstring(L, s);
    lua_settable(L, -3);
}

/*
  Set package.path and package.cpath in a given environment. Also honor
  LUA_PATH/LUA_CPATH to mimic the default Lua behavior.
*/
static void sb_lua_set_paths(lua_State *L)
{
  lua_getglobal(L, "package");

  int top = lua_gettop(L);

  lua_pushliteral(L, "./?.lua;");
  lua_pushliteral(L, "./?/init.lua;");
  lua_pushliteral(L, "./src/lua/?.lua;");

  const char *home = getenv("HOME");
  if (home != NULL)
  {
    lua_pushstring(L, home);
    lua_pushliteral(L, "/.luarocks/share/lua/5.1/?.lua;");
    lua_pushstring(L, home);
    lua_pushliteral(L, "/.luarocks/share/lua/5.1/?/init.lua;");
    lua_pushstring(L, home);
    lua_pushliteral(L, "/.luarocks/share/lua/?.lua;");
    lua_pushstring(L, home);
    lua_pushliteral(L, "/.luarocks/share/lua/?/init.lua;");
  }

  lua_pushliteral(L, "/usr/local/share/lua/5.1/?.lua;");

  lua_pushliteral(L, DATADIR "/?.lua;");

  lua_concat(L, lua_gettop(L) - top);

  /* Mimic the default Lua behavior with respect to LUA_PATH and ';;' */
  const char *path = getenv("LUA_PATH");
  if (path != NULL)
  {
    const char *def = lua_tostring(L, -1);
    path = luaL_gsub(L, path, ";;", ";\1;");
    luaL_gsub(L, path, "\1", def);
    lua_remove(L, -2);
    lua_remove(L, -2);
  }
  lua_setfield(L, top, "path");

  lua_pushliteral(L, "./?" DLEXT ";");
  if (home != NULL) {
    lua_pushstring(L, home);
    lua_pushliteral(L, "/.luarocks/lib/lua/5.1/?" DLEXT ";");
    lua_pushstring(L, home);
    lua_pushliteral(L, "/.luarocks/lib/lua/?" DLEXT ";");
  }

  lua_pushliteral(L, "/usr/local/lib/lua/5.1/?" DLEXT ";");

  lua_pushliteral(L, LIBDIR ";");
  lua_concat(L, lua_gettop(L) - top);

  /* Mimic the default Lua behavior with respect to LUA_CPATH and ';;' */
  path = getenv("LUA_CPATH");
  if (path != NULL)
  {
    const char *def = lua_tostring(L, -1);
    path = luaL_gsub(L, path, ";;", ";\1;");
    luaL_gsub(L, path, "\1", def);
    lua_remove(L, -2);
    lua_remove(L, -2);
  }
  lua_setfield(L, top, "cpath");

  lua_pop(L, 1); /* package */
}

/* Create a deep copy of the 'args' array and store it in sbtest.args */

int sb_lua_set_test_args(sb_arg_t *args, size_t len)
{
  sbtest.args = malloc((len + 1) * sizeof(sb_arg_t));

  for (size_t i = 0; i < len; i++)
  {
    sbtest.args[i].name = strdup(args[i].name);
    sbtest.args[i].desc = strdup(args[i].desc);
    sbtest.args[i].type = args[i].type;

    sbtest.args[i].value = args[i].value != NULL ? strdup(args[i].value) : NULL;
    sbtest.args[i].validate = args[i].validate;
  }

  sbtest.args[len] = (sb_arg_t) {.name = NULL};

  return 0;
}

/*
  Parse command line options definitions, if present in the script as a
  sysbench.cmdline.options table. If there was a parsing error, return 1. Return
  0 on success.
*/

static int read_cmdline_options(lua_State *L)
{
  lua_getglobal(L, "sysbench");
  lua_getfield(L, -1, "cmdline");
  lua_getfield(L, -1, "read_cmdline_options");

  if (!lua_isfunction(L, -1))
  {
    log_text(LOG_WARNING,
             "Cannot find sysbench.cmdline.read_cmdline_options()");
    lua_pop(L, 3);

    return 1;
  }

  if (lua_pcall(L, 0, 1, 0) != 0)
  {
    call_error(L, "sysbench.cmdline.read_cmdline_options");
    lua_pop(L, 2);
    return 1;
  }

  int rc = lua_toboolean(L, -1) == 0;

  lua_pop(L, 3);

  return rc;
}

/* Allocate and initialize new interpreter state */

static lua_State *sb_lua_new_state(void)
{
  lua_State      *L;

  L = luaL_newstate();

  luaL_openlibs(L);

  if (sb_globals.luajit_cmd && sb_lua_do_jitcmd(L, sb_globals.luajit_cmd))
    return NULL;

  sb_lua_set_paths(L);

  /* Export variables into per-state 'sysbench' table */

  lua_newtable(L);

  /* sysbench.tid */
  sb_lua_var_number(L, "tid", sb_tls_thread_id);

  /* Export functions into per-state 'sysbench.db' table  */

  lua_pushliteral(L, "db");
  lua_newtable(L);

  sb_lua_var_func(L, "connect", sb_lua_db_connect);
  sb_lua_var_func(L, "disconnect", sb_lua_db_disconnect);
  sb_lua_var_func(L, "query", sb_lua_db_query);
  sb_lua_var_func(L, "bulk_insert_init", sb_lua_db_bulk_insert_init);
  sb_lua_var_func(L, "bulk_insert_next", sb_lua_db_bulk_insert_next);
  sb_lua_var_func(L, "bulk_insert_done", sb_lua_db_bulk_insert_done);
  sb_lua_var_func(L, "prepare", sb_lua_db_prepare);
  sb_lua_var_func(L, "bind_param", sb_lua_db_bind_param);
  sb_lua_var_func(L, "bind_result", sb_lua_db_bind_result);
  sb_lua_var_func(L, "execute", sb_lua_db_execute);
  sb_lua_var_func(L, "close", sb_lua_db_close);
  sb_lua_var_func(L, "store_results", sb_lua_db_store_results);
  sb_lua_var_func(L, "free_results", sb_lua_db_free_results);

  sb_lua_var_number(L, "DB_ERROR_NONE", DB_ERROR_NONE);
  sb_lua_var_number(L, "DB_ERROR_RESTART_TRANSACTION", DB_ERROR_IGNORABLE);
  sb_lua_var_number(L, "DB_ERROR_FAILED", DB_ERROR_FATAL);

  lua_settable(L, -3); /* sysbench.db */

  lua_pushliteral(L, "error");
  lua_newtable(L);

  sb_lua_var_number(L, "NONE", SB_LUA_ERROR_NONE);
  sb_lua_var_number(L, "RESTART_EVENT", SB_LUA_ERROR_RESTART_EVENT);

  lua_settable(L, -3); /* sysbench.error */

   /* sysbench.version */
  sb_lua_var_string(L, "version", PACKAGE_VERSION);
   /* sysbench.version_string */
  sb_lua_var_string(L, "version_string",
                    PACKAGE_NAME " " PACKAGE_VERSION SB_GIT_SHA);

  lua_pushliteral(L, "cmdline");
  lua_newtable(L);

  lua_pushliteral(L, "argv");
  lua_createtable(L, sb_globals.argc, 0);

  for (int i = 0; i < sb_globals.argc; i++)
  {
    lua_pushstring(L, sb_globals.argv[i]);
    lua_rawseti(L, -2, i);
  }

  lua_settable(L, -3); /* sysbench.cmdline.argv */

  /* Export command name as sysbench.cmdline.command */
  if (sb_globals.cmdname)
  {
    lua_pushliteral(L, "command");
    lua_pushstring(L, sb_globals.cmdname);
    lua_settable(L, -3);
  }

  /* Export script path as sysbench.cmdline.script_path */
  sb_lua_var_string(L, "script_path", sbtest.lname);

  lua_settable(L, -3); /* sysbench.cmdline */

  lua_setglobal(L, "sysbench");

  luaL_newmetatable(L, "sysbench.stmt");
  luaL_newmetatable(L, "sysbench.rs");

  if (load_internal_scripts(L))
    return NULL;

  int rc;

  if ((rc = luaL_loadfile(L, sbtest.lname)) != 0)
  {
    if (rc != LUA_ERRFILE)
      goto loaderr;

    /* Try to handle the given string as a module name */
    lua_getglobal(L, "require");
    lua_pushstring(L, sbtest.lname);
    if (lua_pcall(L, 1, 1, 0))
    {
      const char *msg = lua_tostring(L, -1);
      if (msg && strncmp(msg, "module ", 7))
        goto loaderr;

      log_text(LOG_FATAL, "Cannot find benchmark '%s': no such built-in test, "
               "file or module", sbtest.lname);

      return NULL;
    }
  }
  else if (lua_pcall(L, 0, 0, 0))
    goto loaderr;

  /* Create new L context */
  tls_lua_ctxt.L = L;

  return L;

loaderr:
  report_error(L);

  return NULL;
}

/* Close interpreter state */

int sb_lua_close_state(lua_State *state)
{
  sb_lua_ctxt_t * const ctxt = &tls_lua_ctxt;

  if (state != NULL)
    lua_close(state);

  if (ctxt != NULL)
  {
    sb_lua_db_disconnect(state);

    if (ctxt->driver != NULL)
    {
      db_destroy(ctxt->driver);
      ctxt->driver = NULL;
    }
  }

  ctxt->L = NULL;

  return 0;
}

/* Execute a given command */
static int execute_command(const char *cmd)
{
  if (export_options(gstate))
    return 1;

  lua_getglobal(gstate, cmd);

  if (lua_pcall(gstate, 0, 1, 0) != 0)
  {
    call_error(gstate, cmd);
    return 1;
  }

  lua_pop(gstate, 1);

  return 0;
}

/* Prepare command */

int sb_lua_cmd_prepare(void)
{
  return execute_command(PREPARE_FUNC);
}

/* Cleanup command */

int sb_lua_cmd_cleanup(void)
{
  return execute_command(CLEANUP_FUNC);
}

/* Help command */

int sb_lua_cmd_help(void)
{
  return execute_command(HELP_FUNC);
}


int sb_lua_db_connect(lua_State *L)
{
  sb_lua_ctxt_t * const ctxt = &tls_lua_ctxt;

  ctxt->driver = db_create(NULL);
  if (ctxt->driver == NULL)
  {
    luaL_error(L, "DB initialization failed");
  }

  lua_pushstring(L, ctxt->driver->sname);
  lua_setglobal(L, "db_driver");

  if (ctxt->con != NULL)
    return 0;

  ctxt->con = db_connection_create(ctxt->driver);
  if (ctxt->con == NULL)
    luaL_error(L, "Failed to connect to the database");

  return 0;
}

int sb_lua_db_disconnect(lua_State *L)
{
  (void) L; /* unused */

  if (tls_lua_ctxt.con)
  {
    db_connection_close(tls_lua_ctxt.con);
    db_connection_free(tls_lua_ctxt.con);

    tls_lua_ctxt.con = NULL;
  }

  return 0;
}

/*
  Throw an error with the { errcode = RESTART_EVENT } table. This will make
  thread_run() restart the event.
*/

static void throw_restart_event(lua_State *L)
{
  log_text(LOG_DEBUG, "Ignored error encountered, restarting transaction");

  lua_createtable(L, 0, 1);
  lua_pushliteral(L, "errcode");
  lua_pushnumber(L, SB_LUA_ERROR_RESTART_EVENT);
  lua_settable(L, -3);

  lua_error(L); /* this call never returns */
}

int sb_lua_db_query(lua_State *L)
{
  const char        *query;
  db_result_t       *rs;
  size_t            len;

  if (tls_lua_ctxt.con == NULL)
    sb_lua_db_connect(L);

  db_conn_t * const con = tls_lua_ctxt.con;

  query = luaL_checklstring(L, 1, &len);
  rs = db_query(con, query, len);
  if (rs == NULL)
  {
    if (con->error == DB_ERROR_IGNORABLE)
      throw_restart_event(L);
    else if (con->error == DB_ERROR_FATAL)
      luaL_error(L, "db_query() failed");
  }

  if (rs != NULL)
    db_free_results(rs);

  return 0;
}

int sb_lua_db_bulk_insert_init(lua_State *L)
{
  const char *query;
  size_t len;

  if (tls_lua_ctxt.con == NULL)
    sb_lua_db_connect(L);

  query = luaL_checklstring(L, 1, &len);
  if (db_bulk_insert_init(tls_lua_ctxt.con, query, len))
    luaL_error(L, "db_bulk_insert_init() failed");

  return 0;
}

int sb_lua_db_bulk_insert_next(lua_State *L)
{
  const char *query;
  size_t len;

  check_connection(L, &tls_lua_ctxt);

  query = luaL_checklstring(L, 1, &len);
  if (db_bulk_insert_next(tls_lua_ctxt.con, query, len))
    luaL_error(L, "db_bulk_insert_next() failed");

  return 0;
}

int sb_lua_db_bulk_insert_done(lua_State *L)
{
  check_connection(L, &tls_lua_ctxt);

  db_bulk_insert_done(tls_lua_ctxt.con);

  return 0;
}

int sb_lua_db_prepare(lua_State *L)
{
  sb_lua_db_stmt_t *stmt;
  const char       *query;
  size_t           len;

  if (tls_lua_ctxt.con == NULL)
    sb_lua_db_connect(L);

  query = luaL_checklstring(L, 1, &len);

  stmt = (sb_lua_db_stmt_t *)lua_newuserdata(L, sizeof(sb_lua_db_stmt_t));
  luaL_getmetatable(L, "sysbench.stmt");
  lua_setmetatable(L, -2);
  memset(stmt, 0, sizeof(sb_lua_db_stmt_t));

  stmt->ptr = db_prepare(tls_lua_ctxt.con, query, len);
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

  check_connection(L, &tls_lua_ctxt);

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

  check_connection(L, &tls_lua_ctxt);

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

  check_connection(L, &tls_lua_ctxt);

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
  if (ptr == NULL && tls_lua_ctxt.con->error == DB_ERROR_IGNORABLE)
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

  check_connection(L, &tls_lua_ctxt);

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

  check_connection(L, &tls_lua_ctxt);

  rs = (sb_lua_db_rs_t *)luaL_checkudata(L, 1, "sysbench.rs");
  luaL_argcheck(L, rs != NULL, 1, "result set expected");

  /* noop as db_store_results() is now performed automatically by db_query() */

  return 0;
}

int sb_lua_db_free_results(lua_State *L)
{
  sb_lua_db_rs_t   *rs;

  check_connection(L, &tls_lua_ctxt);

  rs = (sb_lua_db_rs_t *)luaL_checkudata(L, 1, "sysbench.rs");
  luaL_argcheck(L, rs != NULL, 1, "result set expected");

  if (rs->ptr != NULL)
  {
    db_free_results(rs->ptr);
    rs->ptr = NULL;
  }

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

/* Check if a specified hook exists */

static bool sb_lua_hook_defined(lua_State *L, const char *name)
{
  if (L == NULL)
    return false;

  lua_getglobal(L, "sysbench");
  lua_getfield(L, -1, "hooks");
  lua_getfield(L, -1, name);

  bool rc = lua_isfunction(L, -1);

  lua_pop(L, 3);

  return rc;
}

/* Push a specified hook on stack */

static bool sb_lua_hook_push(lua_State *L, const char *name)
{
  if (L == NULL)
    return false;

  lua_getglobal(L, "sysbench");
  lua_getfield(L, -1, "hooks");
  lua_getfield(L, -1, name);

  if (!lua_isfunction(L, -1))
  {
    lua_pop(L, 3);
    return false;
  }

  lua_remove(L, -2); /* hooks */
  lua_remove(L, -2); /* sysbench */

  return true;
}


bool sb_lua_loaded(void)
{
  return gstate != NULL;
}

/* Check if a specified custom command exists */

bool sb_lua_custom_command_defined(const char *name)
{
  lua_State * const L = gstate;

  lua_getglobal(L, "sysbench");
  lua_getfield(L, -1, "cmdline");
  lua_getfield(L, -1, "command_defined");

  if (!lua_isfunction(L, -1))
  {
    log_text(LOG_WARNING,
             "Cannot find the sysbench.cmdline.command_defined function");
    lua_pop(L, 3);

    return 1;
  }

  lua_pushstring(L, name);

  if (lua_pcall(L, 1, 1, 0) != 0)
  {
    call_error(L, "sysbench.cmdline.command_defined");
    lua_pop(L, 2);
    return 1;
  }

  bool rc = lua_toboolean(L, -1);

  lua_pop(L, 3);

  return rc;
}

/* Check if a specified custom command supports parallel execution */

static bool sb_lua_custom_command_parallel(const char *name)
{
  lua_State * const L = gstate;

  lua_getglobal(L, "sysbench");
  lua_getfield(L, -1, "cmdline");
  lua_getfield(L, -1, "command_parallel");

  if (!lua_isfunction(L, -1))
  {
    log_text(LOG_WARNING,
             "Cannot find the sysbench.cmdline.command_parallel function");
    lua_pop(L, 3);

    return 1;
  }

  lua_pushstring(L, name);

  if (lua_pcall(L, 1, 1, 0) != 0)
  {
    call_error(L, "sysbench.cmdline.command_parallel");
    lua_pop(L, 2);
    return 1;
  }

  bool rc = lua_toboolean(L, -1);

  lua_pop(L, 3);

  return rc;
}

static int call_custom_command(lua_State *L)
{
  if (export_options(L))
    return 1;

  lua_getglobal(L, "sysbench");
  lua_getfield(L, -1, "cmdline");
  lua_getfield(L, -1, "call_command");

  if (!lua_isfunction(L, -1))
  {
    log_text(LOG_WARNING,
             "Cannot find the sysbench.cmdline.call_command function");
    lua_pop(L, 3);

    return 1;
  }

  lua_pushstring(L, sb_lua_custom_command);

  if (lua_pcall(L, 1, 1, 0) != 0)
  {
    call_error(L, "sysbench.cmdline.call_command");
    lua_pop(L, 2);
    return 1;
  }

  bool rc = lua_toboolean(L, -1);

  lua_pop(L, 3);

  return rc ? EXIT_SUCCESS : EXIT_FAILURE;
}


static void *cmd_worker_thread(void *arg)
{
  sb_thread_ctxt_t   *ctxt= (sb_thread_ctxt_t *)arg;

  sb_tls_thread_id = ctxt->id;

  /* Initialize thread-local RNG state */
  sb_rand_thread_init();

  lua_State * const L = sb_lua_new_state();

  if (L == NULL)
  {
    log_text(LOG_FATAL, "failed to create a thread to execute command");
    return NULL;
  }

  call_custom_command(L);

  sb_lua_close_state(L);

  return NULL;
}

/* Call a specified custom command */

int sb_lua_call_custom_command(const char *name)
{
  sb_lua_custom_command = name;

  if (sb_lua_custom_command_parallel(name) && sb_globals.threads > 1)
  {
    int err;

    if ((err = sb_thread_create_workers(cmd_worker_thread)))
      return err;

    return sb_thread_join_workers();
  }

  return call_custom_command(gstate);
}

#define stat_to_number(name) sb_lua_var_number(L, #name, stat->name)

static void stat_to_lua_table(lua_State *L, sb_stat_t *stat)
{
  lua_newtable(L);
  stat_to_number(threads_running);
  stat_to_number(time_interval);
  stat_to_number(time_total);
  stat_to_number(latency_pct);
  stat_to_number(events);
  stat_to_number(reads);
  stat_to_number(writes);
  stat_to_number(other);
  stat_to_number(errors);
  stat_to_number(reconnects);
}

/* Call sysbench.hooks.report_intermediate */

static void sb_lua_report_intermediate(sb_stat_t *stat)
{
  lua_State * const L = tls_lua_ctxt.L;

  if (!sb_lua_hook_push(L, REPORT_INTERMEDIATE_HOOK))
    return;

  stat_to_lua_table(L, stat);

  /*
    The following is only available for intermediate reports with tx_rate > 0
  */
  stat_to_number(queue_length);
  stat_to_number(concurrency);

  if (lua_pcall(L, 1, 0, 0))
  {
    call_error(L, REPORT_INTERMEDIATE_HOOK);
  }
}

/* Call sysbench.hooks.report_cumulative */

static void sb_lua_report_cumulative(sb_stat_t *stat)
{
  lua_State * const L = tls_lua_ctxt.L;

  /*
    This may be called either from a separate checkpoint thread (in which case
    options are exported by sb_lua_report_thread_init(), or from the master
    thread on benchmark exit. In the latter case, options must be exported, as
    we don't normally do that for the global Lua state.
  */
  if (L == gstate)
    export_options(L);

  if (!sb_lua_hook_push(L, REPORT_CUMULATIVE_HOOK))
    return;

  stat_to_lua_table(L, stat);

  /* The following stats are only available for cumulative reports */
  stat_to_number(latency_min);
  stat_to_number(latency_max);
  stat_to_number(latency_avg);
  stat_to_number(latency_sum);

  if (lua_pcall(L, 1, 0, 0))
  {
    call_error(L, REPORT_CUMULATIVE_HOOK);
  }
}

#undef stat_to_number


int sb_lua_report_thread_init(void)
{
  if (tls_lua_ctxt.L == NULL)
  {
    sb_lua_new_state();
    export_options(tls_lua_ctxt.L);
  }

  return 0;
}

void sb_lua_report_thread_done(void)
{
  sb_lua_close_state(tls_lua_ctxt.L);
}

/*
  Perform a LuaJIT engine control command. This is taken with modifications from
  luajit.c
*/

int sb_lua_do_jitcmd(lua_State *L, const char *cmd)
{
  const char *opt = strchr(cmd, '=');

  lua_pushlstring(L, cmd, opt ? (size_t) (opt - cmd) : strlen(cmd));
  lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
  lua_getfield(L, -1, "jit");  /* Get jit.* module table. */
  lua_remove(L, -2);
  lua_pushvalue(L, -2);
  lua_gettable(L, -2);  /* Lookup library function. */

  if (!lua_isfunction(L, -1))
  {
    lua_pop(L, 2);  /* Drop non-function and jit.* table, keep module name. */

    /* Load add-on module. */
    lua_getglobal(L, "require");
    lua_pushliteral(L, "jit.");
    lua_pushvalue(L, -3);
    lua_concat(L, 2);

    if (lua_pcall(L, 1, 1, 0))
    {
      const char *msg = lua_tostring(L, -1);
      if (msg && !strncmp(msg, "module ", 7))
        goto nomodule;
      call_error(L, "require");
      return 1;
    }

    lua_getfield(L, -1, "start");
    if (lua_isnil(L, -1)) {
  nomodule:
      log_text(LOG_FATAL,
               "unknown luaJIT command or jit.* modules not installed");
      return 1;
    }
    lua_remove(L, -2);  /* Drop module table. */
  }
  else
  {
    lua_remove(L, -2);  /* Drop jit.* table. */
  }
  lua_remove(L, -2);  /* Drop module name. */

  int narg = 0;

  if (opt && *++opt)
  {
    for (;;)
    {
      /* Split arguments. */
      const char *p = strchr(opt, ',');
      narg++;

      if (!p)
        break;

      if (p == opt)
        lua_pushnil(L);
      else
        lua_pushlstring(L, opt, (size_t) (p - opt));

      opt = p + 1;
    }

    if (*opt)
      lua_pushstring(L, opt);
    else
      lua_pushnil(L);
  }

  if (lua_pcall(L, narg, 0, 0))
  {
    call_error(L, "lua_pcall");
    return 1;
  }

  return 0;
}
