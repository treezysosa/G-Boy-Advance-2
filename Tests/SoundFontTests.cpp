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
}
int runSoundFontTests(){
    auto root=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("gba2-sf2-"+juce::Uuid().toString());ensure(root.createDirectory().wasOk(),"SF2 temp directory failed");
    const auto data=sf2fixture::bank();auto source=root.getChildFile("Original Test Bank.sf2");ensure(source.replaceWithData(data.data(),data.size()),"SF2 fixture write failed");
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
            const auto upper=root.getChildFile("Dropped Bank \xc3\xa9.SF2");ensure(upper.replaceWithData(data.data(),data.size()),"Drop fixture write failed");
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
        for(int i=0;i<12;++i){int before=blocks.load();p->romHacks.loadFile(cached);if(!p->romHacks.waitUntilIdle(10000)){loaded=false;break;}++publications;auto until=juce::Time::getMillisecondCounterHiRes()+5000;while(blocks.load()<before+16&&juce::Time::getMillisecondCounterHiRes()<until)juce::Thread::sleep(1);if(blocks.load()<before+16){loaded=false;break;}}
        stop=true;audioThread.join();ensure(loaded&&publications==12&&blocks>=192&&finite,"Concurrent SoundFont publication/audio failed");std::cout<<"PASS "<<publications<<" SoundFont publications across "<<blocks<<" concurrent audio blocks\n";
        p->romHacks.loadFile(cached);p->romHacks.useSoundFont(false);juce::Thread::sleep(30);ensure(!p->romHacks.enabled(),"Canceled SoundFont load re-enabled playback");p->presets.restoreDefault();ensure(!p->romHacks.enabled()&&play(*p)>.001f,"Factory preset did not restore synthesis playback");
    }
    ensure(root.deleteRecursively(),"SF2 temporary data cleanup failed");std::cout<<"PASS "<<sfChecks<<" SoundFont application checks\n";return sfChecks;
}
