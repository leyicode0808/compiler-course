# 编译原理课程设计报告：实践 3 中间代码生成

课程设计名称：编译原理课程设计

实践内容：实践 3 中间代码生成

## 一、目的和任务

本次实践的目标是在词法分析、语法分析和语义分析完成的基础上，将 C-- 源程序翻译为线性中间代码。中间代码采用三地址代码形式，用于描述赋值、算术运算、条件跳转、函数调用、参数传递、数组访问、结构体字段访问和输入输出操作。

主要任务如下：

1. 在语义检查通过后遍历 AST，生成中间代码文件。
2. 生成临时变量和跳转标号，翻译表达式、语句和控制流。
3. 翻译函数定义、形参、实参、函数调用和返回语句。
4. 翻译局部数组和结构体变量的空间申请、地址计算和间接读写。
5. 特殊处理内置函数 `read` 和 `write`，生成 `READ`、`WRITE` 指令。
6. 对 Project 3 的样例输入批量生成 IR 并进行验证。

## 二、过程

### 1. IR 生成入口

为了不破坏前两个实践的运行方式，程序默认仍保持语法树输出；实践 2 使用 `CMM_SEMANTIC=1` 进入语义分析模式；实践 3 使用 `CMM_IR=1` 进入 IR 生成模式，并额外接收输出文件路径：

```sh
CMM_IR=1 build/cmmc tests/project3/inputs/A-1.cmm build/project3-ir/A-1.ir
```

IR 模式下，程序先执行词法、语法和语义检查。只有输入无错误时才调用 `ir_generate(parse_root, output_path)` 输出中间代码。

### 2. 类型和符号信息

新增 `src/irgen.c` 和 `src/irgen.h`。IR 生成阶段单独维护一套轻量级类型与符号信息：

- `IrType`：表示 `int`、`float`、数组和结构体。
- `IrField`：表示结构体字段或函数参数，并记录字段偏移。
- `IrSymbol`：表示当前作用域中的变量，记录变量类型以及是否为地址参数。
- `IrFunc`：表示函数签名，用于判断实参传值还是传地址。

结构体字段偏移和数组元素宽度通过类型大小递归计算。基本类型大小为 4 字节，数组大小为元素大小乘以长度，结构体大小为所有字段大小之和。

### 3. 表达式和语句翻译

表达式翻译返回一个可用于 IR 的地址字符串，例如变量名、临时变量或立即数。普通算术表达式生成：

```text
t1 := x + y
```

赋值语句根据左值形式区分直接赋值和间接赋值：

```text
x := t1
*t2 := t1
```

数组访问和结构体字段访问先生成地址，再通过 `*addr` 读写内存：

```text
t1 := i * #4
t2 := &a + t1
t3 := *t2
```

条件语句和循环语句通过 `LABEL`、`GOTO`、`IF ... GOTO` 翻译。布尔表达式在控制流上下文中直接生成跳转代码，在普通表达式上下文中转换为 `0` 或 `1`。

### 4. 函数和参数翻译

函数定义输出：

```text
FUNCTION f :
PARAM x
```

普通整型参数按值传递；数组和结构体参数按地址传递。调用函数时先逆序输出 `ARG`，再输出 `CALL`：

```text
ARG y
ARG x
t1 := CALL f
```

内置函数特殊处理：

```text
READ t1
WRITE x
```

### 5. 测试脚本

新增 `tools/run_ir_tests.sh`，对 Project 3 的 16 个样例逐个生成 `.ir` 文件，并检查输出非空且包含 `FUNCTION` 指令。生成目录为：

```text
build/project3-ir/
```

## 三、结果与结论分析

### 1. 测试结果

执行：

```sh
make ir-test
```

输出：

```text
ok: A-1 -> build/project3-ir/A-1.ir
ok: A-2 -> build/project3-ir/A-2.ir
ok: A-3 -> build/project3-ir/A-3.ir
ok: A-4 -> build/project3-ir/A-4.ir
ok: A-5 -> build/project3-ir/A-5.ir
ok: B-1 -> build/project3-ir/B-1.ir
ok: B-2 -> build/project3-ir/B-2.ir
ok: B-3 -> build/project3-ir/B-3.ir
ok: C-1 -> build/project3-ir/C-1.ir
ok: C-2 -> build/project3-ir/C-2.ir
ok: E1-1 -> build/project3-ir/E1-1.ir
ok: E1-2 -> build/project3-ir/E1-2.ir
ok: E1-3 -> build/project3-ir/E1-3.ir
ok: E2-1 -> build/project3-ir/E2-1.ir
ok: E2-2 -> build/project3-ir/E2-2.ir
ok: E2-3 -> build/project3-ir/E2-3.ir
ir tests: 16 generated, 0 failed
```

同时回归前两部分：

```text
lexer tests: 6 passed, 0 failed
parser tests: 20 passed, 0 failed
semantic tests: 30 passed, 0 failed
```

课程提供的 `irsim.pyc` 是 Python 3.8 字节码，并依赖 PyQt5。当前通过 `tools/build_irsim_image.sh` 构建 `cmm-irsim-py38` 镜像，提供 Python 3.8、PyQt5 和 Qt 运行依赖。测试脚本通过截取 `irsim.pyc` 中的类定义部分，绕过 GUI 主事件循环，直接调用 `IRSim.loadFile()` 和 `IRSim.run()`，并用 monkeypatch 为 `READ` 指令提供测试输入。

执行：

```sh
make irsim-test
```

输出：

```text
irsim tests: 16 passed, 0 failed
```

### 2. 问题和解决

实现中的重点是左值地址和右值求值的区分。变量赋值可以直接生成 `x := y`，但数组元素和结构体字段需要先计算地址，再生成 `*addr := y` 或 `x := *addr`。为了避免生成 `*&x` 这类不稳定格式，若地址表达式是 `&x`，会先将其保存到临时变量再进行间接访问。

另一个问题是函数参数传递。普通整型参数直接传值，数组和结构体参数传地址。IR 生成阶段会先收集函数签名，在翻译函数调用时根据形参类型决定实参使用值还是地址。

### 3. 结论

本次实践完成了 C-- 到三地址中间代码的生成。程序支持 Project 3 样例中的必做内容，并扩展支持结构体变量、结构体参数、高维数组和一维数组参数。生成的中间代码格式符合课程说明，可作为后续目标代码生成和优化实践的输入。
