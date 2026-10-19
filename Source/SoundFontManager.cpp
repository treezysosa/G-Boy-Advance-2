#include "SoundFontManager.h"
#include <exception>
SoundFontManager::SoundFontManager(juce::File root):Thread("GBA2 SoundFont loader"),cache(root.getChildFile("SoundFonts")){retired.reserve(2);}
SoundFontManager::~SoundFontManager(){signalThreadShouldExit();notify();stopThread(-1);}
void SoundFontManager::reclaim(){auto* protectedBank=hazard.load();retired.erase(std::remove_if(retired.begin(),retired.end(),[&](const auto& p){return p.get()!=protectedBank;}),retired.end());}
void SoundFontManager::enqueue(const juce::File& file,int index,bool importing){
    {std::lock_guard lock(mutex);request=Request{file,index,importing,++generation};summary.loading=true;summary.status="Loading "+file.getFileName()+"...";if(!isThreadRunning())startThread();}
    notify();
}
void SoundFontManager::loadFile(const juce::File& file){enqueue(file,0,true);}
void SoundFontManager::select(int index){
    std::lock_guard lock(mutex);if(!current||summary.loading||index<0||index>=summary.instruments.size())return;
    current->selected.store(index);activeIndex=summary.selected=index;switchedOn=true;++changes;
}
void SoundFontManager::step(int delta,bool bank){
    const auto info=view();if(info.loading||info.instruments.isEmpty())return;
    const int count=info.instruments.size();int next=(info.selected+delta+count)%count;
    if(bank){for(int i=0;i<count&&info.banks[next]==info.banks[info.selected];++i)next=(next+delta+count)%count;}
    select(next);
}
void SoundFontManager::useSoundFont(bool enable){
    std::lock_guard lock(mutex);
    const bool changed=enable||switchedOn.load()||summary.loading;
    if(!enable){++generation;request.reset();summary.loading=false;switchedOn=false;summary.status="Synthesis selected. Choose an instrument or USE SOUNDFONT to return.";}
    else if(current){switchedOn=true;summary.status="SoundFont ready. Play MIDI or the piano keyboard.";}
    else {summary.status="Load an .sf2 SoundFont first.";return;}
    if(changed)++changes;
}
SoundFontManager::View SoundFontManager::view() const{std::lock_guard lock(mutex);auto result=summary;result.enabled=switchedOn.load();return result;}
juce::var SoundFontManager::saveState() const{
    std::lock_guard lock(mutex);if(!switchedOn.load())return {};
    auto* o=new juce::DynamicObject();o->setProperty("format","gba2.soundfont");o->setProperty("version",1);o->setProperty("path",activeFile.getFullPathName());o->setProperty("preset",activeIndex);o->setProperty("name",summary.fileName);return juce::var(o);
}
void SoundFontManager::restore(const juce::var& data){
    if(data.isVoid()){useSoundFont(false);return;}
    const bool valid=data.isObject()&&data["format"].toString()=="gba2.soundfont"&&data["version"].isInt()&&int(data["version"])==1&&data["path"].isString()&&data["path"].toString().length()<=4096&&juce::File::isAbsolutePath(data["path"].toString())&&data["preset"].isInt()&&int(data["preset"])>=0;
    {std::lock_guard lock(mutex);++generation;request.reset();published.store(nullptr);if(current)retired.push_back(std::move(current));reclaim();summary={};switchedOn=true;activeIndex=valid?int(data["preset"]):0;activeFile=valid?juce::File(data["path"].toString()):juce::File();
        summary.fileName=data["name"].isString()?data["name"].toString().substring(0,260):juce::String();
        summary.status=valid?"Restoring SoundFont...":"Invalid SoundFont project reference. Load the bank again in ROM Hacks.";}
    if(valid)enqueue(juce::File(data["path"].toString()),int(data["preset"]),false);
}
bool SoundFontManager::waitUntilIdle(int ms){auto until=juce::Time::getMillisecondCounterHiRes()+ms;while(juce::Time::getMillisecondCounterHiRes()<until){{std::lock_guard lock(mutex);if(!summary.loading)return true;}juce::Thread::sleep(1);}return false;}
void SoundFontManager::prepare(double sr){std::lock_guard lock(mutex);rate=sr;if(current)current->synth.prepare(sr);}
gba::SoundFont* SoundFontManager::beginBlock(const gba::Settings& settings) noexcept{
    Bank* bank=nullptr;
    do{bank=published.load();hazard.store(bank);}while(bank!=published.load());
    rendering=bank;if(!bank)return nullptr;
    const int selected=bank->selected.load();if(bank->playing!=selected){bank->synth.select(selected);bank->playing=selected;}
    bank->synth.setSettings(settings);return &bank->synth;
}
gba::SoundFont* SoundFontManager::audio() const noexcept{return rendering?&rendering->synth:nullptr;}
void SoundFontManager::endBlock() noexcept{rendering=nullptr;hazard.store(nullptr);}
void SoundFontManager::run(){
    while(!threadShouldExit()){
        std::optional<Request> job;bool cleanupPending=false;
        {std::lock_guard lock(mutex);job=std::move(request);request.reset();reclaim();cleanupPending=!retired.empty();}
        if(!job){wait(cleanupPending?100:-1);continue;}
        try {
        juce::String error;juce::MemoryBlock bytes;std::unique_ptr<Bank> bank;juce::File stored=job->file;
        if(!job->file.hasFileExtension("sf2")||job->file.isSymbolicLink()||!job->file.existsAsFile())error="SoundFont is missing or unsupported. Use LOAD SF2 to locate a regular .sf2 file.";
        else if(job->file.getSize()>juce::int64(gba::SoundFont::maxBytes))error="SoundFont exceeds the 128 MiB file limit.";
        else {
            juce::FileInputStream input(job->file);input.readIntoMemoryBlock(bytes,gba::SoundFont::maxBytes+1);
            if(!input.openedOk()||input.getStatus().failed()||bytes.getSize()>gba::SoundFont::maxBytes)error="Could not read the complete SoundFont.";
            else {bank=std::make_unique<Bank>();std::string detail;
                if(!bank->synth.load(bytes.getData(),bytes.getSize(),detail)){error=juce::String(detail);bank.reset();}
                else if(job->selected<0||job->selected>=int(bank->synth.instruments().size())){error="The saved SoundFont instrument is unavailable. Load the correct bank again.";bank.reset();}
                else {bank->id=job->id;bank->selected=job->selected;bank->synth.prepare(rate.load());}
            }
        }
        if(bank&&job->import){
            if(cache.createDirectory().failed())error="Could not create the SoundFonts folder in application data.";
            else {stored=cache.getChildFile(juce::Uuid().toString()+".sf2");juce::TemporaryFile temp(stored);
                {juce::FileOutputStream output(temp.getFile());if(!output.openedOk()||!output.write(bytes.getData(),bytes.getSize()))error="Could not cache the SoundFont.";output.flush();if(output.getStatus().failed())error="Could not finish writing the SoundFont cache.";}
                if(error.isEmpty()&&!temp.overwriteTargetFileWithTemporary())error="Could not finish caching the SoundFont.";
            }
        }
        View ready;
        if(bank&&error.isEmpty()){
            for(const auto& p:bank->synth.instruments()){ready.instruments.add(juce::String(p.bank)+":"+juce::String(p.program)+"  "+juce::String::createStringFromData(p.name.data(),int(p.name.size())));ready.banks.add(p.bank);}
            ready.selected=job->selected;ready.fileName=job->file.getFileName();
            ready.status="SoundFont ready. Play MIDI or the piano keyboard. Shared filter, attack, glide and effects apply.";
        }
        {std::lock_guard lock(mutex);
            if(job->id!=generation||threadShouldExit()){if(job->import&&stored!=job->file&&stored.existsAsFile())stored.deleteFile();continue;}
            summary.loading=false;
            if(error.isNotEmpty()||!bank){summary.status=error.isNotEmpty()?error:"SoundFont could not be loaded.";continue;}
            // Publication, preparation and metadata never lock the audio consumer.
            bank->synth.prepare(rate.load());
            if(!job->import&&summary.fileName.isNotEmpty())ready.fileName=summary.fileName;
            auto previous=std::move(current);current=std::move(bank);published.store(current.get());
            if(previous)retired.push_back(std::move(previous));
            summary=std::move(ready);activeIndex=job->selected;activeFile=stored;switchedOn=true;++changes;reclaim();
        }
        }catch(const std::exception&){
            std::lock_guard lock(mutex);if(job->id==generation){summary.loading=false;summary.status="SoundFont loading failed. Try a smaller or different SF2 bank.";}
        }
    }
}
