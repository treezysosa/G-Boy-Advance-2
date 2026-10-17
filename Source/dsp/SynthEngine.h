#pragma once
#include "Parameters.h"
#include "DspMath.h"
#include <array>
#include <vector>

namespace gba {
class Effects {
public:
    void prepare(float sampleRate);
    void reset() noexcept;
    void process(float& l,float& r,const Settings& s) noexcept;
private:
    struct Line {
        std::vector<float> data;
        std::size_t pos=0;
        float damp=0;
        float read(float delay) const noexcept;
        void push(float value) noexcept;
    };
    float sr=48000,phase=0,holdL=0,holdR=0,lowL=0,lowR=0,highL=0,highR=0,prevL=0,prevR=0;
    int counter=0;
    std::array<Line,2> delay,mod,tape;
    std::array<Line,4> room;
    std::array<std::array<float,4>,2> allpass{};
};
class SynthEngine {
public:
    void prepare(double sampleRate);
    void reset() noexcept;
    void setSettings(const Settings& settings) noexcept;
    void noteOn(int channel,int note,float velocity,bool preview=false) noexcept;
    void noteOff(int channel,int note) noexcept;
    void controller(int channel,int number,int value) noexcept;
    void pitchWheel(int channel,int value) noexcept;
    void allNotesOff(bool immediate=false) noexcept;
    void stopPreview() noexcept;
    void render(float* left,float* right,int samples) noexcept;
    int activeVoices() const noexcept;
    const Settings& settings() const noexcept {return target;}
private:
    static constexpr int maxVoices=32, stringSize=32768;
    struct Voice {
        int note=-1,channel=1,controlCounter=0;
        bool held=false,preview=false;
        std::uint64_t age=0;
        float velocity=1,frequency=440,targetFrequency=440,p1=0,p2=0,fmPhase=0,subPhase=0;
        double modalElapsed=0;
        bool stringActive=false,modalActive=false;
        LfoPhase lfo1,lfo2,vibrato,tremolo,syllable;
        std::array<float,6> modalPhase{};
        float tail=0,tailStep=0,last=0,stringLow=0,stringDelay=100,panMod=0;
        Envelope amp,filterEnv;
        SVF filter;
        std::array<SVF,3> formants,vocalFormants;
        Random noise;
        std::array<float,stringSize> string{};
        int stringPos=0;
    };
    struct Key {bool down=false;float velocity=0;std::uint64_t order=0;unsigned presses=0;};
    bool tablesReady=false,prepared=false;
    float sampleRate=48000,lastFrequency=0;
    int previewSamples=0,lastMode=0,lastVoiceCount=16;
    std::uint64_t clock=0;
    Settings target,current;
    float smoothing=0;
    int choiceFadeSamples=240;
    ChoiceFade<6> wave1Fade,wave2Fade,lfo1Route,lfo2Route;
    ChoiceFade<5> filterFade;
    ChoiceFade<8> engineFade;
    std::array<Voice,maxVoices> voices{};
    std::array<float,16> bend{},wheel{};
    std::array<bool,16> sustain{};
    std::array<std::array<Key,128>,16> keys{};
    Effects effects;
    std::array<float,24> harmonicInverseRoot{},harmonicWeight{},harmonicScale{};
    // Eight mip levels, generated once; no samples or external assets.
    std::array<std::array<std::array<float,1024>,8>,4> tables{};
    void buildTables() noexcept;
    void start(Voice&,int channel,int note,float velocity,bool preview,bool legato) noexcept;
    void release(Voice&) noexcept;
    float oscillator(int wave,float phase,float dt,float width,Random&) noexcept;
    float mixedOscillator(const ChoiceFade<6>&,float phase,float dt,float width,Random&) noexcept;
    float wavetable(float phase,float frequency,float position) const noexcept;
    float voiceSample(Voice&) noexcept;
    float excitation(Voice&,int engine,float frequency,float dt,float dt2,float filterEnvelope) noexcept;
    void exciteString(Voice&) noexcept;
    void updateFormants(Voice&,float vowel) noexcept;
};
}
