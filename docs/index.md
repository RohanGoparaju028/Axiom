---
title: Overview
---

# Axiom 0.1.0

Axiom is an experimental language for building native applications with concise, strongly typed code. Its Lean-inspired syntax has named functions, immutable bindings, closures, and tagged data types. Source files use `.ax`; projects use `Axiom.toml` and the `axiom` command. The compiler emits C++17 and invokes Clang or GCC to build native programs.

## Download Axiom

Axiom 0.1.0 has tested compiler archives for three platforms:

| Platform | Download |
| --- | --- |
| Linux x86_64 | [axiom-0.1.0-linux-x86_64.zip](https://github.com/RohanGoparaju028/Axiom/releases/download/v0.1.0/axiom-0.1.0-linux-x86_64.zip) |
| macOS ARM64 | [axiom-0.1.0-macos-arm64.zip](https://github.com/RohanGoparaju028/Axiom/releases/download/v0.1.0/axiom-0.1.0-macos-arm64.zip) |
| Windows x86_64 | [axiom-0.1.0-windows-x86_64.zip](https://github.com/RohanGoparaju028/Axiom/releases/download/v0.1.0/axiom-0.1.0-windows-x86_64.zip) |

Extract the archive and put its `bin` directory on your `PATH`. Keep a C++17 Clang or GCC toolchain installed to build applications. Windows users can set up the UCRT64 toolchain using the [installation guide](installation.md#windows). View the [release and its checksums](https://github.com/RohanGoparaju028/Axiom/releases/tag/v0.1.0).

## Your first program

Create a project directory and manifest:

```sh
mkdir hello
cd hello
axiom mod init hello
mkdir src
```

Save this as `src/main.ax`:

```text
let add (left right : i32) := left + right;
let main := println(add(20, 22));
```

Run it:

```sh
axiom run
```

It prints `42`. The program defines `add` and its entry point with `let`; the parameter types are explicit and the return types are inferred. For a generated starter project, use `axiom new hello` in the parent directory instead.

## Learn Axiom

1. Follow [Getting started](getting-started.md) to create, check, build, and run a project.
2. Learn the [Lean-inspired syntax](lean-inspired.md), then use the [language reference](language.md).
3. Add and restore Git dependencies with [modules and dependencies](modules.md).
4. Explore the [feature guide](features.md), [runnable examples](examples.md), and [current limits](status.md).
5. Set up [VS Code](vscode.md) or [Vim/Neovim](vim.md).

Axiom takes inspiration from Lean but does not include Lean's proof system or dependent types. It can call into existing C++ libraries through the optional [native integration](cpp.md) boundary. For compiler installation from source, see [Installation](installation.md).

Axiom is licensed under [Apache-2.0](https://github.com/RohanGoparaju028/Axiom/blob/main/LICENSE). The source is on [GitHub](https://github.com/RohanGoparaju028/Axiom).
