#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
CASE="${1:-C-1}"
OUT_DIR="$ROOT/build/project4-mips"
P3_INPUT_DIR="$ROOT/tests/project3/inputs"
P4_EXAMPLE_DIR="$ROOT/examples/project4"
if [ ! -d "$P3_INPUT_DIR" ] && [ -d "$ROOT/proj3/tests/project3/inputs" ]; then
	P3_INPUT_DIR="$ROOT/proj3/tests/project3/inputs"
fi
if [ ! -d "$P4_EXAMPLE_DIR" ] && [ -d "$ROOT/proj4/examples/project4" ]; then
	P4_EXAMPLE_DIR="$ROOT/proj4/examples/project4"
fi
mkdir -p "$OUT_DIR"

case "$CASE" in
	A-1) SRC="$P3_INPUT_DIR/A-1.cmm"; INPUT="" ;;
	A-2) SRC="$P3_INPUT_DIR/A-2.cmm"; INPUT="50\n40\n20\n" ;;
	A-3) SRC="$P3_INPUT_DIR/A-3.cmm"; INPUT="2\n" ;;
	A-4) SRC="$P3_INPUT_DIR/A-4.cmm"; INPUT="3\n" ;;
	A-5) SRC="$P3_INPUT_DIR/A-5.cmm"; INPUT="6\n" ;;
	B-1) SRC="$P3_INPUT_DIR/B-1.cmm"; INPUT="1\n4\n3\n9\n2\n5\n" ;;
	B-2) SRC="$P3_INPUT_DIR/B-2.cmm"; INPUT="10\n" ;;
	B-3) SRC="$P3_INPUT_DIR/B-3.cmm"; INPUT="3\n" ;;
	C-1) SRC="$P3_INPUT_DIR/C-1.cmm"; INPUT="48\n18\n" ;;
	C-2) SRC="$P3_INPUT_DIR/C-2.cmm"; INPUT="-123\n" ;;
	E1-1) SRC="$P3_INPUT_DIR/E1-1.cmm"; INPUT="" ;;
	E1-2) SRC="$P3_INPUT_DIR/E1-2.cmm"; INPUT="3\n5\n" ;;
	E1-3) SRC="$P3_INPUT_DIR/E1-3.cmm"; INPUT="2\n3\n5\n" ;;
	E2-1) SRC="$P3_INPUT_DIR/E2-1.cmm"; INPUT="" ;;
	E2-2) SRC="$P3_INPUT_DIR/E2-2.cmm"; INPUT="" ;;
	E2-3) SRC="$P3_INPUT_DIR/E2-3.cmm"; INPUT="5\n" ;;
	P4-fibonacci) SRC="$P4_EXAMPLE_DIR/fibonacci.cmm"; INPUT="7\n" ;;
	P4-factorial) SRC="$P4_EXAMPLE_DIR/factorial.cmm"; INPUT="7\n" ;;
	*)
		printf 'unknown case: %s\n' "$CASE" >&2
		printf 'examples: C-1, A-5, B-1, E2-3, P4-fibonacci, P4-factorial\n' >&2
		exit 1
		;;
esac

ASM="$OUT_DIR/$CASE.s"
"$ROOT/build/cmmc" "$SRC" "$ASM"

printf 'generated: %s\n' "${ASM#$ROOT/}" >&2
printf '%b' "$INPUT" | spim -quiet -file "$ASM"
