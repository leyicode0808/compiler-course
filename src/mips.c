#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mips.h"

typedef struct VarInfo {
    char *name;
    int offset;
    struct VarInfo *next;
} VarInfo;

static VarInfo *vars = NULL;
static int stack_offset = 0;

static int is_node(ASTNode *node, const char *name) {
    return node != NULL && node->name != NULL && strcmp(node->name, name) == 0;
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

static char *copy_string(const char *text) {
    char *result;

    result = malloc(strlen(text) + 1);
    if (result == NULL) {
        fprintf(stderr, "fatal: out of memory\n");
        exit(1);
    }

    strcpy(result, text);
    return result;
}

static void clear_vars(void) {
    VarInfo *cur = vars;

    while (cur != NULL) {
        VarInfo *next = cur->next;
        free(cur->name);
        free(cur);
        cur = next;
    }

    vars = NULL;
    stack_offset = 0;
}

static VarInfo *find_var(const char *name) {
    VarInfo *cur = vars;

    while (cur != NULL) {
        if (strcmp(cur->name, name) == 0) {
            return cur;
        }
        cur = cur->next;
    }

    return NULL;
}

static void add_var(const char *name) {
    VarInfo *var;

    if (find_var(name) != NULL) {
        return;
    }

    stack_offset -= 4;

    var = malloc(sizeof(VarInfo));
    if (var == NULL) {
        fprintf(stderr, "fatal: out of memory\n");
        exit(1);
    }

    var->name = copy_string(name);
    var->offset = stack_offset;
    var->next = vars;
    vars = var;

    printf("    addi $sp, $sp, -4\n");
}

static void gen_expr(ASTNode *node);

static void gen_binary(ASTNode *node, const char *op) {
    ASTNode *left = node->first_child;
    ASTNode *right = left != NULL ? left->next_sibling : NULL;

    gen_expr(left);
    printf("    addi $sp, $sp, -4\n");
    printf("    sw $t0, 0($sp)\n");

    gen_expr(right);
    printf("    lw $t1, 0($sp)\n");
    printf("    addi $sp, $sp, 4\n");

    if (strcmp(op, "+") == 0) {
        printf("    add $t0, $t1, $t0\n");
    } else if (strcmp(op, "-") == 0) {
        printf("    sub $t0, $t1, $t0\n");
    } else if (strcmp(op, "*") == 0) {
        printf("    mul $t0, $t1, $t0\n");
    } else if (strcmp(op, "/") == 0) {
        printf("    div $t1, $t0\n");
        printf("    mflo $t0\n");
    }
}

static void gen_expr(ASTNode *node) {
    VarInfo *var;

    if (node == NULL) {
        printf("    li $t0, 0\n");
        return;
    }

    if (is_node(node, "Int")) {
        printf("    li $t0, %s\n", node->value);
        return;
    }

    if (is_node(node, "Var")) {
        var = find_var(node->value);
        if (var != NULL) {
            printf("    lw $t0, %d($fp)\n", var->offset);
        } else {
            printf("    li $t0, 0\n");
        }
        return;
    }

    if (is_node(node, "Add")) {
        gen_binary(node, "+");
        return;
    }

    if (is_node(node, "Sub")) {
        gen_binary(node, "-");
        return;
    }

    if (is_node(node, "Mul")) {
        gen_binary(node, "*");
        return;
    }

    if (is_node(node, "Div")) {
        gen_binary(node, "/");
        return;
    }

    printf("    li $t0, 0\n");
}

static void gen_declaration(ASTNode *node) {
    ASTNode *decl_list = find_child(node, "DeclaratorList");
    ASTNode *decl = decl_list != NULL ? decl_list->first_child : NULL;

    while (decl != NULL) {
        if (is_node(decl, "VarDecl")) {
            add_var(decl->value);
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
    if (node == NULL) {
        return;
    }

    if (is_node(node, "Assign")) {
        ASTNode *left = node->first_child;
        ASTNode *right = left != NULL ? left->next_sibling : NULL;
        VarInfo *var;

        gen_expr(right);

        if (left != NULL && is_node(left, "Var")) {
            var = find_var(left->value);
            if (var != NULL) {
                printf("    sw $t0, %d($fp)\n", var->offset);
            }
        }
        return;
    }

    if (is_node(node, "Return")) {
        ASTNode *expr = node->first_child;

        gen_expr(expr);
        printf("    move $a0, $t0\n");
        printf("    li $v0, 1\n");
        printf("    syscall\n");
        printf("    li $v0, 10\n");
        printf("    syscall\n");
        return;
    }

    if (is_node(node, "Compound")) {
        gen_block_items(find_child(node, "BlockItems"));
        return;
    }

    printf("    # unsupported statement: %s\n", node->name);
}

static void gen_function(ASTNode *node) {
    ASTNode *compound;

    clear_vars();

    printf("\n%s:\n", node->value);
    printf("    move $fp, $sp\n");

    compound = find_child(node, "Compound");
    gen_block_items(find_child(compound, "BlockItems"));

    clear_vars();
}

static void gen_function_list(ASTNode *node) {
    ASTNode *child;

    if (node == NULL) {
        return;
    }

    child = node->first_child;
    while (child != NULL) {
        if (is_node(child, "Function")) {
            gen_function(child);
        }
        child = child->next_sibling;
    }
}

void mips_generate(ASTNode *root) {
    printf("\nMIPS code:\n");
    printf(".data\n");
    printf(".text\n");
    printf(".globl main\n");

    gen_function_list(find_child(root, "FunctionList"));
}
