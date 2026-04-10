# Operators

**English** | **[中文](operators_zh.md)**

---

### Arithmetic

| Operator | Example | Description |
|----------|---------|-------------|
| `+` | `a + b` | Addition |
| `-` | `a - b` | Subtraction |
| `*` | `a * b` | Multiplication |
| `/` | `a / b` | Division (integer) |
| `%` | `a % b` | Modulo |
| `-x` | `-a` | Unary negation |

### Comparison

| Operator | Description |
|----------|-------------|
| `==` | Equal |
| `!=` | Not equal |
| `<` | Less than |
| `>` | Greater than |
| `<=` | Less than or equal |
| `>=` | Greater than or equal |

Returns `bool`.

### Logical

| Operator | Description |
|----------|-------------|
| `&&` | Logical AND |
| `||` | Logical OR |
| `!` | Logical NOT |

```olang
if (a > 0i32 && b > 0i32) { }
if (!done) { }
```

### Bitwise

| Operator | Example | Description |
|----------|---------|-------------|
| `&` | `a & b` | Bitwise AND |
| `|` | `a | b` | Bitwise OR |
| `^` | `a ^ b` | Bitwise XOR |
| `<<` | `a << n` | Left shift |
| `>>` | `a >> n` | Right shift |

### Precedence

High → Low:

```
() [] . addr cast load      // highest
! -                         // unary
* / %
+ -
<< >>
< > <= >=
== !=
&
^
|
&&
||                          // lowest
```

### Examples

```olang
let x: i32 = (a + b) * c;           // parentheses for clarity
let y: bool = (x > 0i32) && flag;   // comparison before logic
let z: i32 = a & 0xFFi32;           // get low 8 bits
```

---

[Return to Docs](../README.md)
