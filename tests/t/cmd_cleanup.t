  $ sysbench cleanup
  sysbench * (glob)
  
  Cannot find script cleanup: No such file or directory
  [1]

  $ cat >cmd_cleanup.lua <<EOF
  > function cleanup()
  >   print('function cleanup()')
  > end
  > EOF
  $ sysbench cmd_cleanup.lua cleanup
  sysbench * (glob)
  
  function cleanup()
