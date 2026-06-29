#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "optimize.h"

static int is_node(ASTNode *node, const char *name) {
    return node != NULL && node->name != NULL && strcmp(node->name, name) == 0;
}

static char *copy_string(const char *text) {
    char *result;

    if (text == NULL) {
        return NULL;
    }

    result = malloc(strlen(text) + 1);
    if (result == NULL) {
        fprintf(stderr, "fatal: out of memory\n");
        exit(1);
    }

    strcpy(result, text);
    return result;
}

static int is_binary_op(ASTNode *node) {
    return is_node(node, "Add")
        || is_node(node, "Sub")
        || is_node(node, "Mul")
        || is_node(node, "Div");
}

static const char *op_symbol(ASTNode *node) {
    if (is_node(node, "Add")) {
        return "+";
    }
    if (is_node(node, "Sub")) {
        return "-";
    }
    if (is_node(node, "Mul")) {
        return "*";
    }
    if (is_node(node, "Div")) {
        return "/";
    }
    return "?";
}

static int eval_binary(ASTNode *node, int left, int right, int *ok) {
    *ok = 1;

    if (is_node(node, "Add")) {
        return left + right;
    }
    if (is_node(node, "Sub")) {
        return left - right;
    }
    if (is_node(node, "Mul")) {
        return left * right;
    }
    if (is_node(node, "Div")) {
        if (right == 0) {
            *ok = 0;
            return 0;
        }
        return left / right;
    }

    *ok = 0;
    return 0;
}

static void replace_with_int(ASTNode *node, int value) {
    char buffer[64];

    snprintf(buffer, sizeof(buffer), "%d", value);

    if (node->first_child != NULL) {
        ast_free(node->first_child);
        node->first_child = NULL;
    }

    free(node->name);
    free(node->value);

    node->name = copy_string("Int");
    node->value = copy_string(buffer);
}

static void optimize_node(ASTNode *node) {
    ASTNode *child;
    ASTNode *left;
    ASTNode *right;
    int left_value;
    int right_value;
    int result;
    int ok;

    if (node == NULL) {
        return;
    }

    child = node->first_child;
    while (child != NULL) {
        optimize_node(child);
        child = child->next_sibling;
    }

    if (!is_binary_op(node)) {
        return;
    }

    left = node->first_child;
    right = left != NULL ? left->next_sibling : NULL;

    if (!is_node(left, "Int") || !is_node(right, "Int")) {
        return;
    }

    left_value = atoi(left->value);
    right_value = atoi(right->value);
    result = eval_binary(node, left_value, right_value, &ok);

    if (!ok) {
        return;
    }

    printf("Optimization: constant folded %d %s %d -> %d\n",
           left_value, op_symbol(node), right_value, result);

    replace_with_int(node, result);
}

void optimize_ast(ASTNode *root) {
    optimize_node(root);
}
