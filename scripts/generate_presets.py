#!/usr/bin/env python3
"""Build reproducible, original parameter-only factory patches. No audio assets.
The committed JSON and C++ include must be regenerated together after edits.
"""
import argparse
import hashlib
import json
import pathlib
import random
import re

ROOT = pathlib.Path(__file__).resolve().parents[1]
GROUPS = {
    'Guitars': '''70s Soul Guitar|Neo Soul Guitar|Jazz Guitar|Funk Guitar|City Pop Guitar|Indie Guitar|Dream Pop Guitar|Chorus Guitar|Nylon-Style Guitar|Acoustic-Style Guitar|Lo-Fi Guitar|Dark Ambient Guitar|Latin Guitar|Retro Game Guitar''',
    'Pianos and Keys': '''Vintage Grand-Style Piano|70s Soul Piano|Tine Electric Piano|Reed Electric Piano|80s Digital Piano|90s R&B Keys|City Pop Keys|Jazz Keys|Lo-Fi Piano|Dream Piano|Horror Piano|Toy Piano|FM Piano|Retro Console Piano''',
    'Bells': '''Crystal Bells|Dream Bells|Anime-Inspired Bells|VHS Bells|Dark Bells|FM Bells|Music Box Bells|Celesta-Style Bells|Arcade Bells|Glass Bells''',
    'Pads': '''Analog Soul Pad|80s Pop Pad|City Pop Pad|Dream Pad|Dark Horror Pad|Retro Console Pad|VHS Pad|Ambient Pad|Angelic Pad|Nostalgia Pad|Space Pad|Warm Tape Pad''',
    'Bass': '''Vintage Analog Bass|Funk Bass|Soul Bass|Synth Bass|Sub Bass|Memphis Bass|808-Style Synth Bass|City Pop Bass|Slap-Style Bass|FM Bass|Retro Game Bass|Dark Reese Bass|Electric-Style Bass|Lo-Fi Bass''',
    'Flutes': '''Soul Flute|Jazz Flute|Anime-Inspired Flute|Bamboo-Style Flute|Dream Flute|Horror Flute|Retro Console Flute|Pan Flute|Breath Flute|Ethereal Flute''',
    'Plucks': '''Dream Pluck|City Pop Pluck|Analog Pluck|Crystal Pluck|Retro Game Pluck|Dark Pluck|Lo-Fi Pluck|FM Pluck|Harp-Style Pluck|Soul Pluck''',
    'Sax-Style Leads': '''Smooth Jazz Sax-Style Lead|Soul Sax-Style Lead|80s Pop Sax-Style Lead|City Pop Sax-Style Lead|Alto Sax-Style Lead|Tenor Sax-Style Lead|Breath Sax-Style Lead|Dream Sax-Style Lead''',
    'Strings': '''Vintage Strings|Soul Strings|Cinematic Strings|Horror Strings|Anime-Inspired Strings|Synth Strings|Orchestral-Style Strings|Pizzicato Strings|Dream Strings|Retro Game Strings''',
    'Vocals - Global Pop Textures': '''Japanese City Pop Vocal Synth|Korean Pop Vocal Synth|Russian Pop-Inspired Vocal Texture|Latin Pop Vocal Synth|African Pop-Inspired Vocal Texture|Brazilian Pop Vocal Synth|Mexican Pop-Inspired Vocal Texture|Spanish Pop Vocal Synth|French Pop Vocal Texture|Middle Eastern Pop-Inspired Vocal Synth|Iranian Pop-Inspired Vocal Texture|Afghan Pop-Inspired Vocal Texture''',
    'Vocals - Horror': '''Dark Horror Vocal Synth|Ghost Whisper Synth|Haunted Choir|Distorted Vocal Synth|Psychological Horror Voice Texture|Survival Horror-Inspired Vocal Synth|Cinematic Horror Choir''',
    'Vocals - Choirs and Textures': '''Children's Choir-Style Synth|Rock Vocal-Style Synth|Ethereal High-Register Vocal Pad|Dreamy Vocal Synth|Nostalgic Vocal Pad|Lo-Fi Vocal Synth|Vintage Radio Vocal Synth|Synthetic Vocal Chop|Dark Formant Bass|Angelic Choir Synth|Neon 80s Pop Vowels|Velvet Smooth Soul Choir|Glossy R&B Formants|Melancholy Rap Vowels|Bedroom Indie Choir|West Coast Party Chant|Memphis Shadow Chant|Southern Club Formants''',
    'Retro Games': '''8-Bit Lead|Handheld Dream Keys|16-Bit FM Bass|Retro Arcade Bell|Pixel Pluck|Game Menu Pad|Retro RPG Strings|Console Choir Synth|Dreamy Save-Point Pad|Pixel Horror Lead|Retro Racing Synth|Vintage Sports Game Lead|Retro Platformer Pluck|2000s Handheld Synth|Pocket Duty Bass|Cartridge Noise Snare|Short Register Hat|Pocket Triangle Sub|Arcade Ring Keys|FM Glass Checkpoint|Vector Race Brass|Pixel Harmonic Organ|Original Table Flute|Late Night Polygon Pad|Low Bit Modal Marimba|Tape Menu Harp|Pixel Vowel Choir|Handheld Pulse Motion''',
}


def schema():
    specs = []
    for line in (ROOT/'Source/dsp/ParameterList.inc').read_text().splitlines():
        m = re.match(r'GBA_PARAM\((\w+), "([^"]+)", "[^"]+", ([^,]+), ([^,]+), ([^,]+), ([^,]+),', line)
        if m:
            symbol, key, lo, hi, default, step = m.groups()
            number = lambda x: float(x.rstrip('f'))
            specs.append((symbol, key, number(lo), number(hi), number(default), number(step)))
    return specs


def make_patch(category, name, variation, specs):
    seed = int.from_bytes(hashlib.sha256((name+str(variation)).encode()).digest()[:8], 'little')
    rng = random.Random(seed)
    p = {key: default for _,key,_,_,default,_ in specs}
    def setp(**values):
        for symbol,value in values.items():
            p[next(k for s,k,*_ in specs if s==symbol)]=value
    # Category recipes establish synthesis identity; individual voicings alter excitation,
    # oscillator structure, filter motion, and articulation, rather than just FX or names.
    setp(Volume=0.76, Detune=rng.uniform(2,14), Brightness=rng.uniform(.28,.85),
         Cutoff=rng.uniform(2600,10000), Decay=rng.uniform(.35,1.5),
         FilterEnv=rng.uniform(.2,2.8), Lfo1Rate=rng.uniform(.08,.65),
         Lfo1Depth=rng.uniform(.02,.15), Wavetable=rng.uniform(0,3),
         FmRatio=rng.choice([.5,1,1.5,2,2.5,3,3.5,4.01]), FmIndex=rng.uniform(.7,5),
         OscMix=rng.uniform(.12,.48), PulseWidth=rng.uniform(.18,.72),
         Reverb=rng.uniform(.08,.3), ReverbSize=rng.uniform(.3,.82))
    if category=='Guitars':
        setp(Engine=4, Attack=.002, Decay=rng.uniform(.7,2.5), Sustain=0,
             Release=.24, StringDamping=rng.uniform(.975,.9985), FilterEnv=1.1,
             FilterDecay=.12, Chorus=.17)
    elif category=='Pianos and Keys':
        setp(Engine=rng.choice([1,7]), Wave1=0, Attack=.002, Decay=rng.uniform(.6,2.9),
             Sustain=.06, Release=rng.uniform(.12,.8), FmIndex=rng.uniform(.6,3.8), FmRatio=rng.choice([1,2,3,4.01]), FilterEnv=1.4)
    elif category=='Bells':
        setp(Engine=1, Attack=.001, Decay=rng.uniform(1.2,4.5), Sustain=0, Release=1.2,
             FmRatio=rng.choice([1.414,2.71,3.5,5.19,7.01]), FmIndex=rng.uniform(1.2,6.2),
             Cutoff=14000, FilterEnv=0, Reverb=.32)
    elif category=='Pads':
        setp(Engine=rng.choice([0,2,3]), Attack=rng.uniform(.4,2.8), Decay=2.5,
             Sustain=.76, Release=rng.uniform(1.5,4.5), Chorus=.48, Reverb=.38,
             FilterEnv=1.4, FilterAttack=2, FilterDecay=3, Lfo1Depth=.2)
    elif category=='Bass':
        setp(Engine=rng.choice([0,1]), Mode=1, Voices=1, Wave1=2, Wave2=0,
             Attack=.002, Decay=rng.uniform(.12,.6), Sustain=.5, Release=.12,
             Cutoff=rng.uniform(180,1900), FilterEnv=rng.uniform(1,4), FilterDecay=.15,
             Reverb=0, Chorus=0, Drive=.18, Glide=.04, Width=.35)
    elif category=='Flutes':
        setp(Engine=2, Harmonics=rng.randint(3,8), Brightness=.15, Wave1=0,
             Attack=rng.uniform(.045,.18), Sustain=.8, Release=.26, Vibrato=.19,
             Cutoff=rng.uniform(1400,3800), FilterEnv=.2, Breath=.24, Reverb=.22)
    elif category=='Plucks':
        setp(Engine=rng.choice([0,1,3,4]), Attack=.001, Decay=rng.uniform(.13,.65),
             Sustain=0, Release=.14, FilterDecay=.16, FilterEnv=3, Delay=.18,
             DelayTime=rng.choice([.1875,.25,.375]), StringDamping=.983)
    elif category=='Sax-Style Leads':
        setp(Engine=5, Vowel=rng.uniform(.4,2.8), Breath=.16, Choir=0, Attack=.055,
             Decay=.35, Sustain=.72, Release=.22, Vibrato=.25, FormantShift=-5,
             Mode=2, Glide=.06, Cutoff=7000, Drive=.16, Reverb=.12)
    elif category=='Strings':
        setp(Engine=rng.choice([0,2,3]), Wave1=2, Wave2=2, Detune=rng.uniform(7,22),
             Attack=rng.uniform(.22,.85), Sustain=.8, Release=1.3, Chorus=.55,
             Cutoff=rng.uniform(2800,6500), Brightness=.65, Reverb=.3)
    elif category.startswith('Vocals'):
        setp(Engine=5, Vowel=rng.uniform(0,4), FormantShift=rng.uniform(-5,5),
             Breath=rng.uniform(.06,.26), Choir=rng.uniform(.15,.65),
             Attack=rng.uniform(.06,.42), Decay=rng.uniform(.5,1.7), Sustain=.7,
             Release=rng.uniform(.35,1.3), FilterEnv=.1, Cutoff=13000,
             Lfo1Dest=4, Lfo1Depth=rng.uniform(.06,.3), Vibrato=rng.uniform(.06,.24),
             Detune=rng.uniform(3,18), Chorus=.25, Reverb=.26)
        if 'Southern' in category: setp(FormantShift=-8, Syllable=.7, Drive=.3, CrusherBits=10, Reverb=.15)
        if 'Horror' in category: setp(FormantShift=-7, Breath=.48, Attack=.8, Release=2.2, Vowel=3.3, Reverb=.48, Phaser=.35)
        if 'Gaming' in category: setp(CrusherBits=rng.choice([6,8,10,12]), CrusherRate=rng.choice([2,3,4]), Syllable=.65, Attack=.012, Sustain=.4)
        if 'Global' in category: setp(Lfo2Dest=1,Lfo2Depth=rng.uniform(.04,.2), Lfo2Rate=rng.uniform(3,7), Syllable=rng.uniform(.15,.5))
    elif category=='Retro Games':
        setp(Engine=6, Wave1=rng.choice([0,1,3,4]), Wave2=0, Attack=.003,
             Decay=.4, Sustain=.5, Release=.15, CrusherBits=8, Reverb=.12,
             Cutoff=12000, Lfo1Depth=.03)
        if 'FM' in name or 'Bell' in name: setp(Engine=1, FmRatio=3.5, FmIndex=4)
        if 'Keys' in name: setp(Engine=7, Sustain=.12)
        if 'Pluck' in name: setp(Sustain=0, Decay=.2)
        if 'Pad' in name or 'Strings' in name: setp(Engine=3, Attack=.7, Release=1.5, Chorus=.4)
        if 'Choir' in name: setp(Engine=5, Vowel=1.4, Choir=.6)
    # Named articulation / tone voicings shared across families.
    low=name.lower()
    if any(w in low for w in ['lo-fi','vhs','tape','vintage tv','radio']):
        setp(Wow=.3, CrusherBits=11, Cutoff=min(p['filter.cutoff'],4200), EqHigh=-4, Drive=.18)
    if any(w in low for w in ['dark','horror','haunted','shadow']):
        setp(Cutoff=min(p['filter.cutoff'],2400), Lfo1Rate=.17, Reverb=.4, EqHigh=-3)
    if any(w in low for w in ['dream','ethereal','angelic','nostalg','space']):
        setp(Reverb=.43, Delay=.2, Chorus=.4, DelayTime=.375)
    if any(w in low for w in ['funk','slap','pizzicato','chop','chant','party']):
        setp(Attack=.002, Decay=.22, Sustain=.12, Release=.12, FilterDecay=.13, FilterEnv=2.8)
    if 'choir' in low: setp(Choir=.8, Detune=14, Attack=.4, Release=1.5)
    if 'female' in low or 'high-register' in low or 'children' in low: setp(FormantShift=6.5, Breath=.18)
    if 'sub bass' in low or '808' in low: setp(Engine=0,Wave1=0,Wave2=0,Cutoff=800,Decay=1.6,Sustain=.05,FilterEnv=0,Drive=.22)
    if 'reese' in low: setp(Engine=0, Wave1=2, Wave2=2, Detune=28, OscMix=.5, Cutoff=900)
    if 'fm' in low: setp(Engine=1)
    if 'nylon' in low: setp(Brightness=.22, StringDamping=.987)
    if 'acoustic' in low: setp(Brightness=.8, StringDamping=.996)
    if 'tine' in low: setp(Engine=1, FmRatio=1, FmIndex=1.4, Decay=2.1)
    if 'reed' in low: setp(Engine=7, Brightness=.7, Drive=.24)
    if 'music box' in low or 'toy piano' in low: setp(Engine=7, Brightness=.9, Decay=1.1)
    # Named acoustic/keyboard/ensemble profiles take precedence over broad style words.
    profile = (hiphop_designs() if category.startswith('Hip-Hop - ') else retro_designs() if category=='Retro Games' else vocal_designs() if category.startswith('Vocals') else instrument_designs()).get(name)
    if profile:
        assert profile['category'] == category
        setp(**profile['parameters'])
    refinement=compatibility_refinements().get(name)
    if refinement and category!='Retro Games':setp(**refinement)
    setp(Volume=1)
    if variation:
        engine=int(p['engine'])
        if engine==0:
            setp(Wave2=(int(p['osc2.wave'])+2)%5,OscMix=.42,Detune=min(32,p['osc.detune']+8),PulseWidth=.22 if p['osc.pulse']>.35 else .62)
        elif engine==1:
            setp(FmRatio=p['fm.ratio']*1.5 if p['fm.ratio']<6 else p['fm.ratio']/2,FmIndex=p['fm.index']*1.45,FilterSustain=min(.8,p['filter.sustain']+.15),FilterDecay=p['filter.decay']*1.4)
        elif engine==2:
            setp(Harmonics=min(24,p['additive.harmonics']+6),Brightness=min(.98,p['tone.brightness']+.24))
        elif engine==3:
            setp(Wavetable=(p['wavetable.position']+1.1)%3,Detune=min(32,p['osc.detune']+9),OscMix=.38)
        elif engine==4:
            setp(Brightness=.1+.8*(1-p['tone.brightness']),StringDamping=p['string.damping']-.0015)
        elif engine==5:
            setp(Vowel=(p['formant.vowel']+1.25)%4,FormantShift=p['formant.shift']+3,Breath=min(.65,p['formant.breath']+.12),Choir=min(.9,p['formant.choir']+.25),Detune=min(32,p['osc.detune']+8))
        elif engine==6:
            setp(Wave1={0:4,1:3,2:1,3:4,4:1,5:3}[int(p['osc1.wave'])],PulseWidth=.22,CrusherBits=max(6,p['fx.crusherBits']-2))
        elif engine==7:
            setp(Brightness=(p['tone.brightness']+.43)%1)
        setp(Decay=p['amp.decay']*1.4,Attack=p['amp.attack']*1.25,Cutoff=p['filter.cutoff']*.72,
             FilterEnv=p['filter.env']*-.45,Delay=.2,Phaser=.16)
    if variation and profile:setp(**profile.get('afterglow',{}))
    full_name=name+(' - Afterglow' if variation else '')
    revision=diversity_refinements().get(full_name)
    if revision:setp(**revision['parameters'])
    for _,key,lo,hi,_,step in specs:
        v=max(lo,min(hi,p[key]))
        if step:v=lo+round((v-lo)/step)*step
        p[key]=round(v,6)
    tags=[category.split(' - ')[-1].lower(),'synthesis', 'afterglow' if variation else 'original']
    tags += [ENGINE_NAMES[int(p['engine'])]]
    if profile and category=='Retro Games':tags+=['designed retro',profile['technicalStyle']]
    if profile: tags+=['designed vocal' if category.startswith('Vocals') else 'designed instrument', 'percussive' if p['amp.sustain']<.15 else 'sustained']
    if profile and category.startswith('Hip-Hop - '):tags+=profile['tags']
    if category.startswith('Vocals'): tags+=['formant','synthetic vowels']
    return dict(id='factory.'+hashlib.sha256(full_name.encode()).hexdigest()[:16], name=full_name,
                category=category,tags=tags,parameters=p)


ENGINE_NAMES=['subtractive','fm','additive','wavetable','physical modeling','formant','chiptune','modal']
_DESIGNS=None
_REFINEMENTS={}
def instrument_designs():
    global _DESIGNS,_REFINEMENTS
    if _DESIGNS is None:
        data=json.loads((ROOT/'Presets/instrument-designs.json').read_text())
        assert data['format']=='gba2.instrument-designs' and data['schemaVersion']==1
        _DESIGNS=data['designs'];_REFINEMENTS=data.get('compatibilityRefinements',{})
        specs={row[0]:row for row in schema()}
        expected={name for cat,names in GROUPS.items() if not cat.startswith('Vocals') and cat!='Retro Games' for name in names.split('|')}
        assert set(_DESIGNS)==expected
        parameter_sets=[parameters for profile in _DESIGNS.values() for parameters in (profile['parameters'],profile.get('afterglow',{}))]
        assert set(_REFINEMENTS)<={name for names in GROUPS.values() for name in names.split('|')}
        parameter_sets+=list(_REFINEMENTS.values())
        for parameters in parameter_sets:
            for symbol,value in parameters.items():
                assert symbol in specs
                _,_,lo,hi,_,_=specs[symbol]
                assert type(value) in (int,float) and lo<=value<=hi,(symbol,value)
    return _DESIGNS

def compatibility_refinements():
    instrument_designs()
    return _REFINEMENTS

_VOCALS=None
def vocal_designs():
    global _VOCALS
    if _VOCALS is None:
        data=json.loads((ROOT/'Presets/vocal-designs.json').read_text())
        assert data['format']=='gba2.vocal-designs' and data['schemaVersion']==1
        _VOCALS=data['designs'];specs={row[0]:row for row in schema()}
        expected={name:cat for cat,names in GROUPS.items() if cat.startswith('Vocals') for name in names.split('|')}
        assert set(_VOCALS)==set(expected)
        for name,profile in _VOCALS.items():
            assert profile['category']==expected[name] and profile['designNotes']
            assert profile['parameters']['Engine']==5
            for parameters in (profile['parameters'],profile['afterglow']):
                for symbol,value in parameters.items():
                    assert symbol in specs
                    _,_,lo,hi,_,_=specs[symbol]
                    assert type(value) in (int,float) and lo<=value<=hi,(name,symbol,value)
    return _VOCALS

_RETRO=None
def retro_designs():
    global _RETRO
    if _RETRO is None:
        data=json.loads((ROOT/'Presets/retro-designs.json').read_text())
        assert data['format']=='gba2.retro-designs' and data['schemaVersion']==1
        _RETRO=data['designs'];specs={row[0]:row for row in schema()}
        assert set(_RETRO)==set(GROUPS['Retro Games'].split('|'))
        for name,profile in _RETRO.items():
            assert profile['category']=='Retro Games' and profile['technicalStyle'] and profile['designNotes']
            for parameters in (profile['parameters'],profile['afterglow']):
                for symbol,value in parameters.items():
                    assert symbol in specs
                    _,_,lo,hi,_,_=specs[symbol]
                    assert type(value) in (int,float) and lo<=value<=hi,(name,symbol,value)
    return _RETRO

_HIPHOP=None
def hiphop_designs():
    global _HIPHOP
    if _HIPHOP is None:
        data=json.loads((ROOT/'Presets/hiphop-designs.json').read_text())
        assert data['format']=='gba2.hiphop-designs' and data['schemaVersion']==1
        _HIPHOP=data['designs'];specs={row[0]:row for row in schema()}
        assert len(_HIPHOP)==60
        counts={category:0 for category in ('Hip-Hop - Detroit','Hip-Hop - West Coast','Hip-Hop - Florida')}
        for name,profile in _HIPHOP.items():
            assert profile['category'] in counts and profile['designNotes'] and profile['tags']
            assert profile['family'] in ('piano','bell','pluck','pad','vox','bass','lead')
            counts[profile['category']]+=1
            for parameters in (profile['parameters'],profile['afterglow']):
                for symbol,value in parameters.items():
                    assert symbol in specs
                    _,_,lo,hi,_,_=specs[symbol]
                    assert type(value) in (int,float) and lo<=value<=hi,(name,symbol,value)
        assert set(counts.values())=={20}
    return _HIPHOP

_DIVERSITY=None
def diversity_refinements():
    global _DIVERSITY
    if _DIVERSITY is None:
        data=json.loads((ROOT/'Presets/diversity-refinements.json').read_text())
        assert data['format']=='gba2.diversity-refinements' and data['schemaVersion']==1
        _DIVERSITY=data['presets'];specs={row[0]:row for row in schema()}
        known={name+suffix for names in GROUPS.values() for name in names.split('|') for suffix in ('',' - Afterglow')}
        known|={name+suffix for name in hiphop_designs() for suffix in ('',' - Afterglow')}
        assert set(_DIVERSITY)<=known
        for name,revision in _DIVERSITY.items():
            assert revision['designNotes'] and revision['parameters']
            for symbol,value in revision['parameters'].items():
                assert symbol in specs
                _,_,lo,hi,_,_=specs[symbol]
                assert type(value) in (int,float) and lo<=value<=hi,(name,symbol,value)
    return _DIVERSITY

def recipe_digest():
    digest=hashlib.sha256()
    for name in ['scripts/generate_presets.py','Presets/instrument-designs.json','Presets/vocal-designs.json','Presets/retro-designs.json','Presets/hiphop-designs.json','Presets/diversity-refinements.json','Presets/groove-designs.json','Presets/brass-designs.json','Presets/expansion-designs.json','Source/dsp/RetroEngine.h','Source/dsp/ParameterList.inc','Source/dsp/VocalEngine.h','Source/dsp/SynthEngine.cpp','Source/dsp/SynthEngine.h','Source/dsp/Effects.cpp','Source/dsp/Effects.h','Source/dsp/DspMath.h']:
        digest.update(name.encode()+b'\0'+(ROOT/name).read_bytes().replace(b'\r\n',b'\n'))
    return digest.hexdigest()

def groove_patches(specs):
    """Individually authored complete patches; no automatic paired variations."""
    data=json.loads((ROOT/'Presets/groove-designs.json').read_text())
    assert data['format']=='gba2.groove-designs' and data['schemaVersion']==1
    counts={'Hip-Hop - Detroit':20,'Hip-Hop - Chicago Drill':24,'Funk':24,'Japan Inspired':24,'80s Pop':28}
    actual=dict.fromkeys(counts,0);patches=[]
    for name,profile in data['designs'].items():
        assert profile['category'] in counts and profile['designNotes'] and profile['tags']
        actual[profile['category']]+=1
        assert set(profile['parameters'])=={row[0] for row in specs},name
        values={}
        for symbol,key,lo,hi,_,step in specs:
            value=profile['parameters'][symbol]
            assert type(value) in (int,float) and lo<=value<=hi,(name,symbol,value)
            assert not step or abs((value-lo)/step-round((value-lo)/step))<1e-4,(name,symbol,value)
            values[key]=round(value,6)
        tags=list(dict.fromkeys(['synthesis','original','groove pack',profile['family'],ENGINE_NAMES[int(values['engine'])],*profile['tags']]))
        patches.append(dict(id='factory.'+hashlib.sha256(name.encode()).hexdigest()[:16],name=name,
                            category=profile['category'],tags=tags,parameters=values))
    assert actual==counts,actual
    return patches

def brass_patches(specs):
    data=json.loads((ROOT/'Presets/brass-designs.json').read_text())
    assert data['format']=='gba2.brass-designs' and data['schemaVersion']==1
    counts={'Trumpets':4,'Trombones':4,'Horns':4,'Brass Sections':6,'Synth Brass':6}
    actual=dict.fromkeys(counts,0);patches=[]
    for name,profile in data['designs'].items():
        assert profile['category']=='Brass' and profile['subcategory'] in counts and profile['designNotes'] and profile['tags']
        actual[profile['subcategory']]+=1
        assert set(profile['parameters'])=={row[0] for row in specs},name
        values={}
        for symbol,key,lo,hi,_,step in specs:
            value=profile['parameters'][symbol]
            assert type(value) in (int,float) and lo<=value<=hi,(name,symbol,value)
            assert not step or abs((value-lo)/step-round((value-lo)/step))<1e-4,(name,symbol,value)
            values[key]=round(value,6)
        assert values['voice.count']<=8 and values['amp.release']<=1.6
        patches.append(dict(id='factory.'+hashlib.sha256(name.encode()).hexdigest()[:16],name=name,
                            category='Brass',subcategory=profile['subcategory'],
                            tags=['synthesis','original',ENGINE_NAMES[int(values['engine'])],*profile['tags']],parameters=values))
    assert actual==counts,actual
    return patches

def expansion_patches(specs):
    data=json.loads((ROOT/'Presets/expansion-designs.json').read_text())
    assert data['format']=='gba2.expansion-designs' and data['schemaVersion']==1
    counts={'Percussion':24,'Woodwinds':20,'Vocals':12,'Hip Hop Milwaukee':24}
    actual=dict.fromkeys(counts,0);patches=[]
    for name,profile in data['designs'].items():
        assert profile['category'] in counts and profile['designNotes'] and profile['tags']
        actual[profile['category']]+=1
        assert set(profile['parameters'])=={row[0] for row in specs},name
        values={}
        for symbol,key,lo,hi,_,step in specs:
            value=profile['parameters'][symbol]
            assert type(value) in (int,float) and lo<=value<=hi,(name,symbol,value)
            assert not step or abs((value-lo)/step-round((value-lo)/step))<1e-4,(name,symbol,value)
            values[key]=round(value,6)
        assert values['voice.count']<=8 and values['amp.release']<=1.6
        patch=dict(id='factory.'+hashlib.sha256(name.encode()).hexdigest()[:16],name=name,
                   category=profile['category'],tags=['synthesis','original','expansion',ENGINE_NAMES[int(values['engine'])],*profile['tags']],parameters=values)
        if profile['category']!='Hip Hop Milwaukee':patch['subcategory']=profile['family']
        patches.append(patch)
    assert actual==counts,actual
    return patches

CATEGORY_NAMES = {
    'Hip-Hop - Detroit': 'Detroit',
    'Hip-Hop - West Coast': 'West Coast',
    'Hip-Hop - Florida': 'Florida',
    'Hip-Hop - Chicago Drill': 'Chicago',
    'Hip Hop Milwaukee': 'Milwaukee',
}

def generate(uncalibrated=False):
    specs=schema()
    patches=[make_patch(category,name,variation,specs) for category,names in GROUPS.items()
             for name in names.split('|') for variation in range(2)]
    patches += [make_patch(profile['category'],name,variation,specs)
                for name,profile in hiphop_designs().items() for variation in range(2)]
    patches += groove_patches(specs)
    patches += brass_patches(specs)
    patches += expansion_patches(specs)
    # Keep legacy recipe groups and search tags: their names participate in the
    # authored voicing logic. Merge only the browser metadata, after synthesis.
    for patch in patches:
        if patch['category'] in ('Vocals - Global Pop Textures', 'Vocals - Horror', 'Vocals - Choirs and Textures'):
            patch['category'] = 'Vocals'
        elif patch['category']=='Sax-Style Leads':
            patch['category']='Brass';patch['subcategory']='Sax-Style Leads'
        patch['category']=CATEGORY_NAMES.get(patch['category'],patch['category'])
    index=json.loads((ROOT/'Presets/factory-index.json').read_text())
    assert index['format']=='gba2.factory-index' and index['schemaVersion']==1
    by_id={p['id']:p for p in patches}
    ordered=[]
    programs=[]
    for program,identity in enumerate(index['presets']):
        if identity.get('retired',False):
            assert identity['id'] not in by_id,'Retired preset must not have an active recipe.'
            programs.append(dict(id=identity['id'],name=f'Reserved {program+1} (Default)',patchIndex=-1))
            continue
        patch=by_id.pop(identity['id'])
        assert all(patch[key]==identity[key] for key in ('id','name','category'))
        assert patch.get('subcategory','')==identity.get('subcategory','')
        programs.append(dict(id=identity['id'],name=identity['name'],patchIndex=len(ordered)))
        ordered.append(patch)
    assert not by_id,'Register new factory identities before generating the bank.'
    patches=ordered
    if not uncalibrated:
        levels=json.loads((ROOT/'Presets/factory-levels.json').read_text())
        assert levels['format']=='gba2.factory-levels' and levels['schemaVersion']==1
        assert levels['recipeSha256']==recipe_digest(),'Recipes changed: regenerate uncalibrated data, render measurements, then recalibrate.'
        assert set(levels['volumes'])=={p['id'] for p in patches}
        for patch in patches:
            gain=levels['volumes'][patch['id']]
            assert isinstance(gain,(int,float)) and .05<=gain<=.95
            patch['parameters']['volume']=gain
    assert len(patches)>=300
    assert len({json.dumps(p['parameters'],sort_keys=True) for p in patches})==len(patches)
    payload=dict(format='gba2.factory',schemaVersion=1,description='Original synthesis parameters; no audio, voices, phrases, or game data.',presets=patches)
    (ROOT/'Presets/factory.json').write_text(json.dumps(payload,indent=2)+'\n')
    lines=['// Generated by scripts/generate_presets.py. Do not edit by hand.', '#pragma once', '#include "dsp/Parameters.h"',
           'namespace gba {', 'struct FactoryPatch { const char *id,*name,*category,*tags; std::array<float,parameterCount> values; int program; const char *subcategory; };',
           f'inline constexpr std::array<FactoryPatch,{len(patches)}> factoryPatches {{{{']
    slots={p['id']:i for i,p in enumerate(programs)}
    for p in patches:
        strings=', '.join(json.dumps(p[k]) for k in ['id','name','category'])+', '+json.dumps(' '.join(p['tags']))
        values=', '.join(format(float(p['parameters'][key]),'.8f')+'f' for _,key,*_ in specs)
        lines.append('    {'+strings+', {{'+values+'}}, '+str(slots[p['id']])+', '+json.dumps(p.get('subcategory',''))+'},')
    lines+=['}};', 'struct FactoryProgram { const char *id,*name; int patchIndex; };',
            f'inline constexpr std::array<FactoryProgram,{len(programs)}> factoryPrograms {{{{']
    for program in programs:
        lines.append('    {'+json.dumps(program['id'])+', '+json.dumps(program['name'])+', '+str(program['patchIndex'])+'},')
    lines+=['}};','}']
    (ROOT/'Source/FactoryData.h').write_text('\n'.join(lines)+'\n')
    print(f'Generated {len(patches)} patches, {len({p["category"] for p in patches})} categories, {len(specs)} stable parameters.')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--uncalibrated',action='store_true');args=parser.parse_args();generate(args.uncalibrated)
