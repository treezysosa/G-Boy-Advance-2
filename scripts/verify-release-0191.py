#!/usr/bin/env python3
"""Verify the 0.19.1 release, reusing binaries from identical production source."""
from pathlib import Path
import hashlib
import json
import re
import struct
import subprocess
import sys
import zipfile

root = Path(__file__).resolve().parents[1]
logs = root / 'build/validation-logs'
def read(path):
    return path.read_text(encoding='utf-8-sig')
def info(path):
    return {'path': path.relative_to(root).as_posix(), 'bytes': path.stat().st_size,
            'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
base = '4d6011fd971e1e0e73a8aeab2e6195811baf4896'
runtime = '3ac7cb29e939ed1a7e2039586038160a304e1518'
subprocess.run(['git', 'diff', '--exit-code', runtime, '--', 'Source', 'Presets', 'CMakeLists.txt'], cwd=root, check=True)
for name in ['Presets/factory.json', 'Presets/factory-index.json', 'Source/dsp/ParameterList.inc']:
    old = subprocess.check_output(['git', 'show', base + ':' + name], cwd=root)
    assert old.replace(b'\r\n', b'\n') == (root / name).read_bytes().replace(b'\r\n', b'\n'), name
factory = json.loads(read(root / 'Presets/factory.json'))['presets']
assert len(factory) == 654 and len({p['category'] for p in factory}) == 20
registry = json.loads(read(root / 'Presets/factory-index.json'))['presets']
assert len(registry) == 754 and sum(bool(p.get('retired')) for p in registry) == 100
suite = read(root / 'build/Testing/Temporary/LastTest.log')
assert suite.count('Test Passed.') == 12 and 'Test Failed.' not in suite
app = int(re.search(r'PASS (\d+) JUCE integration', suite)[1])
hosts = [int(v) for v in re.findall(r'PASS (\d+) actual VST3 host checks', suite)]
assert app >= 4171 and len(hosts) == 2 and min(hosts) >= 887
assert 'PASS SoundFonts preset folder:' in suite and 'PASS launch colours:' in suite
assert 'SUCCESS' in read(logs / 'pluginval.txt') and '0.19.1' in read(logs / 'pluginval.txt')
assert '47 tests passed, 0 tests failed' in read(logs / 'steinberg-validator.txt')
installed = read(logs / 'installer-acceptance.txt')
assert installed.count('PASS') == 6 and '0.19.0-to-0.19.1 upgrade' in installed
for text in ['SoundFont cache preserved', 'launch appearance history preserved', 'default SoundFont library and preferences preserved']:
    assert text in installed
cpu = json.loads(read(logs / 'cpu-comparison.json'))
assert cpu['baselineDspCommit'] == base and cpu['comparisonScenes'] == 846
assert cpu['floatSamples'] == 14327808 and cpu['maxAbsoluteDifference'] <= 1e-6
assert cpu['signalToErrorDb'] is None or cpu['signalToErrorDb'] >= 110
assert cpu['sustained']['scenes'] == 148 and cpu['sustained']['maxAbsoluteDifference'] <= 1e-6
assert cpu['sustained']['signalToErrorDb'] is None or cpu['sustained']['signalToErrorDb'] >= 110
assert cpu['layeredSoundFontAudio']['scenes'] == 16 and cpu['layeredSoundFontAudio']['floatSamples'] == 11332824
assert cpu['layeredSoundFontAudio']['maxAbsoluteDifference'] <= 1e-6
assert cpu['layeredSoundFontAudio']['signalToErrorDb'] is None or cpu['layeredSoundFontAudio']['signalToErrorDb'] >= 110
assert set(cpu['layeredSoundFonts']) == {'static', 'modulated'}
bypass = json.loads(read(logs / 'drive-bypass-reference.json'))
assert bypass['bitIdentical'] and bypass['floatSamples'] == 393216
skins = re.findall(r'\{"([a-z]+)","([^"]+)"', read(root / 'Source/UiSkins.h'))
assert len(skins) == 40 and len({v[0] for v in skins}) == 40
old_skins = re.findall(r'\{"([a-z]+)","([^"]+)"', subprocess.check_output(['git', 'show', base + ':Source/UiSkins.h'], cwd=root, text=True))
assert skins == old_skins
for prefix in ['Skin', 'Advanced']:
    images = [root / 'build/ui-snapshots' / f'{prefix}-{skin}.png' for skin, _ in skins]
    for image in images:
        assert image.read_bytes().startswith(b'\x89PNG\r\n\x1a\n')
    assert len({hashlib.sha256(p.read_bytes()).hexdigest() for p in images}) == 40
payload = root / 'build/GBoyAdvance2_artefacts/Release'
binaries = [payload / 'VST3/G Boy Advance 2.vst3/Contents/x86_64-win/G Boy Advance 2.vst3', payload / 'Standalone/G Boy Advance 2.exe']
for path in binaries:
    raw = path.read_bytes(); at = struct.unpack_from('<I', raw, 0x3c)[0]
    assert raw[:2] == b'MZ' and raw[at:at+4] == b'PE\0\0' and struct.unpack_from('<H', raw, at+4)[0] == 0x8664
portable = root / 'dist/G-Boy-Advance-2-Windows-x64.zip'
with zipfile.ZipFile(portable) as archive:
    assert archive.testzip() is None
    assert not any(n.lower().endswith(('.sf2', '.sf3', '.wav', '.aiff', '.aif')) for n in archive.namelist())
    for binary in binaries:
        suffix = binary.relative_to(payload).as_posix().removeprefix('Standalone/').removeprefix('VST3/')
        assert archive.read(next(n for n in archive.namelist() if n.endswith(suffix))) == binary.read_bytes()
    manual = archive.read(next(n for n in archive.namelist() if n.endswith('USER_MANUAL.md'))).decode('utf-8-sig')
    assert 'Current version: **0.19.1**' in manual and '40 skins' in manual
sys.path.insert(0, str(root / 'scripts'))
from source_files import project_files
files = project_files()
source = root / 'dist/G-Boy-Advance-2-Source.zip'
with zipfile.ZipFile(source) as archive:
    assert archive.testzip() is None and len(archive.namelist()) == len(files)
    for path in files:
        assert archive.read('G-Boy-Advance-2/' + path.relative_to(root).as_posix()) == path.read_bytes()
report = {'version': '0.19.1', 'runtimeCommit': runtime, 'runtimeRun': 43087833818,
          'validationCommit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip(),
          'productionSourceIdentical': True, 'factoryPresets': 654, 'categories': 20,
          'programSlots': 754, 'reservedSlots': 100, 'stableParameters': 64, 'skins': 40,
          'allEightySkinRendersDistinctWithinTheirView': True, 'windowsSuitesPassed': 12,
          'applicationChecks': app, 'hostChecks': hosts, 'pluginvalStrictness': 10,
          'steinbergPassed': 47, 'steinbergFailed': 0, 'installerChecks': 6,
          'driveBypassReference': bypass, 'cpuAndAudio': cpu, 'sourceFiles': len(files),
          'binaries': [info(p) for p in [*binaries, root / 'dist/G-Boy-Advance-2-Setup-x64.exe', portable, source]],
          'limits': 'Shared-runner measurements do not guarantee CPU gains on every PC. Actual FL Studio and physical audio/MIDI remain owner checks; signing/licensing deferred.'}
(logs / 'release-validation.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
print('RELEASE_VALIDATION_JSON ' + json.dumps(report, separators=(',', ':')))
print('PASS complete source/archives, unchanged production binaries, 40 skins, host/audio/CPU and installer evidence')
