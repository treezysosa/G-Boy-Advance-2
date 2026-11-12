#!/usr/bin/env python3
"""Render every factory sound for human review; never alter a factory patch."""
import argparse
import concurrent.futures
import csv
import hashlib
import io
import json
import math
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import zipfile

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
RATE, SECONDS = 48000, 20
SECTIONS = [dict(name='Low / soft', start=0, end=.8, notes=[48], velocity=.45),
            dict(name='Middle / soft', start=1.3, end=2.1, notes=[60], velocity=.45),
            dict(name='Middle / strong', start=2.6, end=5.6, notes=[60], velocity=.9),
            dict(name='High', start=6.2, end=7, notes=[72], velocity=.75),
            dict(name='Chord (native voice mode)', start=7.6, end=11.6, notes=[48, 52, 55, 60], velocity=.7),
            dict(name='Release / effects tail', start=11.6, end=20, notes=[], velocity=0)]


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def meter_summary(stderr, name):
    pattern = r'\[ebur128@' + re.escape(name) + r' @[^\]]+\] Summary:\s*(.*?)(?=\n\[ebur128@|\Z)'
    block = re.search(pattern, stderr, re.S)
    if block is None:
        raise RuntimeError('Missing FFmpeg loudness summary: ' + name)
    integrated = re.search(r'\bI:\s+(-?\d+(?:\.\d+)?)\s+LUFS', block[1])
    if integrated is None:
        raise RuntimeError('Unusable loudness: ' + name)
    result = dict(lufs=float(integrated[1]))
    peak = re.search(r'Peak:\s+(-?\d+(?:\.\d+)?)\s+dBFS', block[1])
    if peak:
        result['truePeakDbtp'] = float(peak[1])
    return result


def encode_and_measure(source, output, ffmpeg):
    graph = ('[0:a]asplit=3[full][note][chord];'
             '[full]ebur128@phrase=peak=true:framelog=verbose[f];'
             '[note]atrim=2.6:5.6,ebur128@note=framelog=verbose[n];'
             '[chord]atrim=7.6:11.6,ebur128@chord=framelog=verbose[c]')
    command = [ffmpeg, '-hide_banner', '-nostats', '-nostdin', '-y', '-i', str(source),
               '-filter_complex', graph, '-map', '[f]', '-c:a', 'flac', '-sample_fmt', 's32',
               '-bits_per_raw_sample', '24', '-threads', '1', str(output),
               '-map', '[n]', '-f', 'null', '-', '-map', '[c]', '-f', 'null', '-']
    result = subprocess.run(command, capture_output=True, text=True, check=True)
    return {name: meter_summary(result.stderr, name) for name in ('phrase', 'note', 'chord')}


def write_float_wav(path, samples):
    raw = np.asarray(samples, dtype='<f4').tobytes()
    header = struct.pack('<4sI4s4sIHHIIHH4sI', b'RIFF', 36+len(raw), b'WAVE', b'fmt ', 16,
                         3, 2, RATE, RATE*8, 8, 32, b'data', len(raw))
    path.write_bytes(header + raw)


def self_test(ffmpeg):
    # Known-amplitude stereo 1 kHz signals verify meter parsing, relative LUFS
    # and true-peak behavior, independently of the synthesizer and its presets.
    with tempfile.TemporaryDirectory() as directory:
        folder = Path(directory)
        time = np.arange(RATE*SECONDS, dtype=np.float64)/RATE
        signal = np.repeat((.1*np.sin(2*np.pi*1000*time))[:, None], 2, axis=1)
        results = []
        for i, gain in enumerate((1., .5)):
            source = folder/f'{i}.wav'
            write_float_wav(source, signal*gain)
            results.append(encode_and_measure(source, folder/f'{i}.flac', ffmpeg))
        for section in ('phrase', 'note', 'chord'):
            assert abs(results[0][section]['lufs'] - results[1][section]['lufs'] - 6.02) < .15
        assert abs(results[0]['phrase']['truePeakDbtp'] + 20) < .15
        assert abs(results[1]['phrase']['truePeakDbtp'] + 26.02) < .15
    print('PASS meter controls: known 1 kHz true peak and 6.02 LU gain difference', flush=True)


def listening_family(patch):
    category = patch['category']
    fixed = {'Guitars': 'guitar', 'Pianos and Keys': 'keys', 'Bells': 'bell', 'Pads': 'pad',
             'Bass': 'bass', 'Flutes': 'flute', 'Plucks': 'pluck', 'Strings': 'strings',
             'Woodwinds': 'woodwind', 'Brass': 'brass'}
    if category in fixed:
        return fixed[category]
    tags = set(patch['tags'])
    if category == 'Vocals':
        return 'vox-stab' if 'vox stabs' in tags else 'vocal'
    for token, family in [('bass', 'bass'), ('piano', 'keys'), ('keys', 'keys'),
                          ('bell', 'bell'), ('bells', 'bell'), ('pad', 'pad'),
                          ('strings', 'strings'), ('pluck', 'pluck'), ('plucks', 'pluck'),
                          ('vox', 'vocal'), ('choir', 'vocal'), ('brass', 'brass'),
                          ('flute', 'flute'), ('lead', 'lead')]:
        if token in tags:
            return family
    name = patch['name'].lower()
    for word, family in [('bass', 'bass'), ('piano', 'keys'), ('keys', 'keys'),
                         ('bell', 'bell'), ('pad', 'pad'), ('strings', 'strings'),
                         ('pluck', 'pluck'), ('choir', 'vocal'), ('lead', 'lead')]:
        if word in name:
            return family
    return category


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--renderer', type=Path)
    parser.add_argument('--output', type=Path, default=ROOT/'build-preset-listening/Listening Pack')
    parser.add_argument('--similarity', type=Path)
    parser.add_argument('--ffmpeg', default='ffmpeg')
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--limit', type=int)
    parser.add_argument('--resume', action='store_true')
    parser.add_argument('--self-test', action='store_true')
    parser.add_argument('--zip', action='store_true', help='Also create complete and category ZIPs')
    args = parser.parse_args()
    if args.self_test:
        self_test(args.ffmpeg)
        return
    if args.renderer is None or not args.renderer.is_file():
        parser.error('Supply the compiled --renderer')
    if not 1 <= args.jobs <= 4 or (args.limit is not None and args.limit < 1):
        parser.error('Use 1-4 jobs and a positive optional limit')
    factory_raw = (ROOT/'Presets/factory.json').read_bytes()
    patches = json.loads(factory_raw)['presets']
    if len(patches) != 654:
        raise RuntimeError('Expected the current 654-preset bank')
    factory_hash = digest(factory_raw.replace(b'\r\n', b'\n'))
    provenance = dict(factorySha256=factory_hash, rendererSha256=digest(args.renderer.read_bytes()),
                      phraseVersion=1, sampleRate=RATE, frames=RATE*SECONDS,
                      ffmpeg=subprocess.check_output([args.ffmpeg, '-version'], text=True).splitlines()[0])
    key = digest(json.dumps(provenance, sort_keys=True).encode())
    output = args.output.resolve()
    if output.exists() and any(output.iterdir()) and not args.resume:
        parser.error('Output is not empty; use --resume to verify and reuse matching renders')
    (output/'audio').mkdir(parents=True, exist_ok=True)
    (output/'metrics').mkdir(exist_ok=True)
    neighbors = {}
    if args.similarity:
        with args.similarity.open(encoding='utf-8-sig', newline='') as file:
            neighbors = {row['id']: row for row in csv.DictReader(file)}
        if set(neighbors) != {patch['id'] for patch in patches}:
            raise RuntimeError('Similarity report does not cover this bank exactly')
    selected = patches[:args.limit] if args.limit else patches
    renderer = str(args.renderer.resolve())

    def render(item):
        index, patch = item
        name = patch['id']
        audio, cache = output/'audio'/f'{name}.flac', output/'metrics'/f'{name}.json'
        if args.resume and cache.exists() and audio.exists():
            data = json.loads(cache.read_text())
            if data.get('cacheKey') == key and data.get('audioSha256') == digest(audio.read_bytes()):
                return data
        with tempfile.TemporaryDirectory(prefix='gba2-audition-') as temporary:
            source = Path(temporary)/'render.wav'
            native = subprocess.run([renderer, '--index', str(index), '--output', str(source)],
                                    check=True, capture_output=True, text=True)
            raw = source.read_bytes()
            if len(raw) != 44+RATE*SECONDS*8:
                raise RuntimeError('Incomplete native render: '+name)
            samples = np.frombuffer(raw, dtype='<f4', offset=44).reshape(-1, 2)
            if not np.all(np.isfinite(samples)) or np.max(np.abs(samples)) > 1:
                raise RuntimeError('Invalid audio: '+name)
            metrics = encode_and_measure(source, audio, args.ffmpeg)
            tail = math.sqrt(float(np.mean(samples[-RATE//2:].astype(np.float64)**2)))
            data = dict(id=name, name=patch['name'], category=patch['category'], tags=patch['tags'],
                        index=index, program=None, cacheKey=key, audio=f'audio/{name}.flac',
                        audioSha256=digest(audio.read_bytes()), bytes=audio.stat().st_size,
                        samplePeak=float(np.max(np.abs(samples))), dc=float(np.mean(samples)),
                        tailRmsDbfs=round(20*math.log10(max(tail, 1e-15)), 2), metrics=metrics,
                        nativeEvidence=native.stdout.strip(), voiceMode=patch['parameters']['voice.mode'],
                        masterVolume=patch['parameters']['volume'])
            neighbor = neighbors.get(name)
            if neighbor:
                data['nearest'] = dict(id=neighbor['nearestId'], name=neighbor['nearestName'],
                                       dryDistance=float(neighbor['dryDistance']), wetDistance=float(neighbor['wetDistance']))
            cache.write_text(json.dumps(data, indent=2)+'\n')
            return data

    rows = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for completed, result in enumerate(pool.map(render, enumerate(selected)), 1):
            rows.append(result)
            if completed % 25 == 0 or completed == len(selected):
                print(f'Rendered and metered {completed}/{len(selected)}', flush=True)
    # Sound-family medians are screening references, not instructions to flatten the
    # intended differences between basses, pads, short plucks and vocal textures.
    for row in rows:
        row['listeningFamily'] = listening_family(row)
        family = [p['metrics']['note']['lufs'] for p in rows if listening_family(p) == row['listeningFamily']]
        median = float(np.median(family))
        difference = row['metrics']['note']['lufs']-median
        row['familyMedianNoteLufs'] = round(median, 2)
        row['relativeNoteLu'] = round(difference, 2)
        row['reviewFlags'] = []
        if abs(difference) > 3:
            row['reviewFlags'].append('level compared with sound family')
        if row['metrics']['phrase']['truePeakDbtp'] > -3:
            row['reviewFlags'].append('peak headroom')
        if row['tailRmsDbfs'] > -60:
            row['reviewFlags'].append('tail extends beyond excerpt')
        if row.get('nearest', {}).get('dryDistance', 1) < .08:
            row['reviewFlags'].append('close-pair listening candidate')
        # A bounded proposal only. No bank gain or parameter is changed here.
        proposed = max(-3., min(3., -difference)) if abs(difference) > 3 else 0.
        peak_allowance = -3-row['metrics']['phrase']['truePeakDbtp']
        volume_allowance = 20*math.log10(.95/row['masterVolume'])
        row['proposedGainDb'] = round(min(proposed, peak_allowance, volume_allowance), 2) if proposed > 0 else round(proposed, 2)
        row['proposedGainDb'] = max(-3., row['proposedGainDb'])
    report = dict(format='gba2.listening-pack', schemaVersion=1, provenance=provenance,
                  factoryPresets=654, renderedPresets=len(rows), humanReviewedPresets=0,
                  bankIds=[patch['id'] for patch in patches],
                  phraseSeconds=SECONDS, sections=SECTIONS, presets=rows,
                  limits='Numerical screening and gain proposals only. No human listening or subjective quality certification. No factory changes applied.')
    (output/'manifest.json').write_text(json.dumps(report, indent=2)+'\n')
    columns = ['id', 'name', 'category', 'family', 'phraseLufs', 'noteLufs', 'chordLufs', 'truePeakDbtp',
               'relativeNoteLu', 'proposedGainDb', 'tailRmsDbfs', 'nearestId', 'dryDistance', 'flags', 'humanReview']
    with (output/'review.csv').open('w', newline='') as file:
        writer = csv.DictWriter(file, fieldnames=columns)
        writer.writeheader()
        for row in sorted(rows, key=lambda p: -abs(p['relativeNoteLu'])):
            writer.writerow(dict(id=row['id'], name=row['name'], category=row['category'], family=row['listeningFamily'],
                                 phraseLufs=row['metrics']['phrase']['lufs'], noteLufs=row['metrics']['note']['lufs'],
                                 chordLufs=row['metrics']['chord']['lufs'], truePeakDbtp=row['metrics']['phrase']['truePeakDbtp'],
                                 relativeNoteLu=row['relativeNoteLu'], proposedGainDb=row['proposedGainDb'],
                                 tailRmsDbfs=row['tailRmsDbfs'], nearestId=row.get('nearest', {}).get('id', ''),
                                 dryDistance=row.get('nearest', {}).get('dryDistance', ''),
                                 flags='; '.join(row['reviewFlags']), humanReview='unreviewed'))
    template = (ROOT/'Tools/listening-review.html').read_text()

    def html_for(data):
        return template.replace('/*__PACK_DATA__*/null', json.dumps(data, ensure_ascii=False).replace('<', '\\u003c'))

    (output/'Listen.html').write_text(html_for(report))
    readme = ('G BOY ADVANCE 2 — factory listening pack\n\nExtract the entire ZIP, then double-click Listen.html in Chrome or Edge. '
              'Audio is original synthesis rendered from the existing bank, not content loaded by the plugin. '
              'No internet, server or installation is needed. Start with a comfortable playback level.\n\n'
              'Each preset has the same 20-second phrase: C3 soft, C4 soft, C4 strong, C5, C3/E3/G3/C4 chord, '
              'then 8.4 seconds of release. Mono/legato settings are retained. Very long tails may exceed the excerpt. '
              'The complete bank contains 218 minutes of audio; take listening breaks.\n\n'
              'Select a preset and play it; compare its nearest dry-spectrum neighbor with A/B. '
              'The match option attenuates the louder clip only. Rate Keep / Too quiet / Too loud / Weak / Similar '
              'and add notes. Export your feedback before moving/deleting files or changing browsers. '
              'Browser storage is a convenience, not a backup. Import exported JSON to continue.\n\n'
              'LUFS/true-peak values are FFmpeg EBU R128 measurements (0.1-unit reporting precision), not human judgments. '
              'Sound-family deviations and close-pair flags are listening priorities. Gain proposals are bounded to 3 dB, '
              'existing master-volume limits and measured 3 dB peak headroom; they are not applied. '
              'No preset was approved, replaced or level-adjusted by a human in this pack.\n')
    (output/'READ-ME.txt').write_text(readme)
    if args.zip:
        def archive(path, data):
            with zipfile.ZipFile(path, 'w', zipfile.ZIP_DEFLATED, compresslevel=1) as file:
                file.writestr('GBA2 Listening/Listen.html', html_for(data))
                file.writestr('GBA2 Listening/manifest.json', json.dumps(data, indent=2))
                file.writestr('GBA2 Listening/READ-ME.txt', readme)
                selected_ids = {row['id'] for row in data['presets']}
                text = io.StringIO(newline='')
                writer = csv.DictWriter(text, fieldnames=columns)
                writer.writeheader()
                with (output/'review.csv').open(newline='') as source:
                    writer.writerows(row for row in csv.DictReader(source) if row['id'] in selected_ids)
                file.writestr('GBA2 Listening/review.csv', text.getvalue())
                for row in data['presets']:
                    file.write(output/row['audio'], 'GBA2 Listening/'+row['audio'])
            with zipfile.ZipFile(path) as file:
                if file.testzip() is not None:
                    raise RuntimeError('Listening archive CRC failed')
        archive(output.parent/f'G-Boy-Advance-2-All-{len(rows)}-Listening.zip', report)
        category_dir = output.parent/'category-downloads'
        category_dir.mkdir(exist_ok=True)
        for category in sorted({p['category'] for p in rows}):
            category_rows = [p for p in rows if p['category'] == category]
            parts, part, size = [], [], 0
            for row in category_rows:
                if part and size+row['bytes'] > 27*1024*1024:
                    parts.append(part)
                    part, size = [], 0
                part.append(row)
                size += row['bytes']
            if part:
                parts.append(part)
            for i, part in enumerate(parts, 1):
                subset = dict(report, presets=part, renderedPresets=len(part))
                filename = re.sub(r'[^A-Za-z0-9-]+', '-', category).strip('-')
                archive(category_dir/f'{filename}-{i:02}.zip', subset)
    print(json.dumps(dict(renderedPresets=len(rows), bytes=sum(p['bytes'] for p in rows),
                          flaggedPresets=sum(bool(p['reviewFlags']) for p in rows), humanReviewedPresets=0,
                          levelFlags=sum('level compared with sound family' in p['reviewFlags'] for p in rows),
                          peakFlags=sum('peak headroom' in p['reviewFlags'] for p in rows),
                          noteLufsRange=[min(p['metrics']['note']['lufs'] for p in rows), max(p['metrics']['note']['lufs'] for p in rows)])))


if __name__ == '__main__':
    main()
