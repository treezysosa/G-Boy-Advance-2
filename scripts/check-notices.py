#!/usr/bin/env python3
"""Validate retained upstream license text, permitting only checkout newline conversion."""
import hashlib
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1] / 'docs' / 'licenses'
manifest = json.loads((root / 'manifest.json').read_text())
for item in manifest['files']:
    content = (root / item['file']).read_bytes()
    lf = content.replace(b'\r\n', b'\n')
    candidates = (content, lf, lf.replace(b'\n', b'\r\n'))
    hashes = {hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest() for data in candidates}
    if item['gitBlobSha'] not in hashes:
        raise SystemExit('Upstream notice changed: ' + item['file'])
print(f"PASS {len(manifest['files'])} pinned upstream notices (checkout newline conversion permitted)")
