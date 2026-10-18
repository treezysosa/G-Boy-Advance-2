#include "SampleManager.h"

namespace {
gba::SampleOptions options(const juce::var& v) {
    return {juce::jlimit(0,2,int(v["mode"])),juce::jlimit(0,127,int(v.getProperty("rootNote",60))),bool(v["loop"])};
}
juce::var copyObject(const juce::var& v) {return v.clone();}
}
SampleManager::SampleManager():Thread("GBA2 sample loader") {startThread();}
SampleManager::~SampleManager(){signalThreadShouldExit();wake.signal();stopThread(-1);}
void SampleManager::loadFile(const juce::File& file){queue(file,{});}
void SampleManager::queue(juce::File file,juce::var document) {
    std::lock_guard lock(mutex);const auto id=++generation;
    desired=copyObject(document);pending=std::make_unique<Job>(Job{id,file,copyObject(document)});loading=true;message="Loading sample...";wake.signal();
}
void SampleManager::restore(const juce::var& document) {
    if(!document.isVoid()){queue({},document);return;}
    std::lock_guard lock(mutex);if(!loading&&requested.load()==nullptr)return;
    ++generation;desired=juce::var{};pending.reset();requested.store(nullptr,std::memory_order_seq_cst);loading=false;changed=true;
    message="No sample loaded. Synthesis mode.";wake.signal();
}
juce::var SampleManager::state() const {std::lock_guard lock(mutex);auto* p=requested.load();return loading&&!desired.isVoid()?copyObject(desired):(p?copyObject(p->document):juce::var());}
juce::String SampleManager::status() const {std::lock_guard lock(mutex);return message;}
void SampleManager::configure(gba::SampleOptions value) {
    std::lock_guard lock(mutex);auto* current=requested.load();if(!current||loading)return;
    auto next=std::make_unique<Published>(*current);next->document=copyObject(current->document);
    next->options={juce::jlimit(0,2,value.mode),juce::jlimit(0,127,value.root),value.loop};
    auto* object=next->document.getDynamicObject();object->setProperty("mode",next->options.mode);object->setProperty("rootNote",next->options.root);object->setProperty("loop",next->options.loop);
    auto* ptr=next.get();retained.push_back(std::move(next));requested.store(ptr,std::memory_order_seq_cst);changed=true;wake.signal();
}
void SampleManager::synchronize(gba::SynthEngine& engine) noexcept {
    const Published* p;
    do {p=requested.load(std::memory_order_seq_cst);hazard.store(p,std::memory_order_seq_cst);}while(p!=requested.load(std::memory_order_seq_cst));
    engine.setSample(p?p->audio.get():nullptr,p?p->options:gba::SampleOptions{});
}
bool SampleManager::waitUntilIdle(int milliseconds) {
    const auto end=juce::Time::getMillisecondCounterHiRes()+milliseconds;
    while(loading&&juce::Time::getMillisecondCounterHiRes()<end)juce::Thread::sleep(2);
    return !loading;
}
juce::Result SampleManager::validateDocument(const juce::var& v) {
    if(!v.isObject()||v["format"].toString()!="gba2.sample"||!v["schemaVersion"].isInt()||int(v["schemaVersion"])!=1||!v["audio"].isString()||v["audio"].toString().isEmpty()||v["audio"].toString().length()>((maxFileBytes+2)/3)*4)
        return juce::Result::fail("Invalid or oversized embedded sample.");
    if(!v["name"].isString()||v["name"].toString().length()>512||!v["mode"].isInt()||!v["rootNote"].isInt()||!v["loop"].isBool()||int(v["mode"])<0||int(v["mode"])>2||int(v["rootNote"])<0||int(v["rootNote"])>127)
        return juce::Result::fail("Invalid sample name/settings.");
    return juce::Result::ok();
}
std::unique_ptr<SampleManager::Published> SampleManager::decode(const Job& job,juce::String& error) {
    juce::MemoryBlock bytes;juce::var document;
    if(job.file!=juce::File()) {
        if(!job.file.hasFileExtension("wav;aif;aiff")||job.file.isSymbolicLink()||!job.file.existsAsFile()||job.file.getSize()>maxFileBytes){error="Choose a regular WAV/AIFF file, at most 8 MiB. Maize .mse and .chl banks are not supported.";return {};}
        juce::FileInputStream input(job.file);if(!input.openedOk()){error="Could not read the sample file.";return {};}
        input.readIntoMemoryBlock(bytes,maxFileBytes+1);if(input.getStatus().failed()||bytes.getSize()>maxFileBytes){error="Sample file exceeds 8 MiB or could not be read.";return {};}
        auto* o=new juce::DynamicObject;document=juce::var(o);o->setProperty("format","gba2.sample");o->setProperty("schemaVersion",1);o->setProperty("name",job.file.getFileName());o->setProperty("rootNote",60);o->setProperty("mode",1);o->setProperty("loop",false);o->setProperty("audio",juce::Base64::toBase64(bytes.getData(),bytes.getSize()));
    } else {
        document=copyObject(job.document);
        const auto validation=validateDocument(document);if(validation.failed()){error=validation.getErrorMessage();return {};}
        if(!document.isObject()||document["format"].toString()!="gba2.sample"||!document["schemaVersion"].isInt()||int(document["schemaVersion"])!=1||!document["audio"].isString()||document["audio"].toString().length()>((maxFileBytes+2)/3)*4){error="Invalid or oversized embedded sample.";return {};}
        for(const auto* name:{"mode","rootNote"})if(!document[name].isInt()){error="Invalid sample configuration.";return {};}
        if(!document["loop"].isBool()||int(document["mode"])<0||int(document["mode"])>2||int(document["rootNote"])<0||int(document["rootNote"])>127){error="Sample settings are outside their supported ranges.";return {};}
        juce::MemoryOutputStream stream(bytes,false);if(!juce::Base64::convertFromBase64(stream,document["audio"].toString())||bytes.getSize()>maxFileBytes){error="Invalid sample audio encoding.";return {};}
    }
    auto input=std::make_unique<juce::MemoryInputStream>(bytes,false);
    juce::AudioFormatManager formats;formats.registerFormat(new juce::WavAudioFormat,true);formats.registerFormat(new juce::AiffAudioFormat,false);
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(std::move(input)));
    if(!reader||reader->lengthInSamples<8||reader->lengthInSamples>4*1024*1024||reader->numChannels<1||reader->numChannels>2||!std::isfinite(reader->sampleRate)||reader->sampleRate<8000||reader->sampleRate>192000){error="Unsupported/corrupt audio. Use mono/stereo WAV/AIFF, 8–192 kHz, at least 8 frames, at most 4M frames.";return {};}
    const int length=int(reader->lengthInSamples);juce::AudioBuffer<float> buffer(2,length);
    if(!reader->read(&buffer,0,length,0,true,true)){error="The audio data could not be decoded.";return {};}
    auto audio=std::make_shared<gba::SampleData>();audio->rate=reader->sampleRate;gba::SampleData::Level first;first.left.resize(std::size_t(length));first.right.resize(std::size_t(length));
    for(int i=0;i<length;++i){const float l=buffer.getSample(0,i),r=buffer.getSample(reader->numChannels==1?0:1,i);if(!std::isfinite(l)||!std::isfinite(r)){error="The sample contains non-finite audio.";return {};}first.left[std::size_t(i)]=juce::jlimit(-1.f,1.f,l);first.right[std::size_t(i)]=juce::jlimit(-1.f,1.f,r);}
    audio->levels.push_back(std::move(first));audio->buildMips();
    auto result=std::make_unique<Published>();result->audio=std::move(audio);result->options=options(document);result->document=std::move(document);return result;
}
void SampleManager::reclaim() {
    const auto* keep=requested.load(std::memory_order_seq_cst);const auto* used=hazard.load(std::memory_order_seq_cst);
    retained.erase(std::remove_if(retained.begin(),retained.end(),[&](const auto& p){return p.get()!=keep&&p.get()!=used;}),retained.end());
}
void SampleManager::run() {
    while(!threadShouldExit()) {
        std::unique_ptr<Job> job;
        {std::lock_guard lock(mutex);job=std::move(pending);reclaim();}
        if(!job){wake.wait(100);continue;}
        juce::String error;std::unique_ptr<Published> result;
        try{result=decode(*job,error);}catch(const std::exception&){error="Not enough memory or invalid sample data.";}
        std::lock_guard lock(mutex);if(job->generation!=generation.load())continue;
        if(result){message=result->document["name"].toString()+" / sample ready";auto* p=result.get();retained.push_back(std::move(result));requested.store(p,std::memory_order_seq_cst);changed=true;}
        else {message=error;if(job->file==juce::File()){requested.store(nullptr,std::memory_order_seq_cst);changed=true;}}
        desired=juce::var{};loading=false;reclaim();
    }
}
