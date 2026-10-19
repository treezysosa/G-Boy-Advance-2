#include "dsp/SoundFont.h"
#include "dsp/Sf2Validation.h"
#include "SoundFontFixture.h"
#include <iostream>
#include <stdexcept>
#include <array>
#include <algorithm>
namespace {int checks=0;void check(bool ok,const char* m){++checks;if(!ok)throw std::runtime_error(m);}
gba::Settings settings(){gba::Settings s;s.set(gba::Param::Cutoff,20000);s.set(gba::Param::Resonance,0);s.set(gba::Param::Attack,.001f);s.set(gba::Param::Reverb,0);s.set(gba::Param::Delay,0);s.set(gba::Param::Chorus,0);s.set(gba::Param::Volume,.7f);return s;}
double render(gba::SoundFont& f,int frames){std::array<float,256> l{},r{};double sum=0;while(frames>0){int n=std::min(256,frames);f.render(l.data(),r.data(),n);for(int i=0;i<n;++i){check(std::isfinite(l[std::size_t(i)])&&std::isfinite(r[std::size_t(i)]),"Non-finite SF2 audio");sum+=l[std::size_t(i)]*l[std::size_t(i)]+r[std::size_t(i)]*r[std::size_t(i)];}frames-=n;}return sum;}}
int main(){try{const auto bytes=sf2fixture::bank();std::string error;
    check(gba::validateSf2(bytes.data(),bytes.size(),error),"Original fixture is not a valid SF2 bank");
    for(double rate:{44100.,48000.,96000.}){gba::SoundFont f;check(f.load(bytes.data(),bytes.size(),error),error.c_str());check(f.instruments().size()==2&&f.instruments()[1].bank==2&&f.instruments()[1].program==10,"SF2 instrument/bank names missing");f.setSettings(settings());f.prepare(rate);
        const auto heap=gba::SoundFont::heapOperations();f.noteOn(1,69,.8f);check(render(f,int(rate*.1))>.05,"SF2 MIDI produced no audio");
        f.controller(1,64,127);f.noteOff(1,69);check(render(f,int(rate*.25))>.05&&f.activeVoices()>0,"SF2 sustain failed");f.controller(1,64,0);render(f,int(rate*.5));check(f.activeVoices()==0,"SF2 pedal release left a stuck voice");
        f.noteOn(1,69,.9f,true);f.noteOn(1,72,.8f);f.stopPreview();render(f,int(rate*.3));check(f.activeVoices()>0,"Stopping preview cut a normal MIDI note");f.reset();
        f.noteOn(1,69,.8f,true);render(f,int(rate));check(f.activeVoices()==0,"SF2 preview did not auto-release");
        f.select(1);f.noteOn(16,81,.8f);check(render(f,4096)>.01,"Second SF2 instrument/channel 16 was silent");f.pitchWheel(16,16383);f.controller(16,1,127);f.controller(16,10,10);render(f,512);f.controller(16,123,0);render(f,int(rate*.5));check(f.activeVoices()==0,"SF2 all-notes-off failed");
        for(int i=0;i<80;++i){f.noteOn(1,40+i%50,.8f);render(f,32);check(f.activeVoices()<=32,"SF2 voice budget exceeded");}check(f.activeVoices()>0,"SF2 exhausted voices and stopped playing");f.reset();
        auto silent=settings();silent.set(gba::Param::Volume,0);f.setSettings(silent);render(f,int(rate*.2));f.noteOn(1,69,.8f);check(render(f,int(rate*.1))<1e-7,"SF2 master volume zero leaked audio");f.reset();
        check(gba::SoundFont::heapOperations()==heap,"SF2 audio/MIDI/reset allocated or freed heap memory");
    }
    for(std::size_t n:{std::size_t(0),std::size_t(11),bytes.size()-1})check(!gba::validateSf2(bytes.data(),n,error),"Truncated SF2 was accepted");
    for(const char* table:{"phdr","pbag","inst","ibag","pgen","igen"}){auto invalid=bytes;auto found=std::search(invalid.begin(),invalid.end(),table,table+4);check(found!=invalid.end(),"Missing fixture table");auto pos=std::size_t(found-invalid.begin())+8;std::size_t off=std::string(table)=="phdr"?24:std::string(table)=="inst"?20:std::string(table)=="pgen"||std::string(table)=="igen"?2:0;if(std::string(table)=="igen"){invalid[pos]=53;invalid[pos+1]=0;}invalid[pos+off]=255;invalid[pos+off+1]=255;check(!gba::validateSf2(invalid.data(),invalid.size(),error),"Out-of-range SF2 index was accepted");}
    std::uint32_t random=0x534632u;int decodedMutations=0;
    for(int i=0;i<256;++i){auto changed=bytes;
        for(int k=0;k<1+i%6;++k){random^=random<<13;random^=random>>17;random^=random<<5;auto pos=changed.size()-900+random%900;changed[pos]^=std::uint8_t((random>>24)|1);}
        gba::SoundFont mutated;if(mutated.load(changed.data(),changed.size(),error)){++decodedMutations;mutated.setSettings(settings());mutated.prepare(48000);mutated.noteOn(1,60,.8f);render(mutated,256);}
    }
    check(decodedMutations>0,"Structured SF2 mutation test did not reach the decoder");
    std::cout<<"PASS "<<checks<<" SoundFont DSP checks: SF2 banks/instruments, three rates, audio, MIDI, sustain, preview, voice limits, volume, allocation-free processing and malformed indices.\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}}
