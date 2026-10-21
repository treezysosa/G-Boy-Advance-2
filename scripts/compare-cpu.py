#!/usr/bin/env python3
"""Compare native old/new renders and matched callback profiles, off the audio thread."""
import array
import csv
import hashlib
import json
import math
from pathlib import Path
import sys

folder = Path(sys.argv[1])
root=Path(__file__).resolve().parents[1]
patch_count=len(json.loads((root/'Presets/factory.json').read_text())['presets'])
scene_count=patch_count+120
expected_floats=(120*256+patch_count*96)*128
old = (folder / 'cpu-before.f32').read_bytes()
new = (folder / 'cpu-after.f32').read_bytes()
if len(old) != len(new) or len(old) != expected_floats * 4:
    raise SystemExit('Numerical comparison has the wrong sample count.')
maximum = energy = error = 0.0
different = 0
if old != new:
    before, after = array.array('f'), array.array('f')
    before.frombytes(old)
    after.frombytes(new)
    for a, b in zip(before, after):
        if not math.isfinite(a) or not math.isfinite(b):
            raise SystemExit('Nonfinite numerical output.')
        delta = a - b
        maximum = max(maximum, abs(delta))
        energy += a * a
        error += delta * delta
        different += a != b
snr = None if error == 0 else 10 * math.log10(max(energy, 1e-300) / error)
result = {'comparisonScenes': scene_count, 'floatSamples': len(old) // 4,
          'bitIdentical': old == new, 'differentSamples': different,
          'maxAbsoluteDifference': maximum, 'signalToErrorDb': snr,
          'beforeSha256': hashlib.sha256(old).hexdigest(),
          'afterSha256': hashlib.sha256(new).hexdigest(), 'profiles': {}}


def read_profile(path):
    with path.open(newline='') as file:
        rows = list(csv.DictReader(file))
    if len(rows) != 66 or any(int(row['invalid_samples']) for row in rows):
        raise SystemExit(f'Incomplete or nonfinite profile: {path}')
    return {(row['sample_rate'], row['block'], row['engine'], row['voices']): row for row in rows}


for voices in (8, 32):
    before = read_profile(folder / f'cpu-before-{voices}.csv')
    after = read_profile(folder / ('callback-profile.csv' if voices == 32 else 'cpu-after-8.csv'))
    if before.keys() != after.keys():
        raise SystemExit('Mismatched CPU workloads.')
    groups = {}
    for group, select in [('all', lambda e: True), ('steady', lambda e: e < 8),
                          ('switching', lambda e: e >= 8)]:
        keys = [key for key in before if select(int(key[2]))]
        total_before = sum(float(before[k]['mean_us']) * int(before[k]['callbacks']) for k in keys)
        total_after = sum(float(after[k]['mean_us']) * int(after[k]['callbacks']) for k in keys)
        groups[group] = {'beforeMeasuredUs': total_before, 'afterMeasuredUs': total_after,
                         'reductionPercent': 100 * (1 - total_after / total_before)}
    groups['afterOverBudget'] = sum(int(row['over_budget']) for row in after.values())
    groups['afterMaxBudgetRatio'] = max(float(row['max_budget_ratio']) for row in after.values())
    result['profiles'][str(voices)] = groups
with (folder/'play-before.csv').open() as f:before_play=list(csv.DictReader(f))
with (folder/'play-after.csv').open() as f:after_play=list(csv.DictReader(f))
assert len(before_play)==len(after_play)==612
identity=lambda r:tuple(r[k] for k in ('fixture','workload','sample_rate','block','voices'))
assert [identity(r) for r in before_play]==[identity(r) for r in after_play]
assert all(int(r['invalid_samples'])==0 for r in before_play+after_play)
result['playing']={}
for voices in (1,4,8):
    for workload in ('held-chord','retrigger'):
        chosen=lambda rows:[r for r in rows if int(r['voices'])==voices and r['workload']==workload]
        a=sum(float(r['mean_us'])*int(r['callbacks']) for r in chosen(before_play))
        b=sum(float(r['mean_us'])*int(r['callbacks']) for r in chosen(after_play))
        result['playing'][f'{voices}-{workload}']={'beforeMeasuredUs':a,'afterMeasuredUs':b,'reductionPercent':100*(1-b/a)}
result['baselineDspCommit']='f079323677e6024be8356a6afdfa333eb9db4558'
def sustained_comparison():
    before_hash,after_hash=hashlib.sha256(),hashlib.sha256()
    total=changed=0;maximum=energy=noise=0.0
    with (folder/'sustained-before.f32').open('rb') as a,(folder/'sustained-after.f32').open('rb') as b:
        while True:
            left,right=a.read(1024*1024),b.read(1024*1024)
            if len(left)!=len(right):raise SystemExit('Sustained comparison lengths differ.')
            if not left:break
            before_hash.update(left);after_hash.update(right);total+=len(left)//4
            x=array.array('f');x.frombytes(left)
            energy+=sum(float(v)*v for v in x)
            if left!=right:
                y=array.array('f');y.frombytes(right)
                for v,w in zip(x,y):
                    if not math.isfinite(v) or not math.isfinite(w):raise SystemExit('Nonfinite sustained output.')
                    d=float(v)-w;maximum=max(maximum,abs(d));noise+=d*d;changed+=v!=w
    if total!=147532832:raise SystemExit('Wrong sustained comparison sample count.')
    snr=None if noise==0 else 10*math.log10(max(energy,1e-300)/noise)
    if maximum>1e-6 or (snr is not None and snr<110):raise SystemExit('Sustained audio changed beyond the existing 1e-6 / 110 dB tolerance.')
    return {'scenes':148,'secondsPerScene':8,'factoryPadStringPresets':44,'sf2Scenes':16,'floatSamples':total,'bitIdentical':before_hash.digest()==after_hash.digest(),'differentSamples':changed,'maxAbsoluteDifference':maximum,'signalToErrorDb':snr,'beforeSha256':before_hash.hexdigest(),'afterSha256':after_hash.hexdigest()}
result['sustained']=sustained_comparison()
result['padStrings']={}
for engine in ('synth','sf2'):
    result['padStrings'][engine]={}
    for voices in (4,8,16,32):
        measurements={}
        for kind in ('before','after'):
            rows=[r for repeat in range(2) for r in csv.DictReader((folder/f'pad-string-{kind}-{repeat}.csv').open()) if r['engine']==engine and int(r['voices'])==voices]
            if len(rows)!=(288 if engine=='synth' else 48) or any(int(r['invalid_samples']) for r in rows):raise SystemExit('Incomplete or nonfinite pad/string profile.')
            measurements[kind+'MeasuredUs']=sum(float(r['mean_us'])*int(r['callbacks']) for r in rows)
            measurements[kind+'OverBudget']=sum(int(r['over_budget']) for r in rows)
        measurements['reductionPercent']=100*(1-measurements['afterMeasuredUs']/measurements['beforeMeasuredUs'])
        result['padStrings'][engine][str(voices)]=measurements
for repeat in range(2):
    with (folder/f'pad-string-before-{repeat}.csv').open() as a,(folder/f'pad-string-after-{repeat}.csv').open() as b:
        x,y=list(csv.DictReader(a)),list(csv.DictReader(b))
        key=lambda r:tuple(r[k] for k in ('engine','fixture','workload','sample_rate','block','voices'))
        if len(x)!=672 or [key(r) for r in x]!=[key(r) for r in y]:raise SystemExit('Mismatched pad/string workloads.')
# Report CPU time and reciprocal throughput separately: a 50% throughput gain
# corresponds to one third less render time, not half. Keep the stricter CPU
# reduction target explicit so neither interpretation can inflate the claim.
for entry in [*(v for group in result['profiles'].values() for k,v in group.items() if isinstance(v,dict)), *result['playing'].values(), *(v for group in result['padStrings'].values() for v in group.values())]:
    entry['throughputIncreasePercent']=100*(entry['beforeMeasuredUs']/entry['afterMeasuredUs']-1)
result['performanceTarget']={'requestedPercent':50,'metric':'CPU render time reduction against 0.17.1; reciprocal throughput is also reported','synthPadStringVoicesMeeting50Percent':[int(k) for k,v in result['padStrings']['synth'].items() if v['reductionPercent']>=50],'sf2PadStringVoicesMeeting50Percent':[int(k) for k,v in result['padStrings']['sf2'].items() if v['reductionPercent']>=50],'universalGainGuaranteed':False}
result['limit'] = 'Native DSP on one shared runner with identical current factory arrays; timings are observational, not FL Studio CPU percentages or physical-driver deadline certification.'
(folder / 'cpu-comparison.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result, indent=2))
if maximum > 1e-6 or (snr is not None and snr < 110):
    raise SystemExit('Numerical comparison exceeded the audio-preservation tolerance.')
