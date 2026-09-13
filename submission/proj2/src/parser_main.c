#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "semantic.h"

extern FILE *yyin;
extern int yyparse(void);
extern void yyrestart(FILE *input_file);
extern AstNode *parse_root;
extern int lexical_error_count;
extern int syntax_error_count;

int main(int argc, char **argv) {
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
    int parse_result = yyparse();

    int semantic_errors = 0;
    if (lexical_error_count == 0 && syntax_error_count == 0 && parse_result == 0 &&
        getenv("CMM_SEMANTIC") != NULL) {
        semantic_errors = semantic_check(parse_root);
    } else if (lexical_error_count == 0 && syntax_error_count == 0 && parse_result == 0) {
        ast_print(parse_root, 0);
    }

    ast_free(parse_root);
    fclose(yyin);
    return (lexical_error_count || syntax_error_count || parse_result || semantic_errors) ? 1 : 0;
}
