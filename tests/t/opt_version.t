########################################################################
Test for the --version option
########################################################################

  $ sysbench --version
  sysbench [.0-9]+(-[a-f0-9]+)? (re)

  $ version=$(sysbench --version | cut -d ' ' -f 1,2)
  $ test "$version" = "$SBTEST_VERSION_STRING"
