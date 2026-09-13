#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
PARSER="$ROOT/build/cmmc"
OUT_DIR="$ROOT/build/project4-mips"

mkdir -p "$OUT_DIR"

run_case() {
	title="$1"
	id="$2"
	file="$3"
	input="$4"
	output="$OUT_DIR/$id.s"
	printf '\n===== %s =====\n' "$title"
	printf 'source: %s\n' "${file#$ROOT/}"
	printf 'output: %s\n\n' "${output#$ROOT/}"
	"$PARSER" "$file" "$output"
	sed -n '1,120p' "$output"
	printf '\n----- SPIM output -----\n'
	printf '%b' "$input" | spim -quiet -file "$output"
}

run_case 'Project_4 required sample 1: Fibonacci sequence' 'proj4-5-3-6-exp1-1' "$ROOT/test/proj4-5-3-6-exp1-1.cmm" '7\n'
run_case 'Project_4 required sample 2: recursive factorial' 'proj4-5-3-6-exp2-1' "$ROOT/test/proj4-5-3-6-exp2-1.cmm" '7\n'
run_case 'Project_4 handwritten sample: loop and function call' 'proj4-test1' "$ROOT/test/proj4-test1.cmm" ''
