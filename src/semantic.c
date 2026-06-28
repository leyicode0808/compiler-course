#include <stddef.h>
#include "semantic.h"

static void visit(ASTNode *node) {
    ASTNode *child;

    if (node == NULL) {
        return;
    }

    child = node->first_child;
    while (child != NULL) {
        visit(child);
        child = child->next_sibling;
    }
}

int semantic_analyze(ASTNode *root) {
    visit(root);
    return 0;
}
