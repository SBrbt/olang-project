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

Explicit conversions the compiler allows between **different** types: widening/narrowing within the same integer family (`i*`↔`i*`, `u*`↔`u*`), `b*`↔`b*`, `f*`↔`f*`, `bool`↔integers, and float↔integer conversions. You cannot `cast` to the same type as the expression (use `reinterpret` for same-width rebinding).

```olang
cast<i32>(i64_value)   // narrow
cast<i32>(i8_value)    // widen
cast<u32>(200u8)       // unsigned widen
```

#### `reinterpret<DestType>(expr)`

Rebinds the **same-sized** value as another type (e.g. `i32`↔`u32`, `u64`↔`ptr`, `b32`↔`i32`). Not allowed for `bool`, structs, or arrays.

```olang
reinterpret<u32>(-1i32)      // same 4 bytes, unsigned view
reinterpret<ptr>(0x1000u64)  // integer to pointer (8-byte types)
```

### Type Categories

| Category | Types | Initialization |
|----------|-------|----------------|
| Scalar | All built-ins except `void` | Must initialize at `let` |
| Aggregate | struct, array | Can defer initialization |

---

[Return to Docs](../README.md)
