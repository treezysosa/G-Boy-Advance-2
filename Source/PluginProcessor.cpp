#include "PluginProcessor.h"
#include "UiSkins.h"
#include "PluginEditor.h"
#include "FactoryData.h"
#include <map>

namespace {
// Only editor creation enters this path. Scanning, state restore and audio
// callbacks do not touch the appearance file. Serialize separate DAW processes
// too, so two windows opened at once do not both choose the previous colour.
int launchSkin(const juce::File& root,bool automatic,int fixed) {
    static juce::CriticalSection io;
    static std::map<juce::String,int> previousInProcess;
    const juce::ScopedLock guard(io);
    const auto key=root.getFullPathName();
    juce::InterProcessLock processLock("GBA2-appearance-"+juce::String::toHexString(key.toLowerCase().hashCode64()));
    const bool locked=processLock.enter(250);
    auto found=previousInProcess.find(key);
    int previous=found==previousInProcess.end()?-1:found->second;
    const auto preferences=root.getChildFile("appearance.json");
    if(locked&&preferences.getSize()<=4096) {
        const auto saved=juce::JSON::parse(preferences);
        if(saved["format"].toString()=="gba2.appearance"&&int(saved["schemaVersion"])==1) {
            const auto id=saved["lastSkin"].toString();
            for(std::size_t i=0;i<gba_ui::skins.size();++i)if(id==gba_ui::skins[i].id.data())previous=int(i);
        }
    }
    auto& random=juce::Random::getSystemRandom();
    const int selected=automatic?(previous<0?random.nextInt(int(gba_ui::skins.size())):
        gba_ui::differentSkin(previous,random.nextInt(int(gba_ui::skins.size())-1))):gba_ui::validSkin(fixed);
    previousInProcess[key]=selected;
    if(locked) {
        if(!preferences.isSymbolicLink()&&(!preferences.exists()||preferences.existsAsFile())&&root.createDirectory().wasOk()) {
            auto* object=new juce::DynamicObject();juce::var saved(object);
            object->setProperty("format","gba2.appearance");object->setProperty("schemaVersion",1);
            object->setProperty("lastSkin",gba_ui::skins[std::size_t(selected)].id.data());
            juce::TemporaryFile temporary(preferences);
            if(temporary.getFile().replaceWithText(juce::JSON::toString(saved)))
                temporary.overwriteTargetFileWithTemporary();
        }
        processLock.exit();
    }
    // Read-only app data or a busy external process never prevents UI creation.
    return selected;
}
}

juce::AudioProcessorValueTreeState::ParameterLayout GbaProcessor::createLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for(const auto& spec:gba::specs) {
        auto id=juce::ParameterID(juce::String(spec.id.data()),1);
        if(!spec.choices.empty()) {
            auto choices=juce::StringArray::fromTokens(juce::String(spec.choices.data()),"|","");
            layout.add(std::make_unique<juce::AudioParameterChoice>(id,juce::String(spec.name.data()),choices,int(spec.initial)));
        } else {
            juce::NormalisableRange<float> range(spec.min,spec.max,spec.step,spec.skew);
            layout.add(std::make_unique<juce::AudioParameterFloat>(id,juce::String(spec.name.data()),range,spec.initial));
        }
    }
    return layout;
}
GbaProcessor::GbaProcessor(juce::File dataRoot)
    : AudioProcessor(BusesProperties().withOutput("Stereo output",juce::AudioChannelSet::stereo(),true)),
      state(*this,nullptr,"GBA2_STATE",createLayout()),presets(*this,dataRoot),romHacks(presets.userFolder().getParentDirectory()),engine(std::make_unique<gba::SynthEngine>()) {
    for(std::size_t i=0;i<raw.size();++i) raw[i]=state.getRawParameterValue(juce::String(gba::specs[i].id.data()));
    setLatencySamples(gba::SynthEngine::latencySamples);
    keyboard.addListener(this);presets.restoreDefault(false);romHacks.armStartupRecall();startTimerHz(20);
}
GbaProcessor::~GbaProcessor() {stopTimer();keyboard.removeListener(this);}
double GbaProcessor::getTailLengthSeconds() const {
    const auto s=snapshot();
    // Include the envelope and cascaded echo/room decay, down to -100 dB
    // relative to their initial feedback level. This also avoids a 30-second
    // truncation estimate for a 1.5-second delay with 85% feedback.
    const auto repeats=[](double feedback) {return feedback>0?std::ceil(std::log(1e-5)/std::log(feedback)):0.0;};
    const double echo=s[gba::Param::Delay]>0?s[gba::Param::DelayTime]*(1+repeats(s[gba::Param::DelayFeedback])):0;
    const double room=s[gba::Param::Reverb]>0?.045*(1+repeats(.5+.46*s[gba::Param::ReverbSize])):0;
    return (romHacks.enabled()?std::max(30.0,double(s[gba::Param::Release])):double(s[gba::Param::Release]))+echo+room+.1;
}
void GbaProcessor::prepareToPlay(double sr,int) {engine->setSettings(snapshot());engine->prepare(sr);romHacks.prepare(sr);romHacks.startLibrary();soundFontMode=romHacks.enabled();modeFade=0;lastLeft=lastRight=0;}
void GbaProcessor::releaseResources() {reset();}
void GbaProcessor::reset() {engine->reset();if(auto* sf=romHacks.beginBlock(snapshot()))sf->reset();romHacks.endBlock();active=0;level=0;lastLeft=lastRight=0;}
bool GbaProcessor::isBusesLayoutSupported(const BusesLayout& layout) const {
    return layout.getMainInputChannelSet().isDisabled()&&layout.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();
}
gba::Settings GbaProcessor::snapshot() const noexcept {
    gba::Settings s;for(std::size_t i=0;i<raw.size();++i)s.values[i]=raw[i]->load(std::memory_order_relaxed);return s;
}
void GbaProcessor::enqueue(Command c) noexcept {
    int first=0,count=0,second=0,count2=0;commandFifo.prepareToWrite(1,first,count,second,count2);
    if(count>0) {commands[std::size_t(first)]=c;commandFifo.finishedWrite(1);}
    else panicRequested.store(true); // Overflow cannot leave a stuck key.
}
void GbaProcessor::handleNoteOn(juce::MidiKeyboardState*,int c,int n,float v) {enqueue({1,c,n,v});}
void GbaProcessor::handleNoteOff(juce::MidiKeyboardState*,int c,int n,float) {enqueue({2,c,n,0});}
void GbaProcessor::preview() {enqueue({3,1,60,.85f});}
void GbaProcessor::stopPreview() {enqueue({4,1,60,0});}
void GbaProcessor::uiPitch(int value) {pitchRequested.store(juce::jlimit(0,16383,value));}
void GbaProcessor::panic() {panicRequested.store(true);}
void GbaProcessor::drainCommands() noexcept {
    int first=0,count=0,second=0,count2=0;commandFifo.prepareToRead(255,first,count,second,count2);
    const auto process=[&](int begin,int amount) {
        for(int i=0;i<amount;++i) {const auto& c=commands[std::size_t(begin+i)];
            if(soundFontMode){if(auto* sf=romHacks.audio()){
                if(c.kind==1)sf->noteOn(c.channel,c.note,c.value);
                if(c.kind==2)sf->noteOff(c.channel,c.note);
                if(c.kind==3)sf->noteOn(c.channel,c.note,c.value,true);
                if(c.kind==4)sf->stopPreview();
            }continue;}
            if(c.kind==1)engine->noteOn(c.channel,c.note,c.value);
            if(c.kind==2)engine->noteOff(c.channel,c.note);
            if(c.kind==3)engine->noteOn(c.channel,c.note,c.value,true);
            if(c.kind==4)engine->stopPreview();
        }
    };
    process(first,count);process(second,count2);commandFifo.finishedRead(count+count2);
    if(panicRequested.exchange(false)){engine->allNotesOff(true);if(auto* sf=romHacks.audio())sf->reset();}
    int pitch=pitchRequested.exchange(-1);if(pitch>=0){engine->pitchWheel(1,pitch);if(auto* sf=romHacks.audio())sf->pitchWheel(1,pitch);}
}
void GbaProcessor::midi(const juce::MidiMessage& m) noexcept {
    if(soundFontMode){if(auto* sf=romHacks.audio()){
        if(m.isNoteOn())sf->noteOn(m.getChannel(),m.getNoteNumber(),m.getFloatVelocity());
        else if(m.isNoteOff())sf->noteOff(m.getChannel(),m.getNoteNumber());
        else if(m.isPitchWheel())sf->pitchWheel(m.getChannel(),m.getPitchWheelValue());
        else if(m.isController())sf->controller(m.getChannel(),m.getControllerNumber(),m.getControllerValue());
        else if(m.isProgramChange())sf->programChange(m.getChannel(),m.getProgramChangeNumber());
    }return;}
    if(m.isNoteOn())engine->noteOn(m.getChannel(),m.getNoteNumber(),m.getFloatVelocity());
    else if(m.isNoteOff())engine->noteOff(m.getChannel(),m.getNoteNumber());
    else if(m.isPitchWheel())engine->pitchWheel(m.getChannel(),m.getPitchWheelValue());
    else if(m.isController())engine->controller(m.getChannel(),m.getControllerNumber(),m.getControllerValue());
    else if(m.isProgramChange())setCurrentProgram(m.getProgramChangeNumber());
    else if(m.isAllNotesOff())engine->controller(m.getChannel(),123,0);
    else if(m.isAllSoundOff())engine->controller(m.getChannel(),120,0);
}
void GbaProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& messages) {
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();if(buffer.getNumChannels()<2)return;
    const auto settings=snapshot();const bool requested=romHacks.enabled();
    auto* sf=romHacks.beginBlock(settings);
    if(requested!=soundFontMode){soundFontMode=requested;engine->reset();if(sf)sf->reset();modeFade=240;modeLeft=lastLeft;modeRight=lastRight;}
    engine->setSettings(settings);drainCommands();
    int offset=0,total=buffer.getNumSamples();
    auto* left=buffer.getWritePointer(0);auto* right=buffer.getWritePointer(1);
    const auto render=[&](int begin,int count){if(soundFontMode){if(sf)sf->render(left+begin,right+begin,count);}else engine->render(left+begin,right+begin,count);};
    for(const auto metadata:messages) {
        int at=juce::jlimit(offset,total,metadata.samplePosition);
        render(offset,at-offset);offset=at;if(metadata.numBytes<=3)midi(metadata.getMessage());
    }
    render(offset,total-offset);
    for(int i=0;modeFade>0&&i<total;++i){const float f=float(modeFade--)/240;left[i]=left[i]*(1-f)+modeLeft*f;right[i]=right[i]*(1-f)+modeRight*f;}
    if(total>0){lastLeft=left[total-1];lastRight=right[total-1];}
    messages.clear();active.store(soundFontMode?(sf?sf->activeVoices():0):engine->activeVoices(),std::memory_order_relaxed);
    romHacks.endBlock();
    level.store(total>0?buffer.getMagnitude(0,total):0,std::memory_order_relaxed);
}
int GbaProcessor::getNumPrograms() {return int(gba::factoryPrograms.size());}
void GbaProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& messages) {
    // Keep MIDI, envelopes and effects moving while the zero-input instrument
    // is muted. In particular, note-offs during bypass must not leave held notes
    // when the host resumes. The fixed-latency DSP path remains warm.
    processBlock(buffer,messages);
    buffer.clear();
}
void GbaProcessor::setCurrentProgram(int i) {if(i>=0&&i<getNumPrograms())pendingProgram.store(i);}
const juce::String GbaProcessor::getProgramName(int i) {return i>=0&&i<getNumPrograms()?juce::String(gba::factoryPrograms[std::size_t(i)].name):juce::String();}
void GbaProcessor::timerCallback(){servicePendingProgram();const auto r=romHacks.revision();if(r!=soundFontRevision){soundFontRevision=r;updateHostDisplay(juce::AudioProcessorListener::ChangeDetails{}.withNonParameterStateChanged(true));}}
void GbaProcessor::servicePendingProgram() {
    const int i=pendingProgram.exchange(-1);if(i<0||i>=getNumPrograms())return;
    const auto& program=gba::factoryPrograms[std::size_t(i)];
    if(program.patchIndex>=0){presets.load(program.id);return;}
    // Removed categories have no runtime recipes. Keep their host numbers reserved
    // so program automation for every surviving sound continues selecting it.
    presets.restoreDefault(false);
    const juce::ScopedLock lock(metadataLock);
    activePresetId=program.id;activePresetName=program.name;currentProgram.store(i);
}
void GbaProcessor::applyPreset(const GbaPreset& p) {
    const juce::ScopedLock lock(metadataLock);
    removedSamplePlayback=false;romHacks.useSoundFont(false);
    // All changes use host-visible parameter notifications; the audio thread reads atomics only.
    for(std::size_t i=0;i<gba::parameterCount;++i) {
        auto* parameter=state.getParameter(juce::String(gba::specs[i].id.data()));
        parameter->beginChangeGesture();parameter->setValueNotifyingHost(parameter->convertTo0to1(p.settings.values[i]));parameter->endChangeGesture();
    }
    activePresetId=p.id;activePresetName=p.name;activePresetCategory=p.category;
    currentProgram.store(0);
    for(std::size_t i=0;i<gba::factoryPrograms.size();++i)if(p.id==gba::factoryPrograms[i].id) {currentProgram.store(int(i));break;}
    stopPreview();
}
juce::String GbaProcessor::presetId() const {const juce::ScopedLock lock(metadataLock);return activePresetId;}
juce::String GbaProcessor::presetName() const {const juce::ScopedLock lock(metadataLock);return activePresetName;}
juce::String GbaProcessor::presetCategory() const {const juce::ScopedLock lock(metadataLock);return activePresetCategory;}
juce::String GbaProcessor::stateNotice() const {
    const juce::ScopedLock lock(metadataLock);
    if(removedSamplePlayback)return "Sample playback was removed. Only synthesis settings were restored; the original project file is unchanged.";
    const int program=currentProgram.load();
    if(activePresetCategory=="Saved Project"&&program>=0&&std::size_t(program)<gba::factoryPrograms.size()
       &&gba::factoryPrograms[std::size_t(program)].patchIndex<0)
        return "This factory category was removed. The sound saved in your project was restored. Use Save As to keep a user preset.";
    return {};
}
void GbaProcessor::setEditorSkin(int index) {
    const int selected=gba_ui::validSkin(index);
    if(editorSkin.exchange(selected)!=selected)
        updateHostDisplay(juce::AudioProcessorListener::ChangeDetails{}.withNonParameterStateChanged(true));
}
void GbaProcessor::setNewSkinOnLaunch(bool enabled) {
    if(newSkinOnLaunch.exchange(enabled)!=enabled)
        updateHostDisplay(juce::AudioProcessorListener::ChangeDetails{}.withNonParameterStateChanged(true));
}
void GbaProcessor::initializeEditorAppearance() {
    const juce::ScopedLock lock(metadataLock);
    if(editorAppearanceInitialized)return;
    editorSkin.store(launchSkin(presets.userFolder().getParentDirectory(),newSkinOnLaunch.load(),editorSkin.load()));
    editorAppearanceInitialized=true;
}
void GbaProcessor::renameCurrentPreset(const juce::String& name) {const juce::ScopedLock lock(metadataLock);activePresetName=name;}
void GbaProcessor::getStateInformation(juce::MemoryBlock& destination) {
    const juce::ScopedLock lock(metadataLock);
    auto copy=state.copyState();copy.setProperty("presetId",activePresetId,nullptr);copy.setProperty("presetName",activePresetName,nullptr);copy.setProperty("presetCategory",activePresetCategory,nullptr);copy.setProperty("schemaVersion",1,nullptr);copy.setProperty("editorWidth",editorWidth.load(),nullptr);
    copy.setProperty("editorSkin",juce::String(gba_ui::skins[std::size_t(gba_ui::validSkin(editorSkin.load()))].id.data()),nullptr);
    copy.setProperty("newSkinOnLaunch",newSkinOnLaunch.load(),nullptr);
    copy.removeProperty("sampleData",nullptr);
    auto rom=romHacks.saveState();if(rom.isVoid())copy.removeProperty("romHacks",nullptr);else copy.setProperty("romHacks",juce::JSON::toString(rom),nullptr);
    if(removedSamplePlayback)copy.setProperty("samplePlaybackRemoved",true,nullptr);else copy.removeProperty("samplePlaybackRemoved",nullptr);
    if(auto xml=copy.createXml())copyXmlToBinary(*xml,destination);
}
void GbaProcessor::setStateInformation(const void* data,int bytes) {
    // Accept the old 0.12.0 project size limit solely to discard embedded audio.
    // No audio decoder, sample allocation or sample playback path remains.
    if(bytes<=0||bytes>16*1024*1024)return;
    auto xml=getXmlFromBinary(data,bytes);if(!xml)return;
    auto restored=juce::ValueTree::fromXml(*xml);
    if(!restored.hasType("GBA2_STATE")||int(restored.getProperty("schemaVersion",1))!=1)return;
    const bool removed=restored.hasProperty("sampleData")||bool(restored.getProperty("samplePlaybackRemoved",false));
    restored.removeProperty("sampleData",nullptr);
    // Validate parameter domains before replacing host state. Malformed projects keep the current patch.
    for(auto child:restored) {
        auto id=child.getProperty("id").toString();
        for(const auto& spec:gba::specs) if(id==spec.id.data()) {
            float value=float(child.getProperty("value",spec.initial));if(!std::isfinite(value))return;
            child.setProperty("value",juce::jlimit(spec.min,spec.max,value),nullptr);
        }
    }
    juce::var rom;if(restored.hasProperty("romHacks")&&juce::JSON::parse(restored.getProperty("romHacks").toString(),rom).failed())return;
    const juce::ScopedLock lock(metadataLock);
    romHacks.restore(rom);romHacks.waitUntilIdle(30000);
    removedSamplePlayback=removed;
    pendingProgram.store(-1);state.replaceState(restored);
    activePresetId=restored.getProperty("presetId").toString();activePresetName=restored.getProperty("presetName","Custom patch").toString();activePresetCategory=restored.getProperty("presetCategory","User").toString();
    if(removed&&!activePresetName.startsWith("SYNTH ONLY: "))activePresetName="SYNTH ONLY: "+activePresetName;
    editorWidth.store(juce::jlimit(960,1920,int(restored.getProperty("editorWidth",1280))));
    const bool automatic=bool(restored.getProperty("newSkinOnLaunch",true));
    newSkinOnLaunch.store(automatic);
    // A newly loaded instance chooses once when its first window opens. State
    // refresh/undo and reopening that same window must never reroll its colour.
    if(!automatic||!editorAppearanceInitialized)
        editorSkin.store(gba_ui::skinIndex(restored.getProperty("editorSkin","purple").toString().toStdString()));
    currentProgram.store(0);
    for(std::size_t i=0;i<gba::factoryPrograms.size();++i)if(activePresetId==gba::factoryPrograms[i].id){
        currentProgram.store(int(i));
        if(gba::factoryPrograms[i].patchIndex<0)activePresetCategory="Saved Project";
        break;
    }
    panicRequested.store(true);
}
juce::AudioProcessorEditor* GbaProcessor::createEditor() {initializeEditorAppearance();romHacks.startLibrary();return new GbaEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {return new GbaProcessor();}
