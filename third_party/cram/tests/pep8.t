Skip this test if pep8 isn't available:

  $ command -v pep8 > /dev/null || exit 80

Check that the Python source code style is PEP 8 compliant:

  $ pep8 --config="$TESTDIR/.."/setup.cfg --repeat "$TESTDIR/.."
