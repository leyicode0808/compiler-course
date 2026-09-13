#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
PARSER="$ROOT/build/cmmc"

run_case() {
	name="$1"
	file="$2"
	extra_env="${3:-}"
	printf '\n===== %s =====\n' "$name"
	printf 'source: %s\n\n' "${file#$ROOT/}"
	if [ "$extra_env" = "no-scope" ]; then
		CMM_SEMANTIC=1 CMM_NO_SCOPE=1 "$PARSER" "$file" || true
	else
		CMM_SEMANTIC=1 "$PARSER" "$file" || true
	fi
}

run_case 'Project_2 required sample 1: undefined variable' "$ROOT/test/proj2-3-3-6-exp1-1.cmm"
run_case 'Project_2 required sample 2: undefined function' "$ROOT/test/proj2-3-3-6-exp2-1.cmm"
run_case 'Project_2 required sample 3: redefined variable' "$ROOT/test/proj2-3-3-6-exp3-1.cmm"
run_case 'Project_2 required sample 5: assignment type mismatch' "$ROOT/test/proj2-3-3-6-exp5-1.cmm"
run_case 'Project_2 required sample 8: return type mismatch' "$ROOT/test/proj2-3-3-6-exp8-1.cmm"
run_case 'Project_2 required sample 14: non-existent struct field' "$ROOT/test/proj2-3-3-6-exp14-1.cmm"
run_case 'Project_2 required sample 17: undefined structure' "$ROOT/test/proj2-3-3-6-exp17-1.cmm"
run_case 'Project_2 optional sample 1: function declaration and definition' "$ROOT/test/proj2-3-3-7-exp1-0.cmm"
run_case 'Project_2 optional sample 2: inconsistent function declaration' "$ROOT/test/proj2-3-3-7-exp2-0.cmm"
run_case 'Project_2 optional sample 3: nested scope accepted' "$ROOT/test/proj2-3-3-7-exp3-0.cmm"
run_case 'Project_2 optional sample 4: nested scope redefinition' "$ROOT/test/proj2-3-3-7-exp4-0.cmm"
