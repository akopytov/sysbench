Simple commands:

  $ echo foo
  foo
  $ printf 'bar\nbaz\n' | cat
  bar
  baz

Multi-line command:

  $ foo() {
  >     echo bar
  > }
  $ foo
  bar

Regular expression:

  $ echo foobarbaz
  foobar.* (re)

Glob:

  $ printf '* \\foobarbaz {10}\n'
  \* \\fo?bar* {10} (glob)

Literal match ending in (re) and (glob):

  $ echo 'foo\Z\Z\Z bar (re)'
  foo\Z\Z\Z bar (re)
  $ echo 'baz??? quux (glob)'
  baz??? quux (glob)

Exit code:

  $ (exit 1)
  [1]

Write to stderr:

  $ echo foo >&2
  foo

No newline:

  $ printf foo
  foo (no-eol)
  $ printf 'foo\nbar'
  foo
  bar (no-eol)
  $ printf '  '
     (no-eol)
  $ printf '  \n  '
    
     (no-eol)
  $ echo foo
  foo
  $ printf foo
  foo (no-eol)

Escaped output:

  $ printf '\00\01\02\03\04\05\06\07\010\011\013\014\016\017\020\021\022\n'
  \x00\x01\x02\x03\x04\x05\x06\x07\x08\t\x0b\x0c\x0e\x0f\x10\x11\x12 (esc)
  $ printf '\023\024\025\026\027\030\031\032\033\034\035\036\037\040\047\n'
  \x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f ' (esc)
  $ echo hi
  \x68\x69 (esc)
  $ echo '(esc) in output (esc)'
  (esc) in output (esc)
  $ echo '(esc) in output (esc)'
  (esc) in output \x28esc\x29 (esc)

Command that closes a pipe:

  $ cat /dev/urandom | head -1 > /dev/null

If Cram let Python's SIGPIPE handler get inherited by this script, we
might see broken pipe messages.
