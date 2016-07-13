/* Copyright (C) 2006 MySQL AB
   Copyright (C) 2006 Alexey Kopytov <akopytov@gmail.com>
   Copyright (C) 2016 Percona

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
#include "db_driver.h"

/* Interpreter context */

typedef struct {
  int       thread_id; /* SysBench thread ID */
  db_conn_t *con;      /* Database connection */
} sb_lua_ctxt_t;

/* Load a specified Lua script */

int script_load_lua(const char *testname, sb_test_t *test);
