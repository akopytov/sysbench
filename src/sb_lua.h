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

#include "sysbench.h"
#include "lua.h"

/* Load a specified Lua script */

sb_test_t *sb_load_lua(const char *testname);

void sb_lua_done(void);

int sb_lua_hook_call(lua_State *L, const char *name);

bool sb_lua_custom_command_defined(const char *name);

int sb_lua_call_custom_command(const char *name);

int sb_lua_report_thread_init(void);

void sb_lua_report_thread_done(void);

bool sb_lua_loaded(void);
