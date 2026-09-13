#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
PARSER="$ROOT/build/cmmc"
OUT_DIR="$ROOT/build/project3-ir"

mkdir -p "$OUT_DIR"

run_case() {
	title="$1"
	id="$2"
	file="$3"
	output="$OUT_DIR/$id.ir"
	printf '\n===== %s =====\n' "$title"
	printf 'source: %s\n' "${file#$ROOT/}"
	printf 'output: %s\n\n' "${output#$ROOT/}"
	CMM_IR=1 "$PARSER" "$file" "$output"
	sed -n '1,120p' "$output"
}

run_case 'Project_3 required sample 1: sign function' 'proj3-4-3-6-exp1-1' "$ROOT/test/proj3-4-3-6-exp1-1.cmm"
run_case 'Project_3 required sample 2: recursive factorial' 'proj3-4-3-6-exp2-1' "$ROOT/test/proj3-4-3-6-exp2-1.cmm"
run_case 'Project_3 optional sample 1: struct parameter' 'proj3-4-3-7-exp1-0' "$ROOT/test/proj3-4-3-7-exp1-0.cmm"
run_case 'Project_3 optional sample 2: array parameter and two-dimensional array' 'proj3-4-3-7-exp2-0' "$ROOT/test/proj3-4-3-7-exp2-0.cmm"
run_case 'Project_3 handwritten sample: loop and function call' 'proj3-test1' "$ROOT/test/proj3-test1.cmm"
