/* Copyright (C) 2004 MySQL AB
   Copyright (C) 2004-2008 Alexey Kopytov <akopytov@gmail.com>

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

#ifndef SB_MEMORY_H
#define SB_MEMORY_H

/* Memory request type definition */
typedef enum
{
  SB_MEM_OP_NONE,
  SB_MEM_OP_READ,
  SB_MEM_OP_WRITE,
  SB_MEM_OP_LATENCY
} sb_mem_op_t;


/* Memory scope type definition */
typedef enum
{
  SB_MEM_SCOPE_GLOBAL,
  SB_MEM_SCOPE_LOCAL
} sb_mem_scope_t;


int register_test_memory(sb_list_t *tests);

#endif
