#include "Sf2Validation.h"
#include "SoundFont.h"
#include <array>
#include <cstring>
#include <cstdint>
#include <algorithm>
namespace gba {
namespace {
using Byte=unsigned char;
std::uint16_t u16(const Byte* p){return std::uint16_t(p[0]|(unsigned(p[1])<<8));}
std::uint32_t u32(const Byte* p){return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);}
bool is(const Byte* p,const char* s){return std::memcmp(p,s,4)==0;}
struct Table {const Byte* p=nullptr;std::size_t count=0;};
}
bool validateSf2(const void* raw,std::size_t size,std::string& error,std::size_t* sampleCount){
    auto fail=[&](const char* text){error=text;return false;};
    if(!raw||size<12||size>SoundFont::maxBytes)return fail("Choose a complete SF2 file of at most 128 MiB.");
    auto* p=static_cast<const Byte*>(raw);
    if(!is(p,"RIFF")||!is(p+8,"sfbk")||std::uint64_t(u32(p+4))+8!=size)return fail("Not a complete RIFF SoundFont 2 bank (SF3, ROM and game files are unsupported).");
    const std::array<const char*,9> names{{"phdr","pbag","pmod","pgen","inst","ibag","imod","igen","shdr"}};
    const std::array<std::size_t,9> strides{{38,4,10,4,22,4,10,4,46}};
    std::array<Table,9> t{};std::size_t samples=0;bool sdta=false,pdta=false;
    for(std::size_t pos=12;pos<size;){
        if(size-pos<8)return fail("Truncated SF2 chunk header.");
        std::size_t n=u32(p+pos+4),end=pos+8+n;
        if(end>size||end<pos)return fail("SF2 chunk exceeds the file.");
        if(is(p+pos,"LIST")){
            if(n<4)return fail("Invalid SF2 list.");
            const bool data=is(p+pos+8,"sdta"),presets=is(p+pos+8,"pdta");
            if((data&&sdta)||(presets&&pdta))return fail("Duplicate SF2 data lists.");
            sdta|=data;pdta|=presets;
            if(data||presets)for(std::size_t at=pos+12;at<end;){
                if(end-at<8)return fail("Truncated SF2 table.");
                std::size_t bytes=u32(p+at+4),next=at+8+bytes;
                if(next>end||next<at)return fail("SF2 table exceeds its list.");
                if(data&&is(p+at,"smpl")){
                    if(samples||bytes<4||bytes%2)return fail("Invalid PCM sample table.");
                    samples=bytes/2;
                }
                if(presets)for(std::size_t i=0;i<names.size();++i)if(is(p+at,names[i])){
                    if(t[i].p||!bytes||bytes%strides[i]||bytes/strides[i]>65536)return fail("Invalid or duplicate SF2 table.");
                    t[i]={p+at+8,bytes/strides[i]};
                }
                // TinySoundFont's RIFF reader does not consume odd table padding.
                if(bytes%2)return fail("Unsupported odd-sized SF2 synthesis table.");
                at=next;
            }
        }
        pos=end+(n&1);if(pos>size)return fail("Missing RIFF padding.");
    }
    if(!samples||!sdta||!pdta)return fail("SoundFont contains no PCM samples or preset data.");
    for(auto table:t)if(!table.p)return fail("Incomplete SoundFont preset tables.");
    if(t[0].count<2||t[0].count>4097||t[4].count<2||t[8].count<2)return fail("SoundFont has no playable instruments or exceeds 4096 presets.");
    const auto indexes=[&](int a,std::size_t stride,std::size_t offset,int b){
        unsigned previous=0;
        for(std::size_t i=0;i<t[std::size_t(a)].count;++i){unsigned value=u16(t[std::size_t(a)].p+i*stride+offset);if(value<previous||value>=t[std::size_t(b)].count)return false;previous=value;}
        return true;
    };
    if(!indexes(0,38,24,1)||!indexes(4,22,20,5)||!indexes(1,4,0,3)||!indexes(1,4,2,2)||!indexes(5,4,0,7)||!indexes(5,4,2,6))return fail("Out-of-range or decreasing SoundFont zone indices.");
    for(int index:{3,7})for(std::size_t i=0;i+1<t[std::size_t(index)].count;++i){
        auto* g=t[std::size_t(index)].p+i*4;auto op=u16(g),value=u16(g+2);
        if((op==41&&(index!=3||value>=t[4].count-1))||(op==53&&(index!=7||value>=t[8].count-1)))return fail("Invalid SoundFont instrument/sample reference.");
        if((op==43||op==44)&&(g[2]>g[3]||g[3]>127))return fail("Invalid SoundFont key/velocity range.");
    }
    for(std::size_t i=0;i+1<t[8].count;++i){
        auto* h=t[8].p+i*46;auto start=u32(h+20),end=u32(h+24),rate=u32(h+36);auto type=u16(h+44);
        if(start>=end||end>=samples||rate<400||rate>384000||(type&0x8010)!=0)return fail("Invalid, compressed or ROM-backed SoundFont sample.");
    }
    // Bound the cross-product before the decoder allocates expanded regions.
    std::size_t total=0,work=0;
    for(std::size_t preset=0;preset+1<t[0].count;++preset){
        std::size_t count=0;
        for(unsigned bag=u16(t[0].p+preset*38+24),last=u16(t[0].p+(preset+1)*38+24);bag<last;++bag)
            for(unsigned gen=u16(t[1].p+bag*4),lastGen=u16(t[1].p+(bag+1)*4);gen<lastGen;++gen){
                if(++work>4000000)return fail("SoundFont zone graph is too complex.");
                auto* g=t[3].p+gen*4;if(u16(g)!=41)continue;auto inst=u16(g+2);
                for(unsigned ib=u16(t[4].p+inst*22+20),lastIb=u16(t[4].p+(inst+1)*22+20);ib<lastIb;++ib){
                    if(++work>4000000)return fail("SoundFont zone graph is too complex.");
                    for(unsigned ig=u16(t[5].p+ib*4),lastIg=u16(t[5].p+(ib+1)*4);ig<lastIg;++ig){
                        if(++work>4000000)return fail("SoundFont zone graph is too complex.");
                        if(u16(t[7].p+ig*4)==53&&++count>1024)return fail("A SoundFont preset exceeds 1024 sample regions.");
                    }
                }
            }
        total+=count;if(total>65536)return fail("SoundFont exceeds 65536 expanded regions.");
    }
    if(sampleCount)*sampleCount=samples;
    return true;
}
}
