-- Copyright (C) 2017 Alexey Kopytov <akopytov@gmail.com>

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
-- Bounded histograms API
-- ----------------------------------------------------------------------

ffi = require("ffi")

sysbench.histogram = {}

ffi.cdef[[
typedef struct histogram sb_histogram_t;

/*
  Allocate a new histogram and initialize it with sb_histogram_init().
*/
sb_histogram_t *sb_histogram_new(size_t size, double range_min,
                                 double range_max);

/*
  Deallocate a histogram allocated with sb_histogram_new().
*/
void sb_histogram_delete(sb_histogram_t *h);

/* Update histogram with a given value. */
void sb_histogram_update(sb_histogram_t *h, double value);

/*
  Print a given histogram to stdout
*/
void sb_histogram_print(sb_histogram_t *h);
]]

local histogram = {}

function histogram:update(value)
   ffi.C.sb_histogram_update(self, value)
end

function histogram:print()
   ffi.C.sb_histogram_print(self)
end

local histogram_mt = {
   __index = histogram,
   __tostring = '<sb_histogram>'
}
ffi.metatype('sb_histogram_t', histogram_mt)

function sysbench.histogram.new(size, range_min, range_max)
   local h = ffi.C.sb_histogram_new(size, range_min, range_max)

   return ffi.gc(h, ffi.C.sb_histogram_delete)
end
