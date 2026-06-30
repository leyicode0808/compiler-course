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

static int is_token_node(const char *name) {
    return strcmp(name, "ID") == 0 ||
           strcmp(name, "TYPE") == 0 ||
           strcmp(name, "INT") == 0 ||
           strcmp(name, "FLOAT") == 0 ||
           strcmp(name, "SEMI") == 0 ||
           strcmp(name, "COMMA") == 0 ||
           strcmp(name, "ASSIGNOP") == 0 ||
           strcmp(name, "RELOP") == 0 ||
           strcmp(name, "PLUS") == 0 ||
           strcmp(name, "MINUS") == 0 ||
           strcmp(name, "STAR") == 0 ||
           strcmp(name, "DIV") == 0 ||
           strcmp(name, "AND") == 0 ||
           strcmp(name, "OR") == 0 ||
           strcmp(name, "NOT") == 0 ||
           strcmp(name, "DOT") == 0 ||
           strcmp(name, "LP") == 0 ||
           strcmp(name, "RP") == 0 ||
           strcmp(name, "LB") == 0 ||
           strcmp(name, "RB") == 0 ||
           strcmp(name, "LC") == 0 ||
           strcmp(name, "RC") == 0 ||
           strcmp(name, "RETURN") == 0 ||
           strcmp(name, "IF") == 0 ||
           strcmp(name, "ELSE") == 0 ||
           strcmp(name, "WHILE") == 0 ||
           strcmp(name, "STRUCT") == 0;
}

void ast_print(ASTNode *node, int indent) {
    if (node == NULL) {
        return;
    }

    for (int i = 0; i < indent; i++) {
        printf("  ");
    }

    if (is_token_node(node->name)) {
        if (node->value != NULL) {
            printf("%s: %s\n", node->name, node->value);
        } else {
            printf("%s\n", node->name);
        }
    } else {
        if (node->value != NULL) {
            printf("%s: %s", node->name, node->value);
        } else {
            printf("%s", node->name);
        }

        if (node->line > 0) {
            printf(" (%d)", node->line);
        }

        printf("\n");
    }

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

static void p1_indent(int indent) {
    int i;
    for (i = 0; i < indent; i++) {
        printf("  ");
    }
}

static int p1_line(ASTNode *node) {
    if (node != NULL && node->line > 0) {
        return node->line;
    }
    return 1;
}

static void p1_nonterm(const char *name, ASTNode *node, int indent) {
    p1_indent(indent);
    printf("%s (%d)\n", name, p1_line(node));
}

static void p1_token(const char *name, const char *value, int indent) {
    p1_indent(indent);

    if (value != NULL &&
        (strcmp(name, "ID") == 0 ||
         strcmp(name, "TYPE") == 0 ||
         strcmp(name, "INT") == 0 ||
         strcmp(name, "FLOAT") == 0)) {
        printf("%s: %s\n", name, value);
    } else {
        printf("%s\n", name);
    }
}

static ASTNode *p1_child_named(ASTNode *node, const char *name) {
    ASTNode *child;

    if (node == NULL) {
        return NULL;
    }

    for (child = node->first_child; child != NULL; child = child->next_sibling) {
        if (strcmp(child->name, name) == 0) {
            return child;
        }
    }

    return NULL;
}

static int p1_is_declaration(ASTNode *node) {
    return node != NULL && strcmp(node->name, "Declaration") == 0;
}

static void p1_print_exp(ASTNode *node, int indent);
static void p1_print_stmt(ASTNode *node, int indent);
static void p1_print_declaration(ASTNode *node, int indent);

static void p1_print_specifier(ASTNode *type_node, int indent) {
    p1_nonterm("Specifier", type_node, indent);

    if (type_node != NULL && type_node->value != NULL) {
        if (strncmp(type_node->value, "struct ", 7) == 0) {
            p1_nonterm("StructSpecifier", type_node, indent + 1);
            p1_token("STRUCT", NULL, indent + 2);
            p1_token("ID", type_node->value + 7, indent + 2);
        } else {
            p1_token("TYPE", type_node->value, indent + 1);
        }
    }
}

static void p1_print_vardec(ASTNode *node, int indent) {
    p1_nonterm("VarDec", node, indent);

    if (node == NULL) {
        return;
    }

    if (strcmp(node->name, "VarDecl") == 0) {
        p1_token("ID", node->value, indent + 1);
    } else if (strcmp(node->name, "ArrayDecl") == 0) {
        ASTNode *size = p1_child_named(node, "Size");

        p1_token("ID", node->value, indent + 1);
        p1_token("LB", NULL, indent + 1);

        if (size != NULL) {
            p1_token("INT", size->value, indent + 1);
        }

        p1_token("RB", NULL, indent + 1);
    }
}

static void p1_print_declaration(ASTNode *node, int indent) {
    ASTNode *type_node;
    ASTNode *list;
    ASTNode *decl;

    p1_nonterm("Def", node, indent);

    type_node = p1_child_named(node, "Type");
    p1_print_specifier(type_node, indent + 1);

    list = p1_child_named(node, "DeclaratorList");
    p1_nonterm("DecList", list, indent + 1);

    if (list != NULL) {
        for (decl = list->first_child; decl != NULL; decl = decl->next_sibling) {
            p1_nonterm("Dec", decl, indent + 2);
            p1_print_vardec(decl, indent + 3);

            if (decl->next_sibling != NULL) {
                p1_token("COMMA", NULL, indent + 2);
            }
        }
    }

    p1_token("SEMI", NULL, indent + 1);
}

static void p1_print_compst(ASTNode *node, int indent) {
    ASTNode *block_items;
    ASTNode *item;
    int has_def = 0;
    int has_stmt = 0;

    p1_nonterm("CompSt", node, indent);
    p1_token("LC", NULL, indent + 1);

    block_items = p1_child_named(node, "BlockItems");

    if (block_items != NULL) {
        for (item = block_items->first_child; item != NULL; item = item->next_sibling) {
            if (p1_is_declaration(item)) {
                has_def = 1;
            } else {
                has_stmt = 1;
            }
        }

        if (has_def) {
            p1_nonterm("DefList", block_items, indent + 1);

            for (item = block_items->first_child; item != NULL; item = item->next_sibling) {
                if (p1_is_declaration(item)) {
                    p1_print_declaration(item, indent + 2);
                }
            }
        }

        if (has_stmt) {
            p1_nonterm("StmtList", block_items, indent + 1);

            for (item = block_items->first_child; item != NULL; item = item->next_sibling) {
                if (!p1_is_declaration(item)) {
                    p1_print_stmt(item, indent + 2);
                }
            }
        }
    }

    p1_token("RC", NULL, indent + 1);
}

static void p1_print_args(ASTNode *node, int indent) {
    ASTNode *arg;

    p1_nonterm("Args", node, indent);

    if (node == NULL) {
        return;
    }

    for (arg = node->first_child; arg != NULL; arg = arg->next_sibling) {
        p1_print_exp(arg, indent + 1);

        if (arg->next_sibling != NULL) {
            p1_token("COMMA", NULL, indent + 1);
        }
    }
}

static void p1_print_exp(ASTNode *node, int indent) {
    if (node == NULL) {
        return;
    }

    p1_nonterm("Exp", node, indent);

    if (strcmp(node->name, "Int") == 0) {
        p1_token("INT", node->value, indent + 1);
    } else if (strcmp(node->name, "Float") == 0) {
        p1_token("FLOAT", node->value, indent + 1);
    } else if (strcmp(node->name, "Var") == 0) {
        p1_token("ID", node->value, indent + 1);
    } else if (strcmp(node->name, "MemberAccess") == 0) {
        ASTNode *field = p1_child_named(node, "Field");

        p1_nonterm("Exp", node, indent + 1);
        p1_token("ID", node->value, indent + 2);
        p1_token("DOT", NULL, indent + 1);

        if (field != NULL) {
            p1_token("ID", field->value, indent + 1);
        }
    } else if (strcmp(node->name, "ArrayAccess") == 0) {
        p1_token("ID", node->value, indent + 1);
        p1_token("LB", NULL, indent + 1);
        p1_print_exp(node->first_child, indent + 1);
        p1_token("RB", NULL, indent + 1);
    } else if (strcmp(node->name, "Call") == 0) {
        p1_token("ID", node->value, indent + 1);
        p1_token("LP", NULL, indent + 1);

        if (node->first_child != NULL && node->first_child->first_child != NULL) {
            p1_print_args(node->first_child, indent + 1);
        }

        p1_token("RP", NULL, indent + 1);
    } else if (strcmp(node->name, "Add") == 0 ||
               strcmp(node->name, "Sub") == 0 ||
               strcmp(node->name, "Mul") == 0 ||
               strcmp(node->name, "Div") == 0 ||
               strcmp(node->name, "Relop") == 0 ||
               strcmp(node->name, "And") == 0 ||
               strcmp(node->name, "Or") == 0) {
        ASTNode *left = node->first_child;
        ASTNode *right = left != NULL ? left->next_sibling : NULL;
        const char *op = "PLUS";

        if (strcmp(node->name, "Sub") == 0) {
            op = "MINUS";
        } else if (strcmp(node->name, "Mul") == 0) {
            op = "STAR";
        } else if (strcmp(node->name, "Div") == 0) {
            op = "DIV";
        } else if (strcmp(node->name, "Relop") == 0) {
            op = "RELOP";
        } else if (strcmp(node->name, "And") == 0) {
            op = "AND";
        } else if (strcmp(node->name, "Or") == 0) {
            op = "OR";
        }

        p1_print_exp(left, indent + 1);
        p1_token(op, NULL, indent + 1);
        p1_print_exp(right, indent + 1);
    } else if (strcmp(node->name, "Not") == 0) {
        p1_token("NOT", NULL, indent + 1);
        p1_print_exp(node->first_child, indent + 1);
    } else if (strcmp(node->name, "Assign") == 0) {
        ASTNode *left = node->first_child;
        ASTNode *right = left != NULL ? left->next_sibling : NULL;

        p1_print_exp(left, indent + 1);
        p1_token("ASSIGNOP", NULL, indent + 1);
        p1_print_exp(right, indent + 1);
    }
}

static void p1_print_stmt(ASTNode *node, int indent) {
    p1_nonterm("Stmt", node, indent);

    if (node == NULL) {
        return;
    }

    if (strcmp(node->name, "Return") == 0) {
        p1_token("RETURN", NULL, indent + 1);
        p1_print_exp(node->first_child, indent + 1);
        p1_token("SEMI", NULL, indent + 1);
    } else if (strcmp(node->name, "Assign") == 0) {
        p1_print_exp(node, indent + 1);
        p1_token("SEMI", NULL, indent + 1);
    } else if (strcmp(node->name, "ExprStmt") == 0) {
        p1_print_exp(node->first_child, indent + 1);
        p1_token("SEMI", NULL, indent + 1);
    } else if (strcmp(node->name, "EmptyStmt") == 0) {
        p1_token("SEMI", NULL, indent + 1);
    } else if (strcmp(node->name, "Compound") == 0) {
        p1_print_compst(node, indent + 1);
    } else if (strcmp(node->name, "If") == 0) {
        ASTNode *cond = node->first_child;
        ASTNode *then_stmt = cond != NULL ? cond->next_sibling : NULL;

        p1_token("IF", NULL, indent + 1);
        p1_token("LP", NULL, indent + 1);
        p1_print_exp(cond, indent + 1);
        p1_token("RP", NULL, indent + 1);
        p1_print_stmt(then_stmt, indent + 1);
    } else if (strcmp(node->name, "IfElse") == 0) {
        ASTNode *cond = node->first_child;
        ASTNode *then_stmt = cond != NULL ? cond->next_sibling : NULL;
        ASTNode *else_stmt = then_stmt != NULL ? then_stmt->next_sibling : NULL;

        p1_token("IF", NULL, indent + 1);
        p1_token("LP", NULL, indent + 1);
        p1_print_exp(cond, indent + 1);
        p1_token("RP", NULL, indent + 1);
        p1_print_stmt(then_stmt, indent + 1);
        p1_token("ELSE", NULL, indent + 1);
        p1_print_stmt(else_stmt, indent + 1);
    } else if (strcmp(node->name, "While") == 0) {
        ASTNode *cond = node->first_child;
        ASTNode *body = cond != NULL ? cond->next_sibling : NULL;

        p1_token("WHILE", NULL, indent + 1);
        p1_token("LP", NULL, indent + 1);
        p1_print_exp(cond, indent + 1);
        p1_token("RP", NULL, indent + 1);
        p1_print_stmt(body, indent + 1);
    }
}

static void p1_print_param(ASTNode *node, int indent) {
    ASTNode *type_node;

    p1_nonterm("ParamDec", node, indent);

    type_node = p1_child_named(node, "Type");
    p1_print_specifier(type_node, indent + 1);

    p1_nonterm("VarDec", node, indent + 1);
    p1_token("ID", node->value, indent + 2);
}

static void p1_print_fun_dec(ASTNode *node, int indent) {
    ASTNode *params;
    ASTNode *param;

    p1_nonterm("FunDec", node, indent);
    p1_token("ID", node->value, indent + 1);
    p1_token("LP", NULL, indent + 1);

    params = p1_child_named(node, "Params");
    if (params != NULL && params->first_child != NULL) {
        p1_nonterm("VarList", params, indent + 1);

        for (param = params->first_child; param != NULL; param = param->next_sibling) {
            p1_print_param(param, indent + 2);

            if (param->next_sibling != NULL) {
                p1_token("COMMA", NULL, indent + 2);
            }
        }
    }

    p1_token("RP", NULL, indent + 1);
}

static void p1_print_function(ASTNode *node, int indent) {
    ASTNode *ret_type;
    ASTNode *compound;

    p1_nonterm("ExtDef", node, indent);

    ret_type = p1_child_named(node, "ReturnType");
    p1_print_specifier(ret_type, indent + 1);

    p1_print_fun_dec(node, indent + 1);

    compound = p1_child_named(node, "Compound");
    p1_print_compst(compound, indent + 1);
}

static void p1_print_struct_def(ASTNode *node, int indent) {
    ASTNode *block_items;
    ASTNode *item;

    p1_nonterm("ExtDef", node, indent);
    p1_nonterm("Specifier", node, indent + 1);
    p1_nonterm("StructSpecifier", node, indent + 2);
    p1_token("STRUCT", NULL, indent + 3);
    p1_token("ID", node->value, indent + 3);
    p1_token("LC", NULL, indent + 3);

    block_items = p1_child_named(node, "BlockItems");
    if (block_items != NULL) {
        p1_nonterm("DefList", block_items, indent + 3);

        for (item = block_items->first_child; item != NULL; item = item->next_sibling) {
            if (p1_is_declaration(item)) {
                p1_print_declaration(item, indent + 4);
            }
        }
    }

    p1_token("RC", NULL, indent + 3);
    p1_token("SEMI", NULL, indent + 1);
}

void ast_print_project1(ASTNode *node, int indent) {
    ASTNode *function_list;
    ASTNode *func;

    if (node == NULL) {
        return;
    }

    if (strcmp(node->name, "Program") != 0) {
        ast_print(node, indent);
        return;
    }

    p1_nonterm("Program", node, indent);

    function_list = p1_child_named(node, "FunctionList");
    if (function_list == NULL) {
        return;
    }

    p1_nonterm("ExtDefList", function_list, indent + 1);

    for (func = function_list->first_child; func != NULL; func = func->next_sibling) {
        if (strcmp(func->name, "StructDef") == 0) {
            p1_print_struct_def(func, indent + 2);
        } else {
            p1_print_function(func, indent + 2);
        }
    }
}
