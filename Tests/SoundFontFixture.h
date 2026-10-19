#pragma once
#include <vector>
#include <cstdint>
#include <cmath>
#include <string>
#include <utility>
namespace sf2fixture {
using Bytes=std::vector<std::uint8_t>;
inline void word(Bytes& b,unsigned n){b.push_back(std::uint8_t(n));b.push_back(std::uint8_t(n>>8));}
inline void dword(Bytes& b,unsigned n){word(b,n);word(b,n>>16);}
inline void name(Bytes& b,const std::string& s,int count){for(int i=0;i<count;++i)b.push_back(i<int(s.size())?std::uint8_t(s[std::size_t(i)]):0);}
inline Bytes chunk(const char* id,const Bytes& data){Bytes b;name(b,id,4);dword(b,unsigned(data.size()));b.insert(b.end(),data.begin(),data.end());if(data.size()%2)b.push_back(0);return b;}
inline void append(Bytes& b,const Bytes& a){b.insert(b.end(),a.begin(),a.end());}
inline Bytes list(const char* type,const Bytes& data){Bytes b;name(b,type,4);append(b,data);return chunk("LIST",b);}
inline Bytes bank(){
    Bytes info;append(info,chunk("ifil",Bytes{2,0,1,0}));append(info,chunk("INAM",Bytes{'G','B','A','2',' ','T','e','s','t',0}));
    constexpr unsigned length=8820,stride=length+46;Bytes pcm;
    for(int sample=0;sample<2;++sample){for(unsigned i=0;i<length;++i){double p=6.283185307179586*1000*i/44100;double v=sample==0?std::sin(p)*.65:std::sin(p)*.3+std::sin(p*2)*.22+std::sin(p*3)*.15;word(pcm,unsigned(std::int16_t(v*32767)));}for(int i=0;i<46;++i)word(pcm,0);}
    Bytes sdta;append(sdta,chunk("smpl",pcm));Bytes phdr,pbag,pgen,inst,ibag,igen,shdr;
    for(int p=0;p<3;++p){name(phdr,p==0?"Fixture Sine":p==1?"Fixture Bright":"EOP",20);word(phdr,p==1?10:0);word(phdr,p==1?2:0);word(phdr,unsigned(p));dword(phdr,0);dword(phdr,0);dword(phdr,0);word(pbag,unsigned(p));word(pbag,0);word(pgen,p<2?41:0);word(pgen,p<2?unsigned(p):0);}
    for(int i=0;i<3;++i){name(inst,i<2?"Original Test Instrument":"EOI",20);word(inst,unsigned(i));word(ibag,unsigned(i*5));word(ibag,0);}
    for(int i=0;i<2;++i){for(auto gen:std::vector<std::pair<unsigned,int>>{{34,-12000},{38,-3600},{54,1},{17,i?250:0},{53,i}}){word(igen,gen.first);word(igen,unsigned(gen.second));}}
    word(igen,0);word(igen,0);
    for(int i=0;i<3;++i){unsigned start=unsigned(i)*stride;name(shdr,i<2?"Original Synthetic Wave":"EOS",20);dword(shdr,i<2?start:0);dword(shdr,i<2?start+length:0);dword(shdr,i<2?start:0);dword(shdr,i<2?start+length:0);dword(shdr,i<2?44100:0);shdr.push_back(i<2?69:0);shdr.push_back(0);word(shdr,0);word(shdr,i<2?1:0);}
    Bytes pdta;append(pdta,chunk("phdr",phdr));append(pdta,chunk("pbag",pbag));append(pdta,chunk("pmod",Bytes(10)));append(pdta,chunk("pgen",pgen));append(pdta,chunk("inst",inst));append(pdta,chunk("ibag",ibag));append(pdta,chunk("imod",Bytes(10)));append(pdta,chunk("igen",igen));append(pdta,chunk("shdr",shdr));
    Bytes body;name(body,"sfbk",4);append(body,list("INFO",info));append(body,list("sdta",sdta));append(body,list("pdta",pdta));return chunk("RIFF",body);
}
}
