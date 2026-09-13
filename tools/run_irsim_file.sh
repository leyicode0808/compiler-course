#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
IMAGE=cmm-irsim-py38

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
	printf 'Usage: %s <input.ir> [stdin-text]\n' "$0" >&2
	exit 1
fi

INPUT_IR="$1"
STDIN_TEXT="${2:-}"

case "$INPUT_IR" in
	/*) SRC_IR="$INPUT_IR" ;;
	*) SRC_IR="$ROOT/$INPUT_IR" ;;
esac

if [ ! -f "$SRC_IR" ]; then
	printf 'IR file not found: %s\n' "$INPUT_IR" >&2
	exit 1
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

if [ ! -f "$ROOT/build/irsim/irsim/irsim.pyc" ]; then
	printf 'missing IRSim files; expected build/irsim/irsim/irsim.pyc\n' >&2
	printf 'course archive should be available at proj3/course/irsim.zip in submission.\n' >&2
	exit 1
fi

if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
	printf 'missing docker image: %s\n' "$IMAGE" >&2
	printf 'run ./tools/build_irsim_image.sh before running this command.\n' >&2
	exit 1
fi

mkdir -p "$ROOT/build/manual-irsim"
CASE_IR="$ROOT/build/manual-irsim/input.ir"
cp "$SRC_IR" "$CASE_IR"

docker run -i --rm \
	-e QT_QPA_PLATFORM=offscreen \
	-e IR_INPUT="$STDIN_TEXT" \
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

raw_input = os.environ.get("IR_INPUT", "").replace("\\n", "\n")
values = [int(line) for line in raw_input.splitlines() if line.strip()]


def get_int(*args, **kwargs):
    if not values:
        raise RuntimeError("IR input exhausted")
    return values.pop(0), True


QInputDialog.getInt = get_int
sim = namespace["IRSim"]()
sim.loadFile("build/manual-irsim/input.ir")
sim.run()
print(sim.console.toPlainText().strip())
PY
