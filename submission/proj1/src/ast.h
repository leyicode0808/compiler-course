#ifndef CMM_AST_H
#define CMM_AST_H

#include <stddef.h>

typedef enum AstKind {
    AST_NONTERMINAL,
    AST_TOKEN
} AstKind;

typedef struct AstNode {
    AstKind kind;
    char *name;
    char *value;
    int line;
    size_t child_count;
    struct AstNode **children;
} AstNode;

AstNode *ast_token(const char *name, const char *value, int line);
AstNode *ast_node(const char *name, int line, size_t child_count, ...);
AstNode *ast_adopt(const char *name, int line, AstNode *first, AstNode *second);
void ast_print(const AstNode *node, int indent);
void ast_free(AstNode *node);

long ast_parse_int(const char *text);

#endif
