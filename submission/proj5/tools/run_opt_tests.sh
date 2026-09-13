#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
IMAGE=${IRSIM_IMAGE:-cmm-irsim-py38}
RUN_IRSIM=1

if [ "${1:-}" = "--no-irsim" ]; then
	RUN_IRSIM=0
fi

OUT_DIR="$ROOT/build/project5-opt"
mkdir -p "$OUT_DIR"

pass=0
fail=0

for input in "$ROOT"/test/*.ir; do
	name=$(basename "$input" .ir)
	output="$OUT_DIR/$name.opt.ir"

	if "$ROOT/build/cmm-opt" "$input" "$output" && [ -s "$output" ]; then
		before=$(wc -l < "$input")
		after=$(wc -l < "$output")
		printf 'ok: %s (%s -> %s) -> %s\n' "$name" "$before" "$after" "build/project5-opt/$name.opt.ir"
		pass=$((pass + 1))
	else
		printf 'fail: %s\n' "$name" >&2
		fail=$((fail + 1))
	fi
done

printf 'opt demo: %d optimized, %d failed\n' "$pass" "$fail"

if [ "$fail" -ne 0 ]; then
	exit 1
fi

if [ "$RUN_IRSIM" -eq 0 ]; then
	exit 0
fi

if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
	printf 'missing docker image: %s\n' "$IMAGE" >&2
	printf 'run ./tools/build_irsim_image.sh before running IRSim validation.\n' >&2
	exit 1
fi

mkdir -p "$ROOT/build/irsim"
if [ ! -f "$ROOT/build/irsim/irsim/irsim.pyc" ]; then
	unzip -q "$ROOT/course/irsim.zip" -d "$ROOT/build/irsim"
fi

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

cases = {
    "proj5-6-3-6-exp1-1": [5, 7],
    "proj5-6-3-6-exp2-1": [5, 7],
    "proj5-6-3-6-exp3-1": [],
    "proj5-test1": [],
}


def run_file(path, inputs):
    values = list(inputs)

    def get_int(*args, **kwargs):
        if not values:
            raise RuntimeError(f"{path}: input exhausted")
        return values.pop(0), True

    QInputDialog.getInt = get_int
    sim = namespace["IRSim"]()
    sim.loadFile(str(path))
    sim.run()
    text = sim.console.toPlainText().strip()
    return [int(line) for line in text.splitlines() if line.strip()]


failed = 0
saved_lines = 0
for name, inputs in cases.items():
    original_path = Path("test") / f"{name}.ir"
    optimized_path = Path("build/project5-opt") / f"{name}.opt.ir"
    original = run_file(original_path, inputs)
    optimized = run_file(optimized_path, inputs)
    before = original_path.read_text().splitlines()
    after = optimized_path.read_text().splitlines()
    saved = len(before) - len(after)
    saved_lines += saved
    if original == optimized:
        print(f"ok: {name} -> {optimized} ({len(before)} -> {len(after)}, saved {saved})")
    else:
        print(f"fail: {name} original {original}, optimized {optimized}")
        failed += 1

print(f"irsim opt tests: {len(cases) - failed} passed, {failed} failed, saved {saved_lines} IR lines")
raise SystemExit(failed)
PY
