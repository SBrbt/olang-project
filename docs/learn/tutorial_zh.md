# 教程

**[English](tutorial.md)** | **中文**

---

完整 OLang 语言教程。

### 目录

1. [基础概念](#基础概念)
2. [变量与类型](#变量与类型)
3. [控制流](#控制流)
4. [函数](#函数)
5. [结构体](#结构体)
6. [数组](#数组)
7. [指针](#指针)
8. [多文件](#多文件)

---

### 基础概念

程序返回整数作为退出码：

```olang
extern fn main() -> i32 {
    return 42;
}
```

顶层元素：
- `type` — 类型定义
- `let` — 全局变量
- `fn` — 内部函数（仅本文件）
- `extern fn` — 导出函数（可链接）

---

### 变量与类型

#### 标量类型（必须初始化）

| 类型 | 大小 | 示例 |
|------|------|------|
| `i32` | 4字节 | `42i32` 或 `42` |
| `i64` | 8字节 | `42i64` |
| `u32` | 4字节 | `42u32` |
| `u64` | 8字节 | `42u64`, `18446744073709551615u64` |
| `u8` | 1字节 | `255u8` |
| `bool` | 1字节 | `true`, `false` |
| `ptr` | 8字节 | 指针 |

```olang
let x: i32 = 42i32;
let y: i64 = 100i64;
let z = 50;  // 默认 i32
```

#### 聚合类型（可延后初始化）

```olang
type Point = struct { x: i32, y: i32 };
let p: Point;  // 无需立即初始化
p.x = 10i32;
p.y = 20i32;
```

#### 赋值与值拷贝

```olang
let a: i32 = 0i32;
a = a + 1i32;  // 重新赋值

type Pair = struct { a: i64, b: i64 };
let x: Pair; x.a = 1i64; x.b = 2i64;
let y: Pair;
y = x;         // 值拷贝！y 获得独立副本
x.b = 9i64;    // 不影响 y
```

#### 类型转换

```olang
cast<i32>(value)              // 显式转换（仅允许的组合）
reinterpret<ptr>(0x1000u64)   // 同宽重解释：u64 ↔ ptr、i32 ↔ u32 等
```

---

### 控制流

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

### 函数

```olang
// 内部函数
fn add(a: i32, b: i32) -> i32 {
    return a + b;
}

// 导出函数
extern fn main() -> i32 {
    return add(3i32, 4i32);
}

// 声明外部函数
extern fn write(fd: i32, buf: ptr, count: u64) -> i64;
```

**限制**：最多 8 个寄存器参数；更多参数通过栈传递。

#### 递归

```olang
fn factorial(n: i32) -> i32 {
    if (n <= 1i32) { return 1i32; }
    return n * factorial(n - 1i32);
}
```

---

### 结构体

```olang
type Point = struct { x: i32, y: i32 };
type Rect = struct { tl: Point, br: Point };

let r: Rect;
r.tl.x = 0i32;  // 嵌套访问
r.br.x = 10i32;

// 聚合字段赋值（值拷贝）
let p: Point; p.x = 1i32; p.y = 2i32;
r.tl = p;  // 拷贝整个 Point
```

---

### 数组

```olang
type Int5 = array<i32, 5>;

let arr: Int5;
arr[0] = 10i32;
arr[1] = 20i32;

fn sum(a: Int5) -> i32 {  // 传递指针
    return a[0] + a[1];
}
```

**注意**：不检查越界。

---

### 指针

```olang
// 取地址
let x: i32 = 42i32;
let p: ptr = addr x;

// 加载/存储
let v: i32 = load<i32>(p);  // 读取
store<i32>(p, 100i32);       // 写入

// 字符串字面量
let s: ptr = addr "Hello\n";
```

#### 系统调用示例

使用 kasm POSIX 封装层（`libposix.kasm`）提供的 `posix_write`：

```olang
extern fn posix_write(fd: i64, buf: ptr, n: i64) -> i64;

extern fn main() -> i32 {
    posix_write(1i64, "Hello from OLang!\n", 18i64);
    return 0;
}
```

---

### 多文件

**lib.ol**：
```olang
extern fn multiply(a: i32, b: i32) -> i32 {
    return a * b;
}
```

**main.ol**：
```olang
extern fn multiply(a: i32, b: i32) -> i32;  // 声明

extern fn main() -> i32 {
    return multiply(6i32, 7i32);
}
```

编译：
```bash
bash examples/olc -o examples/out/prog.elf main.ol lib.ol
```

---

### 下一步

- [更多示例](examples_zh.md)
- [语法速查](../book/syntax_zh.md)
- [实现细节](../internals/architecture_zh.md)
