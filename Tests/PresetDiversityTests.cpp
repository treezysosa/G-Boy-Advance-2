#include "dsp/SynthEngine.h"
#include "FactoryData.h"
#include <complex>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <vector>

// Offline, gain-independent comparisons of actual rendered audio. Comparing the
// same absolute pitches across categories also catches bass/key cross-category
// duplicates. No audio files or analysis code are used by the plugin itself.
namespace {
constexpr int rate=48000,fftSize=4096,scenes=6,bands=64,features=2*bands+20;
constexpr double duplicateThreshold=.025,reviewThreshold=.06;
using Feature=std::array<double,features>;
struct Profile {std::array<Feature,scenes> dry{},wet{};};
int checks=0,renders=0;
void check(bool pass,const std::string& message) {++checks;if(!pass)throw std::runtime_error(message);}
void fft(std::array<std::complex<double>,fftSize>& a) {
    for(std::size_t i=1,j=0;i<a.size();++i){std::size_t bit=a.size()>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[i],a[j]);}
    for(std::size_t width=2;width<=a.size();width<<=1){const auto step=std::polar(1.,-2*std::numbers::pi_v<double>/double(width));for(std::size_t start=0;start<a.size();start+=width){std::complex<double> w=1;for(std::size_t j=0;j<width/2;++j){const auto u=a[start+j],v=a[start+j+width/2]*w;a[start+j]=u+v;a[start+j+width/2]=u-v;w*=step;}}}
}
gba::Settings dry(gba::Settings s) {
    for(auto p:{gba::Param::Reverb,gba::Param::Delay,gba::Param::Chorus,gba::Param::Phaser,gba::Param::Flanger,gba::Param::Drive,gba::Param::Distortion,gba::Param::Wow,gba::Param::EqLow,gba::Param::EqHigh})s.set(p,0);
    s.set(gba::Param::CrusherBits,16);s.set(gba::Param::CrusherRate,1);s.set(gba::Param::Width,1);s.set(gba::Param::Volume,1);return s;
}
Feature render(const gba::SynthEngine& prototype,gba::Settings s,int note,float velocity) {
    auto e=std::make_unique<gba::SynthEngine>(prototype);e->setSettings(s);e->reset();
    const int held=int(rate*std::clamp(double(s[gba::Param::Attack])+.45,1.2,6.0));
    const int count=held+rate/2;
    std::vector<float> l(std::size_t(count),0.f),r(l);
    e->noteOn(1,note,velocity);e->render(l.data(),r.data(),held);e->noteOff(1,note);e->render(l.data()+held,r.data()+held,count-held);++renders;
    double peakRms=0,energy=0;bool finite=true;
    for(int n=0;n<count;++n){const auto i=std::size_t(n);finite&=std::isfinite(l[i])&&std::isfinite(r[i]);energy+=(double(l[i])*l[i]+double(r[i])*r[i])*.5;if(n>=960){const auto k=std::size_t(n-960);energy-=(double(l[k])*l[k]+double(r[k])*r[k])*.5;}if(n>=959)peakRms=std::max(peakRms,std::sqrt(std::max(0.,energy)/960));}
    check(finite&&peakRms>1e-6,"Diversity render is silent or nonfinite");
    Feature f{};
    const int starts[]{int(rate*.025),std::min(held-fftSize,int(rate*(double(s[gba::Param::Attack])+.25)))};
    for(int window=0;window<2;++window) {
        std::array<std::complex<double>,fftSize> a{};
        for(int n=0;n<fftSize;++n){const auto i=std::size_t(starts[window]+n);a[std::size_t(n)]=.5*(l[i]+r[i])*(.5-.5*std::cos(2*std::numbers::pi_v<double>*n/(fftSize-1)));}
        fft(a);std::array<double,bands> spectrum{};
        for(int bin=1;bin<fftSize/2;++bin){const double hz=double(bin)*rate/fftSize;if(hz<30||hz>=20000)continue;const int band=std::clamp(int(std::log(hz/30)/std::log(20000./30)*bands),0,bands-1);spectrum[std::size_t(band)]+=std::norm(a[std::size_t(bin)]);}
        const double total=std::accumulate(spectrum.begin(),spectrum.end(),0.);
        for(int band=0;band<bands;++band)f[std::size_t(window*bands+band)]=std::sqrt(.5*spectrum[std::size_t(band)]/std::max(total,1e-30));
    }
    constexpr double times[]{.005,.015,.03,.05,.08,.12,.18,.25,.35,.5,.65,.8,1.,1.2,1.6,2.};
    for(int frame=0;frame<20;++frame) {
        const int at=frame<16?std::min(held-960,int(times[frame]*rate)):held+(frame-16)*rate/8;
        double sum=0;for(int n=at;n<at+960;++n){const auto i=std::size_t(n);sum+=(double(l[i])*l[i]+double(r[i])*r[i])*.5;}
        f[std::size_t(2*bands+frame)]=std::sqrt(.025*sum/960)/peakRms;
    }
    return f;
}
Profile profile(const gba::SynthEngine& e,gba::Settings s) {
    Profile p;int i=0;
    for(int note:{48,60,72})for(float velocity:{.4f,.9f}) {
        p.dry[std::size_t(i)]=render(e,dry(s),note,velocity);
        p.wet[std::size_t(i)]=render(e,s,note,velocity);++i;
    }
    return p;
}
double distance(const std::array<Feature,scenes>& a,const std::array<Feature,scenes>& b) {
    double sum=0;for(int s=0;s<scenes;++s)for(int f=0;f<features;++f){const double d=a[std::size_t(s)][std::size_t(f)]-b[std::size_t(s)][std::size_t(f)];sum+=d*d;}return std::sqrt(sum/scenes);
}
void detectorControls(const gba::SynthEngine& e) {
    gba::Settings s;s.set(gba::Param::Engine,0);s.set(gba::Param::Wave1,0);s.set(gba::Param::Wave2,0);s.set(gba::Param::Attack,.001f);s.set(gba::Param::Cutoff,18000);s=dry(s);
    const auto original=profile(e,s),renamed=profile(e,s);
    check(distance(original.dry,renamed.dry)==0,"Detector missed an identical renamed patch");
    s.set(gba::Param::Volume,.13f);s.set(gba::Param::Reverb,.8f);s.set(gba::Param::Chorus,.7f);const auto disguised=profile(e,s);
    check(distance(original.dry,disguised.dry)==0,"Detector missed a gain/effect-only duplicate");
    check(distance(original.wet,disguised.wet)>duplicateThreshold,"Wet comparison did not register real effects");
    s=dry(s);s.set(gba::Param::Wave1,2);s.set(gba::Param::Wave2,2);const auto different=profile(e,s);
    check(distance(original.dry,different.dry)>reviewThreshold,"Detector cannot distinguish sine and saw excitation");
    std::cout<<"PASS duplicate-detector controls: renamed, gain/effects-only and different excitation\n";
}
}
int main() {try {
    auto prototype=std::make_unique<gba::SynthEngine>();prototype->prepare(rate);detectorControls(*prototype);
    std::vector<Profile> profiles;profiles.reserve(gba::factoryPatches.size());
    for(const auto& patch:gba::factoryPatches){gba::Settings s;s.values=patch.values;profiles.push_back(profile(*prototype,s));if(profiles.size()%50==0)std::cout<<"Diversity audited "<<profiles.size()<<" / "<<gba::factoryPatches.size()<<"\n"<<std::flush;}
    std::ofstream csv("factory-diversity.csv"),pairs("factory-diversity-review.csv");
    csv<<std::setprecision(10)<<"id,name,nearestId,nearestName,dryDistance,wetDistance\n";
    pairs<<std::setprecision(10)<<"id,name,otherId,otherName,dryDistance,wetDistance\n";
    int duplicates=0,review=0;double minimum=100;
    for(std::size_t i=0;i<profiles.size();++i){double best=100;std::size_t closest=i;
        for(std::size_t j=0;j<profiles.size();++j)if(i!=j){const double d=distance(profiles[i].dry,profiles[j].dry);if(d<best){best=d;closest=j;}if(j<i&&d<reviewThreshold){++review;if(d<=duplicateThreshold)++duplicates;const auto& a=gba::factoryPatches[i];const auto& b=gba::factoryPatches[j];pairs<<a.id<<",\""<<a.name<<"\","<<b.id<<",\""<<b.name<<"\","<<d<<","<<distance(profiles[i].wet,profiles[j].wet)<<"\n";}}
        const auto& a=gba::factoryPatches[i];const auto& b=gba::factoryPatches[closest];minimum=std::min(minimum,best);csv<<a.id<<",\""<<a.name<<"\","<<b.id<<",\""<<b.name<<"\","<<best<<","<<distance(profiles[i].wet,profiles[closest].wet)<<"\n";
    }
    check(bool(csv)&&bool(pairs),"Cannot write diversity audit reports");
    std::ofstream summary("factory-diversity.json");summary<<std::setprecision(10)<<"{\"presets\":"<<profiles.size()<<",\"pairs\":"<<profiles.size()*(profiles.size()-1)/2<<",\"factoryRenders\":"<<profiles.size()*12<<",\"notes\":[48,60,72],\"velocities\":[0.4,0.9],\"sampleRate\":48000,\"duplicateThreshold\":"<<duplicateThreshold<<",\"reviewThreshold\":"<<reviewThreshold<<",\"duplicatePairs\":"<<duplicates<<",\"reviewPairs\":"<<review<<",\"minimumDryDistance\":"<<minimum<<",\"limits\":\"Objective spectral/envelope screen, not a human listening certificate. Effects and gain removed for dry comparison; all categories compared at identical absolute pitches.\"}\n";
    check(bool(summary),"Cannot write diversity summary");
    std::cout<<"Factory diversity: "<<profiles.size()<<" presets, "<<profiles.size()*12<<" renders, "<<profiles.size()*(profiles.size()-1)/2<<" pairs, "<<duplicates<<" duplicate pairs (<=0.025), "<<review<<" review pairs (<0.06), minimum "<<minimum<<"\n";
    check(duplicates==0,"Duplicate or nearly identical dry factory sounds found; see factory-diversity-review.csv");
    check(review==0,"Closely related factory sounds need review; see factory-diversity-review.csv");
    std::cout<<"PASS "<<checks<<" factory diversity checks\n";return 0;
}catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;}}
