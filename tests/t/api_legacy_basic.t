########################################################################
Legacy basic Lua API tests
########################################################################

  $ if [ -z "${SBTEST_MYSQL_ARGS:-}" ]
  > then
  >   exit 80
  > fi

  $ SB_ARGS="--verbosity=0 --max-requests=2 --db-driver=mysql $SBTEST_MYSQL_ARGS --test=$CRAMTMP/api_legacy_basic.lua"

  $ cat >$CRAMTMP/api_legacy_basic.lua <<EOF
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
  > EOF

  $ sysbench $SB_ARGS prepare
  WARNING: the --test option is deprecated. You can pass a script name or path on the command line without any options.
  WARNING: --max-requests is deprecated, use --events instead
  tid:(nil) prepare()

  $ sysbench $SB_ARGS run
  WARNING: the --test option is deprecated. You can pass a script name or path on the command line without any options.
  WARNING: --max-requests is deprecated, use --events instead
  tid:0 thread_init()
  tid:0 event()
  tid:0 event()
  tid:0 thread_done()

  $ sysbench $SB_ARGS cleanup
  WARNING: the --test option is deprecated. You can pass a script name or path on the command line without any options.
  WARNING: --max-requests is deprecated, use --events instead
  tid:(nil) cleanup()

  $ sysbench $SB_ARGS help
  WARNING: the --test option is deprecated. You can pass a script name or path on the command line without any options.
  WARNING: --max-requests is deprecated, use --events instead
  tid:0 help()

  $ cat >$CRAMTMP/api_legacy_basic.lua <<EOF
  > function prepare()
  >   print(test)
  > end
  > EOF
  $ sysbench $SB_ARGS prepare
  WARNING: the --test option is deprecated. You can pass a script name or path on the command line without any options.
  WARNING: --max-requests is deprecated, use --events instead
  */api_legacy_basic.lua (glob)
