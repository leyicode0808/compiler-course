#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
PARSER="$ROOT/build/cmmc"
OUT_DIR="$ROOT/build/project3-ir"
INPUT_DIR="$ROOT/tests/project3/inputs"
if [ ! -d "$INPUT_DIR" ] && [ -d "$ROOT/proj3/tests/project3/inputs" ]; then
	INPUT_DIR="$ROOT/proj3/tests/project3/inputs"
fi

mkdir -p "$OUT_DIR"

pass=0
fail=0

check_file() {
	name="$1"
	input="$2"
	output="$OUT_DIR/$name.ir"

	if CMM_IR=1 "$PARSER" "$input" "$output" && [ -s "$output" ] && grep -q '^FUNCTION ' "$output"; then
		printf 'ok: %s -> %s\n' "$name" "$output"
		pass=$((pass + 1))
	else
		printf 'fail: %s\n' "$name"
		fail=$((fail + 1))
	fi
}

for input in "$INPUT_DIR"/*.cmm; do
	name=$(basename "$input" .cmm)
	check_file "$name" "$input"
done

printf 'ir tests: %d generated, %d failed\n' "$pass" "$fail"

if [ "$fail" -ne 0 ]; then
	exit 1
fi
