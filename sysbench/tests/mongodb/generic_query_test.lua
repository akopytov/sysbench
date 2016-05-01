-- Copyright (C) 2016 Percona
pathtest = string.match(test, "(.*/)") or ""

dofile(pathtest .. "common.lua")

function thread_init(thread_id)
         db_connect()
	 set_vars()
end

function event(thread_id)

   for i=1, sb_rand(2, 10) do
	mongodb_generic_query("sbtest" .. sb_rand(1, oltp_tables_count), "{ \"_id\" : \"" .. sb_rand(1, oltp_table_size) .. "\"}",  "{\"_id\" : \"1\" }")
   end
   mongodb_fake_commit()
 
end
