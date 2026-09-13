SHELL := /bin/sh

.PHONY: help check-env prepare-tests lexer lexer-test parser parser-test semantic semantic-test ir ir-test irsim-test mips mips-test opt opt-test test clean status

help:
	@printf '%s\n' \
		'Targets:' \
		'  make check-env      Check required local tools' \
		'  make prepare-tests  Extract course test archives into tests/project*/' \
		'  make lexer          Build the practice 1.1 lexer' \
		'  make lexer-test     Run practice 1.1 lexer regression tests' \
		'  make parser         Build the practice 1.2 parser' \
		'  make parser-test    Run practice 1.2 parser smoke tests' \
		'  make semantic       Build the practice 2 semantic analyzer' \
		'  make semantic-test  Run practice 2 semantic regression tests' \
		'  make ir             Build the practice 3 IR generator' \
		'  make ir-test        Generate IR for practice 3 samples' \
		'  make irsim-test     Run generated IR with the course simulator' \
		'  make mips           Build the practice 4 MIPS generator' \
		'  make mips-test      Run generated MIPS with SPIM' \
		'  make opt            Build the practice 5 IR optimizer' \
		'  make opt-test       Optimize generated IR and validate with IRSim' \
		'  make test           Run practice 1-5 regression tests' \
		'  make clean          Remove build outputs and extracted generated files' \
		'  make status         Show version-control status'

check-env:
	@./tools/check_env.sh

prepare-tests:
	@./tools/extract_tests.sh

lexer: build/cmm-lexer

build/cmm-lexer: src/lexer.l src/lexer_main.c src/token.h
	@mkdir -p build
	flex -o build/lexer.c src/lexer.l
	gcc -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -I src src/lexer_main.c build/lexer.c -lfl -o build/cmm-lexer

lexer-test: build/cmm-lexer prepare-tests
	@./tools/run_lexer_tests.sh

parser: build/cmmc

build/cmmc: src/parser.y src/lexer.l src/parser_main.c src/ast.c src/ast.h src/semantic.c src/semantic.h src/irgen.c src/irgen.h src/mipsgen.c src/mipsgen.h
	@mkdir -p build
	bison -d -o build/parser.tab.c src/parser.y
	flex -DPARSER_BUILD -o build/parser_lexer.c src/lexer.l
	gcc -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -I src -I build \
		src/parser_main.c src/ast.c src/semantic.c src/irgen.c src/mipsgen.c build/parser.tab.c build/parser_lexer.c -lfl -o build/cmmc

parser-test: build/cmmc prepare-tests
	@./tools/run_parser_tests.sh

semantic: build/cmmc

semantic-test: build/cmmc prepare-tests
	@./tools/run_semantic_tests.sh

ir: build/cmmc

ir-test: build/cmmc prepare-tests
	@./tools/run_ir_tests.sh

irsim-test: ir-test
	@./tools/run_irsim_tests.sh

mips: build/cmmc

mips-test: build/cmmc prepare-tests
	@./tools/run_mips_tests.sh

opt: build/cmm-opt

build/cmm-opt: src/iropt_main.c src/iropt.c src/iropt.h
	@mkdir -p build
	gcc -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -I src \
		src/iropt_main.c src/iropt.c -o build/cmm-opt

opt-test: build/cmm-opt ir-test
	@./tools/run_opt_tests.sh

test:
	@$(MAKE) lexer-test
	@$(MAKE) parser-test
	@$(MAKE) semantic-test
	@$(MAKE) irsim-test
	@$(MAKE) mips-test
	@$(MAKE) opt-test

clean:
	@rm -rf build tests/project1 tests/project2 tests/project3

status:
	@./tools/gitw status --short --branch
