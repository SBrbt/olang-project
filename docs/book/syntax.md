# Syntax Reference

**English** | **[中文](syntax_zh.md)**

---

### Lexical

#### Comments
```olang
// single-line comment
```

#### Identifiers
`[A-Za-z_][A-Za-z0-9_]*` (implementation limit: 63 characters)

#### Keywords

- **Syntax / control:** `extern`, `let`, `if`, `else`, `while`, `break`, `continue`, `return`, `type`, `struct`, `array`
- **Operations:** `find`, `load`, `store`, `addr`, `sizeof`
- **Types / literals:** `void`, `bool`, `ptr`, `true`, `false`, `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`, `f16`, `f32`, `f64`, `b8`, `b16`, `b32`, `b64`

#### Literals

| Type | Format | Example |
|------|--------|---------|
| Integer | decimal | `42` |
| | hexadecimal | `0xFF` |
| | binary | `0b1010` |
| | octal | `0o77` |
| | suffix | `42i32`, `42u64`, `255u8`, `1b64` |
| Floating-point | decimal fraction or exponent | `3.14`, `1e-3`, optional `f32` / `f64` / `f16` suffix |
| Boolean | `true`, `false` |
| Character | `'a'`, `'\n'` (u8) |
| String | `"hello\n"` (ptr) |

---

### Types

```
Type ::= void | bool | ptr | i8 | i16 | i32 | i64 | u8 | u16 | u32 | u64
       | f16 | f32 | f64 | b8 | b16 | b32 | b64 | Ident
```

#### Struct
```olang
type Ident = struct { FieldList };
type Point = struct { x: i32, y: i32 };
```

#### Array
```olang
type Ident = array(ElementType, Size);
type Int5 = array(i32, 5);
```

> **Aggregate types** are arrays and structs. They can be initialized after declaration, unlike scalars (primitive types like `i32`, `bool`, etc.) which must be initialized at declaration.

---

### Expressions

#### Precedence (high → low)

1. `addr`, builtin `T(…)` (**value** cast), `find[…]`, `sizeof[Type]`, `load[…]`, `[]`, `.`, `()` — call (ref position also has **`<Type>…`**; see **Reference expressions** below — do not confuse with `T(…)` value cast)
2. `!`, `~`, `-` — unary (`~` only for `b*`; `!` only for `bool`)
3. `*`, `/`, `%`
4. `+`, `-`
5. `<<`, `>>`
6. `<`, `>`, `<=`, `>=`
7. `==`, `!=`
8. `&`
9. `^`
10. `|`
11. `&&`
12. `||`

#### Expressions vs reference expressions

**General expression** atoms / prefixes:

```olang
i32(expr)               // explicit value conversion; inner is a full expression (see [types](types.md))
sizeof[Type]            // compile-time constant: bit width of `Type`; expression type is `u64`
load[expr]              // read through a reference expression (see semantic rules)
addr [ RefExpr ]         // address of operand storage (rvalue `ptr`; sema: simple name for now)
find[Expr]              // `Expr` must yield `ptr`; same form as in RefExpr (see ref model)
```

**Right-hand side of `let` (grammar: `RefExpr`)**

**`let` does not carry a type.** After **`let`** comes the **name**; **what follows must be a reference**, not an arbitrary expression. The forms below are for lookup:

```olang
( RefExpr )
find[Expr]              // `Expr` must be `ptr` rvalue; compile-time ref to a memory object (see [ref model](../internals/specs/olang-refmodel.md))
addr [ RefExpr ]
stack[ … ]              // stack allocation; functions only: [bit width, optional initializer]
data / bss / rodata / section[ … ]   // globals; see **Variable binding**
< Type > RefExpr        // **new** reference to the same storage as inner; element type `Type` is bound on **that new** reference — not `T(expr)` value cast
```

**`T(expr)` vs `<Type> …`:** the former converts a **value**; the latter wraps a **reference** and yields a **new** reference named by **`let`**. The **`inner`** of **`<Type>`** must be a **reference**, not **`addr[…]`** — see [ref model](../internals/specs/olang-refmodel.md).

**`stack[…]`** (functions only) takes **one** `let` name per placement. Brackets are **`[`** an **unsuffixed decimal bit width** **`,`** optional **initializer expression** **`]`**. The placement expression is an **untyped** reference; the binding’s element type comes from the **initializer**, or from **`let v <T> raw`** after a placement **without** initializer. **`bss`** is **`[`** bit width **`]`** only (no initializer). **`section`** is **`[`** string **`,`** bit width **`,`** optional init **`]`**. **Do not** list multiple view types inside the brackets; use **`let a let b (rhs)`**, **`struct`** fields, or **`let n <T>x`**.

**`store` is a statement**, not nested inside a general expression:

```olang
store[lvalue, expr];    // lvalue: name, `a.b`, or `a[i]`; comma, then value, then `];`
```

Named storage is introduced only by `let` with a **storage placement** form (see below). Literals and `load` / `addr` are **values**. Indirect bindings (`let …` with `find` / `<T>…`) use pointer slots with a logical element type; reads and writes use `load` / `store` on those names. The code generator may spill subexpressions to stack slots as an implementation detail.

---

### Statements

#### Variable binding (named storage)

Inside a function, only **`stack[...]`** is allowed for stack-backed storage. Each **`stack[…]`** introduces **one** name. The binding segment lists **identifiers only**: with an initializer, the element type is inferred; without, the slot is **untyped** until **`let v <T> raw`**. **Several names** on the same logical storage use **`let a let b (rhs)`**, **`struct`**, or **`let n <T>x`**.

```olang
let x stack[32, 42i32];                 // 32 bits, initializer fixes type
let raw stack[32];
let y <i32> raw;                        // no init: raw blob + view
let p_raw stack[128];
let p <Pair> p_raw;                     // aggregate: blob + view
let t stack[64, expr];                  // explicit width + initializer
```

At file scope (not **`stack`**): **`data`** / **`rodata`** use the same **`[`** bit width **`,`** optional init **`]`** form; **`bss`** is **`[`** bit width **`]`** only (untyped blob); **`section`** is **`["name",`** bit width **`,`** optional init **`]`**. Use an initializer at file scope when you need a typed scalar without a separate **`let … <T> …`** (not available at top level). Layouts with multiple fields use **`struct`** types or separate globals.

```olang
let gcount data[32, 10];               // .data
let c rodata[32, 7u32];                // .rodata
let y bss[64];                         // .bss, untyped blob
let z section[".mysec", 32, 0i32];     // custom section
```

File-scope constraints (enforced by sema):

- Global `let` right side must be `data` / `rodata` / `bss` / `section` (not `stack`, not general `RefExpr`).
- In one global `let`, names must be unique and binding count must be valid.
- If you split one global into multiple names, each view must be scalar and total view bit widths must sum to the declared bit width.
- Untyped global placement (void element) allows only one name.
- `bss[...]` cannot have an initializer.
- `rodata[...]` initializers must be compile-time constants.

#### Writes (no `=` assignment)

There is **no** `Expr = Expr` statement. Writes use **`store`**:

```olang
store[lvalue, expr];   // lvalue: name, field, or subscript (see above)
```

#### Control Flow
```olang
if (Expr) Block [else Block]
while (Expr) Block
break;
continue;
return Expr;
```

---

### Top-level Definitions

#### Functions

Return type comes before the name (C-style). Parameters use `name: Type`. Use `void` for no return value.

```olang
// Internal (single translation unit)
Type name(params) { body }

// Exported definition
extern Type name(params) { body }

// Forward declaration only
extern Type name(params);
```

#### Parameters
```
Params ::= Param (, Param)*
Param  ::= Ident: Type
```

**Limit**: up to 8 register parameters; additional parameters are passed on the stack.

---

### Limitations

- Scalar **`stack`** / writable global `let` must follow initializer rules (or use **`bss`** without init where allowed)
- No compound assignment (`+=`, `-=`)
- No increment/decrement (`++`, `--`)
- No array bounds checking

---

[Return to Docs](../README.md)
