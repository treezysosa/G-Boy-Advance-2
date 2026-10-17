#include "dsp/SynthEngine.h"
#include "FactoryData.h"
#include <atomic>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
extern std::atomic<bool> trackAllocations;
extern std::atomic<int> allocations;
namespace {
int checks=0;
void check(bool ok,const char* text){++checks;if(!ok)throw std::runtime_error(text);}
struct Audio{std::vector<float> l,r;explicit Audio(int n):l(std::size_t(n)),r(std::size_t(n)) {}};
void render(gba::SynthEngine& e,Audio& a){e.render(a.l.data(),a.r.data(),int(a.l.size()));}
double energy(const std::vector<float>& a){double sum=0;for(float x:a)sum+=double(x)*x;return sum/double(a.size());}
double component(const std::vector<float>& a,double hz,double rate){double re=0,im=0,weight=0;for(std::size_t n=0;n<a.size();++n){const double w=.5-.5*std::cos(2*std::numbers::pi_v<double>*double(n)/double(a.size()-1)),phase=2*std::numbers::pi_v<double>*hz*double(n)/rate;re+=a[n]*w*std::cos(phase);im+=a[n]*w*std::sin(phase);weight+=w;}return 2*std::hypot(re,im)/weight;}
gba::Settings clean(){gba::Settings s;s.set(gba::Param::Engine,6);s.set(gba::Param::Wave1,3);s.set(gba::Param::Wave2,3);s.set(gba::Param::OscMix,0);s.set(gba::Param::Detune,0);s.set(gba::Param::Coarse,0);s.set(gba::Param::Attack,.001f);s.set(gba::Param::Sustain,1);s.set(gba::Param::Release,.06f);s.set(gba::Param::FilterEnv,0);s.set(gba::Param::Cutoff,20000);s.set(gba::Param::Resonance,0);s.set(gba::Param::Lfo1Depth,0);s.set(gba::Param::Lfo2Depth,0);s.set(gba::Param::Vibrato,0);s.set(gba::Param::Reverb,0);s.set(gba::Param::Chorus,0);s.set(gba::Param::Drive,0);return s;}
Audio note(gba::SynthEngine& e,const gba::Settings& s,int pitch,double rate,int frames){e.setSettings(s);e.prepare(rate);e.noteOn(1,pitch,.9f);Audio warm(int(rate*.04));render(e,warm);Audio a(frames);render(e,a);return a;}
void noiseRegisters(){
    for(int bits:{7,15}){const std::uint32_t seed=(1u<<bits)-1;auto state=seed;const int period=(1<<bits)-1;bool noEarlyRepeat=true,nonzero=true;int ones=0;
        for(int n=1;n<=period;++n){ones+=int(state&1u);gba::RetroEngine::ClockedNoise::step(state,bits);if(n<period)noEarlyRepeat&=state!=seed;nonzero&=state>0&&state<(1u<<bits);}
        check(noEarlyRepeat&&state==seed,"LFSR period is not the expected 127/32767 steps");check(nonzero&&ones==(1<<(bits-1)),"LFSR locks or lacks maximal-sequence balance");
    }
    for(double rate:{44100.,48000.,96000.}){int clocks[2]{};bool finite=true;for(int octave=0;octave<2;++octave){gba::RetroEngine::ClockedNoise n;for(int i=0;i<int(rate);++i){auto prior=n.longRegister;const float out=n.next(float(220*(octave+1)/rate),.4f);clocks[octave]+=prior!=n.longRegister;finite&=std::isfinite(out)&&std::abs(out)<=1;}}
        check(finite,"Clocked noise is non-finite/unbounded");check(std::abs(clocks[0]-3520)<=1&&std::abs(clocks[1]-7040)<=1,"Noise clock does not track pitch/sample rate");
    }
    gba::RetroEngine::ClockedNoise n;bool bounded=true;for(int i=0;i<100000;++i){n.next(100,.5f);bounded&=n.phase>=0&&n.phase<1;}check(bounded,"Noise timebase grows outside its bounded phase");
    std::cout<<"PASS 7/15-bit maximal sequences, balance, pitched clock at three rates and bounded timebase\n";
}
void waveforms(){
    for(int shape:{0,1,2}){std::set<int> levels;bool bounded=true;for(int i=0;i<8192;++i){float x=gba::RetroEngine::wave(shape,float(i)/8192,1.f/8192,.5f,0);bounded&=std::isfinite(x)&&std::abs(x)<=1;levels.insert(int(std::round((x+1)*10000)));}check(bounded,"Digital waveform is unbounded");check(levels.size()==std::size_t(shape==2?32:16),"Sine/triangle/saw DAC level count is wrong");}
    for(float width:{.125f,.25f,.5f,.75f}){double mean=0;for(int i=0;i<8192;++i)mean+=gba::RetroEngine::wave(4,float(i)/8192,1.f/8192,width,0);check(std::abs(mean/8192-(2*width-1))<.001,"Pulse width does not set duty cycle");}
    for(double rate:{44100.,48000.,96000.}){const double hz=gba::midiHz(rate>48000?117.f:105.f);float p=0,dt=float(hz/rate);std::vector<float> blep(std::size_t(int(rate*.3))),naive(blep.size());for(std::size_t i=0;i<blep.size();++i){p=gba::wrap(p+dt);blep[i]=gba::RetroEngine::wave(3,p,dt,.5f,0);naive[i]=p<.5f?1.f:-1.f;}const double folded=std::abs(std::remainder(hz*7,rate));
        check(component(blep,folded,rate)<component(naive,folded,rate)*.4,"Pulse edge correction fails to reduce the measured folded seventh harmonic");}
    std::cout<<"PASS DAC steps, four pulse duties and measured BLEP pulse alias reduction at three rates\n";
}
void musicalControls(gba::SynthEngine& e){
    for(double rate:{44100.,48000.,96000.}){auto s=clean();auto single=note(e,s,69,rate,int(rate*.5));check(component(single.l,440,rate)>.03,"Retro oscillator is silent/detuned");
        s.set(gba::Param::OscMix,.5f);s.set(gba::Param::Coarse,12);auto dual=note(e,s,69,rate,int(rate*.5));check(component(dual.l,880,rate)>component(single.l,880,rate)*20&&component(dual.l,880,rate)>.02,"Second retro oscillator does not track blend/coarse tuning");
        s=clean();s.set(gba::Param::Wave1,4);s.set(gba::Param::PulseWidth,.125f);auto pulse=note(e,s,69,rate,int(rate*.5));check(component(pulse.l,880,rate)>component(pulse.l,440,rate)*.5,"Pulse duty spectrum is inert");
        s=clean();s.set(gba::Param::Brightness,0);auto dark=note(e,s,69,rate,int(rate*.5));s.set(gba::Param::Brightness,1);auto bright=note(e,s,69,rate,int(rate*.5));check(component(bright.l,3080,rate)>component(dark.l,3080,rate)*2,"Retro brightness does not alter the tone filter");
        e.pitchWheel(1,16383);Audio bent(int(rate*.5));render(e,bent);const double target=440*std::exp2(2./12);check(component(bent.l,target,rate)>.03&&component(bent.l,440,rate)<.001,"Retro pitch bend does not retune the audio");
        e.noteOff(1,69);Audio release(int(rate*.1));render(e,release);check(e.activeVoices()==0,"Retro notes do not release");
    }
    auto s=clean();auto a=note(e,s,60,48000,20000);e.setSettings(s);e.prepare(48000);e.noteOn(1,60,.9f);Audio warm(1920);render(e,warm);Audio b(20000);std::size_t at=0;while(at<b.l.size()){int count=int(std::min<std::size_t>(127,b.l.size()-at));e.render(b.l.data()+at,b.r.data()+at,count);at+=std::size_t(count);}check(a.l==b.l&&a.r==b.r,"Retro output depends on host block partitioning");
    s.set(gba::Param::Voices,32);e.setSettings(s);e.prepare(96000);for(int n=0;n<32;++n)e.noteOn(1,40+n,.8f);Audio block(64);allocations=0;trackAllocations=true;bool finite=true;
    for(int i=0;i<400;++i){s.set(gba::Param::Wave1,float(i%6));s.set(gba::Param::Wave2,float((i+2)%6));s.set(gba::Param::PulseWidth,.05f+.9f*float(i%17)/16);s.set(gba::Param::OscMix,float(i%5)/4);s.set(gba::Param::Brightness,float(i%11)/10);e.setSettings(s);render(e,block);for(float x:block.l)finite&=std::isfinite(x)&&std::abs(x)<=1;}
    trackAllocations=false;check(finite,"32-voice retro automation is non-finite/unbounded");check(allocations==0,"Retro render/settings allocates on the audio thread");
    std::cout<<"PASS dual oscillator, duty/brightness, bend/release, block invariance and allocation-free 32-voice automation\n";
}
void factoryRegisters(){int count=0;std::set<std::string> styles;for(double rate:{44100.,96000.}){auto prototype=std::make_unique<gba::SynthEngine>();prototype->prepare(rate);for(const auto& p:gba::factoryPatches)if(std::string_view(p.category)=="Retro Games"){
        ++count;for(std::string style:{"8-bit handheld","16-bit FM","16-bit wavetable aesthetic","early 3D","late-90s soundtrack","early-2000s handheld","arcade"})if(std::string_view(p.tags).find(style)!=std::string_view::npos)styles.insert(style);
        gba::Settings s;s.values=p.values;for(int pitch:{48,72}){auto e=std::make_unique<gba::SynthEngine>(*prototype);e->setSettings(s);e->reset();e->noteOn(1,pitch,.8f);Audio a(int(rate*.3));render(*e,a);bool finite=true;for(float x:a.l)finite&=std::isfinite(x)&&std::abs(x)<=.951;check(finite&&energy(a.l)>1e-9,"Retro factory patch is silent/unbounded in extended rate/register grid");}
    }}check(count==112&&styles.size()==7,"Retro coverage must include 56 presets and seven technical styles at two rates");std::cout<<"PASS all 56 retro presets at 44.1/96 kHz in low/high registers and seven technical styles\n";}
}
int main(){try{noiseRegisters();waveforms();auto e=std::make_unique<gba::SynthEngine>();musicalControls(*e);factoryRegisters();std::cout<<"All "<<checks<<" Phase 8 retro synthesis checks passed.\n";return 0;}catch(const std::exception& ex){trackAllocations=false;std::cerr<<"FAIL: "<<ex.what()<<"\n";return 1;}}
