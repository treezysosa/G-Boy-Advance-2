#include "dsp/SynthEngine.h"
#include "FactoryData.h"
#include <fstream>
#include <iostream>
#include <memory>

// Offline numerical comparison input for old/new engines. This is test output,
// never a plugin asset or a substitute for perceptual listening/DAW testing.
int main(int argc,char** argv) {
    if(argc!=2){std::cerr<<"Usage: gba2-performance-probe output.f32\n";return 1;}
    std::ofstream out(argv[1],std::ios::binary);
    if(!out)return 1;
    auto e=std::make_unique<gba::SynthEngine>();
    std::array<float,64> left{},right{};std::size_t samples=0;int scenes=0;
    const auto write=[&] {
        e->render(left.data(),right.data(),64);
        for(int i=0;i<64;++i)if(!std::isfinite(left[i])||!std::isfinite(right[i]))return false;
        out.write(reinterpret_cast<const char*>(left.data()),sizeof(left));
        out.write(reinterpret_cast<const char*>(right.data()),sizeof(right));
        samples+=128;return bool(out);
    };
    // Static controls, automation, voiced/formant transitions, pan modulation,
    // sustain, release, stealing and sample-rate reinitialization.
    for(double rate:{44100.,48000.,96000.})for(int engine=0;engine<8;++engine)for(int filter=0;filter<5;++filter) {
        gba::Settings s;s.set(gba::Param::Engine,float(engine));s.set(gba::Param::FilterType,float(filter));
        s.set(gba::Param::Voices,8);s.set(gba::Param::Sustain,1);s.set(gba::Param::Harmonics,24);
        s.set(gba::Param::Glide,.04f);s.set(gba::Param::FilterDecay,.02f);s.set(gba::Param::Release,.04f);
        s.set(gba::Param::Vibrato,0);s.set(gba::Param::Lfo1Depth,0);
        e->setSettings(s);e->prepare(rate);
        for(int n=0;n<8;++n)e->noteOn(n%2+1,48+n*3,.3f+float(n)*.08f);
        for(int block=0;block<256;++block) {
            if(block>=64&&block<128) {
                s.set(gba::Param::Brightness,float(block%19)/18);s.set(gba::Param::FormantShift,float(block%25)-12);
                s.set(gba::Param::Vowel,float(block%17)/4);s.set(gba::Param::Cutoff,400+float(block%31)*500);
                s.set(gba::Param::Lfo1Depth,.6f);s.set(gba::Param::Lfo1Dest,float(block%6));
                s.set(gba::Param::CrusherBits,block%8<4?16:8);s.set(gba::Param::CrusherRate,block%8<4?1:4);
                s.set(gba::Param::FilterType,float((filter+block/16)%5));
                s.set(gba::Param::Chorus,block<96?.45f:0);s.set(gba::Param::Flanger,block<96?.4f:0);s.set(gba::Param::Wow,block<96?.3f:0);
                s.set(gba::Param::Drive,block<96?float(block%11)/10:0);s.set(gba::Param::Distortion,block<96?float(block%7)/6:0);
                s.set(gba::Param::EqLow,float(block%25)-12);s.set(gba::Param::EqHigh,12-float(block%25));
                s.set(gba::Param::Glide,float(block%9)*.1f);s.set(gba::Param::Detune,float(block%71)-35);
                s.set(gba::Param::Attack,.001f+float(block%17)*.01f);s.set(gba::Param::Decay,.01f+float(block%13)*.02f);
                s.set(gba::Param::Sustain,float(block%11)/10);s.set(gba::Param::FilterAttack,.001f+float(block%7)*.02f);
                s.set(gba::Param::FilterDecay,.01f+float(block%9)*.03f);s.set(gba::Param::FilterSustain,float(block%9)/8);
                if(block%16==0)s.set(gba::Param::Engine,float((engine+block/16)%8));
            }
            if(block==128){s.set(gba::Param::Lfo1Depth,0);s.set(gba::Param::Engine,float(engine));e->controller(1,64,127);}
            if(block==20)e->noteOn(2,70,.65f);
            if(block==32)e->controller(2,1,96);
            if(block==100)e->noteOff(2,70);
            if(block==136)e->pitchWheel(2,11000);
            if(block==144)for(int n=0;n<8;++n)e->noteOff(n%2+1,48+n*3);
            if(block==160)e->noteOn(1,72,.7f);
            if(block==168)e->controller(1,64,0);
            if(block==176)e->controller(2,120,0);
            if(block==192)e->allNotesOff();
            e->setSettings(s);if(!write())return 1;
        }
        ++scenes;
    }
    // Every factory patch with a chord and its release, retaining factory arrays.
    for(const auto& patch:gba::factoryPatches) {
        gba::Settings s;s.values=patch.values;e->setSettings(s);e->prepare(48000);
        for(int n:{48,55,60,64})e->noteOn(1,n,.8f);
        for(int block=0;block<96;++block) {
            if(block==64)for(int n:{48,55,60,64})e->noteOff(1,n);
            if(!write())return 1;
        }
        ++scenes;
    }
    std::cout<<"PASS "<<scenes<<" numerical comparison scenes; "<<samples<<" finite float samples\n";
    return 0;
}
