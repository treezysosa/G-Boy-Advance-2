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
old = (folder / 'cpu-before.f32').read_bytes()
new = (folder / 'cpu-after.f32').read_bytes()
if len(old) != len(new) or len(old) != 10444800 * 4:
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
result = {'comparisonScenes': 650, 'floatSamples': len(old) // 4,
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
assert len(before_play)==len(after_play)==432
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
result['baselineDspCommit']='f19b9fafd80cf54aa1a6b264a4ccf2a5af2f3a84'
result['limit'] = 'Native DSP on one shared runner with identical 530-patch arrays; timings are observational, not FL Studio CPU percentages or physical-driver deadline certification.'
(folder / 'cpu-comparison.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result, indent=2))
if maximum > 1e-6 or (snr is not None and snr < 110):
    raise SystemExit('Numerical comparison exceeded the audio-preservation tolerance.')
