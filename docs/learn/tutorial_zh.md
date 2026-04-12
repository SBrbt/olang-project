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
- `let` — 全局变量
- `Ret name(...)` — 内部函数（仅本文件，返回类型 `Ret` 在前）
- `extern Ret name(...)` — 导出或仅声明（可链接）

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
let x<i32> @stack<32>(42i32);
let y<i64> @stack<64>(100i64);
let z<i32> @stack<32>(50);  // 无后缀十进制字面量为 i32
```

#### 多绑定（`@stack`）

可以在**同一块栈上存储**上挂**多个名字**：写成「`let` + `名字<类型>`」重复（每多一个名字就多写一个 `let`），再写 `@stack<总位数>(初始化式)`（中间无 `from`）。

- 各绑定类型位宽之和必须等于 `总位数`。
- **多个名字**时，每个绑定只能是**标量**（不能是 `struct` / `array`）；类型**可以互不相同**（例如一段里既有 `f32` 又有 `i32`）。通常用一个整数初始化式一次性给出整块位的模式。
- 内存按声明顺序**紧密排列**（第一个名字在块内较低地址一侧）。

```olang
// 低 32 位为 1.0f32，高 32 位为 42i32（小端 u64 常量）
let x<f32> let n<i32> @stack<64>(0x0000002A3F800000u64);
// x 上做浮点运算，n 上做整型运算；store<x, …> 只改写低半部分，不碰 n
```

示例：`ex_rt_multi_view.ol`。

#### 文件级 `let`（全局）

函数外不用 `@stack`，改用 `@data`、`@bss`、`@rodata` 或 `@section("…")`。写法与上面相同：`(名字<类型>)+` + `@分配器<总位数>(…)`。

- **`@data`** — 可写 `.data`，一般要有初始化式（与 `@bss` / `@rodata` 的约束见语法说明）。
- **`@bss`** — 可写 `.bss`，**不能**写初始化式（由加载器按零初始化）。
- **`@rodata`** — 只读；初始化式须为常量。
- **`@section("段名")`** — 放进自定义段（若链接布局依赖该段，链接脚本里要一致）。

**多个名字**共享一块静态存储时，规则与栈上多绑定相同：**仅标量**，总位数对齐。**只有一个**绑定时，类型可以是**聚合**（`struct` / `array`）；链接器里该对象的符号名是**第一个**名字，其余名字表示同一块存储上的不同视图/偏移。

```olang
let gcount<i32> @data<32>(10);
let gx<f32> let gn<i32> @data<64>(0x0000002A3F800000u64);
```

更多示例：`ex_rt_global_sections.ol`、`ex_rt_global_multi_view.ol`、`ex_rt_section_custom.ol`。完整规则见 [语法参考](../book/syntax_zh.md)「变量绑定」。

#### 聚合类型（可延后初始化）

```olang
type Point = struct { x: i32, y: i32 };
let p<Point> @stack<64>();  // 无需立即初始化
store<p.x, 10i32>;
store<p.y, 20i32>;
```

#### 写入（`store`）与值拷贝

```olang
let a<i32> @stack<32>(0i32);
store<a, a + 1i32>;  // 更新存储

type Pair = struct { a: i64, b: i64 };
let x<Pair> @stack<128>();
store<x.a, 1i64>;
store<x.b, 2i64>;
let y<Pair> @stack<128>();
store<y, x>;         // 值拷贝！y 获得独立副本
store<x.b, 9i64>;    // 不影响 y
```

#### 类型转换

```olang
cast<i32>(value)              // 显式转换（仅允许的组合）
cast<ptr>(0x1000u64)          // 同宽值：字面量 / 非裸变量表达式
let u<u32> <u32>addr x;         // 同宽另一名字：对已有存储取址再包一层 <T>（或同一 let 里多绑定）
```

同宽换视图示例：`ex_rt_u32_view.ol`。

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

let r<Rect> @stack<128>();
store<r.tl.x, 0i32>;  // 嵌套字段写入
store<r.br.x, 10i32>;

// 聚合字段写入（整结构体值拷贝）
let p<Point> @stack<64>();
store<p.x, 1i32>;
store<p.y, 2i32>;
store<r.tl, p>;  // 拷贝整个 Point
```

---

### 数组

```olang
type Int5 = array<i32, 5>;

let arr<Int5> @stack<160>();
store<arr[0], 10i32>;
store<arr[1], 20i32>;

i32 sum(a: Int5) {  // 传递指针
    return a[0] + a[1];
}
```

**注意**：不检查越界。

---

### 指针

```olang
// 取地址
let x<i32> @stack<32>(42i32);
let p<ptr> @stack<64>(addr x);

// 通过 `find<p>` 绑定间接视图后 `load`/`store` 名字（与 `x` 同一块存储）
let v<i32> <i32>(find<p>);
store<v, 100i32>;

// 字符串字面量
let s<ptr> @stack<64>(addr "Hello\n");
```

#### 系统调用示例

使用 kasm POSIX 封装层（`libposix.kasm`）提供的 `posix_write`：

```olang
extern i64 posix_write(fd: i64, buf: ptr, n: i64);

extern i32 main() {
    posix_write(1i64, "Hello from OLang!\n", 18i64);
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
