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
# Test files carry per-test fixtures (state, hooks, helpers) that add a
# substantial constant baseline. Allow them more vertical headroom.
TEST_LIMITS = {
    ".cpp": 500,
    ".hpp": 400,
    ".c": 500,
    ".h": 400,
}

violations: list[str] = []
checked = 0

for base in (ROOT / "src", ROOT / "tests"):
    if not base.exists():
        continue

    is_tests = base.name == "tests"
    table = TEST_LIMITS if is_tests else LIMITS
    for path in base.rglob("*"):
        if path.suffix not in table:
            continue

        checked += 1
        lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
        limit = table[path.suffix]
        if len(lines) > limit:
            violations.append(
                f"{path.relative_to(ROOT)} has {len(lines)} lines; limit is {limit}"
            )

if violations:
    print("line-limit violations:", file=sys.stderr)
    for item in violations:
        print(f"  - {item}", file=sys.stderr)
    sys.exit(1)

print(f"line limits ok ({checked} files checked)")