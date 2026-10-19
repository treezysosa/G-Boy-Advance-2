#include "dsp/SynthEngine.h"
#include "FactoryData.h"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

// Observational callback timings for ordinary notes, chords and retriggers.
// Both engine versions receive identical factory arrays, events and drivers.
int main(int argc,char** argv) {
    if(argc!=2){std::cerr<<"Usage: gba2-play-benchmark output.csv\n";return 1;}
    std::ofstream out(argv[1]);if(!out)return 1;
    out<<"fixture,workload,sample_rate,block,voices,callbacks,mean_us,p50_us,p95_us,p99_us,max_us,budget_us,over_budget,invalid_samples\n"<<std::setprecision(9);
    auto e=std::make_unique<gba::SynthEngine>();int cases=0;
    for(std::string_view name:{"FM Piano","Dream Pad","Smooth R&B Vocal Pad",
        "Detroit Concrete Upright","Detroit Ice Glass Bell","Detroit Short Ah Vox",
        "West Coast Sunset Tine Keys","West Coast Freeway String Pad","West Coast High Whistle Lead",
        "Florida Bright Club Piano","Florida Heat Wave Pluck","Florida Airy Ooh Texture",
        "Detroit Brick Hammer Piano","Drill Vacant Block Strings","Funk Wah Clav Snap",
        "Japan Opening Theme Lead","80s Digital Aurora Pad"}) {
        const auto patch=std::find_if(gba::factoryPatches.begin(),gba::factoryPatches.end(),[&](const auto& p){return name==p.name;});
        if(patch==gba::factoryPatches.end())return 1;
        for(double rate:{44100.,48000.,96000.})for(int block:{64,256})for(int voices:{1,4,8})for(bool retrigger:{false,true}) {
            gba::Settings s;s.values=patch->values;s.set(gba::Param::Voices,float(voices));s.set(gba::Param::Mode,0);
            e->setSettings(s);e->prepare(rate);for(int n=0;n<voices;++n)e->noteOn(1,48+n*3,.8f);
            std::vector<float> l(std::size_t(block),0.f),r(l);for(int i=0;i<16;++i)e->render(l.data(),r.data(),block);
            const int count=int(rate*.15/block)+1;std::vector<double> times;times.reserve(std::size_t(count));
            const double budget=1e6*block/rate;double total=0;int missed=0,bad=0,root=48;
            for(int i=0;i<count;++i) {
                const auto start=std::chrono::steady_clock::now();
                if(retrigger&&i%8==0) {
                    for(int n=0;n<voices;++n)e->noteOff(1,root+n*3);
                    root=48+(i/8)%4;for(int n=0;n<voices;++n)e->noteOn(1,root+n*3,.8f);
                }
                e->setSettings(s);e->render(l.data(),r.data(),block);
                const double us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
                times.push_back(us);total+=us;if(us>budget)++missed;
                for(int n=0;n<block;++n)if(!std::isfinite(l[std::size_t(n)])||!std::isfinite(r[std::size_t(n)]))++bad;
            }
            if(bad)return 1;
            std::sort(times.begin(),times.end());
            const auto percentile=[&](double p){return times[std::size_t(p*(times.size()-1))];};
            out<<name<<','<<(retrigger?"retrigger":"held-chord")<<','<<rate<<','<<block<<','<<voices<<','<<count<<','<<total/count<<','<<percentile(.5)<<','<<percentile(.95)<<','<<percentile(.99)<<','<<times.back()<<','<<budget<<','<<missed<<','<<bad<<'\n';++cases;
        }
    }
    std::cout<<"PASS "<<cases<<" note/chord/retrigger profiling scenes\n";return out?0:1;
}
