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
-- Scans the table but filters all rows to avoid stressing client/server network
-- ----------------------------------------------------------------------

require("windmills/oltp_common")

function thread_init()
   drv = sysbench.sql.driver()
   con = drv:connect()

   stmt = {}
   params = {}

   for t = 1, sysbench.opt.tables do
      stmt[t] = {}
      params[t] = {}
   end

  if sysbench.opt.threads > sysbench.opt.tables then
     	sysbench.opt.threads = (sysbench.opt.tables - 1) 
  end


end


function prepare_statements()
   -- We do not use prepared statements here, but oltp_common.sh expects this
   -- function to be defined
end

function event()
   local tid = (sysbench.tid % sysbench.opt.threads) + 1
   local table_name = sysbench.opt.table_name .. tid

   con:query(string.format("SELECT * from %s WHERE LENGTH(location) < 0", table_name))
   check_reconnect()
end