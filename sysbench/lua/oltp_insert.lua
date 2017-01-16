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
-- Insert-Only OLTP benchmark
-- ----------------------------------------------------------------------

pathtest = string.match(test, "(.*/)")

if pathtest then
   dofile(pathtest .. "oltp_common.lua")
else
   require("oltp_common")
end

function event()
   local table_name = "sbtest" .. sb_rand_uniform(1, oltp_tables_count)\
   local k_val = sysbench.rand.default(1, oltp_table_size)
   local c_val = get_c_value()
   local pad_val = get_pad_value()

   if (drv:name() == "pgsql" and oltp_auto_inc) then
      con:query(string.format("INSERT INTO %s (k, c, pad) VALUES " ..
                                 "(%d, '%s', '%s')",
                              table_name, k_val, c_val, pad_val))
   else
      if (oltp_auto_inc) then
         i = 0
      else
         i = sysbench.rand.unique()
      end

      con:query(string.format("INSERT INTO %s (id, k, c, pad) VALUES " ..
                                 "(%d, %d, '%s', '%s')",
                              table_name, i, k_val, c_val, pad_val))
   end
end
