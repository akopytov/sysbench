/* Copyright (C) 2004 MySQL AB

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

#ifndef SB_MEMORY_H
#define SB_MEMORY_H

/* Memory request type definition */
typedef enum
{
  SB_MEM_OP_NONE,
  SB_MEM_OP_READ,
  SB_MEM_OP_WRITE
} sb_mem_op_t;


/* Memory scope type definition */
typedef enum
{
  SB_MEM_SCOPE_GLOBAL,
  SB_MEM_SCOPE_LOCAL
} sb_mem_scope_t;


/* Memory IO request definition */

typedef struct
{
  sb_mem_op_t    type;
  size_t  block_size;
  sb_mem_scope_t scope;
} sb_mem_request_t;

int register_test_memory(sb_list_t *tests);

#endif
