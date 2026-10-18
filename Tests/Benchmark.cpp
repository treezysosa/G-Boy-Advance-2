#include "dsp/SynthEngine.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>
// Deliberately an observational benchmark, not a machine-speed CI threshold.
// Timing includes settings, automation and render; initialization is outside it.
int main(int argc,char** argv) {
    int voices=32;
    try {if(argc==3)voices=std::stoi(argv[2]);}catch(...){voices=0;}
    std::ofstream file;if(argc>=2)file.open(argv[1]);
    if(argc>3||voices<1||voices>32||(argc>=2&&!file)){std::cerr<<"Usage: gba2-benchmark [output.csv] [voices: 1-32]\n";return 1;}
    auto& out=argc>=2?static_cast<std::ostream&>(file):std::cout;
    out<<"sample_rate,block,engine,voices,callbacks,mean_us,p50_us,p95_us,p99_us,max_us,budget_us,max_budget_ratio,over_budget,invalid_samples\n"<<std::setprecision(9);
    auto e=std::make_unique<gba::SynthEngine>();bool invalid=false;
    for(double sr:{44100.,48000.,96000.})for(int block:{64,256})for(int mode=0;mode<11;++mode) {
        gba::Settings s;s.set(gba::Param::Voices,float(voices));s.set(gba::Param::Engine,float(mode<8?mode:mode==8?0:7));
        s.set(gba::Param::Harmonics,24);s.set(gba::Param::Brightness,.95f);s.set(gba::Param::Sustain,1);
        s.set(gba::Param::Reverb,.7f);s.set(gba::Param::Delay,.4f);s.set(gba::Param::Chorus,.5f);
        s.set(gba::Param::Flanger,.4f);s.set(gba::Param::Phaser,.5f);s.set(gba::Param::Drive,.5f);
        s.set(gba::Param::Distortion,.4f);s.set(gba::Param::Wow,.4f);
        e->setSettings(s);e->prepare(sr);for(int n=24;n<24+voices;++n)e->noteOn(1,n,.8f);
        std::vector<float> left(std::size_t(block),0.f),right(std::size_t(block),0.f);
        for(int i=0;i<16;++i)e->render(left.data(),right.data(),block);
        const int count=int(sr*.25/block)+1;std::vector<double> times;times.reserve(std::size_t(count));
        const double budget=1e6*block/sr;int missed=0,bad=0;double sum=0;
        for(int i=0;i<count;++i) {
            auto start=std::chrono::steady_clock::now();
            // Rapid selector changes, then the two expensive engine pairs.
            if(mode==8)s.set(gba::Param::Engine,float(i%8));
            if(mode==9)s.set(gba::Param::Engine,float(i%2?2:7));
            if(mode==10)s.set(gba::Param::Engine,float(i%2?6:7));
            s.set(gba::Param::Cutoff,500.f+15000.f*float(i%31)/30.f);
            s.set(gba::Param::DelayTime,.08f+.6f*float(i%19)/18.f);
            e->setSettings(s);e->render(left.data(),right.data(),block);
            const double us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
            times.push_back(us);sum+=us;if(us>budget)++missed;
            for(int j=0;j<block;++j)if(!std::isfinite(left[std::size_t(j)])||!std::isfinite(right[std::size_t(j)]))++bad;
        }
        std::sort(times.begin(),times.end());auto percentile=[&](double p){return times[std::size_t(p*(times.size()-1))];};
        out<<sr<<','<<block<<','<<mode<<','<<voices<<','<<count<<','<<sum/count<<','<<percentile(.5)<<','<<percentile(.95)<<','<<percentile(.99)<<','<<times.back()<<','<<budget<<','<<times.back()/budget<<','<<missed<<','<<bad<<'\n';
        invalid|=bad>0;
    }
    return invalid?1:0;
}
