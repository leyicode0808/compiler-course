


# Compiler Course Project

本项目是编译原理课程设计，实现了一个面向 C-- 语言子集的编译器。项目按照课程实践要求逐步完成词法分析、语法分析、语义分析、中间代码生成、MIPS 目标代码生成以及基础优化，并能够将测试程序生成 MIPS32 汇编后交给 SPIM 运行。


## 1. 实验环境

开发环境：

- Ubuntu 22.04.5 LTS
- GCC 11.4.0
- Flex 2.6.4
- Bison 3.8.2
- GNU Make 4.3
- Python 3.10.12
- SPIM 8.0

安装依赖示例：

```bash
sudo apt install gcc flex bison make spim
```


## 2. 项目结构

```text
.
├── Makefile
├── src
│   ├── lexer.l        # 词法分析器
│   ├── parser.y       # 语法分析器与 AST 构建
│   ├── ast.c/h        # 抽象语法树结构与打印
│   ├── semantic.c/h   # 语义分析
│   ├── ir.c/h         # 中间代码生成
│   ├── mips.c/h       # MIPS 目标代码生成
│   └── optimize.c/h   # 基础优化
├── tests              # 测试用例
├── tools              # 调试工具
├── backups            # 阶段性备份
└── day1-flex          # 第一阶段 Flex 实验文件
```


## 3. 实践内容对应关系

| 课程实践 | 要求内容 | 本项目完成情况 |
|---|---|---|
| 实践 1 | 安装实验环境，分析 C-- 语言，完成词法和语法分析 | 已完成 Flex 词法分析、Bison 语法分析，并构建 AST |
| 实践 2 | 语义分析 | 已完成变量、函数、类型、参数、数组下标等基础语义检查 |
| 实践 3 | 中间代码生成 | 已完成变量声明、赋值、表达式、数组、函数调用、条件和循环的中间代码生成 |
| 实践 4 | 将中间代码翻译为 MIPS32，并在 SPIM 上运行 | 已完成基础 MIPS 目标代码生成，并通过 SPIM 测试 |
| 实践 5 | 中间代码优化 | 已完成整数常量折叠优化 |


## 4. 已实现功能

### 4.1 词法分析

词法分析器由 `src/lexer.l` 实现，能够识别：

- 关键字：`int`、`float`、`return`、`if`、`else`、`while`
- 标识符
- 整数和浮点数
- 算术运算符：`+ - * /`
- 关系运算符：`> < >= <= == !=`
- 逻辑运算符：`&& || !`
- 赋值符号：`=`
- 分隔符：`; , ( ) [ ] { }`
- 单行注释和块注释
- 非法八进制、非法十六进制、未知字符、未闭合注释等词法错误


### 4.2 语法分析与 AST

语法分析器由 `src/parser.y` 实现，支持：

- 函数定义
- 参数列表和实参列表
- 局部变量声明
- 一维数组声明和访问
- 赋值语句
- 表达式语句
- `return`
- `if`
- `if-else`
- `while`
- 算术表达式
- 关系表达式
- 逻辑表达式
- 函数调用

语法分析成功后会构建并打印 AST。


### 4.3 语义分析

语义分析由 `src/semantic.c` 实现，当前支持：

- 变量重复定义检查
- 未定义变量检查
- 函数重复定义检查
- 未定义函数检查
- 函数参数个数检查
- 函数参数类型检查
- 赋值类型检查
- 返回值类型检查
- 数组下标类型检查


### 4.4 中间代码生成

中间代码生成由 `src/ir.c` 实现，支持：

- `FUNCTION`
- `DEC`
- `RETURN`
- `ARG`
- `CALL`
- 临时变量
- 标签和跳转
- 算术表达式
- 关系条件
- 逻辑短路条件
- `if / if-else / while`
- 一维数组读写

示例：

```text
FUNCTION main :
DEC a
a := #3
IF a > #0 GOTO label1
GOTO label2
LABEL label1 :
t1 := a - #1
a := t1
LABEL label2 :
RETURN a
```


### 4.5 MIPS 目标代码生成

MIPS 生成由 `src/mips.c` 实现，当前支持：

- 整数常量和变量
- 加减乘除
- 赋值
- `return`
- `if / if-else / while`
- 逻辑条件跳转
- 一维数组读写
- 简单函数调用
- `$a0-$a3` 参数传递
- `$v0` 返回值
- `jal` 和 `jr $ra`
- SPIM 运行

示例测试结果：

```text
test_ir_basic.cmm           -> 3
test_ir_control.cmm         -> 0
test_array.cmm              -> 1
test_function_params.cmm    -> 3
test_logic_expr.cmm         -> 1
test_opt_const.cmm          -> 7
```


### 4.6 常量折叠优化

优化模块由 `src/optimize.c` 实现，目前支持整数常量表达式折叠。

示例源代码：

```c
int main() {
    int a;
    a = 1 + 2 * 3;
    return a;
}
```

优化输出：

```text
Optimization: constant folded 2 * 3 -> 6
Optimization: constant folded 1 + 6 -> 7
```

优化后的中间代码：

```text
FUNCTION main :
DEC a
a := #7
RETURN a
```


## 5. 编译与运行

编译：

```bash
make clean
make parser
```

运行单个测试：

```bash
./build/parser tests/test_ir_basic.cmm
```

运行语法、语义、中间代码和 MIPS 生成测试：

```bash
make test
```

运行 MIPS + SPIM 测试：

```bash
make mips-test
```


## 6. 主要测试用例

| 测试文件 | 作用 |
|---|---|
| `test_lexer.cmm` | 词法分析合法输入测试 |
| `test_lexer_error.cmm` | 词法错误测试 |
| `test_minimal.cmm` | 最小函数测试 |
| `test_minimal_error.cmm` | 语法错误测试 |
| `test_decl_return.cmm` | 声明和返回语句测试 |
| `test_assign_expr.cmm` | 赋值和算术表达式测试 |
| `test_control.cmm` | 条件和循环语句测试 |
| `test_array.cmm` | 一维数组声明、赋值和读取测试 |
| `test_function_params.cmm` | 函数参数和函数调用测试 |
| `test_logic_expr.cmm` | 逻辑表达式和短路条件测试 |
| `test_ast_full.cmm` | 综合语法、语义、IR、MIPS 测试 |
| `test_opt_const.cmm` | 常量折叠优化测试 |
| `test_semantic_*.cmm` | 语义错误测试 |


## 7. 主要提交记录

| 提交 | 内容 |
|---|---|
| `60c86b0` | 完成初始 C-- 词法分析器 |
| `98dbf8a` | 改进词法错误处理 |
| `664027a` | 添加最小 Bison 语法分析器 |
| `eb1f227` | 支持声明和 return 表达式 |
| `cdb16af` | 支持赋值和算术表达式 |
| `de41959` | 支持控制流语法 |
| `f86bdda` | 支持逻辑表达式 |
| `675f538` | 支持数组声明和数组访问 |
| `de3fd88` | 支持多函数和简单函数调用 |
| `e8a3bc3` | 支持函数参数和实参 |
| `f29ecb6` | 支持浮点字面量 |
| `1965fe8` | 为 AST 构建准备 parser |
| `3404c9a` | 添加语义分析框架 |
| `12f9c04` | 检查基础类型错误 |
| `105d5fc` | 检查函数参数和数组下标类型 |
| `85cbba1` | 生成基础中间代码 |
| `7e333eb` | 支持函数调用和数组的中间代码 |
| `a2d0452` | 生成控制流中间代码 |
| `d3f73c0` | 生成基础 MIPS 代码 |
| `77d0052` | 生成控制流 MIPS 代码 |
| `c613d35` | 生成数组 MIPS 代码 |
| `2f9cd1a` | 生成函数调用 MIPS 代码 |
| `ea47051` | 生成逻辑条件 MIPS 代码 |
| `193efef` | 添加常量折叠优化 |


## 8. 当前限制

本项目已经打通了完整编译流程，但仍有一些限制：

- 主要支持 C-- 的基础子集，并非完整 C--。
- MIPS 目标代码目前主要支持 `int`，`float` 的 MIPS 运行支持不完整。
- 尚未实现 `struct` 和成员访问。
- 函数调用采用简化调用约定，当前主要支持前 4 个参数。
- 暂未完整支持递归函数和复杂栈帧管理。
- 当前 MIPS 生成主要基于 AST 直接生成，同时保留 IR 输出展示；后续可改造成严格从 IR 链表生成 MIPS。
- Project 5 目前实现了常量折叠，尚未实现公共子表达式消除、死代码删除、寄存器优化等高级优化。


## 9. 总结

本项目从零开始实现了一个基础版 C-- 编译器，完成了从源程序到 MIPS32 汇编并在 SPIM 上运行的完整链路。项目按照课程实践逐步推进，涵盖词法分析、语法分析、AST 构建、语义分析、中间代码生成、目标代码生成和基础优化，能够用于展示编译器各阶段的主要工作流程。

