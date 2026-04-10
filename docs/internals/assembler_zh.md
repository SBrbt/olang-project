# 汇编器实现

**[English](assembler.md)** | **中文**

---

位置：`kasm/`

### 结构

```
kasm/
├── src/
│   ├── main.c          # 入口
│   ├── kasm_asm.c/h    # 汇编核心
│   └── kasm_isa.c/h    # ISA 解析
└── isa/
    └── x86_64_linux.json  # 指令集定义
```

### ISA 定义

`kasm/isa/x86_64_linux.json` 定义：
- 指令编码格式
- 操作数类型
- 寄存器编码

示例条目：
```json
{
  "mnemonic": "mov",
  "operands": ["r64", "r64"],
  "encoding": ["0x48", "0x89", "/r"]
}
```

### 汇编流程

两遍汇编：

1. **第一遍** — 收集符号（标签地址）
2. **第二遍** — 生成代码（解析指令、应用重定位）

### 关键函数

```c
// 汇编主函数
int kasm_assemble(KasmState *S, const char *input, size_t len);

// ISA 加载
int isa_load(ISA *isa, const char *path);
```

### 输入格式

```kasm
; 注释
label:
    mov rax, 42
    jmp label
```

### 输出

`.oobj` 对象文件：
- 代码段
- 符号表
- 重定位表

---

[返回](README_zh.md)
