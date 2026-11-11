#!/usr/bin/env python3
"""Remove the factory Percussion bank while retaining all existing host slots.
Run only on the 0.18.2 source or verify an already migrated 0.18.3 tree.
Generated legacy fixtures contain the previous parameter values, never audio.
"""
from pathlib import Path
import importlib.util, json, re, subprocess
ROOT=Path(__file__).resolve().parents[1]
def read(name): return (ROOT/name).read_text(encoding='utf-8')
def write(name,text):
    p=ROOT/name;p.parent.mkdir(parents=True,exist_ok=True)
    with p.open('w',encoding='utf-8',newline='\n') as stream:stream.write(text)
def edit(name,old,new):
    text=read(name)
    if old not in text:raise RuntimeError('Missing migration anchor: '+name+' / '+old[:80])
    write(name,text.replace(old,new))
def dump(name,value):write(name,json.dumps(value,indent=2)+'\n')
def main():
    bank=json.loads(read('Presets/factory.json'))
    old=bank['presets'];removed=[p for p in old if p['category']=='Percussion']
    if not removed:
        assert len(old)==654 and len({p['category'] for p in old})==20
        registry=json.loads(read('Presets/factory-index.json'))['presets']
        assert len(registry)==754 and sum(bool(p.get('retired')) for p in registry)==100
        assert 'VERSION 0.18.3 ' in read('CMakeLists.txt')
        baseline=json.loads(subprocess.check_output(['git','show','9517f59b4a32290209dccb0307d08d17822a0621:Presets/factory.json'],cwd=ROOT,text=True))['presets']
        removed_ids={p['id'] for p in baseline if p['category']=='Percussion'}
        assert len(removed_ids)==24 and old==[p for p in baseline if p['id'] not in removed_ids]
        previous_index=json.loads(subprocess.check_output(['git','show','9517f59b4a32290209dccb0307d08d17822a0621:Presets/factory-index.json'],cwd=ROOT,text=True))['presets']
        for before,after in zip(previous_index,registry):
            expected=dict(before)
            if expected['id'] in removed_ids:expected['retired']=True
            assert after==expected
        previous_header=subprocess.check_output(['git','show','9517f59b4a32290209dccb0307d08d17822a0621:Source/FactoryData.h'],cwd=ROOT,text=True)
        old_rows=[line for line in previous_header.splitlines() if line.lstrip().startswith('{"factory.') and '{{' in line and '"Percussion"' not in line]
        new_rows=[line for line in read('Source/FactoryData.h').splitlines() if line.lstrip().startswith('{"factory.') and '{{' in line]
        assert old_rows==new_rows and len(new_rows)==654
        dump('build/percussion-removal-proof.json',{'removedPresets':24,'remainingPresets':654,'categories':20,'hostSlots':754,'retiredSlots':100,'retainedDictionariesIdentical':True,'retainedGeneratedRowsIdentical':True,'registryIdentitiesUnchanged':True,'removedIds':sorted(removed_ids)})
        print('PASS already migrated 0.18.3 bank, retained sound dictionaries/rows and 100 reserved slots');return
    assert len(old)==678 and len(removed)==24
    ids={p['id'] for p in removed};expected=[p for p in old if p['id'] not in ids]
    old_header=read('Source/FactoryData.h')
    rows=[line for line in old_header.splitlines() if line.lstrip().startswith('{"factory.') and '"Percussion"' in line]
    assert len(rows)==24
    write('Tests/RemovedPercussionPatches.h',
          '// Parameter fixtures captured from the 0.18.2 bank. Test-only, no audio.\n'
          '#pragma once\n#include "FactoryData.h"\nnamespace removed_percussion {\n'
          'inline constexpr std::array<gba::FactoryPatch,24> patches {{\n'
          +'\n'.join(rows)+'\n}};\n}\n')
    registry=json.loads(read('Presets/factory-index.json'));before=[dict(p) for p in registry['presets']]
    assert len(before)==754 and sum(bool(p.get('retired')) for p in before)==76
    for row in registry['presets']:
        if row['id'] in ids:
            assert row['category']=='Percussion' and not row.get('retired')
            row['retired']=True
    dump('Presets/factory-index.json',registry)
    designs=json.loads(read('Presets/expansion-designs.json'));count=len(designs['designs'])
    designs['designs']={name:patch for name,patch in designs['designs'].items() if patch['category']!='Percussion'}
    assert count==80 and len(designs['designs'])==56
    designs['description']='56 retained independently authored native synthesis patches. Vox additions remain in Vocals. Percussion retired in 0.18.3. No audio assets.'
    dump('Presets/expansion-designs.json',designs)
    edit('scripts/generate_presets.py',"'Percussion':24,",'')
    spec=importlib.util.spec_from_file_location('removed_percussion_generator',ROOT/'scripts/generate_presets.py')
    generator=importlib.util.module_from_spec(spec);spec.loader.exec_module(generator)
    levels=json.loads(read('Presets/factory-levels.json'))
    previous_gains=dict(levels['volumes'])
    for key in ['volumes','presetTargets']:
        levels[key]={k:v for k,v in levels[key].items() if k not in ids}
    levels['gainCeilingIds']=[k for k in levels['gainCeilingIds'] if k not in ids]
    levels['targets'].pop('Percussion')
    levels['recipeSha256']=generator.recipe_digest()
    dump('Presets/factory-levels.json',levels);generator.generate()
    now=json.loads(read('Presets/factory.json'))['presets']
    assert now==expected and len(now)==654 and len({p['category'] for p in now})==20
    assert levels['volumes']=={p['id']:previous_gains[p['id']] for p in expected}
    for original,current in zip(before,registry['presets']):
        target=dict(original)
        if target['id'] in ids:target['retired']=True
        assert current==target
    # Each retained generated patch row must be exactly the old row. Program
    # table patch indices are regenerated; the public slot numbers never move.
    new_header=read('Source/FactoryData.h')
    kept_rows=[line for line in new_header.splitlines() if line.lstrip().startswith('{"factory.') and '{{' in line]
    old_rows=[line for line in old_header.splitlines() if line.lstrip().startswith('{"factory.') and '{{' in line and '"Percussion"' not in line]
    assert kept_rows==old_rows and len(kept_rows)==654
    edit('CMakeLists.txt','VERSION 0.18.2 ','VERSION 0.18.3 ')
    edit('Tests/LibraryTests.cpp','==678','==654')
    edit('Tests/LibraryTests.cpp','std::size_t(679)','std::size_t(655)')
    edit('Tests/LibraryTests.cpp','std::array<std::pair<const char*,std::size_t>,4>','std::array<std::pair<const char*,std::size_t>,3>')
    edit('Tests/LibraryTests.cpp','{"Percussion",24},','')
    edit('Tests/PresetTests.cpp','factoryPatches.size()==678&&categories.size()==21','factoryPatches.size()==654&&categories.size()==20')
    edit('Tests/PresetTests.cpp','{"Percussion",24},','')
    edit('Tests/PresetTests.cpp','{"Percussion","Woodwinds","Vocals","Milwaukee"}','{"Woodwinds","Vocals","Milwaukee"}')
    edit('Tests/PluginTests.cpp','#include "RemovedVocalStates.h"','#include "RemovedVocalStates.h"\n#include "RemovedPercussionPatches.h"')
    for a,b in [('size()==678','size()==654'),('categories().size()==21','categories().size()==20'),('i<21','i<20'),('visited.size()==21','visited.size()==20'),('21 browser categories','20 browser categories'),('21-category navigation','20-category navigation')]:
        edit('Tests/PluginTests.cpp',a,b)
    text=read('Tests/PluginTests.cpp')
    start=text.index('            categoryFilter->setText("Percussion"')
    end=text.index('            categoryFilter->setText("Woodwinds"',start)
    text=text[:start]+'''            bool percussionShown=false;for(int i=0;i<categoryFilter->getNumItems();++i)percussionShown|=categoryFilter->getItemText(i)=="Percussion";
            check(!percussionShown&&!p.presets.categories().contains("Percussion"),"Percussion remains in the preset folder menu");
            snapshot(*editor,"ui-snapshots/Factory-Without-Percussion-960.png");
'''+text[end:];write('Tests/PluginTests.cpp',text)
    app_test=r'''void removedPercussionProjects(const juce::File& root) {
    auto p=std::make_unique<GbaProcessor>(root.getChildFile("RemovedPercussion"));
    p->setRateAndBufferSizeDetails(48000,256);p->prepareToPlay(48000,256);
    const auto defaultSettings=p->snapshot();
    check(p->presets.all().size()==654&&!p->presets.categories().contains("Percussion"),"Percussion remains in factory browsing");
    for(const auto& patch:removed_percussion::patches) {
        check(!p->presets.find(patch.id)&&!p->presets.load(patch.id),"Removed percussion remains loadable as a factory patch");
        check(p->getProgramName(patch.program).startsWith("Reserved "),"Percussion host slot was removed or still advertises a patch");
        // Reconstruct a legacy project using the captured 0.18.2 parameters and
        // the unchanged project serializer. This is not an old-binary export.
        GbaPreset legacy;legacy.id=patch.id;legacy.name=patch.name;legacy.category="Percussion";legacy.tags=patch.tags;legacy.settings.values=patch.values;
        p->applyPreset(legacy);set(*p,"volume",legacy.settings[gba::Param::Volume]*.9f);set(*p,"fx.eqLow",2.5f);
        const auto edited=p->snapshot();juce::MemoryBlock state;p->getStateInformation(state);
        p->presets.restoreDefault(false);p->setStateInformation(state.getData(),int(state.getSize()));
        check(p->presetId()==patch.id&&p->presetName()==patch.name&&p->getCurrentProgram()==patch.program,"Legacy percussion identity or program moved");
        check(p->presetCategory()=="Saved Project"&&p->stateNotice().contains("saved in your project was restored"),"Legacy percussion was not restored as a saved project");
        check(p->snapshot().values==edited.values,"Legacy percussion edits were replaced with current/default settings");
        p->reset();juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;
        const int note=juce::String(patch.tags).contains("bass")?48:60;
        midi.addEvent(juce::MidiMessage::noteOn(1,note,.8f),0);p->processBlock(audio,midi);
        float peak=audio.getMagnitude(0,256);
        for(int n=0;n<64;++n){render(*p,audio,1);peak=std::max(peak,audio.getMagnitude(0,256));}
        check(std::isfinite(peak)&&peak>.00001f,"Restored percussion project produced no audible synthesis");
        p->panic();render(*p,audio,1);juce::MemoryBlock saved;p->getStateInformation(saved);
        p->presets.restoreDefault(false);p->setStateInformation(saved.getData(),int(saved.getSize()));
        check(p->snapshot().values==edited.values&&p->getCurrentProgram()==patch.program,"Resaving a percussion project changed its sound or program");
        p->setCurrentProgram(patch.program);p->servicePendingProgram();
        check(p->snapshot().values==edited.values,"Host program echo overwrote restored percussion edits");
        p->presets.restoreDefault(false);p->setCurrentProgram(patch.program);p->servicePendingProgram();
        check(p->getCurrentProgram()==patch.program&&p->snapshot().values==defaultSettings.values,"Reserved percussion slot does not select the documented default");
        p->setStateInformation(saved.getData(),int(saved.getSize()));
    }
    const auto kept=p->snapshot();check(p->presets.saveAs("Kept Percussion Project").wasOk(),"Cannot save a removed percussion sound as a user preset");
    const auto id=p->presetId();p->presets.restoreDefault(false);
    check(p->presets.load(id)&&p->snapshot().values==kept.values,"Saved percussion user preset cannot reload");
    for(int i=0;i<20;++i){p->presets.stepCategory(1);check(p->presetCategory()!="Percussion","D-pad reaches the removed Percussion category");}
    std::cout<<"PASS 24 retired percussion parameter-state projects: edited settings, audible notes, resave, host echo, reserved slots and user Save As\n";
}
'''
    edit('Tests/PluginTests.cpp','void unifiedVocals(const juce::File& root) {',app_test+'void unifiedVocals(const juce::File& root) {')
    edit('Tests/PluginTests.cpp','        removedVocalProjects(root);','        removedVocalProjects(root);\n        removedPercussionProjects(root);')
    edit('Tests/HostTests.cpp','#include <cmath>','#include <cmath>\n#include <limits>')
    edit('Tests/HostTests.cpp','#include "RemovedVocalStates.h"','#include "RemovedVocalStates.h"\n#include "RemovedPercussionPatches.h"')
    hosted=r'''        // Previous-bank parameter fixtures reconstructed into the processor's
        // schema and fed through the actual VST3 IComponent wrapper.
        for(const auto& patch:removed_percussion::patches) {
            juce::MemoryBlock seed;p->getStateInformation(seed);
            auto outer=juce::AudioProcessor::getXmlFromBinary(seed.getData(),int(seed.getSize()));
            const auto* component=outer?outer->getChildByName("IComponent"):nullptr;juce::MemoryBlock bytes;
            check(component&&bytes.fromBase64Encoding(component->getAllSubText()),"Cannot decode hosted state for percussion compatibility");
            auto xml=juce::AudioProcessor::getXmlFromBinary(bytes.getData(),int(bytes.getSize()));
            check(xml!=nullptr,"Missing hosted processor state");
            xml->setAttribute("presetId",patch.id);xml->setAttribute("presetName",patch.name);xml->setAttribute("presetCategory","Percussion");
            auto expected=patch.values;expected[gba::index(gba::Param::Volume)]*=.9f;expected[gba::index(gba::Param::EqLow)]=2.5f;
            int parameters=0;
            for(auto* child:xml->getChildIterator())for(std::size_t i=0;i<gba::parameterCount;++i)
                if(child->getStringAttribute("id")==gba::specs[i].id.data()){child->setAttribute("value",double(expected[i]));++parameters;}
            check(parameters==64,"Legacy percussion hosted fixture lost parameters");
            juce::MemoryBlock legacy;juce::AudioProcessor::copyXmlToBinary(*xml,legacy);
            juce::XmlElement envelope("VST3PluginState");envelope.createNewChildElement("IComponent")->addTextElement(legacy.toBase64Encoding());
            juce::MemoryBlock input;juce::AudioProcessor::copyXmlToBinary(envelope,input);p->setStateInformation(input.getData(),int(input.getSize()));
            check(p->getCurrentProgram()==patch.program&&p->getProgramName(patch.program).startsWith("Reserved "),"VST3 wrapper moved a retired percussion slot");
            juce::MemoryBlock saved;p->getStateInformation(saved);auto savedOuter=juce::AudioProcessor::getXmlFromBinary(saved.getData(),int(saved.getSize()));
            const auto* savedComponent=savedOuter?savedOuter->getChildByName("IComponent"):nullptr;juce::MemoryBlock savedBytes;
            check(savedComponent&&savedBytes.fromBase64Encoding(savedComponent->getAllSubText()),"Cannot decode restored percussion host state");
            auto restoredXml=juce::AudioProcessor::getXmlFromBinary(savedBytes.getData(),int(savedBytes.getSize()));
            check(restoredXml&&restoredXml->getStringAttribute("presetId")==patch.id&&restoredXml->getStringAttribute("presetCategory")=="Saved Project","Hosted percussion lost saved-project metadata");
            bool matches=true;int matched=0;
            for(auto* child:restoredXml->getChildIterator())for(std::size_t i=0;i<gba::parameterCount;++i)
                if(child->getStringAttribute("id")==gba::specs[i].id.data()){matches&=std::abs(float(child->getDoubleAttribute("value"))-expected[i])<=std::max(.0000001f,(gba::specs[i].max-gba::specs[i].min)*4*std::numeric_limits<float>::epsilon());++matched;}
            check(matches&&matched==64,"Actual VST3 percussion restore changed edited sound settings");
            p->setCurrentProgram(patch.program);pump(60);juce::MemoryBlock echo;p->getStateInformation(echo);
            check(echo==saved,"Actual VST3 program echo overwrote restored percussion settings");
            prepare(*p,48000,256);juce::AudioBuffer<float> percussionAudio(2,256);juce::MidiBuffer note;
            note.addEvent(juce::MidiMessage::noteOn(1,juce::String(patch.tags).contains("bass")?48:60,.8f),0);
            float peak=process(*p,percussionAudio,note);peak=std::max(peak,render(*p,percussionAudio,64));
            check(peak>.00001f,"Actual VST3 restored percussion project is silent");p->releaseResources();
        }
        std::cout<<"PASS 24 retired percussion VST3 parameter-state projects: settings, audible notes, reserved programs and host echo\n";
'''
    edit('Tests/HostTests.cpp','        p=load(); // Keep the general host tests independent of fixture voicings.',hosted+'        p=load(); // Keep the general host tests independent of fixture voicings.')
    edit('scripts/test-installer-windows.ps1','-ne 678','-ne 654')
    edit('scripts/test-installer-windows.ps1','-ne 21','-ne 20')
    edit('scripts/test-installer-windows.ps1',"'Percussion'=24;",'')
    edit('scripts/test-installer-windows.ps1',"if(@($factory.presets | Where-Object category -eq 'Vocals')","if($categories -contains 'Percussion') {throw 'Removed Percussion category remains installed.'}\n    if(@($factory.presets | Where-Object category -eq 'Vocals')")
    edit('scripts/test-installer-windows.ps1','0.18.2','0.18.3')
    edit('scripts/test-installer-windows.ps1','0.18.1','0.18.2')
    # Workflow changes are applied through the repository connection, not the build bot.
    edit('scripts/source_files.py',"REQUIRED = {","REQUIRED = {\n    'Tests/RemovedPercussionPatches.h', 'docs/RELEASE_0_18_3.md',\n    'scripts/remove-percussion-0183.py', '.github/workflows/remove-percussion-0183.yml',")
    edit('scripts/report_presets.py','The 120 groove-pack and 24 new brass designs are individually authored complete patches, without automatic Afterglow variants.','The 120 groove-pack, 24 brass and 56 retained expansion designs are individually authored complete patches, without automatic Afterglow variants. Percussion was removed from the active bank in 0.18.3.')
    text=read('README.md');text=text.replace('# G BOY ADVANCE 2 — 0.18.2','# G BOY ADVANCE 2 — 0.18.3',1)
    lines=text.splitlines()
    lines[2]='**0.18.3 removes the factory Percussion folder and its 24 presets. Remaining sounds and saved project settings are retained; the updated Windows installer is pending verification.**'
    lines[4]='**654 factory presets across 20 categories and 30 UI skins.** Woodwinds (20), Milwaukee (24) and Vox stabs inside Vocals remain available. START → SoundFonts still opens your SF2 library. All other categories keep their sounds.'
    text='\n'.join(lines)+'\n'
    text=text.replace('All 598 prior factory parameter dictionaries, 674 existing host slots and 64 parameter IDs remain intact. Eighty new host slots are appended (754 total, including 76 reserved); user presets, SoundFonts and projects remain available.',
                      'All 654 retained parameter dictionaries and 64 parameter IDs are unchanged. The 754 host slots remain stable, including 100 reserved slots. Projects containing removed Percussion sounds restore their saved settings; user presets and SoundFonts remain available.')
    text=text.replace('docs/RELEASE_0_18_2.md','docs/RELEASE_0_18_3.md').replace('feature/soundfonts-folder-0182','feature/percussion-removed-0183').replace('feature%2Fsoundfonts-folder-0182','feature%2Fpercussion-removed-0183').replace('[Download Windows installer 0.18.2]','[Previous Windows installer 0.18.2]')
    text=text.replace('- 678 factory patches across 21 categories','- 654 factory patches across 20 categories')
    write('README.md',text)
    text=read('docs/USER_MANUAL.md')
    text=text.replace('Current version: **0.18.2**','Current version: **0.18.3**').replace('RELEASE_0_18_2.md','RELEASE_0_18_3.md')
    lines=text.splitlines();lines[0]='> Current version: **0.18.3**. 654 presets in 20 factory categories, plus SoundFonts navigation, and 30 skins. Percussion is removed from Factory; existing projects retain their saved sounds. [Current changes](RELEASE_0_18_3.md).'
    text='\n'.join(lines)+'\n'
    text=text.replace('678 factory sounds in 21 categories','654 factory sounds in 20 categories')
    text=text.replace('## Percussion, Woodwinds, Milwaukee and Vox (0.18.0)','## Woodwinds, Milwaukee and Vox')
    start=text.index('START opens the browser. Choose Percussion')
    end=text.index('\n\nMilwaukee contains',start)
    text=text[:start]+'START opens the browser. Choose Woodwinds for 20 synthetic clarinet, oboe, bassoon, recorder and pipe sounds. Its subfolder selector narrows the instrument family. The former 24-sound Percussion folder was removed in 0.18.3. Older projects retain their stored Percussion settings under Saved Project; use Save As to keep those sounds as user presets.'+text[end:]
    write('docs/USER_MANUAL.md',text)
    text=read('docs/PRESET_FORMAT.md')
    text=text.replace('contains 80 independent complete 64-symbol synthesis dictionaries','contains 56 retained independent complete 64-symbol synthesis dictionaries')
    text=text.replace('Percussion has 24, Woodwinds 20, Hip Hop Milwaukee 24 and Vocals 12.','Woodwinds has 20, Milwaukee 24 and Vocals 12. The 24 Percussion designs were removed in 0.18.3.')
    text=text.replace('Percussion has 24, Woodwinds 20, Milwaukee 24 and Vocals 12.','Woodwinds has 20, Milwaukee 24 and Vocals 12. The 24 Percussion designs were removed in 0.18.3.')
    text=text.replace('Percussion/Woodwinds use family subfolders','Woodwinds uses family subfolders')
    write('docs/PRESET_FORMAT.md',text)
    text=read('docs/FACTORY_LIBRARY.md')
    write('docs/FACTORY_LIBRARY.md','> Current 0.18.3 bank: 654 presets in 20 categories; Percussion removed. The retained 654 sound dictionaries and gains are unchanged. The render report below is historical 0.18.0 evidence; current validation is in RELEASE_0_18_3.md.\n\n'+text)
    write('docs/RELEASE_0_18_3.md',"""# G BOY ADVANCE 2 — 0.18.3

Percussion is removed from the factory browser, category menu, search and D-pad category navigation. Its 24 factory presets are retired. All 654 remaining presets retain their names, tags, synthesis parameters, gains and public program numbers. There are 20 factory categories, plus the existing SoundFonts navigation folder, and 30 skins.

The total host-program count remains 754: 654 active sounds and 100 reserved slots. Projects with removed Percussion sounds restore the settings saved in the project under Saved Project. Save As can keep them as user presets. User files, SoundFonts, SF2 folder browsing and all other instrument categories are retained. No audio-DSP or parameter-ID changes and no new CPU improvement are claimed.

## Validation

Pending current Windows verification. The migration compares every retained factory dictionary and generated patch row with the exact 0.18.2 source and preserves all 754 registry identities. New tests reconstruct edited legacy parameter-state fixtures from the captured 0.18.2 Percussion values, then verify audible notes, state reload/resave, host-program echo and user Save As. These are parameter fixtures, not state exports from an old binary. Both VST3 wrapper hosts exercise the same cases through IComponent. The 76 actual old vocal-project fixtures remain tested.

Core synthesis/preset/diversity checks, Windows application/bundle tests, pluginval strictness 10, the official Steinberg validator and a real 0.18.2-to-0.18.3 installer upgrade are required before sharing the new installer. Actual FL Studio, physical audio/MIDI/DPI and Explorer interactions remain owner acceptance.

## Download and installation

The updated installer link will be added after validation. Close FL Studio and standalone, extract the installer ZIP into a fresh folder and run G-Boy-Advance-2-Setup-x64.exe. User presets and SoundFonts stay in their existing application-data folders.

Source branch: feature/percussion-removed-0183. Build using PowerShell -ExecutionPolicy Bypass -File scripts/build-windows.ps1 with Visual Studio 2022 x64 and CMake 3.24+. VST3 output: build/GBoyAdvance2_artefacts/Release/VST3/G Boy Advance 2.vst3. Installation: C:\\Program Files\\Common Files\\VST3\\G Boy Advance 2.vst3.

The prepared build is unsigned. The owner deferred distribution licensing; this update does not choose new terms. The earlier 50% performance target remains unmet.
""")
    proof={'removedPresets':24,'remainingPresets':654,'categories':20,'hostSlots':754,'retiredSlots':100,'retainedDictionariesIdentical':True,'retainedGeneratedRowsIdentical':True,'registryIdentitiesUnchanged':True,'removedIds':sorted(ids)}
    dump('build/percussion-removal-proof.json',proof)
    print('PASS removal proof: 24 retired, 654 identical retained dictionaries/rows, 20 categories, 754 stable slots with 100 reserved')
if __name__=='__main__':main()
