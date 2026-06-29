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
IR_C = $(SRC_DIR)/ir.c
IR_H = $(SRC_DIR)/ir.h
MIPS_C = $(SRC_DIR)/mips.c
MIPS_H = $(SRC_DIR)/mips.h

OPTIMIZE_C = $(SRC_DIR)/optimize.c
OPTIMIZE_H = $(SRC_DIR)/optimize.h

.PHONY: all parser test mips-test clean

all: parser

parser: $(PARSER_BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(PARSER_C) $(PARSER_H): $(PARSER_SRC) $(AST_H) $(SEMANTIC_H) $(IR_H) $(MIPS_H) $(OPTIMIZE_H) | $(BUILD_DIR)
	$(BISON) -d -o $(PARSER_C) $(PARSER_SRC)

$(LEXER_C): $(LEXER_SRC) $(PARSER_H) | $(BUILD_DIR)
	$(FLEX) -o $(LEXER_C) $(LEXER_SRC)

$(PARSER_BIN): $(PARSER_C) $(LEXER_C) $(AST_C) $(AST_H) $(SEMANTIC_C) $(SEMANTIC_H) $(IR_C) $(IR_H) $(MIPS_C) $(MIPS_H) $(OPTIMIZE_C) $(OPTIMIZE_H)
	$(CC) -I$(BUILD_DIR) -I$(SRC_DIR) $(PARSER_C) $(LEXER_C) $(AST_C) $(SEMANTIC_C) $(IR_C) $(MIPS_C) $(OPTIMIZE_C) -o $(PARSER_BIN)

test: parser
	./$(PARSER_BIN) tests/test_minimal.cmm
	./$(PARSER_BIN) tests/test_float.cmm
	./$(PARSER_BIN) tests/test_function_params.cmm
	./$(PARSER_BIN) tests/test_array.cmm
	./$(PARSER_BIN) tests/test_logic_expr.cmm
	./$(PARSER_BIN) tests/test_control.cmm
	./$(PARSER_BIN) tests/test_ast_full.cmm
	
	./$(PARSER_BIN) tests/test_ir_basic.cmm
	./$(PARSER_BIN) tests/test_ir_control.cmm
	
	./$(PARSER_BIN) tests/test_opt_const.cmm
	
	-./$(PARSER_BIN) tests/test_minimal_error.cmm
	./$(PARSER_BIN) tests/test_semantic_var_error.cmm
	./$(PARSER_BIN) tests/test_semantic_func_error.cmm
	./$(PARSER_BIN) tests/test_semantic_type_error.cmm
	./$(PARSER_BIN) tests/test_semantic_arg_type_error.cmm
	./$(PARSER_BIN) tests/test_semantic_array_index_error.cmm
	
mips-test: parser
	./$(PARSER_BIN) tests/test_ir_basic.cmm > $(BUILD_DIR)/output.txt
	sed -n '/^\.data/,$$p' $(BUILD_DIR)/output.txt > $(BUILD_DIR)/test_ir_basic.s
	spim -file $(BUILD_DIR)/test_ir_basic.s
	./$(PARSER_BIN) tests/test_ir_control.cmm > $(BUILD_DIR)/output.txt
	sed -n '/^\.data/,$$p' $(BUILD_DIR)/output.txt > $(BUILD_DIR)/test_ir_control.s
	spim -file $(BUILD_DIR)/test_ir_control.s
	./$(PARSER_BIN) tests/test_array.cmm > $(BUILD_DIR)/output.txt
	sed -n '/^\.data/,$$p' $(BUILD_DIR)/output.txt > $(BUILD_DIR)/test_array.s
	spim -file $(BUILD_DIR)/test_array.s
	./$(PARSER_BIN) tests/test_function_params.cmm > $(BUILD_DIR)/output.txt
	sed -n '/^\.data/,$$p' $(BUILD_DIR)/output.txt > $(BUILD_DIR)/test_function_params.s
	spim -file $(BUILD_DIR)/test_function_params.s
	./$(PARSER_BIN) tests/test_logic_expr.cmm > $(BUILD_DIR)/output.txt
	sed -n '/^\.data/,$$p' $(BUILD_DIR)/output.txt > $(BUILD_DIR)/test_logic_expr.s
	spim -file $(BUILD_DIR)/test_logic_expr.s
	./$(PARSER_BIN) tests/test_opt_const.cmm > $(BUILD_DIR)/output.txt
	sed -n '/^\.data/,$$p' $(BUILD_DIR)/output.txt > $(BUILD_DIR)/test_opt_const.s
	spim -file $(BUILD_DIR)/test_opt_const.s
clean:
	rm -f $(BUILD_DIR)/lex.yy.c
	rm -f $(BUILD_DIR)/parser.tab.c
	rm -f $(BUILD_DIR)/parser.tab.h
	rm -f $(BUILD_DIR)/parser
