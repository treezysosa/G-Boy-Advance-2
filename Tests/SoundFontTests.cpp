#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "SoundFontFixture.h"
#include <atomic>
#include <thread>
#include <iostream>
#include <stdexcept>
namespace {
int sfChecks=0;
void ensure(bool ok,const char* message){++sfChecks;if(!ok)throw std::runtime_error(message);}
void setSf(GbaProcessor& p,const char* id,float value){auto* v=p.state.getParameter(id);ensure(v!=nullptr,"Unknown shared SoundFont parameter");v->setValueNotifyingHost(v->convertTo0to1(value));}
void dry(GbaProcessor& p){for(const char* id:{"fx.reverb","fx.delay","fx.chorus","fx.phaser","fx.flanger","fx.saturation","fx.distortion"})setSf(p,id,0);setSf(p,"amp.attack",.001f);setSf(p,"filter.cutoff",20000);setSf(p,"filter.resonance",0);setSf(p,"volume",.7f);}
float play(GbaProcessor& p,int note=69){juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;midi.addEvent(juce::MidiMessage::noteOn(1,note,.8f),0);p.processBlock(audio,midi);float peak=0;for(int i=0;i<32;++i){midi.clear();p.processBlock(audio,midi);peak=std::max(peak,audio.getMagnitude(0,256));}return peak;}
juce::Component* namedSf(juce::Component& c,const juce::String& name){if(c.getName()==name)return &c;for(auto* child:c.getChildren())if(auto* found=namedSf(*child,name))return found;return nullptr;}
juce::TextButton* buttonSf(juce::Component& c,const juce::String& name){if(auto* b=dynamic_cast<juce::TextButton*>(&c);b&&b->getButtonText()==name)return b;for(auto* child:c.getChildren())if(auto* b=buttonSf(*child,name))return b;return nullptr;}
void picture(juce::Component& c,const juce::String& path){auto image=c.createComponentSnapshot(c.getLocalBounds());auto file=juce::File::getCurrentWorkingDirectory().getChildFile(path);ensure(file.getParentDirectory().createDirectory().wasOk(),"SF2 screenshot directory failed");juce::FileOutputStream output(file);ensure(output.openedOk()&&output.setPosition(0)&&output.truncate().wasOk(),"SF2 screenshot file failed");ensure(juce::PNGImageFormat().writeImageToStream(image,output),"SF2 screenshot encode failed");}
void libraryTests(const juce::File& root,const std::vector<unsigned char>& data){
    const auto user=root.getChildFile("LibraryUser"),folder=user.getChildFile("SoundFont Library");
    auto p=std::make_unique<GbaProcessor>(user);p->setRateAndBufferSizeDetails(48000,256);p->prepareToPlay(48000,256);dry(*p);
    ensure(p->romHacks.waitUntilIdle(10000),"Default library initialization timed out");
    ensure(folder.isDirectory()&&p->romHacks.libraryView().folder==folder&&p->romHacks.libraryView().rememberLast,"Default SoundFont folder/recall preference is missing");
    const auto alpha=folder.getChildFile("Alpha Bank.sf2"),zulu=folder.getChildFile(juce::String::fromUTF8("Zulu Bank \xc3\xa9.SF2"));
    ensure(alpha.replaceWithData(data.data(),data.size())&&zulu.replaceWithData(data.data(),data.size()),"Library banks could not be written");
    ensure(folder.getChildFile("Ignored.sf3").replaceWithText("unsupported")&&folder.getChildFile("ignored.wav").replaceWithText("unsupported"),"Library ignored-file fixtures failed");
    ensure(folder.getChildFile("Subfolder.sf2").createDirectory().wasOk(),"Library directory fixture failed");
    ensure(folder.getChildFile("Subfolder.sf2/Nested.sf2").replaceWithData(data.data(),data.size()),"Nested library fixture failed");
    p->romHacks.refreshLibrary();ensure(p->romHacks.waitUntilIdle(10000),"Library refresh timed out");
    ensure(p->romHacks.libraryView().files.size()==2&&p->romHacks.libraryView().names[0]==alpha.getFileName(),"Library scan missed SF2/SF2 case or included unsupported files/directories");
    juce::MemoryBlock project;
    {
        std::unique_ptr<juce::AudioProcessorEditor> editor(p->createEditor());editor->setSize(960,485);buttonSf(*editor,"ROM Hacks")->onClick();
        auto* banks=dynamic_cast<juce::ComboBox*>(namedSf(*editor,"SoundFont bank"));
        auto* remember=dynamic_cast<juce::ToggleButton*>(namedSf(*editor,"Remember last SoundFont"));
        ensure(banks&&banks->getNumItems()==2&&remember&&remember->getToggleState(),"Folder bank selector/remember switch is missing");
        ensure(buttonSf(*editor,"OPEN FOLDER")&&buttonSf(*editor,"CHOOSE FOLDER")&&buttonSf(*editor,"REFRESH"),"Folder buttons are missing");
        banks->setSelectedId(2,juce::sendNotificationSync);ensure(p->romHacks.waitUntilIdle(10000),"Folder bank selection timed out");juce::MessageManager::getInstance()->runDispatchLoopUntil(120);
        ensure(p->romHacks.enabled()&&p->romHacks.view().fileName==zulu.getFileName()&&play(*p)>.001f,"Folder bank selector failed to load audible MIDI");
        auto* instruments=dynamic_cast<juce::ComboBox*>(namedSf(*editor,"SoundFont instrument"));ensure(instruments&&instruments->getNumItems()==2,"Folder bank instruments missing");instruments->setSelectedId(2,juce::sendNotificationSync);
        ensure(p->romHacks.waitUntilIdle(10000),"Remembering instrument timed out");p->getStateInformation(project);
        auto* path=dynamic_cast<juce::Label*>(namedSf(*editor,"SoundFont folder"));ensure(path&&path->getText()==folder.getFullPathName(),"Default folder path is not shown");
        picture(*editor,"ui-snapshots/SF2-Library-960.png");editor->setSize(1920,969);picture(*editor,"ui-snapshots/SF2-Library-1920.png");
    }
    {
        // Load the real managed library through the preset category menu, with
        // stale synthesis filters, then return to a native patch and restore SF2.
        std::unique_ptr<juce::AudioProcessorEditor> editor(p->createEditor());editor->setSize(960,485);buttonSf(*editor,"START")->onClick();
        auto* category=dynamic_cast<juce::ComboBox*>(namedSf(*editor,"Preset category"));auto* sourceFilter=dynamic_cast<juce::ComboBox*>(namedSf(*editor,"Preset source"));auto* search=dynamic_cast<juce::TextEditor*>(namedSf(*editor,"Preset search"));
        ensure(category&&sourceFilter&&search,"Preset-folder controls are missing");
        sourceFilter->setSelectedId(2,juce::sendNotificationSync);search->setText("no matching synthesis patch");category->setText("SoundFonts",juce::sendNotificationSync);
        auto* sf=namedSf(*editor,"Preset SoundFonts");ensure(sf&&sf->isVisible()&&!sf->getBounds().isEmpty()&&category->getText()=="SoundFonts","SoundFonts is not a folder in the preset browser");
        ensure(!sourceFilter->isVisible()&&!search->isVisible(),"Synthesis filters remain active over the SF2 library");
        ensure(p->getNumPrograms()==754&&!p->presets.categories().contains("SoundFonts"),"SoundFont navigation changed factory program identities");
        auto* banks=dynamic_cast<juce::ComboBox*>(namedSf(*sf,"SoundFont bank"));ensure(banks&&banks->getNumItems()==2,"Preset SoundFonts folder did not show the configured library");
        banks->setSelectedId(1,juce::sendNotificationSync);ensure(p->romHacks.waitUntilIdle(10000),"Preset-folder bank load timed out");juce::MessageManager::getInstance()->runDispatchLoopUntil(120);
        ensure(p->romHacks.enabled()&&p->romHacks.view().fileName==alpha.getFileName()&&play(*p)>.001f,"Preset-folder bank selection did not produce audible SoundFont MIDI");
        auto* instruments=dynamic_cast<juce::ComboBox*>(namedSf(*sf,"SoundFont instrument"));ensure(instruments&&instruments->getNumItems()==2,"Preset-folder instruments are missing");instruments->setSelectedId(2,juce::sendNotificationSync);
        ensure(p->romHacks.view().selected==1,"Preset-folder instrument selection is inert");
        buttonSf(*sf,"A  PREVIEW")->onClick();ensure(play(*p)>.001f,"Preset-folder preview failed");buttonSf(*sf,"B  STOP")->onClick();
        picture(*editor,"ui-snapshots/SF2-Preset-Folder-960.png");editor->setSize(1920,969);picture(*editor,"ui-snapshots/SF2-Preset-Folder-1920.png");editor->setSize(960,485);
        category->setText("Milwaukee",juce::sendNotificationSync);ensure(!sf->isVisible()&&sourceFilter->isVisible()&&sourceFilter->getSelectedId()==2&&search->getText()=="no matching synthesis patch","Leaving SoundFonts did not restore synthesis filters");
        search->setText("");juce::MessageManager::getInstance()->runDispatchLoopUntil(30);auto* list=dynamic_cast<juce::ListBox*>(namedSf(*editor,"Presets"));
        ensure(list&&list->getListBoxModel()->getNumRows()==24,"SoundFonts folder displaced Milwaukee presets");list->selectRow(0);list->keyPressed(juce::KeyPress(juce::KeyPress::returnKey));
        ensure(!p->romHacks.enabled()&&p->presetCategory()=="Milwaukee"&&play(*p)>.001f,"Loading a factory patch did not return to audible synthesis");
        category->setText("SoundFonts",juce::sendNotificationSync);ensure(sf->isVisible(),"SoundFonts folder cannot be reopened");banks->setSelectedId(2,juce::sendNotificationSync);ensure(p->romHacks.waitUntilIdle(10000),"Second preset-folder bank load timed out");juce::MessageManager::getInstance()->runDispatchLoopUntil(120);instruments->setSelectedId(2,juce::sendNotificationSync);dry(*p);
        juce::MemoryBlock folderProject;p->getStateInformation(folderProject);auto restored=std::make_unique<GbaProcessor>(root.getChildFile("PresetFolderProject"));restored->setStateInformation(folderProject.getData(),int(folderProject.getSize()));restored->prepareToPlay(48000,256);
        ensure(restored->romHacks.enabled()&&restored->romHacks.view().fileName==zulu.getFileName()&&restored->romHacks.view().selected==1&&play(*restored)>.001f,"SoundFont chosen in preset folders did not restore audible bank/instrument in a project");
        for(int i=0;i<6;++i){category->setText("Brass",juce::sendNotificationSync);category->setText("SoundFonts",juce::sendNotificationSync);}ensure(sf->isVisible()&&p->romHacks.enabled(),"Repeated preset-folder navigation changed SoundFont playback");
        buttonSf(*editor,"BACK / START")->onClick();juce::MessageManager::getInstance()->runDispatchLoopUntil(30);ensure(namedSf(*editor,"Preset browser")==nullptr&&p->romHacks.enabled(),"Closing the preset SoundFont folder stopped playback or retained the overlay");
        std::cout<<"PASS SoundFonts preset folder: configured banks, instrument selection, audible MIDI/preview, preserved filters, factory return and project recall\n";
    }
    p.reset();ensure(zulu.deleteFile(),"Library source removal fixture failed");
    auto reload=std::make_unique<GbaProcessor>(user);reload->setRateAndBufferSizeDetails(48000,256);reload->prepareToPlay(48000,256);dry(*reload);
    ensure(reload->romHacks.waitUntilIdle(10000),"Startup bank recall timed out");
    ensure(reload->romHacks.enabled()&&reload->romHacks.view().selected==1&&reload->romHacks.view().fileName==zulu.getFileName()&&play(*reload)>.001f,"New instance did not restore cached last bank/instrument without its source");
    ensure(reload->romHacks.saveState()["path"].toString()==juce::JSON::parse(user.getChildFile("soundfont-library.json"))["lastBank"]["path"].toString(),"Startup recall duplicated the cached bank");
    ensure(reload->romHacks.libraryView().files.size()==1,"Startup folder scan retained removed files");
    {
        juce::ChildProcess process;ensure(process.start(juce::StringArray{juce::File::getSpecialLocation(juce::File::currentExecutableFile).getFullPathName(),"--soundfont-startup-probe",user.getFullPathName()}),"Could not launch SoundFont restart probe");
        const bool finished=process.waitForProcessToFinish(15000);if(!finished)process.kill();ensure(finished&&process.getExitCode()==51,"Separate OS process did not recall the last audible bank/instrument");
    }
    reload->romHacks.loadFile(alpha);ensure(reload->romHacks.waitUntilIdle(10000),"Replacement library bank load timed out");
    auto fromProject=std::make_unique<GbaProcessor>(user);fromProject->setStateInformation(project.getData(),int(project.getSize()));fromProject->prepareToPlay(48000,256);
    ensure(fromProject->romHacks.waitUntilIdle(10000)&&fromProject->romHacks.view().fileName==zulu.getFileName()&&fromProject->romHacks.view().selected==1,"Startup default overwrote the bank saved in a DAW project");
    ensure(juce::JSON::parse(user.getChildFile("soundfont-library.json"))["lastBank"]["name"].toString()==alpha.getFileName(),"Restoring a project overwrote the global startup bank");
    const auto lastCached=juce::File(reload->romHacks.saveState()["path"].toString()),hidden=lastCached.withFileExtension("away");
    ensure(lastCached.moveFileTo(hidden),"Could not hide startup bank fixture");
    {auto missing=std::make_unique<GbaProcessor>(user);missing->prepareToPlay(48000,256);ensure(missing->romHacks.waitUntilIdle(10000),"Missing startup bank timed out");ensure(missing->romHacks.enabled()&&missing->romHacks.view().status.contains("missing")&&play(*missing)==0,"Missing startup bank silently played a different sound");}
    ensure(hidden.moveFileTo(lastCached),"Could not restore startup bank fixture");
    auto synth=std::make_unique<GbaProcessor>(root.getChildFile("SynthOnlyProject"));juce::MemoryBlock synthState;synth->getStateInformation(synthState);
    auto synthReload=std::make_unique<GbaProcessor>(user);synthReload->setStateInformation(synthState.getData(),int(synthState.getSize()));synthReload->prepareToPlay(48000,256);
    ensure(synthReload->romHacks.waitUntilIdle(10000)&&!synthReload->romHacks.enabled(),"SoundFont default overrode a saved synthesis project");
    {auto factory=std::make_unique<GbaProcessor>(user);factory->prepareToPlay(48000,256);factory->presets.restoreDefault(false);ensure(factory->romHacks.waitUntilIdle(10000)&&!factory->romHacks.enabled(),"Pending startup recall overrode an explicit factory selection");}
    const auto custom=root.getChildFile(juce::String::fromUTF8("Custom Folder \xc3\xa9"));ensure(custom.createDirectory().wasOk()&&custom.getChildFile("Custom.sf2").replaceWithData(data.data(),data.size()),"Custom folder fixture failed");
    reload->romHacks.setLibraryFolder(custom);ensure(reload->romHacks.waitUntilIdle(10000)&&reload->romHacks.libraryView().files.size()==1,"Custom SoundFont folder scan failed");
    // This instance has the old folder in memory; saving only rememberLast must
    // merge its field without replacing the newer folder chosen above.
    fromProject->romHacks.setRememberLast(false);ensure(fromProject->romHacks.waitUntilIdle(10000),"Recall switch persistence timed out");
    auto fresh=std::make_unique<GbaProcessor>(user);fresh->prepareToPlay(48000,256);ensure(fresh->romHacks.waitUntilIdle(10000),"Custom-folder restart timed out");
    ensure(!fresh->romHacks.enabled()&&!fresh->romHacks.libraryView().rememberLast&&fresh->romHacks.libraryView().folder==custom&&fresh->romHacks.libraryView().names.size()==1&&fresh->romHacks.libraryView().names[0]=="Custom.sf2","Remember opt-out/custom folder did not persist, or another instance lost a preference field");
    fresh->romHacks.setLibraryFolder(root.getChildFile("Unavailable Folder"));ensure(fresh->romHacks.waitUntilIdle(10000)&&fresh->romHacks.libraryView().status.contains("unavailable"),"Missing library folder did not report an error");
    const auto many=root.getChildFile("Many Banks");ensure(many.createDirectory().wasOk(),"Large library fixture directory failed");bool written=true;
    for(int i=0;i<513;++i)written&=many.getChildFile("Bank"+juce::String(i)+".sf2").replaceWithText("");
    ensure(written,"Large library fixtures failed");
    fresh->romHacks.setLibraryFolder(many);ensure(fresh->romHacks.waitUntilIdle(10000)&&fresh->romHacks.libraryView().files.size()==512&&fresh->romHacks.libraryView().status.contains("limited"),"Library scan limit was not enforced");
    fresh->romHacks.setRememberLast(true);fresh.reset();
    ensure(bool(juce::JSON::parse(user.getChildFile("soundfont-library.json"))["rememberLast"]),"Immediate shutdown lost the last library setting");
    fresh=std::make_unique<GbaProcessor>(user);fresh->prepareToPlay(48000,256);ensure(fresh->romHacks.waitUntilIdle(10000)&&fresh->romHacks.enabled(),"Immediate-close preference did not take effect on next startup");
    const auto corruptRoot=root.getChildFile("CorruptLibrary"),corrupt=corruptRoot.getChildFile("soundfont-library.json");ensure(corruptRoot.createDirectory().wasOk()&&corrupt.replaceWithText("corrupt library preferences"),"Corrupt library fixture failed");
    {auto bad=std::make_unique<GbaProcessor>(corruptRoot);bad->prepareToPlay(48000,256);ensure(bad->romHacks.waitUntilIdle(10000)&&bad->romHacks.libraryView().status.contains("preferences")&&!bad->romHacks.enabled(),"Corrupt preferences prevented safe initialization");bad->romHacks.loadFile(alpha);ensure(bad->romHacks.waitUntilIdle(10000)&&bad->romHacks.enabled()&&corrupt.loadFileAsString()=="corrupt library preferences","Bad preferences prevented manual playback or were silently overwritten");}
    std::cout<<"PASS SoundFont library: default/custom folders, bank selector, process restart, last instrument, missing source, project precedence and recall opt-out\n";
}

}
int runSoundFontTests(){
    auto root=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("gba2-sf2-"+juce::Uuid().toString());ensure(root.createDirectory().wasOk(),"SF2 temp directory failed");
    const auto data=sf2fixture::bank();auto source=root.getChildFile("Original Test Bank.sf2");ensure(source.replaceWithData(data.data(),data.size()),"SF2 fixture write failed");
    libraryTests(root,data);
    {
        auto p=std::make_unique<GbaProcessor>(root.getChildFile("User"));p->setRateAndBufferSizeDetails(48000,256);p->prepareToPlay(48000,256);dry(*p);
        p->romHacks.loadFile(source);ensure(p->romHacks.waitUntilIdle(10000),"SF2 loader timed out");auto info=p->romHacks.view();ensure(info.enabled&&!info.loading&&info.instruments.size()==2,"SF2 UI import did not publish a bank");
        ensure(info.instruments[1].contains("2:10")&&info.instruments[1].contains("Fixture Bright"),"SF2 bank/program names missing");
        const auto doc=p->romHacks.saveState();auto cached=juce::File(doc["path"].toString());ensure(cached.existsAsFile()&&cached!=source&&cached.getParentDirectory().getFileName()=="SoundFonts","SF2 bank not copied into app data");
        juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;midi.addEvent(juce::MidiMessage::noteOn(1,69,.8f),128);p->processBlock(audio,midi);ensure(audio.getMagnitude(0,0,128)==0,"SF2 MIDI sample offset ignored");ensure(play(*p)>.001f,"SF2 plugin produced no audible signal");
        p->romHacks.select(1);ensure(p->romHacks.view().selected==1,"SF2 instrument selection failed");ensure(p->presets.saveAs("Not a synth patch").failed(),"SF2 was silently saved as an unrelated synthesis patch");
        juce::MemoryBlock state;p->getStateInformation(state);ensure(state.getSize()<32768,"SoundFont audio was embedded in DAW state");ensure(source.deleteFile(),"Could not remove test-owned source bank");
        auto restored=std::make_unique<GbaProcessor>(root.getChildFile("User"));restored->setStateInformation(state.getData(),int(state.getSize()));restored->setRateAndBufferSizeDetails(48000,256);restored->prepareToPlay(48000,256);
        ensure(restored->romHacks.enabled()&&restored->romHacks.view().selected==1,"SF2 project bank/instrument did not restore");ensure(play(*restored)>.001f,"SF2 first/offline MIDI after project reload was silent");
        auto isolated=std::make_unique<GbaProcessor>(root.getChildFile("Other"));ensure(!isolated->romHacks.enabled(),"SF2 state leaked to another instance");
        auto invalid=root.getChildFile("invalid.sf2");ensure(invalid.replaceWithText("This is not a bank"),"SF2 invalid fixture write failed");restored->romHacks.loadFile(invalid);ensure(restored->romHacks.waitUntilIdle(10000),"Invalid SF2 load did not finish");ensure(restored->romHacks.enabled()&&restored->romHacks.view().instruments.size()==2&&restored->romHacks.view().status.contains("SoundFont"),"Bad SF2 replaced the working bank");
        auto hidden=cached.withFileExtension("away");ensure(cached.moveFileTo(hidden),"Missing-bank fixture rename failed");isolated->setStateInformation(state.getData(),int(state.getSize()));isolated->prepareToPlay(48000,256);ensure(isolated->romHacks.enabled()&&isolated->romHacks.view().instruments.isEmpty()&&isolated->romHacks.view().status.contains("missing"),"Missing SoundFont was silently substituted");ensure(play(*isolated)==0,"Missing SoundFont played an unrelated synth sound");ensure(hidden.moveFileTo(cached),"SF2 fixture restore failed");
        {
            std::unique_ptr<juce::AudioProcessorEditor> editor(p->createEditor());editor->setSize(1280,646);auto* cart=buttonSf(*editor,"ROM Hacks");ensure(cart&&cart->isVisible(),"ROM Hacks cartridge button missing");picture(*editor,"ui-snapshots/ROM-Cartridge-1280.png");cart->onClick();
            ensure(namedSf(*editor,"ROM Hacks browser")!=nullptr&&!cart->isEnabled(),"Cartridge did not open an isolated SoundFont browser");auto* list=dynamic_cast<juce::ComboBox*>(namedSf(*editor,"SoundFont instrument"));ensure(list&&list->getNumItems()==2,"ROM Hacks did not expose instruments");list->setSelectedId(1,juce::sendNotificationSync);ensure(p->romHacks.view().selected==0,"ROM Hacks instrument control is inert");
            editor->setSize(960,485);picture(*editor,"ui-snapshots/ROM-Hacks-960.png");editor->setSize(1920,969);picture(*editor,"ui-snapshots/ROM-Hacks-1920.png");
            buttonSf(*editor,"RETURN TO SYNTH")->onClick();ensure(!p->romHacks.enabled(),"ROM Hacks return-to-synth is inert");buttonSf(*editor,"USE SOUNDFONT")->onClick();ensure(p->romHacks.enabled(),"ROM Hacks re-enable is inert");
            editor->keyPressed(juce::KeyPress(juce::KeyPress::F3Key));ensure(namedSf(*editor,"ROM Hacks browser")==nullptr&&cart->isEnabled(),"F3 did not close ROM Hacks");
            auto* drop=dynamic_cast<juce::FileDragAndDropTarget*>(editor.get());ensure(drop!=nullptr,"Plugin window is not a native file-drop target");
            const auto upper=root.getChildFile(juce::String::fromUTF8("Dropped Bank \xc3\xa9.SF2"));ensure(upper.getFileName().containsChar(0x00e9),"Drop fixture did not retain its Unicode filename");ensure(upper.replaceWithData(data.data(),data.size()),"Drop fixture write failed");
            const juce::StringArray valid{upper.getFullPathName()};
            ensure(drop->isInterestedInFileDrag(valid),"Uppercase SF2 drop with Unicode/spaces was rejected");
            const auto beforeDrop=p->romHacks.saveState();
            for(const auto& rejected:std::vector<juce::StringArray>{{},{"relative.sf2"},{root.getChildFile("bank.sf3").getFullPathName()},{root.getChildFile("bank.wav").getFullPathName()},{cached.getFullPathName(),upper.getFullPathName()}}) {
                ensure(!drop->isInterestedInFileDrag(rejected),"Unsupported or multi-file drag was accepted");drop->filesDropped(rejected,1,1);
                ensure(juce::JSON::toString(p->romHacks.saveState())==juce::JSON::toString(beforeDrop)&&namedSf(*editor,"ROM Hacks browser")==nullptr,"Rejected drop changed bank or opened browser");
            }
            editor->setSize(960,485);drop->fileDragEnter(valid,10,10);picture(*editor,"ui-snapshots/SF2-Drag-960.png");drop->fileDragExit(valid);
            ensure(!p->romHacks.view().loading,"Drag enter/exit started a load before dropping");
            drop->filesDropped(valid,editor->getWidth()/2,editor->getHeight()/2);
            auto* opened=namedSf(*editor,"ROM Hacks browser");ensure(opened&&!cart->isEnabled(),"Dropping did not open ROM Hacks");
            ensure(p->romHacks.waitUntilIdle(10000),"Dropped SoundFont load timed out");juce::MessageManager::getInstance()->runDispatchLoopUntil(120);
            ensure(p->romHacks.enabled()&&p->romHacks.view().fileName==upper.getFileName()&&p->romHacks.view().instruments.size()==2,"Dropped bank did not activate its instruments");
            ensure(play(*p)>.001f,"Dropped SoundFont produced no audible MIDI");
            ensure(juce::File(p->romHacks.saveState()["path"].toString()).getParentDirectory().getFileName()=="SoundFonts","Dropped bank bypassed managed cache");
            drop->filesDropped(valid,20,20);ensure(namedSf(*editor,"ROM Hacks browser")==opened,"Second drop closed or replaced the already-open browser");ensure(p->romHacks.waitUntilIdle(10000),"Second drop timed out");
            const auto good=p->romHacks.saveState();drop->filesDropped(juce::StringArray{invalid.getFullPathName()},0,0);ensure(p->romHacks.waitUntilIdle(10000),"Invalid dropped bank timed out");
            juce::MessageManager::getInstance()->runDispatchLoopUntil(120);
            auto* error=dynamic_cast<juce::Label*>(namedSf(*editor,"SoundFont status"));
            ensure(error&&error->getText()==p->romHacks.view().status&&error->getText().contains("SoundFont"),"Invalid dropped bank did not show an actionable error");
            ensure(juce::JSON::toString(good)==juce::JSON::toString(p->romHacks.saveState())&&play(*p)>.001f,"Invalid drop discarded the working instrument");
            editor->keyPressed(juce::KeyPress(juce::KeyPress::F2Key));ensure(namedSf(*editor,"Advanced tabs")!=nullptr,"Advanced panel missing before drop test");
            drop->filesDropped(valid,40,40);ensure(namedSf(*editor,"Advanced tabs")==nullptr&&namedSf(*editor,"ROM Hacks browser")!=nullptr,"Drop over advanced panel did not open ROM Hacks");ensure(p->romHacks.waitUntilIdle(10000),"Advanced-panel drop timed out");
#if JUCE_WINDOWS
            // Exercise JUCE's native peer target search over child components,
            // rather than calling only our drop callback directly.
            editor->addToDesktop(0);editor->setVisible(true);editor->toFront(false);juce::MessageManager::getInstance()->runDispatchLoopUntil(40);
            auto* peer=editor->getPeer();ensure(peer!=nullptr,"Drop test lacks a native peer");
            auto* instrument=namedSf(*editor,"SoundFont instrument");ensure(instrument!=nullptr,"Drop test instrument control missing");
            juce::ComponentPeer::DragInfo drag;drag.files=valid;drag.position=editor->getLocalPoint(instrument,instrument->getLocalBounds().getCentre());
            ensure(peer->handleDragMove(drag),"Native drag target search over instrument combo rejected SF2");peer->handleDragExit(drag);
            ensure(peer->handleDragMove(drag)&&peer->handleDragDrop(drag),"Native peer did not deliver the SF2 drop");
            juce::MessageManager::getInstance()->runDispatchLoopUntil(120);ensure(p->romHacks.waitUntilIdle(10000)&&p->romHacks.view().fileName==upper.getFileName(),"Native peer drop did not publish bank");
            editor->removeFromDesktop();
#endif
            editor->keyPressed(juce::KeyPress(juce::KeyPress::F3Key));
        }
        p->reset();const auto heap=gba::SoundFont::heapOperations();ensure(play(*p)>.001f,"SF2 stopped after editor closed");ensure(gba::SoundFont::heapOperations()==heap,"Processor SF2 render allocated or freed decoder memory");
        std::atomic<bool> stop{false},finite{true};std::atomic<int> blocks{0};
        std::thread audioThread([&]{juce::AudioBuffer<float> b(2,128);while(!stop){juce::MidiBuffer m;if(blocks.load()%32==0)m.addEvent(juce::MidiMessage::noteOn(1,48+blocks.load()%36,.75f),0);p->processBlock(b,m);for(int c=0;c<2;++c)for(int i=0;i<128;++i)if(!std::isfinite(b.getSample(c,i)))finite=false;++blocks;std::this_thread::yield();}});
        bool loaded=true;int publications=0;
        for(int i=0;i<12;++i){int before=blocks.load();p->romHacks.refreshLibrary();p->romHacks.loadFile(cached);if(!p->romHacks.waitUntilIdle(10000)){loaded=false;break;}++publications;auto until=juce::Time::getMillisecondCounterHiRes()+5000;while(blocks.load()<before+16&&juce::Time::getMillisecondCounterHiRes()<until)juce::Thread::sleep(1);if(blocks.load()<before+16){loaded=false;break;}}
        stop=true;audioThread.join();ensure(loaded&&publications==12&&blocks>=192&&finite,"Concurrent SoundFont publication/audio failed");std::cout<<"PASS "<<publications<<" SoundFont publications across "<<blocks<<" concurrent audio blocks\n";
        p->romHacks.loadFile(cached);p->romHacks.useSoundFont(false);juce::Thread::sleep(30);ensure(!p->romHacks.enabled(),"Canceled SoundFont load re-enabled playback");p->presets.restoreDefault();ensure(!p->romHacks.enabled()&&play(*p)>.001f,"Factory preset did not restore synthesis playback");
    }
    ensure(root.deleteRecursively(),"SF2 temporary data cleanup failed");std::cout<<"PASS "<<sfChecks<<" SoundFont application checks\n";return sfChecks;
}
