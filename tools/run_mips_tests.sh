#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"

python3 - <<'PY'
import os
import re
import subprocess
from pathlib import Path

root = Path(".")
parser = root / "build" / "cmmc"
out_dir = root / "build" / "project4-mips"
out_dir.mkdir(parents=True, exist_ok=True)
p3_inputs = root / "tests" / "project3" / "inputs"
p4_examples = root / "examples" / "project4"
if not p3_inputs.is_dir() and (root / "proj3" / "tests" / "project3" / "inputs").is_dir():
    p3_inputs = root / "proj3" / "tests" / "project3" / "inputs"
if not p4_examples.is_dir() and (root / "proj4" / "examples" / "project4").is_dir():
    p4_examples = root / "proj4" / "examples" / "project4"

cases = [
    ("A-1", p3_inputs / "A-1.cmm", [], [120, 325]),
    ("A-2", p3_inputs / "A-2.cmm", [50, 40, 20], [7, 1000, 3000]),
    ("A-3", p3_inputs / "A-3.cmm", [2], [3, 7, 28]),
    ("A-4", p3_inputs / "A-4.cmm", [3], [24]),
    ("A-5", p3_inputs / "A-5.cmm", [6], [8]),
    ("B-1", p3_inputs / "B-1.cmm", [1, 4, 3, 9, 2, 5], [3, 9, 19]),
    ("B-2", p3_inputs / "B-2.cmm", [10], [2]),
    ("B-3", p3_inputs / "B-3.cmm", [3], [693, 11]),
    ("C-1", p3_inputs / "C-1.cmm", [48, 18], [6]),
    ("C-2", p3_inputs / "C-2.cmm", [-123], [6]),
    ("E1-1", p3_inputs / "E1-1.cmm", [], [42, 13]),
    ("E1-2", p3_inputs / "E1-2.cmm", [3, 5], [12]),
    ("E1-3", p3_inputs / "E1-3.cmm", [2, 3, 5], [8]),
    ("E2-1", p3_inputs / "E2-1.cmm", [], [31, 6]),
    ("E2-2", p3_inputs / "E2-2.cmm", [], [12, 21]),
    ("E2-3", p3_inputs / "E2-3.cmm", [5], [0, 27]),
    ("P4-fibonacci", p4_examples / "fibonacci.cmm", [7], [1, 1, 2, 3, 5, 8, 13]),
    ("P4-factorial", p4_examples / "factorial.cmm", [7], [5040]),
]

failed = 0
for name, src, inputs, expected in cases:
    asm = out_dir / f"{name}.s"
    gen = subprocess.run([str(parser), str(src), str(asm)], text=True, capture_output=True)
    if gen.returncode != 0:
        print(f"fail: {name} -> compiler exited {gen.returncode}")
        if gen.stderr:
            print(gen.stderr.strip())
        failed += 1
        continue
    stdin = "".join(f"{value}\n" for value in inputs)
    run = subprocess.run(["spim", "-quiet", "-file", str(asm)], input=stdin, text=True, capture_output=True)
    text = run.stdout + run.stderr
    actual = [int(line) for line in text.splitlines() if re.fullmatch(r"-?\d+", line.strip())]
    if run.returncode == 0 and actual == expected:
        print(f"ok: {name} -> {actual}")
    else:
        print(f"fail: {name} -> {actual}, expected {expected}")
        if run.returncode != 0:
            print(f"spim exited {run.returncode}")
        failed += 1

print(f"mips tests: {len(cases) - failed} passed, {failed} failed")
raise SystemExit(failed)
PY
