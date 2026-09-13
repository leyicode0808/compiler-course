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

这里的“没有定义 `PARSER_BUILD`”指的是构建命令中没有给 Flex 传入 `-DPARSER_BUILD` 参数。实践 1.1 的构建命令是：

```sh
flex -o build/lexer.c src/lexer.l
```

因此 `lexer.l` 中的条件编译会走 `#else` 分支：

```c
#else
#include "token.h"
#endif
```

同时，返回 token 的宏也会走普通词法分析分支：

```c
#define TOKEN(name) return TOK_##name
#define TOKEN_VALUE(name) return TOK_##name
```

例如 Flex 识别到关键字 `return` 时，规则中写的是：

```lex
"return" { TOKEN(RETURN); }
```

在实践 1.1 中，这个宏会展开成：

```c
return TOK_RETURN;
```

也就是说，独立词法分析器只需要告诉 `lexer_main.c` 当前识别到了哪一种 token，不需要构造语法树，也不需要和 Bison 交互。

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

其中 `yyin` 是 Flex 约定使用的全局输入文件指针。Flex 生成的 `yylex()` 默认从 `yyin` 指向的文件中读取字符；如果不设置 `yyin`，通常会从标准输入读取。这里的：

```c
yyin = fopen(argv[1], "r");
```

表示用只读方式打开命令行传入的源文件，并把这个文件交给 Flex 扫描器作为输入。`argv[1]` 是程序运行时传入的第一个参数，例如执行：

```sh
build/cmm-lexer test/proj1-test1.cmm
```

此时 `argv[1]` 就是：

```text
test/proj1-test1.cmm
```

如果文件打开失败，`yyin` 会是空指针，所以后面用：

```c
if (!yyin) {
    perror(argv[1]);
    return 1;
}
```

输出系统错误并退出，避免后续扫描一个不存在的输入文件。

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

## 实践 1.2 Bison 语法分析

### 1. 本阶段目标

实践 1.2 在实践 1.1 词法分析器的基础上实现语法分析器。程序读取 `.cmm` 源文件，先由 Flex 识别 token，再由 Bison 按 C-- 文法进行规约。如果输入没有词法和语法错误，就输出语法树；如果存在语法错误，就输出 B 类错误。

本阶段主要完成以下功能：

- 复用实践 1.1 的 `lexer.l`
- 使用 Bison 描述 C-- 语法规则
- 设置表达式优先级和结合性
- 解决 `if-else` 的悬挂 else 问题
- 构造 AST 语法树
- 输出课程要求格式的语法树
- 对常见语法错误做局部恢复，减少连锁报错

### 2. 构建顺序

在实践 1.2 所在目录执行：

```sh
cd /home/leyi/Desktop/submission/proj1
make parser
```

`make parser` 对应 `Makefile` 中的目标：

```make
parser: build/cmmc

build/cmmc: src/parser.y src/lexer.l src/parser_main.c src/ast.c src/ast.h
	@mkdir -p build
	bison -d -o build/parser.tab.c src/parser.y
	flex -DPARSER_BUILD -o build/parser_lexer.c src/lexer.l
	gcc -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -I src -I build \
		src/parser_main.c src/ast.c build/parser.tab.c build/parser_lexer.c -lfl -o build/cmmc
```

构建过程分为三步：

1. 使用 Bison 把 `src/parser.y` 转换成 `build/parser.tab.c` 和 `build/parser.tab.h`。
2. 使用 Flex 把 `src/lexer.l` 转换成 parser 模式下的 `build/parser_lexer.c`。
3. 使用 GCC 把 `parser_main.c`、`ast.c`、Bison 生成的 parser、Flex 生成的 lexer 链接成 `build/cmmc`。

Bison 命令中的 `-d` 会额外生成头文件 `parser.tab.h`。这个头文件中包含 Bison token 定义，后续 Flex 在 `PARSER_BUILD` 模式下需要包含它，才能返回 Bison 认识的 token。

Flex 命令中的 `-DPARSER_BUILD` 会定义 `PARSER_BUILD` 宏，使 `lexer.l` 进入 parser 模式。实践 1.1 中 lexer 只返回 `TOK_*` 枚举；实践 1.2 中 lexer 要把 token 交给 Bison，所以需要返回 `parser.tab.h` 里定义的 token，并通过 `yylval` 传递 AST 终结符节点。

### 3. 构建涉及的文件

实践 1.2 的核心源码文件如下：

```text
src/parser.y
src/parser_main.c
src/ast.h
src/ast.c
src/lexer.l
```

它们的关系如下：

```text
parser.y
  定义 Bison token、语法规则、优先级、错误恢复
  生成 build/parser.tab.c 和 build/parser.tab.h

lexer.l
  在 PARSER_BUILD 模式下包含 parser.tab.h
  识别 token 后把 AST 终结符节点写入 yylval
  生成 build/parser_lexer.c

ast.h / ast.c
  定义 AST 节点结构
  提供 AST 创建、打印、释放函数

parser_main.c
  打开输入文件
  调用 yyparse()
  如果没有错误就打印 AST
```

### 4. `parser_main.c`：语法分析器入口

`parser_main.c` 的结构和 1.1 的 `lexer_main.c` 类似，都是先打开输入文件，再交给 Flex/Bison 处理。

主要流程：

```c
yyin = fopen(argv[1], "r");
if (!yyin) {
    perror(argv[1]);
    return 1;
}

yyrestart(yyin);
int parse_result = yyparse();
```

`yyin` 仍然是 Flex 使用的输入文件指针。`yyrestart(yyin)` 用来重置 Flex 扫描器的输入流。不同的是，实践 1.1 中主程序直接循环调用 `yylex()`，而实践 1.2 中主程序调用的是 Bison 生成的 `yyparse()`。

`yyparse()` 内部会在需要下一个 token 时自动调用 `yylex()`。因此语法分析阶段的控制流程是：

```text
parser_main.c 调用 yyparse()
  yyparse() 根据文法分析输入
  yyparse() 需要 token 时调用 yylex()
  yylex() 从 yyin 中扫描字符并返回 token
```

`yyparse()` 是 Bison 根据 `parser.y` 自动生成的语法分析函数。它的工作不是逐字符扫描，而是根据文法规则维护一个语法分析栈：当栈顶符号能够匹配某条产生式右部时，就进行规约；当还需要更多输入时，就调用 `yylex()` 读取下一个 token。实践 1.1 中主函数直接控制扫描循环，实践 1.2 中控制权交给 `yyparse()`，由 Bison 决定什么时候读取 token、什么时候规约产生式、什么时候报告语法错误。

例如输入中出现 `return i;` 时，Flex 会依次返回 `RETURN`、`ID`、`SEMI`。Bison 根据文法先把 `ID` 规约为 `Exp`，再把 `RETURN Exp SEMI` 规约为 `Stmt`。规约时执行产生式后面的 C 代码动作，创建对应的 AST 节点。

语法分析结束后，主程序根据错误数量决定是否打印语法树：

```c
if (lexical_error_count == 0 && syntax_error_count == 0 && parse_result == 0) {
    ast_print(parse_root, 0);
}
```

这里同时检查了三类状态：

- `lexical_error_count == 0`：没有词法错误
- `syntax_error_count == 0`：没有语法错误
- `parse_result == 0`：Bison 分析过程正常结束

只有三者都满足时才打印 AST。这样可以避免错误输入下同时输出错误信息和不完整语法树。

### 5. `ast.h` 和 `ast.c`：语法树结构

AST 节点定义在 `ast.h` 中：

```c
typedef enum AstKind {
    AST_NONTERMINAL,
    AST_TOKEN
} AstKind;

typedef struct AstNode {
    AstKind kind;
    char *name;
    char *value;
    int line;
    size_t child_count;
    struct AstNode **children;
} AstNode;
```

AST 节点分为两类：

- `AST_TOKEN`：终结符节点，例如 `ID`、`INT`、`TYPE`、`RETURN`
- `AST_NONTERMINAL`：非终结符节点，例如 `Program`、`ExtDef`、`Stmt`、`Exp`

每个节点保存：

- `name`：节点名称
- `value`：终结符的文本值，非终结符通常为空
- `line`：行号
- `child_count`：子节点数量
- `children`：子节点数组

#### 5.1 终结符节点

终结符节点由 `ast_token()` 创建：

```c
AstNode *ast_token(const char *name, const char *value, int line) {
    AstNode *node = calloc(1, sizeof(*node));
    node->kind = AST_TOKEN;
    node->name = xstrdup(name);
    node->value = xstrdup(value);
    node->line = line;
    return node;
}
```

实践 1.2 中，Flex 在识别 token 时会调用这个函数。例如识别到标识符 `main` 时，会创建名称为 `ID`、值为 `main`、行号为当前行号的终结符节点。

#### 5.2 非终结符节点

非终结符节点由 `ast_node()` 创建：

```c
AstNode *ast_node(const char *name, int line, size_t child_count, ...)
```

这个函数使用可变参数接收多个子节点。Bison 每规约一个产生式，就可以用 `ast_node()` 构造对应的语法树节点。例如：

```yacc
Stmt
    : RETURN Exp SEMI
      { $$ = ast_node("Stmt", node_line($1), 3, $1, $2, $3); }
    ;
```

这条规则表示：当 Bison 识别到 `RETURN Exp SEMI` 时，构造一个 `Stmt` 节点，下面挂三个孩子：`RETURN`、`Exp`、`SEMI`。

#### 5.3 语法树打印

AST 打印由 `ast_print()` 完成：

```c
void ast_print(const AstNode *node, int indent)
```

非终结符输出格式：

```text
NodeName (line)
```

AST 打印采用的是先序深度优先遍历。也就是先输出当前节点，再从左到右递归输出它的所有子节点。代码中对应的结构是：

```c
printf("%s (%d)\n", node->name, node->line);
for (size_t i = 0; i < node->child_count; ++i) {
    ast_print(node->children[i], indent + 2);
}
```

这里 `indent + 2` 表示子节点比父节点多缩进两个空格，所以打印结果能够直观看出语法树的层次结构。课程样例要求的 AST 输出形式本质上就是这种先序遍历格式。

终结符根据类型输出：

```c
if (strcmp(node->name, "INT") == 0) {
    printf("INT: %ld\n", ast_parse_int(node->value));
} else if (strcmp(node->name, "FLOAT") == 0) {
    printf("FLOAT: %f\n", strtof(node->value, NULL));
} else if (strcmp(node->name, "ID") == 0 || strcmp(node->name, "TYPE") == 0) {
    printf("%s: %s\n", node->name, node->value);
} else {
    printf("%s\n", node->name);
}
```

这里对 `INT` 做了特殊处理：

```c
long ast_parse_int(const char *text) {
    return strtol(text, NULL, 0);
}
```

`strtol(text, NULL, 0)` 会根据前缀自动识别进制，所以 `0123` 会按八进制解释，`0x3F` 会按十六进制解释，输出时统一转成十进制。这是为了符合课程样例对 AST 中整数显示格式的要求。

#### 5.4 AST 的构造过程

AST 是在 Bison 规约产生式时逐层构造出来的。词法分析阶段先创建终结符节点，例如 `TYPE: int`、`ID: main`、`INT: 1`；语法分析阶段再根据产生式把这些终结符节点组合成非终结符节点。

AST 本身不是用来做规约检查的数据结构。规约检查由 Bison 生成的语法分析器完成，Bison 内部维护一个分析栈，并根据 `parser.y` 中的文法规则和自动生成的分析表决定当前应该移进 token 还是规约产生式。AST 是在“某条产生式已经被 Bison 判断可以规约”之后，由产生式动作代码创建出来的结果。

也就是说，语法检查和 AST 构造的关系是：

```text
Flex 读字符，返回 token
    ↓
Bison 根据 token 和文法规则做移进/规约判断
    ↓
如果某条产生式规约成功，执行 { ... } 中的 C 代码
    ↓
动作代码调用 ast_node()，把右侧符号的 AST 节点组合成左侧节点
```

例如对 `return i;`，Bison 的判断过程是：先根据 token 判断 `ID` 可以规约成 `Exp`，再判断 `RETURN Exp SEMI` 可以规约成 `Stmt`。只有当 `RETURN Exp SEMI` 这条产生式匹配成功时，动作代码才会执行：

```yacc
{ $$ = ast_node("Stmt", node_line($1), 3, $1, $2, $3); }
```

所以 AST 是语法分析成功过程的记录结果，而不是 Bison 用 AST 去反过来检查语法。

以表达式 `i = i + 1;` 为例，构造过程可以理解为：

```text
ID i
  -> Exp

ID i
  -> Exp

INT 1
  -> Exp

Exp PLUS Exp
  -> Exp

Exp ASSIGNOP Exp
  -> Exp

Exp SEMI
  -> Stmt
```

每次箭头右侧的非终结符，都是 Bison 规约时调用 `ast_node()` 创建出来的。最终这些局部节点会逐层挂到 `StmtList`、`CompSt`、`ExtDef`、`ExtDefList` 和 `Program` 下面，形成整棵语法树。

### 6. `parser.y`：Bison 语法分析核心

`parser.y` 是实践 1.2 的核心文件。它负责声明 token、设置优先级、编写文法产生式、构造 AST，并处理语法错误。

这里需要区分“语法规则”和“语义规则”。实践 1.2 主要完成的是语法分析，所以 `parser.y` 中定义的是 C-- 语言的语法规则，也就是哪些 token 序列可以组成变量声明、函数定义、语句、表达式等结构。Bison 根据这些语法规则生成 `yyparse()`，运行时由 `yyparse()` 判断输入 token 序列是否符合文法。

`parser.y` 的产生式后面虽然有 `{ ... }` 形式的 C 代码动作，Bison 文档中通常称为 semantic action，但这里的作用主要是构造 AST 和输出语法错误，并不是实践 2 中的语义分析。真正的语义检查，例如变量是否定义、类型是否匹配、函数参数是否一致，是后续实践 2 的 `semantic.c` 完成的。

因此实践 1.2 可以概括为：

```text
lexer.l 负责把字符流切成 token
parser.y 负责定义 token 如何组成合法语法结构
Bison 根据 parser.y 生成 yyparse()
yyparse() 根据文法判断语法是否正确
规约成功时，动作代码顺便构造 AST
```

#### 6.1 头部变量和辅助函数

文件开头定义了几个全局变量：

```c
AstNode *parse_root = NULL;
int syntax_error_count = 0;
static int last_syntax_error_line = 0;
```

`parse_root` 保存最终语法树根节点。`syntax_error_count` 记录语法错误数量。`last_syntax_error_line` 用来避免同一行重复输出多个语法错误。

`parse_root` 需要定义成全局变量，是因为最终语法树是在 `yyparse()` 内部构造出来的，而打印语法树发生在 `parser_main.c` 中。`yyparse()` 返回值只能表示分析是否成功，不能直接返回一棵 AST，所以在 `Program` 规约成功时把根节点保存到 `parse_root`，主函数再通过这个全局变量访问整棵语法树。

行号辅助函数：

```c
static int node_line(AstNode *node) {
    return node ? node->line : yylineno;
}
```

构造非终结符节点时，需要给节点设置行号。通常使用第一个子节点的行号；如果节点为空，就使用当前 `yylineno`。

语法错误输出函数：

```c
static void syntax_error_at(int line, const char *msg) {
    if (lexical_error_count == 0 && line != last_syntax_error_line) {
        printf("Error type B at Line %d: %s.\n", line, msg);
        last_syntax_error_line = line;
    }
    syntax_error_count++;
}
```

这里有两个处理：

- 如果存在词法错误，就不再输出语法错误，避免 A 类错误和 B 类错误混在一起。
- 同一行只输出一次语法错误，减少重复报错。

#### 6.2 Bison 的类型和 token 声明

`%union` 定义 Bison 语义值类型：

```yacc
%union {
    AstNode *node;
}
```

这表示 Bison 里的终结符和非终结符都可以携带一个 `AstNode *`。

Bison 的每个语法符号都可以带一个“语义值”。如果不声明 `%union`，Bison 默认的语义值类型很有限，不能直接保存 AST 节点指针。这里用 `%union` 声明语义值中有一个字段 `node`，类型是 `AstNode *`，这样 token 和非终结符就都能携带语法树节点。

token 声明：

```yacc
%token <node> INT FLOAT ID SEMI COMMA ASSIGNOP RELOP PLUS MINUS STAR DIV
%token <node> AND OR DOT NOT TYPE LP RP LB RB LC RC STRUCT RETURN IF ELSE WHILE
```

这些 token 都带有 `<node>` 类型，即它们的语义值是 AST 节点指针。Flex 在 `PARSER_BUILD` 模式下会把 token 节点写入 `yylval.node`，Bison 就可以在产生式动作中使用 `$1`、`$2` 等变量访问这些节点。

`yylval` 是 Flex 和 Bison 之间传递语义值的全局变量。Flex 返回 token 类型的同时，把该 token 对应的 AST 节点写入 `yylval.node`。Bison 接收到 token 后，就能在产生式动作中通过 `$1`、`$2` 等编号取出这些节点。

非终结符类型声明：

```yacc
%type <node> Program ExtDefList ExtDef ExtDecList Specifier StructSpecifier
%type <node> OptTag Tag VarDec FunDec VarList ParamDec CompSt StmtList Stmt
%type <node> DefList Def DecList Dec Exp Args InitList
```

这表示这些非终结符规约后也返回 `AstNode *`。

Bison 产生式动作中的 `$$` 表示当前产生式左侧非终结符的语义值，`$1`、`$2`、`$3` 表示右侧第 1、2、3 个符号的语义值。例如：

```yacc
Stmt
    : RETURN Exp SEMI
      { $$ = ast_node("Stmt", node_line($1), 3, $1, $2, $3); }
    ;
```

这条规则中，`$1` 是 `RETURN` 节点，`$2` 是 `Exp` 节点，`$3` 是 `SEMI` 节点。`$$` 是规约后得到的 `Stmt` 节点。动作代码把三个子节点挂到新的 `Stmt` 节点下面，并把这个新节点作为当前产生式的结果传给上层文法继续使用。

#### 6.3 表达式优先级和结合性

表达式优先级在 `parser.y` 中这样设置：

```yacc
%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT UMINUS
%left LP RP LB RB DOT
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE
```

从上到下优先级逐渐升高。赋值是右结合；加减、乘除是左结合；一元负号和逻辑非是右结合。数组访问、函数调用、结构体成员访问优先级最高。

`LOWER_THAN_ELSE` 和 `ELSE` 用来解决悬挂 else 问题：

```yacc
| IF LP Exp RP Stmt %prec LOWER_THAN_ELSE
| IF LP Exp RP Stmt ELSE Stmt
```

没有 `ELSE` 的 `if` 语句被人为设置为较低优先级，这样当后面出现 `ELSE` 时，Bison 会把 `ELSE` 归给最近的未匹配 `if`，符合 C 类语言习惯。

悬挂 else 的典型输入是：

```c
if (a)
    if (b)
        x = 1;
    else
        x = 2;
```

这里的 `else` 应该匹配内层 `if (b)`，而不是外层 `if (a)`。如果不设置优先级，Bison 会遇到移进/规约冲突：它既可以把 `if (b) x = 1;` 先规约成一条完整语句，也可以继续读取 `else`。通过给无 `else` 的规则设置 `%prec LOWER_THAN_ELSE`，再让 `ELSE` 的优先级更高，Bison 会选择继续移进 `ELSE`，从而把它绑定到最近的 `if`。

#### 6.4 程序和外部定义文法

语法分析入口是 `Program`：

```yacc
Program
    : ExtDefList
      { parse_root = ast_node("Program", node_line($1), 1, $1); $$ = parse_root; }
    ;
```

`Program` 规约成功后，会创建根节点，并赋值给全局变量 `parse_root`。后续 `parser_main.c` 就通过这个全局变量打印整棵语法树。

这里同时写了：

```yacc
$$ = parse_root;
```

是为了让 `Program` 这个非终结符本身也有语义值。虽然主程序最终通过 `parse_root` 打印语法树，但保持 `$$` 的赋值可以让文法动作在结构上保持完整。

外部定义列表：

```yacc
ExtDefList
    : ExtDef ExtDefList
      { $$ = $2 ? ast_node("ExtDefList", node_line($1), 2, $1, $2)
                : ast_node("ExtDefList", node_line($1), 1, $1); }
    | /* empty */
      { $$ = NULL; }
    ;
```

这里空产生式返回 `NULL`。构造 AST 时，如果右侧列表为空，就只保留非空子节点。这样打印语法树时不会输出空产生式节点，符合课程样例格式。

#### 6.5 变量、函数和复合语句

变量声明：

```yacc
VarDec
    : ID
      { $$ = ast_node("VarDec", node_line($1), 1, $1); }
    | VarDec LB INT RB
      { $$ = ast_node("VarDec", node_line($1), 4, $1, $2, $3, $4); }
    | VarDec LB FLOAT RB
      { syntax_error_at(node_line($3), "Array dimension must be integer"); $$ = ast_node("VarDec", node_line($1), 4, $1, $2, $3, $4); }
    ;
```

`ID` 表示普通变量；`VarDec LB INT RB` 表示数组变量；`VarDec LB FLOAT RB` 是错误恢复规则，用来捕获数组维度不是整数的语法错误。

函数声明：

```yacc
FunDec
    : ID LP VarList RP
    | ID LP VarList COMMA RP
    | ID LP RP
    ;
```

其中 `ID LP VarList COMMA RP` 用来检查函数形参列表末尾多余逗号。

复合语句：

```yacc
CompSt
    : LC DefList StmtList RC
```

`DefList` 是局部定义列表，`StmtList` 是语句列表。构造 AST 时会根据 `DefList`、`StmtList` 是否为空选择不同子节点数量，避免打印空节点。

#### 6.6 语句文法

语句规则包括表达式语句、复合语句、返回语句、条件语句和循环语句：

```yacc
Stmt
    : Exp SEMI
    | CompSt
    | RETURN Exp SEMI
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE
    | IF LP Exp RP Stmt ELSE Stmt
    | WHILE LP Exp RP Stmt
    ;
```

同时增加了若干错误恢复规则：

```yacc
| Exp DOT SEMI
  { syntax_error_at(node_line($2), "Missing member name after '.'"); ... }
| RETURN SEMI
  { syntax_error_at(node_line($1), "Missing return value in non-void function"); ... }
| WHILE LP Exp RP SEMI
  { syntax_error_at(node_line($5), "Empty while statement body"); ... }
| Def
  { syntax_error_at(node_line($1), "Variable declaration after statements in block"); ... }
| error SEMI
  { yyerrok; $$ = NULL; }
```

这些规则的作用不是改变语言定义，而是在遇到常见错误时尽量把错误限制在局部范围内。例如 `error SEMI` 表示遇到一条错误语句时，可以跳到分号后继续分析后面的内容，避免一个错误导致后续多行都被误报。

`error` 是 Bison 内置的特殊错误符号。语法分析器发现当前输入无法匹配任何正常产生式时，会进入错误恢复流程，丢弃部分栈内容或输入 token，直到找到能接收 `error` 的位置。`error SEMI` 的含义是：如果一条语句中间出现语法错误，就把错误内容跳过，直到遇到分号为止，然后把这一条错误语句结束掉。

`yyerrok` 用来告诉 Bison 当前错误已经被处理，后续可以恢复正常报错。如果不调用 `yyerrok`，Bison 可能还处于错误恢复状态，短时间内会抑制后续错误信息。这里在 `error SEMI` 后调用 `yyerrok`，是为了让分号后的代码能按正常语法继续分析。

这种恢复方式的目的不是让错误代码变正确，而是让一个局部错误不要破坏后面代码的分析。例如一行表达式少了操作数，如果没有恢复规则，后面的函数、语句可能都会被连带误报；有了 `error SEMI` 后，语法分析器可以在分号处重新同步。

#### 6.7 表达式文法

表达式规则覆盖赋值、逻辑运算、关系运算、算术运算、括号、一元运算、函数调用、数组访问、结构体成员访问和基本表达式：

```yacc
Exp
    : Exp ASSIGNOP Exp
    | Exp AND Exp
    | Exp OR Exp
    | Exp RELOP Exp
    | Exp PLUS Exp
    | Exp MINUS Exp
    | Exp STAR Exp
    | Exp DIV Exp
    | LP Exp RP
    | MINUS Exp %prec UMINUS
    | NOT Exp
    | ID LP Args RP
    | ID LP RP
    | Exp LB Exp RB
    | Exp DOT ID
    | ID
    | INT
    | FLOAT
    ;
```

每条产生式动作都是构造一个 `Exp` 节点，把参与规约的子节点挂到下面。例如：

```yacc
| Exp PLUS Exp
  { $$ = ast_node("Exp", node_line($1), 3, $1, $2, $3); }
```

这表示表达式 `a + b` 会形成一个 `Exp` 节点，下面依次是左表达式、`PLUS`、右表达式。

表达式中也处理了部分错误情况：

```yacc
| Exp PLUS ASSIGNOP Exp
  { syntax_error_at(node_line($2), "Unsupported compound assignment operator '+='"); ... }
| ID LP Args COMMA RP
  { syntax_error_at(node_line($4), "Trailing comma in function call arguments"); ... }
```

其中 `Exp PLUS ASSIGNOP Exp` 实际上捕获的是 `+=` 这种不属于 C-- 文法的复合赋值写法，因为词法阶段会把 `+=` 拆成 `PLUS` 和 `ASSIGNOP`。

#### 6.8 默认语法错误处理

Bison 默认遇到语法错误时会调用 `yyerror()`：

```c
void yyerror(const char *msg) {
    (void)msg;
    if (lexical_error_count == 0 && yylineno != last_syntax_error_line) {
        printf("Error type B at Line %d: Syntax error.\n", yylineno);
        last_syntax_error_line = yylineno;
    }
    syntax_error_count++;
}
```

这里没有直接打印 Bison 的英文错误信息，而是统一输出课程要求的 B 类错误格式。`last_syntax_error_line` 用来避免同一行重复输出错误。`lexical_error_count == 0` 则保证如果已经有词法错误，就不再额外输出语法错误。

### 7. 样例输入和输出

#### 7.1 语法错误样例

运行：

```sh
build/cmmc test/proj1-2-3-6-exp2-1.cmm
```

输出：

```text
Error type B at Line 5: Syntax error.
Error type B at Line 6: Syntax error.
```

这个样例说明语法分析器能够在语法错误输入上输出 B 类错误，并且不会打印不完整 AST。

#### 7.2 简单函数 AST 样例

运行：

```sh
build/cmmc test/proj1-2-3-6-exp3-1.cmm
```

输出片段：

```text
Program (1)
  ExtDefList (1)
    ExtDef (1)
      Specifier (1)
        TYPE: int
      FunDec (1)
        ID: inc
        LP
        RP
      CompSt (2)
        LC
        DefList (3)
          Def (3)
            Specifier (3)
              TYPE: int
            DecList (3)
              Dec (3)
                VarDec (3)
                  ID: i
            SEMI
        StmtList (4)
          Stmt (4)
            Exp (4)
              Exp (4)
                ID: i
              ASSIGNOP
              Exp (4)
                Exp (4)
                  ID: i
                PLUS
                Exp (4)
                  INT: 1
            SEMI
        RC
```

这个输出体现了 AST 的层次结构。`Program` 是根节点，下面是外部定义列表；函数定义中包含返回类型、函数声明和复合语句；复合语句中又包含局部变量定义和语句列表。

#### 7.3 结构体 AST 样例

运行：

```sh
build/cmmc test/proj1-2-3-6-exp4-1.cmm
```

这个样例用于检查结构体定义、结构体变量声明和成员访问相关的语法树。输出中可以看到 `StructSpecifier`、`DefList`、`Exp DOT ID` 等节点，说明结构体相关文法已经接入语法分析器。

### 8. 本阶段实现小结

实践 1.2 的构建顺序是先用 Bison 生成 parser，再用 Flex 以 `PARSER_BUILD` 模式生成 lexer，最后把 parser、lexer、AST 模块和主函数链接成 `build/cmmc`。运行时由 `parser_main.c` 调用 `yyparse()`，Bison 在分析过程中调用 Flex 的 `yylex()` 获取 token。每个 token 和非终结符都携带 `AstNode *`，所以产生式规约时可以同步构造 AST。语法正确时打印完整语法树，存在词法或语法错误时只输出错误信息。

## 实践 2 语义分析

### 1. 本阶段目标

实践 2 在实践 1 的词法分析和语法分析基础上继续实现语义分析。词法和语法阶段只能判断程序的形式结构是否正确，例如 token 是否合法、语句和表达式是否符合文法；语义分析要进一步判断程序含义是否正确，例如变量是否定义、函数是否存在、赋值左右类型是否一致、数组和结构体访问是否合法。

本阶段主要完成以下功能：

- 建立 C-- 类型系统，支持 `int`、`float`、数组、结构体和函数类型
- 建立符号表和作用域结构
- 检查变量、函数、结构体的定义和重定义
- 检查赋值、运算、返回语句、函数调用、数组访问、结构体成员访问
- 支持函数声明和定义一致性检查
- 支持内置函数 `read` 和 `write`
- 只在词法和语法分析成功后进入语义分析

### 2. 构建顺序

在实践 2 所在目录执行：

```sh
cd /home/leyi/Desktop/submission/proj2
make semantic
```

`make semantic` 对应 `Makefile` 中的目标：

```make
semantic: build/cmmc

build/cmmc: src/parser.y src/lexer.l src/parser_main.c src/ast.c src/ast.h src/semantic.c src/semantic.h
	@mkdir -p build
	bison -d -o build/parser.tab.c src/parser.y
	flex -DPARSER_BUILD -o build/parser_lexer.c src/lexer.l
	gcc -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -I src -I build \
		src/parser_main.c src/ast.c src/semantic.c build/parser.tab.c build/parser_lexer.c -lfl -o build/cmmc
```

和实践 1.2 相比，实践 2 的构建多加入了：

```text
src/semantic.c
src/semantic.h
```

构建流程仍然是先由 Bison 生成 parser，再由 Flex 生成 parser 模式下的 lexer，最后把 parser、lexer、AST 模块和语义分析模块一起链接成 `build/cmmc`。

### 3. 运行入口

实践 2 的 `parser_main.c` 在实践 1.2 基础上增加了语义分析模式：

```c
int semantic_errors = 0;
if (lexical_error_count == 0 && syntax_error_count == 0 && parse_result == 0 &&
    getenv("CMM_SEMANTIC") != NULL) {
    semantic_errors = semantic_check(parse_root);
} else if (lexical_error_count == 0 && syntax_error_count == 0 && parse_result == 0) {
    ast_print(parse_root, 0);
}
```

这里的逻辑是：只有词法错误数为 0、语法错误数为 0、Bison 分析结果正常，并且设置了环境变量 `CMM_SEMANTIC`，程序才调用：

```c
semantic_check(parse_root);
```

因此实践 2 的语义分析运行方式是：

```sh
CMM_SEMANTIC=1 build/cmmc test/proj2-test1.cmm
```

如果不加 `CMM_SEMANTIC=1`，同一个可执行文件仍然保持实践 1.2 的行为，也就是语法正确时打印 AST。这样前端语法树输出和语义分析功能可以共用一套构建结果。

### 4. `semantic.h`：语义分析接口

`semantic.h` 中只暴露一个入口函数：

```c
int semantic_check(AstNode *root);
```

这说明语义分析的输入是实践 1.2 构造出的 AST 根节点，而不是原始源码文本。实践 2 的整体流程可以表示为：

```text
C-- 源码
  -> lexer.l 词法分析
  -> parser.y 语法分析并生成 AST
  -> semantic_check(AST) 做语义检查
```

### 5. `semantic.c`：语义分析核心

`semantic.c` 是实践 2 的核心文件，里面主要包含四部分内容：

```text
类型系统
符号表和作用域
AST 遍历分析函数
语义错误检查逻辑
```

#### 5.1 类型系统

类型种类定义为：

```c
typedef enum {
    TY_ERROR,
    TY_INT,
    TY_FLOAT,
    TY_ARRAY,
    TY_STRUCT,
    TY_FUNC
} TypeKind;
```

对应 C-- 中的基本类型、数组类型、结构体类型和函数类型。`TY_ERROR` 是错误类型，用于抑制连锁报错。

统一的类型结构是：

```c
struct Type {
    TypeKind kind;
    Type *elem;
    int size;
    char *struct_name;
    Field *fields;
    Type *ret;
    Field *params;
    int param_count;
    int defined;
    int declared_line;
};
```

这个结构根据 `kind` 的不同解释不同字段：

- `TY_ARRAY` 使用 `elem` 和 `size` 表示数组元素类型和数组长度
- `TY_STRUCT` 使用 `struct_name` 和 `fields` 表示结构体名和字段列表
- `TY_FUNC` 使用 `ret`、`params`、`param_count`、`defined` 表示函数返回类型、参数和是否已定义

类型比较由 `type_equal()` 完成。基本类型要求类型种类相同；数组要求维度大小和元素类型一致；结构体如果有名字，则按结构体名比较；函数要求返回类型和参数列表一致。函数中如果遇到 `TY_ERROR`，会认为类型比较通过，这样可以避免一个前置错误继续引发大量后续类型错误。

#### 5.2 字段、符号和作用域

字段结构：

```c
struct Field {
    char *name;
    Type *type;
    int line;
    Field *next;
};
```

字段用于表示结构体成员，也用于表示函数参数列表。

例如结构体：

```c
struct Point {
    int x;
    float y;
};
```

它的字段链表可以表示为：

```text
x:int -> y:float
```

这个链表挂在 `TY_STRUCT` 类型的 `fields` 字段下。后续分析 `p.x` 时，会到这个字段链表里查找 `x`，找到后表达式 `p.x` 的类型就是 `int`；如果访问 `p.z`，字段链表中找不到 `z`，就会报结构体字段不存在错误。

函数参数也复用 `Field` 链表。例如：

```c
int add(int a, float b)
```

参数可以表示为：

```text
a:int -> b:float
```

这个链表挂在 `TY_FUNC` 类型的 `params` 字段下。函数调用时，语义分析会把实参类型也整理成一个列表，再和形参列表比较。

符号结构：

```c
struct Symbol {
    char *name;
    Type *type;
    int line;
    Symbol *next;
};
```

符号用于记录变量名、函数名以及它们对应的类型。

例如：

```c
int a;
float b;
```

当前作用域的符号表可以表示为：

```text
a:int -> b:float
```

后续遇到表达式 `a = 1;` 时，语义分析会通过 `find_symbol("a")` 在符号表中查找 `a`。如果能找到，就得到变量类型；如果找不到，就报未定义变量。函数名也会作为 `Symbol` 存入函数表，只是它的 `type` 是 `TY_FUNC`。

作用域结构：

```c
struct Scope {
    Symbol *symbols;
    Scope *parent;
};
```

每个作用域保存当前层定义的变量，`parent` 指向外层作用域。查找变量时从当前作用域开始，一层层向外查找：

```c
static Symbol *find_symbol(const char *name) {
    for (Scope *scope = current_scope; scope; scope = scope->parent) {
        Symbol *sym = find_in_scope(scope, name);
        if (sym) {
            return sym;
        }
    }
    return NULL;
}
```

新增作用域使用 `push_scope()`，退出作用域使用 `pop_scope()`。函数体和复合语句会产生新的作用域。

例如：

```c
int x;

int main() {
    int a;
    {
        float b;
        a = 1;
        b = 2.0;
        x = 3;
    }
}
```

作用域链可以表示为：

```text
内部块作用域：b:float
  -> main 函数作用域：a:int
      -> 全局作用域：x:int, main:function
```

在内部块中查找 `b`，当前作用域就能找到；查找 `a`，需要到外层函数作用域；查找 `x`，需要继续到全局作用域；如果所有作用域都找不到，就报未定义变量。

### 6. 语义分析入口流程

语义分析总入口是：

```c
int semantic_check(AstNode *root) {
    semantic_errors = 0;
    current_scope = NULL;
    global_functions = NULL;
    struct_defs = NULL;
    pending_errors = NULL;
    no_scope_conflicts = getenv("CMM_NO_SCOPE") != NULL;
    push_scope();
    add_builtin_functions();
    if (root && root->child_count > 0) {
        analyze_ext_def_list(child(root, 0));
    }
    check_undefined_functions();
    flush_pending_errors();
    pop_scope();
    return semantic_errors;
}
```

执行顺序是：

1. 清空语义分析状态。
2. 建立全局作用域。
3. 加入内置函数 `read` 和 `write`。
4. 从 `Program` 的 `ExtDefList` 开始遍历 AST。
5. 检查只有声明没有定义的函数。
6. 输出延迟保存的错误。
7. 退出全局作用域并返回错误数量。

内置函数在 `add_builtin_functions()` 中加入：

```text
read() -> int
write(int) -> int
```

所以源程序中可以直接调用 `read` 和 `write`，不用用户自己声明。

`read` 和 `write` 可以类比成简化版的 `scanf` 和 `printf`。`read()` 读取一个整数并返回 `int`，`write(int)` 输出一个整数。实践 2 中提前把它们加入函数表，是为了让语义分析能够识别它们并检查调用是否合法。如果不提前加入，程序中出现 `read()` 或 `write(x)` 时，语义分析会把它们误报为未定义函数。

这两个内置函数在后续阶段还有实际作用。实践 3 会把 `read()` 和 `write(x)` 翻译成专门的 `READ`、`WRITE` 中间代码；实践 4 再把 `READ`、`WRITE` 翻译成 SPIM/MIPS 的输入输出 `syscall`。因此它们在实践 2 中是内置函数符号，在实践 3 中是 IR 输入输出指令，在实践 4 中对应真正的系统调用。

### 7. AST 遍历结构

语义分析按 AST 的语法结构递归进行：

```text
analyze_ext_def_list()
  -> analyze_ext_def()
      处理全局变量、函数声明、函数定义、结构体定义

analyze_compst()
  -> analyze_def_list()
      处理局部变量定义
  -> analyze_stmt_list()
      处理语句列表

analyze_stmt()
  -> 处理 return / if / while / 表达式语句

analyze_exp()
  -> 推导表达式类型并检查表达式语义
```

实践 1.2 的 AST 表示程序结构，实践 2 就沿着这棵树检查每个结构是否满足语义约束。比如 `ExtDef` 表示外部定义，语义分析会判断它是全局变量、函数声明还是函数定义；`Exp` 表示表达式，语义分析会根据子节点判断它是变量、赋值、二元运算、函数调用、数组访问还是结构体成员访问。

这些分析函数有明确的先后顺序，基本原则是先处理定义，再处理使用；先建立环境，再检查表达式。总入口 `semantic_check()` 先建立全局作用域并加入内置函数，然后从 `ExtDefList` 开始处理全局定义。遇到函数定义时，先把函数加入函数表，再进入函数作用域，把形参加入当前作用域，然后分析函数体。函数体中先分析 `DefList`，把局部变量加入符号表，再分析 `StmtList`，检查语句和表达式。表达式内部再递归分析子表达式，自底向上推导类型。

例如：

```c
int add(int a, int b) {
    int c;
    c = a + b;
    return c;
}
```

分析顺序是：

```text
读取函数返回类型 int
读取函数名 add
收集参数 a:int, b:int
把 add 加入函数表
进入函数作用域
把参数 a、b 加入当前作用域
处理局部定义 int c，把 c 加入符号表
分析 c = a + b，查找 c、a、b 并检查赋值和运算类型
分析 return c，检查 c 的类型是否等于函数返回类型
退出函数作用域
```

这个顺序保证了后面使用变量或函数时，前面已经把相关定义放入符号表或函数表。

### 8. 典型语义检查

#### 8.1 未定义变量

在 `analyze_exp()` 中，如果表达式只有一个子节点，并且这个子节点是 `ID`，说明当前表达式是变量使用：

```c
if (is_node(leaf, "ID")) {
    Symbol *sym = find_symbol(leaf->value);
    if (!sym) {
        report_error(1, leaf->line, "Undefined Variable");
        return type_error();
    }
    *is_lvalue = 1;
    return sym->type;
}
```

检查逻辑是：用变量名去当前作用域和外层作用域中查找。如果找不到，就输出错误类型 1：未定义变量。返回 `type_error()` 可以避免后续表达式继续基于这个未知变量产生连锁错误。

#### 8.2 赋值类型检查

赋值表达式在 `analyze_exp()` 中处理：

```c
if (is_node(op, "ASSIGNOP")) {
    int left_lvalue = 0, right_lvalue = 0;
    Type *left = analyze_exp(child(node, 0), &left_lvalue);
    Type *right = analyze_exp(child(node, 2), &right_lvalue);
    if (left->kind == TY_ERROR || right->kind == TY_ERROR) {
        return left;
    }
    if (!left_lvalue) {
        report_error(6, op->line, "The left-hand side of an assignment must be a variable");
    } else if (!type_equal(left, right)) {
        report_error(5, op->line, "Type mismatched for assignment");
    }
    return left;
}
```

这里同时检查两件事：

- 赋值左侧必须是左值，例如变量、数组元素或结构体字段。
- 赋值左右两侧类型必须一致。

`is_lvalue` 是通过参数传出的标记。普通变量、数组元素、结构体字段会把它设置为 1；普通计算表达式、函数调用结果、常量不会是左值。

#### 8.3 运算对象类型检查

二元运算由 `binary_numeric()` 处理：

```c
Type *left = analyze_exp(child(node, 0), &l1);
Type *right = analyze_exp(child(node, 2), &l2);
if (!is_numeric(left) || !is_numeric(right) || !type_equal(left, right)) {
    report_error(7, child(node, 1)->line, "Type mismatched for operands");
    return type_error();
}
```

这里要求运算对象是数字类型，并且左右类型一致。关系运算、逻辑运算返回 `int`，普通算术运算返回左操作数类型。

#### 8.4 函数调用检查

函数调用分为无参调用和有参调用。无参调用时，如果函数不存在，就检查同名变量是否存在：

```c
Symbol *func = find_function(id->value);
Symbol *var = find_symbol(id->value);
if (!func) {
    report_error(var ? 11 : 2, id->line, var ? "Not a function" : "Undefined function");
    return type_error();
}
```

如果没有函数定义但有同名变量，说明是“对非函数使用函数调用”，报错误类型 11；如果函数和变量都没有，报错误类型 2：未定义函数。

有参调用会把实参表达式转成字段列表，再和函数形参列表比较：

```c
Field *args = args_from_node(child(node, 2), &arg_count);
if (arg_count != func->type->param_count || !field_list_equal(args, func->type->params)) {
    report_error(9, id->line, "Function is not applicable for arguments");
    return type_error();
}
```

#### 8.5 数组访问检查

数组访问对应 AST 形态：

```text
Exp LB Exp RB
```

语义分析逻辑是：

```c
Type *base = analyze_exp(child(node, 0), &lv);
Type *idx = analyze_exp(child(node, 2), &idx_lv);
if (base->kind != TY_ARRAY) {
    report_error(10, child(node, 1)->line, "Not an array");
    return type_error();
}
if (idx->kind != TY_INT && idx->kind != TY_ERROR) {
    report_error(12, child(node, 2)->line, "Not an integer");
}
*is_lvalue = 1;
return base->elem;
```

这里检查两点：

- 被访问对象必须是数组。
- 下标必须是 `int`。

数组元素可以作为赋值左侧，所以成功时会把 `is_lvalue` 设置为 1，并返回数组元素类型。

#### 8.6 结构体成员访问检查

结构体成员访问对应 AST 形态：

```text
Exp DOT ID
```

语义分析逻辑是：

```c
Type *base = analyze_exp(child(node, 0), &lv);
AstNode *field_id = child(node, 2);
if (base->kind != TY_STRUCT) {
    report_error(13, op->line, "Illegal use of \".\"");
    return type_error();
}
Field *field = find_field(base->fields, field_id->value);
if (!field) {
    report_error(14, field_id->line, "Not-existen field");
    return type_error();
}
*is_lvalue = 1;
return field->type;
```

这里先检查点号左边是不是结构体，再检查结构体字段列表中是否存在右边的字段名。结构体字段访问成功后也是左值。

#### 8.7 返回类型检查

返回语句在 `analyze_stmt()` 中处理：

```c
} else if (is_node(first, "RETURN")) {
    int dummy = 0;
    Type *actual = analyze_exp(child(node, 1), &dummy);
    if (!type_equal(return_type, actual)) {
        report_error(8, first->line, "Type mismatched for return");
    }
}
```

这里 `return_type` 是当前函数定义时传入的返回类型，`actual` 是 return 后表达式的实际类型。如果二者不一致，就报错误类型 8。

### 9. 本阶段实现小结

实践 2 的核心是 `semantic.c`。它接收实践 1.2 生成的 AST，从根节点开始递归遍历，在遍历过程中建立符号表、维护作用域、构造和比较类型，并在变量使用、函数调用、赋值、运算、数组访问、结构体访问、返回语句等位置检查语义约束。语法分析解决的是 token 序列能否组成合法结构，语义分析解决的是这些结构在含义上是否正确。

## 实践 3 中间代码生成

### 1. 本阶段目标

实践 3 在词法分析、语法分析和语义分析都正确的基础上，把 C-- 程序翻译成三地址形式的中间代码 IR。IR 是源语言和目标汇编之间的中间表示，后续实践 4 的 MIPS 生成和实践 5 的 IR 优化都以它为基础。

本阶段主要完成以下功能：

- 复用前端生成的 AST
- 在生成 IR 前先执行语义检查
- 生成函数、参数、返回语句相关 IR
- 生成赋值、算术表达式、条件跳转、循环跳转相关 IR
- 生成函数调用、实参传递相关 IR
- 处理数组和结构体的地址计算
- 将内置函数 `read`、`write` 翻译成 `READ`、`WRITE` 指令

### 2. 构建顺序

在实践 3 所在目录执行：

```sh
cd /home/leyi/Desktop/submission/proj3
make ir
```

`make ir` 对应 `Makefile` 中的目标：

```make
ir: build/cmmc

build/cmmc: src/parser.y src/lexer.l src/parser_main.c src/ast.c src/ast.h src/semantic.c src/semantic.h src/irgen.c src/irgen.h
	@mkdir -p build
	bison -d -o build/parser.tab.c src/parser.y
	flex -DPARSER_BUILD -o build/parser_lexer.c src/lexer.l
	gcc -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -I src -I build \
		src/parser_main.c src/ast.c src/semantic.c src/irgen.c build/parser.tab.c build/parser_lexer.c -lfl -o build/cmmc
```

和实践 2 相比，实践 3 的构建多加入了：

```text
src/irgen.c
src/irgen.h
```

构建流程仍然是先由 Bison 生成 parser，再由 Flex 生成 parser 模式下的 lexer，最后把 parser、lexer、AST 模块、语义分析模块和 IR 生成模块一起链接成 `build/cmmc`。

### 3. 运行入口

实践 3 的 `parser_main.c` 增加了 IR 模式：

```c
int ir_mode = getenv("CMM_IR") != NULL;
```

如果设置了 `CMM_IR=1`，程序要求输入文件和输出 IR 文件两个参数：

```sh
CMM_IR=1 build/cmmc test/proj3-test1.cmm /tmp/proj3-test1.ir
```

核心逻辑是：

```c
if (lexical_error_count == 0 && syntax_error_count == 0 && parse_result == 0 &&
    ir_mode) {
    semantic_errors = semantic_check(parse_root);
    if (semantic_errors == 0) {
        ir_errors = ir_generate(parse_root, argv[2]);
    }
}
```

实践 3 不是语法正确就直接生成 IR，而是先调用 `semantic_check(parse_root)`。只有语义检查没有错误时，才调用：

```c
ir_generate(parse_root, argv[2]);
```

因此 IR 生成的输入一定是通过语义检查的 AST。

### 4. `irgen.h`：IR 生成接口

`irgen.h` 中只暴露一个入口：

```c
int ir_generate(AstNode *root, const char *output_path);
```

它的输入是 AST 根节点，输出是 IR 文件路径。实践 3 的核心工作就是遍历 AST，把对应的语句和表达式写入 `output_path`。

这里的 `root` 是内存中的 AST 根节点，不是语法树文件。AST 是 Bison 在 `yyparse()` 过程中构造出来的，保存在 `parse_root` 中。`output_path` 才是磁盘上的 IR 输出文件路径。例如执行：

```sh
CMM_IR=1 build/cmmc test/proj3-test1.cmm /tmp/proj3-test1.ir
```

此时：

```text
argv[1] = test/proj3-test1.cmm
argv[2] = /tmp/proj3-test1.ir
```

`ir_generate(parse_root, argv[2])` 表示遍历内存中的 AST，把生成的 IR 写入 `/tmp/proj3-test1.ir`。

### 5. `irgen.c`：IR 生成核心

`irgen.c` 主要包含以下内容：

```text
IR 阶段的类型系统
IR 阶段的符号表和函数表
临时变量和标签生成
AST 到 IR 的翻译函数
```

#### 5.1 IR 阶段类型系统

IR 生成阶段定义了自己的轻量级类型：

```c
typedef enum {
    IR_TY_INT,
    IR_TY_FLOAT,
    IR_TY_ARRAY,
    IR_TY_STRUCT
} IrTypeKind;
```

对应 C-- 中的基本类型、数组和结构体。IR 生成阶段仍然需要类型信息，因为数组访问需要知道元素大小，结构体访问需要知道字段偏移，函数调用需要知道数组和结构体参数是否按地址传递。

IR 类型结构：

```c
struct IrType {
    IrTypeKind kind;
    IrType *elem;
    int size;
    char *struct_name;
    IrField *fields;
};
```

数组类型使用 `elem` 和 `size` 表示元素类型和维度大小；结构体类型使用 `struct_name` 和 `fields` 表示结构体名和字段列表。

字段结构：

```c
struct IrField {
    char *name;
    IrType *type;
    int offset;
    IrField *next;
};
```

这里的 `offset` 是实践 3 相比实践 2 更关注的内容。结构体字段访问要翻译成地址偏移，所以每个字段需要记录它在结构体内部的偏移。例如：

```c
struct Point {
    int x;
    int y;
};
```

字段偏移可以表示为：

```text
x offset = 0
y offset = 4
```

访问 `p.y` 时，就可以生成类似：

```text
t1 := &p + #4
```

#### 5.2 IR 输出和临时变量

IR 输出文件由全局变量保存：

```c
static FILE *out;
```

所有 IR 指令通过 `emit()` 输出：

```c
static void emit(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(out, fmt, ap);
    va_end(ap);
    fputc('\n', out);
}
```

临时变量由 `new_temp()` 生成：

```c
static char *new_temp(void) {
    return xasprintf("t%d", ++temp_id);
}
```

生成结果是：

```text
t1, t2, t3, ...
```

标签由 `new_label()` 生成：

```c
static char *new_label(void) {
    return xasprintf("label%d", ++label_id);
}
```

生成结果是：

```text
label1, label2, label3, ...
```

临时变量用于保存中间表达式结果，标签用于表示控制流跳转目标。

### 6. IR 生成总入口

`ir_generate()` 是实践 3 的总入口：

```c
int ir_generate(AstNode *root, const char *output_path) {
    out = fopen(output_path, "w");
    temp_id = 0;
    label_id = 0;
    current_scope = NULL;
    struct_defs = NULL;
    functions = NULL;
    push_scope();
    AstNode *ext_def_list = root && root->child_count > 0 ? child(root, 0) : NULL;
    collect_function_sigs(ext_def_list);
    gen_ext_def_list(ext_def_list);
    pop_scope();
    fclose(out);
    return 0;
}
```

执行顺序是：

1. 打开输出 IR 文件。
2. 初始化临时变量编号和标签编号。
3. 建立 IR 阶段作用域。
4. 从 AST 根节点取出 `ExtDefList`。
5. 先收集函数签名。
6. 再遍历外部定义生成 IR。
7. 关闭输出文件。

其中：

```c
out = fopen(output_path, "w");
```

打开的是准备写入 IR 的输出文件，不是打开 AST 文件。`"w"` 表示写入模式：如果文件不存在就创建；如果文件已经存在，就清空旧内容重新写入。后续所有 `emit()` 输出的 IR 指令都会写到这个文件中。

完整的数据流是：

```text
读取 argv[1] 源文件
  -> Flex/Bison 在内存中构造 AST parse_root
  -> semantic_check(parse_root)
  -> ir_generate(parse_root, argv[2])
  -> 打开 argv[2] 指向的 .ir 文件
  -> 遍历 AST，把 IR 写入输出文件
```

这里先调用：

```c
collect_function_sigs(ext_def_list);
```

再调用：

```c
gen_ext_def_list(ext_def_list);
```

原因是函数调用生成 IR 时需要知道被调用函数的参数类型。普通 `int` 参数按值传递，数组和结构体参数按地址传递。如果不先收集函数签名，生成函数调用时就无法判断实参应该传值还是传地址。

### 7. 函数和参数 IR

函数定义由 `gen_ext_def()` 处理：

```c
emit("FUNCTION %s :", name);
push_scope();
gen_params(func ? func->params : NULL);
gen_compst(first_child_name(node, "CompSt"), 0);
pop_scope();
```

例如：

```c
int add(int a, int b) {
    return a + b;
}
```

会生成函数开头：

```text
FUNCTION add :
PARAM a
PARAM b
```

参数生成由 `gen_params()` 完成：

```c
static void gen_params(IrField *params) {
    for (IrField *param = params; param; param = param->next) {
        int is_addr = param->type->kind == IR_TY_ARRAY || param->type->kind == IR_TY_STRUCT;
        add_symbol(param->name, param->type, is_addr);
        emit("PARAM %s", param->name);
    }
}
```

这里把参数加入当前 IR 符号表，并输出 `PARAM` 指令。数组和结构体参数会标记为地址参数，因为它们不能简单按一个整数值传递，需要传递地址。

`params` 是函数参数链表。例如：

```c
int add(int a, int b) {
    return a + b;
}
```

参数链表可以表示为：

```text
a:int -> b:int
```

`gen_params()` 会逐个处理参数：

```text
处理 a：
  is_addr = 0
  add_symbol("a", int, 0)
  emit "PARAM a"

处理 b：
  is_addr = 0
  add_symbol("b", int, 0)
  emit "PARAM b"
```

所以生成的函数开头是：

```text
FUNCTION add :
PARAM a
PARAM b
```

`add_symbol()` 的作用是把参数加入当前函数的 IR 符号表。后面生成函数体时，如果遇到 `a + b`，IR 生成器就能查到 `a` 和 `b` 的类型，并知道它们是不是地址参数。

数组和结构体参数会设置 `is_addr = 1`。例如：

```c
int sum(int a[10]) {
    return a[0];
}
```

这里参数 `a` 本身代表传进来的数组地址。后面访问 `a[0]` 时，`gen_addr()` 会直接把 `a` 当地址使用，而不是再生成 `&a`。这避免把数组参数错误地当成普通局部变量地址处理。

### 8. 语句 IR

语句由 `gen_stmt()` 处理。

IR 生成整体是有顺序的。总入口先收集函数签名，再按 AST 中的外部定义顺序生成函数 IR。每个函数内部先输出 `FUNCTION`，再输出 `PARAM`，再进入函数体。函数体中先处理局部定义，必要时输出 `DEC` 分配数组或结构体空间，然后再按源码顺序生成语句 IR。语句内部如果包含表达式，就递归生成子表达式，再生成当前表达式或当前语句的 IR。

整体顺序可以概括为：

```text
ir_generate()
  -> collect_function_sigs()
  -> gen_ext_def_list()
      -> gen_ext_def()
          -> FUNCTION
          -> gen_params()
              -> PARAM
          -> gen_compst()
              -> collect_def_list()
                  -> DEC
              -> gen_stmt_list()
                  -> gen_stmt()
                      -> gen_exp() / gen_cond()
```

例如：

```c
int add(int a, int b) {
    int c;
    c = a + b;
    return c;
}
```

IR 生成顺序是：

```text
FUNCTION add :
PARAM a
PARAM b
处理局部变量 c
生成 a + b
生成 c := ...
生成 RETURN c
```

表达式内部通常是先生成子表达式，再生成父表达式。例如 `a = b + c * d;` 会先生成 `c * d`，再生成 `b + t1`，最后生成赋值。

#### 8.1 return 语句

```c
emit("RETURN %s", gen_exp(child(node, 1)));
```

例如：

```c
return x;
```

生成：

```text
RETURN x
```

#### 8.2 if 语句

`if` 语句使用标签和条件跳转表示：

```text
条件为真 -> then 标签
条件为假 -> else 标签
then 结束后 -> end 标签
```

生成形态类似：

```text
IF x > #0 GOTO label1
GOTO label2
LABEL label1 :
...
GOTO label3
LABEL label2 :
...
LABEL label3 :
```

#### 8.3 while 语句

`while` 语句使用循环开始标签、循环体标签和循环结束标签：

```text
LABEL label_begin :
条件判断
LABEL label_body :
循环体
GOTO label_begin
LABEL label_end :
```

这样就把高级循环结构转换成了底层的标签和跳转。

### 9. 表达式 IR

表达式由 `gen_exp()` 处理。这个函数返回一个字符串，表示表达式结果所在的位置。

常量：

```c
return xasprintf("#%ld", ast_parse_int(leaf->value));
```

例如整数 `1` 返回：

```text
#1
```

变量：

```c
return xstrdup(leaf->value);
```

例如变量 `x` 返回：

```text
x
```

二元运算：

```c
char *left = gen_exp(child(node, 0));
char *right = gen_exp(child(node, 2));
char *tmp = new_temp();
emit("%s := %s %s %s", tmp, left, symbol, right);
return tmp;
```

例如：

```c
a + b
```

生成：

```text
t1 := a + b
```

并返回 `t1`，表示这个表达式的结果保存在临时变量 `t1` 中。

### 10. 赋值、数组和结构体地址

赋值表达式分两类处理。

普通变量赋值：

```c
x = y + 1;
```

可以生成：

```text
t1 := y + #1
x := t1
```

数组元素或结构体字段赋值需要先计算地址，再间接写入：

```c
a[i] = x;
p.y = x;
```

对应 IR 形式类似：

```text
t1 := i * #4
t2 := &a + t1
*t2 := x
```

`gen_exp()` 负责生成表达式的值，`gen_addr()` 负责生成左值地址。实践 3 中一个重要区分就是：

```text
右值求值：gen_exp()
左值取地址：gen_addr()
```

普通变量作为右值时直接返回变量名；数组元素和结构体字段作为左值时，需要先计算地址。

数组元素地址计算逻辑是：

```text
元素地址 = 数组基地址 + 下标 * 元素大小
```

结构体字段地址计算逻辑是：

```text
字段地址 = 结构体基地址 + 字段偏移
```

### 11. 函数调用和内置函数

函数调用由 `gen_call()` 处理。

内置函数 `read` 特殊翻译为：

```c
READ t1
```

`write` 特殊翻译为：

```c
WRITE x
```

例如：

```c
x = read();
write(x);
```

可以生成：

```text
READ t1
x := t1
WRITE x
```

普通函数调用使用 `ARG` 和 `CALL`：

```text
ARG y
ARG x
t1 := CALL add
```

参数倒序输出是为了符合课程 IR 调用约定。生成实参时，如果形参是数组或结构体，会传递地址；如果是普通整型参数，会传递值。

`ARG` 表示准备传入一个实参，`CALL` 表示真正调用函数并取得返回值。例如：

```c
z = add(x, y);
```

可以生成：

```text
ARG y
ARG x
t1 := CALL add
z := t1
```

源代码中的参数顺序是 `x, y`，IR 中倒序输出为 `ARG y`、`ARG x`。这是为了符合课程 IR 的调用约定，也便于后续栈式传参：先压入后一个参数，再压入前一个参数，被调函数读取时仍能按 `PARAM a`、`PARAM b` 的顺序得到正确实参。

实参生成时会根据形参类型决定传值还是传地址。代码中先找到当前实参对应的形参：

```c
IrField *param = nth_param(func, idx);
```

如果形参是数组或结构体：

```c
place = gen_addr(child(args, 0), &dummy);
```

传递的是地址；否则：

```c
place = gen_exp(child(args, 0));
```

传递的是表达式的值。普通 `int` 参数直接传值，数组和结构体参数传地址，是因为数组和结构体可能占用多字节空间，函数内部通过地址访问它们。

### 12. 条件表达式

条件表达式由 `gen_cond()` 处理。关系表达式会生成：

```text
IF left relop right GOTO true_label
GOTO false_label
```

例如：

```c
if (x > 0)
```

生成：

```text
IF x > #0 GOTO label1
GOTO label2
```

逻辑与 `&&` 和逻辑或 `||` 按短路求值翻译。`a && b` 中，如果 `a` 为假，就直接跳到假标签，不再判断 `b`；`a || b` 中，如果 `a` 为真，就直接跳到真标签，不再判断 `b`。

### 13. IR 生成样例

实践 3 的手写样例是 `test/proj3-test1.cmm`：

```c
int square(int x)
{
    return x * x;
}

int main()
{
    int i;
    int total;

    i = 1;
    total = 0;
    while (i < 5)
    {
        total = total + square(i);
        i = i + 1;
    }
    write(total);
    return 0;
}
```

这个程序定义了一个 `square` 函数，然后在 `main` 中循环计算：

```text
1 * 1 + 2 * 2 + 3 * 3 + 4 * 4
```

生成 IR 时执行：

```sh
cd /home/leyi/Desktop/submission/proj3
make ir
CMM_IR=1 build/cmmc test/proj3-test1.cmm /tmp/proj3-test1.ir
sed -n '1,120p' /tmp/proj3-test1.ir
```

生成结果是：

```text
FUNCTION square :
PARAM x
t1 := x * x
RETURN t1
FUNCTION main :
i := #1
total := #0
LABEL label1 :
IF i < #5 GOTO label2
GOTO label3
LABEL label2 :
ARG i
t2 := CALL square
t3 := total + t2
total := t3
t4 := i + #1
i := t4
GOTO label1
LABEL label3 :
WRITE total
RETURN #0
```

这段 IR 可以按源程序结构理解。`FUNCTION square` 和 `FUNCTION main` 对应两个函数；`PARAM x` 对应 `square` 的形参；`t1 := x * x` 对应乘法表达式；`LABEL label1`、`IF i < #5 GOTO label2`、`GOTO label3` 对应 `while` 的循环判断；`ARG i` 和 `t2 := CALL square` 对应函数调用；`WRITE total` 对应 `write(total)`。

批量展示 IR 生成时可以运行：

```sh
cd /home/leyi/Desktop/submission/proj3
make ir-test
```

这个命令会调用 `tools/run_ir_tests.sh`，对课程样例和手写样例都生成 IR，并把每个 IR 文件的前 120 行打印出来。生成的 IR 文件保存在：

```text
build/project3-ir/
```

其中包括：

```text
proj3-4-3-6-exp1-1.ir
proj3-4-3-6-exp2-1.ir
proj3-4-3-7-exp1-0.ir
proj3-4-3-7-exp2-0.ir
proj3-test1.ir
```

### 14. IRSim 验证样例

IR 生成以后，还需要确认 IR 本身能被解释执行，结果和源程序预期一致。实践 3 使用课程提供的 IRSim 做验证。

单独验证手写样例可以运行：

```sh
cd /home/leyi/Desktop/submission/proj3
./tools/run_one_irsim_case.sh proj3-test1
```

这个脚本内部先生成：

```text
build/project3-ir/proj3-test1.ir
```

然后用 Docker 中的 IRSim 加载这个 IR 文件并运行。`proj3-test1` 的输出是：

```text
generated: build/project3-ir/proj3-test1.ir
30
```

这里的 `30` 正好对应源程序的计算结果：

```text
1 * 1 + 2 * 2 + 3 * 3 + 4 * 4 = 30
```

批量验证可以运行：

```sh
cd /home/leyi/Desktop/submission/proj3
make irsim-test
```

实际验证输出为：

```text
ok: proj3-4-3-6-exp1-1 -> [1]
ok: proj3-4-3-6-exp2-1 -> [5040]
ok: proj3-4-3-7-exp1-0 -> [3]
ok: proj3-4-3-7-exp2-0 -> [1, 3]
ok: proj3-test1 -> [30]
irsim tests: 5 passed, 0 failed
```

这说明生成的 IR 不只是形式上有 `FUNCTION`、`LABEL`、`CALL` 等指令，而且能够被 IRSim 正确执行。实践 3 的验证链路就是：源程序生成 IR，再用 IRSim 跑 IR，最后检查输出结果。

### 15. 本阶段实现小结

实践 3 的核心是 `irgen.c`。它接收已经通过语义检查的 AST，先收集函数签名和类型信息，再遍历函数体，把高级语句和表达式翻译成线性的三地址 IR。普通表达式用临时变量保存结果，控制流用 `LABEL`、`GOTO`、`IF` 表示，函数调用用 `ARG`、`CALL` 表示，数组和结构体访问统一转成地址计算和间接读写，`read` 和 `write` 则特殊转成 `READ`、`WRITE` 指令。

## 实践 4 MIPS 目标代码生成

### 1. 本阶段目标

实践 4 在实践 3 的基础上继续向后端推进，把已经生成的中间代码 IR 翻译成可以在 SPIM 中运行的 MIPS 汇编。它不是直接从 AST 翻译到 MIPS，而是复用前一阶段的 IR：

```text
C-- 源程序
  -> 词法分析
  -> 语法分析，生成 AST
  -> 语义分析
  -> IR 生成
  -> MIPS 生成
```

这样做的好处是后端只需要面对较简单的三地址指令。语法结构、类型检查、表达式递归翻译这些复杂工作已经在前面阶段处理过了，实践 4 主要关注 IR 指令如何落到寄存器、栈帧、跳转和系统调用上。

### 2. 构建过程

在实践 4 所在目录执行：

```sh
cd /home/leyi/Desktop/submission/proj4
make mips
```

`Makefile` 中 `mips` 目标仍然构建同一个主程序 `build/cmmc`：

```make
mips: build/cmmc
```

完整构建命令会把实践 1 到实践 4 的源文件都编译进去：

```make
build/cmmc: src/parser.y src/lexer.l src/parser_main.c src/ast.c src/ast.h src/semantic.c src/semantic.h src/irgen.c src/irgen.h src/mipsgen.c src/mipsgen.h
	bison -d -o build/parser.tab.c src/parser.y
	flex -DPARSER_BUILD -o build/parser_lexer.c src/lexer.l
	gcc ... src/parser_main.c src/ast.c src/semantic.c src/irgen.c src/mipsgen.c ...
```

和实践 3 相比，实践 4 主要新增：

```text
src/mipsgen.h
src/mipsgen.c
```

`mipsgen.h` 对外只暴露一个入口：

```c
int mips_generate_from_ir(const char *ir_path, const char *output_path);
```

也就是说，MIPS 生成器的输入是一个 IR 文件路径，输出是一个 `.s` 汇编文件路径。

### 3. 主程序如何进入 MIPS 模式

实践 4 的入口仍然在 `parser_main.c`。程序通过环境变量或参数个数判断是否进入 MIPS 模式：

```c
int ir_mode = getenv("CMM_IR") != NULL;
int mips_mode = getenv("CMM_MIPS") != NULL || (!ir_mode && argc == 3);
```

如果不设置 `CMM_IR`，并且命令行是：

```sh
build/cmmc input.cmm output.s
```

那么程序会把它当成 MIPS 生成任务。进入 MIPS 模式后，主程序先做语义检查，再生成一个临时 IR 文件：

```c
snprintf(tmp_ir, sizeof(tmp_ir), "/tmp/cmm_ir_%ld.ir", (long)getpid());
ir_errors = ir_generate(parse_root, tmp_ir);
```

如果 IR 生成没有错误，再调用：

```c
mips_errors = mips_generate_from_ir(tmp_ir, argv[2]);
```

最后删除临时 IR：

```c
remove(tmp_ir);
```

所以实践 4 的真实流程是：用户给的是 `.cmm` 输入和 `.s` 输出，但程序内部会先生成一个临时 `.ir`，再把临时 IR 翻译成 MIPS。这样后端可以直接复用实践 3 的成果，不需要重新分析 AST。

### 4. mipsgen.c 的整体结构

`mipsgen.c` 可以分成两个阶段：

```text
第一阶段：读取 IR，收集函数、变量、参数和原始指令
第二阶段：为每个函数分配栈帧，并把每条 IR 指令输出成 MIPS
```

代码中定义了几个核心结构：

```c
typedef struct Var {
    char *name;
    int size;
    int offset;
    struct Var *next;
} Var;
```

`Var` 表示一个 IR 变量或临时变量。`offset` 是它在当前函数栈帧中的位置。

```c
typedef struct Instr {
    char *text;
    struct Instr *next;
} Instr;
```

`Instr` 保存一条原始 IR 指令文本。实践 4 没有把 IR 建成复杂语法树，而是按行读取，再根据指令格式解析。

```c
typedef struct Func {
    char *name;
    Instr *head;
    Instr *tail;
    Var *vars;
    char **params;
    int param_count;
    int frame_size;
    struct Func *next;
} Func;
```

`Func` 表示一个函数。它保存函数名、函数内所有 IR 指令、局部变量和临时变量、参数列表，以及最终计算出的栈帧大小。

### 5. 第一阶段：读取 IR

MIPS 生成入口是：

```c
int mips_generate_from_ir(const char *ir_path, const char *output_path)
```

它先打开 IR 文件：

```c
FILE *input = fopen(ir_path, "r");
```

然后调用：

```c
Func *funcs = parse_ir(input);
```

`parse_ir()` 按行读取 IR。遇到：

```text
FUNCTION main :
```

就新建一个函数；遇到其他 IR 指令，就把指令保存到当前函数中，并调用 `collect_vars()` 收集这一行中出现的变量。

例如 IR：

```text
t3 := total + t2
```

这一行会把 `t3`、`total`、`t2` 都记录为当前函数可能需要分配栈空间的名字。这样第二阶段生成 MIPS 时，每个变量都能找到自己的栈偏移。

`PARAM x` 会被记录为函数参数：

```c
add_param(func, name);
```

`DEC arr size` 会按实际大小记录变量：

```c
add_var(func, name, size);
```

普通整型变量和临时变量默认按 4 字节处理，数组或结构体这种由 `DEC` 声明的对象按指定大小处理。

### 6. 栈帧布局

读取完一个函数后，生成 MIPS 前会调用：

```c
layout_frame(func);
```

这个函数给当前函数里的所有变量分配栈空间。实践 4 使用 `$fp` 作为稳定的栈帧基准，变量放在 `$fp` 的负偏移位置。

代码中预留了两个固定位置：

```text
-4($fp)  保存返回地址 $ra
-8($fp)  保存旧的 $fp
```

局部变量和临时变量继续向更小的地址分配，例如：

```text
-12($fp)  t1
-16($fp)  x
-20($fp)  total
```

函数入口处会输出函数序言：

```asm
addi $sp, $sp, -frame_size
sw $ra, frame_size-4($sp)
sw $fp, frame_size-8($sp)
addi $fp, $sp, frame_size
```

它的作用是开辟当前函数的栈帧，保存返回地址和旧帧指针，然后让 `$fp` 指向当前函数栈帧的顶部。这样函数内部访问变量时，就可以统一用 `offset($fp)`。

函数返回时输出函数尾声：

```asm
move $sp, $fp
lw $ra, -4($fp)
lw $fp, -8($fp)
jr $ra
```

`main` 函数比较特殊，返回时直接使用 SPIM 的退出系统调用：

```asm
li $v0, 10
syscall
```

### 7. 操作数翻译

IR 中的操作数主要有几类：

```text
#5      立即数
x       普通变量
&x      变量地址
*p      地址 p 指向的值
```

这些由 `load_operand()` 统一处理。

立即数：

```text
#5
```

翻译成：

```asm
li $t0, 5
```

普通变量：

```text
x
```

翻译成从栈帧加载：

```asm
lw $t0, offset($fp)
```

取地址：

```text
&x
```

翻译成计算变量地址：

```asm
addi $t0, $fp, offset
```

间接访问：

```text
*p
```

先从栈里取出 `p` 保存的地址，再从这个地址读值：

```asm
lw $t0, offset_of_p($fp)
lw $t0, 0($t0)
```

这个区分对应实践 3 中的地址和值。实践 3 把数组和结构体访问翻译成地址相关 IR，实践 4 再把 `&` 和 `*` 落成真实的 MIPS 地址计算和内存访问。

### 8. 赋值和算术指令

赋值、加减乘除主要由 `emit_assignment()` 处理。

IR：

```text
t1 := x * x
```

会翻译成：

```asm
lw $t0, offset_x($fp)
lw $t1, offset_x($fp)
mul $t2, $t0, $t1
sw $t2, offset_t1($fp)
```

如果是加法：

```text
t3 := total + t2
```

会翻译成加载两个操作数、执行 `add`、再把结果写回栈帧。

除法稍微特殊，MIPS 的 `div` 结果不直接写入普通寄存器，而是写入特殊寄存器，所以代码会使用：

```asm
div $t0, $t1
mflo $t2
```

然后再把 `$t2` 存回目标变量。

### 9. 标签、跳转和条件分支

IR 中的标签：

```text
LABEL label1 :
```

直接变成 MIPS 标签：

```asm
label1:
```

无条件跳转：

```text
GOTO label1
```

翻译成：

```asm
j label1
```

条件跳转：

```text
IF i < #5 GOTO label2
```

会先加载左右操作数：

```asm
lw $t0, offset_i($fp)
li $t1, 5
```

再根据关系运算符选择分支指令：

```asm
blt $t0, $t1, label2
```

`branch_op()` 负责把 IR 里的 `==`、`!=`、`>`、`<`、`>=`、`<=` 映射到 MIPS 的 `beq`、`bne`、`bgt`、`blt`、`bge`、`ble`。

### 10. 函数调用

函数调用对应 IR 中的 `ARG` 和 `CALL`。

例如：

```text
ARG i
t2 := CALL square
```

实践 4 遇到 `ARG i` 时不会立刻调用函数，而是先把参数暂存在 `pending_args` 中。等遇到：

```text
t2 := CALL square
```

才调用 `emit_call_args()`，把参数依次压入栈：

```asm
lw $t0, offset_i($fp)
addi $sp, $sp, -4
sw $t0, 0($sp)
jal square
sw $v0, offset_t2($fp)
addi $sp, $sp, 4
```

`jal square` 会跳转到 `square` 函数，并把返回地址保存到 `$ra`。被调函数把返回值放到 `$v0`，调用者再把 `$v0` 存入 IR 左侧变量 `t2`。

函数入口处还会把参数从调用者栈上传入的位置取出来，保存到本函数自己的栈帧变量中：

```asm
lw $t0, 0($fp)
sw $t0, offset_x($fp)
```

因此 `PARAM x` 在 MIPS 阶段不直接输出一条指令，而是影响函数入口处的参数装载。

编译时的生成顺序是先看见 `ARG`，把实参暂存在 `pending_args` 中；再看见 `t2 := CALL square`，把暂存实参压入运行时栈，输出 `jal square`，再把 `$v0` 中的返回值存到 `t2`。调用结束后，调用者还要把刚才压入的实参空间回收。

运行时的执行过程可以按这个顺序理解：

```text
main 加载 i
-> main 把 i 压栈
-> jal square，跳转到 square，并把返回地址保存到 $ra
-> square 建立自己的栈帧
-> square 从 0($fp) 取到参数 x
-> square 计算 x * x
-> square 把结果放入 $v0
-> square 恢复 $sp、$fp、$ra
-> jr $ra 返回 main
-> main 把 $v0 保存到 t2
-> main 回收参数栈空间
-> main 继续执行 total = total + t2
```

这一套调用约定中，几个寄存器的作用比较固定：

```text
$sp  栈顶指针，用来压参数和开辟栈帧
$fp  帧指针，用稳定偏移访问当前函数的变量
$ra  返回地址，jal 写入，jr $ra 用来返回
$v0  函数返回值，也用于 syscall 编号
$t0/$t1/$t2  临时计算寄存器
```

一句话概括，实践 4 的函数调用是：调用者压参数，`jal` 跳转，被调函数建栈帧并取参数，返回值放 `$v0`，`jr $ra` 回到调用者，调用者保存返回值并回收参数空间。

### 11. IR 和 MIPS 的区别

IR 是编译器内部使用的中间表示，MIPS 是可以被 SPIM 执行的目标汇编。IR 更接近程序逻辑，MIPS 更接近机器执行步骤。

例如 IR 中一条加法：

```text
t3 := total + t2
```

它只表达 `total + t2` 的结果放进 `t3`。对应到 MIPS 时，必须说明从哪里取值、放到哪个寄存器、执行哪条指令、结果存回哪里：

```asm
lw $t0, -24($fp)
lw $t1, -20($fp)
add $t2, $t0, $t1
sw $t2, -16($fp)
```

函数调用也一样。IR 是：

```text
ARG i
t2 := CALL square
```

MIPS 必须展开成：

```asm
lw $t0, -28($fp)
addi $sp, $sp, -4
sw $t0, 0($sp)
jal square
sw $v0, -20($fp)
addi $sp, $sp, 4
```

所以二者的区别是：

```text
IR：
中间代码
更抽象
方便分析和优化
用变量名、临时变量、标签表达程序逻辑
不直接处理寄存器、栈帧和系统调用细节

MIPS：
目标汇编代码
面向具体指令集
可以被 SPIM 执行
必须处理寄存器、栈、调用约定和 syscall
每一步都更接近真实机器执行
```

在这个项目中，实践 3 负责 `AST -> IR`，实践 4 负责 `IR -> MIPS`。IR 起到桥梁作用，让前端不用直接面对汇编细节，也让后端不用重新理解 AST 中的复杂语法结构。

### 12. read 和 write

实践 3 中的：

```text
READ x
```

在实践 4 中翻译成 SPIM 的整数输入系统调用：

```asm
li $v0, 5
syscall
sw $v0, offset_x($fp)
```

实践 3 中的：

```text
WRITE x
```

翻译成整数输出系统调用：

```asm
lw $a0, offset_x($fp)
li $v0, 1
syscall
```

为了每次输出后换行，文件开头还会在数据段定义：

```asm
.data
_ret: .asciiz "\n"
```

`WRITE` 输出整数后，再用系统调用 4 输出 `_ret` 字符串。

所以 `read` 和 `write` 从实践 2 的内置函数，到实践 3 的 `READ`、`WRITE` IR，再到实践 4 的 SPIM `syscall`，形成了一条完整的实现链路。

### 13. 输出文件结构

`mips_generate_from_ir()` 打开输出 `.s` 文件后，先写入汇编文件头：

```asm
.data
_ret: .asciiz "\n"
.globl main
.text
```

然后依次输出每个函数：

```c
for (Func *func = funcs; func; func = func->next) {
    emit_func(func);
}
```

每个函数由 `emit_func()` 生成，顺序是：

```text
计算栈帧
输出函数标签
输出函数序言
装载参数
逐条翻译 IR 指令
```

### 14. 样例输入和输出

样例程序：

```c
int square(int x)
{
    return x * x;
}

int main()
{
    int i;
    int total;

    i = 1;
    total = 0;
    while (i < 5)
    {
        total = total + square(i);
        i = i + 1;
    }
    write(total);
    return 0;
}
```

先生成 MIPS：

```sh
cd /home/leyi/Desktop/submission/proj4
build/cmmc test/proj4-test1.cmm /tmp/proj4-test1.s
sed -n '1,180p' /tmp/proj4-test1.s
```

对应 IR 片段是：

```text
FUNCTION square :
PARAM x
t1 := x * x
RETURN t1
FUNCTION main :
i := #1
total := #0
LABEL label1 :
IF i < #5 GOTO label2
GOTO label3
LABEL label2 :
ARG i
t2 := CALL square
t3 := total + t2
total := t3
t4 := i + #1
i := t4
GOTO label1
LABEL label3 :
WRITE total
RETURN #0
```

生成的 MIPS 中可以看到函数序言、乘法、循环分支、函数调用和输出：

```asm
square:
  addi $sp, $sp, -16
  sw $ra, 12($sp)
  sw $fp, 8($sp)
  addi $fp, $sp, 16
  lw $t0, 0($fp)
  sw $t0, -16($fp)
  lw $t0, -16($fp)
  lw $t1, -16($fp)
  mul $t2, $t0, $t1
  sw $t2, -12($fp)
  lw $v0, -12($fp)
  move $sp, $fp
  lw $ra, -4($fp)
  lw $fp, -8($fp)
  jr $ra
main:
  addi $sp, $sp, -32
  sw $ra, 28($sp)
  sw $fp, 24($sp)
  addi $fp, $sp, 32
  li $t0, 1
  sw $t0, -28($fp)
  li $t0, 0
  sw $t0, -24($fp)
label1:
  lw $t0, -28($fp)
  li $t1, 5
  blt $t0, $t1, label2
  j label3
label2:
  lw $t0, -28($fp)
  addi $sp, $sp, -4
  sw $t0, 0($sp)
  jal square
  sw $v0, -20($fp)
```

运行：

```sh
spim -quiet -file /tmp/proj4-test1.s
```

程序会输出：

```text
30
```

因为循环计算的是：

```text
1 * 1 + 2 * 2 + 3 * 3 + 4 * 4 = 30
```

### 15. 本阶段实现小结

实践 4 的核心是 `mipsgen.c`。它先读取实践 3 生成的 IR，按函数收集指令、变量和参数；然后为每个函数计算栈帧，把变量映射到 `$fp` 的偏移；最后逐条把 IR 翻译成 MIPS。赋值和算术操作使用 `$t0`、`$t1`、`$t2` 临时寄存器，控制流用标签和分支指令，函数调用通过栈传参、`jal` 调用和 `$v0` 返回值实现，`READ` 和 `WRITE` 则翻译成 SPIM 的输入输出系统调用。

## 实践 5 IR 优化

### 1. 本阶段目标

实践 5 的目标不是继续生成更底层的代码，而是在 IR 层做优化。输入是一个已有的 `.ir` 文件，输出是优化后的 `.opt.ir` 文件：

```text
原始 IR
  -> IR 优化器
  -> 优化后 IR
```

它和实践 3、实践 4 的关系是：

```text
实践 3：AST -> IR
实践 4：IR -> MIPS
实践 5：IR -> 优化后的 IR
```

IR 比 MIPS 更适合做优化，因为 IR 保留了变量名、临时变量、标签和三地址表达式，结构比汇编清楚；同时它又比 AST 更接近执行过程，方便判断哪些赋值没有用、哪些表达式可以替换。

实践 5 优化的对象不是 `.cmm` 源程序，也不是实践 4 生成的 MIPS，而是实践 3 生成的原始 IR 文本。优化器直接读取 `.ir` 文件，通过识别课程 IR 的固定指令格式，完成常量传播、常量折叠、复制传播、公共表达式消除和死代码删除。

### 2. 构建过程

在实践 5 所在目录执行：

```sh
cd /home/leyi/Desktop/submission/proj5
make opt
```

`Makefile` 中优化器是一个独立程序：

```make
opt: build/cmm-opt

build/cmm-opt: src/iropt_main.c src/iropt.c src/iropt.h
	gcc ... src/iropt_main.c src/iropt.c -o build/cmm-opt
```

和前面几个实践不同，实践 5 不需要重新编译词法、语法、语义、IR 生成和 MIPS 生成模块。它只编译：

```text
src/iropt_main.c
src/iropt.c
src/iropt.h
```

因为它处理的是已经存在的 IR 文件。

### 3. 命令入口

实践 5 的主函数在 `iropt_main.c`：

```c
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.ir> <output.ir>\n", argv[0]);
        return 1;
    }
    return ir_optimize_file(argv[1], argv[2]);
}
```

手动优化一个 IR 文件时执行：

```sh
cd /home/leyi/Desktop/submission/proj5
make opt
build/cmm-opt test/proj5-test1.ir /tmp/proj5-test1.opt.ir
```

这里 `test/proj5-test1.ir` 是输入 IR，`/tmp/proj5-test1.opt.ir` 是优化后的输出 IR。

### 4. 优化器总入口

优化器对外暴露的入口在 `iropt.h`：

```c
int ir_optimize_file(const char *input_path, const char *output_path);
```

`iropt.c` 中的实现是：

```c
int ir_optimize_file(const char *input_path, const char *output_path) {
    LineVec lines = {0};
    if (read_lines(input_path, &lines) != 0) {
        return 1;
    }
    for (int iter = 0; iter < 8; ++iter) {
        int changed = rewrite_pass(&lines);
        changed |= dce_pass(&lines);
        changed |= global_dce_pass(&lines);
        if (!changed) {
            break;
        }
    }
    int result = write_lines(output_path, &lines);
    free_lines(&lines);
    return result;
}
```

整体流程是：

```text
读取输入 IR
-> 多轮执行优化 pass
-> 如果一轮没有变化就提前停止
-> 写出优化后的 IR
```

最多循环 8 轮，是因为一次优化可能暴露新的优化机会。例如常量传播以后，某些表达式变成常量表达式；常量折叠以后，又可能让原来的临时变量变成无用赋值。

### 5. IR 行结构

实践 5 没有重新从 AST 分析程序，也没有把 IR 进一步解析成复杂的结构化 IR 节点，而是把原始 IR 文件按行保存：

```c
typedef struct {
    char *text;
    int deleted;
} IrLine;
```

`text` 保存原始 IR 文本，`deleted` 表示这一行是否被优化删除。

多行 IR 放在：

```c
typedef struct {
    IrLine *items;
    int len;
    int cap;
} LineVec;
```

这样做比较适合课程 IR。因为课程 IR 是线性三地址代码，一行基本对应一条指令，而且格式比较固定。优化器可以通过匹配每一行的指令格式，识别赋值、二元运算、条件跳转、返回、输出和参数传递，例如：

```text
x := y
x := a + b
IF a < b GOTO label1
RETURN x
WRITE x
ARG x
```

例如读到：

```text
t1 := v2 + #20
```

优化器可以解析出：

```text
左侧定义变量：t1
左操作数：v2
运算符：+
右操作数：#20
```

如果前面已经知道 `v2` 等于 `#10`，这一行就可以先改写成：

```text
t1 := #10 + #20
```

再通过常量折叠变成：

```text
t1 := #30
```

因此本阶段的优化方式可以概括为：继续处理原始 IR 文件，按行识别固定格式的 IR 指令，再在这些指令上做替换、折叠、复用和删除。

### 6. rewrite_pass：表达式改写

第一类优化集中在 `rewrite_pass()`，主要做：

```text
常量传播
常量折叠
复制传播
公共表达式消除
```

它维护两个表。

一个是变量绑定表：

```c
typedef struct {
    char *name;
    char *value;
} Binding;
```

例如：

```text
v1 := #10
v2 := v1
```

优化器可以记录：

```text
v1 -> #10
v2 -> #10
```

后面遇到使用 `v2` 的地方，就可以替换成 `#10`。

另一个是表达式表：

```c
typedef struct {
    char *op;
    char *left;
    char *right;
    char *result;
} ExprEntry;
```

例如已经出现过：

```text
t1 := v1 + #2
```

表达式表可以记录：

```text
v1 + #2 -> t1
```

后面如果又出现同样表达式，就可以直接复用已有结果，而不重新计算。

### 7. 常量传播和常量折叠

常量传播指的是把变量替换成已经知道的常量。

例如：

```text
v1 := #10
v2 := v1
t1 := v2 + #20
WRITE t1
```

传播以后可以变成：

```text
v1 := #10
v2 := #10
t1 := #10 + #20
WRITE t1
```

常量折叠指的是在编译阶段直接计算常量表达式：

```text
t1 := #10 + #20
```

可以直接改写成：

```text
t1 := #30
```

相关计算在 `eval_binary()` 中完成，支持：

```text
+
-
*
/
```

除法会避免除以 0，如果右操作数是 0，就不做折叠。

### 8. 复制传播

复制传播处理的是变量之间的简单赋值：

```text
v2 := v1
t1 := v2 + #20
```

如果中间没有重新定义 `v1` 或 `v2`，就可以把 `v2` 的使用替换成 `v1`：

```text
t1 := v1 + #20
```

如果 `v1` 已经绑定到常量，还可以继续传播成：

```text
t1 := #10 + #20
```

这也是为什么实践 5 要多轮迭代。一次传播、折叠、删除之后，后面可能还能继续简化。

### 9. 公共表达式消除

公共表达式消除处理重复计算。

例如：

```text
t3 := v1 + #2
v3 := t3
t7 := v1 + #2
v7 := t7
```

如果 `v1` 中间没有改变，第二次 `v1 + #2` 不需要重新计算，可以直接复用前面的结果：

```text
t7 := t3
```

代码中 `normalize_expr()` 还会处理加法和乘法的交换性。比如：

```text
a + b
b + a
```

对于 `+` 和 `*` 来说是同一个表达式，规范化后更容易识别重复表达式。

### 10. dce_pass：基本块内死代码删除

`dce_pass()` 做的是基本块内死代码删除。它从后往前扫描，如果一个赋值的结果后面没有被使用，就可以删除。

例如：

```text
v5 := #999
WRITE v3
RETURN #0
```

`v5` 后面没有被读，也没有被输出或返回，所以这一行可以删除。

但有副作用的语句不能随便删。例如：

```text
x := CALL f
*p := x
```

函数调用可能有副作用，间接写内存也可能影响别处，所以代码中 `assignment_has_side_effect()` 会保护这类语句。

### 11. global_dce_pass：全局死代码删除

基本块内向后看还不够，因为变量是否有用可能跨越标签和跳转。`global_dce_pass()` 做的是基于控制流的活跃变量分析。

它先为每行 IR 计算：

```text
use：这一行使用了哪些变量
def：这一行定义了哪些变量
```

然后根据控制流后继计算：

```text
live_out
live_in
```

规则可以理解成：

```text
live_out = 所有后继 live_in 的并集
live_in = use 并上 live_out 中没有被当前行重新定义的变量
```

如果一条赋值定义的变量在该行之后不再活跃，并且这条赋值没有副作用，就可以删除。

全局死代码删除会特别保护取地址变量：

```c
address_taken
```

如果某个变量被 `&x` 取过地址，就不能简单认为直接使用列表里没有 `x` 就可以删，因为它可能通过指针间接访问。

### 12. 优化样例

手写样例 `test/proj5-test1.ir`：

```text
FUNCTION main :
v1 := #10
v2 := v1
t1 := v2 + #20
v3 := t1
t2 := #10 + #20
v4 := t2
v5 := #999
WRITE v3
RETURN #0
```

运行：

```sh
cd /home/leyi/Desktop/submission/proj5
make opt
build/cmm-opt test/proj5-test1.ir /tmp/proj5-test1.opt.ir
diff -u test/proj5-test1.ir /tmp/proj5-test1.opt.ir
```

优化结果是：

```text
FUNCTION main :
WRITE #30
RETURN #0
```

这里发生了几件事：

```text
v1 := #10              记录 v1 是常量 10
v2 := v1               传播为 v2 是常量 10
t1 := v2 + #20         变成 #10 + #20，再折叠成 #30
v3 := t1               传播为 v3 是 #30
v5 := #999             后面没有使用，被删除
WRITE v3               改写成 WRITE #30
```

最终只保留对程序结果有影响的输出和返回。

### 13. 批量优化和验证

只展示优化效果时运行：

```sh
cd /home/leyi/Desktop/submission/proj5
make opt-demo
```

实际输出为：

```text
ok: proj5-6-3-6-exp1-1 (19 -> 4) -> build/project5-opt/proj5-6-3-6-exp1-1.opt.ir
ok: proj5-6-3-6-exp2-1 (31 -> 13) -> build/project5-opt/proj5-6-3-6-exp2-1.opt.ir
ok: proj5-6-3-6-exp3-1 (17 -> 3) -> build/project5-opt/proj5-6-3-6-exp3-1.opt.ir
ok: proj5-test1 (10 -> 3) -> build/project5-opt/proj5-test1.opt.ir
opt demo: 4 optimized, 0 failed
```

要验证优化前后语义一致，运行：

```sh
cd /home/leyi/Desktop/submission/proj5
make opt-test
```

这个命令会先生成优化后的 IR，再用 IRSim 分别运行原始 IR 和优化后 IR，对比输出是否一致。实际验证结果为：

```text
ok: proj5-6-3-6-exp1-1 -> [] (19 -> 4, saved 15)
ok: proj5-6-3-6-exp2-1 -> [12, 21] (31 -> 13, saved 18)
ok: proj5-6-3-6-exp3-1 -> [] (17 -> 3, saved 14)
ok: proj5-test1 -> [30] (10 -> 3, saved 7)
irsim opt tests: 4 passed, 0 failed, saved 54 IR lines
```

这说明优化后的 IR 行数明显减少，但执行结果和原始 IR 保持一致。

### 14. 本阶段实现小结

实践 5 的核心是 `iropt.c`。它把 IR 文件按行读入，反复执行表达式改写、基本块死代码删除和全局死代码删除。表达式改写负责常量传播、常量折叠、复制传播和公共表达式消除；死代码删除负责去掉不会影响输出和返回结果的赋值。最后通过 IRSim 同时运行优化前后的 IR，确认优化没有改变程序语义。
