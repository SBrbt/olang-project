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
  - Global `let`: **`let`** repeated per binding (see `LetBindings` below), then a **global** placement — **`data[…]`**, **`rodata[…]`**, **`bss[…]`**, or **`section[…]`** (**not** `stack`, which is function-local)
  - `type` definitions (`struct` / `array`)
- At least one function with a body is required.
- **`extern` definitions** export symbols; functions without `extern` are TU-local. ELF entry is chosen by the **link script**, not the compiler.

### Lexical

- **Whitespace**, **`//` comments**, **identifiers** (max 63 chars)
- **Keywords**:
  - Control / decl: `extern`, `let`, `if`, `else`, `while`, `break`, `continue`, `return`, `type`, `struct`, `array`
  - Ops: `find`, `load`, `store`, `addr`, `sizeof`, `stack`, `data`, `bss`, `rodata`, `section`
  - Types / literals: `void`, `bool`, `ptr`, `true`, `false`, `i8` … `b64` (see [syntax.md](../../book/syntax.md))
- **Literals**: int / float / bool / char / string (same as user doc)
- **Comparisons / shifts**: `<`, `>`, `<=`, `>=`, `<<`, `>>` are tokenized as in C (no angle-bracket generics).

### Types

```
Type ::= void | bool | ptr | i8 | … | b64 | Ident
```

### Expressions (Expr)

In `parse_expr`: builtin **`T(expr)`** forms (`i32`, `f32`, …) for value casts; **`sizeof[Type]`**; **`load[expr]`**; **`addr [ RefExpr ]`** (expression type **`ptr`**); **`find[expr]`**; literals, calls, field, index, unary/binary ops (see `parser.c`).

### Reference expressions (RefExpr)

Used as the tail of `let LetBindings` (**`stack` / `data` / …** or a `RefExpr`), including **`<Type> RefExpr`** (new reference with bound element type).

Implementation note: `parser.c` composes **`parse_let_bindings`** with **`parse_alloc_expr`** or **`parse_ref_expr`**.

```
LetBindings ::= LetBinding ( let LetBinding )*
LetBinding ::= Ident                // element type from initializer, or untyped blob + `<T>` view

RefExpr ::= ( RefExpr )
          | find [ Expr ]
          | addr [ RefExpr ]
          | stack [ BitWidthOptInit ]
          | data [ BitWidthOptInit ]
          | rodata [ BitWidthOptInit ]
          | bss [ BitWidth ]
          | section [ StrLit , BitWidthOptInit ]
          | < Type > RefExpr
          | Ident

BitWidth ::= DecIntLit            // unsuffixed positive decimal integer; bit count
BitWidthOptInit ::= BitWidth [ , Expr ]
```

- **Placement expressions** (`stack` / `data` / `rodata` / `section`) have type **untyped reference** (`ref` with **void** element) after sema; the **`let`** name gets its element type from the **initializer**, or remains **untyped** until **`let v <T> raw`**, or (file-scope scalars) from an initializer on **`data`/`rodata`/… .
- **Stack / data / rodata (functions use `stack`; globals use `data` / `rodata`):** **`[`** **bit width** **`,`** optional **initializer** **`]`**. Bit width is an **unsuffixed** decimal integer literal (bits). Initializer omitted ⇒ **untyped** blob (locals: add **`<T>`**; globals: use **`bss[`** *width* **`]`** or **`data`** with an initializer if a typed scalar is needed at file scope).
- **`bss`:** **`bss[`** **bit width** **`]`** only — no second argument, no initializer.
- **`section`:** **`section[`** string **`,`** **bit width** **`,`** optional initializer **`]`** (same width/init rules as `data`).
- **No** comma-separated **view type list** inside allocators — use **`let a let b (x)`** (chain), **struct** fields, or **`let n <T>x`**.
- **`< Type > inner`**: introduces a **new** reference (or a **`ptr`** view when **`Type`** is **`void`**). Not `T(expr)` value cast. **Sema** requires **`inner`** to be a **reference** (typed or untyped), **never** **`ptr`** — so not **`addr[…]`**. **`addr [ … ]`** is a **`RefExpr`** form but types as **`ptr`**.
- Only **`stack`** inside functions for stack slots; at file scope use **`data` / `bss` / `rodata` / `section`** (not **`stack`**).
- **`Ident`**: must resolve to **ref**-typed storage in sema (enables `let a let b rhsName`).
- **`addr [ RefExpr ]`**: **`RefExpr`** in the grammar; expression type **`ptr`**. **Sema** currently requires the inner **`RefExpr`** to be a simple name (**`OL_EX_VAR`**); see **`OL_EX_ADDR`** in **`sema.c`**.

### Statements

```
Stmt ::= Block
       | let LetBindings RefExpr ;
       | store [ LValue , Expr ] ;
       | if ( Expr ) Stmt [ else Stmt ]
       | while ( Expr ) Stmt
       | break ;
       | continue ;
       | return Expr? ;
       | Expr ;

LValue ::= Ident | Expr . Ident | Expr [ Expr ]
```

- **No** assignment statement **`Expr = Expr`**; use **`store[lhs, rhs];`**.
- **`let`**: `LetBindings` then **`RefExpr`** or a **`stack` / `data` / …** placement form. No `from` keyword.

### Top-level (continued)

```
AllocatorG ::= data [ BitWidthOptInit ]
             | rodata [ BitWidthOptInit ]
             | bss [ BitWidth ]
             | section [ StrLit , BitWidthOptInit ]

TopLevel ::= … | let LetBindings AllocatorG ;
```

### Limitations

- Up to 8 register parameters; rest on stack.
- Chained `let` over a reference name, `stack[…]` placement (one name per placement), scalar vs aggregate, init rules: see [syntax.md](../../book/syntax.md).
- No compound assignment; no `++`/`--`.

### Values and storage

- `load`, `addr`, literals are **values**. Named storage comes from `let` + **`stack` / `data` / …** placement.
- Spilling subexpressions to slots is an implementation detail.

---

[Back](../README.md)
