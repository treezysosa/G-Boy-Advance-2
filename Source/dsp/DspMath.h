#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
namespace gba {
inline constexpr float pi = std::numbers::pi_v<float>;
inline constexpr float tau = 2*pi;
inline float sine(float phase) noexcept { return std::sin(tau * phase); }
inline float midiHz(float note) noexcept { return 440.0f * std::exp2((note-69.0f)/12.0f); }
inline float wrap(float p) noexcept { return p-std::floor(p); }
inline float blep(float t, float dt) noexcept {
    if (t < dt) { t/=dt; return t+t-t*t-1; }
    if (t > 1-dt) { t=(t-1)/dt; return t*t+t+t+1; }
    return 0;
}
struct Random {
    std::uint32_t state = 0x1579ab23u;
    float next() noexcept { state^=state<<13; state^=state>>17; state^=state<<5; return float(state>>8)*(2.0f/16777216.0f)-1; }
};
struct Envelope {
    enum Stage { Off, Attack, Decay, Sustain, Release } stage = Off;
    double value = 0, releaseStep = 0;
    void start(bool retrigger) noexcept { if (retrigger) value=0; stage=Attack; }
    void stop(float seconds, float sr) noexcept { if (stage!=Off&&stage!=Release) { stage=Release; releaseStep=value/std::max(1.0,double(seconds)*sr); } }
    float next(float a, float d, float s, float sr) noexcept {
        switch(stage) {
            case Off: value=0; break;
            case Attack: value+=1/std::max(1.0,double(a)*sr); if(value>=1) {value=1;stage=Decay;} break;
            case Decay: value-=(1-double(s))/std::max(1.0,double(d)*sr); if(value<=s) {value=s;stage=Sustain;} break;
            case Sustain: value=s; break;
            case Release: value-=releaseStep; if(value<=0) {value=0;stage=Off;} break;
        }
        return float(value);
    }
};
// Bounded double-precision timebase: no elapsed-time float that eventually stops
// advancing on a held note. The rates used by the engine are below sample rate.
struct LfoPhase {
    double phase=0;
    void advance(double hz,double sr) noexcept {phase+=hz/sr;if(phase>=1)phase-=std::floor(phase);}
    float value() const noexcept {return float(std::sin(2*std::numbers::pi_v<double>*phase));}
};
// Fixed-size, interruptible linear crossfade. Retargeting starts at the current
// blend; reset/prepare starts directly on the selected choice.
template<std::size_t Count> struct ChoiceFade {
    std::array<float,Count> weights{},step{};
    int target=0,remaining=0;
    void reset(int choice) noexcept {target=choice;remaining=0;weights.fill(0);weights[std::size_t(choice)]=1;}
    void advance(int choice,int samples) noexcept {
        if(choice!=target) {
            target=choice;remaining=std::max(1,samples);
            for(std::size_t i=0;i<Count;++i)step[i]=((i==std::size_t(target)?1.0f:0.0f)-weights[i])/float(remaining);
        }
        if(remaining>0) {
            if(--remaining==0)reset(target);
            else for(std::size_t i=0;i<Count;++i)weights[i]+=step[i];
        }
    }
};
// Topology preserving state variable filter, stable as cutoff approaches Nyquist.
struct SVF {
    struct Outputs {
        float low,high,band,notch;
        float select(int type) const noexcept {switch(type){case 1:return high;case 2:return band;case 3:return notch;default:return low;}}
    };
    float s1=0,s2=0,g=0.1f,k=1;
    void coefficients(float hz,float resonance,float sr) noexcept {
        g=std::tan(pi*std::clamp(hz, 15.0f, sr*0.44f)/sr); k=2-1.95f*std::clamp(resonance,0.0f,0.95f);
    }
    Outputs process(float x) noexcept {
        const float a=1/(1+g*(g+k)), v1=a*(s1+g*(x-s2)), v2=s2+g*v1;
        s1=2*v1-s1; s2=2*v2-s2;
        return {v2,x-k*v1-v2,v1,x-k*v1};
    }
    float next(float x,int type) noexcept {return process(x).select(type);}
    void clear() noexcept {s1=s2=0;}
};
struct Smooth {
    float value=0, target=0, coefficient=0;
    void prepare(float seconds,float sr) noexcept {coefficient=std::exp(-1/(seconds*sr));}
    float next() noexcept {value=target+coefficient*(value-target); return value;}
};
}
