#include "SynthEngine.h"
#include <limits>
#if defined(__SSE2__) || defined(_M_X64)
#include <emmintrin.h>
#endif

namespace gba {
void SynthEngine::prepare(double rate) {
    sampleRate=static_cast<float>(std::clamp(rate,8000.0,192000.0));
    smoothing=1-std::exp(-1/(0.015f*sampleRate));
    choiceFadeSamples=std::max(1,int(std::round(sampleRate*.005f)));
    vibratoIncrement=5.3/double(sampleRate);tremoloIncrement=4.2/double(sampleRate);syllableIncrement=3.0/double(sampleRate);
    for(std::size_t i=0;i<harmonicInverseRoot.size();++i)harmonicInverseRoot[i]=1/std::sqrt(float(i+1));
    for(std::size_t i=0;i<voices.size();++i)voices[i].stringIndex=int(i);
    effects.prepare(sampleRate);if(!tablesReady){buildTables();tablesReady=true;}reset();prepared=true;
}
void SynthEngine::reset() noexcept {
    for(auto& v:voices) {v.note=-1;v.amp={};v.filterEnv={};v.filter.clear();v.tail=0;v.last=0;v.held=false;v.preview=false;v.filterReady=v.formantsReady=v.vocalReady=false;}
    bend.fill(0);wheel.fill(0);sustain.fill(false);keys={};clock=0;previewSamples=0;lastFrequency=0;
    current=target;effects.reset();lastMode=int(target[Param::Mode]);lastVoiceCount=int(target[Param::Voices]);
    movingCount=0;parameterMoving.fill(false);
    wave1Fade.reset(int(target[Param::Wave1]));wave2Fade.reset(int(target[Param::Wave2]));filterFade.reset(int(target[Param::FilterType]));
    engineFade.reset(int(target[Param::Engine]));
    lfo1Route.reset(int(target[Param::Lfo1Dest]));lfo2Route.reset(int(target[Param::Lfo2Dest]));
    cachedGlide=-1;cachedOscillatorInterval=1000;cachedBrightness=-1;
    scalarCache={};frameCaches={};cachedFormantVowel=cachedVocalVowel=-100;
}
void SynthEngine::setSettings(const Settings& s) noexcept {
    for(std::size_t p=0;p<parameterCount;++p) {
        if(specs[p].step>0||p>=index(Param::MacroA))current.values[p]=s.values[p];
        else if(s.values[p]!=target.values[p]&&!parameterMoving[p]) {
            movingParameters[movingCount++]=p;parameterMoving[p]=true;
        }
    }
    target=s;
    const int mode=int(s[Param::Mode]),count=int(s[Param::Voices]);
    if(mode!=lastMode || count!=lastVoiceCount) {
        // Preserve release tails, but clear held-key bookkeeping when topology changes.
        allNotesOff();lastMode=mode;lastVoiceCount=count;
    }
}
void SynthEngine::buildTables() noexcept {
    for(int shape=0;shape<4;++shape) for(int mip=0;mip<8;++mip) {
        int harmonics=128>>mip;
        for(int i=0;i<1024;++i) {
            float p=float(i)/1024, sum=0;
            for(int h=1;h<=harmonics;++h) {
                float amplitude=0;
                if(shape==0) amplitude=1.0f/float(h);
                if(shape==1) amplitude=(h%2)?1.0f/float(h):0;
                if(shape==2) amplitude=std::exp(-0.07f*float(h))*std::cos(float(h)*1.7f)/std::sqrt(float(h));
                if(shape==3) amplitude=std::exp(-0.5f*std::pow((float(h)-5)/2.3f,2.0f))/float(h);
                sum+=sine(p*float(h))*amplitude;
            }
            tables[shape][mip][i]=sum*(shape==2?0.45f:0.6f);
        }
    }
}
inline float SynthEngine::wavetable(float p,float frequency,float position,TableTuning& tuning) const noexcept {
    // Fade each level out one octave before its highest partial reaches 0.45 * sample rate.
    // Both contributing levels therefore stay below Nyquist, including between mip boundaries.
    if(tuning.frequency!=frequency||tuning.rate!=sampleRate) {
        const float mip=std::clamp(std::log2(std::max(1.0f,frequency*256/(sampleRate*.45f))),0.0f,7.0f);
        tuning.level=int(mip);tuning.next=std::min(tuning.level+1,7);tuning.blend=mip-float(tuning.level);
        tuning.frequency=frequency;tuning.rate=sampleRate;
    }
    const int m=tuning.level,m2=tuning.next,a=std::clamp(int(position),0,3),b=std::min(a+1,3);
    float t=p*1024;int i=int(t)&1023,j=(i+1)&1023;float f=t-std::floor(t);
    const auto read=[&](int shape,int level) {const auto& table=tables[shape][level];return table[i]+f*(table[j]-table[i]);};
    // Both blends are bounded to [0,1]; a direct interpolation avoids
    // the extra sign/range branches in the generic std::lerp implementation.
    const auto linear=[](float x,float y,float blend){return x+(y-x)*blend;};
    float x=linear(read(a,m),read(a,m2),tuning.blend),y=linear(read(b,m),read(b,m2),tuning.blend);
    return linear(x,y,position-float(a));
}
void SynthEngine::start(Voice& v,int channel,int note,float velocity,bool preview,bool legato) noexcept {
    const bool active=v.amp.stage!=Envelope::Off;
    v.tail=active?v.last:0;v.tailStep=v.tail/64;
    v.channel=channel;v.note=note;v.held=true;v.preview=preview;v.age=++clock;v.velocity=velocity;v.targetFrequency=midiHz(float(note));
    v.velocityRoot=std::sqrt(velocity);
    const float pan=std::clamp(0.5f+0.16f*float((note%5)-2),0.0f,1.0f);
    v.panLeft=std::sqrt(1-pan);v.panRight=std::sqrt(pan);
    if(!legato) {
        v.frequency=(target[Param::Glide]>0 && lastFrequency>0)?lastFrequency:v.targetFrequency;
        v.amp.start(true);v.filterEnv.start(true);v.p1=0;v.p2=0;v.fmPhase=0;v.subPhase=0;v.modalPhase={};v.modalElapsed=0;v.stringActive=false;v.modalActive=false;
        v.lfo1={};v.lfo2={};v.vibrato={};v.tremolo={};v.syllable={};v.panMod=0;
        v.filter.clear();for(auto& f:v.formants) f.clear();v.vocal.reset();v.retro.reset();v.controlCounter=0;
    } else {v.frequency=std::max(v.frequency,1.0f);v.tail=0;}
    lastFrequency=v.targetFrequency;
}
void SynthEngine::release(Voice& v) noexcept {
    v.held=false;v.amp.stop(target[Param::Release],sampleRate);v.filterEnv.stop(target[Param::FilterRelease],sampleRate);
}
void SynthEngine::noteOn(int channel,int note,float velocity,bool preview) noexcept {
    if(channel<1 || channel>16 || note<0 || note>127) return;
    if(velocity<=0) {noteOff(channel,note);return;}
    velocity=std::clamp(velocity,0.0f,1.0f);
    if(!preview) {auto& key=keys[channel-1][note];key.down=true;key.velocity=velocity;key.order=++clock;key.presses=std::min(key.presses+1,128u);}
    int mode=int(target[Param::Mode]),count=int(target[Param::Voices]);
    if(mode!=0 && !preview) {bool legato=mode==2&&voices[0].held&&voices[0].amp.stage!=Envelope::Off;start(voices[0],channel,note,velocity,false,legato);return;}
    Voice* choice=nullptr;
    for(int i=0;i<count;++i) if(voices[i].amp.stage==Envelope::Off) {choice=&voices[i];break;}
    if(!choice) {
        choice=&voices[0];
        for(int i=1;i<count;++i) {
            bool releasing=voices[i].amp.stage==Envelope::Release;
            bool oldReleasing=choice->amp.stage==Envelope::Release;
            if((releasing&&!oldReleasing) || (releasing==oldReleasing&&voices[i].age<choice->age)) choice=&voices[i];
        }
    }
    start(*choice,channel,note,velocity,preview,false);
    if(preview) previewSamples=int(sampleRate*0.65f);
}
void SynthEngine::noteOff(int channel,int note) noexcept {
    if(channel<1||channel>16||note<0||note>127) return;
    auto& key=keys[channel-1][note];
    const auto outstanding=key.presses;
    if(outstanding==0)return;
    --key.presses;key.down=key.presses>0;
    unsigned heldMatching=0;
    for(const auto& v:voices)if(v.channel==channel&&v.note==note&&!v.preview&&v.held)++heldMatching;
    // MIDI pairs repeated note-ons with note-offs in onset order. A stolen onset
    // still needs its note-off, but that event must not release a newer voice.
    if(outstanding>heldMatching)return;
    if(int(target[Param::Mode])!=0 && voices[0].note==note && voices[0].channel==channel) {
        std::uint64_t newest=0;int next=-1,nextChannel=channel;
        for(int c=0;c<16;++c) for(int n=0;n<128;++n) if(keys[c][n].down&&keys[c][n].order>newest) {newest=keys[c][n].order;next=n;nextChannel=c+1;}
        if(next>=0) {start(voices[0],nextChannel,next,keys[nextChannel-1][next].velocity,false,int(target[Param::Mode])==2);return;}
    }
    Voice* released=nullptr;
    for(auto& v:voices) if(v.channel==channel&&v.note==note&&!v.preview&&v.held) {if(!released||v.age<released->age)released=&v;}
    if(released) {released->held=false;if(!sustain[channel-1])release(*released);}
}
void SynthEngine::controller(int channel,int number,int value) noexcept {
    if(channel<1||channel>16) return;
    value=std::clamp(value,0,127);auto c=std::size_t(channel-1);
    if(number==1) wheel[c]=float(value)/127;
    if(number==64) {
        sustain[c]=value>=64;
        if(!sustain[c]) for(auto& v:voices) if(v.channel==channel&&!v.held) release(v);
    }
    if(number==120||number==123) {
        keys[c]={};sustain[c]=false;
        for(auto& v:voices) if(v.channel==channel) {if(number==120) {v.amp={};v.note=-1;v.tail=0;} else release(v);}
    }
    if(number==121) {wheel[c]=0;bend[c]=0;controller(channel,64,0);}
}
void SynthEngine::pitchWheel(int channel,int value) noexcept {
    if(channel<1||channel>16) return;
    value=std::clamp(value,0,16383);bend[channel-1]=float(value-8192)/(value>=8192?8191.0f:8192.0f);
}
void SynthEngine::allNotesOff(bool immediate) noexcept {
    for(auto& v:voices) {if(immediate) {v.amp={};v.filterEnv={};v.note=-1;v.tail=0;} else release(v);}
    keys={};sustain.fill(false);previewSamples=0;
}
void SynthEngine::stopPreview() noexcept {for(auto& v:voices) if(v.preview) release(v);previewSamples=0;}
int SynthEngine::activeVoices() const noexcept {int n=0;for(const auto& v:voices) if(v.amp.stage!=Envelope::Off) ++n;return n;}
float SynthEngine::oscillator(int wave,float p,float dt,float width,Random& random) noexcept {
    switch(wave) {
        case 0:return sine(p);
        case 1:return triangle(p,dt);
        case 2:return 2*p-1-blep(p,dt);
        case 3:width=0.5f;[[fallthrough]];
        case 4:return (p<width?1.0f:-1.0f)+blep(p,dt)-blep(wrap(p+1-width),dt);
        default:return random.next();
    }
}
float SynthEngine::mixedOscillator(const ChoiceFade<6>& fade,float p,float dt,float width,Random& random) noexcept {
    if(fade.remaining==0)return oscillator(fade.target,p,dt,width,random);
    float sum=0;
    for(std::size_t i=0;i<fade.weights.size();++i)if(fade.weights[i]>0)sum+=fade.weights[i]*oscillator(int(i),p,dt,width,random);
    return sum;
}
void SynthEngine::updateFormants(Voice& v,float vowel) noexcept {
    constexpr float vowels[5][3] {{800,1150,2900},{400,1700,2600},{300,2200,3000},{500,800,2600},{350,600,2400}};
    vowel=std::clamp(vowel,0.0f,4.0f);int a=int(vowel),b=std::min(a+1,4);float t=vowel-float(a);
    const float shift=current[Param::FormantShift];
    if(!v.formantsReady||v.formantVowel!=vowel||v.formantShift!=shift) {
        if(cachedFormantVowel!=vowel||cachedFormantShift!=shift) {
            const float scale=std::exp2(shift/12);
            for(int i=0;i<3;++i)formantTuning[i].coefficients(std::lerp(vowels[a][i],vowels[b][i],t)*scale,0.87f,sampleRate);
            cachedFormantVowel=vowel;cachedFormantShift=shift;
        }
        for(int i=0;i<3;++i)v.formants[i].copyTuningFrom(formantTuning[i]);
        v.formantVowel=vowel;v.formantShift=shift;v.formantsReady=true;
    }
    if(engineFade.weights[5]>0&&(!v.vocalReady||v.vocalVowel!=vowel||v.vocalShift!=shift||v.vocalBrightness!=current[Param::Brightness])) {
        if(cachedVocalVowel!=vowel||cachedVocalShift!=shift||cachedVocalBrightness!=current[Param::Brightness]) {
            vocalTuning.configure(vowel,shift,current[Param::Brightness],sampleRate);
            cachedVocalVowel=vowel;cachedVocalShift=shift;cachedVocalBrightness=current[Param::Brightness];
        }
        v.vocal.copyTuningFrom(vocalTuning);
        v.vocalVowel=vowel;v.vocalShift=shift;v.vocalBrightness=current[Param::Brightness];v.vocalReady=true;
    }
}
void SynthEngine::exciteString(Voice& v) noexcept {
    auto& string=strings[std::size_t(v.stringIndex)];
    const int length=std::min(stringSize-2,int(std::ceil(v.stringDelay))+1);
    string.fill(0);v.stringPos=0;v.stringLow=0;
    float previous=0,mean=0;
    for(int i=stringSize-length;i<stringSize;++i) {
        const float x=v.noise.next();string[i]=.5f*(x+previous);previous=x;mean+=string[i];
    }
    mean/=float(length);
    for(int i=stringSize-length;i<stringSize;++i)string[i]-=mean;
}
inline float SynthEngine::excitation(Voice& v,int engine,float frequency,float dt,float dt2,float fenv) noexcept {
    const auto& s=current;float x=0;
    switch(engine) {
        case 0:x=std::lerp(mixedOscillator(wave1Fade,v.p1,dt,s[Param::PulseWidth],v.noise),mixedOscillator(wave2Fade,v.p2,dt2,s[Param::PulseWidth],v.noise),s[Param::OscMix]);break;
        case 1:{float ratio=s[Param::FmRatio],index=s[Param::FmIndex]*(0.15f+0.85f*fenv);float limit=std::max(0.0f,(sampleRate*0.45f/frequency-1)/std::max(1.0f,ratio));index=std::min(index,limit);x=std::sin(tau*v.p1+sine(v.fmPhase)*index);x=std::lerp(x,sine(v.subPhase),s[Param::OscMix]*0.4f);break;}
        case 2:{
            // sin((n+1)t) = 2 cos(t) sin(nt) - sin((n-1)t).
            // Double precision keeps the recurrence stable near the phase boundaries.
            const double phase=double(v.p1)*2*std::numbers::pi_v<double>;
            const double factor=2*std::cos(phase);
            double partial=std::sin(phase),sum=0;
#if !defined(__SSE2__) && !defined(_M_X64)
            double previous=0;
#endif
            int used=int(s[Param::Harmonics]);
            // Most pad/string notes keep the entire bank below Nyquist. Check
            // its highest partial once; retain the exact original cutoff test
            // when a high note actually needs fewer harmonics.
            if(float(used)*dt>=0.45f){used=0;while(used<int(s[Param::Harmonics])&&float(used+1)*dt<0.45f)++used;}
#if defined(__SSE2__) || defined(_M_X64)
            // Four interleaved harmonic recurrences share two SSE2 double lanes.
            // The scalar path below retains portability; both retain every partial.
            const double sn=partial,cs=factor*.5,sn2=2*sn*cs,sn3=2*cs*sn2-sn,sn4=2*cs*sn3-sn2;
            const double cs2=2*cs*cs-1;
            const __m128d step=_mm_set1_pd(2*(2*cs2*cs2-1));
            __m128d a=_mm_set_pd(sn2,sn),b=_mm_set_pd(sn4,sn3);
            __m128d pa=_mm_set_pd(-sn2,-sn3),pb=_mm_set_pd(0,-sn),total=_mm_setzero_pd();
            int h=0;
            for(;h+3<used;h+=4) {
                total=_mm_add_pd(total,_mm_add_pd(_mm_mul_pd(a,_mm_set_pd(harmonicWeight[std::size_t(h+1)],harmonicWeight[std::size_t(h)])),_mm_mul_pd(b,_mm_set_pd(harmonicWeight[std::size_t(h+3)],harmonicWeight[std::size_t(h+2)]))));
                const auto na=_mm_sub_pd(_mm_mul_pd(step,a),pa),nb=_mm_sub_pd(_mm_mul_pd(step,b),pb);pa=a;pb=b;a=na;b=nb;
            }
            double lanes[2];_mm_storeu_pd(lanes,total);sum=lanes[0]+lanes[1];
            double rest[4];_mm_storeu_pd(rest,a);_mm_storeu_pd(rest+2,b);
            for(int i=0;h<used;++h,++i)sum+=rest[i]*harmonicWeight[std::size_t(h)];
#else
            for(int h=0;h<used;++h) {
                sum+=partial*harmonicWeight[std::size_t(h)];
                const double next=factor*partial-previous;
                previous=partial;partial=next;
            }
#endif
            x=used>0?float(sum)*harmonicScale[std::size_t(used-1)]:0;
            break;
        }
        case 3:x=std::lerp(wavetable(v.p1,frequency,s[Param::Wavetable],v.table1),wavetable(v.p2,dt2*sampleRate,s[Param::Wavetable],v.table2),s[Param::OscMix]);break;
        case 4:{
            if(!v.stringActive){exciteString(v);v.stringActive=true;}
            auto& string=strings[std::size_t(v.stringIndex)];
            float read=float(v.stringPos)-v.stringDelay;
            if(read<0)read+=stringSize;
            const int a=int(read),b=(a+1)%stringSize;
            const float delayed=std::lerp(string[a],string[b],read-float(a));
            v.stringLow+=(.15f+.8f*s[Param::Brightness])*(delayed-v.stringLow);
            string[v.stringPos]=.5f*(delayed+v.stringLow)*s[Param::StringDamping];
            v.stringPos=(v.stringPos+1)%stringSize;x=delayed*2.2f;
            break;
        }
        case 5:x=v.vocal.next(v.p1,v.p2,dt,dt2,s[Param::Choir],s[Param::Breath],v.noise);break;
        case 6:x=v.retro.next(wave1Fade,wave2Fade,v.p1,v.p2,dt,dt2,s[Param::PulseWidth],s[Param::OscMix]);break;
        case 7:{ // Original modal bar / tine synthesis with inharmonic resonances.
            if(!v.modalActive){v.modalPhase={};v.modalElapsed=0;v.modalActive=true;}
            v.modalElapsed+=1/double(sampleRate);
            constexpr float ratios[]{1,2.01f,2.76f,4.07f,5.43f,6.79f};
            // exp(-t*(i+1)/decayTime) is a power of exp(-t/decayTime). Compute
            // one exponential and multiply successive partial decays in double
            // precision, retaining the original response to decay automation.
            const double fundamentalDecay=std::exp(-v.modalElapsed/(0.3+s[Param::Decay]));
            double partialDecay=fundamentalDecay;
            for(int i=0;i<6;++i) {
                float ratio=std::lerp(float(i+1),ratios[i],s[Param::Brightness]);
                if(ratio*dt>=0.45f)break;
                v.modalPhase[i]=wrap(v.modalPhase[i]+dt*ratio);
                x+=sine(v.modalPhase[i])*float(partialDecay)/float(i+1);
                partialDecay*=fundamentalDecay;
            }
            break;}
        default:break;
    }
    return x;
}
template<int Engine> float SynthEngine::voiceSample(Voice& v,FrameCache& cache) noexcept {
    const auto& s=current;
    v.lfo1.advanceIncrement(lfo1Increment);v.lfo2.advanceIncrement(lfo2Increment);
    v.vibrato.advanceIncrement(vibratoIncrement);v.tremolo.advanceIncrement(tremoloIncrement);v.syllable.advanceIncrement(syllableIncrement);
    // Keep both clocks advancing, but zero depth needs no sine evaluation.
    const float lfo1=s[Param::Lfo1Depth]>0?cache.lfo1.read(v.lfo1):0;
    const float lfo2=s[Param::Lfo2Depth]>0?cache.lfo2.read(v.lfo2):0;
    float modPitch=0,modFilter=0,modAmp=0,modVowel=0;
    const auto route=[&](int destination,float amount) {
        switch(destination) {case 1:modPitch+=amount*2;break;case 2:modFilter+=amount*3;break;case 3:modAmp+=amount*0.45f;break;case 4:modVowel+=amount*2;break;default:break;}
    };
    const auto applyRoute=[&](const ChoiceFade<6>& fade,float amount) {
        if constexpr(Engine>=0)route(fade.target,amount);
        else {
            if(fade.remaining==0)route(fade.target,amount);
            else for(int destination=1;destination<5;++destination)route(destination,amount*fade.weights[std::size_t(destination)]);
        }
    };
    applyRoute(lfo1Route,lfo1*s[Param::Lfo1Depth]);applyRoute(lfo2Route,lfo2*s[Param::Lfo2Depth]);
    v.panMod=lfo1*s[Param::Lfo1Depth]*.45f*lfo1Route.weights[5]+lfo2*s[Param::Lfo2Depth]*.45f*lfo2Route.weights[5];
    const float vibrato=s[Param::Vibrato]+wheel[v.channel-1]*0.6f;
    if(vibrato>0)modPitch+=cache.vibrato.read(v.vibrato)*vibrato*0.6f;
    v.frequency+=(v.targetFrequency-v.frequency)*glideStep;
    const float pitch=bend[v.channel-1]*s[Param::BendRange]+modPitch;
    // Matching chord voices share bend/vibrato pitch offsets. Cache only the
    // exact offset's exponential; each voice retains its own glide and frequency.
    if(pitch!=0&&pitch!=cache.pitch){cache.pitchRatio=std::exp2(pitch/12);cache.pitch=pitch;}
    float frequency=std::clamp(pitch==0?v.frequency:v.frequency*cache.pitchRatio,8.0f,sampleRate*0.4f);
    v.fmPhase=wrap(v.fmPhase+frequency*current[Param::FmRatio]/sampleRate);v.subPhase=wrap(v.subPhase+frequency*0.5f/sampleRate);
    float dt=frequency/sampleRate,dt2=std::min(0.45f,dt*oscillator2Ratio);
    v.p1=wrap(v.p1+dt);v.p2=wrap(v.p2+dt2);
    float env=v.amp.nextSteps(ampRates.attackStep,ampRates.decayStep,s[Param::Sustain]);
    float fenv=v.filterEnv.nextSteps(filterRates.attackStep,filterRates.decayStep,s[Param::FilterSustain]);
    const float syllableMotion=s[Param::Syllable]>0?cache.syllable.read(v.syllable):0;
    if(v.controlCounter==0||((Engine==4||(Engine<0&&engineFade.weights[4]>0))&&!v.stringActive)) {
        const float octaves=fenv*s[Param::FilterEnv]+modFilter;
        if(cache.filterBase!=s[Param::Cutoff]||cache.filterOctaves!=octaves) {
            cache.cutoff=std::clamp(s[Param::Cutoff]*std::exp2(octaves),15.0f,sampleRate*.44f);
            cache.filterBase=s[Param::Cutoff];cache.filterOctaves=octaves;
        }
        const float cutoff=cache.cutoff;
        if(!v.filterReady||v.filterCutoff!=cutoff||v.filterResonance!=s[Param::Resonance]) {
            if(cache.filterHz!=cutoff||cache.filterResonance!=s[Param::Resonance]) {
                cache.filterTuning.coefficients(cutoff,s[Param::Resonance],sampleRate);
                cache.filterHz=cutoff;cache.filterResonance=s[Param::Resonance];
            }
            v.filter.copyTuningFrom(cache.filterTuning);
            v.filterCutoff=cutoff;v.filterResonance=s[Param::Resonance];v.filterReady=true;
        }
        updateFormants(v,s[Param::Vowel]+modVowel+s[Param::Syllable]*syllableMotion*0.8f);
        if constexpr(Engine<0||Engine==4||Engine==6) {
          if(Engine==6||(Engine<0&&engineFade.weights[6]>0))v.retro.configure(s[Param::Brightness],sampleRate);
          if((Engine==4||(Engine<0&&engineFade.weights[4]>0))&&(v.stringFrequency!=frequency||v.stringBrightness!=s[Param::Brightness]||v.stringRate!=sampleRate)) {
            // Compensate the feedback low-pass phase delay so brightness does not detune the string.
            const float omega=tau*frequency/sampleRate,alpha=.15f+.8f*s[Param::Brightness],r=1-alpha;
            const float c=std::cos(omega),sn=std::sin(omega),den=1-2*r*c+r*r;
            const float real=.5f*(1+alpha*(1-r*c)/den),imag=-.5f*alpha*r*sn/den;
            const float lag=std::atan2(-imag,real)/omega;
            v.stringDelay=std::clamp(sampleRate/frequency-lag,2.0f,float(stringSize-2));
            v.stringFrequency=frequency;v.stringBrightness=s[Param::Brightness];v.stringRate=sampleRate;
          }
        }
    }
    v.controlCounter=(v.controlCounter+1)&15;
    float x=0;
    if constexpr(Engine>=0) {
        if constexpr(Engine!=4)v.stringActive=false;
        if constexpr(Engine!=7)v.modalActive=false;
        x=excitation(v,Engine,frequency,dt,dt2,fenv);
    } else {
        if(engineFade.weights[4]==0)v.stringActive=false;
        if(engineFade.weights[7]==0)v.modalActive=false;
        if(engineFade.remaining==0)x=excitation(v,engineFade.target,frequency,dt,dt2,fenv);
        else for(int engine=0;engine<8;++engine)if(engineFade.weights[std::size_t(engine)]>0)
            x+=engineFade.weights[std::size_t(engine)]*excitation(v,engine,frequency,dt,dt2,fenv);
    }
    const auto filtered=v.filter.process(x);
    float formant=0;
    if(filterFade.target==4||filterFade.weights[4]>0) {for(auto& f:v.formants)formant+=f.next(x,2)*0.22f;}
    if(filterFade.remaining==0)x=filterFade.target==4?formant:filtered.select(filterFade.target);
    else x=filtered.low*filterFade.weights[0]+filtered.high*filterFade.weights[1]+filtered.band*filterFade.weights[2]+filtered.notch*filterFade.weights[3]+formant*filterFade.weights[4];
    float syllable=1-s[Param::Syllable]*0.6f*(0.5f+0.5f*syllableMotion);
    float velocity=std::lerp(1.0f,v.velocityRoot,s[Param::Velocity]);
    const float tremolo=s[Param::Tremolo]>0?s[Param::Tremolo]*(0.5f+0.5f*cache.tremolo.read(v.tremolo)):0;
    x*=env*velocity*std::max(0.0f,1+modAmp-tremolo)*syllable*0.16f;
    if(std::abs(v.tail)>std::abs(v.tailStep)) {x+=v.tail;v.tail-=v.tailStep;} else v.tail=0;
    v.last=x;return x;
}
void SynthEngine::updateSharedControls() noexcept {
    // Shared by every voice, including continuously smoothed automation.
    const float glide=current[Param::Glide];
    if(glide!=cachedGlide) {glideStep=glide<0.001f?1.0f:1-std::exp(-1/(glide*sampleRate));cachedGlide=glide;}
    const float interval=(current[Param::Coarse]+current[Param::Detune]/100)/12;
    if(interval!=cachedOscillatorInterval) {oscillator2Ratio=std::exp2(interval);cachedOscillatorInterval=interval;}
    ampRates.configure(current[Param::Attack],current[Param::Decay],current[Param::Sustain],sampleRate);
    filterRates.configure(current[Param::FilterAttack],current[Param::FilterDecay],current[Param::FilterSustain],sampleRate);
    lfo1Increment=double(current[Param::Lfo1Rate])/double(sampleRate);lfo2Increment=double(current[Param::Lfo2Rate])/double(sampleRate);
}
void SynthEngine::updateHarmonics() noexcept {
    if(engineFade.weights[2]>0&&current[Param::Brightness]!=cachedBrightness) {
        // Brightness is shared by all voices. Compute harmonic weights and their
        // prefix normalization once per sample, preserving continuous smoothing.
        const float falloff=std::exp(-(0.08f+(1-current[Param::Brightness])*0.7f));
        float amplitude=1,energy=0;
        for(std::size_t h=0;h<harmonicWeight.size();++h) {
            harmonicWeight[h]=amplitude*harmonicInverseRoot[h];
            energy+=harmonicWeight[h]*harmonicWeight[h];
            harmonicScale[h]=1/std::sqrt(std::max(1.0f,energy));
            amplitude*=falloff;
        }
        cachedBrightness=current[Param::Brightness];
    }
}
bool SynthEngine::canRenderStable() const noexcept {
    const int engine=int(current[Param::Engine]);
    return movingCount==0&&previewSamples==0
        &&engineFade.remaining==0&&engineFade.target==engine
        &&wave1Fade.remaining==0&&wave1Fade.target==int(current[Param::Wave1])
        &&wave2Fade.remaining==0&&wave2Fade.target==int(current[Param::Wave2])
        &&filterFade.remaining==0&&filterFade.target==int(current[Param::FilterType])
        &&lfo1Route.remaining==0&&lfo1Route.target==int(current[Param::Lfo1Dest])
        &&lfo2Route.remaining==0&&lfo2Route.target==int(current[Param::Lfo2Dest]);
}
void SynthEngine::renderStable(float* left,float* right,int samples) noexcept {
    switch(int(current[Param::Engine])) {
        case 0:renderStableEngine<0>(left,right,samples);break;
        case 1:renderStableEngine<1>(left,right,samples);break;
        case 2:renderStableEngine<2>(left,right,samples);break;
        case 3:renderStableEngine<3>(left,right,samples);break;
        case 4:renderStableEngine<4>(left,right,samples);break;
        case 5:renderStableEngine<5>(left,right,samples);break;
        case 6:renderStableEngine<6>(left,right,samples);break;
        default:renderStableEngine<7>(left,right,samples);break;
    }
}
template<int Engine> void SynthEngine::renderStableEngine(float* left,float* right,int samples) noexcept {
    updateSharedControls();updateHarmonics();
    // Process a short run of each voice while its state is hot in cache. The
    // per-frame exact-input caches retain the original chord sharing of LFO,
    // bend and filter math. Mixing still visits voices in their original order.
    for(int at=0;at<samples;at+=int(frameCaches.size())) {
        const int count=std::min(int(frameCaches.size()),samples-at);
        std::array<float,32> l{},r{};
        for(auto& v:voices)for(int n=0;n<count&&v.amp.stage!=Envelope::Off;++n) {
            const float x=voiceSample<Engine>(v,frameCaches[std::size_t(n)]);
            if(v.panMod==0){l[std::size_t(n)]+=x*v.panLeft;r[std::size_t(n)]+=x*v.panRight;}
            else {
                float pan=0.5f+0.16f*float((v.note%5)-2);
                pan=std::clamp(pan+v.panMod,0.0f,1.0f);l[std::size_t(n)]+=x*std::sqrt(1-pan);r[std::size_t(n)]+=x*std::sqrt(pan);
            }
        }
        for(int n=0;n<count;++n) {
            effects.process(l[std::size_t(n)],r[std::size_t(n)],current);
            left[at+n]=softLimit(l[std::size_t(n)])*current[Param::Volume];
            right[at+n]=softLimit(r[std::size_t(n)])*current[Param::Volume];
        }
    }
}
void SynthEngine::render(float* left,float* right,int samples) noexcept {
    if(!prepared) {std::fill_n(left,samples,0.0f);std::fill_n(right,samples,0.0f);return;}
    if(canRenderStable()){renderStable(left,right,samples);return;}
    // MIDI starts voices between render segments. Collect the current voices
    // once per segment, in the original summation order, using fixed stack storage.
    std::array<Voice*,maxVoices> sounding;
    int soundingCount=0;
    for(auto& v:voices)if(v.amp.stage!=Envelope::Off)sounding[std::size_t(soundingCount++)]=&v;
    for(int i=0;i<samples;++i) {
        for(std::size_t item=0;item<movingCount;) {
            const auto p=movingParameters[item];
            const float before=current.values[p];
            current.values[p]+=(target.values[p]-current.values[p])*smoothing;
            // Stop only at the identical floating-point fixed point the original
            // recurrence reaches. No epsilon, snapping or shorter smoothing time.
            if(current.values[p]==before) {
                parameterMoving[p]=false;movingParameters[item]=movingParameters[--movingCount];
            } else ++item;
        }
        updateSharedControls();
        wave1Fade.advance(int(current[Param::Wave1]),choiceFadeSamples);wave2Fade.advance(int(current[Param::Wave2]),choiceFadeSamples);filterFade.advance(int(current[Param::FilterType]),choiceFadeSamples);
        lfo1Route.advance(int(current[Param::Lfo1Dest]),choiceFadeSamples);lfo2Route.advance(int(current[Param::Lfo2Dest]),choiceFadeSamples);
        engineFade.advance(int(current[Param::Engine]),choiceFadeSamples);
        updateHarmonics();
        if(previewSamples>0 && --previewSamples==0) stopPreview();
        float l=0,r=0;
        for(int voice=0;voice<soundingCount;++voice)if(auto& v=*sounding[std::size_t(voice)];v.amp.stage!=Envelope::Off) {
            float x=voiceSample<-1>(v,scalarCache);
            if(v.panMod==0) {l+=x*v.panLeft;r+=x*v.panRight;}
            else {
                float pan=0.5f+0.16f*float((v.note%5)-2);
                pan=std::clamp(pan+v.panMod,0.0f,1.0f);l+=x*std::sqrt(1-pan);r+=x*std::sqrt(pan);
            }
        }
        effects.process(l,r,current);
        // Bounded soft limiting protects the host from extreme patch combinations.
        left[i]=softLimit(l)*current[Param::Volume];right[i]=softLimit(r)*current[Param::Volume];
    }
}
}
