# 实践 1.2：Bison 语法分析

## 构建

```sh
make parser
```

生成：

```text
build/cmmc
```

## 运行

```sh
build/cmmc tests/project1/inputs/C-1.cmm
```

若输入无词法或语法错误，程序按先序遍历输出语法树。若存在词法或语法错误，只输出错误信息，不输出语法树。

## 测试

```sh
make parser-test
```

当前测试覆盖 `tests/project1/expects/` 中所有已有期望输出：

- A 组：单个词法或语法错误
- B 组：多个语法错误与错误恢复
- C 组：结构体、数组、函数、循环等综合语法树
- D 组：八进制、十六进制、表达式优先级等
- E 组：扩展词法、注释和复杂语法树

当前结果：

```text
parser tests: 20 passed, 0 failed
```

## 实现说明

- `src/parser.y`：Bison 语法规则、优先级、错误恢复和语法树构造。
- `src/ast.c`、`src/ast.h`：语法树节点、打印、释放和数值格式转换。
- `src/parser_main.c`：语法分析主入口。
- `src/lexer.l`：同时支持独立 lexer 模式和 parser 模式。
- `tools/run_parser_tests.sh`：完整 folder1 输出回归。

`src/parser.y` 中使用 `%expect 6` 记录 6 个预期 shift/reduce 冲突。这些冲突来自为课程错误样例添加的恢复产生式，不影响当前测试输出。
