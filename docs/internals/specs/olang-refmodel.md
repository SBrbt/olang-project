# Olang reference model (ref-centric surface)

**English** | **[中文](olang-refmodel_zh.md)**

This document specifies the **reference-centered** surface language targeted by the `olang` compiler rewrite. It is **normative** for new syntax; legacy forms listed under “Removed” must not be accepted by the parser.

## Core ideas

- A **symbol** binds a **name** to a **reference** (compile-time place + optional static type binding).
- **`ptr`** is an address type only; it does not carry an element type.
- **Type binding** `<[Type]> RefExpr` yields a **new** reference, independent of the operand; use `<void>` for an untyped ptr view.
- **`find<Expr>`** requires **`Expr`** to evaluate to a **`ptr` rvalue**; the result is a **compile-time reference to a memory object** (usual lowering evaluates `Expr` like any subexpression). **Nested `find`** is invalid. Misuse of the address / object is **UB** (same as other views).
- **Reads/writes** use **`load<name>`** / **`store<name, Expr>;`**. Assignment **`Expr = Expr`** is removed.
- **`cast<Type>`** (only form: `cast<Type>(RefExpr)` or EBNF-equivalent) **allocates/materializes a new object** and yields a **reference** carrying `Type`. Legacy **`cast<T>(expr)`** as a plain rvalue conversion is **removed**.
- **`addr Ident`** is a **`ptr` rvalue**; it does not allocate storage.
- **`@data|@bss|@rodata|@section("…")<bits>(Expr)`** / **`@stack<bits>(Expr)`** allocate storage; the result is a reference whose binding is **`ptr`** to the allocated block.
- **`sizeof<Type>`** is a **compile-time** integer (bit width of `Type`).
- **`addr Ident`** yields the address of named storage (`ptr`); combine with **`let n<T> <T>addr x`** or multi-binding **`let`** for another typed view of the same bytes.
- **Safety**: the language provides operations only; **no** alias or layout guarantees. **UB** is possible if the user misuses views.

## Removed (must be rejected)

- `load<T>(ptr)`, `store<T>(ptr, val)` (old keyword forms)
- `deref name as T;`
- legacy `reinterpret<T>(expr)` (removed from docs; use `addr` + `<T>` / multi-binding `let` for storage views)
- Legacy `cast<T>(expr)` rvalue model (parenthesized form after `cast` without the `cast<Type>` bracket syntax as defined in EBNF)
- `Expr = Expr` assignment
- **Legacy multi-bind without `let` between names**, e.g. `let x<T> y<U> @stack<…>(…)` — each additional name **must** be introduced with the keyword `let` (see `LetBindings` below).

## EBNF sketch (aligned with current `parser.c`)

- **`LetNameTy`** → `Ident` `<` `Type` `>` | `Ident` `<` `>` — second alternative is an empty type parameter (`<>`).
- **`LetBindings`** → `LetNameTy` (`let` `LetNameTy`)* — **every** extra binding repeats the keyword `let` (no space-separated `Ident<Type>` chains).

- **Local `let`** → `let` `LetBindings` (`@stack` `<` `BitWidth` `>` `(` `Expr`? `)` | `RefExpr`) `;`
- **Global `let`** → `let` `LetBindings` `AllocatorG` `;` *(see [olang-syntax.md](olang-syntax.md) for `AllocatorG`; tail is an allocator only, not a general `RefExpr`)*

- **`RefExpr`** → `(` `RefExpr` `)`
          | `find` `<` `Expr` `>`
          | `addr` `Ident`
          | `@` allocator … *(e.g. `@stack`, `@data`, …)*
          | `<` `Type` `>` `RefExpr`   *(incl. `<void>`; disambiguate vs comparison via tokens / parentheses)*
          | `cast` `<` `Type` `>` `(` `RefExpr` `)`
          | `Ident` *(must denote ref-typed storage; used for chained `let … refname` tails)*
          | *builtin type keyword* `<` `RefExpr` `>` *(same-angle closing as `parse_ref_expr`; `void` is not a starter — see `is_T_angle_conv_starter` in `parser.c`)*

- **`Expr`** → … | `find` `<` `Expr` `>` | `load` `<` `Ident` `>` | `sizeof` `<` `Type` `>` | `cast` `<` `Type` `>` `(` `Expr` `)` | literals | calls | field | index | unary | binary …

- **`Stmt`** → … | `store` `<` `LValue` `,` `Expr` `>` `;` | local/global `let` | control flow | `Expr` `;` — **no** `Expr = Expr`

- Keywords introducing `<` in dedicated sub-syntax: `find`, `cast`, `load`, `store`, `sizeof` — lexer/parser use dedicated paths so they do not clash with `<` comparison when unambiguous rules apply.

### Chained `let` (binding order)

For `let b1 let b2 … let bN` `RefExpr`, the parser records bindings **left to right** (`b1` … `bN`). When `RefExpr` is a reference **name** (`Ident`), the semantic layer treats the **rightmost** binding (`bN`) as the alias closest to that name and builds nested ref-views **outward** toward `b1` (see `check_let_ref_chain_views` in `sema.c`). When the tail is `@stack` / a global allocator, all bindings share one allocation blob (multi-view layout).

## Notes

- **`()`** after `<Type>` in expressions: **grouping/precedence only**, not a “call” of type.