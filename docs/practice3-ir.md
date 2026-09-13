# 实践 3：中间代码生成

## 构建

```sh
make ir
```

生成：

```text
build/cmmc
```

## 运行

IR 生成模式通过环境变量开启，并额外传入输出文件：

```sh
CMM_IR=1 build/cmmc tests/project3/inputs/A-1.cmm build/project3-ir/A-1.ir
```

程序会先完成词法、语法和语义检查；若没有错误，则把中间代码写入指定 `.ir` 文件。

## 查看运行结果

实践三本身输出的是中间代码文件。验收时通常分两步看：

1. 先查看编译器生成的 IR 文本。
2. 再用课程提供的 IRSim 执行该 IR，查看程序实际输出。

以最大公约数样例 `C-1` 为例：

```sh
make ir
CMM_IR=1 build/cmmc tests/project3/inputs/C-1.cmm /tmp/C-1.ir
cat /tmp/C-1.ir
```

如果要把该样例放入批量测试目录，并用课程 IRSim 查看运行输出：

```sh
make ir-test
./tools/run_one_irsim_case.sh C-1
```

`C-1` 的测试输入由脚本提供，相当于输入：

```text
48
18
```

期望 IRSim 输出：

```text
6
```

也可以查看其他样例的实际输出，例如：

```sh
./tools/run_one_irsim_case.sh A-5
./tools/run_one_irsim_case.sh B-1
./tools/run_one_irsim_case.sh E2-3
```

这些命令会直接打印 IRSim 控制台中的输出值，而不是只显示 `ok`。

## 测试

```sh
make ir-test
```

当前会为 `tests/project3/inputs/` 中全部 16 个样例生成 IR：

- A/B/C：必做内容，包含整数变量、读写、赋值、算术表达式、条件、循环、函数调用、一维数组。
- E1：要求 4.1，包含结构体变量、结构体字段访问、结构体参数。
- E2：要求 4.2，包含高维数组、一维数组参数和数组地址传参。

当前结果：

```text
ir tests: 16 generated, 0 failed
```

课程包内的 `irsim.pyc` 是 Python 3.8 字节码，并依赖 PyQt5。当前通过 Docker 镜像 `cmm-irsim-py38` 提供 Python 3.8、PyQt5 和 Qt 运行依赖，可使用官方解释器验证生成的 IR：

```sh
./tools/build_irsim_image.sh
make irsim-test
```

当前结果：

```text
irsim tests: 16 passed, 0 failed
```

## 实现说明

- `src/irgen.c`、`src/irgen.h`：IR 类型系统、局部符号表、结构体字段偏移、函数签名收集和 AST 到三地址代码的翻译。
- `src/parser_main.c`：默认保持实践 1.2 的语法树输出；设置 `CMM_IR=1` 时执行语义检查并生成 IR。
- `src/semantic.c`：增加内置函数 `read` 和 `write`，满足实践 3 的输入输出函数约定。
- `tools/build_irsim_image.sh`：构建 Python 3.8 + PyQt5 的本地 Docker 镜像。
- `tools/run_ir_tests.sh`：批量生成 Project 3 样例 IR。
- `tools/run_one_irsim_case.sh`：执行单个 Project 3 样例 IR，并打印 IRSim 实际输出。
- `tools/run_irsim_tests.sh`：在 Python 3.8 Docker 环境中调用课程 `IRSim`，验证生成 IR 的实际运行输出。

完整回归入口：

```sh
make test
```

支持的主要 IR 指令：

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
