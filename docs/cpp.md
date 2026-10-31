---
title: C++ interoperability
---

# Use native headers and source files

Axiom projects include a visible `cpp/` directory. Copy a small library's headers and implementation files there, declare its header with `cpp "header";`, and use direct typed calls or explicit `extern cpp fn` signature bindings.

## Direct C++ calls, objects, and callbacks

```text
cpp "vector";
cpp "algorithm";
cpp type IntVector = "std::vector<std::int32_t>";

fn main() -> i32 {
    let answer = unsafe { cpp::std::max<i32>(20, 42) };
    unsafe {
        let values = IntVector();
        values.push_back::<unit>(answer);
        println(values.at::<i32>(0));
        println(values.size::<i64>());
        let text = "Axiom";
        println(text.substr::<string>(0, 4));
    }
    0
}
```

`cpp::namespace::function<Result>(arguments)` emits a direct call. `object.method::<Result>(arguments)` emits a native member call. `Result` is the expected Axiom result type. The native compiler resolves ordinary calls first; typed methods also support a result-typed template fallback described below. Numeric results are checked when converted to Axiom numbers. Strings use `std::string`. Arguments evaluate left to right and are materialized as values.

`cpp type Name = "qualified::Type<Arguments>";` declares a native type alias, including instantiated C++ templates. `Name(arguments)` constructs an object. Prefix the declaration with `pub` to export it from an Axiom module. Native objects use C++ value-copy and RAII destruction semantics; copying a `std::vector` creates an independent vector. Types must support the copies required by the program. Native references do not escape calls: declared result types materialize values. A `const` receiver is emitted as a const reference, so C++ rejects mutating methods on it during the native build.

Typed `Fn<Parameters..., Result>` callbacks cross direct calls as `std::function`; parameter/result types must themselves be supported boundary types. For example, a header can expose:

```cpp
#include <cstdint>
#include <functional>
inline std::int32_t apply_twice(std::function<std::int32_t(std::int32_t)> f,
                                std::int32_t value) {
    return f(f(value));
}
```

Then Axiom can call `cpp::apply_twice<i32>(fn(value: i32) => value + 1, 40)` inside `unsafe`. Returned callbacks can use `Fn<...>` as their annotated result. Captures are copied into the callback, allowing C++ to retain a copy without dangling references to Axiom local variables.

`check` validates Axiom types and the unsafe boundary; it does not parse C++ headers. `build` and `run` validate native names, methods, overloads, constness, copyability, and result conversions with the C++ compiler. Unsupported native APIs produce native compiler diagnostics. There is no automatic header discovery, raw pointer/reference model, move-only ownership, native inheritance syntax, or full F#/.NET-like type metadata integration yet. The older scalar signature bridge below remains supported when exact overload selection is useful.

## A complete working project

```sh
axiomc new native-demo --cpp
cd native-demo
axiomc run
```

This prints `42`. It creates:

```text
native-demo/
  Axiom.toml
  src/main.ax
  cpp/
    README.md
    demo.hpp
    demo.cpp
  axiom-cxx/release/       # Created by axiomc run.
    program.cpp
    native-demo
```

`cpp/demo.hpp` declares the function:

```cpp
#pragma once
#include <cstdint>
namespace demo {
    std::int32_t twice(std::int32_t value);
}
```

`cpp/demo.cpp` implements it:

```cpp
#include "demo.hpp"
std::int32_t demo::twice(std::int32_t value) {
    return value * 2;
}
```

`src/main.ax` includes the header and binds the function:

```text
cpp "demo.hpp";

extern cpp fn twice(value: i32) -> i32 = "demo::twice";

fn main() -> i32 {
    unsafe { println(twice(21)); }
    return 0;
}
```

No native file list is required for files in `cpp/`. The manifest can be just:

```toml
[project]
schema = 1
name = "native-demo"
entry = "src/main.ax"
cxx_dir = "axiom-cxx"
```

## One quoted syntax for every header

```text
cpp "demo.hpp";
cpp "vendor/math.hpp";
cpp "cmath";
```

Declarations belong at module scope. They can appear before/interspersed with the initial import list or among top-level declarations. Header lookup searches `cpp/` first, additional manifest include directories next, and then the native compiler's standard include paths. There is no angle-bracket form in Axiom. Imported Axiom modules contribute their header declarations too.

Header names must be relative include names; absolute paths, parent traversal, and invalid characters are rejected. Inclusion is verified by the native compiler during `build`/`run`, not by `check`. Builtins such as `println`, `length`, and `assert` need no header declarations.

## Standard-library functions

No local `cmath` file is necessary:

```text
cpp "cmath";
extern cpp fn square_root(value: f64) -> f64 = "std::sqrt";

fn main() -> i32 {
    unsafe { println(square_root(81.0)); }
    return 0;
}
```

This prints `9`. The bridge selects the exact overload matching the declared signature. Use `std::int32_t` for `i32`, `std::int64_t` for `i64`, `double` for `f64`, and `bool` for `bool`.

## Header-only libraries

Place this in `cpp/inline_math.hpp`:

```cpp
#pragma once
#include <cstdint>
inline std::int32_t triple(std::int32_t value) {
    return value * 3;
}
```

Then use:

```text
cpp "inline_math.hpp";
extern cpp fn triple(value: i32) -> i32 = "triple";

fn main() -> i32 {
    unsafe { println(triple(14)); }
    return 0;
}
```

No `.cpp` file is needed. Header-only implementations are compiled as part of the generated translation unit. Libraries with templates, macros, or complex argument types may still need a wrapper exposing a supported function signature.

## Copied C and C++ implementations

The builder recursively discovers `.cpp`, `.cc`, `.cxx`, `.c++`, and `.c` files under `cpp/`. C++ sources compile as C++17. C sources compile separately as C11, then their objects are linked with the application. Preserve the library's directory layout so its relative includes still work.

C headers need C linkage when consumed from C++:

```c
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int32_t answer(void);
#ifdef __cplusplus
}
#endif
```

The `.c` implementation can define `int32_t answer(void) { return 42; }`. In Axiom, use `cpp "answer.h";` and `extern cpp fn answer() -> i32 = "answer";`.

Copy only the required implementation files. Every discovered source is compiled; library examples/tests with their own `main` or mutually exclusive platform implementations must be kept outside `cpp/`. The builder does not infer a library's configuration macros or specialized build requirements.

## Additional include paths and installed libraries

Use `[cpp]` when the library needs more than copied files:

```toml
[cpp]
headers = []
sources = ["vendor/extra.cpp"]
include_dirs = ["vendor/include"]
library_dirs = ["vendor/lib"]
libraries = ["example"]
```

These input paths must exist and stay within the project. The header/source arrays remain supported for existing projects. Sources listed explicitly and also discovered under `cpp/` compile once. Library names generate ordinary `-l` options; include/library paths generate `-I`/`-L` options. System native-library installation is not provided; `axiom get` fetches Axiom Git packages and includes their configured native sources in builds.

## Choose where generated C++ goes

```sh
axiomc build --cxx-dir generated-cpp
axiomc run --cxx-dir generated-cpp
axiomc emit-cpp --cxx-dir generated-cpp
```

These write `generated-cpp/release/program.cpp`; builds also create the native executable there by default. For a persistent project setting, use `axiomc new app --cxx-dir generated-cpp` or edit `[project] cxx_dir`. The default is `axiom-cxx`, and debug builds use its `debug/` subdirectory. See [the build reference](build.md).

## Boundary limits

Headers supply declarations to the generated C++ translation unit. Axiom does not automatically import their type information. Use direct calls with annotated results, `cpp type` aliases for native value types, or explicit scalar function bindings. Arbitrary template syntax at call sites, C varargs, raw pointers/references, and move-only objects require a suitable wrapper or additional compiler support.

Foreign calls require a lexical `unsafe` block and cannot run in compile-time `test` blocks. Arbitrary C++ code can corrupt memory or crash. Non-finite floating-point results become checked Axiom errors; standard C++ exceptions reaching the program entry are reported, but native crashes and non-standard exceptions are not converted.

Run the repository's [native-files example](https://github.com/RohanGoparaju028/Argon/tree/main/examples/native_files) to exercise local, header-only, C, and standard-library functionality together.

Typed method calls first try an ordinary C++ method with the supplied arguments. If that call is unavailable, the compiler tries the method template with the annotated result type, supporting APIs such as `value.get::<i32>()`. This preserves result annotations on ordinary methods such as `vector.at::<i32>(0)` and `vector.push_back::<unit>(42)`.
