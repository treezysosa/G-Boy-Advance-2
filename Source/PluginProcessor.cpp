#include "PluginProcessor.h"
#include "UiSkins.h"
#include "PluginEditor.h"
#include "FactoryData.h"

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
      state(*this,nullptr,"GBA2_STATE",createLayout()),presets(*this,dataRoot),engine(std::make_unique<gba::SynthEngine>()) {
    for(std::size_t i=0;i<raw.size();++i) raw[i]=state.getRawParameterValue(juce::String(gba::specs[i].id.data()));
    setLatencySamples(gba::SynthEngine::latencySamples);
    keyboard.addListener(this);presets.restoreDefault(false);startTimerHz(20);
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
    return s[gba::Param::Release]+echo+room+.1;
}
void GbaProcessor::prepareToPlay(double sr,int) {engine->setSettings(snapshot());engine->prepare(sr);}
void GbaProcessor::releaseResources() {engine->reset();active=0;level=0;}
void GbaProcessor::reset() {engine->reset();active=0;level=0;}
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
            if(c.kind==1)engine->noteOn(c.channel,c.note,c.value);
            if(c.kind==2)engine->noteOff(c.channel,c.note);
            if(c.kind==3)engine->noteOn(c.channel,c.note,c.value,true);
            if(c.kind==4)engine->stopPreview();
        }
    };
    process(first,count);process(second,count2);commandFifo.finishedRead(count+count2);
    if(panicRequested.exchange(false))engine->allNotesOff(true);
    int pitch=pitchRequested.exchange(-1);if(pitch>=0)engine->pitchWheel(1,pitch);
}
void GbaProcessor::midi(const juce::MidiMessage& m) noexcept {
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
    engine->setSettings(snapshot());drainCommands();
    int offset=0,total=buffer.getNumSamples();
    auto* left=buffer.getWritePointer(0);auto* right=buffer.getWritePointer(1);
    for(const auto metadata:messages) {
        int at=juce::jlimit(offset,total,metadata.samplePosition);
        engine->render(left+offset,right+offset,at-offset);offset=at;if(metadata.numBytes<=3)midi(metadata.getMessage());
    }
    engine->render(left+offset,right+offset,total-offset);
    messages.clear();active.store(engine->activeVoices(),std::memory_order_relaxed);
    level.store(total>0?buffer.getMagnitude(0,total):0,std::memory_order_relaxed);
}
int GbaProcessor::getNumPrograms() {return int(gba::factoryPatches.size());}
void GbaProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& messages) {
    // Keep MIDI, envelopes and effects moving while the zero-input instrument
    // is muted. In particular, note-offs during bypass must not leave held notes
    // when the host resumes. The fixed-latency DSP path remains warm.
    processBlock(buffer,messages);
    buffer.clear();
}
void GbaProcessor::setCurrentProgram(int i) {if(i>=0&&i<getNumPrograms())pendingProgram.store(i);}
const juce::String GbaProcessor::getProgramName(int i) {return i>=0&&i<getNumPrograms()?juce::String(gba::factoryPatches[std::size_t(i)].name):juce::String();}
void GbaProcessor::servicePendingProgram() {int i=pendingProgram.exchange(-1);if(i>=0)presets.load(gba::factoryPatches[std::size_t(i)].id);}
void GbaProcessor::applyPreset(const GbaPreset& p) {
    const juce::ScopedLock lock(metadataLock);
    // All changes use host-visible parameter notifications; the audio thread reads atomics only.
    for(std::size_t i=0;i<gba::parameterCount;++i) {
        auto* parameter=state.getParameter(juce::String(gba::specs[i].id.data()));
        parameter->beginChangeGesture();parameter->setValueNotifyingHost(parameter->convertTo0to1(p.settings.values[i]));parameter->endChangeGesture();
    }
    activePresetId=p.id;activePresetName=p.name;activePresetCategory=p.category;
    currentProgram.store(0);
    for(std::size_t i=0;i<gba::factoryPatches.size();++i)if(p.id==gba::factoryPatches[i].id) {currentProgram.store(int(i));break;}
    stopPreview();
}
juce::String GbaProcessor::presetId() const {const juce::ScopedLock lock(metadataLock);return activePresetId;}
juce::String GbaProcessor::presetName() const {const juce::ScopedLock lock(metadataLock);return activePresetName;}
juce::String GbaProcessor::presetCategory() const {const juce::ScopedLock lock(metadataLock);return activePresetCategory;}
void GbaProcessor::renameCurrentPreset(const juce::String& name) {const juce::ScopedLock lock(metadataLock);activePresetName=name;}
void GbaProcessor::getStateInformation(juce::MemoryBlock& destination) {
    const juce::ScopedLock lock(metadataLock);
    auto copy=state.copyState();copy.setProperty("presetId",activePresetId,nullptr);copy.setProperty("presetName",activePresetName,nullptr);copy.setProperty("presetCategory",activePresetCategory,nullptr);copy.setProperty("schemaVersion",1,nullptr);copy.setProperty("editorWidth",editorWidth.load(),nullptr);
    copy.setProperty("editorSkin",juce::String(gba_ui::skins[std::size_t(gba_ui::validSkin(editorSkin.load()))].id.data()),nullptr);
    if(auto xml=copy.createXml())copyXmlToBinary(*xml,destination);
}
void GbaProcessor::setStateInformation(const void* data,int bytes) {
    if(bytes<=0||bytes>4*1024*1024)return;
    auto xml=getXmlFromBinary(data,bytes);if(!xml)return;
    auto restored=juce::ValueTree::fromXml(*xml);
    if(!restored.hasType("GBA2_STATE")||int(restored.getProperty("schemaVersion",1))!=1)return;
    // Validate parameter domains before replacing host state. Malformed projects keep the current patch.
    for(auto child:restored) {
        auto id=child.getProperty("id").toString();
        for(const auto& spec:gba::specs) if(id==spec.id.data()) {
            float value=float(child.getProperty("value",spec.initial));if(!std::isfinite(value))return;
            child.setProperty("value",juce::jlimit(spec.min,spec.max,value),nullptr);
        }
    }
    const juce::ScopedLock lock(metadataLock);
    pendingProgram.store(-1);state.replaceState(restored);
    activePresetId=restored.getProperty("presetId").toString();activePresetName=restored.getProperty("presetName","Custom patch").toString();activePresetCategory=restored.getProperty("presetCategory","User").toString();
    editorWidth.store(juce::jlimit(960,1920,int(restored.getProperty("editorWidth",1280))));
    editorSkin.store(gba_ui::skinIndex(restored.getProperty("editorSkin","purple").toString().toStdString()));
    currentProgram.store(0);
    for(std::size_t i=0;i<gba::factoryPatches.size();++i)if(restored.getProperty("presetId").toString()==gba::factoryPatches[i].id){currentProgram.store(int(i));break;}
    panicRequested.store(true);
}
juce::AudioProcessorEditor* GbaProcessor::createEditor() {return new GbaEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {return new GbaProcessor();}
