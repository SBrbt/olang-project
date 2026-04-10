# Architecture

**English** | **[中文](architecture_zh.md)**

---

```
┌─────────────────────────────────────────┐
│              olang compiler             │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  │
│  │ Frontend│->│   IR    │->│ Backend │  │
│  │ lexer   │  │         │  │  x64    │  │
│  │ parser  │  │         │  │ codegen │  │
│  │ sema    │  │         │  │         │  │
│  └─────────┘  └─────────┘  └─────────┘  │
└─────────────────────────────────────────┘
                    | .oobj
                    ↓
┌─────────────────────────────────────────┐
│            alinker linker               │        ┌─────────────────────────┐
│         (link.json script driven)       │<-------│    kasm assembler       │
│                                         │  .oobj │   (JSON-ISA driven)     │
└─────────────────────────────────────────┘        └─────────────────────────┘
                    ↓
                   ELF
```

### Compilation Flow

```
hello.ol ──> olang ──> hello.oobj ──┐
                                    ├──> alinker ──> hello.elf
lib.ol ────> olang ──> lib.oobj ────┘
```

### Components

#### olang

**Frontend** (`olang/src/frontend/`)
- `lexer.c` — Lexical analysis, literal parsing
- `parser.c` — Recursive descent parser
- `sema.c` — Type checking, scope analysis

**Backend** (`olang/src/backend/`)
- `codegen_x64.c` — x86_64 code generation
- Stack layout: addresses grow downward
- Aggregate types: word-by-word copy with negative offsets

#### kasm

JSON-ISA based assembler (`kasm/isa/x86_64_linux.json`):
- Two-pass assembly
- Supports labels, immediates, registers

#### alinker

Link script driven (`link.json`):
- Segment layout
- Symbol resolution
- Relocation handling

### Object Format (.oobj)

```c
typedef struct {
    char name[32];
    uint32_t flags;
    uint32_t data_len;
    uint8_t *data;
} OlObjSection;

typedef struct {
    char name[64];
    uint64_t value;
    uint32_t section;
    uint32_t flags;
} OlObjSymbol;
```

### Code Generation Details

#### Function Calls (OLang ABI)

OLang uses its own calling convention, decoupled from SysV AMD64.

Register arguments: r12, r13, r14, r15, rdi, rsi, r8, r9 (up to 8)  
Stack overflow: arguments 9+ passed on the stack (right-to-left push)  
Return value: rax  

The kasm POSIX shim layer (`libposix.kasm`) converts OLang ABI registers
to Linux syscall registers (rdi, rsi, rdx, r10, r8, r9).

#### Aggregate Type Copy

```asm
; Source in rcx, target in rbx
; Copy 8 bytes at a time, negative offsets (stack grows down)
mov rax, [rcx]      ; offset 0
mov [rbx], rax
mov rax, [rcx-8]    ; offset -8
mov [rbx-8], rax
; ...
```

---

[Return](README.md)
