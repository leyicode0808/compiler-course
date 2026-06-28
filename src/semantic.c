#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantic.h"

typedef struct Symbol {
    char *name;
    int line;
    struct Symbol *next;
} Symbol;

static Symbol *symbols = NULL;
static int error_count = 0;

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

static int is_node(ASTNode *node, const char *name) {
    return node != NULL && node->name != NULL && strcmp(node->name, name) == 0;
}

static Symbol *find_symbol(const char *name) {
    Symbol *symbol = symbols;

    while (symbol != NULL) {
        if (strcmp(symbol->name, name) == 0) {
            return symbol;
        }
        symbol = symbol->next;
    }

    return NULL;
}

static void add_symbol(const char *name, int line) {
    Symbol *symbol;

    if (find_symbol(name) != NULL) {
        fprintf(stderr,
                "Semantic error at line %d: redefined variable: %s\n",
                line, name);
        error_count++;
        return;
    }

    symbol = malloc(sizeof(Symbol));
    if (symbol == NULL) {
        fprintf(stderr, "fatal: out of memory\n");
        exit(1);
    }

    symbol->name = copy_string(name);
    symbol->line = line;
    symbol->next = symbols;
    symbols = symbol;
}

static void check_symbol_used(const char *name, int line) {
    if (find_symbol(name) == NULL) {
        fprintf(stderr,
                "Semantic error at line %d: undefined variable: %s\n",
                line, name);
        error_count++;
    }
}

static void clear_symbols(void) {
    Symbol *symbol = symbols;

    while (symbol != NULL) {
        Symbol *next = symbol->next;
        free(symbol->name);
        free(symbol);
        symbol = next;
    }

    symbols = NULL;
}

static void collect_declarations(ASTNode *node) {
    ASTNode *child;

    if (node == NULL) {
        return;
    }

    if (is_node(node, "VarDecl") ||
    is_node(node, "ArrayDecl") ||
    is_node(node, "Param")) {
    add_symbol(node->value, node->line);
    return;
    }

    child = node->first_child;
    while (child != NULL) {
        collect_declarations(child);
        child = child->next_sibling;
    }
}

static void check_uses(ASTNode *node) {
    ASTNode *child;

    if (node == NULL) {
        return;
    }

    if (is_node(node, "Var") || is_node(node, "ArrayAccess")) {
        check_symbol_used(node->value, node->line);
    }

    child = node->first_child;
    while (child != NULL) {
        check_uses(child);
        child = child->next_sibling;
    }
}

static void analyze_function(ASTNode *function) {
    ASTNode *child;

    clear_symbols();

    child = function->first_child;
    while (child != NULL) {
        if (is_node(child, "Params")) {
            collect_declarations(child);
        }
        if (is_node(child, "Compound")) {
            collect_declarations(child);
            check_uses(child);
        }
        child = child->next_sibling;
    }

    clear_symbols();
}

static void visit(ASTNode *node) {
    ASTNode *child;

    if (node == NULL) {
        return;
    }

    if (is_node(node, "Function")) {
        analyze_function(node);
        return;
    }

    child = node->first_child;
    while (child != NULL) {
        visit(child);
        child = child->next_sibling;
    }
}

int semantic_analyze(ASTNode *root) {
    error_count = 0;
    symbols = NULL;

    visit(root);

    clear_symbols();

    return error_count;
}
