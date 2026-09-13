# 实践 5：中间代码优化

## 构建

在 `proj5` 目录下执行：

```sh
make opt
```

生成：

```text
build/cmm-opt
```

优化器的命令行格式为：

```sh
build/cmm-opt input.ir output.ir
```

## 单独优化指令

完整单独优化流程：

```sh
make opt
nl -ba test/proj5-test1.ir
build/cmm-opt test/proj5-test1.ir /tmp/proj5-test1.opt.ir
sed -n '1,160p' /tmp/proj5-test1.opt.ir
diff -u test/proj5-test1.ir /tmp/proj5-test1.opt.ir
```

查看手写 IR 样例：

```sh
nl -ba test/proj5-test1.ir
```

只优化手写样例：

```sh
build/cmm-opt test/proj5-test1.ir /tmp/proj5-test1.opt.ir
```

查看优化后的 IR：

```sh
sed -n '1,160p' /tmp/proj5-test1.opt.ir
```

查看优化前后差异：

```sh
diff -u test/proj5-test1.ir /tmp/proj5-test1.opt.ir
```

也可以替换为任意一个 `test/` 下的 IR 文件，例如：

```sh
build/cmm-opt test/proj5-6-3-6-exp1-1.ir /tmp/proj5-exp1.opt.ir
sed -n '1,160p' /tmp/proj5-exp1.opt.ir
diff -u test/proj5-6-3-6-exp1-1.ir /tmp/proj5-exp1.opt.ir
```

## 批量测试

只检查优化器能否处理当前 `test/` 目录下的 IR 样例：

```sh
make opt-demo
```

优化并使用 IRSim 比较优化前后输出：

```sh
make opt-test
```

如果本机没有 IRSim 的 Docker 镜像，先执行：

```sh
./tools/build_irsim_image.sh
```

`make opt-test` 会把优化结果写入：

```text
build/project5-opt/
```

## 测试样例命名

当前 `test/` 中的样例按实践五 PDF 的章节编号命名：

- `proj5-6-3-6-exp1-1.ir`：6.3.6 必做样例 1，局部公共子表达式消除。
- `proj5-6-3-6-exp2-1.ir`：6.3.6 必做样例 2，局部无用代码消除。
- `proj5-6-3-6-exp3-1.ir`：6.3.6 必做样例 3，常量折叠。
- `proj5-test1.ir`：手写样例，用于单独观察常量传播、折叠和无用赋值删除效果。

末尾的 `1` 表示必做内容；若后续补充 6.3.7 的选做样例，可使用 `proj5-6-3-7-expN-0.ir`。

## 文件结构

- `src/iropt.c`：文本 IR 优化器主体，实现传播、折叠、公共子表达式消除和无用赋值删除。
- `src/iropt.h`：优化器对外接口，声明 `ir_optimize_file`。
- `src/iropt_main.c`：命令行入口，检查参数并调用优化器。
- `test/`：实践五当前验收使用的 IR 输入样例。
- `tools/run_opt_tests.sh`：批量优化 `test/*.ir`，并可调用 IRSim 比较优化前后输出。
- `tools/build_irsim_image.sh`：构建运行 IRSim 所需的 Python 3.8 + PyQt5 Docker 镜像。
- `docs/`：实践五说明文档和报告。
- `course/`：课程 PDF、IRSim 压缩包和参考材料。

## 优化范围

当前实现采用保守策略，优先保证语义正确：

- 常量传播：记录基本块内变量的已知常量，后续使用处替换为 `#const`。
- 复制传播：记录 `x := y` 形式的简单复制关系，减少无意义中转变量。
- 常量折叠：将两个常量组成的二元表达式在优化阶段直接计算。
- 局部公共子表达式消除：同一基本块中复用已经计算过且操作数未被重新定义的二元表达式。
- 局部无用赋值删除：删除被覆盖前没有被使用的赋值。
- 全局无用赋值删除：建立行级控制流后继关系，通过活跃变量分析删除没有副作用且结果不再活跃的赋值。

遇到函数入口、标号、跳转、条件跳转、返回和函数调用时，优化器会清空局部传播状态。对间接写、输入输出、函数调用和地址相关变量采用保守处理，避免因为别名或副作用改变程序行为。
