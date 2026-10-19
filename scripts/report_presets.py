#!/usr/bin/env python3
"""Summarize the offline factory audit; this is not a perceptual listening certificate."""
import argparse,collections,csv,json,math
from pathlib import Path
from generate_presets import ROOT,ENGINE_NAMES

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('audit_dir',type=Path);parser.add_argument('--output',type=Path,default=ROOT/'docs/FACTORY_LIBRARY.md');args=parser.parse_args()
    patches=json.loads((ROOT/'Presets/factory.json').read_text())['presets'];by_id={p['id']:p for p in patches}
    rows=list(csv.DictReader((args.audit_dir/'factory-audit.csv').open()));central=[r for r in rows if float(r['velocity'])==.9 and int(r['note'])==(48 if r['category']=='Bass' or 'bass' in by_id[r['id']]['tags'] else 60)]
    assert len(rows)==len(patches)*4 and len(central)==len(patches)
    levels=json.loads((ROOT/'Presets/factory-levels.json').read_text());groups=collections.defaultdict(list)
    for row in central:groups[row['category']].append(row)
    values=[float(r['max100msStereoRms']) for r in central]
    flags=[];seen=set()
    for row in csv.DictReader((args.audit_dir/'factory-similarity.csv').open()):
        if float(row['dryAudioDistance'])>=.025:continue
        pair=tuple(sorted((row['id'],row['nearestId'])))
        if pair not in seen:flags.append((pair,float(row['dryAudioDistance'])));seen.add(pair)
    within=sum(abs(20*math.log10(float(r['max100msStereoRms'])/levels.get('presetTargets',{}).get(r['id'],levels['targets'][r['category']])))<=.5 for r in central)
    text=['# Factory library audit','',f'{len(patches)} parameter-only presets across {len(groups)} categories. The 102 named instrument designs, 75 named vocal designs, 28 named retro designs and 60 regional hip-hop designs each have an active Afterglow voicing. All legacy IDs and host program indices are frozen by `Presets/factory-index.json`.','',
          'This report describes native numerical renders at 48 kHz. It does not certify perceptual uniqueness, natural acoustic realism or commercial sound quality. Human listening acceptance remains pending.','',
          f'Central-note maximum rolling 100 ms stereo RMS: {min(values):.6f}–{max(values):.6f} ({20*math.log10(max(values)/min(values)):.2f} dB spread). {within}/{len(patches)} patches are within 0.5 dB of their category target. {len(levels["gainCeilingIds"])} master gains reached 0.95; quieter articulations remain rather than changing the engine gain or adding hidden normalization.','',
          '| Category | Presets | Engines | Central RMS min–max |','|---|---:|---|---:|']
    for category,items in groups.items():
        engines=sorted({ENGINE_NAMES[int(by_id[r['id']]['parameters']['engine'])] for r in items});v=[float(r['max100msStereoRms']) for r in items]
        text.append(f'| {category} | {len(items)} | {", ".join(engines)} | {min(v):.5f}–{max(v):.5f} |')
    text+=['','## Listening renders','','`gba2-preset-tests --auditions` creates one six-second stereo PCM excerpt per category plus an index under `preset-auditions/`. Two notes at different velocities and a release section make patches easy to compare. Twelve additional contrasting vocal excerpts are written to `vocal-auditions/`, and all 28 base retro designs to `retro-auditions/`. These original synthesized WAVs are test output only and are never installed or loaded by the instrument. Six seconds may not contain the entire effects tail.','','## Similarity review','','The audit compares 32 log-frequency bands from three Hann-windowed 4096-point FFTs plus eight amplitude-envelope measurements, with effects disabled and volume normalized. It uses the same deterministic excitation seed for each patch. A distance below 0.025 flags a listening candidate; this metric can both miss perceptual similarity and flag intentionally related instruments.','']
    if flags:
        text+=['| Patch | Neighbor | Dry distance |','|---|---|---:|']
        for pair,distance in flags:text.append(f'| {by_id[pair[0]]["name"]} | {by_id[pair[1]]["name"]} | {distance:.6f} |')
    else:text+=['No pairs are below the configured dry spectral/envelope threshold on this audit grid. This does not establish that every listener will perceive every patch as distinct.']
    text+=['','## Rebuilding the bank','','See [preset format](PRESET_FORMAT.md) for the uncalibrated render → calibration → regeneration workflow. Design recipes, level parameters and identity registry are source metadata; the runtime embeds the final parameter arrays and reads no audio files.','']
    args.output.write_text('\n'.join(text))
    summary={'presets':len(patches),'categories':len(groups),'centralRmsMin':min(values),'centralRmsMax':max(values),'centralSpreadDb':20*math.log10(max(values)/min(values)),'withinHalfDb':within,'gainCeilingCount':len(levels['gainCeilingIds']),'nearPairCount':len(flags),'humanListening':'Pending'}
    (args.audit_dir/'factory-summary.json').write_text(json.dumps(summary,indent=2)+'\n');print(json.dumps(summary))
if __name__=='__main__':main()
