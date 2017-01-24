  $ sysbench run
  sysbench * (glob)
  
  Cannot find script run: No such file or directory
  [1]

  $ cat >cmd_run.lua <<EOF
  > function thread_run()
  >   print('function thread_run()')
  > end
  > function event()
  > end
  > EOF
  $ sysbench --verbosity=0 cmd_run.lua run
  function thread_run()
