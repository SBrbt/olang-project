# Compiler module boundaries

**English** | **[中文](compiler-modules_zh.md)**

---

This note complements [compiler.md](compiler.md): where responsibilities split, what each layer consumes and produces, and sensible places to extend or refactor.

## Data flow

```
lexer.c  → tokens
parser.c → OlProgram + AST (OlFuncDef, OlStmt, OlExpr, …)
sema.c   → type-checked program; same AST shapes + symbol/layout metadata
ir.c     → OlProgram container helpers (typedefs, globals, string table)
codegen  → OobjObject (.oobj) via backend vtable
```

## `lexer.c`

- **Input**: source bytes / `FILE*`
- **Output**: `OlToken` stream (`ol_lexer_next`)
- **Owns**: numeric literal parsing bases (hex/binary/octal), identifier rules, punctuation.

Keep lexer free of type rules except where a single token class needs disambiguation.

## `parser.c`

- **Input**: tokens
- **Output**: populated `OlProgram` (functions, globals, externs, typedefs) and nested AST for bodies.
- **Owns**: grammar, error messages for syntax, construction of `OlExpr` / `OlStmt` trees.

Parser should **not** assign stack slots, global addresses, or emit code. It may attach raw names and literal types as parsed.

## `sema.c`

- **Input**: `OlProgram` from parsing
- **Output**: semantically valid program or diagnostics; fills in types, checks operations, resolves names to bindings.
- **Owns**: scopes, `SymSlot` / indirect bindings, storage layout for locals and globals (structs, allocators), call arity, cast legality.

Semantic analysis is the right place for **type compatibility** and **binding rules**. It should not emit machine instructions; it informs the backend via the decorated program and consistent layout conventions documented in code comments.

## `ir.c` / `ir.h`

- **Owns**: `OlProgram` lifetime (`ol_program_init` / `ol_program_free`), typedef and global registration helpers used by parser and sema.

This is a thin program container, not a separate SSA/lowering IR.

## `codegen_x64.c` (+ `x64_backend.c`, `reloc/x64_reloc.c`)

- **Input**: type-checked `OlProgram`, `OlTargetInfo`
- **Output**: `OobjObject` bytes (sections, symbols, relocations).
- **Owns**: frame layout (`FRAME_SLOTS`, slot → `rbp` offsets), lowering of statements/expressions to x86-64, ABI register/stack argument passing, aggregate copies, reloc records.

The backend assumes sema left **consistent** layout and type info; codegen focuses on instruction selection and object encoding.

## Refactoring heuristics

When `parser.c` or `codegen_x64.c` grows further:

1. **Parser**: split by declaration vs statement vs expression parsing into separate compilation units *only if* you keep a single shared `ParseCtx` API to avoid duplicated error handling.
2. **Codegen**: extract per-construct helpers (e.g. calls, aggregates, control flow) into `codegen_x64_*.c` static modules included from one place, or separate files with shared `CG` typedef in a small internal header—avoid circular includes with the AST.

## See also

- [architecture.md](architecture.md) — toolchain picture and ABI summary
- [specs/olang-syntax.md](specs/olang-syntax.md) — surface syntax reference
