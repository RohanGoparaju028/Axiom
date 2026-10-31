# Axiom in Vim and Neovim

The same runtime plugin supports both editors: file detection, syntax highlighting, indentation/comment settings, checks with quickfix diagnostics, project/file running, and native debugging.

## Install

Add this directory to your runtime path before enabling filetype plugins:

```vim
set runtimepath+=/absolute/path/to/Axiom/editors/vim
filetype plugin on
syntax on
let g:axiom_compiler = '/absolute/path/to/Axiom/build/axiomc'
```

For Neovim Lua configuration:

```lua
vim.opt.rtp:append('/absolute/path/to/Axiom/editors/vim')
vim.g.ax_compiler = '/absolute/path/to/Axiom/build/axiomc'
vim.cmd('filetype plugin on')
vim.cmd('syntax on')
```

You can instead install this directory as a normal Vim/Neovim package. If no compiler is configured, the plugin searches ancestor `build/axiomc` paths, then PATH. Windows uses `axiomc.exe`.

## Commands

| Command | Action |
| --- | --- |
| `:AxiomRun` | Build and run the current file in a terminal split |
| `:AxiomRunProject` | Build and run the nearest manifest's entry |
| `:AxiomCheck` | Check the file and execute its compile-time blocks |
| `:AxiomCheckProject` | Check the manifest's entry |
| `:AxiomBuild` | Build the project |
| `:AxiomDebug` | Debug the current file |
| `:AxiomDebugProject` | Debug the project |

Commands save the current buffer; save modified imported modules before invoking a project command. Build/check errors populate quickfix and open it on failure. Use `:cnext`, `:cprev`, and `:copen` to navigate. Optional mappings can use `nnoremap <buffer> <F5> :AxiomDebug<CR>` and `<F6>` for `:AxiomRun`.

## Debugging

Vim needs terminal support (`+terminal`) and LLDB. Neovim also supports this terminal workflow without additional plugins. `:AxiomDebug` first builds an unoptimized executable with source mappings, then opens LLDB, sets a breakpoint in the Axiom main function, and runs to it. Commands such as `breakpoint set --file pointers.ax --line 10`, `next`, `step`, `frame variable`, and `continue` operate on the native program. Set `g:axiom_lldb` for a non-default LLDB executable. Plugin-owned temporary debug files are removed when the editor exits.

Neovim optionally uses [nvim-dap](https://github.com/mfussenegger/nvim-dap) when it is installed and an `lldb-dap` executable is available. Set:

```lua
vim.g.ax_dap_command = '/absolute/path/to/lldb-dap'
```

Then `:AxiomDebug` uses DAP instead of a terminal debugger. Use nvim-dap's `toggle_breakpoint()`, `continue()`, `step_over()`, and `step_into()` functions and your preferred DAP UI. There is no bundled adapter or automatic debugger download. The default adapter command is `lldb-dap`; on macOS, `xcrun -f lldb-dap` can locate it. See the [official LLDB DAP guide](https://lldb.llvm.org/use/lldbdap.html).

Debuggers show generated C++ representations, including source names within generated local identifiers. There is no Axiom expression evaluator or custom variable formatter yet. Build/run/check only happen on explicit commands, not file-opening or save autocommands.
