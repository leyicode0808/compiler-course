%code requires {
#include "ast.h"
}

%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "semantic.h"
#include "ir.h"
#include "mips.h"
#include "optimize.h"

extern int yylex(void);
extern int yylineno;
extern FILE *yyin;

static ASTNode *root = NULL;

static ASTNode *new_node(const char *name, const char *value) {
    return ast_new_node(name, value, yylineno);
}

static void add_child(ASTNode *parent, ASTNode *child) {
    if (parent != NULL && child != NULL) {
        ast_add_child(parent, child);
    }
}

void yyerror(const char *msg);
%}

%union {
    char *text;
    ASTNode *node;
}

%token <text> TYPE ID INT FLOAT RELOP
%token RETURN IF ELSE WHILE
%token LP RP LB RB LC RC SEMI COMMA
%token ASSIGNOP
%token PLUS MINUS STAR DIV
%token AND OR NOT

%type <node> program function_list function
%type <node> param_list_opt param_list param
%type <node> compound_stmt block_items block_item
%type <node> declaration declarator_list declarator
%type <node> stmt lvalue expr
%type <node> arg_list_opt arg_list

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE
%left OR
%left AND
%right NOT
%left RELOP
%left PLUS MINUS
%left STAR DIV

%%

program
    : function_list
      {
          root = new_node("Program", NULL);
	  add_child(root, $1);
	  
	  if (semantic_analyze(root) == 0) {
    optimize_ast(root);
    ast_print(root, 0);
    ir_generate(root);
    mips_generate(root);
}
		
	  ast_free(root);
      }
    ;

function_list
    : function
      {
          $$ = new_node("FunctionList", NULL);
          add_child($$, $1);
      }
    | function_list function
      {
          $$ = $1;
          add_child($$, $2);
      }
    ;

function
    : TYPE ID LP param_list_opt RP compound_stmt
      {
          $$ = new_node("Function", $2);

          ASTNode *ret_type = new_node("ReturnType", $1);
          add_child($$, ret_type);
          add_child($$, $4);
          add_child($$, $6);

          free($1);
          free($2);
      }
    ;

param_list_opt
    : /* empty */
      {
          $$ = new_node("Params", NULL);
      }
    | param_list
      {
          $$ = $1;
      }
    ;

param_list
    : param
      {
          $$ = new_node("Params", NULL);
          add_child($$, $1);
      }
    | param_list COMMA param
      {
          $$ = $1;
          add_child($$, $3);
      }
    ;

param
    : TYPE ID
      {
          $$ = new_node("Param", $2);
          add_child($$, new_node("Type", $1));

          free($1);
          free($2);
      }
    ;

compound_stmt
    : LC block_items RC
      {
          $$ = new_node("Compound", NULL);
          add_child($$, $2);
      }
    ;

block_items
    : /* empty */
      {
          $$ = new_node("BlockItems", NULL);
      }
    | block_items block_item
      {
          $$ = $1;
          add_child($$, $2);
      }
    ;

block_item
    : declaration
      {
          $$ = $1;
      }
    | stmt
      {
          $$ = $1;
      }
    ;

declaration
    : TYPE declarator_list SEMI
      {
          $$ = new_node("Declaration", NULL);
          add_child($$, new_node("Type", $1));
          add_child($$, $2);

          free($1);
      }
    ;

declarator_list
    : declarator
      {
          $$ = new_node("DeclaratorList", NULL);
          add_child($$, $1);
      }
    | declarator_list COMMA declarator
      {
          $$ = $1;
          add_child($$, $3);
      }
    ;

declarator
    : ID
      {
          $$ = new_node("VarDecl", $1);
          free($1);
      }
    | ID LB INT RB
      {
          $$ = new_node("ArrayDecl", $1);
          add_child($$, new_node("Size", $3));

          free($1);
          free($3);
      }
    ;

stmt
    : SEMI
      {
          $$ = new_node("EmptyStmt", NULL);
      }
    | expr SEMI
      {
          $$ = new_node("ExprStmt", NULL);
          add_child($$, $1);
      }
    | RETURN expr SEMI
      {
          $$ = new_node("Return", NULL);
          add_child($$, $2);
      }
    | lvalue ASSIGNOP expr SEMI
      {
          $$ = new_node("Assign", NULL);
          add_child($$, $1);
          add_child($$, $3);
      }
    | compound_stmt
      {
          $$ = $1;
      }
    | IF LP expr RP stmt %prec LOWER_THAN_ELSE
      {
          $$ = new_node("If", NULL);
          add_child($$, $3);
          add_child($$, $5);
      }
    | IF LP expr RP stmt ELSE stmt
      {
          $$ = new_node("IfElse", NULL);
          add_child($$, $3);
          add_child($$, $5);
          add_child($$, $7);
      }
    | WHILE LP expr RP stmt
      {
          $$ = new_node("While", NULL);
          add_child($$, $3);
          add_child($$, $5);
      }
    ;

lvalue
    : ID
      {
          $$ = new_node("Var", $1);
          free($1);
      }
    | ID LB expr RB
      {
          $$ = new_node("ArrayAccess", $1);
          add_child($$, $3);

          free($1);
      }
    ;

expr
    : INT
      {
          $$ = new_node("Int", $1);
          free($1);
      }
    | FLOAT
      {
          $$ = new_node("Float", $1);
          free($1);
      }
    | lvalue
      {
          $$ = $1;
      }
    | ID LP arg_list_opt RP
      {
          $$ = new_node("Call", $1);
          add_child($$, $3);

          free($1);
      }
    | expr OR expr
      {
          $$ = new_node("Or", NULL);
          add_child($$, $1);
          add_child($$, $3);
      }
    | expr AND expr
      {
          $$ = new_node("And", NULL);
          add_child($$, $1);
          add_child($$, $3);
      }
    | NOT expr
      {
          $$ = new_node("Not", NULL);
          add_child($$, $2);
      }
    | expr RELOP expr
      {
          $$ = new_node("Relop", $2);
          add_child($$, $1);
          add_child($$, $3);

          free($2);
      }
    | expr PLUS expr
      {
          $$ = new_node("Add", NULL);
          add_child($$, $1);
          add_child($$, $3);
      }
    | expr MINUS expr
      {
          $$ = new_node("Sub", NULL);
          add_child($$, $1);
          add_child($$, $3);
      }
    | expr STAR expr
      {
          $$ = new_node("Mul", NULL);
          add_child($$, $1);
          add_child($$, $3);
      }
    | expr DIV expr
      {
          $$ = new_node("Div", NULL);
          add_child($$, $1);
          add_child($$, $3);
      }
    | LP expr RP
      {
          $$ = $2;
      }
    ;

arg_list_opt
    : /* empty */
      {
          $$ = new_node("Args", NULL);
      }
    | arg_list
      {
          $$ = $1;
      }
    ;

arg_list
    : expr
      {
          $$ = new_node("Args", NULL);
          add_child($$, $1);
      }
    | arg_list COMMA expr
      {
          $$ = $1;
          add_child($$, $3);
      }
    ;

%%

void yyerror(const char *msg) {
    fprintf(stderr, "Syntax error at line %d: %s\n", yylineno, msg);
}

int main(int argc, char **argv) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (yyin == NULL) {
            perror(argv[1]);
            return 1;
        }
    }

    int result = yyparse();

    if (argc > 1) {
        fclose(yyin);
    }

    return result;
}
