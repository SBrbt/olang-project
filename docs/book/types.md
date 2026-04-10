# Type System

**English** | **[中文](types_zh.md)**

---

### Built-in Types

| Type | Size | Range | Description |
|------|------|-------|-------------|
| `void` | - | - | No return value |
| `bool` | 1 byte | true/false | Boolean |
| `u8` | 1 byte | 0..255 | Unsigned byte |
| `i32` | 4 bytes | -2^31..2^31-1 | Signed 32-bit |
| `u32` | 4 bytes | 0..2^32-1 | Unsigned 32-bit |
| `i64` | 8 bytes | -2^63..2^63-1 | Signed 64-bit |
| `u64` | 8 bytes | 0..2^64-1 | Unsigned 64-bit |
| `ptr` | 8 bytes | address | Raw pointer |

### Literal Suffixes

| Suffix | Type | Range |
|--------|------|-------|
| (none) | `i32` | -2^31..2^31-1 |
| `i32` | `i32` | -2^31..2^31-1 |
| `u32` | `u32` | 0..2^32-1 |
| `i64` | `i64` | -2^63..2^63-1 |
| `u64` | `u64` | 0..2^64-1 |
| `u8` | `u8` | 0..255 |

```olang
42           // i32
42i64        // i64
42u64        // u64
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

```olang
cast<DestType>(source)
```

- Explicit cast
- Bit pattern preserved
- Integer ↔ pointer conversion allowed

```olang
cast<i32>(i64_value)     // truncate
cast<u32>(-1i32)         // reinterpret bit pattern
cast<ptr>(0x1000u64)     // int to pointer
```

### Type Categories

| Category | Types | Initialization |
|----------|-------|----------------|
| Scalar | i32, i64, u32, u64, u8, bool, ptr | Must initialize at `let` |
| Aggregate | struct, array | Can defer initialization |

---

[Return to Docs](../README.md)
