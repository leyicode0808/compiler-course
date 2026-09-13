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
build/cmmc test/proj1-2-3-6-exp3-1.cmm
```

若输入无词法或语法错误，程序按先序遍历输出语法树。若存在词法或语法错误，只输出错误信息，不输出语法树。

## 演示

```sh
make parser-demo
```

当前演示使用 `Project_1.pdf` 中的公开样例：

- 必做样例 2：数组访问和 if-else 语法错误
- 必做样例 3：正确程序语法树
- 必做样例 4：结构体语法树

## 实现说明

- `src/parser.y`：Bison 语法规则、优先级、错误恢复和语法树构造。
- `src/ast.c`、`src/ast.h`：语法树节点、打印、释放和数值格式转换。
- `src/parser_main.c`：语法分析主入口。
- `src/lexer.l`：同时支持独立 lexer 模式和 parser 模式。
- `tools/run_parser_tests.sh`：运行公开样例并打印语法错误或语法树，便于人工对照 `course/Project_1.pdf` 验收。

`src/parser.y` 中使用 `%expect 6` 记录 6 个预期 shift/reduce 冲突。这些冲突来自为课程错误样例添加的恢复产生式，不影响当前测试输出。
