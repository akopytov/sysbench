-- Copyright (C) 2016 Percona

function insert_bulk(table_id)
   local i
   local j
   local query

   i = table_id
   print("Creating index on sbtest" .. i .. "(k)" )
   mongodb_create_index("sbtest" .. i, "k")

   print("Inserting " .. oltp_table_size .. " documents into 'sbtest" .. i .. "'")
   local c_val
   local pad_val
   local bulk_pos = 0
   for j = 1,oltp_table_size do

    c_val = sb_rand_str([[
 ###########-###########-###########-###########-###########-###########-###########-###########-###########-###########]])
    pad_val = sb_rand_str([[
 ###########-###########-###########-###########-###########]])
    if bulk_pos >= oltp_bulk_size then
       bulk_pos = 0
       mongodb_bulk_execute()
    end
    bulk_pos = bulk_pos + 1
    mongodb_bulk_insert("sbtest" .. i, j, sb_rand(1, oltp_table_size), c_val, pad_val)
   end
   mongodb_bulk_execute()
end

function insert(table_id)
   local i
   local j
   local query

   i = table_id
   print("Creating index on sbtest" .. i .. "(k)" )
   mongodb_create_index("sbtest" .. i, "k")

   print("Inserting " .. oltp_table_size .. " documents into 'sbtest" .. i .. "'")
   local c_val
   local pad_val
   local bulk_pos = 0
   for j = 1,oltp_table_size do

    c_val = sb_rand_str([[
 ###########-###########-###########-###########-###########-###########-###########-###########-###########-###########]])
    pad_val = sb_rand_str([[
 ###########-###########-###########-###########-###########]])
    if not mongodb_insert("sbtest" .. i, j, sb_rand(1, oltp_table_size), c_val, pad_val) then return end
   end
end

function prepare()
   set_vars()
   db_connect()
   for i = 1,oltp_tables_count do
     insert_bulk(i)
   end
end

function cleanup()
   set_vars()
   db_connect()
   for i = 1,oltp_tables_count do
    print("Dropping collection 'sbtest" .. i .. "'...")
    mongodb_drop_collection("sbtest" .. i)
   end
end

function set_vars()
   oltp_table_size = oltp_table_size or 10000
   oltp_range_size = oltp_range_size or 100
   oltp_tables_count = oltp_tables_count or 1
   oltp_point_selects = oltp_point_selects or 10
   oltp_simple_ranges = oltp_simple_ranges or 1
   oltp_sum_ranges = oltp_sum_ranges or 1
   oltp_order_ranges = oltp_order_ranges or 1
   oltp_distinct_ranges = oltp_distinct_ranges or 1
   oltp_index_updates = oltp_index_updates or 1
   oltp_non_index_updates = oltp_non_index_updates or 1
   oltp_inserts = oltp_inserts or 1
   oltp_bulk_size = oltp_bulk_size or 1000

   if (oltp_auto_inc == 'off') then
      oltp_auto_inc = false
   else
      oltp_auto_inc = true
   end

   if (oltp_read_only == 'on') then
      oltp_read_only = true
   else
      oltp_read_only = false
   end

end

