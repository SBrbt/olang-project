# OLang

**[English](README.md)** | **中文**

---

从零构建的静态类型系统编程语言。

```bash
make all
bash examples/olc -o examples/out/hello.elf examples/programs/ex_hello.ol
./examples/out/hello.elf
```

**[开始 →](docs/README_zh.md)**

### 一句话

OLang 是编译型语言，直接生成 ELF，无外部依赖。

### 设计取向

OLang **有意**尽可能暴露底层控制能力（内存布局、裸指针、按位重解释等），而不是把机器模型藏起来。编译器会做静态类型与许多规则检查，但**运行层面的安全**——例如避免越界、悬空指针、未定义行为——**由底层开发者自行保证**；语言不承诺替你承担 C 级世界里本应由约定与正确用法承担的责任。

### 文档地图

| 我想... | 去这里 |
|---------|--------|
| 5分钟上手 | [docs/learn/quickstart_zh.md](docs/learn/quickstart_zh.md) |
| 系统学习 | [docs/learn/tutorial_zh.md](docs/learn/tutorial_zh.md) |
| 查语法/特性 | [docs/book/](docs/book/) |
| 看示例 | [docs/learn/examples_zh.md](docs/learn/examples_zh.md) |
| 了解实现 | [docs/internals/](docs/internals/) |
| 参与开发 | [docs/internals/README_zh.md](docs/internals/README_zh.md) |

### 项目状态

Phase 1（基础功能完整）：类型系统、函数、结构体、数组、控制流、多文件编译 — [查看测试](tests/README_zh.md)

### 目录

```
├── docs/          # 所有文档
├── olang/         # 编译器源码
├── preproc/       # 预处理器（olprep）
├── kasm/          # 汇编器
├── alinker/       # 链接器
├── common/        # 共享代码
├── examples/      # 示例程序
├── tests/         # 测试套件
└── bin/           # 编译产物
```

**许可证**: [MIT](LICENSE)
