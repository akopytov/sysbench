-- Input parameters
-- oltp-tables-count - number of tables to create
-- oltp-secondary - use secondary key instead PRIMARY key for id column
--
--

function create_insert(table_id)

   local index_name
   local i
   local j
   local query

   if (oltp_secondary) then
     index_name = "KEY xid"
   else
     index_name = "PRIMARY KEY"
   end

   if (pgsql_variant == 'redshift') then
      auto_inc_type = "INTEGER IDENTITY(1,1)"
   else
      auto_inc_type = "SERIAL"
   end

   i = table_id

   print("Creating table 'sbtest" .. i .. "'...")
   if ((db_driver == "mysql") or (db_driver == "attachsql")) then
      query = [[
CREATE TABLE sbtest]] .. i .. [[ (
id INTEGER UNSIGNED NOT NULL ]] ..
((oltp_auto_inc and "AUTO_INCREMENT") or "") .. [[,
k INTEGER UNSIGNED DEFAULT '0' NOT NULL,
c CHAR(120) DEFAULT '' NOT NULL,
pad CHAR(60) DEFAULT '' NOT NULL,
]] .. index_name .. [[ (id)
) /*! ENGINE = ]] .. mysql_table_engine ..
" MAX_ROWS = " .. myisam_max_rows .. " */ " ..
   (mysql_table_options or "")

   elseif (db_driver == "pgsql") then
      query = [[
CREATE TABLE sbtest]] .. i .. [[ (
id ]] .. auto_inc_type .. [[ NOT NULL,
k INTEGER DEFAULT '0' NOT NULL,
c CHAR(120) DEFAULT '' NOT NULL,
pad CHAR(60) DEFAULT '' NOT NULL,
]] .. index_name .. [[ (id)
) ]]

   elseif (db_driver == "drizzle") then
      query = [[
CREATE TABLE sbtest (
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

   print("Inserting " .. oltp_table_size .. " records into 'sbtest" .. i .. "'")

   if (oltp_auto_inc) then
      db_bulk_insert_init("INSERT INTO sbtest" .. i .. "(k, c, pad) VALUES")
   else
      db_bulk_insert_init("INSERT INTO sbtest" .. i .. "(id, k, c, pad) VALUES")
   end

   local c_val
   local pad_val


   for j = 1,oltp_table_size do

   c_val = sb_rand_str([[
###########-###########-###########-###########-###########-###########-###########-###########-###########-###########]])
   pad_val = sb_rand_str([[
###########-###########-###########-###########-###########]])

      if (oltp_auto_inc) then
	 db_bulk_insert_next("(" .. sb_rand(1, oltp_table_size) .. ", '".. c_val .."', '" .. pad_val .. "')")
      else
	 db_bulk_insert_next("("..j.."," .. sb_rand(1, oltp_table_size) .. ",'".. c_val .."', '" .. pad_val .. "'  )")
      end
   end

   db_bulk_insert_done()

   if oltp_create_secondary then
     print("Creating secondary indexes on 'sbtest" .. i .. "'...")
     db_query("CREATE INDEX k_" .. i .. " on sbtest" .. i .. "(k)")
   end

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

   set_vars()

   for i = 1,oltp_tables_count do
   print("Dropping table 'sbtest" .. i .. "'...")
   db_query("DROP TABLE IF EXISTS sbtest".. i )
   end
end

function set_vars()
   oltp_table_size = tonumber(oltp_table_size) or 10000
   oltp_range_size = tonumber(oltp_range_size) or 100
   oltp_tables_count = tonumber(oltp_tables_count) or 1
   oltp_point_selects = tonumber(oltp_point_selects) or 10
   oltp_simple_ranges = tonumber(oltp_simple_ranges) or 1
   oltp_sum_ranges = tonumber(oltp_sum_ranges) or 1
   oltp_order_ranges = tonumber(oltp_order_ranges) or 1
   oltp_distinct_ranges = tonumber(oltp_distinct_ranges) or 1
   oltp_index_updates = tonumber(oltp_index_updates) or 1
   oltp_non_index_updates = tonumber(oltp_non_index_updates) or 1
   oltp_delete_inserts = tonumber(oltp_delete_inserts) or 1

   if (oltp_range_selects == 'off') then
      oltp_range_selects = false
   else
      oltp_range_selects = true
   end

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

   if (oltp_write_only == 'on') then
      oltp_write_only = true
   else
      oltp_write_only = false
   end

   if (oltp_read_only and oltp_write_only) then
      error("--oltp-read-only and --oltp-write-only are mutually exclusive")
   end

   if (oltp_skip_trx == 'on') then
      oltp_skip_trx = true
   else
      oltp_skip_trx = false
   end

   if (oltp_create_secondary == 'off') then
      oltp_create_secondary = false
   else
      oltp_create_secondary = true
   end

   if (pgsql_variant == 'redshift') then
      oltp_create_secondary = false
      oltp_delete_inserts = 0
   end

end
