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
- **Operations:** `cast`, `reinterpret`, `load`, `store`, `addr`, `deref`, `as`
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
type Ident = array<ElementType, Size>;
type Int5 = array<i32, 5>;
```

> **Aggregate types** are arrays and structs. They can be initialized after declaration, unlike scalars (primitive types like `i32`, `bool`, etc.) which must be initialized at declaration.

---

### Expressions

#### Precedence (high → low)

1. `addr`, `cast<T>`, `reinterpret<T>`, `load<T>`, `[]`, `.`, `()` — call
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

#### Type Operations

```olang
cast<T>(expr)           // explicit conversion (see [types](types.md))
reinterpret<T>(expr)    // same bit width, different type (no bool; no aggregates)
load<T>(ptr)            // read from pointer (rvalue; does not create a named object)
store<T>(ptr, val)      // write to pointer
addr Ident   // address: local name, then global `let`, then `extern` or function symbol (rvalue `ptr`)
```

Any local of type `ptr` may be used with `deref` (the slot continues to hold the pointer value at runtime).

Named storage is introduced only by `let` with an **allocator** (see below). Literals and the results of `load` / `addr` are **values** (typical rvalue uses); the current code generator may still spill expression results to stack slots as an implementation detail.

---

### Statements

#### Variable binding (named storage)

Inside a function, only **`@stack`** is allowed. Each binding is `Ident < Type >`. One or more bindings may share a single stack allocation; the integer in `@stack<bits>` is the **total size in bits**; the sum of all binding types’ sizes (in bits) must equal that value. Layout is a tight pack in declaration order.

```olang
let x<i32> @stack<32>(Expr);                    // scalar: initializer required; bit width matches type
let a<i32> b<i32> @stack<64>(Expr);            // two i32 views on one 64-bit object
let s<MyStruct> @stack<N>();                  // aggregate: optional init; N = 8 * sizeof(MyStruct)
```

At file scope, use a **global allocator** (not `@stack`). Like local `let`, you may list **one or more** `Ident < Type >` sharing one static blob; after `@data` / `@bss` / `@rodata` / `@section("name")` comes **`<bits>`** (total bit width), and the sum of binding sizes in bits must match. With multiple names, only **scalar** types are allowed; a single binding may be an aggregate (struct/array). The **first** binding name is the linker symbol for the object; other names refer to offsets into that symbol.

```olang
let x<i32> @data<32>(Expr);                    // .data
let y<i64> @bss<64>();                        // .bss (no initializer)
let c<i32> @rodata<32>(Expr);                  // .rodata (constant initializer)
let z<i32> @section("name")<32>(Expr);         // custom section
let a<i32> b<i32> @data<64>(Expr);             // two i32 views on one 64-bit blob (example)
```

#### Assignment
```olang
Expr = Expr;           // simple assignment
Expr.Ident = Expr;     // field
Expr[Expr] = Expr;     // array element
```

#### `deref` (ABI helper)
```olang
deref name as T;   // compile-time only: mark `name` as an indirect binding (stack slot still holds `ptr`;
                   // logical type for checking is `T`). `T` must be 1, 2, 4, or 8 bytes.
                   // Reading `name` emits a load; `name = rhs` emits a store through that pointer.
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

- Scalar `@stack` / global `let` must have an initializer (or use `@bss` without init where allowed)
- No compound assignment (`+=`, `-=`)
- No increment/decrement (`++`, `--`)
- No array bounds checking

---

[Return to Docs](../README.md)
