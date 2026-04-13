# 编译器模块边界

**[English](compiler-modules.md)** | **中文**

---

本文与 [compiler_zh.md](compiler_zh.md) 配合使用：说明各层职责、输入输出，以及扩展或拆分时的合理位置。

## 数据流

```
lexer.c  → 词法单元
parser.c → OlProgram 与 AST（OlFuncDef、OlStmt、OlExpr 等）
sema.c   → 通过类型检查的同一套 AST，外加符号与布局信息
ir.c     → OlProgram 容器（typedef、全局量、字符串表等）
codegen  → 经后端 vtable 生成 OobjObject（.oobj）
```

## `lexer.c`

- **输入**：源码
- **输出**：`OlToken` 流（`ol_lexer_next`）
- **职责**：各进制数字字面量、标识符、标点。

除必要时区分词法类别外，lexer 不应实现类型规则。

## `parser.c`

- **输入**：词法单元
- **输出**：填充后的 `OlProgram`（函数、全局、extern、typedef）及函数体 AST。
- **职责**：文法、语法错误信息、`OlExpr` / `OlStmt` 树的构造。

解析器**不应**分配栈槽、全局地址或生成指令；可只记录原始名字与按文法解析出的类型信息。

## `sema.c`

- **输入**：解析得到的 `OlProgram`
- **输出**：语义合法则通过，否则报错；补全类型、检查运算、解析名字到绑定。
- **职责**：作用域、`SymSlot` / 间接绑定、局部与全局存储布局（聚合体、分配器）、调用参数个数、转换合法性。

**类型相容性**与**绑定规则**属于语义分析；此处不应生成机器码，而是通过一致的布局约定把信息交给后端（见源码注释）。

## `ir.c` / `ir.h`

- **职责**：`OlProgram` 生命周期（`ol_program_init` / `ol_program_free`）、供 parser/sema 使用的 typedef 与全局注册辅助函数。

这是轻量程序容器，不是单独的 SSA 或更低层 IR。

## `codegen_x64.c`（及 `x64_backend.c`、`reloc/x64_reloc.c`）

- **输入**：通过语义检查的 `OlProgram`、`OlTargetInfo`
- **输出**：`OobjObject`（段、符号、重定位）。
- **职责**：栈帧（`FRAME_SLOTS`、槽到 `rbp` 偏移）、语句/表达式到 x86-64 的 lowering、ABI 寄存器与栈传参、聚合拷贝、重定位记录。

后端假定 sema 已给出**一致**的布局与类型信息；codegen 专注指令选择与对象编码。

## 拆分时的经验原则

当 `parser.c` 或 `codegen_x64.c` 继续变大时：

1. **Parser**：若按「声明 / 语句 / 表达式」拆到多个 `.c` 文件，应保留统一的 `ParseCtx` 接口，避免重复的错误处理路径。
2. **Codegen**：可按构造（调用、聚合、控制流）抽到 `codegen_x64_*.c` 或独立文件，并用小型内部头文件共享 `CG` 结构——注意避免与 AST 形成循环包含。

## 另见

- [architecture_zh.md](architecture_zh.md) — 工具链与 ABI 概览
- [specs/olang-syntax_zh.md](specs/olang-syntax_zh.md) — 表面语法
