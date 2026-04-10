# ELF 输出格式

**[English](elf-layout.md)** | **中文**

---

### 概述

alinker 输出标准 ELF64 可执行文件（x86_64 Linux）。

### 布局

```
┌─────────────────────┐ 0x0
│ ELF Header          │
├─────────────────────┤ 0x40
│ Program Header      │
│ (PT_LOAD × 2)       │
├─────────────────────┤
│ .text 段 (RX)       │ 0x400000
│   - 代码            │
├─────────────────────┤
│ .data/.bss 段 (RW)  │
│   - 全局变量        │
├─────────────────────┤
│ Section Headers     │ (可选)
└─────────────────────┘
```

### 段配置

典型 `link.json` 使用双段：

```json
{
  "segments": [
    {
      "name": ".text",
      "flags": "RX",
      "vaddr": 4194304
    },
    {
      "name": ".data",
      "flags": "RW",
      "vaddr": 4202496
    }
  ]
}
```

| 段 | 标志 | 对齐 | 说明 |
|----|------|------|------|
| .text | RX | 0x1000 | 代码，只读 |
| .data | RW | 0x1000 | 已初始化数据 |
| .bss | RW | - | 未初始化数据（不占文件空间） |

### 入口点

默认入口符号：`_start` 或第一个导出函数。

`link.json` 指定：
```json
{
  "entry": "_start"
}
```

### 运行时

链接时包含：
- `krt.kasm.oobj` — 运行时启动代码
- `olrt.kasm.oobj` — OLang 运行时
- `libposix.kasm.oobj` — POSIX 系统调用包装

运行时启动流程：
1. 设置栈
2. 调用 `main`
3. 以 `main` 返回值调用 `exit`

---

[返回](../README_zh.md)
