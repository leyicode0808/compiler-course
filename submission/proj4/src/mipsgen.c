#include "mipsgen.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Var Var;
typedef struct Instr Instr;
typedef struct Func Func;

struct Var {
    char *name;
    int size;
    int offset;
    Var *next;
};

struct Instr {
    char *text;
    Instr *next;
};

struct Func {
    char *name;
    Instr *head;
    Instr *tail;
    Var *vars;
    char **params;
    int param_count;
    int param_cap;
    int frame_size;
    Func *next;
};

static FILE *out;

static char *xstrdup(const char *text) {
    size_t len = strlen(text);
    char *copy = malloc(len + 1);
    if (!copy) {
        perror("malloc");
        exit(1);
    }
    memcpy(copy, text, len + 1);
    return copy;
}

static char *trim(char *text) {
    while (isspace((unsigned char)*text)) {
        ++text;
    }
    char *end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) {
        *--end = '\0';
    }
    return text;
}

static int starts_with(const char *text, const char *prefix) {
    return strncmp(text, prefix, strlen(prefix)) == 0;
}

static void emit(const char *text) {
    fputs(text, out);
    fputc('\n', out);
}

static Var *find_var(Func *func, const char *name) {
    for (Var *var = func->vars; var; var = var->next) {
        if (strcmp(var->name, name) == 0) {
            return var;
        }
    }
    return NULL;
}

static Var *add_var(Func *func, const char *name, int size) {
    if (!name || !*name || name[0] == '#') {
        return NULL;
    }
    Var *var = find_var(func, name);
    if (var) {
        if (size > var->size) {
            var->size = size;
        }
        return var;
    }
    var = calloc(1, sizeof(*var));
    if (!var) {
        perror("calloc");
        exit(1);
    }
    var->name = xstrdup(name);
    var->size = size > 0 ? size : 4;
    var->next = func->vars;
    func->vars = var;
    return var;
}

static void add_param(Func *func, const char *name) {
    add_var(func, name, 4);
    if (func->param_count == func->param_cap) {
        int next_cap = func->param_cap ? func->param_cap * 2 : 4;
        char **next = realloc(func->params, (size_t)next_cap * sizeof(*next));
        if (!next) {
            perror("realloc");
            exit(1);
        }
        func->params = next;
        func->param_cap = next_cap;
    }
    func->params[func->param_count++] = xstrdup(name);
}

static void add_instr(Func *func, const char *text) {
    Instr *instr = calloc(1, sizeof(*instr));
    if (!instr) {
        perror("calloc");
        exit(1);
    }
    instr->text = xstrdup(text);
    if (!func->head) {
        func->head = instr;
    } else {
        func->tail->next = instr;
    }
    func->tail = instr;
}

static int is_ident_char(int ch) {
    return isalnum(ch) || ch == '_';
}

static void clean_operand(const char *operand, char *buf, size_t size) {
    size_t i = 0;
    while (*operand && isspace((unsigned char)*operand)) {
        ++operand;
    }
    if (*operand == '*' || *operand == '&') {
        ++operand;
    }
    while (*operand && is_ident_char((unsigned char)*operand) && i + 1 < size) {
        buf[i++] = *operand++;
    }
    buf[i] = '\0';
}

static void collect_operand(Func *func, const char *operand) {
    char buf[128];
    clean_operand(operand, buf, sizeof(buf));
    if (buf[0] && buf[0] != '#') {
        add_var(func, buf, 4);
    }
}

static int split_assign(char *line, char **lhs, char **rhs) {
    char *mark = strstr(line, " := ");
    if (!mark) {
        return 0;
    }
    *mark = '\0';
    *lhs = trim(line);
    *rhs = trim(mark + 4);
    return 1;
}

static void collect_assignment(Func *func, char *line) {
    char *lhs = NULL;
    char *rhs = NULL;
    if (!split_assign(line, &lhs, &rhs)) {
        return;
    }
    collect_operand(func, lhs);
    if (starts_with(rhs, "CALL ")) {
        return;
    }
    char a[128], op[8], b[128];
    if (sscanf(rhs, "%127s %7s %127s", a, op, b) == 3) {
        collect_operand(func, a);
        collect_operand(func, b);
        return;
    }
    collect_operand(func, rhs);
}

static void collect_vars(Func *func, const char *text) {
    char line[512];
    snprintf(line, sizeof(line), "%s", text);
    char name[128];
    int size = 0;
    if (sscanf(line, "PARAM %127s", name) == 1) {
        add_param(func, name);
        return;
    }
    if (sscanf(line, "DEC %127s %d", name, &size) == 2) {
        add_var(func, name, size);
        return;
    }
    if (sscanf(line, "READ %127s", name) == 1) {
        add_var(func, name, 4);
        return;
    }
    if (starts_with(line, "WRITE ")) {
        collect_operand(func, trim(line + 6));
        return;
    }
    if (starts_with(line, "RETURN ")) {
        collect_operand(func, trim(line + 7));
        return;
    }
    if (starts_with(line, "ARG ")) {
        collect_operand(func, trim(line + 4));
        return;
    }
    if (starts_with(line, "IF ")) {
        char a[128], relop[8], b[128], label[128];
        if (sscanf(line, "IF %127s %7s %127s GOTO %127s", a, relop, b, label) == 4) {
            collect_operand(func, a);
            collect_operand(func, b);
        }
        return;
    }
    collect_assignment(func, line);
}

static Func *parse_ir(FILE *input) {
    Func *head = NULL;
    Func *tail = NULL;
    Func *current = NULL;
    char raw[512];
    while (fgets(raw, sizeof(raw), input)) {
        char *line = trim(raw);
        if (!*line) {
            continue;
        }
        char name[128];
        if (sscanf(line, "FUNCTION %127s :", name) == 1) {
            current = calloc(1, sizeof(*current));
            if (!current) {
                perror("calloc");
                exit(1);
            }
            current->name = xstrdup(name);
            if (!head) {
                head = current;
            } else {
                tail->next = current;
            }
            tail = current;
            continue;
        }
        if (!current) {
            continue;
        }
        collect_vars(current, line);
        add_instr(current, line);
    }
    return head;
}

static void layout_frame(Func *func) {
    int cursor = -8;
    for (Var *var = func->vars; var; var = var->next) {
        int size = (var->size + 3) / 4 * 4;
        cursor -= size;
        var->offset = cursor;
    }
    int bytes = -cursor;
    func->frame_size = (bytes + 7) / 8 * 8;
    if (func->frame_size < 8) {
        func->frame_size = 8;
    }
}

static void load_operand(Func *func, const char *operand, const char *reg) {
    char token[128];
    snprintf(token, sizeof(token), "%s", operand);
    char *op = trim(token);
    if (*op == '#') {
        fprintf(out, "  li %s, %d\n", reg, atoi(op + 1));
        return;
    }
    if (*op == '&') {
        char name[128];
        clean_operand(op, name, sizeof(name));
        Var *var = find_var(func, name);
        fprintf(out, "  addi %s, $fp, %d\n", reg, var ? var->offset : 0);
        return;
    }
    if (*op == '*') {
        char name[128];
        clean_operand(op, name, sizeof(name));
        Var *var = find_var(func, name);
        fprintf(out, "  lw %s, %d($fp)\n", reg, var ? var->offset : 0);
        fprintf(out, "  lw %s, 0(%s)\n", reg, reg);
        return;
    }
    Var *var = find_var(func, op);
    fprintf(out, "  lw %s, %d($fp)\n", reg, var ? var->offset : 0);
}

static void store_var(Func *func, const char *name, const char *reg) {
    Var *var = find_var(func, name);
    fprintf(out, "  sw %s, %d($fp)\n", reg, var ? var->offset : 0);
}

static void emit_epilogue(Func *func) {
    if (strcmp(func->name, "main") == 0) {
        emit("  li $v0, 10");
        emit("  syscall");
        return;
    }
    emit("  move $sp, $fp");
    emit("  lw $ra, -4($fp)");
    emit("  lw $fp, -8($fp)");
    emit("  jr $ra");
}

static const char *branch_op(const char *relop) {
    if (strcmp(relop, "==") == 0) {
        return "beq";
    }
    if (strcmp(relop, "!=") == 0) {
        return "bne";
    }
    if (strcmp(relop, ">") == 0) {
        return "bgt";
    }
    if (strcmp(relop, "<") == 0) {
        return "blt";
    }
    if (strcmp(relop, ">=") == 0) {
        return "bge";
    }
    return "ble";
}

static void emit_assignment(Func *func, char *line, char ***pending_args, int *pending_count, int *pending_cap) {
    (void)pending_args;
    (void)pending_count;
    (void)pending_cap;
    char *lhs = NULL;
    char *rhs = NULL;
    if (!split_assign(line, &lhs, &rhs)) {
        return;
    }
    if (starts_with(rhs, "CALL ")) {
        char callee[128];
        sscanf(rhs, "CALL %127s", callee);
        fprintf(out, "  jal %s\n", callee);
        store_var(func, lhs, "$v0");
        return;
    }
    char a[128], op[8], b[128];
    if (sscanf(rhs, "%127s %7s %127s", a, op, b) == 3) {
        load_operand(func, a, "$t0");
        load_operand(func, b, "$t1");
        if (strcmp(op, "+") == 0) {
            emit("  add $t2, $t0, $t1");
        } else if (strcmp(op, "-") == 0) {
            emit("  sub $t2, $t0, $t1");
        } else if (strcmp(op, "*") == 0) {
            emit("  mul $t2, $t0, $t1");
        } else {
            emit("  div $t0, $t1");
            emit("  mflo $t2");
        }
        if (*lhs == '*') {
            load_operand(func, lhs + 1, "$t3");
            emit("  sw $t2, 0($t3)");
        } else {
            store_var(func, lhs, "$t2");
        }
        return;
    }
    if (*rhs == '*') {
        load_operand(func, rhs + 1, "$t0");
        emit("  lw $t0, 0($t0)");
    } else {
        load_operand(func, rhs, "$t0");
    }
    if (*lhs == '*') {
        load_operand(func, lhs + 1, "$t1");
        emit("  sw $t0, 0($t1)");
    } else {
        store_var(func, lhs, "$t0");
    }
}

static void add_pending_arg(char ***pending_args, int *pending_count, int *pending_cap, const char *arg) {
    if (*pending_count == *pending_cap) {
        int next_cap = *pending_cap ? *pending_cap * 2 : 4;
        char **next = realloc(*pending_args, (size_t)next_cap * sizeof(*next));
        if (!next) {
            perror("realloc");
            exit(1);
        }
        *pending_args = next;
        *pending_cap = next_cap;
    }
    (*pending_args)[(*pending_count)++] = xstrdup(arg);
}

static void emit_call_args(Func *func, char **pending_args, int pending_count) {
    for (int i = 0; i < pending_count; ++i) {
        load_operand(func, pending_args[i], "$t0");
        emit("  addi $sp, $sp, -4");
        emit("  sw $t0, 0($sp)");
    }
}

static void clear_pending_args(char **pending_args, int *pending_count) {
    for (int i = 0; i < *pending_count; ++i) {
        free(pending_args[i]);
    }
    *pending_count = 0;
}

static void emit_instr(Func *func, const char *text, char ***pending_args, int *pending_count, int *pending_cap) {
    char line[512];
    snprintf(line, sizeof(line), "%s", text);
    if (starts_with(line, "PARAM ") || starts_with(line, "DEC ")) {
        return;
    }
    char label[128];
    if (sscanf(line, "LABEL %127s :", label) == 1) {
        fprintf(out, "%s:\n", label);
        return;
    }
    if (sscanf(line, "GOTO %127s", label) == 1) {
        fprintf(out, "  j %s\n", label);
        return;
    }
    char a[128], relop[8], b[128];
    if (sscanf(line, "IF %127s %7s %127s GOTO %127s", a, relop, b, label) == 4) {
        load_operand(func, a, "$t0");
        load_operand(func, b, "$t1");
        fprintf(out, "  %s $t0, $t1, %s\n", branch_op(relop), label);
        return;
    }
    if (starts_with(line, "RETURN ")) {
        load_operand(func, trim(line + 7), "$v0");
        emit_epilogue(func);
        return;
    }
    if (starts_with(line, "READ ")) {
        char name[128];
        if (sscanf(line, "READ %127s", name) == 1) {
            emit("  li $v0, 5");
            emit("  syscall");
            store_var(func, name, "$v0");
        }
        return;
    }
    if (starts_with(line, "WRITE ")) {
        load_operand(func, trim(line + 6), "$a0");
        emit("  li $v0, 1");
        emit("  syscall");
        emit("  li $v0, 4");
        emit("  la $a0, _ret");
        emit("  syscall");
        return;
    }
    if (starts_with(line, "ARG ")) {
        add_pending_arg(pending_args, pending_count, pending_cap, trim(line + 4));
        return;
    }
    char *rhs = strstr(line, " := CALL ");
    if (rhs && *pending_count > 0) {
        emit_call_args(func, *pending_args, *pending_count);
    }
    emit_assignment(func, line, pending_args, pending_count, pending_cap);
    if (rhs && *pending_count > 0) {
        fprintf(out, "  addi $sp, $sp, %d\n", (*pending_count) * 4);
        clear_pending_args(*pending_args, pending_count);
    }
}

static void emit_func(Func *func) {
    layout_frame(func);
    fprintf(out, "%s:\n", func->name);
    fprintf(out, "  addi $sp, $sp, -%d\n", func->frame_size);
    fprintf(out, "  sw $ra, %d($sp)\n", func->frame_size - 4);
    fprintf(out, "  sw $fp, %d($sp)\n", func->frame_size - 8);
    fprintf(out, "  addi $fp, $sp, %d\n", func->frame_size);
    for (int i = 0; i < func->param_count; ++i) {
        fprintf(out, "  lw $t0, %d($fp)\n", i * 4);
        store_var(func, func->params[i], "$t0");
    }
    char **pending_args = NULL;
    int pending_count = 0;
    int pending_cap = 0;
    for (Instr *instr = func->head; instr; instr = instr->next) {
        emit_instr(func, instr->text, &pending_args, &pending_count, &pending_cap);
    }
    clear_pending_args(pending_args, &pending_count);
    free(pending_args);
}

int mips_generate_from_ir(const char *ir_path, const char *output_path) {
    FILE *input = fopen(ir_path, "r");
    if (!input) {
        perror(ir_path);
        return 1;
    }
    Func *funcs = parse_ir(input);
    fclose(input);
    out = fopen(output_path, "w");
    if (!out) {
        perror(output_path);
        return 1;
    }
    emit(".data");
    emit("_ret: .asciiz \"\\n\"");
    emit(".globl main");
    emit(".text");
    for (Func *func = funcs; func; func = func->next) {
        emit_func(func);
    }
    fclose(out);
    return 0;
}
