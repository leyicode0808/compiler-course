#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
LEXER="$ROOT/build/cmm-lexer"

run_case() {
	name="$1"
	file="$2"
	dump="${3:-1}"
	printf '\n===== %s =====\n' "$name"
	printf 'source: %s\n\n' "${file#$ROOT/}"
	if [ "$dump" = "1" ]; then
		CMM_LEX_DUMP=1 "$LEXER" "$file" || true
	else
		"$LEXER" "$file" || true
	fi
}

run_case 'Project_1 required sample 1: lexical error' "$ROOT/test/proj1-2-3-6-exp1-1.cmm" 0
run_case 'Project_1 optional sample 1: octal and hex integer tokens' "$ROOT/test/proj1-2-3-7-exp1-0.cmm" 1
run_case 'Project_1 optional sample 5: comments are skipped' "$ROOT/test/proj1-2-3-7-exp5-0.cmm" 1
