-- Copyright (C) 2016-2017 Alexey Kopytov <akopytov@gmail.com>

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
-- Compatibility wrappers/aliases. These may be removed in later versions
-- ----------------------------------------------------------------------

thread_id = sysbench.tid

test = sysbench.cmdline.script_path

function sb_rnd()
   -- Keep lower 32 bits from sysbench.rand.uniform_uint64() and convert them to
   -- a Lua number
   return tonumber(sysbench.rand.uniform_uint64() % 4294967296)
end

sb_rand = sysbench.rand.default
sb_rand_str = sysbench.rand.string
sb_rand_uniform = sysbench.rand.uniform
sb_rand_gaussian = sysbench.rand.gaussian
sb_rand_special = sysbench.rand.special

function sb_rand_uniq(a, b)
   local res
   if type(a) == "nil" then
      a = 0
   end
   if type(b) == "nil" then
      b = 4294967295
   end
   repeat
      res = sysbench.rand.unique()
   until res >= a and res <= b
   return res
end

db_connect = sysbench.db.connect
db_disconnect = sysbench.db.disconnect

db_query = sysbench.db.query

db_bulk_insert_init = sysbench.db.bulk_insert_init
db_bulk_insert_next = sysbench.db.bulk_insert_next
db_bulk_insert_done = sysbench.db.bulk_insert_done

db_prepare = sysbench.db.prepare
db_bind_param = sysbench.db.bind_param
db_bind_result = sysbench.db.bind_result
db_execute = sysbench.db.execute

db_store_results = sysbench.db.store_results
db_free_results = sysbench.db.free_results

db_close = sysbench.db.close

DB_ERROR_NONE = sysbench.db.DB_ERROR_NONE
DB_ERROR_RESTART_TRANSACTION = sysbench.db.DB_ERROR_RESTART_TRANSACTION
DB_ERROR_FAILED = sysbench.db.DB_ERROR_FAILED

mysql_table_engine = mysql_table_engine or "innodb"
myisam_max_rows = 1000000
