#include "dsp/SynthEngine.h"
#include <chrono>
#include <iostream>
#include <memory>
int main() {
    auto e=std::make_unique<gba::SynthEngine>();
    float left[64]{},right[64]{};
    std::cout<<"engine,seconds_per_rendered_second\n";
    for(int mode=0;mode<8;++mode) {
        gba::Settings s;s.set(gba::Param::Voices,32);s.set(gba::Param::Engine,float(mode));
        s.set(gba::Param::Harmonics,24);s.set(gba::Param::Brightness,.95f);s.set(gba::Param::Sustain,1);
        e->setSettings(s);e->prepare(96000);for(int n=24;n<56;++n)e->noteOn(1,n,.8f);
        auto start=std::chrono::steady_clock::now();
        for(int sample=0;sample<96000;sample+=64)e->render(left,right,64);
        double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
        std::cout<<mode<<","<<seconds<<"\n";
    }
}
