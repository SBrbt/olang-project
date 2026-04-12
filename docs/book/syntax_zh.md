# 语法参考

**[English](syntax.md)** | **中文**

---

### 词法

#### 注释
```olang
// 单行注释
```

#### 标识符
`[A-Za-z_][A-Za-z0-9_]*`（实现限制：最多 63 个字符）

#### 关键字

- **语法与控制：** `extern`、`let`、`if`、`else`、`while`、`break`、`continue`、`return`、`type`、`struct`、`array`
- **运算：** `cast`、`reinterpret`、`load`、`store`、`addr`、`deref`、`as`
- **类型与字面量：** `void`、`bool`、`ptr`、`true`、`false`、`i8`、`i16`、`i32`、`i64`、`u8`、`u16`、`u32`、`u64`、`f16`、`f32`、`f64`、`b8`、`b16`、`b32`、`b64`

#### 字面量

| 类型 | 格式 | 示例 |
|------|------|------|
| 整数 | 十进制 | `42` |
| | 十六进制 | `0xFF` |
| | 二进制 | `0b1010` |
| | 八进制 | `0o77` |
| | 后缀 | `42i32`、`42u64`、`255u8`、`1b64` |
| 浮点 | 小数或科学计数 | `3.14`、`1e-3`，可选 `f32` / `f64` / `f16` 后缀 |
| 布尔 | `true`、`false` |
| 字符 | `'a'`、`'\n'`（u8） |
| 字符串 | `"hello\n"`（ptr） |

---

### 类型

```
Type ::= void | bool | ptr | i8 | i16 | i32 | i64 | u8 | u16 | u32 | u64
       | f16 | f32 | f64 | b8 | b16 | b32 | b64 | Ident
```

#### 结构体
```olang
type Ident = struct { FieldList };
type Point = struct { x: i32, y: i32 };
```

#### 数组
```olang
type Ident = array<ElementType, Size>;
type Int5 = array<i32, 5>;
```

> **聚合类型**指数组和结构体。与标量（`i32`、`bool` 等原始类型）不同，聚合类型允许先声明、延后初始化。

---

### 表达式

#### 优先级（高→低）

1. `addr`、`cast<T>`、`reinterpret<T>`、`load<T>`、`[]`、`.`、`()` — 调用
2. `!`、`~`、`-` — 一元（`~` 仅用于 `b*`；`!` 仅用于 `bool`）
3. `*`、`/`、`%`
4. `+`、`-`
5. `<<`、`>>`
6. `<`、`>`、`<=`、`>=`
7. `==`、`!=`
8. `&`
9. `^`
10. `|`
11. `&&`
12. `||`

#### 类型相关运算

```olang
cast<T>(expr)           // 显式转换（见 [types_zh](types_zh.md)）
reinterpret<T>(expr)    // 同宽重解释（不含 bool；不含聚合类型）
load<T>(ptr)            // 从指针读取（右值，不单独创建具名对象）
store<T>(ptr, val)      // 向指针写入
addr Ident   // 取址：依次解析局部名、全局 `let`、带 `extern` 声明的符号或函数符号（右值，类型为 `ptr`）
```

任意 `ptr` 类型的局部变量都可以用于 `deref`（运行时栈槽里仍然保存指针值）。

具名存储只能通过带**分配器**的 `let` 建立（见下）。字面量以及 `load`、`addr` 的求值结果是**值**（通常作右值）；当前后端实现仍可能把表达式结果先放到栈槽里，属于实现细节。

---

### 语句

#### 变量绑定（具名存储）

函数体内只能使用 **`@stack`**。每个绑定形如 `名字 < 类型 >`；多个绑定可共享同一块栈上存储；`@stack<位数>` 中的整数为**总位数**，各绑定类型大小（按位相加）必须与之相等，按声明顺序紧密排列。

```olang
let x<i32> @stack<32>(Expr);                    // 标量：必须有初始化式；位数与类型一致
let a<i32> b<i32> @stack<64>(Expr);            // 两个 i32 视图，共用 64 位
let s<MyStruct> @stack<N>();                  // 聚合：可有可空无括号；N = 8 * sizeof(MyStruct)
```

文件顶层使用**全局分配器**（不能用 `@stack`）。写法与函数体内的 `let` 类似：可声明**一个或多个** `Ident < Type >` 共享同一块静态存储；`@data` / `@bss` / `@rodata` / `@section("段名")` 后为 **`<位数>`**（总位数），各绑定类型大小（按位相加）须与该位数一致。多视图时仅允许**标量**类型；单绑定可为结构体 / 数组等聚合类型。链接器中的对象符号名为**第一个**绑定名，其余名字通过偏移访问同一块存储。

```olang
let x<i32> @data<32>(Expr);                    // .data
let y<i64> @bss<64>();                        // .bss（无初始化式）
let c<i32> @rodata<32>(Expr);                 // .rodata（常量初始化）
let z<i32> @section("name")<32>(Expr);         // 自定义段
let a<i32> b<i32> @data<64>(Expr);             // 两个 i32 视图，共用 64 位（示例）
```

#### 赋值
```olang
Expr = Expr;           // 简单赋值
Expr.Ident = Expr;     // 字段
Expr[Expr] = Expr;     // 数组元素
```

#### `deref`（ABI 辅助）
```olang
deref name as T;   // 仅编译期：把 `name` 标成间接绑定（栈槽仍存 `ptr`；类型检查上的逻辑类型为 `T`）。`T` 须为 1、2、4 或 8 字节。
                   // 读取 `name` 时生成 load；`name = rhs` 时经该指针生成 store。
```

#### 控制流
```olang
if (Expr) Block [else Block]
while (Expr) Block
break;
continue;
return Expr;
```

---

### 顶层定义

#### 函数

返回类型写在函数名之前（与 C 类似）。参数为 `名字: 类型`。无返回写 `void`。

```olang
// 内部函数（仅本编译单元）
Type name(params) { body }

// 导出：带函数体的声明
extern Type name(params) { body }

// 仅声明（由其它目标文件或汇编实现）
extern Type name(params);
```


#### 参数
```
Params ::= Param (, Param)*
Param  ::= Ident: Type
```

**限制**：最多 8 个寄存器参数；更多参数通过栈传递。

---

### 限制

- 标量 `@stack` / 带初值的全局 `let` 必须初始化（`@bss` 等按段规则）
- 无复合赋值 (`+=`, `-=`)
- 无自增/自减 (`++`, `--`)
- 数组不检查越界

---

[返回文档](../README_zh.md)
