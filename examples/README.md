# Axiom examples

From the repository root, run `./build/axiomc run examples/NAME.ar` (or use the editor's Axiom runner).

| File | Demonstrates |
| --- | --- |
| [native_files/](native_files/) | Complete project with local headers, C/C++ implementations, a header-only function, and a standard-library call |
| [axiom.ax](axiom.ax) | Primary Axiom syntax: fn rec, mutable let, range/while loops, and native C++ vector methods |
| [ml_dialect.ax](ml_dialect.ax) | ML-style functions, immutable bindings, datatypes, case expressions, and a C++ call |
| [tour.ax](tour.ax) | Compile-time blocks, tagged enums, matches, structs, loops, arrays, switches |
| [pointers.ax](pointers.ax) | Owning pointers, shared mutation, weak upgrade, expiry, and checked access |
| [family.ax](family.ax) | Reference-counted classes with a weak parent link to avoid cycles |
| [linked_nodes.ax](linked_nodes.ax) | A recursive struct linked by typed owning pointers |
| [collections.ax](collections.ax) | Maps, sets, linked lists, stacks, queues, and deques |
| [shared_collections.ax](shared_collections.ax) | Shared storage, explicit cloning, and weak collection observers |
| [debugging.ax](debugging.ax) | A small program for source breakpoints and stepping |

`axiomc new native-demo --cpp` creates a complete additional example with an Axiom caller and C++ header/source compiled together by the build tool. See [C++ interoperability](../docs/cpp.md).

The [Lean-inspired example](lean.ax) demonstrates `def`, grouped parameters, `fun`, inductive types, exhaustive matching, and explicit mutable bindings. Run it with `axiomc run examples/lean.ax`.
