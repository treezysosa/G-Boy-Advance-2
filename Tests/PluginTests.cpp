#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
void set(GbaProcessor& p,const char* id,float value) {auto* v=p.state.getParameter(id);v->setValueNotifyingHost(v->convertTo0to1(value));}
void render(GbaProcessor& p,juce::AudioBuffer<float>& audio,int blocks) {for(int i=0;i<blocks;++i){juce::MidiBuffer midi;p.processBlock(audio,midi);}}
}
int main() {
    juce::ScopedJuceInitialiser_GUI gui;
    auto root=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("gba2-tests-"+juce::Uuid().toString());
    try {
        auto processor=std::make_unique<GbaProcessor>(root);auto& p=*processor;
        p.setRateAndBufferSizeDetails(48000,256);p.prepareToPlay(48000,256);
        juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;
        // Verify the host wrapper splits rendering at MIDI sample offsets.
        set(p,"amp.attack",.001f);midi.addEvent(juce::MidiMessage::noteOn(1,60,.8f),128);p.processBlock(audio,midi);
        check(audio.getMagnitude(0,0,128)==0,"MIDI sample offset was ignored");
        render(p,audio,8);check(audio.getMagnitude(0,256)>0.0001f,"Plugin wrapper produced no audio");
        set(p,"filter.cutoff",1234);set(p,"macro.a.target",4);set(p,"voice.glide",.21f);set(p,"engine",5);set(p,"formant.vowel",2.4f);set(p,"voice.count",24);
        p.editorWidth=1440;
        auto saved=p.presets.saveAs("Integration Custom Vowels");check(saved.wasOk(),"Save As failed");auto id=p.presetId();
        p.presets.toggleFavorite(id);check(p.presets.isFavorite(id),"Favorite failed");check(p.presets.isRecent(id),"Recently used failed");
        juce::MemoryBlock state;p.getStateInformation(state);
        auto restored=std::make_unique<GbaProcessor>(root);restored->setStateInformation(state.getData(),int(state.getSize()));
        check(restored->presetId()==id,"State preset identity lost");
        for(std::size_t i=0;i<gba::parameterCount;++i)check(std::abs(p.snapshot().values[i]-restored->snapshot().values[i])<.001f,"State parameter roundtrip failed");
        check(restored->editorWidth==1440,"State editor size lost");check(restored->presets.find(id)!=nullptr,"User preset did not reload from disk");check(restored->presets.isFavorite(id),"Favorite did not persist");
        check(restored->presets.load(id),"User preset load failed");check(std::abs(restored->snapshot()[gba::Param::Cutoff]-1234)<.1f,"User preset values changed");
        check(restored->presets.renameUser(id,"Renamed Vowels").wasOk(),"Rename failed");check(restored->presetName()=="Renamed Vowels","Rename identity failed");
        check(restored->presets.deleteUser(id).wasOk(),"Delete failed");check(restored->presets.find(id)==nullptr,"Delete retained user patch");
        auto defaultId=restored->presetId();check(restored->presets.deleteUser(defaultId).failed(),"Factory delete was allowed");
        check(restored->presets.saveAs("").failed(),"Empty preset name was allowed");
        auto before=restored->snapshot();const char invalid[]{0,1,2,3,4};restored->setStateInformation(invalid,5);check(before.values==restored->snapshot().values,"Invalid state altered patch");
        restored->setCurrentProgram(0);restored->servicePendingProgram();check(restored->presetName()==restored->getProgramName(0),"Host program selection failed");
        auto prior=restored->presetId();restored->presets.step(1);check(restored->presetId()!=prior,"Next preset failed");restored->presets.step(-1);check(restored->presetId()==prior,"Previous preset failed");
        auto category=restored->presetCategory();restored->presets.stepCategory(1);check(restored->presetCategory()!=category,"Next category failed");
        {std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());check(editor!=nullptr,"Editor creation failed");editor->setSize(960,485);editor->setSize(1920,969);auto image=editor->createComponentSnapshot(editor->getLocalBounds());check(image.isValid(),"Scalable editor snapshot failed");
            auto screenshot=juce::File::getCurrentWorkingDirectory().getChildFile("GBA2-editor.png");juce::FileOutputStream output(screenshot);juce::PNGImageFormat png;png.writeImageToStream(image,output);}
        p.releaseResources();restored->releaseResources();restored.reset();processor.reset();root.deleteRecursively();
        std::cout<<"PASS plugin audio/MIDI timing, complete state, user CRUD, favorites, programs, categories, editor creation and sizes.\n";return 0;
    } catch(const std::exception& ex) {std::cerr<<"FAIL: "<<ex.what()<<"\n";root.deleteRecursively();return 1;}
}
