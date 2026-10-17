"""Shared source-package inventory, including build scripts and workflow files."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ROOT_FILES = {'CMakeLists.txt', 'README.md', '.gitignore', 'LICENSE', 'LICENSE.md', 'NOTICE', 'CHANGELOG.md'}
SOURCE_DIRS = {'Source', 'Tests', 'Presets', 'scripts', 'docs', 'packaging', '.github'}
REQUIRED = {
    'Tests/ModalReferenceProbe.cpp', 'docs/PHASE_10_FINALIZATION.md', 'scripts/check-notices.py', 'scripts/sign-release-windows.ps1', 'docs/licenses/manifest.json',
    'Tests/HostTests.cpp', 'Tests/Benchmark.cpp', 'scripts/test-installer-windows.ps1',
    'scripts/validate-steinberg-windows.ps1', 'docs/PHASE_10.md', 'docs/RELEASE_CHECKLIST.md',
    'CMakeLists.txt', 'README.md', 'Source/PluginProcessor.cpp', 'Source/PluginEditor.cpp',
    'Source/PresetManager.cpp', 'Source/dsp/SynthEngine.cpp', 'Source/dsp/Effects.cpp', 'Source/dsp/Effects.h', 'Source/dsp/VocalEngine.h', 'Source/dsp/RetroEngine.h', 'Tests/RetroTests.cpp', 'Presets/retro-designs.json', 'Source/FactoryData.h',
    'Presets/factory.json', 'Presets/instrument-designs.json', 'Presets/factory-levels.json', 'Presets/factory-index.json', 'Tests/EngineTests.cpp', 'Tests/CoreSynthesisTests.cpp', 'Tests/AdvancedSynthesisTests.cpp', 'Tests/PluginTests.cpp', 'Tests/LibraryTests.cpp', 'Tests/EffectsTests.cpp', 'Tests/PresetTests.cpp', 'Tests/VocalTests.cpp', 'Presets/vocal-designs.json',
    'scripts/calibrate_presets.py', 'scripts/report_presets.py', 'scripts/build-windows.ps1', 'scripts/validate-windows.ps1', 'scripts/package-windows.ps1', 'packaging/installer.iss',
    '.github/workflows/build.yml', 'docs/PHASE_4.md', 'docs/PHASE_5.md', 'docs/PHASE_6.md', 'docs/PHASE_7.md', 'docs/PHASE_8.md', 'docs/PHASE_9.md',
}


def project_files():
    files = []
    for path in ROOT.rglob('*'):
        if not path.is_file():
            continue
        parts = path.relative_to(ROOT).parts
        if len(parts) == 1:
            include = parts[0] in ROOT_FILES
        else:
            include = parts[0] in SOURCE_DIRS and not any(
                part in {'__pycache__', '.git', 'Dependencies'} for part in parts[:-1])
        if include:
            files.append(path)
    names = {path.relative_to(ROOT).as_posix() for path in files}
    missing = REQUIRED - names
    if missing:
        raise RuntimeError('Source package is missing required files: ' + ', '.join(sorted(missing)))
    return sorted(files)
