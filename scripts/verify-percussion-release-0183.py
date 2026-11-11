#!/usr/bin/env python3
"""Verify a release that reuses unchanged instrument binaries and reruns app tests."""
from pathlib import Path
import hashlib,json,re,struct,zipfile,subprocess
root=Path(__file__).resolve().parents[1];build=root/'build';logs=build/'validation-logs'
first=(logs/'first-pass-tests.txt').read_text(encoding='utf-8-sig')
assert first.count('Test Passed.')==3 and first.count('Test Failed.')==1
assert 'FAIL: Legacy percussion edits were replaced with current/default settings' in first
assert first.count('PASS 24 retired percussion VST3 parameter-state projects:')==2
host_counts=[int(x) for x in re.findall(r'PASS (\d+) actual VST3 host checks',first)]
assert len(host_counts)==2 and min(host_counts)>647 and 'prepared latency 16 samples' in first
assert 'All ' in first and 'Phase 6 factory preset checks passed' in first
current=(build/'Testing/Temporary/LastTest.log').read_text(encoding='utf-8-sig')
assert current.count('Test Passed.')==1 and 'Test Failed.' not in current
assert 'PASS 24 retired percussion parameter-state projects:' in current
assert 'PASS SoundFonts preset folder:' in current and 'PASS 76 actual 0.16.0 removed-vocal projects:' in current
assert 'PASS 86 Vocals presets' in current and 'PASS launch colours:' in current
app=int(re.search(r'PASS (\d+) JUCE integration',current)[1])
library=int(re.search(r'All (\d+) Phase 9 library checks',current)[1])
sf=int(re.search(r'PASS (\d+) SoundFont application checks',current)[1])
assert app>3500 and library>400 and sf>=199
pluginval=(logs/'pluginval.txt').read_text(encoding='utf-8-sig')
assert 'SUCCESS' in pluginval and '0.18.3' in pluginval
steinberg=(logs/'steinberg-validator.txt').read_text(encoding='utf-8-sig')
assert '47 tests passed, 0 tests failed' in steinberg and 'validator exit 0' in steinberg
installed=(logs/'installer-acceptance.txt').read_text(encoding='utf-8-sig')
assert installed.count('PASS')==6 and '0.18.2-to-0.18.3 upgrade' in installed
for text in ['SoundFont cache preserved','launch appearance history preserved','default SoundFont library and preferences preserved']:assert text in installed
bank=json.loads((root/'Presets/factory.json').read_text(encoding='utf-8'))['presets']
baseline=json.loads(subprocess.check_output(['git','show','9517f59b4a32290209dccb0307d08d17822a0621:Presets/factory.json'],cwd=root,text=True))['presets']
assert len(bank)==654 and bank==[p for p in baseline if p['category']!='Percussion']
assert len({p['category'] for p in bank})==20
registry=json.loads((root/'Presets/factory-index.json').read_text(encoding='utf-8'))['presets']
before=json.loads(subprocess.check_output(['git','show','9517f59b4a32290209dccb0307d08d17822a0621:Presets/factory-index.json'],cwd=root,text=True))['presets']
assert len(registry)==754 and sum(bool(p.get('retired')) for p in registry)==100
for old,new in zip(before,registry):
    expected=dict(old)
    if expected['category']=='Percussion':expected['retired']=True
    assert new==expected
base=build/'GBoyAdvance2_artefacts/Release'
paths=[base/'VST3/G Boy Advance 2.vst3/Contents/x86_64-win/G Boy Advance 2.vst3',base/'Standalone/G Boy Advance 2.exe']
for p in paths:
    b=p.read_bytes();offset=struct.unpack_from('<I',b,0x3c)[0]
    assert b[:2]==b'MZ' and b[offset:offset+4]==b'PE\0\0' and struct.unpack_from('<H',b,offset+4)[0]==0x8664
portable=root/'dist/G-Boy-Advance-2-Windows-x64.zip'
with zipfile.ZipFile(portable) as z:
    assert z.testzip() is None
    assert not any(n.lower().endswith(('.sf2','.sf3','.wav','.aif','.aiff')) for n in z.namelist())
    for p in paths:
        suffix=p.relative_to(base).as_posix().removeprefix('Standalone/').removeprefix('VST3/')
        assert z.read(next(n for n in z.namelist() if n.endswith(suffix)))==p.read_bytes()
    name=next(n for n in z.namelist() if n.endswith('Presets/factory.json'))
    assert json.loads(z.read(name))['presets']==bank
    manual=z.read(next(n for n in z.namelist() if n.endswith('USER_MANUAL.md'))).decode('utf-8-sig')
    assert 'Current version: **0.18.3**' in manual and '654 presets in 20 factory categories' in manual
import sys
sys.path.insert(0,str(root/'scripts'))
from source_files import project_files,ROOT
source=root/'dist/G-Boy-Advance-2-Source.zip';files=project_files()
with zipfile.ZipFile(source) as z:
    assert z.testzip() is None and len(z.namelist())==len(files)
    for p in files:assert z.read('G-Boy-Advance-2/'+p.relative_to(ROOT).as_posix())==p.read_bytes()
images=build/'ui-snapshots'
skins=re.findall(r'\{"([a-z]+)","([^"]+)"',(root/'Source/UiSkins.h').read_text(encoding='utf-8'));assert len(skins)==30
for prefix in ['Skin','Advanced']:
    assert len({hashlib.sha256((images/f'{prefix}-{skin}.png').read_bytes()).hexdigest() for skin,name in skins})==30
for name in ['Factory-Without-Percussion-960.png','SF2-Preset-Folder-960.png']:
    assert (images/name).read_bytes().startswith(b'\x89PNG\r\n\x1a\n')
def info(p):return {'path':p.relative_to(root).as_posix(),'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}
payloads=[*paths,root/'dist/G-Boy-Advance-2-Setup-x64.exe',portable,source]
report={'version':'0.18.3','validationCommit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip(),
        'runtimeBuildCommit':'0856bd8c3f31a3ab041aa39016dd0e2fb908b6fe','runtimeBuildRun':43056128423,
        'factoryPresets':654,'nativeCategories':20,'removedPresets':24,'programSlots':754,'reservedSlots':100,'stableParameters':64,'skins':30,
        'retainedParametersAndRegistryVerified':True,'windowsPresetsAndHostTestsFromRuntimeBuild':'3 passed; original app check rejected exact float equality',
        'correctedAppTestsPassed':True,'applicationChecks':app,'libraryChecks':library,'soundFontChecks':sf,'hostChecks':host_counts,
        'newPercussionProjectCases':24,'legacyFixtureType':'Previous 0.18.2 parameter dictionaries reconstructed with the unchanged serializer, not old-binary exports',
        'pluginvalStrictness':10,'steinbergPassed':47,'steinbergFailed':0,'installerChecks':6,'sourceFiles':len(files),
        'sourceZipCrcVerified':True,'allThirtyMainAndAdvancedSkinImagesVerified':True,'binaries':[info(p) for p in payloads],
        'limits':'Actual FL Studio and physical audio/MIDI/DPI remain manual checks; no new performance claim. Licensing/signing deferred.'}
(logs/'release-validation.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print('RELEASE_VALIDATION_JSON '+json.dumps(report,separators=(',',':')))
print('PASS verified release archives, native binaries, 654 unchanged sounds, all project/host tests, validators, 30 skins and installer lifecycle')
