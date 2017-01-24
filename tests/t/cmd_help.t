  $ sysbench help
  sysbench * (glob)
  
  Cannot find script help: No such file or directory
  [1]

  $ cat >cmd_help.lua <<EOF
  > function help()
  >   print('function help()')
  > end
  > EOF
  $ sysbench cmd_help.lua help
  sysbench * (glob)
  
  function help()
