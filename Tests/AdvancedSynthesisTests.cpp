#include "dsp/SynthEngine.h"
#include <atomic>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

extern std::atomic<bool> trackAllocations;
extern std::atomic<int> allocations;
namespace {
int checks=0;
void check(bool ok,const std::string& text) {++checks;if(!ok)throw std::runtime_error(text);}
struct Audio {std::vector<float> l,r;explicit Audio(int n):l(std::size_t(n)),r(std::size_t(n)) {}};
void render(gba::SynthEngine& e,Audio& a) {e.render(a.l.data(),a.r.data(),int(a.l.size()));}
double rms(const Audio& a) {double sum=0;for(float x:a.l)sum+=double(x)*x;return std::sqrt(sum/double(a.l.size()));}
// Hann-windowed complex projection suppresses leakage at non-bin-aligned MIDI pitches.
double component(const Audio& a,double hz,double sr) {
    double real=0,imag=0,weight=0;
    for(std::size_t n=0;n<a.l.size();++n) {
        const double w=.5-.5*std::cos(2*std::numbers::pi_v<double>*double(n)/double(a.l.size()-1));
        const double p=2*std::numbers::pi_v<double>*hz*double(n)/sr;
        real+=w*a.l[n]*std::cos(p);imag+=w*a.l[n]*std::sin(p);weight+=w;
    }
    return 2*std::hypot(real,imag)/weight;
}
double correlationPitch(const Audio& a,double expected,double sr) {
    const int lo=int(sr/expected*.92),hi=int(std::ceil(sr/expected*1.08)),count=int(a.l.size())-hi;
    const auto correlation=[&](int lag) {
        double xy=0,xx=0,yy=0;
        for(int n=0;n<count;++n){double x=a.l[std::size_t(n)],y=a.l[std::size_t(n+lag)];xy+=x*y;xx+=x*x;yy+=y*y;}
        return xy/std::sqrt(std::max(1e-30,xx*yy));
    };
    int best=lo;double peak=-1;
    for(int lag=lo;lag<=hi;++lag){double c=correlation(lag);if(c>peak){peak=c;best=lag;}}
    const double l=correlation(best-1),r=correlation(best+1),fraction=.5*(l-r)/(l-2*peak+r);
    return sr/(best+fraction);
}
gba::Settings clean(int engine) {
    gba::Settings s;s.set(gba::Param::Engine,float(engine));s.set(gba::Param::OscMix,0);s.set(gba::Param::Detune,0);
    s.set(gba::Param::Cutoff,20000);s.set(gba::Param::Resonance,0);s.set(gba::Param::FilterEnv,0);
    s.set(gba::Param::Lfo1Depth,0);s.set(gba::Param::Lfo2Depth,0);s.set(gba::Param::Vibrato,0);
    s.set(gba::Param::Reverb,0);s.set(gba::Param::Chorus,0);s.set(gba::Param::Drive,0);
    s.set(gba::Param::Attack,.001f);s.set(gba::Param::Sustain,1);s.set(gba::Param::Breath,0);s.set(gba::Param::Choir,0);
    s.set(gba::Param::FilterSustain,1);return s;
}
void init(gba::SynthEngine& e,const gba::Settings& s,double sr=48000) {e.setSettings(s);e.prepare(sr);}
void fmAndAdditive(gba::SynthEngine& e) {
    for(double sr:{44100.,48000.,96000.}) {
        Audio warm(int(sr*.03)),a(int(sr*.2));auto s=clean(1);s.set(gba::Param::FmIndex,0);init(e,s,sr);e.noteOn(1,69,1);render(e,warm);render(e,a);
        check(component(a,440,sr)>.02&&component(a,1320,sr)<component(a,440,sr)*.002,"FM zero index is not a clean carrier");
        s.set(gba::Param::FmIndex,1);s.set(gba::Param::FmRatio,2);init(e,s,sr);e.noteOn(1,69,1);render(e,warm);render(e,a);
        check(component(a,1320,sr)>.22*component(a,440,sr),"FM ratio 2 lacks its 3x sideband");
        check(component(a,880,sr)<.002*component(a,440,sr),"FM ratio 2 has an unexpected even sideband");
        s.set(gba::Param::FmRatio,3);init(e,s,sr);e.noteOn(1,69,1);render(e,warm);render(e,a);
        check(component(a,880,sr)>.2*component(a,440,sr)&&component(a,1760,sr)>.2*component(a,440,sr),"FM ratio changes do not move sidebands");
        s=clean(2);s.set(gba::Param::Harmonics,1);init(e,s,sr);e.noteOn(1,69,1);render(e,warm);render(e,a);
        check(component(a,440,sr)>.02&&component(a,880,sr)<component(a,440,sr)*.002,"One-partial additive bank is not sinusoidal");
        s.set(gba::Param::Harmonics,12);s.set(gba::Param::Brightness,1);init(e,s,sr);e.noteOn(1,69,1);render(e,warm);render(e,a);
        const double bright=component(a,3520,sr)/component(a,440,sr);
        check(component(a,880,sr)>.5*component(a,440,sr)&&bright>.1,"Additive partial count/brightness has no expected harmonics");
        s.set(gba::Param::Brightness,0);init(e,s,sr);e.noteOn(1,69,1);render(e,warm);render(e,a);
        check(component(a,3520,sr)/component(a,440,sr)<bright*.03,"Additive brightness does not attenuate upper partials");
    }
    std::cout<<"PASS FM carrier/ratio sidebands and additive partial count/brightness at three rates\n";
}
void wavetables(gba::SynthEngine& e) {
    for(double sr:{44100.,48000.,96000.}) {
        Audio warm(int(sr*.03)),a(int(sr*.2));auto s=clean(3);
        std::array<double,4> even{};
        for(int shape=0;shape<4;++shape) {
            s.set(gba::Param::Wavetable,float(shape));init(e,s,sr);e.noteOn(1,69,1);render(e,warm);render(e,a);
            check(rms(a)>.002,"Generated wavetable shape is silent");even[std::size_t(shape)]=component(a,880,sr)/component(a,440,sr);
        }
        check(even[0]>.4&&even[1]<.002,"Saw/odd wavetable identities are wrong");
        check(std::abs(even[2]-even[0])>.1&&std::abs(even[3]-even[0])>.1,"Original wavetable spectra are not distinct");
        s.set(gba::Param::Wavetable,.5f);init(e,s,sr);e.noteOn(1,69,1);render(e,warm);render(e,a);
        const double middle=component(a,880,sr)/component(a,440,sr);check(middle>even[1]+.1&&middle<even[0]-.1,"Wavetable morph is not between its endpoint spectra");
        // Old mip blending retained harmonics above Nyquist. Test the folded 8th partial.
        s.set(gba::Param::Wavetable,0);init(e,s,sr);const int note=sr>48000?117:105;const double hz=gba::midiHz(float(note));e.noteOn(1,note,1);render(e,warm);render(e,a);
        const double folded=std::abs(std::remainder(hz*8,sr));
        if(folded<sr*.45&&folded>4000)check(component(a,folded,sr)<component(a,hz,sr)*.003,"Wavetable mip blend introduces a folded high partial");
    }
    std::cout<<"PASS four mathematical tables, morph spectra and high-register mip alias regression\n";
}
void physicalModels(gba::SynthEngine& e) {
    for(double sr:{44100.,48000.,96000.})for(float brightness:{.1f,.9f})for(int note:{48,60,72,84}) {
        auto s=clean(4);s.set(gba::Param::Brightness,brightness);s.set(gba::Param::StringDamping,.9995f);init(e,s,sr);e.noteOn(1,note,1);
        // Measure the resonating pitch after 30 periods; the initial noisy pick transient is not periodic.
        Audio warm(int(sr*30/gba::midiHz(float(note)))),a(int(sr*.065));render(e,warm);render(e,a);
        const double expected=gba::midiHz(float(note)),hz=correlationPitch(a,expected,sr),cents=1200*std::log2(hz/expected);
        check(rms(a)>1e-5,"Plucked string is silent");
        check(std::abs(cents)<2,"String brightness detunes note "+std::to_string(note)+" by "+std::to_string(cents)+" cents at "+std::to_string(sr));
    }
    double tails[2]{};
    for(int i=0;i<2;++i){auto s=clean(4);s.set(gba::Param::Brightness,1);s.set(gba::Param::StringDamping,i==0?.9f:.9995f);init(e,s);e.noteOn(1,60,1);Audio warm(14400),a(4800);render(e,warm);render(e,a);tails[i]=rms(a);}
    check(tails[1]>tails[0]*5&&tails[1]>.0001,"String damping does not control physical decay");
    auto s=clean(7);s.set(gba::Param::Brightness,1);s.set(gba::Param::Decay,3);init(e,s);e.noteOn(1,69,1);Audio warm(480),a(12000);render(e,warm);render(e,a);
    check(component(a,440*2.76,48000)>component(a,1320,48000)*10,"Modal keys lack their inharmonic third mode");
    std::cout<<"PASS string tuning within 2 cents over three octaves/two brightnesses/three rates, decay and modal resonances\n";
}
void vocals(gba::SynthEngine& e) {
    Audio warm(4800),a(24000);double vowelPeaks[5]{};
    // Unvoiced excitation exposes the actual vowel filter resonances without a harmonic grid.
    for(int vowel=0;vowel<5;++vowel) {
        // Use the same deterministic noise realization for each vowel comparison.
        auto isolated=std::make_unique<gba::SynthEngine>();auto s=clean(5);s.set(gba::Param::Breath,1);s.set(gba::Param::Vowel,float(vowel));init(*isolated,s);isolated->noteOn(1,57,1);render(*isolated,warm);render(*isolated,a);
        check(rms(a)>.001,"Synthetic vowel bank is silent");
        double best=-1,at=0;
        for(int hz=250;hz<=950;hz+=25){double level=component(a,hz,48000);if(level>best){best=level;at=hz;}}
        vowelPeaks[vowel]=at;
    }
    check(vowelPeaks[0]>650&&vowelPeaks[2]<425&&vowelPeaks[4]<475,"A/I/U first formants are not separated");
    auto s=clean(5);s.set(gba::Param::Vowel,0);init(e,s);e.noteOn(1,57,1);render(e,warm);render(e,a);
    const double original=component(a,880,48000)/component(a,440,48000);
    s.set(gba::Param::FormantShift,-12);init(e,s);e.noteOn(1,57,1);render(e,warm);render(e,a);
    check(component(a,880,48000)/component(a,440,48000)<original*.5,"Formant shift does not move resonances independently of MIDI pitch");
    s=clean(5);s.set(gba::Param::Choir,1);s.set(gba::Param::Detune,25);init(e,s);e.noteOn(1,57,1);render(e,warm);render(e,a);
    check(component(a,440*std::exp2(25./1200),48000)>.1*component(a,440,48000),"Choir detuned excitation is absent");
    // Both banks must process independently during engine/filter transitions, without runaway feedback.
    s.set(gba::Param::FilterType,4);init(e,s);e.noteOn(1,57,1);render(e,warm);render(e,a);
    bool finite=true;for(float x:a.l)finite&=std::isfinite(x)&&std::abs(x)<=1;
    check(finite&&rms(a)>.001,"Vocal excitation plus independent formant filter failed");
    std::cout<<"PASS synthetic vowel resonances, formant shift, detuned choir and independent filter bank\n";
}
void boundedEngineChanges() {
    for(double sr:{44100.,48000.,96000.}) {
        const int fadeSamples=int(std::round(sr*.005));
        gba::EngineFade fade;fade.reset(0);
        bool bounded=true,normalized=true,continuous=true;
        for(int i=0;i<fadeSamples*40;++i) {
            const auto prior=fade.weights;
            fade.advance((i/7)%8,fadeSamples);
            int active=0;float sum=0;
            for(std::size_t j=0;j<fade.weights.size();++j) {
                const auto value=fade.weights[j];active+=value>0;sum+=value;
                bounded&=std::isfinite(value)&&value>=-1e-5f&&value<=1.00001f;
                continuous&=std::abs(value-prior[j])<=1.f/float(fadeSamples)+1e-4f;
            }
            bounded&=active<=2;normalized&=std::abs(sum-1)<1e-4f;
        }
        check(bounded,"Rapid engine changes activate more than two engines or invalid weights");
        check(normalized,"Engine retargeting changed total blend gain");
        check(continuous,"Engine retargeting abruptly discarded an audible engine");
        for(int i=0;i<2*fadeSamples;++i)fade.advance(7,fadeSamples);
        check(fade.remaining==0&&fade.target==7&&fade.weights[7]==1,"Latest engine request did not settle within 10 ms");
        fade.reset(4);check(fade.weights[4]==1&&fade.remaining==0,"Prepare/reset retained an engine transition");
    }
    std::cout<<"PASS bounded two-engine transitions, continuity, unity gain and latest-target convergence at three rates\n";
}
void transitions(gba::SynthEngine& e) {
    Audio warm(4064),one(1),settle(12000);
    for(double sr:{44100.,48000.,96000.})for(int next=1;next<8;++next) {
        auto s=clean(0);s.set(gba::Param::Wave1,0);init(e,s,sr);e.noteOn(1,69,1);render(e,warm);
        const double before=warm.l.back();s.set(gba::Param::Engine,float(next));e.setSettings(s);render(e,one);
        check(std::abs(one.l[0]-before)<.012,"Engine change makes an abrupt output step");render(e,settle);
        check(rms(settle)>1e-5,"Engine fade did not reach an audible new engine");
    }
    // Decaying models must excite when selected on an already-held note, even seconds after note-on.
    for(int engine:{4,7}) {auto s=clean(0);init(e,s);e.noteOn(1,60,1);Audio hold(96000);render(e,hold);s.set(gba::Param::Engine,float(engine));e.setSettings(s);render(e,settle);check(rms(settle)>.001,"Held-note physical model did not excite on activation");}
    auto s=clean(2);init(e,s);e.noteOn(1,60,1);render(e,warm);allocations=0;trackAllocations=true;bool finite=true;
    for(int n=0;n<800;++n) {s.set(gba::Param::Engine,float(n%8));s.set(gba::Param::FilterType,float(n%5));e.setSettings(s);render(e,one);finite&=std::isfinite(one.l[0])&&std::abs(one.l[0])<=1;}
    render(e,settle);trackAllocations=false;check(finite,"Rapid engine retargeting became unbounded");check(allocations==0,"Engine crossfade/model activation allocates on audio thread");
    std::cout<<"PASS all engine fades at three rates, late physical-model activation and allocation-free interrupted transitions\n";
}
}
int main() {
    try {auto e=std::make_unique<gba::SynthEngine>();fmAndAdditive(*e);wavetables(*e);physicalModels(*e);vocals(*e);boundedEngineChanges();transitions(*e);
        std::cout<<"All "<<checks<<" Phase 4 advanced synthesis checks passed.\n";return 0;
    }catch(const std::exception& error){trackAllocations=false;std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;}
}
