#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
IMAGE=cmm-irsim-py38
P5_EXAMPLE_DIR="$ROOT/examples/project5"
if [ ! -d "$P5_EXAMPLE_DIR" ] && [ -d "$ROOT/proj5/examples/project5" ]; then
	P5_EXAMPLE_DIR="$ROOT/proj5/examples/project5"
fi

if [ ! -f "$ROOT/build/irsim/irsim/irsim.pyc" ]; then
	IRSIM_ZIP="$ROOT/编译原理课程设计/3/irsim.zip"
	if [ ! -f "$IRSIM_ZIP" ] && [ -f "$ROOT/proj3/course/irsim.zip" ]; then
		IRSIM_ZIP="$ROOT/proj3/course/irsim.zip"
	fi
	if [ -f "$IRSIM_ZIP" ]; then
		mkdir -p "$ROOT/build/irsim"
		unzip -q "$IRSIM_ZIP" -d "$ROOT/build/irsim"
	fi
fi

if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
	printf 'missing docker image: %s\n' "$IMAGE" >&2
	printf 'run ./tools/build_irsim_image.sh before running this test.\n' >&2
	exit 1
fi

mkdir -p "$ROOT/build/project5-ir"
for input in "$ROOT"/build/project3-ir/*.ir; do
	name=$(basename "$input")
	"$ROOT/build/cmm-opt" "$input" "$ROOT/build/project5-ir/$name"
done

mkdir -p "$ROOT/build/project5-examples"
for input in "$P5_EXAMPLE_DIR"/*.ir; do
	name=$(basename "$input" .ir)
	output="$ROOT/build/project5-examples/$name.opt.ir"
	"$ROOT/build/cmm-opt" "$input" "$output"
	if [ ! -s "$output" ]; then
		printf 'fail: project5 example %s produced empty output\n' "$name" >&2
		exit 1
	fi
	before=$(wc -l < "$input")
	after=$(wc -l < "$output")
	if [ "$after" -lt "$before" ]; then
		printf 'ok: project5 example %s (%s -> %s)\n' "$name" "$before" "$after"
	else
		printf 'fail: project5 example %s was not reduced (%s -> %s)\n' "$name" "$before" "$after" >&2
		exit 1
	fi
done

docker run -i --rm \
	-e QT_QPA_PLATFORM=offscreen \
	-v "$ROOT:/work" \
	-w /work \
	"$IMAGE" python - <<'PY'
import marshal
import opcode
import sys
from pathlib import Path

sys.path.insert(0, "build/irsim/irsim")

from PyQt5.QtWidgets import QApplication, QMessageBox, QInputDialog

with open("build/irsim/irsim/irsim.pyc", "rb") as f:
    f.read(16)
    code = marshal.load(f)

none_idx = code.co_consts.index(None)
class_defs = code.replace(
    co_code=code.co_code[:168]
    + bytes([opcode.opmap["LOAD_CONST"], none_idx, opcode.opmap["RETURN_VALUE"], 0])
)

namespace = {"__name__": "irsim_mod"}
exec(class_defs, namespace)

app = QApplication([])
QMessageBox.information = lambda *args, **kwargs: None
QMessageBox.warning = lambda *args, **kwargs: None
QMessageBox.critical = lambda *args, **kwargs: None


def run_case(name, inputs):
    values = list(inputs)

    def get_int(*args, **kwargs):
        if not values:
            raise RuntimeError(f"{name}: input exhausted")
        return values.pop(0), True

    QInputDialog.getInt = get_int
    sim = namespace["IRSim"]()
    sim.loadFile(f"build/project5-ir/{name}.ir")
    sim.run()
    text = sim.console.toPlainText().strip()
    return [int(line) for line in text.splitlines() if line.strip()]


cases = {
    "A-1": ([], [120, 325]),
    "A-2": ([50, 40, 20], [7, 1000, 3000]),
    "A-3": ([2], [3, 7, 28]),
    "A-4": ([3], [24]),
    "A-5": ([6], [8]),
    "B-1": ([1, 4, 3, 9, 2, 5], [3, 9, 19]),
    "B-2": ([10], [2]),
    "B-3": ([3], [693, 11]),
    "C-1": ([48, 18], [6]),
    "C-2": ([-123], [6]),
    "E1-1": ([], [42, 13]),
    "E1-2": ([3, 5], [12]),
    "E1-3": ([2, 3, 5], [8]),
    "E2-1": ([], [31, 6]),
    "E2-2": ([], [12, 21]),
    "E2-3": ([5], [0, 27]),
}

failed = 0
saved_lines = 0
for name, (inputs, expected) in cases.items():
    actual = run_case(name, inputs)
    before = Path(f"build/project3-ir/{name}.ir").read_text().splitlines()
    after = Path(f"build/project5-ir/{name}.ir").read_text().splitlines()
    saved = len(before) - len(after)
    saved_lines += saved
    if actual == expected:
        print(f"ok: {name} -> {actual} ({len(before)} -> {len(after)}, saved {saved})")
    else:
        print(f"fail: {name} -> {actual}, expected {expected}")
        failed += 1

print(f"opt tests: {len(cases) - failed} passed, {failed} failed, saved {saved_lines} IR lines")
raise SystemExit(failed)
PY
