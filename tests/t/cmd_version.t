########################################################################
Test for the 'version' command
########################################################################

  $ sysbench version
  sysbench [.0-9]+(-[a-f0-9]+)? (re)

  $ version=$(sysbench version | cut -d ' ' -f 1,2)
  $ test "$version" = "$SBTEST_VERSION_STRING"
