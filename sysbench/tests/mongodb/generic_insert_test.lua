-- Copyright (C) 2016 Percona
pathtest = string.match(test, "(.*/)") or ""

dofile(pathtest .. "common.lua")

function thread_init(thread_id)
         db_connect()
	 set_vars()
end

function event(thread_id)

   for i=1, sb_rand(2, 10) do
      local a = sb_rand_str([[#####-#####]])
      local b = sb_rand_str([[######################-######################]])
      mongodb_generic_insert("sbtest" .. sb_rand(1, oltp_tables_count), "{ \"a\" : \"" .. a .. "\", \"subdoc\": {\"b\" : \"" .. b .. "\"}}")
   end
   mongodb_fake_commit()
 
end
