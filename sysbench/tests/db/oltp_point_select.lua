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
-- OLTP Point Select benchmark
-- ----------------------------------------------------------------------

pathtest = string.match(test, "(.*/)")

if pathtest then
   dofile(pathtest .. "oltp_common.lua")
else
   require("oltp_common")
end

function thread_init()
   set_vars()
   drv = sysbench.sql.driver()
   con = drv:connect()
end

function event()
  con:query(string.format("SELECT c FROM sbtest%d WHERE id=%d",
                          sysbench.rand.uniform(1, oltp_tables_count),
                          sysbench.rand.default(1, oltp_table_size)))
end
