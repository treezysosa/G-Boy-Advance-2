#pragma once
#include "SoundFontFixture.h"

namespace sf2stress {
// Original mathematical waves only. The bank is test input, never distributed
// as a factory instrument. Four zones per key exercise layering and modulation.
inline sf2fixture::Bytes bank(bool dynamic) {
    using namespace sf2fixture;
    Bytes info;append(info,chunk("ifil",Bytes{2,0,1,0}));append(info,chunk("INAM",Bytes{'G','B','A','2',' ','S','t','r','e','s','s',0}));
    constexpr unsigned length=8192,stride=length+46;Bytes pcm;
    for(int sample=0;sample<2;++sample) {
        for(unsigned n=0;n<length;++n) {
            const double phase=6.283185307179586*16*n/length;
            const double value=sample?std::sin(phase)*.25+std::sin(phase*2)*.15+std::sin(phase*3)*.10:std::sin(phase)*.65;
            word(pcm,unsigned(std::int16_t(value*32767)));
        }
        for(int n=0;n<46;++n)word(pcm,0);
    }
    Bytes sdta;append(sdta,chunk("smpl",pcm));Bytes phdr,pbag,pgen,inst,ibag,igen,shdr;
    for(int program=0;program<3;++program) {
        name(phdr,program==0?"Layered Pad":program==1?"Layered Strings":"EOP",20);
        word(phdr,program<2?unsigned(program):0);word(phdr,0);word(phdr,unsigned(program));
        dword(phdr,0);dword(phdr,0);dword(phdr,0);word(pbag,unsigned(program));word(pbag,0);
        word(pgen,program<2?41:0);word(pgen,program<2?unsigned(program):0);
    }
    unsigned generators=0;
    for(int instrument=0;instrument<2;++instrument) {
        name(inst,instrument?"Original layered strings":"Original layered pad",20);word(inst,unsigned(instrument*4));
        for(int layer=0;layer<4;++layer) {
            word(ibag,generators);word(ibag,0);
            std::vector<std::pair<unsigned,int>> values{{34,-2400},{36,-1200},{37,350},{38,1200},
                {8,8200-layer*220},{9,80},{17,-330+220*layer},{52,(layer-2)*7},{54,1},{58,60}};
            if(dynamic)for(auto value:std::vector<std::pair<unsigned,int>>{{5,7},{6,17},{10,700},{11,1600},{13,20},
                {21,-12000},{22,-900},{23,-12000},{24,100},{26,-1200},{28,0},{29,400},{30,1200}})values.push_back(value);
            values.push_back({53,(instrument+layer)%2});
            for(auto [generator,value]:values){word(igen,generator);word(igen,unsigned(value));++generators;}
        }
    }
    name(inst,"EOI",20);word(inst,8);word(ibag,generators);word(ibag,0);word(igen,0);word(igen,0);
    for(int sample=0;sample<3;++sample) {
        const unsigned start=unsigned(sample)*stride;name(shdr,sample<2?"Original generated wave":"EOS",20);
        dword(shdr,sample<2?start:0);dword(shdr,sample<2?start+length:0);
        dword(shdr,sample<2?start:0);dword(shdr,sample<2?start+length:0);
        dword(shdr,sample<2?48000:0);shdr.push_back(sample<2?60:0);shdr.push_back(0);word(shdr,0);word(shdr,sample<2?1:0);
    }
    Bytes pdta;append(pdta,chunk("phdr",phdr));append(pdta,chunk("pbag",pbag));append(pdta,chunk("pmod",Bytes(10)));
    append(pdta,chunk("pgen",pgen));append(pdta,chunk("inst",inst));append(pdta,chunk("ibag",ibag));
    append(pdta,chunk("imod",Bytes(10)));append(pdta,chunk("igen",igen));append(pdta,chunk("shdr",shdr));
    Bytes body;name(body,"sfbk",4);append(body,list("INFO",info));append(body,list("sdta",sdta));append(body,list("pdta",pdta));return chunk("RIFF",body);
}
}
