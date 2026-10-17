#pragma once
#include "PluginProcessor.h"

class ConsoleLook final : public juce::LookAndFeel_V4 {
public:
    ConsoleLook();
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
    void drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool) override;
    void drawLinearSlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider::SliderStyle,juce::Slider&) override;
};
class DirectionPad final : public juce::Component {
public:
    std::function<void(int)> onDirection;
    DirectionPad();
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    bool keyPressed(const juce::KeyPress&) override;
private:
    int pressed=-1;
};
class GbaEditor final : public juce::AudioProcessorEditor,private juce::Timer {
public:
    explicit GbaEditor(GbaProcessor&);
    ~GbaEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;
private:
    GbaProcessor& processor;
    ConsoleLook look;
    juce::TooltipWindow tooltip {this,650};
    juce::MidiKeyboardComponent keyboard;
    juce::TextButton previous {"<"},next {">"},presetDisplay,start {"START"},select {"SELECT"},aButton {"A"},bButton {"B"};
    DirectionPad dpad;
    juce::Slider volume,glide,pitch;
    std::array<juce::Slider,4> macros;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAttachment,glideAttachment;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,4> macroAttachments;
    std::array<int,4> mapping {{-1,-1,-1,-1}};
    std::unique_ptr<juce::Component> overlay;
    void timerCallback() override;
    void updateMacros();
    void showBrowser();
    void showAdvanced();
    void closeOverlay();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GbaEditor)
};
