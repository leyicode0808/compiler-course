# C-- 编译器项目导航说明书

本文用于快速了解当前仓库的实现结构。建议先看“总体链路”和“常用命令”，再按实践阶段查对应文件。

## 0. 总体链路

当前项目实现的是课程 C-- 子集到 MIPS32/SPIM 的教学编译器，并额外提供 IR 优化器：

```text
C-- 源码
  -> 词法分析 lexer
  -> 语法分析 parser / AST
  -> 语义分析 semantic
  -> 三地址 IR 生成 irgen
  -> IR 优化 iropt
  -> MIPS32 目标代码生成 mipsgen
  -> SPIM 运行
```

主编译器二进制：

```text
build/cmmc
```

独立 IR 优化器二进制：

```text
build/cmm-opt
```

## 1. 常用命令

环境检查：

```sh
make check-env
```

完整回归：

```sh
make test
```

单阶段测试：

```sh
make lexer-test
make parser-test
make semantic-test
make irsim-test
make mips-test
make opt-test
```

运行主编译器：

```sh
# 实践 1.2：输出语法树
build/cmmc input.cmm

# 实践 2：只做语义检查
CMM_SEMANTIC=1 build/cmmc input.cmm

# 实践 3：生成 IR
CMM_IR=1 build/cmmc input.cmm output.ir

# 实践 4：生成 MIPS
build/cmmc input.cmm output.s

# 实践 5：优化 IR
build/cmm-opt input.ir output.ir
```

## 1.1 功能、命令、脚本来源速查

| 你要完成的功能 | 直接命令 | Makefile 目标 | 实际脚本/源码来源 | 主要产物 |
| --- | --- | --- | --- | --- |
| 检查环境 | `make check-env` | `check-env` | `tools/check_env.sh` | 终端环境检查结果 |
| 解压课程测试 | `make prepare-tests` | `prepare-tests` | `tools/extract_tests.sh` | `tests/project1/`、`tests/project2/`、`tests/project3/` |
| 构建词法分析器 | `make lexer` | `lexer` | `src/lexer.l`、`src/lexer_main.c` | `build/cmm-lexer` |
| 测词法分析 | `make lexer-test` | `lexer-test` | `tools/run_lexer_tests.sh` | token 输出对比结果 |
| 构建主编译器 | `make parser` | `parser` | `src/parser.y`、`src/lexer.l`、`src/parser_main.c` 等 | `build/cmmc` |
| 输出语法树 | `build/cmmc input.cmm` | `parser-test` 间接覆盖 | `src/parser_main.c`、`src/ast.c` | 终端 AST |
| 测语法分析 | `make parser-test` | `parser-test` | `tools/run_parser_tests.sh` | AST 对比结果 |
| 语义检查 | `CMM_SEMANTIC=1 build/cmmc input.cmm` | `semantic-test` 间接覆盖 | `src/semantic.c` | 语义错误输出 |
| 测语义分析 | `make semantic-test` | `semantic-test` | `tools/run_semantic_tests.sh` | 语义错误对比结果 |
| 生成 IR | `CMM_IR=1 build/cmmc input.cmm output.ir` | `ir-test` 间接覆盖 | `src/irgen.c` | `.ir` 文件 |
| 批量生成 IR | `make ir-test` | `ir-test` | `tools/run_ir_tests.sh` | `build/project3-ir/*.ir` |
| 验证 IR 行为 | `make irsim-test` | `irsim-test` | `tools/run_irsim_tests.sh` | IRSim 输出对比结果 |
| 构建 IRSim 环境 | `./tools/build_irsim_image.sh` | 无 | `tools/build_irsim_image.sh` | Docker 镜像 `cmm-irsim-py38` |
| 生成 MIPS | `build/cmmc input.cmm output.s` | `mips-test` 间接覆盖 | `src/mipsgen.c` | `.s` 文件 |
| 用 SPIM 跑 MIPS | `spim -quiet -file output.s` | `mips-test` 间接覆盖 | 系统 `spim` | 终端程序输出 |
| 测 MIPS | `make mips-test` | `mips-test` | `tools/run_mips_tests.sh` | SPIM 输出对比结果 |
| 构建优化器 | `make opt` | `opt` | `src/iropt_main.c`、`src/iropt.c` | `build/cmm-opt` |
| 优化 IR | `build/cmm-opt input.ir output.ir` | `opt-test` 间接覆盖 | `src/iropt.c` | 优化后 `.ir` |
| 测优化后 IR | `make opt-test` | `opt-test` | `tools/run_opt_tests.sh` | `build/project5-ir/*.ir` 和 IRSim 对比结果 |
| 全量验收 | `make test` | `test` | 顺序调用全部测试目标 | 实践 1-5 全部结果 |

## 1.2 演示用最短流程

现场演示时建议选一个递归或数组程序，能覆盖函数、控制流和读写。

生成并运行 MIPS：

```sh
make parser
build/cmmc examples/project4/factorial.cmm /tmp/factorial.s
printf '7\n' | spim -quiet -file /tmp/factorial.s
```

期望输出：

```text
5040
```

展示 IR：

```sh
CMM_IR=1 build/cmmc examples/project4/factorial.cmm /tmp/factorial.ir
sed -n '1,80p' /tmp/factorial.ir
```

展示优化：

```sh
make ir-test
build/cmm-opt build/project3-ir/E2-1.ir /tmp/E2-1.opt.ir
wc -l build/project3-ir/E2-1.ir /tmp/E2-1.opt.ir
diff -u build/project3-ir/E2-1.ir /tmp/E2-1.opt.ir | sed -n '1,120p'
```

完整验收：

```sh
make test
```

## 2. 构建关系

`Makefile` 是总入口。

词法分析器：

```text
src/lexer.l + src/lexer_main.c + src/token.h
  -> flex 生成 build/lexer.c
  -> gcc 链接 libfl
  -> build/cmm-lexer
```

主编译器：

```text
src/parser.y + src/lexer.l
  -> bison 生成 build/parser.tab.c / build/parser.tab.h
  -> flex -DPARSER_BUILD 生成 build/parser_lexer.c

src/parser_main.c
src/ast.c
src/semantic.c
src/irgen.c
src/mipsgen.c
build/parser.tab.c
build/parser_lexer.c
  -> gcc 链接 libfl
  -> build/cmmc
```

IR 优化器：

```text
src/iropt_main.c
src/iropt.c
  -> gcc
  -> build/cmm-opt
```

## 2.1 文件结构总览

```text
.
├── Makefile                         # 总构建入口
├── src/
│   ├── lexer.l                      # Flex 词法规则
│   ├── lexer_main.c                 # 独立 lexer 入口
│   ├── token.h                      # lexer token 枚举
│   ├── parser.y                     # Bison 文法
│   ├── parser_main.c                # 主编译器入口和模式选择
│   ├── ast.c / ast.h                # AST 构造、打印、释放
│   ├── semantic.c / semantic.h      # 符号表和语义检查
│   ├── irgen.c / irgen.h            # AST 到三地址 IR
│   ├── mipsgen.c / mipsgen.h        # IR 到 MIPS32 汇编
│   ├── iropt.c / iropt.h            # IR 优化器
│   └── iropt_main.c                 # 独立优化器入口
├── tools/
│   ├── check_env.sh                 # 环境检查
│   ├── extract_tests.sh             # 解压课程测试包
│   ├── run_lexer_tests.sh           # 词法测试
│   ├── run_parser_tests.sh          # 语法测试
│   ├── run_semantic_tests.sh        # 语义测试
│   ├── run_ir_tests.sh              # IR 生成测试
│   ├── run_irsim_tests.sh           # IRSim 验证
│   ├── run_mips_tests.sh            # SPIM 验证
│   ├── run_opt_tests.sh             # 优化后 IRSim 验证
│   ├── build_irsim_image.sh         # 构建 Python 3.8 IRSim 镜像
│   └── gitw                         # Git 包装脚本
├── docs/                            # 阶段说明和导航文档
├── report/                          # 报告 Markdown 草稿
├── examples/
│   ├── manual/                      # 手写验收 C-- 程序
│   ├── project4/                    # 实践四公开 C-- 样例
│   └── project5/                    # 实践五三个必做公开 IR 样例
├── 编译原理课程设计/                 # 课程 PDF、zip 原始资料
├── tests/                           # 解压后的测试，Git 忽略
└── build/                           # 编译产物和生成结果，Git 忽略
```

## 2.2 入口模式解释

`src/parser_main.c` 根据参数数量和环境变量选择阶段：

| 模式 | 条件 | 调用链 |
| --- | --- | --- |
| AST 输出 | `argc == 2`，无特殊环境变量 | `yyparse()` -> `ast_print()` |
| 语义检查 | `CMM_SEMANTIC=1` | `yyparse()` -> `semantic_check()` |
| IR 生成 | `CMM_IR=1` 且两个参数 | `yyparse()` -> `semantic_check()` -> `ir_generate()` |
| MIPS 生成 | 两个参数，且没有 `CMM_IR=1` | `yyparse()` -> `semantic_check()` -> `ir_generate(tmp)` -> `mips_generate_from_ir()` |

实践五优化器不走 `parser_main.c`，入口是 `src/iropt_main.c`。

## 3. 实践 1.1：词法分析

### 涉及文件

- `src/lexer.l`：Flex 词法规则；同时服务 lexer 独立程序和 parser。
- `src/token.h`：lexer 独立模式下的 token 枚举。
- `src/lexer_main.c`：独立词法分析入口，读取 `.cmm` 并输出 token。
- `tools/run_lexer_tests.sh`：词法测试脚本。
- `docs/practice1.1-lexer.md`、`report/practice1.1-lexer-report.md`：说明和报告草稿。

### 编译链接

```sh
make lexer
```

实际命令：

```text
flex -o build/lexer.c src/lexer.l
gcc -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -I src \
    src/lexer_main.c build/lexer.c -lfl -o build/cmm-lexer
```

### 运行

```sh
build/cmm-lexer tests/project1/inputs/A-1.cmm
```

### 函数索引

`src/lexer.l`

- `lexical_error(message)`：按课程格式输出普通词法错误。
- `lexical_error_text(message, text)`：输出带原始文本的词法错误，例如非法十六进制、非法浮点数。

`src/lexer_main.c`

- `token_name(kind)`：把 token 枚举转成输出名称。
- `token_has_value(kind)`：判断 token 输出时是否需要携带词素值。
- `main(argc, argv)`：打开输入文件，循环调用 `yylex()`，输出 token 或词法错误。

### 测试

```sh
make lexer-test
```

当前结果：

```text
lexer tests: 6 passed, 0 failed
```

## 4. 实践 1.2：语法分析和 AST

### 涉及文件

- `src/parser.y`：Bison 文法和语法错误处理。
- `src/ast.c`、`src/ast.h`：AST 节点构造、打印、释放。
- `src/parser_main.c`：主编译器入口；默认一参数模式输出语法树。
- `tools/run_parser_tests.sh`：语法树输出回归测试。
- `docs/practice1.2-parser.md`、`report/practice1.2-parser-report.md`：说明和报告草稿。

### 编译链接

```sh
make parser
```

主编译器构建时，`bison` 生成 parser，`flex -DPARSER_BUILD` 生成 parser 使用的 lexer，然后和 AST、语义、IR、MIPS 模块一起链接成 `build/cmmc`。

### 运行

```sh
build/cmmc tests/project1/inputs/A-1.cmm
```

### 函数索引

`src/parser.y`

- `node_line(node)`：取 AST 节点行号，错误恢复时使用。
- `syntax_error_at(line, msg)`：按课程格式输出语法错误。
- `ast_has_array_vardec(node)`：检查函数声明错误恢复场景中的数组形参。
- `yyerror(msg)`：Bison 默认错误回调。

`src/ast.c`

- `xstrdup(text)`：字符串复制辅助函数。
- `ast_token(name, value, line)`：构造 token 叶子节点。
- `ast_node(name, line, child_count, ...)`：构造普通 AST 节点。
- `ast_adopt(name, line, first, second)`：把两个列表节点拼接成一个节点。
- `ast_parse_int(text)`：解析整数字面量。
- `print_indent(indent)`：打印语法树缩进。
- `ast_print(node, indent)`：递归输出语法树。
- `ast_free(node)`：递归释放 AST。

`src/parser_main.c`

- `main(argc, argv)`：统一入口。根据环境变量选择 AST、语义、IR、MIPS 模式。

### 测试

```sh
make parser-test
```

当前结果：

```text
parser tests: 20 passed, 0 failed
```

## 5. 实践 2：语义分析

### 涉及文件

- `src/semantic.c`、`src/semantic.h`：符号表、类型系统、语义错误检查。
- `src/parser_main.c`：`CMM_SEMANTIC=1` 时调用语义分析。
- `tools/run_semantic_tests.sh`：语义错误回归测试。
- `docs/practice2-semantic.md`、`report/practice2-semantic-report.md`：说明和报告草稿。

### 运行

```sh
CMM_SEMANTIC=1 build/cmmc tests/project2/inputs/A-1.cmm
```

### 核心数据结构

- `Type`：表示 `int`、`float`、数组、结构体、函数、错误类型。
- `Field`：结构体字段或函数参数。
- `Symbol`：符号表条目。
- `Scope`：作用域链。
- `VarInfo`：从 `VarDec` 中读出的变量名和数组维度。

### 函数索引

基础辅助：

- `xstrdup(text)`：复制字符串。
- `is_node(node, name)`：判断 AST 节点类型。
- `child(node, idx)`：取第 `idx` 个子节点。
- `has_child_name(node, name)`：判断是否存在指定子节点。
- `first_child_name(node, name)`：取第一个指定名称子节点。
- `report_error(type, line, msg)`：立即输出语义错误。
- `queue_error(type, line, msg)`：暂存语义错误。
- `flush_pending_errors()`：按顺序输出暂存错误。

类型系统：

- `new_type(kind)`：创建类型对象。
- `type_int()`、`type_float()`、`type_error()`：基础类型单例。
- `array_of(elem, size)`：构造数组类型。
- `struct_type(name, fields)`：构造结构体类型。
- `func_type(ret, params, param_count, defined, line)`：构造函数类型。
- `field_list_equal(a, b)`：比较字段/参数列表。
- `type_equal(a, b)`：比较类型是否等价。
- `is_numeric(type)`：判断是否为数值类型。

符号表和结构体：

- `push_scope()`、`pop_scope()`：进入/退出作用域。
- `find_in_scope(scope, name)`：在单个作用域查找。
- `find_symbol(name)`：沿作用域链查找变量。
- `find_function(name)`：查找函数符号。
- `add_symbol_current(name, type, line)`：向当前作用域插入符号。
- `find_field(fields, name)`：查找结构体字段。
- `append_field(head, field)`：追加字段。
- `new_field(name, type, line)`：创建字段。
- `find_struct_def(name)`：查找结构体定义。
- `add_struct_def(name, type, line)`：登记结构体定义。

AST 分析：

- `read_vardec(var_dec, out)`：解析变量名和数组维度。
- `apply_dims(base, info)`：把数组维度应用到基础类型上。
- `analyze_struct_specifier(node)`：分析结构体定义或引用。
- `analyze_specifier(specifier)`：分析类型说明符。
- `collect_dec(dec, base, as_fields, fields)`：分析单个声明项。
- `collect_dec_list(dec_list, base, as_fields, fields)`：分析声明列表。
- `analyze_def(def, as_fields, fields)`：分析局部定义或结构体字段定义。
- `analyze_def_list(node, as_fields, fields)`：分析定义列表。
- `collect_params(var_list, count)`：收集函数形参。
- `add_params_to_scope(params)`：把形参加入函数体作用域。
- `handle_function(ext_def, ret, is_definition)`：处理函数声明/定义。
- `handle_ext_decl_list(node, base)`：处理全局变量声明列表。
- `analyze_ext_def(node)`：分析外部定义。
- `analyze_ext_def_list(node)`：分析外部定义列表。
- `analyze_compst(node, return_type, create_scope)`：分析复合语句。
- `analyze_stmt(node, return_type)`：分析语句。
- `analyze_stmt_list(node, return_type)`：分析语句列表。
- `args_from_node(args, count)`：分析实参列表。
- `binary_numeric(node, is_lvalue)`：分析二元数值表达式。
- `analyze_exp(node, is_lvalue)`：分析表达式并返回类型。
- `check_undefined_functions()`：检查声明但未定义的函数。
- `add_builtin_functions()`：加入内置 `read` 和 `write`。
- `semantic_check(root)`：语义分析公开入口。

### 测试

```sh
make semantic-test
```

当前结果：

```text
semantic tests: 30 passed, 0 failed
```

## 6. 实践 3：中间代码生成

### 涉及文件

- `src/irgen.c`、`src/irgen.h`：AST 到三地址 IR。
- `src/parser_main.c`：`CMM_IR=1` 时调用 IR 生成。
- `tools/run_ir_tests.sh`：批量生成 IR。
- `tools/run_irsim_tests.sh`：使用课程 IRSim 验证 IR。
- `tools/build_irsim_image.sh`：构建 Python 3.8 + PyQt5 的 IRSim Docker 镜像。
- `docs/practice3-ir.md`、`report/practice3-ir-report.md`：说明和报告草稿。

### 运行

```sh
CMM_IR=1 build/cmmc tests/project3/inputs/A-1.cmm build/project3-ir/A-1.ir
```

### 核心数据结构

- `IrType`：IR 阶段类型。
- `IrField`：结构体字段或函数参数，并记录字段偏移。
- `IrSymbol`：IR 生成时的符号表条目。
- `IrScope`：IR 生成阶段作用域。
- `IrFunc`：函数签名。
- `ArgList`：函数调用实参链表。

### 函数索引

通用辅助：

- `xstrdup(text)`、`xasprintf(fmt, ...)`：字符串辅助。
- `is_node(node, name)`、`child(node, idx)`、`first_child_name(node, name)`：AST 访问辅助。
- `emit(fmt, ...)`：输出一行 IR。
- `new_temp()`：生成临时变量 `tN`。
- `new_label()`：生成标签 `labelN`。
- `materialize_addr(addr)`：把 `&x` 形式地址转成临时变量，便于间接访问。

类型、字段、符号：

- `new_type(kind)`、`type_int()`、`type_float()`：创建/获取类型。
- `array_of(elem, size)`、`struct_type(name, fields)`：构造复合类型。
- `type_size(type)`：计算类型占用字节数。
- `new_field(name, type)`、`append_field(head, field)`、`find_field(fields, name)`：字段操作。
- `find_struct_def(name)`：查找结构体定义。
- `push_scope()`、`pop_scope()`：维护作用域。
- `find_symbol(name)`、`add_symbol(name, type, is_addr_param)`：符号表操作。
- `find_func(name)`、`add_func(name, ret, params)`：函数签名表。

AST 到 IR：

- `read_vardec(var_dec, out_info)`：解析变量名和维度。
- `apply_dims(base, info)`：构造数组类型。
- `analyze_struct_specifier(node)`、`analyze_specifier(specifier)`：分析类型。
- `collect_dec(dec, base, as_fields, fields, emit_decs)`：收集声明并按需输出 `DEC`。
- `collect_dec_list(...)`、`collect_def(...)`、`collect_def_list(...)`：处理声明列表。
- `collect_params(var_list)`：收集函数形参。
- `collect_function_sigs(node)`：预收集函数签名，便于调用时判断传值/传地址。
- `nth_param(func, idx)`：取第 `idx` 个形参。
- `gen_params(params)`：输出 `PARAM`。
- `gen_compst(node, create_scope)`：生成复合语句 IR。
- `gen_ext_def(node)`、`gen_ext_def_list(node)`：生成外部定义 IR。
- `gen_stmt_list(node)`、`gen_stmt(node)`：生成语句 IR。
- `exp_type(node)`：推断表达式类型。
- `gen_addr(node, out_type)`：生成左值地址。
- `append_arg(head, place)`、`collect_args(args, func, idx)`、`emit_args_reverse(args)`：处理函数实参。
- `gen_call(node)`：生成函数调用，特殊处理 `read/write`。
- `gen_exp(node)`：生成表达式 IR。
- `gen_cond(node, true_label, false_label)`：生成条件跳转 IR。
- `ir_generate(root, output_path)`：IR 生成公开入口。

### 测试

```sh
make ir-test
make irsim-test
```

当前结果：

```text
ir tests: 16 generated, 0 failed
irsim tests: 16 passed, 0 failed
```

## 7. 实践 4：MIPS 目标代码生成

### 涉及文件

- `src/mipsgen.c`、`src/mipsgen.h`：IR 到 MIPS32 汇编。
- `src/parser_main.c`：两个参数默认生成 MIPS。
- `tools/run_mips_tests.sh`：SPIM 回归测试。
- `examples/project4/`：实践四 PDF 中的 Fibonacci 和 Factorial 公开样例。
- `docs/practice4-mips.md`、`report/practice4-mips-report.md`：说明和报告草稿。

### 运行

```sh
build/cmmc examples/project4/factorial.cmm /tmp/factorial.s
printf '7\n' | spim -quiet -file /tmp/factorial.s
```

### 后端策略

当前采用正确性优先的朴素栈式后端：

- 每个函数建立独立栈帧。
- 所有局部变量、临时变量、`DEC` 空间放在栈帧中。
- 每条 IR 指令按需 `lw` 到 `$t0/$t1`，计算后 `sw` 回栈。
- `ARG` 压栈，`CALL` 使用 `jal`，返回值在 `$v0`。
- `READ`/`WRITE` 使用 SPIM syscall。

### 函数索引

`src/mipsgen.c`

- `xstrdup(text)`、`trim(text)`、`starts_with(text, prefix)`、`emit(text)`：基础辅助。
- `find_var(func, name)`、`add_var(func, name, size)`：函数局部变量表。
- `add_param(func, name)`：记录函数形参。
- `add_instr(func, text)`：保存一条 IR 指令。
- `is_ident_char(ch)`、`clean_operand(operand, buf, size)`：操作数字符串处理。
- `collect_operand(func, operand)`：从操作数中收集变量。
- `split_assign(line, lhs, rhs)`：拆分赋值 IR。
- `collect_assignment(func, line)`、`collect_vars(func, text)`：扫描 IR 收集栈帧所需变量。
- `parse_ir(input)`：按函数解析文本 IR。
- `layout_frame(func)`：计算栈帧偏移。
- `load_operand(func, operand, reg)`：把 IR 操作数加载到 MIPS 寄存器。
- `store_var(func, name, reg)`：把寄存器值写回变量槽。
- `emit_epilogue(func)`：输出函数返回代码。
- `branch_op(relop)`：把 IR 关系运算映射成 MIPS 分支指令。
- `emit_assignment(func, line, pending_args, pending_count, pending_cap)`：翻译赋值和 `CALL`。
- `add_pending_arg(...)`、`emit_call_args(...)`、`clear_pending_args(...)`：处理函数调用参数。
- `emit_instr(func, text, pending_args, pending_count, pending_cap)`：翻译单条 IR 指令。
- `emit_func(func)`：输出一个函数的 MIPS。
- `mips_generate_from_ir(ir_path, output_path)`：MIPS 生成公开入口。

### 测试

```sh
make mips-test
```

当前结果：

```text
mips tests: 18 passed, 0 failed
```

## 8. 实践 5：IR 优化

### 涉及文件

- `src/iropt.c`、`src/iropt.h`：文本 IR 优化器。
- `src/iropt_main.c`：优化器命令行入口。
- `tools/run_opt_tests.sh`：优化后 IRSim 回归测试。
- `examples/project5/`：实践五 PDF 中的三个必做公开 IR 样例片段。
- `docs/practice5-opt.md`、`report/practice5-opt-report.md`：说明和报告草稿。

### 运行

```sh
build/cmm-opt build/project3-ir/A-3.ir build/project5-ir/A-3.ir
```

### 优化内容

- 常量传播。
- 复制传播。
- 常量折叠。
- 局部公共子表达式消除。
- 局部无用赋值删除。
- 基于活跃变量分析的全局无用赋值删除。

### 函数索引

`src/iropt_main.c`

- `main(argc, argv)`：读取输入/输出路径，调用 `ir_optimize_file()`。

`src/iropt.c`

通用辅助：

- `xstrdup(text)`、`xasprintf(fmt, ...)`、`trim(text)`、`starts_with(text, prefix)`：字符串辅助。
- `line_push(vec, text)`、`free_lines(vec)`：IR 行数组管理。
- `is_number(text)`、`is_const_operand(text)`、`is_var_operand(text)`：操作数分类。
- `parse_assign(line, lhs, rhs)`、`parse_binary(rhs, left, op, right)`、`parse_simple_operand(rhs, operand)`：IR 解析。
- `operand_uses_var(operand, var)`、`line_uses_var(line, var)`、`line_defines_var(line, var)`：变量使用/定义判断。
- `is_barrier(line)`：判断局部优化边界。

常量/复制传播：

- `bind_clear(vec)`、`bind_free(vec)`、`bind_reserve(vec)`：绑定表管理。
- `bind_get(vec, name)`、`bind_remove_at(vec, idx)`、`bind_kill(vec, name)`、`bind_set(vec, name, value)`：变量到常量/别名的绑定。
- `substitute_operand(bindings, operand, size)`：根据绑定替换操作数。
- `eval_binary(lhs, op, rhs, result)`：常量折叠计算。
- `replace_line(line, text)`：替换 IR 行文本。
- `rewrite_assignment(line, bindings, exprs)`：改写赋值指令。
- `rewrite_non_assignment(line, bindings)`：改写 `IF/RETURN/WRITE/ARG` 等指令。
- `rewrite_pass(lines)`：执行一轮传播、折叠和公共子表达式改写。

公共子表达式：

- `expr_clear(vec)`、`expr_free(vec)`、`expr_remove_at(vec, idx)`：表达式表管理。
- `expr_kill_var(vec, name)`：变量重定义时清理相关表达式。
- `normalize_expr(left, op, right)`：规范化可交换表达式。
- `expr_find(vec, left, op, right)`：查找已有表达式。
- `expr_add(vec, left, op, right, result)`：登记表达式结果。

局部 DCE：

- `block_end(lines, start)`：找到局部块结束位置。
- `assignment_has_side_effect(line)`：判断赋值是否有副作用。
- `can_remove_assignment(lines, idx)`：判断局部赋值能否删除。
- `dce_pass(lines)`：执行局部无用赋值删除。

全局活跃变量 DCE：

- `set_free(set)`、`set_clear(set)`、`set_contains(set, name)`、`set_add(set, name)`、`set_remove(set, name)`：字符串集合。
- `set_union_into(dst, src)`、`set_equals(a, b)`、`set_assign(dst, src)`：集合运算。
- `add_operand_var(set, operand)`、`collect_address_operand(address_taken, operand)`：收集 use 和取地址变量。
- `collect_line_use_def(line, use, def, address_taken)`：为单条 IR 计算 use/def。
- `next_active(lines, idx)`：找下一条未删除指令。
- `label_target(lines, label)`：查找标签目标。
- `add_fallthrough_successor(lines, idx, succs, count)`：加入顺序后继。
- `line_successors(lines, idx, succs)`：计算一条 IR 的控制流后继。
- `removable_global_assignment(line, live_out, address_taken)`：判断全局 DCE 是否可删。
- `global_dce_pass(lines)`：反向迭代活跃变量，删除 live-out 不活跃的无副作用赋值。

文件 IO：

- `read_lines(input_path, lines)`：读取 IR。
- `write_lines(output_path, lines)`：写出未删除 IR。
- `ir_optimize_file(input_path, output_path)`：优化器公开入口。

### 测试

```sh
make opt-test
```

当前结果：

```text
opt tests: 16 passed, 0 failed, saved 38 IR lines
```

## 9. 脚本说明

- `tools/check_env.sh`：检查 `gcc/make/flex/bison/unzip/pdftotext/spim`。
- `tools/extract_tests.sh`：从课程 zip 包解压 Project 1-3 测试样例。
- `tools/run_lexer_tests.sh`：对比 lexer token 输出。
- `tools/run_parser_tests.sh`：对比 parser AST 输出。
- `tools/run_semantic_tests.sh`：对比语义错误输出。
- `tools/run_ir_tests.sh`：批量生成 Project 3 IR。
- `tools/build_irsim_image.sh`：构建运行 `irsim.pyc` 所需 Docker 镜像。
- `tools/run_irsim_tests.sh`：在 Docker 中调用课程 IRSim 验证原始 IR。
- `tools/run_mips_tests.sh`：生成 MIPS 并用 SPIM 比较输出。
- `tools/run_opt_tests.sh`：优化 IR 后用 IRSim 比较输出。
- `tools/gitw`：本仓库使用的 git 包装脚本。

## 10. 目录速查

```text
src/        编译器和优化器源码
tools/      构建、测试、环境脚本
docs/       阶段说明和本导航文档
report/     实验报告 Markdown 草稿
examples/   手写验收代码和公开样例
tests/      解压后的课程测试样例，Git 忽略
build/      编译产物和生成结果，Git 忽略
```

## 11. 当前验收结果

最近一次完整回归：

```text
lexer tests: 6 passed, 0 failed
parser tests: 20 passed, 0 failed
semantic tests: 30 passed, 0 failed
irsim tests: 16 passed, 0 failed
mips tests: 18 passed, 0 failed
opt tests: 16 passed, 0 failed, saved 38 IR lines
```

## 12. 重要边界

- 当前编译器面向课程 C-- 子集，不是完整 C 编译器。
- `float` 已在词法/语法/语义层识别，但 IR/MIPS 后端主要按整数路径实现。
- MIPS 后端采用朴素栈式分配，优先正确性，没有做图染色寄存器分配或指令调度。
- 实践五优化器偏保守，遇到取地址和副作用会放弃删除，避免错误优化。

## 13. 演示或提问时的定位建议

如果被问“词法规则在哪里”：

- 看 `src/lexer.l`。
- token 名称和独立输出在 `src/token.h`、`src/lexer_main.c`。
- 测试入口是 `tools/run_lexer_tests.sh`。

如果被问“语法树怎么构建”：

- 文法动作在 `src/parser.y`。
- `ast_node()` 和 `ast_token()` 在 `src/ast.c`。
- 默认一参数运行 `build/cmmc input.cmm` 会调用 `ast_print()`。

如果被问“符号表在哪里”：

- 看 `src/semantic.c` 中的 `Scope`、`Symbol`、`push_scope()`、`pop_scope()`、`add_symbol_current()`、`find_symbol()`。
- 类型等价在 `type_equal()`。
- 函数定义和声明检查在 `handle_function()`。

如果被问“结构体和数组怎么处理”：

- 语义阶段：`read_vardec()` 读取数组维度，`analyze_struct_specifier()` 处理结构体。
- IR 阶段：`type_size()` 算大小，`gen_addr()` 统一生成数组/结构体字段地址。
- MIPS 阶段：`DEC x size` 分配栈帧空间，`&x`、`*p` 转成地址计算和 `lw/sw`。

如果被问“函数调用怎么做”：

- IR 生成：`collect_args()` 收集实参，`emit_args_reverse()` 输出 `ARG`，`gen_call()` 输出 `CALL`。
- MIPS 生成：`add_pending_arg()` 暂存实参，`emit_call_args()` 压栈，`jal` 调用，返回值保存在 `$v0`。
- 被调函数在 `emit_func()` 入口把参数复制进自己的局部栈槽。

如果被问“递归为什么能跑”：

- MIPS 后端每次调用都会建立新栈帧。
- `$ra` 和旧 `$fp` 保存在当前帧中。
- 参数和临时变量都在当前调用帧内，所以递归层之间不会互相覆盖。

如果被问“read/write 怎么实现”：

- IR 阶段：`read()` 转为 `READ t`，`write(x)` 转为 `WRITE x`。
- MIPS 阶段：
  - `READ` 使用 syscall 5。
  - `WRITE` 使用 syscall 1 输出整数，再使用 syscall 4 输出换行。

如果被问“优化怎么保证不改语义”：

- 常量/复制传播只在基本块内保留状态，遇到 label、跳转、return、call 会清空。
- 全局 DCE 只删除无副作用赋值。
- `*p := x`、`CALL`、`READ`、`WRITE`、`RETURN`、`ARG` 不删除。
- 出现 `&x` 的变量写入保守保留，避免指针/地址别名问题。
- 每次优化后都用 IRSim 跑输出一致性测试。

如果被问“为什么实践五在实践四之后做”：

- 教学顺序是先用实践四闭环生成 MIPS。
- 工程流水线中，优化实际位于 IR 生成和 MIPS 生成之间：

```text
C-- -> IR -> 优化 IR -> MIPS
```

当前优化器是独立命令 `build/cmm-opt input.ir output.ir`，方便单独验收。

如果被问“为什么没有做图染色寄存器分配”：

- 实践四检查重点是生成的 MIPS 能否在 SPIM 正确运行。
- 当前后端选择朴素栈式分配，变量都放栈，需要时加载到临时寄存器，正确性稳定。
- 图染色和指令调度属于更高质量目标代码优化，当前没有实现。

## 14. 常见现场命令模板

完整测试：

```sh
make test
```

查看最近提交：

```sh
./tools/gitw log --oneline -8
```

查看工作区是否干净：

```sh
./tools/gitw status --short --branch
```

手写程序编译到 MIPS：

```sh
build/cmmc demo.cmm demo.s
spim -quiet -file demo.s
```

手写程序生成 IR 并优化：

```sh
CMM_IR=1 build/cmmc demo.cmm demo.ir
build/cmm-opt demo.ir demo.opt.ir
```

只看某个阶段文件：

```sh
sed -n '1,160p' src/semantic.c
sed -n '1,160p' src/irgen.c
sed -n '1,160p' src/mipsgen.c
sed -n '1,160p' src/iropt.c
```

## 15. 一句话介绍项目

可以这样介绍：

```text
这个项目实现了一个面向课程 C-- 子集的教学编译器。前端使用 Flex/Bison 完成词法和语法分析，并构建 AST；语义阶段实现符号表和类型检查；中端生成三地址 IR 并提供基础优化；后端将 IR 翻译为 MIPS32 汇编，最后用 IRSim 和 SPIM 验证程序行为。
```
