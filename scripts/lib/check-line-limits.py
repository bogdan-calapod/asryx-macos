#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
LIMITS = {
    ".cpp": 350,
    ".hpp": 260,
    ".c": 350,
    ".h": 260,
}
FUNCTION_HINT_LIMIT = 50

violations: list[str] = []
for base in (ROOT / "src", ROOT / "tests"):
    if not base.exists():
        continue
    for path in base.rglob("*"):
        if path.suffix not in LIMITS:
            continue
        lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
        physical = len(lines)
        limit = LIMITS[path.suffix]
        if physical > limit:
            violations.append(f"{path.relative_to(ROOT)} has {physical} lines; limit is {limit}")

if violations:
    print("line-limit violations:", file=sys.stderr)
    for item in violations:
        print(f"  - {item}", file=sys.stderr)
    sys.exit(1)
