#include "dsp/SoundFont.h"
#include "StressSoundFontFixture.h"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>

namespace {
bool renderProbe(gba::SoundFont& sf,gba::Settings s,double rate,std::ostream& output,std::size_t& total) {
    sf.setSettings(s);sf.prepare(rate);std::array<float,257> l{},r{};
    for(int n=0;n<8;++n)sf.noteOn(n%2+1,48+n*2,.75f);
    double energy=0;const int end=int(rate*6);constexpr int blocks[]{1,17,64,257};int at=0,block=0,event=0;
    const int events[]{int(rate*.5),int(rate),int(rate*1.5),int(rate*2),int(rate*2.5),int(rate*3),int(rate*3.5),int(rate*4),int(rate*4.5)};
    while(at<end) {
        if(event<int(std::size(events))&&at==events[event]) {
            switch(event++) {
                case 0:sf.controller(1,64,127);sf.controller(2,1,100);sf.pitchWheel(2,12000);break;
                case 1:for(int n=0;n<8;++n)sf.noteOff(n%2+1,48+n*2);break;
                case 2:for(int n=0;n<8;++n)sf.noteOn(n%2+1,55+n,.62f);break;
                case 3:sf.controller(1,64,0);s.set(gba::Param::Cutoff,3500);s.set(gba::Param::Drive,.4f);sf.setSettings(s);break;
                case 4:sf.controller(2,121,0);sf.controller(1,101,0);sf.controller(1,100,1);sf.controller(1,6,65);break;
                case 5:sf.noteOn(1,72,.8f,true);sf.pitchWheel(1,16000);break;
                case 6:sf.stopPreview();s.set(gba::Param::Chorus,.1f);s.set(gba::Param::CrusherBits,10);sf.setSettings(s);break;
                case 7:for(int n=0;n<8;++n)sf.noteOff(n%2+1,55+n);break;
                case 8:sf.select(1);sf.noteOn(2,57,.7f);break;
            }
        }
        int count=std::min(blocks[block++%4],end-at);
        if(event<int(std::size(events)))count=std::min(count,events[event]-at);
        sf.render(l.data(),r.data(),count);
        for(int n=0;n<count;++n){if(!std::isfinite(l[std::size_t(n)])||!std::isfinite(r[std::size_t(n)]))return false;energy+=double(l[std::size_t(n)])*l[std::size_t(n)]+double(r[std::size_t(n)])*r[std::size_t(n)];}
        output.write(reinterpret_cast<const char*>(l.data()),count*std::streamsize(sizeof(float)));
        output.write(reinterpret_cast<const char*>(r.data()),count*std::streamsize(sizeof(float)));
        total+=std::size_t(count)*2;at+=count;
    }
    return bool(output)&&energy>1e-8;
}
bool benchmark(gba::SoundFont& sf,gba::Settings s,bool dynamic,int program,std::ostream& output,int& scenes) {
    for(double rate:{44100.,48000.,96000.,48000.5})for(int block:{64,256})for(int voices:{4,8,16,32})for(bool overlap:{false,true}) {
        sf.select(program);s.set(gba::Param::Voices,float(voices));sf.setSettings(s);sf.prepare(rate);std::array<float,256> l{},r{};
        for(int n=0;n<voices/4;++n)sf.noteOn(n%2+1,48+n*2,.75f);
        for(int n=0;n<int(rate*.15/block)+1;++n)sf.render(l.data(),r.data(),block);
        if(overlap){for(int n=0;n<voices/4;++n)sf.noteOff(n%2+1,48+n*2);for(int n=0;n<voices/8;++n)sf.noteOn(n%2+1,55+n*2,.65f);}
        if(!sf.activeVoices())return false;
        const int callbacks=int(rate*.125/block)+1;std::vector<double> times;times.reserve(std::size_t(callbacks));double total=0,energy=0;int over=0;
        for(int n=0;n<callbacks;++n) {
            const auto start=std::chrono::steady_clock::now();sf.setSettings(s);sf.render(l.data(),r.data(),block);
            const double elapsed=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
            total+=elapsed;times.push_back(elapsed);over+=elapsed>1e6*block/rate;
            for(int i=0;i<block;++i){if(!std::isfinite(l[std::size_t(i)])||!std::isfinite(r[std::size_t(i)]))return false;energy+=double(l[std::size_t(i)])*l[std::size_t(i)]+double(r[std::size_t(i)])*r[std::size_t(i)];}
        }
        if(energy<=1e-10)return false;
        std::sort(times.begin(),times.end());
        output<<dynamic<<','<<program<<','<<(overlap?"release-overlap":"held-chord")<<','<<rate<<','<<block<<','<<voices<<','<<callbacks<<','<<total/callbacks<<','<<times[std::size_t(.95*(times.size()-1))]<<','<<times.back()<<','<<over<<",0\n";++scenes;
    }
    return bool(output);
}
}
int main(int argc,char** argv) {
    if(argc!=3){std::cerr<<"Usage: gba2-soundfont-stress --probe|--benchmark output\n";return 1;}
    const bool probe=std::string_view(argv[1])=="--probe";if(!probe&&std::string_view(argv[1])!="--benchmark")return 1;
    std::ofstream output(argv[2],std::ios::binary);if(!output)return 1;std::size_t total=0;int scenes=0;
    if(!probe)output<<"dynamic,program,workload,sample_rate,block,voices,callbacks,mean_us,p95_us,max_us,over_budget,invalid_samples\n"<<std::setprecision(9);
    for(bool dynamic:{false,true}) {
        const auto data=sf2stress::bank(dynamic);gba::SoundFont sf;std::string error;
        if(!sf.load(data.data(),data.size(),error)){std::cerr<<error<<'\n';return 1;}
        if(sf.instruments().size()!=2)return 1;
        for(int program=0;program<2;++program) {
            gba::Settings s;s.set(gba::Param::Voices,32);s.set(gba::Param::Attack,.2f);s.set(gba::Param::Glide,.1f);s.set(gba::Param::Chorus,.4f);s.set(gba::Param::Reverb,.3f);s.set(gba::Param::Vibrato,.1f);
            if(probe){for(double rate:{44100.,48000.,96000.,48000.5}){sf.select(program);if(!renderProbe(sf,s,rate,output,total))return 1;++scenes;}}
            else if(!benchmark(sf,s,dynamic,program,output,scenes))return 1;
        }
    }
    std::cout<<"PASS "<<scenes<<" layered SoundFont "<<(probe?"reference scenes; ":"CPU scenes; ")<<total<<" reference float samples\n";return 0;
}
