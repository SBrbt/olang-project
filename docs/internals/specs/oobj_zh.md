# 对象文件格式 (.oobj)

**[English](oobj.md)** | **中文**

---

### 概述

.oobj 是 OLang 工具链的中间对象格式，用于 olang/kasm 输出和 alinker 输入。

### 文件结构

```
┌──────────────────┐
│  头部 (Header)    │
├──────────────────┤
│  段表 (Sections)  │
├──────────────────┤
│  符号表 (Symbols) │
├──────────────────┤
│  重定位表(Relocs) │
├──────────────────┤
│  段数据 (Data)    │
└──────────────────┘
```

### C 结构定义

```c
typedef struct OlObjHeader {
    char magic[4];        // "OOBJ"
    uint32_t version;
    uint32_t num_sections;
    uint32_t num_symbols;
    uint32_t num_relocs;
} OlObjHeader;

typedef struct OlObjSection {
    char name[32];
    uint32_t flags;       // SHF_*
    uint32_t data_len;
    uint32_t data_offset; // 相对于文件头
} OlObjSection;

typedef struct OlObjSymbol {
    char name[64];
    uint64_t value;
    uint32_t section;     // 所在段索引，0=未定义
    uint32_t flags;       // STB_*, STT_*
} OlObjSymbol;

typedef struct OlObjReloc {
    uint64_t offset;      // 重定位位置（段内偏移）
    uint32_t section;     // 目标段
    uint32_t symbol;      // 符号索引
    uint32_t type;        // R_X86_64_*
    int64_t addend;
} OlObjReloc;
```

### 段标志 (flags)

| 标志 | 值 | 说明 |
|------|-----|------|
| `SHF_WRITE` | 0x1 | 可写 |
| `SHF_ALLOC` | 0x2 | 加载到内存 |
| `SHF_EXEC` | 0x4 | 可执行 |

### 符号标志 (flags)

| 标志 | 值 | 说明 |
|------|-----|------|
| `STB_LOCAL` | 0x0 | 局部符号 |
| `STB_GLOBAL` | 0x1 | 全局符号 |
| `STB_WEAK` | 0x2 | 弱符号 |
| `STT_FUNC` | 0x10 | 函数 |
| `STT_OBJECT` | 0x20 | 数据对象 |

### 重定位类型

| 类型 | 值 | 说明 |
|------|-----|------|
| `R_X86_64_64` | 1 | 绝对 64 位 |
| `R_X86_64_PC32` | 2 | PC 相对 32 位 |
| `R_X86_64_32` | 10 | 绝对 32 位 |

### 工具

`obinutils` 提供：
- `oobj-dump` — 显示对象文件内容
- `oobj-nm` — 列出符号表
- `oobj-diff` — 比较两个对象文件

---

[返回](../README_zh.md)
