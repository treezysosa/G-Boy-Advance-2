#!/usr/bin/env python3
"""Package the complete buildable source tree, excluding local build/dependency caches."""
import hashlib
import zipfile
from source_files import ROOT, REQUIRED, project_files

files = project_files()
output = ROOT / 'dist/G-Boy-Advance-2-Source.zip'
output.parent.mkdir(exist_ok=True)
prefix = 'G-Boy-Advance-2/'
with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED) as archive:
    for path in files:
        archive.write(path, prefix + path.relative_to(ROOT).as_posix())
# Reopen only after the writer and its buffers/central directory are closed.
# Reading through an active Windows ZIP writer can produce false CRC failures.
with zipfile.ZipFile(output, 'r') as archive:
    assert REQUIRED <= {name.removeprefix(prefix) for name in archive.namelist()}
    assert archive.testzip() is None
print(f'{output}: {len(files)} source/assets, {output.stat().st_size} bytes')
print('SHA256 ' + hashlib.sha256(output.read_bytes()).hexdigest())
