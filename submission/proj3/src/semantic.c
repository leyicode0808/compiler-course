#include "semantic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    TY_ERROR,
    TY_INT,
    TY_FLOAT,
    TY_ARRAY,
    TY_STRUCT,
    TY_FUNC
} TypeKind;

typedef struct Type Type;
typedef struct Field Field;
typedef struct Symbol Symbol;
typedef struct Scope Scope;
typedef struct PendingError PendingError;

struct Type {
    TypeKind kind;
    Type *elem;
    int size;
    char *struct_name;
    Field *fields;
    Type *ret;
    Field *params;
    int param_count;
    int defined;
    int declared_line;
};

struct Field {
    char *name;
    Type *type;
    int line;
    Field *next;
};

struct Symbol {
    char *name;
    Type *type;
    int line;
    Symbol *next;
};

struct Scope {
    Symbol *symbols;
    Scope *parent;
};

struct PendingError {
    int type;
    int line;
    const char *msg;
    PendingError *next;
};

static Scope *current_scope;
static Symbol *global_functions;
static Field *struct_defs;
static PendingError *pending_errors;
static int semantic_errors;
static int no_scope_conflicts;

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

static int is_node(const AstNode *node, const char *name) {
    return node && strcmp(node->name, name) == 0;
}

static AstNode *child(const AstNode *node, size_t idx) {
    return node && idx < node->child_count ? node->children[idx] : NULL;
}

static int has_child_name(const AstNode *node, const char *name) {
    for (size_t i = 0; node && i < node->child_count; ++i) {
        if (is_node(node->children[i], name)) {
            return 1;
        }
    }
    return 0;
}

static AstNode *first_child_name(const AstNode *node, const char *name) {
    for (size_t i = 0; node && i < node->child_count; ++i) {
        if (is_node(node->children[i], name)) {
            return node->children[i];
        }
    }
    return NULL;
}

static void report_error(int type, int line, const char *msg) {
    printf("Error type %d at Line %d: %s.\n", type, line, msg);
    semantic_errors++;
}

static void queue_error(int type, int line, const char *msg) {
    PendingError *err = calloc(1, sizeof(*err));
    if (!err) {
        perror("calloc");
        exit(1);
    }
    err->type = type;
    err->line = line;
    err->msg = msg;
    if (!pending_errors) {
        pending_errors = err;
        return;
    }
    PendingError *tail = pending_errors;
    while (tail->next) {
        tail = tail->next;
    }
    tail->next = err;
}

static void flush_pending_errors(void) {
    for (PendingError *err = pending_errors; err; err = err->next) {
        report_error(err->type, err->line, err->msg);
    }
}

static Type *new_type(TypeKind kind) {
    Type *type = calloc(1, sizeof(*type));
    if (!type) {
        perror("calloc");
        exit(1);
    }
    type->kind = kind;
    return type;
}

static Type *type_int(void) {
    static Type type = { .kind = TY_INT };
    return &type;
}

static Type *type_float(void) {
    static Type type = { .kind = TY_FLOAT };
    return &type;
}

static Type *type_error(void) {
    static Type type = { .kind = TY_ERROR };
    return &type;
}

static Type *array_of(Type *elem, int size) {
    Type *type = new_type(TY_ARRAY);
    type->elem = elem;
    type->size = size;
    return type;
}

static Type *struct_type(const char *name, Field *fields) {
    Type *type = new_type(TY_STRUCT);
    type->struct_name = xstrdup(name);
    type->fields = fields;
    return type;
}

static Type *func_type(Type *ret, Field *params, int param_count, int defined, int line) {
    Type *type = new_type(TY_FUNC);
    type->ret = ret;
    type->params = params;
    type->param_count = param_count;
    type->defined = defined;
    type->declared_line = line;
    return type;
}

static int type_equal(const Type *a, const Type *b);

static int field_list_equal(const Field *a, const Field *b) {
    while (a && b) {
        if (!type_equal(a->type, b->type)) {
            return 0;
        }
        a = a->next;
        b = b->next;
    }
    return !a && !b;
}

static int type_equal(const Type *a, const Type *b) {
    if (!a || !b || a->kind == TY_ERROR || b->kind == TY_ERROR) {
        return 1;
    }
    if (a->kind != b->kind) {
        return 0;
    }
    switch (a->kind) {
    case TY_INT:
    case TY_FLOAT:
        return 1;
    case TY_ARRAY:
        return a->size == b->size && type_equal(a->elem, b->elem);
    case TY_STRUCT:
        if (a->struct_name && b->struct_name) {
            return strcmp(a->struct_name, b->struct_name) == 0;
        }
        return field_list_equal(a->fields, b->fields);
    case TY_FUNC:
        return type_equal(a->ret, b->ret) && field_list_equal(a->params, b->params);
    case TY_ERROR:
        return 1;
    }
    return 0;
}

static int is_numeric(const Type *type) {
    return type && (type->kind == TY_INT || type->kind == TY_FLOAT || type->kind == TY_ERROR);
}

static void push_scope(void) {
    Scope *scope = calloc(1, sizeof(*scope));
    if (!scope) {
        perror("calloc");
        exit(1);
    }
    scope->parent = current_scope;
    current_scope = scope;
}

static void pop_scope(void) {
    if (current_scope) {
        current_scope = current_scope->parent;
    }
}

static Symbol *find_in_scope(Scope *scope, const char *name) {
    for (Symbol *sym = scope ? scope->symbols : NULL; sym; sym = sym->next) {
        if (strcmp(sym->name, name) == 0) {
            return sym;
        }
    }
    return NULL;
}

static Symbol *find_symbol(const char *name) {
    for (Scope *scope = current_scope; scope; scope = scope->parent) {
        Symbol *sym = find_in_scope(scope, name);
        if (sym) {
            return sym;
        }
    }
    return NULL;
}

static Symbol *find_function(const char *name) {
    for (Symbol *sym = global_functions; sym; sym = sym->next) {
        if (strcmp(sym->name, name) == 0) {
            return sym;
        }
    }
    return NULL;
}

static int add_symbol_current(const char *name, Type *type, int line) {
    if (find_in_scope(current_scope, name)) {
        report_error(3, line, "Redefined Variable");
        return 0;
    }
    if (no_scope_conflicts) {
        for (Scope *scope = current_scope ? current_scope->parent : NULL; scope; scope = scope->parent) {
            if (find_in_scope(scope, name)) {
                report_error(3, line, "Redefined variable or structure conflict");
                return 0;
            }
        }
    }
    Symbol *sym = calloc(1, sizeof(*sym));
    if (!sym) {
        perror("calloc");
        exit(1);
    }
    sym->name = xstrdup(name);
    sym->type = type;
    sym->line = line;
    sym->next = current_scope->symbols;
    current_scope->symbols = sym;
    return 1;
}

static Field *find_field(Field *fields, const char *name) {
    for (Field *field = fields; field; field = field->next) {
        if (strcmp(field->name, name) == 0) {
            return field;
        }
    }
    return NULL;
}

static Field *append_field(Field *head, Field *field) {
    if (!head) {
        return field;
    }
    Field *tail = head;
    while (tail->next) {
        tail = tail->next;
    }
    tail->next = field;
    return head;
}

static Field *new_field(const char *name, Type *type, int line) {
    Field *field = calloc(1, sizeof(*field));
    if (!field) {
        perror("calloc");
        exit(1);
    }
    field->name = xstrdup(name);
    field->type = type;
    field->line = line;
    return field;
}

static Field *find_struct_def(const char *name) {
    return find_field(struct_defs, name);
}

static void add_struct_def(const char *name, Type *type, int line) {
    if (!name) {
        return;
    }
    if (find_struct_def(name)) {
        report_error(16, line, "Duplicated name");
        return;
    }
    struct_defs = append_field(struct_defs, new_field(name, type, line));
}

typedef struct VarInfo {
    char *name;
    int line;
    int dims[16];
    int dim_count;
} VarInfo;

static void read_vardec(AstNode *var_dec, VarInfo *out) {
    memset(out, 0, sizeof(*out));
    AstNode *node = var_dec;
    while (node && is_node(node, "VarDec") && node->child_count == 4) {
        AstNode *int_node = child(node, 2);
        out->dims[out->dim_count++] = (int)ast_parse_int(int_node->value);
        node = child(node, 0);
    }
    AstNode *id = child(node, 0);
    if (id && is_node(id, "ID")) {
        out->name = id->value;
        out->line = id->line;
    }
    for (int i = 0; i < out->dim_count / 2; ++i) {
        int tmp = out->dims[i];
        out->dims[i] = out->dims[out->dim_count - 1 - i];
        out->dims[out->dim_count - 1 - i] = tmp;
    }
}

static Type *apply_dims(Type *base, const VarInfo *info) {
    Type *type = base;
    for (int i = info->dim_count - 1; i >= 0; --i) {
        type = array_of(type, info->dims[i]);
    }
    return type;
}

static Type *analyze_specifier(AstNode *specifier);
static void analyze_ext_def(AstNode *node);
static void analyze_compst(AstNode *node, Type *return_type, int create_scope);
static void analyze_def_list(AstNode *node, int as_fields, Field **fields);
static void analyze_stmt_list(AstNode *node, Type *return_type);
static Type *analyze_exp(AstNode *node, int *is_lvalue);

static Type *analyze_struct_specifier(AstNode *node) {
    if (node->child_count == 2) {
        AstNode *tag = child(node, 1);
        AstNode *id = child(tag, 0);
        Field *def = find_struct_def(id->value);
        if (!def) {
            report_error(17, id->line, "Undefined structure3");
            return type_error();
        }
        return def->type;
    }

    const char *name = NULL;
    int line = child(node, 0)->line;
    AstNode *opt_tag = first_child_name(node, "OptTag");
    if (opt_tag) {
        AstNode *id = child(opt_tag, 0);
        name = id->value;
        line = id->line;
    }

    Field *fields = NULL;
    AstNode *def_list = first_child_name(node, "DefList");
    if (def_list) {
        analyze_def_list(def_list, 1, &fields);
    }
    Type *type = struct_type(name, fields);
    add_struct_def(name, type, line);
    return type;
}

static Type *analyze_specifier(AstNode *specifier) {
    AstNode *inner = child(specifier, 0);
    if (is_node(inner, "TYPE")) {
        return strcmp(inner->value, "int") == 0 ? type_int() : type_float();
    }
    return analyze_struct_specifier(inner);
}

static void collect_dec(AstNode *dec, Type *base, int as_fields, Field **fields) {
    AstNode *var_dec = child(dec, 0);
    VarInfo info;
    read_vardec(var_dec, &info);
    Type *type = apply_dims(base, &info);
    if (as_fields) {
        if (dec->child_count == 3) {
            report_error(15, child(dec, 1)->line, "Field can't be inited");
        }
        if (find_field(*fields, info.name)) {
            report_error(15, info.line, "Redefined field");
            return;
        }
        *fields = append_field(*fields, new_field(info.name, type, info.line));
        return;
    }

    add_symbol_current(info.name, type, info.line);
    if (dec->child_count == 3) {
        int dummy = 0;
        Type *rhs = analyze_exp(child(dec, 2), &dummy);
        if (!type_equal(type, rhs)) {
            report_error(5, child(dec, 1)->line, "Type mismatched for assignment");
        }
    }
}

static void collect_dec_list(AstNode *dec_list, Type *base, int as_fields, Field **fields) {
    if (!dec_list) {
        return;
    }
    collect_dec(child(dec_list, 0), base, as_fields, fields);
    if (dec_list->child_count == 3) {
        collect_dec_list(child(dec_list, 2), base, as_fields, fields);
    }
}

static void analyze_def(AstNode *def, int as_fields, Field **fields) {
    Type *base = analyze_specifier(child(def, 0));
    AstNode *dec_list = first_child_name(def, "DecList");
    collect_dec_list(dec_list, base, as_fields, fields);
}

static void analyze_def_list(AstNode *node, int as_fields, Field **fields) {
    if (!node) {
        return;
    }
    analyze_def(child(node, 0), as_fields, fields);
    if (node->child_count == 2) {
        analyze_def_list(child(node, 1), as_fields, fields);
    }
}

static Field *collect_params(AstNode *var_list, int *count) {
    if (!var_list) {
        return NULL;
    }
    AstNode *param = child(var_list, 0);
    Type *base = analyze_specifier(child(param, 0));
    VarInfo info;
    read_vardec(child(param, 1), &info);
    Type *type = apply_dims(base, &info);
    Field *head = new_field(info.name, type, info.line);
    (*count)++;
    if (var_list->child_count == 3) {
        head->next = collect_params(child(var_list, 2), count);
    }
    return head;
}

static void add_params_to_scope(Field *params) {
    for (Field *param = params; param; param = param->next) {
        add_symbol_current(param->name, param->type, param->line);
    }
}

static void handle_function(AstNode *ext_def, Type *ret, int is_definition) {
    AstNode *fun_dec = first_child_name(ext_def, "FunDec");
    AstNode *id = child(fun_dec, 0);
    int param_count = 0;
    Field *params = NULL;
    if (fun_dec->child_count >= 4 && is_node(child(fun_dec, 2), "VarList")) {
        params = collect_params(child(fun_dec, 2), &param_count);
    }
    Type *func = func_type(ret, params, param_count, is_definition, id->line);
    Symbol *old = find_function(id->value);
    if (old) {
        if (!type_equal(old->type, func)) {
            if (is_definition) {
                report_error(4, id->line, "Redefined Function");
            } else {
                queue_error(19, id->line, "Inconsistent declaration");
            }
        } else if (is_definition && old->type->defined) {
            report_error(4, id->line, "Redefined Function");
        } else if (is_definition) {
            old->type->defined = 1;
        }
    } else {
        Symbol *sym = calloc(1, sizeof(*sym));
        if (!sym) {
            perror("calloc");
            exit(1);
        }
        sym->name = xstrdup(id->value);
        sym->type = func;
        sym->line = id->line;
        sym->next = global_functions;
        global_functions = sym;
    }

    if (is_definition) {
        push_scope();
        add_params_to_scope(params);
        analyze_compst(first_child_name(ext_def, "CompSt"), ret, 0);
        pop_scope();
    }
}

static void handle_ext_decl_list(AstNode *node, Type *base) {
    AstNode *var_dec = child(node, 0);
    VarInfo info;
    read_vardec(var_dec, &info);
    Type *type = apply_dims(base, &info);
    add_symbol_current(info.name, type, info.line);
    if (node->child_count == 3) {
        handle_ext_decl_list(child(node, 2), base);
    }
}

static void analyze_ext_def(AstNode *node) {
    if (!node) {
        return;
    }
    Type *base = analyze_specifier(child(node, 0));
    if (first_child_name(node, "FunDec")) {
        handle_function(node, base, has_child_name(node, "CompSt"));
    } else if (first_child_name(node, "ExtDecList")) {
        handle_ext_decl_list(first_child_name(node, "ExtDecList"), base);
    }
}

static void analyze_ext_def_list(AstNode *node) {
    if (!node) {
        return;
    }
    analyze_ext_def(child(node, 0));
    if (node->child_count == 2) {
        analyze_ext_def_list(child(node, 1));
    }
}

static void analyze_compst(AstNode *node, Type *return_type, int create_scope) {
    if (!node) {
        return;
    }
    if (create_scope) {
        push_scope();
    }
    AstNode *def_list = first_child_name(node, "DefList");
    AstNode *stmt_list = first_child_name(node, "StmtList");
    analyze_def_list(def_list, 0, NULL);
    analyze_stmt_list(stmt_list, return_type);
    if (create_scope) {
        pop_scope();
    }
}

static void analyze_stmt(AstNode *node, Type *return_type) {
    AstNode *first = child(node, 0);
    if (is_node(first, "Exp")) {
        int dummy = 0;
        analyze_exp(first, &dummy);
    } else if (is_node(first, "CompSt")) {
        analyze_compst(first, return_type, 1);
    } else if (is_node(first, "RETURN")) {
        int dummy = 0;
        Type *actual = analyze_exp(child(node, 1), &dummy);
        if (!type_equal(return_type, actual)) {
            report_error(8, first->line, "Type mismatched for return");
        }
    } else if (is_node(first, "IF")) {
        int dummy = 0;
        analyze_exp(child(node, 2), &dummy);
        analyze_stmt(child(node, 4), return_type);
        if (node->child_count == 7) {
            analyze_stmt(child(node, 6), return_type);
        }
    } else if (is_node(first, "WHILE")) {
        int dummy = 0;
        analyze_exp(child(node, 2), &dummy);
        analyze_stmt(child(node, 4), return_type);
    }
}

static void analyze_stmt_list(AstNode *node, Type *return_type) {
    if (!node) {
        return;
    }
    analyze_stmt(child(node, 0), return_type);
    if (node->child_count == 2) {
        analyze_stmt_list(child(node, 1), return_type);
    }
}

static Field *args_from_node(AstNode *args, int *count) {
    if (!args) {
        return NULL;
    }
    int lvalue = 0;
    Type *type = analyze_exp(child(args, 0), &lvalue);
    Field *head = new_field("", type, child(args, 0)->line);
    (*count)++;
    if (args->child_count == 3) {
        head->next = args_from_node(child(args, 2), count);
    }
    return head;
}

static Type *binary_numeric(AstNode *node, int *is_lvalue) {
    int l1 = 0, l2 = 0;
    Type *left = analyze_exp(child(node, 0), &l1);
    Type *right = analyze_exp(child(node, 2), &l2);
    *is_lvalue = 0;
    if (!is_numeric(left) || !is_numeric(right) || !type_equal(left, right)) {
        report_error(7, child(node, 1)->line, "Type mismatched for operands");
        return type_error();
    }
    if (is_node(child(node, 1), "RELOP") || is_node(child(node, 1), "AND") || is_node(child(node, 1), "OR")) {
        return type_int();
    }
    return left;
}

static Type *analyze_exp(AstNode *node, int *is_lvalue) {
    *is_lvalue = 0;
    if (node->child_count == 1) {
        AstNode *leaf = child(node, 0);
        if (is_node(leaf, "INT")) {
            return type_int();
        }
        if (is_node(leaf, "FLOAT")) {
            return type_float();
        }
        if (is_node(leaf, "ID")) {
            Symbol *sym = find_symbol(leaf->value);
            if (!sym) {
                report_error(1, leaf->line, "Undefined Variable");
                return type_error();
            }
            *is_lvalue = 1;
            return sym->type;
        }
    }

    if (node->child_count == 2) {
        int lv = 0;
        Type *type = analyze_exp(child(node, 1), &lv);
        if (!is_numeric(type)) {
            report_error(7, child(node, 0)->line, "Type mismatched for operands");
            return type_error();
        }
        return type;
    }

    if (node->child_count == 3) {
        if (is_node(child(node, 0), "ID") && is_node(child(node, 1), "LP")) {
            AstNode *id = child(node, 0);
            Symbol *func = find_function(id->value);
            Symbol *var = find_symbol(id->value);
            if (!func) {
                report_error(var ? 11 : 2, id->line, var ? "Not a function" : "Undefined function");
                return type_error();
            }
            if (func->type->param_count != 0) {
                report_error(9, id->line, "Function is not applicable for arguments");
                return type_error();
            }
            return func->type->ret;
        }
        AstNode *op = child(node, 1);
        if (is_node(child(node, 0), "LP")) {
            return analyze_exp(child(node, 1), is_lvalue);
        }
        if (is_node(op, "ASSIGNOP")) {
            int left_lvalue = 0, right_lvalue = 0;
            Type *left = analyze_exp(child(node, 0), &left_lvalue);
            Type *right = analyze_exp(child(node, 2), &right_lvalue);
            if (!left_lvalue) {
                report_error(6, op->line, "The left-hand side of an assignment must be a variable");
            } else if (left->kind != TY_ERROR && right->kind != TY_ERROR && !type_equal(left, right)) {
                report_error(5, op->line, "Type mismatched for assignment");
            }
            return left;
        }
        if (is_node(op, "DOT")) {
            int lv = 0;
            Type *base = analyze_exp(child(node, 0), &lv);
            AstNode *field_id = child(node, 2);
            if (base->kind != TY_STRUCT) {
                report_error(13, op->line, "Illegal use of \".\"");
                *is_lvalue = 1;
                return type_error();
            }
            Field *field = find_field(base->fields, field_id->value);
            if (!field) {
                report_error(14, field_id->line, "Not-existen field");
                return type_error();
            }
            *is_lvalue = 1;
            return field->type;
        }
        return binary_numeric(node, is_lvalue);
    }

    if (node->child_count == 4) {
        if (is_node(child(node, 0), "ID")) {
            AstNode *id = child(node, 0);
            Symbol *func = find_function(id->value);
            Symbol *var = find_symbol(id->value);
            if (!func) {
                report_error(var ? 11 : 2, id->line, var ? "Not a function" : "Undefined function");
                return type_error();
            }
            int arg_count = 0;
            Field *args = args_from_node(child(node, 2), &arg_count);
            if (arg_count != func->type->param_count || !field_list_equal(args, func->type->params)) {
                report_error(9, id->line, "Function is not applicable for arguments");
                return type_error();
            }
            return func->type->ret;
        }
        if (is_node(child(node, 1), "LB")) {
            int lv = 0, idx_lv = 0;
            Type *base = analyze_exp(child(node, 0), &lv);
            Type *idx = analyze_exp(child(node, 2), &idx_lv);
            if (base->kind != TY_ARRAY) {
                report_error(10, child(node, 1)->line, "Not an array");
                *is_lvalue = 1;
                return type_error();
            }
            if (idx->kind != TY_INT && idx->kind != TY_ERROR) {
                report_error(12, child(node, 2)->line, "Not an integer");
            }
            *is_lvalue = 1;
            return base->elem;
        }
    }

    return type_error();
}

static void check_undefined_functions(void) {
    for (Symbol *func = global_functions; func; func = func->next) {
        if (!func->type->defined) {
            report_error(18, func->line, "Undefined function");
        }
    }
}

static void add_builtin_functions(void) {
    Symbol *read_sym = calloc(1, sizeof(*read_sym));
    Symbol *write_sym = calloc(1, sizeof(*write_sym));
    if (!read_sym || !write_sym) {
        perror("calloc");
        exit(1);
    }
    read_sym->name = xstrdup("read");
    read_sym->type = func_type(type_int(), NULL, 0, 1, 0);
    read_sym->next = global_functions;
    global_functions = read_sym;

    Field *write_param = new_field("value", type_int(), 0);
    write_sym->name = xstrdup("write");
    write_sym->type = func_type(type_int(), write_param, 1, 1, 0);
    write_sym->next = global_functions;
    global_functions = write_sym;
}

int semantic_check(AstNode *root) {
    semantic_errors = 0;
    current_scope = NULL;
    global_functions = NULL;
    struct_defs = NULL;
    pending_errors = NULL;
    no_scope_conflicts = getenv("CMM_NO_SCOPE") != NULL;
    push_scope();
    add_builtin_functions();
    if (root && root->child_count > 0) {
        analyze_ext_def_list(child(root, 0));
    }
    check_undefined_functions();
    flush_pending_errors();
    pop_scope();
    return semantic_errors;
}
