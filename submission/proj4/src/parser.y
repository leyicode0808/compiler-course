%{
#include <stdio.h>
#include <string.h>

#include "ast.h"

extern int yylex(void);
extern int yylineno;
extern int lexical_error_count;
void yyerror(const char *msg);

AstNode *parse_root = NULL;
int syntax_error_count = 0;
static int last_syntax_error_line = 0;

static int node_line(AstNode *node) {
    return node ? node->line : yylineno;
}

static void syntax_error_at(int line, const char *msg) {
    if (lexical_error_count == 0 && line != last_syntax_error_line) {
        printf("Error type B at Line %d: %s.\n", line, msg);
        last_syntax_error_line = line;
    }
    syntax_error_count++;
}

static int ast_has_array_vardec(AstNode *node) {
    if (!node) {
        return 0;
    }
    if (strcmp(node->name, "VarDec") == 0 && node->child_count == 4) {
        return 1;
    }
    for (size_t i = 0; i < node->child_count; ++i) {
        if (ast_has_array_vardec(node->children[i])) {
            return 1;
        }
    }
    return 0;
}
%}

%code requires {
#include "ast.h"
}

%define parse.error verbose
%expect 6
%union {
    AstNode *node;
}

%token <node> INT FLOAT ID SEMI COMMA ASSIGNOP RELOP PLUS MINUS STAR DIV
%token <node> AND OR DOT NOT TYPE LP RP LB RB LC RC STRUCT RETURN IF ELSE WHILE

%type <node> Program ExtDefList ExtDef ExtDecList Specifier StructSpecifier
%type <node> OptTag Tag VarDec FunDec VarList ParamDec CompSt StmtList Stmt
%type <node> DefList Def DecList Dec Exp Args InitList

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

%%

Program
    : ExtDefList
      { parse_root = ast_node("Program", node_line($1), 1, $1); $$ = parse_root; }
    ;

ExtDefList
    : ExtDef ExtDefList
      { $$ = $2 ? ast_node("ExtDefList", node_line($1), 2, $1, $2)
                : ast_node("ExtDefList", node_line($1), 1, $1); }
    | /* empty */
      { $$ = NULL; }
    ;

ExtDef
    : Specifier ExtDecList SEMI
      { $$ = ast_node("ExtDef", node_line($1), 3, $1, $2, $3); }
    | Specifier ExtDecList RB SEMI
      { syntax_error_at(node_line($3), "Extra closing bracket in array declaration"); $$ = ast_node("ExtDef", node_line($1), 4, $1, $2, $3, $4); }
    | Specifier VarDec ASSIGNOP Exp SEMI
      { syntax_error_at(node_line($3), "Global variable initialization not allowed"); $$ = ast_node("ExtDef", node_line($1), 5, $1, $2, $3, $4, $5); }
    | Specifier SEMI
      { $$ = ast_node("ExtDef", node_line($1), 2, $1, $2); }
    | Specifier FunDec SEMI
      {
          if (ast_has_array_vardec($2)) {
              syntax_error_at(node_line($2), "Syntax error");
          }
          $$ = ast_node("ExtDef", node_line($1), 3, $1, $2, $3);
      }
    | Specifier FunDec CompSt
      { $$ = ast_node("ExtDef", node_line($1), 3, $1, $2, $3); }
    | error SEMI
      { yyerrok; $$ = NULL; }
    ;

ExtDecList
    : VarDec
      { $$ = ast_node("ExtDecList", node_line($1), 1, $1); }
    | VarDec COMMA ExtDecList
      { $$ = ast_node("ExtDecList", node_line($1), 3, $1, $2, $3); }
    ;

Specifier
    : TYPE
      { $$ = ast_node("Specifier", node_line($1), 1, $1); }
    | StructSpecifier
      { $$ = ast_node("Specifier", node_line($1), 1, $1); }
    ;

StructSpecifier
    : STRUCT OptTag LC DefList RC
      {
          if ($2 && $4) $$ = ast_node("StructSpecifier", node_line($1), 5, $1, $2, $3, $4, $5);
          else if ($2) $$ = ast_node("StructSpecifier", node_line($1), 4, $1, $2, $3, $5);
          else if ($4) $$ = ast_node("StructSpecifier", node_line($1), 4, $1, $3, $4, $5);
          else $$ = ast_node("StructSpecifier", node_line($1), 3, $1, $3, $5);
      }
    | STRUCT Tag
      { $$ = ast_node("StructSpecifier", node_line($1), 2, $1, $2); }
    ;

OptTag
    : ID
      { $$ = ast_node("OptTag", node_line($1), 1, $1); }
    | /* empty */
      { $$ = NULL; }
    ;

Tag
    : ID
      { $$ = ast_node("Tag", node_line($1), 1, $1); }
    ;

VarDec
    : ID
      { $$ = ast_node("VarDec", node_line($1), 1, $1); }
    | VarDec LB INT RB
      { $$ = ast_node("VarDec", node_line($1), 4, $1, $2, $3, $4); }
    | VarDec LB FLOAT RB
      { syntax_error_at(node_line($3), "Array dimension must be integer"); $$ = ast_node("VarDec", node_line($1), 4, $1, $2, $3, $4); }
    ;

FunDec
    : ID LP VarList RP
      { $$ = ast_node("FunDec", node_line($1), 4, $1, $2, $3, $4); }
    | ID LP VarList COMMA RP
      { syntax_error_at(node_line($4), "Trailing comma in function parameters"); $$ = ast_node("FunDec", node_line($1), 5, $1, $2, $3, $4, $5); }
    | ID LP RP
      { $$ = ast_node("FunDec", node_line($1), 3, $1, $2, $3); }
    ;

VarList
    : ParamDec COMMA VarList
      { $$ = ast_node("VarList", node_line($1), 3, $1, $2, $3); }
    | ParamDec COMMA
      { syntax_error_at(node_line($2), "Trailing comma in function parameters"); $$ = ast_node("VarList", node_line($1), 2, $1, $2); }
    | ParamDec
      { $$ = ast_node("VarList", node_line($1), 1, $1); }
    ;

ParamDec
    : Specifier VarDec
      { $$ = ast_node("ParamDec", node_line($1), 2, $1, $2); }
    ;

CompSt
    : LC DefList StmtList RC
      {
          if ($2 && $3) $$ = ast_node("CompSt", node_line($1), 4, $1, $2, $3, $4);
          else if ($2) $$ = ast_node("CompSt", node_line($1), 3, $1, $2, $4);
          else if ($3) $$ = ast_node("CompSt", node_line($1), 3, $1, $3, $4);
          else $$ = ast_node("CompSt", node_line($1), 2, $1, $4);
      }
    ;

StmtList
    : Stmt StmtList
      { $$ = $2 ? ast_node("StmtList", node_line($1), 2, $1, $2)
                : ast_node("StmtList", node_line($1), 1, $1); }
    | /* empty */
      { $$ = NULL; }
    ;

Stmt
    : Exp SEMI
      { $$ = ast_node("Stmt", node_line($1), 2, $1, $2); }
    | Exp DOT SEMI
      { syntax_error_at(node_line($2), "Missing member name after '.'"); $$ = ast_node("Stmt", node_line($1), 3, $1, $2, $3); }
    | CompSt
      { $$ = ast_node("Stmt", node_line($1), 1, $1); }
    | RETURN Exp SEMI
      { $$ = ast_node("Stmt", node_line($1), 3, $1, $2, $3); }
    | RETURN SEMI
      { syntax_error_at(node_line($1), "Missing return value in non-void function"); $$ = ast_node("Stmt", node_line($1), 2, $1, $2); }
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE
      { $$ = ast_node("Stmt", node_line($1), 5, $1, $2, $3, $4, $5); }
    | IF LP Exp RP Stmt ELSE Stmt
      { $$ = ast_node("Stmt", node_line($1), 7, $1, $2, $3, $4, $5, $6, $7); }
    | WHILE LP Exp RP Stmt
      { $$ = ast_node("Stmt", node_line($1), 5, $1, $2, $3, $4, $5); }
    | WHILE LP Exp RP SEMI
      { syntax_error_at(node_line($5), "Empty while statement body"); $$ = ast_node("Stmt", node_line($1), 5, $1, $2, $3, $4, $5); }
    | Def
      { syntax_error_at(node_line($1), "Variable declaration after statements in block"); $$ = ast_node("Stmt", node_line($1), 1, $1); }
    | error SEMI
      { yyerrok; $$ = NULL; }
    ;

DefList
    : Def DefList
      { $$ = $2 ? ast_node("DefList", node_line($1), 2, $1, $2)
                : ast_node("DefList", node_line($1), 1, $1); }
    | /* empty */
      { $$ = NULL; }
    ;

Def
    : Specifier DecList SEMI
      { $$ = ast_node("Def", node_line($1), 3, $1, $2, $3); }
    | Specifier SEMI
      { syntax_error_at(node_line($1), "Anonymous struct declaration"); $$ = ast_node("Def", node_line($1), 2, $1, $2); }
    ;

DecList
    : Dec
      { $$ = ast_node("DecList", node_line($1), 1, $1); }
    | Dec COMMA DecList
      { $$ = ast_node("DecList", node_line($1), 3, $1, $2, $3); }
    ;

Dec
    : VarDec
      { $$ = ast_node("Dec", node_line($1), 1, $1); }
    | VarDec ASSIGNOP Exp
      { $$ = ast_node("Dec", node_line($1), 3, $1, $2, $3); }
    | VarDec ASSIGNOP LC InitList RC
      { syntax_error_at(node_line($3), "Array initialization not supported"); $$ = ast_node("Dec", node_line($1), 5, $1, $2, $3, $4, $5); }
    ;

InitList
    : Exp
      { $$ = ast_node("InitList", node_line($1), 1, $1); }
    | Exp COMMA InitList
      { $$ = ast_node("InitList", node_line($1), 3, $1, $2, $3); }
    ;

Exp
    : Exp ASSIGNOP Exp
      { $$ = ast_node("Exp", node_line($1), 3, $1, $2, $3); }
    | Exp AND Exp
      { $$ = ast_node("Exp", node_line($1), 3, $1, $2, $3); }
    | Exp OR Exp
      { $$ = ast_node("Exp", node_line($1), 3, $1, $2, $3); }
    | Exp RELOP Exp
      { $$ = ast_node("Exp", node_line($1), 3, $1, $2, $3); }
    | Exp PLUS Exp
      { $$ = ast_node("Exp", node_line($1), 3, $1, $2, $3); }
    | Exp PLUS ASSIGNOP Exp
      { syntax_error_at(node_line($2), "Unsupported compound assignment operator '+='"); $$ = ast_node("Exp", node_line($1), 4, $1, $2, $3, $4); }
    | Exp MINUS Exp
      { $$ = ast_node("Exp", node_line($1), 3, $1, $2, $3); }
    | Exp STAR Exp
      { $$ = ast_node("Exp", node_line($1), 3, $1, $2, $3); }
    | Exp DIV Exp
      { $$ = ast_node("Exp", node_line($1), 3, $1, $2, $3); }
    | LP Exp RP
      { $$ = ast_node("Exp", node_line($1), 3, $1, $2, $3); }
    | MINUS Exp %prec UMINUS
      { $$ = ast_node("Exp", node_line($1), 2, $1, $2); }
    | NOT Exp
      { $$ = ast_node("Exp", node_line($1), 2, $1, $2); }
    | ID LP Args RP
      { $$ = ast_node("Exp", node_line($1), 4, $1, $2, $3, $4); }
    | ID LP Args COMMA RP
      { syntax_error_at(node_line($4), "Trailing comma in function call arguments"); $$ = ast_node("Exp", node_line($1), 5, $1, $2, $3, $4, $5); }
    | ID LP RP
      { $$ = ast_node("Exp", node_line($1), 3, $1, $2, $3); }
    | Exp LB Exp RB
      { $$ = ast_node("Exp", node_line($1), 4, $1, $2, $3, $4); }
    | Exp DOT ID
      { $$ = ast_node("Exp", node_line($1), 3, $1, $2, $3); }
    | ID
      { $$ = ast_node("Exp", node_line($1), 1, $1); }
    | INT
      { $$ = ast_node("Exp", node_line($1), 1, $1); }
    | FLOAT
      { $$ = ast_node("Exp", node_line($1), 1, $1); }
    ;

Args
    : Exp COMMA Args
      { $$ = ast_node("Args", node_line($1), 3, $1, $2, $3); }
    | Exp COMMA
      { syntax_error_at(node_line($2), "Trailing comma in function call arguments"); $$ = ast_node("Args", node_line($1), 2, $1, $2); }
    | Exp
      { $$ = ast_node("Args", node_line($1), 1, $1); }
    ;

%%

void yyerror(const char *msg) {
    (void)msg;
    if (lexical_error_count == 0 && yylineno != last_syntax_error_line) {
        printf("Error type B at Line %d: Syntax error.\n", yylineno);
        last_syntax_error_line = yylineno;
    }
    syntax_error_count++;
}
