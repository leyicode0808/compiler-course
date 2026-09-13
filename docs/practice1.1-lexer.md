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
build/cmm-lexer tests/project1/inputs/A-1.cmm
```

查看 token 流时使用调试开关：

```sh
CMM_LEX_DUMP=1 build/cmm-lexer tests/project1/inputs/E1-1.cmm
```

## 测试

```sh
make lexer-test
```

当前回归覆盖：

- 未定义字符：`$`
- 数字开头的非法标识符：`2x`
- 非法浮点常量：`3.14.15`
- 非法十六进制常量：`0xQQQ`
- 非法八进制常量：`099`
- 非法指数浮点：`2.5e`、`1.0E+`
- 嵌套块注释

## 后续衔接

`src/lexer.l` 后续会继续复用。进入实践 1.2 后，词法动作需要从调试输出模式改为向 Bison 返回 token，并传递 `ID`、`TYPE`、`INT`、`FLOAT` 等属性值。
