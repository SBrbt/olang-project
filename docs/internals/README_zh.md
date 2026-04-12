# 内部文档

**[English](README.md)** | **中文**

---

面向贡献者的实现文档。

### 快速导航

| 主题 | 文档 |
|------|------|
| 架构概览 | [architecture_zh.md](architecture_zh.md) |
| 编译器实现 | [compiler_zh.md](compiler_zh.md) |
| 编译器模块边界 | [compiler-modules_zh.md](compiler-modules_zh.md) |
| 汇编器实现 | [assembler_zh.md](assembler_zh.md) |
| 链接器实现 | [linker_zh.md](linker_zh.md) |
| 对象格式 | [specs/oobj_zh.md](specs/oobj_zh.md) |
| 示例 ELF 布局（linux_x86_64） | [specs/elf-layout_zh.md](specs/elf-layout_zh.md) |
| 语法规格 | [specs/olang-syntax_zh.md](specs/olang-syntax_zh.md) |
| 预处理器 | [preproc_zh.md](preproc_zh.md) |

### 源码地图

```
olang/
├── src/frontend/
│   ├── lexer.c       # 词法分析
│   ├── parser.c      # 语法分析
│   └── sema.c        # 语义分析
└── src/backend/
    ├── x64_backend.c   # x86_64 后端入口
    ├── codegen_x64.c   # 代码生成
    └── reloc/          # 重定位

kasm/
├── src/
│   ├── kasm_asm.c    # 汇编器
│   └── kasm_isa.c    # ISA 解析
└── isa/
    └── x86_64.json  # 指令集定义（仅体系结构）

alinker/
└── src/
    ├── link_core.c     # 链接核心
    └── main.c          # CLI

preproc/
└── src/
    └── main.c          # olprep（仅 #include）
```

### 开发流程

1. **修改代码** → `olang/src/`, `kasm/src/`, etc.
2. **构建** → `make all`
3. **测试** → `make check`
4. **验证示例** → `bash tests/olang/run_programs_olc.sh`

---

**快速链接**: [文档中心](../README_zh.md) | [项目主页](../../README_zh.md)
