# Olang 引用模型（以引用为中心的表面语法）

**[English](olang-refmodel.md)** | **中文**

本文描述 `olang` 重写所面向的**以引用为中心**的表面语言；**「已移除」**一节中的旧形式解析器**不得**再接受。

## 核心概念

- **符号**把**名字**绑定到一条**引用**（编译期位置 + 可选的静态类型绑定）。
- **`ptr`** 只表示地址类型，不带元素类型。
- **类型绑定** `<[Type]> RefExpr` 产生**新引用**，与操作数引用**独立**；不附加静态类型时写 `<void>`（仍为 ptr 视图）。
- **`find<Expr>`**：**`Expr`** 须求值为 **`ptr` 右值**；结果为**编译期对内存对象的引用**（实现上与其它子表达式一样对 `Expr` 求值）。**禁止嵌套 `find`**。误用地址/对象与其它视图一样可导致 **UB**。
- **读写**用 **`load<name>`** / **`store<name, Expr>;`**，**不使用** **`=`** 赋值。
- **`cast<Type>`**（唯一 cast 形态，如 `cast<Type>(RefExpr)`）**创建新对象**，结果为**带 `Type` 的引用**；旧 **`cast<T>(expr)` 右值转换**已**完全移除**。
- **`addr Ident`** 为 **`ptr` 右值**，不分配存储。
- **`@…<bits>(Expr)`** 分配存储，结果为绑定为 **`ptr`**、指向该块的引用。
- **`sizeof<Type>`** 为**编译期**整数（类型的**位宽**）。
- **`addr Ident`** 得到具名存储的地址（`ptr`）；与同宽 **`let n<T> <T>addr x`**、多绑定 `let` 配合得到同一段字节的另一类型视图。
- **安全**：语言只提供操作；**不保证**别名与布局，误用可导致 **UB**。

## 已移除（须拒绝）

- 旧 **`load<T>(ptr)`**、**`store<T>(…, …)`**
- **`deref … as …;`**
- 旧式 **`reinterpret<T>(expr)`**（文档与语言表面已去除；存储多视图用 `addr` + `<T>` / 多绑定 `let`）
- 旧 **`cast<T>(expr)`** 右值模型（见英文档「Removed」）
- **`Expr = Expr` 赋值**
- **旧式多绑定**：两名字之间**无** **`let`**，例如 **`let x<T> y<U> @stack<…>(…)`** —— 每多一个名字都必须再写关键字 **`let`**（见下 **`LetBindings`**）。

## EBNF 草案（与当前 `parser.c` 对齐）

- **`LetNameTy`** → `Ident` `<` `Type` `>` | `Ident` `<` `>`（后者即 `<>`，无元素类型）。
- **`LetBindings`** → `LetNameTy`（`let` `LetNameTy`）* —— **每个**额外绑定前都要出现 **`let`**，不能用仅靠空格连接的多个 `Ident<Type>`。

- **函数体内 `let`** → `let` `LetBindings`（`@stack` `<` `BitWidth` `>` `(` `Expr`? `)` | `RefExpr`）`;`
- **全局 `let`** → `let` `LetBindings` `AllocatorG` `;`（**仅**全局分配器，见 [olang-syntax_zh.md](olang-syntax_zh.md) 中的 `AllocatorG`；**不是**任意 `RefExpr` 尾）

- **`RefExpr`** → `(` `RefExpr` `)`
          | `find` `<` `Expr` `>`
          | `addr` `Ident`
          | `@` 分配器 …（如 `@stack`、`@data` …）
          | `<` `Type` `>` `RefExpr`（含 `<void>`）
          | `cast` `<` `Type` `>` `(` `RefExpr` `)`
          | `Ident`（须为**引用**类存储；用于 **`let … refName`** 链式右侧）
          | 内建类型关键字 `<` `RefExpr` `>`（`void` 不作为起始；见 `parser.c` 中 `is_T_angle_conv_starter`）

- **`Expr`** → …（含 `find` / `load` / `sizeof` / `cast` 等，见 [olang-syntax_zh.md](olang-syntax_zh.md)）

- **`Stmt`** → … | `store<…>` | 局部/全局 `let` | 控制流 | `Expr` `;` —— **无** **`=`** 赋值

细则与 **`RefExpr`** 完整列表以 [olang-syntax_zh.md](olang-syntax_zh.md) 为准；英文对照见 [olang-refmodel.md](olang-refmodel.md)。

### 链式 `let`（绑定顺序）

对 **`let b1 let b2 … let bN` `RefExpr`**，解析器按**从左到右**记录 **`b1`…`bN`**。当 **`RefExpr`** 为**引用名**（`Ident`）时，语义层把**最靠近右侧**的绑定（**`bN`**）视为紧贴该名字的一层，再向外叠加重视图（见 `sema.c` 中 **`check_let_ref_chain_views`**）。当尾部为 **`@stack`** / 全局分配器时，所有绑定共享同一块分配（多视图布局）。

## 其它
