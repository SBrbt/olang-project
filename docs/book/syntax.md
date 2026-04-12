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

- **Syntax / control:** `extern`, `fn`, `let`, `if`, `else`, `while`, `break`, `continue`, `return`, `type`, `struct`, `array`
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
load<T>(ptr)            // read from pointer
store<T>(ptr, val)      // write to pointer
addr Ident   // address: resolves local name, then global `let`, then extern / `fn` symbol
```

Any local of type `ptr` may be used with `deref` (the slot continues to hold the pointer value at runtime).

---

### Statements

#### Variable Binding
```olang
let Ident: Type = Expr;  // scalars (primitives) must be initialized
let Ident: Type;         // aggregates (arrays/structs) can defer initialization
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
```olang
// Internal function
fn name(params) -> Type { body }

// Exported function
extern fn name(params) -> Type { body }

// External declaration
extern fn name(params) -> Type;
```

#### Parameters
```
Params ::= Param (, Param)*
Param  ::= Ident: Type
```

**Limit**: up to 8 register parameters; additional parameters are passed on the stack.

#### Global Variables
```olang
[@data | @bss | @rodata | @section("name")]  // optional section attributes
let Ident: Type = Expr;
```

---

### Limitations

- Scalar `let` must be initialized
- No compound assignment (`+=`, `-=`)
- No increment/decrement (`++`, `--`)
- No array bounds checking

---

[Return to Docs](../README.md)
