#pragma once
#include "DspMath.h"

namespace gba {
// Original retro synthesis; no ROM, sample data or cycle-accurate console emulation.
struct RetroEngine {
    struct ClockedNoise {
        std::uint32_t longRegister=0x7fff,shortRegister=0x7f;
        double phase=0;
        static void step(std::uint32_t& state,int bits) noexcept {
            const auto feedback=(state^(state>>1))&1u;
            state=(state>>1)|(feedback<<(bits-1));
        }
        float next(float dt,float shortBlend) noexcept {
            phase+=std::clamp(double(dt)*16,0.0,.45);
            if(phase>=1){phase-=1;step(longRegister,15);step(shortRegister,7);}
            const float a=(longRegister&1u)?1.f:-1.f,b=(shortRegister&1u)?1.f:-1.f;
            return std::lerp(a,b,shortBlend);
        }
    };
    std::array<ClockedNoise,2> noise{};
    float tone=0,coefficient=.5f,shortBlend=.5f;
    void reset() noexcept {noise={};noise[1].longRegister=0x5a35;noise[1].shortRegister=0x55;tone=0;}
    void configure(float brightness,float rate) noexcept {
        shortBlend=brightness;
        coefficient=1-std::exp(-tau*(800+brightness*14000)/rate);
    }
    static float dac(float x,int levels) noexcept {
        return std::round((std::clamp(x,-1.0f,1.0f)+1)*.5f*float(levels-1))*2/float(levels-1)-1;
    }
    static float wave(int shape,float p,float dt,float width,float noiseValue) noexcept {
        switch(shape){
            case 0:return dac(sine(p),16);
            case 1:{float x=0;for(int h=1;h<=15;h+=2)if(float(h)*dt<.48f)x+=sine(p*float(h))*((h%4==1)?1.f:-1.f)/float(h*h);return dac(x*.81f,16);}
            case 2:return dac(2*p-1-blep(p,dt),32);
            case 3:width=.5f;[[fallthrough]];
            case 4:return (p<width?1.f:-1.f)+blep(p,dt)-blep(wrap(p+1-width),dt);
            default:return noiseValue;
        }
    }
    static float mixed(const ChoiceFade<6>& fade,float p,float dt,float width,float n) noexcept {
        if(fade.remaining==0)return wave(fade.target,p,dt,width,n);
        float sum=0;for(std::size_t i=0;i<6;++i)if(fade.weights[i]>0)sum+=wave(int(i),p,dt,width,n)*fade.weights[i];return sum;
    }
    float next(const ChoiceFade<6>& a,const ChoiceFade<6>& b,float p1,float p2,float dt1,float dt2,float width,float mix) noexcept {
        const float n1=noise[0].next(dt1,shortBlend),n2=noise[1].next(dt2,shortBlend);
        const float x=std::lerp(mixed(a,p1,dt1,width,n1),mixed(b,p2,dt2,width,n2),mix);
        tone+=coefficient*(x-tone);return tone;
    }
};
}
