# OLang 语法规格

**[English](olang-syntax.md)** | **中文**

---

> 用户文档：[docs/book/syntax_zh.md](../../book/syntax_zh.md)

本文描述 `lexer.c` / `parser.c` 实际接受的写法。

### 源文件与顶层

- 一个 `.ol` 文件即一个编译单元；**无** import / 模块
- 顶层元素：
  - `extern Type Ident ( ParamList ) ;` 或 `extern Type Ident ( ParamList ) Block`（声明或导出定义）
  - `Type Ident ( ParamList ) Block`（内部函数，返回类型 `Type` 在前）
  - `let` 全局变量：一个或多个 `名字 < 类型 >`，再写**分配器** `@data<位数>`、`@bss<位数>`、`@rodata<位数>`、`@section("段名")<位数>`（位数为整数字面量，表示存储总位数；不能用 `@stack`）
  - `type` 类型定义
- 至少有一个带体的函数
- 带 **`extern` 的函数定义**会导出符号；顶层函数**没有** `extern` 则仅在当前编译单元内可见（`.oobj` 不导出）。ELF 进程入口符号只由**链接脚本**的 `"entry"` 决定（例如 [`examples/linux_x86_64/link/linux_elf_exe.json`](../../examples/linux_x86_64/link/linux_elf_exe.json)），不由编译器指定

### 词法

- **空白**：空格、制表、换行
- **注释**：`//` 至行尾
- **标识符**：`[A-Za-z_][A-Za-z0-9_]*`（最长 63 字符）
- **关键字**（在适用处亦作类型名保留）：
  - 控制与声明：`extern`、`let`、`if`、`else`、`while`、`break`、`continue`、`return`、`type`、`struct`、`array`
  - 运算：`cast`、`reinterpret`、`load`、`store`、`addr`、`deref`、`as`
  - 类型与字面量：`void`、`bool`、`ptr`、`true`、`false`、`i8`、`i16`、`i32`、`i64`、`u8`、`u16`、`u32`、`u64`、`f16`、`f32`、`f64`、`b8`、`b16`、`b32`、`b64`
- **整数字面量**：十进制、十六进制(`0x`)、二进制(`0b`)、八进制(`0o`)，可选后缀 `i8`/`i16`/`i32`/`i64`/`u8`/`u16`/`u32`/`u64`/`b8`/`b16`/`b32`/`b64`
- **浮点字面量**：十进制，可含 `.` 小数与 `e`/`E` 指数；可选后缀 `f16`/`f32`/`f64`（无后缀时默认为 `f64`）
- **布尔**：`true` / `false`
- **字符**：`'x'`，支持 `\\` `\'` `\n` `\r` `\t` `\0`
- **字符串**：`"..."`，相同转义

### 类型

```
Type ::= void | bool | ptr | i8 | i16 | i32 | i64 | u8 | u16 | u32 | u64
       | f16 | f32 | f64 | b8 | b16 | b32 | b64 | Ident
```

### 表达式

```
Expr ::= Literal | Ident | Expr ( [Expr[,]]* ) | Expr . Ident | Expr [ Expr ]
       | - Expr | ! Expr
       | Expr + Expr | Expr - Expr | Expr * Expr | Expr / Expr | Expr % Expr
       | Expr == Expr | Expr != Expr | Expr < Expr | Expr > Expr | Expr <= Expr | Expr >= Expr
       | Expr && Expr | Expr || Expr
       | Expr & Expr | Expr | Expr | Expr ^ Expr | Expr << Expr | Expr >> Expr
       | cast<Type>(Expr) | reinterpret<Type>(Expr)
       | load<Type>(Expr) | store<Type>(Expr, Expr) | addr Ident
```

（`addr Ident` 依次解析局部名、全局 `let`、`extern` 声明或函数符号。）

### 语句

```
Stmt ::= Expr ;
       | let ( Ident < Type > )+ @stack < IntLit > ( [Expr] ) ;
       | Expr = Expr ;
       | { [Stmt]* }
       | if ( Expr ) Stmt [else Stmt]
       | while ( Expr ) Stmt
       | break ;
       | continue ;
       | return [Expr] ;
       | deref Ident as Type ;
```

（`IntLit` 为整数字面量，表示 `@stack` / 全局分配器后的**总位数**。）

### 顶层定义

```
AllocatorG ::= @data < IntLit > ( [Expr] )
             | @bss < IntLit > ( )
             | @rodata < IntLit > ( Expr )
             | @section ( StrLit ) < IntLit > ( [Expr] )

TopLevel ::= extern Type Ident ( [ParamList] ) ;
           | extern Type Ident ( [ParamList] ) Block
           | Type Ident ( [ParamList] ) Block
           | type Ident = struct { [Field[,]]* } ;
           | type Ident = array < Type , Int > ;
           | let ( Ident < Type > )+ AllocatorG ;

ParamList ::= Param (, Param)*
Param     ::= Ident : Type
Field     ::= Ident : Type
```

### 限制

- 最多 8 个寄存器参数；更多参数通过栈传递
- 局部 `let`：`名字 < 类型 >` 可重复，然后是 `@stack<位数>(...)`（位数为整数字面量，表示总位数）。标量必须有初始化式；聚合可写空 `()`。多视图时仅标量。全局 `let` 与之类似：`(名字 < 类型 >)+` 加 `@data|@bss|@rodata|@section(...)<位数>(...)`；多视图时仅标量；单绑定可为聚合。链接符号名为第一个绑定名。
- 无复合赋值运算符

### 值与存储

- `load`、`addr`、字面量在表达式中是**值**（通常作右值）。具名存储由 `let … @stack` 或全局分配器引入。
- 当前 `codegen_x64.c` 仍可能为子表达式分配栈槽，属实现细节，不是语言保证。

---

[返回](../README_zh.md)
