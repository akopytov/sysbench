-- for proper initialization use --max-requests = N, where N is --num-threads
--
pathtest = string.match(test, "(.*/)") or ""

dofile(pathtest .. "common.lua")

function thread_init(thread_id)
   set_vars()
end

function event(thread_id)
   local i
   print("thread prepare"..thread_id)

   for i=thread_id+1, oltp_tables_count, num_threads  do
    db_connect()
    insert(i)
   end

end
