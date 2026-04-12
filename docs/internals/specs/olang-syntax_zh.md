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
  - 全局 `let`：按 **`LetBindings`**（每个名字前可有重复的 `let`，见下），再写**全局**分配器 `@data<…>`、`@bss<…>`、`@rodata<…>`、`@section("段名")<…>`（**不能** `@stack`）
  - `type` 类型定义（`struct` / `array`）
- 至少有一个带函数体的函数。
- 带 **`extern` 的函数定义**导出符号；无 `extern` 的顶层函数仅本编译单元可见。ELF 入口由**链接脚本**决定，不由编译器指定。

### 词法

- **空白**：空格、制表、换行
- **注释**：`//` 至行尾
- **标识符**：`[A-Za-z_][A-Za-z0-9_]*`（最长 63 字符）
- **关键字**：
  - 控制与声明：`extern`、`let`、`if`、`else`、`while`、`break`、`continue`、`return`、`type`、`struct`、`array`
  - 运算：`cast`、`find`、`load`、`store`、`addr`、`sizeof`
  - 类型与字面量：`void`、`bool`、`ptr`、`true`、`false`、`i8` … `b64`（同用户文档）
  - **保留**：`as`（词法有关键字，解析器尚无对应语法）
- **字面量**：整型 / 浮点 / 布尔 / 字符 / 字符串（同 [syntax_zh](../../book/syntax_zh.md)）

### 类型

```
Type ::= void | bool | ptr | i8 | … | b64 | Ident
```

### 表达式（Expr）

在 `parse_expr` 中：`cast<T>(expr)` 的 `expr` 为完整表达式；另有 `sizeof<T>`、`load<Ident>`、`addr Ident`、**`find < Expr >`**（与 `RefExpr` 中形式相同；操作数须为 `ptr` 右值）、字面量、变量、调用、字段、下标、一元/二元运算符（见 `parser.c`）。

### 引用表达式（RefExpr）

用于 `let LetBindings` 的第二段（`@…` 或单个 `RefExpr`），以及 `cast<T>(…)` **在 RefExpr 位置**时的内层（内层须为 RefExpr，非任意 Expr）：

```
LetNameTy ::= Ident < Type >
            | Ident < >

LetBindings ::= LetNameTy ( let LetNameTy )*

RefExpr ::= ( RefExpr )
          | find < Expr >
          | addr Ident
          | @ stack | data | bss | rodata | section ( StrLit ) < BitWidth > ( Expr? )
          | < [Type] > RefExpr
          | cast < Type > ( RefExpr )
          | Ident

BitWidth ::= IntLit | sizeof < Type >
```

另：内建类型关键字（**非** `void`）与 **`<` `RefExpr` `>`** 的 **`T<…>`** 形式由 `parse_ref_expr` 一并解析（见 `is_T_angle_conv_starter`），不单独展开为一行，以免重复罗列关键字。

- 函数体内分配器仅 `@stack`；文件顶层无 `@stack`。
- `<void>` 表示不附加静态类型（内层须为 `ptr` 视图）。
- **`Ident`**：在语义上须解析为**引用**类存储（支持 **`let a<T> let b<U> rhsName`** 等形式）。
- **`find < Expr >`**：`Expr` 须为 **`ptr`** 类型的右值；结果为**编译期对内存对象的引用**（实现上照常对 `Expr` 求值）。**`find<>`** 非法；**不得**嵌套 **`find<find<…>>`**。在 `find<…>` 内若要用含顶层 **`>`** 的比较，须给比较式加圆括号（如 **`find<(a > b)>`**），避免与 `find` 的结束 **`>`** 混淆。

### 语句

```
Stmt ::= Block
       | let LetBindings ( @stack < BitWidth > ( Expr? ) | RefExpr ) ;
       | store < LValue , Expr > ;
       | if ( Expr ) Stmt [ else Stmt ]
       | while ( Expr ) Stmt
       | break ;
       | continue ;
       | return Expr? ;
       | Expr ;

LValue ::= Ident | Expr . Ident | Expr [ Expr ]
```

- **无** **`Expr = Expr`** 赋值语句；赋值通过 **`store<…>`**（见引用模型）。
- **`let`**：使用上面的 **`LetBindings`** —— **每多一个** `Ident<Type>` 都要再写 **`let`**（旧写法 **`let x<T> y<U>`** 无中间 **`let`** 已拒绝）。随后要么是 **`@stack<…>(…)`**，要么是单个 **`RefExpr`**。无 `from` 关键字。链式结合顺序与嵌套重视图见 [olang-refmodel_zh.md](olang-refmodel_zh.md) 中「链式 `let`」。
- **`store<…>`**：尖括号内为左值、逗号、右值表达式，再闭合 `>`；右值解析为避免与 `>` 冲突，使用较浅的表达式层（见 `parse_shift`）。

### 顶层定义（续）

```
AllocatorG ::= @data < BitWidth > ( Expr? )
             | @bss < BitWidth > ( )
             | @rodata < BitWidth > ( Expr )
             | @section ( StrLit ) < BitWidth > ( Expr? )

TopLevel ::= … | let LetBindings AllocatorG ;
```

### 限制

- 最多 8 个寄存器参数，其余走栈。
- 局部 / 全局 `let` 多视图、标量/聚合、初始化规则见 [syntax_zh](../../book/syntax_zh.md)。
- 无复合赋值；无 `++`/`--`。

### 值与存储

- `load`、`addr`、字面量为**值**。具名存储由 `let` + 分配器引入。
- 实现可为子表达式分配栈槽，非语言保证。

---

[返回](../README_zh.md)
