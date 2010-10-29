-- This test is designed for testing MariaDB's key_cache_segments for MyISAM,
-- and should work with other storage engines as well.
--
-- For details about key_cache_segments please refer to:
-- http://kb.askmonty.org/v/segmented-key-cache
--
function prepare()
   local query
   local i

   set_vars()

   db_connect()

   print("Creating table 'sbtest'...")

   if (db_driver == "mysql") then
      query = [[
        CREATE TABLE sbtest (
          id INTEGER UNSIGNED NOT NULL ]] .. ((oltp_auto_inc and "AUTO_INCREMENT") or "") .. [[,
          k INTEGER UNSIGNED DEFAULT '0' NOT NULL,
          c CHAR(120) DEFAULT '' NOT NULL,
          pad CHAR(60) DEFAULT '' NOT NULL,
          PRIMARY KEY (id)
        ) /*! ENGINE = ]] .. mysql_table_engine .. " MAX_ROWS = " .. myisam_max_rows .. " */"

   elseif (db_driver == "oracle") then
      query = [[
        CREATE TABLE sbtest (
          id INTEGER NOT NULL,
          k INTEGER,
          c CHAR(120) DEFAULT '' NOT NULL,
          pad CHAR(60 DEFAULT '' NOT NULL,
          PRIMARY KEY (id)
       ) ]]
    

   elseif (db_driver == "pgsql") then
      query = [[
        CREATE TABLE sbtest (
          id ]] .. (sb.oltp_auto_inc and "SERIAL") or "" .. [[,
          k INTEGER DEFAULT '0' NOT NULL,
          c CHAR(120) DEFAULT '' NOT NULL,
          pad CHAR(60) DEFAULT '' NOT NULL, 
          PRIMARY KEY (id)
        ) ]]

   elseif (db_driver == "drizzle") then
      query = [[
        CREATE TABLE sbtest (
          id INTEGER NOT NULL ]] .. ((oltp_auto_inc and "AUTO_INCREMENT") or "") .. [[,
          k INTEGER DEFAULT '0' NOT NULL,
          c CHAR(120) DEFAULT '' NOT NULL,
          pad CHAR(60) DEFAULT '' NOT NULL,
          PRIMARY KEY (id)
        ) ]]

   else
      print("Unknown database driver: " .. db_driver)
      return 1
   end

   db_query(query)

   if (db_driver == "oracle") then
      db_query("CREATE SEQUENCE sbtest_seq")
      db_query([[CREATE TRIGGER sbtest_trig BEFORE INSERT ON sbtest 
                 FOR EACH ROW BEGIN SELECT sbtest_seq.nextval INTO :new.id FROM DUAL; END;]])
   end

   db_query("CREATE INDEX k on sbtest(k)")

   print("Inserting " .. oltp_table_size .. " records into 'sbtest'")
   
   if (oltp_auto_inc) then
      db_bulk_insert_init("INSERT INTO sbtest(k, c, pad) VALUES")
   else
      db_bulk_insert_init("INSERT INTO sbtest(id, k, c, pad) VALUES")
   end

   for i = 1,oltp_table_size do
      if (oltp_auto_inc) then
          db_bulk_insert_next("("..i..", ' ', 'qqqqqqqqqqwwwwwwwwwweeeeeeeeeerrrrrrrrrrtttttttttt')")
      else
          db_bulk_insert_next("("..i..", "..i..", ' ', 'qqqqqqqqqqwwwwwwwwwweeeeeeeeeerrrrrrrrrrtttttttttt')")
      end
   end

   db_bulk_insert_done()

   return 0
end

function cleanup()
   print("Dropping table 'sbtest'...")
   db_query("DROP TABLE sbtest")
end

function thread_init(thread_id)
   set_vars()

   ranges = ""
   for i = 1,number_of_ranges do
      ranges = ranges .. "k BETWEEN ? AND ? OR "
   end
   
   -- Get rid of last OR and space.
   ranges = string.sub(ranges, 1, string.len(ranges) - 3)

   stmt = db_prepare([[
        SELECT count(k)
          FROM sbtest
          WHERE ]] .. ranges .. [[
        ]])

   params = {}
   for j = 1,number_of_ranges * 2 do
      params[j] = 1
   end

   db_bind_param(stmt, params)

end

function event(thread_id)
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

function set_vars()
   oltp_table_size = oltp_table_size or 10000
   number_of_ranges = number_of_ranges or 10
   delta = random_ranges_delta or 5

   if (oltp_auto_inc == 'off') then
      oltp_auto_inc = false
   else
      oltp_auto_inc = true
   end
end
