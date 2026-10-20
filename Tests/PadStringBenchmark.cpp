#include "dsp/SynthEngine.h"
#include "dsp/SoundFont.h"
#include "FactoryData.h"
#include "SoundFontFixture.h"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

// Matched native DSP workloads. The SF2 bank contains original mathematical
// test waves only; these fixtures are never installed with the instrument.
namespace {
template<class Engine> bool profile(Engine& engine,gba::Settings settings,
    std::string_view name,std::string_view kind,std::ostream& out,int& cases) {
    for(double rate:{44100.,48000.,96000.})for(int block:{64,256})for(int voices:{4,8,16,32})for(bool overlap:{false,true}) {
        settings.set(gba::Param::Voices,float(voices));settings.set(gba::Param::Mode,0);
        engine.setSettings(settings);engine.prepare(rate);
        std::array<float,256> l{},r{};
        const int first=overlap?voices/2:voices;
        for(int n=0;n<first;++n){engine.noteOn(n%2+1,43+n%21,.8f);if(overlap)engine.render(l.data(),r.data(),32);}
        for(int i=0;i<int(rate*.15/block)+1;++i)engine.render(l.data(),r.data(),block);
        if(overlap) {
            for(int n=0;n<first;++n)engine.noteOff(n%2+1,43+n%21);
            for(int n=0;n<first;++n){engine.noteOn(n%2+1,55+n%21,.75f);engine.render(l.data(),r.data(),32);}
        }
        if(engine.activeVoices()==0)return false;
        const int count=int(rate*.125/block)+1;std::vector<double> times;times.reserve(std::size_t(count));
        const double budget=1e6*block/rate;double total=0;int missed=0,bad=0;
        for(int i=0;i<count;++i) {
            const auto start=std::chrono::steady_clock::now();
            engine.setSettings(settings);engine.render(l.data(),r.data(),block);
            const double us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
            times.push_back(us);total+=us;if(us>budget)++missed;
            for(int n=0;n<block;++n)if(!std::isfinite(l[std::size_t(n)])||!std::isfinite(r[std::size_t(n)]))++bad;
        }
        if(bad)return false;
        std::sort(times.begin(),times.end());
        const auto percentile=[&](double p){return times[std::size_t(p*(times.size()-1))];};
        out<<kind<<','<<name<<','<<(overlap?"release-overlap":"held-chord")<<','<<rate<<','<<block<<','<<voices<<','<<count<<','<<total/count<<','<<percentile(.5)<<','<<percentile(.95)<<','<<percentile(.99)<<','<<times.back()<<','<<budget<<','<<missed<<','<<bad<<'\n';++cases;
    }
    return bool(out);
}
}
int main(int argc,char** argv) {
    if(argc!=2){std::cerr<<"Usage: gba2-pad-string-benchmark output.csv\n";return 1;}
    std::ofstream out(argv[1]);if(!out)return 1;
    out<<"engine,fixture,workload,sample_rate,block,voices,callbacks,mean_us,p50_us,p95_us,p99_us,max_us,budget_us,over_budget,invalid_samples\n"<<std::setprecision(9);
    int cases=0;auto synth=std::make_unique<gba::SynthEngine>();
    for(std::string_view name:{"Analog Soul Pad","Warm Tape Pad","Dream Pad","Dark Horror Pad","Ambient Pad","Angelic Pad",
        "Vintage Strings","Synth Strings","Cinematic Strings","Orchestral-Style Strings","Dream Strings","Pizzicato Strings"}) {
        const auto patch=std::find_if(gba::factoryPatches.begin(),gba::factoryPatches.end(),[&](const auto& p){return name==p.name;});
        if(patch==gba::factoryPatches.end())return 1;
        gba::Settings s;s.values=patch->values;if(!profile(*synth,s,name,"synth",out,cases))return 1;
    }
    auto data=sf2fixture::bank();
    // Two-second release for sustained-bank overlap, retaining legal SF2 data.
    const char table[]{'i','g','e','n'};const auto at=std::search(data.begin(),data.end(),table,table+4);
    if(at==data.end())return 1;
    const auto offset=std::size_t(at-data.begin())+8;
    for(std::size_t pos=offset;pos<offset+40;pos+=4)if(data[pos]==38&&data[pos+1]==0){data[pos+2]=0xb0;data[pos+3]=4;}
    gba::SoundFont sf;std::string error;if(!sf.load(data.data(),data.size(),error)){std::cerr<<error<<'\n';return 1;}
    for(int instrument=0;instrument<2;++instrument) {
        sf.select(instrument);gba::Settings s;s.set(gba::Param::Attack,.2f);s.set(gba::Param::Release,2);
        s.set(gba::Param::Chorus,.4f);s.set(gba::Param::Reverb,.3f);s.set(gba::Param::Vibrato,.1f);
        if(!profile(sf,s,instrument?"SF2 sustained harmonics":"SF2 sustained sine","sf2",out,cases))return 1;
    }
    std::cout<<"PASS "<<cases<<" pad/string/SF2 held-chord and release-overlap scenes\n";return out?0:1;
}
