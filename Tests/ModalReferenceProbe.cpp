#include "dsp/SynthEngine.h"
#include <fstream>
#include <memory>
#include <vector>
int main(int argc,char** argv) {
    if(argc!=2)return 1;
    std::ofstream file(argv[1],std::ios::binary);auto e=std::make_unique<gba::SynthEngine>();
    for(double sr:{44100.,48000.,96000.})for(int note:{24,60,96})for(float decay:{.005f,.4f,4.f})for(float velocity:{.4f,1.f}) {
        gba::Settings s;s.set(gba::Param::Engine,7);s.set(gba::Param::Decay,decay);s.set(gba::Param::Reverb,0);s.set(gba::Param::Delay,0);
        s.set(gba::Param::Lfo2Depth,0);s.set(gba::Param::Sustain,1);s.set(gba::Param::Attack,.001f);
        e->setSettings(s);e->prepare(sr);e->noteOn(1,note,velocity);
        const int count=int(sr*.5);std::vector<float> l(count),r(count);e->render(l.data(),r.data(),count);
        file.write(reinterpret_cast<const char*>(l.data()),count*sizeof(float));file.write(reinterpret_cast<const char*>(r.data()),count*sizeof(float));
    }
    return file?0:1;
}
