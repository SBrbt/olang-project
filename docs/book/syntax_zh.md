# 语法参考

**[English](syntax.md)** | **中文**

---

### 词法

#### 注释
```olang
// 单行注释
```

#### 标识符
`[A-Za-z_][A-Za-z0-9_]*`

#### 关键字
`extern fn let if else while break continue return type struct array cast load store addr ptrbind deref bool u8 i32 u32 i64 u64 ptr void true false`

#### 字面量

| 类型 | 格式 | 示例 |
|------|------|------|
| 整数 | 十进制 | `42` |
| | 十六进制 | `0xFF` |
| | 二进制 | `0b1010` |
| | 八进制 | `0o77` |
| | 后缀 | `42i32`, `42u64` |
| 布尔 | `true`, `false` |
| 字符 | `'a'`, `'\n'` (u8) |
| 字符串 | `"hello\n"` (ptr) |

---

### 类型

```
Type ::= void | bool | u8 | i32 | u32 | i64 | u64 | ptr | Ident
```

#### 结构体
```olang
type Ident = struct { FieldList };
type Point = struct { x: i32, y: i32 };
```

#### 数组
```olang
type Ident = array<ElementType, Size>;
type Int5 = array<i32, 5>;
```

> **聚合类型**指数组和结构体。与标量（`i32`、`bool`等原始类型）不同，聚合类型允许先声明、延后初始化。

---

### 表达式

#### 优先级（高→低）

1. `addr`, `cast<T>`, `load<T>`, `[]`, `.`, `()` — 调用
2. `!`, `-` — 一元
3. `*`, `/`, `%`
4. `+`, `-`
5. `<<`, `>>`
6. `<`, `>`, `<=`, `>=`
7. `==`, `!=`
8. `&`
9. `^`
10. `|`
11. `&&`
12. `||`

#### 类型操作

```olang
cast<T>(expr)        // 类型转换
load<T>(ptr)         // 从指针读取
store<T>(ptr, val)   // 向指针写入
addr expr            // 取地址
```

---

### 语句

#### 变量绑定
```olang
let Ident: Type = Expr;  // 标量（原始类型）必须初始化
let Ident: Type;         // 聚合类型（数组/结构体）可延后初始化
```

#### 赋值
```olang
Expr = Expr;           // 简单赋值
Expr.Ident = Expr;     // 字段
Expr[Expr] = Expr;     // 数组元素
```

#### 控制流
```olang
if (Expr) Block [else Block]
while (Expr) Block
break;
continue;
return Expr;
```

---

### 顶层定义

#### 函数
```olang
// 内部函数
fn name(params) -> Type { body }

// 导出函数
extern fn name(params) -> Type { body }

// 外部声明
extern fn name(params) -> Type;
```

#### 参数
```
Params ::= Param (, Param)*
Param  ::= Ident: Type
```

**限制**：最多 8 个寄存器参数；更多参数通过栈传递。

#### 全局变量
```olang
let Ident: Type = Expr;
```

---

### 限制

- 标量 `let` 必须初始化
- 无复合赋值 (`+=`, `-=`)
- 无自增/自减 (`++`, `--`)
- 数组不检查越界
- 无浮点数

---

[返回文档](../README_zh.md)
