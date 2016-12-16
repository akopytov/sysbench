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

   table_name = "sbtest".. sb_rand_uniform(1, oltp_tables_count)

   point_stmt = db_prepare("SELECT c FROM ".. table_name .." WHERE id = ?")
   simple_range_stmt = db_prepare("SELECT c FROM ".. table_name .." WHERE id BETWEEN ? AND ?")
   sum_range_stmt = db_prepare("SELECT SUM(K) FROM ".. table_name .." WHERE id BETWEEN ? AND ?")
   order_range_stmt = db_prepare("SELECT c FROM ".. table_name .." WHERE id BETWEEN ? AND ? ORDER BY c")
   distinct_range_stmt = db_prepare("SELECT DISTINCT c FROM ".. table_name .." WHERE id BETWEEN ? AND ? ORDER BY c")
   index_update_stmt = db_prepare("UPDATE " .. table_name .. " SET k = k + 1 WHERE id = ?")
   non_index_update_stmt = db_prepare("UPDATE " .. table_name .. " SET c = ? WHERE id = ?")
   delete_stmt = db_prepare("DELETE FROM " .. table_name .. " WHERE id = ?")
   insert_stmt = db_prepare("INSERT INTO " .. table_name ..  " (id, k, c, pad) VALUES (?, ?, ?, ?)")

   point_params = {}
   point_params[1] = 1
   range_params = {1, 1}
   non_index_update_params = {"###########-###########-###########-###########-###########-###########-###########-###########-###########-###########", 1}
   insert_params = {1, 1, "###########-###########-###########-###########-###########-###########-###########-###########-###########-###########", "###########-###########-###########-###########-###########"}
   db_bind_param(point_stmt, point_params)
   db_bind_param(simple_range_stmt, range_params)
   db_bind_param(sum_range_stmt, range_params)
   db_bind_param(order_range_stmt, range_params)
   db_bind_param(distinct_range_stmt, range_params)
   db_bind_param(index_update_stmt, point_params)
   db_bind_param(non_index_update_stmt, non_index_update_params)
   db_bind_param(delete_stmt, point_params)
   db_bind_param(insert_stmt, insert_params)
end

function event(thread_id)
   local rs
   local i
   local range_start
   local c_val
   local pad_val
   local query

   if not oltp_skip_trx then
      db_query(begin_query)
   end

   for i=1, oltp_point_selects do
      point_params[1] = sb_rand(1, oltp_table_size)
      rs = db_execute(point_stmt)
      db_free_results(rs)
   end

   for i=1, oltp_simple_ranges do
      range_params[1] = sb_rand(1, oltp_table_size)
      range_params[2] = range_params[1] + oltp_range_size
      rs = db_execute(simple_range_stmt)
      db_free_results(rs)
   end
  
   for i=1, oltp_sum_ranges do
      range_params[1] = sb_rand(1, oltp_table_size)
      range_params[2] = range_params[1] + oltp_range_size
      rs = db_execute(sum_range_stmt)
      db_free_results(rs)
   end
   
   for i=1, oltp_order_ranges do
      range_params[1] = sb_rand(1, oltp_table_size)
      range_params[2] = range_params[1] + oltp_range_size
      rs = db_execute(order_range_stmt)
      db_free_results(rs)
   end

   for i=1, oltp_distinct_ranges do
      range_params[1] = sb_rand(1, oltp_table_size)
      range_params[2] = range_params[1] + oltp_range_size
      rs = db_execute(distinct_range_stmt)
      db_free_results(rs)
   end

   if not oltp_read_only then

      for i=1, oltp_index_updates do
         point_params[1] = sb_rand(1, oltp_table_size)
         rs = db_execute(index_update_stmt)
      end

      for i=1, oltp_non_index_updates do
         non_index_update_params[1] = sb_rand_str("###########-###########-###########-###########-###########-###########-###########-###########-###########-###########")
         non_index_update_params[2] = sb_rand(1, oltp_table_size)
         rs = db_execute(non_index_update_stmt)
         --prints table: 0x... if enabled
		 --if rs then
           --print(non_index_update_params)
         --end
      end

      point_params[1] = sb_rand(1, oltp_table_size)
      rs = db_execute(delete_stmt)

      insert_params[1] = point_params[1]
      insert_params[2] = sb_rand(1, oltp_table_size)
      insert_params[3] = sb_rand_str([[
###########-###########-###########-###########-###########-###########-###########-###########-###########-###########]])
      insert_params[4] = sb_rand_str([[
###########-###########-###########-###########-###########]])

      rs = db_execute(insert_stmt)
   end -- oltp_read_only

   if not oltp_skip_trx then
      db_query(commit_query)
   end

end

