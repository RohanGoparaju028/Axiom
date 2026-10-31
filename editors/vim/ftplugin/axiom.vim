if exists('b:did_axiom_ftplugin') | finish | endif
let b:did_axiom_ftplugin = 1
setlocal commentstring=//\ %s
setlocal comments=s1:/*,mb:*,ex:*/,://
setlocal suffixesadd=.ax,.ar,.fe
setlocal expandtab shiftwidth=4 softtabstop=4
command! -buffer AxiomRun call axiom#run(0)
command! -buffer AxiomRunProject call axiom#run(1)
command! -buffer AxiomCheck call axiom#check(0)
command! -buffer AxiomCheckProject call axiom#check(1)
command! -buffer AxiomBuild call axiom#build(1)
command! -buffer AxiomDebug call axiom#debug(0)
command! -buffer AxiomDebugProject call axiom#debug(1)
let b:undo_ftplugin = 'setlocal commentstring< comments< suffixesadd< expandtab< shiftwidth< softtabstop< | silent! delcommand AxiomRun | silent! delcommand AxiomRunProject | silent! delcommand AxiomCheck | silent! delcommand AxiomCheckProject | silent! delcommand AxiomBuild | silent! delcommand AxiomDebug | silent! delcommand AxiomDebugProject | unlet! b:did_axiom_ftplugin'
