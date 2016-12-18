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
   dofile(pathtest .. "common.lua")
else
   require("common")
end

function thread_init()
   set_vars_points()

   points = ""
   for i = 1,random_points do
      points = points .. "?, "
   end
   
   -- Get rid of last comma and space.
   points = string.sub(points, 1, string.len(points) - 2)
   
   stmt = db_prepare([[
        SELECT id, k, c, pad
          FROM sbtest1
          WHERE k IN (]] .. points .. [[)
        ]])

   params = {}
   for j = 1,random_points do
      params[j] = 1
   end

   db_bind_param(stmt, params)
end

function event()
   local rs

   -- To prevent overlapping of our range queries we need to partition the whole table
   -- into num_threads segments and then make each thread work with its own segment.
   for i = 1,random_points do
      params[i] = sb_rand(oltp_table_size / num_threads * thread_id, oltp_table_size / num_threads * (thread_id + 1))
   end

   rs = db_execute(stmt)
   db_store_results(rs)
   db_free_results(rs)
end

function set_vars_points()
set_vars()
random_points = random_points or 10
end
