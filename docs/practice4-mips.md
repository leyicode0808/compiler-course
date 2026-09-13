# 实践 4：目标代码生成

## 构建

```sh
make mips
```

生成：

```text
build/cmmc
```

## 运行

实践四提交形式使用两个参数：输入 C-- 文件和输出汇编文件。

```sh
build/cmmc tests/project3/inputs/A-5.cmm build/project4-mips/A-5.s
```

程序会先完成词法、语法和语义检查，再生成临时 IR，最后把 IR 翻译为可在 SPIM 中运行的 MIPS32 汇编。仍可显式设置 `CMM_MIPS=1` 使用同一模式；一参数运行时保留实践 1.2 的语法树输出。

## 查看运行结果

实践四的输出是 MIPS32 汇编文件。验收时通常分两步看：

1. 先查看生成的 `.s` 汇编文件。
2. 再用 SPIM 执行汇编，查看程序实际输出。

以最大公约数样例 `C-1` 为例：

```sh
make mips
build/cmmc tests/project3/inputs/C-1.cmm /tmp/C-1.s
cat /tmp/C-1.s
printf '48\n18\n' | spim -quiet -file /tmp/C-1.s
```

期望 SPIM 输出：

```text
6
```

如果只想直接看某个内置样例的运行输出，可以使用单样例脚本：

```sh
./tools/run_one_mips_case.sh C-1
./tools/run_one_mips_case.sh A-5
./tools/run_one_mips_case.sh P4-factorial
./tools/run_one_mips_case.sh P4-fibonacci
```

这些命令会先生成对应 `.s` 文件，再调用 `spim`，直接打印程序输出，而不是只显示 `ok`。

## 测试

```sh
make mips-test
```

测试脚本会对 Project 3 的 16 个样例以及 Project 4 PDF 中的 2 个公开样例生成汇编，并调用本机 `spim` 执行，比较输出整数序列。

当前结果：

```text
mips tests: 18 passed, 0 failed
```

完整回归入口：

```sh
make test
```

当前 `make test` 覆盖实践 1-4：词法、语法、语义、IRSim 验证和 SPIM 验证。

## 实现说明

- `src/mipsgen.c`、`src/mipsgen.h`：读取实践 3 生成的线性 IR，并翻译为 MIPS32 汇编。
- `src/parser_main.c`：新增 `CMM_MIPS=1` 模式。该模式先调用 `ir_generate()` 生成临时 IR，再调用 `mips_generate_from_ir()` 生成 `.s` 文件。
- `tools/run_one_mips_case.sh`：执行单个实践四样例，生成汇编并打印 SPIM 实际输出。
- `tools/run_mips_tests.sh`：批量生成汇编，使用 `spim -quiet -file` 运行，并比较输出。
- `examples/project4/`：保存实践四 PDF 中的 Fibonacci 和递归阶乘两个公开样例。
- `Makefile`：新增 `mips` 和 `mips-test` 目标，并将实践 4 纳入 `make test`。

## 后端策略

当前采用朴素但稳定的栈式后端：

- 每个函数建立独立栈帧。
- 局部变量、临时变量、数组和结构体展开后的存储空间都放在当前函数活动记录中。
- `DEC x size` 为数组或结构体变量分配连续字节空间。
- `&x` 生成相对 `$fp` 的地址。
- `*p` 和 `*p := x` 翻译为 MIPS 的间接 `lw`、`sw`。
- `ARG` 先按 IR 顺序压栈，`CALL` 后由调用者回收参数空间。
- 被调函数在入口把传入参数复制到自己的局部槽位，因此递归调用不会覆盖上一层调用的数据。
- `READ` 使用 syscall 5，`WRITE` 使用 syscall 1 并额外输出换行。

支持的 IR 指令包括：

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
