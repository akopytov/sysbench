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
-- Write-Only OLTP benchmark
-- ----------------------------------------------------------------------

pathtest = string.match(test, "(.*/)")

if pathtest then
   dofile(pathtest .. "oltp_common.lua")
else
   require("oltp_common")
end

function event()
   local table_name = "sbtest" .. sb_rand_uniform(1, oltp_tables_count)

   if not oltp_skip_trx then
      con:query("BEGIN")
   end

   execute_index_updates(con, table_name)
   execute_non_index_updates(con, table_name)
   execute_delete_inserts(con, table_name)

   if not oltp_skip_trx then
      con:query("COMMIT")
   end
end
