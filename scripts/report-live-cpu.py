#!/usr/bin/env python3
"""Report matched held/overlap timings without implying DAW CPU percentages."""
import argparse
import csv
import json
from pathlib import Path

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('directory',type=Path)
args=parser.parse_args()
rows={}
identity=lambda r:tuple(r[k] for k in ('engine','fixture','workload','sample_rate','block','voices'))
for kind in ('before','after'):
    for repeat in range(2):
        with (args.directory/f'pad-string-{kind}-{repeat}.csv').open(encoding='utf-8-sig',newline='') as file:
            data=list(csv.DictReader(file))
        assert len(data)==672 and all(int(r['invalid_samples'])==0 for r in data)
        rows[kind,repeat]=data
for repeat in range(2):
    assert [identity(r) for r in rows['before',repeat]]==[identity(r) for r in rows['after',repeat]]
report={}
for engine in ('synth','sf2'):
    report[engine]={}
    for voices in (4,8,16,32):
        values={}
        for kind in ('before','after'):
            selected=[r for repeat in range(2) for r in rows[kind,repeat] if r['engine']==engine and int(r['voices'])==voices]
            values[kind+'MeasuredUs']=sum(float(r['mean_us'])*int(r['callbacks']) for r in selected)
        values['reductionPercent']=100*(1-values['afterMeasuredUs']/values['beforeMeasuredUs'])
        report[engine][str(voices)]=values
(args.directory/'live-cpu-probe.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
