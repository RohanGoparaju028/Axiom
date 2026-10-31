# Axiom

Axiom is an experimental programming language for building native programs with concise, strongly typed code. Its Lean-inspired syntax includes expression-based functions, immutable bindings, inductive types, and pattern matching. Projects use `.ax` source files, an `Axiom.toml` manifest, and the `axiom` command.

```text
def add (left right : i32) := left + right;

def main := {
    let answer := add(20, 22);
    println(answer);
};
```

This program prints `42`. Function parameters have explicit types; the compiler infers the result. Axiom is an early implementation: it does not implement Lean's proof system or dependent types. See [current capabilities and limits](docs/status.md).

## Install

You need Git, CMake 3.16 or newer, a C++17-capable Clang or GCC compiler, and Make or Ninja. Axiom currently compiles through C++17, so keep the native compiler installed to build your programs.

On macOS or Linux, once those prerequisites are installed:

```sh
git clone https://github.com/RohanGoparaju028/Argon.git Axiom
cd Axiom
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
cmake --install build --prefix "$HOME/.local"
export PATH="$HOME/.local/bin:$PATH"
axiom --version
axiom doctor
```

The repository still lives at its original GitHub URL. If you already have a checkout, start with the CMake commands in that directory. Add the `export PATH` line to your shell configuration to make it persistent. The build also provides `axiomc`, which accepts the same commands.

For prerequisite installation and platform-specific setup, including Windows through MSYS2 UCRT64, follow the [installation guide](docs/installation.md). To try Axiom without installing it, run `./build/axiom run examples/lean.ax` from the repository.

## Write your first project

With `axiom` on your PATH, open the directory where you keep your projects and run:

```sh
mkdir hello-axiom
cd hello-axiom
axiom mod init hello-axiom
mkdir src
cat > src/main.ax <<'AX'
def main := println("Hello from Axiom!");
AX
axiom run
```

Output:

```text
Hello from Axiom!
```

These commands use Bash/zsh syntax; they also work in the Windows UCRT64 Bash terminal. You can create `src/main.ax` in an editor instead.

`axiom mod init` creates **only the manifest**. It preserves your existing files. For a starter project with source files already generated, use `axiom new hello-axiom` instead; the destination must not exist.

The manifest for this project is:

```toml
[project]
schema = 1
name = "hello-axiom"
entry = "src/main.ax"

[dependencies]
```

`main` takes no parameters. An effect-only body, such as the `println` above, exits successfully. Returning an `i32` supplies the process exit status.

## Check, build, and run

Run these commands from your project directory:

| Command | What it does |
| --- | --- |
| `axiom check` | Check Axiom types and evaluate global compile-time `test` blocks |
| `axiom run` | Build and run the project |
| `axiom build` | Produce a native executable |
| `axiom build --debug` | Build with debug information |
| `axiom get` | Restore dependencies pinned in the manifest |
| `axiom emit-cpp --stdout` | Show the generated C++ |

For `hello-axiom`, the release executable is `axiom-cxx/release/hello-axiom` (`hello-axiom.exe` on Windows). Debug builds go in `axiom-cxx/debug/`. `run` rebuilds before executing. `check` does not compile or link native C++ bindings.

You can also run a single file without creating a manifest:

```sh
axiom run hello.ax
```

Ignore generated files in version control:

```gitignore
.axiom/
axiom-cxx/
```

Commit your `.ax` source files and `Axiom.toml`.

## Learn the language

Replace `src/main.ax` with this complete example:

```text
inductive Outcome where
    | Success (value : i32)
    | Failure (message : string);

def describe (result : Outcome) := match result with
    | Outcome::Success(value) => "answer: " + str(value)
    | Outcome::Failure(message) => message;

def main := {
    let double := fun (value : i32) => value * 2;
    let mut total := 0;
    for i in 0..5 { total += i; }
    println(describe(Outcome::Success(double(total))));
};
```

Run `axiom run`; it prints `answer: 20`.

- `def` defines a function; `fun` creates a closure.
- `let name := value` is immutable. Use `let mut` when you need assignment.
- `inductive` defines a tagged union; `match` checks that all variants are covered.
- `0..5` includes 0 through 4. Blocks use braces and calls use parentheses.
- A function's final expression is its result. Recursive `def` functions require an explicit result type.

For a local module, create `src/math.ax`:

```text
pub def double (value : i32) := value * 2;
```

Then use it from `src/main.ax`:

```text
import math;
def main := println(math.double(21));
```

`pub` makes a declaration available to importing modules. This program prints `42`.

Continue with the [Lean-inspired language guide](docs/lean-inspired.md), [memory and collections](docs/memory.md), and [runnable examples](examples/README.md). Existing `fn` syntax remains supported; legacy `let x = value` is mutable, unlike `let x := value`.

## Add dependencies

Axiom fetches libraries from Git repositories. Replace the example URL below with an actual Axiom library repository:

```sh
axiom get https://github.com/your-team/math.git --name math
# Select a tag, branch, or commit explicitly:
axiom get https://github.com/your-team/math.git --name math --rev v1.0.0
```

A local Git repository also works:

```sh
axiom get ../math --name math
```

The repository must contain `Axiom.toml` pointing to its library entry, or a `src/lib.ax` / `lib.ax` file. If it exports `pub def double`, you can use the same `import math;` program shown above. Remove a local `src/math.ax` when switching to the dependency: local modules take precedence.

`get` records the exact commit in `Axiom.toml` and downloads sources into `.axiom/deps/`. After cloning an application, run `axiom get` to restore its pinned dependencies, including transitive dependencies, then `axiom run`. Builds use the cache without fetching over the network.

There is no package registry or semantic-version range resolver yet. Local Git paths are recorded as absolute paths; use remote URLs for shared projects. See [modules and dependencies](docs/modules.md) for library authoring and updates.

## Editors and native libraries

Set up [VS Code](docs/vscode.md) or [Vim/Neovim](docs/vim.md) for `.ax` highlighting, checks, run commands, and native debugging. Axiom does not yet provide a language server.

For existing C++ libraries, `axiom new native-demo --cpp` creates an integration example. Native calls use an explicit `unsafe` boundary. See [C++ integration](docs/cpp.md); ordinary Axiom programs can use the language without writing C++.

## More documentation

- [Getting started](docs/getting-started.md) and [installation](docs/installation.md)
- [Modules and dependencies](docs/modules.md) and [build configuration](docs/build.md)
- [Language reference](docs/language.md) and [keywords](docs/keywords.md)
- [Current status and limitations](docs/status.md)

To test the compiler itself, run `ctest --test-dir build --output-on-failure` from its repository after building.

## Migrating older projects

Axiom replaces the ArgsML and Argon names. Rename `ArgsML.toml` or `Argon.toml` to `Axiom.toml`, rename `.argsml` source files to `.ax`, and update the manifest's entry path. Use `axiom` (or `axiomc`) and the `axiom` editor settings. `.ar` and `.fe` remain readable as legacy extensions.
