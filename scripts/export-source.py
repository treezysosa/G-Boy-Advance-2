#!/usr/bin/env python3
"""Read-only UTF-8/base64 source export for the connected repository tool."""
import base64
import hashlib
import json
import sys
from source_files import ROOT, project_files

files = project_files()


def payload(path):
    data = path.read_bytes()
    try:
        return data.decode('utf-8'), False
    except UnicodeDecodeError:
        return base64.b64encode(data).decode('ascii'), True


if len(sys.argv) == 1:
    manifest = []
    for path in files:
        data = path.read_bytes()
        content, binary = payload(path)
        manifest.append(dict(path=path.relative_to(ROOT).as_posix(), length=len(content),
                             binary=binary, sha=hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()))
    print(json.dumps(manifest))
else:
    path = ROOT / sys.argv[1]
    if path not in files:
        raise ValueError('Not a project source file')
    content, _ = payload(path)
    offset = int(sys.argv[2])
    print(json.dumps(content[offset:offset + 12000]))
