---
title: Functions and expressions
---


Lean-inspired `def`, `fun`, `inductive`, `where`, `with`, `mut`, and `:=` forms are described in the [Lean-inspired programming guide](lean-inspired.md). Legacy syntax remains supported.
# Axiom functions and expressions

Axiom supports `def` and `fn` for named functions, and `fun` and `fn` for anonymous functions. `let x := value` creates immutable locals; `let mut x := value` and legacy `let x = value` create mutable locals; `const` creates immutable bindings. Function names, parameter lists, arrows, braces, and indentation follow the [style guide](style-guide.md).

## Named functions

```text
fn add(left: i32, right: i32) = left + right;

fn rec factorial(n: i32) -> i32 {
    if n <= 1 {
        1
    } else {
        n * factorial(n - 1)
    }
}

fn main() = {
    println(add(20, 22));
    println(factorial(5));
    0
};
```

Parameters require types. An `= expression` body infers its result when the annotation is omitted. Brace bodies default to `unit`/`void` unless a result type is annotated. A final expression without a semicolon supplies the result. Explicit `return expression;` remains available. The final conditional in a function body may return through both branches; every normal path of a non-unit function must return a compatible value.

Direct and mutual recursion require `fn rec` on every function in the cycle. Recursive result types must be explicit. Recursion does not imply tail-call optimization; use loops for unbounded iteration. `main` has the same declaration and inference rules as every other function: `def main { ...; 0 }` is the standard entry form; legacy `fn` forms remain accepted. All executable entry points are named `main`, have no parameters, and return inferred or annotated `unit` or `i32`. Modules may not declare a second entry point. `check` also accepts library files without main; building an executable requires it.

## Closures and function types

```text
fn apply(value: i32, transform: Fn<i32, i32>) = transform(value);
fn make_adder(offset: i32) -> Fn<i32, i32> =
    fn(value: i32) => value + offset;
```

`Fn<A, B, R>` represents a function with two parameters and result `R`; `Fn<R>` takes no parameters. Lambdas use `fn(parameters) => expression`, with explicitly typed parameters and an inferred result. Named functions can also be passed as values, assigned to local variables, and returned. Calls use parentheses. Types are monomorphic: generic Axiom functions, automatic currying, and polymorphic inference are not implemented.

Closures capture surrounding values by copy and cannot assign to those captured bindings. A closure can outlive its defining function. Mutating an outer scalar later does not alter the captured copy. Copying owning handles retains shared storage, so other aliases can still change that storage. Lambda parameters and locally declared `let` variables are mutable. Capture-by-reference and recursive local bindings are not supported. Function values cannot be evaluated by the scalar compile-time interpreter.

## Expression blocks and conditionals

```text
fn choose(flag: bool) = if flag { 42 } else { 0 };
fn calculate() = {
    let number = 20;
    number += 22;
    number
};
```

A value block scopes declarations and returns its final expression. A block without a final value has type `unit`. `if condition { value } else { value }` and `if condition then value else value` evaluate only the selected branch. Branch types must agree or allow numeric widening. `return` cannot escape a value block or lambda; use its final expression. Loops inside a value block support their own `break` and `continue`, but those cannot escape into surrounding loops.

The ML spelling `let let x = 42 in x end` remains accepted; prefer braces for new code. The compatibility spelling `val x = 42` is immutable. `int`, `real`, and `unit` alias `i32`, `f64`, and `void` in annotations. `unit` describes an effect-only result and is not a storable value in this version.

## Record types

TypeScript-style record declarations use `type`:

```text
type User = {
    name: string;
    age: i32;
};

fn display(user: User) = println(user.name);
fn main() = display(User("Ada", 37));
```

Record constructors take field values in declaration order. Field access and mutation follow the existing value-struct rules. This form declares records; it does not yet provide TypeScript-style unions, optional properties, or structural typing.

`main` uses the same expression body as any function. A `unit` result exits with status 0; an `i32` result supplies the process exit code.

## Datatypes and pattern matching

```text
datatype Result = Ok of i32 | Error of string;

fn describe(result: Result) =
    case result of
        Result::Ok(value) => "value: " + str(value)
      | Result::Error(message) => message;
```

`datatype` defines a checked tagged union. Multiple payload types are separated by `*`; constructor arguments and payload patterns are parenthesized and comma-separated. Qualified constructors avoid ambiguous names. `case … of` and brace-based `match` check exhaustiveness, duplicate patterns, and unreachable arms. Payload bindings are immutable. Scalar patterns and `_` are supported; nested patterns and guards are not.

## Mutation and loops

```text
fn main() = {
    let total = 0;
    for i in 0..5 {
        if i == 2 { continue; }
        total += i;
    }
    while total < 12 {
        total += 1;
    }
    println(total);
    0
};
```

Range bounds must be integers and are evaluated once, left to right. The end is exclusive; reversed and empty ranges execute zero times. The range variable is immutable and scoped to its loop. For mutable loop indices, use the existing C-style `for (let i = 0; i < limit; i++) { ... }`. Both loop forms support `break` and `continue` and work in scalar compile-time evaluation.

Equality uses `==`; assignment uses `=`. `andalso`, `orelse`, `not`, `<>`, and `~` remain aliases for `&&`, `||`, `!`, `!=`, and unary `-`. Comments use `//` and `/* ... */`. Axiom is an ML-family design, not a source-compatible implementation of Standard ML or F#.
