-- Copyright (C) 2016-2018 Alexey Kopytov <akopytov@gmail.com>

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

double sb_rand_uniform_double(void);
static double sb_rand_exp(double lambda, double uniform_parameter)
{
  return -lambda * log(1 - uniform_parameter);
}

typedef struct timeval {
                long tv_sec;
                long tv_usec;
           } timeval;

int gettimeofday(struct timeval* t, void* tzp);
int usleep(unsigned int usec);
]]

gettimeofday_struct = ffi.new("struct timeval")
local function gettimeofday()
        ffi.C.gettimeofday(gettimeofday_struct, nil)
        return tonumber(gettimeofday_struct.tv_sec) * 1000000 + tonumber(gettimeofday_struct.tv_usec)
end

-- ----------------------------------------------------------------------
-- This event loop can work with muliple groups of threads.
-- For that purpose global variable 'thread_groups' should
-- be defined in benchmark script.end
-- thread_groups = {{id=i, event=func1, rate_controller=func2, rate=k, thread_amount=n}, {}, ...}
-- id is any number or string.
-- func1 is function as event(), func2 is functions witch controls sleeps period (optional),
-- n is amount of threads in the group (im sum of groups threads_amount less then in --threads
-- than all other threads go to the last group)
-- rate is integer corresponding rate of the thread
-- ----------------------------------------------------------------------
function thread_run(thread_id)
   while ffi.C.sb_more_events(thread_id) do
      ffi.C.sb_event_start(thread_id)

      local event_func = nil
      local rate_controller_func = nil
      local rate = 0
      local acc = 0
      for i = 1, #thread_groups
      do
         acc = acc + thread_groups[i].thread_amount
         if thread_id < acc
         then
            event_func           = thread_groups[i].event
            rate_controller_func = thread_groups[i].rate_controller
            rate                 = thread_groups[i].rate
            break
         end

        -- if total amount of threads more then sum of amounts in groups
        -- all other threads rely to the last group
         if i == #thread_groups
         then
            event_func           = thread_groups[i].event
            rate_controller_func = thread_groups[i].rate_controller
            rate                 = thread_groups[i].rate
         end
      end

      local success, ret
      repeat
            success, ret = pcall(event_func, thread_id)

         if not success then
            if type(ret) == "table" and
               ret.errcode == sysbench.error.RESTART_EVENT
            then
               if sysbench.hooks.before_restart_event then
                  sysbench.hooks.before_restart_event(ret)
               end
            else
               error(ret, 2) -- propagate unknown errors
            end
         end
      until success

      -- Stop the benchmark if event() returns a value other than nil or false
      if ret then
         break
      end
      ffi.C.sb_event_stop(thread_id)

      -- It supposed that rete controller return execution flow after some pause
      if rate_controller_func ~= nil and rate > 0
      then
         pcall(rate_controller_func, rate)
      end


   end
end



-- This function controls rate of thread's events in that way:
-- It stores the last time it was called and calculate random
-- exponentially distributed interval to the next event (with median 1sec/rate).
-- When it called again it measure time and if it passed less time then 'inteval'
-- it sleeps for the rest part of interval.
function default_rate_controller(rate)

    local Tcur = gettimeofday()
    local lambda = 1e6 / rate

    local uniform_parameter = ffi.C.sb_rand_uniform_double()
    local interval = -lambda * math.log(uniform_parameter)

    if Tnext == nil
    then
       Tnext = Tcur + interval
    else
       Tnext = Tnext + interval
       if Tnext > Tcur
       then
          ffi.C.usleep(Tnext - Tcur);
       end
    end
end