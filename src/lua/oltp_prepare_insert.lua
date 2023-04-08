#!/usr/bin/env sysbench

require("oltp_common")

function prepare_statements()
   -- We do not use prepared statements here, but oltp_common.sh expects this
   -- function to be defined
end

function event()
   local table_name = "sbtest" .. sysbench.rand.uniform(1, sysbench.opt.tables)
   local k_val
   local c_val
   local pad_val

   local start_time = os.time()

   local query = "INSERT INTO " .. table_name .. "(id, k, c, pad) VALUES"

   -- print("Thread " .. sysbench.tid .. " Task: " .. math.floor(sysbench.opt.table_size * sysbench.tid / sysbench.opt.threads) .. " ~ " .. math.floor(sysbench.opt.table_size * (sysbench.tid + 1) / sysbench.opt.threads - 1))

   con:bulk_insert_init(query)

   for i = math.floor(sysbench.opt.table_size * sysbench.tid / sysbench.opt.threads), math.floor(sysbench.opt.table_size * (sysbench.tid + 1) / sysbench.opt.threads - 1) do
      k_val = sysbench.rand.default(1, sysbench.opt.table_size)
      c_val = get_c_value()
      pad_val = get_pad_value()

      query = string.format("(%d, %d, '%s', '%s')", i, k_val, c_val, pad_val)

      con:bulk_insert_next(query)
   end

   con:bulk_insert_done()

   if sysbench.tid == 0 then
     con:query("SET innodb_ddl_threads = 8")
     con:query("SET innodb_ddl_buffer_size = 1048576000")
     con:query("SET innodb_parallel_read_threads = 8")
     con:query("ALTER TABLE " .. table_name .." ADD INDEX idx_1(k)")
     con:query("ANALYZE TABLE " .. table_name)

     local rs = con:query("SHOW CREATE TABLE " .. table_name)
     local row = rs:fetch_row()
     print(row[2])

     query = string.format("SELECT (DATA_LENGTH+DATA_FREE+INDEX_LENGTH)/1024/1024/1024 FROM information_schema.tables WHERE TABLE_SCHEMA = database() AND TABLE_NAME = '%s'", table_name)
     print(query)
     rs = con:query(query)
     row = rs:fetch_row()
     print(string.format("Table size: %s GB", row[1]))
   end

   local end_time = os.time()
   local time_spend = end_time - start_time
   print("Thread " .. sysbench.tid .. " finished. Time spent: " .. time_spend)

   os.execute("sleep " .. tonumber(3))
--   os.exit()
end
