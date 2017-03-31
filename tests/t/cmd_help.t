  $ sysbench help
  sysbench * (glob)
  
  FATAL: Cannot find benchmark 'help': no such built-in test, file or module
  [1]

  $ cat >cmd_help.lua <<EOF
  > function help()
  >   print('function help()')
  > end
  > EOF
  $ sysbench cmd_help.lua help
  sysbench * (glob)
  
  function help()
