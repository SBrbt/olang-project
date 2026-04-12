# OLang Syntax Spec

**English** | **[中文](olang-syntax_zh.md)**

---

> User guide: [docs/book/syntax.md](../../book/syntax.md) (maintained alongside the implementation)  
> This file tracks what `lexer.c` / `parser.c` **currently** accept.

### Source file and top-level

- One `.ol` file is one compilation unit; **no** import / module (`#include` is handled by `olprep`; see [preproc.md](../preproc.md)).
- Top-level items:
  - `extern Type Ident ( ParamList ) ;` or `extern Type Ident ( ParamList ) Block`
  - `Type Ident ( ParamList ) Block` (internal function)
  - Global `let`: **`let`** repeated per binding (see `LetBindings` below), then a **global** allocator `@data<…>`, `@bss<…>`, `@rodata<…>`, `@section("name")<…>` (**not** `@stack`)
  - `type` definitions (`struct` / `array`)
- At least one function with a body is required.
- **`extern` definitions** export symbols; functions without `extern` are TU-local. ELF entry is chosen by the **link script**, not the compiler.

### Lexical

- **Whitespace**, **`//` comments**, **identifiers** (max 63 chars)
- **Keywords**:
  - Control / decl: `extern`, `let`, `if`, `else`, `while`, `break`, `continue`, `return`, `type`, `struct`, `array`
  - Ops: `cast`, `find`, `load`, `store`, `addr`, `sizeof`
  - Types / literals: `void`, `bool`, `ptr`, `true`, `false`, `i8` … `b64` (see [syntax.md](../../book/syntax.md))
  - **Reserved:** `as` (keyword in the lexer; no `as` syntax in the parser yet)
- **Literals**: int / float / bool / char / string (same as user doc)

### Types

```
Type ::= void | bool | ptr | i8 | … | b64 | Ident
```

### Expressions (Expr)

In `parse_expr`: `cast<T>(expr)` uses a full `expr`; also `sizeof<T>`, `load<Ident>`, `addr Ident`, **`find < Expr >`** (same form as in `RefExpr`; operand must be a `ptr` rvalue), literals, calls, field, index, unary/binary ops (see `parser.c`).

### Reference expressions (RefExpr)

Used as the tail of `let LetBindings` (`@…` or a `RefExpr`), and as the inner operand of `cast<T>(…)` **in ref position** (inner must be `RefExpr`, not any `expr`):

```
LetNameTy ::= Ident < Type >
            | Ident < >

LetBindings ::= LetNameTy ( let LetNameTy )*

RefExpr ::= ( RefExpr )
          | find < Expr >
          | addr Ident
          | @ stack | data | bss | rodata | section ( StrLit ) < BitWidth > ( Expr? )
          | < [Type] > RefExpr
          | cast < Type > ( RefExpr )
          | Ident

BitWidth ::= IntLit | sizeof < Type >
```

Plus the **`T<RefExpr>`** form where **`T`** is a builtin type keyword other than `void` (see `is_T_angle_conv_starter` in `parser.c`); it is parsed as part of `RefExpr`, not shown as a separate line above to avoid duplicating the keyword list.

- Only `@stack` inside functions; no `@stack` at file scope.
- `<void>` means no static type attached (inner must be a `ptr` view).
- **`Ident`**: must resolve to **ref**-typed storage in sema (enables `let a<T> let b<U> rhsName` where `rhsName` is an existing reference).
- **Builtin `Type` `<` `RefExpr` `>`**: same-angle closing rule as in `parse_ref_expr` (`is_T_angle_conv_starter`; `void` is excluded).
- **`find < Expr >`**: `Expr` must have type **`ptr`** (a ptr rvalue); the result is a **compile-time reference to a memory object** (the usual lowering evaluates `Expr` in place). **`find<>`** is invalid. **Nesting `find<find<…>>`** is invalid. Inside `find<…>`, a comparison that uses **`>`** at the top level must parenthesize the comparison (e.g. `find<(a > b)>`) so the closing `>` of `find<…>` is not ambiguous.

### Statements

```
Stmt ::= Block
       | let LetBindings ( @stack < BitWidth > ( Expr? ) | RefExpr ) ;
       | store < LValue , Expr > ;
       | if ( Expr ) Stmt [ else Stmt ]
       | while ( Expr ) Stmt
       | break ;
       | continue ;
       | return Expr? ;
       | Expr ;

LValue ::= Ident | Expr . Ident | Expr [ Expr ]
```

- **No** assignment statement **`Expr = Expr`**; use **`store<…>`** (see ref model).
- **`let`**: `LetBindings` as above — **each** additional `Ident<Type>` is prefixed by **`let`** (legacy `let x<T> y<U>` without `let` between names is rejected). Then either `@stack<…>(…)` or a single `RefExpr`. No `from` keyword. Chaining order vs. nested ref-views: see [olang-refmodel.md](olang-refmodel.md) § “Chained `let`”.
- **`store<…>`**: inside angle brackets: lvalue, comma, value expression, then `>`; value parsing uses a shallow expression layer (see `parse_shift`).

### Top-level (continued)

```
AllocatorG ::= @data < BitWidth > ( Expr? )
             | @bss < BitWidth > ( )
             | @rodata < BitWidth > ( Expr )
             | @section ( StrLit ) < BitWidth > ( Expr? )

TopLevel ::= … | let LetBindings AllocatorG ;
```

### Limitations

- Up to 8 register parameters; rest on stack.
- Multi-binding `let`, scalar vs aggregate, init rules: see [syntax.md](../../book/syntax.md).
- No compound assignment; no `++`/`--`.

### Values and storage

- `load`, `addr`, literals are **values**. Named storage comes from `let` + allocators.
- Spilling subexpressions to slots is an implementation detail.

---

[Back](../README.md)
