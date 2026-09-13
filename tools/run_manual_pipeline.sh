#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

if [ "$#" -lt 1 ]; then
	printf 'usage: %s <source.cmm> [stdin-text]\n' "$0" >&2
	printf "example: %s examples/manual/gcd.cmm '48\\n18\\n'\n" "$0" >&2
	exit 1
fi

SRC="$1"
INPUT="${2:-}"

if [ ! -f "$SRC" ]; then
	printf 'source file not found: %s\n' "$SRC" >&2
	exit 1
fi

case "$SRC" in
	/*) SRC_ABS="$SRC" ;;
	*) SRC_ABS=$(CDPATH= cd -- "$(dirname -- "$SRC")" && pwd)/$(basename "$SRC") ;;
esac

mkdir -p "$ROOT/build/manual"
BASE=$(basename "$SRC" .cmm)
IR="$ROOT/build/manual/$BASE.ir"
OPT_IR="$ROOT/build/manual/$BASE.opt.ir"
ASM="$ROOT/build/manual/$BASE.s"

cd "$ROOT"

if [ ! -x build/cmm-lexer ] || [ ! -x build/cmmc ] || [ ! -x build/cmm-opt ]; then
	printf 'building required tools in %s\n' "$ROOT" >&2
	make lexer parser opt
fi

printf '\n===== Practice 1.1: Lexer errors / tokens =====\n'
build/cmm-lexer "$SRC_ABS"
printf '\n--- token dump ---\n'
CMM_LEX_DUMP=1 build/cmm-lexer "$SRC_ABS"

printf '\n===== Practice 1.2: Syntax tree =====\n'
build/cmmc "$SRC_ABS"

printf '\n===== Practice 2: Semantic check =====\n'
if CMM_SEMANTIC=1 build/cmmc "$SRC_ABS"; then
	printf 'semantic check passed: no output means no semantic error.\n'
fi

printf '\n===== Practice 3: IR =====\n'
CMM_IR=1 build/cmmc "$SRC_ABS" "$IR"
cat "$IR"

printf '\n===== Practice 5: Optimized IR =====\n'
build/cmm-opt "$IR" "$OPT_IR"
printf '\n--- diff: original IR -> optimized IR ---\n'
if ! diff -u "$IR" "$OPT_IR"; then
	:
fi
printf '\n--- optimized IR ---\n'
cat "$OPT_IR"

printf '\n===== Practice 4: MIPS =====\n'
build/cmmc "$SRC_ABS" "$ASM"
cat "$ASM"

printf '\n===== SPIM output =====\n'
printf '%b' "$INPUT" | spim -quiet -file "$ASM"
