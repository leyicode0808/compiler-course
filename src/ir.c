#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ir.h"

static int temp_count = 1;
static int label_count = 1;

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

static char *new_label(void) {
    char buffer[32];

    snprintf(buffer, sizeof(buffer), "label%d", label_count);
    label_count++;

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
 	
 if (is_node(node, "Float")) {
    snprintf(buffer, sizeof(buffer), "#%s", node->value);
    return copy_string(buffer);
}

if (is_node(node, "ArrayAccess")) {
    char *index = gen_expr(node->first_child);
    char *place = new_temp();

    printf("%s := %s[%s]\n", place, node->value, index);

    free(index);
    return place;
}

if (is_node(node, "Call")) {
    ASTNode *args = find_child(node, "Args");
    ASTNode *arg = args != NULL ? args->first_child : NULL;
    char *place = new_temp();

    while (arg != NULL) {
        char *arg_place = gen_expr(arg);
        printf("ARG %s\n", arg_place);
        free(arg_place);
        arg = arg->next_sibling;
    }

    printf("%s := CALL %s\n", place, node->value);
    return place;
}




if (is_node(node, "Relop")) {
    return gen_binary(node, node->value);
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
} else if (is_node(decl, "ArrayDecl")) {
    ASTNode *size = find_child(decl, "Size");
    printf("DEC %s[%s]\n", decl->value, size != NULL ? size->value : "0");
}
        decl = decl->next_sibling;
    }
}


static void gen_cond(ASTNode *node, const char *true_label, const char *false_label) {
    if (node == NULL) {
        printf("GOTO %s\n", false_label);
        return;
    }

    if (is_node(node, "Relop")) {
        char *left = gen_expr(node->first_child);
        char *right = gen_expr(node->first_child->next_sibling);

        printf("IF %s %s %s GOTO %s\n", left, node->value, right, true_label);
        printf("GOTO %s\n", false_label);

        free(left);
        free(right);
        return;
    }
    if (is_node(node, "And")) {
    char *label_mid = new_label();

    gen_cond(node->first_child, label_mid, false_label);
    printf("LABEL %s :\n", label_mid);
    gen_cond(node->first_child->next_sibling, true_label, false_label);

    free(label_mid);
    return;
}

if (is_node(node, "Or")) {
    char *label_mid = new_label();

    gen_cond(node->first_child, true_label, label_mid);
    printf("LABEL %s :\n", label_mid);
    gen_cond(node->first_child->next_sibling, true_label, false_label);

    free(label_mid);
    return;
}

if (is_node(node, "Not")) {
    gen_cond(node->first_child, false_label, true_label);
    return;
}


    {
        char *place = gen_expr(node);
        printf("IF %s != #0 GOTO %s\n", place, true_label);
        printf("GOTO %s\n", false_label);
        free(place);
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
} else if (left != NULL && is_node(left, "ArrayAccess")) {
    char *index = gen_expr(left->first_child);
    printf("%s[%s] := %s\n", left->value, index, right);
    free(index);
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
    
    if (is_node(node, "If")) {
    ASTNode *cond = node->first_child;
    ASTNode *then_stmt = cond != NULL ? cond->next_sibling : NULL;
    char *label_true = new_label();
    char *label_false = new_label();

    gen_cond(cond, label_true, label_false);

    printf("LABEL %s :\n", label_true);
    gen_stmt(then_stmt);
    printf("LABEL %s :\n", label_false);

    free(label_true);
    free(label_false);
    return;
}

if (is_node(node, "IfElse")) {
    ASTNode *cond = node->first_child;
    ASTNode *then_stmt = cond != NULL ? cond->next_sibling : NULL;
    ASTNode *else_stmt = then_stmt != NULL ? then_stmt->next_sibling : NULL;
    char *label_true = new_label();
    char *label_false = new_label();
    char *label_end = new_label();

    gen_cond(cond, label_true, label_false);

    printf("LABEL %s :\n", label_true);
    gen_stmt(then_stmt);
    printf("GOTO %s\n", label_end);

    printf("LABEL %s :\n", label_false);
    gen_stmt(else_stmt);

    printf("LABEL %s :\n", label_end);

    free(label_true);
    free(label_false);
    free(label_end);
    return;
}

if (is_node(node, "While")) {
    ASTNode *cond = node->first_child;
    ASTNode *body = cond != NULL ? cond->next_sibling : NULL;
    char *label_begin = new_label();
    char *label_true = new_label();
    char *label_false = new_label();

    printf("LABEL %s :\n", label_begin);
    gen_cond(cond, label_true, label_false);

    printf("LABEL %s :\n", label_true);
    gen_stmt(body);
    printf("GOTO %s\n", label_begin);

    printf("LABEL %s :\n", label_false);

    free(label_begin);
    free(label_true);
    free(label_false);
    return;
}
}

static void gen_function(ASTNode *node) {
    ASTNode *compound;

    temp_count = 1;
label_count = 1;

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
