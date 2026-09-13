#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
LOCK="$ROOT/tests/.extract.lock"

mkdir -p "$ROOT/tests"

while ! mkdir "$LOCK" 2>/dev/null; do
	sleep 1
done
trap 'rmdir "$LOCK"' EXIT INT TERM

extract_project() {
	project="$1"
	zip_file="$ROOT/编译原理课程设计/$project/Tests_$project.zip"
	dest="$ROOT/tests/project$project"
	bundled="$ROOT/proj$project/tests/project$project"

	if [ ! -f "$zip_file" ]; then
		if [ -d "$bundled" ]; then
			printf 'using bundled tests: proj%s/tests/project%s\n' "$project" "$project"
		else
			printf 'skip: %s not found\n' "$zip_file"
		fi
		return
	fi

	rm -rf "$dest"
	mkdir -p "$dest"
	unzip -q "$zip_file" -d "$dest"

	if [ -d "$dest/Tests" ]; then
		mv "$dest/Tests"/* "$dest/"
		rmdir "$dest/Tests"
	fi

	printf 'extracted: %s -> %s\n' "$zip_file" "$dest"
}

extract_project 1
extract_project 2
extract_project 3

printf 'Done. Extracted tests are ignored by Git.\n'
