Output needing escaping:

  $ printf '\00\01\02\03\04\05\06\07\010\011\013\014\016\017\020\021\022\n'
  foo
  $ printf '\023\024\025\026\027\030\031\032\033\034\035\036\037\040\047\n'
  bar

Wrong output and bad regexes:

  $ echo 1
  2
  $ printf '1\nfoo\n1\n'
  +++ (re)
  foo\ (re)
   (re)

Filler to force a second diff hunk:


Offset regular expression:

  $ printf 'foo\n\n1\n'
  
  \d (re)
