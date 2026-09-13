SHELL := /bin/sh

.PHONY: help check-env lexer parser semantic ir ir-demo ir-test irsim-demo irsim-test demo test clean

help:
	@printf '%s\n' \
		'Targets:' \
		'  make check-env  Check tools required by proj3' \
		'  make lexer      Build the Flex lexer' \
		'  make parser     Build the Bison parser / AST printer' \
		'  make semantic   Build the semantic analyzer' \
		'  make ir         Build the IR generator' \
		'  make ir-demo    Generate IR for Project_3.pdf examples' \
		'  make ir-test    Alias of make ir-demo' \
		'  make irsim-demo Generate IR and run Project_3.pdf examples with IRSim' \
		'  make irsim-test Alias of make irsim-demo' \
		'  make demo       Alias of make ir-demo' \
		'  make test       Alias of make ir-demo' \
		'  make clean      Remove build outputs'

check-env:
	@./tools/check_env.sh

lexer: build/cmm-lexer

build/cmm-lexer: src/lexer.l src/lexer_main.c src/token.h
	@mkdir -p build
	flex -o build/lexer.c src/lexer.l
	gcc -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -I src src/lexer_main.c build/lexer.c -lfl -o build/cmm-lexer

parser: build/cmmc

semantic: build/cmmc

ir: build/cmmc

build/cmmc: src/parser.y src/lexer.l src/parser_main.c src/ast.c src/ast.h src/semantic.c src/semantic.h src/irgen.c src/irgen.h
	@mkdir -p build
	bison -d -o build/parser.tab.c src/parser.y
	flex -DPARSER_BUILD -o build/parser_lexer.c src/lexer.l
	gcc -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -I src -I build \
		src/parser_main.c src/ast.c src/semantic.c src/irgen.c build/parser.tab.c build/parser_lexer.c -lfl -o build/cmmc

ir-demo: build/cmmc
	@./tools/run_ir_tests.sh

ir-test: ir-demo

irsim-demo: build/cmmc
	@./tools/run_irsim_tests.sh

irsim-test: irsim-demo

demo: ir-demo

test: ir-demo

clean:
	@rm -rf build
