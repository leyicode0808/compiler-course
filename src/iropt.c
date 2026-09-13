#include "iropt.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *text;
    int deleted;
} IrLine;

typedef struct {
    IrLine *items;
    int len;
    int cap;
} LineVec;

typedef struct {
    char *name;
    char *value;
} Binding;

typedef struct {
    Binding *items;
    int len;
    int cap;
} BindVec;

typedef struct {
    char *op;
    char *left;
    char *right;
    char *result;
} ExprEntry;

typedef struct {
    ExprEntry *items;
    int len;
    int cap;
} ExprVec;

typedef struct {
    char **items;
    int len;
    int cap;
} StringSet;

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

static char *xasprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    va_list copy;
    va_copy(copy, ap);
    int len = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);
    char *buf = malloc((size_t)len + 1);
    if (!buf) {
        perror("malloc");
        exit(1);
    }
    vsnprintf(buf, (size_t)len + 1, fmt, ap);
    va_end(ap);
    return buf;
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

static void line_push(LineVec *vec, const char *text) {
    if (vec->len == vec->cap) {
        int next_cap = vec->cap ? vec->cap * 2 : 64;
        IrLine *next = realloc(vec->items, (size_t)next_cap * sizeof(*next));
        if (!next) {
            perror("realloc");
            exit(1);
        }
        vec->items = next;
        vec->cap = next_cap;
    }
    vec->items[vec->len].text = xstrdup(text);
    vec->items[vec->len].deleted = 0;
    vec->len++;
}

static void free_lines(LineVec *vec) {
    for (int i = 0; i < vec->len; ++i) {
        free(vec->items[i].text);
    }
    free(vec->items);
}

static int is_number(const char *text) {
    if (*text == '-') {
        ++text;
    }
    if (!isdigit((unsigned char)*text)) {
        return 0;
    }
    while (*text) {
        if (!isdigit((unsigned char)*text++)) {
            return 0;
        }
    }
    return 1;
}

static int is_const_operand(const char *text) {
    return text[0] == '#' && is_number(text + 1);
}

static int is_var_operand(const char *text) {
    return text[0] && text[0] != '#' && text[0] != '&' && text[0] != '*';
}

static int parse_assign(const char *line, char *lhs, char *rhs) {
    const char *mark = strstr(line, " := ");
    if (!mark) {
        return 0;
    }
    snprintf(lhs, 128, "%.*s", (int)(mark - line), line);
    snprintf(rhs, 256, "%s", mark + 4);
    return 1;
}

static int parse_binary(const char *rhs, char *left, char *op, char *right) {
    return sscanf(rhs, "%127s %7s %127s", left, op, right) == 3;
}

static int parse_simple_operand(const char *rhs, char *operand) {
    char extra[128];
    return sscanf(rhs, "%127s %127s", operand, extra) == 1;
}

static int operand_uses_var(const char *operand, const char *var) {
    if (operand[0] == '#' || operand[0] == '\0') {
        return 0;
    }
    if (operand[0] == '&' || operand[0] == '*') {
        return strcmp(operand + 1, var) == 0;
    }
    return strcmp(operand, var) == 0;
}

static int line_uses_var(const char *line, const char *var) {
    char lhs[128], rhs[256], a[128], op[8], b[128], label[128];
    if (parse_assign(line, lhs, rhs)) {
        if (lhs[0] == '*' && operand_uses_var(lhs + 1, var)) {
            return 1;
        }
        if (parse_binary(rhs, a, op, b)) {
            return operand_uses_var(a, var) || operand_uses_var(b, var);
        }
        return operand_uses_var(rhs, var);
    }
    if (sscanf(line, "IF %127s %7s %127s GOTO %127s", a, op, b, label) == 4) {
        return operand_uses_var(a, var) || operand_uses_var(b, var);
    }
    if (starts_with(line, "RETURN ")) {
        return operand_uses_var(line + 7, var);
    }
    if (starts_with(line, "WRITE ")) {
        return operand_uses_var(line + 6, var);
    }
    if (starts_with(line, "ARG ")) {
        return operand_uses_var(line + 4, var);
    }
    return 0;
}

static int line_defines_var(const char *line, const char *var) {
    char lhs[128], rhs[256], name[128];
    if (parse_assign(line, lhs, rhs) && lhs[0] != '*') {
        return strcmp(lhs, var) == 0;
    }
    if (sscanf(line, "READ %127s", name) == 1) {
        return strcmp(name, var) == 0;
    }
    return 0;
}

static int is_barrier(const char *line) {
    return starts_with(line, "FUNCTION ") || starts_with(line, "LABEL ") ||
           starts_with(line, "GOTO ") || starts_with(line, "IF ") ||
           starts_with(line, "RETURN ");
}

static void bind_clear(BindVec *vec) {
    for (int i = 0; i < vec->len; ++i) {
        free(vec->items[i].name);
        free(vec->items[i].value);
    }
    vec->len = 0;
}

static void bind_free(BindVec *vec) {
    bind_clear(vec);
    free(vec->items);
}

static void bind_reserve(BindVec *vec) {
    if (vec->len == vec->cap) {
        int next_cap = vec->cap ? vec->cap * 2 : 32;
        Binding *next = realloc(vec->items, (size_t)next_cap * sizeof(*next));
        if (!next) {
            perror("realloc");
            exit(1);
        }
        vec->items = next;
        vec->cap = next_cap;
    }
}

static const char *bind_get(BindVec *vec, const char *name) {
    for (int i = 0; i < vec->len; ++i) {
        if (strcmp(vec->items[i].name, name) == 0) {
            return vec->items[i].value;
        }
    }
    return NULL;
}

static void bind_remove_at(BindVec *vec, int idx) {
    free(vec->items[idx].name);
    free(vec->items[idx].value);
    memmove(&vec->items[idx], &vec->items[idx + 1], (size_t)(vec->len - idx - 1) * sizeof(vec->items[0]));
    vec->len--;
}

static void bind_kill(BindVec *vec, const char *name) {
    for (int i = 0; i < vec->len;) {
        if (strcmp(vec->items[i].name, name) == 0 ||
            (is_var_operand(vec->items[i].value) && strcmp(vec->items[i].value, name) == 0)) {
            bind_remove_at(vec, i);
        } else {
            ++i;
        }
    }
}

static void bind_set(BindVec *vec, const char *name, const char *value) {
    bind_kill(vec, name);
    bind_reserve(vec);
    vec->items[vec->len].name = xstrdup(name);
    vec->items[vec->len].value = xstrdup(value);
    vec->len++;
}

static void expr_clear(ExprVec *vec) {
    for (int i = 0; i < vec->len; ++i) {
        free(vec->items[i].op);
        free(vec->items[i].left);
        free(vec->items[i].right);
        free(vec->items[i].result);
    }
    vec->len = 0;
}

static void expr_free(ExprVec *vec) {
    expr_clear(vec);
    free(vec->items);
}

static void expr_remove_at(ExprVec *vec, int idx) {
    free(vec->items[idx].op);
    free(vec->items[idx].left);
    free(vec->items[idx].right);
    free(vec->items[idx].result);
    memmove(&vec->items[idx], &vec->items[idx + 1], (size_t)(vec->len - idx - 1) * sizeof(vec->items[0]));
    vec->len--;
}

static void expr_kill_var(ExprVec *vec, const char *name) {
    for (int i = 0; i < vec->len;) {
        ExprEntry *entry = &vec->items[i];
        if (strcmp(entry->result, name) == 0 || operand_uses_var(entry->left, name) ||
            operand_uses_var(entry->right, name)) {
            expr_remove_at(vec, i);
        } else {
            ++i;
        }
    }
}

static void normalize_expr(char *left, const char *op, char *right) {
    if ((strcmp(op, "+") == 0 || strcmp(op, "*") == 0) && strcmp(left, right) > 0) {
        char tmp[128];
        snprintf(tmp, sizeof(tmp), "%s", left);
        snprintf(left, 128, "%s", right);
        snprintf(right, 128, "%s", tmp);
    }
}

static const char *expr_find(ExprVec *vec, const char *left, const char *op, const char *right) {
    for (int i = 0; i < vec->len; ++i) {
        ExprEntry *entry = &vec->items[i];
        if (strcmp(entry->left, left) == 0 && strcmp(entry->op, op) == 0 &&
            strcmp(entry->right, right) == 0) {
            return entry->result;
        }
    }
    return NULL;
}

static void expr_add(ExprVec *vec, const char *left, const char *op, const char *right, const char *result) {
    if (vec->len == vec->cap) {
        int next_cap = vec->cap ? vec->cap * 2 : 32;
        ExprEntry *next = realloc(vec->items, (size_t)next_cap * sizeof(*next));
        if (!next) {
            perror("realloc");
            exit(1);
        }
        vec->items = next;
        vec->cap = next_cap;
    }
    vec->items[vec->len].left = xstrdup(left);
    vec->items[vec->len].op = xstrdup(op);
    vec->items[vec->len].right = xstrdup(right);
    vec->items[vec->len].result = xstrdup(result);
    vec->len++;
}

static void substitute_operand(BindVec *bindings, char *operand, size_t size) {
    if (!is_var_operand(operand)) {
        return;
    }
    const char *value = bind_get(bindings, operand);
    if (value && (is_const_operand(value) || is_var_operand(value))) {
        snprintf(operand, size, "%s", value);
    }
}

static int eval_binary(int lhs, const char *op, int rhs, int *result) {
    if (strcmp(op, "+") == 0) {
        *result = lhs + rhs;
    } else if (strcmp(op, "-") == 0) {
        *result = lhs - rhs;
    } else if (strcmp(op, "*") == 0) {
        *result = lhs * rhs;
    } else if (strcmp(op, "/") == 0) {
        if (rhs == 0) {
            return 0;
        }
        *result = lhs / rhs;
    } else {
        return 0;
    }
    return 1;
}

static void replace_line(IrLine *line, const char *text) {
    free(line->text);
    line->text = xstrdup(text);
}

static int rewrite_assignment(IrLine *line, BindVec *bindings, ExprVec *exprs) {
    char lhs[128], rhs[256], a[128], op[8], b[128];
    if (!parse_assign(line->text, lhs, rhs) || lhs[0] == '*') {
        return 0;
    }
    if (starts_with(rhs, "CALL ")) {
        bind_clear(bindings);
        expr_clear(exprs);
        return 0;
    }
    int changed = 0;
    if (parse_binary(rhs, a, op, b)) {
        substitute_operand(bindings, a, sizeof(a));
        substitute_operand(bindings, b, sizeof(b));
        int value = 0;
        if (is_const_operand(a) && is_const_operand(b) && eval_binary(atoi(a + 1), op, atoi(b + 1), &value)) {
            char folded_rhs[64];
            snprintf(folded_rhs, sizeof(folded_rhs), "#%d", value);
            char *next = xasprintf("%s := %s", lhs, folded_rhs);
            changed = strcmp(line->text, next) != 0;
            replace_line(line, next);
            free(next);
            bind_set(bindings, lhs, folded_rhs);
            expr_kill_var(exprs, lhs);
            return changed;
        }
        normalize_expr(a, op, b);
        const char *existing = expr_find(exprs, a, op, b);
        if (existing) {
            char *next = xasprintf("%s := %s", lhs, existing);
            changed = strcmp(line->text, next) != 0;
            replace_line(line, next);
            free(next);
            bind_set(bindings, lhs, existing);
            expr_kill_var(exprs, lhs);
            return changed;
        }
        char *next = xasprintf("%s := %s %s %s", lhs, a, op, b);
        changed = strcmp(line->text, next) != 0;
        replace_line(line, next);
        free(next);
        bind_kill(bindings, lhs);
        expr_kill_var(exprs, lhs);
        expr_add(exprs, a, op, b, lhs);
        return changed;
    }
    if (parse_simple_operand(rhs, a)) {
        substitute_operand(bindings, a, sizeof(a));
        char *next = xasprintf("%s := %s", lhs, a);
        changed = strcmp(line->text, next) != 0;
        replace_line(line, next);
        free(next);
        expr_kill_var(exprs, lhs);
        if (is_const_operand(a) || is_var_operand(a)) {
            bind_set(bindings, lhs, a);
        } else {
            bind_kill(bindings, lhs);
        }
        return changed;
    }
    return 0;
}

static int rewrite_non_assignment(IrLine *line, BindVec *bindings) {
    char a[128], op[8], b[128], label[128];
    if (sscanf(line->text, "IF %127s %7s %127s GOTO %127s", a, op, b, label) == 4) {
        substitute_operand(bindings, a, sizeof(a));
        substitute_operand(bindings, b, sizeof(b));
        char *next = xasprintf("IF %s %s %s GOTO ", a, op, b);
        char *full = malloc(strlen(next) + strlen(label) + 1);
        if (!full) {
            perror("malloc");
            exit(1);
        }
        strcpy(full, next);
        strcat(full, label);
        int changed = strcmp(line->text, full) != 0;
        replace_line(line, full);
        free(next);
        free(full);
        return changed;
    }
    const char *prefix = NULL;
    int prefix_len = 0;
    if (starts_with(line->text, "RETURN ")) {
        prefix = "RETURN ";
        prefix_len = 7;
    } else if (starts_with(line->text, "WRITE ")) {
        prefix = "WRITE ";
        prefix_len = 6;
    } else if (starts_with(line->text, "ARG ")) {
        prefix = "ARG ";
        prefix_len = 4;
    }
    if (prefix) {
        snprintf(a, sizeof(a), "%s", line->text + prefix_len);
        substitute_operand(bindings, a, sizeof(a));
        char *next = malloc(strlen(prefix) + strlen(a) + 1);
        if (!next) {
            perror("malloc");
            exit(1);
        }
        strcpy(next, prefix);
        strcat(next, a);
        int changed = strcmp(line->text, next) != 0;
        replace_line(line, next);
        free(next);
        return changed;
    }
    return 0;
}

static int rewrite_pass(LineVec *lines) {
    BindVec bindings = {0};
    ExprVec exprs = {0};
    int changed = 0;
    for (int i = 0; i < lines->len; ++i) {
        if (lines->items[i].deleted) {
            continue;
        }
        char *text = lines->items[i].text;
        if (starts_with(text, "FUNCTION ") || starts_with(text, "LABEL ")) {
            bind_clear(&bindings);
            expr_clear(&exprs);
            continue;
        }
        if (starts_with(text, "READ ")) {
            char name[128];
            if (sscanf(text, "READ %127s", name) == 1) {
                bind_kill(&bindings, name);
                expr_kill_var(&exprs, name);
            }
            continue;
        }
        if (strstr(text, " := CALL ")) {
            changed |= rewrite_assignment(&lines->items[i], &bindings, &exprs);
            continue;
        }
        if (strstr(text, " := ")) {
            changed |= rewrite_assignment(&lines->items[i], &bindings, &exprs);
            continue;
        }
        changed |= rewrite_non_assignment(&lines->items[i], &bindings);
        if (starts_with(text, "GOTO ") || starts_with(text, "IF ") || starts_with(text, "RETURN ")) {
            bind_clear(&bindings);
            expr_clear(&exprs);
        }
    }
    bind_free(&bindings);
    expr_free(&exprs);
    return changed;
}

static int block_end(LineVec *lines, int start) {
    int i = start + 1;
    while (i < lines->len && !is_barrier(lines->items[i].text)) {
        ++i;
    }
    return i;
}

static int assignment_has_side_effect(const char *line) {
    char lhs[128], rhs[256];
    if (!parse_assign(line, lhs, rhs)) {
        return 1;
    }
    return lhs[0] == '*' || starts_with(rhs, "CALL ");
}

static int can_remove_assignment(LineVec *lines, int idx) {
    char lhs[128], rhs[256];
    if (!parse_assign(lines->items[idx].text, lhs, rhs) || assignment_has_side_effect(lines->items[idx].text)) {
        return 0;
    }
    int end = block_end(lines, idx);
    for (int i = idx + 1; i < end; ++i) {
        if (lines->items[i].deleted) {
            continue;
        }
        if (line_uses_var(lines->items[i].text, lhs)) {
            return 0;
        }
        if (line_defines_var(lines->items[i].text, lhs)) {
            return 1;
        }
    }
    if (lhs[0] == 't') {
        for (int i = idx + 1; i < lines->len; ++i) {
            if (!lines->items[i].deleted && line_uses_var(lines->items[i].text, lhs)) {
                return 0;
            }
        }
        return 1;
    }
    return 0;
}

static int dce_pass(LineVec *lines) {
    int changed = 0;
    for (int i = lines->len - 1; i >= 0; --i) {
        if (!lines->items[i].deleted && can_remove_assignment(lines, i)) {
            lines->items[i].deleted = 1;
            changed = 1;
        }
    }
    return changed;
}

static void set_free(StringSet *set) {
    for (int i = 0; i < set->len; ++i) {
        free(set->items[i]);
    }
    free(set->items);
}

static void set_clear(StringSet *set) {
    for (int i = 0; i < set->len; ++i) {
        free(set->items[i]);
    }
    set->len = 0;
}

static int set_contains(const StringSet *set, const char *name) {
    for (int i = 0; i < set->len; ++i) {
        if (strcmp(set->items[i], name) == 0) {
            return 1;
        }
    }
    return 0;
}

static int set_add(StringSet *set, const char *name) {
    if (!name || !*name || name[0] == '#') {
        return 0;
    }
    if (set_contains(set, name)) {
        return 0;
    }
    if (set->len == set->cap) {
        int next_cap = set->cap ? set->cap * 2 : 16;
        char **next = realloc(set->items, (size_t)next_cap * sizeof(*next));
        if (!next) {
            perror("realloc");
            exit(1);
        }
        set->items = next;
        set->cap = next_cap;
    }
    set->items[set->len++] = xstrdup(name);
    return 1;
}

static int set_remove(StringSet *set, const char *name) {
    for (int i = 0; i < set->len; ++i) {
        if (strcmp(set->items[i], name) == 0) {
            free(set->items[i]);
            memmove(&set->items[i], &set->items[i + 1], (size_t)(set->len - i - 1) * sizeof(set->items[0]));
            set->len--;
            return 1;
        }
    }
    return 0;
}

static int set_union_into(StringSet *dst, const StringSet *src) {
    int changed = 0;
    for (int i = 0; i < src->len; ++i) {
        changed |= set_add(dst, src->items[i]);
    }
    return changed;
}

static int set_equals(const StringSet *a, const StringSet *b) {
    if (a->len != b->len) {
        return 0;
    }
    for (int i = 0; i < a->len; ++i) {
        if (!set_contains(b, a->items[i])) {
            return 0;
        }
    }
    return 1;
}

static int set_assign(StringSet *dst, const StringSet *src) {
    if (set_equals(dst, src)) {
        return 0;
    }
    set_clear(dst);
    for (int i = 0; i < src->len; ++i) {
        set_add(dst, src->items[i]);
    }
    return 1;
}

static void add_operand_var(StringSet *set, const char *operand) {
    if (!operand || !*operand || operand[0] == '#') {
        return;
    }
    if (operand[0] == '&' || operand[0] == '*') {
        set_add(set, operand + 1);
    } else {
        set_add(set, operand);
    }
}

static void collect_address_operand(StringSet *address_taken, const char *operand) {
    if (operand && operand[0] == '&') {
        set_add(address_taken, operand + 1);
    }
}

static void collect_line_use_def(const char *line, StringSet *use, StringSet *def, StringSet *address_taken) {
    char lhs[128], rhs[256], a[128], op[8], b[128], label[128], name[128];
    if (parse_assign(line, lhs, rhs)) {
        if (lhs[0] == '*') {
            add_operand_var(use, lhs + 1);
        } else {
            set_add(def, lhs);
        }
        if (starts_with(rhs, "CALL ")) {
            return;
        }
        if (parse_binary(rhs, a, op, b)) {
            add_operand_var(use, a);
            add_operand_var(use, b);
            collect_address_operand(address_taken, a);
            collect_address_operand(address_taken, b);
        } else if (parse_simple_operand(rhs, a)) {
            add_operand_var(use, a);
            collect_address_operand(address_taken, a);
        }
        return;
    }
    if (sscanf(line, "READ %127s", name) == 1) {
        set_add(def, name);
        return;
    }
    if (sscanf(line, "PARAM %127s", name) == 1) {
        set_add(def, name);
        return;
    }
    if (sscanf(line, "IF %127s %7s %127s GOTO %127s", a, op, b, label) == 4) {
        add_operand_var(use, a);
        add_operand_var(use, b);
        collect_address_operand(address_taken, a);
        collect_address_operand(address_taken, b);
        return;
    }
    if (starts_with(line, "RETURN ")) {
        snprintf(a, sizeof(a), "%s", line + 7);
    } else if (starts_with(line, "WRITE ")) {
        snprintf(a, sizeof(a), "%s", line + 6);
    } else if (starts_with(line, "ARG ")) {
        snprintf(a, sizeof(a), "%s", line + 4);
    } else {
        return;
    }
    add_operand_var(use, a);
    collect_address_operand(address_taken, a);
}

static int next_active(LineVec *lines, int idx) {
    for (int i = idx; i < lines->len; ++i) {
        if (!lines->items[i].deleted) {
            return i;
        }
    }
    return -1;
}

static int label_target(LineVec *lines, const char *label) {
    char current[128];
    for (int i = 0; i < lines->len; ++i) {
        if (!lines->items[i].deleted && sscanf(lines->items[i].text, "LABEL %127s :", current) == 1 &&
            strcmp(current, label) == 0) {
            return i;
        }
    }
    return -1;
}

static int add_fallthrough_successor(LineVec *lines, int idx, int *succs, int count) {
    int next = next_active(lines, idx + 1);
    if (next >= 0 && !starts_with(lines->items[next].text, "FUNCTION ")) {
        succs[count++] = next;
    }
    return count;
}

static int line_successors(LineVec *lines, int idx, int *succs) {
    char label[128], a[128], op[8], b[128];
    const char *line = lines->items[idx].text;
    if (starts_with(line, "RETURN ")) {
        return 0;
    }
    if (sscanf(line, "GOTO %127s", label) == 1) {
        int target = label_target(lines, label);
        if (target >= 0) {
            succs[0] = target;
            return 1;
        }
        return 0;
    }
    if (sscanf(line, "IF %127s %7s %127s GOTO %127s", a, op, b, label) == 4) {
        int count = 0;
        int target = label_target(lines, label);
        if (target >= 0) {
            succs[count++] = target;
        }
        return add_fallthrough_successor(lines, idx, succs, count);
    }
    return add_fallthrough_successor(lines, idx, succs, 0);
}

static int removable_global_assignment(const char *line, const StringSet *live_out, const StringSet *address_taken) {
    char lhs[128], rhs[256];
    if (!parse_assign(line, lhs, rhs)) {
        return 0;
    }
    if (lhs[0] == '*' || starts_with(rhs, "CALL ") || set_contains(address_taken, lhs)) {
        return 0;
    }
    return !set_contains(live_out, lhs);
}

static int global_dce_pass(LineVec *lines) {
    int n = lines->len;
    StringSet *use = calloc((size_t)n, sizeof(*use));
    StringSet *def = calloc((size_t)n, sizeof(*def));
    StringSet *live_in = calloc((size_t)n, sizeof(*live_in));
    StringSet *live_out = calloc((size_t)n, sizeof(*live_out));
    StringSet address_taken = {0};
    if (!use || !def || !live_in || !live_out) {
        perror("calloc");
        exit(1);
    }

    for (int i = 0; i < n; ++i) {
        if (!lines->items[i].deleted) {
            collect_line_use_def(lines->items[i].text, &use[i], &def[i], &address_taken);
        }
    }

    int changed = 1;
    while (changed) {
        changed = 0;
        for (int i = n - 1; i >= 0; --i) {
            if (lines->items[i].deleted) {
                continue;
            }
            StringSet next_out = {0};
            int succs[2];
            int succ_count = line_successors(lines, i, succs);
            for (int s = 0; s < succ_count; ++s) {
                set_union_into(&next_out, &live_in[succs[s]]);
            }
            changed |= set_assign(&live_out[i], &next_out);
            set_free(&next_out);

            StringSet next_in = {0};
            set_union_into(&next_in, &live_out[i]);
            for (int d = 0; d < def[i].len; ++d) {
                set_remove(&next_in, def[i].items[d]);
            }
            set_union_into(&next_in, &use[i]);
            changed |= set_assign(&live_in[i], &next_in);
            set_free(&next_in);
        }
    }

    int removed = 0;
    for (int i = 0; i < n; ++i) {
        if (!lines->items[i].deleted &&
            removable_global_assignment(lines->items[i].text, &live_out[i], &address_taken)) {
            lines->items[i].deleted = 1;
            removed = 1;
        }
    }

    for (int i = 0; i < n; ++i) {
        set_free(&use[i]);
        set_free(&def[i]);
        set_free(&live_in[i]);
        set_free(&live_out[i]);
    }
    set_free(&address_taken);
    free(use);
    free(def);
    free(live_in);
    free(live_out);
    return removed;
}

static int read_lines(const char *input_path, LineVec *lines) {
    FILE *input = fopen(input_path, "r");
    if (!input) {
        perror(input_path);
        return 1;
    }
    char raw[512];
    while (fgets(raw, sizeof(raw), input)) {
        char *line = trim(raw);
        if (*line) {
            line_push(lines, line);
        }
    }
    fclose(input);
    return 0;
}

static int write_lines(const char *output_path, LineVec *lines) {
    FILE *output = fopen(output_path, "w");
    if (!output) {
        perror(output_path);
        return 1;
    }
    for (int i = 0; i < lines->len; ++i) {
        if (!lines->items[i].deleted) {
            fputs(lines->items[i].text, output);
            fputc('\n', output);
        }
    }
    fclose(output);
    return 0;
}

int ir_optimize_file(const char *input_path, const char *output_path) {
    LineVec lines = {0};
    if (read_lines(input_path, &lines) != 0) {
        return 1;
    }
    for (int iter = 0; iter < 8; ++iter) {
        int changed = rewrite_pass(&lines);
        changed |= dce_pass(&lines);
        changed |= global_dce_pass(&lines);
        if (!changed) {
            break;
        }
    }
    int result = write_lines(output_path, &lines);
    free_lines(&lines);
    return result;
}
