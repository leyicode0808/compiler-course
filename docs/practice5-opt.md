# 实践 5：中间代码优化

## 构建

```sh
make opt
```

生成：

```text
build/cmm-opt
```

## 运行

实践五的输入和输出都是实践三格式的 IR 文本文件：

```sh
build/cmm-opt build/project3-ir/A-3.ir build/project5-ir/A-3.ir
```

程序逐行读取 IR，输出语义等价但更精简的 IR。它不重新解析 C-- 源程序，因此可以独立测试优化阶段。

## 测试

```sh
make opt-test
```

测试流程：

1. 先运行 `make ir-test`，为 Project 3 的 16 个样例生成原始 IR。
2. 使用 `build/cmm-opt` 生成优化后 IR，输出到 `build/project5-ir/`。
3. 使用课程 IRSim 运行优化后 IR，并比较输出结果。

当前结果：

```text
opt tests: 16 passed, 0 failed, saved 38 IR lines
```

完整回归入口：

```sh
make test
```

当前 `make test` 覆盖实践 1-5：词法、语法、语义、IRSim 验证、SPIM 验证和优化后 IRSim 验证。

## 实现说明

- `src/iropt.c`、`src/iropt.h`：文本 IR 优化器。
- `src/iropt_main.c`：实践五命令行入口。
- `tools/run_opt_tests.sh`：批量优化 Project 3 IR，并用 IRSim 验证行为。
- `examples/project5/`：保存实践五 PDF 中的三个必做公开 IR 样例片段。

## 优化范围

当前实现采用保守的优化策略，优先保证语义稳定：

- 常量传播：记录基本块内已知常量，将后续使用替换为 `#const`。
- 复制传播：记录 `x := y` 形式的简单别名，将后续使用替换为原值。
- 常量折叠：将 `#a + #b`、`#a - #b`、`#a * #b`、`#a / #b` 直接计算为常量。
- 局部公共子表达式消除：同一基本块内，若相同二元表达式的操作数未被重定义，则复用已有结果。
- 局部无用赋值删除：删除被下一次定义覆盖、且覆盖前未被使用的赋值；删除全函数内不再被使用的临时变量赋值。
- 全局无用赋值删除：为 IR 建立行级控制流后继关系，基于活跃变量分析删除 live-out 中不再活跃、且没有副作用的赋值。

为避免错误优化，遇到函数入口、标号、跳转、返回、函数调用等控制流或副作用边界时会清空局部传播状态。间接写 `*p := x`、函数调用和输入输出不会被删除。若变量出现过 `&x`，优化器会保守地保留对 `x` 的写入，避免别名访问导致语义变化。
