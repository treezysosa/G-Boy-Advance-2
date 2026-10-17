#include <juce_audio_utils/juce_audio_utils.h>
#include "dsp/Parameters.h"
#include <atomic>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
int checks=0;
void check(bool condition,const char* message) {++checks;if(!condition)throw std::runtime_error(message);}
using Plugin=juce::AudioPluginInstance;
juce::AudioProcessorParameter* parameter(Plugin& p,gba::Param id) {
    const auto name=juce::String(gba::specs[gba::index(id)].name.data());
    for(auto* param:p.getParameters())if(param->getName(128)==name)return param;
    throw std::runtime_error("Missing hosted parameter: "+name.toStdString());
}
void set(Plugin& p,gba::Param id,float value) {
    const auto& spec=gba::specs[gba::index(id)];
    juce::NormalisableRange<float> range(spec.min,spec.max,spec.step,spec.skew);
    parameter(p,id)->setValueNotifyingHost(range.convertTo0to1(value));
}
void dry(Plugin& p) {
    set(p,gba::Param::Engine,0);set(p,gba::Param::Wave1,0);set(p,gba::Param::Wave2,0);
    set(p,gba::Param::Attack,.001f);set(p,gba::Param::Decay,.01f);
    set(p,gba::Param::Sustain,1);set(p,gba::Param::Release,.02f);
    set(p,gba::Param::Reverb,0);set(p,gba::Param::Delay,0);
    set(p,gba::Param::Chorus,0);set(p,gba::Param::Flanger,0);
    set(p,gba::Param::Phaser,0);set(p,gba::Param::Wow,0);
    set(p,gba::Param::Drive,0);set(p,gba::Param::Distortion,0);
}
void prepare(Plugin& p,double sr,int block) {
    p.setRateAndBufferSizeDetails(sr,block);p.prepareToPlay(sr,block);
    check(p.getLatencySamples()==16,"Prepared VST3 host latency is not 16 samples");
}
float process(Plugin& p,juce::AudioBuffer<float>& audio,juce::MidiBuffer& midi,bool bypass=false) {
    audio.clear();if(bypass)p.processBlockBypassed(audio,midi);else p.processBlock(audio,midi);
    for(int c=0;c<2;++c)for(int i=0;i<audio.getNumSamples();++i)
        if(!std::isfinite(audio.getSample(c,i)))throw std::runtime_error("Nonfinite hosted output");
    return audio.getMagnitude(0,audio.getNumSamples());
}
float render(Plugin& p,juce::AudioBuffer<float>& audio,int blocks) {
    float peak=0;for(int i=0;i<blocks;++i){juce::MidiBuffer midi;peak=std::max(peak,process(p,audio,midi));}return peak;
}
void pump(int ms) {juce::MessageManager::getInstance()->runDispatchLoopUntil(ms);}
}
int main(int argc,char** argv) {
    juce::ScopedJuceInitialiser_GUI gui;
    try {
        check(argc==2,"Usage: gba2-host-tests <bundle.vst3>");
        auto bundle=juce::File(juce::String::fromUTF8(argv[1]));
        juce::AudioPluginFormatManager manager;manager.addFormat(new juce::VST3PluginFormat());
        juce::OwnedArray<juce::PluginDescription> types;
        manager.getFormat(0)->findAllTypesForFile(types,bundle.getFullPathName());
        check(types.size()==1&&types[0]->isInstrument,"VST3 instrument scan failed");
        auto load=[&] {
            juce::String error;auto p=manager.createPluginInstance(*types[0],48000,256,error);
            if(!p)throw std::runtime_error("VST3 load failed: "+error.toStdString());
            return p;
        };
        auto p=load();check(p->getTotalNumInputChannels()==0&&p->getTotalNumOutputChannels()==2,"Unexpected VST3 buses");
        check(p->acceptsMidi()&&!p->producesMidi(),"Unexpected VST3 MIDI contract");
        check(p->getNumPrograms()==410,"Factory host program contract changed");
        check(p->getProgramName(409)=="Handheld Pulse Motion - Afterglow","Final factory program moved");
        // Confirm all 64 public controls are present, not just a generic host bypass.
        for(std::size_t i=0;i<gba::parameterCount;++i)check(parameter(*p,gba::Param(i))!=nullptr,"Hosted parameter missing");
        std::cout<<"Loaded actual VST3 version "<<types[0]->version<<"\n";
        for(double sr:{44100.,48000.,96000.})for(int block:{64,257,1024}) {
            dry(*p);prepare(*p,sr,block);juce::AudioBuffer<float> audio(2,block);
            check(render(*p,audio,2)==0,"Reprepared VST3 retained old audio");
            juce::MidiBuffer midi;midi.addEvent(juce::MidiMessage::noteOn(1,60,.8f),block/2);
            process(*p,audio,midi);
            check(audio.getMagnitude(0,0,block/2)==0,"VST3 note ignored MIDI sample offset");
            check(render(*p,audio,12)>.0001f,"VST3 note produced no audio");
            // A real VST3 bypass parameter/path must keep consuming note-offs.
            midi.clear();midi.addEvent(juce::MidiMessage::noteOff(1,60),0);
            check(process(*p,audio,midi,true)==0,"VST3 bypass leaked synthesized audio");
            for(int i=0;i<int(sr*.15/block)+1;++i){midi.clear();process(*p,audio,midi,true);}
            check(render(*p,audio,4)<1e-5f,"Note-off during bypass left a held note on resume");
            midi.clear();midi.addEvent(juce::MidiMessage::noteOn(1,67,.7f),0);process(*p,audio,midi);
            check(render(*p,audio,4)>.0001f,"VST3 failed to resume after bypass");
            p->releaseResources();
        }
        dry(*p);prepare(*p,48000,256);juce::AudioBuffer<float> audio(2,256);
        set(*p,gba::Param::Cutoff,4321);set(*p,gba::Param::Volume,.371f);
        render(*p,audio,2);pump(30);
        std::vector<float> expected;
        for(std::size_t i=0;i<gba::parameterCount;++i)expected.push_back(parameter(*p,gba::Param(i))->getValue());
        juce::MemoryBlock state;p->getStateInformation(state);check(state.getSize()>0,"VST3 state is empty");
        auto restored=load();restored->setStateInformation(state.getData(),int(state.getSize()));
        prepare(*restored,48000,256);render(*restored,audio,2);pump(30);
        for(std::size_t i=0;i<gba::parameterCount;++i)
            check(std::abs(parameter(*restored,gba::Param(i))->getValue()-expected[i])<1e-4f,"VST3 parameter state roundtrip failed");
        // Two loaded modules/instances: playing one must not create audio in the other.
        juce::MidiBuffer midi;midi.addEvent(juce::MidiMessage::noteOn(1,72,.8f),0);process(*p,audio,midi);
        check(render(*restored,audio,8)==0,"VST3 instances share active voices");
        check(render(*p,audio,8)>.0001f,"First VST3 instance stopped unexpectedly");
        const unsigned char sysex[]{0x7d,1,2,3};midi.clear();midi.addEvent(juce::MidiMessage::createSysExMessage(sysex,4),0);
        process(*p,audio,midi);check(render(*p,audio,2)>.0001f,"SysEx damaged hosted synthesis");
        // VST3 state calls belong on the message thread. Run actual processing
        // on a worker while the message thread serializes the component/controller.
        std::atomic<bool> done{false},badAudio{false};bool badState=false;
        std::jthread audioThread([&]{
            juce::AudioBuffer<float> concurrentAudio(2,256);int blocks=0;
            try {while(!done.load()||blocks<80){juce::MidiBuffer events;process(*p,concurrentAudio,events);++blocks;}}
            catch(const std::exception&){badAudio=true;}
        });
        for(int i=0;i<40;++i){juce::MemoryBlock s;p->getStateInformation(s);if(s.getSize()==0)badState=true;}
        done=true;audioThread.join();
        check(!badState&&!badAudio,"Concurrent VST3 processing/serialization failed");
        p->setNonRealtime(true);check(render(*p,audio,4)>.0001f,"Offline VST3 processing failed");p->setNonRealtime(false);
        // Recreate an instance from its project state, then verify release/tail bounds.
        p->releaseResources();p.reset();p=load();p->setStateInformation(state.getData(),int(state.getSize()));prepare(*p,48000,256);
        check(p->getTailLengthSeconds()>=.02&&p->getTailLengthSeconds()<1,"Dry VST3 tail estimate is incorrect");
        midi.clear();midi.addEvent(juce::MidiMessage::noteOn(1,60,.8f),0);process(*p,audio,midi);render(*p,audio,8);
        midi.clear();midi.addEvent(juce::MidiMessage::noteOff(1,60),0);process(*p,audio,midi);
        render(*p,audio,64);check(render(*p,audio,4)<1e-5f,"Dry VST3 note release did not reach silence");
        set(*p,gba::Param::Delay,.8f);set(*p,gba::Param::DelayTime,1.5f);set(*p,gba::Param::DelayFeedback,.85f);
        set(*p,gba::Param::Reverb,.8f);set(*p,gba::Param::ReverbSize,1);set(*p,gba::Param::Release,12);
        render(*p,audio,2);check(p->getTailLengthSeconds()>120,"VST3 wrapper truncated long export-tail estimate");
        p->releaseResources();restored->releaseResources();
        std::cout<<"PASS "<<checks<<" actual VST3 host checks; prepared latency 16 samples at 44.1/48/96 kHz.\n";
        return 0;
    } catch(const std::exception& ex){std::cerr<<"FAIL host acceptance: "<<ex.what()<<"\n";return 1;}
}
