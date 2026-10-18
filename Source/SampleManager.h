#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include "dsp/SynthEngine.h"
#include <atomic>
#include <memory>
#include <mutex>

// Worker owns every sample allocation. Audio uses a hazard-protected immutable
// snapshot; it never reads files, locks, allocates or destroys sample buffers.
class SampleManager final : private juce::Thread {
public:
    SampleManager();
    ~SampleManager() override;
    void loadFile(const juce::File&);
    void restore(const juce::var&); // Empty = synthesis only. Non-audio threads.
    void configure(gba::SampleOptions);
    void clear() {restore({});}
    juce::var state() const;
    juce::String status() const;
    bool busy() const noexcept {return loading.load();}
    bool consumeChange() noexcept {return changed.exchange(false);}
    bool waitUntilIdle(int milliseconds=10000);
    void synchronize(gba::SynthEngine&) noexcept;
    static juce::Result validateDocument(const juce::var&);
    static constexpr int maxFileBytes=8*1024*1024,maxStateBytes=16*1024*1024;
private:
    struct Published {std::shared_ptr<const gba::SampleData> audio;juce::var document;gba::SampleOptions options;};
    struct Job {std::uint64_t generation=0;juce::File file;juce::var document;};
    mutable std::mutex mutex;
    std::unique_ptr<Job> pending;
    std::vector<std::unique_ptr<Published>> retained;
    std::atomic<const Published*> requested {nullptr},hazard {nullptr};
    std::atomic<std::uint64_t> generation {0};
    std::atomic<bool> loading {false},changed {false};
    juce::var desired;
    juce::String message {"No sample loaded. Choose a WAV or AIFF file."};
    juce::WaitableEvent wake;
    void run() override;
    void queue(juce::File,juce::var);
    std::unique_ptr<Published> decode(const Job&,juce::String& error);
    void reclaim();
};
