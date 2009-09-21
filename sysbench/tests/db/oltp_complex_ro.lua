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
   set_vars()

   if (db_driver == "mysql" and mysql_table_engine == "myisam") then
      begin_stmt = db_prepare("LOCK TABLES sbtest READ")
      commit_stmt = db_prepare("UNLOCK TABLES")
   else
      begin_stmt = db_prepare("BEGIN")
      commit_stmt = db_prepare("COMMIT")
   end

   point_stmt = db_prepare("SELECT c FROM sbtest WHERE id=?")
   point_params = {0}
   db_bind_param(point_stmt, point_params)

   range_stmt = db_prepare("SELECT c FROM sbtest WHERE id BETWEEN ? AND ?")
   range_params = {0,0}
   db_bind_param(range_stmt, range_params)
   
   sum_stmt = db_prepare("SELECT SUM(K) FROM sbtest WHERE id BETWEEN ? AND ?")
   sum_params = {0,0}
   db_bind_param(sum_stmt, sum_params)

   order_stmt= db_prepare("SELECT c FROM sbtest WHERE id BETWEEN ? AND ? ORDER BY c")
   order_params = {0,0}
   db_bind_param(order_stmt, order_params)

   distinct_stmt = db_prepare("SELECT DISTINCT c FROM sbtest WHERE id BETWEEN ? AND ? ORDER BY c")
   distinct_params = {0,0}
   db_bind_param(distinct_stmt, distinct_params)

end

function event(thread_id)
   local rs
   local i

   db_execute(begin_stmt)

   for i=1,10 do
      point_params[1] = sb_rand(1, oltp_table_size)
      rs = db_execute(point_stmt)
      db_store_results(rs)
      db_free_results(rs)
   end
   
   range_params[1] = sb_rand(1, oltp_table_size)
   range_params[2] = range_params[1] + oltp_range_size - 1
   rs = db_execute(range_stmt)
   db_store_results(rs)
   db_free_results(rs)
   
   sum_params[1] = sb_rand(1, oltp_table_size)
   sum_params[2] = sum_params[1] + oltp_range_size - 1
   rs = db_execute(sum_stmt)
   db_store_results(rs)
   db_free_results(rs)
   
   order_params[1] = sb_rand(1, oltp_table_size)
   order_params[2] = order_params[1] + oltp_range_size - 1
   rs = db_execute(order_stmt)
   db_store_results(rs)
   db_free_results(rs)
   
   distinct_params[1] = sb_rand(1, oltp_table_size)
   distinct_params[2] = distinct_params[1] + oltp_range_size - 1
   rs = db_execute(distinct_stmt)
   db_store_results(rs)
   db_free_results(rs)

   db_execute(commit_stmt)
   
end

function set_vars()
   oltp_table_size = oltp_table_size or 10000
   oltp_range_size = oltp_range_size or 100
   
   if (oltp_auto_inc == 'off') then
      oltp_auto_inc = false
   else
      oltp_auto_inc = true
   end
end
