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
	 db_bulk_insert_next("(0, ' ', 'qqqqqqqqqqwwwwwwwwwweeeeeeeeeerrrrrrrrrrtttttttttt')")
      else
	 db_bulk_insert_next("("..i..",0,' ','qqqqqqqqqqwwwwwwwwwweeeeeeeeeerrrrrrrrrrtttttttttt')")
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
   local query

   set_vars()
   
   stmt = db_prepare([[
			   SELECT pad FROM sbtest WHERE id = ?
		     ]])
   params = {0}
   db_bind_param(stmt, params)
end

function event(thread_id)
   local rs
   params[1] = sb_rand(1, oltp_table_size)
   rs = db_execute(stmt)
   db_store_results(rs)
   db_free_results(rs)
end

function set_vars()
   oltp_table_size = oltp_table_size or 10000

   if (oltp_auto_inc == 'off') then
      oltp_auto_inc = false
   else
      oltp_auto_inc = true
   end
end