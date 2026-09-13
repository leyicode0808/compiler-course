#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
IMAGE=${IRSIM_IMAGE:-cmm-irsim-py38}
OUT_DIR="$ROOT/build/project3-ir"

if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
	printf 'missing docker image: %s\n' "$IMAGE" >&2
	printf 'run ./tools/build_irsim_image.sh before running this test.\n' >&2
	exit 1
fi

mkdir -p "$OUT_DIR" "$ROOT/build/irsim"
if [ ! -f "$ROOT/build/irsim/irsim/irsim.pyc" ]; then
	unzip -q "$ROOT/course/irsim.zip" -d "$ROOT/build/irsim"
fi

CMM_IR=1 "$ROOT/build/cmmc" "$ROOT/test/proj3-4-3-6-exp1-1.cmm" "$OUT_DIR/proj3-4-3-6-exp1-1.ir"
CMM_IR=1 "$ROOT/build/cmmc" "$ROOT/test/proj3-4-3-6-exp2-1.cmm" "$OUT_DIR/proj3-4-3-6-exp2-1.ir"
CMM_IR=1 "$ROOT/build/cmmc" "$ROOT/test/proj3-4-3-7-exp1-0.cmm" "$OUT_DIR/proj3-4-3-7-exp1-0.ir"
CMM_IR=1 "$ROOT/build/cmmc" "$ROOT/test/proj3-4-3-7-exp2-0.cmm" "$OUT_DIR/proj3-4-3-7-exp2-0.ir"
CMM_IR=1 "$ROOT/build/cmmc" "$ROOT/test/proj3-test1.cmm" "$OUT_DIR/proj3-test1.ir"

docker run -i --rm \
	-e QT_QPA_PLATFORM=offscreen \
	-v "$ROOT:/work" \
	-w /work \
	"$IMAGE" python - <<'PY'
import marshal
import opcode
import sys

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
    "proj3-4-3-6-exp1-1": ([7], [1]),
    "proj3-4-3-6-exp2-1": ([7], [5040]),
    "proj3-4-3-7-exp1-0": ([], [3]),
    "proj3-4-3-7-exp2-0": ([], [1, 3]),
    "proj3-test1": ([], [30]),
}

def run_case(name, inputs):
    values = list(inputs)

    def get_int(*args, **kwargs):
        if not values:
            raise RuntimeError(f"{name}: input exhausted")
        return values.pop(0), True

    QInputDialog.getInt = get_int
    sim = namespace["IRSim"]()
    sim.loadFile(f"build/project3-ir/{name}.ir")
    sim.run()
    text = sim.console.toPlainText().strip()
    return [int(line) for line in text.splitlines() if line.strip()]

failed = 0
for name, (inputs, expected) in cases.items():
    actual = run_case(name, inputs)
    if actual == expected:
        print(f"ok: {name} -> {actual}")
    else:
        print(f"fail: {name} -> {actual}, expected {expected}")
        failed += 1

print(f"irsim tests: {len(cases) - failed} passed, {failed} failed")
raise SystemExit(failed)
PY
