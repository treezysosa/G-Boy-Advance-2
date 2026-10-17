#!/usr/bin/env python3
"""Derive parameter-only factory master levels from native offline synth measurements."""
import argparse,csv,json,math
from pathlib import Path
from generate_presets import ROOT,recipe_digest

def target(category):
    if category=='Bass':return .036
    if category in ('Pads','Strings'):return .028
    if category.startswith('Vocals'):return .020
    if category=='Retro Games':return .025
    return .026

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('measurements',type=Path);args=parser.parse_args()
    rows=list(csv.DictReader(args.measurements.open()))
    rows=[r for r in rows if float(r['velocity'])==.9 and int(r['note'])==(48 if r['category']=='Bass' else 60)]
    volumes={};limited=[]
    for row in rows:
        measured=float(row['max100msStereoRms']);prior=float(row['volume']);assert measured>0 and prior>0
        volume=max(.05,min(.95,target(row['category'])*prior/measured))
        volumes[row['id']]=round(volume,6)
        if volume==.95:limited.append(row['id'])
    assert len(volumes)==382 and len(rows)==382
    data={'format':'gba2.factory-levels','schemaVersion':1,'recipeSha256':recipe_digest(),
          'metric':'Maximum rolling 100 ms stereo RMS at 48 kHz, MIDI 60 (bass 48), velocity 0.9, hold max(0.8s, attack+0.35s). Master volume is after the limiter; no audio is stored.',
          'targets':{category:target(category) for category in sorted({r['category'] for r in rows})},
          'volumeBounds':[.05,.95],'gainCeilingIds':limited,'volumes':volumes}
    (ROOT/'Presets/factory-levels.json').write_text(json.dumps(data,indent=2)+'\n')
    print(f'Calibrated {len(volumes)} factory master volumes; {len(limited)} reached the gain ceiling (quieter articulation retained).')
if __name__=='__main__':main()
