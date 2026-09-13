# 实践 1.1：Flex 词法分析

## 构建

```sh
make lexer
```

生成：

```text
build/cmm-lexer
```

## 运行

默认运行只输出词法错误，符合实践内容一“有错误时只输出错误信息”的要求：

```sh
build/cmm-lexer test/proj1-2-3-6-exp1-1.cmm
```

查看 token 流时使用调试开关：

```sh
CMM_LEX_DUMP=1 build/cmm-lexer test/proj1-2-3-7-exp1-0.cmm
```

## 演示

```sh
make lexer-demo
```

当前演示使用 `Project_1.pdf` 中的公开样例：

- 必做样例 1：未定义字符 `~`
- 选做样例 1：八进制和十六进制整数
- 选做样例 5：行注释和块注释

## 后续衔接

`src/lexer.l` 后续会继续复用。进入实践 1.2 后，词法动作需要从调试输出模式改为向 Bison 返回 token，并传递 `ID`、`TYPE`、`INT`、`FLOAT` 等属性值。
