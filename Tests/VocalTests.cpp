#include "dsp/SynthEngine.h"
#include "FactoryData.h"
#include <atomic>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

extern std::atomic<bool> trackAllocations;
extern std::atomic<int> allocations;
namespace {
int checks=0;
void check(bool ok,const char* text){++checks;if(!ok)throw std::runtime_error(text);}
struct Audio {std::vector<float> l,r;explicit Audio(int n):l(std::size_t(n)),r(std::size_t(n)) {}};
void render(gba::SynthEngine& e,Audio& a){e.render(a.l.data(),a.r.data(),int(a.l.size()));}
double energy(const std::vector<float>& a){double x=0;for(float v:a)x+=double(v)*v;return x/double(a.size());}
double component(const std::vector<float>& a,double hz,double rate){
    double re=0,im=0,weight=0;
    for(std::size_t n=0;n<a.size();++n){const double w=.5-.5*std::cos(2*std::numbers::pi_v<double>*double(n)/double(a.size()-1));const double p=2*std::numbers::pi_v<double>*hz*double(n)/rate;re+=a[n]*w*std::cos(p);im+=a[n]*w*std::sin(p);weight+=w;}
    return 2*std::hypot(re,im)/weight;
}
double periodicity(const std::vector<float>& a,int lag){double xy=0,xx=0,yy=0;for(std::size_t i=0;i+std::size_t(lag)<a.size();++i){const double x=a[i],y=a[i+std::size_t(lag)];xy+=x*y;xx+=x*x;yy+=y*y;}return xy/std::sqrt(std::max(1e-30,xx*yy));}
gba::Settings clean(){gba::Settings s;s.set(gba::Param::Engine,5);s.set(gba::Param::Attack,.001f);s.set(gba::Param::Sustain,1);s.set(gba::Param::Release,.08f);s.set(gba::Param::FilterEnv,0);s.set(gba::Param::Cutoff,20000);s.set(gba::Param::Resonance,0);s.set(gba::Param::Lfo1Depth,0);s.set(gba::Param::Lfo2Depth,0);s.set(gba::Param::Vibrato,0);s.set(gba::Param::Reverb,0);s.set(gba::Param::Chorus,0);s.set(gba::Param::Drive,0);s.set(gba::Param::Breath,0);s.set(gba::Param::Choir,0);return s;}
Audio note(gba::SynthEngine& e,const gba::Settings& s,int pitch,double rate,int length){e.setSettings(s);e.prepare(rate);e.noteOn(1,pitch,.9f);Audio warm(int(rate*.1));render(e,warm);Audio out(length);render(e,out);return out;}
void resonances(){
    // Independent audio-response oracle: impulse spectra peak at five expected vowel centers.
    constexpr float expected[5][5]{{800,1150,2900,3900,4950},{400,1700,2600,3500,4500},{300,2200,3000,3800,4900},{500,800,2600,3500,4500},{350,600,2400,3400,4400}};
    for(float rate:{44100.f,48000.f,96000.f})for(int vowel=0;vowel<5;++vowel)for(float shift:{-12.f,0.f,12.f}){
        gba::VocalEngine voice;voice.configure(float(vowel),shift,.5f,rate);
        for(int band=0;band<5;++band){auto filter=voice.bands[std::size_t(band)];std::vector<float> impulse(8192);for(std::size_t i=0;i<impulse.size();++i)impulse[i]=filter.next(i==0?1.f:0.f,2)*filter.k;
            const double hz=expected[vowel][band]*std::exp2(shift/12);double at=0,best=-1;
            // Raw complex projection of the impulse response, with no window.
            const auto response=[&](double f){double re=0,im=0;for(std::size_t n=0;n<impulse.size();++n){double p=2*std::numbers::pi_v<double>*f*double(n)/rate;re+=impulse[n]*std::cos(p);im+=impulse[n]*std::sin(p);}return std::hypot(re,im);};
            for(int step=-4;step<=4;++step){double f=hz*(1+double(step)*.015),m=response(f);if(m>best){best=m;at=f;}}
            check(std::abs(at/hz-1)<.02,"Vowel resonance/shift/sample-rate response is misplaced");check(best>.98&&best<1.02,"Formant peak normalization is not stable across rates/shift");
        }
    }
    std::cout<<"PASS five vowel resonances across three rates and three formant shifts\n";
}
void excitation(gba::SynthEngine& e){
    for(double rate:{44100.,48000.,96000.}){
        auto s=clean();s.set(gba::Param::Vowel,0);auto voiced=note(e,s,57,rate,int(rate*.5));
        check(periodicity(voiced.l,int(std::round(rate/220)))>.95,"Voiced excitation has lost its MIDI-periodic pitch");
        s.set(gba::Param::Breath,1);auto air=note(e,s,57,rate,int(rate*.5));
        check(periodicity(air.l,int(std::round(rate/220)))<.5,"Unvoiced blend is still harmonically periodic");check(energy(air.l)>.00001,"Unvoiced vowel is silent");
        s.set(gba::Param::Breath,0);s.set(gba::Param::Brightness,0);auto dark=note(e,s,57,rate,int(rate*.5));
        s.set(gba::Param::Brightness,1);auto bright=note(e,s,57,rate,int(rate*.5));
        const double darkRatio=component(dark.l,2860,rate)/component(dark.l,220,rate),brightRatio=component(bright.l,2860,rate)/component(bright.l,220,rate);
        check(brightRatio>darkRatio*2,"Vocal brightness does not open harmonic spectral tilt");
        s.set(gba::Param::Choir,1);s.set(gba::Param::Detune,25);auto choir=note(e,s,57,rate,int(rate));
        const double fundamental=component(choir.l,220,rate);
        check(component(choir.l,220*std::exp2(25./1200),rate)>fundamental*.1,"Upper choir layer missing");
        check(component(choir.l,220*std::exp2(-25./1200),rate)>fundamental*.1,"Lower choir layer missing");
        // Unvoiced excitation must keep the same formant spectrum across MIDI pitch changes.
        auto prototype=std::make_unique<gba::SynthEngine>();s.set(gba::Param::Choir,0);s.set(gba::Param::Breath,1);prototype->setSettings(s);prototype->prepare(rate);
        auto low=std::make_unique<gba::SynthEngine>(*prototype),high=std::make_unique<gba::SynthEngine>(*prototype);
        low->noteOn(1,48,1);high->noteOn(1,73,1);Audio a(8192),b(8192);render(*low,a);render(*high,b);
        double difference=0;
        // Different panning is allowed; compare normalized harmonic-free waveform shape instead.
        const double scale=std::sqrt(energy(a.l)/energy(b.l));difference=0;for(std::size_t n=0;n<a.l.size();++n)difference+=std::abs(a.l[n]-b.l[n]*scale);
        check(difference/double(a.l.size())<1e-6,"MIDI pitch unexpectedly shifts the unvoiced formants");
    }
    std::cout<<"PASS harmonic/noise blend, spectral tilt, three choir layers and pitch-independent formants\n";
}
void factoryRegisters(){
    int vocalCount=0;
    for(double rate:{44100.,96000.}){
        auto prototype=std::make_unique<gba::SynthEngine>();prototype->prepare(rate);
        for(const auto& patch:gba::factoryPatches)if(std::string_view(patch.category).starts_with("Vocals")){
            ++vocalCount;gba::Settings s;s.values=patch.values;
            for(int pitch:{48,72}){auto e=std::make_unique<gba::SynthEngine>(*prototype);e->setSettings(s);e->reset();e->noteOn(1,pitch,.8f);Audio a(int(rate*.3));render(*e,a);bool finite=true;for(float x:a.l)finite&=std::isfinite(x)&&std::abs(x)<=.951f;
                check(finite&&energy(a.l)>1e-9,"Factory vocal is silent/non-finite in extended rate/register grid");}
        }
    }
    check(vocalCount==240,"Expected 120 vocal presets at two additional sample rates");
    std::cout<<"PASS all 120 vocal presets at 44.1/96 kHz in low/high registers\n";
}
void articulation(gba::SynthEngine& e){
    auto s=clean();s.set(gba::Param::Syllable,1);auto articulated=note(e,s,60,48000,48000);
    s.set(gba::Param::Syllable,0);auto steady=note(e,s,60,48000,48000);
    double diff=0;for(std::size_t i=0;i<steady.l.size();++i)diff+=std::abs(steady.l[i]-articulated.l[i]);
    check(diff/steady.l.size()>.003,"Synthetic syllabic vowel/amplitude articulation is inert");
    // Extreme live formant/choir/pitch automation with all voices active is allocation-free.
    s.set(gba::Param::Voices,32);e.setSettings(s);e.prepare(48000);for(int n=0;n<32;++n)e.noteOn(1,40+n,.8f);Audio block(128);
    allocations=0;trackAllocations=true;bool finite=true;
    for(int n=0;n<300;++n){s.set(gba::Param::Vowel,float(n%5));s.set(gba::Param::FormantShift,float(n%25-12));s.set(gba::Param::Choir,float(n%2));s.set(gba::Param::Breath,float(n%3)*.5f);s.set(gba::Param::Brightness,float(n%2));e.setSettings(s);e.pitchWheel(1,(n*53)%16384);render(e,block);for(float x:block.l)finite&=std::isfinite(x)&&std::abs(x)<=1;}
    trackAllocations=false;check(finite,"Vocal automation becomes non-finite or unbounded");check(allocations==0,"Vocal DSP allocates in render/MIDI/settings");
    e.allNotesOff();Audio release(6000);render(e,release);check(e.activeVoices()==0,"Vocal notes do not release");
    std::cout<<"PASS syllabic articulation, 32-voice automation, allocation-free processing and release\n";
}
}
int main(){try{resonances();auto e=std::make_unique<gba::SynthEngine>();excitation(*e);articulation(*e);factoryRegisters();std::cout<<"All "<<checks<<" Phase 7 vocal synthesis checks passed.\n";return 0;}catch(const std::exception& ex){trackAllocations=false;std::cerr<<"FAIL: "<<ex.what()<<"\n";return 1;}}
