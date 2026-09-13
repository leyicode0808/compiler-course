#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ast.h"
#include "irgen.h"
#include "mipsgen.h"
#include "semantic.h"

extern FILE *yyin;
extern int yyparse(void);
extern void yyrestart(FILE *input_file);
extern AstNode *parse_root;
extern int lexical_error_count;
extern int syntax_error_count;

int main(int argc, char **argv) {
    int ir_mode = getenv("CMM_IR") != NULL;
    int mips_mode = getenv("CMM_MIPS") != NULL || (!ir_mode && argc == 3);
    int output_mode = ir_mode || mips_mode;
    if ((!output_mode && argc != 2) || (output_mode && argc != 3)) {
        fprintf(stderr, "Usage: %s <source.cmm>%s\n", argv[0], output_mode ? " <output>" : "");
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
    int ir_errors = 0;
    int mips_errors = 0;
    if (lexical_error_count == 0 && syntax_error_count == 0 && parse_result == 0 &&
        (ir_mode || mips_mode)) {
        semantic_errors = semantic_check(parse_root);
        if (semantic_errors == 0) {
            if (mips_mode) {
                char tmp_ir[256];
                snprintf(tmp_ir, sizeof(tmp_ir), "/tmp/cmm_ir_%ld.ir", (long)getpid());
                ir_errors = ir_generate(parse_root, tmp_ir);
                if (ir_errors == 0) {
                    mips_errors = mips_generate_from_ir(tmp_ir, argv[2]);
                }
                remove(tmp_ir);
            } else {
                ir_errors = ir_generate(parse_root, argv[2]);
            }
        }
    } else if (lexical_error_count == 0 && syntax_error_count == 0 && parse_result == 0 &&
        getenv("CMM_SEMANTIC") != NULL) {
        semantic_errors = semantic_check(parse_root);
    } else if (lexical_error_count == 0 && syntax_error_count == 0 && parse_result == 0) {
        ast_print(parse_root, 0);
    }

    ast_free(parse_root);
    fclose(yyin);
    return (lexical_error_count || syntax_error_count || parse_result || semantic_errors || ir_errors || mips_errors) ? 1 : 0;
}
