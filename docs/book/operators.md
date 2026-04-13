# Operators

**English** | **[中文](operators_zh.md)**

---

OLang is **strongly and statically typed**: for every binary operator, the left and right operands must have the **same** type. There are **no implicit numeric promotions** in expressions—use **`T(…)`** (value cast) or **`let n <T>x`** (same storage, new view) when you need a different type.

The tables below list, **by type**, which operators are accepted by the compiler (`sema.c`).

---

### `bool`

| Operator | Meaning | Result type |
|----------|---------|-------------|
| `!a` | Logical not | `bool` |
| `a && b`, `a \|\| b` | Logical and / or (both operands `bool`) | `bool` |
| `a == b`, `a != b` | Equality | `bool` |

There is no ordering (`<`, `>`, …) for `bool`.

---

### Integers (`i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`)

Both operands must use the **same** integer type (e.g. two `i32`, not `i32` and `u32` in one expression).

| Operator | Meaning | Result type |
|----------|---------|-------------|
| `-a` | Negation | same as operand |
| `a + b`, `a - b`, `a * b`, `a / b`, `a % b` | Arithmetic | same as operands |
| `a < b`, `a > b`, `a <= b`, `a >= b` | Ordered comparison | `bool` |
| `a == b`, `a != b` | Equality | `bool` |

**Not available on integers:** `&`, `|`, `^`, `<<`, `>>` (those apply only to `b*` types). Use `b8`…`b64` if you need bitwise operations, or use an explicit `bN(…)` / `uN(…)` value cast after reading the rules in [types](types.md).

---

### Floating-point (`f16`, `f32`, `f64`)

Both operands must have the **same** float type.

| Operator | Meaning | Result type |
|----------|---------|-------------|
| `-a` | Negation | same as operand |
| `a + b`, `a - b`, `a * b`, `a / b` | Arithmetic | same as operands |
| `a < b`, `a > b`, `a <= b`, `a >= b` | Ordered comparison | `bool` |
| `a == b`, `a != b` | Equality | `bool` |

**Not available:** `%` (remainder is only for integers and `b*`).

---

### Bit vectors (`b8`, `b16`, `b32`, `b64`)

Both operands must have the **same** `b*` type. These types support both “word” arithmetic and bitwise ops.

| Operator | Meaning | Result type |
|----------|---------|-------------|
| `-a` | Negation | same as operand |
| `~a` | Bitwise NOT | same as operand |
| `a + b`, … `a % b` | Arithmetic (same set as integers, including `%`) | same as operands |
| `a & b`, `a \| b`, `a ^ b`, `a << b`, `a >> b` | Bitwise and / or / xor / shifts | same as operands |
| `a == b`, `a != b` | Equality | `bool` |

**Not available:** ordered comparison (`<`, `>`, `<=`, `>=`).

---

### `ptr`

| Operator | Meaning | Result type |
|----------|---------|-------------|
| `a == b`, `a != b` | Address equality | `bool` |

Other binary operators are not defined for `ptr`.

---

### Aggregates (struct, array)

Struct and array values are **not** “scalar” in the checker: you cannot use `==` / `!=` or arithmetic on them in expressions. Writes use **`store[…]`** on an lvalue (including field and index paths). Compare fields or use field/index patterns if you need a scalar view.

---

### Precedence (all types)

High → low:

```
() [] . addr[find] sizeof load <T> T()      // highest (T() = value cast; <T> = new ref + element type)
! ~ -                               // unary
* / %
+ -
<< >>
< > <= >=
== !=
&
^
|
&&
||                                  // lowest
```

---

### Examples

```olang
let i stack[32, (a + b) * c];
let cmp stack[8, (i > 0i32) && done];     // i32 compares; bool logic

let x stack[32, 0xFF00b32];
let y stack[32, x & 0x00FFb32];             // bitwise only on b*
```

---

[Return to Docs](../README.md)
