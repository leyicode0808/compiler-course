#ifndef AST_H
#define AST_H

typedef struct ASTNode {
    char *name;
    char *value;
    int line;
    struct ASTNode *first_child;
    struct ASTNode *next_sibling;
} ASTNode;

ASTNode *ast_new_node(const char *name, const char *value, int line);
void ast_add_child(ASTNode *parent, ASTNode *child);
void ast_print(ASTNode *node, int indent);
void ast_free(ASTNode *node);

#endif
