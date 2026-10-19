#include "Effects.h"

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
float Effects::Fir::next(float x,const std::array<float,33>& coefficients) noexcept {
    history[std::size_t(pos)]=history[std::size_t(pos+33)]=x;
    float y=history[std::size_t(pos+17)]*coefficients[16];
    // A half-band FIR has zero even taps except for its center.
    for(int i=1;i<33;i+=2)y+=history[std::size_t(pos+33-i)]*coefficients[std::size_t(i)];
    if(++pos==33)pos=0;
    return y;
}
void Effects::Fir::push(float x) noexcept {
    history[std::size_t(pos)]=history[std::size_t(pos+33)]=x;
    if(++pos==33)pos=0;
}
float Effects::Fir::upEven(float x,const std::array<float,33>& coefficients) noexcept {
    // The interpolator always receives (2*x, 0). At its even phase every
    // noncentral contributing tap multiplies a stored zero. Keep the identical
    // history/phase and center product, without evaluating those zero products.
    history[std::size_t(pos)]=history[std::size_t(pos+33)]=x;
    const float y=history[std::size_t(pos+17)]*coefficients[16];
    if(++pos==33)pos=0;
    return y;
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
}
float Effects::Shelf::next(float x) noexcept {
    const float y=b0*x+z1;z1=b1*x-a1*y+z2;z2=b2*x-a2*y;
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
    delayFadeLength=std::max(1,int(std::round(sr*.02f)));
    ready=true;reset();
}
void Effects::reset() noexcept {
    for(auto* lines:{&delay,&mod,&tape})for(auto& line:*lines){std::fill(line.data.begin(),line.data.end(),0.0f);line.pos=0;line.damp=0;}
    for(auto& line:room){std::fill(line.data.begin(),line.data.end(),0.0f);line.pos=0;line.damp=0;}
    allpass={};up={};down={};dry={};held={};lowShelf={};highShelf={};
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
float Effects::nonlinear(float x,int c) noexcept {
    const float delayed=dry[c][std::size_t(dryPos)];dry[c][std::size_t(dryPos)]=x;
    const auto shape=[&](float v) {
        if(driveMix==0)return v;
        const float driven=v*driveGain;
        return std::lerp(std::tanh(driven),std::clamp(driven,-1.0f,1.0f),hardness)/driveNorm;
    };
    const float a=down[c].next(shape(up[c].upEven(2*x,halfband)),halfband);
    // The odd decimator output is discarded. Only its input history is needed
    // for the next even output; nonlinear shaping and both FIR histories stay warm.
    down[c].push(shape(up[c].next(0,halfband)));
    return std::lerp(delayed,a,driveMix);
}
void Effects::process(float& left,float& right,const Settings& s) noexcept {
    if(!ready){left=right=0;return;}
    if(controlCounter==0||!coefficientsReady){update(s);controlCounter=0;}
    if(!coefficientsReady){delayA=delayB=sr*s[Param::DelayTime];coefficientsReady=true;}
    controlCounter=(controlCounter+1)&15;
    // Keep each delay read head fixed during a crossfade; queue later changes.
    if(delayFade==0&&std::abs(sr*s[Param::DelayTime]-delayB)>.5f){delayA=delayB;delayB=sr*s[Param::DelayTime];delayFade=delayFadeLength;}
    const float delayBlend=delayFade>0?1-float(delayFade)/float(delayFadeLength):1;
    chorusPhase.advance(.27,sr);flangePhase.advance(.27*.71,sr);phaserPhase.advance(.27*.61,sr);
    wowPhase.advance(.27*1.37,sr);flutterPhase.advance(.27*29,sr);
    const float wobble=s[Param::Wow]>0?s[Param::Wow]*(.0015f*wowPhase.value()+.00025f*flutterPhase.value()):0;
    float* channels[]{&left,&right};
    for(int c=0;c<2;++c) {
        float x=nonlinear(*channels[c],c);
        if(crusherCounter==0)held[c]=std::round(x*quantSteps)/quantSteps;
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
        const float echo=std::lerp(delay[c].read(delayA),delay[c].read(delayB),delayBlend);
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
