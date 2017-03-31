  $ sysbench cleanup
  sysbench * (glob)
  
  FATAL: Cannot find benchmark 'cleanup': no such built-in test, file or module
  [1]

  $ cat >cmd_cleanup.lua <<EOF
  > function cleanup()
  >   print('function cleanup()')
  > end
  > EOF
  $ sysbench cmd_cleanup.lua cleanup
  sysbench * (glob)
  
  function cleanup()
