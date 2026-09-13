#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
LEXER="$ROOT/build/cmm-lexer"
TEST_DIR="$ROOT/tests/project1"
if [ ! -d "$TEST_DIR" ] && [ -d "$ROOT/proj1/tests/project1" ]; then
	TEST_DIR="$ROOT/proj1/tests/project1"
fi

pass=0
fail=0

check_file() {
	name="$1"
	input="$2"
	expected="$3"
	actual=$(mktemp)
	expected_norm=$(mktemp)
	actual_norm=$(mktemp)

	if "$LEXER" "$input" >"$actual"; then
		:
	fi

	printf '%s\n' "$(cat "$expected")" >"$expected_norm"
	printf '%s\n' "$(cat "$actual")" >"$actual_norm"

	if diff -u "$expected_norm" "$actual_norm"; then
		printf 'ok: %s\n' "$name"
		pass=$((pass + 1))
	else
		printf 'fail: %s\n' "$name"
		fail=$((fail + 1))
	fi

	rm -f "$actual" "$expected_norm" "$actual_norm"
}

check_text() {
	name="$1"
	input="$2"
	expected_text="$3"
	expected=$(mktemp)
	actual=$(mktemp)

	printf '%s\n' "$expected_text" >"$expected"

	if "$LEXER" "$input" >"$actual"; then
		:
	fi

	if diff -u "$expected" "$actual"; then
		printf 'ok: %s\n' "$name"
		pass=$((pass + 1))
	else
		printf 'fail: %s\n' "$name"
		fail=$((fail + 1))
	fi

	rm -f "$expected" "$actual"
}

check_file A-1 "$TEST_DIR/inputs/A-1.cmm" "$TEST_DIR/expects/A-1.exp"
check_file A-2 "$TEST_DIR/inputs/A-2.cmm" "$TEST_DIR/expects/A-2.exp"
check_file A-5 "$TEST_DIR/inputs/A-5.cmm" "$TEST_DIR/expects/A-5.exp"

check_text E1-2 "$TEST_DIR/inputs/E1-2.cmm" "Error type A at Line 3: Invalid hexadecimal literal '0xQQQ'.
Error type A at Line 4: Invalid octal literal '099'."

check_text E2-2 "$TEST_DIR/inputs/E2-2.cmm" "Error type A at Line 3: Invalid floating point literal '2.5e'.
Error type A at Line 4: Invalid floating point literal '1.0E+'."

check_text E3-2 "$TEST_DIR/inputs/E3-2.cmm" "Error type A at Line 1: Nested block comment is not allowed."

printf 'lexer tests: %d passed, %d failed\n' "$pass" "$fail"

if [ "$fail" -ne 0 ]; then
	exit 1
fi
