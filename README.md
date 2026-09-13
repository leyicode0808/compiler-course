# C-- Compiler Course Project

本项目是一个面向 C-- 语言的编译原理课程设计，实现了从源程序到中间代码、目标代码以及 IR 优化的完整编译流程。项目使用 C 语言开发，前端基于 Flex 和 Bison，后端支持三地址 IR 生成、MIPS32 目标代码生成，并通过 IRSim 和 SPIM 验证运行结果。

## 项目功能

整体编译流程如下：

```text
C-- 源程序
-> 词法分析
-> 语法分析与 AST 构造
-> 语义分析
-> 中间代码 IR 生成
-> MIPS32 目标代码生成
-> IR 优化
```

主要完成内容：

- 使用 Flex 实现 C-- 词法分析，支持关键字、标识符、整数、浮点数、运算符、分隔符和注释识别。
- 使用 Bison 实现 C-- 语法分析，构造 AST，并支持常见语法错误恢复。
- 设计类型系统、符号表和作用域链，完成变量、函数、数组、结构体、表达式和返回语句的语义检查。
- 将 AST 翻译为线性三地址 IR，支持控制流、函数调用、数组/结构体访问和 `read`/`write`。
- 将 IR 翻译为 MIPS32 汇编，支持栈帧管理、参数传递、函数调用、返回值处理和 SPIM 运行。
- 实现 IR 优化器，支持常量传播、常量折叠、复制传播、公共表达式消除和死代码删除。

## 技术栈

- C
- Flex
- Bison
- Makefile
- MIPS32 / SPIM
- IRSim
- Docker

## 目录结构

```text
.
├── src/                         # 编译器核心源码
│   ├── lexer.l                   # Flex 词法规则
│   ├── parser.y                  # Bison 语法规则
│   ├── ast.c / ast.h             # AST 数据结构和打印
│   ├── semantic.c / semantic.h   # 语义分析
│   ├── irgen.c / irgen.h         # IR 生成
│   ├── mipsgen.c / mipsgen.h     # MIPS 生成
│   └── iropt.c / iropt.h         # IR 优化
├── tools/                        # 构建、测试和验证脚本
├── docs/                         # 各实践设计说明
├── report/                       # 汇总报告和实践报告
├── examples/                     # 手写示例程序
├── tests/                        # 测试目录
└── submission/                   # 按课程实践拆分的提交材料
```

`submission/` 下按实践拆分为 `proj1` 到 `proj5`，每个目录都包含对应阶段的源码、测试样例、脚本和报告，便于单独验收。

## 构建环境

需要安装以下工具：

```text
gcc
make
flex
bison
spim
docker
```

检查环境：

```sh
make check-env
```

## 构建与运行

构建词法分析器：

```sh
make lexer
```

构建语法分析器、语义分析器、IR 生成器和 MIPS 生成器：

```sh
make parser
make semantic
make ir
make mips
```

构建 IR 优化器：

```sh
make opt
```

## 示例命令

词法分析：

```sh
CMM_LEX_DUMP=1 build/cmm-lexer examples/manual/gcd.cmm
```

语法分析并输出 AST：

```sh
build/cmmc examples/manual/gcd.cmm /tmp/gcd.ast
sed -n '1,80p' /tmp/gcd.ast
```

生成 IR：

```sh
CMM_IR=1 build/cmmc examples/manual/gcd.cmm /tmp/gcd.ir
sed -n '1,120p' /tmp/gcd.ir
```

生成 MIPS 并使用 SPIM 运行：

```sh
build/cmmc examples/manual/gcd.cmm /tmp/gcd.s
spim -quiet -file /tmp/gcd.s
```

优化 IR：

```sh
build/cmm-opt submission/proj5/test/proj5-test1.ir /tmp/proj5-test1.opt.ir
diff -u submission/proj5/test/proj5-test1.ir /tmp/proj5-test1.opt.ir || true
```

## 测试

运行全部测试：

```sh
make test
```

也可以按阶段单独运行：

```sh
make lexer-test
make parser-test
make semantic-test
make irsim-test
make mips-test
make opt-test
```

其中：

- `irsim-test` 使用 IRSim 验证生成的 IR 能正确执行。
- `mips-test` 使用 SPIM 验证生成的 MIPS 汇编能正确运行。
- `opt-test` 对比优化前后 IR 的执行结果，验证优化不改变程序语义。

## 各阶段说明

### 1. 词法分析

词法分析器由 `src/lexer.l` 实现，负责将源程序字符流切分为 token。独立 lexer 模式下使用 `src/token.h` 中的 `TOK_*` 枚举；接入 Bison 后，通过 `PARSER_BUILD` 切换为 Bison 生成的 token。

### 2. 语法分析

语法分析由 `src/parser.y` 实现。Bison 根据文法生成 `yyparse()`，并在规约动作中构造 AST。AST 节点保存语法单元名称、行号、属性值和子节点关系。

### 3. 语义分析

语义分析由 `src/semantic.c` 实现。核心数据结构包括 `Type`、`Field`、`Symbol` 和 `Scope`，用于表示类型、字段/参数、符号和作用域。分析过程会检查未定义变量、重定义、类型不匹配、函数调用错误、数组访问错误和结构体字段错误等。

### 4. IR 生成

IR 生成由 `src/irgen.c` 实现。语义检查通过后，编译器遍历 AST，生成线性三地址代码。表达式使用临时变量保存结果，控制流使用 `LABEL`、`GOTO`、`IF`，函数调用使用 `ARG` 和 `CALL`。

### 5. MIPS 生成

MIPS 生成由 `src/mipsgen.c` 实现。后端先读取 IR，收集函数、变量、参数和指令，再为每个函数分配栈帧，最后逐条翻译为 MIPS32 指令。函数调用通过栈传参、`jal` 调用和 `$v0` 返回值实现。

### 6. IR 优化

IR 优化由 `src/iropt.c` 实现。优化器直接读取 `.ir` 文件，按行识别固定格式的三地址指令，执行常量传播、常量折叠、复制传播、公共表达式消除和死代码删除。

## 课程提交材料

`submission/` 目录保存了按课程要求整理的实践提交内容：

```text
submission/proj1  Flex 词法分析与 Bison 语法分析
submission/proj2  语义分析
submission/proj3  中间代码生成
submission/proj4  MIPS 目标代码生成
submission/proj5  IR 优化
```

每个实践目录均可独立构建和运行对应测试。
