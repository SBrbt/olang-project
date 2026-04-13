# Olang 引用模型（以引用为中心的表面语法）

**[English](olang-refmodel.md)** | **中文**

本文描述 `olang` 编译器面向的**以引用为中心**的表面语言。完整文法见 [olang-syntax_zh.md](olang-syntax_zh.md)。

## 核心概念

### `let`、右侧与 `<Type>`

- **引用**可以带**元素类型**，也可以是**无类型**（元素类型为 **`void`** 的引用：仅有存储/视图，尚未绑定静态元素类型；分配器、`find` 等常见于此）。**`let` 名字**做的是**把符号绑定到右侧的那条引用**；**`let` 关键字本身不带类型**。**`let` 后是名字**（**`LetBindings`**），**再往后是引用一侧**（文法名 **`RefExpr`**，产生式见 [olang-syntax_zh.md](olang-syntax_zh.md)）。**`RefExpr`** 只是文法分类，**不等于**「表达式的类型一定是引用」（例如 **`addr[…]`** 的类型是 **`ptr`**）。
- **链式名字**（**`let n1 let n2 … let nK` `RefExpr`**）：每个 **`let nk`** 引入的名字**本身都表示一条引用**。在语义展开里，**靠左**的名字绑定到从**靠右**叠出来的**视图**（通常当 **`RefExpr`** 是 **`Ident`** 时，最右边的 **`nk`** 紧挨该名字）；详见下文 *链式 `let`*。**靠右**一侧给出的那条引用，供更左侧的 **`let`** 继续用。
- **`<Type> 内层`**（**`Type`** 含 **`void`**）：**`内层`** 须为**引用**（有类型或无类型的 **`ref`**），**不能**是 **`ptr`**。在同一块存储上引入新引用（**`<void>`** 时给出与 **`ptr`** 兼容的视图）。不要写 **`let n <T> addr[x]`**，应写 **`let n <T> x`**。见 **`sema.c`** 中 **`OL_EX_REF_BIND`**。

- **符号**（整条 **`let`** 处理完之后）按上面规则把每个引入的**名字**绑定到**引用**。
- **`ptr`** 只表示地址，不携带元素类型。
- **`find[Expr]`** 要求 **`Expr`** 求值为 **`ptr` 右值**；结果为**编译期对某块内存的引用**（一般 lowering 会像普通子表达式一样对 `Expr` 求值）。**禁止**嵌套 **`find`**。错误使用地址/对象是 **UB**。
- 读写使用 **`load[expr]`** / **`store[左值, 表达式];`**。不支持 **`表达式 = 表达式`**。
- **值**转换只用 **`T(表达式)`**（圆括号内是完整表达式），**没有** `cast` 关键字；**仅**在 **`iN`/`uN`/`bN`/`fN`** 之间（各族内变宽/截断及整数↔浮点），**不**与 **`ptr`**、**`bool`** 做值转换。与 **`<Type> RefExpr`**（新建引用并绑定类型）不要混为一谈。
- **`addr [ RefExpr ]`** 是 **`ptr` 右值**（文法上的 **`RefExpr`** 一种），不分配新存储；需要 **`ptr` 值**时用（例如 **`stack[64, addr[x]]`**）。**`<T> …`** **不**以 **`addr[…]`** 为内层（**`T`** 含 **`void`** 时亦然）。
- **存储放置：** 函数内使用 **`stack`**，形式为 **`[`** 位宽 **`,`** 可选初始化式 **`]`**。文件作用域使用 **`data` / `rodata` / `bss` / `section`**；其中 `data` / `rodata` / `section` 为 **`[`** 位宽 **`,`** 可选初始化式 **`]`**，`bss` 仅 **`[`** 位宽 **`]`**。分配表达式是**无类型引用**；**`let`** 的元素类型来自初始化式，或在函数内对无类型块后续加 `<T>` 视图。细则见 [olang-syntax_zh.md](olang-syntax_zh.md)。
- **`sizeof[Type]`** 为**编译期**整数（**`Type`** 的位宽）。
- **`addr [ … ]`** 取方括号内引用所指存储的地址（**`ptr`**）。同址换视图应写 **`let n <T> x`**（**`x`** 为**引用**），而不是 **`let n <T> addr[x]`**。
- **安全**：语言只提供运算，**不**保证别名或布局；误用视图可能 **UB**。

## EBNF 概要（与当前 `parser.c` 对齐）

完整产生式与说明以 [olang-syntax_zh.md](olang-syntax_zh.md) 为准；此处为摘要。

- **`LetBindings`** → `Ident`（`let` `Ident`）* —— 绑定处**不写**类型。

- **局部 `let`** → `let` `LetBindings` `RefExpr` `;` —— 右侧为 **`RefExpr`**（见上文与文法稿）；文件顶层用 **`AllocatorG`**，不用这一形式里的 **`RefExpr`**。

- **全局 `let`** → `let` `LetBindings` `AllocatorG` `;` —— **`AllocatorG`** 仅为 **`data` / `rodata` / `bss` / `section`**（见 [olang-syntax_zh.md](olang-syntax_zh.md)）。

```
LetBindings ::= Ident ( let Ident )*

RefExpr ::= ( RefExpr )
          | find [ Expr ]
          | addr [ RefExpr ]
          | stack [ … ]
          | < Type > RefExpr
          | Ident

BitWidth ::= IntLit | sizeof [ Type ]
```

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

- **`Expr`**（片段）：**`sizeof[Type]`**、**`load[expr]`**、**`find[expr]`**、**`addr [ RefExpr ]`**、内建 **`T(表达式)`** 值转换、字面量、调用、字段、下标、一元/二元运算 —— 见 `parser.c` 的 `parse_expr` 与 [olang-syntax_zh.md](olang-syntax_zh.md)。

### 链式 `let`（绑定顺序）

对 **`let b1 let b2 … let bN` `RefExpr`**，解析器按**从左到右**记录绑定。当前语义检查会先把右侧解析为一条引用，再把每个绑定名都作为该引用的间接别名绑定，并共享同一元素类型。对于放置分配器（`stack[…]` / 全局分配器），应保持一次放置只绑定一个名字；额外别名/视图通过后续 `let` 完成。

### 全局 `let` 约束（当前 sema）

- 全局 `let` 的右侧必须是 `AllocatorG`（`data` / `rodata` / `bss` / `section`），不能是 `stack`。
- 绑定数量必须在 1..`OL_MAX_LET_BINDINGS`，且同一条全局 `let` 内名字必须唯一。
- 多名字全局视图只支持标量，不支持聚合多视图拆分。
- 若全局放置保持无类型（`void` 元素），只允许一个名字。
- 所有绑定视图位宽之和必须等于分配器声明位宽。
- `bss[...]` 不能带初始化式。
- `rodata[...]` 初始化式必须是编译期常量表达式。

## 说明

- **`T(表达式)`** —— 内建类型关键字加圆括号，括号内按完整 **`expr`** 解析（**`parse_expr`**），只做**值**转换；**没有**「`T(RefExpr)`」这种表面形式。
- **`<Type> 内层`** —— **`内层`** 必须是**引用**；**`Type`** 可为 **`void`**。与 **`T(表达式)`** 不是一回事。
- **`(` `expr` `)`** 为普通分组，不是转换。

---

[返回](../README_zh.md)
