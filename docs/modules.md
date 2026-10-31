---
title: Modules and dependencies
---

# Modules and dependencies

Use `axiom` for project development. `axiomc` remains available and accepts the same commands. Both are built and installed together. Git must be installed to fetch dependencies.

## Initialize a module

```sh
mkdir hello
cd hello
axiom mod init hello
```

This creates only `Axiom.toml`, preserving existing sources and refusing to overwrite a manifest:

```toml
[project]
schema = 1
name = "hello"
entry = "src/main.ax"

[dependencies]
```

The name is optional and defaults to the directory name. Names contain letters, digits, underscores, or hyphens. If `src/lib.ax`, `lib.ax`, `src/main.ax`, or `main.ax` already exists, the first matching path becomes the entry. Otherwise the default is `src/main.ax`; create that file before building:

```text
def main := println("Hello from Axiom!");
```

```sh
axiom run
axiom check
axiom build
```

For a generated starter program, `axiom new hello` remains available.

## Add a dependency

```sh
# Replace this example URL with a repository containing an Axiom library.
axiom get https://github.com/your-team/math.git --name math
axiom get https://github.com/your-team/math.git@v1.0.0 --name math
# Equivalent revision selection:
axiom get https://github.com/your-team/math.git --rev v1.0.0 --name math
```

HTTPS and SSH URLs, `github.com/owner/repository` and `gitlab.com/owner/repository` shorthand, and existing local Git repository directories are supported. Private repository authentication uses your Git configuration. Without a revision, `get` resolves the repository's default branch. The dependency name defaults to the repository basename without `.git`, with hyphens changed to underscores. Use `--name` for a different import name; it must be an Axiom identifier other than `std`.

A dependency contains either an `Axiom.toml` specifying its library entry or a `src/lib.ax` / `lib.ax` file. Library declarations intended for callers must be `pub`:

```text
pub def double (value : i32) := value * 2;
```

Then use it in the application:

```text
import math;
def main := println(math.double(21));
```

`import math.extra;` loads `extra.ax` relative to the dependency's library entry directory. Each dependency resolves its own dependencies in its own package namespace. Native sources and include directories from dependency projects participate in the build.

## Pinning and restoring

`get` records the resolved commit directly in `Axiom.toml`:

```toml
[dependencies]
math = "https://github.com/your-team/math.git#0123456789abcdef0123456789abcdef01234567"
```

The hash above is illustrative. Commit the actual manifest written by `axiom get`. Downloads go in `.axiom/deps/`; add `.axiom/` to your application's `.gitignore`.

After checking out a project, restore its exact dependencies with:

```sh
axiom get
axiom run
```

Restoration includes transitive dependencies. Builds use the local cache and do not fetch over the network. Missing dependencies produce a diagnostic asking you to run `axiom get`. Existing checkouts are reused at their pinned revision. Avoid editing cache contents; use your library's source repository for changes.

Running `get REPOSITORY --name NAME` again explicitly updates that dependency. Failed downloads or invalid library entries leave the manifest unchanged. Source revisions are pinned in the manifest itself; there is no separate lockfile. Local repository paths are stored as absolute paths and are intended for local development and tests.

This initial package manager uses Git rather than a package registry. It does not resolve semantic-version ranges, run installation scripts, initialize Git submodules, or accept symlinks in downloaded source trees. Both `mod init` and `get` can be used before the application's entry source exists; builds require that source.
