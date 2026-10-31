# Changelog

## 0.1.0

Initial Axiom release. The language is experimental; future versions may change its syntax and APIs.

### Language

- `.ax` source files and Lean-inspired `def`, `fun`, and `inductive` declarations.
- Direct function bodies, including `def main { ... }`, without `:=` or a required trailing semicolon.
- Inferred function results, explicitly typed parameters, immutable `let ... :=` bindings, and explicit `let mut` mutation.
- Tagged unions, exhaustive pattern matching, closures, local modules, and `pub` exports.
- Checked arithmetic, collections, reference-counted storage, and compile-time scalar `test` blocks.

### Projects and tools

- `axiom mod init` creates `Axiom.toml` while preserving existing sources.
- `axiom get` fetches Git libraries, pins commits in the manifest, and restores transitive dependencies.
- `axiom check`, `build`, `run`, and `emit-cpp` support native application development.
- The `axiomc` command remains available as a compatible alternative.
- VS Code and Vim/Neovim integration provides highlighting, checks, run commands, and native debugger integration.

### Compiler fixes

- Ordinary collection methods and qualified module calls are no longer misclassified as native C++ method calls.
- Native method result annotations prefer ordinary C++ calls, with a result-typed template fallback for APIs such as `get<T>()`.
- Generated starter projects use the direct `def main { ... }` syntax.

### Requirements and limitations

Building Axiom requires CMake 3.16 or newer and a C++17-capable Clang or GCC toolchain. Building applications also requires the native toolchain; Git is required to fetch dependencies.

This release does not include Lean's proof system, dependent types, user-defined generics, a language server, a package registry, semantic-version range resolution, or async/concurrent execution APIs. See [current status](docs/status.md) for detailed boundaries.

### Migration

Rename `ArgsML.toml` or `Argon.toml` to `Axiom.toml`, rename `.argsml` files to `.ax`, and update the entry path. Existing `fn` declarations and `def ... := expression` definitions remain accepted. New examples use direct brace bodies for `main`. `.ar` and `.fe` remain readable as legacy source extensions.
