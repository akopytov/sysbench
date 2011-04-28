pathtest = string.match(test, "(.*/)") or ""

dofile(pathtest .. "common.lua")

function thread_init(thread_id)
   set_vars()

   if (db_driver == "mysql" and mysql_table_engine == "myisam") then
      begin_query = "LOCK TABLES sbtest WRITE"
      commit_query = "UNLOCK TABLES"
   else
      begin_query = "BEGIN"
      commit_query = "COMMIT"
   end

end

function event(thread_id)
   local rs
   local i
   local table_name
   local range_start
   local c_val
   local pad_val
   local query

   table_name = "sbtest".. sb_rand_uniform(1, oltp_tables_count)
   db_query(begin_query)

   for i=1, oltp_point_selects do
      rs = db_query("SELECT c FROM ".. table_name .." WHERE id=" .. sb_rand(1, oltp_table_size))
   end

   for i=1, oltp_simple_ranges do
      range_start = sb_rand(1, oltp_table_size)
      rs = db_query("SELECT c FROM ".. table_name .." WHERE id BETWEEN " .. range_start .. " AND " .. range_start .. "+" .. oltp_range_size - 1)
   end
  
   for i=1, oltp_sum_ranges do
      range_start = sb_rand(1, oltp_table_size)
      rs = db_query("SELECT SUM(K) FROM ".. table_name .." WHERE id BETWEEN " .. range_start .. " AND " .. range_start .. "+" .. oltp_range_size - 1)
   end
   
   for i=1, oltp_order_ranges do
      range_start = sb_rand(1, oltp_table_size)
      rs = db_query("SELECT c FROM ".. table_name .." WHERE id BETWEEN " .. range_start .. " AND " .. range_start .. "+" .. oltp_range_size - 1 .. " ORDER BY c")
   end

   for i=1, oltp_distinct_ranges do
      range_start = sb_rand(1, oltp_table_size)
      rs = db_query("SELECT DISTINCT c FROM ".. table_name .." WHERE id BETWEEN " .. range_start .. " AND " .. range_start .. "+" .. oltp_range_size - 1 .. " ORDER BY c")
   end

   if not oltp_read_only then

   for i=1, oltp_index_updates do
      rs = db_query("UPDATE " .. table_name .. " SET k=k+1 WHERE id=" .. sb_rand(1, oltp_table_size))
   end

   for i=1, oltp_non_index_updates do
      c_val = sb_rand_str("###########-###########-###########-###########-###########-###########-###########-###########-###########-###########")
      query = "UPDATE " .. table_name .. " SET c='" .. c_val .. "' WHERE id=" .. sb_rand(1, oltp_table_size)
      rs = db_query(query)
      if rs then
        print(query)
      end
   end

   i = sb_rand(1, oltp_table_size)

   rs = db_query("DELETE FROM " .. table_name .. " WHERE id=" .. i)
   
   c_val = sb_rand_str([[
###########-###########-###########-###########-###########-###########-###########-###########-###########-###########]])
   pad_val = sb_rand_str([[
###########-###########-###########-###########-###########]])

   rs = db_query("INSERT INTO " .. table_name ..  " (id, k, c, pad) VALUES " .. string.format("(%d, %d, '%s', '%s')",i, sb_rand(1, oltp_table_size) , c_val, pad_val))

   end -- oltp_read_only

   db_query(commit_query)

end

