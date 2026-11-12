#include "dsp/SynthEngine.h"
#include "FactoryData.h"
#include <bit>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

// An offline authoring tool, never linked into the instrument. Every factory
// sound receives exactly the same MIDI phrase, with its own voice mode intact.
namespace {
constexpr int rate=48000, duration=20, block=256;
struct Event {int frame,note;float velocity;};
constexpr std::array<Event,16> events {{{0,48,.45f},{38400,48,0},
    {62400,60,.45f},{100800,60,0},{124800,60,.9f},{268800,60,0},
    {297600,72,.75f},{336000,72,0},
    {364800,48,.7f},{364800,52,.7f},{364800,55,.7f},{364800,60,.7f},
    {556800,48,0},{556800,52,0},{556800,55,0},{556800,60,0}}};
void u16(std::ostream& f,std::uint16_t x){const char b[]{char(x&255),char(x>>8)};f.write(b,2);}
void u32(std::ostream& f,std::uint32_t x){const char b[]{char(x&255),char((x>>8)&255),char((x>>16)&255),char(x>>24)};f.write(b,4);}
}
int main(int argc,char** argv){try{
    int index=-1;std::filesystem::path output;
    for(int i=1;i<argc;++i){const std::string arg=argv[i];
        if(arg=="--index"&&i+1<argc){std::size_t end=0;const std::string value=argv[++i];index=std::stoi(value,&end);if(end!=value.size())throw std::runtime_error("Invalid index");}
        else if(arg=="--output"&&i+1<argc)output=argv[++i];
        else throw std::runtime_error("Usage: gba2-preset-audition --index N --output file.wav");
    }
    if(index<0||std::size_t(index)>=gba::factoryPatches.size()||output.empty())throw std::runtime_error("A valid factory index and output file are required");
    const auto& patch=gba::factoryPatches[std::size_t(index)];
    auto engine=std::make_unique<gba::SynthEngine>();engine->prepare(rate);
    gba::Settings settings;settings.values=patch.values;engine->setSettings(settings);engine->reset();
    // Refuse to overwrite existing work, including any personal audio.
    if(std::filesystem::exists(output))throw std::runtime_error("Output already exists");
    std::ofstream f(output,std::ios::binary);if(!f)throw std::runtime_error("Cannot create output");
    constexpr std::uint32_t bytes=rate*duration*2*4;
    f.write("RIFF",4);u32(f,36+bytes);f.write("WAVEfmt ",8);u32(f,16);u16(f,3);u16(f,2);
    u32(f,rate);u32(f,rate*8);u16(f,8);u16(f,32);f.write("data",4);u32(f,bytes);
    std::array<float,block> left{},right{};std::size_t next=0;double energy=0,peak=0;
    for(int frame=0;frame<rate*duration;){
        while(next<events.size()&&events[next].frame==frame){const auto& e=events[next++];if(e.velocity>0)engine->noteOn(1,e.note,e.velocity);else engine->noteOff(1,e.note);}
        const int stop=next<events.size()?events[next].frame:rate*duration;
        const int count=std::min(block,stop-frame);
        if(count<=0)throw std::runtime_error("Invalid phrase schedule");
        engine->render(left.data(),right.data(),count);
        for(int n=0;n<count;++n)for(float sample:{left[std::size_t(n)],right[std::size_t(n)]}){
            if(!std::isfinite(sample)||std::abs(sample)>1.0f)throw std::runtime_error("Non-finite or unbounded audition audio");
            energy+=double(sample)*sample;peak=std::max(peak,std::abs(double(sample)));u32(f,std::bit_cast<std::uint32_t>(sample));
        }
        frame+=count;
    }
    f.close();if(!f||energy<1e-10)throw std::runtime_error("Incomplete or silent audition");
    std::cout<<patch.id<<" peak="<<peak<<" frames="<<rate*duration<<"\n";
    return 0;
}catch(const std::exception& ex){std::cerr<<"FAIL: "<<ex.what()<<"\n";return 1;}}
