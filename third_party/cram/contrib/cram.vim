" Vim syntax file
" Language: Cram Tests
" Author: Steve Losh (steve@stevelosh.com)
"
" Add the following line to your ~/.vimrc to enable:
" au BufNewFile,BufRead *.t set filetype=cram
"
" If you want folding you'll need the following line as well:
" let cram_fold=1
"
" You might also want to set the starting foldlevel for Cram files:
" autocmd Syntax cram setlocal foldlevel=1

if exists("b:current_syntax")
  finish
endif

syn include @Shell syntax/sh.vim

syn match cramComment /^[^ ].*$/ contains=@Spell
syn region cramOutput start=/^  [^$>]/ start=/^  $/ end=/\v.(\n\n*[^ ])\@=/me=s end=/^  [$>]/me=e-3 end=/^$/ fold containedin=cramBlock
syn match cramCommandStart /^  \$ / containedin=cramCommand
syn region cramCommand start=/^  \$ /hs=s+4,rs=s+4 end=/^  [^>]/me=e-3 end=/^  $/me=e-2 containedin=cramBlock contains=@Shell keepend
syn region cramBlock start=/^  /ms=e-2 end=/\v.(\n\n*[^ ])\@=/me=s end=/^$/me=e-1 fold keepend

hi link cramCommandStart Keyword
hi link cramComment Normal
hi link cramOutput Comment

if exists("cram_fold")
  setlocal foldmethod=syntax
endif

syn sync match cramSync grouphere NONE "^$"
syn sync maxlines=200

" It's okay to set tab settings here, because an indent of two spaces is specified
" by the file format.
setlocal tabstop=2 softtabstop=2 shiftwidth=2 expandtab

let b:current_syntax = "cram"
