SHELL := /bin/sh

.PHONY: help check-env lexer parser semantic semantic-demo semantic-test demo test clean

help:
	@printf '%s\n' \
		'Targets:' \
		'  make check-env      Check tools required by proj2' \
		'  make lexer          Build the Flex lexer' \
		'  make parser         Build the Bison parser / AST printer' \
		'  make semantic       Build the semantic analyzer' \
		'  make semantic-demo  Show semantic output for Project_2.pdf examples' \
		'  make semantic-test  Alias of make semantic-demo' \
		'  make demo           Alias of make semantic-demo' \
		'  make test           Alias of make semantic-demo' \
		'  make clean          Remove build outputs'

check-env:
	@./tools/check_env.sh

lexer: build/cmm-lexer

build/cmm-lexer: src/lexer.l src/lexer_main.c src/token.h
	@mkdir -p build
	flex -o build/lexer.c src/lexer.l
	gcc -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -I src src/lexer_main.c build/lexer.c -lfl -o build/cmm-lexer

parser: build/cmmc

semantic: build/cmmc

build/cmmc: src/parser.y src/lexer.l src/parser_main.c src/ast.c src/ast.h src/semantic.c src/semantic.h
	@mkdir -p build
	bison -d -o build/parser.tab.c src/parser.y
	flex -DPARSER_BUILD -o build/parser_lexer.c src/lexer.l
	gcc -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -I src -I build \
		src/parser_main.c src/ast.c src/semantic.c build/parser.tab.c build/parser_lexer.c -lfl -o build/cmmc

semantic-demo: build/cmmc
	@./tools/run_semantic_tests.sh

semantic-test: semantic-demo

demo: semantic-demo

test: semantic-demo

clean:
	@rm -rf build
