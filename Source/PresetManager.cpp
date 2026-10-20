#include "PresetManager.h"
#include "PluginProcessor.h"
#include "FactoryData.h"
#if JUCE_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cstdio>
#endif
namespace {
// Serializes library read/modify/write among plugin instances in one process.
// Never entered by processBlock. Separate DAW processes can still race metadata writes.
juce::CriticalSection libraryIo;
constexpr juce::int64 maxBytes=1024*1024;
bool validName(const juce::String& value){auto name=value.trim();if(name.isEmpty()||name.length()>100)return false;for(auto c:name)if(c<32||c==127)return false;return true;}
bool userId(const juce::String& id){if(!id.startsWith("user."))return false;auto value=id.substring(5);if(value.length()!=32&&value.length()!=36)return false;auto uuid=juce::Uuid(value);return !uuid.isNull()&&(value.equalsIgnoreCase(uuid.toString())||value.equalsIgnoreCase(uuid.toDashedString()));}
bool version(const juce::var& v){return (v.isInt()||v.isInt64())&&int(v)==1;}
juce::Result atomicJson(const juce::File& destination,const juce::var& value){
    if(destination.isSymbolicLink()||(destination.exists()&&!destination.existsAsFile()))return juce::Result::fail("The destination is not a regular file.");
    auto directory=destination.getParentDirectory().createDirectory();if(directory.failed())return directory;
    juce::TemporaryFile temp(destination);
    {juce::FileOutputStream stream(temp.getFile());if(!stream.openedOk()||!stream.writeText(juce::JSON::toString(value),false,false,"\n"))return juce::Result::fail("Could not write the temporary file.");stream.flush();if(stream.getStatus().failed())return stream.getStatus();}
    // Same-directory native replacement. No copy/delete fallback and no retry sleeps.
#if JUCE_WINDOWS
    const bool ok=MoveFileExW(temp.getFile().getFullPathName().toWideCharPointer(),destination.getFullPathName().toWideCharPointer(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
    const bool ok=std::rename(temp.getFile().getFullPathName().toRawUTF8(),destination.getFullPathName().toRawUTF8())==0;
#endif
    return ok?juce::Result::ok():juce::Result::fail("Could not replace the file; the previous file was retained.");
}
juce::Result boundedJson(const juce::File& file,juce::var& result){
    juce::FileInputStream stream(file);if(!stream.openedOk())return stream.getStatus();juce::MemoryBlock bytes;
    stream.readIntoMemoryBlock(bytes,std::size_t(maxBytes+1));if(stream.getStatus().failed())return stream.getStatus();if(bytes.getSize()>std::size_t(maxBytes))return juce::Result::fail("JSON file exceeds 1 MiB.");
    return juce::JSON::parse(juce::String::createStringFromData(bytes.getData(),int(bytes.getSize())),result);
}

}
PresetManager::PresetManager(GbaProcessor& p,juce::File dataRoot):processor(p){
    folder=(dataRoot==juce::File()?juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Treezy Audio/G Boy Advance 2"):dataRoot).getChildFile("Presets");preferences=folder.getParentDirectory().getChildFile("library.json");
    for(const auto& patch:gba::factoryPatches){GbaPreset preset;preset.id=patch.id;preset.name=patch.name;preset.category=patch.category;preset.subcategory=patch.subcategory;preset.tags=patch.tags;preset.settings.values=patch.values;presets.push_back(std::move(preset));}
    refresh();
}
juce::Result PresetManager::finish(juce::Result result){lastError=result.failed()?result.getErrorMessage():juce::String();return result;}
const GbaPreset* PresetManager::find(const juce::String& id) const {for(const auto& p:presets)if(p.id==id)return &p;return nullptr;}
juce::StringArray PresetManager::categories() const {juce::StringArray result;for(const auto& p:presets)result.addIfNotAlreadyThere(p.category);return result;}
juce::StringArray PresetManager::subcategories(const juce::String& category) const {juce::StringArray result;for(const auto& p:presets)if(p.category==category&&p.subcategory.isNotEmpty())result.addIfNotAlreadyThere(p.subcategory);return result;}
std::vector<juce::String> PresetManager::filtered(const Filter& filter) const {
    auto words=juce::StringArray::fromTokens(filter.query," \t\r\n","\"");std::vector<juce::String> result;
    for(const auto& p:presets){
        if(filter.category.isNotEmpty()&&p.category!=filter.category)continue;
        if(filter.subcategory.isNotEmpty()&&p.subcategory!=filter.subcategory)continue;
        if(filter.source==Source::factory&&!p.factory)continue;
        if(filter.source==Source::user&&p.factory)continue;
        if(filter.favorites&&!isFavorite(p.id))continue;
        if(filter.recent&&!isRecent(p.id))continue;
        auto text=p.name+" "+p.category+" "+p.subcategory+" "+p.tags;bool matches=true;
        for(auto word:words){word=word.trim().unquoted();if(word.isNotEmpty()&&!text.containsIgnoreCase(word)){matches=false;break;}}
        if(matches)result.push_back(p.id);
    }
    if(filter.recent)std::stable_sort(result.begin(),result.end(),[this](const auto& a,const auto& b){return recent.indexOf(a)<recent.indexOf(b);});
    return result;
}
juce::Result PresetManager::read(const juce::File& file,GbaPreset& p,bool external) const {
    if((!external&&file.getParentDirectory()!=folder)||file.isSymbolicLink()||!file.existsAsFile()||file.getSize()>maxBytes)return juce::Result::fail("Missing, linked or oversized preset: "+file.getFileName());
    juce::var data;auto parsed=boundedJson(file,data);if(parsed.failed()||!data.isObject())return juce::Result::fail("Invalid JSON preset: "+file.getFileName());
    if(data.hasProperty("sample"))return juce::Result::fail("Sample playback was removed. This preset was left unchanged: "+file.getFileName());
    if(data["format"].toString()!="gba2.preset"||!version(data["schemaVersion"]))return juce::Result::fail("Unsupported preset: "+file.getFileName());
    if(!data["id"].isString()||!userId(data["id"].toString())||!data["name"].isString()||!validName(data["name"].toString()))return juce::Result::fail("Invalid preset identity/name: "+file.getFileName());
    p={};p.id=data["id"].toString();p.name=data["name"].toString().trim();p.category="User";p.file=file;p.factory=false;
    auto tags=data["tags"];if(tags.isString())p.tags=tags.toString();else if(auto* array=tags.getArray()){for(const auto& tag:*array){if(!tag.isString())return juce::Result::fail("Invalid preset tags.");p.tags+=tag.toString()+" ";}p.tags=p.tags.trim();}else if(!tags.isVoid())return juce::Result::fail("Invalid preset tags.");
    if(p.tags.length()>512)return juce::Result::fail("Preset tags exceed 512 characters.");
    auto* parameters=data["parameters"].getDynamicObject();if(!parameters)return juce::Result::fail("Missing synthesis parameters: "+file.getFileName());
    for(std::size_t i=0;i<gba::parameterCount;++i){auto key=juce::Identifier(juce::String(gba::specs[i].id.data()));if(parameters->hasProperty(key)){auto v=parameters->getProperty(key);if(!(v.isDouble()||v.isInt()||v.isInt64())||!std::isfinite(double(v))||!std::isfinite(float(v)))return juce::Result::fail("Invalid synthesis parameter: "+file.getFileName());p.settings.set(static_cast<gba::Param>(i),float(v));}}
    return juce::Result::ok();
}
juce::Result PresetManager::readPreferences(std::set<juce::String>& f,juce::StringArray& r,bool merging) const {
    f.clear();r.clear();if(!preferences.exists())return juce::Result::ok();
    if(preferences.isSymbolicLink()||!preferences.existsAsFile()||preferences.getSize()>maxBytes)return juce::Result::fail("Library metadata is not a regular file or exceeds 1 MiB.");
    juce::var data;auto parsed=boundedJson(preferences,data);if(parsed.failed()||!data.isObject())return juce::Result::fail("Library metadata contains invalid JSON; the file was retained.");
    // Unversioned 0.1–0.8 metadata migrates on the next successful explicit update.
    if((data.hasProperty("format")||data.hasProperty("schemaVersion"))&&(data["format"].toString()!="gba2.library"||!version(data["schemaVersion"])))return juce::Result::fail("Unsupported library metadata; the file was retained.");
    if((!data["favorites"].isVoid()&&!data["favorites"].isArray())||(!data["recent"].isVoid()&&!data["recent"].isArray()))return juce::Result::fail("Invalid favorites/history metadata; the file was retained.");
    const auto accepted=[&](const juce::var& v){return v.isString()&&(find(v.toString())||(merging&&userId(v.toString())));};
    if(auto* a=data["favorites"].getArray())for(const auto& id:*a)if(accepted(id))f.insert(id.toString());
    if(auto* a=data["recent"].getArray())for(const auto& id:*a)if(accepted(id)&&r.size()<32)r.addIfNotAlreadyThere(id.toString());
    return juce::Result::ok();
}
void PresetManager::sortUsers(){
    auto first=std::find_if(presets.begin(),presets.end(),[](const auto& p){return !p.factory;});std::stable_sort(first,presets.end(),[](const auto& a,const auto& b){auto n=a.name.compareIgnoreCase(b.name);return n==0?a.id<b.id:n<0;});
}
juce::Result PresetManager::refresh(){
    const juce::ScopedLock lock(libraryIo);presets.erase(std::remove_if(presets.begin(),presets.end(),[](const auto& p){return !p.factory;}),presets.end());
    juce::String warning;auto files=folder.findChildFiles(juce::File::findFiles,false,"*.gba2preset");files.sort();
    for(const auto& file:files){GbaPreset p;auto result=read(file,p);if(result.failed()){warning=result.getErrorMessage();continue;}if(find(p.id)){warning="Skipped duplicate preset ID: "+file.getFileName();continue;}presets.push_back(std::move(p));}
    sortUsers();
    std::set<juce::String> f;juce::StringArray r;auto result=readPreferences(f,r,false);if(result.wasOk()){favorites=std::move(f);recent=std::move(r);}else warning=result.getErrorMessage();
    ++generation;return finish(warning.isEmpty()?juce::Result::ok():juce::Result::fail(warning));
}
bool PresetManager::load(juce::String id){if(auto* p=find(id)){auto copy=*p;processor.applyPreset(copy);auto result=remember(copy.id);if(result.failed())lastError="Preset loaded; history could not be saved: "+result.getErrorMessage();return true;}lastError="Preset not found. Refresh the library.";return false;}
void PresetManager::step(int direction){if(presets.empty())return;int selected=0;for(std::size_t i=0;i<presets.size();++i)if(presets[i].id==processor.presetId()){selected=int(i);break;}const int count=int(presets.size());selected=((selected+direction)%count+count)%count;load(presets[std::size_t(selected)].id);}
void PresetManager::stepCategory(int direction){auto list=categories();if(list.isEmpty())return;int selected=std::max(0,list.indexOf(processor.presetCategory()));selected=((selected+direction)%list.size()+list.size())%list.size();for(const auto& p:presets)if(p.category==list[selected]){load(p.id);return;}}
bool PresetManager::nameExists(const juce::String& name,const juce::String& except) const {for(const auto& p:presets)if(!p.factory&&p.id!=except&&p.name.equalsIgnoreCase(name.trim()))return true;return false;}
juce::Result PresetManager::write(const GbaPreset& p,bool external){
    if((!external&&p.file.getParentDirectory()!=folder)||!userId(p.id))return juce::Result::fail("Invalid user preset destination.");
    auto* object=new juce::DynamicObject();juce::var data(object);object->setProperty("format","gba2.preset");object->setProperty("schemaVersion",schemaVersion);object->setProperty("id",p.id);object->setProperty("name",p.name);object->setProperty("category","User");object->setProperty("tags",p.tags);
    auto* parameters=new juce::DynamicObject();object->setProperty("parameters",juce::var(parameters));for(std::size_t i=0;i<gba::parameterCount;++i)parameters->setProperty(juce::Identifier(juce::String(gba::specs[i].id.data())),p.settings.values[i]);return atomicJson(p.file,data);
}
juce::Result PresetManager::writePreferences(const std::set<juce::String>& f,const juce::StringArray& r){auto* object=new juce::DynamicObject();juce::var data(object);juce::Array<juce::var> fa,ra;for(const auto& id:f)fa.add(id);for(const auto& id:r)ra.add(id);object->setProperty("format","gba2.library");object->setProperty("schemaVersion",1);object->setProperty("favorites",fa);object->setProperty("recent",ra);return atomicJson(preferences,data);}
juce::Result PresetManager::saveAs(juce::String name,juce::String tags){
    const juce::ScopedLock lock(libraryIo);if(processor.romHacks.enabled())return finish(juce::Result::fail("SoundFont selections are saved in the DAW project. Return to synthesis before saving a synthesis preset."));if(!validName(name))return finish(juce::Result::fail("Enter 1 to 100 characters without control characters."));if(tags.length()>512)return finish(juce::Result::fail("Tags must contain at most 512 characters."));
    // Refresh user files before authoring so another instance's names are respected.
    refresh();if(nameExists(name))return finish(juce::Result::fail("A user preset with this name already exists."));
    GbaPreset p;p.name=name.trim();p.tags=tags.trim().isEmpty()?"user synthesis":tags.trim();p.id="user."+juce::Uuid().toString();p.factory=false;p.category="User";p.settings=processor.snapshot();p.file=folder.getChildFile(p.id+".gba2preset");auto result=write(p);if(result.failed())return finish(result);
    presets.push_back(p);sortUsers();++generation;load(p.id);return juce::Result::ok();
}
juce::Result PresetManager::importPreset(const juce::File& source){
    const juce::ScopedLock lock(libraryIo);
    if(source.hasFileExtension("mse"))return finish(juce::Result::fail("Maize .mse instruments contain samples. This synthesizer imports .gba2preset synthesis patches only."));
    if(source.hasFileExtension("chl"))return finish(juce::Result::fail("This .chl instrument format is not supported. Export a G Boy Advance 2 .gba2preset synthesis patch instead."));
    if(!source.hasFileExtension("gba2preset"))return finish(juce::Result::fail("Choose a .gba2preset synthesis patch."));
    GbaPreset p;auto result=read(source,p,true);if(result.failed())return finish(result);
    refresh();
    // Imported patches are independent copies; never overwrite an existing identity or name.
    const auto original=p.name;
    for(int n=2;nameExists(p.name);++n){const auto suffix=" ("+juce::String(n)+")";p.name=original.substring(0,100-suffix.length())+suffix;}
    p.id="user."+juce::Uuid().toString();p.file=folder.getChildFile(p.id+".gba2preset");
    result=write(p);if(result.failed())return finish(result);
    presets.push_back(p);sortUsers();++generation;load(p.id);return juce::Result::ok();
}
juce::Result PresetManager::exportCurrent(const juce::File& destination,juce::String name,juce::String tags){
    if(processor.romHacks.enabled())return finish(juce::Result::fail("SoundFont selections are saved in the DAW project. Return to synthesis before exporting a synthesis preset."));
    const juce::ScopedLock lock(libraryIo);
    if(!destination.hasFileExtension("gba2preset"))return finish(juce::Result::fail("Use the .gba2preset extension."));
    if(destination.getParentDirectory()==folder)return finish(juce::Result::fail("Use Save As to store sounds in your preset library. Export to another folder."));
    if(name.trim().isEmpty())name=processor.presetName();
    if(!validName(name)||tags.length()>512)return finish(juce::Result::fail("Use a name of 1 to 100 characters and tags of at most 512 characters."));
    GbaPreset p;p.id="user."+juce::Uuid().toString();p.name=name.trim();p.tags=tags.trim();p.settings=processor.snapshot();p.file=destination;
    return finish(write(p,true));
}
juce::Result PresetManager::renameUser(juce::String id,juce::String name){
    const juce::ScopedLock lock(libraryIo);if(!validName(name))return finish(juce::Result::fail("Enter 1 to 100 characters without control characters."));
    auto* found=find(id);if(!found)return finish(juce::Result::fail("Preset not found."));if(found->factory)return finish(juce::Result::fail("Factory patches are read-only. Use Save As."));auto file=found->file;
    refresh();if(nameExists(name,id))return finish(juce::Result::fail("A user preset with this name already exists."));
    GbaPreset copy;auto result=read(file,copy);if(result.failed()||copy.id!=id)return finish(juce::Result::fail("The preset file changed or is missing. Refresh the library."));copy.name=name.trim();result=write(copy);if(result.failed())return finish(result);
    for(auto& p:presets)if(p.id==id)p=copy;
    sortUsers();if(processor.presetId()==id)processor.renameCurrentPreset(copy.name);
    ++generation;return finish(juce::Result::ok());
}
juce::Result PresetManager::deleteUser(juce::String id){
    const juce::ScopedLock lock(libraryIo);lastError.clear();auto* found=find(id);if(!found)return finish(juce::Result::fail("Preset not found."));if(found->factory)return finish(juce::Result::fail("Factory patches cannot be deleted."));auto file=found->file;GbaPreset copy;auto result=read(file,copy);if(result.failed()||copy.id!=id)return finish(juce::Result::fail("The preset file changed or is missing. Refresh the library."));
    if(!file.deleteFile())return finish(juce::Result::fail("Could not delete the preset file."));
    bool active=processor.presetId()==id;presets.erase(std::remove_if(presets.begin(),presets.end(),[&](const auto& p){return p.id==id;}),presets.end());favorites.erase(id);recent.removeString(id);++generation;
    std::set<juce::String> f;juce::StringArray r;result=readPreferences(f,r,true);if(result.wasOk()){f.erase(id);r.removeString(id);result=writePreferences(f,r);if(result.wasOk()){favorites=std::move(f);recent=std::move(r);}}
    auto warning=result.failed()?"Preset deleted; metadata could not be saved: "+result.getErrorMessage():juce::String();if(active)restoreDefault();if(warning.isNotEmpty())lastError=warning;return juce::Result::ok();
}
void PresetManager::restoreDefault(bool remember){for(const auto& p:presets)if(p.name=="Dream Pad"&&p.factory){if(remember)load(p.id);else processor.applyPreset(p);return;}}
bool PresetManager::isFavorite(const juce::String& id) const {return favorites.count(id)>0;}
juce::Result PresetManager::toggleFavorite(juce::String id){const juce::ScopedLock lock(libraryIo);if(!find(id))return finish(juce::Result::fail("Preset not found."));std::set<juce::String> f;juce::StringArray r;auto result=readPreferences(f,r,true);if(result.failed())return finish(result);if(!f.erase(id))f.insert(id);result=writePreferences(f,r);if(result.wasOk()){favorites=std::move(f);recent=std::move(r);++generation;}return finish(result);}
bool PresetManager::isRecent(const juce::String& id) const {return recent.contains(id);}
juce::Result PresetManager::remember(const juce::String& id){const juce::ScopedLock lock(libraryIo);std::set<juce::String> f;juce::StringArray r;auto result=readPreferences(f,r,true);if(result.failed())return finish(result);r.removeString(id);r.insert(0,id);while(r.size()>32)r.remove(r.size()-1);result=writePreferences(f,r);if(result.wasOk()){favorites=std::move(f);recent=std::move(r);++generation;}return finish(result);}
