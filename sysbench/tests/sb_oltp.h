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

#ifndef SB_OLTP_H
#define SB_OLTP_H

#include "sb_list.h"

/* SQL query types definiton */

typedef enum
  {
  SB_SQL_QUERY_NULL,
  SB_SQL_QUERY_LOCK,
  SB_SQL_QUERY_UNLOCK,
  SB_SQL_QUERY_POINT,
  SB_SQL_QUERY_CALL,
  SB_SQL_QUERY_RANGE,
  SB_SQL_QUERY_RANGE_SUM,
  SB_SQL_QUERY_RANGE_ORDER,
  SB_SQL_QUERY_RANGE_DISTINCT,
  SB_SQL_QUERY_UPDATE_INDEX,
  SB_SQL_QUERY_UPDATE_NON_INDEX,
  SB_SQL_QUERY_DELETE,
  SB_SQL_QUERY_INSERT,
  SB_SQL_QUERY_RECONNECT
} sb_sql_query_type_t;

/* SQL queries definitions */

typedef struct
{
  int id;
} sb_sql_query_point_t;

typedef struct
{
  int from;
  int to;
} sb_sql_query_range_t;

typedef struct
{
  int id;
} sb_sql_query_update_t;

typedef struct
{
  int id;
} sb_sql_query_delete_t;

typedef struct
{
  int id;
} sb_sql_query_insert_t;

typedef struct
{
  int thread_id;
  int nthreads;
} sb_sql_query_call_t;

/* SQL request definition */

typedef struct
{
  sb_sql_query_type_t type;
  unsigned int        num_times;   /* times to repeat the query */
  unsigned int        think_time;  /* sleep this time before executing query */
  unsigned long long  nrows;       /* expected number of rows in a result set */
  union
  {
    sb_sql_query_point_t   point_query;
    sb_sql_query_range_t   range_query;
    sb_sql_query_update_t  update_query;
    sb_sql_query_delete_t  delete_query;
    sb_sql_query_insert_t  insert_query;
  } u;
  
  sb_list_item_t listitem;  /* this struct can be stored in a linked list */
} sb_sql_query_t;

typedef struct
{
  sb_list_t *queries;  /* list of actual queries to perform */
} sb_sql_request_t;

int register_test_oltp(sb_list_t *tests);

#endif
