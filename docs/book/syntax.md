# Syntax Reference

**English** | **[中文](syntax_zh.md)**

---

### Lexical

#### Comments
```olang
// single-line comment
```

#### Identifiers
`[A-Za-z_][A-Za-z0-9_]*`

#### Keywords
`extern fn let if else while break continue return type struct array cast load store addr ptrbind deref bool u8 i32 u32 i64 u64 ptr void true false`

#### Literals

| Type | Format | Example |
|------|--------|---------|
| Integer | decimal | `42` |
| | hexadecimal | `0xFF` |
| | binary | `0b1010` |
| | octal | `0o77` |
| | suffix | `42i32`, `42u64` |
| Boolean | `true`, `false` |
| Character | `'a'`, `'\n'` (u8) |
| String | `"hello\n"` (ptr) |

---

### Types

```
Type ::= void | bool | u8 | i32 | u32 | i64 | u64 | ptr | Ident
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

1. `addr`, `cast<T>`, `load<T>`, `[]`, `.`, `()` — call
2. `!`, `-` — unary
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

#### Operators

| Operator | Description |
|----------|-------------|
| `+ - * / %` | Arithmetic |
| `== != < > <= >=` | Comparison |
| `&& || !` | Logical |
| `& | ^ << >>` | Bitwise |
| `-x` | Negation |

#### Type Operations

```olang
cast<T>(expr)        // type cast
load<T>(ptr)         // read from pointer
store<T>(ptr, val)   // write to pointer
addr expr            // address-of
```

---

### Statements

#### Variable Binding
```olang
let Ident: Type = Expr;  // scalars (primitives) must be initialized
let Ident: Type;       // aggregates (arrays/structs) can defer initialization
```

#### Assignment
```olang
Expr = Expr;           // simple assignment
Expr.Ident = Expr;     // field
Expr[Expr] = Expr;     // array element
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

**Limit**: maximum 6 parameters.

#### Global Variables
```olang
let Ident: Type = Expr;
```

---

### Limitations

- Scalar `let` must be initialized
- No compound assignment (`+=`, `-=`)
- No increment/decrement (`++`, `--`)
- No array bounds checking
- No floating point

---

[Return to Docs](../README.md)
