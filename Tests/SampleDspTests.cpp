#include "dsp/SynthEngine.h"
#include <atomic>
#include <iostream>
#include <memory>
#include <stdexcept>
extern std::atomic<bool> trackAllocations;
extern std::atomic<int> allocations;
int main(){int checks=0;const auto check=[&](bool ok,const char* text){++checks;if(!ok)throw std::runtime_error(text);};
try {
    gba::SampleData sample;sample.rate=48000;gba::SampleData::Level data;data.left.resize(48000);data.right.resize(48000);
    for(int i=0;i<48000;++i)data.left[std::size_t(i)]=.5f*std::sin(float(2*std::numbers::pi_v<double>*261.625565*i/48000));
    sample.levels.push_back(std::move(data));sample.buildMips();
    const auto below=sample.read(123.25,1.99999,true),above=sample.read(123.25,2.00001,true);
    check(std::abs(below[0]-above[0])<.0001f,"Sample mip transition is discontinuous");
    auto engine=std::make_unique<gba::SynthEngine>();gba::Settings s;
    s.set(gba::Param::Reverb,0);s.set(gba::Param::Chorus,0);s.set(gba::Param::Drive,0);s.set(gba::Param::Vibrato,0);s.set(gba::Param::Lfo1Depth,0);s.set(gba::Param::FilterEnv,0);s.set(gba::Param::Cutoff,20000);s.set(gba::Param::Resonance,0);s.set(gba::Param::Attack,.001f);s.set(gba::Param::Sustain,1);s.set(gba::Param::Release,.02f);
    std::array<float,256> l{},r{};
    for(double rate:{44100.,48000.,96000.})for(int mode:{1,2}){
        engine->setSettings(s);engine->prepare(rate);engine->setSample(&sample,{mode,60,true});engine->noteOn(1,60,1);
        float peak=0;for(int n=0;n<200;++n){engine->render(l.data(),r.data(),256);for(auto v:l){check(std::isfinite(v),"Non-finite sample output");peak=std::max(peak,std::abs(v));}}
        check(peak>.01f,"Sample playback is silent");engine->controller(1,64,127);engine->noteOff(1,60);engine->render(l.data(),r.data(),256);check(engine->activeVoices()>0,"Sample sustain failed");engine->controller(1,64,0);
        for(int n=0;n<100;++n){engine->render(l.data(),r.data(),256);}
        check(engine->activeVoices()==0,"Sample release stuck");
    }
    engine->setSample(&sample,{1,60,true});engine->noteOn(1,60,1);allocations=0;trackAllocations=true;
    for(int n=0;n<256;++n){engine->pitchWheel(1,n%2?16383:0);engine->render(l.data(),r.data(),256);if(n%8==0)engine->noteOn(1,48+n%36,.8f);}
    engine->setSample(nullptr,{});engine->render(l.data(),r.data(),256);trackAllocations=false;
    check(allocations==0,"Sample render/MIDI/switch allocated on audio thread");check(engine->activeVoices()==0,"Sample removal left active notes");
    std::cout<<"PASS "<<checks<<" native sample DSP checks; zero render/MIDI/switch allocations.\n";return 0;
}catch(const std::exception& e){trackAllocations=false;std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}}
