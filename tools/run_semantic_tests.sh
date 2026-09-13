#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
PARSER="$ROOT/build/cmmc"
TEST_DIR="$ROOT/tests/project2"
if [ ! -d "$TEST_DIR" ] && [ -d "$ROOT/proj2/tests/project2" ]; then
	TEST_DIR="$ROOT/proj2/tests/project2"
fi

pass=0
fail=0

check_exact() {
	name="$1"
	input="$2"
	expected="$3"
	actual=$(mktemp)
	expected_norm=$(mktemp)
	actual_norm=$(mktemp)

	if [ "$name" = "D-2" ]; then
		CMM_SEMANTIC=1 CMM_NO_SCOPE=1 "$PARSER" "$input" >"$actual" || :
	elif CMM_SEMANTIC=1 "$PARSER" "$input" >"$actual"; then
		:
	fi

	awk '{ sub(/\r$/, ""); print }' "$expected" >"$expected_norm"
	awk '{ sub(/\r$/, ""); print }' "$actual" >"$actual_norm"

	if diff -u "$expected_norm" "$actual_norm"; then
		printf 'ok: %s\n' "$name"
		pass=$((pass + 1))
	else
		printf 'fail: %s\n' "$name"
		fail=$((fail + 1))
	fi

	rm -f "$actual" "$expected_norm" "$actual_norm"
}

for expected in "$TEST_DIR"/expects/*.exp; do
	name=$(basename "$expected" .exp)
	input="$TEST_DIR/inputs/$name.cmm"
	if [ -f "$input" ]; then
		check_exact "$name" "$input" "$expected"
	fi
done

printf 'semantic tests: %d passed, %d failed\n' "$pass" "$fail"

if [ "$fail" -ne 0 ]; then
	exit 1
fi
