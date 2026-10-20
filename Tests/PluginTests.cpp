#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FactoryData.h"
#include "RemovedVocalStates.h"
#include <iostream>
#include <set>
#include <stdexcept>

int runLibraryTests();
int runSoundFontTests();
namespace {
int checks=0;
struct StateListener final : juce::AudioProcessorListener {
    int appearanceChanges=0;
    void audioProcessorParameterChanged(juce::AudioProcessor*,int,float) override {}
    void audioProcessorChanged(juce::AudioProcessor*,const ChangeDetails& details) override {if(details.nonParameterStateChanged)++appearanceChanges;}
};
void check(bool ok,const char* message) {++checks;if(!ok)throw std::runtime_error(message);}
void set(GbaProcessor& p,const char* id,float value) {auto* v=p.state.getParameter(id);v->setValueNotifyingHost(v->convertTo0to1(value));}
juce::Component* named(juce::Component& root,const juce::String& name) {if(root.getName()==name)return &root;for(auto* c:root.getChildren())if(auto* match=named(*c,name))return match;return nullptr;}
juce::TextButton* button(juce::Component& root,const juce::String& label) {if(auto* b=dynamic_cast<juce::TextButton*>(&root);b&&b->getButtonText()==label)return b;for(auto* c:root.getChildren())if(auto* match=button(*c,label))return match;return nullptr;}
void render(GbaProcessor& p,juce::AudioBuffer<float>& audio,int blocks) {for(int i=0;i<blocks;++i){juce::MidiBuffer midi;p.processBlock(audio,midi);}}
void snapshot(juce::Component& component,const juce::String& name,float scale=1) {
    auto image=component.createComponentSnapshot(component.getLocalBounds(),true,scale);check(image.isValid(),"Scalable editor snapshot failed");
    auto file=juce::File::getCurrentWorkingDirectory().getChildFile(name);check(file.getParentDirectory().createDirectory().wasOk(),"Could not create snapshot folder");
    juce::FileOutputStream output(file);check(output.openedOk(),"Could not create editor snapshot");
    check(output.setPosition(0)&&output.truncate().wasOk(),"Could not replace editor snapshot");juce::PNGImageFormat png;check(png.writeImageToStream(image,output),"Could not encode editor snapshot");
}
#if JUCE_WINDOWS
juce::MouseEvent mouse(juce::Component& component,juce::Point<float> point) {
    auto now=juce::Time::getCurrentTime();
    return {juce::Desktop::getInstance().getMainMouseSource(),point,juce::ModifierKeys::leftButtonModifier,1,0,0,0,0,&component,&component,now,point,now,1,false};
}
#endif
void removedVocalProjects(const juce::File& root) {
    const auto fixtures=removedVocalStateFixtures();const auto* projects=fixtures["projects"].getArray();
    check(projects&&projects->size()==76,"Missing actual old vocal project fixtures");
    auto p=std::make_unique<GbaProcessor>(root.getChildFile("RemovedVocals"));
    p->setRateAndBufferSizeDetails(48000,256);p->prepareToPlay(48000,256);
    const auto defaultSettings=p->snapshot();
    check(p->presets.all().size()==574&&p->presets.categories().size()==18,"Retired categories remain in the factory browser");
    for(const char* name:{"Vocals - Pop and Indie","Vocals - Southern and West Coast","Vocals - Cartoon and Gaming","Vocals - Soul and R&B"}) {
        PresetManager::Filter f;f.category=name;check(p->presets.filtered(f).empty()&&!p->presets.categories().contains(name),"Removed category remains selectable");
    }
    for(const auto& patch:gba::factoryPatches)check(p->getProgramName(patch.program)==patch.name,"Surviving factory host program number moved");
    for(const auto& record:*projects) {
        const auto id=record["id"].toString();const int program=int(record["program"]);
        check(!p->presets.find(id)&&!p->presets.load(id),"Removed factory patch can still be loaded from the library");
        check(p->getProgramName(program).startsWith("Reserved "),"Removed host slot still advertises the deleted sound");
        juce::MemoryBlock state;check(state.fromBase64Encoding(record["state"].toString()),"Invalid old state fixture");
        p->setStateInformation(state.getData(),int(state.getSize()));
        check(p->presetId()==id&&p->presetName()==record["name"].toString()&&p->getCurrentProgram()==program,"Old project identity/program was lost");
        check(p->presetCategory()=="Saved Project"&&p->stateNotice().contains("saved in your project was restored"),"Restored project recreates a removed category or lacks its notice");
        const auto* expected=record["parameters"].getArray();check(expected&&expected->size()==int(gba::parameterCount),"Old fixture lost synthesis parameters");
        const auto settings=p->snapshot();bool matches=true;
        // These fixtures were exported on Linux; tolerate only host float
        // normalization precision when restoring them on Windows.
        for(std::size_t i=0;i<gba::parameterCount;++i)matches&=std::abs(settings.values[i]-float((*expected)[int(i)]))<=std::max(.0000001f,(gba::specs[i].max-gba::specs[i].min)*4*std::numeric_limits<float>::epsilon());
        check(matches,"Removed-category project no longer restores its edited sound settings");
        juce::AudioBuffer<float> audio(2,256);p->reset();juce::MidiBuffer note;note.addEvent(juce::MidiMessage::noteOn(1,60,.8f),0);p->processBlock(audio,note);render(*p,audio,32);
        check(audio.getMagnitude(0,256)>.00001f,"Restored removed-category project is silent");p->panic();render(*p,audio,1);
        juce::MemoryBlock saved;p->getStateInformation(saved);p->presets.restoreDefault(false);p->setStateInformation(saved.getData(),int(saved.getSize()));
        check(p->snapshot().values==settings.values&&p->getCurrentProgram()==program,"Resaving an old vocal project changed its sound or program");
        p->setCurrentProgram(program);p->servicePendingProgram();
        check(p->snapshot().values==settings.values,"Host program echo replaced a restored removed-category project sound");
        p->presets.restoreDefault(false);
        p->setCurrentProgram(program);p->servicePendingProgram();
        check(p->getCurrentProgram()==program&&p->presetName()==p->getProgramName(program)&&p->snapshot().values==defaultSettings.values,"Reserved host slot failed to select the documented default");
    }
    juce::MemoryBlock lastProject;check(lastProject.fromBase64Encoding(projects->getLast()["state"].toString()),"Last old project fixture is invalid");p->setStateInformation(lastProject.getData(),int(lastProject.getSize()));
    const auto kept=p->snapshot();check(p->presets.saveAs("Kept Project Sound").wasOk(),"Could not keep a restored sound as a user preset");
    const auto userId=p->presetId();p->presets.restoreDefault(false);check(p->presets.load(userId)&&p->snapshot().values==kept.values,"User preset no longer reloads independently of removed factory categories");
    for(int i=0;i<18;++i){p->presets.stepCategory(1);check(p->presetCategory()!="Vocals - Pop and Indie"&&p->presetCategory()!="Vocals - Southern and West Coast"&&p->presetCategory()!="Vocals - Cartoon and Gaming"&&p->presetCategory()!="Vocals - Soul and R&B","D-pad category navigation reaches a removed category");}
    std::cout<<"PASS 76 actual 0.16.0 removed-vocal projects: edited settings, audible notes, resave, reserved host slots, surviving program numbers and 18 browser categories\n";
}
void unifiedVocals(const juce::File& root) {
    auto p=std::make_unique<GbaProcessor>(root.getChildFile("UnifiedVocals"));
    PresetManager::Filter filter;filter.category="Vocals";filter.source=PresetManager::Source::factory;
    check(p->presets.filtered(filter).size()==74,"Unified Vocals category lost factory sounds");
    const char* names[]{"Japanese City Pop Vocal Synth","Ghost Whisper Synth","Children's Choir-Style Synth"};
    const char* categories[]{"Vocals - Global Pop Textures","Vocals - Horror","Vocals - Choirs and Textures"};
    for(int group=0;group<3;++group) {
        check(!p->presets.categories().contains(categories[group]),"Old vocal folder remains selectable");
        for(const auto& patch:gba::factoryPatches)if(juce::String(patch.name)==names[group]) {
            check(p->presets.load(patch.id),"Could not load a merged vocal preset");
            set(*p,"filter.cutoff",4321);const auto edited=p->snapshot();
            juce::MemoryBlock current;p->getStateInformation(current);
            auto xml=juce::AudioProcessor::getXmlFromBinary(current.getData(),int(current.getSize()));
            check(xml!=nullptr,"Could not create legacy-category project fixture");
            xml->setAttribute("presetCategory",categories[group]);
            juce::MemoryBlock legacy;juce::AudioProcessor::copyXmlToBinary(*xml,legacy);
            p->presets.restoreDefault(false);p->setStateInformation(legacy.getData(),int(legacy.getSize()));
            check(p->presetCategory()=="Vocals"&&p->presetId()==patch.id&&p->presetName()==patch.name&&p->getCurrentProgram()==patch.program,"Saved vocal project lost its identity or new category");
            check(p->snapshot().values==edited.values,"Category migration replaced edited project parameters");
            juce::MemoryBlock saved;p->getStateInformation(saved);
            auto migrated=juce::AudioProcessor::getXmlFromBinary(saved.getData(),int(saved.getSize()));
            check(migrated&&migrated->getStringAttribute("presetCategory")=="Vocals","Resaved vocal project retained the old category");
        }
    }
    std::set<juce::String> visited;
    for(int i=0;i<18;++i){p->presets.stepCategory(1);visited.insert(p->presetCategory());}
    check(visited.size()==18&&visited.count("Vocals")==1,"D-pad navigation did not visit each merged category once");
    std::cout<<"PASS 74 merged Vocals presets, three legacy-category edited projects and 18-category navigation\n";
}
void launchAppearance(const juce::File& root) {
    for(int previous=0;previous<int(gba_ui::skins.size());++previous) {
        std::array<bool,gba_ui::skins.size()> reached{};
        for(int draw=0;draw<int(gba_ui::skins.size())-1;++draw) {
            const int selected=gba_ui::differentSkin(previous,draw);
            check(selected>=0&&selected<int(reached.size())&&selected!=previous&&!reached[std::size_t(selected)],"Launch colour mapping repeats or skips a colour");
            reached[std::size_t(selected)]=true;
        }
    }
    const auto folder=root.getChildFile("LaunchColours"),file=folder.getChildFile("appearance.json");
    auto first=std::make_unique<GbaProcessor>(folder);
    check(first->newSkinOnLaunch&&!file.exists(),"Fresh instances must enable launch colours without writing during scanning");
    juce::MemoryBlock automatic;first->getStateInformation(automatic);
    check(!file.exists(),"State serialization initialized the appearance file");
    std::unique_ptr<juce::AudioProcessorEditor> editor(first->createEditor());
    const int original=first->editorSkin.load();
    const auto contents=file.loadFileAsString();
    check(original>=0&&original<int(gba_ui::skins.size())&&juce::JSON::parse(contents)["lastSkin"].toString()==gba_ui::skins[std::size_t(original)].id.data(),"First launch colour was not persisted");
    editor.reset();editor.reset(first->createEditor());
    check(first->editorSkin==original&&file.loadFileAsString()==contents,"Reopening the same window rerolled or rewrote its colour");
    first->presets.step(1);check(first->editorSkin==original,"Changing a preset rerolled the skin");
    first->getStateInformation(automatic);
    auto next=std::make_unique<GbaProcessor>(folder);next->setStateInformation(automatic.getData(),int(automatic.getSize()));
    check(file.loadFileAsString()==contents,"DAW restore performed launch appearance I/O");
    std::unique_ptr<juce::AudioProcessorEditor> nextEditor(next->createEditor());
    const int nextColour=next->editorSkin.load();
    check(next->newSkinOnLaunch&&nextColour!=original,"New plugin load restored the previous launch colour");
    check(next->snapshot().values==first->snapshot().values,"Launch colour changed the project sound");
    next->setStateInformation(automatic.getData(),int(automatic.getSize()));
    check(next->editorSkin==nextColour,"State refresh rerolled a visible instance");
    nextEditor->keyPressed(juce::KeyPress(juce::KeyPress::F2Key));
    auto* toggle=dynamic_cast<juce::ToggleButton*>(named(*nextEditor,"New colour on load"));
    check(toggle&&toggle->getToggleState(),"Advanced launch-colour switch is missing or off by default");
    StateListener listener;next->addListener(&listener);
    toggle->setToggleState(false,juce::dontSendNotification);toggle->onClick();
    check(!next->newSkinOnLaunch&&listener.appearanceChanges==1,"Launch-colour switch did not change project preference and notify host");
    next->setEditorSkin(7);juce::MemoryBlock fixed;next->getStateInformation(fixed);
    auto pinned=std::make_unique<GbaProcessor>(folder);pinned->setStateInformation(fixed.getData(),int(fixed.getSize()));
    std::unique_ptr<juce::AudioProcessorEditor> pinnedEditor(pinned->createEditor());
    check(!pinned->newSkinOnLaunch&&pinned->editorSkin==7&&dynamic_cast<ConsoleLook&>(pinnedEditor->getLookAndFeel()).skin()==7,"Fixed project colour did not restore");
    next->removeListener(&listener);
    pinnedEditor.reset();pinned.reset();nextEditor.reset();next.reset();editor.reset();first.reset();
    int last=7;
    for(int i=0;i<3;++i) {
        juce::ChildProcess process;
        check(process.start(juce::StringArray{juce::File::getSpecialLocation(juce::File::currentExecutableFile).getFullPathName(),"--skin-launch-probe",folder.getFullPathName()}),"Could not start launch persistence probe");
        const bool finished=process.waitForProcessToFinish(10000);if(!finished)process.kill();
        check(finished,"Launch persistence probe timed out");
        const int colour=int(process.getExitCode())-20;
        check(colour>=0&&colour<int(gba_ui::skins.size())&&colour!=last,"Launch colour repeated after a new OS process");
        check(juce::JSON::parse(file)["lastSkin"].toString()==gba_ui::skins[std::size_t(colour)].id.data(),"Child process did not persist its launch colour");last=colour;
    }
    check(file.replaceWithText("corrupt appearance preference"),"Could not write invalid appearance fixture");
    {auto p=std::make_unique<GbaProcessor>(folder);std::unique_ptr<juce::AudioProcessorEditor> e(p->createEditor());
        check(p->editorSkin>=0&&p->editorSkin<int(gba_ui::skins.size())&&juce::JSON::parse(file)["format"].toString()=="gba2.appearance","Invalid preference prevented launch or safe repair");}
    const auto blocked=root.getChildFile("BlockedAppearance");check(blocked.getChildFile("appearance.json").createDirectory().wasOk(),"Could not create blocked preference fixture");
    {auto p=std::make_unique<GbaProcessor>(blocked);std::unique_ptr<juce::AudioProcessorEditor> e(p->createEditor());
        check(e&&blocked.getChildFile("appearance.json").isDirectory(),"Unwritable preferences prevented UI creation or replaced a directory");}
    std::cout<<"PASS launch colours: nonrepeat mapping, project/fixed mode, window reopen and three separate OS process launches\n";
}
}
int main(int argc,char** argv) {
    juce::ScopedJuceInitialiser_GUI gui;
    if(argc==3&&juce::String(argv[1])=="--skin-launch-probe") {
        GbaProcessor processor{juce::File(juce::String::fromUTF8(argv[2]))};
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        return 20+processor.editorSkin.load();
    }
    if(argc==3&&juce::String(argv[1])=="--soundfont-startup-probe") {
        GbaProcessor processor{juce::File(juce::String::fromUTF8(argv[2]))};
        processor.setRateAndBufferSizeDetails(48000,256);processor.prepareToPlay(48000,256);
        if(!processor.romHacks.waitUntilIdle(10000)||!processor.romHacks.enabled())return 2;
        juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;midi.addEvent(juce::MidiMessage::noteOn(1,69,.8f),0);processor.processBlock(audio,midi);
        float peak=0;for(int i=0;i<64;++i){midi.clear();processor.processBlock(audio,midi);peak=std::max(peak,audio.getMagnitude(0,256));}
        return peak>.0001f?50+processor.romHacks.view().selected:3;
    }
    auto root=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("gba2-tests-"+juce::Uuid().toString());
    try {
        removedVocalProjects(root);
        unifiedVocals(root);
        launchAppearance(root);
        checks+=runLibraryTests();
        checks+=runSoundFontTests();
        auto processor=std::make_unique<GbaProcessor>(root);auto& p=*processor;
        p.setRateAndBufferSizeDetails(48000,256);p.prepareToPlay(48000,256);
        check(p.getLatencySamples()==16,"Host latency does not match the oversampled effects path");
        set(p,"fx.reverb",0);set(p,"fx.delay",0);set(p,"amp.release",.2f);
        check(p.getTailLengthSeconds()>=.2&&p.getTailLengthSeconds()<1,"Dry patch tail is not based on envelope release");
        set(p,"fx.delay",.8f);set(p,"fx.delayTime",1.5f);set(p,"fx.delayFeedback",0);
        check(p.getTailLengthSeconds()>=1.7&&p.getTailLengthSeconds()<2,"Single echo tail was omitted or overestimated");
        set(p,"fx.delayFeedback",.85f);set(p,"fx.reverb",.8f);set(p,"fx.reverbSize",1);set(p,"amp.release",12);
        check(p.getTailLengthSeconds()>120&&p.getTailLengthSeconds()<150,"Host tail estimate truncates long feedback delays");
        p.presets.restoreDefault();
        const auto factoryId=p.presetId();set(p,"filter.cutoff",4321);set(p,"volume",.371f);
        juce::MemoryBlock factoryState;p.getStateInformation(factoryState);
        auto factoryReload=std::make_unique<GbaProcessor>(root);factoryReload->setStateInformation(factoryState.getData(),int(factoryState.getSize()));
        check(factoryReload->presetId()==factoryId,"Factory project identity did not survive state reload");
        check(std::abs(factoryReload->snapshot()[gba::Param::Cutoff]-4321)<.1f&&std::abs(factoryReload->snapshot()[gba::Param::Volume]-.371f)<.0001f,"Factory state was replaced by current library values on project reload");
        check(p.getNumPrograms()==650,"Expanded factory host-program count is wrong");
        p.setCurrentProgram(409);p.servicePendingProgram();
        check(p.getCurrentProgram()==409&&p.presetName()=="Handheld Pulse Motion - Afterglow","Appended retro host program is unreachable");
        set(p,"volume",.43f);juce::MemoryBlock retroState;p.getStateInformation(retroState);
        factoryReload->setStateInformation(retroState.getData(),int(retroState.getSize()));
        check(factoryReload->getCurrentProgram()==409&&factoryReload->presetId()==p.presetId(),"Appended retro program identity did not restore");
        check(factoryReload->snapshot()[gba::Param::Engine]==6&&factoryReload->snapshot()[gba::Param::Coarse]==-12&&std::abs(factoryReload->snapshot()[gba::Param::Volume]-.43f)<.0001f,"Appended retro settings did not restore");
        p.setCurrentProgram(529);p.servicePendingProgram();
        check(p.getCurrentProgram()==529&&p.presetName()=="Florida High Night Whistle - Afterglow","Final regional program is unreachable");
        juce::MemoryBlock regionalState;p.getStateInformation(regionalState);factoryReload->setStateInformation(regionalState.getData(),int(regionalState.getSize()));
        check(factoryReload->getCurrentProgram()==529&&factoryReload->snapshot().values==p.snapshot().values,"Regional factory project state did not restore");
        p.setCurrentProgram(649);p.servicePendingProgram();
        check(p.getCurrentProgram()==649&&p.presetName()=="80s Neon Vowel Chords","New groove host program is unreachable");
        juce::MemoryBlock grooveState;p.getStateInformation(grooveState);factoryReload->setStateInformation(grooveState.getData(),int(grooveState.getSize()));
        check(factoryReload->getCurrentProgram()==649&&factoryReload->snapshot().values==p.snapshot().values,"Groove factory project state did not restore");
        p.presets.restoreDefault();
        juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;
        // Verify the host wrapper splits rendering at MIDI sample offsets.
        set(p,"amp.attack",.001f);midi.addEvent(juce::MidiMessage::noteOn(1,60,.8f),128);p.processBlock(audio,midi);
        check(audio.getMagnitude(0,0,128)==0,"MIDI sample offset was ignored");
        render(p,audio,8);check(audio.getMagnitude(0,256)>0.0001f,"Plugin wrapper produced no audio");
        p.reset();set(p,"fx.reverb",0);set(p,"fx.delay",0);set(p,"amp.release",.02f);
        midi.clear();midi.addEvent(juce::MidiMessage::noteOn(1,65,.8f),0);p.processBlock(audio,midi);render(p,audio,16);
        midi.clear();midi.addEvent(juce::MidiMessage::noteOff(1,65),0);p.processBlockBypassed(audio,midi);
        check(audio.getMagnitude(0,256)==0,"Bypassed instrument leaked audio");
        for(int i=0;i<128;++i){midi.clear();p.processBlockBypassed(audio,midi);}
        render(p,audio,4);check(p.voiceCount()==0&&audio.getMagnitude(0,256)<1e-5f,"Bypass dropped note-off or froze envelopes");
        p.presets.restoreDefault();
        p.reset();p.keyboard.noteOn(1,72,.8f);render(p,audio,32);
        check(p.voiceCount()>0&&audio.getMagnitude(0,256)>0.0001f,"On-screen keyboard state did not reach the audio engine");
        p.keyboard.noteOff(1,72,0);p.panic();render(p,audio,1);
        check(p.voiceCount()==0,"Keyboard panic did not clear voices");
        set(p,"filter.cutoff",1234);set(p,"macro.a.target",4);set(p,"voice.glide",.21f);set(p,"engine",5);set(p,"formant.vowel",2.4f);set(p,"voice.count",24);
        check(p.getParameters().size()==64,"Appearance changed the stable audio parameter count");
        p.editorWidth=1440;p.editorSkin=4;
        auto saved=p.presets.saveAs("Integration Custom Vowels");check(saved.wasOk(),"Save As failed");auto id=p.presetId();
        p.presets.toggleFavorite(id);check(p.presets.isFavorite(id),"Favorite failed");check(p.presets.isRecent(id),"Recently used failed");
        juce::MemoryBlock state;p.getStateInformation(state);
        auto restored=std::make_unique<GbaProcessor>(root);restored->setStateInformation(state.getData(),int(state.getSize()));
        check(restored->presetId()==id,"State preset identity lost");
        for(std::size_t i=0;i<gba::parameterCount;++i)check(std::abs(p.snapshot().values[i]-restored->snapshot().values[i])<.001f,"State parameter roundtrip failed");
        check(restored->editorWidth==1440,"State editor size lost");check(restored->editorSkin==4,"Project skin was lost");check(restored->presets.find(id)!=nullptr,"User preset did not reload from disk");check(restored->presets.isFavorite(id),"Favorite did not persist");
        check(restored->presets.load(id),"User preset load failed");check(std::abs(restored->snapshot()[gba::Param::Cutoff]-1234)<.1f,"User preset values changed");
        check(restored->presets.renameUser(id,"Renamed Vowels").wasOk(),"Rename failed");check(restored->presetName()=="Renamed Vowels","Rename identity failed");
        check(restored->presets.deleteUser(id).wasOk(),"Delete failed");check(restored->presets.find(id)==nullptr,"Delete retained user patch");
        auto defaultId=restored->presetId();check(restored->presets.deleteUser(defaultId).failed(),"Factory delete was allowed");
        check(restored->presets.saveAs("").failed(),"Empty preset name was allowed");
        auto legacy=p.state.copyState();legacy.setProperty("schemaVersion",1,nullptr);legacy.removeProperty("editorSkin",nullptr);
        juce::MemoryBlock legacyBytes;juce::AudioProcessor::copyXmlToBinary(*legacy.createXml(),legacyBytes);
        restored->setStateInformation(legacyBytes.getData(),int(legacyBytes.getSize()));check(restored->editorSkin==0,"Old projects did not default to Purple");
        legacy.setProperty("editorSkin","future-unknown-skin",nullptr);juce::AudioProcessor::copyXmlToBinary(*legacy.createXml(),legacyBytes);
        restored->editorSkin=3;restored->setStateInformation(legacyBytes.getData(),int(legacyBytes.getSize()));check(restored->editorSkin==0,"Unknown skin ID did not fall back safely");
        // 0.12.0 projects may contain large embedded audio. Restore synthesis
        // settings without decoding, retaining, or silently claiming that audio.
        for(int payloadBytes:{8,5*1024*1024}) {
            auto oldSample=p.state.copyState();oldSample.setProperty("schemaVersion",1,nullptr);
            oldSample.setProperty("presetName","Old Sample Patch",nullptr);oldSample.setProperty("editorSkin","black",nullptr);
            oldSample.setProperty("sampleData",juce::String::repeatedString("x",payloadBytes),nullptr);
            juce::MemoryBlock oldBytes;juce::AudioProcessor::copyXmlToBinary(*oldSample.createXml(),oldBytes);
            restored->setStateInformation(oldBytes.getData(),int(oldBytes.getSize()));
            check(restored->snapshot().values==p.snapshot().values&&restored->editorSkin==2,"Removing old embedded audio lost synthesis parameters or skin");
            check(restored->stateNotice().contains("Sample playback was removed")&&restored->presetName()=="SYNTH ONLY: Old Sample Patch","Old sample project did not disclose synthesis-only restoration");
            juce::MemoryBlock rewritten;restored->getStateInformation(rewritten);
            auto rewrittenTree=juce::ValueTree::fromXml(*juce::AudioProcessor::getXmlFromBinary(rewritten.getData(),int(rewritten.getSize())));
            check(!rewrittenTree.hasProperty("sampleData")&&rewritten.getSize()<32768,"Re-saved project retained embedded audio");
            restored->setStateInformation(rewritten.getData(),int(rewritten.getSize()));
            check(restored->stateNotice().isNotEmpty()&&restored->presetName()=="SYNTH ONLY: Old Sample Patch","Migration notice was lost or duplicated after project reload");
        }
        restored->presets.restoreDefault();check(restored->stateNotice().isEmpty(),"New synthesis preset retained the old sample-removal notice");
        auto samplePreset=root.getChildFile("old-sample.gba2preset");const juce::String sampleJson="{\"format\":\"gba2.preset\",\"schemaVersion\":2,\"sample\":{}}";
        check(samplePreset.replaceWithText(sampleJson),"Could not write old sample preset fixture");
        check(p.presets.importPreset(samplePreset).failed()&&p.presets.lastError.contains("Sample playback was removed"),"Removed sample preset was silently imported");
        check(samplePreset.loadFileAsString()==sampleJson,"Rejected sample preset file was modified");
        p.editorSkin=0;
        auto before=restored->snapshot();const char invalid[]{0,1,2,3,4};restored->setStateInformation(invalid,5);check(before.values==restored->snapshot().values,"Invalid state altered patch");
        restored->setCurrentProgram(0);restored->servicePendingProgram();check(restored->presetName()==restored->getProgramName(0),"Host program selection failed");
        auto prior=restored->presetId();restored->presets.step(1);check(restored->presetId()!=prior,"Next preset failed");restored->presets.step(-1);check(restored->presetId()==prior,"Previous preset failed");
        auto category=restored->presetCategory();restored->presets.stepCategory(1);check(restored->presetCategory()!=category,"Next category failed");
        {std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());check(editor!=nullptr,"Editor creation failed");editor->setVisible(true);
            auto* piano=dynamic_cast<juce::MidiKeyboardComponent*>(named(*editor,"Piano keyboard"));check(piano!=nullptr,"Piano keyboard component missing");
            const auto firstKey=piano->getLowestVisibleKey();editor->setSize(960,485);editor->setSize(1920,969);
            const auto usableWidth=p.editorWidth.load();editor->setSize(0,0);
            check(piano->getKeyWidth()>0&&p.editorWidth==usableWidth,"Transient zero-sized host window corrupted keyboard width or saved editor size");
            editor->setSize(960,485);
            check(piano->getLowestVisibleKey()==firstKey,"Editor resizing reset the piano octave");auto* master=dynamic_cast<juce::Slider*>(named(*editor,"Master volume"));check(master!=nullptr,"Volume component missing");master->setValue(.37,juce::sendNotificationSync);check(std::abs(p.snapshot()[gba::Param::Volume]-.37f)<.001f,"Volume UI is not attached");
            auto* pitch=dynamic_cast<juce::Slider*>(named(*editor,"Pitch wheel"));check(pitch!=nullptr,"Pitch wheel missing");pitch->setValue(12000,juce::sendNotificationSync);pitch->onDragEnd();check(pitch->getValue()==8192,"Pitch wheel did not center");
            auto* glide=dynamic_cast<juce::Slider*>(named(*editor,"Glide"));check(glide!=nullptr,"Glide control missing");glide->setValue(.31,juce::sendNotificationSync);check(std::abs(p.snapshot()[gba::Param::Glide]-.31f)<.001f,"Glide UI is not attached");
            auto current=p.presetId();auto* next=button(*editor,">");check(next!=nullptr,"Next button missing");next->onClick();check(p.presetId()!=current,"Next UI button is inert");
            p.panic();render(p,audio,1);
            auto* a=button(*editor,"A");auto* b=button(*editor,"B");check(a&&b,"Preview buttons missing");check(!a->hitTest(1,1)&&a->hitTest(a->getWidth()/2,a->getHeight()/2),"Preview button hit area does not match its circular face");a->onClick();render(p,audio,4);check(p.voiceCount()>0,"A preview UI is inert");b->onClick();render(p,audio,256);check(p.voiceCount()==0,"B preview stop UI is inert");
            DirectionPad* pad=nullptr;for(auto* child:editor->getChildren())if(auto* candidate=dynamic_cast<DirectionPad*>(child))pad=candidate;
            check(pad!=nullptr,"D-pad missing");auto oldCategory=p.presetCategory();pad->keyPressed(juce::KeyPress(juce::KeyPress::downKey));check(p.presetCategory()!=oldCategory,"D-pad category navigation is inert");
            check(pad->getComponentAt(2,2)==nullptr,"D-pad blank corner consumes clicks");
            check(pad->getComponentAt(pad->getLocalBounds().getCentre())==nullptr,"D-pad neutral center changes presets");
            auto clickPad=[&](const char* name){auto* direction=dynamic_cast<juce::Button*>(named(*pad,name));check(direction&&direction->getWantsKeyboardFocus(),"D-pad direction is not a focusable button");direction->onClick();};
            auto padId=p.presetId();clickPad("Next preset");check(p.presetId()!=padId,"D-pad right button is inert");clickPad("Previous preset");check(p.presetId()==padId,"D-pad left button is inert");
            auto padCategory=p.presetCategory();clickPad("Next preset category");check(p.presetCategory()!=padCategory,"D-pad down button is inert");clickPad("Previous preset category");check(p.presetCategory()==padCategory,"D-pad up button is inert");
            // JUCE mouse containment requires a real native peer. Linux's software-only
            // runner has no display; the Windows CI runner exercises the actual mouse path.
#if JUCE_WINDOWS
            editor->setSize(960,485);editor->addToDesktop(0);editor->setTopLeftPosition(20,20);editor->toFront(false);
#if GBA_WINDOWS_SOFTWARE_EDITOR
            auto* peer=editor->getPeer();check(peer!=nullptr,"Editor native peer is missing");
            const int software=peer->getAvailableRenderingEngines().indexOf("Software Renderer");
            check(software>=0&&peer->getCurrentRenderingEngine()==software,"Windows editor did not select its software renderer after attachment");
#endif
            juce::MessageManager::getInstance()->runDispatchLoopUntil(50);
            p.panic();render(p,audio,1);auto key=piano->getRectangleForKey(60);auto event=mouse(*piano,{key.getCentreX(),key.getBottom()-8});
            piano->mouseDown(event);check(p.keyboard.isNoteOn(1,60),"Mouse click on piano did not start C4");render(p,audio,8);check(p.voiceCount()>0,"Mouse piano note did not reach audio engine");
            piano->mouseUp(event);check(!p.keyboard.isNoteOn(1,60),"Mouse release left the piano note held");
            piano->mouseExit(mouse(*piano,{-1,-1}));
            editor->removeFromDesktop();
#endif
            p.presets.restoreDefault();
            juce::MessageManager::getInstance()->runDispatchLoopUntil(80);
            auto* macro=dynamic_cast<juce::Slider*>(named(*editor,"Macro A"));check(macro!=nullptr,"Macro A missing");macro->setValue(1730,juce::sendNotificationSync);check(std::abs(p.snapshot()[gba::Param::Cutoff]-1730)<.1f,"Macro A does not control filter cutoff");
            const char* macroNames[]{"Macro B","Macro C","Macro D"};const gba::Param macroTargets[]{gba::Param::Resonance,gba::Param::Attack,gba::Param::Reverb};const float macroValues[]{.42f,.21f,.36f};
            for(int i=0;i<3;++i){auto* control=dynamic_cast<juce::Slider*>(named(*editor,macroNames[i]));check(control!=nullptr,"Macro component missing");control->setValue(macroValues[i],juce::sendNotificationSync);check(std::abs(p.snapshot()[macroTargets[i]]-macroValues[i])<.001f,"Default macro destination is not attached");}
            set(p,"macro.a.target",4);juce::MessageManager::getInstance()->runDispatchLoopUntil(80);macro->setValue(3.75,juce::sendNotificationSync);check(std::abs(p.snapshot()[gba::Param::FmIndex]-3.75f)<.001f,"Macro reassignment is inert");
            auto* start=button(*editor,"START");start->onClick();check(button(*editor,"SAVE AS")!=nullptr,"START did not open browser");
            check(!master->isEnabled()&&!pad->isEnabled(),"Covered main controls remain keyboard-focusable in browser");
            auto browserId=p.presetId();check(!editor->keyPressed(juce::KeyPress(juce::KeyPress::rightKey))&&p.presetId()==browserId,"Overlay arrow key leaked into D-pad navigation");
            auto* search=dynamic_cast<juce::TextEditor*>(named(*editor,"Preset search"));auto* categoryFilter=dynamic_cast<juce::ComboBox*>(named(*editor,"Preset category"));auto* sourceFilter=dynamic_cast<juce::ComboBox*>(named(*editor,"Preset source"));auto* presetList=dynamic_cast<juce::ListBox*>(named(*editor,"Presets"));auto* presetName=dynamic_cast<juce::TextEditor*>(named(*editor,"Preset name"));auto* presetTags=dynamic_cast<juce::TextEditor*>(named(*editor,"Preset tags"));
            check(search&&categoryFilter&&sourceFilter&&presetList&&presetName&&presetTags,"Browser controls are missing accessible names");
            auto rows=[&]{return presetList->getListBoxModel()->getNumRows();};auto query=[&](const juce::String& value){search->setText(value);juce::MessageManager::getInstance()->runDispatchLoopUntil(30);};
            sourceFilter->setSelectedId(2,juce::sendNotificationSync);categoryFilter->setText("Hip-Hop - Detroit",juce::sendNotificationSync);check(rows()==60,"Regional browser category is incomplete");query("piano");check(rows()==30,"Regional piano tag search failed");query("vox");check(rows()==6,"Regional vox tag search failed");query("");
            for(const char* category:{"Hip-Hop - Chicago Drill","Funk","Japan Inspired","80s Pop"}) {
                categoryFilter->setText(category,juce::sendNotificationSync);check(rows()==(juce::String(category)=="80s Pop"?28:24),"Groove browser category is incomplete");
                query("bass");check(rows()>0,"Groove bass tag search failed");query("");
            }
            categoryFilter->setText("Retro Games",juce::sendNotificationSync);check(rows()==56,"Factory/category browser intersection failed");
            auto beforeBrowse=p.presetId();presetList->selectRow(0);check(p.presetId()==beforeBrowse,"Keyboard browsing changes the loaded sound");presetList->keyPressed(juce::KeyPress(juce::KeyPress::downKey));check(presetList->getSelectedRow()==1&&p.presetId()==beforeBrowse,"List arrow navigation loads a patch or fails to move");presetList->keyPressed(juce::KeyPress(juce::KeyPress::returnKey));check(p.presetCategory()=="Retro Games","Enter did not load the selected preset");render(p,audio,8);check(p.voiceCount()>0,"Browser Enter did not preview real audio");button(*editor,"B  STOP")->onClick();p.panic();render(p,audio,1);
            presetName->setText("Browser User");presetTags->setText("custom azure keys");button(*editor,"SAVE AS")->onClick();auto browserUser=p.presetId();
            check(p.presetCategory()=="User"&&p.presets.find(browserUser)->tags=="custom azure keys","Browser Save As lost custom tags");check(sourceFilter->getSelectedId()==2&&categoryFilter->getText()=="Retro Games","Save As reset browser filters");check(!button(*editor,"A  PREVIEW")->isEnabled()&&!button(*editor,"FAVORITE")->isEnabled(),"Hidden active patch leaves browser actions enabled");
            sourceFilter->setSelectedId(3,juce::sendNotificationSync);categoryFilter->setText("User",juce::sendNotificationSync);query("azure custom");check(rows()==1,"Browser AND tag search failed");auto browseSettings=p.snapshot();query("no-such-patch");check(rows()==0&&!button(*editor,"DELETE")->isEnabled()&&!button(*editor,"A  PREVIEW")->isEnabled(),"Empty results target a hidden patch");check(p.snapshot().values==browseSettings.values,"Filtering changed the current sound");query("");
            presetName->setText("Browser User Renamed");button(*editor,"RENAME")->onClick();check(p.presetName()=="Browser User Renamed"&&sourceFilter->getSelectedId()==3&&categoryFilter->getText()=="User","Rename failed or reset source/category filters");
            auto* currentUser=p.presets.find(browserUser);auto external=juce::JSON::parse(currentUser->file);external.getDynamicObject()->setProperty("id","user."+juce::Uuid().toString());external.getDynamicObject()->setProperty("name","External Browser Patch");auto externalFile=p.presets.userFolder().getChildFile("external-browser.gba2preset");check(externalFile.replaceWithText(juce::JSON::toString(external)),"External browser fixture failed");button(*editor,"REFRESH")->onClick();check(rows()==2&&p.presetId()==browserUser,"Refresh did not add an external patch or changed active sound");
            button(*editor,"FAVORITE")->onClick();auto* favorites=dynamic_cast<juce::ToggleButton*>(named(*editor,"Favorites only"));auto* recent=dynamic_cast<juce::ToggleButton*>(named(*editor,"Recently used"));check(favorites&&recent,"Browser metadata filters missing");favorites->setToggleState(true,juce::dontSendNotification);favorites->onClick();recent->setToggleState(true,juce::dontSendNotification);recent->onClick();check(rows()==1&&p.presets.isFavorite(browserUser),"Combined browser favorites/recent filters failed");
            editor->setSize(960,485);snapshot(*editor,"ui-snapshots/Browser-960.png");
            editor->keyPressed(juce::KeyPress(juce::KeyPress::F1Key));check(button(*editor,"SAVE AS")==nullptr&&master->isEnabled(),"F1 did not return from browser to main controls");
            auto* select=button(*editor,"SELECT");select->onClick();check(button(*editor,"BACK / SELECT")!=nullptr,"SELECT did not open advanced editor");
            auto* synthTabs=dynamic_cast<juce::TabbedComponent*>(named(*editor,"Advanced tabs"));
            check(synthTabs&&synthTabs->getNumTabs()==4&&!synthTabs->getTabNames().contains("SAMPLES"),"Sample playback controls remain in the editor");
            auto* skin=dynamic_cast<juce::ComboBox*>(named(*editor,"UI skin"));check(skin&&skin->getNumItems()==20,"Twenty-skin selector missing");
            auto* autoSkin=dynamic_cast<juce::ToggleButton*>(named(*editor,"New colour on load"));check(autoSkin!=nullptr,"Launch-colour preference is missing");
            autoSkin->setToggleState(false,juce::dontSendNotification);autoSkin->onClick();p.setEditorSkin(0);juce::MessageManager::getInstance()->runDispatchLoopUntil(60);
            const auto sound=p.snapshot();const auto presetId=p.presetId();
            StateListener listener;p.addListener(&listener);
            auto isolated=std::make_unique<GbaProcessor>(root.getChildFile("IsolatedSkin"));isolated->setNewSkinOnLaunch(false);isolated->setEditorSkin(0);std::unique_ptr<juce::AudioProcessorEditor> other(isolated->createEditor());
            std::set<juce::uint32> shellColours;
            for(std::size_t i=0;i<gba_ui::skins.size();++i){
                skin->setSelectedId(int(i)+1,juce::sendNotificationSync);
                check(p.editorSkin==int(i)&&dynamic_cast<ConsoleLook&>(editor->getLookAndFeel()).skin()==int(i),"Skin selection did not update the editor");
                const auto& look=dynamic_cast<ConsoleLook&>(editor->getLookAndFeel());
                check(shellColours.insert(look.shell(juce::Colour(0xff8450ec)).getARGB()).second,"Two skins produce the same shell colour");
                check(p.snapshot().values==sound.values&&p.presetId()==presetId,"Skin changed synthesis or active patch");
                check(dynamic_cast<ConsoleLook&>(other->getLookAndFeel()).skin()==0,"Skin leaked into another plugin instance");
                juce::MemoryBlock bytes;p.getStateInformation(bytes);isolated->setStateInformation(bytes.getData(),int(bytes.getSize()));
                check(isolated->editorSkin==int(i),"Selected skin failed project roundtrip");isolated->editorSkin=0;
                snapshot(*editor,"ui-snapshots/Advanced-"+juce::String(gba_ui::skins[i].id.data())+".png");
                editor->keyPressed(juce::KeyPress(juce::KeyPress::F2Key));snapshot(*editor,"ui-snapshots/Skin-"+juce::String(gba_ui::skins[i].id.data())+".png");
                editor->keyPressed(juce::KeyPress(juce::KeyPress::F2Key));skin=dynamic_cast<juce::ComboBox*>(named(*editor,"UI skin"));
            }
            check(listener.appearanceChanges==int(gba_ui::skins.size())-1,"Skin changes did not notify the host about unsaved state");p.removeListener(&listener);
            other.reset();isolated.reset();
            skin->setSelectedId(1,juce::sendNotificationSync);
            snapshot(*editor,"ui-snapshots/Advanced-960.png");
            editor->keyPressed(juce::KeyPress(juce::KeyPress::F2Key));check(button(*editor,"BACK / SELECT")==nullptr&&master->isEnabled(),"F2 did not return from advanced editor");
            select->onClick();
            editor->keyPressed(juce::KeyPress(juce::KeyPress::escapeKey));
            p.presets.restoreDefault();juce::MessageManager::getInstance()->runDispatchLoopUntil(80);
            auto* display=dynamic_cast<juce::TextButton*>(named(*editor,"Preset display"));check(display&&display->getButtonText()=="DREAM PAD","Preset display failed to refresh");
            editor->setSize(1280,646);snapshot(*editor,"ui-snapshots/Main-1280.png");snapshot(*editor,"ui-snapshots/Main-1280-at-125pct.png",1.25f);snapshot(*editor,"ui-snapshots/Main-1280-at-150pct.png",1.5f);snapshot(*editor,"ui-snapshots/Main-1280-at-200pct.png",2);
            editor->setSize(960,485);snapshot(*editor,"ui-snapshots/Main-960.png");editor->setSize(1920,969);snapshot(*editor,"GBA2-editor.png");}
        p.releaseResources();restored->releaseResources();restored.reset();processor.reset();root.deleteRecursively();
        std::cout<<"PASS "<<checks<<" JUCE integration checks: MIDI timing, state, user presets, keyboard, preview, D-pad, macros, browser and scalable editor.\n";return 0;
    } catch(const std::exception& ex) {std::cerr<<"FAIL: "<<ex.what()<<"\n";root.deleteRecursively();return 1;}
}
