#pragma once
#include "DspMath.h"

namespace gba {
// Original synthetic vocal excitation. No speech model, recordings or audio assets.
// The five resonances describe broad vowels, independent of the played pitch.
struct VocalEngine {
    std::array<SVF,5> bands{};
    std::array<float,3> tilt{};
    float lowerPhase=0,airLow=0,tiltCoefficient=.3f,airCoefficient=.02f;
    void reset() noexcept {for(auto& f:bands)f.clear();tilt={};lowerPhase=0;airLow=0;}
    void copyTuningFrom(const VocalEngine& other) noexcept {
        for(std::size_t i=0;i<bands.size();++i)bands[i].copyTuningFrom(other.bands[i]);
        tiltCoefficient=other.tiltCoefficient;airCoefficient=other.airCoefficient;
    }
    static std::array<float,5> frequencies(float vowel,float shift) noexcept {
        constexpr float vowels[5][5]{{800,1150,2900,3900,4950},{400,1700,2600,3500,4500},
            {300,2200,3000,3800,4900},{500,800,2600,3500,4500},{350,600,2400,3400,4400}};
        vowel=std::clamp(vowel,0.0f,4.0f);const int a=int(vowel),b=std::min(a+1,4);
        const float t=vowel-float(a),scale=std::exp2(shift/12);std::array<float,5> result{};
        for(std::size_t i=0;i<result.size();++i)result[i]=std::lerp(vowels[a][i],vowels[b][i],t)*scale;
        return result;
    }
    void configure(float vowel,float shift,float brightness,float rate) noexcept {
        const auto hz=frequencies(vowel,shift);constexpr float bandwidth[]{95,115,170,240,300};
        const float scale=std::exp2(shift/12);
        for(std::size_t i=0;i<bands.size();++i){
            const float f=std::clamp(hz[i],25.0f,rate*.42f);
            bands[i].tuning(std::tan(pi*f/rate),std::clamp(bandwidth[i]*scale/f,.055f,.65f));
        }
        // Spectral tilt is linear and follows the existing brightness control.
        tiltCoefficient=1-std::exp(-tau*(900+brightness*8500)/rate);
        airCoefficient=1-std::exp(-tau*160/rate);
    }
    float next(float phase,float upperPhase,float dt,float upperDt,float choir,float breath,Random& noise) noexcept {
        const float lowerDt=std::min(.45f,dt*dt/std::max(upperDt,1e-8f));
        lowerPhase=wrap(lowerPhase+lowerDt);
        const std::array<float,3> p{phase,upperPhase,lowerPhase};
        const std::array<float,3> delta{dt,upperDt,lowerDt};
        float voiced=0;
        for(std::size_t i=0;i<3;++i){
            if(i>0&&choir==0)continue;
            const float source=(2*p[i]-1-blep(p[i],delta[i]))*.75f+sine(p[i])*.25f;
            tilt[i]+=tiltCoefficient*(source-tilt[i]);
            voiced+=tilt[i]*(i==0?1.0f:choir*.5f);
        }
        voiced/=std::sqrt(1+choir*choir*.5f);
        const float air=noise.next();airLow+=airCoefficient*(air-airLow);
        const float input=std::lerp(voiced,(air-airLow)*.65f,breath);
        constexpr float gains[]{1,.82f,.56f,.32f,.2f};float out=0;
        for(std::size_t i=0;i<bands.size();++i)out+=bands[i].next(input,2)*bands[i].k*gains[i];
        return out*6;
    }
};
}
