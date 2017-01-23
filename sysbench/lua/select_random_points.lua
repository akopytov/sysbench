-- This test is designed for testing MariaDB's key_cache_segments for MyISAM,
-- and should work with other storage engines as well.
--
-- For details about key_cache_segments please refer to:
-- http://kb.askmonty.org/v/segmented-key-cache
--

-- Override oltp_tables_count, this test only supports a single table
oltp_tables_count = 1

require("oltp_common")

function thread_init()
   set_vars_points()

   drv = sysbench.sql.driver()
   con = drv:connect()

   local points = string.rep("?, ", random_points - 1) .. "?"

   stmt = con:prepare(string.format([[
        SELECT id, k, c, pad
          FROM sbtest1
          WHERE k IN (%s)
        ]], points))

   params = {}
   for j = 1,random_points do
      params[j] = stmt:bind_create(sysbench.sql.type.INT)
   end

   stmt:bind_param(unpack(params))

   rlen = oltp_table_size / num_threads
end

function event(thread_id)
   -- To prevent overlapping of our range queries we need to partition the whole
   -- table into num_threads segments and then make each thread work with its
   -- own segment.
   for i = 1,random_points do
      local rmin = rlen * thread_id
      local rmax = rmin + rlen
      params[i]:set(sb_rand(rmin, rmax))
   end

   stmt:execute()
end

function set_vars_points()
   set_vars()
   random_points = random_points or 10
end
