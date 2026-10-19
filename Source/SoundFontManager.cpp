#include "SoundFontManager.h"
#include <exception>
namespace {
constexpr int folderPreference=1,rememberPreference=2,bankPreference=4;
juce::CriticalSection libraryIo;
struct PreferenceLock {
    juce::ScopedLock local;
    juce::InterProcessLock process;
    bool held;
    explicit PreferenceLock(const juce::File& file):local(libraryIo),process("GBA2-sf2-library-"+juce::String::toHexString(file.getFullPathName().toLowerCase().hashCode64())),held(process.enter(250)){}
    ~PreferenceLock(){if(held)process.exit();}
};
juce::Result readLibrary(const juce::File& file,juce::var& config) {
    if(!file.exists()) {
        auto* o=new juce::DynamicObject();config=juce::var(o);o->setProperty("format","gba2.soundfont-library");o->setProperty("version",1);return juce::Result::ok();
    }
    juce::FileInputStream stream(file);juce::MemoryBlock bytes;
    if(!stream.openedOk())return juce::Result::fail("Could not read SoundFont library preferences.");
    stream.readIntoMemoryBlock(bytes,65537);
    if(stream.getStatus().failed()||bytes.getSize()>65536)return juce::Result::fail("SoundFont library preferences are unreadable or exceed 64 KiB.");
    auto result=juce::JSON::parse(juce::String::createStringFromData(bytes.getData(),int(bytes.getSize())),config);
    if(result.failed()||!config.isObject()||config["format"].toString()!="gba2.soundfont-library"||!config["version"].isInt()||int(config["version"])!=1)
        return juce::Result::fail("Invalid SoundFont library preferences. Back up or remove soundfont-library.json to reset them.");
    return juce::Result::ok();
}
bool validBankReference(const juce::var& data) {
    return data.isObject()&&data["format"].toString()=="gba2.soundfont"&&data["version"].isInt()&&int(data["version"])==1&&data["path"].isString()&&data["path"].toString().length()<=4096&&juce::File::isAbsolutePath(data["path"].toString())&&data["preset"].isInt()&&int(data["preset"])>=0;
}
}
SoundFontManager::SoundFontManager(juce::File root):Thread("GBA2 SoundFont loader"),cache(root.getChildFile("SoundFonts")),libraryPreferences(root.getChildFile("soundfont-library.json")){retired.reserve(2);library.folder=root.getChildFile("SoundFont Library");}
SoundFontManager::~SoundFontManager(){signalThreadShouldExit();notify();stopThread(-1);}
void SoundFontManager::reclaim(){auto* protectedBank=hazard.load();retired.erase(std::remove_if(retired.begin(),retired.end(),[&](const auto& p){return p.get()!=protectedBank;}),retired.end());}
void SoundFontManager::enqueue(const juce::File& file,int index,bool importing){
    {std::lock_guard lock(mutex);request=Request{file,index,importing,++generation,importing};summary.loading=true;summary.status="Loading "+file.getFileName()+"...";if(!isThreadRunning())startThread();}
    notify();
}
void SoundFontManager::loadFile(const juce::File& file){enqueue(file,0,true);}
void SoundFontManager::select(int index){
    std::lock_guard lock(mutex);if(!current||summary.loading||index<0||index>=summary.instruments.size())return;
    current->selected.store(index);activeIndex=summary.selected=index;switchedOn=true;++changes;rememberBank();
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
    else if(current){switchedOn=true;summary.status="SoundFont ready. Play MIDI or the piano keyboard.";rememberBank();}
    else {summary.status="Load an .sf2 SoundFont first.";return;}
    if(changed)++changes;
}
SoundFontManager::View SoundFontManager::view() const{std::lock_guard lock(mutex);auto result=summary;result.enabled=switchedOn.load();return result;}
juce::var SoundFontManager::saveState() const{
    std::lock_guard lock(mutex);return switchedOn.load()?reference():juce::var();
}
void SoundFontManager::restore(const juce::var& data){
    if(data.isVoid()){useSoundFont(false);return;}
    const bool valid=validBankReference(data);
    {std::lock_guard lock(mutex);++generation;request.reset();published.store(nullptr);if(current)retired.push_back(std::move(current));reclaim();summary={};library.source.clear();switchedOn=true;activeIndex=valid?int(data["preset"]):0;activeFile=valid?juce::File(data["path"].toString()):juce::File();
        summary.fileName=data["name"].isString()?data["name"].toString().substring(0,260):juce::String();
        summary.status=valid?"Restoring SoundFont...":"Invalid SoundFont project reference. Load the bank again in ROM Hacks.";}
    if(valid)enqueue(juce::File(data["path"].toString()),int(data["preset"]),false);
}
bool SoundFontManager::waitUntilIdle(int ms){auto until=juce::Time::getMillisecondCounterHiRes()+ms;while(juce::Time::getMillisecondCounterHiRes()<until){{std::lock_guard lock(mutex);if(!summary.loading&&!initializePending&&!scanPending&&!dirtyPreferences&&!workerBusy)return true;}juce::Thread::sleep(1);}return false;}
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
void SoundFontManager::loadBank(const Request& item){
        const auto* job=&item;
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
            if(job->id!=generation||threadShouldExit()){if(job->import&&stored!=job->file&&stored.existsAsFile())stored.deleteFile();return;}
            summary.loading=false;
            if(error.isNotEmpty()||!bank){summary.status=error.isNotEmpty()?error:"SoundFont could not be loaded.";return;}
            // Publication, preparation and metadata never lock the audio consumer.
            bank->synth.prepare(rate.load());
            if(!job->import&&summary.fileName.isNotEmpty())ready.fileName=summary.fileName;
            auto previous=std::move(current);current=std::move(bank);published.store(current.get());
            if(previous)retired.push_back(std::move(previous));
            summary=std::move(ready);activeIndex=job->selected;activeFile=stored;switchedOn=true;++changes;reclaim();
            if(job->import)library.source=job->file.getFullPathName();
            if(job->remember)rememberBank();
        }
        }catch(const std::exception&){
            std::lock_guard lock(mutex);if(job->id==generation){summary.loading=false;summary.status="SoundFont loading failed. Try a smaller or different SF2 bank.";}
        }
}

void SoundFontManager::armStartupRecall(){std::lock_guard lock(mutex);startupGeneration=generation;}
void SoundFontManager::startLibrary(){
    {std::lock_guard lock(mutex);if(libraryStarted)return;libraryStarted=true;initializePending=true;library.scanning=true;if(!isThreadRunning())startThread();}
    notify();
}
void SoundFontManager::refreshLibrary(){startLibrary();{std::lock_guard lock(mutex);scanPending=true;library.scanning=true;}notify();}
void SoundFontManager::setLibraryFolder(const juce::File& folder){
    startLibrary();{std::lock_guard lock(mutex);library.folder=folder;++folderRevision;scanPending=true;library.scanning=true;dirtyPreferences|=folderPreference;}notify();
}
void SoundFontManager::setRememberLast(bool enabled){
    startLibrary();{std::lock_guard lock(mutex);library.rememberLast=enabled;dirtyPreferences|=rememberPreference;}notify();
}
SoundFontManager::LibraryView SoundFontManager::libraryView() const {std::lock_guard lock(mutex);return library;}
juce::var SoundFontManager::reference() const {
    auto* o=new juce::DynamicObject();o->setProperty("format","gba2.soundfont");o->setProperty("version",1);o->setProperty("path",activeFile.getFullPathName());o->setProperty("preset",activeIndex);o->setProperty("name",summary.fileName);return juce::var(o);
}
void SoundFontManager::rememberBank(){
    if(!current||activeFile==juce::File())return;
    lastBank=reference();lastBank.getDynamicObject()->setProperty("source",library.source);dirtyPreferences|=bankPreference;notify();
}
void SoundFontManager::initializeLibrary(){
    juce::var config;juce::Result result=juce::Result::ok();
    {PreferenceLock lock(libraryPreferences);result=lock.held?readLibrary(libraryPreferences,config):juce::Result::fail("SoundFont library preferences are busy. Current bank and folder remain available.");}
    std::lock_guard lock(mutex);
    if(result.failed())library.status=result.getErrorMessage();
    else {
        const auto folder=config["folder"].toString();
        if(folderRevision==0&&folder.length()<=4096&&juce::File::isAbsolutePath(folder))library.folder=juce::File(folder);
        if(!(dirtyPreferences&rememberPreference))library.rememberLast=config["rememberLast"].isBool()?bool(config["rememberLast"]):true;
        if(!(dirtyPreferences&bankPreference))lastBank=config["lastBank"];
        if(library.rememberLast&&validBankReference(lastBank)&&generation==startupGeneration) {
            activeFile=juce::File(lastBank["path"].toString());activeIndex=int(lastBank["preset"]);
            summary.fileName=lastBank["name"].toString().substring(0,260);summary.loading=true;summary.status="Restoring your last SoundFont...";switchedOn=true;
            library.source=lastBank["source"].toString();request=Request{activeFile,activeIndex,false,++generation,false};
        }
    }
    scanPending=true;
}
void SoundFontManager::scanLibrary(){
    juce::File folder;std::uint64_t revision=0;
    {std::lock_guard lock(mutex);folder=library.folder;revision=folderRevision;}
    juce::StringArray files,names;juce::String status;
    const auto defaultFolder=libraryPreferences.getParentDirectory().getChildFile("SoundFont Library");
    if(folder==defaultFolder&&folder.createDirectory().failed())status="Could not create the default SoundFont folder. Choose another folder.";
    else if(!folder.isDirectory())status="SoundFont folder is unavailable. Use CHOOSE FOLDER or restore that folder.";
    else {
        int visited=0;
        for(const auto& entry:juce::RangedDirectoryIterator(folder,false,"*",juce::File::findFiles)) {
            if(threadShouldExit())break;
            if(++visited>4096||files.size()>=512){status="Folder list limited to 512 banks / 4096 entries. Use a smaller folder or LOAD SF2.";break;}
            const auto file=entry.getFile();if(file.hasFileExtension("sf2")&&!file.isSymbolicLink())files.add(file.getFullPathName());
        }
        files.sortNatural();for(const auto& path:files)names.add(juce::File(path).getFileName());
        if(status.isEmpty())status=files.isEmpty()?"Put .sf2 files in this folder, then press REFRESH.":juce::String(files.size())+" SoundFont banks. Choose one above; REFRESH finds added or removed files.";
    }
    std::lock_guard lock(mutex);
    if(revision==folderRevision){library.files=std::move(files);library.names=std::move(names);library.scanning=false;if(library.status.isEmpty()||!library.status.contains("preferences"))library.status=status;}
}
void SoundFontManager::writeLibraryPreferences(){
    int dirty=0;juce::File folder;bool remember=true;juce::var bank;
    {std::lock_guard lock(mutex);dirty=dirtyPreferences;dirtyPreferences=0;folder=library.folder;remember=library.rememberLast;bank=lastBank;}
    if(!dirty)return;
    juce::Result result=juce::Result::ok();
    {
        PreferenceLock lock(libraryPreferences);juce::var config;
        if(!lock.held)result=juce::Result::fail("SoundFont library preferences are busy; this change could not be saved.");
        else if(libraryPreferences.isSymbolicLink())result=juce::Result::fail("SoundFont library preferences must be a regular file.");
        else result=readLibrary(libraryPreferences,config);
        if(result.wasOk()) {
            auto* o=config.getDynamicObject();
            if(dirty&folderPreference)o->setProperty("folder",folder.getFullPathName());
            if(dirty&rememberPreference)o->setProperty("rememberLast",remember);
            if(dirty&bankPreference)o->setProperty("lastBank",bank);
            if(libraryPreferences.getParentDirectory().createDirectory().failed())result=juce::Result::fail("Could not create SoundFont library preferences.");
            else {juce::TemporaryFile temporary(libraryPreferences);
                if(!temporary.getFile().replaceWithText(juce::JSON::toString(config))||!temporary.overwriteTargetFileWithTemporary())result=juce::Result::fail("Could not save SoundFont library preferences. Current playback is unchanged.");}
        }
    }
    if(result.failed()){std::lock_guard lock(mutex);library.status=result.getErrorMessage();}
}
void SoundFontManager::run(){
    while(!threadShouldExit()){
        std::optional<Request> job;bool initialize=false,scan=false,cleanup=false,work=false;
        {std::lock_guard lock(mutex);job=std::move(request);request.reset();initialize=initializePending;initializePending=false;scan=scanPending;scanPending=false;reclaim();cleanup=!retired.empty();work=bool(job)||initialize||scan||dirtyPreferences!=0;workerBusy=work;}
        if(!work){wait(cleanup?100:-1);continue;}
        try {
            if(initialize)initializeLibrary();
            {std::lock_guard lock(mutex);scan=scan||scanPending;scanPending=false;}
            if(scan)scanLibrary();
            if(job)loadBank(*job);
            writeLibraryPreferences();
        }catch(const std::exception&){std::lock_guard lock(mutex);library.scanning=false;library.status="SoundFont library operation failed. Current playback is unchanged.";}
        {std::lock_guard lock(mutex);workerBusy=false;}
    }
    // Shutdown can arrive immediately after an instrument/folder/recall click.
    // Persist the last successful choice on this worker before it exits.
    try {writeLibraryPreferences();}catch(const std::exception&){}
}
