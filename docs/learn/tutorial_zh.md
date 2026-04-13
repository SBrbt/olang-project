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
extern i32 main() {
    return 42;
}
```

顶层元素：
- `type` — 类型定义
- `let` — **符号指向引用**：**`let` 后面跟名字**（可链式写多个 `let`），语义上是把**这个名字**绑定到**右侧给出的那条引用**上；**`let` 本身不是类型说明符**。引用可以**带元素类型**，也可以**无类型**（未绑定静态元素类型，例如裸的 `stack[…]` / `find[…]` 一侧，再用 **`<T>…`** 视图）。在**文件顶层**用全局放置时，名字是**文件作用域**；在**函数体内**是**局部**。
- `Ret name(...)` — 内部函数（仅本文件，返回类型 `Ret` 在前）
- `extern Ret name(...)` — 导出或仅声明（可链接）

---

### 变量与类型

**`let` 名字** 在常见写法里就够用：**右边是引用**（例如 `stack[…]`）。**`let` 不是类型说明符**；绑定段**只写标识符**，元素类型由**初始化式**或后续的 **`let v <T> raw`** 等推出。  
**`let` + `stack[…]`** 常见两种：

1. **有初始化式**：**`let` 名字 `stack[` 位宽 `,` 初始化式 `];`** —— 元素类型**只**由**初始化式**决定。
2. **无初始化式**（要先占位再 `store`）：**`let` 裸名 `stack[` 位宽 `];`** 得到**无类型**存储，再 **`let` 视图名 `<` 类型 `>` 裸名** 绑定带元素类型的引用，例如：
   ```olang
   let raw stack[128];
   let s <Pair> raw;
   ```
   见 `ex_rt_multi_view.ol`、`ex_rt_aggregate_copy.ol`。文件顶层**只能**写 `data`/`bss`/… 分配器，不能写 **`let` `<T>` 引用表达式**；需要标量布局时可用 **`data`[`位宽`,` 初值`]`** 等（如 `data[64, 0i64]`）从初始化式推断类型。

方括号里：**第一项**永远是**无后缀十进制位宽（位数）**；**第二项**要么是初始化式，要么省略（省略时表示无类型块，须再跟 **`<T>`** 视图或后续再 `store`）。

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
let x stack[32, 42i32];
let y stack[64, 100i64];
let z stack[32, 50];  // 第一个参数为位数；无后缀十进制初始化式为 i32
```

#### 同一块存储上的多个名字

**`stack[…]`** 每次只对应**一个** `let` 名字；方括号里**不要**塞多种「视图类型」列表。

- **同一引用再绑名字**：**`let a let b (x)`** —— **`x`** 是已有引用；括号防止两个名字连在一起被误解析；见 `ex_rt_chain_let.ol`。
- **多个字段同一块布局**：先 `type … = struct`，再 **`let raw stack[位宽];`** + **`let s <Typedef> raw`**，见 `ex_rt_multi_view.ol`。

```olang
type Pack = struct { x: f32, n: i32 };
let s_raw stack[64];
let s <Pack> s_raw;
store[s.x, 1.0f32];
store[s.n, 42i32];
```

示例：`ex_rt_multi_view.ol`。

#### 顶层绑定与文件作用域存储

**仍是 `let` 名字 …，右侧是引用。** 在函数外不用 `stack`，而用 **`data` / `rodata` / `bss` / `section`**，名字落在**文件作用域**。方括号规则与上一段 **`stack`** 同类；**细节**见 [语法参考](../book/syntax_zh.md)。文件顶层无初始化式时多为**无类型**全局块或靠**初始化式**推断标量类型。

```olang
let gcount data[32, 10];
```

更多示例：`ex_rt_global_sections.ol`、`ex_rt_global_multi_view.ol`、`ex_rt_section_custom.ol`。完整规则见 [语法参考](../book/syntax_zh.md)「变量绑定」。

#### 聚合类型（可延后初始化）

```olang
type Point = struct { x: i32, y: i32 };
let p_raw stack[64];
let p <Point> p_raw;
store[p.x, 10i32];
store[p.y, 20i32];
```

#### 写入（`store`）与值拷贝

```olang
let a stack[32, 0i32];
store[a, a + 1i32];  // 更新存储
```

```olang
type Pair = struct { a: i64, b: i64 };
let x_raw stack[128];
let x <Pair> x_raw;
store[x.a, 1i64];
store[x.b, 2i64];
let y_raw stack[128];
let y <Pair> y_raw;
store[y, x];         // 值拷贝！y 获得独立副本
store[x.b, 9i64];    // 不影响 y
```

#### 类型转换

```olang
i32(value)                    // 仅 i/u/b/f 族及整数↔浮点；不含 ptr、bool
f32(3i32)
let u <u32>x;            // 新引用 u，元素 u32，与 x 同址；同址多种视图可用 struct、链式 let、<T>…
```

同宽存储换解释用 **`let n <T>x`**（见 `ex_rt_u32_view.ol`），不要用裸变量上的 `T(…)`。

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

### 函数

```olang
// 内部函数
i32 add(a: i32, b: i32) {
    return a + b;
}

// 导出函数
extern i32 main() {
    return add(3i32, 4i32);
}

// 声明外部函数
extern i64 write(fd: i32, buf: ptr, count: u64);
```

**限制**：最多 8 个寄存器参数；更多参数通过栈传递。

#### 递归

```olang
i32 factorial(n: i32) {
    if (n <= 1i32) { return 1i32; }
    return n * factorial(n - 1i32);
}
```

---

### 结构体

```olang
type Point = struct { x: i32, y: i32 };
type Rect = struct { tl: Point, br: Point };

let r_raw stack[128];
let r <Rect> r_raw;   // 两个 Point 字段
store[r.tl.x, 0i32];  // 嵌套字段写入
store[r.br.x, 10i32];

// 聚合字段写入（整结构体值拷贝）
let p_raw stack[64];
let p <Point> p_raw;
store[p.x, 1i32];
store[p.y, 2i32];
store[r.tl, p];  // 拷贝整个 Point
```

---

### 数组

```olang
type Int5 = array(i32, 5);

let arr_raw stack[160];   // 位宽：5 × i32 → 5 × 32 = 160 位
let arr <Int5> arr_raw;
store[arr[0], 10i32];
store[arr[1], 20i32];

i32 sum(a: Int5) {  // 传递指针
    return a[0] + a[1];
}
```

**注意**：不检查越界。

---

### 指针

```olang
// 取地址
let x stack[32, 42i32];
let p stack[64, addr[x]];

// `find[Expr]` 要求 Expr 的类型为 ptr。`p` 是「存 ptr 的引用」，须用 `load[p]` 取出 ptr 值。
// `let` 右侧用 `<T>find[...]` 得到带元素类型 T 的引用视图（不是 `(i32)(...)`；值转换才是 `i32(表达式)`）。
let v <i32>(find[load[p]]);
store[v, 100i32];

// 字符串字面量
let s stack[64, "Hello\n"];
```

#### 系统调用示例

使用 kasm POSIX 封装层（`libposix.kasm`）提供的 `posix_write`：

```olang
extern i64 posix_write(fd: i32, buf: ptr, n: u64);

extern i32 main() {
    posix_write(1i32, "Hello from OLang!\n", 18u64);
    return 0;
}
```

---

### 多文件

**lib.ol**：
```olang
extern i32 multiply(a: i32, b: i32) {
    return a * b;
}
```

**main.ol**：
```olang
extern i32 multiply(a: i32, b: i32);  // 声明

extern i32 main() {
    return multiply(6i32, 7i32);
}
```

编译：
```bash
bash examples/linux_x86_64/olc -o examples/linux_x86_64/out/prog.elf main.ol lib.ol
```

---

### 下一步

- [更多示例](examples_zh.md)
- [语法速查](../book/syntax_zh.md)
- [实现细节](../internals/architecture_zh.md)
