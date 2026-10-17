#include "SynthEngine.h"
#include <limits>

namespace gba {
float Effects::Line::read(float d) const noexcept {
    float p=float(pos)-std::clamp(d,1.0f,float(data.size()-2));
    if(p<0) p+=float(data.size());
    auto i=static_cast<std::size_t>(p); float f=p-float(i);
    return data[i]+f*(data[(i+1)%data.size()]-data[i]);
}
void Effects::Line::push(float x) noexcept {data[pos]=x;pos=(pos+1)%data.size();}
void Effects::prepare(float rate) {
    sr=rate;
    for(auto* lines:{&delay,&mod,&tape}) for(auto& l:*lines) l.data.assign(static_cast<std::size_t>(sr*1.6f)+16,0);
    const float lengths[]{0.0297f,0.0371f,0.0411f,0.0437f};
    for(int i=0;i<4;++i) room[i].data.assign(static_cast<std::size_t>(sr*lengths[i])+3,0);
    reset();
}
void Effects::reset() noexcept {
    for(auto* lines:{&delay,&mod,&tape}) for(auto& l:*lines) {std::fill(l.data.begin(),l.data.end(),0);l.pos=0;l.damp=0;}
    for(auto& l:room) {std::fill(l.data.begin(),l.data.end(),0);l.pos=0;l.damp=0;}
    allpass={};phase=0;counter=0;holdL=holdR=lowL=lowR=highL=highR=prevL=prevR=0;
}
void Effects::process(float& l,float& r,const Settings& s) noexcept {
    phase=wrap(phase+0.27f/sr);
    float* channels[]{&l,&r};
    for(int c=0;c<2;++c) {
        float x=*channels[c];
        // Two evaluations per sample with interpolation soften aliasing in the nonlinear stages.
        const float amount=1+10*s[Param::Drive]+22*s[Param::Distortion];
        float& previous=c==0?prevL:prevR;
        const auto shape=[&](float v) {return std::tanh(v*amount)/(1+0.2f*(amount-1));};
        const float shaped=0.5f*(shape((x+previous)*0.5f)+shape(x)); previous=x;
        x+=(shaped-x)*std::clamp((s[Param::Drive]+s[Param::Distortion])*2,0.0f,1.0f);
        if(counter==0) {
            const float steps=std::exp2(s[Param::CrusherBits]-1);
            (c==0?holdL:holdR)=std::round(x*steps)/steps;
        }
        if(s[Param::CrusherBits]<16 || s[Param::CrusherRate]>1) x=c==0?holdL:holdR;
        const float wobble=s[Param::Wow]*(0.0015f*sine(phase*1.37f)+0.00025f*sine(phase*29.0f));
        tape[c].push(x);
        if(s[Param::Wow]>0.0001f) x=tape[c].read(sr*(0.004f+wobble));
        mod[c].push(x);
        const float chorusDelay=sr*(0.018f+0.006f*sine(phase+0.25f*float(c)));
        x+=s[Param::Chorus]*0.45f*(mod[c].read(chorusDelay)-x);
        x+=s[Param::Flanger]*0.45f*mod[c].read(sr*(0.0025f+0.002f*sine(phase*0.71f+0.4f*float(c))));
        float ap=x;
        const float a=0.12f+0.72f*(0.5f+0.5f*sine(phase*0.61f+0.1f*float(c)));
        for(auto& state:allpass[c]) {const float y=-a*ap+state; state=ap+a*y;ap=y;}
        x+=s[Param::Phaser]*0.48f*ap;
        const float echo=delay[c].read(sr*s[Param::DelayTime]);
        delay[c].push(x+echo*s[Param::DelayFeedback]); x+=echo*s[Param::Delay];
        float& lo=c==0?lowL:lowR; float& hi=c==0?highL:highR;
        lo+=(1-std::exp(-tau*180/sr))*(x-lo);hi+=(1-std::exp(-tau*3800/sr))*(x-hi);
        x+=lo*(std::pow(10.0f,s[Param::EqLow]/20)-1)+(x-hi)*(std::pow(10.0f,s[Param::EqHigh]/20)-1);
        *channels[c]=x;
    }
    counter=(counter+1)%std::max(1,int(s[Param::CrusherRate]));
    // Four-line orthogonal feedback delay network with damped feedback.
    std::array<float,4> taps{};
    for(int i=0;i<4;++i) {auto& line=room[i];float v=line.data[line.pos];line.damp+=0.35f*(v-line.damp);taps[i]=line.damp;}
    float total=taps[0]+taps[1]+taps[2]+taps[3];
    for(int i=0;i<4;++i) room[i].push((i%2==0?l:r)*0.22f+(0.5f*total-taps[i])*(0.5f+0.46f*s[Param::ReverbSize]));
    l+=(taps[0]+taps[2])*s[Param::Reverb];r+=(taps[1]+taps[3])*s[Param::Reverb];
    float mid=(l+r)*0.5f,side=(l-r)*0.5f*s[Param::Width];l=mid+side;r=mid-side;
}
void SynthEngine::prepare(double rate) {
    sampleRate=static_cast<float>(std::clamp(rate,8000.0,192000.0));
    smoothing=1-std::exp(-1/(0.015f*sampleRate));
    for(std::size_t i=0;i<harmonicInverseRoot.size();++i)harmonicInverseRoot[i]=1/std::sqrt(float(i+1));
    effects.prepare(sampleRate);if(!tablesReady){buildTables();tablesReady=true;}reset();prepared=true;
}
void SynthEngine::reset() noexcept {
    for(auto& v:voices) {v.note=-1;v.amp={};v.filterEnv={};v.filter.clear();v.tail=0;v.last=0;v.held=false;v.preview=false;}
    bend.fill(0);wheel.fill(0);sustain.fill(false);keys={};clock=0;previewSamples=0;lastFrequency=0;
    current=target;effects.reset();lastMode=int(target[Param::Mode]);lastVoiceCount=int(target[Param::Voices]);
}
void SynthEngine::setSettings(const Settings& s) noexcept {
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
float SynthEngine::wavetable(float p,float frequency,float position) const noexcept {
    float mip=std::clamp(std::log2(std::max(1.0f,frequency*256/sampleRate)),0.0f,7.0f);
    int m=int(mip),m2=std::min(m+1,7),a=std::clamp(int(position),0,3),b=std::min(a+1,3);
    float t=p*1024;int i=int(t)&1023,j=(i+1)&1023;float f=t-std::floor(t);
    const auto read=[&](int shape,int level) {const auto& table=tables[shape][level];return table[i]+f*(table[j]-table[i]);};
    float x=std::lerp(read(a,m),read(a,m2),mip-float(m)),y=std::lerp(read(b,m),read(b,m2),mip-float(m));
    return std::lerp(x,y,position-float(a));
}
void SynthEngine::start(Voice& v,int channel,int note,float velocity,bool preview,bool legato) noexcept {
    const bool active=v.amp.stage!=Envelope::Off;
    v.tail=active?v.last:0;v.tailStep=v.tail/64;
    v.channel=channel;v.note=note;v.held=true;v.preview=preview;v.age=++clock;v.velocity=velocity;v.targetFrequency=midiHz(float(note));
    if(!legato) {
        v.frequency=(target[Param::Glide]>0 && lastFrequency>0)?lastFrequency:v.targetFrequency;
        v.amp.start(true);v.filterEnv.start(true);v.p1=0;v.p2=0;v.fmPhase=0;v.subPhase=0;v.modalPhase={};v.elapsed=0;v.lfo1=0;v.lfo2=0;
        v.filter.clear();for(auto& f:v.formants) f.clear();v.controlCounter=0;
        const int period=std::clamp(int(sampleRate/v.targetFrequency),2,stringSize-2);
        v.string.fill(0);v.stringPos=0;v.stringLow=0;
        float previous=0;
        for(int i=0;i<period;++i) {const float x=v.noise.next();v.string[stringSize-period+i]=0.5f*(x+previous);previous=x;}
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
        case 1:{ // Harmonic-limited triangle avoids the aliases of a naive corner waveform.
            float x=0;for(int h=1;h<=15;h+=2) if(float(h)*dt<0.48f) x+=sine(p*float(h))*((h%4==1)?1.0f:-1.0f)/float(h*h);return x*0.81f;
        }
        case 2:return 2*p-1-blep(p,dt);
        case 3:width=0.5f;[[fallthrough]];
        case 4:return (p<width?1.0f:-1.0f)+blep(p,dt)-blep(wrap(p+1-width),dt);
        default:return random.next();
    }
}
void SynthEngine::updateFormants(Voice& v,float vowel) noexcept {
    constexpr float vowels[5][3] {{800,1150,2900},{400,1700,2600},{300,2200,3000},{500,800,2600},{350,600,2400}};
    vowel=std::clamp(vowel,0.0f,4.0f);int a=int(vowel),b=std::min(a+1,4);float t=vowel-float(a);
    float shift=std::exp2(current[Param::FormantShift]/12);
    for(int i=0;i<3;++i) v.formants[i].coefficients(std::lerp(vowels[a][i],vowels[b][i],t)*shift,0.87f,sampleRate);
}
float SynthEngine::voiceSample(Voice& v) noexcept {
    const auto& s=current;
    v.elapsed+=1/sampleRate;
    v.lfo1=wrap(v.lfo1+s[Param::Lfo1Rate]/sampleRate);v.lfo2=wrap(v.lfo2+s[Param::Lfo2Rate]/sampleRate);
    float modPitch=0,modFilter=0,modAmp=0,modVowel=0;
    const auto route=[&](int destination,float amount) {
        switch(destination) {case 1:modPitch+=amount*2;break;case 2:modFilter+=amount*3;break;case 3:modAmp+=amount*0.45f;break;case 4:modVowel+=amount*2;break;default:break;}
    };
    route(int(s[Param::Lfo1Dest]),sine(v.lfo1)*s[Param::Lfo1Depth]);route(int(s[Param::Lfo2Dest]),sine(v.lfo2)*s[Param::Lfo2Depth]);
    modPitch+=sine(v.elapsed*5.3f)*(s[Param::Vibrato]+wheel[v.channel-1]*0.6f)*0.6f;
    float glide=s[Param::Glide];v.frequency+=(v.targetFrequency-v.frequency)*(glide<0.001f?1.0f:1-std::exp(-1/(glide*sampleRate)));
    float frequency=std::clamp(v.frequency*std::exp2((bend[v.channel-1]*s[Param::BendRange]+modPitch)/12),8.0f,sampleRate*0.4f);
    v.fmPhase=wrap(v.fmPhase+frequency*current[Param::FmRatio]/sampleRate);v.subPhase=wrap(v.subPhase+frequency*0.5f/sampleRate);
    float dt=frequency/sampleRate,dt2=std::min(0.45f,dt*std::exp2((s[Param::Coarse]+s[Param::Detune]/100)/12));
    v.p1=wrap(v.p1+dt);v.p2=wrap(v.p2+dt2);
    float env=v.amp.next(s[Param::Attack],s[Param::Decay],s[Param::Sustain],sampleRate);
    float fenv=v.filterEnv.next(s[Param::FilterAttack],s[Param::FilterDecay],s[Param::FilterSustain],sampleRate);
    int engine=int(s[Param::Engine]);float x=0;
    switch(engine) {
        case 0:x=std::lerp(oscillator(int(s[Param::Wave1]),v.p1,dt,s[Param::PulseWidth],v.noise),oscillator(int(s[Param::Wave2]),v.p2,dt2,s[Param::PulseWidth],v.noise),s[Param::OscMix]);break;
        case 1:{float ratio=s[Param::FmRatio],index=s[Param::FmIndex]*(0.15f+0.85f*fenv);float limit=std::max(0.0f,(sampleRate*0.45f/frequency-1)/std::max(1.0f,ratio));index=std::min(index,limit);x=std::sin(tau*v.p1+sine(v.fmPhase)*index);x=std::lerp(x,sine(v.subPhase),s[Param::OscMix]*0.4f);break;}
        case 2:{
            // sin((n+1)t) = 2 cos(t) sin(nt) - sin((n-1)t).
            // Double precision keeps the recurrence stable near the phase boundaries.
            const double phase=double(v.p1)*2*std::numbers::pi_v<double>;
            const double factor=2*std::cos(phase);
            double previous=0,partial=std::sin(phase),sum=0;
            int used=0;
            for(int h=1;h<=int(s[Param::Harmonics]);++h) {
                if(float(h)*dt>=0.45f)break;
                sum+=partial*harmonicWeight[std::size_t(h-1)];
                const double next=factor*partial-previous;
                previous=partial;partial=next;++used;
            }
            x=used>0?float(sum)*harmonicScale[std::size_t(used-1)]:0;
            break;
        }
        case 3:x=std::lerp(wavetable(v.p1,frequency,s[Param::Wavetable]),wavetable(v.p2,dt2*sampleRate,s[Param::Wavetable]),s[Param::OscMix]);break;
        case 4:{float period=std::clamp(sampleRate/frequency,2.0f,float(stringSize-2));float read=float(v.stringPos)-period;if(read<0)read+=stringSize;int a=int(read),b=(a+1)%stringSize;float delayed=std::lerp(v.string[a],v.string[b],read-float(a));v.stringLow+=(0.15f+0.8f*s[Param::Brightness])*(delayed-v.stringLow);float value=0.5f*(delayed+v.stringLow)*s[Param::StringDamping];v.string[v.stringPos]=value;v.stringPos=(v.stringPos+1)%stringSize;x=delayed*2.2f;break;}
        case 5:{float saw=oscillator(2,v.p1,dt,0.5f,v.noise);float choir=oscillator(2,v.p2,dt2,0.5f,v.noise);float excitation=(saw+choir*s[Param::Choir])*0.5f;excitation=std::lerp(excitation,v.noise.next()*0.35f,s[Param::Breath]);for(int i=0;i<3;++i)x+=v.formants[i].next(excitation,2)*(i==0?1.0f:0.65f);x*=0.6f;break;}
        case 6:x=oscillator(int(s[Param::Wave1]),v.p1,dt,s[Param::PulseWidth],v.noise);x=std::round(x*15)/15;break;
        case 7:{ // Original modal bar / tine synthesis with inharmonic resonances.
            constexpr float ratios[]{1,2.01f,2.76f,4.07f,5.43f,6.79f};
            for(int i=0;i<6;++i) {float ratio=std::lerp(float(i+1),ratios[i],s[Param::Brightness]);if(ratio*dt>=0.45f)break;v.modalPhase[i]=wrap(v.modalPhase[i]+dt*ratio);x+=sine(v.modalPhase[i])*std::exp(-v.elapsed*float(i+1)/(0.3f+s[Param::Decay]))/float(i+1);}break;}
        default:break;
    }
    if(v.controlCounter++%16==0) {
        v.filter.coefficients(s[Param::Cutoff]*std::exp2(fenv*s[Param::FilterEnv]+modFilter),s[Param::Resonance],sampleRate);
        updateFormants(v,s[Param::Vowel]+modVowel+s[Param::Syllable]*sine(v.elapsed*3)*0.8f);
    }
    int type=int(s[Param::FilterType]);
    if(type==4 && engine!=5) {float y=0;for(auto& f:v.formants)y+=f.next(x,2)*0.22f;x=y;}
    else x=v.filter.next(x,std::min(type,3));
    float syllable=1-s[Param::Syllable]*0.6f*(0.5f+0.5f*sine(v.elapsed*3));
    float velocity=std::lerp(1.0f,std::sqrt(v.velocity),s[Param::Velocity]);
    x*=env*velocity*std::max(0.0f,1+modAmp-s[Param::Tremolo]*(0.5f+0.5f*sine(v.elapsed*4.2f)))*syllable*0.16f;
    if(std::abs(v.tail)>std::abs(v.tailStep)) {x+=v.tail;v.tail-=v.tailStep;} else v.tail=0;
    v.last=x;return x;
}
void SynthEngine::render(float* left,float* right,int samples) noexcept {
    if(!prepared) {std::fill_n(left,samples,0.0f);std::fill_n(right,samples,0.0f);return;}
    for(int i=0;i<samples;++i) {
        for(std::size_t p=0;p<parameterCount;++p) {
            if(specs[p].step>0 || p>=index(Param::MacroA)) current.values[p]=target.values[p];
            else current.values[p]+=(target.values[p]-current.values[p])*smoothing;
        }
        if(int(current[Param::Engine])==2) {
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
        }
        if(previewSamples>0 && --previewSamples==0) stopPreview();
        float l=0,r=0;
        for(auto& v:voices) if(v.amp.stage!=Envelope::Off) {
            float x=voiceSample(v);
            float pan=0.5f+0.16f*float((v.note%5)-2);
            if(int(current[Param::Lfo1Dest])==5) pan+=sine(v.lfo1)*current[Param::Lfo1Depth]*0.45f;
            if(int(current[Param::Lfo2Dest])==5) pan+=sine(v.lfo2)*current[Param::Lfo2Depth]*0.45f;
            pan=std::clamp(pan,0.0f,1.0f);l+=x*std::sqrt(1-pan);r+=x*std::sqrt(pan);
        }
        effects.process(l,r,current);
        // Bounded soft limiting protects the host from extreme patch combinations.
        left[i]=std::tanh(l)*current[Param::Volume];right[i]=std::tanh(r)*current[Param::Volume];
    }
}
}
