CC = gcc
FLEX = flex

SRC_DIR = src
BUILD_DIR = build
TEST_DIR = tests

LEXER_SRC = $(SRC_DIR)/lexer.l
LEXER_C = $(BUILD_DIR)/lex.yy.c
LEXER_BIN = $(BUILD_DIR)/lexer

.PHONY: all test clean

all: $(LEXER_BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(LEXER_C): $(LEXER_SRC) | $(BUILD_DIR)
	$(FLEX) -o $(LEXER_C) $(LEXER_SRC)

$(LEXER_BIN): $(LEXER_C)
	$(CC) $(LEXER_C) -o $(LEXER_BIN)

test: all
	echo "int main() { return 123; }" | ./$(LEXER_BIN)
	echo "float x; if (x >= 10) return x;" | ./$(LEXER_BIN)

clean:
	rm -f $(LEXER_C) $(LEXER_BIN)
