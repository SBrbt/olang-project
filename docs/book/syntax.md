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
- **Operations:** `cast`, `find`, `load`, `store`, `addr`, `sizeof`
- **Types / literals:** `void`, `bool`, `ptr`, `true`, `false`, `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`, `f16`, `f32`, `f64`, `b8`, `b16`, `b32`, `b64`
- **Reserved, not a statement:** `as` (lexed as a keyword; no `as` syntax in the parser yet)

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

1. `addr`, `cast<T>`, `find<…>`, `sizeof<…>`, `<[T]>`, `load<…>`, `[]`, `.`, `()` — call
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
cast<T>(expr)           // explicit conversion; parentheses contain a full expression (see [types](types.md))
sizeof<T>               // compile-time constant: bit width of `T`; expression type is `u64`
load<name>              // read binding `name` (`name` is an identifier; see semantic rules)
addr Ident              // address: local, then global `let`, then `extern` or function symbol (rvalue `ptr`)
find<Expr>              // `Expr` must yield `ptr`; same form as in RefExpr (see ref model)
```

**Reference expression `RefExpr`** (used after `let name<T>`, and as the inner operand of `cast<T>(…)` in ref position—the inner `cast` is **not** a full `expr`):

```olang
( RefExpr )
find<Expr>              // `Expr` must be `ptr` rvalue; compile-time ref to a memory object (see [ref model](../internals/specs/olang-refmodel.md))
addr Ident
@stack|@data|@bss|@rodata|@section("…") <Int|sizeof<T>> ( [Expr] )   // `@stack` only inside functions
<[T]> RefExpr           // attach static type to inner ref; use `<void>` for untyped `ptr` view
cast<T>(RefExpr)        // differs from general `cast`: inner must be `RefExpr`
```

**`store` is a statement**, not `store<…>(…)` inside a general expression:

```olang
store<lvalue, expr>;    // lvalue: name, `a.b`, or `a[i]`; comma, then value, then `>`
```

Named storage is introduced only by `let` with an **allocator** (see below). Literals and `load` / `addr` are **values**. Indirect bindings (`let …` with `find` / `<T>addr`) are checked as pointer slots with an element type; there is **no** `deref` keyword statement. The code generator may spill subexpressions to stack slots as an implementation detail.

---

### Statements

#### Variable binding (named storage)

Inside a function, only **`@stack`** is allowed. Each binding is `Ident<Type>`. One or more bindings may share a single stack allocation; **each additional binding is introduced with the keyword `let`** (e.g. `let x<f32> let n<i32> @stack<64>(...)`). `@stack<bits>` is the **total size in bits** (literal or `sizeof<Type>`); the sum of all binding types’ sizes (in bits) must equal that value. There is **no** `from` keyword between bindings and `@stack`. Layout is a tight pack in declaration order.

```olang
let x<i32> @stack<32>(Expr);                    // scalar: initializer required; bit width matches type
let x<f32> let n<i32> @stack<64>(Expr);             // e.g. f32 + i32 views on one 64-bit object
let s<MyStruct> @stack<N>();                  // aggregate: optional init; N = 8 * sizeof(MyStruct)
```

At file scope, use a **global allocator** (not `@stack`). Like local `let`, you may list **one or more** `Ident<Type>` sharing one static blob; after `@data` / `@bss` / `@rodata` / `@section("name")` comes **`<bits>`** (total bit width or `sizeof<...>`), and the sum of binding sizes in bits must match. With multiple names, only **scalar** types are allowed; a single binding may be an aggregate (struct/array). The **first** binding name is the linker symbol for the object; other names refer to offsets into that symbol.

```olang
let x<i32> @data<32>(Expr);                    // .data
let y<i64> @bss<64>();                        // .bss (no initializer)
let c<i32> @rodata<32>(Expr);                  // .rodata (constant initializer)
let z<i32> @section("name")<32>(Expr);         // custom section
let gx<f32> let gn<i32> @data<64>(Expr);            // heterogeneous scalars, one 64-bit blob (example)
```

#### Writes (no `=` assignment)

There is **no** `Expr = Expr` statement. Writes use **`store`**:

```olang
store<lvalue, expr>;   // lvalue: name, field, or subscript (see above)
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
