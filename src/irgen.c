#include "irgen.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    IR_TY_INT,
    IR_TY_FLOAT,
    IR_TY_ARRAY,
    IR_TY_STRUCT
} IrTypeKind;

typedef struct IrType IrType;
typedef struct IrField IrField;
typedef struct IrSymbol IrSymbol;
typedef struct IrScope IrScope;
typedef struct IrFunc IrFunc;
typedef struct ArgList ArgList;

struct IrType {
    IrTypeKind kind;
    IrType *elem;
    int size;
    char *struct_name;
    IrField *fields;
};

struct IrField {
    char *name;
    IrType *type;
    int offset;
    IrField *next;
};

struct IrSymbol {
    char *name;
    IrType *type;
    int is_addr_param;
    IrSymbol *next;
};

struct IrScope {
    IrSymbol *symbols;
    IrScope *parent;
};

struct IrFunc {
    char *name;
    IrType *ret;
    IrField *params;
    IrFunc *next;
};

struct ArgList {
    char *place;
    ArgList *next;
};

static FILE *out;
static int temp_id;
static int label_id;
static IrScope *current_scope;
static IrField *struct_defs;
static IrFunc *functions;

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

static char *xasprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    va_list copy;
    va_copy(copy, ap);
    int len = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);
    if (len < 0) {
        perror("vsnprintf");
        exit(1);
    }
    char *buf = malloc((size_t)len + 1);
    if (!buf) {
        perror("malloc");
        exit(1);
    }
    vsnprintf(buf, (size_t)len + 1, fmt, ap);
    va_end(ap);
    return buf;
}

static int is_node(const AstNode *node, const char *name) {
    return node && strcmp(node->name, name) == 0;
}

static AstNode *child(const AstNode *node, size_t idx) {
    return node && idx < node->child_count ? node->children[idx] : NULL;
}

static AstNode *first_child_name(const AstNode *node, const char *name) {
    for (size_t i = 0; node && i < node->child_count; ++i) {
        if (is_node(node->children[i], name)) {
            return node->children[i];
        }
    }
    return NULL;
}

static void emit(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(out, fmt, ap);
    va_end(ap);
    fputc('\n', out);
}

static char *new_temp(void) {
    return xasprintf("t%d", ++temp_id);
}

static char *new_label(void) {
    return xasprintf("label%d", ++label_id);
}

static char *materialize_addr(char *addr) {
    if (addr && addr[0] == '&') {
        char *tmp = new_temp();
        emit("%s := %s", tmp, addr);
        return tmp;
    }
    return addr;
}

static IrType *new_type(IrTypeKind kind) {
    IrType *type = calloc(1, sizeof(*type));
    if (!type) {
        perror("calloc");
        exit(1);
    }
    type->kind = kind;
    return type;
}

static IrType *type_int(void) {
    static IrType type = { .kind = IR_TY_INT };
    return &type;
}

static IrType *type_float(void) {
    static IrType type = { .kind = IR_TY_FLOAT };
    return &type;
}

static IrType *array_of(IrType *elem, int size) {
    IrType *type = new_type(IR_TY_ARRAY);
    type->elem = elem;
    type->size = size;
    return type;
}

static IrType *struct_type(const char *name, IrField *fields) {
    IrType *type = new_type(IR_TY_STRUCT);
    type->struct_name = xstrdup(name);
    type->fields = fields;
    return type;
}

static int type_size(IrType *type) {
    if (!type) {
        return 4;
    }
    switch (type->kind) {
    case IR_TY_INT:
    case IR_TY_FLOAT:
        return 4;
    case IR_TY_ARRAY:
        return type->size * type_size(type->elem);
    case IR_TY_STRUCT: {
        int total = 0;
        for (IrField *field = type->fields; field; field = field->next) {
            total += type_size(field->type);
        }
        return total;
    }
    }
    return 4;
}

static IrField *new_field(const char *name, IrType *type) {
    IrField *field = calloc(1, sizeof(*field));
    if (!field) {
        perror("calloc");
        exit(1);
    }
    field->name = xstrdup(name);
    field->type = type;
    return field;
}

static IrField *append_field(IrField *head, IrField *field) {
    if (!head) {
        return field;
    }
    IrField *tail = head;
    while (tail->next) {
        tail = tail->next;
    }
    tail->next = field;
    return head;
}

static IrField *find_field(IrField *fields, const char *name) {
    for (IrField *field = fields; field; field = field->next) {
        if (strcmp(field->name, name) == 0) {
            return field;
        }
    }
    return NULL;
}

static IrField *find_struct_def(const char *name) {
    return find_field(struct_defs, name);
}

static void push_scope(void) {
    IrScope *scope = calloc(1, sizeof(*scope));
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

static IrSymbol *find_symbol(const char *name) {
    for (IrScope *scope = current_scope; scope; scope = scope->parent) {
        for (IrSymbol *sym = scope->symbols; sym; sym = sym->next) {
            if (strcmp(sym->name, name) == 0) {
                return sym;
            }
        }
    }
    return NULL;
}

static IrSymbol *add_symbol(const char *name, IrType *type, int is_addr_param) {
    IrSymbol *sym = calloc(1, sizeof(*sym));
    if (!sym) {
        perror("calloc");
        exit(1);
    }
    sym->name = xstrdup(name);
    sym->type = type;
    sym->is_addr_param = is_addr_param;
    sym->next = current_scope->symbols;
    current_scope->symbols = sym;
    return sym;
}

static IrFunc *find_func(const char *name) {
    for (IrFunc *func = functions; func; func = func->next) {
        if (strcmp(func->name, name) == 0) {
            return func;
        }
    }
    return NULL;
}

static void add_func(const char *name, IrType *ret, IrField *params) {
    if (find_func(name)) {
        return;
    }
    IrFunc *func = calloc(1, sizeof(*func));
    if (!func) {
        perror("calloc");
        exit(1);
    }
    func->name = xstrdup(name);
    func->ret = ret;
    func->params = params;
    func->next = functions;
    functions = func;
}

typedef struct VarInfo {
    char *name;
    int dims[16];
    int dim_count;
} VarInfo;

static void read_vardec(AstNode *var_dec, VarInfo *out_info) {
    memset(out_info, 0, sizeof(*out_info));
    AstNode *node = var_dec;
    while (node && is_node(node, "VarDec") && node->child_count == 4) {
        out_info->dims[out_info->dim_count++] = (int)ast_parse_int(child(node, 2)->value);
        node = child(node, 0);
    }
    AstNode *id = child(node, 0);
    out_info->name = id ? id->value : NULL;
    for (int i = 0; i < out_info->dim_count / 2; ++i) {
        int tmp = out_info->dims[i];
        out_info->dims[i] = out_info->dims[out_info->dim_count - 1 - i];
        out_info->dims[out_info->dim_count - 1 - i] = tmp;
    }
}

static IrType *apply_dims(IrType *base, const VarInfo *info) {
    IrType *type = base;
    for (int i = info->dim_count - 1; i >= 0; --i) {
        type = array_of(type, info->dims[i]);
    }
    return type;
}

static IrType *analyze_specifier(AstNode *specifier);
static void collect_def_list(AstNode *node, int as_fields, IrField **fields, int emit_decs);
static void gen_stmt_list(AstNode *node);
static void gen_stmt(AstNode *node);
static char *gen_exp(AstNode *node);
static char *gen_addr(AstNode *node, IrType **out_type);
static void gen_cond(AstNode *node, const char *true_label, const char *false_label);

static IrType *analyze_struct_specifier(AstNode *node) {
    if (node->child_count == 2) {
        AstNode *tag = child(node, 1);
        IrField *def = find_struct_def(child(tag, 0)->value);
        return def ? def->type : type_int();
    }

    const char *name = NULL;
    AstNode *opt_tag = first_child_name(node, "OptTag");
    if (opt_tag) {
        name = child(opt_tag, 0)->value;
    }
    IrField *fields = NULL;
    collect_def_list(first_child_name(node, "DefList"), 1, &fields, 0);
    IrType *type = struct_type(name, fields);
    if (name && !find_struct_def(name)) {
        struct_defs = append_field(struct_defs, new_field(name, type));
    }
    return type;
}

static IrType *analyze_specifier(AstNode *specifier) {
    AstNode *inner = child(specifier, 0);
    if (is_node(inner, "TYPE")) {
        return strcmp(inner->value, "int") == 0 ? type_int() : type_float();
    }
    return analyze_struct_specifier(inner);
}

static void collect_dec(AstNode *dec, IrType *base, int as_fields, IrField **fields, int emit_decs) {
    VarInfo info;
    read_vardec(child(dec, 0), &info);
    IrType *type = apply_dims(base, &info);
    if (as_fields) {
        IrField *field = new_field(info.name, type);
        int offset = 0;
        for (IrField *tail = *fields; tail; tail = tail->next) {
            offset = tail->offset + type_size(tail->type);
        }
        field->offset = offset;
        *fields = append_field(*fields, field);
        return;
    }

    add_symbol(info.name, type, 0);
    if (emit_decs && (type->kind == IR_TY_ARRAY || type->kind == IR_TY_STRUCT)) {
        emit("DEC %s %d", info.name, type_size(type));
    }
    if (dec->child_count == 3) {
        char *rhs = gen_exp(child(dec, 2));
        emit("%s := %s", info.name, rhs);
    }
}

static void collect_dec_list(AstNode *dec_list, IrType *base, int as_fields, IrField **fields, int emit_decs) {
    if (!dec_list) {
        return;
    }
    collect_dec(child(dec_list, 0), base, as_fields, fields, emit_decs);
    if (dec_list->child_count == 3) {
        collect_dec_list(child(dec_list, 2), base, as_fields, fields, emit_decs);
    }
}

static void collect_def(AstNode *def, int as_fields, IrField **fields, int emit_decs) {
    IrType *base = analyze_specifier(child(def, 0));
    collect_dec_list(first_child_name(def, "DecList"), base, as_fields, fields, emit_decs);
}

static void collect_def_list(AstNode *node, int as_fields, IrField **fields, int emit_decs) {
    if (!node) {
        return;
    }
    collect_def(child(node, 0), as_fields, fields, emit_decs);
    if (node->child_count == 2) {
        collect_def_list(child(node, 1), as_fields, fields, emit_decs);
    }
}

static IrField *collect_params(AstNode *var_list) {
    if (!var_list) {
        return NULL;
    }
    AstNode *param = child(var_list, 0);
    IrType *base = analyze_specifier(child(param, 0));
    VarInfo info;
    read_vardec(child(param, 1), &info);
    IrType *type = apply_dims(base, &info);
    IrField *head = new_field(info.name, type);
    if (var_list->child_count == 3) {
        head->next = collect_params(child(var_list, 2));
    }
    return head;
}

static void collect_function_sigs(AstNode *node) {
    if (!node) {
        return;
    }
    AstNode *ext_def = child(node, 0);
    if (ext_def) {
        IrType *ret = analyze_specifier(child(ext_def, 0));
        AstNode *fun_dec = first_child_name(ext_def, "FunDec");
        if (fun_dec) {
            IrField *params = NULL;
            if (fun_dec->child_count >= 4 && is_node(child(fun_dec, 2), "VarList")) {
                params = collect_params(child(fun_dec, 2));
            }
            add_func(child(fun_dec, 0)->value, ret, params);
        }
    }
    if (node->child_count == 2) {
        collect_function_sigs(child(node, 1));
    }
}

static IrField *nth_param(IrFunc *func, int idx) {
    IrField *param = func ? func->params : NULL;
    for (int i = 0; param && i < idx; ++i) {
        param = param->next;
    }
    return param;
}

static void gen_params(IrField *params) {
    for (IrField *param = params; param; param = param->next) {
        int is_addr = param->type->kind == IR_TY_ARRAY || param->type->kind == IR_TY_STRUCT;
        add_symbol(param->name, param->type, is_addr);
        emit("PARAM %s", param->name);
    }
}

static void gen_compst(AstNode *node, int create_scope) {
    if (create_scope) {
        push_scope();
    }
    collect_def_list(first_child_name(node, "DefList"), 0, NULL, 1);
    gen_stmt_list(first_child_name(node, "StmtList"));
    if (create_scope) {
        pop_scope();
    }
}

static void gen_ext_def(AstNode *node) {
    AstNode *fun_dec = first_child_name(node, "FunDec");
    if (!fun_dec || !first_child_name(node, "CompSt")) {
        if (!fun_dec) {
            analyze_specifier(child(node, 0));
        }
        return;
    }

    const char *name = child(fun_dec, 0)->value;
    IrFunc *func = find_func(name);
    emit("FUNCTION %s :", name);
    push_scope();
    gen_params(func ? func->params : NULL);
    gen_compst(first_child_name(node, "CompSt"), 0);
    pop_scope();
}

static void gen_ext_def_list(AstNode *node) {
    if (!node) {
        return;
    }
    gen_ext_def(child(node, 0));
    if (node->child_count == 2) {
        gen_ext_def_list(child(node, 1));
    }
}

static void gen_stmt_list(AstNode *node) {
    if (!node) {
        return;
    }
    gen_stmt(child(node, 0));
    if (node->child_count == 2) {
        gen_stmt_list(child(node, 1));
    }
}

static void gen_stmt(AstNode *node) {
    AstNode *first = child(node, 0);
    if (is_node(first, "Exp")) {
        gen_exp(first);
    } else if (is_node(first, "CompSt")) {
        gen_compst(first, 1);
    } else if (is_node(first, "RETURN")) {
        emit("RETURN %s", gen_exp(child(node, 1)));
    } else if (is_node(first, "IF")) {
        char *lt = new_label();
        char *lf = new_label();
        char *lend = new_label();
        gen_cond(child(node, 2), lt, lf);
        emit("LABEL %s :", lt);
        gen_stmt(child(node, 4));
        if (node->child_count == 7) {
            emit("GOTO %s", lend);
            emit("LABEL %s :", lf);
            gen_stmt(child(node, 6));
            emit("LABEL %s :", lend);
        } else {
            emit("LABEL %s :", lf);
        }
    } else if (is_node(first, "WHILE")) {
        char *lbegin = new_label();
        char *lbody = new_label();
        char *lend = new_label();
        emit("LABEL %s :", lbegin);
        gen_cond(child(node, 2), lbody, lend);
        emit("LABEL %s :", lbody);
        gen_stmt(child(node, 4));
        emit("GOTO %s", lbegin);
        emit("LABEL %s :", lend);
    }
}

static IrType *exp_type(AstNode *node) {
    if (!node) {
        return type_int();
    }
    if (node->child_count == 1) {
        AstNode *leaf = child(node, 0);
        if (is_node(leaf, "INT")) {
            return type_int();
        }
        if (is_node(leaf, "FLOAT")) {
            return type_float();
        }
        if (is_node(leaf, "ID")) {
            IrSymbol *sym = find_symbol(leaf->value);
            return sym ? sym->type : type_int();
        }
    }
    if (node->child_count == 2) {
        return exp_type(child(node, 1));
    }
    if (node->child_count == 3) {
        if (is_node(child(node, 0), "LP")) {
            return exp_type(child(node, 1));
        }
        if (is_node(child(node, 1), "DOT")) {
            IrType *base = exp_type(child(node, 0));
            IrField *field = base && base->kind == IR_TY_STRUCT ? find_field(base->fields, child(node, 2)->value) : NULL;
            return field ? field->type : type_int();
        }
        return type_int();
    }
    if (node->child_count == 4) {
        if (is_node(child(node, 0), "ID")) {
            IrFunc *func = find_func(child(node, 0)->value);
            return func ? func->ret : type_int();
        }
        if (is_node(child(node, 1), "LB")) {
            IrType *base = exp_type(child(node, 0));
            return base && base->kind == IR_TY_ARRAY ? base->elem : type_int();
        }
    }
    return type_int();
}

static char *gen_addr(AstNode *node, IrType **out_type) {
    if (node->child_count == 1 && is_node(child(node, 0), "ID")) {
        AstNode *id = child(node, 0);
        IrSymbol *sym = find_symbol(id->value);
        *out_type = sym ? sym->type : type_int();
        if (sym && sym->is_addr_param) {
            return xstrdup(id->value);
        }
        return xasprintf("&%s", id->value);
    }

    if (node->child_count == 4 && is_node(child(node, 1), "LB")) {
        IrType *base_type = NULL;
        char *base_addr = gen_addr(child(node, 0), &base_type);
        IrType *elem_type = base_type && base_type->kind == IR_TY_ARRAY ? base_type->elem : type_int();
        char *idx = gen_exp(child(node, 2));
        char *scaled = new_temp();
        char *addr = new_temp();
        emit("%s := %s * #%d", scaled, idx, type_size(elem_type));
        emit("%s := %s + %s", addr, base_addr, scaled);
        *out_type = elem_type;
        return addr;
    }

    if (node->child_count == 3 && is_node(child(node, 1), "DOT")) {
        IrType *base_type = NULL;
        char *base_addr = gen_addr(child(node, 0), &base_type);
        IrField *field = base_type && base_type->kind == IR_TY_STRUCT ? find_field(base_type->fields, child(node, 2)->value) : NULL;
        *out_type = field ? field->type : type_int();
        if (!field || field->offset == 0) {
            return base_addr;
        }
        char *addr = new_temp();
        emit("%s := %s + #%d", addr, base_addr, field->offset);
        return addr;
    }

    *out_type = exp_type(node);
    return gen_exp(node);
}

static ArgList *append_arg(ArgList *head, char *place) {
    ArgList *arg = calloc(1, sizeof(*arg));
    if (!arg) {
        perror("calloc");
        exit(1);
    }
    arg->place = place;
    if (!head) {
        return arg;
    }
    ArgList *tail = head;
    while (tail->next) {
        tail = tail->next;
    }
    tail->next = arg;
    return head;
}

static ArgList *collect_args(AstNode *args, IrFunc *func, int idx) {
    if (!args) {
        return NULL;
    }
    IrField *param = nth_param(func, idx);
    char *place = NULL;
    if (param && (param->type->kind == IR_TY_ARRAY || param->type->kind == IR_TY_STRUCT)) {
        IrType *dummy = NULL;
        place = gen_addr(child(args, 0), &dummy);
    } else {
        place = gen_exp(child(args, 0));
    }
    ArgList *head = append_arg(NULL, place);
    if (args->child_count == 3) {
        ArgList *rest = collect_args(child(args, 2), func, idx + 1);
        head->next = rest;
    }
    return head;
}

static void emit_args_reverse(ArgList *args) {
    if (!args) {
        return;
    }
    emit_args_reverse(args->next);
    emit("ARG %s", args->place);
}

static char *gen_call(AstNode *node) {
    const char *name = child(node, 0)->value;
    if (strcmp(name, "read") == 0) {
        char *tmp = new_temp();
        emit("READ %s", tmp);
        return tmp;
    }
    if (strcmp(name, "write") == 0) {
        char *arg = gen_exp(child(child(node, 2), 0));
        emit("WRITE %s", arg);
        return "#0";
    }
    IrFunc *func = find_func(name);
    ArgList *args = NULL;
    if (node->child_count == 4) {
        args = collect_args(child(node, 2), func, 0);
    }
    emit_args_reverse(args);
    char *tmp = new_temp();
    emit("%s := CALL %s", tmp, name);
    return tmp;
}

static char *gen_exp(AstNode *node) {
    if (node->child_count == 1) {
        AstNode *leaf = child(node, 0);
        if (is_node(leaf, "INT")) {
            return xasprintf("#%ld", ast_parse_int(leaf->value));
        }
        if (is_node(leaf, "ID")) {
            return xstrdup(leaf->value);
        }
    }
    if (node->child_count == 2) {
        char *val = gen_exp(child(node, 1));
        char *tmp = new_temp();
        if (is_node(child(node, 0), "MINUS")) {
            emit("%s := #0 - %s", tmp, val);
        } else {
            char *lt = new_label();
            char *lf = new_label();
            char *le = new_label();
            emit("%s := #0", tmp);
            gen_cond(node, lt, lf);
            emit("LABEL %s :", lt);
            emit("%s := #1", tmp);
            emit("GOTO %s", le);
            emit("LABEL %s :", lf);
            emit("LABEL %s :", le);
        }
        return tmp;
    }
    if (node->child_count == 3) {
        if (is_node(child(node, 0), "ID") && is_node(child(node, 1), "LP")) {
            return gen_call(node);
        }
        AstNode *op = child(node, 1);
        if (is_node(child(node, 0), "LP")) {
            return gen_exp(child(node, 1));
        }
        if (is_node(op, "ASSIGNOP")) {
            IrType *lhs_type = NULL;
            AstNode *lhs = child(node, 0);
            char *rhs = gen_exp(child(node, 2));
            if (lhs->child_count == 1 && is_node(child(lhs, 0), "ID")) {
                emit("%s := %s", child(lhs, 0)->value, rhs);
                return xstrdup(child(lhs, 0)->value);
            }
            char *addr = gen_addr(lhs, &lhs_type);
            addr = materialize_addr(addr);
            emit("*%s := %s", addr, rhs);
            return rhs;
        }
        if (is_node(op, "DOT")) {
            IrType *type = NULL;
            char *addr = gen_addr(node, &type);
            if (type && (type->kind == IR_TY_ARRAY || type->kind == IR_TY_STRUCT)) {
                return addr;
            }
            addr = materialize_addr(addr);
            char *tmp = new_temp();
            emit("%s := *%s", tmp, addr);
            return tmp;
        }
        if (is_node(op, "PLUS") || is_node(op, "MINUS") || is_node(op, "STAR") || is_node(op, "DIV")) {
            char *left = gen_exp(child(node, 0));
            char *right = gen_exp(child(node, 2));
            char *tmp = new_temp();
            const char *symbol = is_node(op, "PLUS") ? "+" : is_node(op, "MINUS") ? "-" : is_node(op, "STAR") ? "*" : "/";
            emit("%s := %s %s %s", tmp, left, symbol, right);
            return tmp;
        }
        char *tmp = new_temp();
        char *lt = new_label();
        char *lf = new_label();
        char *le = new_label();
        emit("%s := #0", tmp);
        gen_cond(node, lt, lf);
        emit("LABEL %s :", lt);
        emit("%s := #1", tmp);
        emit("GOTO %s", le);
        emit("LABEL %s :", lf);
        emit("LABEL %s :", le);
        return tmp;
    }
    if (node->child_count == 4) {
        if (is_node(child(node, 0), "ID")) {
            return gen_call(node);
        }
        if (is_node(child(node, 1), "LB")) {
            IrType *type = NULL;
            char *addr = gen_addr(node, &type);
            if (type && (type->kind == IR_TY_ARRAY || type->kind == IR_TY_STRUCT)) {
                return addr;
            }
            addr = materialize_addr(addr);
            char *tmp = new_temp();
            emit("%s := *%s", tmp, addr);
            return tmp;
        }
    }
    return "#0";
}

static void gen_cond(AstNode *node, const char *true_label, const char *false_label) {
    if (node->child_count == 2 && is_node(child(node, 0), "NOT")) {
        gen_cond(child(node, 1), false_label, true_label);
        return;
    }
    if (node->child_count == 3) {
        AstNode *op = child(node, 1);
        if (is_node(op, "AND")) {
            char *mid = new_label();
            gen_cond(child(node, 0), mid, false_label);
            emit("LABEL %s :", mid);
            gen_cond(child(node, 2), true_label, false_label);
            return;
        }
        if (is_node(op, "OR")) {
            char *mid = new_label();
            gen_cond(child(node, 0), true_label, mid);
            emit("LABEL %s :", mid);
            gen_cond(child(node, 2), true_label, false_label);
            return;
        }
        if (is_node(op, "RELOP")) {
            char *left = gen_exp(child(node, 0));
            char *right = gen_exp(child(node, 2));
            emit("IF %s %s %s GOTO %s", left, op->value, right, true_label);
            emit("GOTO %s", false_label);
            return;
        }
    }
    char *val = gen_exp(node);
    emit("IF %s != #0 GOTO %s", val, true_label);
    emit("GOTO %s", false_label);
}

int ir_generate(AstNode *root, const char *output_path) {
    out = fopen(output_path, "w");
    if (!out) {
        perror(output_path);
        return 1;
    }
    temp_id = 0;
    label_id = 0;
    current_scope = NULL;
    struct_defs = NULL;
    functions = NULL;
    push_scope();
    AstNode *ext_def_list = root && root->child_count > 0 ? child(root, 0) : NULL;
    collect_function_sigs(ext_def_list);
    gen_ext_def_list(ext_def_list);
    pop_scope();
    fclose(out);
    return 0;
}
