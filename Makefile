CC = gcc
FLEX = flex
BISON = bison

SRC_DIR = src
BUILD_DIR = build

LEXER_SRC = $(SRC_DIR)/lexer.l
PARSER_SRC = $(SRC_DIR)/parser.y
AST_C = $(SRC_DIR)/ast.c
AST_H = $(SRC_DIR)/ast.h
SEMANTIC_C = $(SRC_DIR)/semantic.c
SEMANTIC_H = $(SRC_DIR)/semantic.h

LEXER_C = $(BUILD_DIR)/lex.yy.c
PARSER_C = $(BUILD_DIR)/parser.tab.c
PARSER_H = $(BUILD_DIR)/parser.tab.h

PARSER_BIN = $(BUILD_DIR)/parser

.PHONY: all parser test clean

all: parser

parser: $(PARSER_BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(PARSER_C) $(PARSER_H): $(PARSER_SRC) $(AST_H) $(SEMANTIC_H) | $(BUILD_DIR)
	$(BISON) -d -o $(PARSER_C) $(PARSER_SRC)

$(LEXER_C): $(LEXER_SRC) $(PARSER_H) | $(BUILD_DIR)
	$(FLEX) -o $(LEXER_C) $(LEXER_SRC)

$(PARSER_BIN): $(PARSER_C) $(LEXER_C) $(AST_C) $(AST_H) $(SEMANTIC_C) $(SEMANTIC_H)
	$(CC) -I$(BUILD_DIR) -I$(SRC_DIR) $(PARSER_C) $(LEXER_C) $(AST_C) $(SEMANTIC_C) -o $(PARSER_BIN)

test: parser
	./$(PARSER_BIN) tests/test_minimal.cmm
	./$(PARSER_BIN) tests/test_float.cmm
	./$(PARSER_BIN) tests/test_function_params.cmm
	./$(PARSER_BIN) tests/test_array.cmm
	./$(PARSER_BIN) tests/test_logic_expr.cmm
	./$(PARSER_BIN) tests/test_control.cmm
	./$(PARSER_BIN) tests/test_ast_full.cmm
	-./$(PARSER_BIN) tests/test_minimal_error.cmm
	./$(PARSER_BIN) tests/test_semantic_var_error.cmm

clean:
	rm -f $(BUILD_DIR)/lex.yy.c
	rm -f $(BUILD_DIR)/parser.tab.c
	rm -f $(BUILD_DIR)/parser.tab.h
	rm -f $(BUILD_DIR)/parser
