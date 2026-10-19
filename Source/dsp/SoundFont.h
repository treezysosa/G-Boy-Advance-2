#pragma once
#include "Effects.h"
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
namespace gba {
// A prepared bank is owned off the callback. Only the audio thread mutates voices.
class SoundFont {
public:
    static constexpr std::size_t maxBytes=128*1024*1024;
    struct Instrument {std::string name;int bank=0,program=0;};
    SoundFont();~SoundFont();
    SoundFont(const SoundFont&)=delete;SoundFont& operator=(const SoundFont&)=delete;
    bool load(const void*,std::size_t,std::string& error);
    void prepare(double);
    void reset() noexcept;
    void setSettings(const Settings&) noexcept;
    void select(int index) noexcept;
    void noteOn(int channel,int key,float velocity,bool preview=false) noexcept;
    void noteOff(int channel,int key) noexcept;
    void controller(int channel,int cc,int value) noexcept;
    void pitchWheel(int channel,int value) noexcept;
    void programChange(int channel,int value) noexcept;
    void stopPreview() noexcept;
    void render(float* left,float* right,int frames) noexcept;
    int activeVoices() const noexcept;
    const std::vector<Instrument>& instruments() const noexcept;
    static std::uint64_t heapOperations() noexcept;
private:
    struct Impl;std::unique_ptr<Impl> impl;
};
}
