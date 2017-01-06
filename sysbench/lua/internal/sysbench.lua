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
-- Pseudo-random number generation API
-- ----------------------------------------------------------------------

ffi = require("ffi")

sysbench.rand = {}

ffi.cdef[[
uint64_t sb_rand_uniform_uint64(void);
uint32_t sb_rand_default(uint32_t, uint32_t);
uint32_t sb_rand_uniform(uint32_t, uint32_t);
uint32_t sb_rand_gaussian(uint32_t, uint32_t);
uint32_t sb_rand_special(uint32_t, uint32_t);
uint32_t sb_rand_pareto(uint32_t, uint32_t);
uint32_t sb_rand_uniq(uint32_t, uint32_t);
void sb_rand_str(const char *, char *);
double sb_rand_uniform_double(void);
]]

function sysbench.rand.uniform_uint64()
   return ffi.C.sb_rand_uniform_uint64()
end

function sysbench.rand.default(a, b)
   return ffi.C.sb_rand_default(a, b)
end

function sysbench.rand.uniform(a, b)
   return ffi.C.sb_rand_uniform(a, b)
end

function sysbench.rand.gaussian(a, b)
   return ffi.C.sb_rand_gaussian(a, b)
end

function sysbench.rand.special(a, b)
   return ffi.C.sb_rand_special(a, b)
end

function sysbench.rand.pareto(a, b)
   return ffi.C.sb_rand_pareto(a, b)
end

function sysbench.rand.unique(a, b)
   return ffi.C.sb_rand_uniq(a, b)
end

function sysbench.rand.string(fmt)
   local buflen = #fmt
   local buf = ffi.new("uint8_t[?]", buflen)
   ffi.C.sb_rand_str(fmt, buf)
   return ffi.string(buf, buflen)
end

function sysbench.rand.uniform_double()
   return ffi.C.sb_rand_uniform_double()
end

-- ----------------------------------------------------------------------
-- Compatibility wrappers/aliases. These may be removed in later versions
-- ----------------------------------------------------------------------

thread_id = sysbench.tid

function sb_rnd()
   -- Keep lower 32 bits from sysbench.rand.uniform_uint64() and convert them to
   -- a Lua number
   return tonumber(sysbench.rand.uniform_uint64() %
                      tonumber("100000000", 16))
end

sb_rand = sysbench.rand.default
sb_rand_uniq = sysbench.rand.unique
sb_rand_str = sysbench.rand.string
sb_rand_uniform = sysbench.rand.uniform
sb_rand_gaussian = sysbench.rand.gaussian
sb_rand_special = sysbench.rand.special

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

-- ----------------------------------------------------------------------
-- Main event loop. This is a Lua version of sysbench.c:thread_run()
-- ----------------------------------------------------------------------
function thread_run(thread_id)
   if (type(event) ~= "function") then
      error("Cannot find the 'event' function in the script")
   end
   while sysbench.more_events() do
      sysbench.event_start()
      event(thread_id)
      sysbench.event_stop()
   end
end
