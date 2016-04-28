pathtest = string.match(test, "(.*/)") or ""

dofile(pathtest .. "common.lua")

function thread_init(thread_id)
   set_vars()

   if (((db_driver == "mysql") or (db_driver == "attachsql")) and mysql_table_engine == "myisam") then
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
   local id
   local table_name
   local range_start
   local c_val
   local pad_val
   local query

   table_name = string.format("sbtest%d", sb_rand_uniform(1, oltp_tables_count))

   if not oltp_skip_trx then
      db_query(begin_query)
   end

   for i=1, oltp_point_selects do
      id = sb_rand(1, oltp_table_size)
      rs = db_query(string.format("SELECT c FROM %s WHERE id=%d", table_name, id))
   end

   for i=1, oltp_simple_ranges do
      range_start = string.format("%d", sb_rand(1, oltp_table_size))
      rs = db_query("SELECT c FROM ".. table_name .." WHERE id BETWEEN " .. range_start .. " AND " .. range_start .. "+" .. oltp_range_size - 1)
   end
  
   for i=1, oltp_sum_ranges do
      range_start = string.format("%d", sb_rand(1, oltp_table_size))
      rs = db_query("SELECT SUM(K) FROM ".. table_name .." WHERE id BETWEEN " .. range_start .. " AND " .. range_start .. "+" .. oltp_range_size - 1)
   end
   
   for i=1, oltp_order_ranges do
      range_start = string.format("%d", sb_rand(1, oltp_table_size))
      rs = db_query("SELECT c FROM ".. table_name .." WHERE id BETWEEN " .. range_start .. " AND " .. range_start .. "+" .. oltp_range_size - 1 .. " ORDER BY c")
   end

   for i=1, oltp_distinct_ranges do
      range_start = string.format("%d", sb_rand(1, oltp_table_size))
      rs = db_query("SELECT DISTINCT c FROM ".. table_name .." WHERE id BETWEEN " .. range_start .. " AND " .. range_start .. "+" .. oltp_range_size - 1 .. " ORDER BY c")
   end

   if not oltp_read_only then

   for i=1, oltp_index_updates do
      id = sb_rand(1, oltp_table_size)
      rs = db_query(string.format("UPDATE %s SET k=k+1 WHERE id=%d", table_name, id))
   end

   for i=1, oltp_non_index_updates do
      c_val = sb_rand_str("###########-###########-###########-###########-###########-###########-###########-###########-###########-###########")
      query = string.format("UPDATE %s SET c='%s' WHERE id=%d", table_name, c_val, sb_rand(1, oltp_table_size))
      rs = db_query(query)
      if rs then
        print(query)
      end
   end

   i = sb_rand(1, oltp_table_size)

   rs = db_query(string.format("DELETE FROM %s WHERE id=%d", table_name, i))
   
   c_val = sb_rand_str([[
###########-###########-###########-###########-###########-###########-###########-###########-###########-###########]])
   pad_val = sb_rand_str([[
###########-###########-###########-###########-###########]])

   rs = db_query(string.format("INSERT INTO %s (id, k, c, pad) VALUES (%d, %d, '%s', '%s')", table_name, i, sb_rand(1, oltp_table_size), c_val, pad_val))

   end -- oltp_read_only

   if not oltp_skip_trx then
      db_query(commit_query)
   end

end

