#include "dsp/SynthEngine.h"
#include "FactoryData.h"
#include <complex>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr int rate=48000,fftSize=4096;
int checks=0;
void check(bool ok,const std::string& message){++checks;if(!ok)throw std::runtime_error(message);}
struct Audio {std::vector<float> l,r;explicit Audio(int n):l(std::size_t(n)),r(std::size_t(n)) {}};
struct Metrics {double rms=0,peak=0,dc=0;bool finite=true;std::size_t at=0;};
struct Signature {std::string id,name,category;int note;std::array<double,32> spectrum{};std::array<double,8> envelope{};};
int middle(const gba::FactoryPatch& patch){return std::string_view(patch.category)=="Bass"?48:60;}
std::unique_ptr<gba::SynthEngine> engine(const gba::SynthEngine& prototype,const gba::Settings& settings){
    // Analysis runs off the audio thread. Copying a prepared, unused prototype
    // gives every patch the same noise seed without regenerating mathematical tables.
    auto e=std::make_unique<gba::SynthEngine>(prototype);e->setSettings(settings);e->reset();return e;
}
Audio held(gba::SynthEngine& e,const gba::Settings& s,int note,float velocity){
    const int frames=int(rate*std::clamp(double(s[gba::Param::Attack])+.35,.8,6.0));
    Audio a(frames);e.noteOn(1,note,velocity);e.render(a.l.data(),a.r.data(),frames);return a;
}
Metrics measure(const Audio& a){
    Metrics m;const std::size_t window=4800;double sum=0,dc=0;
    for(std::size_t n=0;n<a.l.size();++n){const double l=a.l[n],r=a.r[n];m.finite&=std::isfinite(l)&&std::isfinite(r);m.peak=std::max({m.peak,std::abs(l),std::abs(r)});dc+=(l+r)*.5;
        sum+=(l*l+r*r)*.5;if(n>=window)sum-=(double(a.l[n-window])*a.l[n-window]+double(a.r[n-window])*a.r[n-window])*.5;
        if(n+1>=window){const double rms=std::sqrt(std::max(0.,sum)/double(window));if(rms>m.rms){m.rms=rms;m.at=n+1-window;}}}
    m.dc=dc/double(a.l.size());return m;
}
void fft(std::array<std::complex<double>,fftSize>& a){
    for(std::size_t i=1,j=0;i<a.size();++i){std::size_t bit=a.size()>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[i],a[j]);}
    for(std::size_t width=2;width<=a.size();width<<=1){const auto step=std::polar(1.,-2*std::numbers::pi_v<double>/double(width));for(std::size_t start=0;start<a.size();start+=width){std::complex<double> w=1;for(std::size_t j=0;j<width/2;++j){const auto u=a[start+j],v=a[start+j+width/2]*w;a[start+j]=u+v;a[start+j+width/2]=u-v;w*=step;}}}
}
Signature signature(const gba::FactoryPatch& p,const Audio& a,int note){
    Signature s;s.id=p.id;s.name=p.name;s.category=p.category;s.note=note;const auto m=measure(a);
    // Three windows around the loudest dry section reduce sensitivity to noise realization.
    for(int offset:{0,1024,2048}){std::array<std::complex<double>,fftSize> data{};const std::size_t start=std::min(m.at+std::size_t(offset),a.l.size()-fftSize);
        for(int n=0;n<fftSize;++n)data[std::size_t(n)]=.5*(a.l[start+std::size_t(n)]+a.r[start+std::size_t(n)])*(.5-.5*std::cos(2*std::numbers::pi_v<double>*n/(fftSize-1)));
        fft(data);for(int bin=1;bin<fftSize/2;++bin){const double hz=double(bin)*rate/fftSize;if(hz<40||hz>=20000)continue;const int band=std::clamp(int(std::log(hz/40)/std::log(500.)*32),0,31);s.spectrum[std::size_t(band)]+=std::norm(data[std::size_t(bin)]);}}
    const double total=std::accumulate(s.spectrum.begin(),s.spectrum.end(),0.);for(auto& x:s.spectrum)x=std::sqrt(x/std::max(total,1e-30));
    constexpr double times[]{.02,.05,.1,.2,.35,.5,.7,1.0};
    for(std::size_t i=0;i<s.envelope.size();++i){const int at=std::min(int(a.l.size())-960,int(times[i]*rate)),end=at+960;double energy=0;for(int n=at;n<end;++n)energy+=(double(a.l[std::size_t(n)])*a.l[std::size_t(n)]+double(a.r[std::size_t(n)])*a.r[std::size_t(n)])*.5;s.envelope[i]=std::sqrt(energy/960)/std::max(m.rms,1e-12);}
    return s;
}
gba::Settings withoutEffects(gba::Settings s){for(auto p:{gba::Param::Reverb,gba::Param::Delay,gba::Param::Chorus,gba::Param::Phaser,gba::Param::Flanger,gba::Param::Drive,gba::Param::Distortion,gba::Param::Wow,gba::Param::EqLow,gba::Param::EqHigh})s.set(p,0);s.set(gba::Param::CrusherBits,16);s.set(gba::Param::CrusherRate,1);s.set(gba::Param::Width,1);s.set(gba::Param::Volume,1);return s;}
double distance(const Signature& a,const Signature& b){double sum=0;for(std::size_t i=0;i<a.spectrum.size();++i)sum+=std::pow(a.spectrum[i]-b.spectrum[i],2);for(std::size_t i=0;i<a.envelope.size();++i)sum+=.025*std::pow(a.envelope[i]-b.envelope[i],2);return std::sqrt(sum);}
void writeWav(const std::filesystem::path& path,const Audio& a){std::ofstream f(path,std::ios::binary);const auto u16=[&](unsigned x){char b[]{char(x&255),char((x>>8)&255)};f.write(b,2);};const auto u32=[&](unsigned x){char b[]{char(x&255),char((x>>8)&255),char((x>>16)&255),char((x>>24)&255)};f.write(b,4);};const unsigned bytes=unsigned(a.l.size())*4;f.write("RIFF",4);u32(36+bytes);f.write("WAVEfmt ",8);u32(16);u16(1);u16(2);u32(rate);u32(rate*4);u16(4);u16(16);f.write("data",4);u32(bytes);for(std::size_t n=0;n<a.l.size();++n){u16(unsigned(int(std::clamp(a.l[n],-1.f,1.f)*32767))&65535);u16(unsigned(int(std::clamp(a.r[n],-1.f,1.f)*32767))&65535);}if(!f)throw std::runtime_error("Could not write audition WAV");}
}
int main(int argc,char** argv){try{
    const bool levelsOnly=argc>1&&std::string_view(argv[1])=="--measure-only";
    const bool audition=argc>1&&std::string_view(argv[1])=="--auditions";
    auto prototype=std::make_unique<gba::SynthEngine>();prototype->prepare(rate);
    std::ofstream csv("factory-audit.csv");csv<<std::setprecision(10)<<"id,name,category,note,velocity,volume,max100msStereoRms,peak,dc\n";
    std::set<std::string> ids,names,categories;std::vector<Signature> signatures;
    double quietest=1,loudest=0,largestPeak=0;int rendered=0;
    for(const auto& patch:gba::factoryPatches){gba::Settings s;s.values=patch.values;const int note=middle(patch);
        check(ids.insert(patch.id).second&&names.insert(patch.name).second,"Duplicate preset identity");categories.insert(patch.category);
        bool valid=true;for(std::size_t p=0;p<gba::parameterCount;++p)valid&=std::isfinite(s.values[p])&&s.values[p]>=gba::specs[p].min&&s.values[p]<=gba::specs[p].max;check(valid,"Invalid preset domain: "+std::string(patch.name));
        auto e=engine(*prototype,s);auto a=held(*e,s,note,.9f);const auto m=measure(a);
        check(m.finite&&m.peak<=1,"Non-finite/unbounded central note: "+std::string(patch.name));check(m.rms>.0001,"Inaudible central note: "+std::string(patch.name));
        quietest=std::min(quietest,m.rms);loudest=std::max(loudest,m.rms);largestPeak=std::max(largestPeak,m.peak);
        csv<<patch.id<<",\""<<patch.name<<"\",\""<<patch.category<<"\","<<note<<",0.9,"<<s[gba::Param::Volume]<<","<<m.rms<<","<<m.peak<<","<<m.dc<<"\n";
        if(!levelsOnly){check(m.rms>=.009&&m.rms<=.05,"Central preset level is outside the calibrated playable band: "+std::string(patch.name));e->noteOff(1,note);Audio release(int(std::ceil(s[gba::Param::Release]*rate))+64);e->render(release.l.data(),release.r.data(),int(release.l.size()));check(e->activeVoices()==0&&measure(release).finite,"Preset note does not release safely");
            for(int other:{note-12,note+12}){auto v=engine(*prototype,s);auto grid=held(*v,s,other,.9f);const auto g=measure(grid);check(g.finite&&g.peak<=.951,"Preset register is non-finite or exceeds headroom: "+std::string(patch.name));check(g.rms>.0001,"Silent register: "+std::string(patch.name));csv<<patch.id<<",\""<<patch.name<<"\",\""<<patch.category<<"\","<<other<<",0.9,"<<s[gba::Param::Volume]<<","<<g.rms<<","<<g.peak<<","<<g.dc<<"\n";}
            auto soft=engine(*prototype,s);auto v=held(*soft,s,note,.4f);const auto vm=measure(v);check(vm.finite&&vm.rms<m.rms*.99&&vm.rms>m.rms*.2,"Velocity response is not playable: "+std::string(patch.name));
            csv<<patch.id<<",\""<<patch.name<<"\",\""<<patch.category<<"\","<<note<<",0.4,"<<s[gba::Param::Volume]<<","<<vm.rms<<","<<vm.peak<<","<<vm.dc<<"\n";
            auto drySettings=withoutEffects(s);auto dryEngine=engine(*prototype,drySettings);auto dryAudio=held(*dryEngine,drySettings,note,.9f);signatures.push_back(signature(patch,dryAudio,note));
        }
        if(++rendered%64==0)std::cout<<"Audited "<<rendered<<" factory patches\n"<<std::flush;
    }
    check(gba::factoryPatches.size()>=300&&categories.size()==17,"Factory coverage is incomplete");
    if(!levelsOnly){std::ofstream pairs("factory-similarity.csv");pairs<<"id,nearestId,dryAudioDistance\n";int close=0;
        for(std::size_t i=0;i<signatures.size();++i){double best=100;std::size_t nearest=i;for(std::size_t j=0;j<signatures.size();++j)if(i!=j&&signatures[i].note==signatures[j].note){const double d=distance(signatures[i],signatures[j]);if(d<best){best=d;nearest=j;}}pairs<<signatures[i].id<<","<<signatures[nearest].id<<","<<best<<"\n";if(best<.025)++close;}
        std::vector<int> parent(signatures.size());std::iota(parent.begin(),parent.end(),0);
        const auto root=[&](int v){while(parent[std::size_t(v)]!=v)v=parent[std::size_t(v)];return v;};
        for(std::size_t i=0;i<signatures.size();++i)for(std::size_t j=0;j<i;++j)if(signatures[i].note==signatures[j].note&&distance(signatures[i],signatures[j])<.025)parent[std::size_t(root(int(i)))]=root(int(j));
        std::set<int> groups;for(std::size_t i=0;i<parent.size();++i)groups.insert(root(int(i)));
        check(groups.size()>=300,"Fewer than 300 distinguishable dry spectral/envelope groups on the audit grid");
        for(std::size_t i=0;i+1<signatures.size();i+=2)check(distance(signatures[i],signatures[i+1])>.025,"Afterglow differs only weakly in dry excitation/envelope: "+signatures[i].name);
        std::cout<<"Dry spectral/envelope groups at 0.025 threshold: "<<groups.size()<<"\n";
        std::cout<<"Dry-spectrum/envelope near-neighbor flags (<0.025): "<<close<<" (listening review, not a perceptual certificate)\n";
    }
    if(audition){
        const auto excerpt=[&](const gba::FactoryPatch& p,const std::filesystem::path& directory,int number,std::ofstream& index){
            gba::Settings s;s.values=p.values;auto e=engine(*prototype,s);Audio a(rate*6);int root=middle(p);e->noteOn(1,root,.85f);e->render(a.l.data(),a.r.data(),rate*2);e->noteOff(1,root);e->noteOn(1,root+7,.6f);e->render(a.l.data()+rate*2,a.r.data()+rate*2,rate*2);e->allNotesOff();e->render(a.l.data()+rate*4,a.r.data()+rate*4,rate*2);
            const auto file=std::to_string(number)+"-"+p.id+".wav";writeWav(directory/file,a);index<<file<<" : "<<p.category<<" / "<<p.name<<"\n";
        };
        std::filesystem::create_directories("preset-auditions");std::set<std::string> done;std::ofstream index("preset-auditions/README.txt");index<<"Original synthesized audit renders. Never loaded by the instrument. Human listening acceptance remains pending.\n";int number=0;
        for(const auto& p:gba::factoryPatches)if(done.insert(p.category).second)excerpt(p,"preset-auditions",++number,index);
        std::filesystem::create_directories("vocal-auditions");std::ofstream vocalIndex("vocal-auditions/README.txt");vocalIndex<<"Original synthesized vowel textures, not speech or identifiable voices. Never loaded by the instrument. Human listening acceptance pending. Six-second excerpts may truncate tails.\n";number=0;
        for(std::string_view name:{"Smooth R&B Vocal Pad","Modern Pop Vocal Synth","Japanese City Pop Vocal Synth","Middle Eastern Pop-Inspired Vocal Synth","Dark Southern Chant Synth","Ghost Whisper Synth","Haunted Choir","16-Bit Vocal Synth","Children's Choir-Style Synth","Synthetic Vocal Chop","Dark Formant Bass","Angelic Choir Synth"}){
            auto it=std::find_if(gba::factoryPatches.begin(),gba::factoryPatches.end(),[&](const auto& p){return p.name==name;});if(it==gba::factoryPatches.end())throw std::runtime_error("Missing audition preset");excerpt(*it,"vocal-auditions",++number,vocalIndex);
        }
    }
    std::cout<<"Central-note max-100ms stereo RMS range: "<<quietest<<" to "<<loudest<<" ("<<20*std::log10(loudest/quietest)<<" dB); peak "<<largestPeak<<"\n";
    std::cout<<"All "<<checks<<" Phase 6 factory preset checks passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}}
