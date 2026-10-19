#pragma once
#include "Parameters.h"
#include "DspMath.h"
#include "Effects.h"
#include "VocalEngine.h"
#include "RetroEngine.h"
#include <array>
#include <vector>

namespace gba {
class SynthEngine {
public:
    static constexpr int latencySamples=Effects::latencySamples;
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
    struct TableTuning {
        float frequency=-1,rate=-1,blend=0;
        int level=0,next=0;
    };
    struct Voice {
        int note=-1,channel=1,controlCounter=0;
        bool held=false,preview=false;
        std::uint64_t age=0;
        float velocity=1,frequency=440,targetFrequency=440,p1=0,p2=0,fmPhase=0,subPhase=0;
        float velocityRoot=1,panLeft=0,panRight=0;
        float filterCutoff=0,filterResonance=0,formantVowel=0,formantShift=0;
        float vocalVowel=0,vocalShift=0,vocalBrightness=0;
        TableTuning table1,table2;
        float stringFrequency=-1,stringBrightness=-1,stringRate=-1;
        bool filterReady=false,formantsReady=false,vocalReady=false;
        double modalElapsed=0;
        bool stringActive=false,modalActive=false;
        LfoPhase lfo1,lfo2,vibrato,tremolo,syllable;
        std::array<float,6> modalPhase{};
        float tail=0,tailStep=0,last=0,stringLow=0,stringDelay=100,panMod=0;
        Envelope amp,filterEnv;
        SVF filter;
        std::array<SVF,3> formants;
        VocalEngine vocal;
        RetroEngine retro;
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
    // Only parameters still changing need a per-sample smoothing operation.
    std::array<std::size_t,parameterCount> movingParameters{};
    std::array<bool,parameterCount> parameterMoving{};
    std::size_t movingCount=0;
    float smoothing=0;
    int choiceFadeSamples=240;
    ChoiceFade<6> wave1Fade,wave2Fade,lfo1Route,lfo2Route;
    ChoiceFade<5> filterFade;
    EngineFade engineFade;
    float glideStep=1,oscillator2Ratio=1;
    float cachedPitch=-1000,pitchRatio=1;
    float cachedGlide=-1,cachedOscillatorInterval=1000,cachedBrightness=-1;
    // Exact last-input caches share coefficient math across chord voices.
    // Delay/filter histories remain private to each voice.
    float cachedFilterBase=-1,cachedFilterOctaves=0,cachedCutoff=0,cachedFilterHz=-1,cachedFilterResonance=-1;
    float cachedFormantVowel=-100,cachedFormantShift=0;
    float cachedVocalVowel=-100,cachedVocalShift=0,cachedVocalBrightness=0;
    SVF filterTuning;
    std::array<SVF,3> formantTuning;
    VocalEngine vocalTuning;
    Envelope::Rates ampRates,filterRates;
    double lfo1Increment=0,lfo2Increment=0,vibratoIncrement=0,tremoloIncrement=0,syllableIncrement=0;
    LfoValueCache lfo1Value,lfo2Value,vibratoValue,tremoloValue,syllableValue;
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
    float wavetable(float phase,float frequency,float position,TableTuning&) const noexcept;
    float voiceSample(Voice&) noexcept;
    float excitation(Voice&,int engine,float frequency,float dt,float dt2,float filterEnvelope) noexcept;
    void exciteString(Voice&) noexcept;
    void updateFormants(Voice&,float vowel) noexcept;
};
}
