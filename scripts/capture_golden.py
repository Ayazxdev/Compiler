#!/usr/bin/env python3
"""
capture_golden.py — Golden-corpus capture script.

Runs every .pinky script through the reference Python pinky interpreter and
saves its exact stdout to tests/golden/<name>.expected.

Must be run from the CD/ directory:
    python scripts/capture_golden.py

The reference interpreter lives in ../pinky/pinky.py (never modified).
Golden .expected files are committed to the repo as the correctness spec.
"""

import io
import os
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
PINKY_DIR = REPO_ROOT / "pinky"
SCRIPTS_DIR = PINKY_DIR / "scripts"
GOLDEN_DIR = Path(__file__).resolve().parent.parent / "CD" / "tests" / "golden"

GOLDEN_DIR.mkdir(parents=True, exist_ok=True)


def capture(script: Path) -> tuple[str, bool]:
    """Run script through Python pinky interpreter, return (output, success)."""
    env = os.environ.copy()
    env["PYTHONPATH"] = str(PINKY_DIR)
    result = subprocess.run(
        [sys.executable, "-c",
         f"""
import sys
sys.path.insert(0, r'{PINKY_DIR}')
from lexer import Lexer
from parser import Parser
from interpreter import Interpreter
with open(r'{script}') as f:
    src = f.read()
tokens = Lexer(src).tokenize()
ast = Parser(tokens).parse()
Interpreter().interpret_ast(ast)
"""],
        capture_output=True,
        text=True,
    )
    if result.returncode == 0:
        return result.stdout, True
    else:
        # Capture the error output as the golden value so the C++ toolchain can match it once those features are implemented.
        combined = result.stderr.strip() + result.stdout.strip()
        return combined + "\n", False


def main() -> None:
    scripts = sorted(SCRIPTS_DIR.glob("*.pinky"))
    if not scripts:
        print(f"No .pinky scripts found in {SCRIPTS_DIR}", file=sys.stderr)
        sys.exit(1)

    for script in scripts:
        name = script.stem
        output, ok = capture(script)
        dest = GOLDEN_DIR / f"{name}.expected"
        dest.write_text(output, encoding="utf-8")
        status = "OK " if ok else "ERR"
        print(f"{status}  {name:30s}  -> {dest.relative_to(REPO_ROOT)}  ({len(output)} bytes)")

    print(f"\nGolden corpus: {len(scripts)} file(s) captured to {GOLDEN_DIR}")


if __name__ == "__main__":
    main()
