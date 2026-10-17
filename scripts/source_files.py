"""Shared source-package inventory, including build scripts and workflow files."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ROOT_FILES = {'CMakeLists.txt', 'README.md', '.gitignore', 'LICENSE', 'LICENSE.md', 'NOTICE', 'CHANGELOG.md'}
SOURCE_DIRS = {'Source', 'Tests', 'Presets', 'scripts', 'docs', 'packaging', '.github'}
REQUIRED = {
    'CMakeLists.txt', 'README.md', 'Source/PluginProcessor.cpp', 'Source/PluginEditor.cpp',
    'Source/PresetManager.cpp', 'Source/dsp/SynthEngine.cpp', 'Source/FactoryData.h',
    'Presets/factory.json', 'Tests/EngineTests.cpp', 'Tests/PluginTests.cpp',
    'scripts/build-windows.ps1', 'scripts/validate-windows.ps1', 'scripts/package-windows.ps1', 'packaging/installer.iss',
    '.github/workflows/build.yml',
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
