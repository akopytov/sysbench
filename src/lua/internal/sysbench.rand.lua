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

ffi = require("ffi")

-- ----------------------------------------------------------------------
-- Pseudo-random number generation API
-- ----------------------------------------------------------------------

sysbench.rand = {}

ffi.cdef[[
uint64_t sb_rand_uniform_uint64(void);
uint32_t sb_rand_default(uint32_t, uint32_t);
uint32_t sb_rand_uniform(uint32_t, uint32_t);
uint32_t sb_rand_gaussian(uint32_t, uint32_t);
uint32_t sb_rand_special(uint32_t, uint32_t);
uint32_t sb_rand_pareto(uint32_t, uint32_t);
uint32_t sb_rand_zipfian(uint32_t, uint32_t);
uint32_t sb_rand_unique(void);
void sb_rand_str(const char *, char *);
uint32_t sb_rand_varstr(char *, uint32_t, uint32_t);
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

function sysbench.rand.zipfian(a, b)
   return ffi.C.sb_rand_zipfian(a, b)
end

function sysbench.rand.unique()
   return ffi.C.sb_rand_unique()
end

function sysbench.rand.string(fmt)
   local buflen = #fmt
   local buf = ffi.new("uint8_t[?]", buflen)
   ffi.C.sb_rand_str(fmt, buf)
   return ffi.string(buf, buflen)
end

function sysbench.rand.varstring(min_len, max_len)
   assert(min_len <= max_len)
   assert(max_len > 0)
   local buflen = max_len
   local buf = ffi.new("uint8_t[?]", buflen)
   local nchars = ffi.C.sb_rand_varstr(buf, min_len, max_len)
   return ffi.string(buf, nchars)
end

function sysbench.rand.uniform_double()
   return ffi.C.sb_rand_uniform_double()
end
