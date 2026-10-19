#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "dsp/SynthEngine.h"
#include "PresetManager.h"
#include "SoundFontManager.h"

class GbaProcessor final : public juce::AudioProcessor, private juce::MidiKeyboardStateListener, private juce::Timer {
public:
    explicit GbaProcessor(juce::File dataRoot={});
    ~GbaProcessor() override;
    void prepareToPlay(double,int) override;
    void releaseResources() override;
    void reset() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override {return true;}
    const juce::String getName() const override {return "G Boy Advance 2";}
    bool acceptsMidi() const override {return true;}
    bool producesMidi() const override {return false;}
    bool isMidiEffect() const override {return false;}
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override {return currentProgram.load();}
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    gba::Settings snapshot() const noexcept;
    void applyPreset(const GbaPreset&);
    void setEditorSkin(int);
    void renameCurrentPreset(const juce::String&);
    juce::String presetId() const;
    juce::String presetName() const;
    juce::String presetCategory() const;
    juce::String stateNotice() const;
    void preview();
    void stopPreview();
    void uiPitch(int);
    void panic();
    void servicePendingProgram();
    int voiceCount() const noexcept {return active.load();}
    float outputLevel() const noexcept {return level.load();}
    std::atomic<int> editorWidth {1280};
    std::atomic<int> editorSkin {0}; // Non-automatable, per-instance project preference.
    juce::AudioProcessorValueTreeState state;
    juce::MidiKeyboardState keyboard;
    PresetManager presets;
    SoundFontManager romHacks;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
private:
    struct Command {int kind=0,channel=1,note=60;float value=0;};
    std::array<Command,256> commands{};
    juce::AbstractFifo commandFifo {256}; // Message thread produces; audio thread consumes.
    std::atomic<bool> panicRequested {false};
    std::atomic<int> pitchRequested {-1};
    std::array<std::atomic<float>*,gba::parameterCount> raw{};
    std::unique_ptr<gba::SynthEngine> engine;
    std::atomic<int> pendingProgram {-1},currentProgram {0},active {0};
    std::atomic<float> level {0};
    // State serialization and UI metadata may run on different non-audio threads.
    mutable juce::CriticalSection metadataLock;
    juce::String activePresetId,activePresetName {"Custom patch"},activePresetCategory {"User"};
    bool removedSamplePlayback=false;
    bool soundFontMode=false;int modeFade=0;float lastLeft=0,lastRight=0,modeLeft=0,modeRight=0;
    std::uint64_t soundFontRevision=0;
    void enqueue(Command) noexcept;
    void drainCommands() noexcept;
    void midi(const juce::MidiMessage&) noexcept;
    void handleNoteOn(juce::MidiKeyboardState*,int,int,float) override;
    void handleNoteOff(juce::MidiKeyboardState*,int,int,float) override;
    void timerCallback() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GbaProcessor)
};
