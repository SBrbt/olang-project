# OLang Syntax Spec

**English** | **[中文](olang-syntax_zh.md)**

---

> User documentation: [docs/book/syntax.md](../../book/syntax.md)

This document describes the syntax accepted by `lexer.c` / `parser.c`.

### Source File and Top-level

- One `.ol` file is one compilation unit; **no** import / module
- Top-level elements:
  - `extern Type Ident ( ParamList ) ;` or `extern Type Ident ( ParamList ) Block` (forward decl or exported definition)
  - `Type Ident ( ParamList ) Block` (internal function; return type first)
  - `let` globals: one or more `Ident < Type >`, then an **allocator** `@data<BITWIDTH>`, `@bss<BITWIDTH>`, `@rodata<BITWIDTH>`, `@section("name")<BITWIDTH>` (`BITWIDTH` is an integer literal, total size in bits; not `@stack`)
  - `type` type definitions
- At least one function with body required
- **`extern` definition** exports the function symbol; a top-level function **without** `extern` is internal to the compilation unit (not exported from the `.oobj`). The ELF process entry symbol is chosen only by the **link script** (`"entry"`, e.g. [`examples/linux_x86_64/link/linux_elf_exe.json`](../../examples/linux_x86_64/link/linux_elf_exe.json)), not by the compiler

### Lexical

- **Whitespace**: space, tab, newline
- **Comment**: `//` to end of line
- **Identifier**: `[A-Za-z_][A-Za-z0-9_]*` (max 63 characters)
- **Keywords** (also reserved as type names where applicable):
  - Control / decl: `extern`, `let`, `if`, `else`, `while`, `break`, `continue`, `return`, `type`, `struct`, `array`
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

(`addr Ident` resolves a local name, then a global `let`, then an `extern` or function symbol.)

### Statements

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

(`IntLit` is an integer literal: the **total bit width** after `@stack` / a global allocator.)

### Top-level Definitions

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

### Limitations

- Up to 8 register parameters; additional parameters passed via stack
- Local `let`: repeat `Ident < Type >`, then `@stack<BITWIDTH>(...)` (`BITWIDTH` integer literal, total bits). Scalars need an initializer; aggregates may use empty `()`. Multiple names: scalars only. Global `let` is analogous: `(Ident < Type >)+` plus `@data|@bss|@rodata|@section(...)<BITWIDTH>(...)`. Multiple names: scalars only; a single binding may be an aggregate. The linker symbol is the **first** binding name.
- No compound assignment operators

### Values vs storage

- `load`, `addr`, and literals act as **rvalues** (expression values). Only `let … @stack` / global allocators introduce **named** storage.
- The current `codegen_x64.c` pipeline may still allocate stack slots for subexpressions; that is an implementation detail, not a language guarantee.

---

[Return](../README.md)
