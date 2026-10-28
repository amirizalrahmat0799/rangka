#!/usr/bin/env python3
"""Generates every combination of options and checks that each JSON, YAML, TOML and XML file in them parses.

    python3 scripts/check_syntax.py path/to/rangka
"""

import itertools
import json
import subprocess
import sys
import tempfile
import tomllib
import xml.etree.ElementTree as ET
from pathlib import Path

import yaml  # pip install pyyaml

rangka = sys.argv[1] if len(sys.argv) > 1 else "build/rangka"
frontends = ["none", "react", "vue"]
backends = ["none", "spring", "express", "fastapi", "go", "cpp"]
databases = ["none", "postgres"]

checked = combos = 0
failures = []
with tempfile.TemporaryDirectory() as tmp:
    for i, (f, b, d) in enumerate(itertools.product(frontends, backends, databases)):
        if f == "none" and b == "none":
            continue
        if b == "none" and d == "postgres":
            continue
        combos += 1
        name = f"p{i}"
        subprocess.run([rangka, "new", name, "-f", f, "-b", b, "-d", d, "-o", tmp, "-y"],
                       check=True, capture_output=True)
        for path in sorted(Path(tmp, name).rglob("*")):
            if not path.is_file():
                continue
            text = path.read_text(encoding="utf-8")
            try:
                if path.suffix == ".json":
                    json.loads(text)
                elif path.suffix in (".yml", ".yaml"):
                    yaml.safe_load(text)
                elif path.suffix == ".toml":
                    tomllib.loads(text)
                elif path.suffix == ".xml":
                    ET.fromstring(text)
                else:
                    continue
                checked += 1
            except Exception as e:
                failures.append(f"{f}/{b}/{d}: {path.relative_to(tmp)}: {e}")

for line in failures:
    print("FAIL", line)
print(f"{combos} combinations, {checked} structured files parsed, {len(failures)} failed")
sys.exit(1 if failures else 0)
