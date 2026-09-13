#include <stdio.h>
#include <stdlib.h>

#include "token.h"

extern FILE *yyin;
extern char *yytext;
extern int yylineno;
extern int yylex(void);
extern void yyrestart(FILE *input_file);

const char *token_name(TokenKind kind) {
    switch (kind) {
    case TOK_EOF: return "EOF";
    case TOK_INT: return "INT";
    case TOK_FLOAT: return "FLOAT";
    case TOK_ID: return "ID";
    case TOK_SEMI: return "SEMI";
    case TOK_COMMA: return "COMMA";
    case TOK_ASSIGNOP: return "ASSIGNOP";
    case TOK_RELOP: return "RELOP";
    case TOK_PLUS: return "PLUS";
    case TOK_MINUS: return "MINUS";
    case TOK_STAR: return "STAR";
    case TOK_DIV: return "DIV";
    case TOK_AND: return "AND";
    case TOK_OR: return "OR";
    case TOK_DOT: return "DOT";
    case TOK_NOT: return "NOT";
    case TOK_TYPE: return "TYPE";
    case TOK_LP: return "LP";
    case TOK_RP: return "RP";
    case TOK_LB: return "LB";
    case TOK_RB: return "RB";
    case TOK_LC: return "LC";
    case TOK_RC: return "RC";
    case TOK_STRUCT: return "STRUCT";
    case TOK_RETURN: return "RETURN";
    case TOK_IF: return "IF";
    case TOK_ELSE: return "ELSE";
    case TOK_WHILE: return "WHILE";
    }
    return "UNKNOWN";
}

static int token_has_value(TokenKind kind) {
    return kind == TOK_INT || kind == TOK_FLOAT || kind == TOK_ID ||
           kind == TOK_TYPE || kind == TOK_RELOP;
}

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

    int dump_tokens = getenv("CMM_LEX_DUMP") != NULL;
    TokenKind kind;
    while ((kind = (TokenKind)yylex()) != TOK_EOF) {
        if (!dump_tokens || lexical_error_count > 0) {
            continue;
        }
        if (token_has_value(kind)) {
            printf("%s: %s\n", token_name(kind), yytext);
        } else {
            printf("%s\n", token_name(kind));
        }
    }

    fclose(yyin);
    return lexical_error_count ? 1 : 0;
}
