#!/bin/sh

# Bash doesn't expand aliases by default in non-interactive mode, so
# we enable it manually if the test is run with --shell=/bin/bash.
[ "$TESTSHELL" = "/bin/bash" ] && shopt -s expand_aliases

# The $PYTHON environment variable should be set when running this test
# from Python.
[ -n "$PYTHON" ] || PYTHON="`which python`"
[ -n "$PYTHONPATH" ] || PYTHONPATH="$TESTDIR/.." && export PYTHONPATH
if [ -n "$COVERAGE" ]; then
  if [ -z "$COVERAGE_FILE" ]; then
    COVERAGE_FILE="$TESTDIR/../.coverage"
    export COVERAGE_FILE
  fi

  alias cram="`which "$COVERAGE"` run -a --rcfile=$TESTDIR/../.coveragerc \
$TESTDIR/../scripts/cram --shell=$TESTSHELL"
  alias doctest="`which "$COVERAGE"` run -a --rcfile=$TESTDIR/../.coveragerc \
$TESTDIR/run-doctests.py"
else
  PYTHON="`command -v "$PYTHON" || echo "$PYTHON"`"
  alias cram="$PYTHON $TESTDIR/../scripts/cram --shell=$TESTSHELL"
  alias doctest="$PYTHON $TESTDIR/run-doctests.py"
fi
command -v md5 > /dev/null || alias md5=md5sum

# Copy in example tests
cp -R "$TESTDIR"/../examples .
find . -name '*.err' -exec rm '{}' \;
