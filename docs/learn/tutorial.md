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
extern fn main() -> i32 {
    return 42;
}
```

Top-level elements:
- `type` — type definitions
- `let` — global variables
- `fn` — internal functions (file-private)
- `extern fn` — exported functions (linkable)

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
let x: i32 = 42i32;
let y: i64 = 100i64;
let z = 50;  // default i32
```

#### Aggregate Types (can defer initialization)

```olang
type Point = struct { x: i32, y: i32 };
let p: Point;  // no immediate initialization needed
p.x = 10i32;
p.y = 20i32;
```

#### Assignment and Value Copy

```olang
let a: i32 = 0i32;
a = a + 1i32;  // reassignment

type Pair = struct { a: i64, b: i64 };
let x: Pair; x.a = 1i64; x.b = 2i64;
let y: Pair;
y = x;         // value copy! y gets independent copy
x.b = 9i64;    // does not affect y
```

#### Type Casting

```olang
cast<i32>(value)   // explicit cast
cast<ptr>(0x1000u64)  // int to pointer
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
let i: i32 = 0i32;
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
fn add(a: i32, b: i32) -> i32 {
    return a + b;
}

// Exported function
extern fn main() -> i32 {
    return add(3i32, 4i32);
}

// External declaration
extern fn write(fd: i32, buf: ptr, count: u64) -> i64;
```

**Limit**: maximum 6 parameters.

#### Recursion

```olang
fn factorial(n: i32) -> i32 {
    if (n <= 1i32) { return 1i32; }
    return n * factorial(n - 1i32);
}
```

---

### Structs

```olang
type Point = struct { x: i32, y: i32 };
type Rect = struct { tl: Point, br: Point };

let r: Rect;
r.tl.x = 0i32;  // nested access
r.br.x = 10i32;

// Aggregate field assignment (value copy)
let p: Point; p.x = 1i32; p.y = 2i32;
r.tl = p;  // copies entire Point
```

---

### Arrays

```olang
type Int5 = array<i32, 5>;

let arr: Int5;
arr[0] = 10i32;
arr[1] = 20i32;

fn sum(a: Int5) -> i32 {  // passes pointer
    return a[0] + a[1];
}
```

**Note**: no bounds checking.

---

### Pointers

```olang
// Address-of
let x: i32 = 42i32;
let p: ptr = addr x;

// Load/Store
let v: i32 = load<i32>(p);  // read
store<i32>(p, 100i32);       // write

// String literal
let s: ptr = addr "Hello\n";
```

#### System Call Example

```olang
extern fn write(fd: i32, buf: ptr, count: u64) -> i64;

extern fn main() -> i32 {
    let msg: ptr = addr "Hello from OLang!\n";
    write(1i32, msg, 19u64);
    return 0;
}
```

---

### Multi-file

**lib.ol**:
```olang
extern fn multiply(a: i32, b: i32) -> i32 {
    return a * b;
}
```

**main.ol**:
```olang
extern fn multiply(a: i32, b: i32) -> i32;  // declaration

extern fn main() -> i32 {
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
