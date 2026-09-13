# 开发前检查清单

## 已准备

- Git 使用 `./tools/gitw`，当前分支为 `main`。
- 原始课程资料保留在 `编译原理课程设计/`。
- C-- 文法整理在 `docs/cmm-grammar.md`。
- `Makefile` 提供统一入口。

## 每次开始前

```sh
make status
make check-env
make prepare-tests
```

## 每次修改后

```sh
make test
./tools/gitw diff
./tools/gitw status
```

测试通过后再提交：

```sh
./tools/gitw add <files>
./tools/gitw commit -m "stage: describe change"
```

## 阶段提交建议

- `lexer:` 词法规则、错误处理、词法测试。
- `parser:` 语法规则、优先级、语法树。
- `semantic:` 类型系统、符号表、语义错误。
- `ir:` 中间代码结构、翻译规则、IR 测试。
- `mips:` 目标代码生成和 SPIM 验证。
- `opt:` 中间代码优化。
