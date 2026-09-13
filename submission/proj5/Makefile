SHELL := /bin/sh

.PHONY: help check-env opt opt-demo opt-test demo test clean

help:
	@printf '%s\n' \
		'Targets:' \
		'  make check-env  Check tools required by proj5' \
		'  make opt        Build the IR optimizer' \
		'  build/cmm-opt test/proj5-test1.ir /tmp/proj5-test1.opt.ir' \
		'                  Optimize one IR file manually after make opt' \
		'  diff -u test/proj5-test1.ir /tmp/proj5-test1.opt.ir' \
		'                  Compare original and optimized IR for one case' \
		'  make opt-demo   Optimize Project_5.pdf IR examples in test/' \
		'  make opt-test   Optimize examples and compare original/optimized IR with IRSim' \
		'  make demo       Alias of make opt-demo' \
		'  make test       Alias of make opt-test' \
		'  make clean      Remove build outputs'

check-env:
	@./tools/check_env.sh

opt: build/cmm-opt

build/cmm-opt: src/iropt_main.c src/iropt.c src/iropt.h
	@mkdir -p build
	gcc -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -I src \
		src/iropt_main.c src/iropt.c -o build/cmm-opt

opt-demo: build/cmm-opt
	@./tools/run_opt_tests.sh --no-irsim

opt-test: build/cmm-opt
	@./tools/run_opt_tests.sh

demo: opt-demo

test: opt-test

clean:
	@rm -rf build
