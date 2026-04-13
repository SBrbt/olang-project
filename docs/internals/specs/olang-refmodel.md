# Olang reference model (ref-centric surface)

**English** | **[中文](olang-refmodel_zh.md)**

This document specifies the **reference-centered** surface language targeted by the `olang` compiler. For full grammar, see [olang-syntax.md](olang-syntax.md).

## Core ideas

### `let`, right-hand side, and `<Type>`

- A **reference** may have an **element type** or be **untyped** (**`void`** element: storage/view without a static element type yet—common for raw allocators and **`find[…]`**, then **`<T>…`**). **`let` *name*** binds the **symbol** to the **reference** on the right; the **`let`** keyword **does not carry a type**. After **`let`** come **names** (**`LetBindings`**); **what follows must be the reference side** (grammar **`RefExpr`**; see [olang-syntax.md](olang-syntax.md)). **`RefExpr`** is only a grammar label — it is **not** the same as “the expression’s type is a reference” (e.g. **`addr[…]`** is **`ptr`**).
- **Chained names** (**`let n1 let n2 … let nK` `RefExpr`**): each **`let nk`** introduces a **name** that **denotes a reference**. In elaboration, names to the **left** are bound to **views** built from the reference associated with the **right** (typically the rightmost **`nk`** sits next to **`RefExpr`** when **`RefExpr`** is an **`Ident`**); see *Chained `let`* below. So the reference produced “on the right” is what further **`let`** names to the left attach to.
- **`<Type> inner`** (any **`Type`**, including **`void`**): **`inner`** must be a **reference** (typed or untyped **`ref`**), **not** **`ptr`**. Introduces a **new** reference (or **`ptr`**-compatible view for **`<void>`**) over the **same** storage; see **`OL_EX_REF_BIND`** in **`sema.c`**. Write **`let n <T> x`**, not **`let n <T> addr[x]`** (**`addr[…]`** is **`ptr`**).

- A **symbol** (after the whole **`let`** is processed) binds each introduced **name** to a **reference** as above.
- **`ptr`** is an address type only; it does not carry an element type.
- **`find[Expr]`** requires **`Expr`** to evaluate to a **`ptr` rvalue**; the result is a **compile-time reference to a memory object** (usual lowering evaluates `Expr` like any subexpression). **Nested `find`** is invalid. Misuse of the address / object is **UB** (same as other views).
- **Reads/writes** use **`load[expr]`** / **`store[lvalue, Expr];`**. Assignment **`Expr = Expr`** is not supported.
- **Value** conversions use **`T(expr)`** with a full expression inside **`()`** (no `cast` keyword) — only between **`iN`/`uN`/`bN`/`fN`** (family widen/narrow and int↔float); **not** to/from **`ptr`** or **`bool`**. Do not confuse with **`<Type> RefExpr`**, which does **not** convert a loaded value.
- **`addr [ RefExpr ]`** is a **`ptr` rvalue** (a **`RefExpr`** form in the grammar); it does not allocate storage. Use it where a **`ptr`** **value** is needed (e.g. **`stack[64, addr[x]]`**); **typed** **`<T> …`** does **not** wrap **`addr[…]`**.
- **Placement:** **`stack` / `data` / `rodata` / `section`** — **`[`** **bit width** **`,`** optional initializer **`]`**; **`bss`** — **`[`** bit width **`]`** only. The allocator expression is an **untyped** reference; the **`let`** binding’s element type comes from the **initializer**, or an **untyped** blob plus **`<T>`** view, or (file-scope scalars) an initializer on **`data`/`rodata`/…**. See [olang-syntax.md](olang-syntax.md).
- **`sizeof[Type]`** is a **compile-time** integer (bit width of `Type`).
- **`addr [ … ]`** yields the address of the operand storage (**`ptr`**). Same-width **view** of existing storage: **`let n <T> x`** (**`x`** a **reference**), not **`addr[x]`** under **typed** **`<T>`**.
- **Safety**: the language provides operations only; **no** alias or layout guarantees. **UB** is possible if the user misuses views.

## EBNF sketch (aligned with current `parser.c`)

Authoritative productions and commentary: [olang-syntax.md](olang-syntax.md). Summary:

- **`LetBindings`** → `Ident` (`let` `Ident`)* — binding sites carry **no** type; repeat **`let`** for each name.

- **Local `let`** → `let` `LetBindings` `RefExpr` `;` — the right-hand side is **`RefExpr`** (see the grammar); at file scope use **`AllocatorG`** instead of this **`RefExpr`** form.

- **Global `let`** → `let` `LetBindings` `AllocatorG` `;` — **`AllocatorG`** is **`data` / `rodata` / `bss` / `section`** only (see [olang-syntax.md](olang-syntax.md)).

```
LetBindings ::= Ident ( let Ident )*

RefExpr ::= ( RefExpr )
          | find [ Expr ]
          | addr [ RefExpr ]
          | stack [ … ]
          | data [ Expr ]
          | rodata [ Expr ]
          | bss [ BitWidth ]
          | section [ StrLit , Expr ]
          | < Type > RefExpr
          | Ident

BitWidth ::= IntLit | sizeof [ Type ]
```

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

- **`Expr`** (fragment): **`sizeof[Type]`**, **`load[expr]`**, **`find[expr]`**, **`addr [ RefExpr ]`**, builtin **`T(expr)`** value casts, literals, calls, field, index, unary/binary — see `parse_expr` in `parser.c` and [olang-syntax.md](olang-syntax.md).

### Chained `let` (binding order)

For `let b1 let b2 … let bN` `RefExpr`, the parser records bindings **left to right** (`b1` … `bN`). When `RefExpr` is a reference **name** (`Ident`), the semantic layer treats the **rightmost** binding (`bN`) as closest to that **`Ident`** and builds nested ref-views **outward** toward `b1` (see `check_let_ref_chain_views` in `sema.c`). When the tail is **`stack[…]`** or a global allocator, **one** name is bound per placement (no type list inside **`stack`**); use **`struct`** fields or **`<T>` / chain** for multiple views.

## Notes

- **`T(expr)`** — builtin type keyword + **`(` `expr` `)`**: **value** conversion only; the inner expression is parsed with **`parse_expr`** (see `parse_primary` in `parser.c`). There is **no** `T(RefExpr)` form.
- **`<Type> inner`** — **new** reference (or **`ptr`** view if **`Type`** is **`void`**), same storage as **`inner`**. **`inner`** must always be a **reference**, not **`ptr`**. This is what **let** / chaining consumes on the left. Not the same as **`T(expr)`**.
- Grouping **`(` `expr` `)`** is ordinary parentheses, not a cast.

---

[Back](../README.md)
