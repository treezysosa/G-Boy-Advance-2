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

for item in manifest.get("additionalDependencies", []):
    data=(root.parents[1]/item["source"]).read_bytes()
    lf=data.replace(b"\r\n",b"\n")
    variants=(data,lf,lf.replace(b"\n",b"\r\n"))
    hashes={hashlib.sha1(b"blob "+str(len(d)).encode()+b"\0"+d).hexdigest() for d in variants}
    if item["gitBlobSha"] not in hashes:
        raise SystemExit("Pinned dependency source changed: "+item["source"])
print("PASS pinned SoundFont decoder source")
