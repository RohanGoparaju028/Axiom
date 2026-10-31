---
title: Getting started
---

# Getting started

## Install Axiom

Follow the [installation guide](installation.md) for macOS, Linux, or Windows with MSYS2 UCRT64. Installation provides `axiom` and the compatible `axiomc` command. Verify your terminal setup:

```sh
axiom --version
axiom doctor
```

Keep Clang or GCC installed: Axiom uses it to produce native executables. The commands below use Bash/zsh syntax, also available in the Windows UCRT64 terminal.

## Initialize your project

From the directory where you keep your projects:

```sh
mkdir hello-axiom
cd hello-axiom
axiom mod init hello-axiom
mkdir src
```

`mod init` creates only `Axiom.toml`. Create `src/main.ax` in your editor:

```text
def add (left right : i32) := left + right;

def main := {
    let answer := add(20, 22);
    println(answer);
};
```

Run it:

```sh
axiom run
```

The program prints `42`. Parameters have explicit types; results can be inferred. `let answer := ...` is immutable. `main` takes no arguments; an effect-only result exits with status 0, while an `i32` result becomes the exit status.

If you prefer generated starter files, use `axiom new hello-axiom` in the parent directory instead of the initialization commands above. Its destination must not exist.

## Build and check

```sh
axiom check
axiom build
axiom build --debug
axiom emit-cpp --stdout
```

`check` validates Axiom code and evaluates global compile-time `test` blocks. It does not compile native C++ bindings. Release executables go in `axiom-cxx/release/`; debug builds go in `axiom-cxx/debug/`. `run` builds before executing.

You can also use `axiom run path/to/program.ax` without a manifest. See [build configuration](build.md) for output paths and native source configuration.

## Add another module

Create `src/math.ax`:

```text
pub def double (value : i32) := value * 2;
```

Replace `src/main.ax` with:

```text
import math;
def main := println(math.double(21));
```

Run `axiom run` again; it prints `42`. Only declarations marked `pub` are available to importing modules.

## Use a library

To use an external library, fetch its Git repository. This URL is a placeholder; substitute a real Axiom library:

```sh
axiom get https://github.com/your-team/math.git --name math
```

The repository needs `Axiom.toml` identifying its library entry, or a `src/lib.ax` / `lib.ax` file. If it exports `double`, the import example above works after removing the local `src/math.ax`; local files take precedence over dependency aliases.

`get` pins the resolved commit in `Axiom.toml`. Commit that manifest with your sources and ignore generated directories:

```gitignore
.axiom/
axiom-cxx/
```

To run a freshly cloned application with dependencies:

```sh
axiom get
axiom run
```

See [modules and dependencies](modules.md) for tags, revisions, transitive dependencies, and local Git libraries.

## Continue learning

- [Lean-inspired programming](lean-inspired.md): functions, immutable bindings, inductive types, and matching.
- [Examples](examples.md): programs you can run and modify.
- [VS Code](vscode.md) and [Vim/Neovim](vim.md): editor setup.
- [C++ integration](cpp.md): optional native library bindings.
- [Current limitations](status.md): what is implemented and what remains experimental.
