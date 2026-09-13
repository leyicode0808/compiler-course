#!/usr/bin/env sh
set -eu

missing=0

need() {
	if command -v "$1" >/dev/null 2>&1; then
		printf 'ok: %s -> %s\n' "$1" "$(command -v "$1")"
	else
		printf 'missing: %s\n' "$1"
		missing=1
	fi
}

need gcc
need make
need flex
need bison
need unzip
need pdftotext
need spim

if [ "$missing" -ne 0 ]; then
	printf 'Environment check failed.\n'
	exit 1
fi

printf 'Environment check passed.\n'

