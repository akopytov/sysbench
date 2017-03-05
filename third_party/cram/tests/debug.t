Set up cram alias and example tests:

  $ . "$TESTDIR"/setup.sh

Debug mode:

  $ printf '  $ echo hi\n  > echo bye' > debug.t
  $ cram -d -v debug.t
  options --debug and --verbose are mutually exclusive
  [2]
  $ cram -d -q debug.t
  options --debug and --quiet are mutually exclusive
  [2]
  $ cram -d -i debug.t
  options --debug and --interactive are mutually exclusive
  [2]
  $ cram -d --xunit-file==cram.xml debug.t
  options --debug and --xunit-file are mutually exclusive
  [2]
  $ cram -d debug.t
  hi
  bye
  $ cram -d examples/empty.t

Debug mode with extra shell arguments:

  $ cram --shell-opts='-s' -d debug.t
  hi
  bye

Test debug mode with set -x:

  $ cat > set-x.t <<EOF
  >   $ echo 1
  >   1
  >   $ set -x
  >   $ echo 2
  > EOF
  $ cram -d set-x.t
  1
  \+.*echo 2 (re)
  2

Test set -x without debug mode:

  $ cram set-x.t
  !
  --- set-x.t
  +++ set-x.t.err
  @@ -1,4 +1,8 @@
     $ echo 1
     1
     $ set -x
  \+  \+.*echo  \(no-eol\) (re)
     $ echo 2
  \+  \+.*echo 2 (re)
  +  2
  \+  \+.*echo  \(no-eol\) (re)
  
  # Ran 1 tests, 0 skipped, 1 failed.
  [1]

Note that the "+ echo  (no-eol)" lines are artifacts of the echo commands
that Cram inserts into the test in order to track output. It'd be nice if
Cram could avoid removing salt/line number/return code information from those
lines, but it isn't possible to distinguish between set -x output and normal
output.
