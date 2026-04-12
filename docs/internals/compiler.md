# Compiler

**English** | **[中文](compiler_zh.md)**

---

Location: `olang/src/`

### Structure

```
olang/src/
├── main.c              # Entry point
├── ir.c/h              # Intermediate representation
├── frontend/
│   ├── lexer.c/h       # Lexical analysis
│   ├── parser.c/h      # Syntax analysis
│   ├── sema.c/h        # Semantic analysis
│   └── ast.h           # AST definitions
└── backend/
    ├── x64_backend.c/h # x64 backend entry
    ├── codegen_x64.c/h # Code generation
    ├── target_info/    # Target info
    └── reloc/          # Relocations
```

Layer responsibilities and refactoring notes: [compiler-modules.md](compiler-modules.md).

### Frontend

#### Lexical Analysis (lexer.c)

Input: character stream  
Output: token stream

Key function:
```c
void ol_lexer_next(OlLexer *L);  // Get next token
```

Supported number formats:
- Decimal: `42`
- Hexadecimal: `0xFF`
- Binary: `0b1010`
- Octal: `0o77`

Integer ranges:
- `u64`: full 64-bit range `0..UINT64_MAX`
- Others: corresponding C type ranges

#### Syntax Analysis (parser.c)

Recursive descent parser, generates AST.

#### Semantic Analysis (sema.c)

- Type checking
- Scope management
- Variable binding validation

### Backend

#### IR (ir.c)

Intermediate representation generation:
- Type size calculation
- Instruction generation

#### Code Generation (codegen_x64.c)

Key functions:

```c
// Stack operations
void emit_mov_rbp_r64(CG *g, uint32_t slot, uint8_t reg);
void emit_mov_r64_rbp(CG *g, uint8_t reg, uint32_t slot);

// Load/Store
bool emit_load_at_rax(CG *g, uint32_t sz);

// Function calls
void emit_call(CG *g, const char *name);
```

### Stack Layout

```
High address
┌─────────────┐
│ Return addr │
├─────────────┤
│ Saved rbp   │ ← rbp
├─────────────┤
│ slot 0      │ ← rbp (local variable)
├─────────────┤
│ slot 1      │ ← rbp - 8
├─────────────┤
│ ...         │
├─────────────┤
│ slot N      │ ← rbp - N*8
└─────────────┘
Low address
```

### Aggregate Type Copy

For structs/arrays larger than 8 bytes:

```c
// Word-by-word copy with negative offsets (stack grows down)
for (w = 0; w < num_words; w++) {
    int32_t neg_offset = -(w * 8);
    // Load from source [rcx + neg_offset]
    // Store to target [rbx + neg_offset]
}
```

### Debugging

Add debug output:
```c
fprintf(stderr, "DEBUG: slot=%u, kind=%s\n", slot, ol_expr_kind_name(e->kind));
```

---

[Return](README.md)
