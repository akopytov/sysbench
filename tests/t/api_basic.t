########################################################################
Basic Lua API tests
########################################################################

  $ SB_ARGS="--verbosity=0 --events=2 --db-driver=mysql $SBTEST_MYSQL_ARGS $CRAMTMP/api_basic.lua"

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
  > function help(thread_id)
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
 
  $ cat >$CRAMTMP/api_basic.lua <<EOF
  > function event()
  >   print(sysbench.version)
  >   print(sysbench.version_string)
  > end
  > EOF

  $ sysbench $SB_ARGS --events=1 run |
  > sed -e "s/$SBTEST_VERSION_STRING/VERSION_STRING/" \
  >     -e "s/$SBTEST_VERSION/VERSION/"
  VERSION
  VERSION_STRING

  $ cat >$CRAMTMP/api_basic.lua <<EOF
  > function event()
  >     print(string.format("sysbench.cmdline.script_path = %s", sysbench.cmdline.script_path))
  > end
  > EOF
  $ sysbench $SB_ARGS --events=1 run
  sysbench.cmdline.script_path = */api_basic.lua (glob)

########################################################################
Error handling
########################################################################

# Syntax errors in the script
  $ cat >$CRAMTMP/api_basic.lua <<EOF
  > foo
  > EOF
  $ sysbench $SB_ARGS run
  FATAL: */api_basic.lua:2: '=' expected near '<eof>' (glob)
  [1]

# Missing event function
  $ cat >$CRAMTMP/api_basic.lua <<EOF
  > function foo()
  > end
  > EOF
  $ sysbench $SB_ARGS run
  FATAL: cannot find the event() function in *api_basic.lua (glob)
  [1]

########################################################################
event() return values
########################################################################

  $ cat >$CRAMTMP/api_basic.lua <<EOF
  > sysbench.cmdline.options = { param = {"param", 0} }
  > function event()
  >   i = (i or 0) + 1
  >   local param = sysbench.opt.param
  >   print(i)
  >   if param == 1 then
  >     return 0
  >   elseif param == 2 then
  >     return 1
  >   elseif param == 3 then
  >     return true
  >   elseif param == 4 then
  >     return {}
  >   elseif param == 5 then
  >     return false
  >   elseif param == 6 then
  >     return nil
  >   else
  >     error("Unknown param value")
  >   end
  > end
  > EOF
  $ sysbench $SB_ARGS run --param=1
  1
  $ sysbench $SB_ARGS run --param=2
  1
  $ sysbench $SB_ARGS run --param=3
  1
  $ sysbench $SB_ARGS run --param=4
  1
  $ sysbench $SB_ARGS run --param=5
  1
  2
  $ sysbench $SB_ARGS run --param=6
  1
  2
