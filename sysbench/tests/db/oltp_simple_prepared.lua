pathtest = string.match(test, "(.*/)") or ""

dofile(pathtest .. "common.lua")

function thread_init(thread_id)
   local table_name
   set_vars()

   table_name = "sbtest".. sb_rand_uniform(1, oltp_tables_count)
   point_select = db_prepare("SELECT c FROM ".. table_name .." WHERE id=?")
   point_params = {}
   point_params[1] = 1 
   db_bind_param(point_select, point_params)
end

function event(thread_id)
   point_params[1] = sb_rand(1, oltp_table_size)
   rs = db_execute(point_select)
end
