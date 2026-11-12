#!/usr/bin/env python3
"""Check the performance release's archives against the validated Windows runtime."""
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
runtime = '2293291be4241901915e0518bf89420e1591d596'
baseline = 'f62e8e02d33bb75013ec1daa00584baf4368ff6c'

def read(path):
    return path.read_text(encoding='utf-8-sig')

def info(path):
    return {'path': path.relative_to(root).as_posix(), 'bytes': path.stat().st_size,
            'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}

subprocess.run(['git', 'diff', '--exit-code', runtime, '--', 'Source', 'Presets', 'CMakeLists.txt'], cwd=root, check=True)
subprocess.run(['git', 'diff', '--exit-code', baseline, '--', 'Source/FactoryData.h', 'Presets/factory.json', 'Presets/factory-index.json', 'Source/dsp/ParameterList.inc', 'Source/UiSkins.h', 'Source/ThemeArt.h', 'Source/PluginEditor.cpp'], cwd=root, check=True)
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
pluginval = read(logs / 'pluginval.txt')
assert 'SUCCESS' in pluginval and '0.19.3' in pluginval
assert '47 tests passed, 0 tests failed' in read(logs / 'steinberg-validator.txt')
installed = read(logs / 'installer-acceptance.txt')
assert installed.count('PASS') == 6 and '0.19.2-to-0.19.3 upgrade' in installed
for value in ['SoundFont cache preserved', 'launch appearance history preserved', 'default SoundFont library and preferences preserved']:
    assert value in installed

skin_source = read(root / 'Source/UiSkins.h')
skins = re.findall(r'\{"([a-z]+)","([^"]+)"', skin_source)
assert len(skins) == 50 and len({value[0] for value in skins}) == 50
old = subprocess.check_output(['git', 'show', baseline + ':Source/UiSkins.h'], cwd=root, text=True)
assert skins == re.findall(r'\{"([a-z]+)","([^"]+)"', old)
# Compare every original palette initializer as well as its stable ID and name.
rows = lambda text: re.findall(r'\{"[a-z]+","[^"]+"[^}]*\}', text)
assert rows(skin_source) == rows(old)
snapshots = root / 'build/ui-snapshots'
for prefix in ['Skin', 'Advanced']:
    images = [snapshots / f'{prefix}-{skin}.png' for skin, _ in skins]
    for path in images:
        assert path.read_bytes().startswith(b'\x89PNG\r\n\x1a\n')
    assert len({hashlib.sha256(path.read_bytes()).hexdigest() for path in images}) == 50
for skin, _ in skins[40:]:
    for name in [f'Browser-{skin}.png', f'Theme-{skin}-150pct.png']:
        assert (snapshots / name).read_bytes().startswith(b'\x89PNG\r\n\x1a\n')
    raw = (snapshots / f'Theme-{skin}-150pct.png').read_bytes()
    assert struct.unpack_from('>II', raw, 16) == (1920, 969)

payload = root / 'build/GBoyAdvance2_artefacts/Release'
binaries = [payload / 'VST3/G Boy Advance 2.vst3/Contents/x86_64-win/G Boy Advance 2.vst3', payload / 'Standalone/G Boy Advance 2.exe']
for path in binaries:
    raw = path.read_bytes()
    offset = struct.unpack_from('<I', raw, 0x3c)[0]
    assert raw[:2] == b'MZ' and raw[offset:offset+4] == b'PE\0\0'
    assert struct.unpack_from('<H', raw, offset+4)[0] == 0x8664
portable = root / 'dist/G-Boy-Advance-2-Windows-x64.zip'
with zipfile.ZipFile(portable) as archive:
    assert archive.testzip() is None
    assert not any(name.lower().endswith(('.sf2', '.sf3', '.wav', '.aiff', '.aif')) for name in archive.namelist())
    for binary in binaries:
        suffix = binary.relative_to(payload).as_posix().removeprefix('Standalone/').removeprefix('VST3/')
        assert archive.read(next(name for name in archive.namelist() if name.endswith(suffix))) == binary.read_bytes()
    manual = archive.read(next(name for name in archive.namelist() if name.endswith('USER_MANUAL.md'))).decode('utf-8-sig')
    assert 'Current version: **0.19.3**' in manual and '50 skins' in manual
sys.path.insert(0, str(root / 'scripts'))
from source_files import project_files
files = project_files()
source = root / 'dist/G-Boy-Advance-2-Source.zip'
with zipfile.ZipFile(source) as archive:
    assert archive.testzip() is None and len(archive.namelist()) == len(files)
    for path in files:
        assert archive.read('G-Boy-Advance-2/' + path.relative_to(root).as_posix()) == path.read_bytes()
levels = json.loads(read(root / 'Presets/factory-levels.json'))
old_levels = json.loads(subprocess.check_output(['git', 'show', baseline + ':Presets/factory-levels.json'], cwd=root, text=True))
levels.pop('recipeSha256'); old_levels.pop('recipeSha256')
assert levels == old_levels
assert len(re.findall(r'^GBA_PARAM\(', read(root / 'Source/dsp/ParameterList.inc'), re.M)) == 64
cpu = json.loads(read(logs / 'cpu-comparison.json'))
assert cpu['baselineDspCommit'] == baseline and cpu['comparisonScenes'] == 846 and cpu['floatSamples'] == 14327808
for audio in [cpu, cpu['sustained'], cpu['layeredSoundFontAudio']]:
    assert audio['maxAbsoluteDifference'] <= 1e-6
    assert audio['signalToErrorDb'] is None or audio['signalToErrorDb'] >= 110
assert cpu['sustained']['scenes'] == 148 and cpu['sustained']['floatSamples'] == 147532832
assert cpu['layeredSoundFontAudio']['scenes'] == 16 and cpu['layeredSoundFontAudio']['floatSamples'] == 11332824
assert set(cpu['padStrings']) == {'synth', 'sf2'} and set(cpu['layeredSoundFonts']) == {'static', 'modulated'}
bypass = json.loads(read(logs / 'drive-bypass-reference.json'))
assert bypass['bitIdentical'] and bypass['floatSamples'] == 393216
report = {'version': '0.19.3', 'runtimeCommit': runtime, 'runtimeRun': 43247498302,
          'validationCommit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip(),
          'productionSourceIdentical': True, 'factoryParametersAndGainsUnchanged': True, 'cpuAndAudio': cpu, 'driveBypassReference': bypass,
          'factoryPresets': 654, 'categories': 20, 'programSlots': 754, 'reservedSlots': 100,
          'stableParameters': 64, 'skins': 50, 'unchangedExistingPalettes': 50,
          'distinctMainRenders': 50, 'distinctAdvancedRenders': 50,
          'newThemeBrowserRenders': 10, 'newTheme150PercentRenders': 10,
          'windowsSuitesPassed': 12, 'applicationChecks': app, 'hostChecks': hosts,
          'pluginvalStrictness': 10, 'steinbergPassed': 47, 'steinbergFailed': 0,
          'installerChecks': 6, 'sourceFiles': len(files),
          'binaries': [info(path) for path in [*binaries, root / 'dist/G-Boy-Advance-2-Setup-x64.exe', portable, source]],
          'limits': 'Native shared-runner measurements do not guarantee gains on every PC. Actual FL Studio and physical audio/MIDI/display checks remain owner acceptance; signing and licensing deferred.'}
(logs / 'release-validation-0193.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
print('RELEASE_VALIDATION_JSON ' + json.dumps(report, separators=(',', ':')))
print('PASS complete source, matching x64 binaries, 50 skins, host/audio and installer evidence')
