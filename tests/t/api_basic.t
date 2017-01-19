########################################################################
Basic Lua API tests
########################################################################

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
  tid:(nil) init()
  tid:0 thread_init()
  tid:0 event()
  tid:0 event()
  tid:0 thread_done()
  tid:(nil) done()

  $ sysbench $SB_ARGS cleanup
  tid:(nil) cleanup()

  $ sysbench $SB_ARGS help
  tid:(nil) help()

########################################################################
Error handling
########################################################################

# Syntax errors in the script
  $ cat >$CRAMTMP/api_basic.lua <<EOF
  > foo
  > EOF
  $ sysbench $SB_ARGS run
  PANIC: unprotected error in call to Lua API (*/api_basic.lua:2: '=' expected near '<eof>') (glob)
  [1]

# Missing event function
  $ cat >$CRAMTMP/api_basic.lua <<EOF
  > function foo()
  > end
  > EOF
  $ sysbench $SB_ARGS run
  FATAL: cannot find the event() function in */api_basic.lua (glob)
  [1]
