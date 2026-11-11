#!/usr/bin/env python3
"""Compare drive bypass against an exported reference source tree, off the audio thread."""
import argparse
import hashlib
import json
from pathlib import Path
import shlex
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('baseline', type=Path)
parser.add_argument('--compiler', default='g++', help='C++20 compiler, for example g++ or cl in a VS developer shell')
parser.add_argument('--report', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
baseline = args.baseline.resolve()
compiler = shlex.split(args.compiler)
msvc = Path(compiler[0]).name.lower() in {'cl', 'cl.exe'}
report = {'frames': 196608, 'floatSamples': 393216, 'rates': [44100, 48000, 96000],
          'bypassLengths': [0, 1, 17, 63, 64, 65, 128, 512], 'hashes': {}}
with tempfile.TemporaryDirectory(prefix='gba2-drive-reference-') as folder:
    work = Path(folder)
    renders = []
    for label, source in [('before', baseline), ('after', root)]:
        effects = source / 'Source/dsp/Effects.cpp'
        if not effects.is_file():
            raise SystemExit(f'Missing reference effects source: {effects}')
        binary = work / (label + ('.exe' if msvc else ''))
        probe = root / 'Tests/BypassHistoryProbe.cpp'
        if msvc:
            command = [*compiler, '/nologo', '/std:c++20', '/O2', '/EHsc', '/MT',
                       '/I' + str(source / 'Source'), str(effects), str(probe), '/Fe:' + str(binary)]
        else:
            command = [*compiler, '-std=c++20', '-O3', '-DNDEBUG', '-I' + str(source / 'Source'),
                       str(effects), str(probe), '-o', str(binary)]
        subprocess.run(command, cwd=work, check=True)
        audio = work / (label + '.f32')
        subprocess.run([str(binary), str(audio)], check=True)
        data = audio.read_bytes()
        if len(data) != report['floatSamples'] * 4:
            raise SystemExit('Incomplete drive bypass reference render.')
        report['hashes'][label] = hashlib.sha256(data).hexdigest()
        renders.append(data)
    report['bitIdentical'] = renders[0] == renders[1]
if not report['bitIdentical']:
    raise SystemExit('Drive sleep/reconstruction differs from the continuously warmed reference.')
if args.report:
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
print('PASS drive-bypass reference: ' + json.dumps(report, separators=(',', ':')))
