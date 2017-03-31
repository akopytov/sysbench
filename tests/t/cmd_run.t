  $ sysbench run
  sysbench * (glob)
  
  FATAL: Cannot find benchmark 'run': no such built-in test, file or module
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
