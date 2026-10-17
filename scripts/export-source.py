#!/usr/bin/env python3
"""Read-only helper to export source through the connected repository tool."""
import json
from pathlib import Path
import sys
root=Path(__file__).resolve().parents[1]
files=[p for p in root.rglob('*') if p.is_file() and not any(part.startswith('build') or part in {'.git','Dependencies','dist','out','__pycache__'} for part in p.relative_to(root).parts) and p.suffix not in {'.wav','.png','.csv'}]
if len(sys.argv)==1:
    print(json.dumps([{'path':p.relative_to(root).as_posix(),'bytes':len(p.read_text())} for p in sorted(files)]))
else:
    path=root/sys.argv[1]
    if path not in files:raise ValueError('Not a source file')
    offset=int(sys.argv[2]);print(json.dumps(path.read_text()[offset:offset+20000]))
