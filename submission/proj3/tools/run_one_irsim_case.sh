#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
CASE="${1:-proj3-test1}"
IMAGE=${IRSIM_IMAGE:-cmm-irsim-py38}
OUT_DIR="$ROOT/build/project3-ir"

case "$CASE" in
	proj3-4-3-6-exp1-1) SRC="$ROOT/test/proj3-4-3-6-exp1-1.cmm"; INPUT="7\n" ;;
	proj3-4-3-6-exp2-1) SRC="$ROOT/test/proj3-4-3-6-exp2-1.cmm"; INPUT="7\n" ;;
	proj3-4-3-7-exp1-0) SRC="$ROOT/test/proj3-4-3-7-exp1-0.cmm"; INPUT="" ;;
	proj3-4-3-7-exp2-0) SRC="$ROOT/test/proj3-4-3-7-exp2-0.cmm"; INPUT="" ;;
	proj3-test1) SRC="$ROOT/test/proj3-test1.cmm"; INPUT="" ;;
	*)
		printf 'unknown case: %s\n' "$CASE" >&2
		printf 'examples: proj3-4-3-6-exp1-1, proj3-4-3-6-exp2-1, proj3-4-3-7-exp1-0, proj3-4-3-7-exp2-0, proj3-test1\n' >&2
		exit 1
		;;
esac

if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
	printf 'missing docker image: %s\n' "$IMAGE" >&2
	printf 'run ./tools/build_irsim_image.sh before running IRSim.\n' >&2
	exit 1
fi

mkdir -p "$OUT_DIR" "$ROOT/build/irsim"
if [ ! -f "$ROOT/build/irsim/irsim/irsim.pyc" ]; then
	unzip -q "$ROOT/course/irsim.zip" -d "$ROOT/build/irsim"
fi

CMM_IR=1 "$ROOT/build/cmmc" "$SRC" "$OUT_DIR/$CASE.ir"
printf 'generated: %s\n' "build/project3-ir/$CASE.ir" >&2

docker run -i --rm \
	-e QT_QPA_PLATFORM=offscreen \
	-v "$ROOT:/work" \
	-w /work \
	"$IMAGE" python - "$CASE" "$INPUT" <<'PY'
import marshal
import opcode
import sys

case = sys.argv[1]
raw_input = sys.argv[2]
inputs = [int(x) for x in raw_input.splitlines() if x.strip()]

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

values = list(inputs)

def get_int(*args, **kwargs):
    if not values:
        raise RuntimeError(f"{case}: input exhausted")
    return values.pop(0), True

QInputDialog.getInt = get_int
sim = namespace["IRSim"]()
sim.loadFile(f"build/project3-ir/{case}.ir")
sim.run()
text = sim.console.toPlainText().strip()
if text:
    print(text)
PY
