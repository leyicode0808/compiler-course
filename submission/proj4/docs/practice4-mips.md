# 实践 4：目标代码生成

## 构建

所有命令均在当前实践目录下执行：

```sh
cd submission/proj4
```

构建 MIPS 生成器：

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
build/cmmc test/proj4-5-3-6-exp1-1.cmm /tmp/proj4-exp1.s
```

程序会先完成词法、语法和语义检查，再生成临时 IR，最后把 IR 翻译为可在 SPIM 中运行的 MIPS32 汇编。一参数运行时保留实践 1.2 的语法树输出。

## 演示

```sh
make mips-demo
```

也可以执行：

```sh
make mips-test
```

当前演示使用 `Project_4.pdf` 中的公开样例：

- `test/proj4-5-3-6-exp1-1.cmm`：5.3.6 必做样例 1，Fibonacci 数列。
- `test/proj4-5-3-6-exp2-1.cmm`：5.3.6 必做样例 2，递归阶乘。
- `test/proj4-test1.cmm`：手写样例，循环和函数调用。

手动查看手写样例源码、生成 MIPS 汇编并运行：

```sh
nl -ba test/proj4-test1.cmm
build/cmmc test/proj4-test1.cmm /tmp/proj4-test1.s
sed -n '1,160p' /tmp/proj4-test1.s
spim -quiet -file /tmp/proj4-test1.s
```

## 实现说明

- `src/mipsgen.c`、`src/mipsgen.h`：读取实践 3 生成的线性 IR，并翻译为 MIPS32 汇编。
- `src/parser_main.c`：两参数模式下先调用 `ir_generate()` 生成临时 IR，再调用 `mips_generate_from_ir()` 生成 `.s` 文件。
- `src/irgen.c`、`src/irgen.h`：目标代码生成前的 IR 生成。
- `src/semantic.c`、`src/semantic.h`：在生成 IR 和 MIPS 前做语义检查。
- `src/parser.y`、`src/lexer.l`：复用前端，负责生成后端所需 AST。
- `tools/run_mips_tests.sh`：运行当前 `test/` 目录下按 `Project_4.pdf` 章节命名的公开样例，打印生成的汇编和 SPIM 实际输出。

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

支持的主要 IR 指令包括：

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
