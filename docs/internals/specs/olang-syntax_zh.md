# OLang 语法规格

**[English](olang-syntax.md)** | **中文**

---

> 用户文档：[docs/book/syntax_zh.md](../../book/syntax_zh.md)

本文描述 `lexer.c` / `parser.c` 实际接受的写法。

### 源文件与顶层

- 一个 `.ol` 文件即一个编译单元；**无** import / 模块
- 顶层元素：
  - `extern fn` 声明或定义
  - `let` 全局变量（可选属性）
  - `type` 类型定义
  - `fn` 内部函数定义
- 至少有一个带体的函数
- 入口：名为 `main` 的导出函数，或最后一个带体函数

### 词法

- **空白**：空格、制表、换行
- **注释**：`//` 至行尾
- **标识符**：`[A-Za-z_][A-Za-z0-9_]*`
- **关键字**：`extern fn let if else while break continue return type struct array cast load store addr ptrbind deref bool u8 i32 u32 i64 u64 ptr void true false`
- **整数字面量**：十进制、十六进制(`0x`)、二进制(`0b`)、八进制(`0o`)，可选后缀 `i32`/`i64`/`u8`/`u32`/`u64`
- **布尔**：`true` / `false`
- **字符**：`'x'`，支持 `\\` `\'` `\n` `\r` `\t` `\0`
- **字符串**：`"..."`，相同转义

### 类型

```
Type ::= void | bool | u8 | i32 | u32 | i64 | u64 | ptr | Ident
```

### 表达式

```
Expr ::= Literal | Ident | Expr ( [Expr[,]]* ) | Expr . Ident | Expr [ Expr ]
       | - Expr | ! Expr
       | Expr + Expr | Expr - Expr | Expr * Expr | Expr / Expr | Expr % Expr
       | Expr == Expr | Expr != Expr | Expr < Expr | Expr > Expr | Expr <= Expr | Expr >= Expr
       | Expr && Expr | Expr || Expr
       | Expr & Expr | Expr | Expr | Expr ^ Expr | Expr << Expr | Expr >> Expr
       | cast<Type>(Expr) | load<Type>(Expr) | store<Type>(Expr, Expr) | addr Expr
```

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

- 最多 6 个函数参数
- 标量 `let` 必须初始化
- 无复合赋值运算符

---

[返回](../README_zh.md)
