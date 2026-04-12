# OLang 语法规格

**[English](olang-syntax.md)** | **中文**

---

> 用户文档：[docs/book/syntax_zh.md](../../book/syntax_zh.md)

本文描述 `lexer.c` / `parser.c` 实际接受的写法。

### 源文件与顶层

- 一个 `.ol` 文件即一个编译单元；**无** import / 模块
- 顶层元素：
  - `extern fn` 声明或定义
  - `let` 全局变量（可选 `@data` / `@bss` / `@rodata` / `@section("name")` 属性）
  - `type` 类型定义
  - `fn` 内部函数定义
- 至少有一个带体的函数
- 入口：名为 `main` 的导出函数，或最后一个带体函数

### 词法

- **空白**：空格、制表、换行
- **注释**：`//` 至行尾
- **标识符**：`[A-Za-z_][A-Za-z0-9_]*`（最长 63 字符）
- **关键字**（在适用处亦作类型名保留）：
  - 控制与声明：`extern`、`fn`、`let`、`if`、`else`、`while`、`break`、`continue`、`return`、`type`、`struct`、`array`
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

（`addr Ident` 依次解析局部名、全局 `let`、`extern` 或 `fn` 符号。）

### 语句

```
Stmt ::= Expr ;
       | let Ident : Type [= Expr] ;
       | Expr = Expr ;
       | { [Stmt]* }
       | if ( Expr ) Stmt [else Stmt]
       | while ( Expr ) Stmt
       | break ;
       | continue ;
       | return [Expr] ;
       | deref Ident as Type ;
```

### 顶层定义

```
TopLevel ::= extern fn Ident ( [ParamList] ) [-> Type] [ ; | Block ]
           | fn Ident ( [ParamList] ) [-> Type] Block
           | type Ident = struct { [Field[,]]* } ;
           | type Ident = array < Type , Int > ;
           | [ @Attr ]* let Ident : Type [= Expr] ;

ParamList ::= Param (, Param)*
Param     ::= Ident : Type
Field     ::= Ident : Type
```

### 限制

- 最多 8 个寄存器参数；更多参数通过栈传递
- 标量 `let` 必须初始化
- 无复合赋值运算符

---

[返回](../README_zh.md)
