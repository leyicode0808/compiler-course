#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantic.h"

typedef struct Symbol {
    char *name;
    int line;
    struct Symbol *next;
} Symbol;

typedef struct FunctionSymbol {
    char *name;
    int line;
    int param_count;
    struct FunctionSymbol *next;
} FunctionSymbol;

static Symbol *symbols = NULL;
static FunctionSymbol *functions = NULL;
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

static FunctionSymbol *find_function(const char *name) {
    FunctionSymbol *function = functions;

    while (function != NULL) {
        if (strcmp(function->name, name) == 0) {
            return function;
        }
        function = function->next;
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

static void add_function(const char *name, int line, int param_count) {
    FunctionSymbol *function;

    if (find_function(name) != NULL) {
        fprintf(stderr,
                "Semantic error at line %d: redefined function: %s\n",
                line, name);
        error_count++;
        return;
    }

    function = malloc(sizeof(FunctionSymbol));
    if (function == NULL) {
        fprintf(stderr, "fatal: out of memory\n");
        exit(1);
    }

    function->name = copy_string(name);
    function->line = line;
    function->param_count = param_count;
    function->next = functions;
    functions = function;
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

static void clear_functions(void) {
    FunctionSymbol *function = functions;

    while (function != NULL) {
        FunctionSymbol *next = function->next;
        free(function->name);
        free(function);
        function = next;
    }

    functions = NULL;
}

static int count_children(ASTNode *node) {
    int count = 0;
    ASTNode *child;

    if (node == NULL) {
        return 0;
    }

    child = node->first_child;
    while (child != NULL) {
        count++;
        child = child->next_sibling;
    }

    return count;
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

static void collect_functions(ASTNode *node) {
    ASTNode *child;

    if (node == NULL) {
        return;
    }

    if (is_node(node, "Function")) {
        ASTNode *params = find_child(node, "Params");
        add_function(node->value, node->line, count_children(params));
        return;
    }

    child = node->first_child;
    while (child != NULL) {
        collect_functions(child);
        child = child->next_sibling;
    }
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

static void check_call(ASTNode *node) {
    FunctionSymbol *function;
    ASTNode *args;
    int arg_count;

    function = find_function(node->value);
    if (function == NULL) {
        fprintf(stderr,
                "Semantic error at line %d: undefined function: %s\n",
                node->line, node->value);
        error_count++;
        return;
    }

    args = find_child(node, "Args");
    arg_count = count_children(args);

    if (arg_count != function->param_count) {
        fprintf(stderr,
                "Semantic error at line %d: argument count mismatch in call to %s\n",
                node->line, node->value);
        error_count++;
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

    if (is_node(node, "Call")) {
        check_call(node);
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

static void visit_functions(ASTNode *node) {
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
        visit_functions(child);
        child = child->next_sibling;
    }
}

int semantic_analyze(ASTNode *root) {
    error_count = 0;
    symbols = NULL;
    functions = NULL;

    collect_functions(root);
    visit_functions(root);

    clear_symbols();
    clear_functions();

    return error_count;
}
