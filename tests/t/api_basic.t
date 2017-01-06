########################################################################
Basic Lua API tests
########################################################################

  $ if [ -z "${SBTEST_MYSQL_ARGS:-}" ]
  > then
  >   exit 80
  > fi

  $ SB_ARGS="--verbosity=0 --test=$CRAMTMP/api_basic.lua --max-requests=2 --num-threads=1 --db-driver=mysql $SBTEST_MYSQL_ARGS"

  $ cat >$CRAMTMP/api_basic.lua <<EOF
  > function init(thread_id)
  >   print("tid:" .. (thread_id or "(nil)") .. " init()")
  > end
  > 
  > function prepare(thread_id)
  >   print("tid:" .. (thread_id or "(nil)") .. " prepare()")
  > end
  > 
  > function run(thread_id)
  >   print("tid:" .. (thread_id or "(nil)") .. " run()")
  > end
  > 
  > function cleanup(thread_id)
  >   print("tid:" .. (thread_id or "(nil)") .. " cleanup()")
  > end
  > 
  > function help()
  >   print("tid:" .. (thread_id or "(nil)") .. " help()")
  > end
  > 
  > function thread_init(thread_id)
  >   print(string.format("tid:%d thread_init()", thread_id))
  > end
  > 
  > function event(thread_id)
  >   print(string.format("tid:%d event()", thread_id))
  > end
  > 
  > function thread_done(thread_id)
  >   print(string.format("tid:%d thread_done()", thread_id))
  > end
  > 
  > function done(thread_id)
  >   print("tid:" .. (thread_id or "(nil)") .. " done()")
  > end
  > 
  > EOF

  $ sysbench $SB_ARGS prepare
  tid:(nil) prepare()

  $ sysbench $SB_ARGS run
  tid:0 thread_init()
  tid:0 event()
  tid:0 event()
  tid:0 thread_done()

  $ sysbench $SB_ARGS cleanup
  tid:(nil) cleanup()

  $ sysbench $SB_ARGS help
  tid:(nil) help()
