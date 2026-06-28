%{
#include <stdio.h>
#include <stdlib.h>

extern int yylex(void);
extern int yylineno;
extern FILE *yyin;

void yyerror(const char *msg);
%}

%token TYPE
%token RETURN
%token ID
%token INT
%token LP RP LB RB LC RC SEMI COMMA
%token ASSIGNOP
%token PLUS MINUS STAR DIV
%token IF ELSE WHILE
%token RELOP
%token AND OR NOT

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
          printf("Syntax OK\n");
      }
    ;

function_list
    : function
    | function_list function
    ;


function
    : TYPE ID LP param_list_opt RP compound_stmt
    ;

param_list_opt
    : /* empty */
    | param_list
    ;

param_list
    : param
    | param_list COMMA param
    ;

param
    : TYPE ID
    ;
    
    
compound_stmt
    : LC block_items RC
    ;

block_items
    : /* empty */
    | block_items block_item
    ;

block_item
    : declaration
    | stmt
    ;

declaration
    : TYPE declarator_list SEMI
    ;

declarator_list
    : declarator
    | declarator_list COMMA declarator
    ;

declarator
    : ID
    | ID LB INT RB
    ;
stmt
    : SEMI
    | expr SEMI
    | RETURN expr SEMI
    | lvalue ASSIGNOP expr SEMI
    | compound_stmt
    | IF LP expr RP stmt %prec LOWER_THAN_ELSE
    | IF LP expr RP stmt ELSE stmt
    | WHILE LP expr RP stmt
    ;

lvalue
    : ID
    | ID LB expr RB
    ;

expr
    : INT
    | lvalue
    | ID LP arg_list_opt RP
    | expr OR expr
    | expr AND expr
    | NOT expr
    | expr RELOP expr
    | expr PLUS expr
    | expr MINUS expr
    | expr STAR expr
    | expr DIV expr
    | LP expr RP
    ;
    
arg_list_opt
    : /* empty */
    | arg_list
    ;

arg_list
    : expr
    | arg_list COMMA expr
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
