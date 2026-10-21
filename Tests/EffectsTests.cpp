#include "dsp/SynthEngine.h"
#include <atomic>
#include <iostream>
#include <memory>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

extern std::atomic<bool> trackAllocations;
extern std::atomic<int> allocations;
namespace {
int checks=0;
void check(bool ok,const std::string& message){++checks;if(!ok)throw std::runtime_error(message);}
struct Audio {std::vector<float> l,r;explicit Audio(int n):l(std::size_t(n)),r(std::size_t(n)) {}};
gba::Settings dry(){gba::Settings s;s.set(gba::Param::Drive,0);s.set(gba::Param::Reverb,0);s.set(gba::Param::Chorus,0);return s;}
void process(gba::Effects& fx,Audio& a,const gba::Settings& s){for(std::size_t n=0;n<a.l.size();++n)fx.process(a.l[n],a.r[n],s);}
double energy(const std::vector<float>& a,int first,int last){double v=0;for(int i=first;i<last;++i)v+=double(a[std::size_t(i)])*a[std::size_t(i)];return v;}
double component(const std::vector<float>& a,double hz,double sr,int start=0){double re=0,im=0;for(std::size_t n=std::size_t(start);n<a.size();++n){const double p=2*std::numbers::pi_v<double>*hz*double(n)/sr;re+=a[n]*std::cos(p);im+=a[n]*std::sin(p);}return 2*std::hypot(re,im)/double(a.size()-std::size_t(start));}
void softLimitAccuracy(){
    double error=0;bool bounded=true,odd=true;
    for(int n=-800000;n<=800000;++n){const float x=float(n)*.00001f,y=gba::softLimit(x);error=std::max(error,double(std::abs(y-std::tanh(x))));bounded&=std::isfinite(y)&&std::abs(y)<=1;odd&=gba::softLimit(-x)==-y;}
    check(error<=1.2e-7&&bounded&&odd,"Optimized soft limiter changes the saturation response beyond float precision");
    check(gba::softLimit(0)==0&&gba::softLimit(100)==1&&gba::softLimit(-100)==-1,"Soft limiter zero or outer-range response is wrong");
    check(std::isnan(gba::softLimit(std::numeric_limits<float>::quiet_NaN()))&&gba::softLimit(std::numeric_limits<float>::infinity())==1,"Soft limiter no longer preserves libm special-value behavior");
    std::cout<<"PASS 1,600,001-point limiter reference sweep; max absolute error "<<error<<"\n";
}
void bypassAndLatency(){
    gba::Effects fx;float l=1,r=1;fx.process(l,r,dry());check(l==0&&r==0,"Unprepared effects must safely return silence");
    for(float sr:{44100.f,48000.f,96000.f}) {
        fx.prepare(sr);auto s=dry();Audio a(128);a.l[0]=1;process(fx,a,s);
        bool exact=true;for(int n=0;n<128;++n)exact&=a.l[std::size_t(n)]==(n==16?1.f:0.f)&&a.r[std::size_t(n)]==0;
        check(exact,"Bypassed chain is not an exact 16-frame delay with channel isolation");
        fx.reset();Audio b(128);b.l[0]=1;process(fx,b,s);check(a.l==b.l&&a.r==b.r,"Reset did not reproduce the impulse response");
        fx.reset();s.set(gba::Param::Drive,.5f);Audio c(128);c.l[0]=.0001f;process(fx,c,s);
        auto peak=std::max_element(c.l.begin(),c.l.end());check(std::distance(c.l.begin(),peak)==16,"Oversampling group delay disagrees with reported host latency");
        s.set(gba::Param::Delay,.8f);s.set(gba::Param::DelayTime,.02f);s.set(gba::Param::Reverb,.8f);s.set(gba::Param::Chorus,.8f);s.set(gba::Param::Phaser,.8f);s.set(gba::Param::Wow,.8f);
        fx.reset();Audio tail(4096);tail.l[0]=tail.r[0]=1;process(fx,tail,s);fx.reset();Audio silence(4096);process(fx,silence,s);
        check(energy(silence.l,0,4096)==0&&energy(silence.r,0,4096)==0,"Reset left effects history audible");
    }
    std::cout<<"PASS exact bypass, channel isolation, reset and measured oversampling latency at three rates\n";
}
void nonlinear(){
    for(float sr:{44100.f,48000.f,96000.f}) {
        gba::Effects fx;fx.prepare(sr);auto s=dry();s.set(gba::Param::Drive,1);Audio a(12000);std::vector<float> reference(a.l.size());
        const double f=.21*sr,alias=.37*sr;
        for(std::size_t n=0;n<a.l.size();++n){float x=.7f*float(std::sin(2*std::numbers::pi_v<double>*.21*double(n)));a.l[n]=a.r[n]=x;reference[n]=std::tanh(x*11)/3;}
        process(fx,a,s);const double ratio=component(a.l,alias,sr,2000)/component(a.l,f,sr,2000),naive=component(reference,alias,sr,2000)/component(reference,f,sr,2000);
        check(ratio<naive*.2,"2x filtering does not reduce the selected folded harmonic by at least 14 dB");
        check(component(a.l,f,sr,2000)>.05,"Oversampling removed the carrier");
    }
    double fifth[2]{};
    for(int mode=0;mode<2;++mode){gba::Effects fx;fx.prepare(48000);auto s=dry();s.set(mode==0?gba::Param::Drive:gba::Param::Distortion,.8f);Audio a(24000);for(std::size_t n=0;n<a.l.size();++n)a.l[n]=.5f*gba::sine(float(n)*220/48000);process(fx,a,s);fifth[mode]=component(a.l,1100,48000,4800)/component(a.l,220,48000,4800);}
    check(fifth[1]>fifth[0]*1.1,"Distortion and saturation do not have distinct nonlinear spectra");
    std::cout<<"PASS oversampled nonlinear alias reduction and distinct saturation/distortion spectra\n";
}
void delays(){
    for(float sr:{44100.f,48000.f,96000.f}) {
        gba::Effects fx;fx.prepare(sr);auto s=dry();s.set(gba::Param::Delay,.5f);s.set(gba::Param::DelayTime,.05f);s.set(gba::Param::DelayFeedback,.4f);
        const int d=int(std::lround(sr*.05f));Audio a(d*4+64);a.l[0]=1;process(fx,a,s);
        check(std::abs(a.l[16]-1)<1e-6,"Delay lost its aligned dry signal");
        for(int repeat=1;repeat<=3;++repeat){double sum=0;for(int i=16+repeat*d-1;i<=16+repeat*d+1;++i)sum+=a.l[std::size_t(i)];check(std::abs(sum-.5*std::pow(.4,repeat-1))<.0002,"Echo timing or geometric feedback amplitude is incorrect");}
        check(energy(a.r,0,int(a.r.size()))==0,"Delay leaks between independent channels");
        fx.reset();s.set(gba::Param::DelayFeedback,0);s.set(gba::Param::DelayTime,(float(d)+.0007f)/sr);
        Audio fractional(d+64);fractional.l[0]=1;process(fx,fractional,s);
        const float fraction=sr*s[gba::Param::DelayTime]-float(d);
        check(fraction>0&&std::abs(fractional.l[std::size_t(16+d)]-.5f*(1-fraction))<1e-6&&std::abs(fractional.l[std::size_t(17+d)]-.5f*fraction)<1e-6,"Tiny fractional delay loses interpolation precision at the ring wrap");
        fx.reset();s.set(gba::Param::DelayTime,.05f);s.set(gba::Param::DelayFeedback,0);s.set(gba::Param::Delay,.8f);
        const int change=int(sr*.12f),count=change+int(sr*.1f);const double hz=713;
        double maxError=0,firstError=0;
        for(int n=0;n<count;++n){if(n==change)s.set(gba::Param::DelayTime,.08f);float l=.2f*float(std::sin(2*std::numbers::pi_v<double>*hz*n/sr)),r=0;fx.process(l,r,s);
            if(n==change){double expected=.2*std::sin(2*std::numbers::pi_v<double>*hz*(n-16)/sr)+.16*std::sin(2*std::numbers::pi_v<double>*hz*(n-16-d)/sr);firstError=std::abs(l-expected);}
            if(n>change+int(sr*.03f)){double expected=.2*std::sin(2*std::numbers::pi_v<double>*hz*(n-16)/sr)+.16*std::sin(2*std::numbers::pi_v<double>*hz*(n-16-sr*.08f)/sr);maxError=std::max(maxError,std::abs(l-expected));}}
        check(firstError<.0002,"Delay-time change jumps immediately to a new read head");
        check(maxError<.001,"Delay crossfade did not settle on the new stationary tap");
    }
    std::cout<<"PASS exact delay taps/feedback and smooth stationary-head time changes at three rates\n";
}
void reverb(){
    for(float sr:{44100.f,48000.f,96000.f}) {
        double late[2]{};
        for(int size=0;size<2;++size){gba::Effects fx;fx.prepare(sr);auto s=dry();s.set(gba::Param::Reverb,.8f);s.set(gba::Param::ReverbSize,size==0?.1f:.9f);Audio a(int(sr*1.2f));a.l[0]=1;process(fx,a,s);
            check(energy(a.l,17,int(sr*.027f))==0,"Reverb output appears before its shortest delay");
            check(energy(a.r,int(sr*.07f),int(sr*.4f))>1e-6,"Reverb does not develop a stereo tail");
            late[size]=energy(a.l,int(sr*.7f),int(sr*1.1f))+energy(a.r,int(sr*.7f),int(sr*1.1f));}
        check(late[1]>late[0]*20&&late[1]>1e-8,"Room size does not increase the reverb decay");
    }
    std::cout<<"PASS reverb onset, stereo diffusion and room-dependent decay at three rates\n";
}
double sineGain(gba::Param parameter,float value,double hz,float sr){gba::Effects fx;fx.prepare(sr);auto s=dry();s.set(parameter,value);const int warm=int(sr*.15f),count=int(sr*.2f);double power=0;for(int n=0;n<warm+count;++n){float l=.05f*float(std::sin(2*std::numbers::pi_v<double>*hz*n/sr)),r=l;fx.process(l,r,s);if(n>=warm)power+=double(l)*l;}return std::sqrt(power/count)*std::sqrt(2.)/.05;}
void modulationAndEq(){
    for(float sr:{44100.f,48000.f,96000.f}) {
        for(auto parameter:{gba::Param::Chorus,gba::Param::Flanger}){gba::Effects fx;fx.prepare(sr);auto s=dry();s.set(parameter,1);Audio a(int(sr*.05f));a.l[0]=a.r[0]=1;process(fx,a,s);
            const int onset=parameter==gba::Param::Chorus?int(sr*.011f):int(sr*.0004f);
            check(energy(a.l,17,onset)==0,"Modulated delay appears before its minimum delay");
            check(energy(a.l,onset,int(a.l.size()))>1e-4,"Chorus/flanger has no delayed output");
            double diff=0;for(std::size_t n=17;n<a.l.size();++n)diff+=std::abs(a.l[n]-a.r[n]);check(diff>.01,"Stereo modulation offsets are inert");}
        double minimum=10,maximum=0;for(double hz:{200.,1000.,5000.}){double gain=sineGain(gba::Param::Phaser,1,hz,sr);minimum=std::min(minimum,gain);maximum=std::max(maximum,gain);}
        check(maximum>minimum*1.2,"Phaser response is only a flat gain");
        for(float db:{-12.f,12.f}){
            check(std::abs(20*std::log10(sineGain(gba::Param::EqLow,db,30,sr))-db)<.2,"Low shelf does not reach its specified bass gain");
            check(std::abs(20*std::log10(sineGain(gba::Param::EqHigh,db,16000,sr))-db)<.3,"High shelf does not reach its specified treble gain");}
        for(float db:{-12.f,12.f}) {
            check(std::abs(20*std::log10(sineGain(gba::Param::EqLow,db,180,sr))-db*.5)<.15,"Low shelf midpoint is not half the dB gain");
            check(std::abs(20*std::log10(sineGain(gba::Param::EqHigh,db,3800,sr))-db*.5)<.15,"High shelf midpoint is not half the dB gain");
        }
        check(std::abs(sineGain(gba::Param::EqLow,12,5000,sr)-1)<.02,"Low shelf excessively changes treble");
        check(std::abs(sineGain(gba::Param::EqHigh,12,30,sr)-1)<.02,"High shelf excessively changes bass");
        gba::Effects fx;fx.prepare(sr);auto s=dry();const int wrap=int(sr/.27f),end=wrap+128;double step=0;float previous=0;
        for(int n=0;n<end;++n){s.set(gba::Param::Wow,1);float l=.2f*float(std::sin(2*std::numbers::pi_v<double>*1100*n/sr)),r=l;fx.process(l,r,s);if(n>wrap-128)step=std::max(step,double(std::abs(l-previous)));previous=l;}
        check(step<.05,"Wow/flutter jumps at the former shared modulation-clock wrap");
        fx.reset();s=dry();float output=0;constexpr int at=4096;
        for(int n=0;n<=at;++n){if(n==at)s.set(gba::Param::Wow,.0002f);float l=.2f*float(std::sin(2*std::numbers::pi_v<double>*1100*n/sr)),r=l;fx.process(l,r,s);output=l;}
        const double drySample=.2*std::sin(2*std::numbers::pi_v<double>*1100*(at-16)/sr);
        check(std::abs(output-drySample)<.001,"Enabling a tiny wow amount introduces a full delayed-signal jump");
    }
    std::cout<<"PASS chorus/flanger timing/stereo, phaser response, measured EQ gains and continuous wow/flutter\n";
}
void crusherWidthAndStress(){
    for(float sr:{44100.f,48000.f,96000.f}) {
        gba::Effects fx;fx.prepare(sr);auto s=dry();s.set(gba::Param::CrusherBits,4);bool quantized=true;
        for(int n=0;n<1000;++n){float l=float(n%100)/55-.9f,r=-l;fx.process(l,r,s);if(n>32)quantized&=std::abs(l*8-std::round(l*8))<1e-5&&std::abs(l+r)<1e-6;}
        check(quantized,"Bit depth does not create independent quantized levels");
        fx.reset();s=dry();s.set(gba::Param::CrusherRate,8);std::array<float,256> held{};
        for(int n=0;n<256;++n){float l=float(n)/500,r=0;fx.process(l,r,s);held[std::size_t(n)]=l;}
        bool correct=true;for(int n=32;n<256;++n)correct&=held[std::size_t(n)]==held[std::size_t(n-n%8)];check(correct,"Rate reduction does not hold eight consecutive frames");
        for(float width:{0.f,1.f,2.f}){fx.reset();s=dry();s.set(gba::Param::Width,width);float l=0,r=0;for(int n=0;n<64;++n){l=.3f;r=.1f;fx.process(l,r,s);}check(std::abs(l-(.2f+.1f*width))<1e-6&&std::abs(r-(.2f-.1f*width))<1e-6,"Stereo width changes the mid or fails to scale side");}
        fx.reset();s=dry();bool finite=true;allocations=0;trackAllocations=true;
        for(int n=0;n<int(sr);++n){if(n%97==0){for(std::size_t p=gba::index(gba::Param::Reverb);p<gba::index(gba::Param::MacroA);++p)s.values[p]=(n/97)%2?gba::specs[p].min:gba::specs[p].max;}
            float l=.3f*gba::sine(float(n)*731/sr),r=.3f*gba::sine(float(n)*419/sr);fx.process(l,r,s);finite&=std::isfinite(l)&&std::isfinite(r)&&std::abs(l)<1000&&std::abs(r)<1000;}
        trackAllocations=false;check(finite,"Combined extreme effects automation is unstable");check(allocations==0,"Effects processing/automation allocates on the audio thread");
    }
    auto a=std::make_unique<gba::SynthEngine>(),b=std::make_unique<gba::SynthEngine>();auto s=dry();s.set(gba::Param::Drive,.7f);s.set(gba::Param::Reverb,.5f);s.set(gba::Param::Delay,.4f);s.set(gba::Param::Wow,.6f);s.set(gba::Param::Chorus,.7f);s.set(gba::Param::EqLow,6);
    for(auto* e:{a.get(),b.get()}){e->setSettings(s);e->prepare(48000);e->noteOn(1,60,.8f);}
    Audio x(8192),y(8192);a->render(x.l.data(),x.r.data(),8192);for(int at=0;at<8192;){const int count=std::min(8192-at,at%127+1);b->render(y.l.data()+at,y.r.data()+at,count);at+=count;}
    check(x.l==y.l&&x.r==y.r,"Effects processing changes when the same audio is split into different block sizes");
    std::cout<<"PASS crusher depth/rate, stereo width, extreme automation, zero allocations and block partition invariance\n";
}
}
void crusherBypassTransitions(){
    for(float sr:{44100.f,48000.f,96000.f})for(int phase=0;phase<4;++phase)for(int bypassFrames:{1,17}) {
        gba::Effects fx;fx.prepare(sr);auto s=dry();
        // Align re-enable to a coefficient tick so the known eight-bit scale
        // is active immediately; stagger the first capture for all prior phases.
        const int enabled=64,bypass=enabled-bypassFrames;bool correct=true;
        for(int n=0;n<enabled+32;++n) {
            if(n==phase){s.set(gba::Param::CrusherBits,8);s.set(gba::Param::CrusherRate,4);}
            if(n==bypass){s.set(gba::Param::CrusherBits,16);s.set(gba::Param::CrusherRate,1);}
            if(n==enabled){s.set(gba::Param::CrusherBits,8);s.set(gba::Param::CrusherRate,4);}
            float l=.0037f*float(n+1)-.2f,r=-l;fx.process(l,r,s);
            if(n>=enabled){const int captured=enabled+(n-enabled)/4*4;const float delayed=.0037f*float(captured-16+1)-.2f;const float expected=std::round(delayed*128)/128;correct&=std::abs(l-expected)<1e-6f&&std::abs(r+expected)<1e-6f;}
        }
        check(correct,"Crusher bypass/re-enable retained stale held audio or lost the four-frame capture timing");
    }
    std::cout<<"PASS crusher re-enable captures fresh latency-aligned audio after short/long bypass at all prior counter phases and three rates\n";
}
int main(){try{softLimitAccuracy();bypassAndLatency();nonlinear();delays();reverb();modulationAndEq();crusherWidthAndStress();crusherBypassTransitions();std::cout<<"All "<<checks<<" Phase 5 effects checks passed.\n";return 0;}catch(const std::exception& e){trackAllocations=false;std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}}
