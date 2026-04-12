# 链接器（alinker）

**[English](linker.md)** | **中文**

---

位置：`alinker/`

### 作用

`alinker` 按 **链接脚本**（JSON）合并多个 `.oobj` 并写出结果文件。实现上 **不假定** 某种可执行格式、某个操作系统或 ABI：只做段与符号合并、重定位、按 `load_groups` 顺序拼接负载字节，再执行脚本里的 **`layout`**（在指定文件偏移写字节块/整数/负载）。文件开头是 ELF 头还是别的布局，由脚本里的 `write_blob` 等决定，**不是**在链接器里写死「ELF 知识」。

可选的 **入口桩**（`call_stub_hex`）从脚本读取；若首字节为 `0xE8`，链接器只负责把 **+1 处的 rel32** 补成对 `"entry"` 所指符号的调用，**不**内置其它桩语义。

### 结构

```
alinker/
└── src/
    ├── main.c          # 命令行
    └── link_core.c/h   # 合并、重定位、layout 求值、写出
```

### 链接流程

```
输入：.oobj + link.json
     ↓
1. 加载并合并对象（段、符号、重定位）
     ↓
2. 按脚本算出的段基址 / 虚拟地址应用重定位
     ↓
3. 拼连续负载（脚本中的桩 + 按 load_groups 排的段数据）
     ↓
4. 执行 layout（write_blob / write_u32_le / write_u64_le / write_payload）
     ↓
输出：由脚本描述的文件
```

### 链接脚本

脚本驱动 **全部** 行为：`entry`、`segments`、`load_groups`、`prepend_call_stub`、`call_stub_hex`、`layout` 等。实现见 `common/link_script.c`，示例见 `examples/linux_x86_64/link/`（Linux ELF）与 `examples/bare_x86_64/link/`（扁平裸机映像）。

- **`"entry"`** — 入口符号名；用于解析入口并在存在 `call_stub_hex` 时修补开头的 `call`。
- **`call_stub_hex`** — 当 **`prepend_call_stub`** 为 true 时 **必须** 提供：在合并段数据前追加的原始字节（alinker **不再**内置默认桩）。

### API

```c
int alinker_link_oobj_only(const LinkScript *script, const char *output_path, int verbose, char *err, size_t err_len);
```

### 重定位（`.oobj` 内）

合并器处理 `OOBJ` 重定位类型（`ABS64`、`PC32`、`PC64`）；文档里可能沿用 x86 风格命名，但链接器对「在偏移处补 N 位」是 **与具体 ISA 解耦** 的。

---

[返回](README_zh.md)
