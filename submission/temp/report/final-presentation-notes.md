# 最终汇报材料

本文档按构建顺序整理课程设计源码。每一部分先说明如何构建，再说明构建过程中涉及哪些源码文件、每个文件承担什么职责，最后结合样例输入和输出说明功能已经完成。

## 实践 1.1 Flex 词法分析

### 1. 本阶段目标

实践 1.1 的目标是实现 C-- 语言的词法分析器。程序读取 `.cmm` 源文件，把字符流识别为 token；如果遇到不符合词法规则的内容，则按课程要求输出 A 类词法错误。

本阶段主要完成以下功能：

- 识别关键字：`int`、`float`、`struct`、`return`、`if`、`else`、`while`
- 识别标识符、整数、浮点数
- 识别运算符和分隔符
- 跳过空白字符、单行注释和块注释
- 检查非法字符、非法整数、非法浮点数、非法标识符、嵌套块注释和未闭合块注释
- 提供独立可运行的词法分析器 `build/cmm-lexer`

### 2. 构建顺序

在实践 1.1 所在目录执行：

```sh
cd /home/leyi/Desktop/submission/proj1
make lexer
```

`make lexer` 对应 `Makefile` 中的目标：

```make
lexer: build/cmm-lexer

build/cmm-lexer: src/lexer.l src/lexer_main.c src/token.h
	@mkdir -p build
	flex -o build/lexer.c src/lexer.l
	gcc -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -I src src/lexer_main.c build/lexer.c -lfl -o build/cmm-lexer
```

构建过程分为两步：

1. 使用 Flex 把 `src/lexer.l` 转换成 C 源码 `build/lexer.c`。
2. 使用 GCC 把 `src/lexer_main.c` 和 `build/lexer.c` 一起编译，生成可执行文件 `build/cmm-lexer`。

在这个构建过程中，`lexer.l` 负责描述词法规则，Flex 根据这些规则生成 `yylex()`；`lexer_main.c` 负责打开输入文件、调用 `yylex()` 循环扫描并输出结果。

### 3. 构建涉及的文件

实践 1.1 的核心源码文件有三个：

```text
src/token.h
src/lexer.l
src/lexer_main.c
```

它们的关系如下：

```text
token.h
  定义 token 枚举和词法错误计数

lexer.l
  定义 Flex 正则规则
  生成 build/lexer.c
  提供 yylex()

lexer_main.c
  打开源文件
  调用 yylex()
  按 token 类型打印结果
```

### 4. `token.h`：定义词法单元类型

`token.h` 中定义了所有词法单元：

```c
typedef enum TokenKind {
    TOK_EOF = 0,
    TOK_INT,
    TOK_FLOAT,
    TOK_ID,
    TOK_SEMI,
    TOK_COMMA,
    TOK_ASSIGNOP,
    TOK_RELOP,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_DIV,
    TOK_AND,
    TOK_OR,
    TOK_DOT,
    TOK_NOT,
    TOK_TYPE,
    TOK_LP,
    TOK_RP,
    TOK_LB,
    TOK_RB,
    TOK_LC,
    TOK_RC,
    TOK_STRUCT,
    TOK_RETURN,
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE
} TokenKind;
```

Flex 每识别出一个 token，就返回这里定义的枚举值。`lexer_main.c` 根据这个枚举值决定打印什么 token 名称。`lexical_error_count` 用于记录词法错误数量，最后决定程序是否返回失败。

`token.h` 是词法阶段的公共接口。词法规则文件和主程序都依赖它，这样 token 类型集中维护，后续接入 Bison 时也更容易复用。

### 5. `lexer.l`：词法分析核心

`lexer.l` 是实践 1.1 的重点。文件开头设置 Flex 选项：

```lex
%option noyywrap yylineno nodefault noinput nounput
```

含义：

- `yylineno`：Flex 自动维护当前行号，方便错误定位。
- `noyywrap`：扫描到文件结束时不需要额外输入文件。
- `nodefault`：不使用 Flex 默认规则，避免非法字符被默认输出。
- `noinput`、`nounput`：不生成未使用的输入输出辅助函数，减少无关警告。

#### 5.1 普通 lexer 模式和 parser 模式共用一份规则

`lexer.l` 中有这段条件编译：

```c
#ifdef PARSER_BUILD
#include "ast.h"
#include "parser.tab.h"
#else
#include "token.h"
#endif
```

实践 1.1 构建词法分析器时没有定义 `PARSER_BUILD`，所以包含 `token.h`，返回 `TOK_*` 枚举。

实践 1.2 构建语法分析器时会加上：

```sh
flex -DPARSER_BUILD -o build/parser_lexer.c src/lexer.l
```

这时同一份词法规则会返回 Bison 使用的 token，并把 token 包装成 AST 终结符节点。

我没有为词法分析和语法分析维护两套 lexer，而是用 `PARSER_BUILD` 宏让同一个 `lexer.l` 同时服务两个阶段。这样可以保证 1.1 和 1.2 的词法识别行为一致，也避免后续修改词法规则时出现两份代码不一致的问题。

#### 5.2 错误输出函数

词法错误统一用两个函数输出：

```c
static void lexical_error(const char *message) {
    printf("Error type A at Line %d: %s.\n", yylineno, message);
    lexical_error_count++;
}

static void lexical_error_text(const char *message, const char *text) {
    printf("Error type A at Line %d: %s '%s'.\n", yylineno, message, text);
    lexical_error_count++;
}
```

所有词法错误都是 A 类错误。`yylineno` 由 Flex 自动维护，`lexical_error_count` 记录错误数。带 `text` 的版本会把非法词素一起打印出来，便于定位问题。

#### 5.3 TOKEN 宏

`lexer.l` 中使用宏统一返回 token：

```c
#ifdef PARSER_BUILD
#define TOKEN(name) do { yylval.node = ast_token(#name, NULL, yylineno); return name; } while (0)
#define TOKEN_VALUE(name) do { yylval.node = ast_token(#name, yytext, yylineno); return name; } while (0)
#else
#define TOKEN(name) return TOK_##name
#define TOKEN_VALUE(name) return TOK_##name
#endif
```

`TOKEN(name)` 用于没有属性值的 token，例如 `SEMI`、`PLUS`、`RETURN`。`TOKEN_VALUE(name)` 用于带属性值的 token，例如 `ID`、`INT`、`FLOAT`、`TYPE`、`RELOP`。

在实践 1.1 中，宏只是返回 `TOK_*`；在实践 1.2 中，宏还会构造 AST 终结符节点。这个宏的作用是让 lexer 在两个阶段复用：对词法分析器来说，返回 token 枚举即可；对语法分析器来说，还要把 token 的文本值传给 Bison。

#### 5.4 正则表达式定义

主要正则如下：

```lex
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

其中 `DEC_INT` 识别十进制整数，`OCT_INT` 识别八进制整数，`HEX_INT` 识别十六进制整数，`FLOAT_LIT` 支持普通小数和指数形式，`ID` 要求首字符是字母或下划线，后续可以是字母、数字或下划线。

合法数字和非法数字的规则顺序很关键。非法十六进制、非法八进制、非法浮点数要放在合法数字之前，否则非法字面量可能会被拆成多个合法 token。例如 `0xQQQ` 如果先匹配合法数字，可能被拆成 `0` 和标识符 `xQQQ`，就无法整体报告非法十六进制数。

#### 5.5 空白和注释处理

空白字符直接跳过：

```lex
[ \t\r\f\v]+ ;
\n           ;
```

单行注释直接跳过：

```lex
"//"[^\n]* ;
```

块注释使用 Flex 的 start condition：

```lex
%x COMMENT
```

读到 `/*` 后进入 `COMMENT` 状态：

```lex
"/*" { block_comment_start_line = yylineno; BEGIN(COMMENT); }
```

读到 `*/` 后退出注释状态：

```lex
<COMMENT>"*/" { BEGIN(INITIAL); }
```

如果块注释中再次出现 `/*`：

```lex
<COMMENT>"/*" { lexical_error("Nested block comment is not allowed"); }
```

如果文件结束时仍在块注释中：

```lex
<COMMENT><<EOF>> {
    printf("Error type A at Line %d: Unterminated block comment.\n",
           block_comment_start_line);
    lexical_error_count++;
    BEGIN(INITIAL);
    return 0;
}
```

块注释没有只靠一个简单正则处理，因为这里需要记录注释开始行号，还要检查未闭合注释和嵌套注释。使用 Flex 的 `COMMENT` 状态后，块注释内部内容可以和普通代码分开扫描，文件结束时也能根据 `block_comment_start_line` 报告开始行号。

#### 5.6 关键字、运算符和分隔符

关键字规则：

```lex
"struct" { TOKEN(STRUCT); }
"return" { TOKEN(RETURN); }
"if"     { TOKEN(IF); }
"else"   { TOKEN(ELSE); }
"while"  { TOKEN(WHILE); }
"int"|"float" { TOKEN_VALUE(TYPE); }
```

运算符和分隔符：

```lex
"&&" { TOKEN(AND); }
"||" { TOKEN(OR); }
">="|"<="|"=="|"!="|">"|"<" { TOKEN_VALUE(RELOP); }
"=" { TOKEN(ASSIGNOP); }
";" { TOKEN(SEMI); }
"," { TOKEN(COMMA); }
"+" { TOKEN(PLUS); }
"-" { TOKEN(MINUS); }
"*" { TOKEN(STAR); }
"/" { TOKEN(DIV); }
"." { TOKEN(DOT); }
"!" { TOKEN(NOT); }
"(" { TOKEN(LP); }
")" { TOKEN(RP); }
"[" { TOKEN(LB); }
"]" { TOKEN(RB); }
"{" { TOKEN(LC); }
"}" { TOKEN(RC); }
```

关键字规则必须放在 `ID` 规则前面。`int`、`return` 这些关键字本身也符合 `ID` 的正则，如果 `ID` 规则放在前面，关键字就可能被识别成普通标识符。Flex 在匹配长度相同的情况下会选择更靠前的规则，所以关键字规则先写可以保证它们被识别为对应的关键字 token。

#### 5.7 非法词法规则

非法十六进制：

```lex
0[xX][0-9a-fA-F]*[g-zG-Z_]{IDCHAR}* { lexical_error_text("Invalid hexadecimal literal", yytext); }
0[xX] { lexical_error_text("Invalid hexadecimal literal", yytext); }
```

非法八进制：

```lex
0[0-7]*[89]{IDCHAR}* { lexical_error_text("Invalid octal literal", yytext); }
```

非法浮点数：

```lex
{DIGIT}+\.{DIGIT}+\.{DIGIT}({DIGIT}|\.)* { lexical_error_text("Invalid floating point literal", yytext); }
({FLOAT_BASE}|{DIGIT}+)[eE][+-]?{LETTER}{IDCHAR}* { lexical_error_text("Invalid floating point literal", yytext); }
({FLOAT_BASE}|{DIGIT}+)[eE][+-]? { lexical_error_text("Invalid floating point literal", yytext); }
```

数字开头的非法标识符：

```lex
{DIGIT}+{LETTER}{IDCHAR}* { lexical_error_text("Invalid identifier starting with digit", yytext); }
```

兜底非法字符：

```lex
. { lexical_error_text("Undefined character", yytext); }
```

最后一条 `.` 是兜底规则。任何没有被前面规则识别的字符，都会被报告为未定义字符，避免非法输入被静默忽略。

### 6. `lexer_main.c`：词法分析器入口

`lexer_main.c` 的作用是运行 Flex 生成的扫描器。

主要流程：

```c
if (argc != 2) {
    fprintf(stderr, "Usage: %s <source.cmm>\n", argv[0]);
    return 1;
}

yyin = fopen(argv[1], "r");
if (!yyin) {
    perror(argv[1]);
    return 1;
}

yyrestart(yyin);
```

这部分负责检查参数、打开输入文件并重置 Flex 输入流。

扫描循环：

```c
TokenKind kind;
while ((kind = (TokenKind)yylex()) != TOK_EOF) {
    if (!dump_tokens || lexical_error_count > 0) {
        continue;
    }
    if (token_has_value(kind)) {
        printf("%s: %s\n", token_name(kind), yytext);
    } else {
        printf("%s\n", token_name(kind));
    }
}
```

`yylex()` 是 Flex 根据 `lexer.l` 自动生成的函数。每调用一次 `yylex()`，就识别并返回一个 token。`token_has_value()` 判断 token 是否需要打印属性值，`yytext` 是 Flex 提供的当前匹配文本。

默认不打印 token 流，只有设置环境变量时才打印：

```c
int dump_tokens = getenv("CMM_LEX_DUMP") != NULL;
```

展示 token 流时使用：

```sh
CMM_LEX_DUMP=1 build/cmm-lexer test/proj1-test1.cmm
```

错误样例通常只要求输出错误信息。如果默认打印 token，错误输出会混入调试信息，不符合课程要求。所以 token 流由环境变量控制，只在调试和展示时打开。

### 7. 样例输入和输出

#### 7.1 非法字符样例

输入文件：

```sh
nl -ba test/proj1-2-3-6-exp1-1.cmm
```

运行：

```sh
build/cmm-lexer test/proj1-2-3-6-exp1-1.cmm
```

输出示例：

```text
Error type A at Line 3: Undefined character '~'.
```

这个样例说明：

- 词法分析器能够识别未定义字符。
- 错误类型为 A。
- 行号来自 Flex 的 `yylineno`。

#### 7.2 八进制和十六进制整数样例

运行：

```sh
CMM_LEX_DUMP=1 build/cmm-lexer test/proj1-2-3-7-exp1-0.cmm
```

输出中可以看到类似 token：

```text
TYPE: int
ID: main
LP
RP
LC
INT: 0123
INT: 0x3F
RETURN
INT: 0
SEMI
RC
```

这个样例说明：

- 八进制整数可以被识别为 `INT`。
- 十六进制整数可以被识别为 `INT`。
- token 流输出由 `CMM_LEX_DUMP=1` 打开。

#### 7.3 注释过滤样例

运行：

```sh
CMM_LEX_DUMP=1 build/cmm-lexer test/proj1-2-3-7-exp5-0.cmm
```

观察点：

- `//` 单行注释不会出现在 token 流里。
- `/* ... */` 块注释不会出现在 token 流里。
- 注释前后的有效 token 能正常输出。

#### 7.4 手写综合样例

运行：

```sh
nl -ba test/proj1-test1.cmm
CMM_LEX_DUMP=1 build/cmm-lexer test/proj1-test1.cmm
```

这个样例覆盖：

- 结构体关键字 `struct`
- 函数定义
- 变量定义
- 成员访问 `.`
- 赋值 `=`
- 函数调用
- 条件语句 `if-else`
- `write`
- `return`

手写样例不是只测单个 token，而是把常见 C-- 语法结构放在一起，验证词法分析器在完整程序中的表现。
