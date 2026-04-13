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
- `let` — **symbol → reference**: after **`let`** comes the **name** (you may chain several `let`s); the name is bound to the **reference** produced on the **right**. **`let` is not a type specifier.** A reference may have an **element type** or be **untyped** (no static element type yet—e.g. raw `stack[…]` / `find[…]`, then **`<T>…`** views). At **file scope** with a global placement, names are **file scope**; **inside a function**, they are **locals**.
- `Ret name(...)` — internal functions (file-private; return type `Ret` first)
- `extern Ret name(...)` — exported definitions or forward declarations (linkable)

---

### Variables and Types

In the usual form, **`let`** *name* is enough: the **right-hand side is a reference** (e.g. **`stack[…]`**). The **`let`** keyword **does not carry a type**; the binding segment lists **identifiers only**, and the element type comes from the **initializer** or a later **`<T>`** view (e.g. **`let v <T> raw`**).

For **`let`** + **`stack[…]`**:

1. **With initializer:** **`let`** *name* **`stack[`** *bit width* **`,`** *initializer* **`];`** — element type comes **only** from the initializer.
2. **Without initializer:** **`let`** *raw* **`stack[`** *bit width* **`];`** gives **untyped** storage, then **`let`** *view* **`<`** *Type* **`>`** *raw* for a typed reference, e.g. **`let raw stack[128];`** / **`let s <Pair> raw;`** (see `ex_rt_multi_view.ol`). At **file scope**, only allocators are allowed after **`let`** — not **`let v <T> x`** — so use an initializer to fix scalar layout (e.g. **`data[64, 0i64]`**) when needed.

Inside **`[`** **`]`**: first item is always **bit width**; second is either an initializer or omitted (omitted ⇒ untyped blob, then **`<T>`** or **`store`**).

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
let x stack[32, 42i32];
let y stack[64, 100i64];
let z stack[32, 50];  // unsuffixed decimal literal is i32; first arg is bit width
```

#### Several names over one piece of storage

Each **`stack[…]`** introduces **one** `let` name — **no** list of view types inside **`stack`**.

- **Aliases / views on an existing ref:** **`let a let b (x)`** — **`x`** is a reference on the right; see `ex_rt_chain_let.ol`.
- **Multiple fields in one layout:** define a **`struct`**, then **`let raw stack[`** *width* **`];`** + **`let s <Typedef> raw`** (see `ex_rt_multi_view.ol`).

```olang
type Pack = struct { x: f32, n: i32 };
let s_raw stack[64];
let s <Pack> s_raw;
store[s.x, 1.0f32];
store[s.n, 42i32];
```

See `ex_rt_multi_view.ol`.

#### Top-level binding and file-scope storage

It is still **`let`** *names* …, but at file scope the right side is restricted to global allocators: **`data`**, **`rodata`**, **`bss`**, or **`section`** (not `stack`, not general `RefExpr`). Names are **file scope**. Bracket rules match the **`stack`** section; details: [syntax reference](../book/syntax.md). Without an initializer, globals are often **untyped** blobs or use an initializer to fix scalar layout.

```olang
let gcount data[32, 10];
```

Practical global constraints:

- Multi-name global views are scalar-only.
- The sum of view sizes must match the declared global bit width.
- `bss[...]` has no initializer.
- `rodata[...]` initializer must be compile-time constant.

More examples: `ex_rt_global_sections.ol`, `ex_rt_global_multi_view.ol`, `ex_rt_section_custom.ol`. Full rules: [syntax reference](../book/syntax.md) (“Variable binding”).

#### Aggregate Types (can defer initialization)

```olang
type Point = struct { x: i32, y: i32 };
let p_raw stack[64];
let p <Point> p_raw;
store[p.x, 10i32];
store[p.y, 20i32];
```

#### Writes (`store`) and Value Copy

```olang
let a stack[32, 0i32];
store[a, a + 1i32];  // update storage
```

```olang
type Pair = struct { a: i64, b: i64 };
let x_raw stack[128];
let x <Pair> x_raw;
store[x.a, 1i64];
store[x.b, 2i64];
let y_raw stack[128];
let y <Pair> y_raw;
store[y, x];         // value copy: y gets an independent copy
store[x.b, 9i64];    // does not affect y
```

#### Type Casting

```olang
i32(value)                    // i/u/b/f families and int↔float only; no ptr/bool
f32(3i32)
let u <u32>x;            // `u` is a new ref (element type u32) to the same bytes as `x`
```

Same-width storage reinterpret: **`let n <T>x`** (see `ex_rt_u32_view.ol`), not `T(expr)` on a bare name.

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
let i stack[32, 0i32];
while (i < 10i32) {
    store[i, i + 1i32];
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

let r_raw stack[128];
let r <Rect> r_raw;   // two Point fields
store[r.tl.x, 0i32];  // nested field writes
store[r.br.x, 10i32];

// Aggregate field writes (whole-struct value copy)
let p_raw stack[64];
let p <Point> p_raw;
store[p.x, 1i32];
store[p.y, 2i32];
store[r.tl, p];  // copies entire Point
```

---

### Arrays

```olang
type Int5 = array(i32, 5);

let arr_raw stack[160];   // 5 × i32 → 160 bits
let arr <Int5> arr_raw;
store[arr[0], 10i32];
store[arr[1], 20i32];

i32 sum(a: Int5) {  // passes pointer
    return a[0] + a[1];
}
```

**Note**: no bounds checking.

---

### Pointers

```olang
// Address-of
let x stack[32, 42i32];
let p stack[64, addr[x]];

// `find[Expr]` requires `Expr` to have type `ptr`. Here `p` holds a `ptr`; use `load[p]` to get that value.
// In `let`, use `<T>find[...]` for a typed reference view (not `(i32)(...)`; value conversion is `i32(expr)`).
let v <i32>(find[load[p]]);
store[v, 100i32];

// String literal (`ptr`-sized slot)
let s stack[64, "Hello\n"];
```

#### System Call Example

Uses `posix_write` from the kasm POSIX shim (`libposix.kasm`):

```olang
extern i64 posix_write(fd: i32, buf: ptr, n: u64);

extern i32 main() {
    posix_write(1i32, "Hello from OLang!\n", 18u64);
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
