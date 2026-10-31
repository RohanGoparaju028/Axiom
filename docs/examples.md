---
title: Examples
---

# Axiom examples

For complete introductory programs with expected output and individual downloads, use the [keyword reference](keywords.md).

From the repository root, run `./build/axiomc run examples/NAME.ar` (or use the editor's Axiom runner).

| File | Demonstrates |
| --- | --- |
| [native_files/](https://github.com/RohanGoparaju028/Axiom/tree/main/examples/native_files) | Complete project with local headers, C/C++ implementations, a header-only function, and a standard-library call |
| [tour.ax](https://github.com/RohanGoparaju028/Axiom/blob/main/examples/tour.ax) | Compile-time blocks, tagged enums, matches, structs, loops, arrays, switches |
| [pointers.ax](https://github.com/RohanGoparaju028/Axiom/blob/main/examples/pointers.ax) | Owning pointers, shared mutation, weak upgrade, expiry, and checked access |
| [family.ax](https://github.com/RohanGoparaju028/Axiom/blob/main/examples/family.ax) | Reference-counted classes with a weak parent link to avoid cycles |
| [linked_nodes.ax](https://github.com/RohanGoparaju028/Axiom/blob/main/examples/linked_nodes.ax) | A recursive struct linked by typed owning pointers |
| [collections.ax](https://github.com/RohanGoparaju028/Axiom/blob/main/examples/collections.ax) | Maps, sets, linked lists, stacks, queues, and deques |
| [shared_collections.ax](https://github.com/RohanGoparaju028/Axiom/blob/main/examples/shared_collections.ax) | Shared storage, explicit cloning, and weak collection observers |
| [debugging.ax](https://github.com/RohanGoparaju028/Axiom/blob/main/examples/debugging.ax) | A small program for source breakpoints and stepping |

`axiomc new native-demo --cpp` creates a complete additional example with an Axiom caller and C++ header/source compiled together by the build tool. See [C++ interoperability](cpp.md).

## Read and run the tour

Save the following as `tour.ax`, then run `axiomc run tour.ax`. It prints `720`, `value: 42`, and `four` on separate lines.

```text
struct Point {
    x: i32;
    y: i32;
}

enum Result {
    Ok(value: i32),
    Err(message: string),
}

fn rec factorial(n: i64) -> i64 {
    if (n <= 1) { return 1; }
    return n * factorial(n - 1);
}

test {
    let n = 6;
    const computed: i64 = factorial(n);
    assert(computed == 720);
}

fn describe(result: Result) -> string {
    return match result {
        Result::Ok(value) => "value: " + str(value),
        Result::Err(message) => "error: " + message,
    };
}

fn main() -> i32 {
    println(computed);
    let point = Point(3, 4);
    let copy = point;
    copy.x = 9;
    assert(point.x == 3);

    let values = Array<i32>();
    for (let i = 0; i < 3; i++) {
        values.push(i * 2);
    }
    values[1] += 40;
    println(describe(Result::Ok(values[1])));

    switch (point.y) {
        case 4: println("four"); break;
        default: println("other");
    }
    return 0;
}
```
