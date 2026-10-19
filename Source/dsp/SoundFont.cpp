#include "SoundFont.h"
#include "Sf2Validation.h"
#include <atomic>
#include <cstdlib>
#include <limits>
namespace {
std::atomic<std::uint64_t> sfHeap{0};
void* sfMalloc(std::size_t n){++sfHeap;return std::malloc(n);}
void* sfRealloc(void* p,std::size_t n){++sfHeap;return std::realloc(p,n);}
void sfFree(void* p){if(p)++sfHeap;std::free(p);}
}
#define TSF_MALLOC sfMalloc
#define TSF_REALLOC sfRealloc
#define TSF_FREE sfFree
#define TSF_NO_STDIO
#define TSF_STATIC
#define TSF_IMPLEMENTATION
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif
#include "../third_party/tsf.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
namespace gba {
struct SoundFont::Impl {
    tsf* font=nullptr;std::vector<Instrument> programs;
    Effects effects;std::array<SVF,2> filters{};
    Settings target,current;float rate=48000,volume=.72f,phase=0;
    std::array<float,32> glide{};std::array<int,17> lastKey{};std::array<float,17> wheel{};
    std::array<float,128> buffer{};int previewRemaining=0,selected=0,fade=0;
    float lastL=0,lastR=0,tailL=0,tailR=0,volumeStep=.002f;
    ~Impl(){if(font)tsf_close(font);}
    void reset() noexcept {
        if(font){for(int i=0;i<font->voiceNum;++i)tsf_voice_kill(&font->voices[i]);
            for(int c=0;c<17;++c){tsf_channel_midi_control(font,c,121,0);tsf_channel_set_sustain(font,c,0);tsf_channel_set_pitchwheel(font,c,8192);}}
        effects.reset();for(auto& f:filters)f.clear();glide.fill(0);lastKey.fill(-1);wheel.fill(0);previewRemaining=0;lastL=lastR=0;fade=0;
    }
    void transition() noexcept {tailL=lastL;tailR=lastR;fade=240;}
};
SoundFont::SoundFont():impl(std::make_unique<Impl>()){}
SoundFont::~SoundFont()=default;
std::uint64_t SoundFont::heapOperations() noexcept{return sfHeap.load();}
const std::vector<SoundFont::Instrument>& SoundFont::instruments() const noexcept{return impl->programs;}
bool SoundFont::load(const void* data,std::size_t bytes,std::string& error){
    std::size_t sampleCount=0;
    if(!validateSf2(data,bytes,error,&sampleCount))return false;
    tsf* f=tsf_load_memory(data,int(bytes));if(!f){error="The SoundFont decoder could not load this bank.";return false;}
    for(int p=0;p<f->presetNum;++p)for(int n=0;n<f->presets[p].regionNum;++n){auto& r=f->presets[p].regions[n];
        r.end=std::min(r.end,unsigned(sampleCount-1));
        if(r.offset>=r.end||r.end<2||r.offset>=sampleCount-1){tsf_close(f);error="Invalid SoundFont sample address after applying generators.";return false;}
        r.pan=std::clamp(r.pan,-.5f,.5f);r.attenuation=std::clamp(r.attenuation,0.0f,144.0f);
        r.pitch_keycenter=std::clamp(r.pitch_keycenter,0,127);r.transpose=std::clamp(r.transpose,-120,120);r.tune=std::clamp(r.tune,-1200,1200);
        r.loop_start=std::clamp(r.loop_start,r.offset,r.end-1);r.loop_end=std::clamp(r.loop_end,r.loop_start,r.end-1);
        for(auto* env:{&r.ampenv,&r.modenv}){
            env->delay=std::clamp(env->delay,0.0f,30.0f);env->attack=std::clamp(env->attack,0.0f,30.0f);env->release=std::clamp(env->release,0.0f,30.0f);
            env->keynumToHold=std::clamp(env->keynumToHold,-120.0f,120.0f);env->keynumToDecay=std::clamp(env->keynumToDecay,-120.0f,120.0f);
            env->hold=env->keynumToHold?std::clamp(env->hold,-12000.0f,11000-67*std::abs(env->keynumToHold)):std::clamp(env->hold,0.0f,600.0f);
            env->decay=env->keynumToDecay?std::clamp(env->decay,-12000.0f,11000-67*std::abs(env->keynumToDecay)):std::clamp(env->decay,0.0f,600.0f);
            env->sustain=std::clamp(env->sustain,0.0f,1.0f);
        }
        r.freqModLFO=std::clamp(r.freqModLFO,-16000,4500);r.freqVibLFO=std::clamp(r.freqVibLFO,-16000,4500);
        r.modLfoToPitch=std::clamp(r.modLfoToPitch,-12000,12000);r.vibLfoToPitch=std::clamp(r.vibLfoToPitch,-12000,12000);r.modEnvToPitch=std::clamp(r.modEnvToPitch,-12000,12000);
        r.modLfoToVolume=std::clamp(r.modLfoToVolume,-96,96);r.initialFilterQ=std::clamp(r.initialFilterQ,0,960);r.pitch_keytrack=std::clamp(r.pitch_keytrack,-1200,1200);
        r.delayModLFO=std::clamp(r.delayModLFO,0.0f,30.0f);r.delayVibLFO=std::clamp(r.delayVibLFO,0.0f,30.0f);
    }
    if(!tsf_set_max_voices(f,32)){tsf_close(f);error="Could not allocate SoundFont voices.";return false;}
    for(int c=0;c<17;++c)if(!tsf_channel_set_presetindex(f,c,0)){tsf_close(f);error="Could not prepare SoundFont MIDI channels.";return false;}
    if(impl->font)tsf_close(impl->font);
    impl->font=f;impl->programs.clear();
    for(int i=0;i<f->presetNum;++i)impl->programs.push_back({tsf_get_presetname(f,i),f->presets[i].bank,f->presets[i].preset});
    prepare(impl->rate);select(0);return true;
}
void SoundFont::prepare(double rate){auto& d=*impl;d.rate=float(std::clamp(rate,8000.,192000.));d.effects.prepare(d.rate);d.volumeStep=1-std::exp(-1/(d.rate*.01f));if(d.font)tsf_set_output(d.font,TSF_STEREO_INTERLEAVED,int(d.rate),-6);d.current=d.target;d.volume=d.target[Param::Volume];d.reset();}
void SoundFont::reset() noexcept{impl->reset();}
void SoundFont::setSettings(const Settings& s) noexcept{impl->target=s;}
void SoundFont::select(int index) noexcept{auto& d=*impl;if(!d.font)return;index=std::clamp(index,0,d.font->presetNum-1);d.transition();for(int v=0;v<d.font->voiceNum;++v)tsf_voice_kill(&d.font->voices[v]);for(int c=0;c<17;++c){tsf_channel_set_presetindex(d.font,c,index);tsf_channel_set_bank(d.font,c,d.font->presets[index].bank);}d.lastKey.fill(-1);d.selected=index;}
void SoundFont::noteOn(int channel,int key,float velocity,bool preview) noexcept{
    auto& d=*impl;if(!d.font||!std::isfinite(velocity)||key<0||key>127||channel<1||channel>16)return;
    int c=preview?16:channel-1;velocity=std::clamp(velocity,0.0f,1.0f);if(velocity<=0){noteOff(channel,key);return;}
    if(preview){stopPreview();d.previewRemaining=int(d.rate*.65f);}
    int limit=std::clamp(int(d.target[Param::Voices]),1,32);const auto& preset=d.font->presets[tsf_channel_get_preset_index(d.font,c)];int wanted=0;
    const int midiVelocity=int(velocity*127);
    for(int r=0;r<preset.regionNum;++r){const auto& zone=preset.regions[r];if(key>=zone.lokey&&key<=zone.hikey&&midiVelocity>=zone.lovel&&midiVelocity<=zone.hivel)++wanted;}
    wanted=std::min(wanted,limit);if(!wanted)return;
    if(int(d.target[Param::Mode])>0&&!preview){for(int v=0;v<d.font->voiceNum;++v)if(d.font->voices[v].playingPreset>=0&&d.font->voices[v].playingChannel==c)tsf_voice_kill(&d.font->voices[v]);d.transition();}
    while(tsf_active_voice_count(d.font)>limit-wanted){tsf_voice* oldest=nullptr;
        for(int v=0;v<d.font->voiceNum;++v){auto& voice=d.font->voices[v];if(voice.playingPreset>=0&&(!oldest||voice.playIndex<oldest->playIndex))oldest=&voice;}
        if(!oldest)break;
        tsf_voice_kill(oldest);d.transition();
    }
    auto play=d.font->voicePlayIndex;tsf_channel_note_on(d.font,c,key,velocity);
    int added=0;
    for(int i=0;i<d.font->voiceNum;++i){auto& v=d.font->voices[i];if(v.playingPreset<0||v.playIndex!=play)continue;
        if(++added>wanted){tsf_voice_kill(&v);continue;}
        // The macro adds a minimum attack without replacing the bank envelope.
        v.ampenv.parameters.attack=std::max(v.ampenv.parameters.attack,d.target[Param::Attack]);tsf_voice_envelope_nextsegment(&v.ampenv,TSF_SEGMENT_NONE,d.rate);
        d.glide[std::size_t(i)]=d.target[Param::Glide]>.0001f&&d.lastKey[std::size_t(c)]>=0?float(d.lastKey[std::size_t(c)]-key):0;
    }
    d.lastKey[std::size_t(c)]=key;
}
void SoundFont::noteOff(int channel,int key) noexcept{if(impl->font&&channel>=1&&channel<=16&&key>=0&&key<=127)tsf_channel_note_off(impl->font,channel-1,key);}
void SoundFont::controller(int channel,int cc,int value) noexcept{auto& d=*impl;if(!d.font||channel<1||channel>16||cc<0||cc>127)return;value=std::clamp(value,0,127);if(cc==1)d.wheel[std::size_t(channel-1)]=float(value)/127;else if(cc==121)d.wheel[std::size_t(channel-1)]=0;tsf_channel_midi_control(d.font,channel-1,cc,value);}
void SoundFont::pitchWheel(int channel,int value) noexcept{if(impl->font&&channel>=1&&channel<=16)tsf_channel_set_pitchwheel(impl->font,channel-1,std::clamp(value,0,16383));}
void SoundFont::programChange(int channel,int value) noexcept{if(impl->font&&channel>=1&&channel<=16&&value>=0&&value<=127)tsf_channel_set_presetnumber(impl->font,channel-1,value,channel==10);}
void SoundFont::stopPreview() noexcept{if(impl->font)tsf_channel_note_off_all(impl->font,16);impl->previewRemaining=0;}
int SoundFont::activeVoices() const noexcept{return impl->font?tsf_active_voice_count(impl->font):0;}
void SoundFont::render(float* left,float* right,int frames) noexcept{
    auto& d=*impl;if(!d.font)return;
    for(int at=0;at<frames;){int count=std::min(32,frames-at);if(d.previewRemaining>0)count=std::min(count,d.previewRemaining);
        const float smooth=1-std::exp(-float(count)/(d.rate*.015f));
        for(std::size_t i=0;i<parameterCount;++i)d.current.values[i]+=(d.target.values[i]-d.current.values[i])*smooth;
        d.filters[0].coefficients(d.current[Param::Cutoff],d.current[Param::Resonance],d.rate);d.filters[1].tuning(d.filters[0].g,d.filters[0].k);
        for(int c=0;c<17;++c)tsf_channel_set_pitchrange(d.font,c,d.target[Param::BendRange]);
        d.phase+=float(count)*5.2f/d.rate;d.phase-=std::floor(d.phase);
        const float glideDecay=d.target[Param::Glide]>.0001f?std::exp(-float(count)/(d.rate*d.target[Param::Glide])):0;
        for(int i=0;i<d.font->voiceNum;++i){auto& v=d.font->voices[i];if(v.playingPreset<0)continue;int c=std::clamp(v.playingChannel,0,16);
            const float vibrato=(d.target[Param::Vibrato]+.5f*d.wheel[std::size_t(c)])*std::sin(tau*d.phase);
            const float bend=(float(tsf_channel_get_pitchwheel(d.font,c))-8192)/8192*tsf_channel_get_pitchrange(d.font,c);
            tsf_voice_calcpitchratio(&v,bend+tsf_channel_get_tuning(d.font,c)+d.glide[std::size_t(i)]+vibrato,d.rate);d.glide[std::size_t(i)]*=glideDecay;
        }
        tsf_render_float(d.font,d.buffer.data(),count,0);
        for(int i=0;i<count;++i){float l=d.filters[0].next(d.buffer[std::size_t(i*2)],std::clamp(int(d.target[Param::FilterType]),0,3));float r=d.filters[1].next(d.buffer[std::size_t(i*2+1)],std::clamp(int(d.target[Param::FilterType]),0,3));
            d.effects.process(l,r,d.current);d.volume+=(d.target[Param::Volume]-d.volume)*d.volumeStep;l=std::tanh(l*d.volume);r=std::tanh(r*d.volume);
            if(d.fade>0){float f=float(d.fade--)/240;l=l*(1-f)+d.tailL*f;r=r*(1-f)+d.tailR*f;}
            left[at+i]=d.lastL=std::isfinite(l)?l:0;right[at+i]=d.lastR=std::isfinite(r)?r:0;
        }
        at+=count;if(d.previewRemaining>0){d.previewRemaining-=count;if(d.previewRemaining==0)stopPreview();}
    }
}
}
