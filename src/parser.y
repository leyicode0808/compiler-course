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
%token LP RP LC RC SEMI

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
    : LC stmt_list RC
    ;

stmt_list
    : stmt
    | stmt_list stmt
    ;

stmt
    : RETURN INT SEMI
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
