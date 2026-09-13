# 编译原理课程设计报告：实践 1.1 Flex 词法分析

课程设计名称：编译原理课程设计

实践内容：实践 1.1 学习 Flex，用 Flex 实现词法分析

## 一、目的和任务

本次实践在 `submission/proj1` 目录中独立完成，目标是根据 C-- 语言词法规则，使用 GNU Flex 实现词法分析程序。程序读取 `.cmm` 源文件后，将字符流识别为词法单元；遇到不符合词法规则的输入时，输出 A 类词法错误、错误行号和说明文字。

本阶段主要任务如下：

1. 阅读 `course/Appendix.pdf` 和 `course/Project_1.pdf`，整理 C-- 语言需要识别的 token。
2. 使用 Flex 编写 `src/lexer.l`，识别关键字、标识符、整数、浮点数、运算符和分隔符。
3. 过滤空白字符、`//` 单行注释和 `/* ... */` 块注释。
4. 对未定义字符、非法整数、非法浮点数、非法标识符和非法注释输出 A 类词法错误。
5. 编写 `Makefile` 和 `tools/run_lexer_tests.sh`，使实践 1.1 可以在 `proj1` 目录中单独构建、单独演示。

## 二、项目结构和文件功能

实践 1.1 相关文件均位于 `submission/proj1` 下，目录结构如下：

```text
submission/proj1/
├── Makefile
├── course/
│   ├── Appendix.pdf
│   └── Project_1.pdf
├── docs/
│   ├── practice1.1-lexer.md
│   └── practice1.1-lexer-report.md
├── src/
│   ├── lexer.l
│   ├── lexer_main.c
│   └── token.h
├── test/
│   ├── proj1-2-3-6-exp1-1.cmm
│   ├── proj1-2-3-7-exp1-0.cmm
│   ├── proj1-2-3-7-exp5-0.cmm
│   └── proj1-test1.cmm
└── tools/
    ├── check_env.sh
    └── run_lexer_tests.sh
```

各文件功能如下：

- `Makefile`：实践一独立构建入口。`make lexer` 生成词法分析器，`make lexer-demo` 运行词法公开样例，`make demo` 同时运行词法和语法演示。
- `src/lexer.l`：Flex 词法规则文件，是实践 1.1 的核心实现。它定义 token 正则表达式、注释处理状态、错误处理动作，并在独立 lexer 模式下返回 `token.h` 中的 token 枚举。
- `src/lexer_main.c`：独立词法分析程序入口。它负责打开输入文件、调用 `yylex()` 循环扫描、根据 `CMM_LEX_DUMP` 决定是否打印 token 流，并在存在词法错误时返回非零状态。
- `src/token.h`：独立 lexer 模式使用的 token 枚举和 `lexical_error_count` 声明，保证 `lexer.l` 和 `lexer_main.c` 使用同一套 token 类型。
- `tools/check_env.sh`：检查本实践需要的 `flex`、`bison`、`gcc` 等工具是否存在。
- `tools/run_lexer_tests.sh`：运行 `Project_1.pdf` 中与词法相关的公开样例，并直接打印源文件路径和程序输出，便于人工对照验收。
- `test/proj1-2-3-6-exp1-1.cmm`：`Project_1.pdf` 2.3.6 必做样例 1，检查未定义字符的 A 类错误。
- `test/proj1-2-3-7-exp1-0.cmm`：`Project_1.pdf` 2.3.7 选做样例 1，检查八进制和十六进制整数 token。
- `test/proj1-2-3-7-exp5-0.cmm`：`Project_1.pdf` 2.3.7 选做样例 5，检查单行注释和块注释过滤。
- `test/proj1-test1.cmm`：手写综合样例，可用于现场展示 token 流。

## 三、实现过程

### 1. Token 和正则表达式设计

`src/lexer.l` 中先定义常用正则片段：

```text
DIGIT       [0-9]
NZDIGIT     [1-9]
LETTER      [_a-zA-Z]
IDCHAR      [_a-zA-Z0-9]
DEC_INT     0|{NZDIGIT}{DIGIT}*
OCT_INT     0[0-7]+
HEX_INT     0[xX][0-9a-fA-F]+
EXP         [eE][+-]?{DIGIT}+
FLOAT_BASE  ({DIGIT}+\.{DIGIT}+|{DIGIT}+\.|\.{DIGIT}+)
FLOAT_LIT   ({FLOAT_BASE}{EXP}?|{DIGIT}+{EXP})
ID          {LETTER}{IDCHAR}*
```

这些定义分别用于识别十进制整数、八进制整数、十六进制整数、浮点数、指数形式浮点数和标识符。关键字规则放在标识符规则之前，保证 `int`、`float`、`struct`、`return`、`if`、`else`、`while` 被识别为关键字，而不是普通 `ID`。

### 2. 主要函数和宏的功能

`src/lexer.l` 中的主要辅助函数和宏如下：

- `lexical_error(const char *message)`：输出不带原始词素的 A 类错误，并递增 `lexical_error_count`。
- `lexical_error_text(const char *message, const char *text)`：输出带原始词素的 A 类错误，例如非法十六进制数、非法八进制数和未定义字符。
- `TOKEN(name)`：在独立 lexer 模式下返回 `TOK_name`；后续接入 Bison 时可切换为返回 Bison token。
- `TOKEN_VALUE(name)`：用于 `ID`、`TYPE`、`INT`、`FLOAT`、`RELOP` 等带属性值的 token。独立词法输出时由 `lexer_main.c` 使用 `yytext` 打印词素。

`src/lexer_main.c` 中的主要函数如下：

- `token_name(TokenKind kind)`：将 token 枚举转换为输出名称，例如 `TOK_INT` 转换为 `INT`。
- `token_has_value(TokenKind kind)`：判断 token 是否需要额外打印词素值。`INT`、`FLOAT`、`ID`、`TYPE`、`RELOP` 会打印为 `TOKEN: value`。
- `main(int argc, char **argv)`：检查命令行参数，打开输入文件，调用 `yyrestart()` 初始化 Flex 输入流，再循环调用 `yylex()`。默认只输出错误；设置 `CMM_LEX_DUMP=1` 后，在没有词法错误时打印完整 token 流。

### 3. 注释和空白处理

空格、制表符、回车和换行不返回 token。单行注释使用 `"//"[^\n]*` 直接跳过。

块注释使用 Flex start condition：

```text
%x COMMENT
```

扫描到 `/*` 后记录起始行号并进入 `COMMENT` 状态；扫描到 `*/` 后回到初始状态。如果注释中再次出现 `/*`，报告嵌套块注释错误；如果文件结束时仍在注释状态中，则使用记录的起始行号报告未闭合块注释错误。

### 4. 错误处理

实践 1.1 中所有词法错误均输出为 A 类错误：

```text
Error type A at Line 行号: 错误说明.
```

当前实现覆盖以下情况：

- 未定义字符，例如 `~`
- 数字开头的非法标识符
- 非法十六进制整数
- 非法八进制整数
- 非法普通浮点数
- 非法指数形式浮点数
- 嵌套块注释
- 未闭合块注释

错误规则放在合法数字规则之前，使非法字面量能整体识别并报告，而不是被拆成多个合法 token。

## 四、构建和运行

所有命令均在当前实践目录下执行：

```sh
cd submission/proj1
```

检查环境：

```sh
make check-env
```

构建词法分析器：

```sh
make lexer
```

该命令生成：

```text
build/cmm-lexer
```

运行词法公开样例：

```sh
make lexer-demo
```

手动查看某个源文件并运行词法分析：

```sh
nl -ba test/proj1-test1.cmm
CMM_LEX_DUMP=1 build/cmm-lexer test/proj1-test1.cmm
```

其中 `nl -ba` 用于打印带行号的源代码，方便对照输出；`CMM_LEX_DUMP=1` 用于打开 token 流输出。

## 五、结果与结论分析

执行：

```sh
make lexer-demo
```

当前会运行三个公开样例：

- `test/proj1-2-3-6-exp1-1.cmm`：2.3.6 必做样例 1，输出未定义字符 `~` 的 A 类错误。
- `test/proj1-2-3-7-exp1-0.cmm`：2.3.7 选做样例 1，输出八进制整数和十六进制整数 token。
- `test/proj1-2-3-7-exp5-0.cmm`：2.3.7 选做样例 5，验证注释被跳过，只输出有效 token。

手写样例 `test/proj1-test1.cmm` 能正常输出 token 流，覆盖结构体、函数、变量定义、成员访问、赋值、函数调用、条件语句、`write` 和 `return` 等常见词法单元。

本次实践完成了基于 Flex 的 C-- 词法分析器，实现了 token 识别、注释过滤、行号维护和 A 类错误报告。代码结构上将词法规则、token 定义和独立运行入口拆分到不同文件中，便于后续实践 1.2 在同一份 `lexer.l` 基础上接入 Bison。
