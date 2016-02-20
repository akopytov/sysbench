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

#ifndef SB_FILEIO_H
#define SB_FILEIO_H

#ifdef _WIN32
#include "sb_win.h" /* ssize_t defined*/
#endif

/* File operation types */
typedef enum
{
  FILE_OP_TYPE_NULL,
  FILE_OP_TYPE_READ,
  FILE_OP_TYPE_WRITE,
  FILE_OP_TYPE_FSYNC
} sb_file_op_t;

/* File IO request definition */

typedef struct
{
  unsigned int    file_id;
  long long       pos;
  ssize_t         size;
  sb_file_op_t    operation; 
} sb_file_request_t;

int register_test_fileio(sb_list_t *tests);

#endif
