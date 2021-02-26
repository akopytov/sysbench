
-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation; either version 2 of the License, or
-- (at your option) any later version.

-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.

-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software
-- Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

-- -----------------------------------------------------------------------------
-- Common code for iiBench (Index insertion benchmark) benchmarks.
-- -----------------------------------------------------------------------------


require("internal/thread_groups")
require("internal/index_stat")

function init()
   assert(sysbench.version >= '1.1.0', "There is a bug with preparing float values in" ..
    " sysbench versions under 1.1.1. Please use iibench with newer sysbench version.")

   assert(event ~= nil,
          "this script is meant to be included by other scripts and " ..
             "should not be called directly.")

end

if sysbench.cmdline.command == nil then
   error("Command is required. Supported commands: prepare, warmup, run, " ..
            "cleanup, help")
end

-- Command line options
sysbench.cmdline.options = {

-- Subject field parameters
   cashregisters =
      {"# cash registers", 1000},
   products =
      {"# products", 10000},
   customers =
      {"# customers", 100000},
   max_price =
      {"Maximum value for price column", 500},


-- Query parameters
   rows_per_commit =
      {"rows inserted per transaction", 1000},
   rows_per_query =
      {"limit of selected rows", 10},
   data_length_max =
      {"Max size of data in data column", 10},
   data_length_min =
      {"Min size of data in data column", 10},
   data_random_pct =
      {"Percentage of row that has random data", 50},
   secondary_at_end =
      {"Create secondary index at end", false},
   insert_rate =
      {"Average rate for inserts", 0},
   inserts_per_second =
      {"Average rate for all insert threads at once", 0},
   select_rate =
      {"Average rate for selects", 0},
   query_rate =
      {"An alias for select_rate", 0},
   insert_threads =
      {"Amount of threads for inserts", 1},
   select_threads =
      {"Amount of threads for selects, default 0 means all others", 0},
   query_threads =
      {"An alias for 'select_threads'", 0},


-- Table parameters
   table_size =
      {"Number of rows in table", 10000},
   max_table_rows =
      {"Max number of rows in table", 10000000},
   num_secondary_indexes =
      {"Number of secondary indexes (0 to 3)", 3},
   num_partitions =
      {"Use range partitioning when not 0", 0},
   rows_per_partition =
      {"Number of rows per partition. If 0 this is computed as max_rows/num_partitions", 0},
   fill_table =
      {"Put table_size rows in table", true},
   with_max_table_rows =
      {"Do we want delete after insert, when table more than max size", false},


-- other options
   tables =
      {"Number of tables", 1},
   reconnect =
      {"Reconnect after every N events. The default (0) is to not reconnect",
       0},
   stat =
      {"This option controls statistic print for warm up command",
       false},
   mysql_storage_engine =
      {"Storage engine, if MySQL is used", "innodb"},
   create_table_options =
      {"Options passed to CREATE TABLE statement", ""},
   pgsql_variant =
      {"Use this PostgreSQL variant when running with the " ..
          "PostgreSQL driver. The only currently supported " ..
          "variant is 'redshift'. When enabled, " ..
          "create_secondary is automatically disabled, and " ..
          "delete_inserts is set to 0"}
}

-- Prepare the dataset. This command supports parallel execution, i.e. will
-- benefit from executing with --threads > 1 as long as --tables > 1
function cmd_prepare()
   local drv = sysbench.sql.driver()
   local con = drv:connect()

   for i = sysbench.tid % sysbench.opt.threads + 1, sysbench.opt.tables,
   sysbench.opt.threads do
     create_table(drv, con, i)
     create_index(drv, con, i)
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
      local t = "sbtest" .. i
      print("Preloading table " .. t)

      if (sysbench.opt.stat)
      then
         print("\nstat before warmup:")
         show_index_stat(string.format("sbtest%u", i))
      end
      con:query("ANALYZE TABLE sbtest" .. i)



--96,76

      con:query(string.format(
                   "SELECT AVG(transactionid) FROM " ..
                      "(SELECT * FROM %s FORCE KEY (PRIMARY) " ..
                      "LIMIT %u) t",
                   t, sysbench.opt.table_size))



      con:query(string.format(
                   "SELECT COUNT(*) FROM " ..
                      "(SELECT * FROM %s WHERE data  LIKE '%%0%%' LIMIT %u) t",
                   t, sysbench.opt.table_size))

      if sysbench.opt.num_secondary_indexes > 0
      then
      con:query(string.format(
                   "SELECT COUNT(*) FROM " ..
                      " %s FORCE KEY ( %s_marketsegment ) WHERE customerid like '%%0%%'",
                   t, t))
      end
      if sysbench.opt.num_secondary_indexes > 1
      then
      con:query(string.format(
                   "SELECT COUNT(*) FROM " ..
                      " %s FORCE KEY ( %s_registersegment ) WHERE cashregisterid like '%%0%%'",
                   t, t))
      end
      if sysbench.opt.num_secondary_indexes > 2
      then
      con:query(string.format(
                   "SELECT COUNT(*) FROM " ..
                      " %s FORCE KEY ( %s_pdc ) WHERE dateandtime like '%%0%%'",
                   t, t))
      end

      if (sysbench.opt.stat)
      then
         print("\nstat after warmup:")
         show_index_stat(string.format("sbtest%u", i))
      end


   end

end

-- Implement parallel prepare and warmup commands, define 'prewarm' as an alias
-- for 'warmup'
sysbench.cmdline.commands = {
   prepare = {cmd_prepare, sysbench.cmdline.PARALLEL_COMMAND},
   warmup = {cmd_warmup, sysbench.cmdline.PARALLEL_COMMAND},
   prewarm = {cmd_warmup, sysbench.cmdline.PARALLEL_COMMAND}
}


function create_table(drv, con, table_num)
   local id_def
   local engine_def =  "/*! ENGINE = " .. sysbench.opt.mysql_storage_engine .. " */"
   local extra_table_options = ""
   local query
   local partition_options = ""
   local rows_per_part = 0

   if (sysbench.opt.num_partitions > 0) then
      if (sysbench.opt.rows_per_partition > 0) then
         rows_per_part = sysbench.opt.rows_per_partition
      else
         rows_per_part = sysbench.opt.table_size / sysbench.opt.num_partitions
      end

      partition_options = "partition by range( transactionid ) ("

      for i = 1, sysbench.opt.num_partitions - 1 do
         partition_options = partition_options ..
            string.format(" partition p%d values less than (%d),\n", i, i*rows_per_part)
      end
      partition_options = partition_options ..
         string.format(" partition p%d values less than (MAXVALUE)\n)", sysbench.opt.num_partitions)
   end

   print(string.format("Creating table 'sbtest%d'...", table_num))

   query = string.format([[
   CREATE TABLE sbtest%d(
   transactionid BIGINT NOT NULL AUTO_INCREMENT,
   dateandtime datetime NOT NULL,
   cashregisterid int NOT NULL,
   customerid int NOT NULL,
   productid int NOT NULL,
   price float NOT NULL,
   data varchar(%d) NOT NULL,
   primary key (transactionid)
   ) %s %s %s
   ]],
   table_num, sysbench.opt.data_length_max, engine_def, sysbench.opt.create_table_options, partition_options)

   con:query(query)

   if (sysbench.opt.table_size > 0) then
      print(string.format("Inserting %d records into 'sbtest%d'",
                          sysbench.opt.table_size, table_num))
   end

   if sysbench.opt.fill_table then
      query = string.format([[INSERT INTO sbtest%d(dateandtime, cashregisterid, customerid, productid, price, data)
         VALUES]], table_num)
   end

   con:bulk_insert_init(query)

   local c_val
   local pad_val
   if sysbench.opt.fill_table then
      for i = 1, sysbench.opt.table_size do
         query = make_insert_query_string()
         con:bulk_insert_next(query)
      end
   end

   con:bulk_insert_done()
end

function create_index(drv, con, table_num)
   if (sysbench.opt.num_secondary_indexes > 0) then
      local index_ddl = string.format("alter table sbtest%d add index sbtest%d_marketsegment (price, customerid) ",
                   table_num, table_num, table_num)
      if (sysbench.opt.num_secondary_indexes > 1) then
         index_ddl = index_ddl ..
            string.format(", add index sbtest%d_registersegment (cashregisterid, price, customerid) ",
               table_num, table_num)
         if (sysbench.opt.num_secondary_indexes > 2) then
            index_ddl = index_ddl ..
               string.format(", add index sbtest%d_pdc (price, dateandtime, customerid)",
                   table_num, table_num)
         end
      end
      print(string.format("Creating %d indexes into 'sbtest%d'",
            sysbench.opt.num_secondary_indexes, table_num))
      con:query(index_ddl)
   end
end


function make_insert_query_string()
    return string.format("('%s', %d, %d, %d, %f, '%s')", create_insert_data())
end

local function get_table_num()
   return sysbench.rand.uniform(1, sysbench.opt.tables)
end

local function get_customerid_id()
   return sysbench.rand.default(1, sysbench.opt.customers)
end

local function get_cashregister_id()
    return sysbench.rand.uniform(0, sysbench.opt.cashregisters)
end

local function get_price(customerid)
   return (sysbench.rand.uniform_double() * sysbench.opt.max_price + customerid) / 100.0
end

function create_insert_data()

    local dateandtime      = os.date('%Y-%m-%d %H:%M:%S')
    local cashregisterid   = get_cashregister_id()
    local customerid       = get_customerid_id()
    local productid        = sysbench.rand.uniform(0, sysbench.opt.products)
    local price            = get_price(customerid)

    local data_length      = sysbench.rand.uniform(sysbench.opt.data_length_min, sysbench.opt.data_length_max)
    local rand_data_length = math.floor(sysbench.opt.data_random_pct / 100 * data_length)
    if data_length == rand_data_length
    then
       rand_data_length = rand_data_length - 1    -- because last letter is always a
    end
    local data             = string.rep('a', data_length - rand_data_length - 1) ..
        sysbench.rand.varstring(rand_data_length, rand_data_length) .. 'a'

    return dateandtime, cashregisterid, customerid, productid, price, data
end


local t = sysbench.sql.type
local stmt_defs = {
   market_queries = {
      [[
         SELECT price, customerid FROM sbtest%u %s
         where (price >= ?) ORDER BY price, customerid LIMIT ?
      ]],
      t.INT, t.INT
   },
   register_queries = {
      [[
         SELECT cashregisterid,price,customerid FROM sbtest%u %s
         where (cashregisterid > ?) ORDER BY cashregisterid,price,customerid LIMIT ?
      ]],
      t.INT, t.INT
   },
   pdc_queries = {
      [[
         SELECT price, dateandtime, customerid FROM sbtest%u %s
         where (price >= ?) ORDER BY price, dateandtime, customerid LIMIT ?
      ]],
      t.INT, t.INT
   },
   deletes = {
      [[
         DELETE FROM sbtest%u ORDER BY transactionid LIMIT ?
      ]], t.INT
      }
}

function prepare_begin()
   stmt.begin = con:prepare("BEGIN")
end

function prepare_commit()
   stmt.commit = con:prepare("COMMIT")
end

function prepare_for_each_table(key)
   for t = 1, sysbench.opt.tables do

      if key == "deletes"
      then
         stmt[t][key] = con:prepare(string.format(stmt_defs[key][1], t))
      else
         -- ternary operators in Lua: x = condition and opt1 or opt2
         local switch = {
            ["market_queries"]   = (sysbench.opt.num_secondary_indexes > 0)
               and string.format("FORCE INDEX (sbtest%u_marketsegment)", t) or "",
            ["register_queries"] =  (sysbench.opt.num_secondary_indexes > 1)
               and string.format("FORCE INDEX (sbtest%u_registersegment)", t) or "",
            ["pdc_queries"]      = (sysbench.opt.num_secondary_indexes > 2)
               and string.format("FORCE INDEX (sbtest%u_pdc)", t) or ""
         }
         stmt[t][key] = con:prepare(string.format(stmt_defs[key][1], t, switch[key]))
      end

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


function prepare_deletes()
   -- variables for deletes
   if (sysbench.opt.with_max_table_rows)
   then
      insert_count = 0
      delete_flag = false
   end

   prepare_for_each_table("deletes")
end

function prepare_market_queries()
   prepare_for_each_table("market_queries")
end

function prepare_pdc_queries()
   prepare_for_each_table("pdc_queries")
end

function prepare_register_queries()
   prepare_for_each_table("register_queries")
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
      print(string.format("Dropping table 'sbtest%d'...", i))
      con:query("DROP TABLE IF EXISTS sbtest" .. i )
   end
end


function begin()
   stmt.begin:execute()
end

function commit()
   stmt.commit:execute()
end


function execute_market_queries()
   local tnum = get_table_num()

   local customer_id = get_customerid_id()
   local price = get_price(customer_id)
   param[tnum].market_queries[1]:set(price)
   param[tnum].market_queries[2]:set(sysbench.opt.rows_per_query)

   stmt[tnum].market_queries:execute()

end


function execute_pdc_queries()
   local tnum = get_table_num()

   local customer_id = get_customerid_id()
   local price = get_price(customer_id)
   param[tnum].pdc_queries[1]:set(price)
   param[tnum].pdc_queries[2]:set(sysbench.opt.rows_per_query)

   stmt[tnum].pdc_queries:execute()
end


function execute_register_queries()
   local tnum = get_table_num()

   param[tnum].register_queries[1]:set(get_cashregister_id())
   param[tnum].register_queries[2]:set(sysbench.opt.rows_per_query)

   stmt[tnum].register_queries:execute()

end


function execute_inserts()
   local tnum = get_table_num()
   local query = string.format([[INSERT INTO sbtest%d(dateandtime, cashregisterid, customerid, productid, price, data)
         VALUES]], tnum)

   con:bulk_insert_init(query)

   for i = 1, sysbench.opt.rows_per_commit do
      query = make_insert_query_string()
      con:bulk_insert_next(query)
   end
   con:bulk_insert_done()

   if sysbench.opt.with_max_table_rows
   then
      if delete_flag
      then
         execute_deletes(tnum)
      else
         insert_count = insert_count + 1
         check_delete_start()
      end
   end
end


function check_delete_start()
   if insert_count > (sysbench.opt.max_table_rows - sysbench.opt.table_size) /
      (sysbench.opt.rows_per_commit * sysbench.opt.insert_threads / sysbench.opt.tables)
   then
      delete_flag = true
   end
end

function execute_deletes(tnum)
   param[tnum].deletes[1]:set(sysbench.opt.rows_per_commit)
   stmt[tnum].deletes:execute()
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
