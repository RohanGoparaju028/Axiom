---
title: Current status
---


Lean-inspired `def`, `fun`, `inductive`, `where`, `with`, `mut`, and `:=` forms are described in the [Lean-inspired programming guide](lean-inspired.md). Legacy syntax remains supported.
# Current status and limitations

This documentation describes **Axiom 0.1.0**, reviewed on **31 October 2026**. It describes implemented behavior, not a promise of source compatibility or production readiness.

See [what Axiom supports today](features.md) for the complete feature inventory and usage examples.

The [keyword reference](keywords.md) lists all reserved words and their actual implementation status. The [ML dialect](ml-dialect.md) adds expression syntax while retaining explicit function types and C++ interoperability.

Build-from-source instructions for macOS, Linux, and Windows are in the [installation guide](installation.md).

## Implementation status

| Feature | Status and boundary |
| --- | --- |
| ML dialect | Implemented additive ML-style expressions and datatypes. No full Standard ML compatibility, polymorphic inference, or ML module system. |
| Compiler | Implemented in C++17; emits C++17 and invokes Clang/GCC for native builds. |
| Functions and modules | Typed functions, explicit returns, imports, and `pub` exports. Declarations are private by default. Typed first-class functions and closures capture values by copy. |
| Basic types | `i32`, `i64`, finite `f64`, `bool`, byte strings, and `void` results. |
| Control flow | `if`, `while`, C-style `for`, `break`, `continue`, `switch`, and expression `match`. |
| Aggregates | Value structs and tagged enums; reference-counted classes. No inheritance, instance methods, field visibility, or custom constructors/destructors. |
| Memory | Shared owning `Ptr<T>`, `Weak<T>`, checked access, and automatic destruction at the final owner. No raw pointers, unique ownership, borrowing, or tracing garbage collection. |
| Collections | Array, linked list, deque, stack, queue, hash map, and hash set with shared storage and explicit cloning. |
| Metaprogramming | Global `test` blocks evaluate scalar computations and generate constants; 100,000 steps and 64 nested calls per compilation. No syntax-generating macros or compile-time heap objects. |
| C++ interoperability | Quoted `cpp "header";` includes, automatic discovery of copied C/C++ sources, and explicit scalar function bindings. Direct typed calls also support strings, copyable native objects, methods, and typed callbacks. No automatic API discovery, raw pointer/reference model, or move-only native values. |
| Build tool | Project scaffolding with `cpp/`, native compilation/linking, debug builds, and C++ export to configurable visible `axiom-cxx/` output. Git dependency installation, pinned revisions, and transitive restoration through `axiom get`. No package registry, semantic-version resolver, or incremental build cache. |
| C++ export | Standalone generated C++ is available through `emit-cpp`. Idiomatic multi-file C++ project migration with a standalone build configuration is planned, not implemented. |
| Editors | Syntax highlighting, explicit check/run/build commands, native debugger integration. No language server, Axiom expression evaluator, or custom debugger variable formatter. |

## Memory safety boundaries

Checked pointer and collection access, initialized fields, reference-counted ownership, and checked numeric operations reduce common errors. This is an early implementation without a formal memory-safety proof. C++ code called through `unsafe` can still corrupt memory or crash. Strong reference cycles leak unless a link is weak or ownership is explicitly broken. `const` restricts mutation through one binding; other mutable aliases may change shared contents.

## Determinism boundaries

For the same source inputs and target behavior, compile-time scalar evaluation has deterministic, bounded execution. It cannot perform I/O, access the environment, or call C++. This does not promise identical runtime output for programs using input or native libraries, identical floating-point results across targets, or byte-identical executables across toolchains.

## Verification and known gaps

The repository contains native runtime lifetime tests, language/project integration tests, editor command tests, and native debug-symbol checks. The development host has passed these suites. Available suites vary with installed tools.

A Windows Vim integration CI failure has been under investigation. These docs do not certify every editor/platform combination; consult [the repository's workflow runs](https://github.com/RohanGoparaju028/Argon/actions/workflows/ci.yml) for the latest results. Documentation deployment runs independently of compiler CI.

Native source mappings and breakpoint resolution have been checked. Live LLDB stepping was not verified on the development host because LLDB could not pause the launched process. Editor debugger support requires the relevant backend and host permissions.

The project does not currently include async/concurrent execution APIs, user-defined generics, aggregate operator overloading, nested/guarded match patterns, or Axiom exception syntax. No release date is promised for unimplemented features.
