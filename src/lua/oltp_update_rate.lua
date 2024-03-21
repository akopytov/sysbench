#!/usr/bin/env sysbench
-- This test is designed for testing MariaDB's key_cache_segments for MyISAM,
-- and should work with other storage engines as well.
--
-- For details about key_cache_segments please refer to:
-- http://kb.askmonty.org/v/segmented-key-cache
--

require("oltp_common")

-- Test specific options
sysbench.cmdline.options.update_rate =
   {"Number of rows to update per second", 100}

function thread_init()
   drv = sysbench.sql.driver()
   con = drv:connect()

   stmt = {}
   params = {}

   for t = 1, sysbench.opt.tables do
      stmt[t] = {}
      params[t] = {}
   end

   for t = 1, sysbench.opt.tables do

      sleep_stmt = con:prepare("SELECT sleep(1)")
      stmt[t] = con:prepare(string.format("UPDATE sbtest%d SET k=k+1 WHERE id >= ? LIMIT %d", t, sysbench.opt.update_rate))

      params[t] = stmt[t]:bind_create(sysbench.sql.type.INT)
      stmt[t]:bind_param(params[t])
   end

end

function thread_done()
   for t = 1, sysbench.opt.tables do
      stmt[t]:close()
   end
   sleep_stmt:close()
   con:disconnect()
end

function event()
   local tnum = sysbench.rand.uniform(1, sysbench.opt.tables)

   params[tnum]:set(sysbench.rand.default(1, sysbench.opt.table_size))

   stmt[tnum]:execute()
   sleep_stmt:execute() 
   
   check_reconnect()
end