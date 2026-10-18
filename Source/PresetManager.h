#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/Parameters.h"
#include <set>
class GbaProcessor;
struct GbaPreset {
    juce::String id,name,category,tags;
    gba::Settings settings;
    juce::var sample; // Optional self-contained sample document; schema 2 only.
    juce::File file;
    bool factory=true;
};
// Model and file operations belong to non-audio threads (normally the message thread).
class PresetManager {
public:
    enum class Source {all,factory,user};
    struct Filter {juce::String query,category;Source source=Source::all;bool favorites=false,recent=false;};
    explicit PresetManager(GbaProcessor&,juce::File dataRoot={});
    const std::vector<GbaPreset>& all() const noexcept {return presets;}
    const GbaPreset* find(const juce::String& id) const;
    juce::StringArray categories() const;
    std::vector<juce::String> filtered(const Filter&) const;
    juce::Result refresh();
    std::uint64_t revision() const noexcept {return generation;}
    bool load(juce::String id);
    void step(int direction);
    void stepCategory(int direction);
    juce::Result saveAs(juce::String name,juce::String tags={});
    juce::Result importPreset(const juce::File&);
    juce::Result exportCurrent(const juce::File&,juce::String name={},juce::String tags={});
    juce::Result renameUser(juce::String id,juce::String name);
    juce::Result deleteUser(juce::String id);
    void restoreDefault(bool remember=true);
    bool isFavorite(const juce::String& id) const;
    juce::Result toggleFavorite(juce::String id);
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
    std::uint64_t generation=0;
    juce::Result write(const GbaPreset&,bool external=false);
    juce::Result read(const juce::File&,GbaPreset&,bool external=false) const;
    juce::Result readPreferences(std::set<juce::String>&,juce::StringArray&,bool merging) const;
    juce::Result writePreferences(const std::set<juce::String>&,const juce::StringArray&);
    juce::Result remember(const juce::String& id);
    juce::Result finish(juce::Result);
    void sortUsers();
    bool nameExists(const juce::String&,const juce::String& except={}) const;
};
