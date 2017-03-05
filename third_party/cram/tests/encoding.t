Set up cram alias and example tests:

  $ . "$TESTDIR"/setup.sh

Test with Windows newlines:

  $ printf "  $ echo hi\r\n  hi\r\n" > windows-newlines.t
  $ cram windows-newlines.t
  .
  # Ran 1 tests, 0 skipped, 0 failed.

Test with Latin-1 encoding:

  $ cat > good-latin-1.t <<EOF
  >   $ printf "hola se\361or\n"
  >   hola se\xf1or (esc)
  > EOF

  $ cat > bad-latin-1.t <<EOF
  >   $ printf "hola se\361or\n"
  >   hey
  > EOF

  $ cram good-latin-1.t bad-latin-1.t
  .!
  --- bad-latin-1.t
  +++ bad-latin-1.t.err
  @@ -1,2 +1,2 @@
     $ printf "hola se\361or\n"
  -  hey
  +  hola se\xf1or (esc)
  
  # Ran 2 tests, 0 skipped, 1 failed.
  [1]

Test with UTF-8 encoding:

  $ cat > good-utf-8.t <<EOF
  >   $ printf "hola se\303\261or\n"
  >   hola se\xc3\xb1or (esc)
  > EOF

  $ cat > bad-utf-8.t <<EOF
  >   $ printf "hola se\303\261or\n"
  >   hey
  > EOF

  $ cram good-utf-8.t bad-utf-8.t
  .!
  --- bad-utf-8.t
  +++ bad-utf-8.t.err
  @@ -1,2 +1,2 @@
     $ printf "hola se\303\261or\n"
  -  hey
  +  hola se\xc3\xb1or (esc)
  
  # Ran 2 tests, 0 skipped, 1 failed.
  [1]

Test file missing trailing newline:

  $ printf '  $ true' > passing-with-no-newline.t
  $ cram passing-with-no-newline.t
  .
  # Ran 1 tests, 0 skipped, 0 failed.

  $ printf '  $ false' > failing-with-no-newline.t
  $ cram failing-with-no-newline.t
  !
  --- failing-with-no-newline.t
  +++ failing-with-no-newline.t.err
  @@ -1,1 +1,2 @@
     $ false
  +  [1]
  
  # Ran 1 tests, 0 skipped, 1 failed.
  [1]
