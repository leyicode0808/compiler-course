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
%token LP RP LC RC SEMI COMMA
%token ASSIGNOP
%token PLUS MINUS STAR DIV
%token IF ELSE WHILE
%token RELOP

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE
%left RELOP
%left PLUS MINUS
%left STAR DIV

%%
program
    : function
      {
          printf("Syntax OK\n");
      }
    ;

function
    : TYPE ID LP RP compound_stmt
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
    : ID
    | declarator_list COMMA ID
    ;

stmt
    : SEMI
    | expr SEMI
    | RETURN expr SEMI
    | ID ASSIGNOP expr SEMI
    | compound_stmt
    | IF LP expr RP stmt %prec LOWER_THAN_ELSE
    | IF LP expr RP stmt ELSE stmt
    | WHILE LP expr RP stmt
    ;

expr
    : INT
    | ID
    | expr RELOP expr
    | expr PLUS expr
    | expr MINUS expr
    | expr STAR expr
    | expr DIV expr
    | LP expr RP
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
