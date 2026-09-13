# 实践 3：中间代码生成

## 构建

所有命令均在当前实践目录下执行：

```sh
cd submission/proj3
```

构建 IR 生成器：

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
CMM_IR=1 build/cmmc test/proj3-4-3-6-exp1-1.cmm /tmp/proj3-exp1.ir
```

程序会先完成词法、语法和语义检查；若没有错误，则把中间代码写入指定 `.ir` 文件。

## 演示

```sh
make ir-demo
```

也可以执行：

```sh
make ir-test
```

当前演示使用 `Project_3.pdf` 中的公开样例：

- `test/proj3-4-3-6-exp1-1.cmm`：4.3.6 必做样例 1，符号函数。
- `test/proj3-4-3-6-exp2-1.cmm`：4.3.6 必做样例 2，递归阶乘。
- `test/proj3-4-3-7-exp1-0.cmm`：4.3.7 选做样例 1，结构体变量和结构体参数。
- `test/proj3-4-3-7-exp2-0.cmm`：4.3.7 选做样例 2，一维数组参数和二维数组。
- `test/proj3-test1.cmm`：手写样例，循环和函数调用。

手动查看某个源文件并生成 IR：

```sh
nl -ba test/proj3-test1.cmm
CMM_IR=1 build/cmmc test/proj3-test1.cmm /tmp/proj3-test1.ir
sed -n '1,120p' /tmp/proj3-test1.ir
```

## IRSim 验证

当前目录也提供基于课程 IRSim 的单独和批量验证指令。Docker 镜像 `cmm-irsim-py38` 已用于运行 Python 3.8 和 PyQt5 环境；如果本机缺少该镜像，可先执行：

```sh
./tools/build_irsim_image.sh
```

批量生成 IR 并用 IRSim 运行当前 `test/` 样例：

```sh
make irsim-test
```

单独运行手写样例：

```sh
./tools/run_one_irsim_case.sh proj3-test1
```

也可以指定公开样例：

```sh
./tools/run_one_irsim_case.sh proj3-4-3-6-exp1-1
```

## 实现说明

- `src/irgen.c`、`src/irgen.h`：IR 类型系统、局部符号表、结构体字段偏移、函数签名收集和 AST 到三地址代码的翻译。
- `src/parser_main.c`：默认保持实践 1.2 的语法树输出；设置 `CMM_SEMANTIC=1` 时执行语义检查；设置 `CMM_IR=1` 时执行语义检查并生成 IR。
- `src/semantic.c`、`src/semantic.h`：在生成 IR 前做语义检查，避免错误程序进入翻译阶段。
- `src/parser.y`、`src/lexer.l`：复用前端，负责生成 IR 翻译所需 AST。
- `tools/run_ir_tests.sh`：运行当前 `test/` 目录下按 `Project_3.pdf` 章节命名的公开样例，并打印生成的 IR。
- `tools/build_irsim_image.sh`：构建 Python 3.8 + PyQt5 的 IRSim Docker 镜像。
- `tools/run_one_irsim_case.sh`：生成并运行单个当前样例的 IR，打印 IRSim 控制台输出。
- `tools/run_irsim_tests.sh`：批量生成并运行当前样例 IR，检查 IRSim 输出是否符合预期。

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
