#include "PresetManager.h"
#include "PluginProcessor.h"
#include "FactoryData.h"

namespace {
juce::Result atomicJson(const juce::File& destination,const juce::var& value) {
    auto directory=destination.getParentDirectory().createDirectory();if(directory.failed())return directory;
    juce::TemporaryFile temp(destination);
    if(!temp.getFile().replaceWithText(juce::JSON::toString(value)))return juce::Result::fail("Could not write temporary preset file.");
    return temp.overwriteTargetFileWithTemporary()?juce::Result::ok():juce::Result::fail("Could not replace preset file.");
}
bool validName(const juce::String& name) {return name.trim().isNotEmpty()&&name.trim().length()<=100;}
}
PresetManager::PresetManager(GbaProcessor& p,juce::File dataRoot):processor(p) {
    folder=(dataRoot==juce::File()?juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Treezy Audio/G Boy Advance 2"):dataRoot).getChildFile("Presets");
    preferences=folder.getParentDirectory().getChildFile("library.json");
    for(const auto& patch:gba::factoryPatches) {
        GbaPreset preset;preset.id=patch.id;preset.name=patch.name;preset.category=patch.category;preset.tags=patch.tags;preset.settings.values=patch.values;
        presets.push_back(std::move(preset));
    }
    scanUsers();
    if(preferences.existsAsFile()&&preferences.getSize()<1024*1024) {
        auto data=juce::JSON::parse(preferences);
        if(auto* list=data["favorites"].getArray())for(const auto& id:*list)favorites.insert(id.toString());
        if(auto* list=data["recent"].getArray())for(const auto& id:*list)if(recent.size()<32)recent.addIfNotAlreadyThere(id.toString());
    }
}
const GbaPreset* PresetManager::find(const juce::String& id) const {for(const auto& p:presets)if(p.id==id)return &p;return nullptr;}
juce::StringArray PresetManager::categories() const {juce::StringArray result;for(const auto& p:presets)result.addIfNotAlreadyThere(p.category);return result;}
void PresetManager::scanUsers() {
    auto files=folder.findChildFiles(juce::File::findFiles,false,"*.gba2preset");files.sort();
    for(const auto& file:files) {
        if(file.getSize()>1024*1024) {lastError="Skipped oversized preset: "+file.getFileName();continue;}
        auto data=juce::JSON::parse(file);
        if(data["format"].toString()!="gba2.preset"||int(data["schemaVersion"])!=schemaVersion) {lastError="Skipped unsupported preset: "+file.getFileName();continue;}
        GbaPreset p;p.id=data["id"].toString();p.name=data["name"].toString();p.category="User";p.tags=data["tags"].toString();p.file=file;p.factory=false;
        auto params=data["parameters"];bool valid=params.isObject()&&p.id.startsWith("user.")&&validName(p.name)&&!find(p.id);
        for(std::size_t i=0;i<gba::parameterCount;++i) {
            auto id=juce::Identifier(juce::String(gba::specs[i].id.data()));
            if(auto* object=params.getDynamicObject();object&&object->hasProperty(id)) {
                auto v=object->getProperty(id);bool numeric=v.isDouble()||v.isInt()||v.isInt64();float value=float(v);
                if(!numeric||!std::isfinite(value))valid=false;
                p.settings.set(static_cast<gba::Param>(i),value);
            }
        }
        if(valid)presets.push_back(std::move(p));else lastError="Skipped invalid preset: "+file.getFileName();
    }
}
bool PresetManager::load(const juce::String& id) {if(auto* p=find(id)){processor.applyPreset(*p);remember(id);return true;}return false;}
void PresetManager::step(int direction) {
    if(presets.empty())return;
    int selected=0;for(std::size_t i=0;i<presets.size();++i)if(presets[i].id==processor.presetId()){selected=int(i);break;}
    selected=(selected+direction+int(presets.size()))%int(presets.size());load(presets[std::size_t(selected)].id);
}
void PresetManager::stepCategory(int direction) {
    auto list=categories();if(list.isEmpty())return;
    int selected=std::max(0,list.indexOf(processor.presetCategory()));selected=(selected+direction+list.size())%list.size();
    for(const auto& p:presets)if(p.category==list[selected]){load(p.id);return;}
}
juce::Result PresetManager::write(const GbaPreset& p) {
    auto* object=new juce::DynamicObject();juce::var data(object);
    object->setProperty("format","gba2.preset");object->setProperty("schemaVersion",schemaVersion);object->setProperty("id",p.id);
    object->setProperty("name",p.name);object->setProperty("category",p.category);object->setProperty("tags",p.tags);
    auto* parameters=new juce::DynamicObject();object->setProperty("parameters",juce::var(parameters));
    for(std::size_t i=0;i<gba::parameterCount;++i)parameters->setProperty(juce::Identifier(juce::String(gba::specs[i].id.data())),p.settings.values[i]);
    return atomicJson(p.file,data);
}
juce::Result PresetManager::saveAs(const juce::String& name) {
    if(!validName(name))return juce::Result::fail("Enter a name containing 1 to 100 characters.");
    GbaPreset p;p.name=name.trim();p.category="User";p.tags="user synthesis";p.id="user."+juce::Uuid().toString();p.factory=false;
    p.settings=processor.snapshot();p.file=folder.getChildFile(p.id+".gba2preset");
    auto result=write(p);if(result.failed())return result;
    presets.push_back(p);load(p.id);return juce::Result::ok();
}
juce::Result PresetManager::renameUser(const juce::String& id,const juce::String& name) {
    if(!validName(name))return juce::Result::fail("Enter a name containing 1 to 100 characters.");
    for(auto& p:presets)if(p.id==id) {
        if(p.factory)return juce::Result::fail("Factory patches are read-only. Use Save As.");
        auto copy=p;copy.name=name.trim();auto result=write(copy);if(result.failed())return result;p=std::move(copy);
        if(processor.presetId()==id)processor.state.state.setProperty("presetName",p.name,nullptr);
        return juce::Result::ok();
    }
    return juce::Result::fail("Preset not found.");
}
juce::Result PresetManager::deleteUser(const juce::String& id) {
    for(auto i=presets.begin();i!=presets.end();++i)if(i->id==id) {
        if(i->factory)return juce::Result::fail("Factory patches cannot be deleted.");
        if(!i->file.deleteFile())return juce::Result::fail("Could not delete preset file.");
        bool active=processor.presetId()==id;presets.erase(i);favorites.erase(id);recent.removeString(id);savePreferences();if(active)restoreDefault();return juce::Result::ok();
    }
    return juce::Result::fail("Preset not found.");
}
void PresetManager::restoreDefault() {for(const auto& p:presets)if(p.name=="Dream Pad"&&p.factory){load(p.id);return;}}
bool PresetManager::isFavorite(const juce::String& id) const {return favorites.count(id)>0;}
void PresetManager::toggleFavorite(const juce::String& id) {if(!favorites.erase(id))favorites.insert(id);savePreferences();}
bool PresetManager::isRecent(const juce::String& id) const {return recent.contains(id);}
void PresetManager::remember(const juce::String& id) {recent.removeString(id);recent.insert(0,id);while(recent.size()>32)recent.remove(recent.size()-1);savePreferences();}
void PresetManager::savePreferences() {
    auto* object=new juce::DynamicObject();juce::var data(object);juce::Array<juce::var> f,r;
    for(const auto& id:favorites)f.add(id);for(const auto& id:recent)r.add(id);
    object->setProperty("favorites",f);object->setProperty("recent",r);
    auto result=atomicJson(preferences,data);if(result.failed())lastError=result.getErrorMessage();
}
