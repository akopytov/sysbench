-- This test is designed for testing MariaDB's key_cache_segments for MyISAM,
-- and should work with other storage engines as well.
--
-- For details about key_cache_segments please refer to:
-- http://kb.askmonty.org/v/segmented-key-cache
--

pathtest = string.match(test, "(.*/)")

if pathtest then
   dofile(pathtest .. "common.lua")
else
   require("common")
end

-- Override oltp_tables_count, this test only supports a single table
oltp_tables_count = 1

function thread_init()
   set_vars_ranges()

   ranges = ""
   for i = 1,number_of_ranges do
      ranges = ranges .. "k BETWEEN ? AND ? OR "
   end
   
   -- Get rid of last OR and space.
   ranges = string.sub(ranges, 1, string.len(ranges) - 3)

   stmt = db_prepare([[
        SELECT count(k)
          FROM sbtest1
          WHERE ]] .. ranges .. [[
        ]])

   params = {}
   for j = 1,number_of_ranges * 2 do
      params[j] = 1
   end

   db_bind_param(stmt, params)

end

function event()
   local rs

   -- To prevent overlapping of our range queries we need to partition the whole table
   -- into num_threads segments and then make each thread work with its own segment.
   for i = 1,number_of_ranges * 2,2 do
      params[i] = sb_rand(oltp_table_size / num_threads * thread_id, oltp_table_size / num_threads * (thread_id + 1))
      params[i + 1] = params[i] + delta
   end

   rs = db_execute(stmt)
   db_store_results(rs)
   db_free_results(rs)
end

function set_vars_ranges()
   set_vars()
   number_of_ranges = number_of_ranges or 10
   delta = random_ranges_delta or 5
end
