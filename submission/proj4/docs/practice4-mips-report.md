# 编译原理课程设计报告：实践 4 目标代码生成

课程设计名称：编译原理课程设计

实践内容：实践 4 目标代码生成

## 一、目的和任务

本次实践在 `submission/proj4` 目录中独立完成，目标是在已有词法分析、语法分析、语义分析和中间代码生成的基础上，将 C-- 程序翻译为可在 SPIM Simulator 中运行的 MIPS32 汇编代码。

本阶段主要任务如下：

1. 增加目标代码生成入口，接收 C-- 输入文件和 `.s` 输出文件。
2. 复用实践三生成的线性 IR，完成从 IR 到 MIPS32 指令的翻译。
3. 实现函数栈帧、参数传递、返回值、递归调用和局部数据存储。
4. 翻译赋值、算术运算、条件跳转、循环、数组地址计算和间接访存。
5. 使用 SPIM 对生成的汇编程序进行运行验证。
6. 编写 `Makefile` 和 `tools/run_mips_tests.sh`，使实践四可以在 `proj4` 目录中单独构建、单独演示。

## 二、项目结构和文件功能

实践四相关文件均位于 `submission/proj4` 下，目录结构如下：

```text
submission/proj4/
├── Makefile
├── course/
│   ├── Appendix.pdf
│   ├── MIPS32_and_SPIM.pdf
│   └── Project_4.pdf
├── docs/
│   ├── cmm-grammar.md
│   ├── practice4-mips.md
│   ├── practice4-mips-report.md
│   └── practice4-mips-report.docx
├── src/
│   ├── ast.c
│   ├── ast.h
│   ├── irgen.c
│   ├── irgen.h
│   ├── lexer.l
│   ├── lexer_main.c
│   ├── mipsgen.c
│   ├── mipsgen.h
│   ├── parser.y
│   ├── parser_main.c
│   ├── semantic.c
│   ├── semantic.h
│   └── token.h
├── test/
│   ├── proj4-5-3-6-exp1-1.cmm
│   ├── proj4-5-3-6-exp2-1.cmm
│   └── proj4-test1.cmm
└── tools/
    ├── check_env.sh
    └── run_mips_tests.sh
```

各文件功能如下：

- `Makefile`：实践四独立构建入口。`make mips` 构建 MIPS 生成器，`make mips-demo` 生成汇编并运行 SPIM。
- `src/mipsgen.c`：实践四核心实现，读取线性 IR，按函数分组，计算栈帧布局，并输出 MIPS32 汇编。
- `src/mipsgen.h`：声明后端入口 `mips_generate_from_ir(const char *ir_path, const char *output_path)`。
- `src/parser_main.c`：主程序入口。一参数时输出 AST；设置 `CMM_SEMANTIC=1` 时只做语义检查；设置 `CMM_IR=1` 时生成 IR；两参数模式下生成 MIPS 汇编。
- `src/irgen.c`、`src/irgen.h`：复用实践三，将 AST 翻译为临时 IR。
- `src/semantic.c`、`src/semantic.h`：复用实践二，在生成 IR 和 MIPS 前做语义检查。
- `src/parser.y`、`src/lexer.l`：复用前端，负责生成后端所需 AST。
- `src/ast.c`、`src/ast.h`：AST 节点结构、节点创建、整数解析、先序打印和内存释放。
- `tools/run_mips_tests.sh`：运行 `Project_4.pdf` 中 5.3.6 的公开样例和手写样例，打印汇编文件前 120 行，并调用 SPIM 输出运行结果。
- `test/`：按真实章节号命名的公开样例，以及手写综合样例 `proj4-test1.cmm`。

## 三、在前三次实践基础上的修改和新增内容

实践四是在实践三 IR 生成器基础上的后端扩展，主要新增和修改如下：

1. 新增 `src/mipsgen.h`，声明 MIPS 目标代码生成入口。
2. 新增 `src/mipsgen.c`，实现 IR 到 MIPS32 汇编的翻译。
3. 修改 `src/parser_main.c`，支持两参数目标代码生成模式：`build/cmmc input.cmm output.s`。
4. 修改 `Makefile`，收敛为实践四当前目录可独立运行的目标，只保留 lexer、parser、semantic、ir、mips 和演示相关入口。
5. 新增 `tools/run_mips_tests.sh`，使用当前 `test/` 目录中的公开样例生成汇编并运行 SPIM。
6. 新增 `test/proj4-5-3-6-...` 样例文件，文件名中的 `5-3-6` 对应 `Project_4.pdf` 的必做样例小节。
7. 删除与当前目录演示无关的旧批量测试、示例和报告目录，使实践四目录只保留当前演示所需材料。

## 四、主要数据结构和函数功能

### 1. MIPS 后端数据结构

`src/mipsgen.c` 中使用以下结构保存 IR 和栈帧信息：

- `Var`：表示一个变量、临时变量、数组或结构体空间，保存名称、大小、栈帧偏移和下一个变量。
- `Instr`：表示一条 IR 文本指令，保存指令字符串和下一条指令。
- `Func`：表示一个函数，保存函数名、IR 指令链表、变量链表、参数列表、栈帧大小和下一个函数。

后端先读取 IR 文本，将指令按函数分组，再为每个函数收集变量和参数，最后计算栈帧布局并生成汇编。

### 2. IR 解析和变量收集函数

- `trim(char *text)`：去掉一行文本首尾空白。
- `starts_with(const char *text, const char *prefix)`：判断指令前缀。
- `parse_ir(FILE *input)`：读取 IR 文件，遇到 `FUNCTION f :` 时创建函数对象，其他行加入当前函数指令链表。
- `collect_vars(Func *func, const char *text)`：分析一条 IR 指令中出现的变量和临时变量。
- `collect_assignment(Func *func, char *line)`：分析赋值指令左右两侧，收集操作数。
- `collect_operand(Func *func, const char *operand)`：从 `x`、`&x`、`*x` 等操作数中提取变量名。
- `add_var(Func *func, const char *name, int size)`：向函数变量表加入变量，`DEC` 指令会用更大的 size 更新变量空间。
- `add_param(Func *func, const char *name)`：记录函数形参，并同时为形参建立本地栈槽。
- `layout_frame(Func *func)`：为函数内所有变量分配 `$fp` 负偏移，并计算对齐后的栈帧大小。

### 3. 操作数装载和存储函数

- `load_operand(Func *func, const char *operand, const char *reg)`：把 IR 操作数装入指定寄存器。立即数 `#n` 用 `li`；普通变量用 `lw`；地址 `&x` 用 `$fp + offset`；间接读 `*p` 用两次 `lw`。
- `store_var(Func *func, const char *name, const char *reg)`：把寄存器内容写回变量对应的栈槽。
- `branch_op(const char *relop)`：把 IR 关系运算符转换为 MIPS 分支指令，例如 `==` 到 `beq`、`<` 到 `blt`。

### 4. 指令翻译函数

- `emit(const char *text)`：输出一行汇编。
- `emit_assignment()`：翻译 `x := y`、`x := y + z`、`x := *p`、`*p := x` 和 `x := CALL f` 等赋值指令。
- `emit_instr()`：翻译单条 IR 指令，分派处理 `LABEL`、`GOTO`、`IF`、`RETURN`、`READ`、`WRITE`、`ARG` 和赋值指令。
- `add_pending_arg()`：记录函数调用前遇到的 `ARG`。
- `emit_call_args()`：在真正执行 `CALL` 前把实参压栈。
- `clear_pending_args()`：函数调用完成后清空暂存实参。
- `emit_epilogue(Func *func)`：输出函数返回序列。`main` 通过 syscall 10 退出；其他函数恢复 `$sp`、`$ra`、`$fp` 后 `jr $ra`。
- `emit_func(Func *func)`：输出函数标签、函数序言、参数复制、函数体指令和必要的返回处理。

### 5. MIPS 生成总入口

`mips_generate_from_ir(const char *ir_path, const char *output_path)` 是实践四的核心入口，执行流程如下：

1. 打开临时 IR 文件。
2. 调用 `parse_ir()` 将 IR 按函数组织起来。
3. 打开输出 `.s` 汇编文件。
4. 输出 `.data`、换行字符串、`.globl main` 和 `.text`。
5. 逐个函数调用 `emit_func()` 输出 MIPS32 汇编。
6. 关闭输出文件并返回状态码。

## 五、目标代码生成策略

当前后端采用朴素但稳定的栈式策略：

- 每个函数建立独立栈帧。
- `$ra` 和旧 `$fp` 保存在当前栈帧中。
- 局部变量、临时变量、数组和结构体展开后的存储空间都放在当前函数活动记录中。
- `DEC x size` 为数组或结构体变量保留连续字节空间。
- `&x` 生成相对 `$fp` 的地址。
- `*p` 和 `*p := x` 翻译为间接 `lw`、`sw`。
- `ARG` 按 IR 顺序暂存，`CALL` 前压栈，`CALL` 后由调用者回收参数空间。
- 被调函数入口按照 `PARAM` 顺序从调用者参数区读取实参，并复制到自己的局部槽位。
- `READ` 使用 syscall 5 读整数。
- `WRITE` 使用 syscall 1 输出整数，并使用 syscall 4 输出换行。

这种策略不追求寄存器利用率，但结构简单，能够稳定支持递归调用和嵌套函数调用。

## 六、构建和运行

所有命令均在当前实践目录下执行：

```sh
cd submission/proj4
```

检查环境：

```sh
make check-env
```

构建 MIPS 生成器：

```sh
make mips
```

该命令生成：

```text
build/cmmc
```

运行公开样例演示：

```sh
make mips-demo
```

也可以执行：

```sh
make mips-test
```

手动查看手写样例源码、生成 MIPS 汇编并运行：

```sh
nl -ba test/proj4-test1.cmm
build/cmmc test/proj4-test1.cmm /tmp/proj4-test1.s
sed -n '1,160p' /tmp/proj4-test1.s
spim -quiet -file /tmp/proj4-test1.s
```

## 七、结果与结论分析

执行：

```sh
make mips-demo
```

当前会运行 `Project_4.pdf` 中的公开样例：

- `test/proj4-5-3-6-exp1-1.cmm`：5.3.6 必做样例 1，输入 `7`，输出前 7 个 Fibonacci 数。
- `test/proj4-5-3-6-exp2-1.cmm`：5.3.6 必做样例 2，输入 `7`，输出 `5040`。
- `test/proj4-test1.cmm`：手写样例，输出循环调用 `square` 后的累加结果。

演示命令会先生成 `.s` 文件，再打印汇编前 120 行，最后调用 SPIM 输出程序运行结果。公开样例覆盖了循环、读写、递归函数调用和返回值；手写样例覆盖普通函数调用、循环和累加赋值。

本次实践完成了 C-- 到 MIPS32 汇编的目标代码生成。程序能够复用前端、语义分析和 IR 生成阶段，在正确输入上输出可被 SPIM 执行的汇编代码。至此，编译器已经形成从源程序到目标汇编的完整主流程。
