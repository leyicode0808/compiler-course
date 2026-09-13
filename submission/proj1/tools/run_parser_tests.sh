#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
PARSER="$ROOT/build/cmmc"

run_case() {
	name="$1"
	file="$2"
	printf '\n===== %s =====\n' "$name"
	printf 'source: %s\n\n' "${file#$ROOT/}"
	"$PARSER" "$file" || true
}

run_case 'Project_1 required sample 2: syntax errors' "$ROOT/test/proj1-2-3-6-exp2-1.cmm"
run_case 'Project_1 required sample 3: simple AST' "$ROOT/test/proj1-2-3-6-exp3-1.cmm"
run_case 'Project_1 required sample 4: struct AST' "$ROOT/test/proj1-2-3-6-exp4-1.cmm"
