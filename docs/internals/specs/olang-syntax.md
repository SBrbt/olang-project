# OLang Syntax Spec

**English** | **[中文](olang-syntax_zh.md)**

---

> User documentation: [docs/book/syntax.md](../../book/syntax.md)

This document describes the syntax accepted by `lexer.c` / `parser.c`.

### Source File and Top-level

- One `.ol` file is one compilation unit; **no** import / module
- Top-level elements:
  - `extern fn` declaration or definition
  - `let` global variables (optional `@data` / `@bss` / `@rodata` / `@section("name")` attributes)
  - `type` type definitions
  - `fn` internal function definitions
- At least one function with body required
- Entry: exported function named `main`, or last function with body

### Lexical

- **Whitespace**: space, tab, newline
- **Comment**: `//` to end of line
- **Identifier**: `[A-Za-z_][A-Za-z0-9_]*` (max 63 characters)
- **Keywords** (also reserved as type names where applicable):
  - Control / decl: `extern`, `fn`, `let`, `if`, `else`, `while`, `break`, `continue`, `return`, `type`, `struct`, `array`
  - Ops: `cast`, `reinterpret`, `load`, `store`, `addr`, `deref`, `as`
  - Types / literals: `void`, `bool`, `ptr`, `true`, `false`, `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`, `f16`, `f32`, `f64`, `b8`, `b16`, `b32`, `b64`
- **Integer literal**: decimal, hex(`0x`), binary(`0b`), octal(`0o`), optional suffix `i8`/`i16`/`i32`/`i64`/`u8`/`u16`/`u32`/`u64`/`b8`/`b16`/`b32`/`b64`
- **Float literal**: decimal with optional `.` fraction and/or `e`/`E` exponent; optional suffix `f16`/`f32`/`f64` (default type `f64` if unsuffixed)
- **Boolean**: `true` / `false`
- **Character**: `'x'`, supports `\\` `\'` `\n` `\r` `\t` `\0`
- **String**: `"..."`, same escapes

### Types

```
Type ::= void | bool | ptr | i8 | i16 | i32 | i64 | u8 | u16 | u32 | u64
       | f16 | f32 | f64 | b8 | b16 | b32 | b64 | Ident
```

### Expressions

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

(`addr Ident` resolves a local name, then a global `let`, then an `extern` or `fn` symbol.)

### Statements

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

### Top-level Definitions

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

### Limitations

- Up to 8 register parameters; additional parameters passed via stack
- Scalar `let` must be initialized
- No compound assignment operators

---

[Return](../README.md)
