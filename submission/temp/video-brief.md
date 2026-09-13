# 编译原理课程设计视频汇报精简稿

## 汇报主线

本次课程设计完成的是一个 C-- 小型编译器的主要流程。整体构建顺序是：

```text
C-- 源程序
-> Flex 词法分析
-> Bison 语法分析并构造 AST
-> 语义分析
-> 中间代码 IR 生成
-> MIPS 目标代码生成
-> IR 优化
```

对应的数据流是：

```text
源码字符流 -> token -> AST -> 类型和符号检查 -> IR -> MIPS / 优化后 IR
```

五个实践对应的核心文件如下：

```text
实践 1.1：proj1/src/lexer.l、token.h、lexer_main.c
实践 1.2：proj1/src/parser.y、ast.c、ast.h、parser_main.c
实践 2：proj2/src/semantic.c、semantic.h
实践 3：proj3/src/irgen.c、irgen.h
实践 4：proj4/src/mipsgen.c、mipsgen.h
实践 5：proj5/src/iropt.c、iropt.h、iropt_main.c
```

## 实践 1.1 Flex 词法分析

实践 1.1 实现的是词法分析器。源程序本来是一串字符，词法分析阶段把字符流切分成 token，例如关键字、标识符、整数、浮点数、运算符和分隔符。

主要设计在 `proj1/src/lexer.l`。这个文件中用 Flex 正则描述 C-- 的词法规则，比如识别 `int`、`float`、`return`、标识符、整数、浮点数、关系运算符和括号分号等符号。空白字符和注释会被过滤，不作为 token 输出。非法字符、非法数字、非法标识符和未闭合注释会输出 A 类词法错误。

`proj1/src/token.h` 用来定义独立词法分析阶段的 `TOK_*` token 类型。实践 1.1 还没有接入 Bison，所以 lexer 需要自己的一套 token 编号。后面实践 1.2 接入 Bison 后，parser 模式会使用 Bison 生成的 token。

`proj1/src/lexer_main.c` 是独立词法分析入口。它打开输入文件，把文件赋给 `yyin`，然后循环调用 Flex 生成的 `yylex()`。每次 `yylex()` 扫描并返回一个 token，直到文件结束返回 0。

运行演示：

```sh
cd /home/leyi/Desktop/submission/proj1
make lexer
nl -ba test/proj1-test1.cmm
CMM_LEX_DUMP=1 build/cmm-lexer test/proj1-test1.cmm
```

这组命令先构建词法分析器，再显示样例源程序，最后输出 token 序列。`CMM_LEX_DUMP=1` 表示开启 token 打印。

## 实践 1.2 Bison 语法分析

实践 1.2 在词法分析基础上实现语法分析。lexer 只能返回一个个 token，parser 要判断这些 token 能不能组成合法的表达式、语句、函数定义和复合语句。

核心文件是 `proj1/src/parser.y`。这个文件中定义 C-- 的文法产生式，例如 `Program`、`ExtDef`、`Stmt`、`Exp` 等。Bison 根据 `parser.y` 生成 `yyparse()`，`yyparse()` 内部维护分析栈，根据文法进行移进和规约。需要新 token 时，`yyparse()` 会调用 `yylex()`。

AST 的设计在 `proj1/src/ast.c` 和 `proj1/src/ast.h` 中。规约成功时，产生式动作会创建 AST 节点，并把子节点连接起来。语法树打印采用先序深度优先遍历，所以输出中会先看到父节点，再看到它下面的子节点。

语法分析本身由 Bison 的 `yyparse()` 完成，AST 是规约成功后构造出来的结果。后续实践 2 的语义分析和实践 3 的 IR 生成都基于这棵 AST。

运行演示：

```sh
cd /home/leyi/Desktop/submission/proj1
make parser
build/cmmc test/proj1-test1.cmm /tmp/proj1-test1.ast
sed -n '1,80p' /tmp/proj1-test1.ast
```

这组命令构建语法分析器，解析手写样例，并显示生成的语法树。

## 实践 2 语义分析

实践 2 在 AST 基础上做语义检查。语法正确不代表语义正确，例如变量可能没有定义，赋值两边类型可能不一致，函数调用参数可能不匹配，数组下标可能不是整数，结构体字段可能不存在。

核心文件是 `proj2/src/semantic.c` 和 `proj2/src/semantic.h`。语义分析入口是：

```c
semantic_check(parse_root)
```

其中 `parse_root` 是实践 1.2 得到的 AST 根节点。

语义分析中设计了类型系统和符号表。`Type` 表示 `int`、`float`、数组、结构体和函数类型；`Field` 表示结构体字段，也表示函数参数链表；`Symbol` 保存名字和类型；`Scope` 表示作用域，并通过父指针形成作用域链。

整体分析流程是：先加入内置函数 `read` 和 `write`，再从 AST 根节点开始分层遍历。遇到定义就填符号表，遇到使用就查符号表，遇到表达式就计算类型并检查是否匹配。

主要调用顺序是：

```text
semantic_check()
-> add_builtin_functions()
-> analyze_ext_def_list()
-> analyze_ext_def()
-> analyze_compst()
-> analyze_stmt()
-> analyze_exp()
```

`read` 和 `write` 提前加入函数表，是为了让用户写 `read()` 或 `write(x)` 时，语义分析能把它们当成普通函数检查。后续实践 3 会把它们翻译成 `READ` 和 `WRITE` IR，实践 4 再翻译成 MIPS 的系统调用。

运行演示：

```sh
cd /home/leyi/Desktop/submission/proj2
make semantic-test
```

单独展示手写样例：

```sh
cd /home/leyi/Desktop/submission/proj2
make semantic
nl -ba test/proj2-test1.cmm
CMM_SEMANTIC=1 build/cmmc test/proj2-test1.cmm
```

## 实践 3 中间代码 IR 生成

实践 3 把通过语义检查的 AST 翻译成线性三地址 IR。IR 是源程序和目标汇编之间的中间表示，比 AST 更接近执行过程，比 MIPS 更抽象，适合后续生成目标代码和做优化。

核心文件是 `proj3/src/irgen.c` 和 `proj3/src/irgen.h`。入口函数是：

```c
ir_generate(parse_root, output_path)
```

它接收 AST 根节点和输出文件路径。这里的 AST 在内存中，`output_path` 是要写出的 `.ir` 文件。

IR 生成的主要顺序是：先收集函数签名，再生成函数定义和函数体。函数体中按照源程序顺序生成声明、语句和表达式。表达式结果用临时变量 `t1`、`t2` 保存；条件和循环用 `LABEL`、`IF`、`GOTO`；函数调用用 `ARG` 和 `CALL`；`read`、`write` 特殊翻译成 `READ`、`WRITE`。

常用函数包括：

```text
ir_generate()
gen_ext_def_list()
gen_stmt()
gen_exp()
gen_addr()
gen_cond()
```

`gen_exp()` 负责生成表达式的值，`gen_addr()` 负责生成左值地址。数组和结构体访问会转换成地址计算和间接访问。

运行演示：

```sh
cd /home/leyi/Desktop/submission/proj3
make ir
nl -ba test/proj3-test1.cmm
CMM_IR=1 build/cmmc test/proj3-test1.cmm /tmp/proj3-test1.ir
sed -n '1,120p' /tmp/proj3-test1.ir
```

`proj3-test1.cmm` 中的 `square(i)` 会生成：

```text
ARG i
t2 := CALL square
```

`while (i < 5)` 会生成：

```text
LABEL label1 :
IF i < #5 GOTO label2
GOTO label3
```

`write(total)` 会生成：

```text
WRITE total
```

IRSim 验证：

```sh
cd /home/leyi/Desktop/submission/proj3
make irsim-test
```

输出中的：

```text
irsim tests: 5 passed, 0 failed
```

表示生成的 IR 可以被 IRSim 正确执行。

## 实践 4 MIPS 目标代码生成

实践 4 把 IR 翻译成 MIPS 汇编。它不是直接从 AST 生成 MIPS，而是复用实践 3 的 IR。`parser_main.c` 在 MIPS 模式下会先生成一个临时 IR 文件，再调用：

```c
mips_generate_from_ir(tmp_ir, output_path)
```

核心文件是 `proj4/src/mipsgen.c` 和 `proj4/src/mipsgen.h`。

`mipsgen.c` 的流程分两步。第一步读取 IR，按 `FUNCTION` 分成多个函数，收集每个函数中的变量、临时变量、参数和原始 IR 指令。第二步为每个函数分配栈帧，把变量映射到 `$fp` 的偏移，再逐条把 IR 指令翻译成 MIPS。

主要函数包括：

```text
mips_generate_from_ir()
parse_ir()
layout_frame()
load_operand()
emit_assignment()
emit_instr()
emit_func()
```

`layout_frame()` 负责给变量分配栈空间。函数入口会保存 `$ra` 和旧 `$fp`，再设置新的 `$fp`。变量通过类似 `-24($fp)` 的偏移访问。

IR 中的赋值和算术会翻译成 `lw`、`add`、`sub`、`mul`、`sw` 等指令。条件跳转会翻译成 `beq`、`bne`、`blt`、`bgt` 等分支指令。函数调用时，调用者先把参数压栈，执行 `jal` 跳转，被调函数把返回值放入 `$v0`，返回后调用者保存 `$v0` 并回收参数空间。

`READ` 和 `WRITE` 会翻译成 SPIM 的系统调用。`WRITE` 输出整数后还会输出换行字符串。

运行演示：

```sh
cd /home/leyi/Desktop/submission/proj4
make mips
nl -ba test/proj4-test1.cmm
build/cmmc test/proj4-test1.cmm /tmp/proj4-test1.s
sed -n '1,140p' /tmp/proj4-test1.s
spim -quiet -file /tmp/proj4-test1.s
```

手写样例输出：

```text
30
```

这是因为程序计算的是：

```text
1 * 1 + 2 * 2 + 3 * 3 + 4 * 4 = 30
```

批量测试：

```sh
cd /home/leyi/Desktop/submission/proj4
make mips-test
```

## 实践 5 IR 优化

实践 5 在 IR 层做优化。优化对象不是 `.cmm` 源程序，也不是实践 4 的 MIPS，而是实践 3 生成的原始 IR 文本。输入是 `.ir` 文件，输出是优化后的 `.opt.ir` 文件。

核心文件是 `proj5/src/iropt.c`、`proj5/src/iropt.h` 和 `proj5/src/iropt_main.c`。`iropt_main.c` 是命令行入口，调用：

```c
ir_optimize_file(input_path, output_path)
```

`iropt.c` 把 IR 文件按行读入 `LineVec`。每一行是一个 `IrLine`，保存原始文本和是否删除的标记。课程 IR 是线性三地址代码，一行基本对应一条指令，所以可以通过识别固定格式来优化，例如：

```text
x := y
x := a + b
IF a < b GOTO label1
RETURN x
WRITE x
ARG x
```

主要优化包括：

```text
常量传播
常量折叠
复制传播
公共表达式消除
基本块死代码删除
全局死代码删除
```

`rewrite_pass()` 负责表达式改写，包括常量传播、常量折叠、复制传播和公共表达式消除。`dce_pass()` 删除基本块内无用赋值。`global_dce_pass()` 做活跃变量分析，删除跨基本块也不会再使用的赋值。

例如原始 IR：

```text
v1 := #10
v2 := v1
t1 := v2 + #20
WRITE t1
```

优化过程是：`v1` 是 `#10`，`v2` 复制 `v1`，所以 `v2` 也是 `#10`；`t1 := v2 + #20` 可以变成 `t1 := #10 + #20`，再折叠成 `#30`；最后 `WRITE t1` 可以改写成：

```text
WRITE #30
```

运行演示：

```sh
cd /home/leyi/Desktop/submission/proj5
make opt
nl -ba test/proj5-test1.ir
build/cmm-opt test/proj5-test1.ir /tmp/proj5-test1.opt.ir
nl -ba /tmp/proj5-test1.opt.ir
diff -u test/proj5-test1.ir /tmp/proj5-test1.opt.ir || true
```

`diff` 有差异时返回码是 1，这是正常的，表示优化前后 IR 内容发生了变化。

批量优化和 IRSim 验证：

```sh
cd /home/leyi/Desktop/submission/proj5
make opt-test
```

输出中的：

```text
irsim opt tests: 4 passed, 0 failed
```

表示优化后的 IR 和原始 IR 运行结果一致。

## 最短运行命令合集

时间紧时按下面顺序运行。

实践 1：

```sh
cd /home/leyi/Desktop/submission/proj1
make lexer
CMM_LEX_DUMP=1 build/cmm-lexer test/proj1-test1.cmm
make parser
build/cmmc test/proj1-test1.cmm /tmp/proj1-test1.ast
sed -n '1,50p' /tmp/proj1-test1.ast
```

实践 2：

```sh
cd /home/leyi/Desktop/submission/proj2
make semantic-test
```

实践 3：

```sh
cd /home/leyi/Desktop/submission/proj3
make ir
CMM_IR=1 build/cmmc test/proj3-test1.cmm /tmp/proj3-test1.ir
sed -n '1,80p' /tmp/proj3-test1.ir
make irsim-test
```

实践 4：

```sh
cd /home/leyi/Desktop/submission/proj4
make mips
build/cmmc test/proj4-test1.cmm /tmp/proj4-test1.s
spim -quiet -file /tmp/proj4-test1.s
```

实践 5：

```sh
cd /home/leyi/Desktop/submission/proj5
make opt-test
diff -u test/proj5-test1.ir build/project5-opt/proj5-test1.opt.ir || true
```

## 结尾总结

本次课程设计完成了一个 C-- 小型编译器的主要阶段。前端用 Flex 和 Bison 完成 token 识别、语法分析和 AST 构造；语义分析在 AST 上检查类型、作用域和函数调用；IR 生成把程序翻译成三地址中间代码；MIPS 后端把 IR 转换为可以在 SPIM 中运行的汇编；最后在 IR 层实现常量传播、常量折叠、公共表达式消除和死代码删除，并用 IRSim 验证优化前后语义一致。
