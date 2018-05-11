########################################################################
# Command line syntax tests
########################################################################

  $ sysbench foo bar
  sysbench * (glob)
  
  FATAL: Cannot find benchmark 'foo': no such built-in test, file or module
  [1]

  $ sysbench foo bar baz
  Unrecognized command line argument: baz
  [1]

  $ sysbench --unknown <<EOF
  > EOF
  sysbench * (glob)
  

  $ sysbench fileio
  sysbench * (glob)
  
  The 'fileio' test requires a command argument. See 'sysbench fileio help'
  [1]

  $ sysbench --help foo | grep Usage:
  Usage:

  $ sysbench <<EOF
  > print('test')
  > EOF
  sysbench * (glob)
  
  test

  $ sysbench run <<EOF
  > print('script body')
  > function event()
  >   print('event function')
  > end
  > EOF
  sysbench * (glob)
  
  FATAL: Cannot find benchmark 'run': no such built-in test, file or module
  [1]

  $ cat >$CRAMTMP/cmdline.lua <<EOF
  > #!/usr/bin/env sysbench
  > print('script body')
  > function event()
  >   print('event function')
  > end
  > EOF
  $ sysbench --events=1 $CRAMTMP/cmdline.lua
  sysbench * (glob)
  
  script body

  $ sysbench --events=1 $CRAMTMP/cmdline.lua run
  sysbench * (glob)
  
  script body
  Running the test with following options:
  Number of threads: 1
  Initializing random number generator from current time
  
  
  Initializing worker threads...
  
  script body
  Threads started!
  
  event function
  
  General statistics:
      total time: *s (glob)
      total number of events:              1
  
  Latency (ms):
           min: *.* (glob)
           avg: *.* (glob)
           max: *.* (glob)
           95th percentile: *.* (glob)
           sum: *.* (glob)
  
  Threads fairness:
      events (avg/stddev):           1.0000/0.00
      execution time (avg/stddev):   *.*/0.00 (glob)
  
########################################################################
Command line options tests
########################################################################

  $ cat >cmdline.lua <<EOF
  > sysbench.cmdline.options = {
  >   str_opt1 = {"str_opt1 description"},
  >   str_opt2 = {"str_opt2 description", "opt2"},
  >   str_opt3 = {"str_opt3 description", "opt3", sysbench.cmdline.ARG_STRING},
  >   bool_opt1 = {"bool_opt1 description", false},
  >   bool_opt2 = {"bool_opt2 description", true},
  >   bool_opt3 = {"bool_opt3 description", nil, sysbench.cmdline.ARG_BOOL},
  >   int_opt1 = {"int_opt1 description", 10},
  >   int_opt2 = {"int_opt2 description", nil, sysbench.cmdline.ARG_INT},
  >   int_opt3 = {"int_opt3 description", 20, sysbench.cmdline.ARG_INT},
  >   float_opt1 = {"float_opt1 description", 3.14, sysbench.cmdline.ARG_DOUBLE},
  >   float_opt2 = {"float_opt2 description", 0.2},
  >   list_opt1 = {"list_opt1 description", {"foo", "bar"}},
  >   list_opt2 = {"list_opt2 description", nil, sysbench.cmdline.ARG_LIST},
  >   ["dash-opt"] = {"dash-opt desc", "dash-opt val"}
  > }
  > 
  > function print_opt_table()
  >    local o = sysbench.opt
  >    print(o.str_opt1)
  >    print(o.str_opt2)
  >    print(o.str_opt3)
  >    print(o.bool_opt1)
  >    print(o.bool_opt2)
  >    print(o.bool_opt3)
  >    print(o.int_opt1)
  >    print(o.int_opt2)
  >    print(o.float_opt1)
  >    print(o.float_opt2)
  >    print(o.list_opt1)
  >    print(o.list_opt2)
  >    print(o.dash_opt)
  >    print()
  > end
  > 
  > function help()
  >    print("function help()")
  >    print("Available options:")
  >    sysbench.cmdline.print_test_options()
  >    print_opt_table()
  > end
  > 
  > function init()
  >   print("function init()")
  >   print_opt_table()
  > end
  > 
  > function thread_init()
  >   print("function thread_init()")
  >   print_opt_table()
  > end
  > 
  > function event()
  >   print("function event()")
  >   print_opt_table()
  > end
  > 
  > function thread_done()
  >   print("function thread_done()")
  >   print_opt_table()
  > end
  > 
  > function done()
  >   print("function done()")
  >   print_opt_table()
  > end
  > EOF

  $ sysbench cmdline.lua
  sysbench * (glob)
  
  $ sysbench cmdline.lua help
  sysbench * (glob)
  
  function help()
  Available options:
    --bool_opt1[=on|off]   bool_opt1 description [off]
    --bool_opt2[=on|off]   bool_opt2 description [on]
    --bool_opt3[=on|off]   bool_opt3 description
    --dash-opt=STRING      dash-opt desc [dash-opt val]
    --float_opt1=N         float_opt1 description [3.14]
    --float_opt2=N         float_opt2 description [0.2]
    --int_opt1=N           int_opt1 description [10]
    --int_opt2=N           int_opt2 description
    --int_opt3=N           int_opt3 description [20]
    --list_opt1=[LIST,...] list_opt1 description [foo,bar]
    --list_opt2=[LIST,...] list_opt2 description
    --str_opt1=STRING      str_opt1 description
    --str_opt2=STRING      str_opt2 description [opt2]
    --str_opt3=STRING      str_opt3 description [opt3]
  
  
  opt2
  opt3
  false
  true
  true
  10
  0
  3.14
  0.2
  table: 0x* (glob)
  table: 0x* (glob)
  dash-opt val
  
  $ sysbench cmdline.lua prepare
  sysbench * (glob)
  
  'cmdline.lua' test does not implement the 'prepare' command.
  [1]

  $ sysbench --non-existing-option=3 cmdline.lua prepare
  sysbench * (glob)
  
  invalid option: --non-existing-option=3
  [1]

  $ sysbench cmdline.lua --events=1 run
  sysbench * (glob)
  
  function init()
  
  opt2
  opt3
  false
  true
  true
  10
  0
  3.14
  0.2
  table: 0x* (glob)
  table: 0x* (glob)
  dash-opt val
  
  Running the test with following options:
  Number of threads: 1
  Initializing random number generator from current time
  
  
  Initializing worker threads...
  
  function thread_init()
  
  opt2
  opt3
  false
  true
  true
  10
  0
  3.14
  0.2
  table: 0x* (glob)
  table: 0x* (glob)
  dash-opt val
  
  Threads started!
  
  function event()
  
  opt2
  opt3
  false
  true
  true
  10
  0
  3.14
  0.2
  table: 0x* (glob)
  table: 0x* (glob)
  dash-opt val
  
  function thread_done()
  
  opt2
  opt3
  false
  true
  true
  10
  0
  3.14
  0.2
  table: 0x* (glob)
  table: 0x* (glob)
  dash-opt val
  
  
  General statistics:
      total time:                          *s (glob)
      total number of events:              1
  
  Latency (ms):
           min:                                  * (glob)
           avg:                                  * (glob)
           max:                                  * (glob)
           95th percentile:                      * (glob)
           sum:                                  * (glob)
  
  Threads fairness:
      events (avg/stddev):           1.0000/0.00
      execution time (avg/stddev):   */0.00 (glob)
  
  function done()
  
  opt2
  opt3
  false
  true
  true
  10
  0
  3.14
  0.2
  table: 0x* (glob)
  table: 0x* (glob)
  dash-opt val
  

  $ sysbench cmdline.lua cleanup
  sysbench * (glob)
  
  'cmdline.lua' test does not implement the 'cleanup' command.
  [1]

  $ cat >cmdline.lua <<EOF
  > 
  > EOF

  $ sysbench cmdline.lua help
  sysbench * (glob)
  
  'cmdline.lua' test does not implement the 'help' command.
  [1]

  $ cat >cmdline.lua <<EOF
  > sysbench.cmdline.options = {
  >   {}, 
  > }
  > 
  > function help()
  > end
  > EOF
  $ sysbench cmdline.lua help
  sysbench * (glob)
  
  FATAL: `sysbench.cmdline.read_cmdline_options' function failed: [string "sysbench.cmdline.lua"]:*: wrong table structure in sysbench.cmdline.options (glob)
  [1]

  $ sysbench fileio --invalid-option prepare
  sysbench * (glob)
  
  invalid option: --invalid-option
  [1]

# Custom commands

  $ sysbench <<EOF
  > sysbench.cmdline.commands = {
  >   cmd1 = "wrong structure"
  > }
  > EOF
  sysbench * (glob)
  
  $ sysbench <<EOF
  > sysbench.cmdline.commands = {
  >   cmd1 = { non_existing_func }
  > }
  > EOF
  sysbench * (glob)
  
  $ cat >cmdline.lua <<EOF
  > ffi.cdef "unsigned int sleep(unsigned int);"
  > function cmd1_func()
  >  print("cmd1, sysbench.tid = " .. sysbench.tid)
  > end
  > function cmd2_func()
  >   ffi.C.sleep(sysbench.tid % 2)
  >   print("cmd2, sysbench.tid = " .. sysbench.tid)
  > end
  > function prepare_func()
  >   ffi.C.sleep(sysbench.tid % 2)
  >   print("prepare_func, sysbench.tid = " .. sysbench.tid)
  > end
  > sysbench.cmdline.commands = {
  >  cmd1 = { cmd1_func },
  >  cmd2 = { cmd2_func, sysbench.cmdline.PARALLEL_COMMAND },
  >  prepare = { prepare_func, sysbench.cmdline.PARALLEL_COMMAND }
  > }
  > EOF

  $ sysbench --threads=2 cmdline.lua cmd1
  sysbench * (glob)
  
  cmd1, sysbench.tid = 0
  $ sysbench --threads=2 cmdline.lua cmd2
  sysbench * (glob)
  
  Initializing worker threads...
  
  cmd2, sysbench.tid = [01] (re)
  cmd2, sysbench.tid = [01] (re)

  $ sysbench --threads=2 cmdline.lua prepare
  sysbench * (glob)
  
  Initializing worker threads...
  
  prepare_func, sysbench.tid = [01] (re)
  prepare_func, sysbench.tid = [01] (re)

  $ sysbench --threads=2 cmdline.lua cmd3
  sysbench * (glob)
  
  Unknown command: cmd3
  [1]

  $ cat >cmdline.lua <<EOF
  > function print_cmd()
  >   print("argv = " .. require("inspect")(sysbench.cmdline.argv))
  >   print(string.format("sysbench.cmdline.command = %s",sysbench.cmdline.command))
  > end
  > function prepare()
  >  print_cmd()
  > end
  > print_cmd()
  > EOF
  $ sysbench --opt1 --opt2=val cmdline.lua
  sysbench * (glob)
  
  argv = { "--opt1", "--opt2=val", "cmdline.lua",
    [0] = "sysbench"
  }
  sysbench.cmdline.command = nil
  $ sysbench --opt1 --opt2=val cmdline.lua prepare
  sysbench * (glob)
  
  argv = { "--opt1", "--opt2=val", "cmdline.lua", "prepare",
    [0] = "sysbench"
  }
  sysbench.cmdline.command = prepare
  argv = { "--opt1", "--opt2=val", "cmdline.lua", "prepare",
    [0] = "sysbench"
  }
  sysbench.cmdline.command = prepare

  $ sysbench - <<EOF
  > print("hello")
  > EOF
  sysbench * (glob)
  
  hello

  $ sysbench - prepare <<EOF
  > function prepare()
  >   print("prepare")
  > end
  > print("global")
  > EOF
  sysbench * (glob)
  
  global

# Test benchmark specification as a module

  $ cat > cmdline_module.lua <<EOF
  > print("cmdline_module loaded")
  > function event()
  >   print("cmdline_module event")
  > end
  > EOF

  $ LUA_PATH="$PWD/?.lua;$LUA_PATH" sysbench cmdline_module --verbosity=0
  cmdline_module loaded

  $ LUA_PATH="$PWD/?.lua;$LUA_PATH" sysbench cmdline_module --events=1 --verbosity=0 run
  cmdline_module loaded
  cmdline_module loaded
  cmdline_module event

# Test that errors thrown by the script itself are reported properly

  $ cat >> cmdline_module.lua <<EOF
  >   error("test error")
  > EOF

  $ LUA_PATH="$PWD/?.lua;$LUA_PATH" sysbench cmdline_module --verbosity=0
  cmdline_module loaded
  FATAL: */cmdline_module.lua:5: test error (glob)
  [1]
  $ LUA_PATH="$PWD/?.lua;$LUA_PATH" sysbench cmdline_module --events=1 --verbosity=0 run
  cmdline_module loaded
  FATAL: */cmdline_module.lua:5: test error (glob)
  [1]

##########################################################################
# Test boolean option validation
##########################################################################
  $ cat > cmdline.lua <<EOF
  > sysbench.cmdline.options = {
  >  bool_opt = {"Flag", false}
  > }
  > 
  > function prepare()
  >   print("bool_opt = " .. tostring(sysbench.opt.bool_opt))
  > end
  > EOF

  $ SB_ARGS=--verbosity=0
  $ sysbench $SB_ARGS cmdline.lua --bool-opt=on prepare
  bool_opt = true
  $ sysbench $SB_ARGS cmdline.lua --bool-opt=off prepare
  bool_opt = false
  $ sysbench $SB_ARGS cmdline.lua --bool-opt=true prepare
  bool_opt = true
  $ sysbench $SB_ARGS cmdline.lua --bool-opt=false prepare
  bool_opt = false
  $ sysbench $SB_ARGS cmdline.lua --bool-opt=1 prepare
  bool_opt = true
  $ sysbench $SB_ARGS cmdline.lua --bool-opt=0 prepare
  bool_opt = false
  $ sysbench $SB_ARGS cmdline.lua --bool-opt=5 prepare
  invalid option: --bool-opt=5
  [1]
  $ sysbench $SB_ARGS cmdline.lua --bool-opt=foo prepare
  invalid option: --bool-opt=foo
  [1]
