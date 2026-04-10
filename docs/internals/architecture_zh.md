# 架构概览

**[English](architecture.md)** | **中文**

---

```
┌─────────────────────────────────────────┐
│              olang 编译器                │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  │
│  │ 前端    │->│  IR     │->│ 后端     │  │
│  │ lexer   │  │         │  │ x64     │  │
│  │ parser  │  │         │  │ codegen │  │
│  │ sema    │  │         │  │         │  │
│  └─────────┘  └─────────┘  └─────────┘  │
└─────────────────────────────────────────┘
                    ↓ .oobj
┌─────────────────────────────────────────┐
│              kasm 汇编器                 │
│           (JSON-ISA 驱动)                │
└─────────────────────────────────────────┘
                    ↓ .oobj
┌─────────────────────────────────────────┐
│            alinker 链接器                │
│         (link.json 脚本驱动)             │
└─────────────────────────────────────────┘
                    ↓ ELF
```

### 编译流程

```
hello.ol ──→ olang ──→ hello.oobj ──┐
                                     ├──→ alinker ──→ hello.elf
lib.ol ────→ olang ──→ lib.oobj ────┘
```

### 组件

#### olang

**前端** (`olang/src/frontend/`)
- `lexer.c` — 词法分析，字面量解析
- `parser.c` — 递归下降语法分析
- `sema.c` — 类型检查、作用域分析

**后端** (`olang/src/backend/`)
- `codegen_x64.c` — x86_64 代码生成
- 栈布局：地址向下增长
- 聚合类型：负偏移逐字拷贝

#### kasm

基于 `kasm/isa/x86_64_linux.json` 的汇编器：
- 两遍汇编
- 支持标签、立即数、寄存器

#### alinker

链接脚本驱动 (`link.json`)：
- 段布局
- 符号解析
- 重定位处理

### 对象格式 (.oobj)

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

### 代码生成细节

#### 函数调用 (System V AMD64)

参数寄存器：rdi, rsi, rdx, rcx, r8, r9  
返回值：rax  
限制：最多 6 个参数

#### 聚合类型拷贝

```asm
; 源地址 rcx，目标地址 rbx
; 逐 8 字节拷贝，负偏移（栈向下增长）
mov rax, [rcx]      ; 偏移 0
mov [rbx], rax
mov rax, [rcx-8]    ; 偏移 -8
mov [rbx-8], rax
; ...
```

---

[返回](README_zh.md)
