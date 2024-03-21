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
-- Read-Only OLTP benchmark
-- ----------------------------------------------------------------------

require("windmills/oltp_common")

-- Test specific options
sysbench.cmdline.options.type_of_range =
   {"What type of range to select default: all | (simple, sum, order, distinct) ", "all"}

function prepare_statements()

   if not sysbench.opt.skip_trx then
      prepare_begin()
      prepare_commit()
   end
     
   if sysbench.opt.type_of_range == "all" then
      prepare_simple_ranges()
      prepare_sum_ranges()
      prepare_order_ranges()
      prepare_distinct_ranges()
    elseif(sysbench.opt.type_of_range == "simple") then
      prepare_simple_ranges()	    
    elseif(sysbench.opt.type_of_range == "sum") then 
      prepare_sum_ranges()
    elseif(sysbench.opt.type_of_range == "order") then 
      prepare_order_ranges()
    elseif(sysbench.opt.type_of_range == "distinct") then       
      prepare_distinct_ranges()
    end      
end

function event()
   if not sysbench.opt.skip_trx then
      begin()
   end

   if sysbench.opt.type_of_range == "all" then
      execute_simple_ranges()
      execute_sum_ranges()
      execute_order_ranges()
      execute_distinct_ranges()
    elseif(sysbench.opt.type_of_range == "simple") then
      execute_simple_ranges()	    
    elseif(sysbench.opt.type_of_range == "sum") then 
      execute_sum_ranges()
    elseif(sysbench.opt.type_of_range == "order") then 
      execute_order_ranges()
    elseif(sysbench.opt.type_of_range == "distinct") then       
      execute_distinct_ranges()
    end      

   if not sysbench.opt.skip_trx then
      commit()
   end

   check_reconnect()
end
