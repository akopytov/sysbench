-- Copyright (C) 2016 Alexey Kopytov <akopytov@gmail.com>

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
-- Compatibility aliases. These may be removed in later versions
-- ----------------------------------------------------------------------

thread_id = sysbench.tid

sb_rand = sysbench.rand_default
sb_rand_uniq = sb_rand_uniq
sb_rnd = sysbench.rnd
sb_rand_str = sysbench.rand_str
sb_rand_uniform = sysbench.rand_uniform
sb_rand_gaussian = sysbench.rand_gaussian
sb_rand_special = sysbench.rand_special

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

DB_ERROR_NONE = sysbench.db.DB_ERROR_NONE
DB_ERROR_RESTART_TRANSACTION = sysbench.db.DB_ERROR_RESTART_TRANSACTION
DB_ERROR_FAILED = sysbench.db.DB_ERROR_FAILED

-- ----------------------------------------------------------------------
-- Main event loop. This is a Lua version of sysbench.c:thread_run()
-- ----------------------------------------------------------------------
function thread_run()
   while sysbench.more_events() do
      sysbench.event_start()
      event()
      sysbench.event_stop()
   end
end
