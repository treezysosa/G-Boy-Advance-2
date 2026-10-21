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
// [9/9] Pade approximation in its accurate central interval. Double arithmetic
// retains float precision; the original libm response handles the outer range.
// No lookup memory, initialization or allocation; checked against libm in tests.
inline float softLimit(float value) noexcept {
    if(std::abs(value)>4||!std::isfinite(value))return std::tanh(value);
    const double x=value,z=x*x;
    return float(x*(34459425+z*(4729725+z*(135135+z*(990+z)))) /
                 (34459425+z*(16216200+z*(945945+z*(13860+z*45)))));
}
inline float midiHz(float note) noexcept { return 440.0f * std::exp2((note-69.0f)/12.0f); }
inline float wrap(float p) noexcept { return p-std::floor(p); }
inline float blep(float t, float dt) noexcept {
    if (t < dt) { t/=dt; return t+t-t*t-1; }
    if (t > 1-dt) { t=(t-1)/dt; return t*t+t+t+1; }
    return 0;
}
// The same odd-harmonic, Nyquist-limited triangle as the direct sine sum.
// A double-precision recurrence derives successive odd harmonics from two trig
// calls, rather than evaluating as many as eight sines for each oscillator.
inline float triangle(float phase,float dt) noexcept {
    if(dt>=.48f)return 0;
    if(3*dt>=.48f)return sine(phase)*.81f;
    const double angle=double(tau*phase),first=std::sin(angle);
    const double factor=2*std::cos(2*angle);
    double previous=-first,partial=first;float sum=0;
    if(15*dt<.48f) {
        // Low and middle notes use every partial. Expose the fixed denominators
        // and remove eight repeated Nyquist branches without changing the sum.
        const auto add=[&](int h) {
            sum+=float(partial)*((h%4==1)?1.f:-1.f)/float(h*h);
            const double next=factor*partial-previous;previous=partial;partial=next;
        };
        add(1);add(3);add(5);add(7);add(9);add(11);add(13);add(15);
        return sum*.81f;
    }
    for(int h=1;h<=15;h+=2) {
        if(float(h)*dt>=.48f)break;
        sum+=float(partial)*((h%4==1)?1.f:-1.f)/float(h*h);
        const double next=factor*partial-previous;
        previous=partial;partial=next;
    }
    return sum*.81f;
}
struct Random {
    std::uint32_t state = 0x1579ab23u;
    float next() noexcept { state^=state<<13; state^=state>>17; state^=state<<5; return float(state>>8)*(2.0f/16777216.0f)-1; }
};
struct Envelope {
    struct Rates {
        double attackStep=0,decayStep=0;
        float attack=-1,decay=-1,sustain=-1,sampleRate=-1;
        void configure(float a,float d,float s,float sr) noexcept {
            if(a==attack&&d==decay&&s==sustain&&sr==sampleRate)return;
            attackStep=1/std::max(1.0,double(a)*sr);
            decayStep=(1-double(s))/std::max(1.0,double(d)*sr);
            attack=a;decay=d;sustain=s;sampleRate=sr;
        }
    };
    enum Stage { Off, Attack, Decay, Sustain, Release } stage = Off;
    double value = 0, releaseStep = 0;
    void start(bool retrigger) noexcept { if (retrigger) value=0; stage=Attack; }
    void stop(float seconds, float sr) noexcept { if (stage!=Off&&stage!=Release) { stage=Release; releaseStep=value/std::max(1.0,double(seconds)*sr); } }
    float next(float a, float d, float s, float sr) noexcept {
        return nextSteps(stage==Attack?1/std::max(1.0,double(a)*sr):0,
                         stage==Decay?(1-double(s))/std::max(1.0,double(d)*sr):0,s);
    }
    float nextSteps(double attackStep,double decayStep,float s) noexcept {
        switch(stage) {
            case Off: value=0; break;
            case Attack: value+=attackStep; if(value>=1) {value=1;stage=Decay;} break;
            case Decay: value-=decayStep; if(value<=s) {value=s;stage=Sustain;} break;
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
    void advanceIncrement(double step) noexcept {phase+=step;if(phase>=1)phase-=std::floor(phase);}
    void advance(double hz,double sr) noexcept {advanceIncrement(hz/sr);}
    float value() const noexcept {return float(std::sin(2*std::numbers::pi_v<double>*phase));}
};
// Chord voices often have identical modulation phases. Reuse the exact sine
// result for matching inputs; each voice still advances its own full-rate clock.
struct LfoValueCache {
    double phase=-1;
    float value=0;
    float read(const LfoPhase& clock) noexcept {
        if(clock.phase!=phase){phase=clock.phase;value=clock.value();}
        return value;
    }
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
// Engine changes have a bounded cost: finish the current transition before
// beginning the latest requested choice. Selection is polled every sample, so
// an intervening request is coalesced without an allocation or a queue. A held
// request starts within one fade and settles within two fades (10 ms total).
// Unlike waveform/routing fades, no more than two engines run simultaneously.
struct EngineFade : ChoiceFade<8> {
    void advance(int choice,int samples) noexcept {
        ChoiceFade<8>::advance(remaining>0?target:choice,samples);
    }
};
// Topology preserving state variable filter, stable as cutoff approaches Nyquist.
struct SVF {
    struct Outputs {
        float low,high,band,notch;
        float select(int type) const noexcept {switch(type){case 1:return high;case 2:return band;case 3:return notch;default:return low;}}
    };
    float s1=0,s2=0,g=0.1f,k=1;
    float normalization=1/(1+g*(g+k));
    void copyTuningFrom(const SVF& other) noexcept {g=other.g;k=other.k;normalization=other.normalization;}
    void tuning(float newG,float newK) noexcept {
        g=newG;k=newK;normalization=1/(1+g*(g+k));
    }
    void coefficients(float hz,float resonance,float sr) noexcept {
        tuning(std::tan(pi*std::clamp(hz, 15.0f, sr*0.44f)/sr),2-1.95f*std::clamp(resonance,0.0f,0.95f));
    }
    Outputs process(float x) noexcept {
        const float v1=normalization*(s1+g*(x-s2)), v2=s2+g*v1;
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
