# 编译原理课程设计报告：实践 3 中间代码生成

课程设计名称：编译原理课程设计

实践内容：实践 3 中间代码生成

## 一、目的和任务

本次实践在 `submission/proj3` 目录中独立完成，目标是在词法分析、语法分析和语义分析完成的基础上，将 C-- 源程序翻译为线性中间代码。中间代码采用三地址代码形式，用于表示赋值、算术运算、条件跳转、循环、函数调用、参数传递、数组访问、结构体字段访问和输入输出操作。

本阶段主要任务如下：

1. 在语义检查通过后遍历 AST，生成 `.ir` 中间代码文件。
2. 生成临时变量和跳转标号，翻译表达式、语句和控制流。
3. 翻译函数定义、形参、实参、函数调用和返回语句。
4. 翻译数组和结构体变量的空间申请、地址计算和间接读写。
5. 特殊处理内置函数 `read` 和 `write`，生成 `READ`、`WRITE` 指令。
6. 保留实践 1 和实践 2 的前端与语义检查，使错误程序不会进入 IR 生成阶段。
7. 编写 `Makefile` 和 `tools/run_ir_tests.sh`，使实践三可以在 `proj3` 目录中单独构建、单独演示。

## 二、项目结构和文件功能

实践三相关文件均位于 `submission/proj3` 下，目录结构如下：

```text
submission/proj3/
├── Makefile
├── course/
│   ├── Appendix.pdf
│   ├── Project_3.pdf
│   ├── Tests_3.zip
│   └── irsim.zip
├── docs/
│   ├── cmm-grammar.md
│   ├── practice3-ir.md
│   ├── practice3-ir-report.md
│   └── practice3-ir-report.docx
├── src/
│   ├── ast.c
│   ├── ast.h
│   ├── irgen.c
│   ├── irgen.h
│   ├── lexer.l
│   ├── lexer_main.c
│   ├── parser.y
│   ├── parser_main.c
│   ├── semantic.c
│   ├── semantic.h
│   └── token.h
├── test/
│   ├── proj3-4-3-6-exp1-1.cmm
│   ├── proj3-4-3-6-exp2-1.cmm
│   ├── proj3-4-3-7-exp1-0.cmm
│   ├── proj3-4-3-7-exp2-0.cmm
│   └── proj3-test1.cmm
└── tools/
    ├── build_irsim_image.sh
    ├── check_env.sh
    ├── run_ir_tests.sh
    ├── run_irsim_tests.sh
    └── run_one_irsim_case.sh
```

各文件功能如下：

- `Makefile`：实践三独立构建入口。`make ir` 构建 IR 生成器，`make ir-demo` 运行公开样例并打印生成的 IR。
- `src/irgen.c`：实践三核心实现，包含 IR 类型系统、局部符号表、结构体字段偏移、函数签名收集、表达式翻译、语句翻译和文件输出。
- `src/irgen.h`：声明 IR 生成入口 `ir_generate(AstNode *root, const char *output_path)`。
- `src/parser_main.c`：主程序入口。默认输出 AST；设置 `CMM_SEMANTIC=1` 时只做语义检查；设置 `CMM_IR=1` 时先做语义检查，再调用 `ir_generate()` 输出 IR。
- `src/semantic.c`、`src/semantic.h`：复用实践二语义检查，保证只有语义正确的程序进入 IR 翻译。
- `src/parser.y`、`src/lexer.l`：复用前端，负责生成 IR 翻译所需 AST。
- `src/ast.c`、`src/ast.h`：AST 节点结构、节点创建、整数解析、先序打印和内存释放。
- `tools/run_ir_tests.sh`：运行 `Project_3.pdf` 中 4.3.6 和 4.3.7 的公开样例，生成 IR 文件并打印前 120 行。
- `tools/build_irsim_image.sh`：构建 Python 3.8 + PyQt5 的 IRSim Docker 镜像。
- `tools/run_one_irsim_case.sh`：生成并运行单个当前样例的 IR，打印 IRSim 控制台输出。
- `tools/run_irsim_tests.sh`：批量生成并运行当前样例 IR，检查 IRSim 输出是否符合预期。
- `test/`：按真实章节号命名的公开样例，以及手写综合样例 `proj3-test1.cmm`。

## 三、在前两次实践基础上的修改和新增内容

实践三是在实践一、实践二基础上的后续扩展，主要新增和修改如下：

1. 新增 `src/irgen.h`，声明中间代码生成入口。
2. 新增 `src/irgen.c`，实现 AST 到三地址中间代码的翻译。
3. 修改 `src/parser_main.c`，增加 `CMM_IR=1` 模式。该模式要求命令行传入输入 `.cmm` 文件和输出 `.ir` 文件。
4. 修改 `Makefile`，收敛为实践三当前目录可独立运行的目标，只保留 lexer、parser、semantic、ir 和演示相关入口。
5. 新增 `tools/run_ir_tests.sh`，使用当前 `test/` 目录中的公开样例生成并打印 IR。
6. 新增 `test/proj3-4-3-6-...` 和 `test/proj3-4-3-7-...` 样例文件，文件名中的 `4-3-6` 和 `4-3-7` 分别对应 `Project_3.pdf` 的必做样例和选做样例小节。
7. 新增 IRSim 单样例和批量验证脚本，使生成的 IR 可以直接用课程模拟器运行。
8. 删除与当前目录演示无关的旧批量测试和报告目录，使实践三目录只保留当前演示所需材料。

## 四、主要数据结构和函数功能

### 1. IR 类型系统

`src/irgen.c` 中使用 `IrTypeKind` 和 `IrType` 描述生成 IR 时需要的类型信息：

```text
IR_TY_INT     int 基本类型
IR_TY_FLOAT   float 基本类型
IR_TY_ARRAY   数组类型
IR_TY_STRUCT  结构体类型
```

相关函数如下：

- `new_type(IrTypeKind kind)`：创建指定种类的 IR 类型。
- `type_int()`、`type_float()`：返回基本类型对象。
- `array_of(IrType *elem, int size)`：构造数组类型。
- `struct_type(const char *name, IrField *fields)`：构造结构体类型。
- `type_size(IrType *type)`：计算类型占用字节数。基本类型为 4 字节；数组大小为元素大小乘以长度；结构体大小为字段大小之和。

### 2. 字段、符号、作用域和函数签名

IR 生成阶段维护轻量级符号信息：

- `IrField`：表示结构体字段或函数参数，保存字段名、类型、偏移和下一个字段。
- `IrSymbol`：表示局部变量或形参，保存名称、类型以及是否为地址参数。
- `IrScope`：表示嵌套作用域，保存当前作用域变量链表和父作用域。
- `IrFunc`：表示函数签名，保存函数名、返回类型和参数链表。

相关函数如下：

- `push_scope()`、`pop_scope()`：进入和退出作用域。
- `find_symbol(const char *name)`：从当前作用域向外查找变量。
- `add_symbol(const char *name, IrType *type, int is_addr_param)`：向当前作用域加入变量或形参。
- `find_func(const char *name)`：查找函数签名。
- `add_func(const char *name, IrType *ret, IrField *params)`：记录函数签名。
- `find_struct_def(const char *name)`：查找结构体定义。
- `find_field(IrField *fields, const char *name)`：查找结构体字段。

### 3. 输出和临时对象生成

- `emit(const char *fmt, ...)`：向输出文件写入一条 IR 指令。
- `new_temp()`：生成临时变量名，例如 `t1`、`t2`。
- `new_label()`：生成跳转标号，例如 `label1`、`label2`。
- `materialize_addr(char *addr)`：将 `&x` 形式的地址表达式必要时保存到临时变量，避免间接读写时生成不稳定格式。

### 4. 声明、类型和函数签名收集

- `read_vardec(AstNode *var_dec, VarInfo *out_info)`：从变量声明 AST 中提取变量名和数组维度。
- `apply_dims(IrType *base, const VarInfo *info)`：把数组维度应用到基础类型，得到完整变量类型。
- `analyze_specifier(AstNode *specifier)`：分析类型说明节点，返回对应 IR 类型。
- `analyze_struct_specifier(AstNode *node)`：分析结构体定义或结构体引用，并记录字段偏移。
- `collect_def_list()`、`collect_dec_list()`、`collect_dec()`：收集局部变量或结构体字段声明；对数组和结构体局部变量生成 `DEC` 指令。
- `collect_params(AstNode *var_list)`：收集函数参数类型。
- `collect_function_sigs(AstNode *node)`：翻译函数体前先遍历全局定义，记录所有函数签名，便于后续函数调用时判断参数传值或传地址。

### 5. 语句和表达式翻译

- `gen_ext_def_list()`、`gen_ext_def()`：翻译外部定义。函数定义会输出 `FUNCTION f :`，再翻译形参和函数体。
- `gen_params(IrField *params)`：输出 `PARAM` 指令，并将形参加入当前作用域。
- `gen_compst(AstNode *node, int create_scope)`：翻译复合语句，处理局部定义和语句列表。
- `gen_stmt_list()`、`gen_stmt()`：翻译表达式语句、复合语句、返回语句、条件语句和循环语句。
- `gen_exp(AstNode *node)`：翻译普通表达式，返回保存结果的变量名、临时变量或立即数。
- `gen_addr(AstNode *node, IrType **out_type)`：翻译左值地址，处理变量、数组元素和结构体字段。
- `gen_cond(AstNode *node, const char *true_label, const char *false_label)`：翻译布尔表达式和条件跳转。
- `gen_call(AstNode *node)`：翻译函数调用。普通函数生成 `ARG` 和 `CALL`；内置 `read` 生成 `READ`，内置 `write` 生成 `WRITE`。
- `collect_args()`、`emit_args_reverse()`：收集并逆序输出实参，满足课程 IR 中函数调用参数传递格式。

### 6. IR 生成总入口

`ir_generate(AstNode *root, const char *output_path)` 是实践三的核心入口，执行流程如下：

1. 打开输出 `.ir` 文件。
2. 初始化临时变量编号、标号编号、作用域、结构体表和函数表。
3. 创建全局作用域。
4. 遍历 AST 收集函数签名。
5. 遍历 AST 生成所有函数和语句的 IR。
6. 关闭输出文件并返回状态码。

## 五、IR 指令格式

当前实现支持的主要三地址指令如下：

```text
FUNCTION f :
PARAM x
LABEL label :
x := y
x := y + z
x := y - z
x := y * z
x := y / z
x := &y
x := *y
*x := y
GOTO label
IF x relop y GOTO label
ARG x
x := CALL f
READ x
WRITE x
RETURN x
DEC x size
```

数组和结构体通过地址计算实现。例如数组元素访问会先计算下标偏移，再使用 `&a + offset` 得到地址；结构体成员访问通过字段偏移定位成员地址。数组和结构体作为函数参数时按地址传递，普通整型参数按值传递。

## 六、构建和运行

所有命令均在当前实践目录下执行：

```sh
cd submission/proj3
```

检查环境：

```sh
make check-env
```

构建 IR 生成器：

```sh
make ir
```

该命令生成：

```text
build/cmmc
```

运行公开样例演示：

```sh
make ir-demo
```

也可以执行：

```sh
make ir-test
```

手动查看某个源文件并生成 IR：

```sh
nl -ba test/proj3-test1.cmm
CMM_IR=1 build/cmmc test/proj3-test1.cmm /tmp/proj3-test1.ir
sed -n '1,120p' /tmp/proj3-test1.ir
```

使用 IRSim 批量验证：

```sh
make irsim-test
```

使用 IRSim 单独验证手写样例：

```sh
./tools/run_one_irsim_case.sh proj3-test1
```

如果本机缺少 IRSim Docker 镜像，可先执行：

```sh
./tools/build_irsim_image.sh
```

## 七、结果与结论分析

执行：

```sh
make ir-demo
```

当前会运行 `Project_3.pdf` 中的公开样例：

- `test/proj3-4-3-6-exp1-1.cmm`：4.3.6 必做样例 1，生成符号函数相关 IR。
- `test/proj3-4-3-6-exp2-1.cmm`：4.3.6 必做样例 2，生成递归阶乘相关 IR。
- `test/proj3-4-3-7-exp1-0.cmm`：4.3.7 选做样例 1，生成结构体变量和结构体参数相关 IR。
- `test/proj3-4-3-7-exp2-0.cmm`：4.3.7 选做样例 2，生成一维数组参数和二维数组相关 IR。
- `test/proj3-test1.cmm`：手写样例，生成循环、函数调用和 `write` 相关 IR。

生成结果中可以看到 `FUNCTION`、`PARAM`、`LABEL`、`IF ... GOTO`、`ARG`、`CALL`、`READ`、`WRITE`、`RETURN`、`DEC` 等指令，说明函数、控制流、调用、数组和结构体相关翻译均已覆盖。

执行：

```sh
make irsim-test
```

当前 IRSim 验证结果为：

```text
irsim tests: 5 passed, 0 failed
```

本次实践完成了 C-- 到三地址中间代码的生成。程序能够在语义检查成功后输出符合课程格式的线性 IR，并支持必做内容以及结构体变量、结构体参数、一维数组参数和高维数组等选做内容。当前目录可以独立完成构建与演示，生成的 IR 可作为后续目标代码生成和优化实践的输入。
