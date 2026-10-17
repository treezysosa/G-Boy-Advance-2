#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/Parameters.h"
#include <set>

class GbaProcessor;
struct GbaPreset {
    juce::String id,name,category,tags;
    gba::Settings settings;
    juce::File file;
    bool factory=true;
};
// File operations and the library model are used only by the message thread.
class PresetManager {
public:
    explicit PresetManager(GbaProcessor&,juce::File dataRoot={});
    const std::vector<GbaPreset>& all() const noexcept {return presets;}
    const GbaPreset* find(const juce::String& id) const;
    juce::StringArray categories() const;
    bool load(const juce::String& id);
    void step(int direction);
    void stepCategory(int direction);
    juce::Result saveAs(const juce::String& name);
    juce::Result renameUser(const juce::String& id,const juce::String& name);
    juce::Result deleteUser(const juce::String& id);
    void restoreDefault();
    bool isFavorite(const juce::String& id) const;
    void toggleFavorite(const juce::String& id);
    bool isRecent(const juce::String& id) const;
    const juce::StringArray& recentIds() const {return recent;}
    juce::File userFolder() const {return folder;}
    juce::String lastError;
    static constexpr int schemaVersion=1;
private:
    GbaProcessor& processor;
    std::vector<GbaPreset> presets;
    juce::File folder,preferences;
    std::set<juce::String> favorites;
    juce::StringArray recent;
    juce::Result write(const GbaPreset&);
    void scanUsers();
    void remember(const juce::String& id);
    void savePreferences();
};
