---
title: Projects and builds
---


For Go-style project initialization and Git dependencies, see [Modules and dependencies](modules.md). `axiom` and `axiomc` accept the same commands.
# Axiom Build

`axiomc` creates projects, checks Axiom code, discovers native sources, invokes Clang/GCC, and links an executable. CMake is needed to build the compiler itself; Axiom applications use `Axiom.toml` and `axiomc`.

## Create a project

```sh
axiomc new hello
cd hello
axiomc run
```

Every new project has a `cpp/` directory for native headers and source files. `--cpp` adds a working native binding example. `init [DIRECTORY]` provides the same layout in an existing directory, refusing to overwrite generated files. Existing `.gitignore` files are preserved.

```text
hello/
  Axiom.toml
  .gitignore
  src/main.ax
  cpp/
    README.md
  axiom-cxx/               # Created when building or exporting.
    release/
      program.cpp
      hello               # hello.exe on Windows.
    debug/                # Created by --debug.
```

The builder no longer creates `.ax/`. Existing `.ax/` artifacts are left untouched; they are not reused. New `.gitignore` files ignore `axiom-cxx/`. Add that entry yourself if initializing a directory with an existing ignore file.

## Choose the generated-output directory

The default is `axiom-cxx/`, with separate `release/` and `debug/` profiles. Each successful native build retains `program.cpp` beside the default executable.

Set a persistent project-relative output location when creating a project:

```sh
axiomc new hello --cxx-dir generated-cpp
axiomc init existing-project --cxx-dir generated-cpp
```

Or edit `[project] cxx_dir` in `Axiom.toml`. Relative manifest values resolve from the project root; absolute values are also accepted. The directory need not already exist.

Override it for a single command:

```sh
axiomc build --cxx-dir generated-cpp
axiomc run --cxx-dir "/absolute/path/to/my-output"
axiomc build --debug --cxx-dir generated-cpp
axiomc emit-cpp --cxx-dir generated-cpp
```

Command-line overrides resolve relative to the current working directory. Thus `axiomc build --cxx-dir generated-cpp` writes `generated-cpp/release/program.cpp` and the executable. `--debug` selects `generated-cpp/debug/`.

Output directories must be separate from the project root, `src/`, and `cpp/`; they cannot contain configured native source files. Output-directory symlinks and file aliases are rejected. `-o` selects a specific executable or exported C++ file; it does not change the native build staging directory.

## Native files and the cpp keyword

Copy native library headers and implementations under `cpp/`, retaining any relative directory structure expected by the library. In an Axiom module:

```text
cpp "vendor/math.hpp";
cpp "cmath";
```

Both project and system headers use this quoted syntax. Header lookup uses `cpp/` first, then configured include directories, then the native compiler's standard include paths. Header declarations in imported Axiom modules are included too. Function signatures still need `extern cpp fn` declarations; inclusion does not automatically translate a C++ API into Axiom types.

All `.cpp`, `.cc`, `.cxx`, `.c++`, and `.c` files under `cpp/` are compiled recursively in stable path order. Files also listed in the manifest are deduplicated. C files compile separately as C11 and are linked with the C++17 application. C headers must expose C linkage when included by C++ (usually with an `extern "C"` guard).

Header-only libraries need no additional source files. Copy library implementation files only; examples or tests defining their own `main` conflict with the application's entry point. Native library dependencies still need installation or local sources and the appropriate link configuration. See [C++ interoperability](cpp.md).

## Manifest

```toml
[project]
schema = 1
name = "hello"
entry = "src/main.ax"
cxx_dir = "axiom-cxx"

# Optional: cpp/ sources and its include path are already discovered.
[cpp]
headers = ["cmath"]
sources = ["vendor/extra.cpp"]
include_dirs = ["vendor/include"]
library_dirs = ["vendor/lib"]
libraries = ["example"]
```

For a simple project, omit `[cpp]` entirely and use `cpp "header";` in source. The section above demonstrates extra configuration, so its paths/library must actually exist if you use it. Older manifests listing `native/` sources and headers explicitly still work.

The parser accepts a small TOML subset: `[project]` and `[cpp]` sections, integer schema `1`, double-quoted strings, single-line arrays of strings, and `#` comments outside strings. Unknown sections/keys and duplicate keys are errors.

`name` contains 1–64 ASCII letters, digits, underscores, or hyphens and starts with a letter or digit. `entry` defaults to `src/main.ax`; `cxx_dir` defaults to `axiom-cxx`. Input file/directory paths must be relative, exist, and resolve within the project. The output setting `cxx_dir` is the exception: it may be absolute and need not exist. Headers and library names are resolved by the compiler/linker.

Imports determine the Axiom dependency graph. Existing import rules can reach files outside the project; manifest validation is not a sandbox for source imports.

## Commands and output

| Command | Behavior |
| --- | --- |
| `axiomc check [FILE]` | Type-check and execute global test blocks; no native compiler required |
| `axiomc build [FILE]` | Build `axiom-cxx/release/NAME` by default |
| `axiomc run [FILE]` | Build, then execute with the project root as working directory |
| `axiomc build --debug` | Use the debug profile with `-O0 -g` |
| `axiomc run --debug` | Build and execute the debug profile |
| `axiomc build -o PATH` | Choose an executable path; its parent must exist |
| `axiomc emit-cpp [FILE]` | Write `axiom-cxx/release/program.cpp` by default |
| `axiomc emit-cpp [FILE] -o FILE.cpp` | Write a specific exported C++ file |
| `axiomc emit-cpp [FILE] --stdout` | Print generated C++ instead of writing a file |
| `axiomc FILE.ar [-o PATH]` | Single-file shorthand; default executable is in the invocation directory |

`--cxx-dir DIRECTORY` is available for `new`, `init`, `build`, `run`, `emit-cpp`, and the single-file shorthand. `check` accepts no output options. For `emit-cpp`, choose one of `-o`, `--cxx-dir`, or `--stdout`.

Windows default executables have an `.exe` suffix. Without FILE, commands find the nearest manifest by walking upward from the current directory. With FILE, discovery starts at its containing directory. A standalone file uses that directory as its project root, with adjacent `cpp/` and `axiom-cxx/` directories.

Builds use a temporary staging directory inside the chosen profile. After successful compilation, the builder publishes the generated C++ and atomically replaces the executable. Frontend/native compile failures preserve previous successful artifacts and prevent `run` from launching an old binary. Source files are protected against output overwrites. On macOS, debug builds retain the corresponding `.dSYM` bundle beside the executable.

`AXIOM_CXX` selects one compiler executable path/name, never a shell command. Otherwise discovery searches `clang++`, `g++`, then `c++`. Compiler arguments are passed directly without a shell. Release builds use `-O2`; debug builds use `-O0 -g`. Fast-math is disabled.

## Export and the migration goal

For an application without external native dependencies:

```sh
axiomc emit-cpp
c++ -std=c++17 axiom-cxx/release/program.cpp -o program
./program
```

The generated file contains Axiom's runtime support. Native bindings still require the corresponding headers, include paths, implementations, and libraries when you compile outside `axiomc`. For a simple C++-only library copied into `cpp/`, for example:

```sh
c++ -std=c++17 -I cpp axiom-cxx/release/program.cpp cpp/demo.cpp -o program
```

The intended future migration export is idiomatic, maintainable C++ with organized files and a standalone build configuration. That is not implemented; today's export is generated compiler code.

## Current boundaries

Builds always recheck and rebuild. Git dependencies can be downloaded and pinned with `axiom get`. There is no package registry, semantic-version resolver, incremental build cache, custom build hooks, general compiler-flag interface, or cross-compilation interface. Native dependencies requiring specialized build steps may need to be built separately before linking.

Compile-time computations are deterministic for fixed inputs and target behavior. Generated code is stable for unchanged sources at the same paths; executable bytes and cross-target floating-point results are not promised reproducible.

## Editors

[VS Code](vscode.md) and [Vim/Neovim](vim.md) invoke the same `axiomc` workflow. Existing custom executable paths used by debugger integrations continue to work.
