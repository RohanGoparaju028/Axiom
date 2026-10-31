---
title: Style guide
---

# Axiom style guide

Axiom adopts the layout and naming conventions of the [Rust Style Guide](https://doc.rust-lang.org/style-guide/) and [Rust naming conventions](https://rust-lang.github.io/api-guidelines/naming.html), with its own mutable-binding and function semantics. This is a writing guide; `axiomc fmt` is not implemented.

- Indent with four spaces. Avoid tabs. Aim for 100 columns.
- Use `snake_case` for variables, parameters, functions, modules, and filenames.
- Use `UpperCamelCase` for datatypes, variants, structs, classes, and native type aliases.
- Use `SCREAMING_SNAKE_CASE` for module constants.
- Put the opening brace on the declaration or control-flow line. Put closing braces on their own lines in multiline blocks.
- Write spaces around operators and after commas and colons. Omit spaces between a function name and `(`.
- Prefer `def name (parameters) : Result := expression;` for new code. See the [Lean-inspired guide](lean-inspired.md); existing `fn` syntax remains supported.
- Use a final expression without a semicolon for the return value. Reserve explicit `return` for early exits.
- Write `fn rec` only where recursion is needed; annotate recursive results. Prefer `while` and `for` for ordinary repeated work.
- Prefer `for item in start..end` for integer ranges. Use `while condition` without unnecessary parentheses.
- Prefer immutable `let x := value;` bindings. Use `let mut x := value;` when mutation is needed. Legacy `let x = value;` remains mutable.
- Put native includes, native type aliases, and module imports near the top of a file. Keep `unsafe` regions small and give native aliases descriptive names.

```text
fn rec factorial(n: i32) -> i32 {
    if n <= 1 {
        1
    } else {
        n * factorial(n - 1)
    }
}

fn main() = {
    let total = 0;
    for index in 0..5 {
        total += index;
    }
    println(total);
    0
};
```
