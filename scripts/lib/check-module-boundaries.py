#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
MODULES = {
    "app": {"runtime", "model", "config", "platform", "engine"},
    "runtime": {"engine", "config", "model", "platform"},
    "engine": {"config", "platform"},
    "model": {"config", "platform"},
    "config": {"platform"},
}
INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"')
violations: list[str] = []

if SRC.exists():
    for path in SRC.rglob("*.hpp"):
        rel = path.relative_to(SRC)
        owner = rel.parts[0] if len(rel.parts) > 1 else None
        if owner not in MODULES:
            continue
        for no, line in enumerate(path.read_text(encoding="utf-8", errors="ignore").splitlines(), start=1):
            match = INCLUDE.match(line)
            if not match:
                continue
            include = match.group(1)
            target = include.split("/", 1)[0]
            if target in MODULES and target != owner and target not in MODULES[owner]:
                violations.append(f"{path.relative_to(ROOT)}:{no}: {owner} may not include {target}")
    for path in SRC.rglob("*.cpp"):
        rel = path.relative_to(SRC)
        
        owner = rel.parts[0] if len(rel.parts) > 1 else None
        if owner not in MODULES:
            continue
        for no, line in enumerate(path.read_text(encoding="utf-8", errors="ignore").splitlines(), start=1):
            match = INCLUDE.match(line)
            if not match:
                continue
            include = match.group(1)
            target = include.split("/", 1)[0]
            if target in MODULES and target != owner and target not in MODULES[owner]:
                violations.append(f"{path.relative_to(ROOT)}:{no}: {owner} may not include {target}")

if violations:
    print("module-boundary violations:", file=sys.stderr)
    for item in violations:
        print(f"  - {item}", file=sys.stderr)
    sys.exit(1)
