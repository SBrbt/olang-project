# 快速开始

**[English](quickstart.md)** | **中文**

---

5 分钟上手 OLang。

### 系统要求

- Linux x86_64
- GCC (C11)
- Make

### 构建

```bash
make all
```

生成：
- `bin/olprep` — 预处理器（仅 `#include`）
- `bin/olang` — 编译器
- `bin/kasm` — 汇编器
- `bin/alinker` — 链接器
- `examples/linux_x86_64/olc` — 驱动脚本（先跑 `olprep` 再链其余工具；中间文件保留在 `<输出名>.olc.d/`）

### 验证

```bash
make check
# OK: all checks passed
```

### 第一个程序

建一个本地练习目录（已在仓库 `.gitignore` 中忽略），源码和生成的 ELF 放在同一目录下：

```bash
mkdir -p examples/linux_x86_64/programs/hello
```

创建 `examples/linux_x86_64/programs/hello/hello.ol`：

```olang
extern i32 main() {
    return 42;
}
```

在项目根目录编译并运行（输出也在 `hello/` 下）：

```bash
bash examples/linux_x86_64/olc -o examples/linux_x86_64/programs/hello/hello.elf examples/linux_x86_64/programs/hello/hello.ol
./examples/linux_x86_64/programs/hello/hello.elf
echo $?
# 42
```

`make check` 按约定只编译 `examples/linux_x86_64/programs/ex_*.ol`；这里的 `hello/` 目录供你自行试验。

### 下一步

- 完整教程：[tutorial_zh.md](tutorial_zh.md)
- 更多示例：[examples_zh.md](examples_zh.md)
- 语法速查：[../book/syntax_zh.md](../book/syntax_zh.md)

### 常见问题

**Q: `make check` 失败？**
检查 GCC 版本：`gcc --version`（需要 4.8+）

**Q: 找不到 `olc`？**
确保在项目根目录，使用：`bash examples/linux_x86_64/olc`

**Q: "标量需要初始值"？**
```olang
let x stack[32, 0i32];  // ✓
// let y stack[32];     // ✗ 标量栈槽需初始化式，或：let raw stack[32]; let y <i32> raw;
```
