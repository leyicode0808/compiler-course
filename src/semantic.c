#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantic.h"

typedef struct Symbol {
    char *name;
    char *type;
    int line;
    struct Symbol *next;
} Symbol;

typedef struct FunctionSymbol {
    char *name;
    char *return_type;
    int line;
    int param_count;
    struct FunctionSymbol *next;
} FunctionSymbol;

static Symbol *symbols = NULL;
static FunctionSymbol *functions = NULL;
static int error_count = 0;
static const char *current_return_type = NULL;

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

static void add_symbol(const char *name, const char *type, int line) {
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
    symbol->type = copy_string(type);
    symbol->line = line;
    symbol->next = symbols;
    symbols = symbol;
}

static void add_function(const char *name,
                         const char *return_type,
                         int line,
                         int param_count) {
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
    function->return_type = copy_string(return_type);
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
        free(symbol->type);
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
        free(function->return_type);
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
        ASTNode *return_type = find_child(node, "ReturnType");
        ASTNode *params = find_child(node, "Params");
        add_function(node->value,
                     return_type != NULL ? return_type->value : "int",
                     node->line,
                     count_children(params));
        return;
    }

    child = node->first_child;
    while (child != NULL) {
        collect_functions(child);
        child = child->next_sibling;
    }
}

static void collect_declarations_with_type(ASTNode *node,
                                           const char *current_type) {
    ASTNode *child;

    if (node == NULL) {
        return;
    }

    if (is_node(node, "Declaration")) {
        ASTNode *type_node = find_child(node, "Type");
        ASTNode *decls = find_child(node, "DeclaratorList");
        collect_declarations_with_type(decls,
                                       type_node != NULL ? type_node->value : current_type);
        return;
    }

    if (is_node(node, "Param")) {
        ASTNode *type_node = find_child(node, "Type");
        add_symbol(node->value,
                   type_node != NULL ? type_node->value : current_type,
                   node->line);
        return;
    }

    if (is_node(node, "VarDecl") || is_node(node, "ArrayDecl")) {
        add_symbol(node->value, current_type, node->line);
        return;
    }

    child = node->first_child;
    while (child != NULL) {
        collect_declarations_with_type(child, current_type);
        child = child->next_sibling;
    }
}

static const char *expression_type(ASTNode *node);

static int type_assignable(const char *left_type, const char *right_type) {
    if (left_type == NULL || right_type == NULL) {
        return 1;
    }

    if (strcmp(left_type, right_type) == 0) {
        return 1;
    }

    if (strcmp(left_type, "float") == 0 && strcmp(right_type, "int") == 0) {
        return 1;
    }

    return 0;
}

static const char *binary_numeric_type(ASTNode *node) {
    const char *left_type;
    const char *right_type;

    left_type = expression_type(node->first_child);
    right_type = expression_type(node->first_child != NULL
                                 ? node->first_child->next_sibling
                                 : NULL);

    if (left_type == NULL || right_type == NULL) {
        return NULL;
    }

    if (strcmp(left_type, "float") == 0 || strcmp(right_type, "float") == 0) {
        return "float";
    }

    return "int";
}

static const char *expression_type(ASTNode *node) {
    Symbol *symbol;
    FunctionSymbol *function;

    if (node == NULL) {
        return NULL;
    }

    if (is_node(node, "Int")) {
        return "int";
    }

    if (is_node(node, "Float")) {
        return "float";
    }

    if (is_node(node, "Var") || is_node(node, "ArrayAccess")) {
        symbol = find_symbol(node->value);
        return symbol != NULL ? symbol->type : NULL;
    }

    if (is_node(node, "Call")) {
        function = find_function(node->value);
        return function != NULL ? function->return_type : NULL;
    }

    if (is_node(node, "Add") ||
        is_node(node, "Sub") ||
        is_node(node, "Mul") ||
        is_node(node, "Div")) {
        return binary_numeric_type(node);
    }

    if (is_node(node, "Relop") ||
        is_node(node, "And") ||
        is_node(node, "Or") ||
        is_node(node, "Not")) {
        return "int";
    }

    return NULL;
}

static void check_assignment(ASTNode *node) {
    ASTNode *left;
    ASTNode *right;
    const char *left_type;
    const char *right_type;

    left = node->first_child;
    right = left != NULL ? left->next_sibling : NULL;

    left_type = expression_type(left);
    right_type = expression_type(right);

    if (!type_assignable(left_type, right_type)) {
        fprintf(stderr,
                "Semantic error at line %d: assignment type mismatch\n",
                node->line);
        error_count++;
    }
}

static void check_return(ASTNode *node) {
    ASTNode *expr = node->first_child;
    const char *return_type = expression_type(expr);

    if (!type_assignable(current_return_type, return_type)) {
        fprintf(stderr,
                "Semantic error at line %d: return type mismatch\n",
                node->line);
        error_count++;
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

    if (is_node(node, "Assign")) {
        check_assignment(node);
    }

    if (is_node(node, "Return")) {
        check_return(node);
    }

    child = node->first_child;
    while (child != NULL) {
        check_uses(child);
        child = child->next_sibling;
    }
}

static void analyze_function(ASTNode *function) {
    ASTNode *child;
    ASTNode *return_type;

    clear_symbols();

    return_type = find_child(function, "ReturnType");
    current_return_type = return_type != NULL ? return_type->value : "int";

    child = function->first_child;
    while (child != NULL) {
        if (is_node(child, "Params")) {
            collect_declarations_with_type(child, NULL);
        }
        if (is_node(child, "Compound")) {
            collect_declarations_with_type(child, NULL);
            check_uses(child);
        }
        child = child->next_sibling;
    }

    current_return_type = NULL;
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
    current_return_type = NULL;

    collect_functions(root);
    visit_functions(root);

    clear_symbols();
    clear_functions();

    return error_count;
}
