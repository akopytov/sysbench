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
-- Read/Write OLTP benchmark
-- ----------------------------------------------------------------------

require("windmills/oltp_common")

-- Test specific options
sysbench.cmdline.options.all_in_one =
   {"All traffic will go to one single table", false}
   

sysbench.cmdline.commands.prepare = {
   function ()
      -- if all on one is active then we only have 1 table for all the traffic
      if ( sysbench.opt.all_in_one) then
         sysbench.opt.tables=1
      end
      cmd_prepare()
   end,
   sysbench.cmdline.PARALLEL_COMMAND
}


function prepare_statements()
  
   if not sysbench.opt.skip_trx then
      prepare_begin()
      prepare_commit()
   end

   prepare_inserts()
end

function event()
  -- if all on one is active then we only have 1 table for all the traffic
   if ( sysbench.opt.all_in_one) then
     sysbench.opt.tables=1
   end
 
   if not sysbench.opt.skip_trx then
      begin()
   end
   execute_inserts()

   if not sysbench.opt.skip_trx then
      commit()
   end
   check_reconnect()
end

function sysbench.hooks.report_intermediate(stat)
   if sysbench.opt.stats_format == "human" then
         sysbench.report_default(stat)
   elseif sysbench.opt.stats_format == "csv" then
         sysbench.report_csv(stat)
   elseif sysbench.opt.stats_format == "json" then      
         sysbench.report_json(stat)
   else
      sysbench.report_default(stat)
   end
end
