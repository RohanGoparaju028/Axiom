let s:debugdirs = []

function! axiom#root(directory) abort
  let l:directory = fnamemodify(a:directory, ':p')
  while 1
    if filereadable(l:directory . '/Axiom.toml') | return l:directory | endif
    let l:parent = fnamemodify(l:directory, ':h')
    if l:parent ==# l:directory | return '' | endif
    let l:directory = l:parent
  endwhile
endfunction

function! axiom#compiler() abort
  if exists('g:axiom_compiler') && !empty(g:axiom_compiler) | return g:axiom_compiler | endif
  let l:name = has('win32') ? 'axiomc.exe' : 'axiomc'
  let l:directory = expand('%:p:h')
  while !empty(l:directory)
    if executable(l:directory . '/build/' . l:name) | return l:directory . '/build/' . l:name | endif
    let l:parent = fnamemodify(l:directory, ':h')
    if l:parent ==# l:directory | break | endif
    let l:directory = l:parent
  endwhile
  return l:name
endfunction

function! s:target(project) abort
  let l:file = expand('%:p')
  if empty(l:file) || index(['ax', 'ar', 'fe'], expand('%:e')) < 0 | throw 'Open a saved .ax file first.' | endif
  silent update
  let l:root = axiom#root(fnamemodify(l:file, ':h'))
  if a:project && empty(l:root) | throw 'No Axiom.toml found.' | endif
  return {'file': l:file, 'cwd': empty(l:root) ? fnamemodify(l:file, ':h') : l:root, 'args': a:project ? [] : [l:file], 'compiler': axiom#compiler()}
endfunction

function! s:execute(args, target) abort
  " Vim's system() takes a shell string; escape EVERY argument separately.
  " No compiler invocation is attached to a buffer-open/save autocommand.
  let l:previous = getcwd()
  try
    execute 'lcd ' . fnameescape(a:target.cwd)
    " Capture compiler diagnostics even when shellredir only captures stdout.
    let l:out = system(join(map(copy(a:args), 'shellescape(v:val)'), ' ') . ' 2>&1')
    let l:status = v:shell_error
  finally
    execute 'lcd ' . fnameescape(l:previous)
  endtry
  call setqflist([], 'r', {'title': 'Axiom', 'lines': split(l:out, "\n"), 'efm': '%Eaxiomc: %f:%l:%c: %m,%E%f:%l:%c: error: %m,%W%f:%l:%c: warning: %m,%-G%.%#'})
  if l:status != 0
    copen
    throw empty(l:out) ? 'Axiom command failed.' : l:out
  endif
  return l:out
endfunction

function! s:terminal(args, cwd) abort
  botright new
  if has('nvim')
    call termopen(a:args, {'cwd': a:cwd})
    startinsert
  elseif exists('*term_start')
    call term_start(a:args, {'cwd': a:cwd, 'curwin': 1})
  else
    throw 'This editor needs terminal support for running/debugging.'
  endif
endfunction

function! axiom#run(project) abort
  try
    let l:target = s:target(a:project)
    call s:terminal([l:target.compiler, 'run'] + l:target.args, l:target.cwd)
  catch | echoerr 'Axiom: ' . v:exception | endtry
endfunction

function! axiom#check(project) abort
  try
    let l:target = s:target(a:project)
    echo s:execute([l:target.compiler, 'check'] + l:target.args, l:target)
  catch | echoerr 'Axiom: ' . v:exception | endtry
endfunction

function! axiom#build(project) abort
  try
    let l:target = s:target(a:project)
    echo s:execute([l:target.compiler, 'build'] + l:target.args, l:target)
  catch | echoerr 'Axiom: ' . v:exception | endtry
endfunction

function! axiom#debug(project) abort
  try
    let l:target = s:target(a:project)
    let l:directory = tempname()
    call mkdir(l:directory, 'p')
    call add(s:debugdirs, l:directory)
    let l:binary = l:directory . (has('win32') ? '/program.exe' : '/program')
    call s:execute([l:target.compiler, 'build'] + l:target.args + ['--debug', '-o', l:binary], l:target)
    if has('nvim') && luaeval("require('axiom').debug(_A)", {'program': l:binary, 'cwd': l:target.cwd}) | return | endif
    let l:lldb = get(g:, 'axiom_lldb', 'lldb')
    if !executable(l:lldb) | throw 'Install LLDB or set g:axiom_lldb to its executable.' | endif
    call s:terminal([l:lldb, '-o', 'breakpoint set --func-regex ^ag_fn[0-9]+_main[(][)]$', '-o', 'run', '--', l:binary], l:target.cwd)
  catch | echoerr 'Axiom: ' . v:exception | endtry
endfunction

function! axiom#cleanup() abort
  for l:directory in s:debugdirs | call delete(l:directory, 'rf') | endfor
  let s:debugdirs = []
endfunction
augroup axiom_debug_cleanup
  autocmd!
  autocmd VimLeavePre * call axiom#cleanup()
augroup END
