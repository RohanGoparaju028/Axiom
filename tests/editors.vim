execute 'set runtimepath^=' . fnameescape($AXIOM_EDITOR_ROOT)
filetype plugin on
syntax on
let g:axiom_compiler = $AXIOM_COMPILER
execute 'edit ' . fnameescape($AXIOM_EXAMPLE)
call assert_equal('axiom', &filetype)
call assert_equal('axiom', get(b:, 'current_syntax', ''))
call assert_equal('// %s', &commentstring)
call assert_equal(2, exists(':AxiomRun'))
call assert_equal(2, exists(':AxiomDebug'))
call assert_equal(2, exists(':AxiomRunProject'))
AxiomCheck
call assert_equal([], getqflist())
let s:bad = tempname() . '.ax'
call writefile(['fn main()->i32{return unknown_name;}'], s:bad)
execute 'edit ' . fnameescape(s:bad)
" Diagnostics must work even when Vim only redirects stdout by default.
let &shellredir = '>%s'
try
  AxiomCheck
catch
  echom v:exception
  " The explicit command reports failure; the diagnostic must still be navigable.
endtry
call assert_equal(1, len(getqflist()))
if !empty(getqflist())
  call assert_equal(1, getqflist()[0].lnum)
  call assert_match('unknown', getqflist()[0].text)
endif
call delete(s:bad)
if has('nvim')
  lua << EOF
local called = false
package.loaded.dap = { adapters = {}, run = function(config)
  assert(config.program == '/test/program')
  assert(config.cwd == '/test')
  assert(config.preRunCommands[1]:find('ag_fn'))
  called = true
end }
vim.g.ax_dap_command = vim.v.progpath
assert(require('axiom').debug({program = '/test/program', cwd = '/test'}))
assert(called)
EOF
endif
if !empty(v:errors)
  for error in v:errors | echom error | endfor
  cquit 1
endif
qa!
