# Language Support Matrix

Nanox now ships with **75 built-in syntax profiles**. Each profile defines comment delimiters, flow/type/keyword sets, and highlighting toggles for triple quotes, brackets, and numerics. All built-ins are loaded from `syntax.ini`, and you can add more via the per-user `langs/` directory.

Profiles marked **regex** use `file_matches` (filename regex) instead of extension matching — these cover files like `Makefile`, `CMakeLists.txt`, `Dockerfile`, and systemd units that have no conventional extension.

| # | Language | Support | Extensions / File Match | Notes |
|---|----------|---------|-------------------------|-------|
| 1 | C | FULL | .c, .h | Includes C11 preprocessor directives |
| 2 | C++ | FULL | .cpp, .hpp, .cxx, .hxx, .cc | Templates, constexpr, and noexcept keywords |
| 3 | C# | FULL | .cs | Async/await aware |
| 4 | Objective-C | FULL | .m, .mm | Highlights @interface blocks and literals |
| 5 | Java | FULL | .java | Records, modules, and modern control flow |
| 6 | Kotlin | FULL | .kt, .kts | Covers coroutines and sealed classes |
| 7 | Scala | FULL | .scala, .sc | Match/case plus implicit constructs |
| 8 | Swift | FULL | .swift | Triple-quoted strings and protocol keywords |
| 9 | Go | FULL | .go | Channels, goroutines, defer |
| 10 | Rust | FULL | .rs | Ownership keywords, async/await |
| 11 | Python | FULL | .py, .pyw, .pyx, .pyi | Complete flow keywords, match/case, triple quotes |
| 12 | Ruby | FULL | .rb, .erb, .rake, .gemspec | Blocks, modules, DSL helpers |
| 13 | JavaScript | FULL | .js, .mjs, .cjs | Modern ES modules and async constructs |
| 14 | TypeScript | FULL | .ts, .tsx | Types, namespaces, declaration syntax; tsx included |
| 15 | PHP | FULL | .php, .phtml | Traits, namespaces, generators |
| 16 | Perl | FULL | .pl, .pm, .t | Given/when and common pragmas |
| 17 | R | FULL | .r | Base functions and constants |
| 18 | Julia | FULL | .jl | Block comments and macros |
| 19 | Dart | FULL | .dart | async/await plus mixins |
| 20 | Lua | FULL | .lua | Long brackets, local/block flow |
| 21 | POSIX Shell | FULL | .sh, .bash, .zsh, .ksh | Case/select, function defs |
| 22 | PowerShell | FULL | .ps1, .psm1, .psd1 | Blocks, trap, workflow |
| 23 | SQL | FULL | .sql | ANSI joins and DDL/DML verbs |
| 24 | HTML | FULL | .html, .htm | Tag-aware brackets |
| 25 | CSS | FULL | .css, .scss, .less | Selector braces and numbers |
| 26 | JSON/JSON5 | FULL | .json, .json5 | Bracket + numeric highlight |
| 27 | YAML | FULL | .yaml, .yml | Inline scalars |
| 28 | TOML | FULL | .toml | Tables and numeric literals |
| 29 | Markdown | FULL | .md, .markdown | Triple quotes for code fences |
| 30 | Visual Basic | FULL | .vb, .vbs, .bas | REM and `'` comments |
| 31 | Groovy/Gradle | FULL | .groovy, .gradle | DSL keywords; covers Gradle build scripts |
| 32 | Haskell | FULL | .hs, .lhs | `{- -}` block comments |
| 33 | INI/Config | FULL | .ini, .cfg, .reg | Section/key-value files, `;` and `#` comments |
| 34 | Makefile | FULL | **regex** `[Mm]akefile`, `*.mk`, `*.make` | ifeq/ifdef directives, .PHONY |
| 35 | Kconfig | FULL | **regex** `Kconfig`, `Kbuild` | Linux kernel config language |
| 36 | Systemd Unit | FULL | **regex** `.service`, `.socket`, `.timer`, `.target`, `.mount`, `.network`, … | Full unit file keyword set |
| 37 | VimScript | FULL | .vim + **regex** `.vimrc`, `init.vim` | VimScript 9 keywords included |
| 38 | CMake | FULL | .cmake + **regex** `CMakeLists.txt` | Functions, macros, find_package |
| 39 | QMake | FULL | .pro, .pri, .prf | Qt project files |
| 40 | XML | FULL | .xml, .xsd, .xsl, .xslt, .svg, .wsdl, .rss, .atom, .xaml, .xhtml, .plist, .resx | Tag-aware brackets |
| 41 | Maven | FULL | **regex** `pom.xml` | Maven POM keyword set |
| 42 | Ant | FULL | **regex** `build.xml`, `ivy.xml` | Ant/Ivy build target keywords |
| 43 | Protobuf | FULL | .proto | Messages, services, RPC stream types |
| 44 | Delphi/Pascal | FULL | .pas, .dpr, .dfm, .pp, .lpr | `{ }` and `(* *)` block comments |
| 45 | Vue | FULL | .vue | SFC template/script/style sections |
| 46 | JSX | FULL | .jsx | React JSX component files |
| 47 | Svelte | FULL | .svelte | each/await/then flow, reactive keyword |
| 48 | Astro | FULL | .astro | Frontmatter + component keywords |
| 49 | GraphQL | FULL | .graphql, .gql | Types, interfaces, directives |
| 50 | Dockerfile | FULL | **regex** `Dockerfile`, `Dockerfile.*` | All Docker instructions |
| 51 | Dotenv | FULL | .env + **regex** `.env`, `.env.*` | Environment variable files |
| 52 | Nix | FULL | .nix | Derivations, builtins, lib helpers |
| 53 | Zig | FULL | .zig | comptime, errdefer, orelse |
| 54 | Terraform/HCL (tf) | FULL | .tf, .tfvars | Resources, providers, lifecycle |
| 55 | HCL | FULL | .hcl | HashiCorp config language |
| 56 | Bazel/Starlark | FULL | .bazel, .bzl + **regex** `BUILD`, `WORKSPACE` | Rules, providers, depsets |
| 57 | Crystal | FULL | .cr | Ruby-like syntax with static types |
| 58 | Nim | FULL | .nim, .nims | `#[ ]#` block comments, pragma style |
| 59 | OCaml | FULL | .ml, .mli, .mll, .mly | `(* *)` block comments, functors |
| 60 | F# | FULL | .fs, .fsi, .fsx, .fsproj | Computation expressions, async |
| 61 | D | FULL | .d, .di | `/* */` and `/+ +/` nesting comments |
| 62 | Clojure | FULL | .clj, .cljs, .cljc, .edn | Lisp macros, threading macros |
| 63 | Elm | FULL | .elm | `{- -}` block comments, type aliases |
| 64 | Solidity | FULL | .sol | Smart contracts, modifiers, events |
| 65 | Awk | FULL | .awk | BEGIN/END blocks, built-in variables |
| 66 | MDX | FULL | .mdx | Markdown + JSX, triple-quote fences |
| 67 | V | FULL | .v, .vv | V language structs, interfaces |
| 68 | Meson | FULL | .meson + **regex** `meson.build` | Build targets, subprojects |
| 69 | Thrift | FULL | .thrift | Services, structs, type definitions |
| 70 | Bicep | FULL | .bicep | Azure resource definitions |
| 71 | Gleam | FULL | .gleam | Functional, type-safe Erlang-VM language |
| 72 | Odin | FULL | .odin | Data-oriented C alternative |
| 73 | ReScript | FULL | .res, .resi | OCaml-based JS compiler |
| 74 | Fish Shell | FULL | .fish | Fish-specific control flow and builtins |
| 75 | Nginx Config | FULL | **regex** `nginx.conf`, `*.nginx` | Server/location blocks, proxy directives |

## Extending Beyond Built-in Languages

Place additional `.ini` files under `~/.config/nanox/langs` (or `~/.local/share/nanox/langs`). Every file can define one or more `[language]` sections using the same keys as the core profiles. Nanox merges these after loading the defaults, so you can override a bundled language or add niche languages such as Ada, COBOL, Erlang, Elixir, or Fortran.

Example (`~/.config/nanox/langs/erlang.ini`):

```ini
[erlang]
extensions = erl,hrl
line_comment_tokens = %
string_delims = ",'
flow = if,of,case,receive,after,fun,try,catch,throw,begin,end
keywords = module,export,import,record,when,spawn,let,apply
return_keywords = return
```

Reference profiles for Ada, COBOL, Elixir, Erlang, and Fortran ship under `configs/nanox/langs` to serve as templates.
