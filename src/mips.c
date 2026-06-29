#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mips.h"

typedef struct VarInfo {
    char *name;
    int offset;
    int size;
    int is_array;
    struct VarInfo *next;
} VarInfo;

static VarInfo *vars = NULL;
static int stack_offset = 0;
static int label_count = 1;
static int current_function_is_main = 0;

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

static char *new_label(void) {
    char buffer[32];

    snprintf(buffer, sizeof(buffer), "mips_label%d", label_count);
    label_count++;

    return copy_string(buffer);
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
    var->size = 1;
    var->is_array = 0;
    var->next = vars;
    vars = var;

    printf("    addi $sp, $sp, -4\n");
}
static void add_array(const char *name, int size) {
    VarInfo *var;
    int bytes;

    if (find_var(name) != NULL) {
        return;
    }

    if (size <= 0) {
        size = 1;
    }

    bytes = size * 4;
    stack_offset -= bytes;

    var = malloc(sizeof(VarInfo));
    if (var == NULL) {
        fprintf(stderr, "fatal: out of memory\n");
        exit(1);
    }

    var->name = copy_string(name);
    var->offset = stack_offset;
    var->size = size;
    var->is_array = 1;
    var->next = vars;
    vars = var;

    printf("    addi $sp, $sp, -%d\n", bytes);
}
static void add_param(const char *name, int index) {
    VarInfo *var;
    const char *arg_regs[] = {"$a0", "$a1", "$a2", "$a3"};

    add_var(name);
    var = find_var(name);

    if (var != NULL && index >= 0 && index < 4) {
        printf("    sw %s, %d($fp)\n", arg_regs[index], var->offset);
    }
}

static void gen_expr(ASTNode *node);
static void gen_cond(ASTNode *node, const char *true_label, const char *false_label);


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
    if (is_node(node, "ArrayAccess")) {
        ASTNode *index = node->first_child;

        var = find_var(node->value);
        if (var == NULL) {
            printf("    li $t0, 0\n");
            return;
        }

        gen_expr(index);
        printf("    sll $t0, $t0, 2\n");
        printf("    addi $t1, $fp, %d\n", var->offset);
        printf("    add $t1, $t1, $t0\n");
        printf("    lw $t0, 0($t1)\n");
        return;
    }
        if (is_node(node, "Call")) {
        ASTNode *args = find_child(node, "Args");
        ASTNode *arg = args != NULL ? args->first_child : NULL;
        const char *arg_regs[] = {"$a0", "$a1", "$a2", "$a3"};
        int index = 0;

        while (arg != NULL && index < 4) {
            gen_expr(arg);
            printf("    move %s, $t0\n", arg_regs[index]);
            index++;
            arg = arg->next_sibling;
        }

        if (strcmp(node->value, "main") == 0) {
    printf("    jal main\n");
} else {
    printf("    jal func_%s\n", node->value);
}

printf("    move $t0, $v0\n");
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
} else if (is_node(decl, "ArrayDecl")) {
    ASTNode *size = find_child(decl, "Size");
    add_array(decl->value, size != NULL ? atoi(size->value) : 1);
}


        decl = decl->next_sibling;
    }
}

static void gen_params(ASTNode *node) {
    ASTNode *param;
    int index = 0;

    if (node == NULL) {
        return;
    }

    param = node->first_child;
    while (param != NULL) {
        if (is_node(param, "Param")) {
            add_param(param->value, index);
            index++;
        }
        param = param->next_sibling;
    }
}

static void gen_cond(ASTNode *node, const char *true_label, const char *false_label) {
    ASTNode *left;
    ASTNode *right;

    if (node == NULL) {
        printf("    j %s\n", false_label);
        return;
    }

    if (is_node(node, "Relop")) {
        left = node->first_child;
        right = left != NULL ? left->next_sibling : NULL;

        gen_expr(left);
        printf("    addi $sp, $sp, -4\n");
        printf("    sw $t0, 0($sp)\n");

        gen_expr(right);
        printf("    lw $t1, 0($sp)\n");
        printf("    addi $sp, $sp, 4\n");

        if (strcmp(node->value, ">") == 0) {
            printf("    bgt $t1, $t0, %s\n", true_label);
        } else if (strcmp(node->value, "<") == 0) {
            printf("    blt $t1, $t0, %s\n", true_label);
        } else if (strcmp(node->value, ">=") == 0) {
            printf("    bge $t1, $t0, %s\n", true_label);
        } else if (strcmp(node->value, "<=") == 0) {
            printf("    ble $t1, $t0, %s\n", true_label);
        } else if (strcmp(node->value, "==") == 0) {
            printf("    beq $t1, $t0, %s\n", true_label);
        } else if (strcmp(node->value, "!=") == 0) {
            printf("    bne $t1, $t0, %s\n", true_label);
        }

        printf("    j %s\n", false_label);
        return;
    }
        if (is_node(node, "And")) {
        char *label_mid = new_label();

        gen_cond(node->first_child, label_mid, false_label);
        printf("%s:\n", label_mid);
        gen_cond(node->first_child->next_sibling, true_label, false_label);

        free(label_mid);
        return;
    }

    if (is_node(node, "Or")) {
        char *label_mid = new_label();

        gen_cond(node->first_child, true_label, label_mid);
        printf("%s:\n", label_mid);
        gen_cond(node->first_child->next_sibling, true_label, false_label);

        free(label_mid);
        return;
    }

    if (is_node(node, "Not")) {
        gen_cond(node->first_child, false_label, true_label);
        return;
    }

    gen_expr(node);
    printf("    bne $t0, $zero, %s\n", true_label);
    printf("    j %s\n", false_label);
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
    return;
}

if (left != NULL && is_node(left, "ArrayAccess")) {
    ASTNode *index = left->first_child;

    printf("    addi $sp, $sp, -4\n");
    printf("    sw $t0, 0($sp)\n");

    var = find_var(left->value);
    if (var != NULL) {
        gen_expr(index);
        printf("    sll $t0, $t0, 2\n");
        printf("    addi $t1, $fp, %d\n", var->offset);
        printf("    add $t1, $t1, $t0\n");
        printf("    lw $t2, 0($sp)\n");
        printf("    sw $t2, 0($t1)\n");
    }

    printf("    addi $sp, $sp, 4\n");
    return;
}

return;
    }

    if (is_node(node, "Return")) {
    ASTNode *expr = node->first_child;

    gen_expr(expr);

    if (current_function_is_main) {
        printf("    move $a0, $t0\n");
        printf("    li $v0, 1\n");
        printf("    syscall\n");
        printf("    li $v0, 10\n");
        printf("    syscall\n");
    } else {
        printf("    move $v0, $t0\n");
        printf("    lw $ra, -4($fp)\n");
        printf("    move $sp, $fp\n");
        printf("    jr $ra\n");
    }

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
        printf("%s:\n", label_true);
        gen_stmt(then_stmt);
        printf("%s:\n", label_false);

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
        printf("%s:\n", label_true);
        gen_stmt(then_stmt);
        printf("    j %s\n", label_end);
        printf("%s:\n", label_false);
        gen_stmt(else_stmt);
        printf("%s:\n", label_end);

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

        printf("%s:\n", label_begin);
        gen_cond(cond, label_true, label_false);
        printf("%s:\n", label_true);
        gen_stmt(body);
        printf("    j %s\n", label_begin);
        printf("%s:\n", label_false);

        free(label_begin);
        free(label_true);
        free(label_false);
        return;
    }
    
    printf("    # unsupported statement: %s\n", node->name);
}

static void gen_function(ASTNode *node) {
    ASTNode *params;
    ASTNode *compound;

    clear_vars();

    current_function_is_main = strcmp(node->value, "main") == 0;

    if (current_function_is_main) {
    printf("\nmain:\n");
} else {
    printf("\nfunc_%s:\n", node->value);
}

printf("    move $fp, $sp\n");

    if (!current_function_is_main) {
        printf("    addi $sp, $sp, -4\n");
        printf("    sw $ra, 0($sp)\n");
        stack_offset = -4;
    }

    params = find_child(node, "Params");
    gen_params(params);

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
