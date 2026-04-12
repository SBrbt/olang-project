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
type IntArray = array<i32, 10>;
```

- Size fixed at compile time
- Variable is actually a pointer
- No bounds checking

### Type Casting

#### `cast<DestType>(expr)`

Explicit conversions the compiler allows between **different** types: widening/narrowing within the same integer family (`i*`↔`i*`, `u*`↔`u*`), `b*`↔`b*`, `f*`↔`f*`, `bool`↔integers, and float↔integer conversions. For **same-width** scalar rebinding of a **computed** value, `cast` is allowed (e.g. `cast<u32>(x + 1i32)`). You **cannot** apply `cast` to a **bare variable** for same-width rebinding—use **`let n<T> <T>addr x`** or multi-binding `let` (see below).

```olang
cast<i32>(i64_value)   // narrow
cast<i32>(i8_value)    // widen
cast<u32>(200u8)       // unsigned widen
cast<u32>(x + 0i32)    // same-width value cast (expression, not a plain variable)
```

#### Same-width **storage** views: `addr` + `<T>`

Use **`let n<T> <T>addr x`** to introduce another name as an indirect view of the same bytes (`addr` yields `ptr`, the outer `<T>` sets the element type). Alternatively, chain **`let`** before each `Ident<Type>` to share one allocation (e.g. `let x<f32> let n<i32> @stack<64>(...)`).

```olang
let x<i32> @stack<32>(-1);
let u<u32> <u32>addr x;   // same 4 bytes as `x`, name `u`
// cast<ptr>(0x1000u64)           // same-width value: literal / non-variable expr
```

### Type Categories

| Category | Types | Initialization |
|----------|-------|----------------|
| Scalar | All built-ins except `void` | Must initialize at `let` |
| Aggregate | struct, array | Can defer initialization |

---

[Return to Docs](../README.md)
