create_indexes_before_dataload =
   {"Create all imdexes before loading data. This can be useful when in the need to avoid the operation with table filled", false}
   

      
}

-- Prepare the dataset. This command supports parallel execution, i.e. will
-- benefit from executing with --threads > 1 as long as --tables > 1
function cmd_prepare()
   local drv = sysbench.sql.driver()
   local con = drv:connect()

   for i = (sysbench.tid % sysbench.opt.threads + 1) + sysbench.opt.from_table, sysbench.opt.tables,
   sysbench.opt.threads do
     create_table(drv, con, i)
   end
end

-- Preload the dataset into the server cache. This command supports parallel
-- execution, i.e. will benefit from executing with --threads > 1 as long as
-- --tables > 1
--
-- PS. Currently, this command is only meaningful for MySQL/InnoDB benchmarks
function cmd_warmup()
   local drv = sysbench.sql.driver()
   local con = drv:connect()

   assert(drv:name() == "mysql", "warmup is currently MySQL only")

   -- Do not create on disk tables for subsequent queries
   con:query("SET tmp_table_size=2*1024*1024*1024")
   con:query("SET max_heap_table_size=2*1024*1024*1024")

   for i = sysbench.tid % sysbench.opt.threads + 1, sysbench.opt.tables,
   sysbench.opt.threads do
      local t = sysbench.opt.table_name .. i
      print("Preloading table " .. t)
      con:query("ANALYZE TABLE ".. t)
      con:query(string.format(
                   "SELECT AVG(id) FROM " ..
                      "(SELECT * FROM %s FORCE KEY (PRIMARY) " ..
                      "LIMIT %u) t",
                   t, sysbench.opt.table_size))
      con:query(string.format(
                   "SELECT COUNT(*) FROM " ..
                      "(SELECT * FROM %s WHERE kwatts_s LIKE '%%0%%' LIMIT %u) t",
                   t, sysbench.opt.table_size))
   end
end

-- Implement parallel prepare and warmup commands, define 'prewarm' as an alias
-- for 'warmup'
sysbench.cmdline.commands = {
   prepare = {cmd_prepare, sysbench.cmdline.PARALLEL_COMMAND},
   warmup = {cmd_warmup, sysbench.cmdline.PARALLEL_COMMAND},
   prewarm = {cmd_warmup, sysbench.cmdline.PARALLEL_COMMAND}
}


-- Template strings of random digits with 11-digit groups separated by dashes

-- 3 characters
local c_value_template = "@@@"

-- 5 groups, 59 characters
local pad_value_template = "###########-###########-###########-" ..
   "###########-###########"

function get_c_value()
   return sysbench.rand.string(c_value_template)
end

function get_pad_value()
   return sysbench.rand.string(pad_value_template)
end

function create_indexes(drv, con, table_num)
   if sysbench.opt.create_secondary then
	  print(string.format("Creating a secondary index on '%s%d'...",
						  sysbench.opt.table_name,table_num))
					
	  con:query(string.format("CREATE INDEX kuuid_x ON %s%d(uuid)",
							  sysbench.opt.table_name,table_num, table_num))
	  con:query(string.format("CREATE INDEX millid_x ON %s%d(millid)",
							  sysbench.opt.table_name,table_num, table_num))
	  con:query(string.format("CREATE INDEX active_x ON %s%d(active)",
							  sysbench.opt.table_name,table_num, table_num))
						  
   end
   if sysbench.opt.create_compound then
	  print(string.format("Creating a compound index on '%s%d'...",
						  sysbench.opt.table_name,table_num))
					  
	  con:query(string.format("CREATE INDEX IDX_millid ON %s%d(`millid`,`active`)",
							  sysbench.opt.table_name,table_num, table_num))                    

	  con:query(string.format("CREATE INDEX IDX_active ON %s%d(`id`,`active`)",
							  sysbench.opt.table_name,table_num, table_num))                    

	  con:query(string.format("CREATE INDEX kcontinent_x ON %s%d(`continent`,`id`)",
							  sysbench.opt.table_name,table_num, table_num))                    

   end

end

function create_table(drv, con, table_num)
   local id_index_def, id_def
   local engine_def = ""
   local extra_table_options = ""
   local query

   if sysbench.opt.secondary then
     id_index_def = "KEY xid"
   else
     id_index_def = "PRIMARY KEY"
   end

   if drv:name() == "mysql" or drv:name() == "attachsql" or
      drv:name() == "drizzle"
   then
      if sysbench.opt.auto_inc then
         id_def = "BIGINT(11) NOT NULL AUTO_INCREMENT"
      else
         id_def = "BIGINT NOT NULL"
      end
      engine_def = "/*! ENGINE = " .. sysbench.opt.mysql_storage_engine .. " */"
      extra_table_options = sysbench.opt.mysql_table_options or ""
   elseif drv:name() == "pgsql"
   then
      if not sysbench.opt.auto_inc then
         id_def = "INTEGER NOT NULL"
      elseif pgsql_variant == 'redshift' then
        id_def = "INTEGER IDENTITY(1,1)"
      else
        id_def = "SERIAL"
      end
   else
      error("Unsupported database driver:" .. drv:name())
   end

   print(string.format("Creating table '%s%d'...", sysbench.opt.table_name,table_num))
   
   --print("DEBUG TABLE OPTION" .. sysbench.opt.mysql_table_options)
   
   con:query(string.format([[DROP TABLE IF EXISTS %s%d]],sysbench.opt.table_name, table_num))
   
   local primaryKeyDefinition = ", PRIMARY KEY (`id`)"
   
   if sysbench.opt.no_primary_key and not sysbench.opt.auto_inc then
      primaryKeyDefinition = ""
   end
   
   query = string.format([[   
   CREATE TABLE `%s%d` (
  `id` %s,
  `uuid` char(36) NOT NULL,
  `millid` smallint(6) NOT NULL,
  `kwatts_s` int(11) NOT NULL,
  `date` date NOT NULL ,
  `location` varchar(50) NOT NULL,
  `continent` varchar(50) NOT NULL,
  `active` smallint UNSIGNED NOT NULL DEFAULT '1',
  `time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `strrecordtype` char(3) COLLATE utf8_bin NOT NULL %s
  ) %s ROW_FORMAT=DYNAMIC  %s]],
sysbench.opt.table_name, table_num, id_def, primaryKeyDefinition,engine_def, extra_table_options)
   
   
   --print("DEBUG :" .. query)
      
   con:query(query)

   if sysbench.opt.create_indexes_before_dataload then 
	   create_indexes(drv, con, table_num)
	end


   if (sysbench.opt.table_size > 0) then
      print(string.format("Inserting %d records into '%s%d'",
                          sysbench.opt.table_size, sysbench.opt.table_name, table_num))
   end
   
   if sysbench.opt.auto_inc then
      query1 = "INSERT INTO " ..  sysbench.opt.table_name .. table_num .. " /*  continent=%s */ (uuid,millid,kwatts_s,date,location,continent,active,strrecordtype) VALUES"
   else
      query1 = "INSERT INTO " ..  sysbench.opt.table_name .. table_num .. " /*  continent=%s */ (id,uuid,millid,kwatts_s,date,location,continent,active,strrecordtype) VALUES"
   end

--   con:bulk_insert_init(query)
   con:bulk_insert_init(query)

   local c_val
   local pad_val
   
   
   local uuid = "UUID()"
   local millid 
   local kwatts_s
   local date = "NOW()"
   local location
   local continent
   local active
   local strrecordtype = "@@@"
   
--sysbench.opt.table_size
   con:bulk_insert_init(query)


   for i = 1, sysbench.opt.table_size do

      c_val = get_c_value()
      strrecordtype =  sysbench.rand.string("@@@")
      location =sysbench.rand.varstringalpha(5, 50)
      continent =sysbench.rand.continent(6)
      active = sysbench.rand.default(0,65535)
      millid = sysbench.rand.default(1,400)
      kwatts_s = sysbench.rand.default(0,4000000)
 
                                                                                                                                  
      if (sysbench.opt.auto_inc) then
        -- "(uuid,millid,kwatts_s,date,location,active,strrecordtyped)
         query = string.format("(%s, %d, %d,%s,'%s','%s',%d,'%s')",
                               uuid,
                               millid,
                               kwatts_s,
                               date,
                               location,
                               continent,
                               active,
                               strrecordtype
                               )
      else
         query = string.format("(%d,%s, %d, %d,%s,'%s','%s',%d,'%s')",
                               i,
                               uuid,
                               millid,
                               kwatts_s,
                               date,
                               location,
                               continent,
                               active,
                               strrecordtype
                               )
      end
     -- print("DEBUG :" .. continent)
      con:bulk_insert_next(query)
   end

   con:bulk_insert_done()

   if not sysbench.opt.create_indexes_before_dataload then 
	   create_indexes(drv, con, table_num)
	end


end

local t = sysbench.sql.type
local insertAction = "INSERT"
local onDuplicateKeyAction = " ON DUPLICATE KEY UPDATE kwatts_s=kwatts_s+1"

local stmt_defs = {
   point_selects = {
      "SELECT id, millid, date,continent,active,kwatts_s FROM %s%u WHERE id=?",
      t.INT},
   simple_ranges = {
      "SELECT id, millid, date,continent,active,kwatts_s FROM %s%u WHERE id BETWEEN ? AND ?",
      t.INT, t.INT},
   sum_ranges = {
      "SELECT SUM(kwatts_s) FROM %s%u WHERE id BETWEEN ? AND ?",
        t.INT, t.INT},
   order_ranges = {
      "SELECT id, millid, date,continent,active,kwatts_s  FROM %s%u WHERE id BETWEEN ? AND ? ORDER BY millid",
       t.INT, t.INT},
   distinct_ranges = {
      "SELECT DISTINCT millid,continent,active,kwatts_s   FROM %s%u WHERE id BETWEEN ? AND ? ",
      t.INT, t.INT},
   index_updates = {
      "UPDATE %s%u SET active=? WHERE id=?",
      t.INT,t.INT},
   non_index_updates = {
      "UPDATE %s%u SET location=? WHERE id=?",
       {t.VARCHAR,50},t.INT},
   deletes = {
      "DELETE FROM %s%u WHERE id=?",
      t.INT},
   inserts = {
      "INSERT INTO %s%u (id,uuid,millid,kwatts_s,date,location,continent,active,strrecordtype) VALUES (?, UUID(), ?, ?, NOW(), ?, ?, ?, ?) ON DUPLICATE KEY UPDATE kwatts_s=kwatts_s+1",
      t.BIGINT, t.INT,t.INT, {t.VARCHAR, 50},{t.VARCHAR, 50},t.INT, {t.CHAR, 3}},
   replace = {
      "REPLACE INTO %s%u (id,uuid,millid,kwatts_s,date,location,continent,active,strrecordtype) VALUES (?, UUID(), ?, ?, NOW(), ?, ?, ?, ?)",
      t.BIGINT, t.INT,t.INT, {t.VARCHAR, 50},{t.VARCHAR, 50},t.INT, {t.CHAR, 3}},
  
}


function prepare_begin()
   stmt.begin = con:prepare("BEGIN")
end

function prepare_commit()
   stmt.commit = con:prepare("COMMIT")
end

function prepare_for_each_table(key)
   for t = 1, sysbench.opt.tables do
   
      stmt[t][key] = con:prepare(string.format(stmt_defs[key][1], sysbench.opt.table_name,t))
-- print("DEBUG: " .. string.format(stmt_defs[key][1], sysbench.opt.table_name,t)

      local nparam = #stmt_defs[key] - 1

      if nparam > 0 then
         param[t][key] = {}
      end

      for p = 1, nparam do
         local btype = stmt_defs[key][p+1]
         local len

         if type(btype) == "table" then
            len = btype[2]
            btype = btype[1]
         end
         if btype == sysbench.sql.type.VARCHAR or
            btype == sysbench.sql.type.CHAR then
               param[t][key][p] = stmt[t][key]:bind_create(btype, len)
         else
            param[t][key][p] = stmt[t][key]:bind_create(btype)
         end
      end

      if nparam > 0 then
         stmt[t][key]:bind_param(unpack(param[t][key]))
      end
   end
end

function prepare_point_selects()
   prepare_for_each_table("point_selects")
end

function prepare_simple_ranges()
   prepare_for_each_table("simple_ranges")
end

function prepare_sum_ranges()
   prepare_for_each_table("sum_ranges")
end

function prepare_order_ranges()
   prepare_for_each_table("order_ranges")
end

function prepare_distinct_ranges()
   prepare_for_each_table("distinct_ranges")
end

function prepare_index_updates()
   prepare_for_each_table("index_updates")
end

function prepare_non_index_updates()
   prepare_for_each_table("non_index_updates")
end

function prepare_delete_inserts()
   prepare_for_each_table("deletes")
   if sysbench.opt.use_replace then
	   prepare_for_each_table("replace")
	else   
	   prepare_for_each_table("inserts")
   end 
end

function prepare_inserts()
   if sysbench.opt.use_replace then
	   prepare_for_each_table("replace")
	else   
	   prepare_for_each_table("inserts")
   end 
end



function thread_init()
   drv = sysbench.sql.driver()
   con = drv:connect()

   -- Create global nested tables for prepared statements and their
   -- parameters. We need a statement and a parameter set for each combination
   -- of connection/table/query
   stmt = {}
   param = {}

   for t = 1, sysbench.opt.tables do
      stmt[t] = {}
      param[t] = {}
   end

   -- This function is a 'callback' defined by individual benchmark scripts
   prepare_statements()
end

-- Close prepared statements
function close_statements()
   for t = 1, sysbench.opt.tables do
      for k, s in pairs(stmt[t]) do
         stmt[t][k]:close()
      end
   end
   if (stmt.begin ~= nil) then
      stmt.begin:close()
   end
   if (stmt.commit ~= nil) then
      stmt.commit:close()
   end
end

function thread_done()
   close_statements()
   con:disconnect()
end

function cleanup()
   local drv = sysbench.sql.driver()
   local con = drv:connect()

   for i = 1, sysbench.opt.tables do
      print(string.format("Dropping table '%s%d'...", sysbench.opt.table_name,i))
      con:query("DROP TABLE IF EXISTS " .. sysbench.opt.table_name .. i )
   end
end

local function get_table_num()
   return sysbench.rand.uniform(1, sysbench.opt.tables)
end

local function get_id()
   return sysbench.rand.default(1, sysbench.opt.table_size)
end

function begin()
   stmt.begin:execute()
end

function commit()
   stmt.commit:execute()
end

function execute_point_selects()
   local tnum = get_table_num()
   local i

   for i = 1, sysbench.opt.point_selects do
      param[tnum].point_selects[1]:set(get_id())

      stmt[tnum].point_selects:execute()
   end
end

local function execute_range(key)
   local tnum = get_table_num()

   for i = 1, sysbench.opt[key] do
      local id = get_id()

      param[tnum][key][1]:set(id)
      param[tnum][key][2]:set(id + sysbench.opt.range_size - 1)

      stmt[tnum][key]:execute()
   end
end

function execute_simple_ranges()
   execute_range("simple_ranges")
end

function execute_sum_ranges()
   execute_range("sum_ranges")
end

function execute_order_ranges()
   execute_range("order_ranges")
end

function execute_distinct_ranges()
   execute_range("distinct_ranges")
end

function execute_index_updates()
   local tnum = get_table_num()

   for i = 1, sysbench.opt.index_updates do
      param[tnum].index_updates[1]:set(sysbench.rand.default(0,65535))
      param[tnum].index_updates[2]:set(get_id())
      stmt[tnum].index_updates:execute()
      
--      param[tnum].index_updates[1]:set(1)
--      param[tnum].index_updates[2]:set(get_id())
--      stmt[tnum].index_updates:execute()      
      
   end
end

function execute_non_index_updates()
   local tnum = get_table_num()
    
   for i = 1, sysbench.opt.non_index_updates do
      param[tnum].non_index_updates[1]:set_rand_str_alpha("")
      param[tnum].non_index_updates[2]:set(get_id())

      stmt[tnum].non_index_updates:execute()
   end
end

function execute_delete_inserts()
   local tnum = get_table_num()

   for i = 1, sysbench.opt.delete_inserts do
      local id = get_id()


--      "INSERT INTO %s%u (id,uuid,millid,kwatts_s,date,location,active,strrecordtyped) VALUES (?, UUID(), ?, ?, NOW(), ?, ?, ?)",
--      t.BIGINT, t.TINYINT, t.INT, {t.VARCHAR, 50},t.TINYINT, {t.CHAR, 3}},
      
      millid = sysbench.rand.default(1,400)
      kwatts_s = sysbench.rand.default(0,4000000)
      location =sysbench.rand.varstringalpha(5, 50)
      continent =sysbench.rand.continent(6)
      active = sysbench.rand.default(0,65535)
      
      param[tnum].deletes[1]:set(id)

      if not sysbench.opt.use_replace then
	    param[tnum].inserts[1]:set(id)
    	param[tnum].inserts[2]:set(millid)
     	param[tnum].inserts[3]:set(kwatts_s)
     	param[tnum].inserts[4]:set(location)
      	param[tnum].inserts[5]:set(continent)
      	param[tnum].inserts[6]:set(active)
      	param[tnum].inserts[7]:set_rand_str_alpha("")
      else
	    param[tnum].replace[1]:set(id)
    	param[tnum].replace[2]:set(millid)
     	param[tnum].replace[3]:set(kwatts_s)
     	param[tnum].replace[4]:set(location)
      	param[tnum].replace[5]:set(continent)
      	param[tnum].replace[6]:set(active)
      	param[tnum].replace[7]:set_rand_str_alpha("")
      end
      
      stmt[tnum].deletes:execute()
      if not sysbench.opt.use_replace then
	     stmt[tnum].inserts:execute()
	    else
	    stmt[tnum].replace:execute()
	  end  
   end
end

function execute_inserts()
   local tnum = get_table_num()

   for i = 1, sysbench.opt.delete_inserts do
      local id = get_id()


--      "INSERT INTO %s%u (id,uuid,millid,kwatts_s,date,location,active,strrecordtyped) VALUES (?, UUID(), ?, ?, NOW(), ?, ?, ?)",
--      t.BIGINT, t.TINYINT, t.INT, {t.VARCHAR, 50},t.TINYINT, {t.CHAR, 3}},
      
      millid = sysbench.rand.default(1,400)
      kwatts_s = sysbench.rand.default(0,4000000)
      location =sysbench.rand.varstringalpha(5, 50)
      continent =sysbench.rand.continent(6)
      active = sysbench.rand.default(0,65535)
          
      if not sysbench.opt.use_replace then
	    param[tnum].inserts[1]:set(id)
    	param[tnum].inserts[2]:set(millid)
     	param[tnum].inserts[3]:set(kwatts_s)
     	param[tnum].inserts[4]:set(location)
      	param[tnum].inserts[5]:set(continent)
      	param[tnum].inserts[6]:set(active)
      	param[tnum].inserts[7]:set_rand_str_alpha("")
      else
	    param[tnum].replace[1]:set(id)
    	param[tnum].replace[2]:set(millid)
     	param[tnum].replace[3]:set(kwatts_s)
     	param[tnum].replace[4]:set(location)
      	param[tnum].replace[5]:set(continent)
      	param[tnum].replace[6]:set(active)
      	param[tnum].replace[7]:set_rand_str_alpha("")
      end
      
      if not sysbench.opt.use_replace then
	     stmt[tnum].inserts:execute()
	    else
	    stmt[tnum].replace:execute()      
      end    

   end
end

-- Re-prepare statements if we have reconnected, which is possible when some of
-- the listed error codes are in the --mysql-ignore-errors list
function sysbench.hooks.before_restart_event(errdesc)
   if errdesc.sql_errno == 2013 or -- CR_SERVER_LOST
      errdesc.sql_errno == 2055 or -- CR_SERVER_LOST_EXTENDED
      errdesc.sql_errno == 2006 or -- CR_SERVER_GONE_ERROR
      errdesc.sql_errno == 2011    -- CR_TCP_CONNECTION
   then
      close_statements()
      prepare_statements()
   end
end

function check_reconnect()
   if sysbench.opt.reconnect > 0 then
      transactions = (transactions or 0) + 1
      if transactions % sysbench.opt.reconnect == 0 then
         close_statements()
         con:reconnect()
         prepare_statements()
      end
   end
end

function sysbench.hooks.report_intermediate(stat)
   if sysbench.opt.stats_format == "human" then
         sysbench.report_default(stat)
   elseif sysbench.opt.stats_format == "csv" then
         sysbench.report_csv(stat)
   elseif sysbench.opt.stats_format == "json" then      
         sysbench.report_json(stat)
   else
      sysbench.report_default(stat)
   end
end

function sysbench.hooks.report_cumulative(stat)
   if sysbench.opt.stats_format == "csv" then
      sysbench.report_cumulative_csv(stat)
   elseif sysbench.opt.stats_format == "json" then
      sysbench.report_cumulative_csv(stat)   
      -- sysbench.report_cumulative_json(stat)
   else
      sysbench.report_cumulative_default(stat)
   end
end

