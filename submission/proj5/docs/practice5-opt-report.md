# 编译原理课程设计报告：实践 5 中间代码优化

课程设计名称：编译原理课程设计

实践内容：实践 5 中间代码优化

## 一、目的和任务

本次实践的目标是在已经生成的中间代码基础上，对 IR 进行语义保持的优化，使代码更精简，并减少后续解释执行或目标代码生成需要处理的无效指令。

主要任务如下：

1. 实现独立 IR 优化器，输入 `.ir` 文件，输出优化后的 `.ir` 文件。
2. 支持常量传播、复制传播、常量折叠、局部公共子表达式消除和无用赋值删除。
3. 对函数调用、输入输出、控制流、数组地址和间接访问采用保守处理，保证优化前后语义一致。
4. 使用实践五 PDF 的 6.3.6 必做 IR 样例和手写样例进行验收，并用 IRSim 比较优化前后的运行输出。

## 二、目录结构

实践五目录按当前提交包独立整理，主要文件如下：

```text
proj5/
  Makefile
  note.txt
  course/
  docs/
  src/
  test/
  tools/
```

各目录职责如下：

- `Makefile`：实践五入口，只保留优化器相关命令，包括 `make opt`、`make opt-demo` 和 `make opt-test`。
- `course/`：保存课程 PDF、IRSim 压缩包和参考优化器压缩包。
- `docs/`：保存实践五说明文档和本报告的 Markdown、Word 版本。
- `src/`：保存编译器源码，其中本实践新增和重点修改的是优化器相关文件。
- `test/`：保存当前实践五验收使用的 IR 样例。
- `tools/`：保存环境检查、IRSim 镜像构建和批量测试脚本。

当前 `test/` 中的样例命名对应实践五 PDF 的章节：

- `proj5-6-3-6-exp1-1.ir`：6.3.6 必做样例 1，局部公共子表达式消除。
- `proj5-6-3-6-exp2-1.ir`：6.3.6 必做样例 2，局部无用代码消除。
- `proj5-6-3-6-exp3-1.ir`：6.3.6 必做样例 3，常量折叠。
- `proj5-test1.ir`：手写样例，用于观察常量传播、常量折叠和无用赋值删除。

命名末尾的 `1` 表示必做内容；若后续加入 6.3.7 选做样例，可使用 `proj5-6-3-7-expN-0.ir`。

## 三、构建与测试方法

在 `proj5` 目录下构建优化器：

```sh
make opt
```

生成的可执行文件为：

```text
build/cmm-opt
```

优化器的单独运行格式为：

```sh
build/cmm-opt input.ir output.ir
```

单独优化手写样例的完整指令为：

```sh
make opt
nl -ba test/proj5-test1.ir
build/cmm-opt test/proj5-test1.ir /tmp/proj5-test1.opt.ir
sed -n '1,160p' /tmp/proj5-test1.opt.ir
diff -u test/proj5-test1.ir /tmp/proj5-test1.opt.ir
```

其中第一行用于构建优化器，第二行用于查看原始 IR，第三行调用优化器生成优化后 IR，第四行查看优化结果，第五行用统一 diff 格式查看优化前后的删改内容。也可以将输入文件替换为任意一个 `test/` 下的 IR 样例，例如：

```sh
build/cmm-opt test/proj5-6-3-6-exp1-1.ir /tmp/proj5-exp1.opt.ir
sed -n '1,160p' /tmp/proj5-exp1.opt.ir
diff -u test/proj5-6-3-6-exp1-1.ir /tmp/proj5-exp1.opt.ir
```

批量优化当前 `test/` 下的样例：

```sh
make opt-demo
```

批量优化并用 IRSim 比较优化前后输出：

```sh
make opt-test
```

如果本机还没有 IRSim 使用的 Docker 镜像，先执行：

```sh
./tools/build_irsim_image.sh
```

优化后的 IR 会输出到：

```text
build/project5-opt/
```

## 四、源码文件和函数功能

### 1. `src/iropt_main.c`

该文件是优化器的命令行入口。`main` 函数检查参数数量，要求输入文件和输出文件两个参数，然后调用 `ir_optimize_file(input, output)`。因此实践五可以不重新解析 C-- 源文件，直接以文本 IR 作为输入进行单独测试。

### 2. `src/iropt.h`

该头文件声明优化器对外接口：

```c
int ir_optimize_file(const char *input_path, const char *output_path);
```

其他模块只需要包含该头文件即可调用优化器。

### 3. `src/iropt.c`

该文件是实践五的核心实现，主要功能分为以下几类：

- IR 行管理：`IrLine`、`LineVec`、`line_push`、`read_lines`、`write_lines` 负责读取、保存和写回文本 IR。
- 字符串和解析辅助：`trim`、`starts_with`、`parse_assign`、`parse_binary`、`parse_simple_operand` 用于识别赋值、二元表达式和操作数。
- 使用和定义分析：`line_uses_var`、`line_defines_var`、`collect_line_use_def` 识别每行 IR 读取和定义了哪些变量。
- 传播表维护：`Binding`、`BindVec`、`bind_get`、`bind_set`、`bind_kill` 保存基本块内的常量和复制关系。
- 表达式表维护：`ExprEntry`、`ExprVec`、`expr_find`、`expr_add`、`expr_kill` 保存基本块内已经计算过的二元表达式。
- 表达式规范化：`normalize_expr` 对加法和乘法的操作数排序，使 `a + b` 与 `b + a` 能识别为同一表达式。
- 指令改写：`substitute_operand`、`eval_binary`、`rewrite_assignment`、`rewrite_non_assignment`、`rewrite_pass` 实现常量传播、复制传播、常量折叠和局部公共子表达式消除。
- 局部无用赋值删除：`assignment_has_side_effect`、`can_remove_assignment`、`dce_pass` 删除基本块内被覆盖且未使用的赋值。
- 全局无用赋值删除：`line_successors`、集合操作函数和 `global_dce_pass` 建立行级控制流后继关系，进行活跃变量分析，删除结果不再活跃且无副作用的赋值。
- 总控入口：`ir_optimize_file` 读取输入 IR，循环执行改写和删除过程，最后写出优化后的 IR。

## 五、修改和添加内容

相对于前一阶段仅生成 IR 的代码，本实践主要增加和修改了以下内容：

1. 新增 `src/iropt.c`，实现独立文本 IR 优化器。
2. 新增 `src/iropt.h`，提供优化器接口声明。
3. 新增 `src/iropt_main.c`，提供 `build/cmm-opt` 命令行程序入口。
4. 修改 `Makefile`，增加 `make opt`、`make opt-demo`、`make opt-test`，并移除与当前实践无关的验收入口。
5. 新增 `test/` 下的实践五 IR 样例，按 6.3.6 必做章节编号命名，并补充手写样例。
6. 修改 `tools/run_opt_tests.sh`，使其只读取当前 `test/*.ir`，生成 `build/project5-opt/*.opt.ir`，并可用 IRSim 对比优化前后的运行输出。
7. 保留 `tools/build_irsim_image.sh`，用于构建 IRSim 运行所需 Docker 镜像。
8. 将报告和说明文档整理到 `docs/`，删除旧的测试目录和报告目录引用。

## 六、优化方法

### 1. 常量传播和复制传播

优化器按基本块扫描 IR，在块内维护变量到已知值的映射。例如 `v1 := #10` 会记录 `v1` 的值为 `#10`，后续使用 `v1` 的位置可以替换为常量。对于 `v2 := v1`，如果 `v1` 已经有已知值，也会继续传播。

### 2. 常量折叠

当二元表达式的两个操作数都是常量时，优化器直接计算结果。例如：

```text
t1 := #10 + #20
```

可优化为：

```text
t1 := #30
```

除法只在除数非零时折叠，避免改变运行时行为。

### 3. 局部公共子表达式消除

同一基本块内，如果再次出现已经计算过的二元表达式，且相关变量没有被重新定义，则复用已有结果。对加法和乘法，优化器会先规范化操作数顺序，因此交换律形式也能被识别。

### 4. 无用赋值删除

局部删除阶段会删除被后续定义覆盖、且覆盖前未被使用的赋值。全局删除阶段会进行活跃变量分析，如果一条赋值的左值在后续已经不再活跃，并且该赋值没有副作用，则删除该行。

输入输出、函数调用、返回、参数传递、间接写和控制流指令都不会被删除。若变量被取地址，优化器会保守地保留相关写入，避免间接访问导致语义错误。

## 七、结果与结论

执行：

```sh
make opt-demo
```

可以看到 `test/` 中 4 个 IR 样例都能生成优化后文件。执行：

```sh
make opt-test
```

脚本会先优化当前样例，再使用 IRSim 分别运行原始 IR 和优化后 IR，并比较两者输出是否一致。当前结果为：

```text
irsim opt tests: 4 passed, 0 failed, saved 54 IR lines
```

该流程验证了当前优化在样例范围内保持程序语义。

本次实践完成了一个保守的 IR 优化器。它能够处理常量传播、复制传播、常量折叠、局部公共子表达式消除和无用赋值删除，并通过当前实践五样例验证优化前后输出一致。后续若继续扩展 6.3.7 选做内容，可以在现有框架上加入跨基本块公共子表达式消除、常量传播、全局无用代码删除、循环不变式外提和强度削弱。
