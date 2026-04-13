# 汇编器（kasm）

**[English](assembler.md)** | **中文**

---

位置：`kasm/`

### 作用

`kasm` 把 `.kasm` 源文件汇编成 `.oobj`（段、符号、重定位）。**不**绑定某个操作系统；由你传入的 ISA JSON（`--isa`）决定 `inst` 有哪些助记符。

### 结构

```
kasm/
├── src/
│   ├── main.c          # 命令行（--in、-o、可选 --isa）
│   ├── kasm_asm.c/h    # 按行汇编
│   └── kasm_isa.c/h    # 将 ISA JSON 读入 `IsaSpec`
└── isa/
    └── x86_64.json     # 示例 ISA：固定前缀操作码 + 可选立即数 + pc32 重定位
```

### ISA JSON（`x86_64.json`）

每条指令对象包含：

| 字段 | 含义 |
|------|------|
| `mnemonic` | `inst` 后面写的名字 |
| `bytes` | 十六进制字符串：先发出的操作码字节 |
| `imm_bits` | `0`、`8`、`16`、`32`、`64`：在 `bytes` **之后**按小端写立即数（非 0 时） |
| `reloc` | 可选 `"pc32"`：要求 `imm_bits` 为 32；操作数为符号名，对应位置做 PC 相对重定位 |

示例：

```json
{ "mnemonic": "mov_rdi_r12", "bytes": "4c89e7", "imm_bits": 0 }
```

ISA 里未收录的编码仍用 `.bytes` 手写。仓库自带的 `x86_64.json` 已包含 `examples/linux_x86_64/asm/runtime/olrt.kasm` 等用到的固定编码（如 `movzx_eax_ax`、`shr_ecx_imm8`、带立即数字节位移的 `je_rel8`、`or_eax_imm32` 等）。

### 汇编方式

当前实现是**单行扫描、一遍向前**处理：标签绑定到**当前段**当前偏移；`inst` / `.bytes` 往该段追加；重定位在生成指令时记录。**没有**单独的「先收集全部标签再第二遍生成」阶段。

### API

```c
int kasm_compile(IsaSpec *isa, const char *input_path, const char *output_path, OobjObject *obj);
```

成功返回 `0`；`main.c` 里再用 `oobj_write_file` 写出 `.oobj`。

### 源语言（子集）

- 行注释：从 `#` 到行尾  
- `.section 名 对齐 flags` — 切换/创建段  
- `.global` / `.extern` — 符号  
- `label:` — 当前位置标签  
- `inst 助记符` 或 `inst 助记符 操作数` — ISA 里定义的指令  
- `.bytes aa bb …` — 原始字节  
- `.equ 名, 值` — 数值常量，供后面 `inst` 操作数替换  

### 默认 ISA 路径

省略 `--isa` 时，`main.c` 会查找 `kasm/isa/x86_64.json`（详见 `--help` 与 `OLANG_TOOLCHAIN_ROOT`）。

### 输出

一个或多个代码段，以及 `.oobj` 中的符号表与重定位表（见 [oobj 规格](specs/oobj_zh.md)）。

---

[返回](README_zh.md)
