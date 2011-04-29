pathtest = string.match(test, "(.*/)") or ""

dofile(pathtest .. "common.lua")

function thread_init(thread_id)
   local index_name
   local i
   set_vars()
   
   print("thread prepare"..thread_id)

   if (oltp_secondary) then
     index_name = "KEY xid"
   else
     index_name = "PRIMARY KEY"
   end

   for i=thread_id+1, oltp_tables_count, num_threads  do
   create_insert(i)
   end

end

function event(thread_id)

end
