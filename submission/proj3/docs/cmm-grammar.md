# C-- 语言文法整理

来源：`编译原理课程设计/Appendix.pdf`，附录 A。

## Tokens

| Token | 形式 |
| --- | --- |
| `INT` | 无符号整型常数，支持十进制、八进制、十六进制 |
| `FLOAT` | 无符号浮点常数，支持普通小数和科学记数法 |
| `ID` | 标识符，字母或下划线开头，后接字母、数字、下划线 |
| `SEMI` | `;` |
| `COMMA` | `,` |
| `ASSIGNOP` | `=` |
| `RELOP` | `>` `<` `>=` `<=` `==` `!=` |
| `PLUS` | `+` |
| `MINUS` | `-` |
| `STAR` | `*` |
| `DIV` | `/` |
| `AND` | `&&` |
| `OR` | `||` |
| `DOT` | `.` |
| `NOT` | `!` |
| `TYPE` | `int` `float` |
| `LP` `RP` | `(` `)` |
| `LB` `RB` | `[` `]` |
| `LC` `RC` | `{` `}` |
| `STRUCT` | `struct` |
| `RETURN` | `return` |
| `IF` | `if` |
| `ELSE` | `else` |
| `WHILE` | `while` |

建议词法正则：

```text
DEC_INT     0|[1-9][0-9]*
OCT_INT     0[0-7]+
HEX_INT     0[xX][0-9a-fA-F]+
INT         {HEX_INT}|{OCT_INT}|{DEC_INT}

DIGIT       [0-9]
EXP         [eE][+-]?{DIGIT}+
FLOAT       ({DIGIT}+\.{DIGIT}+|{DIGIT}+\.|\.{DIGIT}+){EXP}?|{DIGIT}+{EXP}

ID          [_a-zA-Z][_a-zA-Z0-9]*
```

说明：

- Appendix 正文要求普通浮点小数点前后有数字，补充说明允许指数形式中 `43.e-4` 和 `.5E03`，实现时按补充说明接受。
- 保留字优先于 `ID` 匹配。
- 标识符可按课程说明假设长度小于 32。

## Grammar

```text
Program -> ExtDefList

ExtDefList -> ExtDef ExtDefList
            | empty

ExtDef -> Specifier ExtDecList SEMI
        | Specifier SEMI
        | Specifier FunDec CompSt

ExtDecList -> VarDec
            | VarDec COMMA ExtDecList

Specifier -> TYPE
           | StructSpecifier

StructSpecifier -> STRUCT OptTag LC DefList RC
                 | STRUCT Tag

OptTag -> ID
        | empty

Tag -> ID

VarDec -> ID
        | VarDec LB INT RB

FunDec -> ID LP VarList RP
        | ID LP RP

VarList -> ParamDec COMMA VarList
         | ParamDec

ParamDec -> Specifier VarDec

CompSt -> LC DefList StmtList RC

StmtList -> Stmt StmtList
          | empty

Stmt -> Exp SEMI
      | CompSt
      | RETURN Exp SEMI
      | IF LP Exp RP Stmt
      | IF LP Exp RP Stmt ELSE Stmt
      | WHILE LP Exp RP Stmt

DefList -> Def DefList
         | empty

Def -> Specifier DecList SEMI

DecList -> Dec
         | Dec COMMA DecList

Dec -> VarDec
     | VarDec ASSIGNOP Exp

Exp -> Exp ASSIGNOP Exp
     | Exp AND Exp
     | Exp OR Exp
     | Exp RELOP Exp
     | Exp PLUS Exp
     | Exp MINUS Exp
     | Exp STAR Exp
     | Exp DIV Exp
     | LP Exp RP
     | MINUS Exp
     | NOT Exp
     | ID LP Args RP
     | ID LP RP
     | Exp LB Exp RB
     | Exp DOT ID
     | ID
     | INT
     | FLOAT

Args -> Exp COMMA Args
      | Exp
```

## Operator Precedence

从高到低：

| 优先级 | 运算符 | 结合性 |
| --- | --- | --- |
| 1 | `()` 函数调用, `[]`, `.` | 左结合 |
| 2 | unary `-`, `!` | 右结合 |
| 3 | `*`, `/` | 左结合 |
| 4 | `+`, `-` | 左结合 |
| 5 | `<`, `<=`, `>`, `>=`, `==`, `!=` | 左结合 |
| 6 | `&&` | 左结合 |
| 7 | `||` | 左结合 |
| 8 | `=` | 右结合 |

## Comments

- `//` 开始的单行注释直接丢弃到行尾。
- `/* ... */` 是多行注释。
- 多行注释不允许嵌套；遇到嵌套注释需要报错。

## Implementation Notes

- `CompSt` 中局部变量定义必须出现在语句前面。
- `ExtDef -> Specifier SEMI` 允许 `int;` 这类无意义但语法合法的形式，不作为语法错误。
- 结构体可匿名定义，也可带标签定义或引用已定义标签。
- 数组维度语法使用 `INT`，语义阶段再检查维度是否合法。
