# 链接器实现

**[English](linker.md)** | **中文**

---

位置：`alinker/`

### 结构

```
alinker/
└── src/
    ├── main.c          # 入口
    └── link_core.c/h   # 链接核心
```

### 链接流程

```
输入: .oobj 文件 + link.json
     ↓
1. 加载所有对象文件
     ↓
2. 合并段、解析符号
     ↓
3. 处理重定位
     ↓
4. 执行链接脚本 ops
     ↓
输出: ELF 或原始二进制
```

### 链接脚本

`link.json` 格式：

```json
{
  "format": "elf",
  "entry": "_start",
  "segments": [
    { "name": ".text", "flags": "RX", "vaddr": 4194304 }
  ],
  "ops": [
    { "op": "write_blob", "section": ".text", "data": "..." },
    { "op": "write_u64_le", "value": "..." }
  ]
}
```

### 关键函数

```c
// 链接主函数
int link_perform(LinkContext *ctx);

// 处理重定位
int link_apply_relocs(LinkContext *ctx, OlObjFile *obj);
```

### 支持的 ops

| op | 说明 |
|----|------|
| `write_blob` | 写入字节块 |
| `write_u32_le` | 写入 32 位小端 |
| `write_u64_le` | 写入 64 位小端 |
| `write_payload` | 写入段内容 |

### 重定位类型

| 类型 | 说明 |
|------|------|
| `R_X86_64_64` | 绝对 64 位 |
| `R_X86_64_PC32` | PC 相对 32 位 |
| `R_X86_64_32` | 绝对 32 位 |

---

[返回](README_zh.md)
