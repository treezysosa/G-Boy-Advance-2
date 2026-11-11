#include "dsp/Effects.h"
#include <fstream>
#include <iostream>
int main(int argc,char** argv) {
    if(argc!=2)return 1;std::ofstream out(argv[1],std::ios::binary);std::size_t frames=0;
    for(float rate:{44100.f,48000.f,96000.f})for(int bypass:{0,1,17,63,64,65,128,512})for(int offset:{0,17}) {
        gba::Effects fx;fx.prepare(rate);gba::Settings s;s.set(gba::Param::Reverb,0);s.set(gba::Param::Chorus,0);s.set(gba::Param::Delay,0);
        gba::Random rng;
        for(int n=0;n<4096;++n) {
            if(n==0)s.set(gba::Param::Drive,.6f);
            if(n==64+offset)s.set(gba::Param::Drive,0);
            if(n==64+offset+bypass)s.set(gba::Param::Drive,.6f);
            if(n==128){s.set(gba::Param::EqLow,12);s.set(gba::Param::EqHigh,-9);}
            if(n==256){s.set(gba::Param::EqLow,0);s.set(gba::Param::EqHigh,0);}
            if(n==512){s.set(gba::Param::EqLow,-7);s.set(gba::Param::EqHigh,6);}
            if(n==1280){s.set(gba::Param::EqLow,0);s.set(gba::Param::EqHigh,0);}
            if(n==768+offset)s.set(gba::Param::Drive,0);
            if(n==1280+offset){s.set(gba::Param::Drive,.4f);s.set(gba::Param::Distortion,.3f);}
            if(n==1536){s.set(gba::Param::Drive,0);s.set(gba::Param::Distortion,0);}
            if(n==2048)s.set(gba::Param::Drive,1);
            if(n==3072)s.set(gba::Param::Drive,0);
            float l=rng.next()*.3f,r=-l*.75f;fx.process(l,r,s);
            if(!std::isfinite(l)||!std::isfinite(r))return 2;
            out.write(reinterpret_cast<const char*>(&l),4);out.write(reinterpret_cast<const char*>(&r),4);++frames;
        }
    }
    std::cout<<"PASS "<<frames<<" drive-bypass/re-enable stereo frames across three rates\n";return out?0:3;
}
