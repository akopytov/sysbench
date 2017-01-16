-- This test is designed for testing MariaDB's key_cache_segments for MyISAM,
-- and should work with other storage engines as well.
--
-- For details about key_cache_segments please refer to:
-- http://kb.askmonty.org/v/segmented-key-cache
--

-- Override oltp_tables_count, this test only supports a single table
oltp_tables_count = 1

pathtest = string.match(test, "(.*/)")

if pathtest then
   dofile(pathtest .. "oltp_common.lua")
else
   require("oltp_common")
end

function thread_init()
   set_vars_ranges()

   drv = sysbench.sql.driver()
   con = drv:connect()

   local ranges = string.rep("k BETWEEN ? AND ? OR ", number_of_ranges - 1) ..
      "k BETWEEN ? AND ?"

   stmt = con:prepare(string.format([[
        SELECT count(k)
          FROM sbtest1
          WHERE %s]], ranges))

   params = {}
   for j = 1, number_of_ranges*2 do
      params[j] = stmt:bind_create(sysbench.sql.type.INT)
   end

   stmt:bind_param(unpack(params))

   rlen = oltp_table_size / num_threads
end

function event(thread_id)
   -- To prevent overlapping of our range queries we need to partition the whole
   -- table into num_threads segments and then make each thread work with its
   -- own segment.
   for i = 1, number_of_ranges*2, 2 do
      local rmin = rlen * thread_id
      local rmax = rmin + rlen
      local val = sb_rand(rmin, rmax)
      params[i]:set(val)
      params[i+1]:set(val + delta)
   end

   stmt:execute()
end

function set_vars_ranges()
   set_vars()
   number_of_ranges = number_of_ranges or 10
   delta = random_ranges_delta or 5
end
