#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "dsp/SoundFont.h"
#include <atomic>
#include <mutex>
#include <optional>
// Single audio consumer; all bank allocation, file work and destruction is off it.
class SoundFontManager final : private juce::Thread {
public:
    struct View {juce::String fileName,status;juce::StringArray instruments;juce::Array<int> banks;int selected=0;bool loading=false,enabled=false;};
    struct LibraryView {juce::File folder;juce::StringArray files,names;juce::String status,source;bool scanning=false,rememberLast=true;};
    explicit SoundFontManager(juce::File dataRoot);
    ~SoundFontManager() override;
    void loadFile(const juce::File&);
    void armStartupRecall();
    void startLibrary();
    void refreshLibrary();
    void setLibraryFolder(const juce::File&);
    void setRememberLast(bool);
    LibraryView libraryView() const;
    void select(int index);
    void step(int delta,bool bank=false);
    void useSoundFont(bool);
    bool enabled() const noexcept {return switchedOn.load();}
    View view() const;
    juce::var saveState() const;
    void restore(const juce::var&);
    bool waitUntilIdle(int milliseconds);
    void prepare(double);
    gba::SoundFont* beginBlock(const gba::Settings&) noexcept;
    gba::SoundFont* audio() const noexcept;
    void endBlock() noexcept;
    std::uint64_t revision() const noexcept {return changes.load();}
private:
    struct Bank {gba::SoundFont synth;std::atomic<int> selected{0};int playing=-1;std::uint64_t id=0;};
    struct Request {juce::File file;int selected=0;bool import=true;std::uint64_t id=0;bool remember=false;};
    juce::File cache,libraryPreferences;mutable std::mutex mutex;
    std::unique_ptr<Bank> current;std::vector<std::unique_ptr<Bank>> retired;
    std::optional<Request> request;
    std::atomic<Bank*> published{nullptr},hazard{nullptr};Bank* rendering=nullptr;
    std::atomic<bool> switchedOn{false};std::atomic<double> rate{48000};std::atomic<std::uint64_t> changes{0};
    View summary;juce::File activeFile;int activeIndex=0;std::uint64_t generation=0;
    LibraryView library;
    juce::var lastBank;
    std::uint64_t startupGeneration=0,folderRevision=0;
    bool libraryStarted=false,initializePending=false,scanPending=false,workerBusy=false;
    int dirtyPreferences=0;
    void run() override;
    void enqueue(const juce::File&,int,bool);
    void loadBank(const Request&);
    void initializeLibrary();
    void scanLibrary();
    void writeLibraryPreferences();
    void rememberBank(); // Caller holds mutex; schedules worker persistence.
    juce::var reference() const; // Caller holds mutex.
    void reclaim();
};
