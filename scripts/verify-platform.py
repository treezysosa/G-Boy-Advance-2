#!/usr/bin/env python3
"""Verify packaged platform identity and test evidence, without DAW claims."""
import argparse
import hashlib
import json
import plistlib
from pathlib import Path
import re
import struct
import subprocess
import sys
import time

ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--platform',choices=['windows-x86','macos'],required=True)
parser.add_argument('--build',type=Path,required=True)
parser.add_argument('--installed',action='store_true')
args=parser.parse_args()
build=args.build.resolve();logs=build/'validation-logs';logs.mkdir(exist_ok=True)
suite=(build/'Testing/Temporary/LastTest.log').read_text(encoding='utf-8-sig')
expected=12 if args.platform=='windows-x86' else 11
assert suite.count('Test Passed.')==expected and 'Test Failed.' not in suite
factory=json.loads((ROOT/'Presets/factory.json').read_text())['presets']
assert len(factory)==654 and len({p['category'] for p in factory})==20
assert len(re.findall(r'\{"[a-z]+","[^"]+"',(ROOT/'Source/UiSkins.h').read_text()))==50
report={'platform':args.platform,'version':'0.20.0','suitesPassed':expected,'factoryPresets':654,'skins':50,'binaries':[],
        'limits':'Native automated tests and validators; owner DAW, hardware and interactive installation remain acceptance work.'}
if args.platform=='windows-x86':
    folder=build/'GBoyAdvance2_artefacts/Release'
    paths=[folder/'VST3/G Boy Advance 2.vst3/Contents/x86-win/G Boy Advance 2.vst3',folder/'Standalone/G Boy Advance 2.exe']
    for path in paths:
        raw=path.read_bytes();offset=struct.unpack_from('<I',raw,0x3c)[0]
        assert raw[:2]==b'MZ' and raw[offset:offset+4]==b'PE\0\0' and struct.unpack_from('<H',raw,offset+4)[0]==0x14c
        report['binaries'].append({'path':str(path.relative_to(ROOT)),'machine':'x86','sha256':hashlib.sha256(raw).hexdigest()})
    assert '47 tests passed, 0 tests failed' in (logs/'steinberg-validator.txt').read_text(encoding='utf-8-sig')
    report['steinbergPassed']=47
else:
    native=subprocess.check_output(['uname','-m'],text=True).strip();assert native in ('arm64','x86_64');report['executedArchitecture']=native
    base=Path('/') if args.installed else build/'package-root'
    bundles=[base/'Library/Audio/Plug-Ins/VST3/G Boy Advance 2.vst3',base/'Library/Audio/Plug-Ins/Components/G Boy Advance 2.component',base/'Applications/G Boy Advance 2.app']
    for bundle in bundles:
        info=plistlib.loads((bundle/'Contents/Info.plist').read_bytes());path=bundle/'Contents/MacOS'/info['CFBundleExecutable']
        archs=subprocess.check_output(['lipo','-archs',str(path)],text=True).split();assert set(archs)=={'x86_64','arm64'}
        assert info['CFBundleShortVersionString']=='0.20.0'
        subprocess.run(['codesign','--verify','--deep','--strict',str(bundle)],check=True)
        report['binaries'].append({'path':str(bundle),'architectures':archs,'sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
    assert 'SUCCESS' in (logs/f'pluginval-{native}.txt').read_text()
    assert 'AU VALIDATION SUCCEEDED' in (logs/f'auval-{native}.txt').read_text()
    app=bundles[-1];info=plistlib.loads((app/'Contents/Info.plist').read_bytes());binary=app/'Contents/MacOS'/info['CFBundleExecutable']
    with (logs/f'standalone-{native}.txt').open('w') as output:
        process=subprocess.Popen(['arch','-'+native,str(binary)],stdout=output,stderr=subprocess.STDOUT)
        try:
            time.sleep(3);assert process.poll() is None,'Standalone exited during launch smoke test'
        finally:
            process.terminate()
            try:process.wait(timeout=10)
            except subprocess.TimeoutExpired:process.kill();process.wait()
    report.update(pluginvalStrictness=10,audioUnitValidationPassed=True,standaloneLaunchSmokePassed=True,signing='ad-hoc; not Developer ID notarized')
(logs/'platform-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
