Set up cram alias and example tests:

  $ . "$TESTDIR"/setup.sh

Usage:

  $ cram -h
  [Uu]sage: cram \[OPTIONS\] TESTS\.\.\. (re)
  
  [Oo]ptions: (re)
    -h, --help          show this help message and exit
    -V, --version       show version information and exit
    -q, --quiet         don't print diffs
    -v, --verbose       show filenames and test status
    -i, --interactive   interactively merge changed test output
    -d, --debug         write script output directly to the terminal
    -y, --yes           answer yes to all questions
    -n, --no            answer no to all questions
    -E, --preserve-env  don't reset common environment variables
    -e, --no-err-files  don't write .err files on test failures
    --keep-tmpdir       keep temporary directories
    --shell=PATH        shell to use for running tests (default: /bin/sh)
    --shell-opts=OPTS   arguments to invoke shell with
    --indent=NUM        number of spaces to use for indentation (default: 2)
    --xunit-file=PATH   path to write xUnit XML output
  $ cram -V
  Cram CLI testing framework (version 0.7)
  
  Copyright (C) 2010-2016 Brodie Rao <brodie@bitheap.org> and others
  This is free software; see the source for copying conditions. There is NO
  warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  $ cram
  [Uu]sage: cram \[OPTIONS\] TESTS\.\.\. (re)
  [2]
  $ cram -y -n
  options --yes and --no are mutually exclusive
  [2]
  $ cram non-existent also-not-here
  no such file: non-existent
  [2]
  $ mkdir empty
  $ cram empty
  no tests found
  [2]
  $ cram --shell=./badsh
  shell not found: ./badsh
  [2]
