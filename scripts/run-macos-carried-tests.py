#!/usr/bin/env python3
"""Run the same universal test binaries natively on a second Mac CPU type."""
from pathlib import Path
import subprocess
import sys

build=Path(sys.argv[1]).resolve()
architecture=sys.argv[2]
assert architecture in ('arm64','x86_64')
assert subprocess.check_output(['uname','-m'],text=True).strip()==architecture
names=['gba2-tests','gba2-core-tests','gba2-advanced-tests','gba2-effects-tests','gba2-vocal-tests',
       'gba2-retro-tests','gba2-preset-tests','gba2-preset-diversity-tests','gba2-soundfont-tests',
       'gba2-integration','gba2-host-tests']
log=build/'Testing/Temporary/LastTest.log'
with log.open('w') as output:
    for name in names:
        command=['arch','-'+architecture,str(build/'native-tests'/name)]
        if name=='gba2-host-tests':command.append('/Library/Audio/Plug-Ins/VST3/G Boy Advance 2.vst3')
        result=subprocess.run(command,cwd=build,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=600)
        output.write(name+'\n'+result.stdout+'\n');output.flush()
        print(name, 'passed' if result.returncode==0 else 'FAILED',flush=True)
        if result.returncode:
            output.write('Test Failed.\n');print(result.stdout);raise SystemExit(result.returncode)
        output.write('Test Passed.\n');output.flush()
