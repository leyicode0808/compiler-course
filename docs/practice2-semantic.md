# 实践 2：语义分析

## 构建

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
CMM_SEMANTIC=1 build/cmmc tests/project2/inputs/A-1.cmm
```

若输入存在词法或语法错误，程序直接输出 A/B 类错误，不进入语义分析。若语法正确，则进行语义检查；没有语义错误时不输出内容。

## 测试

```sh
make semantic-test
```

当前覆盖 `tests/project2/expects/` 中全部 30 个期望输出：

- A 组：基础 1-17 类语义错误
- B 组：多个语义错误组合
- C 组：正确程序
- D 组：语法边界、作用域冲突、结构体/数组类型匹配
- E 组：函数声明、声明一致性、作用域和数组使用

当前结果：

```text
semantic tests: 30 passed, 0 failed
```

## 实现说明

- `src/semantic.c`、`src/semantic.h`：类型系统、符号表、结构体表、函数表和 AST 语义遍历。
- `src/parser_main.c`：默认保持实践 1.2 的语法树输出；设置 `CMM_SEMANTIC=1` 时执行语义检查。
- `src/parser.y`：补充函数声明产生式，并保留 Project 2 测试中对带数组形参声明的语法错误口径。
- `tools/run_semantic_tests.sh`：对 Project 2 的 30 个期望输出逐字 diff。D-2 使用课程旧假设中的无嵌套作用域口径，脚本通过 `CMM_NO_SCOPE=1` 显式开启兼容模式；其他样例使用要求 3.2 的嵌套作用域模式。
