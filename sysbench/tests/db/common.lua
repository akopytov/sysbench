-- Input parameters
-- oltp-tables-count - number of tables to create
-- oltp-secondary - use secondary key instead PRIMARY key for id column
--
--

function create_insert(table_id)

   local index_name
   local table_name
   local j
   local query

   if (oltp_secondary) then
     index_name = "KEY xid"
   else
     index_name = "PRIMARY KEY"
   end

   table_name = string.format("sbtest%d", table_id)

   print("Creating table '" .. table_name .. "'...")
   if ((db_driver == "mysql") or (db_driver == "attachsql")) then
      query = [[
CREATE TABLE ]] .. table_name .. [[ (
id INTEGER UNSIGNED NOT NULL ]] ..
((oltp_auto_inc and "AUTO_INCREMENT") or "") .. [[,
k INTEGER UNSIGNED DEFAULT '0' NOT NULL,
c CHAR(120) DEFAULT '' NOT NULL,
pad CHAR(60) DEFAULT '' NOT NULL,
]] .. index_name .. [[ (id)
) /*! ENGINE = ]] .. mysql_table_engine ..
" MAX_ROWS = " .. string.format("%d", myisam_max_rows) .. " */ " ..
   (mysql_table_options or "")

   elseif (db_driver == "pgsql") then
      query = [[
CREATE TABLE ]] .. table_name .. [[ (
id SERIAL NOT NULL,
k INTEGER DEFAULT '0' NOT NULL,
c CHAR(120) DEFAULT '' NOT NULL,
pad CHAR(60) DEFAULT '' NOT NULL,
]] .. index_name .. [[ (id)
) ]]

   elseif (db_driver == "drizzle") then
      query = [[
CREATE TABLE ]] .. table_name .. [[ (
id INTEGER NOT NULL ]] .. ((oltp_auto_inc and "AUTO_INCREMENT") or "") .. [[,
k INTEGER DEFAULT '0' NOT NULL,
c CHAR(120) DEFAULT '' NOT NULL,
pad CHAR(60) DEFAULT '' NOT NULL,
]] .. index_name .. [[ (id)
) ]]
   else
      print("Unknown database driver: " .. db_driver)
      return 1
   end

   db_query(query)

   db_query("CREATE INDEX k_" .. string.format("%d", table_id) .. " on " .. table_name .. "(k)")

   print("Inserting " .. oltp_table_size .. " records into '" .. table_name .. "'")

   if (oltp_auto_inc) then
      db_bulk_insert_init("INSERT INTO " .. table_name .. "(k, c, pad) VALUES")
   else
      db_bulk_insert_init("INSERT INTO " .. table_name .. "(id, k, c, pad) VALUES")
   end

   local c_val
   local pad_val


   for j = 1,oltp_table_size do

   c_val = sb_rand_str([[
###########-###########-###########-###########-###########-###########-###########-###########-###########-###########]])
   pad_val = sb_rand_str([[
###########-###########-###########-###########-###########]])

      if (oltp_auto_inc) then
	 db_bulk_insert_next(string.format("( %d, '%s', '%s')", sb_rand(1, oltp_table_size), c_val, pad_val))
      else
	 db_bulk_insert_next(string.format("( %d, %d, '%s', '%s')", j, sb_rand(1, oltp_table_size), c_val, pad_val))
      end
   end

   db_bulk_insert_done()


end


function prepare()
   local query
   local i
   local j

   set_vars()

   db_connect()


   for i = 1,oltp_tables_count do
     create_insert(i)
   end

   return 0
end

function cleanup()
   local i
   local table_name
   set_vars()

   for i = 1,oltp_tables_count do
   table_name = string.format("sbtest%d")
   print("Dropping table '" .. table_name .. "'...")
   db_query("DROP TABLE ".. table_name )
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

   if (oltp_skip_trx == 'on') then
      oltp_skip_trx = true
   else
      oltp_skip_trx = false
   end

end
