pathtest = string.match(test, "(.*/)")

if pathtest then
   dofile(pathtest .. "common.lua")
else
   require("common")
end

function thread_init(thread_id)
   set_vars()

   if (((db_driver == "mysql") or (db_driver == "attachsql")) and mysql_table_engine == "myisam") then
      local i
      local tables = {}
      for i=1, oltp_tables_count do
         tables[i] = string.format("sbtest%i WRITE", i)
      end
      begin_query = "LOCK TABLES " .. table.concat(tables, " ,")
      commit_query = "UNLOCK TABLES"
   else
      begin_query = "BEGIN"
      commit_query = "COMMIT"
   end

end

function get_range_str()
   local start = sb_rand(1, oltp_table_size)
   return string.format(" WHERE id BETWEEN %u AND %u",
                        start, start + oltp_range_size - 1)
end

function event(thread_id)
   local rs
   local i
   local table_name
   local c_val
   local pad_val
   local query

   table_name = "sbtest".. sb_rand_uniform(1, oltp_tables_count)
   if not oltp_skip_trx then
      db_query(begin_query)
   end

   if not oltp_write_only then

   for i=1, oltp_point_selects do
      rs = db_query("SELECT c FROM ".. table_name .." WHERE id=" ..
                       sb_rand(1, oltp_table_size))
   end

   if oltp_range_selects then

   for i=1, oltp_simple_ranges do
      rs = db_query("SELECT c FROM ".. table_name .. get_range_str())
   end

   for i=1, oltp_sum_ranges do
      rs = db_query("SELECT SUM(K) FROM ".. table_name .. get_range_str())
   end

   for i=1, oltp_order_ranges do
      rs = db_query("SELECT c FROM ".. table_name .. get_range_str() ..
                    " ORDER BY c")
   end

   for i=1, oltp_distinct_ranges do
      rs = db_query("SELECT DISTINCT c FROM ".. table_name .. get_range_str() ..
                    " ORDER BY c")
   end

   end

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

   for i=1, oltp_delete_inserts do

   i = sb_rand(1, oltp_table_size)

   rs = db_query("DELETE FROM " .. table_name .. " WHERE id=" .. i)
   
   c_val = sb_rand_str([[
###########-###########-###########-###########-###########-###########-###########-###########-###########-###########]])
   pad_val = sb_rand_str([[
###########-###########-###########-###########-###########]])
   
   oltp_auto_inc = true
   
   if (oltp_auto_inc) then
      rs = db_query("INSERT INTO " .. table_name ..  " (k, c, pad) VALUES " .. string.format("(%d, '%s', '%s')",sb_rand(1, oltp_table_size) , c_val, pad_val))
   else
      rs = db_query("INSERT INTO " .. table_name ..  " (id, k, c, pad) VALUES " .. string.format("(%d, %d, '%s', '%s')",i, sb_rand(1, oltp_table_size) , c_val, pad_val))
   end

   end

   end -- oltp_read_only

   if not oltp_skip_trx then
      db_query(commit_query)
   end

end

