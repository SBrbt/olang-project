# Type System

**English** | **[中文](types_zh.md)**

---

### Built-in Types

| Type | Size | Description |
|------|------|-------------|
| `void` | 0 | No value / no return |
| `bool` | 1 byte | Boolean |
| `i8`, `i16`, `i32`, `i64` | 1 / 2 / 4 / 8 bytes | Signed integers |
| `u8`, `u16`, `u32`, `u64` | 1 / 2 / 4 / 8 bytes | Unsigned integers |
| `f16`, `f32`, `f64` | 2 / 4 / 8 bytes | Floating-point (IEEE binary16/32/64) |
| `b8`, `b16`, `b32`, `b64` | 1 / 2 / 4 / 8 bytes | Raw bit vectors (fixed width) |
| `ptr` | 8 bytes | Raw pointer |

### Literal Suffixes

**Integers:** optional suffix `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`, `b8`, `b16`, `b32`, `b64`. Unsuffixed decimal literals default to `i32` (range checked).

**Floats:** decimal with optional fraction and/or exponent; optional suffix `f16`, `f32`, `f64`. Without a suffix, the literal is `f64`.

```olang
42           // i32
42i64        // i64
42u64        // u64
255b8        // b8
3.14f64      // f64
18446744073709551615u64  // UINT64_MAX
```

### Type Definitions

#### Struct

```olang
type Point = struct { x: i32, y: i32 };
type Rect = struct { tl: Point, br: Point };
```

- Fields in declared order
- Supports nesting
- Assignment uses value copy

#### Array

```olang
type IntArray = array(i32, 10);
```

- Size fixed at compile time
- Variable is actually a pointer
- No bounds checking

### Type Casting

#### Builtin `DestType(expr)` (value position)

There is no `cast` keyword. **`T(expr)`** (value position) only converts between the builtin families **`iN`**, **`uN`**, **`bN`**, and **`fN`**: widening/narrowing within each family (`i*`↔`i*`, `u*`↔`u*`, `b*`↔`b*`, `f*`↔`f*`), and **`uN`/`iN` ↔ `fN`**. There is **no** value cast **to or from** **`ptr`** or **`bool`** (use comparisons, control flow, or ref views). **Same-width** reinterpret between different families (e.g. `i32` vs `u32`, or integer vs `bN` at the same size) is **not** a value cast—use **`let n <T>x`** / chained **`let`** on references. You **cannot** apply **`T(x)`** to a **bare variable** for same-width rebinding—use **`let n <T>x`** (type on the **RefExpr** tail), **`let a let b x`**, or **`struct`** fields (see [syntax](syntax.md)).

```olang
i32(i64_value)   // narrow
i32(i8_value)    // widen
u32(200u8)       // unsigned widen
f32(3i32)        // int to float (including same total width as i32)
```

#### Same-width **storage** views: `addr` + `<T>`

Use **`let n <T>x`** so **`n`** names a **new** reference whose element type is **`T`**, referring to the same bytes as **`addr[x]`** (`addr` yields `ptr`). For several names over one blob, use **`let a let b rhsRef`** or **`struct`** fields — not a type list inside **`stack[...]`**.

```olang
let x stack[32, -1];
let u <u32>x;   // same 4 bytes as `x`, name `u`
```

### Type Categories

| Category | Types | Initialization |
|----------|-------|----------------|
| Scalar | All built-ins except `void` | Must initialize at `let` |
| Aggregate | struct, array | Can defer initialization |

---

[Return to Docs](../README.md)
