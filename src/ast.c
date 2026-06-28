#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *ast_strdup(const char *s) {
    if (s == NULL) {
        return NULL;
    }

    size_t len = strlen(s) + 1;
    char *copy = (char *)malloc(len);
    if (copy == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }

    memcpy(copy, s, len);
    return copy;
}

ASTNode *ast_new_node(const char *name, const char *value, int line) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (node == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }

    node->name = ast_strdup(name);
    node->value = ast_strdup(value);
    node->line = line;
    node->first_child = NULL;
    node->next_sibling = NULL;
    return node;
}

void ast_add_child(ASTNode *parent, ASTNode *child) {
    if (parent == NULL || child == NULL) {
        return;
    }

    if (parent->first_child == NULL) {
        parent->first_child = child;
        return;
    }

    ASTNode *cur = parent->first_child;
    while (cur->next_sibling != NULL) {
        cur = cur->next_sibling;
    }
    cur->next_sibling = child;
}

void ast_print(ASTNode *node, int indent) {
    if (node == NULL) {
        return;
    }

    for (int i = 0; i < indent; i++) {
        printf("  ");
    }

    if (node->value != NULL) {
        printf("%s: %s", node->name, node->value);
    } else {
        printf("%s", node->name);
    }

    if (node->line > 0) {
        printf(" (line %d)", node->line);
    }

    printf("\n");

    for (ASTNode *child = node->first_child; child != NULL; child = child->next_sibling) {
        ast_print(child, indent + 1);
    }
}

void ast_free(ASTNode *node) {
    if (node == NULL) {
        return;
    }

    ASTNode *child = node->first_child;
    while (child != NULL) {
        ASTNode *next = child->next_sibling;
        ast_free(child);
        child = next;
    }

    free(node->name);
    free(node->value);
    free(node);
}
