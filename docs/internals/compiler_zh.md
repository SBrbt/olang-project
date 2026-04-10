# 编译器实现

**[English](compiler.md)** | **中文**

---

位置：`olang/src/`

### 结构

```
olang/src/
├── main.c              # 入口
├── ir.c/h              # 中间表示
├── frontend/
│   ├── lexer.c/h       # 词法分析
│   ├── parser.c/h      # 语法分析
│   ├── sema.c/h        # 语义分析
│   └── ast.h           # AST 定义
└── backend/
    ├── x64_backend.c/h # x64 后端入口
    ├── codegen_x64.c/h # 代码生成
    ├── target_info/    # 目标信息
    └── reloc/          # 重定位
```

### 前端

#### 词法分析 (lexer.c)

输入：字符流  
输出：token 流

关键函数：
```c
void ol_lexer_next(OlLexer *L);  // 获取下一个 token
```

支持的数字格式：
- 十进制：`42`
- 十六进制：`0xFF`
- 二进制：`0b1010`
- 八进制：`0o77`

整数范围：
- `u64`: 完整 64 位范围 `0..UINT64_MAX`
- 其他: 对应 C 类型范围

#### 语法分析 (parser.c)

递归下降解析器，生成 AST。

#### 语义分析 (sema.c)

- 类型检查
- 作用域管理
- 变量绑定验证

### 后端

#### IR (ir.c)

中间表示生成：
- 类型大小计算
- 指令生成

#### 代码生成 (codegen_x64.c)

关键函数：

```c
// 栈操作
void emit_mov_rbp_r64(CG *g, uint32_t slot, uint8_t reg);
void emit_mov_r64_rbp(CG *g, uint8_t reg, uint32_t slot);

// 加载/存储
bool emit_load_at_rax(CG *g, uint32_t sz);

// 函数调用
void emit_call(CG *g, const char *name);
```

### 栈布局

```
高地址
┌─────────────┐
│ 返回地址     │
├─────────────┤
│ 保存的 rbp   │ ← rbp
├─────────────┤
│ slot 0      │ ← rbp (局部变量)
├─────────────┤
│ slot 1      │ ← rbp - 8
├─────────────┤
│ ...         │
├─────────────┤
│ slot N      │ ← rbp - N*8
└─────────────┘
低地址
```

### 聚合类型拷贝

对于大小 > 8 字节的结构体/数组：

```c
// 负偏移逐字拷贝（栈向下增长）
for (w = 0; w < num_words; w++) {
    int32_t neg_offset = -(w * 8);
    // 从源加载 [rcx + neg_offset]
    // 向目标存储 [rbx + neg_offset]
}
```

### 调试

添加调试输出：
```c
fprintf(stderr, "DEBUG: slot=%u, kind=%s\n", slot, ol_expr_kind_name(e->kind));
```

---

[返回](README_zh.md)
