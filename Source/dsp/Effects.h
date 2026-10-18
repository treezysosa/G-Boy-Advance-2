#pragma once
#include "Parameters.h"
#include "DspMath.h"
#include <array>
#include <vector>

namespace gba {
class Effects {
public:
    // Two 33-tap half-band FIRs at 2x rate: 16 base-rate frames of group delay.
    static constexpr int latencySamples=16;
    void prepare(float sampleRate);
    void reset() noexcept;
    void process(float& left,float& right,const Settings&) noexcept;
private:
    struct Line {
        std::vector<float> data;
        std::size_t pos=0;
        float damp=0;
        float read(float delay) const noexcept;
        void push(float value) noexcept;
    };
    struct Fir {
        // Mirrored history makes the same 33-tap window contiguous across wraps.
        std::array<float,66> history{};
        int pos=0;
        float next(float,const std::array<float,33>&) noexcept;
    };
    struct Shelf {
        float b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
        void coefficients(float hz,float db,float sr,bool high) noexcept;
        float next(float x) noexcept;
    };
    float sr=48000,reverbDamping=0;
    float driveGain=1,driveNorm=1,driveMix=0,hardness=0,quantSteps=32768,roomFeedback=.8f;
    float delayA=0,delayB=0;
    int crusherCounter=0,controlCounter=0,delayFade=0,delayFadeLength=960,dryPos=0;
    bool ready=false,coefficientsReady=false;
    bool eqReady=false;
    float cachedEqLow=0,cachedEqHigh=0;
    LfoPhase chorusPhase,flangePhase,phaserPhase,wowPhase,flutterPhase;
    std::array<float,2> held{};
    std::array<float,33> halfband{};
    std::array<Fir,2> up,down;
    std::array<std::array<float,latencySamples>,2> dry{};
    std::array<Line,2> delay,mod,tape;
    std::array<Line,4> room;
    std::array<std::array<float,4>,2> allpass{};
    std::array<Shelf,2> lowShelf,highShelf;
    void update(const Settings&) noexcept;
    float nonlinear(float,int channel) noexcept;
};
}
