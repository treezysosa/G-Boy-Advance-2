#include "dsp/SynthEngine.h"
#include "FactoryData.h"
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>

extern std::atomic<bool> trackAllocations;
extern std::atomic<int> allocations;

namespace {
int checks=0;
void require(bool condition,const std::string& message) {++checks;if(!condition) throw std::runtime_error(message);}
struct Audio {std::vector<float> l,r;explicit Audio(int n):l(static_cast<std::size_t>(n)),r(static_cast<std::size_t>(n)){};};
float rms(const std::vector<float>& data) {double sum=0;for(float x:data)sum+=double(x)*x;return float(std::sqrt(sum/double(data.size())));}
void finite(const Audio& a) {for(auto* c:{&a.l,&a.r}) for(float v:*c) if(!std::isfinite(v)||std::abs(v)>1.001f)throw std::runtime_error("Non-finite or unbounded output");++checks;}
gba::Settings clean() {
    gba::Settings s;
    s.set(gba::Param::Wave1,0);s.set(gba::Param::Wave2,0);s.set(gba::Param::OscMix,0);
    s.set(gba::Param::Cutoff,20000);s.set(gba::Param::FilterEnv,0);s.set(gba::Param::Lfo1Depth,0);
    s.set(gba::Param::Vibrato,0);s.set(gba::Param::Reverb,0);s.set(gba::Param::Chorus,0);
    s.set(gba::Param::Drive,0);s.set(gba::Param::Attack,.001f);s.set(gba::Param::Sustain,1);s.set(gba::Param::Release,.02f);
    return s;
}
void init(gba::SynthEngine& e,const gba::Settings& s,double sr=48000) {e.setSettings(s);e.prepare(sr);}
float crossings(const Audio& a,float sr) {int count=0;std::size_t first=0,last=0;for(std::size_t i=1;i<a.l.size();++i)if(a.l[i-1]<=0&&a.l[i]>0){if(!count)first=i;last=i;++count;}return float(count-1)*sr/float(last-first);}
void writeWav(const std::string& path,const Audio& a,int sr) {
    std::ofstream f(path,std::ios::binary);const auto u16=[&](unsigned v){char b[]{char(v&255),char((v>>8)&255)};f.write(b,2);};
    const auto u32=[&](unsigned v){char b[]{char(v&255),char((v>>8)&255),char((v>>16)&255),char((v>>24)&255)};f.write(b,4);};
    unsigned bytes=unsigned(a.l.size())*4;f.write("RIFF",4);u32(36+bytes);f.write("WAVEfmt ",8);u32(16);u16(1);u16(2);u32(unsigned(sr));u32(unsigned(sr)*4);u16(4);u16(16);f.write("data",4);u32(bytes);
    for(std::size_t i=0;i<a.l.size();++i){u16(unsigned(int(std::clamp(a.l[i],-1.0f,1.0f)*32767))&65535);u16(unsigned(int(std::clamp(a.r[i],-1.0f,1.0f)*32767))&65535);}
}
}
int main(int argc,char** argv) {
    try {
        auto engine=std::make_unique<gba::SynthEngine>();auto& e=*engine;
        Audio early(128);e.render(early.l.data(),early.r.data(),128);require(rms(early.l)==0,"Unprepared engine must safely output silence");
        std::set<std::string_view> ids;
        for(const auto& s:gba::specs) {require(ids.insert(s.id).second,"Duplicate stable parameter ID");require(s.initial>=s.min&&s.initial<=s.max,"Invalid parameter default");}
        auto s=clean();init(e,s);Audio a(24000);e.noteOn(1,69,1);e.render(a.l.data(),a.r.data(),24000);finite(a);
        require(rms(a.l)>.01f,"Sine MIDI note is silent");require(std::abs(crossings(a,48000)-440)<1,"A4 tuning incorrect");
        e.pitchWheel(1,16383);e.render(a.l.data(),a.r.data(),24000);require(std::abs(crossings(a,48000)-493.883f)<2,"Pitch bend incorrect");
        e.noteOff(1,69);e.render(a.l.data(),a.r.data(),24000);require(e.activeVoices()==0,"Note did not release");
        std::cout<<"PASS tuning, pitch bend, release, stable parameter schema\n";
        init(e,s);e.noteOn(3,60,.8f);e.controller(3,64,127);e.noteOff(3,60);e.render(a.l.data(),a.r.data(),24000);require(e.activeVoices()==1,"Sustain failed");
        e.controller(2,64,0);require(e.activeVoices()==1,"Sustain channel isolation failed");
        e.controller(3,64,0);e.render(a.l.data(),a.r.data(),24000);require(e.activeVoices()==0,"Sustain pedal did not release");
        e.noteOn(1,64,0);require(e.activeVoices()==0,"Zero velocity note-on must be note-off");
        e.noteOn(1,60,1,true);e.render(a.l.data(),a.r.data(),24000);e.render(a.l.data(),a.r.data(),24000);require(e.activeVoices()==0,"Preview did not time out");
        e.noteOn(1,60,1);e.noteOn(1,64,1,true);e.stopPreview();e.render(a.l.data(),a.r.data(),24000);require(e.activeVoices()==1,"B button killed DAW notes or did not stop preview");
        e.controller(1,120,0);require(e.activeVoices()==0,"All sound off failed");
        init(e,clean());e.noteOn(1,60,1);e.noteOn(1,60,.8f);e.noteOff(1,60);e.render(a.l.data(),a.r.data(),24000);require(e.activeVoices()==1,"Overlapping identical notes released together");e.noteOff(1,60);e.render(a.l.data(),a.r.data(),24000);require(e.activeVoices()==0,"Overlapping identical note got stuck");
        std::cout<<"PASS sustain, channel isolation, velocity-zero, preview lifecycle, panic\n";
        s.set(gba::Param::Voices,4);init(e,s);for(int n=30;n<90;++n)e.noteOn(1,n,.8f);require(e.activeVoices()==4,"Voice stealing did not respect voice count");finite(a);
        s.set(gba::Param::Mode,2);s.set(gba::Param::Glide,.1f);init(e,s);e.noteOn(1,60,.7f);e.noteOn(1,64,.8f);e.noteOff(1,64);e.render(a.l.data(),a.r.data(),24000);require(e.activeVoices()==1,"Legato last-note priority failed");e.noteOff(1,60);e.render(a.l.data(),a.r.data(),24000);require(e.activeVoices()==0,"Legato note stack stuck");
        std::cout<<"PASS polyphony, stealing, legato, glide, held-note return\n";
        for(double sr:{44100.,48000.,96000.}) for(int mode=0;mode<8;++mode) {
            s=clean();s.set(gba::Param::Engine,float(mode));s.set(gba::Param::Wave1,2);s.set(gba::Param::Sustain,.7f);init(e,s,sr);
            e.noteOn(1,60,.9f);e.render(a.l.data(),a.r.data(),24000);finite(a);require(rms(a.l)>0.0001f,"Engine silent at sample rate "+std::to_string(sr)+", mode "+std::to_string(mode));
        }
        std::cout<<"PASS eight engines at 44.1, 48, 96 kHz\n";
        const gba::Param effects[]{gba::Param::Reverb,gba::Param::Delay,gba::Param::Chorus,gba::Param::Phaser,gba::Param::Flanger,gba::Param::Drive,gba::Param::Distortion,gba::Param::CrusherBits,gba::Param::CrusherRate,gba::Param::Wow,gba::Param::EqLow,gba::Param::EqHigh,gba::Param::Width};
        s=clean();init(e,s);e.noteOn(1,60,1);Audio dry(24000);e.render(dry.l.data(),dry.r.data(),24000);
        for(auto effect:effects) {
            auto wet=s;float value=effect==gba::Param::CrusherBits?6:effect==gba::Param::CrusherRate?8:effect==gba::Param::EqLow||effect==gba::Param::EqHigh?9:effect==gba::Param::Width?0:0.75f;
            wet.set(effect,value);init(e,wet);e.noteOn(1,60,1);e.render(a.l.data(),a.r.data(),24000);finite(a);
            double difference=0;for(std::size_t i=0;i<a.l.size();++i)difference+=std::abs(a.l[i]-dry.l[i]);require(difference>0.02,"Effect is inert: "+std::string(gba::specs[gba::index(effect)].id));
        }
        std::cout<<"PASS all 13 effect controls alter real audio\n";
        Audio shortAudio(8192);std::set<std::array<float,gba::parameterCount>> unique;float quietest=1;
        std::ofstream report("preset-validation.csv");report<<"id,name,category,rms,peak\n";
        for(const auto& patch:gba::factoryPatches) {
            s.values=patch.values;require(unique.insert(s.values).second,"Duplicate factory patch: "+std::string(patch.name));
            init(e,s);e.noteOn(1,std::string_view(patch.category)=="Bass"?36:60,.85f);e.render(shortAudio.l.data(),shortAudio.r.data(),8192);finite(shortAudio);
            float level=rms(shortAudio.l),peak=0;for(float v:shortAudio.l)peak=std::max(peak,std::abs(v));quietest=std::min(level,quietest);
            require(level>0.000005f,"Silent factory patch: "+std::string(patch.name));
            report<<patch.id<<",\""<<patch.name<<"\",\""<<patch.category<<"\","<<level<<","<<peak<<"\n";
        }
        std::cout<<"PASS "<<gba::factoryPatches.size()<<" distinct, audible, finite factory patches; quietest RMS "<<quietest<<"\n";
        init(e,clean());e.noteOn(1,60,1);allocations=0;trackAllocations=true;
        for(int i=0;i<50;++i) {e.noteOn(2,64,.8f);e.pitchWheel(2,9000);e.render(shortAudio.l.data(),shortAudio.r.data(),128);e.noteOff(2,64);e.controller(2,64,i%2?127:0);}
        trackAllocations=false;require(allocations==0,"Heap allocation in real-time DSP/MIDI path");
        std::cout<<"PASS zero heap allocations in real-time DSP/MIDI path\n";
        // Automation extremes and reinitialization must remain bounded.
        for(double sr:{44100.,96000.}) {
            init(e,clean(),sr);for(int n=36;n<68;++n)e.noteOn(n%16+1,n,1);
            for(int iteration=0;iteration<8;++iteration) {auto extreme=clean();for(std::size_t j=0;j<gba::parameterCount;++j)extreme.values[j]=iteration%2?gba::specs[j].min:gba::specs[j].max;extreme.set(gba::Param::Mode,0);extreme.set(gba::Param::Voices,32);extreme.set(gba::Param::Engine,float(iteration));e.setSettings(extreme);e.render(shortAudio.l.data(),shortAudio.r.data(),8192);finite(shortAudio);}
        }
        std::cout<<"PASS extreme automation and sample-rate reinitialization\n";
        if(argc>1 && std::string_view(argv[1])=="--render") {
            const char* names[]{"Dream Pad","70s Soul Guitar","FM Piano","Crystal Bells","90s R&B Vocal-Style Synth","8-Bit Lead"};
            for(auto name:names) for(const auto& patch:gba::factoryPatches)if(std::string_view(patch.name)==name) {
                s.values=patch.values;init(e,s);Audio demo(48000*4);e.noteOn(1,60,.9f);e.noteOn(1,67,.8f);e.render(demo.l.data(),demo.r.data(),48000*2);e.allNotesOff();e.render(demo.l.data()+48000*2,demo.r.data()+48000*2,48000*2);writeWav(std::string(name)+".wav",demo,48000);
            }
        }
        std::cout<<"All "<<checks<<" checks passed.\n";return 0;
    } catch(const std::exception& ex) {trackAllocations=false;std::cerr<<"FAIL: "<<ex.what()<<"\n";return 1;}
}
