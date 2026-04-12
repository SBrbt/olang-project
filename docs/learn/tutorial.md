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

#### File-scope `let` (globals)

Outside functions, use `@data` / `@bss` / `@rodata` / `@section("…")` allocators: one or more `Ident < Type >`, then `@allocator<TOTAL_BITS>(...)`. Rules mirror local multi-binding (total bits must match; multiple names are scalar-only). See [syntax reference](../book/syntax.md) (“Variable binding”) and `ex_rt_global_sections.ol`, `ex_rt_global_multi_view.ol`.

```olang
let gcount<i32> @data<32>(10);
let glo_lo<i32> glo_hi<i32> @data<64>(0x0000000200000001u64);
```

#### Aggregate Types (can defer initialization)

```olang
type Point = struct { x: i32, y: i32 };
let p<Point> @stack<64>();  // no immediate initialization needed
p.x = 10i32;
p.y = 20i32;
```

#### Assignment and Value Copy

```olang
let a<i32> @stack<32>(0i32);
a = a + 1i32;  // reassignment

type Pair = struct { a: i64, b: i64 };
let x<Pair> @stack<128>(); x.a = 1i64; x.b = 2i64;
let y<Pair> @stack<128>();
y = x;         // value copy! y gets independent copy
x.b = 9i64;    // does not affect y
```

#### Type Casting

```olang
cast<i32>(value)              // explicit cast (allowed conversions only)
reinterpret<ptr>(0x1000u64)   // same-sized rebind: u64 ↔ ptr, i32 ↔ u32, …
```

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
    i = i + 1i32;
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
r.tl.x = 0i32;  // nested access
r.br.x = 10i32;

// Aggregate field assignment (value copy)
let p<Point> @stack<64>(); p.x = 1i32; p.y = 2i32;
r.tl = p;  // copies entire Point
```

---

### Arrays

```olang
type Int5 = array<i32, 5>;

let arr<Int5> @stack<160>();
arr[0] = 10i32;
arr[1] = 20i32;

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

// Load/Store
let v<i32> @stack<32>(load<i32>(p));  // read
store<i32>(p, 100i32);       // write

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
bash examples/olc -o examples/out/prog.elf main.ol lib.ol
```

---

### Next Steps

- [More examples](examples.md)
- [Syntax reference](../book/syntax.md)
- [Implementation details](../internals/architecture.md)
