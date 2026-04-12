# Tutorial

**English** | **[中文](tutorial_zh.md)**

---

Complete OLang language tutorial.

### Table of Contents

1. [Basics](#basics)
2. [Variables and Types](#variables-and-types)
3. [Control Flow](#control-flow)
4. [Functions](#functions)
5. [Structs](#structs)
6. [Arrays](#arrays)
7. [Pointers](#pointers)
8. [Multi-file](#multi-file)

---

### Basics

Programs return integers as exit codes:

```olang
extern i32 main() {
    return 42;
}
```

Top-level elements:
- `type` — type definitions
- `let` — global variables
- `Ret name(...)` — internal functions (file-private; return type `Ret` first)
- `extern Ret name(...)` — exported definitions or forward declarations (linkable)

---

### Variables and Types

#### Scalar Types (must be initialized)

| Type | Size | Example |
|------|------|---------|
| `i32` | 4 bytes | `42i32` or `42` |
| `i64` | 8 bytes | `42i64` |
| `u32` | 4 bytes | `42u32` |
| `u64` | 8 bytes | `42u64`, `18446744073709551615u64` |
| `u8` | 1 byte | `255u8` |
| `bool` | 1 byte | `true`, `false` |
| `ptr` | 8 bytes | pointer |

```olang
let x<i32> @stack<32>(42i32);
let y<i64> @stack<64>(100i64);
let z<i32> @stack<32>(50);  // unsuffixed decimal literal is i32
```

#### Multi-binding on `@stack`

You can declare **several names** on **one** stack blob: list `(Ident<Type>)+`, then `@stack<TOTAL_BITS>(initializer)` (no `from` keyword).

- The **sum** of all bindings’ sizes (in bits) must equal `TOTAL_BITS`.
- With **multiple names**, write **`let` before each `Ident<Type>`** (e.g. `let x<f32> let n<i32> @stack<64>(...)`). Each binding must be a **scalar** (not struct/array). Bindings may use **different** scalar types (e.g. `f32` and `i32` in one blob). One initializer expression supplies the bits (often a single integer literal whose pattern covers all slots).
- Layout is a **tight pack** in declaration order (first field at the low-address end of the blob).

```olang
// Low 32 bits: 1.0f32; high 32 bits: 42i32 (little-endian u64 pattern)
let x<f32> let n<i32> @stack<64>(0x0000002A3F800000u64);
// float math on `x`, integer math on `n`; store<x, …> rewrites only the low half
```

See `ex_rt_multi_view.ol`.

#### File-scope `let` (globals)

Outside functions, use `@data`, `@bss`, `@rodata`, or `@section("…")` instead of `@stack`. The same multi-binding shape applies: `(Ident<Type>)+` then `@allocator<TOTAL_BITS>(...)`.

- **`@data`** — writable `.data`, must have an initializer (unless your workflow uses only `@bss` / `@rodata` rules below).
- **`@bss`** — writable `.bss`, **no** initializer (zero-filled at load time).
- **`@rodata`** — read-only; initializer must be a constant.
- **`@section("name")`** — place the blob in a custom section (link script must mention it if you rely on layout).

When there are **several names** on one global blob, they behave like stack multi-binding: **only scalars**, total bits must match. When there is **a single** binding, it may be an **aggregate** (struct or array); the **first** name is the **linker symbol** for the whole object, other names are alternate views/offsets into it.

```olang
let gcount<i32> @data<32>(10);
let gx<f32> let gn<i32> @data<64>(0x0000002A3F800000u64);
```

More examples: `ex_rt_global_sections.ol`, `ex_rt_global_multi_view.ol`, `ex_rt_section_custom.ol`. Full rules: [syntax reference](../book/syntax.md) (“Variable binding”).

#### Aggregate Types (can defer initialization)

```olang
type Point = struct { x: i32, y: i32 };
let p<Point> @stack<64>();  // no immediate initialization needed
store<p.x, 10i32>;
store<p.y, 20i32>;
```

#### Writes (`store`) and Value Copy

```olang
let a<i32> @stack<32>(0i32);
store<a, a + 1i32>;  // update storage

type Pair = struct { a: i64, b: i64 };
let x<Pair> @stack<128>();
store<x.a, 1i64>;
store<x.b, 2i64>;
let y<Pair> @stack<128>();
store<y, x>;         // value copy: y gets an independent copy
store<x.b, 9i64>;    // does not affect y
```

#### Type Casting

```olang
cast<i32>(value)              // explicit cast (allowed conversions only)
cast<ptr>(0x1000u64)          // same-width value (literal / non-variable expr)
let u<u32> <u32>addr x;         // another name, same bytes: wrap `addr` in `<T>` (or multi-bind in one let)
```

Same-width view example: `ex_rt_u32_view.ol`.

---

### Control Flow

#### if/else

```olang
if (x < 0i32) {
    return -x;
} else {
    return x;
}
```

#### while

```olang
let i<i32> @stack<32>(0i32);
while (i < 10i32) {
    store<i, i + 1i32>;
}
```

#### break / continue

```olang
while (true) {
    if (done) { break; }
    if (skip) { continue; }
}
```

---

### Functions

```olang
// Internal function
i32 add(a: i32, b: i32) {
    return a + b;
}

// Exported function
extern i32 main() {
    return add(3i32, 4i32);
}

// External declaration
extern i64 write(fd: i32, buf: ptr, count: u64);
```

**Limit**: up to 8 register parameters; additional parameters are passed on the stack.

#### Recursion

```olang
i32 factorial(n: i32) {
    if (n <= 1i32) { return 1i32; }
    return n * factorial(n - 1i32);
}
```

---

### Structs

```olang
type Point = struct { x: i32, y: i32 };
type Rect = struct { tl: Point, br: Point };

let r<Rect> @stack<128>();
store<r.tl.x, 0i32>;  // nested field writes
store<r.br.x, 10i32>;

// Aggregate field writes (whole-struct value copy)
let p<Point> @stack<64>();
store<p.x, 1i32>;
store<p.y, 2i32>;
store<r.tl, p>;  // copies entire Point
```

---

### Arrays

```olang
type Int5 = array<i32, 5>;

let arr<Int5> @stack<160>();
store<arr[0], 10i32>;
store<arr[1], 20i32>;

i32 sum(a: Int5) {  // passes pointer
    return a[0] + a[1];
}
```

**Note**: no bounds checking.

---

### Pointers

```olang
// Address-of
let x<i32> @stack<32>(42i32);
let p<ptr> @stack<64>(addr x);

// After `find<p>`, load/store the binding name (same storage as `x`)
let v<i32> <i32>(find<p>);
store<v, 100i32>;

// String literal
let s<ptr> @stack<64>(addr "Hello\n");
```

#### System Call Example

Uses `posix_write` from the kasm POSIX shim (`libposix.kasm`):

```olang
extern i64 posix_write(fd: i64, buf: ptr, n: i64);

extern i32 main() {
    posix_write(1i64, "Hello from OLang!\n", 18i64);
    return 0;
}
```

---

### Multi-file

**lib.ol**:
```olang
extern i32 multiply(a: i32, b: i32) {
    return a * b;
}
```

**main.ol**:
```olang
extern i32 multiply(a: i32, b: i32);  // declaration

extern i32 main() {
    return multiply(6i32, 7i32);
}
```

Compile:
```bash
bash examples/linux_x86_64/olc -o examples/linux_x86_64/out/prog.elf main.ol lib.ol
```

---

### Next Steps

- [More examples](examples.md)
- [Syntax reference](../book/syntax.md)
- [Implementation details](../internals/architecture.md)
