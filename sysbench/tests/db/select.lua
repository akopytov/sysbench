pathtest = string.match(test, "(.*/)") or ""

dofile(pathtest .. "common.lua")

function thread_init(thread_id)
   set_vars()
end

function event(thread_id)
   local table_name
   table_name = "sbtest".. sb_rand_uniform(1, oltp_tables_count)
   rs = db_query("SELECT pad FROM ".. table_name .." WHERE id=" .. sb_rand(1, oltp_table_size))
end

