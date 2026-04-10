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
- `bin/olang` — 编译器
- `bin/kasm` — 汇编器
- `bin/alinker` — 链接器
- `examples/olc` — 驱动脚本

### 验证

```bash
make check
# OK: all checks passed
```

### 第一个程序

创建 `hello.ol`：

```olang
extern fn main() -> i32 {
    return 42;
}
```

编译运行：

```bash
mkdir -p examples/out
bash examples/olc -o examples/out/hello.elf hello.ol
./examples/out/hello.elf && echo $?
# 42
```

### 下一步

- 完整教程：[tutorial_zh.md](tutorial_zh.md)
- 更多示例：[examples_zh.md](examples_zh.md)
- 语法速查：[../book/syntax_zh.md](../book/syntax_zh.md)

### 常见问题

**Q: `make check` 失败？**
检查 GCC 版本：`gcc --version`（需要 4.8+）

**Q: 找不到 `olc`？**
确保在项目根目录，使用：`bash examples/olc`

**Q: "标量需要初始值"？**
```olang
let x: i32 = 0i32;  // ✓
let y: i32;         // ✗ 标量必须初始化
```
