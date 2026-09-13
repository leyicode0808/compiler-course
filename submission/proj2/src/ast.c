#include "ast.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *xstrdup(const char *text) {
    if (!text) {
        return NULL;
    }
    size_t len = strlen(text);
    char *copy = malloc(len + 1);
    if (!copy) {
        perror("malloc");
        exit(1);
    }
    memcpy(copy, text, len + 1);
    return copy;
}

AstNode *ast_token(const char *name, const char *value, int line) {
    AstNode *node = calloc(1, sizeof(*node));
    if (!node) {
        perror("calloc");
        exit(1);
    }
    node->kind = AST_TOKEN;
    node->name = xstrdup(name);
    node->value = xstrdup(value);
    node->line = line;
    return node;
}

AstNode *ast_node(const char *name, int line, size_t child_count, ...) {
    AstNode *node = calloc(1, sizeof(*node));
    if (!node) {
        perror("calloc");
        exit(1);
    }
    node->kind = AST_NONTERMINAL;
    node->name = xstrdup(name);
    node->line = line;
    node->child_count = child_count;
    if (child_count > 0) {
        node->children = calloc(child_count, sizeof(*node->children));
        if (!node->children) {
            perror("calloc");
            exit(1);
        }
        va_list args;
        va_start(args, child_count);
        for (size_t i = 0; i < child_count; ++i) {
            node->children[i] = va_arg(args, AstNode *);
        }
        va_end(args);
    }
    return node;
}

AstNode *ast_adopt(const char *name, int line, AstNode *first, AstNode *second) {
    size_t first_count = first ? first->child_count : 0;
    size_t second_count = second ? second->child_count : 0;
    AstNode *node = ast_node(name, line, first_count + second_count);
    size_t idx = 0;
    for (size_t i = 0; first && i < first->child_count; ++i) {
        node->children[idx++] = first->children[i];
        first->children[i] = NULL;
    }
    for (size_t i = 0; second && i < second->child_count; ++i) {
        node->children[idx++] = second->children[i];
        second->children[i] = NULL;
    }
    ast_free(first);
    ast_free(second);
    return node;
}

long ast_parse_int(const char *text) {
    return strtol(text, NULL, 0);
}

static void print_indent(int indent) {
    for (int i = 0; i < indent; ++i) {
        putchar(' ');
    }
}

void ast_print(const AstNode *node, int indent) {
    if (!node) {
        return;
    }
    print_indent(indent);
    if (node->kind == AST_TOKEN) {
        if (node->value) {
            if (strcmp(node->name, "INT") == 0) {
                printf("INT: %ld\n", ast_parse_int(node->value));
            } else if (strcmp(node->name, "FLOAT") == 0) {
                printf("FLOAT: %f\n", strtof(node->value, NULL));
            } else if (strcmp(node->name, "ID") == 0 || strcmp(node->name, "TYPE") == 0) {
                printf("%s: %s\n", node->name, node->value);
            } else {
                printf("%s\n", node->name);
            }
        } else {
            printf("%s\n", node->name);
        }
        return;
    }
    printf("%s (%d)\n", node->name, node->line);
    for (size_t i = 0; i < node->child_count; ++i) {
        ast_print(node->children[i], indent + 2);
    }
}

void ast_free(AstNode *node) {
    if (!node) {
        return;
    }
    for (size_t i = 0; i < node->child_count; ++i) {
        ast_free(node->children[i]);
    }
    free(node->children);
    free(node->name);
    free(node->value);
    free(node);
}
