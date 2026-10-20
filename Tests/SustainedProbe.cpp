#include "dsp/SynthEngine.h"
#include "dsp/SoundFont.h"
#include "FactoryData.h"
#include "SoundFontFixture.h"
#include <fstream>
#include <iostream>
#include <memory>

// Long, deterministic comparison against the released engine. Raw float output
// is temporary test data, never installed or read by the plugin.
namespace {
std::size_t samples=0;int scenes=0;
template<class Engine> bool scene(Engine& e,gba::Settings s,double rate,std::ostream& out,bool soundfont) {
    s.set(gba::Param::Voices,16);e.setSettings(s);e.prepare(rate);
    std::array<float,257> l{},r{};constexpr int sizes[]{1,17,64,257};
    for(int note:{48,55,60,64})e.noteOn(1,note,.8f);
    const int frames=int(rate*8);int at=0,block=0,event=0;
    const int events[]{int(rate*.25),int(rate*.5),int(rate),int(rate*1.5),int(rate*2),int(rate*2.5),int(rate*3),int(rate*3.5),int(rate*4),int(rate*4.5)};
    while(at<frames) {
        if(event<int(std::size(events))&&at==events[event]) {
            switch(event++) {
                case 0:e.noteOn(2,67,.65f);break;
                case 1:e.controller(1,1,96);e.pitchWheel(2,11000);break;
                case 2:e.controller(1,64,127);for(int note:{48,55,60,64})e.noteOff(1,note);break;
                case 3:for(int note:{50,57,62,65})e.noteOn(1,note,.7f);break;
                case 4:e.controller(1,64,0);e.noteOff(2,67);break;
                case 5:s.set(gba::Param::Cutoff,4321);s.set(gba::Param::Brightness,.74f);s.set(gba::Param::Lfo1Dest,5);e.setSettings(s);break;
                case 6:e.controller(1,121,0);e.pitchWheel(1,10000);e.noteOn(1,69,.75f,true);break;
                case 7:e.stopPreview();s.set(gba::Param::Reverb,.2f);s.set(gba::Param::Lfo1Dest,2);e.setSettings(s);break;
                case 8:for(int note:{50,57,62,65})e.noteOff(1,note);break;
                case 9:if(soundfont){e.controller(1,101,0);e.controller(1,100,1);e.controller(1,6,65);}break;
            }
        }
        int count=std::min(sizes[block++%4],frames-at);
        if(event<int(std::size(events)))count=std::min(count,events[event]-at);
        e.render(l.data(),r.data(),count);
        for(int n=0;n<count;++n)if(!std::isfinite(l[std::size_t(n)])||!std::isfinite(r[std::size_t(n)]))return false;
        out.write(reinterpret_cast<const char*>(l.data()),std::streamsize(count*sizeof(float)));
        out.write(reinterpret_cast<const char*>(r.data()),std::streamsize(count*sizeof(float)));
        samples+=std::size_t(count)*2;at+=count;
    }
    ++scenes;return bool(out);
}
}
int main(int argc,char** argv) {
    if(argc!=2){std::cerr<<"Usage: gba2-sustained-probe output.f32\n";return 1;}
    std::ofstream out(argv[1],std::ios::binary);if(!out)return 1;
    auto synth=std::make_unique<gba::SynthEngine>();
    for(const auto& p:gba::factoryPatches)if(std::string_view(p.category)=="Pads"||std::string_view(p.category)=="Strings") {
        gba::Settings s;s.values=p.values;
        for(double rate:{44100.,48000.,96000.})if(!scene(*synth,s,rate,out,false))return 1;
    }
    // Exercise non-default roots/transposition, channel controls, pitch bend,
    // glide, fractional rates, preset changes, preview and irregular blocks.
    for(int variant=0;variant<2;++variant) {
        auto bank=sf2fixture::bank();
        const char table[]{'i','g','e','n'};const auto tableAt=std::search(bank.begin(),bank.end(),table,table+4);
        if(tableAt==bank.end())return 1;
        const auto offset=std::size_t(tableAt-bank.begin())+8;
        for(std::size_t pos=offset;pos<offset+40;pos+=4) {
            if(bank[pos]==38&&bank[pos+1]==0){bank[pos+2]=0xb0;bank[pos+3]=4;}
            if(variant&&bank[pos]==17&&bank[pos+1]==0){bank[pos]=51;bank[pos+2]=7;bank[pos+3]=0;}
        }
        gba::SoundFont sf;std::string error;if(!sf.load(bank.data(),bank.size(),error)){std::cerr<<error<<'\n';return 1;}
        for(int instrument=0;instrument<2;++instrument)for(double rate:{44100.,48000.,96000.,48000.5}) {
            sf.select(instrument);gba::Settings s;s.set(gba::Param::Attack,.2f);s.set(gba::Param::Glide,.13f);s.set(gba::Param::Vibrato,.1f);
            if(!scene(sf,s,rate,out,true))return 1;
        }
    }
    std::cout<<"PASS "<<scenes<<" sustained pad/string/SF2 scenes; "<<samples<<" finite float samples\n";
    return out?0:1;
}
