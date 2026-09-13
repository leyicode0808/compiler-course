SHELL := /bin/sh

.PHONY: help check-env lexer parser semantic ir mips mips-demo mips-test demo test clean

help:
	@printf '%s\n' \
		'Targets:' \
		'  make check-env   Check tools required by proj4' \
		'  make lexer       Build the Flex lexer' \
		'  make parser      Build the Bison parser / AST printer' \
		'  make semantic    Build the semantic analyzer' \
		'  make ir          Build the IR generator' \
		'  make mips        Build the MIPS generator' \
		'  make mips-demo   Generate and run MIPS for Project_4.pdf examples' \
		'  make mips-test   Alias of make mips-demo' \
		'  make demo        Alias of make mips-demo' \
		'  make test        Alias of make mips-demo' \
		'  make clean       Remove build outputs'

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

mips: build/cmmc

build/cmmc: src/parser.y src/lexer.l src/parser_main.c src/ast.c src/ast.h src/semantic.c src/semantic.h src/irgen.c src/irgen.h src/mipsgen.c src/mipsgen.h
	@mkdir -p build
	bison -d -o build/parser.tab.c src/parser.y
	flex -DPARSER_BUILD -o build/parser_lexer.c src/lexer.l
	gcc -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -I src -I build \
		src/parser_main.c src/ast.c src/semantic.c src/irgen.c src/mipsgen.c build/parser.tab.c build/parser_lexer.c -lfl -o build/cmmc

mips-demo: build/cmmc
	@./tools/run_mips_tests.sh

mips-test: mips-demo

demo: mips-demo

test: mips-demo

clean:
	@rm -rf build
