#include "dsp/SynthEngine.h"
#include <atomic>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

extern std::atomic<bool> trackAllocations;
extern std::atomic<int> allocations;

namespace {
int checks=0;
void check(bool ok,const std::string& message) {++checks;if(!ok)throw std::runtime_error(message);}
struct Audio {std::vector<float> l,r;explicit Audio(int size):l(std::size_t(size)),r(std::size_t(size)) {}};
double rms(const std::vector<float>& a) {double sum=0;for(float x:a)sum+=double(x)*x;return std::sqrt(sum/double(a.size()));}
double pitch(const Audio& a,double sr=48000) {
    double first=0,last=0;int crossings=0;
    for(std::size_t i=1;i<a.l.size();++i)if(a.l[i-1]<=0&&a.l[i]>0) {
        const double at=double(i-1)-a.l[i-1]/double(a.l[i]-a.l[i-1]);if(crossings++==0)first=at;last=at;
    }
    return crossings>1?double(crossings-1)*sr/(last-first):0;
}
double harmonic(const Audio& a,double hz,double sr) {
    double real=0,imag=0;
    for(std::size_t i=0;i<a.l.size();++i) {double p=2*std::numbers::pi_v<double>*hz*double(i)/sr;real+=a.l[i]*std::cos(p);imag+=a.l[i]*std::sin(p);}
    return 2*std::hypot(real,imag)/double(a.l.size());
}
gba::Settings clean() {
    gba::Settings s;s.set(gba::Param::Engine,0);s.set(gba::Param::Wave1,0);s.set(gba::Param::Wave2,0);s.set(gba::Param::OscMix,0);
    s.set(gba::Param::Cutoff,20000);s.set(gba::Param::FilterEnv,0);s.set(gba::Param::Lfo1Depth,0);s.set(gba::Param::Lfo2Depth,0);
    s.set(gba::Param::Vibrato,0);s.set(gba::Param::Reverb,0);s.set(gba::Param::Chorus,0);s.set(gba::Param::Drive,0);
    s.set(gba::Param::Attack,.001f);s.set(gba::Param::Sustain,1);s.set(gba::Param::Release,.02f);return s;
}
void init(gba::SynthEngine& e,const gba::Settings& s,double sr=48000) {e.setSettings(s);e.prepare(sr);}
void render(gba::SynthEngine& e,Audio& a) {e.render(a.l.data(),a.r.data(),int(a.l.size()));}
void envelopeTiming() {
    for(float sr:{44100.0f,48000.0f,96000.0f}) {
        gba::Envelope e;e.start(true);int count=0;double previous=0;bool rising=true;
        while(e.stage==gba::Envelope::Attack&&count<int(sr)) {e.next(.05f,.12f,.4f,sr);rising&=e.value>=previous;previous=e.value;++count;}
        check(rising&&std::abs(count-int(std::lround(.05*sr)))<=1,"ADSR attack timing/ramp incorrect");
        count=0;bool falling=true;
        while(e.stage==gba::Envelope::Decay&&count<int(sr)) {e.next(.05f,.12f,.4f,sr);falling&=e.value<=previous;previous=e.value;++count;}
        check(falling&&std::abs(count-int(std::lround(.12*sr)))<=1,"ADSR decay timing/ramp incorrect");
        for(int i=0;i<100;++i)e.next(.05f,.12f,.4f,sr);
        check(std::abs(e.value-.4)<1e-6&&e.stage==gba::Envelope::Sustain,"ADSR sustain level drifted");
        e.stop(.2f,sr);count=0;
        while(e.stage!=gba::Envelope::Off&&count<int(sr)) {if(count%32==0)e.stop(.2f,sr);e.next(.05f,.12f,.4f,sr);++count;}
        check(std::abs(count-int(std::lround(.2*sr)))<=1&&e.value==0,"Repeated release extended ADSR timing");
        e.start(true);for(int i=0;i<int(sr*.015f);++i)e.next(.1f,.1f,.5f,sr);
        const double level=e.value;e.stop(.04f,sr);count=0;
        while(e.stage!=gba::Envelope::Off&&count<int(sr)) {e.next(.1f,.1f,.5f,sr);++count;}
        check(level>0&&level<.2&&std::abs(count-int(std::lround(.04*sr)))<=1,"Release during attack did not use the current level");
        e.start(true);e.next(0,0,1,sr);e.next(0,0,1,sr);e.stop(12,sr);count=0;
        while(e.stage!=gba::Envelope::Off&&count<int(sr*12.1f)) {e.next(0,0,1,sr);++count;}
        check(std::abs(count-int(sr*12))<=1,"Long release lost sample-rate timing precision");
    }
    std::cout<<"PASS ADSR stages, interrupted attack, idempotent release and 12-second timing\n";
}
void longModulationClock() {
    std::array<gba::LfoPhase,5> phases{};constexpr double rates[]{.02,.3,3,4.2,5.3};
    constexpr int sr=96000,seconds=600;
    for(int n=0;n<sr*seconds;++n)for(std::size_t i=0;i<phases.size();++i)phases[i].advance(rates[i],sr);
    for(std::size_t i=0;i<phases.size();++i) {
        check(std::abs(std::remainder(phases[i].phase-std::fmod(rates[i]*seconds,1.0),1.0))<1e-7,"Ten-minute modulation clock drifted");
        auto old=phases[i].phase;for(int n=0;n<97;++n)phases[i].advance(rates[i],sr);
        check(phases[i].phase!=old&&std::isfinite(phases[i].value()),"Long-held-note modulation stopped advancing");
    }
    std::cout<<"PASS five bounded modulation clocks through ten minutes at 96 kHz\n";
}
double response(int type,double hz,float resonance=0) {
    gba::SVF f;f.coefficients(1000,resonance,48000);double sum=0;
    for(int n=0;n<24000;++n) {float x=gba::sine(float(double(n)*hz/48000));float y=f.next(x,type);if(n>=4800)sum+=double(y)*y;}
    return std::sqrt(sum/19200)*std::sqrt(2.0);
}
void filters() {
    check(response(0,100)>.9&&response(0,5000)<.05,"Low-pass frequency response incorrect");
    check(response(1,100)<.015&&response(1,5000)>.9,"High-pass frequency response incorrect");
    check(response(2,1000)>.45&&response(2,100)<.12&&response(2,5000)<.22,"Band-pass frequency response incorrect");
    check(response(3,1000)<.001&&response(3,100)>.97&&response(3,5000)>.9,"Notch frequency response incorrect");
    check(response(0,1000,.95f)>5*response(0,1000,0),"Resonance does not increase the cutoff response");
    for(float sr:{44100.0f,48000.0f,96000.0f}) {
        gba::SVF f;bool finite=true;
        for(int n=0;n<48000;++n) {f.coefficients(n%2?15:sr*.44f,.95f,sr);auto out=f.process(n==0?1.0f:0.0f);finite&=std::isfinite(out.low)&&std::isfinite(out.high)&&std::isfinite(out.band)&&std::isfinite(out.notch);}
        check(finite,"SVF became non-finite during extreme cutoff modulation");
    }
    std::cout<<"PASS measured LP/HP/BP/notch response, resonance and modulation stability\n";
}
void oscillators(gba::SynthEngine& e) {
    for(double sr:{44100.,48000.,96000.}) {
        std::array<std::array<double,3>,5> spectra{};Audio warm(int(sr*.05)),a(int(sr*.25));
        for(int wave=0;wave<6;++wave) {
            auto s=clean();s.set(gba::Param::Wave1,float(wave));s.set(gba::Param::PulseWidth,.23f);init(e,s,sr);e.noteOn(1,69,1);render(e,warm);render(e,a);
            bool finite=true;for(float x:a.l)finite&=std::isfinite(x)&&std::abs(x)<=1;
            check(finite&&rms(a.l)>.001,"Subtractive waveform is silent or non-finite");
            if(wave<5)for(int h=0;h<3;++h)spectra[std::size_t(wave)][std::size_t(h)]=harmonic(a,440.0*(h+1),sr);
        }
        check(spectra[0][1]<spectra[0][0]*.002,"Sine contains an unexpected second harmonic");
        check(spectra[1][2]>.06*spectra[1][0]&&spectra[1][2]<.15*spectra[1][0],"Triangle harmonic profile incorrect");
        check(spectra[2][1]>.4*spectra[2][0],"Saw lacks its second harmonic");
        check(spectra[3][1]<.002*spectra[3][0]&&spectra[3][2]>.25*spectra[3][0],"Square odd-harmonic profile incorrect");
        check(spectra[4][1]>.5*spectra[4][0],"Pulse-width setting does not create its expected even harmonics");
    }
    std::cout<<"PASS six subtractive waveforms and measured harmonic profiles at three rates\n";
}
void voicesAndModulation(gba::SynthEngine& e) {
    Audio warm(4800),a(4800),b(4800),shortBlock(480);
    auto s=clean();s.set(gba::Param::Release,.06f);init(e,s);e.noteOn(1,69,1);render(e,warm);e.noteOff(1,69);
    for(int n=0;n<12;++n){e.controller(1,64,0);render(e,shortBlock);}
    check(e.activeVoices()==0,"Repeated pedal-up messages kept a released note alive");
    for(int limit=1;limit<=32;++limit) {
        s=clean();s.set(gba::Param::Voices,float(limit));init(e,s);for(int n=36;n<72;++n)e.noteOn(1,n,.8f);
        check(e.activeVoices()==limit,"Polyphony/stealing violates its configured voice count");
        for(int n=36;n<72;++n)e.noteOff(1,n);
        render(e,warm);check(e.activeVoices()==0,"Voice-stealing sequence left a note stuck");
    }
    s=clean();s.set(gba::Param::Velocity,1);init(e,s);e.noteOn(1,69,1);render(e,warm);render(e,a);
    init(e,s);e.noteOn(1,69,.25f);render(e,warm);render(e,b);check(std::abs(rms(b.l)/rms(a.l)-.5)<.005,"Velocity amplitude law incorrect");
    double levels[2]{};Audio tail(128),held(1024);
    for(int mode=1;mode<=2;++mode) {
        s=clean();s.set(gba::Param::Mode,float(mode));s.set(gba::Param::Attack,.4f);init(e,s);e.noteOn(1,60,1);render(e,warm);render(e,warm);e.noteOn(1,64,1);render(e,tail);render(e,held);levels[mode-1]=rms(held.l);
    }
    check(levels[1]>5*levels[0],"Legato retriggered its envelope or mono failed to retrigger");
    s=clean();s.set(gba::Param::Mode,2);init(e,s);e.noteOn(1,60,1);e.noteOn(2,64,1);e.noteOn(1,67,1);e.noteOff(1,67);render(e,warm);render(e,a);
    check(std::abs(pitch(a)-gba::midiHz(64))<1,"Mono held-key return chose the wrong note/channel");
    e.noteOff(2,64);render(e,warm);render(e,a);check(std::abs(pitch(a)-gba::midiHz(60))<1,"Mono held-key return lost the oldest held note");
    e.noteOff(1,60);render(e,warm);check(e.activeVoices()==0,"Mono channel-spanning held-key stack got stuck");
    s=clean();s.set(gba::Param::Mode,2);s.set(gba::Param::Glide,.1f);init(e,s);e.noteOn(1,60,1);render(e,warm);e.noteOn(1,72,1);render(e,warm);Audio glide(960);render(e,glide);
    const double expected=gba::midiHz(72)-(gba::midiHz(72)-gba::midiHz(60))*.1*(std::exp(-1.0)-std::exp(-1.2))/.02;
    check(std::abs(pitch(glide)-expected)<3,"Glide pitch trajectory does not follow its configured time constant");
    s=clean();s.set(gba::Param::OscMix,1);s.set(gba::Param::Coarse,12);s.set(gba::Param::Detune,0);init(e,s);e.noteOn(1,69,1);render(e,warm);render(e,a);
    check(std::abs(pitch(a)-880)<1,"Second oscillator blend/coarse tuning does not reach the audio");
    s=clean();init(e,s);e.noteOn(1,60,1);render(e,warm);render(e,a);
    init(e,s);e.controller(1,1,127);e.noteOn(1,60,1);render(e,warm);render(e,b);
    double wheelDifference=0;for(std::size_t i=0;i<a.l.size();++i)wheelDifference+=std::abs(a.l[i]-b.l[i]);check(wheelDifference>.1,"MIDI modulation wheel does not produce vibrato");
    s.set(gba::Param::Tremolo,.8f);init(e,s);e.noteOn(1,60,1);render(e,warm);render(e,b);check(rms(b.l)<.9*rms(a.l),"Tremolo does not modulate amplitude");
    for(int lfo=0;lfo<2;++lfo)for(int destination=1;destination<=5;++destination) {
        s=clean();s.set(gba::Param::Wave1,2);s.set(gba::Param::Cutoff,1600);if(destination==4)s.set(gba::Param::FilterType,4);
        const auto rate=lfo==0?gba::Param::Lfo1Rate:gba::Param::Lfo2Rate,depth=lfo==0?gba::Param::Lfo1Depth:gba::Param::Lfo2Depth,dest=lfo==0?gba::Param::Lfo1Dest:gba::Param::Lfo2Dest;
        s.set(rate,4);s.set(dest,float(destination));init(e,s);e.noteOn(1,60,.9f);render(e,warm);render(e,a);
        s.set(depth,.8f);init(e,s);e.noteOn(1,60,.9f);render(e,warm);render(e,b);
        double difference=0;for(std::size_t i=0;i<a.l.size();++i)difference+=std::abs(a.l[i]-b.l[i])+std::abs(a.r[i]-b.r[i]);
        check(difference>.1,"LFO "+std::to_string(lfo+1)+" destination "+std::to_string(destination)+" is inaudible");
    }
    s=clean();s.set(gba::Param::Wave1,2);s.set(gba::Param::Cutoff,600);init(e,s);e.noteOn(1,60,1);render(e,a);
    s.set(gba::Param::FilterEnv,4);s.set(gba::Param::FilterAttack,.001f);s.set(gba::Param::FilterDecay,.04f);s.set(gba::Param::FilterSustain,0);init(e,s);e.noteOn(1,60,1);render(e,b);
    double difference=0;for(std::size_t i=0;i<a.l.size();++i)difference+=std::abs(a.l[i]-b.l[i]);check(difference>.5,"Filter ADSR does not modulate the audio");
    std::cout<<"PASS all 32 voice limits, MIDI releases, mono/legato, velocity and both LFO routes\n";
}
void transitions(gba::SynthEngine& e) {
    Audio before(4064),after(1),settle(4800);
    auto s=clean();init(e,s);e.noteOn(1,69,1);render(e,before);s.set(gba::Param::Wave1,2);e.setSettings(s);render(e,after);
    check(std::abs(after.l[0]-before.l.back())<.01f,"Waveform change makes an abrupt output step");render(e,settle);check(harmonic(settle,880,48000)>.4*harmonic(settle,440,48000),"Waveform crossfade did not reach saw");
    s=clean();s.set(gba::Param::Cutoff,2000);init(e,s);e.noteOn(1,69,1);render(e,before);s.set(gba::Param::FilterType,1);e.setSettings(s);render(e,after);
    check(std::abs(after.l[0]-before.l.back())<.01f,"Filter type change makes an abrupt output step");render(e,settle);check(rms(settle.l)<.01,"Filter crossfade did not reach high-pass");
    s=clean();s.set(gba::Param::Lfo1Rate,4);s.set(gba::Param::Lfo1Depth,.8f);s.set(gba::Param::Lfo1Dest,0);init(e,s);e.noteOn(1,69,1);render(e,before);
    s.set(gba::Param::Lfo1Dest,3);e.setSettings(s);render(e,after);check(std::abs(after.l[0]-before.l.back())<.005f,"LFO destination change makes an abrupt amplitude step");
    allocations=0;trackAllocations=true;bool bounded=true;
    for(int i=0;i<240;++i) {
        s.set(gba::Param::Wave1,float(i%6));s.set(gba::Param::Wave2,float((i+2)%6));s.set(gba::Param::FilterType,float(i%5));s.set(gba::Param::OscMix,.5f);
        s.set(gba::Param::Lfo1Dest,float(i%6));s.set(gba::Param::Lfo2Dest,float((i+3)%6));e.setSettings(s);render(e,after);
        bounded&=std::isfinite(after.l[0])&&std::isfinite(after.r[0])&&std::abs(after.l[0])<=1&&std::abs(after.r[0])<=1;
    }
    trackAllocations=false;check(bounded,"Interrupted choice transitions became unbounded");check(allocations==0,"Choice crossfades allocate on the audio thread");
    std::cout<<"PASS waveform/filter crossfades, rapid retargeting and no render allocations\n";
}
void stableBlocksAndCopies() {
    for(double rate:{44100.,48000.,96000.})for(int engine:{0,2,3}) {
        auto prototype=std::make_unique<gba::SynthEngine>();auto s=clean();s.set(gba::Param::Engine,float(engine));s.set(gba::Param::Lfo1Depth,.3f);s.set(gba::Param::Lfo2Depth,.2f);s.set(gba::Param::Lfo2Dest,5);s.set(gba::Param::Vibrato,.17f);s.set(gba::Param::Glide,.04f);
        init(*prototype,s,rate);Audio warm(17);
        for(int note:{48,55,60,64}){prototype->noteOn(1,note,.8f);render(*prototype,warm);}
        auto whole=std::make_unique<gba::SynthEngine>(*prototype),split=std::make_unique<gba::SynthEngine>(*prototype);
        Audio a(8192),b(8192);render(*whole,a);constexpr int sizes[]{1,17,64,257};
        for(int at=0,block=0;at<8192;++block){const int count=std::min(sizes[block%4],8192-at);split->render(b.l.data()+at,b.r.data()+at,count);at+=count;}
        check(a.l==b.l&&a.r==b.r,"Stable voice processing depends on host block partition");
        check(whole->activeVoices()==split->activeVoices(),"Stable block partition changed held voices");
    }
    auto original=std::make_unique<gba::SynthEngine>();auto s=clean();s.set(gba::Param::Engine,4);init(*original,s);original->noteOn(1,60,.8f);Audio warm(256);render(*original,warm);
    auto first=std::make_unique<gba::SynthEngine>(*original),second=std::make_unique<gba::SynthEngine>(*original),third=std::make_unique<gba::SynthEngine>(*original);
    Audio a(256),b(256),advance(2048);render(*second,a);render(*first,advance);render(*third,b);
    check(a.l==b.l&&a.r==b.r,"Copied prepared engines share mutable plucked-string memory");
    std::cout<<"PASS stable chords at three rates with irregular blocks and independent prepared-engine copies\n";
}
}
int main() {
    try {
        envelopeTiming();longModulationClock();filters();stableBlocksAndCopies();auto e=std::make_unique<gba::SynthEngine>();oscillators(*e);voicesAndModulation(*e);transitions(*e);
        std::cout<<"All "<<checks<<" Phase 3 core synthesis checks passed.\n";return 0;
    }catch(const std::exception& error){trackAllocations=false;std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;}
}
