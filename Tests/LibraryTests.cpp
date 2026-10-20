#include "PluginProcessor.h"
#include <iostream>
#include <memory>
#include <stdexcept>
#if JUCE_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace {
int checks=0;
void check(bool ok,const char* message){++checks;if(!ok)throw std::runtime_error(message);}
void set(GbaProcessor& p,const char* id,float value){auto* v=p.state.getParameter(id);v->setValueNotifyingHost(v->convertTo0to1(value));}
void json(const juce::File& file,const juce::var& value){check(file.replaceWithText(juce::JSON::toString(value)),"Could not create library test fixture");}
juce::var patch(juce::String id,juce::String name){auto* o=new juce::DynamicObject();juce::var v(o);o->setProperty("format","gba2.preset");o->setProperty("schemaVersion",1);o->setProperty("id",id);o->setProperty("name",name);o->setProperty("tags","test imported");auto* p=new juce::DynamicObject();p->setProperty("filter.cutoff",1234.0);o->setProperty("parameters",juce::var(p));return v;}
}
int runLibraryTests(){checks=0;auto root=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("gba2-library-"+juce::Uuid().toString());
try{
    {
        auto a=std::make_unique<GbaProcessor>(root);auto& m=a->presets;const auto factory=m.all().front().id;
        check(m.all().size()==598,"Factory count/order drifted");check(!root.getChildFile("library.json").exists(),"Construction unexpectedly writes recent history");
        for(const char* category:{"Hip-Hop - Detroit","Hip-Hop - West Coast","Hip-Hop - Florida"}) {
            PresetManager::Filter regional;regional.category=category;regional.source=PresetManager::Source::factory;
            check(m.filtered(regional).size()==(juce::String(category)=="Hip-Hop - Detroit"?60u:40u),"Regional factory category is incomplete");
            for(const char* family:{"piano","bell","pluck","pad","vox"}){regional.query=family;check(!m.filtered(regional).empty(),"Regional family/tag search is incomplete");}
        }
        for(const char* category:{"Hip-Hop - Chicago Drill","Funk","Japan Inspired","80s Pop"}) {
            PresetManager::Filter pack;pack.category=category;pack.source=PresetManager::Source::factory;
            const auto patches=m.filtered(pack);check(patches.size()==(juce::String(category)=="80s Pop"?28u:24u),"Groove category count is wrong");
            for(const auto& patchId:patches){
                check(m.load(patchId)&&a->presetId()==patchId,"Groove preset failed to load");
                const auto expected=m.find(patchId)->settings;const auto actual=a->snapshot();bool matches=true;
                for(std::size_t i=0;i<gba::parameterCount;++i)matches&=std::abs(expected.values[i]-actual.values[i])<=std::max(.00001f,(gba::specs[i].max-gba::specs[i].min)*.000001f);
                check(matches,("Groove preset parameters changed beyond host normalization precision on load: "+m.find(patchId)->name.toStdString()).c_str());
            }
        }
        PresetManager::Filter detroit;detroit.category="Hip-Hop - Detroit";detroit.query="piano";
        check(m.filtered(detroit).size()==30,"Expanded Detroit piano search is incomplete");
        PresetManager::Filter f;f.query="FM bass";auto ids=m.filtered(f);check(!ids.empty(),"AND search across name/category/tags has no results");for(const auto& id:ids){auto* p=m.find(id);check((p->name+" "+p->category+" "+p->tags).containsIgnoreCase("bass"),"AND search admits missing words");}
        f.query="\"early 3D\" retro";ids=m.filtered(f);check(!ids.empty(),"Quoted technical-style search failed");for(const auto& id:ids)check(m.find(id)->tags.containsIgnoreCase("early 3D"),"Quoted phrase was split");
        f={};f.source=PresetManager::Source::user;check(m.filtered(f).empty(),"User source filter includes factory patches");
        set(*a,"filter.cutoff",3210);check(m.saveAs("  Azure Keys  ","custom blue keys").wasOk(),"Tagged Save As failed");auto azure=a->presetId();auto azureFile=m.find(azure)->file;
        check(m.find(azure)->name=="Azure Keys"&&m.find(azure)->tags=="custom blue keys","Save As did not trim/store metadata");check(azureFile.getParentDirectory()==m.userFolder(),"User file escaped preset directory");
        auto count=m.all().size();check(m.saveAs("AZURE KEYS").failed()&&m.all().size()==count,"Case-insensitive duplicate Save As was allowed");check(m.saveAs("line\nbreak").failed(),"Control character in preset name was allowed");check(m.saveAs(juce::String::repeatedString("x",101)).failed(),"Overlong name was allowed");check(m.saveAs("Tags",juce::String::repeatedString("t",513)).failed(),"Overlong tags were allowed");
        check(m.saveAs("Copper Pluck","custom copper").wasOk(),"Second user preset failed");auto copper=a->presetId();check(m.renameUser(copper,"azure keys").failed(),"Rename overwrote an existing name");
        f={};f.source=PresetManager::Source::user;check(m.saveAs("Amber Pad").wasOk()&&m.find(m.filtered(f).front())->name=="Amber Pad","Save As did not maintain user name order");auto amber=a->presetId();check(m.renameUser(amber,"Zinc Pad").wasOk()&&m.filtered(f).back()==amber,"Rename did not maintain user name order");
        check(m.toggleFavorite(azure).wasOk(),"Favorite failed");f={};f.source=PresetManager::Source::user;f.category="User";f.query="custom blue";f.favorites=true;f.recent=true;ids=m.filtered(f);check(ids.size()==1&&ids.front()==azure,"Combined category/source/query/favorite/recent filters failed");
        auto b=std::make_unique<GbaProcessor>(root);auto bytes=root.getChildFile("library.json").loadFileAsString();auto c=std::make_unique<GbaProcessor>(root);check(root.getChildFile("library.json").loadFileAsString()==bytes,"A new instance reordered history");c.reset();
        check(b->presets.toggleFavorite(copper).wasOk(),"Other-instance favorite failed");check(m.toggleFavorite(factory).wasOk(),"Merged favorite failed");b->presets.refresh();check(b->presets.isFavorite(azure)&&b->presets.isFavorite(copper)&&b->presets.isFavorite(factory),"Sequential instance updates lost favorites");
        m.load(copper);m.load(azure);f={};f.recent=true;ids=m.filtered(f);check(ids.size()>=2&&ids[0]==azure&&ids[1]==copper,"Recent filter is not MRU ordered");m.load(azure);check(m.recentIds().indexOf(azure)==0&&m.recentIds().size()>=2,"Recent deduplication failed");
        for(int i=0;i<40;++i)m.load(m.all()[std::size_t(i)].id);
        check(m.recentIds().size()==32,"Recent history is not bounded to 32");
        auto before=a->snapshot();auto active=a->presetId();auto importedId="user."+juce::Uuid().toDashedString();auto imported=m.userFolder().getChildFile("manual.gba2preset");auto v=patch(importedId,"Imported Zebra");auto* params=v["parameters"].getDynamicObject();params->setProperty("voice.count",99);params->setProperty("future.parameter",2);json(imported,v);check(m.refresh().wasOk(),"Refresh did not import valid external preset");check(m.find(importedId)&&m.find(importedId)->settings[gba::Param::Voices]==32,"Imported defaults/clamping/unknown parameter handling failed");check(a->snapshot().values==before.values&&a->presetId()==active,"Refresh replaced the active project sound");
        auto disk=juce::JSON::parse(azureFile);disk["parameters"].getDynamicObject()->setProperty("filter.cutoff",6789);json(azureFile,disk);check(m.renameUser(azure,"Azure Renamed").wasOk(),"Rename of updated disk file failed");check(float(juce::JSON::parse(azureFile)["parameters"]["filter.cutoff"])==6789,"Rename overwrote an external parameter edit");
#if JUCE_WINDOWS
        auto oldFile=azureFile.loadFileAsString();auto oldName=m.find(azure)->name;auto handle=CreateFileW(azureFile.getFullPathName().toWideCharPointer(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        check(handle!=INVALID_HANDLE_VALUE,"Could not lock Windows replacement destination");auto denied=m.renameUser(azure,"Locked Rename");CloseHandle(handle);
        check(denied.failed()&&azureFile.loadFileAsString()==oldFile&&m.find(azure)->name==oldName,"Failed native replacement changed file/model");
#endif
        auto original=m.find(azure)->settings.values;disk=juce::JSON::parse(azureFile);disk.getDynamicObject()->setProperty("id","user."+juce::Uuid().toString());json(azureFile,disk);check(m.deleteUser(azure).failed()&&azureFile.existsAsFile(),"Delete removed a file whose identity changed");disk.getDynamicObject()->setProperty("id",azure);json(azureFile,disk);m.refresh();check(m.find(azure)->settings.values==original,"External identity recovery changed stored parameters");
        check(m.renameUser(factory,"Factory Rename").failed()&&m.deleteUser(factory).failed(),"Factory protection failed");
        auto metadata=root.getChildFile("library.json");auto metadataText=metadata.loadFileAsString();check(metadata.deleteFile()&&metadata.createDirectory().wasOk(),"Could not block metadata destination");bool favorite=m.isFavorite(factory);check(m.toggleFavorite(factory).failed()&&m.isFavorite(factory)==favorite,"Failed favorite persistence changed in-memory state");check(metadata.isDirectory(),"Atomic write replaced a directory");check(m.load(factory)&&a->presetId()==factory&&m.lastError.isNotEmpty(),"Metadata failure prevented sound loading or hid warning");check(metadata.deleteFile()&&metadata.replaceWithText(metadataText),"Could not restore metadata fixture");
        check(metadata.replaceWithText("{broken"),"Could not corrupt metadata fixture");check(m.toggleFavorite(factory).failed()&&metadata.loadFileAsString()=="{broken","Corrupt metadata was silently overwritten");check(metadata.replaceWithText(metadataText),"Could not recover metadata fixture");m.refresh();
        m.load(azure);if(!m.isFavorite(azure))m.toggleFavorite(azure);check(m.deleteUser(azure).wasOk(),"User deletion failed");check(!azureFile.exists()&&!m.find(azure)&&!m.isFavorite(azure)&&!m.isRecent(azure),"Deletion retained file/library metadata");check(a->presetName()=="Dream Pad","Deleting active preset did not restore default");b.reset();
        // MIDI program events must remain queued until the non-audio service runs.
        a->setRateAndBufferSizeDetails(48000,64);a->prepareToPlay(48000,64);auto start=a->presetId();auto metadataBefore=metadata.loadFileAsString();juce::AudioBuffer<float> audio(2,64);juce::MidiBuffer midi;midi.addEvent(juce::MidiMessage::programChange(1,1),0);a->processBlock(audio,midi);check(a->presetId()==start&&metadata.loadFileAsString()==metadataBefore,"Audio callback applied a program/file update");a->servicePendingProgram();check(a->getCurrentProgram()==1,"Deferred program service failed");a->releaseResources();
    }
    {
        auto invalidRoot=root.getChildFile("Malformed");auto p=std::make_unique<GbaProcessor>(invalidRoot);auto folder=p->presets.userFolder();check(folder.createDirectory().wasOk(),"Could not create malformed fixture folder");auto goodId="user."+juce::Uuid().toString();auto good=patch(goodId,"Valid Baseline");json(folder.getChildFile("a-good.gba2preset"),good);
        const auto bad=[&](const juce::String& file,juce::var data){json(folder.getChildFile(file+".gba2preset"),data);};
        bad("duplicate",good);auto v=patch("user.invalid","Bad ID");bad("bad-id",v);v=patch("user."+juce::Uuid().toString(),"Version");v.getDynamicObject()->setProperty("schemaVersion",1.5);bad("version",v);v=patch("user."+juce::Uuid().toString(),"Bool");v["parameters"].getDynamicObject()->setProperty("volume",true);bad("bool",v);v=patch("user."+juce::Uuid().toString(),"Missing");v.getDynamicObject()->removeProperty("parameters");bad("missing",v);v=patch("user."+juce::Uuid().toString(),"Name\nBad");bad("name",v);check(folder.getChildFile("broken.gba2preset").replaceWithText("{broken"),"Could not write invalid JSON fixture");check(folder.getChildFile("big.gba2preset").replaceWithText(juce::String::repeatedString("x",1024*1024+1)),"Could not write oversized fixture");
        check(p->presets.refresh().failed()&&p->presets.all().size()==std::size_t(599)&&p->presets.find(goodId),"Malformed/duplicate/oversized presets were accepted or valid patch lost");
        auto legacy=invalidRoot.getChildFile("library.json");auto* o=new juce::DynamicObject();juce::var prefs(o);juce::Array<juce::var> favorites;favorites.add(goodId);favorites.add("factory.unknown");favorites.add(42);o->setProperty("favorites",favorites);juce::Array<juce::var> recents;recents.add(goodId);recents.add(goodId);o->setProperty("recent",recents);json(legacy,prefs);p->presets.refresh();check(p->presets.isFavorite(goodId)&&p->presets.recentIds().size()==1,"Legacy preference validation/migration failed");check(p->presets.toggleFavorite(goodId).wasOk()&&juce::JSON::parse(legacy)["format"].toString()=="gba2.library","Legacy metadata was not versioned after update");
    }
    {
        auto p=std::make_unique<GbaProcessor>(root.getChildFile("ImportExport"));auto& m=p->presets;
        auto exports=root.getChildFile("Shared");auto file=exports.getChildFile("Shared Keys.gba2preset");
        set(*p,"engine",1);set(*p,"fm.index",3.7f);set(*p,"filter.cutoff",3271);set(*p,"voice.count",8);
        const auto settings=p->snapshot();const auto id=p->presetId();const auto count=m.all().size();
        check(m.exportCurrent(file,"Shared Keys","custom fm keys").wasOk(),"Could not export synthesis preset");
        check(m.all().size()==count&&p->presetId()==id&&p->snapshot().values==settings.values,"Export changed library or active sound");
        const auto data=juce::JSON::parse(file);check(data["format"].toString()=="gba2.preset"&&data["parameters"].getDynamicObject()->getProperties().size()==int(gba::parameterCount),"Export did not include all stable synthesis parameters");
        check(m.importPreset(file).wasOk(),"Could not import exported synthesis preset");const auto imported=p->presetId();
        check(imported!=data["id"].toString()&&m.all().size()==count+1,"Import reused an external identity or omitted the patch");
        check(p->snapshot().values==settings.values&&m.find(imported)->tags=="custom fm keys","Import changed synthesis parameters or tags");
        check(m.find(imported)->file.getParentDirectory()==m.userFolder()&&file.existsAsFile(),"Import moved the external source or escaped the library");
        check(m.importPreset(file).wasOk()&&p->presetName()=="Shared Keys (2)"&&p->presetId()!=imported,"Repeated import overwrote a preset identity/name");
        auto reloaded=std::make_unique<GbaProcessor>(root.getChildFile("ImportExport"));check(reloaded->presets.load(imported)&&reloaded->snapshot().values==settings.values,"Imported patch failed to reload from disk");
        const auto stableId=p->presetId();const auto stableSize=m.all().size();
        check(m.importPreset(exports.getChildFile("Instrument.mse")).failed()&&m.lastError.containsIgnoreCase("samples"),"MSE was falsely accepted as synthesis");
        check(m.importPreset(exports.getChildFile("Instrument.chl")).failed(),"Unknown CHL format was falsely accepted");
        check(m.importPreset(exports.getChildFile("Instrument.wav")).failed(),"Sample import was allowed");
        auto broken=exports.getChildFile("broken.gba2preset");check(broken.replaceWithText("{broken")&&m.importPreset(broken).failed(),"Malformed external JSON was imported");
        auto invalid=patch("user."+juce::Uuid().toString(),"Invalid");invalid["parameters"].getDynamicObject()->setProperty("volume","bad");json(broken,invalid);check(m.importPreset(broken).failed(),"Invalid parameter type was imported");
        check(broken.replaceWithText(juce::String::repeatedString("x",1024*1024+1))&&m.importPreset(broken).failed(),"Oversized external file was imported");
        check(m.exportCurrent(exports.getChildFile("wrong.mse")).failed(),"Export mislabeled a synthesis file as MSE");
        const auto existing=m.find(imported)->file;const auto bytes=existing.loadFileAsString();check(m.exportCurrent(existing,"Overwrite").failed()&&existing.loadFileAsString()==bytes,"Export overwrote a library preset");
        auto blocked=exports.getChildFile("blocked.gba2preset");check(blocked.createDirectory().wasOk()&&m.exportCurrent(blocked).failed(),"Export overwrote a directory");
        check(p->presetId()==stableId&&p->snapshot().values==settings.values&&m.all().size()==stableSize,"Failed import/export changed the current sound or library");
    }
    {
        auto blocked=root.getChildFile("Blocked");check(blocked.createDirectory().wasOk()&&blocked.getChildFile("Presets").replaceWithText("occupied"),"Could not block preset folder");auto p=std::make_unique<GbaProcessor>(blocked);auto before=p->snapshot();auto id=p->presetId();check(p->presets.saveAs("No Write").failed()&&p->presets.all().size()==598,"Failed Save As changed library");check(p->snapshot().values==before.values&&p->presetId()==id,"Failed Save As changed active sound");
    }
    root.deleteRecursively();std::cout<<"All "<<checks<<" Phase 9 library checks passed.\n";return checks;
}catch(...){root.deleteRecursively();throw;}}
