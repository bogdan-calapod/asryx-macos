#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
version = (ROOT / "VERSION").read_text(encoding="utf-8").strip()
violations: list[str] = []

cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
match = re.search(r"project\([^)]*VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)", cmake, re.S)
if not match or match.group(1) != version:
    violations.append("CMakeLists.txt project VERSION must match VERSION")

if violations:
    print("version violations:", file=sys.stderr)
    for item in violations:
        print(f"  - {item}", file=sys.stderr)
    sys.exit(1)
