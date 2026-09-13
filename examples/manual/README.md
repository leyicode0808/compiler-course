# 手写验收代码目录

这个目录用于存放现场或手动验收时临时编写的 C-- 程序。

建议把源码保存为 `.cmm`，生成产物放到 `/tmp` 或 `build/manual/`，避免污染源码目录。

一键查看实践 1-5 主要结果：

```sh
./tools/run_manual_pipeline.sh examples/manual/gcd.cmm '48\n18\n'
./tools/run_manual_pipeline.sh examples/manual/array_stats.cmm '5\n3\n1\n4\n1\n5\n'
```

该命令会依次打印词法 token、语法树、语义检查结果、IR、优化 IR、MIPS 汇编和最终 SPIM 输出。

常用命令：

```sh
# 输出语法树
build/cmmc examples/manual/gcd.cmm

# 生成 IR
CMM_IR=1 build/cmmc examples/manual/gcd.cmm /tmp/gcd.ir

# 优化 IR
build/cmm-opt /tmp/gcd.ir /tmp/gcd.opt.ir

# 生成 MIPS
build/cmmc examples/manual/gcd.cmm /tmp/gcd.s

# 运行 MIPS
printf '48\n18\n' | spim -quiet -file /tmp/gcd.s
```

`gcd.cmm` 的期望输出：

```text
6
```

`array_stats.cmm` 覆盖数组、循环、条件、函数调用和递归。输入为数组长度 5，以及元素 3、1、4、1、5：

```text
5
3
1
4
1
5
```

期望输出：

```text
14
5
106
```

## 命令解释

```sh
CMM_IR=1 build/cmmc examples/manual/gcd.cmm /tmp/gcd.ir
```

这一行的含义：

- `CMM_IR=1`：给当前这次命令设置环境变量，告诉 `build/cmmc` 进入“生成 IR”模式。
- `build/cmmc`：主编译器程序。
- `examples/manual/gcd.cmm`：输入文件，也就是 C-- 源程序。
- `/tmp/gcd.ir`：输出文件，也就是生成的三地址中间代码。

如果没有 `CMM_IR=1`，两个参数运行时默认是实践四模式，会生成 MIPS 汇编：

```sh
build/cmmc examples/manual/gcd.cmm /tmp/gcd.s
```

这一行会把 C-- 源程序编译成 MIPS32 汇编，输出到 `/tmp/gcd.s`。

```sh
printf '48\n18\n' | spim -quiet -file /tmp/gcd.s
```

这一行的含义：

- `printf '48\n18\n'`：向标准输出打印两行输入数据，等价于你手动输入 `48` 回车，再输入 `18` 回车。
- `|`：管道，把左边命令的输出作为右边命令的输入。
- `spim`：MIPS32 模拟器。
- `-quiet`：减少 SPIM 自身提示信息。
- `-file /tmp/gcd.s`：让 SPIM 执行这个 MIPS 汇编文件。

所以整行命令的意思是：把 `48` 和 `18` 作为程序输入，交给 SPIM 运行 `/tmp/gcd.s`。

## 推荐手动验收流程

从 C-- 源程序开始，依次查看每个阶段的产物：

```sh
# 1. 输出语法树
build/cmmc examples/manual/gcd.cmm

# 2. 生成并查看 IR
CMM_IR=1 build/cmmc examples/manual/gcd.cmm /tmp/gcd.ir
cat /tmp/gcd.ir

# 3. 优化 IR，并查看优化前后差异
build/cmm-opt /tmp/gcd.ir /tmp/gcd.opt.ir
diff -u /tmp/gcd.ir /tmp/gcd.opt.ir

# 4. 生成并查看 MIPS
build/cmmc examples/manual/gcd.cmm /tmp/gcd.s
cat /tmp/gcd.s

# 5. 用 SPIM 运行 MIPS
printf '48\n18\n' | spim -quiet -file /tmp/gcd.s
```

最后一步期望输出：

```text
6
```
