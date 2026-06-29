#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ir.h"

static int temp_count = 1;

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

static char *new_temp(void) {
    char buffer[32];

    snprintf(buffer, sizeof(buffer), "t%d", temp_count);
    temp_count++;

    return copy_string(buffer);
}

static ASTNode *find_child(ASTNode *node, const char *name) {
    ASTNode *child;

    if (node == NULL) {
        return NULL;
    }

    child = node->first_child;
    while (child != NULL) {
        if (is_node(child, name)) {
            return child;
        }
        child = child->next_sibling;
    }

    return NULL;
}

static char *gen_expr(ASTNode *node);

static char *gen_binary(ASTNode *node, const char *op) {
    char *left;
    char *right;
    char *place;

    left = gen_expr(node->first_child);
    right = gen_expr(node->first_child->next_sibling);
    place = new_temp();

    printf("%s := %s %s %s\n", place, left, op, right);

    free(left);
    free(right);

    return place;
}

static char *gen_expr(ASTNode *node) {
    char buffer[128];

    if (node == NULL) {
        return copy_string("");
    }

    if (is_node(node, "Int")) {
        snprintf(buffer, sizeof(buffer), "#%s", node->value);
        return copy_string(buffer);
    }

    if (is_node(node, "Var")) {
        return copy_string(node->value);
    }

    if (is_node(node, "Add")) {
        return gen_binary(node, "+");
    }

    if (is_node(node, "Sub")) {
        return gen_binary(node, "-");
    }

    if (is_node(node, "Mul")) {
        return gen_binary(node, "*");
    }

    if (is_node(node, "Div")) {
        return gen_binary(node, "/");
    }

    return copy_string("");
}

static void gen_declaration(ASTNode *node) {
    ASTNode *decl_list;
    ASTNode *decl;

    decl_list = find_child(node, "DeclaratorList");
    decl = decl_list != NULL ? decl_list->first_child : NULL;

    while (decl != NULL) {
        if (is_node(decl, "VarDecl")) {
            printf("DEC %s\n", decl->value);
        }
        decl = decl->next_sibling;
    }
}

static void gen_stmt(ASTNode *node);

static void gen_block_items(ASTNode *node) {
    ASTNode *child;

    if (node == NULL) {
        return;
    }

    child = node->first_child;
    while (child != NULL) {
        if (is_node(child, "Declaration")) {
            gen_declaration(child);
        } else {
            gen_stmt(child);
        }
        child = child->next_sibling;
    }
}

static void gen_stmt(ASTNode *node) {
    char *right;

    if (node == NULL) {
        return;
    }

    if (is_node(node, "Assign")) {
        ASTNode *left = node->first_child;
        ASTNode *expr = left != NULL ? left->next_sibling : NULL;

        right = gen_expr(expr);

        if (left != NULL && is_node(left, "Var")) {
            printf("%s := %s\n", left->value, right);
        }

        free(right);
        return;
    }

    if (is_node(node, "Return")) {
        ASTNode *expr = node->first_child;
        right = gen_expr(expr);
        printf("RETURN %s\n", right);
        free(right);
        return;
    }

    if (is_node(node, "Compound")) {
        gen_block_items(find_child(node, "BlockItems"));
        return;
    }
}

static void gen_function(ASTNode *node) {
    ASTNode *compound;

    temp_count = 1;

    printf("FUNCTION %s :\n", node->value);

    compound = find_child(node, "Compound");
    if (compound != NULL) {
        gen_block_items(find_child(compound, "BlockItems"));
    }
}

static void visit(ASTNode *node) {
    ASTNode *child;

    if (node == NULL) {
        return;
    }

    if (is_node(node, "Function")) {
        gen_function(node);
        return;
    }

    child = node->first_child;
    while (child != NULL) {
        visit(child);
        child = child->next_sibling;
    }
}

void ir_generate(ASTNode *root) {
    printf("\nIntermediate code:\n");
    visit(root);
}
