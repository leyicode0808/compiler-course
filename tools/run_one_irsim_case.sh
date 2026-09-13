#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
IMAGE=cmm-irsim-py38
CASE="${1:-C-1}"

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
	printf 'run ./tools/build_irsim_image.sh before running this command.\n' >&2
	exit 1
fi

if [ ! -f "$ROOT/build/project3-ir/$CASE.ir" ]; then
	printf 'missing IR file: build/project3-ir/%s.ir\n' "$CASE" >&2
	printf 'run make ir-test or generate it with CMM_IR=1 first.\n' >&2
	exit 1
fi

docker run -i --rm \
	-e QT_QPA_PLATFORM=offscreen \
	-e IR_CASE="$CASE" \
	-v "$ROOT:/work" \
	-w /work \
	"$IMAGE" python - <<'PY'
import marshal
import opcode
import os
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

case_inputs = {
    "A-1": [],
    "A-2": [50, 40, 20],
    "A-3": [2],
    "A-4": [3],
    "A-5": [6],
    "B-1": [1, 4, 3, 9, 2, 5],
    "B-2": [10],
    "B-3": [3],
    "C-1": [48, 18],
    "C-2": [-123],
    "E1-1": [],
    "E1-2": [3, 5],
    "E1-3": [2, 3, 5],
    "E2-1": [],
    "E2-2": [],
    "E2-3": [5],
}

case = os.environ["IR_CASE"]
values = list(case_inputs.get(case, []))


def get_int(*args, **kwargs):
    if not values:
        raise RuntimeError(f"{case}: input exhausted")
    return values.pop(0), True


QInputDialog.getInt = get_int
sim = namespace["IRSim"]()
sim.loadFile(f"build/project3-ir/{case}.ir")
sim.run()

print(sim.console.toPlainText().strip())
PY
