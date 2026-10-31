---
title: Lean-inspired programming
---

# Lean-inspired programming

Axiom now supports definitions, grouped binders, immutable bindings, and inductive declarations inspired by Lean. These forms use Axiom's existing type checker and native runtime. They are not Lean source compatibility or a proof system.

```text
inductive Outcome where
    | Success (value : i32)
    | Failure (message : string);

def add (left right : i32) : i32 := left + right;

def describe (result : Outcome) := match result with
    | Outcome::Success(value) => "answer: " + str(value)
    | Outcome::Failure(message) => message;

def main := {
    let double := fun (value : i32) => value * 2;
    println(describe(Outcome::Success(add(double(20), 2))));
};
```

## Definitions and functions

`def name (parameters) : Result := expression;` defines a function. Parameter groups may share a type (`(x y : i32)`) or use comma-separated annotations. Multiple groups are flattened into ordinary arguments: `def add (x : i32) (y : i32) := x + y;` is called as `add(1, 2)`. Omit all groups for a parameterless definition, such as `def main := println("hello");`.

Results are inferred when omitted. Recursive and mutually recursive `def` functions need explicit result types but do not need `rec`. Recursion is not termination-checked or guaranteed to use constant stack space.

`fun (x : i32) => x + 1` creates a closure. It supports the same grouped parameters as `def`; parameter annotations remain required. Capture and function-value behavior match existing `fn` closures. There is no automatic currying or whitespace function application.

## Bindings

Inside blocks, `let x := expression;` declares an immutable binding. `let mut x := expression;` explicitly allows mutation. An optional annotation precedes `:=`, as in `let x : i32 := 42;`. Existing assignment operators (`=`, `+=`, and so on) update mutable values.

At global scope, `test` blocks retain their existing rule: direct immutable bindings are exported as compile-time constants and require an explicit type.

The new words `def`, `fun`, `inductive`, `where`, `with`, and `mut` are reserved and must be renamed if previously used as identifiers.

For compatibility, legacy `let x = expression;` remains mutable. `const` and `val` remain immutable. Immutability restricts mutation through that binding; shared handles can still be changed through another mutable alias. This is not deep immutability.

## Inductive types and matching

`inductive Name where | First | Second (value : i32);` declares a tagged union. Constructors use qualified names, such as `Name::Second(42)`. Payload binders support shared types and multiple groups. The declaration must contain at least one constructor.

`match value with | Name::First => 0 | Name::Second(x) => x;` uses the existing exhaustive pattern checker. Use parentheses around nested matches when branch boundaries would otherwise be ambiguous. Brace-based `match` remains supported. Patterns are still one level deep.

## Scope

These changes add a practical Lean-inspired surface to the existing language. Blocks still use braces, bindings use semicolons, and function calls use parentheses. Dependent types, proofs, tactics, implicit parameters, generic inductive types, and Lean's `do` notation are not implemented. Direct recursive value layouts remain invalid; use the existing owning handles for recursive storage.

Run the complete example with `axiomc run examples/lean.ax`.
