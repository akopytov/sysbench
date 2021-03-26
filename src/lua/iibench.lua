#!/usr/bin/env sysbench
-- Copyright (C) 2006-2017 Alexey Kopytov <akopytov@gmail.com>

-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation; either version 2 of the License, or
-- (at your option) any later version.

-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.

-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software
-- Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

-- ----------------------------------------------------------------------
-- Index insertion benchmark
-- ----------------------------------------------------------------------

require("iibench_common")


function prepare_statements()
   if sysbench.opt.threads - sysbench.opt.insert_threads > 0
   then
      prepare_market_queries()
      prepare_register_queries()
      prepare_pdc_queries()
   end
   prepare_thread_groups()
   if sysbench.opt.instant_delete
   then
       prepare_deletes()
   end
end

function insert_event()
   execute_inserts()

   check_reconnect()
end

function event()
   local query_type = sysbench.rand.uniform(1,3)
   local switch = {
      [1] = execute_market_queries,
      [2] = execute_pdc_queries,
      [3] = execute_register_queries
   }
   switch[query_type]()


   check_reconnect()
end


function prepare_thread_groups()

   -- resolve alias
   if sysbench.opt.query_threads ~= 0
   then
      if sysbench.opt.select_threads == 0
      then
         sysbench.opt.select_threads = sysbench.opt.query_threads
      else
         print('query_threads is an alias for select_threads, but they have ' ..
            'different values')
      end
   end

   if sysbench.opt.query_rate ~= 0
   then
      if sysbench.opt.select_rate == 0
      then
         sysbench.opt.select_rate = sysbench.opt.query_rate
      else
         print('query_rate is an alias for select_rate, but they have ' ..
            'different values')
      end
   end


   local insert_rate = sysbench.opt.inserts_per_second / sysbench.opt.insert_threads
   if insert_rate == 0
   then
      insert_rate = sysbench.opt.insert_rate
   end

   thread_groups = {
      {
         event = insert_event,
         thread_amount = sysbench.opt.insert_threads,
         rate = insert_rate,
         rate_controller = default_rate_controller
      },
      {
         event = event,
         thread_amount = sysbench.opt.select_threads,
         rate = sysbench.opt.select_rate,
         rate_controller = default_rate_controller
      }
   }
end