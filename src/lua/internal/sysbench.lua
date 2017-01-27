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

ffi.cdef[[
void sb_event_start(int thread_id);
void sb_event_stop(int thread_id);
bool sb_more_events(int thread_id);
]]

-- ----------------------------------------------------------------------
-- Main event loop. This is a Lua version of sysbench.c:thread_run()
-- ----------------------------------------------------------------------
function thread_run(thread_id)
   local success, ret

   while ffi.C.sb_more_events(thread_id) do
      ffi.C.sb_event_start(thread_id)

      repeat
         local success, ret = pcall(event, thread_id)

         if not success then
            if type(ret) != "table" or
               ret.errcode != sysbench.error.RESTART_EVENT
            then
               error(ret, 2) -- propagate unknown errors
            end
         end
      until success

      -- Stop the benchmark if event() returns a non-nil value
      if ret then
         break
      end

      ffi.C.sb_event_stop(thread_id)
   end
end

-- ----------------------------------------------------------------------
-- Hooks
-- ----------------------------------------------------------------------

sysbench.hooks = {
   -- sql_error = <func>,
}
