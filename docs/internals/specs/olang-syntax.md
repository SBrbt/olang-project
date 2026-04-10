# OLang Syntax Spec

**English** | **[中文](olang-syntax_zh.md)**

---

> User documentation: [docs/book/syntax.md](../../book/syntax.md)

This document describes the syntax accepted by `lexer.c` / `parser.c`.

### Source File and Top-level

- One `.ol` file is one compilation unit; **no** import / module
- Top-level elements:
  - `extern fn` declaration or definition
  - `let` global variables (optional attributes)
  - `type` type definitions
  - `fn` internal function definitions
- At least one function with body required
- Entry: exported function named `main`, or last function with body

### Lexical

- **Whitespace**: space, tab, newline
- **Comment**: `//` to end of line
- **Identifier**: `[A-Za-z_][A-Za-z0-9_]*`
- **Keywords**: `extern fn let if else while break continue return type struct array cast load store addr ptrbind deref bool u8 i32 u32 i64 u64 ptr void true false`
- **Integer literal**: decimal, hex(`0x`), binary(`0b`), octal(`0o`), optional suffix `i32`/`i64`/`u8`/`u32`/`u64`
- **Boolean**: `true` / `false`
- **Character**: `'x'`, supports `\\` `\'` `\n` `\r` `\t` `\0`
- **String**: `"..."`, same escapes

### Types

```
Type ::= void | bool | u8 | i32 | u32 | i64 | u64 | ptr | Ident
```

### Expressions

```
Expr ::= Literal | Ident | Expr ( [Expr[,]]* ) | Expr . Ident | Expr [ Expr ]
       | - Expr | ! Expr
       | Expr + Expr | Expr - Expr | Expr * Expr | Expr / Expr | Expr % Expr
       | Expr == Expr | Expr != Expr | Expr < Expr | Expr > Expr | Expr <= Expr | Expr >= Expr
       | Expr && Expr | Expr || Expr
       | Expr & Expr | Expr | Expr | Expr ^ Expr | Expr << Expr | Expr >> Expr
       | cast<Type>(Expr) | load<Type>(Expr) | store<Type>(Expr, Expr) | addr Expr
```

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

- Maximum 6 function parameters
- Scalar `let` must be initialized
- No compound assignment operators

---

[Return](../README.md)
