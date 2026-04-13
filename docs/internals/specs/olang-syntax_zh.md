# OLang 语法规格

**[English](olang-syntax.md)** | **中文**

---

> 用户导读：[docs/book/syntax_zh.md](../../book/syntax_zh.md)（面向使用者，与实现同步维护）  
> 本文与 `lexer.c` / `parser.c` **当前**接受的形式对齐。

### 源文件与顶层

- 一个 `.ol` 文件即一个编译单元；**无** import / 模块（预处理器 `#include` 由 `olprep` 处理，见 [preproc_zh.md](../preproc_zh.md)）。
- 顶层元素：
  - `extern Type Ident ( ParamList ) ;` 或 `extern Type Ident ( ParamList ) Block`
  - `Type Ident ( ParamList ) Block`（内部函数）
  - 全局 `let`：按 **`LetBindings`**（每个名字前可有重复的 `let`，见下），再写**全局**放置 — **`data[…]`**、**`rodata[…]`**、**`bss[…]`**、**`section[…]`**（**不能**使用函数体内的 **`stack`**）
  - `type` 类型定义（`struct` / `array`）
- 至少有一个带函数体的函数。
- 带 **`extern` 的函数定义**导出符号；无 `extern` 的顶层函数仅本编译单元可见。ELF 入口由**链接脚本**决定，不由编译器指定。

### 词法

- **空白**：空格、制表、换行
- **注释**：`//` 至行尾
- **标识符**：`[A-Za-z_][A-Za-z0-9_]*`（最长 63 字符）
- **关键字**：
  - 控制与声明：`extern`、`let`、`if`、`else`、`while`、`break`、`continue`、`return`、`type`、`struct`、`array`
  - 运算：`find`、`load`、`store`、`addr`、`sizeof`、`stack`、`data`、`bss`、`rodata`、`section`
  - 类型与字面量：`void`、`bool`、`ptr`、`true`、`false`、`i8` … `b64`（同用户文档）
- **字面量**：整型 / 浮点 / 布尔 / 字符 / 字符串（同 [syntax_zh](../../book/syntax_zh.md)）
- **比较与移位**：`<`、`>`、`<=`、`>=`、`<<`、`>>` 与 C 类似词法分析（无尖括号泛型）。

### 类型

```
Type ::= void | bool | ptr | i8 | … | b64 | Ident
```

### 表达式（Expr）

在 `parse_expr` 中：内建类型关键字 **`T(expr)`** 表示值转换；另有 **`sizeof[Type]`**、**`load[expr]`**、**`addr [ RefExpr ]`**（表达式类型 **`ptr`**）、**`find[expr]`**、字面量、调用、字段、下标、一元/二元运算符（见 `parser.c`）。

### 引用表达式（RefExpr）

用于 `let LetBindings` 的第二段（**`stack` / `data` / …** 或单个 **`RefExpr`**），含 **`<Type> RefExpr`**（新建引用并绑定元素类型）。

实现说明：`parser.c` 通过 **`parse_let_bindings`** 与 **`parse_alloc_expr`** / **`parse_ref_expr`** 组合得到该尾。

```
LetBindings ::= LetBinding ( let LetBinding )*
LetBinding ::= Ident                 // 元素类型由初始化式、或裸放置 + `<T>` 视图推出

RefExpr ::= ( RefExpr )
          | find [ Expr ]
          | addr [ RefExpr ]
          | stack [ BitWidthOptInit ]
          | data [ BitWidthOptInit ]
          | rodata [ BitWidthOptInit ]
          | bss [ BitWidth ]
          | section [ StrLit , BitWidthOptInit ]
          | < Type > RefExpr
          | Ident

BitWidth ::= DecIntLit             // 无后缀十进制正整数，表示位数
BitWidthOptInit ::= BitWidth [ , Expr ]
```

- **`stack` / `data` / `rodata` / `section`** 的分配表达式语义类型为**无类型引用**；**`let`** 侧元素类型由**初始化式**、或**无初始化时的无类型块**再经 **`<T>`** 视图、或（文件顶层标量）**带初值的 `data`/`rodata`/…** 推出。
- **栈 / data / rodata：** **`[`** **位宽** **`,`** 可选 **初始化式** **`]`**；位宽为**无后缀**十进制整数字面量（位数）。
- **`bss`：** 仅 **`[`** **位宽** **`]`**，无第二参数、无初始化式。
- **`section`：** **`section[`** 段名字符串 **`,`** **位宽** **`,`** 可选初始化式 **`]`**。
- **不在**方括号里写「多视图类型表」；多名字见 **`let a let b (x)`**、**`struct`**、**`<T>…`**。
- **`< Type > 内层`**：**新建**引用（**`Type`** 为 **`void`** 时为与 **`ptr`** 兼容的视图）；**不是** **`T(表达式)`** 值转换。**内层**须为**引用**，**不能**是 **`ptr`**（故不能是 **`addr[…]`** 的结果）。**`addr [ … ]`** 文法上是 **`RefExpr`**，类型却是 **`ptr`**。
- 函数体内用 **`stack`** 表示栈上存储；文件顶层用 **`data` / `bss` / `rodata` / `section`**（不用 **`stack`**）。
- **`Ident`**：在语义上须解析为**引用**类存储。
- **`addr [ RefExpr ]`**：文法上属于 **`RefExpr`**，表达式类型为 **`ptr`**。当前**语义**要求方括号内为简单名字（**`OL_EX_VAR`**）；见 **`sema.c`** 中 **`OL_EX_ADDR`**。

### 语句

```
Stmt ::= Block
       | let LetBindings RefExpr ;
       | store [ LValue , Expr ] ;
       | if ( Expr ) Stmt [ else Stmt ]
       | while ( Expr ) Stmt
       | break ;
       | continue ;
       | return Expr? ;
       | Expr ;

LValue ::= Ident | Expr . Ident | Expr [ Expr ]
```

- **无** **`Expr = Expr`** 赋值语句；赋值通过 **`store[左值, 右值];`**。
- **`let`**：**`LetBindings`** 后为 **`RefExpr`** 或 **`stack` / `data` / …**。无 `from` 关键字。

### 顶层定义（续）

```
AllocatorG ::= data [ BitWidthOptInit ]
             | rodata [ BitWidthOptInit ]
             | bss [ BitWidth ]
             | section [ StrLit , BitWidthOptInit ]

TopLevel ::= … | let LetBindings AllocatorG ;
```

### 限制

- 至多 8 个寄存器传参，其余在栈上。
- 链式 `let`（叠在引用名上）、**`stack[…]`**（一次放置一个名）、标量与聚合、初始化规则：见 [syntax_zh](../../book/syntax_zh.md)。
- 无复合赋值；无 `++`/`--`。

### 值与存储

- `load`、`addr`、字面量为**值**；具名存储来自 `let` + **`stack` / `data` / …**。
- 子表达式溢出到临时槽属实现细节。

---

[返回](../README_zh.md)
