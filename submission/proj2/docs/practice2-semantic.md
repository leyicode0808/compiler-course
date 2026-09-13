# 实践 2：语义分析

## 构建

所有命令均在当前实践目录下执行：

```sh
cd submission/proj2
```

构建语义分析器：

```sh
make semantic
```

生成：

```text
build/cmmc
```

## 运行

语义分析模式通过环境变量开启：

```sh
CMM_SEMANTIC=1 build/cmmc test/proj2-3-3-6-exp1-1.cmm
```

若输入存在词法或语法错误，程序直接输出 A/B 类错误，不进入语义分析。若语法正确，则进行语义检查；没有语义错误时不输出内容。

查看手写样例源码并运行：

```sh
nl -ba test/proj2-test1.cmm
CMM_SEMANTIC=1 build/cmmc test/proj2-test1.cmm
```

## 演示

```sh
make semantic-demo
```

也可以执行：

```sh
make semantic-test
```

当前演示使用 `Project_2.pdf` 中的公开样例：

- `test/proj2-3-3-6-exp1-1.cmm`：3.3.6 必做样例 1，未定义变量。
- `test/proj2-3-3-6-exp2-1.cmm`：3.3.6 必做样例 2，未定义函数。
- `test/proj2-3-3-6-exp3-1.cmm`：3.3.6 必做样例 3，变量重定义。
- `test/proj2-3-3-6-exp5-1.cmm`：3.3.6 必做样例 5，赋值类型不匹配。
- `test/proj2-3-3-6-exp8-1.cmm`：3.3.6 必做样例 8，返回类型不匹配。
- `test/proj2-3-3-6-exp14-1.cmm`：3.3.6 必做样例 14，访问不存在的结构体域。
- `test/proj2-3-3-6-exp17-1.cmm`：3.3.6 必做样例 17，使用未定义结构体。
- `test/proj2-3-3-7-exp1-0.cmm`：3.3.7 选做样例 1，函数声明与定义。
- `test/proj2-3-3-7-exp2-0.cmm`：3.3.7 选做样例 2，函数声明不一致。
- `test/proj2-3-3-7-exp3-0.cmm`：3.3.7 选做样例 3，嵌套作用域。
- `test/proj2-3-3-7-exp4-0.cmm`：3.3.7 选做样例 4，同一作用域变量重定义。

## 实现说明

- `src/semantic.c`、`src/semantic.h`：类型系统、符号表、结构体表、函数表、作用域和 AST 语义遍历。
- `src/parser_main.c`：默认保持实践 1.2 的语法树输出；设置 `CMM_SEMANTIC=1` 时执行语义检查。
- `src/parser.y`、`src/lexer.l`：复用实践 1 的前端，负责生成语义分析所需 AST。
- `tools/run_semantic_tests.sh`：运行当前 `test/` 目录下按 `Project_2.pdf` 章节命名的公开样例并打印输出。
