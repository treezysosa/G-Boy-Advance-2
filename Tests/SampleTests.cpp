#include "PluginProcessor.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace {
int checks=0;
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
void set(GbaProcessor& p,const char* id,float value){auto* parameter=p.state.getParameter(id);parameter->setValueNotifyingHost(parameter->convertTo0to1(value));}
void dry(GbaProcessor& p){for(auto id:{"fx.reverb","fx.delay","fx.chorus","fx.saturation","fx.distortion","mod.vibrato","lfo1.depth","lfo2.depth","filter.env"})set(p,id,0);set(p,"fx.width",1);set(p,"filter.cutoff",20000);set(p,"filter.resonance",0);set(p,"amp.attack",.001f);set(p,"amp.sustain",1);set(p,"amp.release",.02f);}
void fixture(const juce::File& file,bool aiff=false,bool stereo=true,double rate=48000,int frames=48000){
    auto stream=file.createOutputStream();check(stream!=nullptr,"Sample fixture file failed");std::unique_ptr<juce::AudioFormat> format;
    if(aiff)format=std::make_unique<juce::AiffAudioFormat>();else format=std::make_unique<juce::WavAudioFormat>();
    std::unique_ptr<juce::AudioFormatWriter> writer(format->createWriterFor(stream.release(),rate,stereo?2:1,24,{},0));check(writer!=nullptr,"Sample fixture writer failed");
    juce::AudioBuffer<float> audio(stereo?2:1,frames);audio.clear();for(int i=0;i<frames;++i)audio.setSample(0,i,.5f*std::sin(float(2*juce::MathConstants<double>::pi*261.625565*i/rate)));
    check(writer->writeFromAudioSampleBuffer(audio,0,frames),"Sample fixture audio write failed");
}
void render(GbaProcessor& p,juce::AudioBuffer<float>& audio,int note=60){juce::MidiBuffer midi;midi.addEvent(juce::MidiMessage::noteOn(1,note,.9f),0);p.processBlock(audio,midi);}
}
int runSampleTests(){checks=0;auto root=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("gba2-samples-"+juce::Uuid().toString());check(root.createDirectory().wasOk(),"Sample test directory failed");
try {
    auto wav=root.getChildFile("Original.wav");fixture(wav);
    auto processor=std::make_unique<GbaProcessor>(root);auto& p=*processor;dry(p);p.setRateAndBufferSizeDetails(48000,256);p.prepareToPlay(48000,256);
    p.samples.loadFile(wav);check(p.samples.waitUntilIdle(),"Sample loader timed out");check(p.samples.state().isObject(),"WAV did not load");
    juce::AudioBuffer<float> audio(2,16384);render(p,audio);check(audio.getMagnitude(0,0,16384)>.01f,"Sample did not produce audible audio");check(audio.getMagnitude(1,0,16384)<1e-7f,"Stereo sample channels were collapsed or leaked");
    int crossings=0;for(int i=4097;i<16384;++i)if(audio.getSample(0,i-1)<0&&audio.getSample(0,i)>=0)++crossings;
    check(std::abs(crossings*48000.0/(16384-4096)-261.625565)<7,"Root sample pitch is incorrect");
    juce::MemoryBlock saved;p.getStateInformation(saved);check(saved.getSize()>10000,"Sample audio was not embedded in project state");
    const auto sample=p.samples.state();check(p.presets.saveAs("Recorded Instrument","sample custom").wasOk(),"Sample instrument Save As failed");check(p.samples.waitUntilIdle(),"Saved instrument load timed out");auto id=p.presetId();
    auto stored=juce::JSON::parse(p.presets.find(id)->file);check(int(stored["schemaVersion"])==2&&stored["sample"]["audio"].toString()==sample["audio"].toString(),"Sample preset did not embed its audio");
    auto exported=root.getChildFile("Portable.gba2preset");check(p.presets.exportCurrent(exported).wasOk(),"Sample instrument export failed");check(p.presets.importPreset(exported).wasOk()&&p.samples.waitUntilIdle(),"Sample instrument import failed");check(p.samples.state()["audio"].toString()==sample["audio"].toString(),"Imported instrument lost audio");
    check(wav.deleteFile(),"Original audio fixture could not be removed");auto restored=std::make_unique<GbaProcessor>(root.getChildFile("Restored"));restored->setStateInformation(saved.getData(),int(saved.getSize()));
    juce::MemoryBlock immediate;restored->getStateInformation(immediate);check(immediate.getSize()>10000,"Saving during asynchronous project restore lost sample data");
    check(restored->samples.waitUntilIdle()&&restored->samples.state().isObject(),"Embedded project sample required the original file");restored->setRateAndBufferSizeDetails(48000,256);restored->prepareToPlay(48000,256);render(*restored,audio);check(audio.getMagnitude(0,0,16384)>.01f,"Restored project sample is silent");
    p.samples.configure({1,60,true});p.panic();render(p,audio);check(p.samples.state()["loop"],"Loop option did not persist");
    juce::MidiBuffer midi;for(int n=48;n<88;++n)midi.addEvent(juce::MidiMessage::noteOn(1,n,.5f),0);p.processBlock(audio,midi);check(p.voiceCount()<=int(p.snapshot()[gba::Param::Voices]),"Samples bypassed voice-count budget");
    midi.addEvent(juce::MidiMessage::allSoundOff(1),0);p.processBlock(audio,midi);check(p.voiceCount()==0,"Sample all-sound-off failed");
    p.samples.loadFile(root.getChildFile("Unknown.mse"));check(p.samples.waitUntilIdle()&&p.samples.status().contains("not supported")&&p.samples.state().isObject(),"Unsupported file replaced good sample or hid the error");
    auto broken=root.getChildFile("Broken.wav");check(broken.replaceWithText("not audio"),"Malformed audio fixture failed");p.samples.loadFile(broken);check(p.samples.waitUntilIdle()&&p.samples.status().contains("corrupt")&&p.samples.state().isObject(),"Malformed audio was accepted or replaced the working instrument");
    auto aiff=root.getChildFile("Original.aiff");fixture(aiff,true,false,44100,4410);p.samples.loadFile(aiff);check(p.samples.waitUntilIdle(),"AIFF decode timed out");p.samples.configure({1,60,true});p.panic();render(p,audio);check(audio.getMagnitude(0,0,16384)>.01f&&audio.getMagnitude(1,0,16384)>.01f,"AIFF/mono duplication/rate conversion/loop failed");
    p.samples.configure({2,60,true});p.panic();render(p,audio);check(int(p.samples.state()["mode"])==2&&audio.getMagnitude(0,0,16384)>.01f,"Layer mode failed");
    p.presets.restoreDefault();check(p.samples.state().isVoid(),"Factory preset retained sample playback");p.panic();render(p,audio);check(audio.getMagnitude(0,0,16384)>.001f,"Clearing sample broke factory synthesis");
    // Rapid publication / cancellation while the real audio callback reads snapshots.
    std::atomic<bool> running{true};std::atomic<int> blocks{0};std::thread audioThread([&]{juce::AudioBuffer<float> block(2,64);while(running){juce::MidiBuffer events;p.processBlock(block,events);++blocks;}});
    for(int i=0;i<80;++i){p.samples.loadFile(aiff);if(i%3==0)p.samples.clear();else p.samples.configure({i%3,48+i%36,i%2!=0});}
    p.samples.clear();check(p.samples.waitUntilIdle(),"Cancelled sample jobs did not settle");running=false;audioThread.join();check(blocks>0&&p.samples.state().isVoid(),"Publication/cancellation stress failed");
    p.releaseResources();restored->releaseResources();restored.reset();processor.reset();root.deleteRecursively();std::cout<<"PASS "<<checks<<" sample playback checks.\n";return checks;
}catch(...){root.deleteRecursively();throw;}}
