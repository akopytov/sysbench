  $ sysbench prepare
  sysbench * (glob)
  
  FATAL: Cannot find benchmark 'prepare': no such built-in test, file or module
  [1]

  $ cat >cmd_prepare.lua <<EOF
  > function prepare()
  >   print('function prepare()')
  > end
  > EOF
  $ sysbench cmd_prepare.lua prepare
  sysbench * (glob)
  
  function prepare()
