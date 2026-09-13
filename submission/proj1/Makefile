SHELL := /bin/sh

.PHONY: help check-env lexer parser lexer-demo parser-demo demo test clean

help:
	@printf '%s\n' \
		'Targets:' \
		'  make check-env    Check tools required by proj1' \
		'  make lexer        Build the Flex lexer' \
		'  make parser       Build the Bison parser / AST printer' \
		'  make lexer-demo   Show lexer output for a Project_1.pdf example' \
		'  make parser-demo  Show parser output for a Project_1.pdf example' \
		'  make demo         Run several public examples for display' \
		'  make test         Alias of make demo' \
		'  make clean        Remove build outputs'

check-env:
	@./tools/check_env.sh

lexer: build/cmm-lexer

build/cmm-lexer: src/lexer.l src/lexer_main.c src/token.h
	@mkdir -p build
	flex -o build/lexer.c src/lexer.l
	gcc -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -I src src/lexer_main.c build/lexer.c -lfl -o build/cmm-lexer

parser: build/cmmc

build/cmmc: src/parser.y src/lexer.l src/parser_main.c src/ast.c src/ast.h
	@mkdir -p build
	bison -d -o build/parser.tab.c src/parser.y
	flex -DPARSER_BUILD -o build/parser_lexer.c src/lexer.l
	gcc -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -I src -I build \
		src/parser_main.c src/ast.c build/parser.tab.c build/parser_lexer.c -lfl -o build/cmmc

lexer-demo: build/cmm-lexer
	@./tools/run_lexer_tests.sh

parser-demo: build/cmmc
	@./tools/run_parser_tests.sh

demo: lexer-demo parser-demo

test: demo

clean:
	@rm -rf build
