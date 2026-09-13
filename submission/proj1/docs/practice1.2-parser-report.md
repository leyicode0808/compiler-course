# 编译原理课程设计报告：实践 1.2 Bison 语法分析

课程设计名称：编译原理课程设计

实践内容：实践 1.2 学习 Bison，用 Bison 实现语法分析

## 一、目的和任务

本次实践继续在 `submission/proj1` 目录中完成，在实践 1.1 词法分析器的基础上，使用 GNU Bison 实现 C-- 语言语法分析程序。程序需要判断输入源代码是否符合 C-- 语法规则；若没有词法或语法错误，则按照课程要求输出语法树；若存在错误，则输出 A 类或 B 类错误信息，并且不输出语法树。

本阶段主要任务如下：

1. 根据 `course/Appendix.pdf` 中的 C-- 文法编写 Bison 产生式。
2. 复用实践 1.1 的 `src/lexer.l`，使 Flex 能向 Bison 返回 token 和属性值。
3. 新增 AST 数据结构，保存语法单元、词法单元、行号、属性值和子节点关系。
4. 设置表达式优先级和结合性，处理赋值、逻辑、关系、算术、一元运算、数组访问、函数调用和结构体成员访问。
5. 对常见语法错误进行恢复，输出 B 类错误并尽量继续分析后续代码。
6. 编写 `Makefile` 和 `tools/run_parser_tests.sh`，使实践 1.2 可以在 `proj1` 目录中单独构建、单独演示。

## 二、项目结构和文件功能

实践 1.2 在实践 1.1 文件基础上新增语法分析和 AST 模块，当前目录结构如下：

```text
submission/proj1/
├── Makefile
├── course/
│   ├── Appendix.pdf
│   └── Project_1.pdf
├── docs/
│   ├── practice1.2-parser.md
│   └── practice1.2-parser-report.md
├── src/
│   ├── ast.c
│   ├── ast.h
│   ├── lexer.l
│   ├── lexer_main.c
│   ├── parser.y
│   ├── parser_main.c
│   └── token.h
├── test/
│   ├── proj1-2-3-6-exp2-1.cmm
│   ├── proj1-2-3-6-exp3-1.cmm
│   ├── proj1-2-3-6-exp4-1.cmm
│   └── proj1-test1.cmm
└── tools/
    ├── check_env.sh
    └── run_parser_tests.sh
```

各文件功能如下：

- `Makefile`：实践一独立构建入口。`make parser` 生成语法分析器和 AST 打印程序，`make parser-demo` 运行语法公开样例。
- `src/parser.y`：Bison 文法文件，是实践 1.2 的核心实现。它定义 token、非终结符类型、优先级、C-- 产生式、语法树构造动作和错误恢复动作。
- `src/parser_main.c`：语法分析程序入口。它负责打开输入文件、调用 `yyparse()`，并在词法错误数和语法错误数都为 0 时打印语法树。
- `src/ast.h`：声明 AST 节点结构和 AST 操作函数。
- `src/ast.c`：实现 AST 节点创建、子树合并、整数转换、先序打印和内存释放。
- `src/lexer.l`：继续复用实践 1.1 的词法规则。编译 parser 时通过 `-DPARSER_BUILD` 切换到 Bison 模式，返回 `parser.tab.h` 中的 token，并通过 `yylval.node` 传递终结符 AST 节点。
- `src/lexer_main.c` 和 `src/token.h`：保留实践 1.1 的独立词法分析入口和 token 枚举，方便单独验证词法部分。
- `tools/run_parser_tests.sh`：运行 `Project_1.pdf` 中与语法树或语法错误相关的公开样例。
- `test/proj1-2-3-6-exp2-1.cmm`：`Project_1.pdf` 2.3.6 必做样例 2，检查语法错误输出。
- `test/proj1-2-3-6-exp3-1.cmm`：`Project_1.pdf` 2.3.6 必做样例 3，检查简单程序语法树。
- `test/proj1-2-3-6-exp4-1.cmm`：`Project_1.pdf` 2.3.6 必做样例 4，检查结构体相关语法树。
- `test/proj1-test1.cmm`：手写综合样例，可用于现场展示 AST 输出。

## 三、在实践 1.1 基础上的修改和新增内容

实践 1.2 没有重写词法分析器，而是在实践 1.1 的基础上做了以下扩展：

1. 新增 `src/parser.y`，用 Bison 描述 C-- 语法规则，并在每个产生式动作中构造 AST 节点。
2. 新增 `src/parser_main.c`，作为语法分析阶段的命令行入口。
3. 新增 `src/ast.h` 和 `src/ast.c`，实现语法树数据结构和输出逻辑。
4. 修改 `src/lexer.l`，加入 `PARSER_BUILD` 条件编译逻辑。独立 lexer 模式仍返回 `TOK_*`；parser 模式则包含 `parser.tab.h`，返回 Bison token。
5. 修改 token 返回动作。parser 模式下，`TOKEN(name)` 和 `TOKEN_VALUE(name)` 会创建终结符 AST 节点并写入 `yylval.node`，从而把 `ID`、`TYPE`、`INT`、`FLOAT`、`RELOP` 等属性传给 Bison。
6. 修改 `Makefile`，新增 `parser` 和 `parser-demo` 目标。`make parser` 先运行 Bison 生成 `build/parser.tab.c` 和 `build/parser.tab.h`，再用 `flex -DPARSER_BUILD` 生成 parser 专用 lexer，最后链接生成 `build/cmmc`。
7. 新增 `tools/run_parser_tests.sh`，用于在当前目录运行公开语法样例。

## 四、主要函数和模块说明

### 1. `parser.y` 中的辅助函数

- `node_line(AstNode *node)`：取得 AST 节点行号；节点为空时使用当前 `yylineno`，用于空产生式或错误恢复场景。
- `syntax_error_at(int line, const char *msg)`：输出 B 类语法错误，并使用 `last_syntax_error_line` 避免同一行重复报错。若已经存在词法错误，则不再输出语法错误，避免混淆错误类型。
- `ast_has_array_vardec(AstNode *node)`：递归检查函数声明参数中是否出现数组形式的变量声明，用于报告课程样例中需要识别的语法问题。
- `yyerror(const char *msg)`：Bison 默认错误回调函数。它将默认语法错误统一输出为 `Error type B at Line 行号: Syntax error.`。

### 2. `ast.h` / `ast.c` 中的函数

- `ast_token(const char *name, const char *value, int line)`：创建终结符节点，保存 token 名称、可选属性值和行号。
- `ast_node(const char *name, int line, size_t child_count, ...)`：创建非终结符节点，并接收可变数量的子节点。
- `ast_adopt(const char *name, int line, AstNode *first, AstNode *second)`：合并两个节点的子节点，适合需要重新组织子树的场景。
- `ast_parse_int(const char *text)`：使用 `strtol(text, NULL, 0)` 将十进制、八进制、十六进制整数字符串转换为十进制数值。
- `ast_print(const AstNode *node, int indent)`：按照课程要求先序遍历输出语法树。非终结符打印 `Name (line)`；`ID`、`TYPE`、`INT`、`FLOAT` 打印属性值；子节点缩进 2 个空格。
- `ast_free(AstNode *node)`：递归释放 AST 节点、名称字符串、属性字符串和子节点数组。

### 3. `parser_main.c` 中的入口逻辑

`main()` 的执行流程如下：

1. 检查命令行参数，要求输入一个 `.cmm` 文件。
2. 打开源文件，并调用 `yyrestart(yyin)` 设置 Flex 输入流。
3. 调用 `yyparse()` 执行语法分析。
4. 如果 `lexical_error_count == 0`、`syntax_error_count == 0` 且 `yyparse()` 返回 0，则调用 `ast_print(parse_root, 0)` 输出语法树。
5. 调用 `ast_free(parse_root)` 释放内存，关闭输入文件。

这种设计保证错误样例只输出错误信息，不会混入语法树内容。

## 五、语法规则和错误恢复

`src/parser.y` 覆盖了 C-- 的主要语法单元：

- 外部定义：`Program`、`ExtDefList`、`ExtDef`、`ExtDecList`
- 类型说明：`Specifier`、`StructSpecifier`、`OptTag`、`Tag`
- 变量和函数：`VarDec`、`FunDec`、`VarList`、`ParamDec`
- 复合语句和语句：`CompSt`、`StmtList`、`Stmt`
- 局部定义：`DefList`、`Def`、`DecList`、`Dec`
- 表达式和实参：`Exp`、`Args`

表达式优先级从低到高设置为：

```text
ASSIGNOP
OR
AND
RELOP
PLUS / MINUS
STAR / DIV
NOT / UMINUS
LP / RP / LB / RB / DOT
```

同时使用 `LOWER_THAN_ELSE` 和 `ELSE` 处理悬空 `else`，使 `else` 与最近的 `if` 匹配。

错误恢复方面，当前实现针对常见错误添加了局部恢复产生式，例如数组维度不是整数、函数参数尾逗号、函数调用实参尾逗号、全局变量初始化、语句后出现变量定义、空 `while` 语句体、数组初始化列表、结构体成员访问缺少成员名等。这样可以在报告错误后继续分析后续输入，减少连锁错误。

## 六、构建和运行

所有命令均在当前实践目录下执行：

```sh
cd submission/proj1
```

构建语法分析器：

```sh
make parser
```

该命令生成：

```text
build/cmmc
```

运行语法公开样例：

```sh
make parser-demo
```

手动查看某个源文件并运行语法分析：

```sh
nl -ba test/proj1-test1.cmm
build/cmmc test/proj1-test1.cmm
```

## 七、结果与结论分析

执行：

```sh
make parser-demo
```

当前会运行三个公开样例：

- `test/proj1-2-3-6-exp2-1.cmm`：2.3.6 必做样例 2，输出两处 B 类语法错误。
- `test/proj1-2-3-6-exp3-1.cmm`：2.3.6 必做样例 3，输出简单函数的语法树。
- `test/proj1-2-3-6-exp4-1.cmm`：2.3.6 必做样例 4，输出结构体声明和结构体变量访问相关语法树。

手写样例 `test/proj1-test1.cmm` 可以正常输出语法树，覆盖结构体定义、函数定义、局部变量定义、成员访问、函数调用、`if-else`、`write` 和 `return` 等语法结构。

本次实践完成了基于 Bison 的 C-- 语法分析器，并与实践 1.1 的 Flex 词法分析器完成衔接。程序在正确输入上能够按照课程格式输出语法树，在错误输入上能够输出 A 类或 B 类错误信息。新增的 AST 模块和 parser 入口也为后续语义分析阶段提供了清晰的数据结构基础。
