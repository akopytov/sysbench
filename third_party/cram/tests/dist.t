Skip this test if check-manifest isn't available:

  $ command -v check-manifest > /dev/null || exit 80

Confirm that "make dist" isn't going to miss any files:

  $ check-manifest "$TESTDIR/.."
  lists of files in version control and sdist match
