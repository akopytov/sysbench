/* Copyright (C) 2006 MySQL AB
   Copyright (C) 2006 Alexey Kopytov <akopytov@gmail.com>

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

#include "sb_script.h"

#ifdef HAVE_LUA
# include "script_lua.h"
#endif

static sb_test_t test;

/* Initialize interpreter with a given script name */

sb_test_t *script_load(const char *testname)
{
  test.sname = strdup(testname);

#ifdef HAVE_LUA
  if (!script_load_lua(testname, &test))
    return &test;
#endif
  
  return NULL;
}
