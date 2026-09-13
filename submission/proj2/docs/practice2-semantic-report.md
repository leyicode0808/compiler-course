# 编译原理课程设计报告：实践 2 语义分析

课程设计名称：编译原理课程设计

实践内容：实践 2 语义分析

## 一、目的和任务

本次实践在 `submission/proj2` 目录中独立完成，目标是在实践 1 词法分析、语法分析和 AST 构造的基础上，实现 C-- 语言的语义分析程序。程序在语法分析成功后遍历语法树，检查变量、函数、数组、结构体和表达式的语义约束；若发现语义错误，则按照课程要求输出错误类型、行号和说明文字；若没有语义错误，则不输出内容。

本阶段主要任务如下：

1. 建立 C-- 语言类型系统，支持 `int`、`float`、数组、结构体和函数类型。
2. 建立符号表和作用域结构，记录变量、函数、结构体定义及其可见范围。
3. 检查未定义变量、未定义函数、变量重定义、函数重定义、结构体重名、未定义结构体等错误。
4. 检查赋值、运算、返回语句、函数调用、数组访问和结构体成员访问的类型一致性。
5. 支持函数声明与函数定义，并检查函数未定义和声明不一致问题。
6. 保留实践 1 的词法、语法和 AST 前端，使语义分析可以直接复用已有语法树。
7. 编写 `Makefile` 和 `tools/run_semantic_tests.sh`，使实践二可以在 `proj2` 目录中单独构建、单独演示。

## 二、项目结构和文件功能

实践二相关文件均位于 `submission/proj2` 下，目录结构如下：

```text
submission/proj2/
├── Makefile
├── course/
│   ├── Appendix.pdf
│   └── Project_2.pdf
├── docs/
│   ├── practice2-semantic.md
│   ├── practice2-semantic-report.md
│   └── practice2-semantic-report.docx
├── src/
│   ├── ast.c
│   ├── ast.h
│   ├── lexer.l
│   ├── lexer_main.c
│   ├── parser.y
│   ├── parser_main.c
│   ├── semantic.c
│   ├── semantic.h
│   └── token.h
├── test/
│   ├── proj2-3-3-6-exp1-1.cmm
│   ├── proj2-3-3-6-exp2-1.cmm
│   ├── proj2-3-3-6-exp3-1.cmm
│   ├── proj2-3-3-6-exp5-1.cmm
│   ├── proj2-3-3-6-exp8-1.cmm
│   ├── proj2-3-3-6-exp14-1.cmm
│   ├── proj2-3-3-6-exp17-1.cmm
│   ├── proj2-3-3-7-exp1-0.cmm
│   ├── proj2-3-3-7-exp2-0.cmm
│   ├── proj2-3-3-7-exp3-0.cmm
│   ├── proj2-3-3-7-exp4-0.cmm
│   └── proj2-test1.cmm
└── tools/
    ├── check_env.sh
    └── run_semantic_tests.sh
```

各文件功能如下：

- `Makefile`：实践二独立构建入口。`make semantic` 构建语义分析器，`make semantic-demo` 运行公开样例，`make test` 作为演示别名。
- `src/semantic.c`：实践二核心实现，包含类型系统、字段链表、符号表、作用域、函数表、结构体表和语义遍历逻辑。
- `src/semantic.h`：声明语义分析入口 `semantic_check(AstNode *root)`。
- `src/parser_main.c`：主程序入口。默认行为保持实践 1.2 的 AST 输出；设置 `CMM_SEMANTIC=1` 时调用 `semantic_check()` 执行语义分析。
- `src/parser.y`：Bison 文法和 AST 构造动作。实践二继续使用该文件生成语法树，语义分析基于语法树进行。
- `src/lexer.l`：Flex 词法规则。实践二继续复用实践一的词法分析器。
- `src/ast.c`、`src/ast.h`：AST 节点结构、节点创建、整数解析、先序打印和内存释放。
- `src/lexer_main.c`、`src/token.h`：保留独立词法分析入口和 token 枚举，方便单独验证前端词法部分。
- `tools/run_semantic_tests.sh`：运行 `Project_2.pdf` 中 3.3.6 和 3.3.7 的公开样例，并打印源文件路径和语义分析输出。
- `test/`：按真实章节号命名的公开样例，以及手写综合样例 `proj2-test1.cmm`。

## 三、在实践 1 基础上的修改和新增内容

实践二是在实践一前端的基础上扩展语义检查，没有重写词法和语法分析部分。主要新增和修改如下：

1. 新增 `src/semantic.h`，声明语义分析统一入口。
2. 新增 `src/semantic.c`，实现类型系统、符号表、函数表、结构体表、作用域管理和 AST 语义遍历。
3. 修改 `src/parser_main.c`，增加 `CMM_SEMANTIC=1` 模式。未设置该环境变量时仍输出 AST；设置后只做语义检查，正确程序不输出内容。
4. 修改 `Makefile`，收敛为实践二当前目录可独立运行的目标，只保留 lexer、parser、semantic 和演示相关入口。
5. 新增 `tools/run_semantic_tests.sh`，使用当前 `test/` 目录中的公开样例进行演示。
6. 新增 `test/proj2-3-3-6-...` 和 `test/proj2-3-3-7-...` 样例文件，文件名中的 `3-3-6` 和 `3-3-7` 分别对应 `Project_2.pdf` 的必做样例和选做样例小节。
7. 对表达式赋值检查做了级联错误控制：当赋值左右任一侧已经是错误类型时，不继续报告左值错误或类型不匹配错误，避免一个根因引出多个无关错误。

## 四、主要数据结构和函数功能

### 1. 类型系统

`src/semantic.c` 中使用 `TypeKind` 和 `Type` 表示 C-- 类型：

```text
TY_ERROR   错误类型，用于抑制级联误报
TY_INT     int 基本类型
TY_FLOAT   float 基本类型
TY_ARRAY   数组类型
TY_STRUCT  结构体类型
TY_FUNC    函数类型
```

`Type` 结构中保存数组元素类型和维度、结构体名和字段链表、函数返回类型、形参链表、形参数量、函数是否已定义以及声明行号。

相关函数如下：

- `new_type(TypeKind kind)`：创建指定种类的类型对象。
- `type_int()`、`type_float()`、`type_error()`：返回基本类型和错误类型的共享对象。
- `array_of(Type *elem, int size)`：构造数组类型。
- `struct_type(const char *name, Field *fields)`：构造结构体类型。
- `func_type(Type *ret, Field *params, int param_count, int defined, int line)`：构造函数类型。
- `type_equal(const Type *a, const Type *b)`：判断两个类型是否等价。基本类型要求种类一致，数组要求元素类型和维度一致，具名结构体按结构体名比较。
- `is_numeric(const Type *type)`：判断类型是否可参与数值运算。

### 2. 字段、符号和作用域

语义分析使用三类链表结构：

- `Field`：表示结构体字段或函数参数，保存名称、类型、行号和下一个字段。
- `Symbol`：表示变量或函数符号，保存名称、类型、定义行和下一个符号。
- `Scope`：表示一个作用域，保存当前作用域符号链表和父作用域指针。

相关函数如下：

- `push_scope()`：进入新作用域。
- `pop_scope()`：退出当前作用域。
- `find_in_scope(Scope *scope, const char *name)`：只在指定作用域查找符号。
- `find_symbol(const char *name)`：从当前作用域向外逐层查找变量符号。
- `add_symbol_current(const char *name, Type *type, int line)`：向当前作用域加入变量，并检查同作用域重定义。
- `find_function(const char *name)`：在全局函数表中查找函数。
- `find_struct_def(const char *name)`：在结构体表中查找结构体定义。
- `add_struct_def(const char *name, Type *type, int line)`：加入结构体定义，并检查结构体名重复。

### 3. AST 辅助函数

语义分析需要频繁读取 AST 节点，主要辅助函数如下：

- `is_node(const AstNode *node, const char *name)`：判断节点名称。
- `child(const AstNode *node, size_t idx)`：获取指定子节点。
- `has_child_name(const AstNode *node, const char *name)`：判断是否存在指定名称子节点。
- `first_child_name(const AstNode *node, const char *name)`：返回第一个指定名称子节点。
- `read_vardec(AstNode *var_dec, VarInfo *out)`：解析变量声明节点，提取变量名、行号和数组维度。
- `apply_dims(Type *base, const VarInfo *info)`：把数组维度应用到基础类型上，得到完整变量类型。

### 4. 语义检查主流程函数

- `semantic_check(AstNode *root)`：语义分析总入口。它初始化全局状态，创建全局作用域，加入内置函数 `read` 和 `write`，遍历外部定义，最后检查只声明未定义的函数。
- `analyze_ext_def_list(AstNode *node)`：遍历外部定义列表。
- `analyze_ext_def(AstNode *node)`：处理全局变量、结构体定义、函数声明和函数定义。
- `handle_function(AstNode *ext_def, Type *ret, int is_definition)`：处理函数声明或定义，检查函数重定义、声明不一致，并在函数定义中创建形参作用域。
- `analyze_compst(AstNode *node, Type *return_type, int create_scope)`：分析复合语句，进入或复用作用域，依次检查局部定义和语句列表。
- `analyze_def_list(AstNode *node, int as_fields, Field **fields)`：分析局部变量定义或结构体字段定义。
- `collect_dec()`、`collect_dec_list()`：收集变量或字段声明，并检查字段初始化、字段重定义和局部变量初始化类型。
- `analyze_stmt_list()`、`analyze_stmt()`：分析语句列表、表达式语句、复合语句、返回语句、条件语句和循环语句。
- `analyze_exp(AstNode *node, int *is_lvalue)`：表达式类型综合函数。它检查标识符、常量、赋值、二元运算、函数调用、数组访问、结构体成员访问等表达式，并返回表达式类型。
- `check_undefined_functions()`：语义遍历结束后检查只有声明没有定义的函数。

### 5. 错误输出函数

- `report_error(int type, int line, const char *msg)`：直接输出语义错误并累加错误数。
- `queue_error(int type, int line, const char *msg)`：暂存需要延后输出的错误，主要用于函数声明不一致。
- `flush_pending_errors()`：语义分析结束时输出暂存错误。

错误输出格式为：

```text
Error type 错误类型 at Line 行号: 错误说明.
```

## 五、语义检查内容

当前实现覆盖以下语义检查：

- 类型 1：变量未定义。
- 类型 2：函数未定义。
- 类型 3：变量重定义。
- 类型 4：函数重定义。
- 类型 5：赋值号两侧类型不匹配。
- 类型 6：赋值号左侧不是合法左值。
- 类型 7：运算符两侧操作数类型不匹配。
- 类型 8：返回值类型与函数返回类型不匹配。
- 类型 9：函数调用实参与形参数量或类型不匹配。
- 类型 10：对非数组变量使用下标。
- 类型 11：对非函数变量使用函数调用。
- 类型 12：数组下标不是整数。
- 类型 13：对非结构体变量使用成员访问。
- 类型 14：访问结构体中不存在的字段。
- 类型 15：结构体字段重复定义或字段初始化。
- 类型 16：结构体名称重复。
- 类型 17：使用未定义结构体。
- 类型 18：函数声明后未定义。
- 类型 19：函数声明不一致。

此外，程序支持嵌套作用域。默认情况下，内层作用域可以遮蔽外层变量；若设置 `CMM_NO_SCOPE=1`，则启用旧口径的外层同名冲突检查。

## 六、构建和运行

所有命令均在当前实践目录下执行：

```sh
cd submission/proj2
```

检查环境：

```sh
make check-env
```

构建语义分析器：

```sh
make semantic
```

该命令生成：

```text
build/cmmc
```

运行公开样例演示：

```sh
make semantic-demo
```

也可以执行：

```sh
make semantic-test
```

手动查看某个源文件并运行语义分析：

```sh
nl -ba test/proj2-test1.cmm
CMM_SEMANTIC=1 build/cmmc test/proj2-test1.cmm
```

其中 `nl -ba` 用于打印带行号的源代码，方便对照错误行号；`CMM_SEMANTIC=1` 用于打开语义分析模式。

## 七、结果与结论分析

执行：

```sh
make semantic-demo
```

当前会运行 `Project_2.pdf` 中的公开样例：

- `test/proj2-3-3-6-exp1-1.cmm`：3.3.6 必做样例 1，输出未定义变量错误。
- `test/proj2-3-3-6-exp2-1.cmm`：3.3.6 必做样例 2，输出未定义函数错误。
- `test/proj2-3-3-6-exp3-1.cmm`：3.3.6 必做样例 3，输出变量重定义错误。
- `test/proj2-3-3-6-exp5-1.cmm`：3.3.6 必做样例 5，输出赋值类型不匹配错误。
- `test/proj2-3-3-6-exp8-1.cmm`：3.3.6 必做样例 8，输出返回类型不匹配错误。
- `test/proj2-3-3-6-exp14-1.cmm`：3.3.6 必做样例 14，输出结构体字段不存在错误。
- `test/proj2-3-3-6-exp17-1.cmm`：3.3.6 必做样例 17，输出未定义结构体错误。
- `test/proj2-3-3-7-exp1-0.cmm`：3.3.7 选做样例 1，函数声明与定义一致，不输出错误。
- `test/proj2-3-3-7-exp2-0.cmm`：3.3.7 选做样例 2，输出函数未定义和声明不一致错误。
- `test/proj2-3-3-7-exp3-0.cmm`：3.3.7 选做样例 3，嵌套作用域下不输出错误。
- `test/proj2-3-3-7-exp4-0.cmm`：3.3.7 选做样例 4，输出同一作用域变量重定义错误。

手写样例 `test/proj2-test1.cmm` 覆盖结构体定义、函数参数、结构体成员访问、赋值类型不匹配、函数实参不匹配和未定义变量等情况，可用于现场展示综合语义检查结果。

本次实践完成了 C-- 语言语义分析器。程序能够在语法分析成功后检查变量、函数、数组、结构体和表达式相关语义错误，并按课程要求输出错误类型和行号。当前目录可以独立完成构建与演示，后续实践可继续在 AST 和语义检查结果基础上进行中间代码生成。
