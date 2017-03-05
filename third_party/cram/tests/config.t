Set up cram alias and example tests:

  $ . "$TESTDIR"/setup.sh

Options in .cramrc:

  $ cat > .cramrc <<EOF
  > [cram]
  > yes = True
  > no = 1
  > indent = 4
  > EOF
  $ cram
  options --yes and --no are mutually exclusive
  [2]
  $ mv .cramrc config
  $ CRAMRC=config cram
  options --yes and --no are mutually exclusive
  [2]
  $ rm config

Invalid option in .cramrc:

  $ cat > .cramrc <<EOF
  > [cram]
  > indent = hmm
  > EOF
  $ cram
  [Uu]sage: cram \[OPTIONS\] TESTS\.\.\. (re)
  
  cram: error: option --indent: invalid integer value: 'hmm'
  [2]
  $ rm .cramrc
  $ cat > .cramrc <<EOF
  > [cram]
  > verbose = hmm
  > EOF
  $ cram
  [Uu]sage: cram \[OPTIONS\] TESTS\.\.\. (re)
  
  cram: error: --verbose: invalid boolean value: 'hmm'
  [2]
  $ rm .cramrc

Options in an environment variable:

  $ CRAM='-y -n' cram
  options --yes and --no are mutually exclusive
  [2]
