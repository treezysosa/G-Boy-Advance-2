#include "Effects.h"
#if defined(__SSE2__) || defined(_M_X64)
#include <emmintrin.h>
#endif

namespace gba {
float Effects::Line::read(float d) const noexcept {
    const float distance=std::clamp(d,1.0f,float(data.size()-2));
    const int whole=int(distance),size=int(data.size());
    int a=int(pos)-whole;if(a<0)a+=size;
    const int b=a==0?size-1:a-1;
    // Wrap integer addresses before interpolation: adding a tiny negative float
    // to the buffer length can round to length and read one element past the end.
    return std::lerp(data[std::size_t(a)],data[std::size_t(b)],distance-float(whole));
}
void Effects::Line::push(float x) noexcept {data[pos]=x;if(++pos==data.size())pos=0;}
Effects::Stereo Effects::Fir::next(const Stereo& x,const std::array<float,33>& coefficients) noexcept {
    history[std::size_t(pos)]=history[std::size_t(pos+33)]=x;
    Stereo result{};
#if defined(__SSE2__) || defined(_M_X64)
    // Each lane follows the old channel's multiply/add order exactly. Pairing
    // channels saves FIR loop/index work without changing taps or summation.
    const auto load=[](const Stereo& frame){return _mm_castsi128_ps(_mm_loadl_epi64(reinterpret_cast<const __m128i*>(frame.data())));};
    __m128 y=_mm_mul_ps(load(history[std::size_t(pos+17)]),_mm_set1_ps(coefficients[16]));
    for(int i=1;i<33;i+=2)y=_mm_add_ps(y,_mm_mul_ps(load(history[std::size_t(pos+33-i)]),_mm_set1_ps(coefficients[std::size_t(i)])));
    _mm_storel_epi64(reinterpret_cast<__m128i*>(result.data()),_mm_castps_si128(y));
#else
    for(int c=0;c<2;++c){
        float y=history[std::size_t(pos+17)][std::size_t(c)]*coefficients[16];
        for(int i=1;i<33;i+=2)y+=history[std::size_t(pos+33-i)][std::size_t(c)]*coefficients[std::size_t(i)];
        result[std::size_t(c)]=y;
    }
#endif
    if(++pos==33)pos=0;
    return result;
}
void Effects::Fir::push(const Stereo& x) noexcept {
    history[std::size_t(pos)]=history[std::size_t(pos+33)]=x;
    if(++pos==33)pos=0;
}
Effects::Stereo Effects::Fir::upEven(const Stereo& x,const std::array<float,33>& coefficients) noexcept {
    history[std::size_t(pos)]=history[std::size_t(pos+33)]=x;
    Stereo result{history[std::size_t(pos+17)][0]*coefficients[16],history[std::size_t(pos+17)][1]*coefficients[16]};
    if(++pos==33)pos=0;
    return result;
}
void Effects::Shelf::coefficients(float hz,float db,float rate,bool high) noexcept {
    const float a=std::pow(10.0f,db/40),w=tau*std::min(hz,rate*.4f)/rate,c=std::cos(w);
    const float beta=std::sqrt(2*a)*std::sin(w),plus=a+1,minus=a-1;
    const float a0=high?plus-minus*c+beta:plus+minus*c+beta;
    b0=a*(high?plus+minus*c+beta:plus-minus*c+beta)/a0;
    b1=(high?-2*a*(minus+plus*c):2*a*(minus-plus*c))/a0;
    b2=a*(high?plus+minus*c-beta:plus-minus*c-beta)/a0;
    a1=(high?2*(minus-plus*c):-2*(minus+plus*c))/a0;
    a2=(high?plus-minus*c-beta:plus+minus*c-beta)/a0;
    flat=b0==1&&b1==a1&&b2==a2;
    identity=flat&&z1==0&&z2==0;
}
float Effects::Shelf::next(float x) noexcept {
    // An identity shelf with zero state returns exactly x + 0. Keep the
    // ordinary path for non-finite/very large input and any remaining tail.
    if(identity&&std::abs(x)<1e30f){z1=z2=0;return x+0.0f;}
    const float y=b0*x+z1;z1=b1*x-a1*y+z2;z2=b2*x-a2*y;
    if(flat)identity=z1==0&&z2==0;
    return y;
}
void Effects::prepare(float rate) {
    sr=std::clamp(rate,8000.0f,192000.0f);
    for(auto& line:delay)line.data.assign(static_cast<std::size_t>(sr*1.5f)+16,0);
    for(auto& line:mod)line.data.assign(static_cast<std::size_t>(sr*.026f)+16,0);
    for(auto& line:tape)line.data.assign(static_cast<std::size_t>(sr*.006f)+16,0);
    constexpr float lengths[]{.0297f,.0371f,.0411f,.0437f};
    for(int i=0;i<4;++i)room[i].data.assign(static_cast<std::size_t>(sr*lengths[i])+3,0);
    double sum=0;
    for(int i=0;i<33;++i) {
        const int k=i-16;
        const double window=.42-.5*std::cos(2*std::numbers::pi_v<double>*i/32)+.08*std::cos(4*std::numbers::pi_v<double>*i/32);
        const double h=k==0?.5:(k%2==0?0:std::sin(std::numbers::pi_v<double>*.5*k)/(std::numbers::pi_v<double>*k));
        halfband[std::size_t(i)]=float(window*h);sum+=window*h;
    }
    for(auto& c:halfband)c/=float(sum);
    reverbDamping=1-std::exp(-tau*3200/sr);
    modulationIncrements={.27/double(sr),(.27*.71)/double(sr),(.27*.61)/double(sr),(.27*1.37)/double(sr),(.27*29)/double(sr)};
    delayFadeLength=std::max(1,int(std::round(sr*.02f)));
    ready=true;reset();
}
void Effects::reset() noexcept {
    for(auto* lines:{&delay,&mod,&tape})for(auto& line:*lines){std::fill(line.data.begin(),line.data.end(),0.0f);line.pos=0;line.damp=0;}
    for(auto& line:room){std::fill(line.data.begin(),line.data.end(),0.0f);line.pos=0;line.damp=0;}
    allpass={};up={};down={};dry={};held={};lowShelf={};highShelf={};
    bypassInput={};bypassPosition=bypassFrames=0;firSleeping=false;
    chorusPhase={};flangePhase={};phaserPhase={};wowPhase={};flutterPhase={};
    crusherCounter=controlCounter=delayFade=dryPos=0;coefficientsReady=false;delayA=delayB=0;
    eqReady=false;
}
void Effects::update(const Settings& s) noexcept {
    driveGain=1+10*s[Param::Drive]+22*s[Param::Distortion];
    driveNorm=1+.2f*(driveGain-1);driveMix=std::clamp((s[Param::Drive]+s[Param::Distortion])*2,0.0f,1.0f);
    hardness=s[Param::Distortion]*.75f;quantSteps=std::exp2(s[Param::CrusherBits]-1);
    roomFeedback=.5f+.46f*s[Param::ReverbSize];
    if(!eqReady||cachedEqLow!=s[Param::EqLow])for(auto& shelf:lowShelf)shelf.coefficients(180,s[Param::EqLow],sr,false);
    if(!eqReady||cachedEqHigh!=s[Param::EqHigh])for(auto& shelf:highShelf)shelf.coefficients(3800,s[Param::EqHigh],sr,true);
    cachedEqLow=s[Param::EqLow];cachedEqHigh=s[Param::EqHigh];eqReady=true;
}
void Effects::nonlinear(float& left,float& right) noexcept {
    const Stereo input{left,right},delayed{dry[0][std::size_t(dryPos)],dry[1][std::size_t(dryPos)]};
    dry[0][std::size_t(dryPos)]=left;dry[1][std::size_t(dryPos)]=right;
    if(driveMix==0) {
        // Both channels always take the same bypass path and FIR phase. Keep
        // all 64 recent input frames for exact wake-up, including stereo silence.
        bypassInput[0][std::size_t(bypassPosition)]=left;bypassInput[1][std::size_t(bypassPosition)]=right;
        bypassPosition=(bypassPosition+1)&63;
        if(bypassFrames<64){
            down.push(up.upEven({2*left,2*right},halfband));down.push(up.next({0,0},halfband));++bypassFrames;
        }else firSleeping=true;
        left=delayed[0];right=delayed[1];return;
    }
    if(firSleeping){
        up={};down={};
        for(int n=0;n<64;++n){
            const auto at=std::size_t((bypassPosition+n)&63);
            down.push(up.upEven({2*bypassInput[0][at],2*bypassInput[1][at]},halfband));down.push(up.next({0,0},halfband));
        }
        firSleeping=false;
    }
    bypassFrames=0;
    const auto shape=[&](Stereo frame){
        if(hardness==0){for(auto& x:frame)x=std::tanh(x*driveGain)/driveNorm;}
        else for(auto& x:frame){const float driven=x*driveGain;x=std::lerp(std::tanh(driven),std::clamp(driven,-1.0f,1.0f),hardness)/driveNorm;}
        return frame;
    };
    const auto even=down.next(shape(up.upEven({2*input[0],2*input[1]},halfband)),halfband);
    down.push(shape(up.next({0,0},halfband)));
    left=std::lerp(delayed[0],even[0],driveMix);right=std::lerp(delayed[1],even[1],driveMix);
}
void Effects::process(float& left,float& right,const Settings& s) noexcept {
    if(!ready){left=right=0;return;}
    if(controlCounter==0||!coefficientsReady){update(s);controlCounter=0;}
    if(!coefficientsReady){delayA=delayB=sr*s[Param::DelayTime];coefficientsReady=true;}
    controlCounter=(controlCounter+1)&15;
    // Keep each delay read head fixed during a crossfade; queue later changes.
    if(delayFade==0&&std::abs(sr*s[Param::DelayTime]-delayB)>.5f){delayA=delayB;delayB=sr*s[Param::DelayTime];delayFade=delayFadeLength;}
    const float delayBlend=delayFade>0?1-float(delayFade)/float(delayFadeLength):1;
    chorusPhase.advanceIncrement(modulationIncrements[0]);flangePhase.advanceIncrement(modulationIncrements[1]);phaserPhase.advanceIncrement(modulationIncrements[2]);
    wowPhase.advanceIncrement(modulationIncrements[3]);flutterPhase.advanceIncrement(modulationIncrements[4]);
    const float wobble=s[Param::Wow]>0?s[Param::Wow]*(.0015f*wowPhase.value()+.00025f*flutterPhase.value()):0;
    nonlinear(left,right);
    float* channels[]{&left,&right};
    for(int c=0;c<2;++c) {
        float x=*channels[c];
        // Bypass ends each sample with counter zero (rate one). Re-enabling
        // therefore captures a fresh held value before it can be read; quantizing
        // an inaudible bypassed value adds work without preserving audible history.
        if(crusherCounter==0&&(s[Param::CrusherBits]<16||s[Param::CrusherRate]>1))held[c]=std::round(x*quantSteps)/quantSteps;
        if(s[Param::CrusherBits]<16||s[Param::CrusherRate]>1)x=held[c];
        const float wow=s[Param::Wow]>0?tape[c].read(sr*(.004f+wobble)):x;tape[c].push(x);
        x=std::lerp(x,wow,s[Param::Wow]);
        // Read heads do not change the delay buffers. Skip inaudible reads and
        // their trigonometry, while advancing every clock and writing warm history.
        const float chorus=s[Param::Chorus]>0?mod[c].read(sr*(.018f+.006f*sine(float(chorusPhase.phase)+.25f*float(c)))):x;
        const float flange=s[Param::Flanger]>0?mod[c].read(sr*(.0025f+.002f*sine(float(flangePhase.phase)+.4f*float(c)))):0;
        mod[c].push(x);
        x+=s[Param::Chorus]*.45f*(chorus-x);x+=s[Param::Flanger]*.45f*flange;
        float ap=x;
        const float a=.12f+.72f*(.5f+.5f*sine(float(phaserPhase.phase)+.1f*float(c)));
        for(auto& state:allpass[c]){const float y=-a*ap+state;state=ap+a*y;ap=y;}
        x+=s[Param::Phaser]*.48f*ap;
        // Settled heads coincide. Read once while keeping feedback, writes and
        // the full two-head crossfade path warm during delay-time automation.
        const float firstEcho=delay[c].read(delayA);
        const float echo=delayA==delayB?firstEcho:std::lerp(firstEcho,delay[c].read(delayB),delayBlend);
        delay[c].push(x+echo*s[Param::DelayFeedback]);x+=echo*s[Param::Delay];
        *channels[c]=highShelf[c].next(lowShelf[c].next(x));
    }
    dryPos=(dryPos+1)%latencySamples;
    crusherCounter=(crusherCounter+1)%std::max(1,int(s[Param::CrusherRate]));
    if(delayFade>0&&--delayFade==0)delayA=delayB;
    std::array<float,4> taps{};
    for(int i=0;i<4;++i){auto& line=room[i];line.damp+=reverbDamping*(line.data[line.pos]-line.damp);taps[i]=line.damp;}
    const float total=taps[0]+taps[1]+taps[2]+taps[3];
    for(int i=0;i<4;++i)room[i].push((i%2==0?left:right)*.22f+(.5f*total-taps[i])*roomFeedback);
    left+=(taps[0]+taps[2])*s[Param::Reverb];right+=(taps[1]+taps[3])*s[Param::Reverb];
    const float mid=(left+right)*.5f,side=(left-right)*.5f*s[Param::Width];left=mid+side;right=mid-side;
}
}
