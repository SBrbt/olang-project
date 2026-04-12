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
- **运算：** `cast`、`find`、`load`、`store`、`addr`、`sizeof`
- **类型与字面量：** `void`、`bool`、`ptr`、`true`、`false`、`i8`、`i16`、`i32`、`i64`、`u8`、`u16`、`u32`、`u64`、`f16`、`f32`、`f64`、`b8`、`b16`、`b32`、`b64`
- **保留但未用作语句：** `as`（词法保留，当前解析器不接受 `as` 语法）

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

1. `addr`、`cast<T>`、`find<…>`、`sizeof<…>`、`<[T]>`、`load<…>`、`[]`、`.`、`()` — 调用
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

#### 表达式与「引用表达式」

**一般表达式**里可用的前缀/原子形式包括：

```olang
cast<T>(expr)           // 显式转换；括号内为完整表达式（见 [types_zh](types_zh.md)）
sizeof<T>               // 编译期常量：类型 `T` 的存储宽度（位），表达式类型为 `u64`
load<name>              // 读取名为 `name` 的绑定（`name` 为标识符，见语义）
addr Ident              // 取址：依次匹配局部名、全局 `let`、`extern` 或函数符号（右值 `ptr`）
find<Expr>              // `Expr` 须为 `ptr` 右值；与下节 `RefExpr` 中形式相同（见引用模型）
```

**引用表达式 `RefExpr`**（用于 `let name<T> …` 的第二段、`cast<T>(` 的内层等——内层不是任意表达式，而是下面这一条链）：

```olang
( RefExpr )             // 分组
find<Expr>              // `Expr` 须为 `ptr` 右值；结果为编译期对内存对象的引用（见 [引用模型](../internals/specs/olang-refmodel_zh.md)）
addr Ident
@stack|@data|@bss|@rodata|@section("…") <Int|sizeof<T>> ( [Expr] )   // 分配存储；函数内仅 `@stack`
<[T]> RefExpr           // 为内层引用附加静态类型；不附加类型时写 `<void>`（仍为 `ptr` 视图）
cast<T>(RefExpr)        // 与上面不同：`cast` 在 RefExpr 位置时括号内必须是 RefExpr，而非任意 `expr`
```

**`store` 是语句，不是普通表达式里的 `store<…>(…)`。** 形式为：

```olang
store<lvalue, expr>;    // lvalue：单个名字、字段 `a.b`、下标 `a[i]`；逗号后为写入的右值，再闭合 `>`
```

具名存储只能通过带**分配器**的 `let` 建立（见下）。字面量以及 `load`、`addr` 的求值结果是**值**（通常作右值）。若某名字通过 `let … <T>find<…>` / `let … <T>addr …` 形成**间接绑定**，则对该名字的读写由实现按「槽内为指针 + 逻辑元素类型」处理（**没有**名为 `deref` 的关键字语句）。当前后端仍可能为子表达式分配临时栈槽，属实现细节。

---

### 语句

#### 变量绑定（具名存储）

函数体内只能使用 **`@stack`**。每个绑定形如 `名字<类型>`；多个绑定可共享同一块栈上存储，**每多一个绑定都要再写关键字 `let`**（例如 `let x<f32> let n<i32> @stack<64>(...)`）。`@stack<位数>` 为**总位数**（可写整数或 `sizeof<Type>`），各绑定类型大小（按位相加）必须与之相等，按声明顺序紧密排列。`let` 与分配器之间**无** `from` 关键字。

```olang
let x<i32> @stack<32>(Expr);                    // 标量：必须有初始化式；位数与类型一致
let x<f32> let n<i32> @stack<64>(Expr);             // 例如 f32 与 i32 两种视图，共用 64 位
let s<MyStruct> @stack<N>();                  // 聚合：可有可空无括号；N = 8 * sizeof(MyStruct)
```

文件顶层使用**全局分配器**（不能用 `@stack`）。写法与函数体内类似：可声明**一个或多个** `Ident<Type>` 共享同一块静态存储；`@data` / `@bss` / `@rodata` / `@section("段名")` 后为 **`<位数>`**（总位数或 `sizeof<...>`），各绑定类型大小（按位相加）须与该位数一致。多视图时仅允许**标量**类型；单绑定可为结构体 / 数组等聚合类型。链接器中的对象符号名为**第一个**绑定名，其余名字通过偏移访问同一块存储。

```olang
let x<i32> @data<32>(Expr);                    // .data
let y<i64> @bss<64>();                        // .bss（无初始化式）
let c<i32> @rodata<32>(Expr);                 // .rodata（常量初始化）
let z<i32> @section("name")<32>(Expr);         // 自定义段
let gx<f32> let gn<i32> @data<64>(Expr);           // 不同类型标量，共用 64 位（示例）
```

#### 写入（无 `=` 赋值语句）

语言**没有** `Expr = Expr` 形式的赋值，写入使用 **`store`**：

```olang
store<左值, 表达式>;
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
